/** @file
  A Setup Menu for configuring boot options defined by bootloader CFR.
  This file parses CFR to produce HII IFR.

  Copyright (c) 2023, 9elements GmbH.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SetupMenu.h"
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CfrHelpersLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/VariablePolicyHelperLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Guid/CfrSetupMenuGuid.h>
#include <Guid/VariableFormat.h>

/**
  CFR_VARBINARY records are used as option name and UI name and help text.
  Convert one to formats used for EDK2 HII.

  Caller is responsible for freeing the UnicodeString pool.

**/
STATIC
VOID
EFIAPI
CfrConvertVarBinaryToStrings (
  IN     CFR_VARBINARY  *CfrString,
  IN OUT CHAR16         **UnicodeString,
  IN OUT EFI_STRING_ID  *HiiStringId      OPTIONAL
  )
{
  EFI_STATUS  Status;

  ASSERT ((CfrString != NULL) && (UnicodeString != NULL));
  if ((CfrString == NULL) || (UnicodeString == NULL)) {
    return;
  }

  *UnicodeString = AllocatePool (CfrString->data_length * sizeof (CHAR16));
  ASSERT (*UnicodeString != NULL);
  Status = AsciiStrToUnicodeStrS (
             (CHAR8 *)CfrString->data,
             *UnicodeString,
             CfrString->data_length
             );
  ASSERT_EFI_ERROR (Status);

  if (HiiStringId != NULL) {
    *HiiStringId = HiiSetString (
                     mSetupMenuPrivate.HiiHandle,
                     0,
                     *UnicodeString,
                     NULL
                     );
    ASSERT (*HiiStringId != 0);
  }
}

/**
  Produce unconditional HII `*_IF` for CFR flags.

  Caller to close each `*_IF` with `HiiCreateEndOpCode()`.

**/
STATIC
VOID
EFIAPI
CfrProduceHiiForFlags (
  IN VOID   *StartOpCodeHandle,
  IN UINT8  OpCode
  )
{
  EFI_IFR_OP_HEADER  IfOpHeader;
  UINT8              *TempHiiBuffer;
  EFI_IFR_OP_HEADER  ConditionTrue;

  if ((OpCode != EFI_IFR_SUPPRESS_IF_OP) && (OpCode != EFI_IFR_GRAY_OUT_IF_OP)) {
    return;
  }

  IfOpHeader.OpCode = OpCode;
  IfOpHeader.Length = sizeof (EFI_IFR_OP_HEADER);
  // `if` statements are new scopes
  IfOpHeader.Scope = 1;

  TempHiiBuffer = HiiCreateRawOpCodes (
                    StartOpCodeHandle,
                    (UINT8 *)&IfOpHeader,
                    sizeof (EFI_IFR_OP_HEADER)
                    );
  ASSERT (TempHiiBuffer != NULL);

  ConditionTrue.OpCode = EFI_IFR_TRUE_OP;
  ConditionTrue.Length = sizeof (EFI_IFR_OP_HEADER);
  // Same scope as above statement
  ConditionTrue.Scope = 0;

  TempHiiBuffer = HiiCreateRawOpCodes (
                    StartOpCodeHandle,
                    (UINT8 *)&ConditionTrue,
                    sizeof (EFI_IFR_OP_HEADER)
                    );
  ASSERT (TempHiiBuffer != NULL);
}

/**
  Produce variable and VARSTORE for CFR option name.

**/
STATIC
VOID
EFIAPI
CfrProduceStorageForOption (
  IN CFR_VARBINARY  *CfrOptionName,
  IN VOID           *CfrOptionDefaultValue,
  IN UINTN          CfrOptionLength,
  IN UINT8          OptionFlags,
  IN VOID           *StartOpCodeHandle,
  IN UINTN          QuestionIdVarStoreId
  )
{
  CHAR16            *VariableCfrName;
  UINT32            VariableAttributes;
  UINTN             DataSize;
  EFI_STATUS        Status;
  UINTN             OptionNameLength;
  UINTN             VarStoreStructSize;
  EFI_IFR_VARSTORE  *VarStore;
  UINT8             *TempHiiBuffer;

  //
  // Initialise defaults for VARSTORE variable
  //
  CfrConvertVarBinaryToStrings (CfrOptionName, &VariableCfrName, NULL);

  //
  // Variables can be runtime accessible later, if desired
  //
  VariableAttributes = EFI_VARIABLE_BOOTSERVICE_ACCESS;
  if (!(OptionFlags & CFR_OPTFLAG_VOLATILE)) {
    VariableAttributes |= EFI_VARIABLE_NON_VOLATILE;
  }

  DataSize = 0;
  Status = gRT->GetVariable (
                  VariableCfrName,
                  &gEficorebootNvDataGuid,
                  NULL,
                  &DataSize,
                  NULL
                  );
  if (Status == EFI_NOT_FOUND) {
    DataSize = CfrOptionLength;
    Status = gRT->SetVariable (
                    VariableCfrName,
                    &gEficorebootNvDataGuid,
                    VariableAttributes,
                    DataSize,
                    CfrOptionDefaultValue
                    );
    ASSERT_EFI_ERROR (Status);
  }

  if (OptionFlags & CFR_OPTFLAG_READONLY && mVariablePolicy != NULL) {
    Status = RegisterBasicVariablePolicy (
               mVariablePolicy,
               &gEficorebootNvDataGuid,
               VariableCfrName,
               VARIABLE_POLICY_NO_MIN_SIZE,
               VARIABLE_POLICY_NO_MAX_SIZE,
               VARIABLE_POLICY_NO_MUST_ATTR,
               VARIABLE_POLICY_NO_CANT_ATTR,
               VARIABLE_POLICY_TYPE_LOCK_NOW
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "CFR: Failed to lock variable \"%s\"!\n", VariableCfrName));
    }
  }

  FreePool (VariableCfrName);

  //
  // Build a `varstore` and copy it as raw HII opcodes. Then free this
  //
  /* Struct contains space for terminator only, allocate with name too */
  OptionNameLength = AsciiStrLen ((CHAR8 *)CfrOptionName->data);
  VarStoreStructSize = sizeof (EFI_IFR_VARSTORE) + OptionNameLength;
  ASSERT (VarStoreStructSize <= 0x7F);
  if (VarStoreStructSize > 0x7F) {
    DEBUG ((DEBUG_ERROR, "CFR: Option name length 0x%x is too long!\n", OptionNameLength));
    return;
  }

  VarStore = AllocateZeroPool (VarStoreStructSize);
  ASSERT (VarStore != NULL);
  if (VarStore == NULL) {
    DEBUG ((DEBUG_ERROR, "CFR: Failed to allocate memory for varstore!\n"));
    return;
  }

  VarStore->Header.OpCode = EFI_IFR_VARSTORE_OP;
  VarStore->Header.Length = VarStoreStructSize;

  /* Direct mapping */
  VarStore->VarStoreId = QuestionIdVarStoreId;
  VarStore->Size = CfrOptionLength;

  CopyMem (&VarStore->Guid, &gEficorebootNvDataGuid, sizeof (EFI_GUID));
  CopyMem (VarStore->Name, CfrOptionName->data, CfrOptionName->data_length);

  TempHiiBuffer = HiiCreateRawOpCodes (
                    StartOpCodeHandle,
                    (UINT8 *)VarStore,
                    VarStoreStructSize
                    );
  ASSERT (TempHiiBuffer != NULL);
  FreePool (VarStore);
}

/**
  Process one CFR form - its UI name - and create HII component.
  Therefore, *do not* advanced index by the size field.

  It's currently too difficult to produce form HII IFR, because these
  seem unable to be nested, so generating the VfrBin at runtime would be required.
  However, maybe we'll look into that, or HII "scopes" later.

**/
STATIC
VOID
EFIAPI
CfrProcessFormOption (
  IN     CFR_OPTION_FORM  *Option,
  IN     VOID             *StartOpCodeHandle,
  IN OUT UINTN            *ProcessedLength
  )
{
  CFR_VARBINARY       *CfrFormName;
  CHAR16              *HiiFormNameString;
  EFI_STRING_ID       HiiFormNameStringId;
  UINT8               *TempHiiBuffer;

  //
  // Extract variable-length fields that follow the header
  //
  *ProcessedLength += sizeof (CFR_OPTION_FORM);
  CfrFormName = CfrExtractVarBinary ((UINT8 *)Option, ProcessedLength, CB_TAG_CFR_VARCHAR_UI_NAME);
  ASSERT (CfrFormName != NULL);

  DEBUG ((
    DEBUG_INFO,
    "CFR: Process form[%d] \"%a\" of size 0x%x\n",
    Option->object_id,
    CfrFormName->data,
    Option->size
    ));

  CfrConvertVarBinaryToStrings (CfrFormName, &HiiFormNameString, &HiiFormNameStringId);
  FreePool (HiiFormNameString);

  if (Option->flags & CFR_OPTFLAG_SUPPRESS) {
    CfrProduceHiiForFlags (StartOpCodeHandle, EFI_IFR_SUPPRESS_IF_OP);
  }
  if (Option->flags & CFR_OPTFLAG_GRAYOUT) {
    CfrProduceHiiForFlags (StartOpCodeHandle, EFI_IFR_GRAY_OUT_IF_OP);
  }

  TempHiiBuffer = HiiCreateSubTitleOpCode (
                    StartOpCodeHandle,
                    HiiFormNameStringId,
                    STRING_TOKEN (STR_EMPTY_STRING),
                    0,
                    0
                    );
  ASSERT (TempHiiBuffer != NULL);

  if (Option->flags & CFR_OPTFLAG_GRAYOUT) {
    TempHiiBuffer = HiiCreateEndOpCode (StartOpCodeHandle);
    ASSERT (TempHiiBuffer != NULL);
  }
  if (Option->flags & CFR_OPTFLAG_SUPPRESS) {
    TempHiiBuffer = HiiCreateEndOpCode (StartOpCodeHandle);
    ASSERT (TempHiiBuffer != NULL);
  }
}

/**
  Process one CFR numeric option and create HII component.

**/
STATIC
VOID
EFIAPI
CfrProcessNumericOption (
  IN     CFR_OPTION_NUMERIC  *Option,
  IN     VOID                *StartOpCodeHandle,
  IN OUT UINTN               *ProcessedLength
  )
{
  UINTN           OptionProcessedLength;
  CFR_VARBINARY   *CfrOptionName;
  CFR_VARBINARY   *CfrDisplayName;
  CFR_VARBINARY   *CfrHelpText;
  UINTN           QuestionIdVarStoreId;
  UINT8           QuestionFlags;
  VOID            *DefaultOpCodeHandle;
  UINT8           *TempHiiBuffer;
  CHAR16          *HiiDisplayString;
  EFI_STRING_ID   HiiDisplayStringId;
  CHAR16          *HiiHelpText;
  EFI_STRING_ID   HiiHelpTextId;
  VOID            *OptionOpCodeHandle;
  CFR_ENUM_VALUE  *CfrEnumValues;
  CFR_VARBINARY   *CfrEnumUiString;
  CHAR16          *HiiEnumStrings;
  EFI_STRING_ID   HiiEnumStringsId;

  //
  // Extract variable-length fields that follow the header
  //
  OptionProcessedLength = sizeof (CFR_OPTION_NUMERIC);

  CfrOptionName = CfrExtractVarBinary ((UINT8 *)Option, &OptionProcessedLength, CB_TAG_CFR_VARCHAR_OPT_NAME);
  ASSERT (CfrOptionName != NULL);
  CfrDisplayName = CfrExtractVarBinary ((UINT8 *)Option, &OptionProcessedLength, CB_TAG_CFR_VARCHAR_UI_NAME);
  ASSERT (CfrDisplayName != NULL);

  // Help text is optional
  CfrHelpText = CfrExtractVarBinary ((UINT8 *)Option, &OptionProcessedLength, CB_TAG_CFR_VARCHAR_UI_HELPTEXT);
  if (CfrHelpText != NULL) {
    ASSERT (CfrHelpText->tag == CB_TAG_CFR_VARCHAR_UI_HELPTEXT);
  }

  DEBUG ((
    DEBUG_INFO,
    "CFR: Process option[%d] \"%a\" of size 0x%x\n",
    Option->object_id,
    CfrOptionName->data,
    Option->size
    ));

  //
  // Processing start
  //
  QuestionIdVarStoreId = CFR_COMPONENT_START + Option->object_id;
  CfrProduceStorageForOption (
    CfrOptionName,
    &Option->default_value,
    sizeof (Option->default_value),
    Option->flags,
    StartOpCodeHandle,
    QuestionIdVarStoreId
    );

  QuestionFlags = EFI_IFR_FLAG_RESET_REQUIRED;
  if (Option->flags & CFR_OPTFLAG_READONLY) {
    QuestionFlags |= EFI_IFR_FLAG_READ_ONLY;
  }

  if (Option->flags & CFR_OPTFLAG_SUPPRESS) {
    CfrProduceHiiForFlags (StartOpCodeHandle, EFI_IFR_SUPPRESS_IF_OP);
  }
  if (Option->flags & CFR_OPTFLAG_GRAYOUT) {
    CfrProduceHiiForFlags (StartOpCodeHandle, EFI_IFR_GRAY_OUT_IF_OP);
  }

  DefaultOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (DefaultOpCodeHandle != NULL);

  TempHiiBuffer = HiiCreateDefaultOpCode (
                    DefaultOpCodeHandle,
                    EFI_HII_DEFAULT_CLASS_STANDARD,
                    EFI_IFR_TYPE_NUM_SIZE_32,
                    Option->default_value
                    );
  ASSERT (TempHiiBuffer != NULL);

  CfrConvertVarBinaryToStrings (CfrDisplayName, &HiiDisplayString, &HiiDisplayStringId);
  FreePool (HiiDisplayString);

  if (CfrHelpText != NULL) {
    CfrConvertVarBinaryToStrings (CfrHelpText, &HiiHelpText, &HiiHelpTextId);
    FreePool (HiiHelpText);
  } else {
    HiiHelpTextId = STRING_TOKEN (STR_EMPTY_STRING);
  }

  //
  // Create HII opcodes, processing complete.
  //
  OptionOpCodeHandle = NULL;
  if (Option->tag == CB_TAG_CFR_OPTION_ENUM) {
    OptionOpCodeHandle = HiiAllocateOpCodeHandle ();
    ASSERT (OptionOpCodeHandle != NULL);

    while (OptionProcessedLength < Option->size) {
      CfrEnumValues = (CFR_ENUM_VALUE *)((UINT8 *)Option + OptionProcessedLength);
      ASSERT (CfrEnumValues->tag == CB_TAG_CFR_ENUM_VALUE);

      CfrEnumUiString = (CFR_VARBINARY *)((UINT8 *)CfrEnumValues + sizeof (CFR_ENUM_VALUE));
      ASSERT (CfrEnumUiString->tag == CB_TAG_CFR_VARCHAR_UI_NAME);
      CfrConvertVarBinaryToStrings (CfrEnumUiString, &HiiEnumStrings, &HiiEnumStringsId);
      FreePool (HiiEnumStrings);

      TempHiiBuffer = HiiCreateOneOfOptionOpCode (
                        OptionOpCodeHandle,
                        HiiEnumStringsId,
                        0,
                        EFI_IFR_TYPE_NUM_SIZE_32,
                        CfrEnumValues->value
                        );
      ASSERT (TempHiiBuffer != NULL);

      OptionProcessedLength += CfrEnumValues->size;
    }

    TempHiiBuffer = HiiCreateOneOfOpCode (
                      StartOpCodeHandle,
                      QuestionIdVarStoreId,
                      QuestionIdVarStoreId,
                      0x0,
                      HiiDisplayStringId,
                      HiiHelpTextId,
                      QuestionFlags,
                      EFI_IFR_NUMERIC_SIZE_4,
                      OptionOpCodeHandle,
                      DefaultOpCodeHandle
                      );
    ASSERT (TempHiiBuffer != NULL);
  } else if (Option->tag == CB_TAG_CFR_OPTION_NUMBER) {
    TempHiiBuffer = HiiCreateNumericOpCode (
                      StartOpCodeHandle,
                      QuestionIdVarStoreId,
                      QuestionIdVarStoreId,
                      0x0,
                      HiiDisplayStringId,
                      HiiHelpTextId,
                      QuestionFlags,
                      EFI_IFR_NUMERIC_SIZE_4 | EFI_IFR_DISPLAY_UINT_DEC,
                      0x00000000,
                      0xFFFFFFFF,
                      0,
                      DefaultOpCodeHandle
                      );
    ASSERT (TempHiiBuffer != NULL);
  } else if (Option->tag == CB_TAG_CFR_OPTION_BOOL) {
    // TODO: Or use ONE_OF instead?
    TempHiiBuffer = HiiCreateCheckBoxOpCode (
                      StartOpCodeHandle,
                      QuestionIdVarStoreId,
                      QuestionIdVarStoreId,
                      0x0,
                      HiiDisplayStringId,
                      HiiHelpTextId,
                      QuestionFlags,
                      0,
                      DefaultOpCodeHandle
                      );
    ASSERT (TempHiiBuffer != NULL);
  }

  if (Option->flags & CFR_OPTFLAG_GRAYOUT) {
    TempHiiBuffer = HiiCreateEndOpCode (StartOpCodeHandle);
    ASSERT (TempHiiBuffer != NULL);
  }
  if (Option->flags & CFR_OPTFLAG_SUPPRESS) {
    TempHiiBuffer = HiiCreateEndOpCode (StartOpCodeHandle);
    ASSERT (TempHiiBuffer != NULL);
  }

  if (OptionOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (OptionOpCodeHandle);
  }
  HiiFreeOpCodeHandle (DefaultOpCodeHandle);

  ASSERT (OptionProcessedLength == Option->size);
  *ProcessedLength += Option->size;
}

/**
  Process one CFR character option and create HII component.

**/
STATIC
VOID
EFIAPI
CfrProcessCharacterOption (
  IN     CFR_OPTION_VARCHAR  *Option,
  IN     VOID                *StartOpCodeHandle,
  IN OUT UINTN               *ProcessedLength
  )
{
  UINTN           OptionProcessedLength;
  CFR_VARBINARY   *CfrOptionName;
  CFR_VARBINARY   *CfrDefaultValue;
  CFR_VARBINARY   *CfrDisplayName;
  CFR_VARBINARY   *CfrHelpText;
  UINTN           QuestionIdVarStoreId;
  CHAR16          *HiiDefaultValue;
  EFI_STRING_ID   HiiDefaultValueId;
  UINTN           HiiDefaultValueLengthChars;
  CHAR16          *HiiDisplayString;
  EFI_STRING_ID   HiiDisplayStringId;
  CHAR16          *HiiHelpText;
  EFI_STRING_ID   HiiHelpTextId;
  UINT8           QuestionFlags;
  VOID            *DefaultOpCodeHandle;
  UINT8           *TempHiiBuffer;

  //
  // Extract variable-length fields that follow the header
  //
  ASSERT (sizeof (CFR_OPTION_VARCHAR) == sizeof (CFR_OPTION_COMMENT));
  OptionProcessedLength = sizeof (CFR_OPTION_VARCHAR);

  // Only true string options have variables
  CfrOptionName = NULL;
  if (Option->tag == CB_TAG_CFR_OPTION_VARCHAR) {
    CfrDefaultValue = CfrExtractVarBinary ((UINT8 *)Option, &OptionProcessedLength, CB_TAG_CFR_VARCHAR_DEF_VALUE);
    ASSERT (CfrDefaultValue != NULL);

    CfrOptionName = CfrExtractVarBinary ((UINT8 *)Option, &OptionProcessedLength, CB_TAG_CFR_VARCHAR_OPT_NAME);
    ASSERT (CfrOptionName != NULL);
  }

  CfrDisplayName = CfrExtractVarBinary ((UINT8 *)Option, &OptionProcessedLength, CB_TAG_CFR_VARCHAR_UI_NAME);
  ASSERT (CfrDisplayName != NULL);

  // Help text is optional
  CfrHelpText = CfrExtractVarBinary ((UINT8 *)Option, &OptionProcessedLength, CB_TAG_CFR_VARCHAR_UI_HELPTEXT);
  if (CfrHelpText != NULL) {
    ASSERT (CfrHelpText->tag == CB_TAG_CFR_VARCHAR_UI_HELPTEXT);
  }

  DEBUG ((
    DEBUG_INFO,
    "CFR: Process option[%d] \"%a\" of size 0x%x\n",
    Option->object_id,
    (CfrOptionName != NULL) ? CfrOptionName->data : CfrDisplayName->data,
    Option->size
    ));

  //
  // Processing start
  //
  if (Option->tag == CB_TAG_CFR_OPTION_VARCHAR) {
    if (CfrDefaultValue->data_length > 0xFF) {
      DEBUG ((DEBUG_ERROR, "CFR: Default value length 0x%x is too long!\n", CfrDefaultValue->data_length));
      *ProcessedLength += Option->size;
      return;
    }

    QuestionIdVarStoreId = CFR_COMPONENT_START + Option->object_id;

    if (CfrDefaultValue->data_length > 1) {
      CfrConvertVarBinaryToStrings (CfrDefaultValue, &HiiDefaultValue, &HiiDefaultValueId);
      HiiDefaultValueLengthChars = CfrDefaultValue->data_length;
    } else {
      HiiDefaultValue = HiiGetString (mSetupMenuPrivate.HiiHandle, STRING_TOKEN (STR_INVALID_STRING), NULL);
      HiiDefaultValueId = STRING_TOKEN (STR_INVALID_STRING);
      HiiDefaultValueLengthChars = StrLen (HiiDefaultValue) + 1;
    }

    CfrProduceStorageForOption (
      CfrOptionName,
      HiiDefaultValue,
      HiiDefaultValueLengthChars * sizeof (CHAR16),
      Option->flags,
      StartOpCodeHandle,
      QuestionIdVarStoreId
      );

    if (HiiDefaultValue != NULL) {
      FreePool (HiiDefaultValue);
    }
  }

  if (Option->flags & CFR_OPTFLAG_SUPPRESS) {
    CfrProduceHiiForFlags (StartOpCodeHandle, EFI_IFR_SUPPRESS_IF_OP);
  }
  if (Option->flags & CFR_OPTFLAG_GRAYOUT) {
    CfrProduceHiiForFlags (StartOpCodeHandle, EFI_IFR_GRAY_OUT_IF_OP);
  }

  CfrConvertVarBinaryToStrings (CfrDisplayName, &HiiDisplayString, &HiiDisplayStringId);
  FreePool (HiiDisplayString);

  if (CfrHelpText != NULL) {
    CfrConvertVarBinaryToStrings (CfrHelpText, &HiiHelpText, &HiiHelpTextId);
    FreePool (HiiHelpText);
  } else {
    HiiHelpTextId = STRING_TOKEN (STR_EMPTY_STRING);
  }

  //
  // Create HII opcodes, processing complete.
  //
  if (Option->tag == CB_TAG_CFR_OPTION_VARCHAR) {
    QuestionFlags = EFI_IFR_FLAG_RESET_REQUIRED;
    if (Option->flags & CFR_OPTFLAG_READONLY) {
      QuestionFlags |= EFI_IFR_FLAG_READ_ONLY;
    }

    DefaultOpCodeHandle = HiiAllocateOpCodeHandle ();
    ASSERT (DefaultOpCodeHandle != NULL);

    TempHiiBuffer = HiiCreateDefaultOpCode (
                      DefaultOpCodeHandle,
                      EFI_HII_DEFAULT_CLASS_STANDARD,
                      EFI_IFR_TYPE_NUM_SIZE_16,
                      HiiDefaultValueId
                      );
    ASSERT (TempHiiBuffer != NULL);

    // TODO: User can adjust length of string?
    TempHiiBuffer = HiiCreateStringOpCode (
                      StartOpCodeHandle,
                      QuestionIdVarStoreId,
                      QuestionIdVarStoreId,
                      0x0,
                      HiiDisplayStringId,
                      HiiHelpTextId,
                      QuestionFlags,
                      0,
                      HiiDefaultValueLengthChars - 1,
                      HiiDefaultValueLengthChars - 1,
                      DefaultOpCodeHandle
                      );
    ASSERT (TempHiiBuffer != NULL);

    HiiFreeOpCodeHandle (DefaultOpCodeHandle);
  } else if (Option->tag == CB_TAG_CFR_OPTION_COMMENT) {
    TempHiiBuffer = HiiCreateTextOpCode (
                      StartOpCodeHandle,
                      HiiDisplayStringId,
                      HiiHelpTextId,
                      STRING_TOKEN (STR_EMPTY_STRING)
                      );
    ASSERT (TempHiiBuffer != NULL);
  }

  if (Option->flags & CFR_OPTFLAG_GRAYOUT) {
    TempHiiBuffer = HiiCreateEndOpCode (StartOpCodeHandle);
    ASSERT (TempHiiBuffer != NULL);
  }
  if (Option->flags & CFR_OPTFLAG_SUPPRESS) {
    TempHiiBuffer = HiiCreateEndOpCode (StartOpCodeHandle);
    ASSERT (TempHiiBuffer != NULL);
  }

  ASSERT (OptionProcessedLength == Option->size);
  *ProcessedLength += Option->size;
}

/**
  Create runtime components by iterating CFR forms.

**/
VOID
EFIAPI
CfrCreateRuntimeComponents (
  VOID
  )
{
  VOID                *StartOpCodeHandle;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartLabel;
  EFI_IFR_GUID_LABEL  *EndLabel;
  EFI_HOB_GUID_TYPE   *GuidHob;
  CFR_OPTION_FORM     *CfrFormHob;
  UINTN               ProcessedLength;
  CFR_OPTION_FORM     *CfrFormData;
  EFI_STATUS          Status;

  //
  // Allocate GUIDed markers at runtime component offset in IFR
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  StartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                       StartOpCodeHandle,
                                       &gEfiIfrTianoGuid,
                                       NULL,
                                       sizeof (EFI_IFR_GUID_LABEL)
                                       );
  ASSERT (StartLabel != NULL);

  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_RT_COMP_START;

  EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                       EndOpCodeHandle,
                                       &gEfiIfrTianoGuid,
                                       NULL,
                                       sizeof (EFI_IFR_GUID_LABEL)
                                       );
  ASSERT (EndLabel != NULL);

  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_RT_COMP_END;

  //
  // For each HOB, create forms
  //
  GuidHob = GetFirstGuidHob (&gEfiCfrSetupMenuFormGuid);
  while (GuidHob != NULL) {
    CfrFormHob = GET_GUID_HOB_DATA (GuidHob);

    ProcessedLength = 0;
    CfrProcessFormOption (
      CfrFormHob,
      StartOpCodeHandle,
      &ProcessedLength
      );

    //
    // Process form tree
    //
    while (ProcessedLength < CfrFormHob->size) {
      CfrFormData = (CFR_OPTION_FORM *)((UINT8 *)CfrFormHob + ProcessedLength);

      switch (CfrFormData->tag) {
        case CB_TAG_CFR_OPTION_FORM:
          DEBUG ((DEBUG_INFO, "CFR: Nested form, will produce subtitle\n"));
          CfrProcessFormOption (
            (CFR_OPTION_FORM *)CfrFormData,
            StartOpCodeHandle,
            &ProcessedLength
            );
          break;
        case CB_TAG_CFR_OPTION_ENUM:
        case CB_TAG_CFR_OPTION_NUMBER:
        case CB_TAG_CFR_OPTION_BOOL:
          CfrProcessNumericOption (
            (CFR_OPTION_NUMERIC *)CfrFormData,
            StartOpCodeHandle,
            &ProcessedLength
            );
          break;
        case CB_TAG_CFR_OPTION_VARCHAR:
        case CB_TAG_CFR_OPTION_COMMENT:
          CfrProcessCharacterOption (
            (CFR_OPTION_VARCHAR *)CfrFormData,
            StartOpCodeHandle,
            &ProcessedLength
            );
          break;
        default:
          DEBUG ((
            DEBUG_ERROR,
            "CFR: Offset 0x%x - Unexpected entry 0x%x (size 0x%x)!\n",
            ProcessedLength,
            CfrFormData->tag,
            CfrFormData->size
            ));
          ProcessedLength += CfrFormData->size;
          break;
      }
    }

    GuidHob = GetNextGuidHob (&gEfiCfrSetupMenuFormGuid, GET_NEXT_HOB (GuidHob));
  }

  //
  // Submit updates
  //
  Status = HiiUpdateForm (
             mSetupMenuPrivate.HiiHandle,
             &mSetupMenuFormsetGuid,
             SETUP_MENU_FORM_ID,
             StartOpCodeHandle,
             EndOpCodeHandle
             );
  ASSERT_EFI_ERROR (Status);

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}

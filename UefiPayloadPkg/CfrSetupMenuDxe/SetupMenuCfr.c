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

typedef struct {
  UINT8         *Data;
  UINTN         Size;
  UINTN         Capacity;
  UINT32        RecordCount;
  EFI_STATUS    Status;
} CFR_FWUPD_SETTINGS_BUILDER;

STATIC CFR_FWUPD_SETTINGS_BUILDER  mCfrFwupdSettings;

/**
  Grow the runtime CFR settings buffer.

  @param[in] ExtraSize  Number of additional bytes required.

  @retval EFI_SUCCESS           The buffer has sufficient capacity.
  @retval EFI_OUT_OF_RESOURCES  The required size is unsupported or allocation
                                failed.
**/
STATIC
EFI_STATUS
CfrFwupdSettingsReserve (
  IN UINTN  ExtraSize
  )
{
  UINT8  *NewData;
  UINTN  NewCapacity;
  UINTN  RequiredSize;

  if (EFI_ERROR (mCfrFwupdSettings.Status)) {
    return mCfrFwupdSettings.Status;
  }

  if ((ExtraSize > MAX_UINT32) ||
      (mCfrFwupdSettings.Size > (MAX_UINT32 - ExtraSize)))
  {
    mCfrFwupdSettings.Status = EFI_OUT_OF_RESOURCES;
    return mCfrFwupdSettings.Status;
  }

  RequiredSize = mCfrFwupdSettings.Size + ExtraSize;
  if (RequiredSize <= mCfrFwupdSettings.Capacity) {
    return EFI_SUCCESS;
  }

  NewCapacity = MAX (mCfrFwupdSettings.Capacity, 256);
  while (NewCapacity < RequiredSize) {
    if (NewCapacity > (MAX_UINT32 / 2)) {
      NewCapacity = RequiredSize;
      break;
    }

    NewCapacity *= 2;
  }

  NewData = ReallocatePool (
              mCfrFwupdSettings.Capacity,
              NewCapacity,
              mCfrFwupdSettings.Data
              );
  if (NewData == NULL) {
    mCfrFwupdSettings.Status = EFI_OUT_OF_RESOURCES;
    return mCfrFwupdSettings.Status;
  }

  mCfrFwupdSettings.Data     = NewData;
  mCfrFwupdSettings.Capacity = NewCapacity;
  return EFI_SUCCESS;
}

/**
  Append bytes to the runtime CFR settings buffer.

  @param[in] Data      Bytes to append.
  @param[in] DataSize  Number of bytes to append.

  @retval EFI_SUCCESS           The bytes were appended.
  @retval EFI_OUT_OF_RESOURCES  The buffer could not be grown.
**/
STATIC
EFI_STATUS
CfrFwupdSettingsAppend (
  IN CONST VOID  *Data,
  IN UINTN       DataSize
  )
{
  EFI_STATUS  Status;

  Status = CfrFwupdSettingsReserve (DataSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DataSize > 0) {
    CopyMem (mCfrFwupdSettings.Data + mCfrFwupdSettings.Size, Data, DataSize);
  }

  mCfrFwupdSettings.Size += DataSize;
  return EFI_SUCCESS;
}

/**
  Reset the runtime CFR settings builder and append its header.
**/
STATIC
VOID
CfrFwupdSettingsInit (
  VOID
  )
{
  CFR_FWUPD_SETTINGS_HEADER  Header;

  if (mCfrFwupdSettings.Data != NULL) {
    FreePool (mCfrFwupdSettings.Data);
  }

  ZeroMem (&mCfrFwupdSettings, sizeof (mCfrFwupdSettings));
  ZeroMem (&Header, sizeof (Header));
  Header.Magic      = CFR_FWUPD_SETTINGS_MAGIC;
  Header.Version    = CFR_FWUPD_SETTINGS_VERSION;
  Header.HeaderSize = sizeof (Header);
  CfrFwupdSettingsAppend (&Header, sizeof (Header));
}

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
  CFR_VARBINARY records are used to store a list of dependency values.
  Get a pointer to an array of enum values and its length.
**/
STATIC
VOID
EFIAPI
CfrConvertVarBinaryToUint32Array (
  IN     CFR_VARBINARY  *CfrList,
  IN OUT UINT32         **Array,
  OUT UINT32            *ArrayLength
  )
{
  ASSERT ((CfrList != NULL) && (Array != NULL) && (ArrayLength != NULL));

  *Array       = (UINT32 *)CfrList->data;
  *ArrayLength = CfrList->data_length / sizeof (UINT32);
}

STATIC
CFR_RUNTIME_APPLY *
CfrExtractRuntimeApply (
  IN     UINT8  *Buffer,
  IN OUT UINTN  *Offset,
  IN     UINTN  BufferSize
  )
{
  CFR_RUNTIME_APPLY  *RuntimeApply;

  if ((*Offset > BufferSize) ||
      ((BufferSize - *Offset) < sizeof (CFR_RUNTIME_APPLY)))
  {
    return NULL;
  }

  RuntimeApply = (CFR_RUNTIME_APPLY *)(Buffer + *Offset);
  if (RuntimeApply->tag != CB_TAG_CFR_RUNTIME_APPLY) {
    return NULL;
  }

  if (RuntimeApply->size != sizeof (CFR_RUNTIME_APPLY)) {
    return NULL;
  }

  *Offset += RuntimeApply->size;
  return RuntimeApply;
}

/**
  Append the data carried by an optional CFR variable-length record.

  @param[in] CfrString  Record to append, or NULL.

  @retval EFI_SUCCESS           The record was absent or its data was appended.
  @retval EFI_OUT_OF_RESOURCES  The output buffer could not be grown.
**/
STATIC
EFI_STATUS
CfrFwupdSettingsAppendVarBinary (
  IN CFR_VARBINARY  *CfrString OPTIONAL
  )
{
  if (CfrString == NULL) {
    return EFI_SUCCESS;
  }

  return CfrFwupdSettingsAppend (CfrString->data, CfrString->data_length);
}

/**
  Count the enum records following a validated CFR numeric option.

  @param[in] Option           Numeric option containing the enum records.
  @param[in] FirstEnumOffset  Offset of the first enum record.

  @return Number of enum records.
**/
STATIC
UINT32
CfrFwupdEnumCount (
  IN CFR_OPTION_NUMERIC  *Option,
  IN UINTN               FirstEnumOffset
  )
{
  UINT32          EnumCount;
  UINTN           Offset;
  CFR_ENUM_VALUE  *CfrEnumValue;

  EnumCount = 0;
  Offset    = FirstEnumOffset;
  while (Offset < Option->size) {
    CfrEnumValue = (CFR_ENUM_VALUE *)((UINT8 *)Option + Offset);
    EnumCount++;
    Offset += CfrEnumValue->size;
  }

  return EnumCount;
}

/**
  Add a runtime-accessible numeric CFR option to the settings buffer.

  @param[in] Option           Numeric option to add.
  @param[in] CfrOptionName    Variable name record.
  @param[in] CfrDisplayName   Display name record.
  @param[in] CfrHelpText      Optional help text record.
  @param[in] CfrRuntimeApply  Optional runtime-apply record.
  @param[in] FirstEnumOffset  Offset of the first enum record.
**/
STATIC
VOID
CfrFwupdSettingsAddNumericOption (
  IN CFR_OPTION_NUMERIC  *Option,
  IN CFR_VARBINARY       *CfrOptionName,
  IN CFR_VARBINARY       *CfrDisplayName,
  IN CFR_VARBINARY       *CfrHelpText OPTIONAL,
  IN CFR_RUNTIME_APPLY   *CfrRuntimeApply OPTIONAL,
  IN UINTN               FirstEnumOffset
  )
{
  EFI_STATUS               Status;
  CFR_FWUPD_OPTION_RECORD  Record;
  CFR_FWUPD_ENUM_RECORD    EnumRecord;
  UINTN                    Offset;
  CFR_ENUM_VALUE           *CfrEnumValue;
  CFR_VARBINARY            *CfrEnumUiString;

  if (((Option->flags & CFR_OPTFLAG_RUNTIME) == 0) ||
      ((Option->flags & (CFR_OPTFLAG_SUPPRESS | CFR_OPTFLAG_VOLATILE)) != 0) ||
      EFI_ERROR (mCfrFwupdSettings.Status))
  {
    return;
  }

  ZeroMem (&Record, sizeof (Record));
  Record.HeaderSize   = sizeof (Record);
  Record.CfrFlags     = Option->flags;
  Record.ObjectId     = Option->object_id;
  Record.DefaultValue = Option->default_value;
  Record.Min          = Option->min;
  Record.Max          = Option->max;
  Record.Step         = Option->step;
  Record.DisplayFlags = Option->display_flags;
  Record.NameSize     = CfrOptionName->data_length;
  Record.UiNameSize   = CfrDisplayName->data_length;
  Record.HelpTextSize = (CfrHelpText != NULL) ? CfrHelpText->data_length : 0;

  if (CfrRuntimeApply != NULL) {
    Record.RuntimeApplyMethod = CfrRuntimeApply->method;
    Record.RuntimeApplyId     = CfrRuntimeApply->id;
  }

  if (Option->tag == CB_TAG_CFR_OPTION_ENUM) {
    Record.Type      = CfrFwupdOptionTypeEnum;
    Record.EnumCount = CfrFwupdEnumCount (Option, FirstEnumOffset);
  } else if (Option->tag == CB_TAG_CFR_OPTION_NUMBER) {
    Record.Type = CfrFwupdOptionTypeNumber;
  } else {
    Record.Type = CfrFwupdOptionTypeBool;
  }

  Status = CfrFwupdSettingsAppend (&Record, sizeof (Record));
  if (!EFI_ERROR (Status)) {
    Status = CfrFwupdSettingsAppendVarBinary (CfrOptionName);
  }

  if (!EFI_ERROR (Status)) {
    Status = CfrFwupdSettingsAppendVarBinary (CfrDisplayName);
  }

  if (!EFI_ERROR (Status)) {
    Status = CfrFwupdSettingsAppendVarBinary (CfrHelpText);
  }

  Offset = FirstEnumOffset;
  while (!EFI_ERROR (Status) && (Offset < Option->size)) {
    CfrEnumValue    = (CFR_ENUM_VALUE *)((UINT8 *)Option + Offset);
    CfrEnumUiString = (CFR_VARBINARY *)(
                                        (UINT8 *)CfrEnumValue + sizeof (CFR_ENUM_VALUE)
                                        );
    EnumRecord.Value      = CfrEnumValue->value;
    EnumRecord.UiNameSize = CfrEnumUiString->data_length;
    Status                = CfrFwupdSettingsAppend (&EnumRecord, sizeof (EnumRecord));
    if (!EFI_ERROR (Status)) {
      Status = CfrFwupdSettingsAppendVarBinary (CfrEnumUiString);
    }

    Offset += CfrEnumValue->size;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "CFR: Failed to add fwupd option: %r\n", Status));
    return;
  }

  if (mCfrFwupdSettings.RecordCount == MAX_UINT32) {
    mCfrFwupdSettings.Status = EFI_OUT_OF_RESOURCES;
    return;
  }

  mCfrFwupdSettings.RecordCount++;
}

/**
  Publish the completed runtime CFR settings buffer and release it.
**/
STATIC
VOID
CfrFwupdSettingsPublish (
  VOID
  )
{
  EFI_STATUS                 Status;
  CFR_FWUPD_SETTINGS_HEADER  *Header;
  UINT32                     Attributes;

  if (EFI_ERROR (mCfrFwupdSettings.Status) ||
      (mCfrFwupdSettings.Data == NULL))
  {
    DEBUG ((
      DEBUG_WARN,
      "CFR: Failed to build fwupd settings: %r\n",
      mCfrFwupdSettings.Status
      ));
    goto Free;
  }

  Header              = (CFR_FWUPD_SETTINGS_HEADER *)mCfrFwupdSettings.Data;
  Header->Size        = (UINT32)mCfrFwupdSettings.Size;
  Header->RecordCount = mCfrFwupdSettings.RecordCount;

  Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS |
               EFI_VARIABLE_RUNTIME_ACCESS;
  Status = gRT->SetVariable (
                  CFR_FWUPD_SETTINGS_VARIABLE_NAME,
                  &gEficorebootNvDataGuid,
                  Attributes,
                  mCfrFwupdSettings.Size,
                  mCfrFwupdSettings.Data
                  );
  if (Status == EFI_INVALID_PARAMETER) {
    gRT->SetVariable (
           CFR_FWUPD_SETTINGS_VARIABLE_NAME,
           &gEficorebootNvDataGuid,
           0,
           0,
           NULL
           );
    Status = gRT->SetVariable (
                    CFR_FWUPD_SETTINGS_VARIABLE_NAME,
                    &gEficorebootNvDataGuid,
                    Attributes,
                    mCfrFwupdSettings.Size,
                    mCfrFwupdSettings.Data
                    );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "CFR: Failed to publish fwupd settings: %r\n", Status));
  }

Free:
  if (mCfrFwupdSettings.Data != NULL) {
    FreePool (mCfrFwupdSettings.Data);
  }

  ZeroMem (&mCfrFwupdSettings, sizeof (mCfrFwupdSettings));
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
  Produce conditional HII `SUPPRESS_IF` based on a dependency:

  Caller to close each `SUPPRESS_IF` with `HiiCreateEndOpCode()`.

**/
STATIC
VOID
EFIAPI
CfrProduceHiiForDependency (
  IN VOID    *StartOpCodeHandle,
  IN UINTN   DependencyId,
  IN UINT32  *DepValues,
  IN UINT32  NumDepValues
  )
{
  EFI_IFR_OP_HEADER       OpHeader;
  UINT8                   *TempHiiBuffer;
  UINTN                   EqIdValListSize;
  EFI_IFR_EQ_ID_VAL_LIST  *EqIdValList;
  EFI_IFR_EQ_ID_VAL       EqIdVal;
  UINTN                   Index;

  OpHeader.OpCode = EFI_IFR_SUPPRESS_IF_OP;
  OpHeader.Length = sizeof (EFI_IFR_OP_HEADER);
  OpHeader.Scope  = 1;

  TempHiiBuffer = HiiCreateRawOpCodes (
                    StartOpCodeHandle,
                    (UINT8 *)&OpHeader,
                    sizeof (EFI_IFR_OP_HEADER)
                    );
  ASSERT (TempHiiBuffer != NULL);

  if (NumDepValues != 0) {
    EqIdValListSize = sizeof (EFI_IFR_EQ_ID_VAL_LIST) + ((NumDepValues - 1) * sizeof (UINT16));
    EqIdValList     = AllocatePool (EqIdValListSize);
    ASSERT (EqIdValList != NULL);

    EqIdValList->Header.OpCode = EFI_IFR_EQ_ID_VAL_LIST_OP;
    EqIdValList->Header.Length = EqIdValListSize;
    EqIdValList->Header.Scope  = 1;

    EqIdValList->QuestionId = DependencyId;
    EqIdValList->ListLength = NumDepValues;
    for (Index = 0; Index < NumDepValues; Index++) {
      EqIdValList->ValueList[Index] = (UINT16)*DepValues++;
    }

    TempHiiBuffer = HiiCreateRawOpCodes (
                      StartOpCodeHandle,
                      (UINT8 *)EqIdValList,
                      EqIdValListSize
                      );
    ASSERT (TempHiiBuffer != NULL);

    FreePool (EqIdValList);

    OpHeader.OpCode = EFI_IFR_NOT_OP;
    OpHeader.Length = sizeof (EFI_IFR_OP_HEADER);
    OpHeader.Scope  = 0;

    TempHiiBuffer = HiiCreateRawOpCodes (
                      StartOpCodeHandle,
                      (UINT8 *)&OpHeader,
                      sizeof (EFI_IFR_OP_HEADER)
                      );
    ASSERT (TempHiiBuffer != NULL);
  } else {
    EqIdVal.Header.OpCode = EFI_IFR_EQ_ID_VAL_OP;
    EqIdVal.Header.Length = sizeof (EFI_IFR_EQ_ID_VAL);
    EqIdVal.Header.Scope  = 1;
    EqIdVal.QuestionId    = DependencyId;
    EqIdVal.Value         = 0;

    TempHiiBuffer = HiiCreateRawOpCodes (
                      StartOpCodeHandle,
                      (UINT8 *)&EqIdVal,
                      sizeof (EFI_IFR_EQ_ID_VAL)
                      );
    ASSERT (TempHiiBuffer != NULL);
  }

  OpHeader.OpCode = EFI_IFR_END_OP;
  OpHeader.Length = sizeof (EFI_IFR_OP_HEADER);
  OpHeader.Scope  = 0;

  TempHiiBuffer = HiiCreateRawOpCodes (
                    StartOpCodeHandle,
                    (UINT8 *)&OpHeader,
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
  UINT32            ExistingAttributes;
  VOID              *ExistingData;
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

  if (OptionFlags & CFR_OPTFLAG_RUNTIME) {
    VariableAttributes |= EFI_VARIABLE_RUNTIME_ACCESS;
  }

  ExistingAttributes = 0;
  ExistingData       = NULL;
  DataSize           = 0;
  Status             = gRT->GetVariable (
                              VariableCfrName,
                              &gEficorebootNvDataGuid,
                              &ExistingAttributes,
                              &DataSize,
                              NULL
                              );
  if (Status == EFI_NOT_FOUND) {
    DataSize = CfrOptionLength;
    Status   = gRT->SetVariable (
                      VariableCfrName,
                      &gEficorebootNvDataGuid,
                      VariableAttributes,
                      DataSize,
                      CfrOptionDefaultValue
                      );
    ASSERT_EFI_ERROR (Status);
  } else if ((Status == EFI_BUFFER_TOO_SMALL) && (ExistingAttributes != VariableAttributes)) {
    ExistingData = AllocatePool (DataSize);
    if (ExistingData == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      Status = gRT->GetVariable (
                      VariableCfrName,
                      &gEficorebootNvDataGuid,
                      &ExistingAttributes,
                      &DataSize,
                      ExistingData
                      );
      if (!EFI_ERROR (Status)) {
        Status = gRT->SetVariable (
                        VariableCfrName,
                        &gEficorebootNvDataGuid,
                        VariableAttributes,
                        DataSize,
                        ExistingData
                        );
        if (Status == EFI_INVALID_PARAMETER) {
          //
          // UEFI variables cannot always be updated in-place when their
          // attributes change. Delete the old variable and recreate it with
          // the current CFR attributes while keeping the stored value.
          //
          Status = gRT->SetVariable (
                          VariableCfrName,
                          &gEficorebootNvDataGuid,
                          0,
                          0,
                          NULL
                          );
          if (!EFI_ERROR (Status)) {
            Status = gRT->SetVariable (
                            VariableCfrName,
                            &gEficorebootNvDataGuid,
                            VariableAttributes,
                            DataSize,
                            ExistingData
                            );
          }
        }
      }

      FreePool (ExistingData);
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "CFR: Failed to update attributes for variable \"%s\": %r\n",
        VariableCfrName,
        Status
        ));
    }
  }

  if (OptionFlags & CFR_OPTFLAG_READONLY && (mVariablePolicy != NULL)) {
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
  OptionNameLength   = AsciiStrLen ((CHAR8 *)CfrOptionName->data);
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
  VarStore->Size       = CfrOptionLength;

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
  UINTN          OptionProcessedLength;
  CFR_VARBINARY  *CfrFormName;
  UINT32         *DepValues;
  UINT32         NumDepValues;
  CFR_VARBINARY  *CfrDepValues;
  CHAR16         *HiiFormNameString;
  EFI_STRING_ID  HiiFormNameStringId;
  UINT8          *TempHiiBuffer;

  //
  // Extract variable-length fields that follow the header
  //
  OptionProcessedLength = sizeof (CFR_OPTION_FORM);
  CfrFormName           = CfrExtractVarBinary (
                            (UINT8 *)Option,
                            &OptionProcessedLength,
                            Option->size,
                            CB_TAG_CFR_VARCHAR_UI_NAME
                            );
  ASSERT (CfrFormName != NULL);

  // Dependency values are optional
  DepValues    = NULL;
  NumDepValues = 0;
  CfrDepValues = CfrExtractVarBinary (
                   (UINT8 *)Option,
                   &OptionProcessedLength,
                   Option->size,
                   CB_TAG_CFR_DEP_VALUES
                   );
  if (CfrDepValues != NULL) {
    ASSERT (CfrDepValues->tag == CB_TAG_CFR_DEP_VALUES);
    CfrConvertVarBinaryToUint32Array (CfrDepValues, &DepValues, &NumDepValues);
  }

  DEBUG ((
    DEBUG_INFO,
    "CFR: Processing form \"%a\", size 0x%x\n",
    CfrFormName->data,
    Option->size
    ));

  CfrConvertVarBinaryToStrings (CfrFormName, &HiiFormNameString, &HiiFormNameStringId);
  FreePool (HiiFormNameString);

  if (Option->dependency_id) {
    CfrProduceHiiForDependency (
      StartOpCodeHandle,
      CFR_COMPONENT_START + Option->dependency_id,
      DepValues,
      NumDepValues
      );
  }

  if (Option->flags & CFR_OPTFLAG_SUPPRESS) {
    CfrProduceHiiForFlags (StartOpCodeHandle, EFI_IFR_SUPPRESS_IF_OP);
  }

  if (Option->flags & CFR_OPTFLAG_INACTIVE) {
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

  if (Option->flags & CFR_OPTFLAG_INACTIVE) {
    TempHiiBuffer = HiiCreateEndOpCode (StartOpCodeHandle);
    ASSERT (TempHiiBuffer != NULL);
  }

  *ProcessedLength += OptionProcessedLength;
  if (Option->flags & CFR_OPTFLAG_SUPPRESS) {
    TempHiiBuffer = HiiCreateEndOpCode (StartOpCodeHandle);
    ASSERT (TempHiiBuffer != NULL);
  }

  if (Option->dependency_id) {
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
  UINTN              OptionProcessedLength;
  CFR_VARBINARY      *CfrOptionName;
  CFR_VARBINARY      *CfrDisplayName;
  CFR_VARBINARY      *CfrHelpText;
  UINT32             *DepValues;
  UINT32             NumDepValues;
  CFR_VARBINARY      *CfrDepValues;
  CFR_RUNTIME_APPLY  *CfrRuntimeApply;
  UINTN              QuestionIdVarStoreId;
  UINT8              QuestionFlags;
  VOID               *DefaultOpCodeHandle;
  UINT8              *TempHiiBuffer;
  CHAR16             *HiiDisplayString;
  EFI_STRING_ID      HiiDisplayStringId;
  CHAR16             *HiiHelpText;
  EFI_STRING_ID      HiiHelpTextId;
  VOID               *OptionOpCodeHandle;
  CFR_ENUM_VALUE     *CfrEnumValues;
  CFR_VARBINARY      *CfrEnumUiString;
  CHAR16             *HiiEnumStrings;
  EFI_STRING_ID      HiiEnumStringsId;

  //
  // Extract variable-length fields that follow the header
  //
  OptionProcessedLength = sizeof (CFR_OPTION_NUMERIC);

  CfrOptionName = CfrExtractVarBinary (
                    (UINT8 *)Option,
                    &OptionProcessedLength,
                    Option->size,
                    CB_TAG_CFR_VARCHAR_OPT_NAME
                    );
  ASSERT (CfrOptionName != NULL);
  CfrDisplayName = CfrExtractVarBinary (
                     (UINT8 *)Option,
                     &OptionProcessedLength,
                     Option->size,
                     CB_TAG_CFR_VARCHAR_UI_NAME
                     );
  ASSERT (CfrDisplayName != NULL);

  // Help text is optional
  CfrHelpText = CfrExtractVarBinary (
                  (UINT8 *)Option,
                  &OptionProcessedLength,
                  Option->size,
                  CB_TAG_CFR_VARCHAR_UI_HELPTEXT
                  );
  if (CfrHelpText != NULL) {
    ASSERT (CfrHelpText->tag == CB_TAG_CFR_VARCHAR_UI_HELPTEXT);
  }

  // Dependency values are optional
  DepValues    = NULL;
  NumDepValues = 0;
  CfrDepValues = CfrExtractVarBinary (
                   (UINT8 *)Option,
                   &OptionProcessedLength,
                   Option->size,
                   CB_TAG_CFR_DEP_VALUES
                   );
  if (CfrDepValues != NULL) {
    ASSERT (CfrDepValues->tag == CB_TAG_CFR_DEP_VALUES);
    CfrConvertVarBinaryToUint32Array (CfrDepValues, &DepValues, &NumDepValues);
  }

  CfrRuntimeApply = CfrExtractRuntimeApply (
                      (UINT8 *)Option,
                      &OptionProcessedLength,
                      Option->size
                      );

  CfrFwupdSettingsAddNumericOption (
    Option,
    CfrOptionName,
    CfrDisplayName,
    CfrHelpText,
    CfrRuntimeApply,
    OptionProcessedLength
    );

  DEBUG ((
    DEBUG_INFO,
    "CFR: Processing option \"%a\", size 0x%x\n",
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

  if (Option->dependency_id) {
    CfrProduceHiiForDependency (
      StartOpCodeHandle,
      CFR_COMPONENT_START + Option->dependency_id,
      DepValues,
      NumDepValues
      );
  }

  if (Option->flags & CFR_OPTFLAG_SUPPRESS) {
    CfrProduceHiiForFlags (StartOpCodeHandle, EFI_IFR_SUPPRESS_IF_OP);
  }

  if (Option->flags & CFR_OPTFLAG_INACTIVE) {
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
                      EFI_IFR_NUMERIC_SIZE_4 |
                      ((Option->display_flags & CFR_NUM_OPT_DISPFLAG_HEX) ?
                       EFI_IFR_DISPLAY_UINT_HEX : EFI_IFR_DISPLAY_UINT_DEC),
                      Option->min,
                      Option->max,
                      Option->step,
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

  if (Option->flags & CFR_OPTFLAG_INACTIVE) {
    TempHiiBuffer = HiiCreateEndOpCode (StartOpCodeHandle);
    ASSERT (TempHiiBuffer != NULL);
  }

  if (Option->flags & CFR_OPTFLAG_SUPPRESS) {
    TempHiiBuffer = HiiCreateEndOpCode (StartOpCodeHandle);
    ASSERT (TempHiiBuffer != NULL);
  }

  if (Option->dependency_id) {
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
  UINTN          OptionProcessedLength;
  CFR_VARBINARY  *CfrOptionName;
  CFR_VARBINARY  *CfrDisplayName;
  CFR_VARBINARY  *CfrHelpText;
  CFR_VARBINARY  *CfrDefaultValue;
  UINT32         *DepValues;
  UINT32         NumDepValues;
  CFR_VARBINARY  *CfrDepValues;
  UINTN          QuestionIdVarStoreId;
  CHAR16         *HiiDefaultValue;
  EFI_STRING_ID  HiiDefaultValueId;
  UINTN          HiiDefaultValueLengthChars;
  CHAR16         *HiiDisplayString;
  EFI_STRING_ID  HiiDisplayStringId;
  CHAR16         *HiiHelpText;
  EFI_STRING_ID  HiiHelpTextId;
  UINT8          QuestionFlags;
  VOID           *DefaultOpCodeHandle;
  UINT8          *TempHiiBuffer;

  //
  // Extract variable-length fields that follow the header
  //
  ASSERT (sizeof (CFR_OPTION_VARCHAR) == sizeof (CFR_OPTION_COMMENT));
  OptionProcessedLength = sizeof (CFR_OPTION_VARCHAR);

  // Only true string options have variables
  CfrOptionName = NULL;
  if (Option->tag == CB_TAG_CFR_OPTION_VARCHAR) {
    CfrDefaultValue = CfrExtractVarBinary (
                        (UINT8 *)Option,
                        &OptionProcessedLength,
                        Option->size,
                        CB_TAG_CFR_VARCHAR_DEF_VALUE
                        );
    ASSERT (CfrDefaultValue != NULL);

    if (CfrDefaultValue->data_length > 0xFF) {
      DEBUG ((DEBUG_ERROR, "CFR: Default value length 0x%x is too long!\n", CfrDefaultValue->data_length));
      *ProcessedLength += Option->size;
      return;
    }

    CfrOptionName = CfrExtractVarBinary (
                      (UINT8 *)Option,
                      &OptionProcessedLength,
                      Option->size,
                      CB_TAG_CFR_VARCHAR_OPT_NAME
                      );
    ASSERT (CfrOptionName != NULL);
  }

  CfrDisplayName = CfrExtractVarBinary (
                     (UINT8 *)Option,
                     &OptionProcessedLength,
                     Option->size,
                     CB_TAG_CFR_VARCHAR_UI_NAME
                     );
  ASSERT (CfrDisplayName != NULL);

  // Help text is optional
  CfrHelpText = CfrExtractVarBinary (
                  (UINT8 *)Option,
                  &OptionProcessedLength,
                  Option->size,
                  CB_TAG_CFR_VARCHAR_UI_HELPTEXT
                  );
  if (CfrHelpText != NULL) {
    ASSERT (CfrHelpText->tag == CB_TAG_CFR_VARCHAR_UI_HELPTEXT);
  }

  // Dependency values are optional
  DepValues    = NULL;
  NumDepValues = 0;
  CfrDepValues = CfrExtractVarBinary (
                   (UINT8 *)Option,
                   &OptionProcessedLength,
                   Option->size,
                   CB_TAG_CFR_DEP_VALUES
                   );
  if (CfrDepValues != NULL) {
    ASSERT (CfrDepValues->tag == CB_TAG_CFR_DEP_VALUES);
    CfrConvertVarBinaryToUint32Array (CfrDepValues, &DepValues, &NumDepValues);
  }

  DEBUG ((
    DEBUG_INFO,
    "CFR: Processing option \"%a\", size 0x%x\n",
    (CfrOptionName != NULL) ? CfrOptionName->data : CfrDisplayName->data,
    Option->size
    ));

  //
  // Processing start
  //
  if (Option->tag == CB_TAG_CFR_OPTION_VARCHAR) {
    QuestionIdVarStoreId = CFR_COMPONENT_START + Option->object_id;

    if (CfrDefaultValue->data_length > 1) {
      CfrConvertVarBinaryToStrings (CfrDefaultValue, &HiiDefaultValue, &HiiDefaultValueId);
      HiiDefaultValueLengthChars = CfrDefaultValue->data_length;
    } else {
      HiiDefaultValue            = HiiGetString (mSetupMenuPrivate.HiiHandle, STRING_TOKEN (STR_INVALID_STRING), NULL);
      HiiDefaultValueId          = STRING_TOKEN (STR_INVALID_STRING);
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

  if (Option->dependency_id) {
    CfrProduceHiiForDependency (
      StartOpCodeHandle,
      CFR_COMPONENT_START + Option->dependency_id,
      DepValues,
      NumDepValues
      );
  }

  if (Option->flags & CFR_OPTFLAG_SUPPRESS) {
    CfrProduceHiiForFlags (StartOpCodeHandle, EFI_IFR_SUPPRESS_IF_OP);
  }

  if (Option->flags & CFR_OPTFLAG_INACTIVE) {
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

  if (Option->flags & CFR_OPTFLAG_INACTIVE) {
    TempHiiBuffer = HiiCreateEndOpCode (StartOpCodeHandle);
    ASSERT (TempHiiBuffer != NULL);
  }

  if (Option->flags & CFR_OPTFLAG_SUPPRESS) {
    TempHiiBuffer = HiiCreateEndOpCode (StartOpCodeHandle);
    ASSERT (TempHiiBuffer != NULL);
  }

  if (Option->dependency_id) {
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
  UINT8               *TempHiiBuffer;

  CfrFwupdSettingsInit ();

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

    TempHiiBuffer = HiiCreateSubTitleOpCode (
                      StartOpCodeHandle,
                      STRING_TOKEN (STR_EMPTY_STRING),
                      0,
                      0,
                      0
                      );
    ASSERT (TempHiiBuffer != NULL);

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

  CfrFwupdSettingsPublish ();

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}

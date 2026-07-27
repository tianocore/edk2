/** @file
  Contains helper functions for working with CFR.

  Copyright (c) 2023, 9elements GmbH.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiMultiPhase.h>
#include <Pi/PiMultiPhase.h>
#include <Library/BaseLib.h>
#include <Library/CfrHelpersLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Guid/CfrSetupMenuGuid.h>

/**
  CFR_VARBINARY records are variable-length, so they aren't formal fields.
  Implement this handling by returning the struct at this offset.

  By incrementing the offset, this function behaves like a queue.
  Optional fields handled by returning NULL immediately.

**/
CFR_VARBINARY *
EFIAPI
CfrExtractVarBinary (
  IN     UINT8   *Buffer,
  IN OUT UINTN   *Offset,
  IN     UINT32  TargetTag
  )
{
  CFR_VARBINARY  *CfrVarBinary;

  CfrVarBinary = (CFR_VARBINARY *)(Buffer + *Offset);
  if (CfrVarBinary->tag != TargetTag) {
    return NULL;
  }

  *Offset += CfrVarBinary->size;

  return CfrVarBinary;
}

/**
  Return pointers into a buffer with the requested option's default value and size.
  This may be used by code that needs CFR defaults before the full CFR driver can write variables.

  TODO: Consider returning pools instead, caller to free.

  @retval EFI_SUCCESS            The default value is found.
  @retval EFI_INVALID_PARAMETER  The function parameters are invalid.
  @retval EFI_NOT_FOUND          The option cannot be found, or type doesn't have default values.

**/
EFI_STATUS
EFIAPI
CfrOptionGetDefaultValue (
  IN     CHAR8  *FormName            OPTIONAL,
  IN     CHAR8  *OptionName,
  IN OUT VOID   **DefaultValueData,
  IN OUT UINTN  *DefaultValueLength  OPTIONAL
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  CFR_OPTION_FORM    *CfrFormHob;
  UINTN              ProcessedLength;
  UINTN              OptionProcessedLength;
  CFR_VARBINARY      *CfrFormName;
  CFR_OPTION_FORM    *CfrFormData;
  CFR_VARBINARY      *CfrOptionName;
  CFR_VARBINARY      *TempUiName;
  CFR_VARBINARY      *TempHelpText;
  CFR_VARBINARY      *CfrDefaultValue;

  if ((OptionName == NULL) || (DefaultValueData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Begin processing a form HOB
  //
  GuidHob = GetFirstGuidHob (&gEfiCfrSetupMenuFormGuid);
  while (GuidHob != NULL) {
    CfrFormHob = GET_GUID_HOB_DATA (GuidHob);

    ProcessedLength = sizeof (CFR_OPTION_FORM);
    CfrFormName = CfrExtractVarBinary ((UINT8 *)CfrFormHob, &ProcessedLength, CB_TAG_CFR_VARCHAR_UI_NAME);
    ASSERT (CfrFormName != NULL);

    //
    // Different form requested, skip this
    //
    if ((FormName != NULL) && (AsciiStrCmp (FormName, (CHAR8 *)CfrFormName->data) != 0)) {
      GuidHob = GetNextGuidHob (&gEfiCfrSetupMenuFormGuid, GET_NEXT_HOB (GuidHob));
      continue;
    }

    //
    // Process form tree
    //
    while (ProcessedLength < CfrFormHob->size) {
      CfrFormData = (CFR_OPTION_FORM *)((UINT8 *)CfrFormHob + ProcessedLength);

      switch (CfrFormData->tag) {
        case CB_TAG_CFR_OPTION_FORM:
          // Processing nested forms, don't advance by `size`
          ProcessedLength += sizeof (CFR_OPTION_FORM);
          CfrFormName = CfrExtractVarBinary ((UINT8 *)CfrFormHob, &ProcessedLength, CB_TAG_CFR_VARCHAR_UI_NAME);
          ASSERT (CfrFormName != NULL);

          // Forms do not have default values
          if (AsciiStrCmp (OptionName, (CHAR8 *)CfrFormName->data) == 0) {
            return EFI_NOT_FOUND;
          }
          break;
        case CB_TAG_CFR_OPTION_ENUM:
        case CB_TAG_CFR_OPTION_NUMBER:
        case CB_TAG_CFR_OPTION_BOOL:
          OptionProcessedLength = sizeof (CFR_OPTION_NUMERIC);
          CfrOptionName = CfrExtractVarBinary ((UINT8 *)CfrFormData, &OptionProcessedLength, CB_TAG_CFR_VARCHAR_OPT_NAME);
          ASSERT (CfrOptionName != NULL);

          if (AsciiStrCmp (OptionName, (CHAR8 *)CfrOptionName->data) == 0) {
            *DefaultValueData = &((CFR_OPTION_NUMERIC *)CfrFormData)->default_value;
            if (DefaultValueLength != NULL) {
              *DefaultValueLength = sizeof (((CFR_OPTION_NUMERIC *)CfrFormData)->default_value);
            }
            return EFI_SUCCESS;
          }
          ProcessedLength += CfrFormData->size;
          break;
        case CB_TAG_CFR_OPTION_VARCHAR:
          // Currently required to remove intermediate VARBINARY structs of UI data
          OptionProcessedLength = sizeof (CFR_OPTION_VARCHAR);
          CfrOptionName = CfrExtractVarBinary ((UINT8 *)CfrFormData, &OptionProcessedLength, CB_TAG_CFR_VARCHAR_OPT_NAME);
          ASSERT (CfrOptionName != NULL);

          TempUiName = CfrExtractVarBinary ((UINT8 *)CfrFormData, &OptionProcessedLength, CB_TAG_CFR_VARCHAR_UI_NAME);
          ASSERT (TempUiName != NULL);
          TempHelpText = CfrExtractVarBinary ((UINT8 *)CfrFormData, &OptionProcessedLength, CB_TAG_CFR_VARCHAR_UI_HELPTEXT);
          if (TempHelpText != NULL) {
            ASSERT (TempHelpText->tag == CB_TAG_CFR_VARCHAR_UI_HELPTEXT);
          }

          CfrDefaultValue = CfrExtractVarBinary ((UINT8 *)CfrFormData, &OptionProcessedLength, CB_TAG_CFR_VARCHAR_DEF_VALUE);
          ASSERT (CfrDefaultValue != NULL);

          if (AsciiStrCmp (OptionName, (CHAR8 *)CfrOptionName->data) == 0) {
            *DefaultValueData = CfrDefaultValue->data;
            if (DefaultValueLength != NULL) {
              *DefaultValueLength = CfrDefaultValue->data_length;
            }
            return EFI_SUCCESS;
          }
          ProcessedLength += CfrFormData->size;
          break;
        case CB_TAG_CFR_OPTION_COMMENT:
        default:
          ProcessedLength += CfrFormData->size;
          break;
      }
    }

    GuidHob = GetNextGuidHob (&gEfiCfrSetupMenuFormGuid, GET_NEXT_HOB (GuidHob));
  }

  return EFI_NOT_FOUND;
}

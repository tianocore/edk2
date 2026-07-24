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

#define CFR_MAX_NESTING_DEPTH  32
#define CFR_OPTION_FLAGS_MASK  (CFR_OPTFLAG_READONLY | CFR_OPTFLAG_INACTIVE | \
                                CFR_OPTFLAG_SUPPRESS | CFR_OPTFLAG_VOLATILE | \
                                CFR_OPTFLAG_RUNTIME)

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
  IN     UINTN   BufferSize,
  IN     UINT32  TargetTag
  )
{
  CFR_VARBINARY  *CfrVarBinary;
  UINTN          RequiredSize;

  if ((Buffer == NULL) || (Offset == NULL) ||
      (*Offset > BufferSize) ||
      ((BufferSize - *Offset) < sizeof (CFR_VARBINARY)))
  {
    return NULL;
  }

  CfrVarBinary = (CFR_VARBINARY *)(Buffer + *Offset);
  if (CfrVarBinary->tag != TargetTag) {
    return NULL;
  }

  if ((CfrVarBinary->size < sizeof (CFR_VARBINARY)) ||
      (CfrVarBinary->size > (BufferSize - *Offset)) ||
      (CfrVarBinary->data_length > (CfrVarBinary->size - sizeof (CFR_VARBINARY))))
  {
    DEBUG ((
      DEBUG_WARN,
      "CFR: record tag 0x%x at offset 0x%x has invalid size 0x%x within 0x%x\n",
      CfrVarBinary->tag,
      *Offset,
      CfrVarBinary->size,
      BufferSize
      ));
    return NULL;
  }

  if (CfrVarBinary->data_length > MAX_UINTN - sizeof (CFR_VARBINARY) - 3) {
    return NULL;
  }

  RequiredSize = ALIGN_VALUE (
                   sizeof (CFR_VARBINARY) + CfrVarBinary->data_length,
                   sizeof (UINT32)
                   );
  if (CfrVarBinary->size != RequiredSize) {
    return NULL;
  }

  if (TargetTag == CB_TAG_CFR_DEP_VALUES) {
    if ((CfrVarBinary->data_length % sizeof (UINT32)) != 0) {
      return NULL;
    }
  } else if ((CfrVarBinary->data_length == 0) ||
             (CfrVarBinary->data[CfrVarBinary->data_length - 1] != '\0'))
  {
    return NULL;
  }

  *Offset += CfrVarBinary->size;

  return CfrVarBinary;
}

STATIC
RETURN_STATUS
CfrValidateVarBinary (
  IN     CONST UINT8  *Buffer,
  IN OUT UINTN        *Offset,
  IN     UINTN        BufferSize,
  IN     UINT32       Tag
  )
{
  if (CfrExtractVarBinary ((UINT8 *)Buffer, Offset, BufferSize, Tag) == NULL) {
    return RETURN_COMPROMISED_DATA;
  }

  return RETURN_SUCCESS;
}

STATIC
RETURN_STATUS
CfrValidateObject (
  IN CONST UINT8  *Buffer,
  IN UINTN        BufferSize,
  IN UINTN        Depth
  )
{
  CONST CFR_OPTION_FORM     *Header;
  CONST CFR_OPTION_NUMERIC  *Numeric;
  CONST CFR_ENUM_VALUE      *EnumValue;
  CONST CFR_RUNTIME_APPLY   *RuntimeApply;
  UINTN                     Offset;
  UINTN                     EnumOffset;
  RETURN_STATUS             Status;

  if ((Depth > CFR_MAX_NESTING_DEPTH) ||
      (BufferSize < sizeof (CFR_OPTION_FORM)))
  {
    return RETURN_COMPROMISED_DATA;
  }

  Header = (CONST CFR_OPTION_FORM *)Buffer;
  if ((Header->size < sizeof (CFR_OPTION_FORM)) ||
      (Header->size > BufferSize) ||
      ((Header->flags & ~CFR_OPTION_FLAGS_MASK) != 0))
  {
    return RETURN_COMPROMISED_DATA;
  }

  switch (Header->tag) {
    case CB_TAG_CFR_OPTION_FORM:
      Offset = sizeof (CFR_OPTION_FORM);
      Status = CfrValidateVarBinary (
                 Buffer,
                 &Offset,
                 Header->size,
                 CB_TAG_CFR_VARCHAR_UI_NAME
                 );
      if (RETURN_ERROR (Status)) {
        return Status;
      }

      CfrExtractVarBinary (
        (UINT8 *)Buffer,
        &Offset,
        Header->size,
        CB_TAG_CFR_DEP_VALUES
        );

      while (Offset < Header->size) {
        Status = CfrValidateObject (
                   Buffer + Offset,
                   Header->size - Offset,
                   Depth + 1
                   );
        if (RETURN_ERROR (Status)) {
          return Status;
        }

        Offset += ((CONST CFR_OPTION_FORM *)(Buffer + Offset))->size;
      }

      return (Offset == Header->size) ?
             RETURN_SUCCESS : RETURN_COMPROMISED_DATA;

    case CB_TAG_CFR_OPTION_ENUM:
    case CB_TAG_CFR_OPTION_NUMBER:
    case CB_TAG_CFR_OPTION_BOOL:
      if (Header->size < sizeof (CFR_OPTION_NUMERIC)) {
        return RETURN_COMPROMISED_DATA;
      }

      Numeric = (CONST CFR_OPTION_NUMERIC *)Buffer;
      Offset  = sizeof (CFR_OPTION_NUMERIC);
      Status  = CfrValidateVarBinary (
                  Buffer,
                  &Offset,
                  Header->size,
                  CB_TAG_CFR_VARCHAR_OPT_NAME
                  );
      if (RETURN_ERROR (Status)) {
        return Status;
      }

      Status = CfrValidateVarBinary (
                 Buffer,
                 &Offset,
                 Header->size,
                 CB_TAG_CFR_VARCHAR_UI_NAME
                 );
      if (RETURN_ERROR (Status)) {
        return Status;
      }

      CfrExtractVarBinary (
        (UINT8 *)Buffer,
        &Offset,
        Header->size,
        CB_TAG_CFR_VARCHAR_UI_HELPTEXT
        );
      CfrExtractVarBinary (
        (UINT8 *)Buffer,
        &Offset,
        Header->size,
        CB_TAG_CFR_DEP_VALUES
        );

      if ((Header->size - Offset) >= sizeof (CFR_RUNTIME_APPLY)) {
        RuntimeApply = (CONST CFR_RUNTIME_APPLY *)(Buffer + Offset);
        if (RuntimeApply->tag == CB_TAG_CFR_RUNTIME_APPLY) {
          if (RuntimeApply->size != sizeof (CFR_RUNTIME_APPLY)) {
            return RETURN_COMPROMISED_DATA;
          }

          Offset += RuntimeApply->size;
        }
      }

      if (Numeric->tag != CB_TAG_CFR_OPTION_ENUM) {
        return (Offset == Header->size) ?
               RETURN_SUCCESS : RETURN_COMPROMISED_DATA;
      }

      while (Offset < Header->size) {
        if ((Header->size - Offset) < sizeof (CFR_ENUM_VALUE)) {
          return RETURN_COMPROMISED_DATA;
        }

        EnumValue = (CONST CFR_ENUM_VALUE *)(Buffer + Offset);
        if ((EnumValue->tag != CB_TAG_CFR_ENUM_VALUE) ||
            (EnumValue->size < sizeof (CFR_ENUM_VALUE)) ||
            (EnumValue->size > (Header->size - Offset)))
        {
          return RETURN_COMPROMISED_DATA;
        }

        EnumOffset = sizeof (CFR_ENUM_VALUE);
        Status = CfrValidateVarBinary (
                   (CONST UINT8 *)EnumValue,
                   &EnumOffset,
                   EnumValue->size,
                   CB_TAG_CFR_VARCHAR_UI_NAME
                   );
        if (RETURN_ERROR (Status) || (EnumOffset != EnumValue->size)) {
          return RETURN_COMPROMISED_DATA;
        }

        Offset += EnumValue->size;
      }

      return RETURN_SUCCESS;

    case CB_TAG_CFR_OPTION_VARCHAR:
      Offset = sizeof (CFR_OPTION_VARCHAR);
      Status = CfrValidateVarBinary (
                 Buffer,
                 &Offset,
                 Header->size,
                 CB_TAG_CFR_VARCHAR_DEF_VALUE
                 );
      if (RETURN_ERROR (Status)) {
        return Status;
      }

      Status = CfrValidateVarBinary (
                 Buffer,
                 &Offset,
                 Header->size,
                 CB_TAG_CFR_VARCHAR_OPT_NAME
                 );
      if (RETURN_ERROR (Status)) {
        return Status;
      }

      Status = CfrValidateVarBinary (
                 Buffer,
                 &Offset,
                 Header->size,
                 CB_TAG_CFR_VARCHAR_UI_NAME
                 );
      if (RETURN_ERROR (Status)) {
        return Status;
      }

      CfrExtractVarBinary (
        (UINT8 *)Buffer,
        &Offset,
        Header->size,
        CB_TAG_CFR_VARCHAR_UI_HELPTEXT
        );
      CfrExtractVarBinary (
        (UINT8 *)Buffer,
        &Offset,
        Header->size,
        CB_TAG_CFR_DEP_VALUES
        );
      return (Offset == Header->size) ?
             RETURN_SUCCESS : RETURN_COMPROMISED_DATA;

    case CB_TAG_CFR_OPTION_COMMENT:
      Offset = sizeof (CFR_OPTION_COMMENT);
      Status = CfrValidateVarBinary (
                 Buffer,
                 &Offset,
                 Header->size,
                 CB_TAG_CFR_VARCHAR_UI_NAME
                 );
      if (RETURN_ERROR (Status)) {
        return Status;
      }

      CfrExtractVarBinary (
        (UINT8 *)Buffer,
        &Offset,
        Header->size,
        CB_TAG_CFR_VARCHAR_UI_HELPTEXT
        );
      CfrExtractVarBinary (
        (UINT8 *)Buffer,
        &Offset,
        Header->size,
        CB_TAG_CFR_DEP_VALUES
        );
      return (Offset == Header->size) ?
             RETURN_SUCCESS : RETURN_COMPROMISED_DATA;

    default:
      return RETURN_COMPROMISED_DATA;
  }
}

RETURN_STATUS
EFIAPI
CfrValidateForm (
  IN CONST CFR_OPTION_FORM  *Form,
  IN UINTN                  FormSize
  )
{
  RETURN_STATUS  Status;

  if (Form == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  Status = CfrValidateObject ((CONST UINT8 *)Form, FormSize, 0);
  if (RETURN_ERROR (Status) ||
      (Form->tag != CB_TAG_CFR_OPTION_FORM) ||
      (Form->size != FormSize))
  {
    return RETURN_COMPROMISED_DATA;
  }

  return RETURN_SUCCESS;
}

/**
  Return pointers into a buffer with the requested option's default value and size.
  This may be used by code that needs CFR defaults before the full CFR driver can write variables.

  TODO: Consider returning pools instead, caller to free.

  @retval EFI_SUCCESS            The default value is found.
  @retval EFI_INVALID_PARAMETER  The function parameters are invalid.
  @retval EFI_COMPROMISED_DATA   A CFR form is malformed.
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
    if (RETURN_ERROR (
          CfrValidateForm (
            CfrFormHob,
            GET_GUID_HOB_DATA_SIZE (GuidHob)
            )
          ))
    {
      return EFI_COMPROMISED_DATA;
    }

    ProcessedLength = sizeof (CFR_OPTION_FORM);
    CfrFormName = CfrExtractVarBinary (
                    (UINT8 *)CfrFormHob,
                    &ProcessedLength,
                    CfrFormHob->size,
                    CB_TAG_CFR_VARCHAR_UI_NAME
                    );
    if (CfrFormName == NULL) {
      return EFI_COMPROMISED_DATA;
    }

    CfrExtractVarBinary (
      (UINT8 *)CfrFormHob,
      &ProcessedLength,
      CfrFormHob->size,
      CB_TAG_CFR_DEP_VALUES
      );

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
          CfrFormName = CfrExtractVarBinary (
                          (UINT8 *)CfrFormHob,
                          &ProcessedLength,
                          CfrFormHob->size,
                          CB_TAG_CFR_VARCHAR_UI_NAME
                          );
          if (CfrFormName == NULL) {
            return EFI_COMPROMISED_DATA;
          }

          CfrExtractVarBinary (
            (UINT8 *)CfrFormHob,
            &ProcessedLength,
            CfrFormHob->size,
            CB_TAG_CFR_DEP_VALUES
            );

          // Forms do not have default values
          if (AsciiStrCmp (OptionName, (CHAR8 *)CfrFormName->data) == 0) {
            return EFI_NOT_FOUND;
          }
          break;
        case CB_TAG_CFR_OPTION_ENUM:
        case CB_TAG_CFR_OPTION_NUMBER:
        case CB_TAG_CFR_OPTION_BOOL:
          OptionProcessedLength = sizeof (CFR_OPTION_NUMERIC);
          CfrOptionName = CfrExtractVarBinary (
                            (UINT8 *)CfrFormData,
                            &OptionProcessedLength,
                            CfrFormData->size,
                            CB_TAG_CFR_VARCHAR_OPT_NAME
                            );
          if (CfrOptionName == NULL) {
            return EFI_COMPROMISED_DATA;
          }

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
          OptionProcessedLength = sizeof (CFR_OPTION_VARCHAR);
          CfrDefaultValue = CfrExtractVarBinary (
                              (UINT8 *)CfrFormData,
                              &OptionProcessedLength,
                              CfrFormData->size,
                              CB_TAG_CFR_VARCHAR_DEF_VALUE
                              );
          if (CfrDefaultValue == NULL) {
            return EFI_COMPROMISED_DATA;
          }

          CfrOptionName = CfrExtractVarBinary (
                            (UINT8 *)CfrFormData,
                            &OptionProcessedLength,
                            CfrFormData->size,
                            CB_TAG_CFR_VARCHAR_OPT_NAME
                            );
          if (CfrOptionName == NULL) {
            return EFI_COMPROMISED_DATA;
          }

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

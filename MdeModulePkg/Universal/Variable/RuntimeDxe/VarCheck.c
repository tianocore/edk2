/** @file
  Implementation functions and structures for var check protocol.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Variable.h"
#include <Library/DevicePathLib.h>

extern LIST_ENTRY mLockedVariableList;
extern BOOLEAN mEndOfDxe;
extern BOOLEAN mEnableLocking;

#define VAR_CHECK_HANDLER_TABLE_SIZE    0x8

UINT32                                  mNumberOfHandler = 0;
UINT32                                  mMaxNumberOfHandler = 0;
VAR_CHECK_SET_VARIABLE_CHECK_HANDLER    *mHandlerTable = NULL;

typedef struct {
  LIST_ENTRY                    Link;
  EFI_GUID                      Guid;
  VAR_CHECK_VARIABLE_PROPERTY   VariableProperty;
  //CHAR16                        *Name;
} VAR_CHECK_VARIABLE_ENTRY;

LIST_ENTRY mVarCheckVariableList = INITIALIZE_LIST_HEAD_VARIABLE (mVarCheckVariableList);

typedef
EFI_STATUS
(EFIAPI *INTERNAL_VAR_CHECK_FUNCTION) (
  IN VAR_CHECK_VARIABLE_PROPERTY    *Propery,
  IN UINTN                          DataSize,
  IN VOID                           *Data
  );

typedef struct {
  CHAR16                        *Name;
  VAR_CHECK_VARIABLE_PROPERTY   VariableProperty;
  INTERNAL_VAR_CHECK_FUNCTION   CheckFunction;
} UEFI_DEFINED_VARIABLE_ENTRY;

/**
  Internal check for load option.

  @param[in] VariablePropery    Pointer to variable property.
  @param[in] DataSize           Data size.
  @param[in] Data               Pointer to data buffer.

  @retval EFI_SUCCESS           The SetVariable check result was success.
  @retval EFI_INVALID_PARAMETER The data buffer is not a valid load option.

**/
EFI_STATUS
EFIAPI
InternalVarCheckLoadOption (
  IN VAR_CHECK_VARIABLE_PROPERTY    *VariablePropery,
  IN UINTN                          DataSize,
  IN VOID                           *Data
  )
{
  UINT16                    FilePathListLength;
  CHAR16                    *Description;
  EFI_DEVICE_PATH_PROTOCOL  *FilePathList;

  FilePathListLength = *((UINT16 *) ((UINTN) Data + sizeof (UINT32)));

  //
  // Check Description
  //
  Description = (CHAR16 *) ((UINTN) Data + sizeof (UINT32) + sizeof (UINT16));
  while (Description < (CHAR16 *) ((UINTN) Data + DataSize)) {
    if (*Description == L'\0') {
      break;
    }
    Description++;
  }
  if ((UINTN) Description >= ((UINTN) Data + DataSize)) {
    return EFI_INVALID_PARAMETER;
  }
  Description++;

  //
  // Check FilePathList
  //
  FilePathList = (EFI_DEVICE_PATH_PROTOCOL *) Description;
  if ((UINTN) FilePathList > (MAX_ADDRESS - FilePathListLength)) {
    return EFI_INVALID_PARAMETER;
  }
  if (((UINTN) FilePathList + FilePathListLength) > ((UINTN) Data + DataSize)) {
    return EFI_INVALID_PARAMETER;
  }
  if (FilePathListLength < sizeof (EFI_DEVICE_PATH_PROTOCOL)) {
    return EFI_INVALID_PARAMETER;
  }
  if (!IsDevicePathValid (FilePathList, FilePathListLength)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Internal check for key option.

  @param[in] VariablePropery    Pointer to variable property.
  @param[in] DataSize           Data size.
  @param[in] Data               Pointer to data buffer.

  @retval EFI_SUCCESS           The SetVariable check result was success.
  @retval EFI_INVALID_PARAMETER The data buffer is not a valid key option.

**/
EFI_STATUS
EFIAPI
InternalVarCheckKeyOption (
  IN VAR_CHECK_VARIABLE_PROPERTY    *VariablePropery,
  IN UINTN                          DataSize,
  IN VOID                           *Data
  )
{
  if (((DataSize - sizeof (EFI_KEY_OPTION)) % sizeof (EFI_INPUT_KEY)) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Internal check for device path.

  @param[in] VariablePropery    Pointer to variable property.
  @param[in] DataSize           Data size.
  @param[in] Data               Pointer to data buffer.

  @retval EFI_SUCCESS           The SetVariable check result was success.
  @retval EFI_INVALID_PARAMETER The data buffer is not a valid device path.

**/
EFI_STATUS
EFIAPI
InternalVarCheckDevicePath (
  IN VAR_CHECK_VARIABLE_PROPERTY    *VariablePropery,
  IN UINTN                          DataSize,
  IN VOID                           *Data
  )
{
  if (!IsDevicePathValid ((EFI_DEVICE_PATH_PROTOCOL *) Data, DataSize)) {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}

/**
  Internal check for ASCII string.

  @param[in] VariablePropery    Pointer to variable property.
  @param[in] DataSize           Data size.
  @param[in] Data               Pointer to data buffer.

  @retval EFI_SUCCESS           The SetVariable check result was success.
  @retval EFI_INVALID_PARAMETER The data buffer is not a Null-terminated ASCII string.

**/
EFI_STATUS
EFIAPI
InternalVarCheckAsciiString (
  IN VAR_CHECK_VARIABLE_PROPERTY    *VariablePropery,
  IN UINTN                          DataSize,
  IN VOID                           *Data
  )
{
  CHAR8     *String;
  UINTN     Index;

  String = (CHAR8 *) Data;
  if (String[DataSize - 1] == '\0') {
    return EFI_SUCCESS;
  } else {
    for (Index = 1; Index < DataSize && (String[DataSize - 1 - Index] != '\0'); Index++);
    if (Index == DataSize) {
      return EFI_INVALID_PARAMETER;
    }
  }
  return EFI_SUCCESS;
}

/**
  Internal check for size array.

  @param[in] VariablePropery    Pointer to variable property.
  @param[in] DataSize           Data size.
  @param[in] Data               Pointer to data buffer.

  @retval EFI_SUCCESS           The SetVariable check result was success.
  @retval EFI_INVALID_PARAMETER The DataSize is not size array.

**/
EFI_STATUS
EFIAPI
InternalVarCheckSizeArray (
  IN VAR_CHECK_VARIABLE_PROPERTY    *VariablePropery,
  IN UINTN                          DataSize,
  IN VOID                           *Data
  )
{
  if ((DataSize % VariablePropery->MinSize) != 0) {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}

//
// To prevent name collisions with possible future globally defined variables,
// other internal firmware data variables that are not defined here must be
// saved with a unique VendorGuid other than EFI_GLOBAL_VARIABLE or
// any other GUID defined by the UEFI Specification. Implementations must
// only permit the creation of variables with a UEFI Specification-defined
// VendorGuid when these variables are documented in the UEFI Specification.
//
UEFI_DEFINED_VARIABLE_ENTRY mGlobalVariableList[] = {
  {
    EFI_LANG_CODES_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_BS_RT,
      1,
      MAX_UINTN
    },
    InternalVarCheckAsciiString
  },
  {
    EFI_LANG_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      1,
      MAX_UINTN
    },
    InternalVarCheckAsciiString
  },
  {
    EFI_TIME_OUT_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (UINT16),
      sizeof (UINT16)
    },
    NULL
  },
  {
    EFI_PLATFORM_LANG_CODES_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_BS_RT,
      1,
      MAX_UINTN
    },
    InternalVarCheckAsciiString
  },
  {
    EFI_PLATFORM_LANG_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      1,
      MAX_UINTN
    },
    InternalVarCheckAsciiString
  },
  {
    EFI_CON_IN_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (EFI_DEVICE_PATH_PROTOCOL),
      MAX_UINTN
    },
    InternalVarCheckDevicePath
  },
  {
    EFI_CON_OUT_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (EFI_DEVICE_PATH_PROTOCOL),
      MAX_UINTN
    },
    InternalVarCheckDevicePath
  },
  {
    EFI_ERR_OUT_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (EFI_DEVICE_PATH_PROTOCOL),
      MAX_UINTN
    },
    InternalVarCheckDevicePath
  },
  {
    EFI_CON_IN_DEV_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_BS_RT,
      sizeof (EFI_DEVICE_PATH_PROTOCOL),
      MAX_UINTN
    },
    InternalVarCheckDevicePath
  },
  {
    EFI_CON_OUT_DEV_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_BS_RT,
      sizeof (EFI_DEVICE_PATH_PROTOCOL),
      MAX_UINTN
    },
    InternalVarCheckDevicePath
  },
  {
    EFI_ERR_OUT_DEV_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_BS_RT,
      sizeof (EFI_DEVICE_PATH_PROTOCOL),
      MAX_UINTN
    },
    InternalVarCheckDevicePath
  },
  {
    EFI_BOOT_ORDER_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (UINT16),
      MAX_UINTN
    },
    InternalVarCheckSizeArray
  },
  {
    EFI_BOOT_NEXT_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (UINT16),
      sizeof (UINT16)
    },
    NULL
  },
  {
    EFI_BOOT_CURRENT_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_BS_RT,
      sizeof (UINT16),
      sizeof (UINT16)
    },
    NULL
  },
  {
    EFI_BOOT_OPTION_SUPPORT_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_BS_RT,
      sizeof (UINT32),
      sizeof (UINT32)
    },
    NULL
  },
  {
    EFI_DRIVER_ORDER_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (UINT16),
      MAX_UINTN
    },
    InternalVarCheckSizeArray
  },
  {
    EFI_SYS_PREP_ORDER_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (UINT16),
      MAX_UINTN
    },
    InternalVarCheckSizeArray
  },
  {
    EFI_HW_ERR_REC_SUPPORT_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (UINT16),
      sizeof (UINT16)
    },
    NULL
  },
  {
    EFI_SETUP_MODE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_BS_RT,
      sizeof (UINT8),
      sizeof (UINT8)
    },
    NULL
  },
  {
    EFI_KEY_EXCHANGE_KEY_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT_AT,
      1,
      MAX_UINTN
    },
    NULL
  },
  {
    EFI_PLATFORM_KEY_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT_AT,
      1,
      MAX_UINTN
    },
    NULL
  },
  {
    EFI_SIGNATURE_SUPPORT_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_BS_RT,
      sizeof (EFI_GUID),
      MAX_UINTN
    },
    InternalVarCheckSizeArray
  },
  {
    EFI_SECURE_BOOT_MODE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_BS_RT,
      sizeof (UINT8),
      sizeof (UINT8)
    },
    NULL
  },
  {
    EFI_KEK_DEFAULT_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_BS_RT,
      1,
      MAX_UINTN
    },
    NULL
  },
  {
    EFI_PK_DEFAULT_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_BS_RT,
      1,
      MAX_UINTN
    },
    NULL
  },
  {
    EFI_DB_DEFAULT_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_BS_RT,
      1,
      MAX_UINTN
    },
    NULL
  },
  {
    EFI_DBX_DEFAULT_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_BS_RT,
      1,
      MAX_UINTN
    },
    NULL
  },
  {
    EFI_DBT_DEFAULT_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_BS_RT,
      1,
      MAX_UINTN
    },
    NULL
  },
  {
    EFI_OS_INDICATIONS_SUPPORT_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_BS_RT,
      sizeof (UINT64),
      sizeof (UINT64)
    },
    NULL
  },
  {
    EFI_OS_INDICATIONS_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (UINT64),
      sizeof (UINT64)
    },
    NULL
  },
  {
    EFI_VENDOR_KEYS_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_BS_RT,
      sizeof (UINT8),
      sizeof (UINT8)
    },
    NULL
  },
};
UEFI_DEFINED_VARIABLE_ENTRY mGlobalVariableList2[] = {
  {
    L"Boot####",
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (UINT32) + sizeof (UINT16),
      MAX_UINTN
    },
    InternalVarCheckLoadOption
  },
  {
    L"Driver####",
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (UINT32) + sizeof (UINT16),
      MAX_UINTN
    },
    InternalVarCheckLoadOption
  },
  {
    L"SysPrep####",
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (UINT32) + sizeof (UINT16),
      MAX_UINTN
    },
    InternalVarCheckLoadOption
  },
  {
    L"Key####",
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (EFI_KEY_OPTION),
      sizeof (EFI_KEY_OPTION) + 3 * sizeof (EFI_INPUT_KEY)
    },
    InternalVarCheckKeyOption
  },
};

//
// EFI_IMAGE_SECURITY_DATABASE_GUID
//
UEFI_DEFINED_VARIABLE_ENTRY mImageSecurityVariableList[] = {
  {
    EFI_IMAGE_SECURITY_DATABASE,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT_AT,
      1,
      MAX_UINTN
    },
    NULL
  },
  {
    EFI_IMAGE_SECURITY_DATABASE1,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT_AT,
      1,
      MAX_UINTN
    },
    NULL
  },
  {
    EFI_IMAGE_SECURITY_DATABASE2,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS_RT_AT,
      1,
      MAX_UINTN
    },
    NULL
  },
};

/**
  Get UEFI defined global variable or image security database variable property.
  The code will check if variable guid is global variable or image security database guid first.
  If yes, further check if variable name is in mGlobalVariableList, mGlobalVariableList2 or mImageSecurityVariableList.

  @param[in]  VariableName      Pointer to variable name.
  @param[in]  VendorGuid        Variable Vendor Guid.
  @param[in]  WildcardMatch     Try wildcard match or not.
  @param[out] VariableProperty  Pointer to variable property.
  @param[out] VarCheckFunction  Pointer to check function.

  @retval EFI_SUCCESS           Variable is not global variable or image security database variable.
  @retval EFI_INVALID_PARAMETER Variable is global variable or image security database variable, but variable name is not in the lists.

**/
EFI_STATUS
GetUefiDefinedVariableProperty (
  IN CHAR16                         *VariableName,
  IN EFI_GUID                       *VendorGuid,
  IN BOOLEAN                        WildcardMatch,
  OUT VAR_CHECK_VARIABLE_PROPERTY   **VariableProperty,
  OUT INTERNAL_VAR_CHECK_FUNCTION   *VarCheckFunction OPTIONAL
  )
{
  UINTN     Index;
  UINTN     NameLength;

  if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid)) {
    //
    // Try list 1, exactly match.
    //
    for (Index = 0; Index < sizeof (mGlobalVariableList)/sizeof (mGlobalVariableList[0]); Index++) {
      if (StrCmp (mGlobalVariableList[Index].Name, VariableName) == 0) {
        if (VarCheckFunction != NULL) {
          *VarCheckFunction = mGlobalVariableList[Index].CheckFunction;
        }
        *VariableProperty = &mGlobalVariableList[Index].VariableProperty;
        return EFI_SUCCESS;
      }
    }

    //
    // Try list 2.
    //
    NameLength = StrLen (VariableName) - 4;
    for (Index = 0; Index < sizeof (mGlobalVariableList2)/sizeof (mGlobalVariableList2[0]); Index++) {
      if (WildcardMatch) {
        if ((StrLen (VariableName) == StrLen (mGlobalVariableList2[Index].Name)) &&
            (StrnCmp (mGlobalVariableList2[Index].Name, VariableName, NameLength) == 0) &&
            IsHexaDecimalDigitCharacter (VariableName[NameLength]) &&
            IsHexaDecimalDigitCharacter (VariableName[NameLength + 1]) &&
            IsHexaDecimalDigitCharacter (VariableName[NameLength + 2]) &&
            IsHexaDecimalDigitCharacter (VariableName[NameLength + 3])) {
          if (VarCheckFunction != NULL) {
            *VarCheckFunction = mGlobalVariableList2[Index].CheckFunction;
          }
          *VariableProperty = &mGlobalVariableList2[Index].VariableProperty;
          return EFI_SUCCESS;
        }
      }
      if (StrCmp (mGlobalVariableList2[Index].Name, VariableName) == 0) {
        if (VarCheckFunction != NULL) {
          *VarCheckFunction = mGlobalVariableList2[Index].CheckFunction;
        }
        *VariableProperty = &mGlobalVariableList2[Index].VariableProperty;
        return EFI_SUCCESS;
      }
    }

    //
    // The variable name is not in the lists.
    //
    return EFI_INVALID_PARAMETER;
  }

  if (CompareGuid (VendorGuid, &gEfiImageSecurityDatabaseGuid)) {
    for (Index = 0; Index < sizeof (mImageSecurityVariableList)/sizeof (mImageSecurityVariableList[0]); Index++) {
      if (StrCmp (mImageSecurityVariableList[Index].Name, VariableName) == 0) {
        if (VarCheckFunction != NULL) {
          *VarCheckFunction = mImageSecurityVariableList[Index].CheckFunction;
        }
        *VariableProperty = &mImageSecurityVariableList[Index].VariableProperty;
        return EFI_SUCCESS;
      }
    }

    return EFI_INVALID_PARAMETER;
  }

  //
  // It is not global variable, image security database variable.
  //
  return EFI_SUCCESS;
}

/**
  Internal SetVariable check.

  @param[in] VariableName       Name of Variable to set.
  @param[in] VendorGuid         Variable vendor GUID.
  @param[in] Attributes         Attribute value of the variable.
  @param[in] DataSize           Size of Data to set.
  @param[in] Data               Data pointer.

  @retval EFI_SUCCESS           The SetVariable check result was success.
  @retval EFI_INVALID_PARAMETER An invalid combination of attribute bits, name, and GUID was supplied,
                                or the DataSize exceeds the minimum or maximum allowed,
                                or the Data value is not following UEFI spec for UEFI defined variables.
  @retval EFI_WRITE_PROTECTED   The variable in question is read-only.
  @retval Others                The return status from check handler.

**/
EFI_STATUS
EFIAPI
InternalVarCheckSetVariableCheck (
  IN CHAR16     *VariableName,
  IN EFI_GUID   *VendorGuid,
  IN UINT32     Attributes,
  IN UINTN      DataSize,
  IN VOID       *Data
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  LIST_ENTRY                    *Link;
  VAR_CHECK_VARIABLE_ENTRY      *Entry;
  CHAR16                        *Name;
  VAR_CHECK_VARIABLE_PROPERTY   *Property;
  INTERNAL_VAR_CHECK_FUNCTION   VarCheckFunction;

  if (!mEndOfDxe) {
    //
    // Only do check after End Of Dxe.
    //
    return EFI_SUCCESS;
  }

  Property = NULL;
  VarCheckFunction = NULL;

  for ( Link = GetFirstNode (&mVarCheckVariableList)
      ; !IsNull (&mVarCheckVariableList, Link)
      ; Link = GetNextNode (&mVarCheckVariableList, Link)
      ) {
    Entry = BASE_CR (Link, VAR_CHECK_VARIABLE_ENTRY, Link);
    Name = (CHAR16 *) ((UINTN) Entry + sizeof (*Entry));
    if (CompareGuid (&Entry->Guid, VendorGuid) && (StrCmp (Name, VariableName) == 0)) {
      Property = &Entry->VariableProperty;
      break;
    }
  }
  if (Property == NULL) {
    Status = GetUefiDefinedVariableProperty (VariableName, VendorGuid, TRUE, &Property, &VarCheckFunction);
    if (EFI_ERROR (Status)) {
      //
      // To prevent name collisions with possible future globally defined variables,
      // other internal firmware data variables that are not defined here must be
      // saved with a unique VendorGuid other than EFI_GLOBAL_VARIABLE or
      // any other GUID defined by the UEFI Specification. Implementations must
      // only permit the creation of variables with a UEFI Specification-defined
      // VendorGuid when these variables are documented in the UEFI Specification.
      //
      DEBUG ((EFI_D_INFO, "Variable Check UEFI defined variable fail %r - %s not in %g namespace\n", Status, VariableName, VendorGuid));
      return Status;
    }
  }
  if (Property != NULL) {
    if (mEnableLocking && ((Property->Property & VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY) != 0)) {
      DEBUG ((EFI_D_INFO, "Variable Check ReadOnly variable fail %r - %g:%s\n", EFI_WRITE_PROTECTED, VendorGuid, VariableName));
      return EFI_WRITE_PROTECTED;
    }
    if (!((((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) && (DataSize == 0)) || (Attributes == 0))) {
      //
      // Not to delete variable.
      //
      if ((Property->Attributes != 0) && ((Attributes & (~EFI_VARIABLE_APPEND_WRITE)) != Property->Attributes)) {
        DEBUG ((EFI_D_INFO, "Variable Check Attributes(0x%08x to 0x%08x) fail %r - %g:%s\n", Property->Attributes, Attributes, EFI_INVALID_PARAMETER, VendorGuid, VariableName));
        return EFI_INVALID_PARAMETER;
      }
      if (DataSize != 0) {
        if ((DataSize < Property->MinSize) || (DataSize > Property->MaxSize)) {
          DEBUG ((EFI_D_INFO, "Variable Check DataSize fail(0x%x not in 0x%x - 0x%x) %r - %g:%s\n", DataSize, Property->MinSize, Property->MaxSize, EFI_INVALID_PARAMETER, VendorGuid, VariableName));
          return EFI_INVALID_PARAMETER;
        }
        if (VarCheckFunction != NULL) {
          Status = VarCheckFunction (
                     Property,
                     DataSize,
                     Data
                     );
          if (EFI_ERROR (Status)) {
            DEBUG ((EFI_D_INFO, "Internal Variable Check function fail %r - %g:%s\n", Status, VendorGuid, VariableName));
            return Status;
          }
        }
      }
    }
  }

  for (Index = 0; Index < mNumberOfHandler; Index++) {
    Status = mHandlerTable[Index] (
               VariableName,
               VendorGuid,
               Attributes,
               DataSize,
               Data
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "Variable Check handler fail %r - %g:%s\n", Status, VendorGuid, VariableName));
      return Status;
    }
  }
  return EFI_SUCCESS;
}

/**
  Reallocates more global memory to store the registered handler list.

  @retval RETURN_SUCCESS            Reallocate memory successfully.
  @retval RETURN_OUT_OF_RESOURCES   No enough memory to allocate.

**/
RETURN_STATUS
EFIAPI
ReallocateHandlerTable (
  VOID
  )
{
  VAR_CHECK_SET_VARIABLE_CHECK_HANDLER  *HandlerTable;

  //
  // Reallocate memory for check handler table.
  //
  HandlerTable = ReallocateRuntimePool (
                     mMaxNumberOfHandler * sizeof (VAR_CHECK_SET_VARIABLE_CHECK_HANDLER),
                     (mMaxNumberOfHandler + VAR_CHECK_HANDLER_TABLE_SIZE) * sizeof (VAR_CHECK_SET_VARIABLE_CHECK_HANDLER),
                     mHandlerTable
                     );

  //
  // No enough resource to allocate.
  //
  if (HandlerTable == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  mHandlerTable = HandlerTable;
  //
  // Increase max handler number.
  //
  mMaxNumberOfHandler = mMaxNumberOfHandler + VAR_CHECK_HANDLER_TABLE_SIZE;
  return RETURN_SUCCESS;
}

/**
  Register SetVariable check handler.

  @param[in] Handler            Pointer to check handler.

  @retval EFI_SUCCESS           The SetVariable check handler was registered successfully.
  @retval EFI_INVALID_PARAMETER Handler is NULL.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the SetVariable check handler register request.
  @retval EFI_UNSUPPORTED       This interface is not implemented.
                                For example, it is unsupported in VarCheck protocol if both VarCheck and SmmVarCheck protocols are present.

**/
EFI_STATUS
EFIAPI
VarCheckRegisterSetVariableCheckHandler (
  IN VAR_CHECK_SET_VARIABLE_CHECK_HANDLER   Handler
  )
{
  EFI_STATUS    Status;

  if (Handler == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mEndOfDxe) {
    return EFI_ACCESS_DENIED;
  }

  DEBUG ((EFI_D_INFO, "RegisterSetVariableCheckHandler - 0x%x\n", Handler));

  Status = EFI_SUCCESS;

  AcquireLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  //
  // Check whether the handler list is enough to store new handler.
  //
  if (mNumberOfHandler == mMaxNumberOfHandler) {
    //
    // Allocate more resources for new handler.
    //
    Status = ReallocateHandlerTable();
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

  //
  // Register new handler into the handler list.
  //
  mHandlerTable[mNumberOfHandler] = Handler;
  mNumberOfHandler++;

Done:
  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  return Status;
}

/**
  Variable property get function.

  @param[in] Name           Pointer to the variable name.
  @param[in] Guid           Pointer to the vendor GUID.
  @param[in] WildcardMatch  Try wildcard match or not.

  @return Pointer to the property of variable specified by the Name and Guid.

**/
VAR_CHECK_VARIABLE_PROPERTY *
VariablePropertyGetFunction (
  IN CHAR16                 *Name,
  IN EFI_GUID               *Guid,
  IN BOOLEAN                WildcardMatch
  )
{
  LIST_ENTRY                    *Link;
  VAR_CHECK_VARIABLE_ENTRY      *Entry;
  CHAR16                        *VariableName;
  VAR_CHECK_VARIABLE_PROPERTY   *Property;

  Property = NULL;

  for ( Link = GetFirstNode (&mVarCheckVariableList)
      ; !IsNull (&mVarCheckVariableList, Link)
      ; Link = GetNextNode (&mVarCheckVariableList, Link)
      ) {
    Entry = BASE_CR (Link, VAR_CHECK_VARIABLE_ENTRY, Link);
    VariableName = (CHAR16 *) ((UINTN) Entry + sizeof (*Entry));
    if (CompareGuid (&Entry->Guid, Guid) && (StrCmp (VariableName, Name) == 0)) {
      return &Entry->VariableProperty;
    }
  }

  GetUefiDefinedVariableProperty (Name, Guid, WildcardMatch, &Property, NULL);

  return Property;
}

/**
  Internal variable property set.

  @param[in] Name               Pointer to the variable name.
  @param[in] Guid               Pointer to the vendor GUID.
  @param[in] VariableProperty   Pointer to the input variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was set successfully.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the variable property set request.

**/
EFI_STATUS
EFIAPI
InternalVarCheckVariablePropertySet (
  IN CHAR16                         *Name,
  IN EFI_GUID                       *Guid,
  IN VAR_CHECK_VARIABLE_PROPERTY    *VariableProperty
  )
{
  EFI_STATUS                    Status;
  VAR_CHECK_VARIABLE_ENTRY      *Entry;
  CHAR16                        *VariableName;
  VAR_CHECK_VARIABLE_PROPERTY   *Property;

  Status = EFI_SUCCESS;

  Property = VariablePropertyGetFunction (Name, Guid, FALSE);
  if (Property != NULL) {
    CopyMem (Property, VariableProperty, sizeof (*VariableProperty));
  } else {
    Entry = AllocateRuntimeZeroPool (sizeof (*Entry) + StrSize (Name));
    if (Entry == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }
    VariableName = (CHAR16 *) ((UINTN) Entry + sizeof (*Entry));
    StrCpyS (VariableName, StrSize(Name)/sizeof(CHAR16), Name);
    CopyGuid (&Entry->Guid, Guid);
    CopyMem (&Entry->VariableProperty, VariableProperty, sizeof (*VariableProperty));
    InsertTailList (&mVarCheckVariableList, &Entry->Link);
  }

Done:
  return Status;
}

/**
  Variable property set.

  @param[in] Name               Pointer to the variable name.
  @param[in] Guid               Pointer to the vendor GUID.
  @param[in] VariableProperty   Pointer to the input variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was set successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string,
                                or the fields of VariableProperty are not valid.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the variable property set request.

**/
EFI_STATUS
EFIAPI
VarCheckVariablePropertySet (
  IN CHAR16                         *Name,
  IN EFI_GUID                       *Guid,
  IN VAR_CHECK_VARIABLE_PROPERTY    *VariableProperty
  )
{
  EFI_STATUS                    Status;

  if (Name == NULL || Name[0] == 0 || Guid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableProperty == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableProperty->Revision != VAR_CHECK_VARIABLE_PROPERTY_REVISION) {
    return EFI_INVALID_PARAMETER;
  }

  if (mEndOfDxe) {
    return EFI_ACCESS_DENIED;
  }

  AcquireLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  Status = InternalVarCheckVariablePropertySet (Name, Guid, VariableProperty);

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  return Status;
}

/**
  Internal variable property get.

  @param[in]  Name              Pointer to the variable name.
  @param[in]  Guid              Pointer to the vendor GUID.
  @param[out] VariableProperty  Pointer to the output variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was got successfully.
  @retval EFI_NOT_FOUND         The property of variable specified by the Name and Guid was not found.

**/
EFI_STATUS
EFIAPI
InternalVarCheckVariablePropertyGet (
  IN CHAR16                         *Name,
  IN EFI_GUID                       *Guid,
  OUT VAR_CHECK_VARIABLE_PROPERTY   *VariableProperty
  )
{
  LIST_ENTRY                    *Link;
  VARIABLE_ENTRY                *Entry;
  CHAR16                        *VariableName;
  BOOLEAN                       Found;
  VAR_CHECK_VARIABLE_PROPERTY   *Property;

  Found = FALSE;

  Property = VariablePropertyGetFunction (Name, Guid, TRUE);
  if (Property != NULL) {
    CopyMem (VariableProperty, Property, sizeof (*VariableProperty));
    Found = TRUE;
  }

  for ( Link = GetFirstNode (&mLockedVariableList)
      ; !IsNull (&mLockedVariableList, Link)
      ; Link = GetNextNode (&mLockedVariableList, Link)
      ) {
    Entry = BASE_CR (Link, VARIABLE_ENTRY, Link);
    VariableName = (CHAR16 *) ((UINTN) Entry + sizeof (*Entry));
    if (CompareGuid (&Entry->Guid, Guid) && (StrCmp (VariableName, Name) == 0)) {
      VariableProperty->Property |= VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY;
      if (!Found) {
        VariableProperty->Revision = VAR_CHECK_VARIABLE_PROPERTY_REVISION;
        Found = TRUE;
      }
    }
  }

  return (Found ? EFI_SUCCESS : EFI_NOT_FOUND);
}

/**
  Variable property get.

  @param[in]  Name              Pointer to the variable name.
  @param[in]  Guid              Pointer to the vendor GUID.
  @param[out] VariableProperty  Pointer to the output variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was got successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string.
  @retval EFI_NOT_FOUND         The property of variable specified by the Name and Guid was not found.

**/
EFI_STATUS
EFIAPI
VarCheckVariablePropertyGet (
  IN CHAR16                         *Name,
  IN EFI_GUID                       *Guid,
  OUT VAR_CHECK_VARIABLE_PROPERTY   *VariableProperty
  )
{
  EFI_STATUS    Status;

  if (Name == NULL || Name[0] == 0 || Guid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableProperty == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  Status = InternalVarCheckVariablePropertyGet (Name, Guid, VariableProperty);

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  return Status;
}


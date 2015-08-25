/** @file
  Implementation functions and structures for var check uefi library.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/VarCheckLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>

#include <Guid/VariableFormat.h>
#include <Guid/GlobalVariable.h>
#include <Guid/HardwareErrorVariable.h>
#include <Guid/ImageAuthentication.h>

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

//
// EFI_HARDWARE_ERROR_VARIABLE
//
UEFI_DEFINED_VARIABLE_ENTRY mHwErrRecVariable = {
  L"HwErrRec####",
  {
    VAR_CHECK_VARIABLE_PROPERTY_REVISION,
    0,
    VARIABLE_ATTRIBUTE_NV_BS_RT_HR,
    1,
    MAX_UINTN
  },
  NULL
};

EFI_GUID *mUefiDefinedGuid[] = {
  &gEfiGlobalVariableGuid,
  &gEfiImageSecurityDatabaseGuid,
  &gEfiHardwareErrorVariableGuid
};

/**
  Check if a Unicode character is a hexadecimal character.

  This function checks if a Unicode character is a
  hexadecimal character.  The valid hexadecimal character is
  L'0' to L'9', L'a' to L'f', or L'A' to L'F'.


  @param[in] Char       The character to check against.

  @retval TRUE          If the Char is a hexadecmial character.
  @retval FALSE         If the Char is not a hexadecmial character.

**/
BOOLEAN
EFIAPI
VarCheckUefiIsHexaDecimalDigitCharacter (
  IN CHAR16             Char
  )
{
  return (BOOLEAN) ((Char >= L'0' && Char <= L'9') || (Char >= L'A' && Char <= L'F') || (Char >= L'a' && Char <= L'f'));
}

/**

  This code checks if variable is hardware error record variable or not.

  According to UEFI spec, hardware error record variable should use the EFI_HARDWARE_ERROR_VARIABLE VendorGuid
  and have the L"HwErrRec####" name convention, #### is a printed hex value and no 0x or h is included in the hex value.

  @param[in] VariableName   Pointer to variable name.
  @param[in] VendorGuid     Variable Vendor Guid.

  @retval TRUE              Variable is hardware error record variable.
  @retval FALSE             Variable is not hardware error record variable.

**/
BOOLEAN
EFIAPI
IsHwErrRecVariable (
  IN CHAR16             *VariableName,
  IN EFI_GUID           *VendorGuid
  )
{
  if (!CompareGuid (VendorGuid, &gEfiHardwareErrorVariableGuid) ||
      (StrLen (VariableName) != StrLen (L"HwErrRec####")) ||
      (StrnCmp(VariableName, L"HwErrRec", StrLen (L"HwErrRec")) != 0) ||
      !VarCheckUefiIsHexaDecimalDigitCharacter (VariableName[0x8]) ||
      !VarCheckUefiIsHexaDecimalDigitCharacter (VariableName[0x9]) ||
      !VarCheckUefiIsHexaDecimalDigitCharacter (VariableName[0xA]) ||
      !VarCheckUefiIsHexaDecimalDigitCharacter (VariableName[0xB])) {
    return FALSE;
  }

  return TRUE;
}

/**
  Get UEFI defined var check function.

  @param[in]  VariableName      Pointer to variable name.
  @param[in]  VendorGuid        Pointer to variable vendor GUID.
  @param[out] VariableProperty  Pointer to variable property.

  @return Internal var check function, NULL if no specific check function.

**/
INTERNAL_VAR_CHECK_FUNCTION
GetUefiDefinedVarCheckFunction (
  IN CHAR16                         *VariableName,
  IN EFI_GUID                       *VendorGuid,
  OUT VAR_CHECK_VARIABLE_PROPERTY   **VariableProperty
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
        *VariableProperty = &(mGlobalVariableList[Index].VariableProperty);
        return mGlobalVariableList[Index].CheckFunction;
      }
    }

    //
    // Try list 2.
    //
    NameLength = StrLen (VariableName) - 4;
    for (Index = 0; Index < sizeof (mGlobalVariableList2)/sizeof (mGlobalVariableList2[0]); Index++) {
      if ((StrLen (VariableName) == StrLen (mGlobalVariableList2[Index].Name)) &&
          (StrnCmp (VariableName, mGlobalVariableList2[Index].Name, NameLength) == 0) &&
          VarCheckUefiIsHexaDecimalDigitCharacter (VariableName[NameLength]) &&
          VarCheckUefiIsHexaDecimalDigitCharacter (VariableName[NameLength + 1]) &&
          VarCheckUefiIsHexaDecimalDigitCharacter (VariableName[NameLength + 2]) &&
          VarCheckUefiIsHexaDecimalDigitCharacter (VariableName[NameLength + 3])) {
        *VariableProperty = &(mGlobalVariableList2[Index].VariableProperty);
        return mGlobalVariableList2[Index].CheckFunction;
      }
    }
  }

  return NULL;
}

/**
  SetVariable check handler UEFI defined.

  @param[in] VariableName       Name of Variable to set.
  @param[in] VendorGuid         Variable vendor GUID.
  @param[in] Attributes         Attribute value of the variable.
  @param[in] DataSize           Size of Data to set.
  @param[in] Data               Data pointer.

  @retval EFI_SUCCESS           The SetVariable check result was success.
  @retval EFI_INVALID_PARAMETER An invalid combination of attribute bits, name, GUID,
                                DataSize and Data value was supplied.
  @retval EFI_WRITE_PROTECTED   The variable in question is read-only.

**/
EFI_STATUS
EFIAPI
SetVariableCheckHandlerUefiDefined (
  IN CHAR16     *VariableName,
  IN EFI_GUID   *VendorGuid,
  IN UINT32     Attributes,
  IN UINTN      DataSize,
  IN VOID       *Data
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  VAR_CHECK_VARIABLE_PROPERTY   Property;
  VAR_CHECK_VARIABLE_PROPERTY   *VarCheckProperty;
  INTERNAL_VAR_CHECK_FUNCTION   VarCheckFunction;

  if ((((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) && (DataSize == 0)) || (Attributes == 0)) {
    //
    // Do not check delete variable.
    //
    return EFI_SUCCESS;
  }

  if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    if (!IsHwErrRecVariable (VariableName, VendorGuid)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  for (Index = 0; Index < sizeof (mUefiDefinedGuid)/sizeof (mUefiDefinedGuid[0]); Index++) {
    if (CompareGuid (VendorGuid, mUefiDefinedGuid[Index])) {
      if (VarCheckLibVariablePropertyGet (VariableName, VendorGuid, &Property) == EFI_NOT_FOUND) {
        //
        // To prevent name collisions with possible future globally defined variables,
        // other internal firmware data variables that are not defined here must be
        // saved with a unique VendorGuid other than EFI_GLOBAL_VARIABLE or
        // any other GUID defined by the UEFI Specification. Implementations must
        // only permit the creation of variables with a UEFI Specification-defined
        // VendorGuid when these variables are documented in the UEFI Specification.
        //
        DEBUG ((EFI_D_INFO, "UEFI Variable Check fail %r - %s not in %g namespace\n", EFI_INVALID_PARAMETER, VariableName, VendorGuid));
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  if (DataSize == 0) {
    return EFI_SUCCESS;
  }

  VarCheckProperty = NULL;
  VarCheckFunction = GetUefiDefinedVarCheckFunction (VariableName, VendorGuid, &VarCheckProperty);
  if (VarCheckFunction != NULL) {
    Status = VarCheckFunction (
               VarCheckProperty,
               DataSize,
               Data
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "UEFI Variable Check function fail %r - %g:%s\n", Status, VendorGuid, VariableName));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Variable property set for UEFI defined variables.

**/
VOID
VariablePropertySetUefiDefined (
  VOID
  )
{
  UINTN     Index;

  //
  // EFI_GLOBAL_VARIABLE
  //
  for (Index = 0; Index < sizeof (mGlobalVariableList)/sizeof (mGlobalVariableList[0]); Index++) {
    VarCheckLibVariablePropertySet (
      mGlobalVariableList[Index].Name,
      &gEfiGlobalVariableGuid,
      &mGlobalVariableList[Index].VariableProperty
      );
  }
  for (Index = 0; Index < sizeof (mGlobalVariableList2)/sizeof (mGlobalVariableList2[0]); Index++) {
    VarCheckLibVariablePropertySet (
      mGlobalVariableList2[Index].Name,
      &gEfiGlobalVariableGuid,
      &mGlobalVariableList2[Index].VariableProperty
      );
  }

  //
  // EFI_IMAGE_SECURITY_DATABASE_GUID
  //
  for (Index = 0; Index < sizeof (mImageSecurityVariableList)/sizeof (mImageSecurityVariableList[0]); Index++) {
    VarCheckLibVariablePropertySet (
      mImageSecurityVariableList[Index].Name,
      &gEfiImageSecurityDatabaseGuid,
      &mImageSecurityVariableList[Index].VariableProperty
      );
  }

  //
  // EFI_HARDWARE_ERROR_VARIABLE
  //
  VarCheckLibVariablePropertySet (
    mHwErrRecVariable.Name,
    &gEfiHardwareErrorVariableGuid,
    &mHwErrRecVariable.VariableProperty
    );
}

/**
  Constructor function of VarCheckUefiLib to set property and
  register SetVariable check handler for UEFI defined variables.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The constructor executed correctly.

**/
EFI_STATUS
EFIAPI
VarCheckUefiLibNullClassConstructor (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  )
{
  VariablePropertySetUefiDefined ();
  VarCheckLibRegisterSetVariableCheckHandler (SetVariableCheckHandlerUefiDefined);

  return EFI_SUCCESS;
}

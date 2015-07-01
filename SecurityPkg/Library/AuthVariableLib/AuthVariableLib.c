/** @file
  Implement authentication services for the authenticated variables.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable data. It may be input in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.
  Variable attribute should also be checked to avoid authentication bypass.
     The whole SMM authentication variable design relies on the integrity of flash part and SMM.
  which is assumed to be protected by platform.  All variable code and metadata in flash/SMM Memory
  may not be modified without authorization. If platform fails to protect these resources,
  the authentication service provided in this driver will be broken, and the behavior is undefined.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AuthServiceInternal.h"

///
/// Global database array for scratch
///
UINT8    *mPubKeyStore;
UINT32   mPubKeyNumber;
UINT32   mMaxKeyNumber;
UINT32   mMaxKeyDbSize;
UINT8    *mCertDbStore;
UINT32   mMaxCertDbSize;
UINT32   mPlatformMode;
UINT8    mVendorKeyState;

EFI_GUID mSignatureSupport[] = {EFI_CERT_SHA1_GUID, EFI_CERT_SHA256_GUID, EFI_CERT_RSA2048_GUID, EFI_CERT_X509_GUID};

//
// Hash context pointer
//
VOID  *mHashCtx = NULL;

VARIABLE_ENTRY_PROPERTY mAuthVarEntry[] = {
  {
    &gEfiSecureBootEnableDisableGuid,
    EFI_SECURE_BOOT_ENABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS,
      sizeof (UINT8),
      sizeof (UINT8)
    }
  },
  {
    &gEfiCustomModeEnableGuid,
    EFI_CUSTOM_MODE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      0,
      VARIABLE_ATTRIBUTE_NV_BS,
      sizeof (UINT8),
      sizeof (UINT8)
    }
  },
  {
    &gEfiVendorKeysNvGuid,
    EFI_VENDOR_KEYS_NV_VARIABLE_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_NV_BS_RT_AT,
      sizeof (UINT8),
      sizeof (UINT8)
    }
  },
  {
    &gEfiAuthenticatedVariableGuid,
    AUTHVAR_KEYDB_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_NV_BS_RT_AW,
      sizeof (UINT8),
      MAX_UINTN
    }
  },
  {
    &gEfiCertDbGuid,
    EFI_CERT_DB_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_NV_BS_RT_AT,
      sizeof (UINT32),
      MAX_UINTN
    }
  },
};

VOID *mAddressPointer[3];

AUTH_VAR_LIB_CONTEXT_IN *mAuthVarLibContextIn = NULL;

/**
  Initialization for authenticated varibale services.
  If this initialization returns error status, other APIs will not work
  and expect to be not called then.

  @param[in]  AuthVarLibContextIn   Pointer to input auth variable lib context.
  @param[out] AuthVarLibContextOut  Pointer to output auth variable lib context.

  @retval EFI_SUCCESS               Function successfully executed.
  @retval EFI_INVALID_PARAMETER     If AuthVarLibContextIn == NULL or AuthVarLibContextOut == NULL.
  @retval EFI_OUT_OF_RESOURCES      Fail to allocate enough resource.
  @retval EFI_UNSUPPORTED           Unsupported to process authenticated variable.

**/
EFI_STATUS
EFIAPI
AuthVariableLibInitialize (
  IN  AUTH_VAR_LIB_CONTEXT_IN   *AuthVarLibContextIn,
  OUT AUTH_VAR_LIB_CONTEXT_OUT  *AuthVarLibContextOut
  )
{
  EFI_STATUS            Status;
  UINT8                 VarValue;
  UINT32                VarAttr;
  UINT8                 *Data;
  UINTN                 DataSize;
  UINTN                 CtxSize;
  UINT8                 SecureBootMode;
  UINT8                 SecureBootEnable;
  UINT8                 CustomMode;
  UINT32                ListSize;

  if ((AuthVarLibContextIn == NULL) || (AuthVarLibContextOut == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  mAuthVarLibContextIn = AuthVarLibContextIn;

  //
  // Initialize hash context.
  //
  CtxSize   = Sha256GetContextSize ();
  mHashCtx  = AllocateRuntimePool (CtxSize);
  if (mHashCtx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Reserve runtime buffer for public key database. The size excludes variable header and name size.
  //
  mMaxKeyDbSize = (UINT32) (mAuthVarLibContextIn->MaxAuthVariableSize - sizeof (AUTHVAR_KEYDB_NAME));
  mMaxKeyNumber = mMaxKeyDbSize / sizeof (AUTHVAR_KEY_DB_DATA);
  mPubKeyStore  = AllocateRuntimePool (mMaxKeyDbSize);
  if (mPubKeyStore == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Reserve runtime buffer for certificate database. The size excludes variable header and name size.
  //
  mMaxCertDbSize = (UINT32) (mAuthVarLibContextIn->MaxAuthVariableSize - sizeof (EFI_CERT_DB_NAME));
  mCertDbStore   = AllocateRuntimePool (mMaxCertDbSize);
  if (mCertDbStore == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Check "AuthVarKeyDatabase" variable's existence.
  // If it doesn't exist, create a new one with initial value of 0 and EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set.
  //
  Status = AuthServiceInternalFindVariable (
             AUTHVAR_KEYDB_NAME,
             &gEfiAuthenticatedVariableGuid,
             (VOID **) &Data,
             &DataSize
             );
  if (EFI_ERROR (Status)) {
    VarAttr       = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
    VarValue      = 0;
    mPubKeyNumber = 0;
    Status        = AuthServiceInternalUpdateVariable (
                      AUTHVAR_KEYDB_NAME,
                      &gEfiAuthenticatedVariableGuid,
                      &VarValue,
                      sizeof(UINT8),
                      VarAttr
                      );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    //
    // Load database in global variable for cache.
    //
    ASSERT ((DataSize != 0) && (Data != NULL));
    //
    // "AuthVarKeyDatabase" is an internal variable. Its DataSize is always ensured not to exceed mPubKeyStore buffer size(See definition before)
    //  Therefore, there is no memory overflow in underlying CopyMem.
    //
    CopyMem (mPubKeyStore, (UINT8 *) Data, DataSize);
    mPubKeyNumber = (UINT32) (DataSize / sizeof (AUTHVAR_KEY_DB_DATA));
  }

  Status = AuthServiceInternalFindVariable (EFI_PLATFORM_KEY_NAME, &gEfiGlobalVariableGuid, (VOID **) &Data, &DataSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "Variable %s does not exist.\n", EFI_PLATFORM_KEY_NAME));
  } else {
    DEBUG ((EFI_D_INFO, "Variable %s exists.\n", EFI_PLATFORM_KEY_NAME));
  }

  //
  // Create "SetupMode" variable with BS+RT attribute set.
  //
  if (EFI_ERROR (Status)) {
    mPlatformMode = SETUP_MODE;
  } else {
    mPlatformMode = USER_MODE;
  }
  Status = AuthServiceInternalUpdateVariable (
             EFI_SETUP_MODE_NAME,
             &gEfiGlobalVariableGuid,
             &mPlatformMode,
             sizeof(UINT8),
             EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create "SignatureSupport" variable with BS+RT attribute set.
  //
  Status  = AuthServiceInternalUpdateVariable (
              EFI_SIGNATURE_SUPPORT_NAME,
              &gEfiGlobalVariableGuid,
              mSignatureSupport,
              sizeof(mSignatureSupport),
              EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
              );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // If "SecureBootEnable" variable exists, then update "SecureBoot" variable.
  // If "SecureBootEnable" variable is SECURE_BOOT_ENABLE and in USER_MODE, Set "SecureBoot" variable to SECURE_BOOT_MODE_ENABLE.
  // If "SecureBootEnable" variable is SECURE_BOOT_DISABLE, Set "SecureBoot" variable to SECURE_BOOT_MODE_DISABLE.
  //
  SecureBootEnable = SECURE_BOOT_DISABLE;
  Status = AuthServiceInternalFindVariable (EFI_SECURE_BOOT_ENABLE_NAME, &gEfiSecureBootEnableDisableGuid, (VOID **) &Data, &DataSize);
  if (!EFI_ERROR (Status)) {
    if (mPlatformMode == SETUP_MODE){
      //
      // PK is cleared in runtime. "SecureBootMode" is not updated before reboot
      // Delete "SecureBootMode" in SetupMode
      //
      Status = AuthServiceInternalUpdateVariable (
                 EFI_SECURE_BOOT_ENABLE_NAME,
                 &gEfiSecureBootEnableDisableGuid,
                 &SecureBootEnable,
                 0,
                 EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS
                 );
    } else {
      SecureBootEnable = *(UINT8 *) Data;
    }
  } else if (mPlatformMode == USER_MODE) {
    //
    // "SecureBootEnable" not exist, initialize it in USER_MODE.
    //
    SecureBootEnable = SECURE_BOOT_ENABLE;
    Status = AuthServiceInternalUpdateVariable (
               EFI_SECURE_BOOT_ENABLE_NAME,
               &gEfiSecureBootEnableDisableGuid,
               &SecureBootEnable,
               sizeof (UINT8),
               EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Create "SecureBoot" variable with BS+RT attribute set.
  //
  if (SecureBootEnable == SECURE_BOOT_ENABLE && mPlatformMode == USER_MODE) {
    SecureBootMode = SECURE_BOOT_MODE_ENABLE;
  } else {
    SecureBootMode = SECURE_BOOT_MODE_DISABLE;
  }
  Status = AuthServiceInternalUpdateVariable (
             EFI_SECURE_BOOT_MODE_NAME,
             &gEfiGlobalVariableGuid,
             &SecureBootMode,
             sizeof (UINT8),
             EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((EFI_D_INFO, "Variable %s is %x\n", EFI_SETUP_MODE_NAME, mPlatformMode));
  DEBUG ((EFI_D_INFO, "Variable %s is %x\n", EFI_SECURE_BOOT_MODE_NAME, SecureBootMode));
  DEBUG ((EFI_D_INFO, "Variable %s is %x\n", EFI_SECURE_BOOT_ENABLE_NAME, SecureBootEnable));

  //
  // Initialize "CustomMode" in STANDARD_SECURE_BOOT_MODE state.
  //
  CustomMode = STANDARD_SECURE_BOOT_MODE;
  Status = AuthServiceInternalUpdateVariable (
             EFI_CUSTOM_MODE_NAME,
             &gEfiCustomModeEnableGuid,
             &CustomMode,
             sizeof (UINT8),
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((EFI_D_INFO, "Variable %s is %x\n", EFI_CUSTOM_MODE_NAME, CustomMode));

  //
  // Check "certdb" variable's existence.
  // If it doesn't exist, then create a new one with
  // EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set.
  //
  Status = AuthServiceInternalFindVariable (
             EFI_CERT_DB_NAME,
             &gEfiCertDbGuid,
             (VOID **) &Data,
             &DataSize
             );
  if (EFI_ERROR (Status)) {
    VarAttr  = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
    ListSize = sizeof (UINT32);
    Status   = AuthServiceInternalUpdateVariable (
                 EFI_CERT_DB_NAME,
                 &gEfiCertDbGuid,
                 &ListSize,
                 sizeof (UINT32),
                 VarAttr
                 );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Check "VendorKeysNv" variable's existence and create "VendorKeys" variable accordingly.
  //
  Status = AuthServiceInternalFindVariable (EFI_VENDOR_KEYS_NV_VARIABLE_NAME, &gEfiVendorKeysNvGuid, (VOID **) &Data, &DataSize);
  if (!EFI_ERROR (Status)) {
    mVendorKeyState = *(UINT8 *)Data;
  } else {
    //
    // "VendorKeysNv" not exist, initialize it in VENDOR_KEYS_VALID state.
    //
    mVendorKeyState = VENDOR_KEYS_VALID;
    Status = AuthServiceInternalUpdateVariable (
               EFI_VENDOR_KEYS_NV_VARIABLE_NAME,
               &gEfiVendorKeysNvGuid,
               &mVendorKeyState,
               sizeof (UINT8),
               EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Create "VendorKeys" variable with BS+RT attribute set.
  //
  Status = AuthServiceInternalUpdateVariable (
             EFI_VENDOR_KEYS_VARIABLE_NAME,
             &gEfiGlobalVariableGuid,
             &mVendorKeyState,
             sizeof (UINT8),
             EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((EFI_D_INFO, "Variable %s is %x\n", EFI_VENDOR_KEYS_VARIABLE_NAME, mVendorKeyState));

  AuthVarLibContextOut->StructVersion = AUTH_VAR_LIB_CONTEXT_OUT_STRUCT_VERSION;
  AuthVarLibContextOut->StructSize = sizeof (AUTH_VAR_LIB_CONTEXT_OUT);
  AuthVarLibContextOut->AuthVarEntry = mAuthVarEntry;
  AuthVarLibContextOut->AuthVarEntryCount = sizeof (mAuthVarEntry) / sizeof (mAuthVarEntry[0]);
  mAddressPointer[0] = mHashCtx;
  mAddressPointer[1] = mPubKeyStore;
  mAddressPointer[2] = mCertDbStore;
  AuthVarLibContextOut->AddressPointer = mAddressPointer;
  AuthVarLibContextOut->AddressPointerCount = sizeof (mAddressPointer) / sizeof (mAddressPointer[0]);

  return Status;
}

/**
  Process variable with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS/EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set.

  @param[in] VariableName           Name of the variable.
  @param[in] VendorGuid             Variable vendor GUID.
  @param[in] Data                   Data pointer.
  @param[in] DataSize               Size of Data.
  @param[in] Attributes             Attribute value of the variable.

  @retval EFI_SUCCESS               The firmware has successfully stored the variable and its data as
                                    defined by the Attributes.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_WRITE_PROTECTED       Variable is write-protected.
  @retval EFI_OUT_OF_RESOURCES      There is not enough resource.
  @retval EFI_SECURITY_VIOLATION    The variable is with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS
                                    or EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS
                                    set, but the AuthInfo does NOT pass the validation
                                    check carried out by the firmware.
  @retval EFI_UNSUPPORTED           Unsupported to process authenticated variable.

**/
EFI_STATUS
EFIAPI
AuthVariableLibProcessVariable (
  IN CHAR16         *VariableName,
  IN EFI_GUID       *VendorGuid,
  IN VOID           *Data,
  IN UINTN          DataSize,
  IN UINT32         Attributes
  )
{
  EFI_STATUS        Status;

  if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) && (StrCmp (VariableName, EFI_PLATFORM_KEY_NAME) == 0)){
    Status = ProcessVarWithPk (VariableName, VendorGuid, Data, DataSize, Attributes, TRUE);
  } else if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) && (StrCmp (VariableName, EFI_KEY_EXCHANGE_KEY_NAME) == 0)) {
    Status = ProcessVarWithPk (VariableName, VendorGuid, Data, DataSize, Attributes, FALSE);
  } else if (CompareGuid (VendorGuid, &gEfiImageSecurityDatabaseGuid) &&
             ((StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE)  == 0) ||
              (StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE1) == 0) ||
              (StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE2) == 0)
             )) {
    Status = ProcessVarWithPk (VariableName, VendorGuid, Data, DataSize, Attributes, FALSE);
    if (EFI_ERROR (Status)) {
      Status = ProcessVarWithKek (VariableName, VendorGuid, Data, DataSize, Attributes);
    }
  } else {
    Status = ProcessVariable (VariableName, VendorGuid, Data, DataSize, Attributes);
  }

  return Status;
}

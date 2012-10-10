/** @file
  Implement authentication services for the authenticated variable
  service in UEFI2.2.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable data. It may be input in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.
  Variable attribute should also be checked to avoid authentication bypass.

  ProcessVarWithPk(), ProcessVarWithKek() and ProcessVariable() are the function to do
  variable authentication.

  VerifyTimeBasedPayload() and VerifyCounterBasedPayload() are sub function to do verification.
  They will do basic validation for authentication data structure, then call crypto library
  to verify the signature.

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Variable.h"
#include "AuthService.h"

///
/// Global database array for scratch
///
UINT8    mPubKeyStore[MAX_KEYDB_SIZE];
UINT32   mPubKeyNumber;
UINT32   mPlatformMode;
EFI_GUID mSignatureSupport[] = {EFI_CERT_SHA1_GUID, EFI_CERT_SHA256_GUID, EFI_CERT_RSA2048_GUID, EFI_CERT_X509_GUID};
//
// Public Exponent of RSA Key.
//
CONST UINT8 mRsaE[] = { 0x01, 0x00, 0x01 };
//
// Hash context pointer
//
VOID  *mHashCtx = NULL;

//
// Pointer to runtime buffer.
// For "Append" operation to an existing variable, a read/modify/write operation
// is supported by firmware internally. Reserve runtime buffer to cache previous
// variable data in runtime phase because memory allocation is forbidden in virtual mode.
//
VOID  *mStorageArea = NULL;

//
// The serialization of the values of the VariableName, VendorGuid and Attributes
// parameters of the SetVariable() call and the TimeStamp component of the
// EFI_VARIABLE_AUTHENTICATION_2 descriptor followed by the variable's new value
// i.e. (VariableName, VendorGuid, Attributes, TimeStamp, Data)
//
UINT8 *mSerializationRuntimeBuffer = NULL;

//
// Requirement for different signature type which have been defined in UEFI spec.
// These data are used to peform SignatureList format check while setting PK/KEK variable.
//
EFI_SIGNATURE_ITEM mSupportSigItem[] = {
//{SigType,                       SigHeaderSize,   SigDataSize  }
  {EFI_CERT_SHA256_GUID,          0,               32           },
  {EFI_CERT_RSA2048_GUID,         0,               256          },
  {EFI_CERT_RSA2048_SHA256_GUID,  0,               256          },
  {EFI_CERT_SHA1_GUID,            0,               20           },
  {EFI_CERT_RSA2048_SHA1_GUID,    0,               256          },
  {EFI_CERT_X509_GUID,            0,               ((UINT32) ~0)},
  {EFI_CERT_SHA224_GUID,          0,               28           },
  {EFI_CERT_SHA384_GUID,          0,               48           },
  {EFI_CERT_SHA512_GUID,          0,               64           }
};

/**
  Determine whether this operation needs a physical present user.

  @param[in]      VariableName            Name of the Variable.
  @param[in]      VendorGuid              GUID of the Variable.

  @retval TRUE      This variable is protected, only a physical present user could set this variable.
  @retval FALSE     This variable is not protected.
  
**/
BOOLEAN
NeedPhysicallyPresent(
  IN     CHAR16         *VariableName,
  IN     EFI_GUID       *VendorGuid
  )
{
  if ((CompareGuid (VendorGuid, &gEfiSecureBootEnableDisableGuid) && (StrCmp (VariableName, EFI_SECURE_BOOT_ENABLE_NAME) == 0))
    || (CompareGuid (VendorGuid, &gEfiCustomModeEnableGuid) && (StrCmp (VariableName, EFI_CUSTOM_MODE_NAME) == 0))) {
    return TRUE;
  }
  
  return FALSE;
}

/**
  Determine whether the platform is operating in Custom Secure Boot mode.

  @retval TRUE           The platform is operating in Custom mode.
  @retval FALSE          The platform is operating in Standard mode.

**/
BOOLEAN
InCustomMode (
  VOID
  )
{
  VARIABLE_POINTER_TRACK  Variable;

  FindVariable (EFI_CUSTOM_MODE_NAME, &gEfiCustomModeEnableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
  if (Variable.CurrPtr != NULL && *(GetVariableDataPtr (Variable.CurrPtr)) == CUSTOM_SECURE_BOOT_MODE) {
    return TRUE;
  }
  
  return FALSE;
}


/**
  Internal function to delete a Variable given its name and GUID, no authentication
  required.

  @param[in]      VariableName            Name of the Variable.
  @param[in]      VendorGuid              GUID of the Variable.

  @retval EFI_SUCCESS              Variable deleted successfully.
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
DeleteVariable (
  IN  CHAR16                    *VariableName,
  IN  EFI_GUID                  *VendorGuid
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;

  Status = FindVariable (VariableName, VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  ASSERT (Variable.CurrPtr != NULL);
  return UpdateVariable (VariableName, VendorGuid, NULL, 0, 0, 0, 0, &Variable, NULL);
}

/**
  Initializes for authenticated varibale service.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resources.

**/
EFI_STATUS
AutenticatedVariableServiceInitialize (
  VOID
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;
  VARIABLE_POINTER_TRACK  PkVariable;
  UINT8                   VarValue;
  UINT32                  VarAttr;
  UINT8                   *Data;
  UINTN                   DataSize;
  UINTN                   CtxSize;
  UINT8                   SecureBootMode;
  UINT8                   SecureBootEnable;
  UINT8                   CustomMode;
  UINT32                  ListSize;

  //
  // Initialize hash context.
  //
  CtxSize   = Sha256GetContextSize ();
  mHashCtx  = AllocateRuntimePool (CtxSize);
  if (mHashCtx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Reserved runtime buffer for "Append" operation in virtual mode.
  //
  mStorageArea  = AllocateRuntimePool (PcdGet32 (PcdMaxVariableSize));
  if (mStorageArea == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Prepare runtime buffer for serialized data of time-based authenticated
  // Variable, i.e. (VariableName, VendorGuid, Attributes, TimeStamp, Data).
  //
  mSerializationRuntimeBuffer = AllocateRuntimePool (PcdGet32 (PcdMaxVariableSize) + sizeof (EFI_GUID) + sizeof (UINT32) + sizeof (EFI_TIME));
  if (mSerializationRuntimeBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Check "AuthVarKeyDatabase" variable's existence.
  // If it doesn't exist, create a new one with initial value of 0 and EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set.
  //
  Status = FindVariable (
             AUTHVAR_KEYDB_NAME,
             &gEfiAuthenticatedVariableGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );

  if (Variable.CurrPtr == NULL) {
    VarAttr       = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
    VarValue      = 0;
    mPubKeyNumber = 0;
    Status        = UpdateVariable (
                      AUTHVAR_KEYDB_NAME,
                      &gEfiAuthenticatedVariableGuid,
                      &VarValue,
                      sizeof(UINT8),
                      VarAttr,
                      0,
                      0,
                      &Variable,
                      NULL
                      );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    //
    // Load database in global variable for cache.
    //
    DataSize  = DataSizeOfVariable (Variable.CurrPtr);
    Data      = GetVariableDataPtr (Variable.CurrPtr);
    ASSERT ((DataSize != 0) && (Data != NULL));
    CopyMem (mPubKeyStore, (UINT8 *) Data, DataSize);
    mPubKeyNumber = (UINT32) (DataSize / EFI_CERT_TYPE_RSA2048_SIZE);
  }

  FindVariable (EFI_PLATFORM_KEY_NAME, &gEfiGlobalVariableGuid, &PkVariable, &mVariableModuleGlobal->VariableGlobal, FALSE);
  if (PkVariable.CurrPtr == NULL) {
    DEBUG ((EFI_D_INFO, "Variable %s does not exist.\n", EFI_PLATFORM_KEY_NAME));
  } else {
    DEBUG ((EFI_D_INFO, "Variable %s exists.\n", EFI_PLATFORM_KEY_NAME));
  }
  
  //
  // Check "SetupMode" variable's existence.
  // If it doesn't exist, check PK database's existence to determine the value.
  // Then create a new one with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set.
  //
  Status = FindVariable (
             EFI_SETUP_MODE_NAME,
             &gEfiGlobalVariableGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );

  if (Variable.CurrPtr == NULL) {
    if (PkVariable.CurrPtr == NULL) {
      mPlatformMode = SETUP_MODE;
    } else {
      mPlatformMode = USER_MODE;
    }

    VarAttr = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
    Status  = UpdateVariable (
                EFI_SETUP_MODE_NAME,
                &gEfiGlobalVariableGuid,
                &mPlatformMode,
                sizeof(UINT8),
                VarAttr,
                0,
                0,
                &Variable,
                NULL
                );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    mPlatformMode = *(GetVariableDataPtr (Variable.CurrPtr));
  }
  //
  // Check "SignatureSupport" variable's existence.
  // If it doesn't exist, then create a new one with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set.
  //
  Status = FindVariable (
             EFI_SIGNATURE_SUPPORT_NAME,
             &gEfiGlobalVariableGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );

  if (Variable.CurrPtr == NULL) {
    VarAttr = EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
    Status  = UpdateVariable (
                EFI_SIGNATURE_SUPPORT_NAME,
                &gEfiGlobalVariableGuid,
                mSignatureSupport,
                sizeof(mSignatureSupport),
                VarAttr,
                0,
                0,
                &Variable,
                NULL
                );
  }

  //
  // If "SecureBootEnable" variable exists, then update "SecureBoot" variable.
  // If "SecureBootEnable" variable is SECURE_BOOT_ENABLE and in USER_MODE, Set "SecureBoot" variable to SECURE_BOOT_MODE_ENABLE.
  // If "SecureBootEnable" variable is SECURE_BOOT_DISABLE, Set "SecureBoot" variable to SECURE_BOOT_MODE_DISABLE.
  //
  SecureBootEnable = SECURE_BOOT_MODE_DISABLE;
  FindVariable (EFI_SECURE_BOOT_ENABLE_NAME, &gEfiSecureBootEnableDisableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
  if (Variable.CurrPtr != NULL) {
    SecureBootEnable = *(GetVariableDataPtr (Variable.CurrPtr));
  } else if (mPlatformMode == USER_MODE) {
    //
    // "SecureBootEnable" not exist, initialize it in USER_MODE.
    //
    SecureBootEnable = SECURE_BOOT_MODE_ENABLE;
    Status = UpdateVariable (
               EFI_SECURE_BOOT_ENABLE_NAME,
               &gEfiSecureBootEnableDisableGuid,
               &SecureBootEnable,
               sizeof (UINT8),
               EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
               0,
               0,
               &Variable,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (SecureBootEnable == SECURE_BOOT_ENABLE && mPlatformMode == USER_MODE) {
    SecureBootMode = SECURE_BOOT_MODE_ENABLE;
  } else {
    SecureBootMode = SECURE_BOOT_MODE_DISABLE;
  }
  FindVariable (EFI_SECURE_BOOT_MODE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
  Status = UpdateVariable (
             EFI_SECURE_BOOT_MODE_NAME,
             &gEfiGlobalVariableGuid,
             &SecureBootMode,
             sizeof (UINT8),
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS,
             0,
             0,
             &Variable,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((EFI_D_INFO, "Variable %s is %x\n", EFI_SETUP_MODE_NAME, mPlatformMode));
  DEBUG ((EFI_D_INFO, "Variable %s is %x\n", EFI_SECURE_BOOT_MODE_NAME, SecureBootMode));
  DEBUG ((EFI_D_INFO, "Variable %s is %x\n", EFI_SECURE_BOOT_ENABLE_NAME, SecureBootEnable));

  //
  // Check "CustomMode" variable's existence.
  //
  FindVariable (EFI_CUSTOM_MODE_NAME, &gEfiCustomModeEnableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
  if (Variable.CurrPtr != NULL) {
    CustomMode = *(GetVariableDataPtr (Variable.CurrPtr));
  } else {
    //
    // "CustomMode" not exist, initialize it in STANDARD_SECURE_BOOT_MODE.
    //
    CustomMode = STANDARD_SECURE_BOOT_MODE;
    Status = UpdateVariable (
               EFI_CUSTOM_MODE_NAME,
               &gEfiCustomModeEnableGuid,
               &CustomMode,
               sizeof (UINT8),
               EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
               0,
               0,
               &Variable,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  
  DEBUG ((EFI_D_INFO, "Variable %s is %x\n", EFI_CUSTOM_MODE_NAME, CustomMode));

  //
  // Check "certdb" variable's existence.
  // If it doesn't exist, then create a new one with 
  // EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set.
  //
  Status = FindVariable (
             EFI_CERT_DB_NAME,
             &gEfiCertDbGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );

  if (Variable.CurrPtr == NULL) {
    VarAttr  = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
    ListSize = 0;
    Status   = UpdateVariable (
                 EFI_CERT_DB_NAME,
                 &gEfiCertDbGuid,
                 &ListSize,
                 sizeof (UINT32),
                 VarAttr,
                 0,
                 0,
                 &Variable,
                 NULL
                 );

  }  

  return Status;
}

/**
  Add public key in store and return its index.

  @param[in]  PubKey                  Input pointer to Public Key data

  @return                             Index of new added item

**/
UINT32
AddPubKeyInStore (
  IN  UINT8               *PubKey
  )
{
  EFI_STATUS              Status;
  BOOLEAN                 IsFound;
  UINT32                  Index;
  VARIABLE_POINTER_TRACK  Variable;
  UINT8                   *Ptr;

  if (PubKey == NULL) {
    return 0;
  }

  Status = FindVariable (
             AUTHVAR_KEYDB_NAME,
             &gEfiAuthenticatedVariableGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );
  ASSERT_EFI_ERROR (Status);
  //
  // Check whether the public key entry does exist.
  //
  IsFound = FALSE;
  for (Ptr = mPubKeyStore, Index = 1; Index <= mPubKeyNumber; Index++) {
    if (CompareMem (Ptr, PubKey, EFI_CERT_TYPE_RSA2048_SIZE) == 0) {
      IsFound = TRUE;
      break;
    }
    Ptr += EFI_CERT_TYPE_RSA2048_SIZE;
  }

  if (!IsFound) {
    //
    // Add public key in database.
    //
    if (mPubKeyNumber == MAX_KEY_NUM) {
      //
      // Notes: Database is full, need enhancement here, currently just return 0.
      //
      return 0;
    }

    CopyMem (mPubKeyStore + mPubKeyNumber * EFI_CERT_TYPE_RSA2048_SIZE, PubKey, EFI_CERT_TYPE_RSA2048_SIZE);
    Index = ++mPubKeyNumber;
    //
    // Update public key database variable.
    //
    Status = UpdateVariable (
               AUTHVAR_KEYDB_NAME,
               &gEfiAuthenticatedVariableGuid,
               mPubKeyStore,
               mPubKeyNumber * EFI_CERT_TYPE_RSA2048_SIZE,
               EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS,
               0,
               0,
               &Variable,
               NULL
               );
    ASSERT_EFI_ERROR (Status);
  }

  return Index;
}

/**
  Verify data payload with AuthInfo in EFI_CERT_TYPE_RSA2048_SHA256_GUID type.
  Follow the steps in UEFI2.2.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.

  @param[in]      Data                    Pointer to data with AuthInfo.
  @param[in]      DataSize                Size of Data.
  @param[in]      PubKey                  Public key used for verification.

  @retval EFI_INVALID_PARAMETER       Invalid parameter.
  @retval EFI_SECURITY_VIOLATION      If authentication failed.
  @retval EFI_SUCCESS                 Authentication successful.

**/
EFI_STATUS
VerifyCounterBasedPayload (
  IN     UINT8          *Data,
  IN     UINTN          DataSize,
  IN     UINT8          *PubKey
  )
{
  BOOLEAN                         Status;
  EFI_VARIABLE_AUTHENTICATION     *CertData;
  EFI_CERT_BLOCK_RSA_2048_SHA256  *CertBlock;
  UINT8                           Digest[SHA256_DIGEST_SIZE];
  VOID                            *Rsa;

  Rsa         = NULL;
  CertData    = NULL;
  CertBlock   = NULL;

  if (Data == NULL || PubKey == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CertData  = (EFI_VARIABLE_AUTHENTICATION *) Data;
  CertBlock = (EFI_CERT_BLOCK_RSA_2048_SHA256 *) (CertData->AuthInfo.CertData);

  //
  // wCertificateType should be WIN_CERT_TYPE_EFI_GUID.
  // Cert type should be EFI_CERT_TYPE_RSA2048_SHA256_GUID.
  //
  if ((CertData->AuthInfo.Hdr.wCertificateType != WIN_CERT_TYPE_EFI_GUID) ||
      !CompareGuid (&CertData->AuthInfo.CertType, &gEfiCertTypeRsa2048Sha256Guid)
        ) {
    //
    // Invalid AuthInfo type, return EFI_SECURITY_VIOLATION.
    //
    return EFI_SECURITY_VIOLATION;
  }
  //
  // Hash data payload with SHA256.
  //
  ZeroMem (Digest, SHA256_DIGEST_SIZE);
  Status  = Sha256Init (mHashCtx);
  if (!Status) {
    goto Done;
  }
  Status  = Sha256Update (mHashCtx, Data + AUTHINFO_SIZE, (UINTN) (DataSize - AUTHINFO_SIZE));
  if (!Status) {
    goto Done;
  }
  //
  // Hash Monotonic Count.
  //
  Status  = Sha256Update (mHashCtx, &CertData->MonotonicCount, sizeof (UINT64));
  if (!Status) {
    goto Done;
  }
  Status  = Sha256Final (mHashCtx, Digest);
  if (!Status) {
    goto Done;
  }
  //
  // Generate & Initialize RSA Context.
  //
  Rsa = RsaNew ();
  ASSERT (Rsa != NULL);
  //
  // Set RSA Key Components.
  // NOTE: Only N and E are needed to be set as RSA public key for signature verification.
  //
  Status = RsaSetKey (Rsa, RsaKeyN, PubKey, EFI_CERT_TYPE_RSA2048_SIZE);
  if (!Status) {
    goto Done;
  }
  Status = RsaSetKey (Rsa, RsaKeyE, mRsaE, sizeof (mRsaE));
  if (!Status) {
    goto Done;
  }
  //
  // Verify the signature.
  //
  Status = RsaPkcs1Verify (
             Rsa,
             Digest,
             SHA256_DIGEST_SIZE,
             CertBlock->Signature,
             EFI_CERT_TYPE_RSA2048_SHA256_SIZE
             );

Done:
  if (Rsa != NULL) {
    RsaFree (Rsa);
  }
  if (Status) {
    return EFI_SUCCESS;
  } else {
    return EFI_SECURITY_VIOLATION;
  }
}

/**
  Update platform mode.

  @param[in]      Mode                    SETUP_MODE or USER_MODE.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SUCCESS                     Update platform mode successfully.

**/
EFI_STATUS
UpdatePlatformMode (
  IN  UINT32                    Mode
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;
  UINT32                  VarAttr;
  UINT8                   SecureBootMode;
  UINT8                   SecureBootEnable;
  UINTN                   VariableDataSize;

  Status = FindVariable (
             EFI_SETUP_MODE_NAME,
             &gEfiGlobalVariableGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mPlatformMode  = Mode;
  VarAttr        = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
  Status         = UpdateVariable (
                     EFI_SETUP_MODE_NAME,
                     &gEfiGlobalVariableGuid,
                     &mPlatformMode,
                     sizeof(UINT8),
                     VarAttr,
                     0,
                     0,
                     &Variable,
                     NULL
                     );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (AtRuntime ()) {
    //
    // SecureBoot Variable indicates whether the platform firmware is operating
    // in Secure boot mode (1) or not (0), so we should not change SecureBoot
    // Variable in runtime.
    //
    return Status;
  }

  //
  // Check "SecureBoot" variable's existence.
  // If it doesn't exist, firmware has no capability to perform driver signing verification,
  // then set "SecureBoot" to 0.
  //
  Status = FindVariable (
             EFI_SECURE_BOOT_MODE_NAME,
             &gEfiGlobalVariableGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );
  //
  // If "SecureBoot" variable exists, then check "SetupMode" variable update.
  // If "SetupMode" variable is USER_MODE, "SecureBoot" variable is set to 1.
  // If "SetupMode" variable is SETUP_MODE, "SecureBoot" variable is set to 0.
  //
  if (Variable.CurrPtr == NULL) {
    SecureBootMode = SECURE_BOOT_MODE_DISABLE;
  } else {
    if (mPlatformMode == USER_MODE) {
      SecureBootMode = SECURE_BOOT_MODE_ENABLE;
    } else if (mPlatformMode == SETUP_MODE) {
      SecureBootMode = SECURE_BOOT_MODE_DISABLE;
    } else {
      return EFI_NOT_FOUND;
    }
  }

  VarAttr = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
  Status  = UpdateVariable (
              EFI_SECURE_BOOT_MODE_NAME,
              &gEfiGlobalVariableGuid,
              &SecureBootMode,
              sizeof(UINT8),
              VarAttr,
              0,
              0,
              &Variable,
              NULL
              );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check "SecureBootEnable" variable's existence. It can enable/disable secure boot feature.
  //
  Status = FindVariable (
             EFI_SECURE_BOOT_ENABLE_NAME,
             &gEfiSecureBootEnableDisableGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );

  if (SecureBootMode == SECURE_BOOT_MODE_ENABLE) {
    //
    // Create the "SecureBootEnable" variable as secure boot is enabled.
    //
    SecureBootEnable = SECURE_BOOT_ENABLE;
    VariableDataSize = sizeof (SecureBootEnable);
  } else {
    //
    // Delete the "SecureBootEnable" variable if this variable exist as "SecureBoot"
    // variable is not in secure boot state.
    //
    if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }
    SecureBootEnable = SECURE_BOOT_DISABLE;
    VariableDataSize = 0;
  }

  Status = UpdateVariable (
             EFI_SECURE_BOOT_ENABLE_NAME,
             &gEfiSecureBootEnableDisableGuid,
             &SecureBootEnable,
             VariableDataSize,
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
             0,
             0,
             &Variable,
             NULL
             );
  return Status;
}

/**
  Check input data form to make sure it is a valid EFI_SIGNATURE_LIST for PK/KEK variable.

  @param[in]  VariableName                Name of Variable to be check.
  @param[in]  VendorGuid                  Variable vendor GUID.
  @param[in]  Data                        Point to the variable data to be checked.
  @param[in]  DataSize                    Size of Data.

  @return EFI_INVALID_PARAMETER           Invalid signature list format.
  @return EFI_SUCCESS                     Passed signature list format check successfully.
  
**/
EFI_STATUS
CheckSignatureListFormat(
  IN  CHAR16                    *VariableName,
  IN  EFI_GUID                  *VendorGuid,
  IN  VOID                      *Data,
  IN  UINTN                     DataSize
  )
{
  EFI_SIGNATURE_LIST     *SigList;
  UINTN                  SigDataSize;
  UINT32                 Index;
  UINT32                 SigCount;
  BOOLEAN                IsPk;

  if (DataSize == 0) {
    return EFI_SUCCESS;
  }

  ASSERT (VariableName != NULL && VendorGuid != NULL && Data != NULL);

  if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) && (StrCmp (VariableName, EFI_PLATFORM_KEY_NAME) == 0)){
    IsPk = TRUE;
  } else if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) && (StrCmp (VariableName, EFI_KEY_EXCHANGE_KEY_NAME) == 0)) {
    IsPk = FALSE;
  } else {
    return EFI_SUCCESS;
  }

  SigCount = 0;
  SigList  = (EFI_SIGNATURE_LIST *) Data;
  SigDataSize  = DataSize;

  //
  // Walk throuth the input signature list and check the data format.
  // If any signature is incorrectly formed, the whole check will fail.
  //
  while ((SigDataSize > 0) && (SigDataSize >= SigList->SignatureListSize)) {
    for (Index = 0; Index < (sizeof (mSupportSigItem) / sizeof (EFI_SIGNATURE_ITEM)); Index++ ) {
      if (CompareGuid (&SigList->SignatureType, &mSupportSigItem[Index].SigType)) {
        //
        // The value of SignatureSize should always be 16 (size of SignatureOwner 
        // component) add the data length according to signature type.
        //
        if (mSupportSigItem[Index].SigDataSize != ((UINT32) ~0) && 
          (SigList->SignatureSize - sizeof (EFI_GUID)) != mSupportSigItem[Index].SigDataSize) {
          return EFI_INVALID_PARAMETER;
        }
        if (mSupportSigItem[Index].SigHeaderSize != ((UINTN) ~0) &&
          SigList->SignatureHeaderSize != mSupportSigItem[Index].SigHeaderSize) {
          return EFI_INVALID_PARAMETER;
        }
        break;
      }
    }

    if (Index == (sizeof (mSupportSigItem) / sizeof (EFI_SIGNATURE_ITEM))) {
      //
      // Undefined signature type.
      //
      return EFI_INVALID_PARAMETER;
    }

    if ((SigList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - SigList->SignatureHeaderSize) % SigList->SignatureSize != 0) {
      return EFI_INVALID_PARAMETER;
    }
    SigCount += (SigList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - SigList->SignatureHeaderSize) / SigList->SignatureSize;
    
    SigDataSize -= SigList->SignatureListSize;
    SigList = (EFI_SIGNATURE_LIST *) ((UINT8 *) SigList + SigList->SignatureListSize);
  }

  if (((UINTN) SigList - (UINTN) Data) != DataSize) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsPk && SigCount > 1) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Process variable with platform key for verification.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.
  This function will check attribute carefully to avoid authentication bypass.

  @param[in]  VariableName                Name of Variable to be found.
  @param[in]  VendorGuid                  Variable vendor GUID.
  @param[in]  Data                        Data pointer.
  @param[in]  DataSize                    Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  Attribute value of the variable
  @param[in]  IsPk                        Indicate whether it is to process pk.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SECURITY_VIOLATION          The variable does NOT pass the validation.
                                          check carried out by the firmware.
  @return EFI_SUCCESS                     Variable passed validation successfully.

**/
EFI_STATUS
ProcessVarWithPk (
  IN  CHAR16                    *VariableName,
  IN  EFI_GUID                  *VendorGuid,
  IN  VOID                      *Data,
  IN  UINTN                     DataSize,
  IN  VARIABLE_POINTER_TRACK    *Variable,
  IN  UINT32                    Attributes OPTIONAL,
  IN  BOOLEAN                   IsPk
  )
{
  EFI_STATUS                  Status;
  BOOLEAN                     Del;
  UINT8                       *Payload;
  UINTN                       PayloadSize;

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0 || 
      (Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == 0) {
    //
    // PK and KEK should set EFI_VARIABLE_NON_VOLATILE attribute and should be a time-based
    // authenticated variable.
    //
    return EFI_INVALID_PARAMETER;
  }

  if (mPlatformMode == USER_MODE && !(InCustomMode() && UserPhysicalPresent())) {
    //
    // Verify against X509 Cert PK.
    //
    Del    = FALSE;
    Status = VerifyTimeBasedPayload (
               VariableName,
               VendorGuid,
               Data,
               DataSize,
               Variable,
               Attributes,
               AuthVarTypePk,
               &Del
               );
    if (!EFI_ERROR (Status)) {
      //
      // If delete PK in user mode, need change to setup mode.
      //
      if (Del && IsPk) {
        Status = UpdatePlatformMode (SETUP_MODE);
      }
    }
    return Status;
  } else {
    //
    // Process PK or KEK in Setup mode or Custom Secure Boot mode.
    //
    Payload = (UINT8 *) Data + AUTHINFO2_SIZE (Data);
    PayloadSize = DataSize - AUTHINFO2_SIZE (Data);

    Status = CheckSignatureListFormat(VariableName, VendorGuid, Payload, PayloadSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = UpdateVariable (
               VariableName,
               VendorGuid,
               Payload,
               PayloadSize,
               Attributes,
               0,
               0,
               Variable,
               &((EFI_VARIABLE_AUTHENTICATION_2 *) Data)->TimeStamp
               );

    if (IsPk) {
      if (PayloadSize != 0) {
        //
        // If enroll PK in setup mode, need change to user mode.
        //
        Status = UpdatePlatformMode (USER_MODE);
      } else {
        //
        // If delete PK in custom mode, need change to setup mode.
        //
        UpdatePlatformMode (SETUP_MODE);
      }
    }   
  }

  return Status;
}

/**
  Process variable with key exchange key for verification.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.
  This function will check attribute carefully to avoid authentication bypass.

  @param[in]  VariableName                Name of Variable to be found.
  @param[in]  VendorGuid                  Variable vendor GUID.
  @param[in]  Data                        Data pointer.
  @param[in]  DataSize                    Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  Attribute value of the variable.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SECURITY_VIOLATION          The variable does NOT pass the validation
                                          check carried out by the firmware.
  @return EFI_SUCCESS                     Variable pass validation successfully.

**/
EFI_STATUS
ProcessVarWithKek (
  IN  CHAR16                               *VariableName,
  IN  EFI_GUID                             *VendorGuid,
  IN  VOID                                 *Data,
  IN  UINTN                                DataSize,
  IN  VARIABLE_POINTER_TRACK               *Variable,
  IN  UINT32                               Attributes OPTIONAL
  )
{
  EFI_STATUS                      Status;
  UINT8                           *Payload;
  UINTN                           PayloadSize;

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0 ||
      (Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == 0) {
    //
    // DB and DBX should set EFI_VARIABLE_NON_VOLATILE attribute and should be a time-based
    // authenticated variable.
    //
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  if (mPlatformMode == USER_MODE && !(InCustomMode() && UserPhysicalPresent())) {
    //
    // Time-based, verify against X509 Cert KEK.
    //
    return VerifyTimeBasedPayload (
             VariableName,
             VendorGuid,
             Data,
             DataSize,
             Variable,
             Attributes,
             AuthVarTypeKek,
             NULL
             );
  } else {
    //
    // If in setup mode or custom secure boot mode, no authentication needed.
    //
    Payload = (UINT8 *) Data + AUTHINFO2_SIZE (Data);
    PayloadSize = DataSize - AUTHINFO2_SIZE (Data);

    Status = UpdateVariable (
               VariableName,
               VendorGuid,
               Payload,
               PayloadSize,
               Attributes,
               0,
               0,
               Variable,
               &((EFI_VARIABLE_AUTHENTICATION_2 *) Data)->TimeStamp
               );
  }

  return Status;
}

/**
  Process variable with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS/EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.
  This function will check attribute carefully to avoid authentication bypass.

  @param[in]  VariableName                Name of Variable to be found.
  @param[in]  VendorGuid                  Variable vendor GUID.

  @param[in]  Data                        Data pointer.
  @param[in]  DataSize                    Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  Attribute value of the variable.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_WRITE_PROTECTED             Variable is write-protected and needs authentication with
                                          EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set.
  @return EFI_SECURITY_VIOLATION          The variable is with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS
                                          set, but the AuthInfo does NOT pass the validation
                                          check carried out by the firmware.
  @return EFI_SUCCESS                     Variable is not write-protected or pass validation successfully.

**/
EFI_STATUS
ProcessVariable (
  IN     CHAR16                             *VariableName,
  IN     EFI_GUID                           *VendorGuid,
  IN     VOID                               *Data,
  IN     UINTN                              DataSize,
  IN     VARIABLE_POINTER_TRACK             *Variable,
  IN     UINT32                             Attributes
  )
{
  EFI_STATUS                      Status;
  BOOLEAN                         IsDeletion;
  BOOLEAN                         IsFirstTime;
  UINT8                           *PubKey;
  EFI_VARIABLE_AUTHENTICATION     *CertData;
  EFI_CERT_BLOCK_RSA_2048_SHA256  *CertBlock;
  UINT32                          KeyIndex;
  UINT64                          MonotonicCount;

  KeyIndex    = 0;
  CertData    = NULL;
  CertBlock   = NULL;
  PubKey      = NULL;
  IsDeletion  = FALSE;

  if (NeedPhysicallyPresent(VariableName, VendorGuid) && !UserPhysicalPresent()) {
    //
    // This variable is protected, only physical present user could modify its value.
    //
    return EFI_SECURITY_VIOLATION;
  }
  
  //
  // Process Time-based Authenticated variable.
  //
  if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) {
    return VerifyTimeBasedPayload (
             VariableName,
             VendorGuid,
             Data,
             DataSize,
             Variable,
             Attributes,
             AuthVarTypePriv,
             NULL
             );
  }

  //
  // Determine if first time SetVariable with the EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS.
  //
  if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
    //
    // Determine current operation type.
    //
    if (DataSize == AUTHINFO_SIZE) {
      IsDeletion = TRUE;
    }
    //
    // Determine whether this is the first time with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set.
    //
    if (Variable->CurrPtr == NULL) {
      IsFirstTime = TRUE;
    } else if ((Variable->CurrPtr->Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == 0) {
      IsFirstTime = TRUE;
    } else {
      KeyIndex   = Variable->CurrPtr->PubKeyIndex;
      IsFirstTime = FALSE;
    }
  } else if ((Variable->CurrPtr != NULL) && 
             ((Variable->CurrPtr->Attributes & (EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)) != 0)
            ) {
    //
    // If the variable is already write-protected, it always needs authentication before update.
    //
    return EFI_WRITE_PROTECTED;
  } else {
    //
    // If without EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS, set and attributes collision.
    // That means it is not authenticated variable, just update variable as usual.
    //
    Status = UpdateVariable (VariableName, VendorGuid, Data, DataSize, Attributes, 0, 0, Variable, NULL);
    return Status;
  }

  //
  // Get PubKey and check Monotonic Count value corresponding to the variable.
  //
  CertData  = (EFI_VARIABLE_AUTHENTICATION *) Data;
  CertBlock = (EFI_CERT_BLOCK_RSA_2048_SHA256 *) (CertData->AuthInfo.CertData);
  PubKey    = CertBlock->PublicKey;

  //
  // Update Monotonic Count value.
  //
  MonotonicCount = CertData->MonotonicCount;

  if (!IsFirstTime) {
    //
    // Check input PubKey.
    //
    if (CompareMem (PubKey, mPubKeyStore + (KeyIndex - 1) * EFI_CERT_TYPE_RSA2048_SIZE, EFI_CERT_TYPE_RSA2048_SIZE) != 0) {
      return EFI_SECURITY_VIOLATION;
    }
    //
    // Compare the current monotonic count and ensure that it is greater than the last SetVariable
    // operation with the EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS attribute set.
    //
    if (CertData->MonotonicCount <= Variable->CurrPtr->MonotonicCount) {
      //
      // Monotonic count check fail, suspicious replay attack, return EFI_SECURITY_VIOLATION.
      //
      return EFI_SECURITY_VIOLATION;
    }
  }
  //
  // Verify the certificate in Data payload.
  //
  Status = VerifyCounterBasedPayload (Data, DataSize, PubKey);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Now, the signature has been verified!
  //
  if (IsFirstTime && !IsDeletion) {
    //
    // Update public key database variable if need.
    //
    KeyIndex = AddPubKeyInStore (PubKey);
    if (KeyIndex == 0) {
      return EFI_SECURITY_VIOLATION;
    }
  }

  //
  // Verification pass.
  //
  return UpdateVariable (VariableName, VendorGuid, (UINT8*)Data + AUTHINFO_SIZE, DataSize - AUTHINFO_SIZE, Attributes, KeyIndex, MonotonicCount, Variable, NULL);
}

/**
  Merge two buffers which formatted as EFI_SIGNATURE_LIST. Only the new EFI_SIGNATURE_DATA
  will be appended to the original EFI_SIGNATURE_LIST, duplicate EFI_SIGNATURE_DATA
  will be ignored.

  @param[in, out]  Data            Pointer to original EFI_SIGNATURE_LIST.
  @param[in]       DataSize        Size of Data buffer.
  @param[in]       NewData         Pointer to new EFI_SIGNATURE_LIST to be appended.
  @param[in]       NewDataSize     Size of NewData buffer.

  @return Size of the merged buffer.

**/
UINTN
AppendSignatureList (
  IN  OUT VOID            *Data,
  IN  UINTN               DataSize,
  IN  VOID                *NewData,
  IN  UINTN               NewDataSize
  )
{
  EFI_SIGNATURE_LIST  *CertList;
  EFI_SIGNATURE_DATA  *Cert;
  UINTN               CertCount;
  EFI_SIGNATURE_LIST  *NewCertList;
  EFI_SIGNATURE_DATA  *NewCert;
  UINTN               NewCertCount;
  UINTN               Index;
  UINTN               Index2;
  UINTN               Size;
  UINT8               *Tail;
  UINTN               CopiedCount;
  UINTN               SignatureListSize;
  BOOLEAN             IsNewCert;

  Tail = (UINT8 *) Data + DataSize;

  NewCertList = (EFI_SIGNATURE_LIST *) NewData;
  while ((NewDataSize > 0) && (NewDataSize >= NewCertList->SignatureListSize)) {
    NewCert      = (EFI_SIGNATURE_DATA *) ((UINT8 *) NewCertList + sizeof (EFI_SIGNATURE_LIST) + NewCertList->SignatureHeaderSize);
    NewCertCount = (NewCertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - NewCertList->SignatureHeaderSize) / NewCertList->SignatureSize;

    CopiedCount = 0;
    for (Index = 0; Index < NewCertCount; Index++) {
      IsNewCert = TRUE;

      Size = DataSize;
      CertList = (EFI_SIGNATURE_LIST *) Data;
      while ((Size > 0) && (Size >= CertList->SignatureListSize)) {
        if (CompareGuid (&CertList->SignatureType, &NewCertList->SignatureType) &&
           (CertList->SignatureSize == NewCertList->SignatureSize)) {
          Cert      = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
          CertCount = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;
          for (Index2 = 0; Index2 < CertCount; Index2++) {
            //
            // Iterate each Signature Data in this Signature List.
            //
            if (CompareMem (NewCert, Cert, CertList->SignatureSize) == 0) {
              IsNewCert = FALSE;
              break;
            }
            Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) Cert + CertList->SignatureSize);
          }
        }

        if (!IsNewCert) {
          break;
        }
        Size -= CertList->SignatureListSize;
        CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList + CertList->SignatureListSize);
      }

      if (IsNewCert) {
        //
        // New EFI_SIGNATURE_DATA, append it.
        //
        if (CopiedCount == 0) {
          //
          // Copy EFI_SIGNATURE_LIST header for only once.
          //
          CopyMem (Tail, NewCertList, sizeof (EFI_SIGNATURE_LIST) + NewCertList->SignatureHeaderSize);
          Tail = Tail + sizeof (EFI_SIGNATURE_LIST) + NewCertList->SignatureHeaderSize;
        }

        CopyMem (Tail, NewCert, NewCertList->SignatureSize);
        Tail += NewCertList->SignatureSize;
        CopiedCount++;
      }

      NewCert = (EFI_SIGNATURE_DATA *) ((UINT8 *) NewCert + NewCertList->SignatureSize);
    }

    //
    // Update SignatureListSize in newly appended EFI_SIGNATURE_LIST.
    //
    if (CopiedCount != 0) {
      SignatureListSize = sizeof (EFI_SIGNATURE_LIST) + NewCertList->SignatureHeaderSize + (CopiedCount * NewCertList->SignatureSize);
      CertList = (EFI_SIGNATURE_LIST *) (Tail - SignatureListSize);
      CertList->SignatureListSize = (UINT32) SignatureListSize;
    }

    NewDataSize -= NewCertList->SignatureListSize;
    NewCertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) NewCertList + NewCertList->SignatureListSize);
  }

  return (Tail - (UINT8 *) Data);
}

/**
  Compare two EFI_TIME data.


  @param FirstTime           A pointer to the first EFI_TIME data.
  @param SecondTime          A pointer to the second EFI_TIME data.

  @retval  TRUE              The FirstTime is not later than the SecondTime.
  @retval  FALSE             The FirstTime is later than the SecondTime.

**/
BOOLEAN
CompareTimeStamp (
  IN EFI_TIME               *FirstTime,
  IN EFI_TIME               *SecondTime
  )
{
  if (FirstTime->Year != SecondTime->Year) {
    return (BOOLEAN) (FirstTime->Year < SecondTime->Year);
  } else if (FirstTime->Month != SecondTime->Month) {
    return (BOOLEAN) (FirstTime->Month < SecondTime->Month);
  } else if (FirstTime->Day != SecondTime->Day) {
    return (BOOLEAN) (FirstTime->Day < SecondTime->Day);
  } else if (FirstTime->Hour != SecondTime->Hour) {
    return (BOOLEAN) (FirstTime->Hour < SecondTime->Hour);
  } else if (FirstTime->Minute != SecondTime->Minute) {
    return (BOOLEAN) (FirstTime->Minute < SecondTime->Minute);
  }

  return (BOOLEAN) (FirstTime->Second <= SecondTime->Second);
}

/**
  Find matching signer's certificates for common authenticated variable
  by corresponding VariableName and VendorGuid from "certdb".

  The data format of "certdb":
  //
  //     UINT32 CertDbListSize;
  // /// AUTH_CERT_DB_DATA Certs1[];
  // /// AUTH_CERT_DB_DATA Certs2[];
  // /// ...
  // /// AUTH_CERT_DB_DATA Certsn[];
  //

  @param[in]  VariableName   Name of authenticated Variable.
  @param[in]  VendorGuid     Vendor GUID of authenticated Variable.
  @param[in]  Data           Pointer to variable "certdb".
  @param[in]  DataSize       Size of variable "certdb".
  @param[out] CertOffset     Offset of matching CertData, from starting of Data.
  @param[out] CertDataSize   Length of CertData in bytes.
  @param[out] CertNodeOffset Offset of matching AUTH_CERT_DB_DATA , from
                             starting of Data.
  @param[out] CertNodeSize   Length of AUTH_CERT_DB_DATA in bytes.

  @retval  EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval  EFI_NOT_FOUND         Fail to find matching certs.
  @retval  EFI_SUCCESS           Find matching certs and output parameters.

**/
EFI_STATUS
FindCertsFromDb (
  IN     CHAR16           *VariableName,
  IN     EFI_GUID         *VendorGuid,
  IN     UINT8            *Data,
  IN     UINTN            DataSize,
  OUT    UINT32           *CertOffset,    OPTIONAL
  OUT    UINT32           *CertDataSize,  OPTIONAL
  OUT    UINT32           *CertNodeOffset,OPTIONAL
  OUT    UINT32           *CertNodeSize   OPTIONAL
  )
{
  UINT32                  Offset;
  AUTH_CERT_DB_DATA       *Ptr;
  UINT32                  CertSize;
  UINT32                  NameSize;
  UINT32                  NodeSize;
  UINT32                  CertDbListSize;

  if ((VariableName == NULL) || (VendorGuid == NULL) || (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether DataSize matches recorded CertDbListSize.
  //
  if (DataSize < sizeof (UINT32)) {
    return EFI_INVALID_PARAMETER;
  }

  CertDbListSize = ReadUnaligned32 ((UINT32 *) Data);

  if (CertDbListSize != (UINT32) DataSize) {
    return EFI_INVALID_PARAMETER;
  }

  Offset = sizeof (UINT32);

  //
  // Get corresponding certificates by VendorGuid and VariableName.
  //
  while (Offset < (UINT32) DataSize) {
    Ptr = (AUTH_CERT_DB_DATA *) (Data + Offset);
    //
    // Check whether VendorGuid matches.
    //
    if (CompareGuid (&Ptr->VendorGuid, VendorGuid)) {
      NodeSize = ReadUnaligned32 (&Ptr->CertNodeSize);
      NameSize = ReadUnaligned32 (&Ptr->NameSize);
      CertSize = ReadUnaligned32 (&Ptr->CertDataSize);

      if (NodeSize != sizeof (EFI_GUID) + sizeof (UINT32) * 3 + CertSize +
          sizeof (CHAR16) * NameSize) {
        return EFI_INVALID_PARAMETER;
      }

      Offset = Offset + sizeof (EFI_GUID) + sizeof (UINT32) * 3;
      //
      // Check whether VariableName matches.
      //
      if ((NameSize == StrLen (VariableName)) && 
          (CompareMem (Data + Offset, VariableName, NameSize * sizeof (CHAR16)) == 0)) {
        Offset = Offset + NameSize * sizeof (CHAR16);

        if (CertOffset != NULL) {
          *CertOffset = Offset;
        }

        if (CertDataSize != NULL) {
          *CertDataSize = CertSize;        
        }

        if (CertNodeOffset != NULL) {
          *CertNodeOffset = (UINT32) ((UINT8 *) Ptr - Data);
        }

        if (CertNodeSize != NULL) {
          *CertNodeSize = NodeSize;
        }

        return EFI_SUCCESS;
      } else {
        Offset = Offset + NameSize * sizeof (CHAR16) + CertSize;
      }
    } else {
      NodeSize = ReadUnaligned32 (&Ptr->CertNodeSize);
      Offset   = Offset + NodeSize;
    }
  }

  return EFI_NOT_FOUND;  
}

/**
  Retrieve signer's certificates for common authenticated variable
  by corresponding VariableName and VendorGuid from "certdb".

  @param[in]  VariableName   Name of authenticated Variable.
  @param[in]  VendorGuid     Vendor GUID of authenticated Variable.
  @param[out] CertData       Pointer to signer's certificates.
  @param[out] CertDataSize   Length of CertData in bytes.

  @retval  EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval  EFI_NOT_FOUND         Fail to find "certdb" or matching certs.
  @retval  EFI_SUCCESS           Get signer's certificates successfully.

**/
EFI_STATUS
GetCertsFromDb (
  IN     CHAR16           *VariableName,
  IN     EFI_GUID         *VendorGuid,
  OUT    UINT8            **CertData,
  OUT    UINT32           *CertDataSize
  )
{
  VARIABLE_POINTER_TRACK  CertDbVariable;
  EFI_STATUS              Status;
  UINT8                   *Data;
  UINTN                   DataSize;
  UINT32                  CertOffset;

  if ((VariableName == NULL) || (VendorGuid == NULL) || (CertData == NULL) || (CertDataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Get variable "certdb".
  //
  Status = FindVariable (
             EFI_CERT_DB_NAME,
             &gEfiCertDbGuid,
             &CertDbVariable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );      
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DataSize  = DataSizeOfVariable (CertDbVariable.CurrPtr);
  Data      = GetVariableDataPtr (CertDbVariable.CurrPtr);
  if ((DataSize == 0) || (Data == NULL)) {
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  Status = FindCertsFromDb (
             VariableName,
             VendorGuid,
             Data,
             DataSize,
             &CertOffset,
             CertDataSize,
             NULL,
             NULL
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  *CertData = Data + CertOffset;
  return EFI_SUCCESS;
}

/**
  Delete matching signer's certificates when deleting common authenticated
  variable by corresponding VariableName and VendorGuid from "certdb".

  @param[in]  VariableName   Name of authenticated Variable.
  @param[in]  VendorGuid     Vendor GUID of authenticated Variable.

  @retval  EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval  EFI_NOT_FOUND         Fail to find "certdb" or matching certs.
  @retval  EFI_OUT_OF_RESOURCES  The operation is failed due to lack of resources.
  @retval  EFI_SUCCESS           The operation is completed successfully.

**/
EFI_STATUS
DeleteCertsFromDb (
  IN     CHAR16           *VariableName,
  IN     EFI_GUID         *VendorGuid
  )
{
  VARIABLE_POINTER_TRACK  CertDbVariable;
  EFI_STATUS              Status;
  UINT8                   *Data;
  UINTN                   DataSize;
  UINT32                  VarAttr;
  UINT32                  CertNodeOffset;
  UINT32                  CertNodeSize;
  UINT8                   *NewCertDb;
  UINT32                  NewCertDbSize;

  if ((VariableName == NULL) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Get variable "certdb".
  //
  Status = FindVariable (
             EFI_CERT_DB_NAME,
             &gEfiCertDbGuid,
             &CertDbVariable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );      
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DataSize  = DataSizeOfVariable (CertDbVariable.CurrPtr);
  Data      = GetVariableDataPtr (CertDbVariable.CurrPtr);
  if ((DataSize == 0) || (Data == NULL)) {
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  if (DataSize == sizeof (UINT32)) {
    //
    // There is no certs in certdb.
    //
    return EFI_SUCCESS;
  }

  //
  // Get corresponding cert node from certdb.
  //
  Status = FindCertsFromDb (
             VariableName,
             VendorGuid,
             Data,
             DataSize,
             NULL,
             NULL,
             &CertNodeOffset,
             &CertNodeSize
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DataSize < (CertNodeOffset + CertNodeSize)) {
    return EFI_NOT_FOUND;
  }

  //
  // Construct new data content of variable "certdb".
  //
  NewCertDbSize = (UINT32) DataSize - CertNodeSize;
  NewCertDb     = AllocateZeroPool (NewCertDbSize);
  if (NewCertDb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy the DB entries before deleting node.
  //
  CopyMem (NewCertDb, Data, CertNodeOffset);
  //
  // Update CertDbListSize.
  //
  CopyMem (NewCertDb, &NewCertDbSize, sizeof (UINT32));
  //
  // Copy the DB entries after deleting node.
  //
  if (DataSize > (CertNodeOffset + CertNodeSize)) {
    CopyMem (
      NewCertDb + CertNodeOffset,
      Data + CertNodeOffset + CertNodeSize,
      DataSize - CertNodeOffset - CertNodeSize
      );
  }

  //
  // Set "certdb".
  // 
  VarAttr  = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;  
  Status   = UpdateVariable (
               EFI_CERT_DB_NAME,
               &gEfiCertDbGuid,
               NewCertDb,
               NewCertDbSize,
               VarAttr,
               0,
               0,
               &CertDbVariable,
               NULL
               );

  FreePool (NewCertDb);
  return Status;
}

/**
  Insert signer's certificates for common authenticated variable with VariableName
  and VendorGuid in AUTH_CERT_DB_DATA to "certdb".

  @param[in]  VariableName   Name of authenticated Variable.
  @param[in]  VendorGuid     Vendor GUID of authenticated Variable.
  @param[in]  CertData       Pointer to signer's certificates.
  @param[in]  CertDataSize   Length of CertData in bytes.

  @retval  EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval  EFI_ACCESS_DENIED     An AUTH_CERT_DB_DATA entry with same VariableName
                                 and VendorGuid already exists.
  @retval  EFI_OUT_OF_RESOURCES  The operation is failed due to lack of resources.
  @retval  EFI_SUCCESS           Insert an AUTH_CERT_DB_DATA entry to "certdb"

**/
EFI_STATUS
InsertCertsToDb (
  IN     CHAR16           *VariableName,
  IN     EFI_GUID         *VendorGuid,
  IN     UINT8            *CertData,
  IN     UINTN            CertDataSize
  )
{
  VARIABLE_POINTER_TRACK  CertDbVariable;
  EFI_STATUS              Status;
  UINT8                   *Data;
  UINTN                   DataSize;
  UINT32                  VarAttr;
  UINT8                   *NewCertDb;
  UINT32                  NewCertDbSize;
  UINT32                  CertNodeSize;
  UINT32                  NameSize;
  AUTH_CERT_DB_DATA       *Ptr;

  if ((VariableName == NULL) || (VendorGuid == NULL) || (CertData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Get variable "certdb".
  //
  Status = FindVariable (
             EFI_CERT_DB_NAME,
             &gEfiCertDbGuid,
             &CertDbVariable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );      
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DataSize  = DataSizeOfVariable (CertDbVariable.CurrPtr);
  Data      = GetVariableDataPtr (CertDbVariable.CurrPtr);
  if ((DataSize == 0) || (Data == NULL)) {
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  //
  // Find whether matching cert node already exists in "certdb".
  // If yes return error.
  //
  Status = FindCertsFromDb (
             VariableName,
             VendorGuid,
             Data,
             DataSize,
             NULL,
             NULL,
             NULL,
             NULL
             );

  if (!EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return EFI_ACCESS_DENIED;
  }

  //
  // Construct new data content of variable "certdb".
  //
  NameSize      = (UINT32) StrLen (VariableName);
  CertNodeSize  = sizeof (AUTH_CERT_DB_DATA) + (UINT32) CertDataSize + NameSize * sizeof (CHAR16); 
  NewCertDbSize = (UINT32) DataSize + CertNodeSize;                  
  NewCertDb     = AllocateZeroPool (NewCertDbSize);
  if (NewCertDb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy the DB entries before deleting node.
  //
  CopyMem (NewCertDb, Data, DataSize);
  //
  // Update CertDbListSize.
  //
  CopyMem (NewCertDb, &NewCertDbSize, sizeof (UINT32));
  //
  // Construct new cert node.
  //
  Ptr = (AUTH_CERT_DB_DATA *) (NewCertDb + DataSize);
  CopyGuid (&Ptr->VendorGuid, VendorGuid);
  CopyMem (&Ptr->CertNodeSize, &CertNodeSize, sizeof (UINT32));
  CopyMem (&Ptr->NameSize, &NameSize, sizeof (UINT32));
  CopyMem (&Ptr->CertDataSize, &CertDataSize, sizeof (UINT32));
  
  CopyMem (
    (UINT8 *) Ptr + sizeof (AUTH_CERT_DB_DATA),
    VariableName,
    NameSize * sizeof (CHAR16)
    );

  CopyMem (
    (UINT8 *) Ptr +  sizeof (AUTH_CERT_DB_DATA) + NameSize * sizeof (CHAR16),
    CertData,
    CertDataSize
    );
  
  //
  // Set "certdb".
  // 
  VarAttr  = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;  
  Status   = UpdateVariable (
               EFI_CERT_DB_NAME,
               &gEfiCertDbGuid,
               NewCertDb,
               NewCertDbSize,
               VarAttr,
               0,
               0,
               &CertDbVariable,
               NULL
               );

  FreePool (NewCertDb);
  return Status;
}

/**
  Process variable with EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.

  @param[in]  VariableName                Name of Variable to be found.
  @param[in]  VendorGuid                  Variable vendor GUID.
  @param[in]  Data                        Data pointer.
  @param[in]  DataSize                    Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  Attribute value of the variable.
  @param[in]  AuthVarType                 Verify against PK or KEK database or private database.
  @param[out] VarDel                      Delete the variable or not.

  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval EFI_SECURITY_VIOLATION          The variable does NOT pass the validation
                                          check carried out by the firmware.
  @retval EFI_OUT_OF_RESOURCES            Failed to process variable due to lack
                                          of resources.
  @retval EFI_SUCCESS                     Variable pass validation successfully.

**/
EFI_STATUS
VerifyTimeBasedPayload (
  IN     CHAR16                             *VariableName,
  IN     EFI_GUID                           *VendorGuid,
  IN     VOID                               *Data,
  IN     UINTN                              DataSize,
  IN     VARIABLE_POINTER_TRACK             *Variable,
  IN     UINT32                             Attributes,
  IN     AUTHVAR_TYPE                       AuthVarType,
  OUT    BOOLEAN                            *VarDel
  )
{
  UINT8                            *RootCert;
  UINT8                            *SigData;
  UINT8                            *PayloadPtr;
  UINTN                            RootCertSize;
  UINTN                            Index;
  UINTN                            CertCount;
  UINTN                            PayloadSize;
  UINT32                           Attr;
  UINT32                           SigDataSize;
  UINT32                           KekDataSize;
  BOOLEAN                          VerifyStatus;
  EFI_STATUS                       Status;
  EFI_SIGNATURE_LIST               *CertList;
  EFI_SIGNATURE_DATA               *Cert;
  VARIABLE_POINTER_TRACK           KekVariable;
  EFI_VARIABLE_AUTHENTICATION_2    *CertData;
  UINT8                            *NewData;
  UINTN                            NewDataSize;
  VARIABLE_POINTER_TRACK           PkVariable;
  UINT8                            *Buffer;
  UINTN                            Length;
  UINT8                            *SignerCerts;
  UINT8                            *WrapSigData;
  UINTN                            CertStackSize;
  UINT8                            *CertsInCertDb;
  UINT32                           CertsSizeinDb;

  VerifyStatus           = FALSE;
  CertData               = NULL;
  NewData                = NULL;
  Attr                   = Attributes;
  WrapSigData            = NULL;
  SignerCerts            = NULL;
  RootCert               = NULL;

  //
  // When the attribute EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS is
  // set, then the Data buffer shall begin with an instance of a complete (and serialized)
  // EFI_VARIABLE_AUTHENTICATION_2 descriptor. The descriptor shall be followed by the new
  // variable value and DataSize shall reflect the combined size of the descriptor and the new
  // variable value. The authentication descriptor is not part of the variable data and is not
  // returned by subsequent calls to GetVariable().
  //
  CertData = (EFI_VARIABLE_AUTHENTICATION_2 *) Data;

  //
  // Verify that Pad1, Nanosecond, TimeZone, Daylight and Pad2 components of the
  // TimeStamp value are set to zero.
  //
  if ((CertData->TimeStamp.Pad1 != 0) ||
      (CertData->TimeStamp.Nanosecond != 0) ||
      (CertData->TimeStamp.TimeZone != 0) ||
      (CertData->TimeStamp.Daylight != 0) ||
      (CertData->TimeStamp.Pad2 != 0)) {
    return EFI_SECURITY_VIOLATION;
  }

  if ((Variable->CurrPtr != NULL) && ((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0)) {
    if (CompareTimeStamp (&CertData->TimeStamp, &Variable->CurrPtr->TimeStamp)) {
      //
      // TimeStamp check fail, suspicious replay attack, return EFI_SECURITY_VIOLATION.
      //
      return EFI_SECURITY_VIOLATION;
    }
  }

  //
  // wCertificateType should be WIN_CERT_TYPE_EFI_GUID.
  // Cert type should be EFI_CERT_TYPE_PKCS7_GUID.
  //
  if ((CertData->AuthInfo.Hdr.wCertificateType != WIN_CERT_TYPE_EFI_GUID) ||
      !CompareGuid (&CertData->AuthInfo.CertType, &gEfiCertPkcs7Guid)) {
    //
    // Invalid AuthInfo type, return EFI_SECURITY_VIOLATION.
    //
    return EFI_SECURITY_VIOLATION;
  }

  //
  // Find out Pkcs7 SignedData which follows the EFI_VARIABLE_AUTHENTICATION_2 descriptor.
  // AuthInfo.Hdr.dwLength is the length of the entire certificate, including the length of the header.
  //
  SigData = CertData->AuthInfo.CertData;
  SigDataSize = CertData->AuthInfo.Hdr.dwLength - (UINT32) (OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData));

  //
  // Find out the new data payload which follows Pkcs7 SignedData directly.
  //
  PayloadPtr = SigData + SigDataSize;
  PayloadSize = DataSize - OFFSET_OF_AUTHINFO2_CERT_DATA - (UINTN) SigDataSize;

  //
  // Construct a buffer to fill with (VariableName, VendorGuid, Attributes, TimeStamp, Data).
  //
  NewDataSize = PayloadSize + sizeof (EFI_TIME) + sizeof (UINT32) +
                sizeof (EFI_GUID) + StrSize (VariableName) - sizeof (CHAR16);
  NewData = mSerializationRuntimeBuffer;

  Buffer = NewData;
  Length = StrLen (VariableName) * sizeof (CHAR16);
  CopyMem (Buffer, VariableName, Length);
  Buffer += Length;

  Length = sizeof (EFI_GUID);
  CopyMem (Buffer, VendorGuid, Length);
  Buffer += Length;

  Length = sizeof (UINT32);
  CopyMem (Buffer, &Attr, Length);
  Buffer += Length;

  Length = sizeof (EFI_TIME);
  CopyMem (Buffer, &CertData->TimeStamp, Length);
  Buffer += Length;

  CopyMem (Buffer, PayloadPtr, PayloadSize);

  if (AuthVarType == AuthVarTypePk) {
    //
    // Get platform key from variable.
    //
    Status = FindVariable (
               EFI_PLATFORM_KEY_NAME,
               &gEfiGlobalVariableGuid,
               &PkVariable,
               &mVariableModuleGlobal->VariableGlobal,
               FALSE
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    CertList = (EFI_SIGNATURE_LIST *) GetVariableDataPtr (PkVariable.CurrPtr);
    Cert     = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
    RootCert      = Cert->SignatureData;
    RootCertSize  = CertList->SignatureSize - (sizeof (EFI_SIGNATURE_DATA) - 1);


    //
    // Verify Pkcs7 SignedData via Pkcs7Verify library.
    //
    VerifyStatus = Pkcs7Verify (
                     SigData,
                     SigDataSize,
                     RootCert,
                     RootCertSize,
                     NewData,
                     NewDataSize
                     );

  } else if (AuthVarType == AuthVarTypeKek) {

    //
    // Get KEK database from variable.
    //
    Status = FindVariable (
               EFI_KEY_EXCHANGE_KEY_NAME,
               &gEfiGlobalVariableGuid,
               &KekVariable,
               &mVariableModuleGlobal->VariableGlobal,
               FALSE
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Ready to verify Pkcs7 SignedData. Go through KEK Signature Database to find out X.509 CertList.
    //
    KekDataSize      = KekVariable.CurrPtr->DataSize;
    CertList         = (EFI_SIGNATURE_LIST *) GetVariableDataPtr (KekVariable.CurrPtr);
    while ((KekDataSize > 0) && (KekDataSize >= CertList->SignatureListSize)) {
      if (CompareGuid (&CertList->SignatureType, &gEfiCertX509Guid)) {
        Cert       = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
        CertCount  = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;
        for (Index = 0; Index < CertCount; Index++) {
          //
          // Iterate each Signature Data Node within this CertList for a verify
          //
          RootCert      = Cert->SignatureData;
          RootCertSize  = CertList->SignatureSize - (sizeof (EFI_SIGNATURE_DATA) - 1);

          //
          // Verify Pkcs7 SignedData via Pkcs7Verify library.
          //
          VerifyStatus = Pkcs7Verify (
                           SigData,
                           SigDataSize,
                           RootCert,
                           RootCertSize,
                           NewData,
                           NewDataSize
                           );
          if (VerifyStatus) {
            goto Exit;
          }
          Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) Cert + CertList->SignatureSize);
        }
      }
      KekDataSize -= CertList->SignatureListSize;
      CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList + CertList->SignatureListSize);
    }
  } else if (AuthVarType == AuthVarTypePriv) {

    //
    // Process common authenticated variable except PK/KEK/DB/DBX.
    // Get signer's certificates from SignedData.
    //
    VerifyStatus = Pkcs7GetSigners (
                     SigData,
                     SigDataSize,
                     &SignerCerts,
                     &CertStackSize,
                     &RootCert,
                     &RootCertSize
                     );
    if (!VerifyStatus) {
      goto Exit;
    }

    //
    // Get previously stored signer's certificates from certdb for existing
    // variable. Check whether they are identical with signer's certificates
    // in SignedData. If not, return error immediately.
    //
    if ((Variable->CurrPtr != NULL)) {
      VerifyStatus = FALSE;

      Status = GetCertsFromDb (VariableName, VendorGuid, &CertsInCertDb, &CertsSizeinDb);
      if (EFI_ERROR (Status)) {
        goto Exit;
      }
    
      if ((CertStackSize != CertsSizeinDb) ||
          (CompareMem (SignerCerts, CertsInCertDb, CertsSizeinDb) != 0)) {
        goto Exit;
      }
    }

    VerifyStatus = Pkcs7Verify (
                     SigData,
                     SigDataSize,
                     RootCert,
                     RootCertSize,
                     NewData,
                     NewDataSize
                     );
    if (!VerifyStatus) {
      goto Exit;
    }

    //
    // Delete signer's certificates when delete the common authenticated variable.
    //
    if ((PayloadSize == 0) && (Variable->CurrPtr != NULL)) {
      Status = DeleteCertsFromDb (VariableName, VendorGuid);
      if (EFI_ERROR (Status)) {
        VerifyStatus = FALSE;
        goto Exit;
      }
    } else if (Variable->CurrPtr == NULL) {
      //
      // Insert signer's certificates when adding a new common authenticated variable.
      //
      Status = InsertCertsToDb (VariableName, VendorGuid, SignerCerts, CertStackSize);
      if (EFI_ERROR (Status)) {
        VerifyStatus = FALSE;
        goto Exit;
      }
    }
  } else {
    return EFI_SECURITY_VIOLATION;
  }

Exit:

  if (AuthVarType == AuthVarTypePriv) {
    Pkcs7FreeSigners (RootCert);
    Pkcs7FreeSigners (SignerCerts);
  }

  if (!VerifyStatus) {
    return EFI_SECURITY_VIOLATION;
  }

  Status = CheckSignatureListFormat(VariableName, VendorGuid, PayloadPtr, PayloadSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((PayloadSize == 0) && (VarDel != NULL)) {
    *VarDel = TRUE;
  }

  //
  // Final step: Update/Append Variable if it pass Pkcs7Verify
  //
  return UpdateVariable (
           VariableName,
           VendorGuid,
           PayloadPtr,
           PayloadSize,
           Attributes,
           0,
           0,
           Variable,
           &CertData->TimeStamp
           );
}


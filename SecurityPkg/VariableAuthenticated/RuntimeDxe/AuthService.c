/** @file
  Implement authentication services for the authenticated variable
  service in UEFI2.2.

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
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
EFI_GUID mSignatureSupport[SIGSUPPORT_NUM] = {EFI_CERT_RSA2048_SHA256_GUID, EFI_CERT_RSA2048_SHA1_GUID};
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

  Status = FindVariable (VariableName, VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal);
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
             &mVariableModuleGlobal->VariableGlobal
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

  FindVariable (EFI_PLATFORM_KEY_NAME, &gEfiGlobalVariableGuid, &PkVariable, &mVariableModuleGlobal->VariableGlobal);
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
             &mVariableModuleGlobal->VariableGlobal
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
             &mVariableModuleGlobal->VariableGlobal
             );

  if (Variable.CurrPtr == NULL) {
    VarAttr = EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
    Status  = UpdateVariable (
                EFI_SIGNATURE_SUPPORT_NAME,
                &gEfiGlobalVariableGuid,
                mSignatureSupport,
                SIGSUPPORT_NUM * sizeof(EFI_GUID),
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
  FindVariable (EFI_SECURE_BOOT_ENABLE_NAME, &gEfiSecureBootEnableDisableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal);
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
  FindVariable (EFI_SECURE_BOOT_MODE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal);
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
  // Detect whether a secure platform-specific method to clear PK(Platform Key)
  // is configured by platform owner. This method is provided for users force to clear PK
  // in case incorrect enrollment mis-haps.
  //
  if (ForceClearPK ()) {
    DEBUG ((EFI_D_INFO, "Variable PK/KEK/DB/DBX will be cleared in clear PK mode.\n"));

    //
    // 1. Clear PK.
    //
    Status = DeleteVariable (EFI_PLATFORM_KEY_NAME, &gEfiGlobalVariableGuid);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // 2. Update "SetupMode" variable to SETUP_MODE.
    //
    UpdatePlatformMode (SETUP_MODE);

    //
    // 3. Clear KEK, DB and DBX.
    //
    DeleteVariable (EFI_KEY_EXCHANGE_KEY_NAME, &gEfiGlobalVariableGuid);
    DeleteVariable (EFI_IMAGE_SECURITY_DATABASE, &gEfiImageSecurityDatabaseGuid);
    DeleteVariable (EFI_IMAGE_SECURITY_DATABASE1, &gEfiImageSecurityDatabaseGuid);
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
             &mVariableModuleGlobal->VariableGlobal
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
  Verify data payload with AuthInfo in EFI_CERT_TYPE_RSA2048_SHA256 type.
  Follow the steps in UEFI2.2.

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
  // Cert type should be EFI_CERT_TYPE_RSA2048_SHA256.
  //
  if ((CertData->AuthInfo.Hdr.wCertificateType != WIN_CERT_TYPE_EFI_GUID) ||
      !CompareGuid (&CertData->AuthInfo.CertType, &gEfiCertRsa2048Sha256Guid)
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
             &mVariableModuleGlobal->VariableGlobal
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
             &mVariableModuleGlobal->VariableGlobal
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
             &mVariableModuleGlobal->VariableGlobal
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
  Process variable with platform key for verification.

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
  VARIABLE_POINTER_TRACK      PkVariable;
  EFI_SIGNATURE_LIST          *OldPkList;
  EFI_SIGNATURE_DATA          *OldPkData;
  EFI_VARIABLE_AUTHENTICATION *CertData;
  BOOLEAN                     TimeBase;
  BOOLEAN                     Del;
  UINT8                       *Payload;
  UINTN                       PayloadSize;
  UINT64                      MonotonicCount;
  EFI_TIME                    *TimeStamp;

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
    //
    // PK and KEK should set EFI_VARIABLE_NON_VOLATILE attribute.
    //
    return EFI_INVALID_PARAMETER;
  }

  if (mPlatformMode == USER_MODE) {

    if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) {
      //
      // EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute means time-based X509 Cert PK.
      //
      TimeBase = TRUE;
    } else if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
      //
      // EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS attribute means counter-based RSA-2048 Cert PK.
      //
      TimeBase = FALSE;
    } else {
      return EFI_INVALID_PARAMETER;
    }

    if (TimeBase) {
      //
      // Verify against X509 Cert PK.
      //
      Del    = FALSE;
      Status = VerifyTimeBasedPayload (VariableName, VendorGuid, Data, DataSize, Variable, Attributes, TRUE, &Del);
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
      // Verify against RSA2048 Cert PK.
      //
      CertData = (EFI_VARIABLE_AUTHENTICATION *) Data;
      if ((Variable->CurrPtr != NULL) && (CertData->MonotonicCount <= Variable->CurrPtr->MonotonicCount)) {
        //
        // Monotonic count check fail, suspicious replay attack, return EFI_SECURITY_VIOLATION.
        //
        return EFI_SECURITY_VIOLATION;
      }
      //
      // Get platform key from variable.
      //
      Status = FindVariable (
                 EFI_PLATFORM_KEY_NAME,
                 &gEfiGlobalVariableGuid,
                 &PkVariable,
                 &mVariableModuleGlobal->VariableGlobal
                 );
      ASSERT_EFI_ERROR (Status);

      OldPkList = (EFI_SIGNATURE_LIST *) GetVariableDataPtr (PkVariable.CurrPtr);
      OldPkData = (EFI_SIGNATURE_DATA *) ((UINT8 *) OldPkList + sizeof (EFI_SIGNATURE_LIST) + OldPkList->SignatureHeaderSize);
      Status    = VerifyCounterBasedPayload (Data, DataSize, OldPkData->SignatureData);
      if (!EFI_ERROR (Status)) {
        Status = UpdateVariable (
                   VariableName,
                   VendorGuid,
                   (UINT8*)Data + AUTHINFO_SIZE,
                   DataSize - AUTHINFO_SIZE,
                   Attributes,
                   0,
                   CertData->MonotonicCount,
                   Variable,
                   NULL
                   );

        if (!EFI_ERROR (Status)) {
          //
          // If delete PK in user mode, need change to setup mode.
          //
          if ((DataSize == AUTHINFO_SIZE) && IsPk) {
            Status = UpdatePlatformMode (SETUP_MODE);
          }
        }
      }
    }
  } else {
    //
    // Process PK or KEK in Setup mode.
    //
    if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) {
      //
      // Time-based Authentication descriptor.
      //
      MonotonicCount = 0;
      TimeStamp = &((EFI_VARIABLE_AUTHENTICATION_2 *) Data)->TimeStamp;
      Payload = (UINT8 *) Data + AUTHINFO2_SIZE (Data);
      PayloadSize = DataSize - AUTHINFO2_SIZE (Data);
    } else if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
      //
      // Counter-based Authentication descriptor.
      //
      MonotonicCount = ((EFI_VARIABLE_AUTHENTICATION *) Data)->MonotonicCount;
      TimeStamp = NULL;
      Payload = (UINT8*) Data + AUTHINFO_SIZE;
      PayloadSize = DataSize - AUTHINFO_SIZE;
    } else {
      //
      // No Authentication descriptor.
      //
      MonotonicCount = 0;
      TimeStamp = NULL;
      Payload = Data;
      PayloadSize = DataSize;
    }

    Status = UpdateVariable (
               VariableName,
               VendorGuid,
               Payload,
               PayloadSize,
               Attributes,
               0,
               MonotonicCount,
               Variable,
               TimeStamp
               );
    //
    // If enroll PK in setup mode, need change to user mode.
    //
    if ((DataSize != 0) && IsPk) {
      Status = UpdatePlatformMode (USER_MODE);
    }
  }

  return Status;
}

/**
  Process variable with key exchange key for verification.

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
  VARIABLE_POINTER_TRACK          KekVariable;
  EFI_SIGNATURE_LIST              *KekList;
  EFI_SIGNATURE_DATA              *KekItem;
  UINT32                          KekCount;
  EFI_VARIABLE_AUTHENTICATION     *CertData;
  EFI_CERT_BLOCK_RSA_2048_SHA256  *CertBlock;
  BOOLEAN                         IsFound;
  UINT32                          Index;
  UINT32                          KekDataSize;
  UINT8                           *Payload;
  UINTN                           PayloadSize;
  UINT64                          MonotonicCount;

  if (mPlatformMode == USER_MODE) {
    if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == 0) {
      //
      // In user mode, should set EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS attribute.
      //
      return EFI_INVALID_PARAMETER;
    }

    CertData  = (EFI_VARIABLE_AUTHENTICATION *) Data;
    CertBlock = (EFI_CERT_BLOCK_RSA_2048_SHA256 *) (CertData->AuthInfo.CertData);
    if ((Variable->CurrPtr != NULL) && (CertData->MonotonicCount <= Variable->CurrPtr->MonotonicCount)) {
      //
      // Monotonic count check fail, suspicious replay attack, return EFI_SECURITY_VIOLATION.
      //
      return EFI_SECURITY_VIOLATION;
    }
    //
    // Get KEK database from variable.
    //
    Status = FindVariable (
               EFI_KEY_EXCHANGE_KEY_NAME,
               &gEfiGlobalVariableGuid,
               &KekVariable,
               &mVariableModuleGlobal->VariableGlobal
               );
    ASSERT_EFI_ERROR (Status);

    KekDataSize = KekVariable.CurrPtr->DataSize;
    KekList     = (EFI_SIGNATURE_LIST *) GetVariableDataPtr (KekVariable.CurrPtr);

    //
    // Enumerate all Kek items in this list to verify the variable certificate data.
    // If anyone is authenticated successfully, it means the variable is correct!
    //
    IsFound   = FALSE;
    while ((KekDataSize > 0) && (KekDataSize >= KekList->SignatureListSize)) {
      if (CompareGuid (&KekList->SignatureType, &gEfiCertRsa2048Guid)) {
        KekItem   = (EFI_SIGNATURE_DATA *) ((UINT8 *) KekList + sizeof (EFI_SIGNATURE_LIST) + KekList->SignatureHeaderSize);
        KekCount  = (KekList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - KekList->SignatureHeaderSize) / KekList->SignatureSize;
        for (Index = 0; Index < KekCount; Index++) {
          if (CompareMem (KekItem->SignatureData, CertBlock->PublicKey, EFI_CERT_TYPE_RSA2048_SIZE) == 0) {
            IsFound = TRUE;
            break;
          }
          KekItem = (EFI_SIGNATURE_DATA *) ((UINT8 *) KekItem + KekList->SignatureSize);
        }
      }
      KekDataSize -= KekList->SignatureListSize;
      KekList = (EFI_SIGNATURE_LIST *) ((UINT8 *) KekList + KekList->SignatureListSize);
    }

    if (!IsFound) {
      return EFI_SECURITY_VIOLATION;
    }

    Status = VerifyCounterBasedPayload (Data, DataSize, CertBlock->PublicKey);
    if (!EFI_ERROR (Status)) {
      Status = UpdateVariable (
                 VariableName,
                 VendorGuid,
                 (UINT8*)Data + AUTHINFO_SIZE,
                 DataSize - AUTHINFO_SIZE,
                 Attributes,
                 0,
                 CertData->MonotonicCount,
                 Variable,
                 NULL
                 );
    }
  } else {
    //
    // If in setup mode, no authentication needed.
    //
    if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
      //
      // Counter-based Authentication descriptor.
      //
      MonotonicCount = ((EFI_VARIABLE_AUTHENTICATION *) Data)->MonotonicCount;
      Payload = (UINT8*) Data + AUTHINFO_SIZE;
      PayloadSize = DataSize - AUTHINFO_SIZE;
    } else {
      //
      // No Authentication descriptor.
      //
      MonotonicCount = 0;
      Payload = Data;
      PayloadSize = DataSize;
    }

    Status = UpdateVariable (
               VariableName,
               VendorGuid,
               Payload,
               PayloadSize,
               Attributes,
               0,
               MonotonicCount,
               Variable,
               NULL
               );
  }

  return Status;
}

/**
  Process variable with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS/EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set

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

  //
  // Process Time-based Authenticated variable.
  //
  if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) {
    return VerifyTimeBasedPayload (VariableName, VendorGuid, Data, DataSize, Variable, Attributes, FALSE, NULL);
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
           (Variable->CurrPtr->Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0
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
    return (BOOLEAN) (FirstTime->Minute < FirstTime->Minute);
  }

  return (BOOLEAN) (FirstTime->Second <= SecondTime->Second);
}

/**
  Process variable with EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set

  @param[in]  VariableName                Name of Variable to be found.
  @param[in]  VendorGuid                  Variable vendor GUID.
  @param[in]  Data                        Data pointer.
  @param[in]  DataSize                    Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  Attribute value of the variable.
  @param[in]  Pk                          Verify against PK or KEK database.
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
  IN     BOOLEAN                            Pk,
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
  BOOLEAN                          Result;
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

  Result                 = FALSE;
  VerifyStatus           = FALSE;
  CertData               = NULL;
  NewData                = NULL;
  Attr                   = Attributes;

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
    return EFI_INVALID_PARAMETER;
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

  if (Pk) {
    //
    // Get platform key from variable.
    //
    Status = FindVariable (
               EFI_PLATFORM_KEY_NAME,
               &gEfiGlobalVariableGuid,
               &PkVariable,
               &mVariableModuleGlobal->VariableGlobal
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    CertList = (EFI_SIGNATURE_LIST *) GetVariableDataPtr (PkVariable.CurrPtr);
    Cert     = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
    RootCert      = Cert->SignatureData;
    RootCertSize  = CertList->SignatureSize;


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

  } else {

    //
    // Get KEK database from variable.
    //
    Status = FindVariable (
               EFI_KEY_EXCHANGE_KEY_NAME,
               &gEfiGlobalVariableGuid,
               &KekVariable,
               &mVariableModuleGlobal->VariableGlobal
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
          RootCertSize  = CertList->SignatureSize;

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
  }

Exit:

  if (!VerifyStatus) {
    return EFI_SECURITY_VIOLATION;
  }

  if ((PayloadSize == 0) && (VarDel != NULL)) {
    *VarDel = TRUE;
  }

  //
  // Final step: Update/Append Variable if it pass Pkcs7Verify
  //
  return   UpdateVariable (
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

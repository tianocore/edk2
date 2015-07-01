/** @file
  Implement authentication services for the authenticated variable
  service in UEFI2.2.

Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
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
UINT32   mPubKeyNumber;
UINT32   mPlatformMode;
EFI_GUID mSignatureSupport[SIGSUPPORT_NUM] = {EFI_CERT_RSA2048_SHA256_GUID, EFI_CERT_RSA2048_SHA1_GUID};
//
// Public Exponent of RSA Key.
//
CONST UINT8 mRsaE[] = { 0x01, 0x00, 0x01 };

/**
  Initializes for authenticated varibale service.

  @retval EFI_SUCCESS           The function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate enough memory resources.

**/
EFI_STATUS
AutenticatedVariableServiceInitialize (
  VOID
  )
{
  EFI_STATUS                        Status;
  VARIABLE_POINTER_TRACK            Variable;
  UINT8                             VarValue;
  UINT32                            VarAttr;
  UINTN                             DataSize;
  UINTN                             CtxSize;
  AUTHENTICATED_VARIABLE_HEADER     VariableHeader;
  BOOLEAN                           Valid;

  ZeroMem (&VariableHeader, sizeof (AUTHENTICATED_VARIABLE_HEADER));

  mVariableModuleGlobal->AuthenticatedVariableGuid[Physical] = &gEfiAuthenticatedVariableGuid;
  mVariableModuleGlobal->CertRsa2048Sha256Guid[Physical]     = &gEfiCertRsa2048Sha256Guid;
  mVariableModuleGlobal->ImageSecurityDatabaseGuid[Physical] = &gEfiImageSecurityDatabaseGuid;

  //
  // Initialize hash context.
  //
  CtxSize   = Sha256GetContextSize ();
  mVariableModuleGlobal->HashContext[Physical] = AllocateRuntimePool (CtxSize);
  ASSERT (mVariableModuleGlobal->HashContext[Physical] != NULL);
  //
  // Check "AuthVarKeyDatabase" variable's existence. 
  // If it doesn't exist, create a new one with initial value of 0 and EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set. 
  //
  Status = FindVariable (
             mVariableModuleGlobal->VariableName[Physical][VAR_AUTH_KEY_DB], 
             &gEfiAuthenticatedVariableGuid, 
             &Variable, 
             &mVariableModuleGlobal->VariableGlobal[Physical],
             mVariableModuleGlobal->FvbInstance
             );

  if (Variable.CurrPtr == 0x0) {
    VarAttr       = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
    VarValue      = 0;
    mPubKeyNumber = 0;
    Status        = UpdateVariable (
                      mVariableModuleGlobal->VariableName[Physical][VAR_AUTH_KEY_DB],
                      &gEfiAuthenticatedVariableGuid,
                      &VarValue,
                      sizeof(UINT8),
                      VarAttr,
                      0,
                      0,
                      FALSE,
                      mVariableModuleGlobal,
                      &Variable
                      );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    //
    // Load database in global variable for cache.
    //
    Valid = IsValidVariableHeader (
              Variable.CurrPtr, 
              Variable.Volatile, 
              &mVariableModuleGlobal->VariableGlobal[Physical], 
              mVariableModuleGlobal->FvbInstance, 
              &VariableHeader
              );
    ASSERT (Valid);

    DataSize  = DataSizeOfVariable (&VariableHeader);
    ASSERT (DataSize <= MAX_KEYDB_SIZE);
    GetVariableDataPtr (
      Variable.CurrPtr,
      Variable.Volatile,
      &mVariableModuleGlobal->VariableGlobal[Physical],
      mVariableModuleGlobal->FvbInstance,
      (CHAR16 *) mVariableModuleGlobal->PubKeyStore
      );

    mPubKeyNumber = (UINT32) (DataSize / EFI_CERT_TYPE_RSA2048_SIZE);
  }
  //
  // Check "SetupMode" variable's existence. 
  // If it doesn't exist, check PK database's existence to determine the value.
  // Then create a new one with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set. 
  //
  Status = FindVariable (
             mVariableModuleGlobal->VariableName[Physical][VAR_SETUP_MODE], 
             &gEfiGlobalVariableGuid, 
             &Variable, 
             &mVariableModuleGlobal->VariableGlobal[Physical],
             mVariableModuleGlobal->FvbInstance
             );

  if (Variable.CurrPtr == 0x0) {
    Status = FindVariable (
               mVariableModuleGlobal->VariableName[Physical][VAR_PLATFORM_KEY], 
               &gEfiGlobalVariableGuid, 
               &Variable, 
               &mVariableModuleGlobal->VariableGlobal[Physical],
               mVariableModuleGlobal->FvbInstance
               );
    if (Variable.CurrPtr == 0x0) {
      mPlatformMode = SETUP_MODE;
    } else {
      mPlatformMode = USER_MODE;
    }

    VarAttr = EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
    Status  = UpdateVariable (
                mVariableModuleGlobal->VariableName[Physical][VAR_SETUP_MODE],
                &gEfiGlobalVariableGuid,
                &mPlatformMode,
                sizeof(UINT8),
                VarAttr,
                0,
                0,
                FALSE,
                mVariableModuleGlobal,
                &Variable
                );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    GetVariableDataPtr (
      Variable.CurrPtr,
      Variable.Volatile,
      &mVariableModuleGlobal->VariableGlobal[Physical],
      mVariableModuleGlobal->FvbInstance,
      (CHAR16 *) &mPlatformMode
      );
  }
  //
  // Check "SignatureSupport" variable's existence. 
  // If it doesn't exist, then create a new one with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set. 
  //
  Status = FindVariable (
             EFI_SIGNATURE_SUPPORT_NAME, 
             &gEfiGlobalVariableGuid, 
             &Variable, 
             &mVariableModuleGlobal->VariableGlobal[Physical],
             mVariableModuleGlobal->FvbInstance
             );

  if (Variable.CurrPtr == 0x0) {
    VarAttr = EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
    Status  = UpdateVariable (
                EFI_SIGNATURE_SUPPORT_NAME,
                &gEfiGlobalVariableGuid,
                mSignatureSupport,
                SIGSUPPORT_NUM * sizeof(EFI_GUID),
                VarAttr,
                0,
                0,
                FALSE,
                mVariableModuleGlobal,
                &Variable
                );
  }

  return Status;
}

/**
  Add public key in store and return its index.

  @param[in]  VirtualMode             The current calling mode for this function.
  @param[in]  Global                  The context of this Extended SAL Variable Services Class call.
  @param[in]  PubKey                  The input pointer to Public Key data.

  @return                             The index of new added item.

**/
UINT32
AddPubKeyInStore (
  IN  BOOLEAN                   VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL      *Global,
  IN  UINT8                     *PubKey
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
             Global->VariableName[VirtualMode][VAR_AUTH_KEY_DB],
             Global->AuthenticatedVariableGuid[VirtualMode],
             &Variable,
             &Global->VariableGlobal[VirtualMode],
             Global->FvbInstance
             );
  ASSERT_EFI_ERROR (Status);
  //
  // Check whether the public key entry does exist.
  //
  IsFound = FALSE;
  for (Ptr = Global->PubKeyStore, Index = 1; Index <= mPubKeyNumber; Index++) {
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

    CopyMem (Global->PubKeyStore + mPubKeyNumber * EFI_CERT_TYPE_RSA2048_SIZE, PubKey, EFI_CERT_TYPE_RSA2048_SIZE);
    Index = ++mPubKeyNumber;
    //
    // Update public key database variable.
    //
    Status = UpdateVariable (
               Global->VariableName[VirtualMode][VAR_AUTH_KEY_DB],
               Global->AuthenticatedVariableGuid[VirtualMode],
               Global->PubKeyStore,
               mPubKeyNumber * EFI_CERT_TYPE_RSA2048_SIZE,
               EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS,
               0,
               0,
               VirtualMode,
               Global,
               &Variable
               );
    ASSERT_EFI_ERROR (Status);
  }

  return Index;
}

/**
  Verify data payload with AuthInfo in EFI_CERT_TYPE_RSA2048_SHA256 type.
  Follow the steps in UEFI2.2.

  @param[in]  VirtualMode             The current calling mode for this function.
  @param[in]  Global                  The context of this Extended SAL Variable Services Class call.
  @param[in]  Data                    The pointer to data with AuthInfo.
  @param[in]  DataSize                The size of Data.
  @param[in]  PubKey                  The public key used for verification.

  @retval EFI_INVALID_PARAMETER       Invalid parameter.
  @retval EFI_SECURITY_VIOLATION      Authentication failed.
  @retval EFI_SUCCESS                 Authentication successful.

**/
EFI_STATUS
VerifyDataPayload (
  IN  BOOLEAN                   VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL      *Global,
  IN  UINT8                     *Data,
  IN  UINTN                     DataSize,
  IN  UINT8                     *PubKey
  )
{
  BOOLEAN                         Status;
  EFI_VARIABLE_AUTHENTICATION     *CertData;
  EFI_CERT_BLOCK_RSA_2048_SHA256  *CertBlock;
  UINT8                           Digest[SHA256_DIGEST_SIZE];
  VOID                            *Rsa;
  VOID                            *HashContext;

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
      !CompareGuid (&CertData->AuthInfo.CertType, Global->CertRsa2048Sha256Guid[VirtualMode])
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
  HashContext = Global->HashContext[VirtualMode];
  Status  = Sha256Init (HashContext);
  if (!Status) {
    goto Done;
  }
  Status  = Sha256Update (HashContext, Data + AUTHINFO_SIZE, (UINTN) (DataSize - AUTHINFO_SIZE));
  if (!Status) {
    goto Done;
  }
  //
  // Hash Monotonic Count.
  //
  Status  = Sha256Update (HashContext, &CertData->MonotonicCount, sizeof (UINT64));
  if (!Status) {
    goto Done;
  }
  Status  = Sha256Final (HashContext, Digest);
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

  @param[in]  VirtualMode             The current calling mode for this function.
  @param[in]  Global                  The context of this Extended SAL Variable Services Class call.
  @param[in]  Mode                    SETUP_MODE or USER_MODE.

**/
VOID
UpdatePlatformMode (
  IN  BOOLEAN                   VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL      *Global,
  IN  UINT32                    Mode
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;
  UINT32                  VarAttr;

  Status = FindVariable (
             Global->VariableName[VirtualMode][VAR_SETUP_MODE], 
             Global->GlobalVariableGuid[VirtualMode], 
             &Variable, 
             &Global->VariableGlobal[VirtualMode],
             Global->FvbInstance
             );
  ASSERT_EFI_ERROR (Status);

  mPlatformMode  = Mode;
  VarAttr        = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
  Status         = UpdateVariable (
                     Global->VariableName[VirtualMode][VAR_SETUP_MODE],
                     Global->GlobalVariableGuid[VirtualMode],
                     &mPlatformMode,
                     sizeof(UINT8),
                     VarAttr,
                     0,
                     0,
                     VirtualMode,
                     Global,
                     &Variable
                     );
  ASSERT_EFI_ERROR (Status);
}

/**
  Process variable with platform key for verification.

  @param[in]  VariableName                The name of Variable to be found.
  @param[in]  VendorGuid                  The variable vendor GUID.
  @param[in]  Data                        The data pointer.
  @param[in]  DataSize                    The size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  VirtualMode                 The current calling mode for this function.
  @param[in]  Global                      The context of this Extended SAL Variable Services Class call.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  The attribute value of the variable.
  @param[in]  IsPk                        Indicates whether to process pk.

  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval EFI_SECURITY_VIOLATION          The variable does NOT pass the validation 
                                          check carried out by the firmware. 
  @retval EFI_SUCCESS                     The variable passed validation successfully.

**/
EFI_STATUS
ProcessVarWithPk (
  IN  CHAR16                    *VariableName,
  IN  EFI_GUID                  *VendorGuid,
  IN  VOID                      *Data,
  IN  UINTN                     DataSize,
  IN  BOOLEAN                   VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL      *Global,
  IN  VARIABLE_POINTER_TRACK    *Variable,
  IN  UINT32                    Attributes OPTIONAL,
  IN  BOOLEAN                   IsPk
  )
{
  EFI_STATUS                    Status;
  VARIABLE_POINTER_TRACK        PkVariable;
  EFI_SIGNATURE_LIST            *OldPkList;
  EFI_SIGNATURE_DATA            *OldPkData;
  EFI_VARIABLE_AUTHENTICATION   *CertData;
  AUTHENTICATED_VARIABLE_HEADER VariableHeader;
  BOOLEAN                       Valid;

  OldPkList = NULL;
  ZeroMem (&VariableHeader, sizeof (AUTHENTICATED_VARIABLE_HEADER));

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
    //
    // PK and KEK should set EFI_VARIABLE_NON_VOLATILE attribute.
    //
    return EFI_INVALID_PARAMETER;
  }

  if (mPlatformMode == USER_MODE) {
    if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == 0) {
      //
      // In user mode, PK and KEK should set EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS attribute.
      //
      return EFI_INVALID_PARAMETER;
    }

    CertData = (EFI_VARIABLE_AUTHENTICATION *) Data;

    if (Variable->CurrPtr != 0x0) {
      Valid = IsValidVariableHeader (
                Variable->CurrPtr, 
                Variable->Volatile, 
                &Global->VariableGlobal[VirtualMode], 
                Global->FvbInstance, 
                &VariableHeader
                );
      ASSERT (Valid);

      if (CertData->MonotonicCount <= VariableHeader.MonotonicCount) {
        //
        // Monotonic count check fail, suspicious replay attack, return EFI_SECURITY_VIOLATION.
        //
        return EFI_SECURITY_VIOLATION;
      }
    }
    //
    // Get platform key from variable.
    //
    Status = FindVariable (
               Global->VariableName[VirtualMode][VAR_PLATFORM_KEY], 
               Global->GlobalVariableGuid[VirtualMode], 
               &PkVariable, 
               &Global->VariableGlobal[VirtualMode],
               Global->FvbInstance
               );
    ASSERT_EFI_ERROR (Status);

    ZeroMem (Global->KeyList, MAX_KEYDB_SIZE);
    GetVariableDataPtr (
      PkVariable.CurrPtr,
      PkVariable.Volatile,
      &Global->VariableGlobal[VirtualMode],
      Global->FvbInstance,
      (CHAR16 *) Global->KeyList
      );

    OldPkList = (EFI_SIGNATURE_LIST *) Global->KeyList;
    OldPkData = (EFI_SIGNATURE_DATA *) ((UINT8 *) OldPkList + sizeof (EFI_SIGNATURE_LIST) + OldPkList->SignatureHeaderSize);
    Status    = VerifyDataPayload (VirtualMode, Global, Data, DataSize, OldPkData->SignatureData);
    if (!EFI_ERROR (Status)) {
      Status = UpdateVariable (
                 VariableName, 
                 VendorGuid, 
                 (UINT8*)Data + AUTHINFO_SIZE, 
                 DataSize - AUTHINFO_SIZE, 
                 Attributes, 
                 0, 
                 CertData->MonotonicCount, 
                 VirtualMode, 
                 Global,
                 Variable
                 );

      if (!EFI_ERROR (Status)) {
        //
        // If delete PK in user mode, need change to setup mode.
        //
        if ((DataSize == AUTHINFO_SIZE) && IsPk) {
          UpdatePlatformMode (VirtualMode, Global, SETUP_MODE);
        }
      }
    }
  } else {
    Status = UpdateVariable (VariableName, VendorGuid, Data, DataSize, Attributes, 0, 0, VirtualMode, Global, Variable);
    //
    // If enroll PK in setup mode, need change to user mode.
    //
    if ((DataSize != 0) && IsPk) {
      UpdatePlatformMode (VirtualMode, Global, USER_MODE);
    }
  }

  return Status;
}

/**
  Process variable with key exchange key for verification.

  @param[in]  VariableName                The name of Variable to be found.
  @param[in]  VendorGuid                  The variable vendor GUID.
  @param[in]  Data                        The data pointer.
  @param[in]  DataSize                    The size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  VirtualMode                 The current calling mode for this function.
  @param[in]  Global                      The context of this Extended SAL Variable Services Class call.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  The attribute value of the variable.

  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval EFI_SECURITY_VIOLATION          The variable did NOT pass the validation 
                                          check carried out by the firmware. 
  @retval EFI_SUCCESS                     The variable passed validation successfully.

**/
EFI_STATUS
ProcessVarWithKek (
  IN  CHAR16                               *VariableName,
  IN  EFI_GUID                             *VendorGuid,
  IN  VOID                                 *Data,
  IN  UINTN                                DataSize,
  IN  BOOLEAN                              VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL                 *Global,
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
  AUTHENTICATED_VARIABLE_HEADER   VariableHeader;
  BOOLEAN                         Valid;

  KekList = NULL;
  ZeroMem (&VariableHeader, sizeof (AUTHENTICATED_VARIABLE_HEADER));

  if (mPlatformMode == USER_MODE) {
    if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == 0) {
      //
      // In user mode, should set EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS attribute.
      //
      return EFI_INVALID_PARAMETER;
    }

    CertData  = (EFI_VARIABLE_AUTHENTICATION *) Data;
    CertBlock = (EFI_CERT_BLOCK_RSA_2048_SHA256 *) (CertData->AuthInfo.CertData);
    if (Variable->CurrPtr != 0x0) {
      Valid = IsValidVariableHeader (
                Variable->CurrPtr, 
                Variable->Volatile, 
                &Global->VariableGlobal[VirtualMode], 
                Global->FvbInstance, 
                &VariableHeader
                );
      ASSERT (Valid);

      if (CertData->MonotonicCount <= VariableHeader.MonotonicCount) {
        //
        // Monotonic count check fail, suspicious replay attack, return EFI_SECURITY_VIOLATION.
        //
        return EFI_SECURITY_VIOLATION;
      }
    }
    //
    // Get KEK database from variable.
    //
    Status = FindVariable (
               Global->VariableName[VirtualMode][VAR_KEY_EXCHANGE_KEY], 
               Global->GlobalVariableGuid[VirtualMode], 
               &KekVariable, 
               &Global->VariableGlobal[VirtualMode],
               Global->FvbInstance
               );
    ASSERT_EFI_ERROR (Status);

    ZeroMem (Global->KeyList, MAX_KEYDB_SIZE);
    GetVariableDataPtr (
      KekVariable.CurrPtr,
      KekVariable.Volatile,
      &Global->VariableGlobal[VirtualMode],
      Global->FvbInstance,
      (CHAR16 *) Global->KeyList
      );
    //
    // Enumerate all Kek items in this list to verify the variable certificate data.
    // If anyone is authenticated successfully, it means the variable is correct!
    //
    KekList   = (EFI_SIGNATURE_LIST *) Global->KeyList;
    IsFound   = FALSE;
    KekCount  = (KekList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - KekList->SignatureHeaderSize) / KekList->SignatureSize;
    KekItem   = (EFI_SIGNATURE_DATA *) ((UINT8 *) KekList + sizeof (EFI_SIGNATURE_LIST) + KekList->SignatureHeaderSize);
    for (Index = 0; Index < KekCount; Index++) {
      if (CompareMem (KekItem->SignatureData, CertBlock->PublicKey, EFI_CERT_TYPE_RSA2048_SIZE) == 0) {
        IsFound = TRUE;
        break;
      }
      KekItem = (EFI_SIGNATURE_DATA *) ((UINT8 *) KekItem + KekList->SignatureSize);
    }

    if (!IsFound) {
      return EFI_SECURITY_VIOLATION;
    }

    Status = VerifyDataPayload (VirtualMode, Global, Data, DataSize, CertBlock->PublicKey);
    if (!EFI_ERROR (Status)) {
      Status = UpdateVariable (
                 VariableName, 
                 VendorGuid, 
                 (UINT8*)Data + AUTHINFO_SIZE, 
                 DataSize - AUTHINFO_SIZE, 
                 Attributes, 
                 0, 
                 CertData->MonotonicCount, 
                 VirtualMode,
                 Global,
                 Variable
                 );
    }
  } else {
    //
    // If in setup mode, no authentication needed.
    //
    Status = UpdateVariable (
               VariableName, 
               VendorGuid, 
               Data, 
               DataSize, 
               Attributes, 
               0, 
               0, 
               VirtualMode,
               Global,
               Variable
               );
  }

  return Status;
}

/**
  Process variable with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set, and return the index of associated public key.

  @param[in]  Data                        The data pointer.
  @param[in]  DataSize                    The size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in]  VirtualMode                 The current calling mode for this function.
  @param[in]  Global                      The context of this Extended SAL Variable Services Class call.
  @param[in]  Variable                    The variable information which is used to keep track of variable usage.
  @param[in]  Attributes                  The attribute value of the variable.
  @param[out] KeyIndex                    The output index of corresponding public key in database.
  @param[out] MonotonicCount              The output value of corresponding Monotonic Count.

  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval EFI_WRITE_PROTECTED             The variable is write-protected and needs authentication with
                                          EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set.
  @retval EFI_SECURITY_VIOLATION          The variable is with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS
                                          set, but the AuthInfo does NOT pass the validation 
                                          check carried out by the firmware. 
  @retval EFI_SUCCESS                     The variable is not write-protected, or passed validation successfully.

**/
EFI_STATUS
VerifyVariable (
  IN  VOID                      *Data,
  IN  UINTN                     DataSize,
  IN  BOOLEAN                   VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL      *Global,
  IN  VARIABLE_POINTER_TRACK    *Variable,
  IN  UINT32                    Attributes OPTIONAL,
  OUT UINT32                    *KeyIndex OPTIONAL,
  OUT UINT64                    *MonotonicCount OPTIONAL
  )
{
  EFI_STATUS                      Status;
  BOOLEAN                         IsDeletion;
  BOOLEAN                         IsFirstTime;
  UINT8                           *PubKey;
  EFI_VARIABLE_AUTHENTICATION     *CertData;
  EFI_CERT_BLOCK_RSA_2048_SHA256  *CertBlock;
  AUTHENTICATED_VARIABLE_HEADER   VariableHeader;
  BOOLEAN                         Valid;

  CertData    = NULL;
  CertBlock   = NULL;
  PubKey      = NULL;
  IsDeletion  = FALSE;
  Valid       = FALSE;

  if (KeyIndex != NULL) {
    *KeyIndex = 0;
  }
  //
  // Determine if first time SetVariable with the EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS.
  //
  ZeroMem (&VariableHeader, sizeof (AUTHENTICATED_VARIABLE_HEADER));
  if (Variable->CurrPtr != 0x0) {
    Valid = IsValidVariableHeader (
              Variable->CurrPtr, 
              Variable->Volatile, 
              &Global->VariableGlobal[VirtualMode], 
              Global->FvbInstance, 
              &VariableHeader
              );
    ASSERT (Valid);
  }

  if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
    if (KeyIndex == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Determine current operation type.
    //
    if (DataSize == AUTHINFO_SIZE) {
      IsDeletion = TRUE;
    }
    //
    // Determine whether this is the first time with EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS set.
    //
    if (Variable->CurrPtr == 0x0) {
      IsFirstTime = TRUE;
    } else if (Valid &&(VariableHeader.Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == 0) {
      IsFirstTime = TRUE;
    } else {
      *KeyIndex   = VariableHeader.PubKeyIndex;
      IsFirstTime = FALSE;
    }
  } else if (Valid && (VariableHeader.Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) { 
      //
      // If the variable is already write-protected, it always needs authentication before update.
      //
      return EFI_WRITE_PROTECTED;
  } else {
    //
    // If without EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS, set and attributes collision.
    // That means it is not authenticated variable, just return EFI_SUCCESS.
    //
    return EFI_SUCCESS;
  }

  //
  // Get PubKey and check Monotonic Count value corresponding to the variable.
  //
  CertData  = (EFI_VARIABLE_AUTHENTICATION *) Data;
  CertBlock = (EFI_CERT_BLOCK_RSA_2048_SHA256 *) (CertData->AuthInfo.CertData);
  PubKey    = CertBlock->PublicKey;

  if (MonotonicCount != NULL) {
    //
    // Update Monotonic Count value.
    //
    *MonotonicCount = CertData->MonotonicCount;
  }

  if (!IsFirstTime) {
    //
    // Check input PubKey.
    //
    if (CompareMem (PubKey, Global->PubKeyStore + (*KeyIndex - 1) * EFI_CERT_TYPE_RSA2048_SIZE, EFI_CERT_TYPE_RSA2048_SIZE) != 0) {
      return EFI_SECURITY_VIOLATION;
    }
    //
    // Compare the current monotonic count and ensure that it is greater than the last SetVariable
    // operation with the EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS attribute set.
    //
    if (CertData->MonotonicCount <= VariableHeader.MonotonicCount) {
      //
      // Monotonic count check fail, suspicious replay attack, return EFI_SECURITY_VIOLATION.
      //
      return EFI_SECURITY_VIOLATION;
    }
  } 
  //
  // Verify the certificate in Data payload.
  //
  Status = VerifyDataPayload (VirtualMode, Global, Data, DataSize, PubKey);
  if (!EFI_ERROR (Status)) {
    //
    // Now, the signature has been verified!
    //
    if (IsFirstTime && !IsDeletion) {
      //
      // Update public key database variable if need and return the index.
      //
      *KeyIndex = AddPubKeyInStore (VirtualMode, Global, PubKey);
    }
  }

  return Status;
}


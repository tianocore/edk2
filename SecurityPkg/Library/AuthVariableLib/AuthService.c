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

  ProcessVarWithPk(), ProcessVarWithKek() and ProcessVariable() are the function to do
  variable authentication.

  VerifyTimeBasedPayloadAndUpdate() and VerifyCounterBasedPayload() are sub function to do verification.
  They will do basic validation for authentication data structure, then call crypto library
  to verify the signature.

Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AuthServiceInternal.h"

#include <Protocol/VariablePolicy.h>
#include <Library/VariablePolicyLib.h>

#define SHA_DIGEST_SIZE_MAX  SHA512_DIGEST_SIZE

/**
  Retrieves the size, in bytes, of the context buffer required for hash operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for hash operations.
  @retval  0   This interface is not supported.

**/
typedef
UINTN
(EFIAPI *EFI_HASH_GET_CONTEXT_SIZE)(
  VOID
  );

/**
  Initializes user-supplied memory pointed by Sha1Context as hash context for
  subsequent use.

  If HashContext is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  HashContext  Pointer to Hashcontext being initialized.

  @retval TRUE   Hash context initialization succeeded.
  @retval FALSE  Hash context initialization failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *EFI_HASH_INIT)(
  OUT  VOID  *HashContext
  );

/**
  Digests the input data and updates Hash context.

  This function performs Hash digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  Hash context should be already correctly initialized by HashInit(), and should not be finalized
  by HashFinal(). Behavior with invalid context is undefined.

  If HashContext is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HashContext  Pointer to the Hash context.
  @param[in]       Data         Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize     Size of Data buffer in bytes.

  @retval TRUE   SHA-1 data digest succeeded.
  @retval FALSE  SHA-1 data digest failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *EFI_HASH_UPDATE)(
  IN OUT  VOID        *HashContext,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

/**
  Completes computation of the Hash digest value.

  This function completes hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the Hash context cannot
  be used again.
  Hash context should be already correctly initialized by HashInit(), and should not be
  finalized by HashFinal(). Behavior with invalid Hash context is undefined.

  If HashContext is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HashContext  Pointer to the Hash context.
  @param[out]      HashValue    Pointer to a buffer that receives the Hash digest
                                value.

  @retval TRUE   Hash digest computation succeeded.
  @retval FALSE  Hash digest computation failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *EFI_HASH_FINAL)(
  IN OUT  VOID   *HashContext,
  OUT     UINT8  *HashValue
  );

typedef struct {
  UINT32                       HashSize;
  EFI_HASH_GET_CONTEXT_SIZE    GetContextSize;
  EFI_HASH_INIT                Init;
  EFI_HASH_UPDATE              Update;
  EFI_HASH_FINAL               Final;
  VOID                         **HashShaCtx;
  UINT8                        *OidValue;
  UINTN                        OidLength;
} EFI_HASH_INFO;

//
// Public Exponent of RSA Key.
//
CONST UINT8  mRsaE[] = { 0x01, 0x00, 0x01 };

UINT8  mSha256OidValue[] = { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01 };
UINT8  mSha384OidValue[] = { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02 };
UINT8  mSha512OidValue[] = { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03 };

EFI_HASH_INFO  mHashInfo[] = {
  { SHA256_DIGEST_SIZE, Sha256GetContextSize, Sha256Init, Sha256Update, Sha256Final, &mHashSha256Ctx, mSha256OidValue, 9 },
  { SHA384_DIGEST_SIZE, Sha384GetContextSize, Sha384Init, Sha384Update, Sha384Final, &mHashSha384Ctx, mSha384OidValue, 9 },
  { SHA512_DIGEST_SIZE, Sha512GetContextSize, Sha512Init, Sha512Update, Sha512Final, &mHashSha512Ctx, mSha512OidValue, 9 },
};

//
// Requirement for different signature type which have been defined in UEFI spec.
// These data are used to perform SignatureList format check while setting PK/KEK variable.
//
EFI_SIGNATURE_ITEM  mSupportSigItem[] = {
  // {SigType,                       SigHeaderSize,   SigDataSize  }
  { EFI_CERT_SHA256_GUID,         0, 32            },
  { EFI_CERT_RSA2048_GUID,        0, 256           },
  { EFI_CERT_RSA2048_SHA256_GUID, 0, 256           },
  { EFI_CERT_SHA1_GUID,           0, 20            },
  { EFI_CERT_RSA2048_SHA1_GUID,   0, 256           },
  { EFI_CERT_X509_GUID,           0, ((UINT32) ~0) },
  { EFI_CERT_SHA224_GUID,         0, 28            },
  { EFI_CERT_SHA384_GUID,         0, 48            },
  { EFI_CERT_SHA512_GUID,         0, 64            },
  { EFI_CERT_X509_SHA256_GUID,    0, 48            },
  { EFI_CERT_X509_SHA384_GUID,    0, 64            },
  { EFI_CERT_X509_SHA512_GUID,    0, 80            }
};

/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.

  @param[in]  VariableName          Name of the variable to be found.
  @param[in]  VendorGuid            Variable vendor GUID to be found.
  @param[out] Data                  Pointer to data address.
  @param[out] DataSize              Pointer to data size.

  @retval EFI_INVALID_PARAMETER     If VariableName is not an empty string,
                                    while VendorGuid is NULL.
  @retval EFI_SUCCESS               Variable successfully found.
  @retval EFI_NOT_FOUND             Variable not found

**/
EFI_STATUS
AuthServiceInternalFindVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  OUT VOID      **Data,
  OUT UINTN     *DataSize
  )
{
  EFI_STATUS          Status;
  AUTH_VARIABLE_INFO  AuthVariableInfo;

  ZeroMem (&AuthVariableInfo, sizeof (AuthVariableInfo));
  Status = mAuthVarLibContextIn->FindVariable (
                                   VariableName,
                                   VendorGuid,
                                   &AuthVariableInfo
                                   );
  *Data     = AuthVariableInfo.Data;
  *DataSize = AuthVariableInfo.DataSize;
  return Status;
}

/**
  Update the variable region with Variable information.

  @param[in] VariableName           Name of variable.
  @param[in] VendorGuid             Guid of variable.
  @param[in] Data                   Data pointer.
  @param[in] DataSize               Size of Data.
  @param[in] Attributes             Attribute value of the variable.

  @retval EFI_SUCCESS               The update operation is success.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_WRITE_PROTECTED       Variable is write-protected.
  @retval EFI_OUT_OF_RESOURCES      There is not enough resource.

**/
EFI_STATUS
AuthServiceInternalUpdateVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN VOID      *Data,
  IN UINTN     DataSize,
  IN UINT32    Attributes
  )
{
  AUTH_VARIABLE_INFO  AuthVariableInfo;

  ZeroMem (&AuthVariableInfo, sizeof (AuthVariableInfo));
  AuthVariableInfo.VariableName = VariableName;
  AuthVariableInfo.VendorGuid   = VendorGuid;
  AuthVariableInfo.Data         = Data;
  AuthVariableInfo.DataSize     = DataSize;
  AuthVariableInfo.Attributes   = Attributes;

  return mAuthVarLibContextIn->UpdateVariable (
                                 &AuthVariableInfo
                                 );
}

/**
  Update the variable region with Variable information.

  @param[in] VariableName           Name of variable.
  @param[in] VendorGuid             Guid of variable.
  @param[in] Data                   Data pointer.
  @param[in] DataSize               Size of Data.
  @param[in] Attributes             Attribute value of the variable.
  @param[in] TimeStamp              Value of associated TimeStamp.

  @retval EFI_SUCCESS               The update operation is success.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_WRITE_PROTECTED       Variable is write-protected.
  @retval EFI_OUT_OF_RESOURCES      There is not enough resource.

**/
EFI_STATUS
AuthServiceInternalUpdateVariableWithTimeStamp (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN VOID      *Data,
  IN UINTN     DataSize,
  IN UINT32    Attributes,
  IN EFI_TIME  *TimeStamp
  )
{
  EFI_STATUS          FindStatus;
  VOID                *OrgData;
  UINTN               OrgDataSize;
  AUTH_VARIABLE_INFO  AuthVariableInfo;

  FindStatus = AuthServiceInternalFindVariable (
                 VariableName,
                 VendorGuid,
                 &OrgData,
                 &OrgDataSize
                 );

  //
  // EFI_VARIABLE_APPEND_WRITE attribute only effects for existing variable
  //
  if (!EFI_ERROR (FindStatus) && ((Attributes & EFI_VARIABLE_APPEND_WRITE) != 0)) {
    if ((CompareGuid (VendorGuid, &gEfiImageSecurityDatabaseGuid) &&
         ((StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE) == 0) || (StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE1) == 0) ||
          (StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE2) == 0))) ||
        (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) && (StrCmp (VariableName, EFI_KEY_EXCHANGE_KEY_NAME) == 0)))
    {
      //
      // For variables with formatted as EFI_SIGNATURE_LIST, the driver shall not perform an append of
      // EFI_SIGNATURE_DATA values that are already part of the existing variable value.
      //
      FilterSignatureList (
        OrgData,
        OrgDataSize,
        Data,
        &DataSize
        );
    }
  }

  ZeroMem (&AuthVariableInfo, sizeof (AuthVariableInfo));
  AuthVariableInfo.VariableName = VariableName;
  AuthVariableInfo.VendorGuid   = VendorGuid;
  AuthVariableInfo.Data         = Data;
  AuthVariableInfo.DataSize     = DataSize;
  AuthVariableInfo.Attributes   = Attributes;
  AuthVariableInfo.TimeStamp    = TimeStamp;
  return mAuthVarLibContextIn->UpdateVariable (
                                 &AuthVariableInfo
                                 );
}

/**
  Determine whether this operation needs a physical present user.

  @param[in]      VariableName            Name of the Variable.
  @param[in]      VendorGuid              GUID of the Variable.

  @retval TRUE      This variable is protected, only a physical present user could set this variable.
  @retval FALSE     This variable is not protected.

**/
BOOLEAN
NeedPhysicallyPresent (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid
  )
{
  // If the VariablePolicy engine is disabled, allow deletion of any authenticated variables.
  if (IsVariablePolicyEnabled ()) {
    if (  (CompareGuid (VendorGuid, &gEfiSecureBootEnableDisableGuid) && (StrCmp (VariableName, EFI_SECURE_BOOT_ENABLE_NAME) == 0))
       || (CompareGuid (VendorGuid, &gEfiCustomModeEnableGuid) && (StrCmp (VariableName, EFI_CUSTOM_MODE_NAME) == 0)))
    {
      return TRUE;
    }
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
  EFI_STATUS  Status;
  VOID        *Data;
  UINTN       DataSize;

  Status = AuthServiceInternalFindVariable (EFI_CUSTOM_MODE_NAME, &gEfiCustomModeEnableGuid, &Data, &DataSize);
  if (!EFI_ERROR (Status) && (*(UINT8 *)Data == CUSTOM_SECURE_BOOT_MODE)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Update platform mode.

  @param[in]      Mode                    SETUP_MODE or USER_MODE.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SUCCESS                     Update platform mode successfully.

**/
EFI_STATUS
UpdatePlatformMode (
  IN  UINT32  Mode
  )
{
  EFI_STATUS  Status;
  VOID        *Data;
  UINTN       DataSize;
  UINT8       SecureBootMode;
  UINT8       SecureBootEnable;
  UINTN       VariableDataSize;

  Status = AuthServiceInternalFindVariable (
             EFI_SETUP_MODE_NAME,
             &gEfiGlobalVariableGuid,
             &Data,
             &DataSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Update the value of SetupMode variable by a simple mem copy, this could avoid possible
  // variable storage reclaim at runtime.
  //
  mPlatformMode = (UINT8)Mode;
  CopyMem (Data, &mPlatformMode, sizeof (UINT8));

  if (mAuthVarLibContextIn->AtRuntime ()) {
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
  Status = AuthServiceInternalFindVariable (
             EFI_SECURE_BOOT_MODE_NAME,
             &gEfiGlobalVariableGuid,
             &Data,
             &DataSize
             );
  //
  // If "SecureBoot" variable exists, then check "SetupMode" variable update.
  // If "SetupMode" variable is USER_MODE, "SecureBoot" variable is set to 1.
  // If "SetupMode" variable is SETUP_MODE, "SecureBoot" variable is set to 0.
  //
  if (EFI_ERROR (Status)) {
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

  //
  // Check "SecureBootEnable" variable's existence. It can enable/disable secure boot feature.
  //
  Status = AuthServiceInternalFindVariable (
             EFI_SECURE_BOOT_ENABLE_NAME,
             &gEfiSecureBootEnableDisableGuid,
             &Data,
             &DataSize
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
    if (EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }

    SecureBootEnable = SECURE_BOOT_DISABLE;
    VariableDataSize = 0;
  }

  Status = AuthServiceInternalUpdateVariable (
             EFI_SECURE_BOOT_ENABLE_NAME,
             &gEfiSecureBootEnableDisableGuid,
             &SecureBootEnable,
             VariableDataSize,
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS
             );
  return Status;
}

/**
  Check input data form to make sure it is a valid EFI_SIGNATURE_LIST for PK/KEK/db/dbx/dbt variable.

  @param[in]  VariableName                Name of Variable to be check.
  @param[in]  VendorGuid                  Variable vendor GUID.
  @param[in]  Data                        Point to the variable data to be checked.
  @param[in]  DataSize                    Size of Data.

  @return EFI_INVALID_PARAMETER           Invalid signature list format.
  @return EFI_SUCCESS                     Passed signature list format check successfully.

**/
EFI_STATUS
CheckSignatureListFormat (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  VOID      *Data,
  IN  UINTN     DataSize
  )
{
  EFI_SIGNATURE_LIST  *SigList;
  UINTN               SigDataSize;
  UINT32              Index;
  UINT32              SigCount;
  BOOLEAN             IsPk;
  VOID                *RsaContext;
  EFI_SIGNATURE_DATA  *CertData;
  UINTN               CertLen;

  if (DataSize == 0) {
    return EFI_SUCCESS;
  }

  ASSERT (VariableName != NULL && VendorGuid != NULL && Data != NULL);

  if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) && (StrCmp (VariableName, EFI_PLATFORM_KEY_NAME) == 0)) {
    IsPk = TRUE;
  } else if ((CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) && (StrCmp (VariableName, EFI_KEY_EXCHANGE_KEY_NAME) == 0)) ||
             (CompareGuid (VendorGuid, &gEfiImageSecurityDatabaseGuid) &&
              ((StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE) == 0) || (StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE1) == 0) ||
               (StrCmp (VariableName, EFI_IMAGE_SECURITY_DATABASE2) == 0))))
  {
    IsPk = FALSE;
  } else {
    return EFI_SUCCESS;
  }

  SigCount    = 0;
  SigList     = (EFI_SIGNATURE_LIST *)Data;
  SigDataSize = DataSize;
  RsaContext  = NULL;

  //
  // Walk through the input signature list and check the data format.
  // If any signature is incorrectly formed, the whole check will fail.
  //
  while ((SigDataSize > 0) && (SigDataSize >= SigList->SignatureListSize)) {
    for (Index = 0; Index < (sizeof (mSupportSigItem) / sizeof (EFI_SIGNATURE_ITEM)); Index++ ) {
      if (CompareGuid (&SigList->SignatureType, &mSupportSigItem[Index].SigType)) {
        //
        // The value of SignatureSize should always be 16 (size of SignatureOwner
        // component) add the data length according to signature type.
        //
        if ((mSupportSigItem[Index].SigDataSize != ((UINT32) ~0)) &&
            ((SigList->SignatureSize - sizeof (EFI_GUID)) != mSupportSigItem[Index].SigDataSize))
        {
          return EFI_INVALID_PARAMETER;
        }

        if ((mSupportSigItem[Index].SigHeaderSize != ((UINT32) ~0)) &&
            (SigList->SignatureHeaderSize != mSupportSigItem[Index].SigHeaderSize))
        {
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

    if (CompareGuid (&SigList->SignatureType, &gEfiCertX509Guid)) {
      //
      // Try to retrieve the RSA public key from the X.509 certificate.
      // If this operation fails, it's not a valid certificate.
      //
      RsaContext = RsaNew ();
      if (RsaContext == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      CertData = (EFI_SIGNATURE_DATA *)((UINT8 *)SigList + sizeof (EFI_SIGNATURE_LIST) + SigList->SignatureHeaderSize);
      CertLen  = SigList->SignatureSize - sizeof (EFI_GUID);
      if (!RsaGetPublicKeyFromX509 (CertData->SignatureData, CertLen, &RsaContext)) {
        RsaFree (RsaContext);
        return EFI_INVALID_PARAMETER;
      }

      RsaFree (RsaContext);
    }

    if ((SigList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - SigList->SignatureHeaderSize) % SigList->SignatureSize != 0) {
      return EFI_INVALID_PARAMETER;
    }

    SigCount += (SigList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - SigList->SignatureHeaderSize) / SigList->SignatureSize;

    SigDataSize -= SigList->SignatureListSize;
    SigList      = (EFI_SIGNATURE_LIST *)((UINT8 *)SigList + SigList->SignatureListSize);
  }

  if (((UINTN)SigList - (UINTN)Data) != DataSize) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsPk && (SigCount > 1)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Update "VendorKeys" variable to record the out of band secure boot key modification.

  @return EFI_SUCCESS           Variable is updated successfully.
  @return Others                Failed to update variable.

**/
EFI_STATUS
VendorKeyIsModified (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mVendorKeyState == VENDOR_KEYS_MODIFIED) {
    return EFI_SUCCESS;
  }

  mVendorKeyState = VENDOR_KEYS_MODIFIED;

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

  return AuthServiceInternalUpdateVariable (
           EFI_VENDOR_KEYS_VARIABLE_NAME,
           &gEfiGlobalVariableGuid,
           &mVendorKeyState,
           sizeof (UINT8),
           EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS
           );
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
  @param[in]  Attributes                  Attribute value of the variable
  @param[in]  IsPk                        Indicate whether it is to process pk.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SECURITY_VIOLATION          The variable does NOT pass the validation.
                                          check carried out by the firmware.
  @return EFI_SUCCESS                     Variable passed validation successfully.

**/
EFI_STATUS
ProcessVarWithPk (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  VOID      *Data,
  IN  UINTN     DataSize,
  IN  UINT32    Attributes OPTIONAL,
  IN  BOOLEAN   IsPk
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Del;
  UINT8       *Payload;
  UINTN       PayloadSize;

  if (((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) ||
      ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == 0))
  {
    //
    // PK, KEK and db/dbx/dbt should set EFI_VARIABLE_NON_VOLATILE attribute and should be a time-based
    // authenticated variable.
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Init state of Del. State may change due to secure check
  //
  Del = FALSE;
  if (  (InCustomMode () && UserPhysicalPresent ())
     || (  (mPlatformMode == SETUP_MODE)
        && !(FeaturePcdGet (PcdRequireSelfSignedPk) && IsPk)))
  {
    Payload     = (UINT8 *)Data + AUTHINFO2_SIZE (Data);
    PayloadSize = DataSize - AUTHINFO2_SIZE (Data);
    if (PayloadSize == 0) {
      Del = TRUE;
    }

    Status = CheckSignatureListFormat (VariableName, VendorGuid, Payload, PayloadSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AuthServiceInternalUpdateVariableWithTimeStamp (
               VariableName,
               VendorGuid,
               Payload,
               PayloadSize,
               Attributes,
               &((EFI_VARIABLE_AUTHENTICATION_2 *)Data)->TimeStamp
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (  (mPlatformMode != SETUP_MODE)
       || (FeaturePcdGet (PcdRequireSelfSignedPk) && IsPk))
    {
      Status = VendorKeyIsModified ();
    }
  } else if (mPlatformMode == USER_MODE) {
    //
    // Verify against X509 Cert in PK database.
    //
    Status = VerifyTimeBasedPayloadAndUpdate (
               VariableName,
               VendorGuid,
               Data,
               DataSize,
               Attributes,
               AuthVarTypePk,
               &Del
               );
  } else {
    //
    // Verify against the certificate in data payload.
    //
    Status = VerifyTimeBasedPayloadAndUpdate (
               VariableName,
               VendorGuid,
               Data,
               DataSize,
               Attributes,
               AuthVarTypePayload,
               &Del
               );
  }

  if (!EFI_ERROR (Status) && IsPk) {
    if ((mPlatformMode == SETUP_MODE) && !Del) {
      //
      // If enroll PK in setup mode, need change to user mode.
      //
      Status = UpdatePlatformMode (USER_MODE);
    } else if ((mPlatformMode == USER_MODE) && Del) {
      //
      // If delete PK in user mode, need change to setup mode.
      //
      Status = UpdatePlatformMode (SETUP_MODE);
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
  @param[in]  Attributes                  Attribute value of the variable.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SECURITY_VIOLATION          The variable does NOT pass the validation
                                          check carried out by the firmware.
  @return EFI_SUCCESS                     Variable pass validation successfully.

**/
EFI_STATUS
ProcessVarWithKek (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  VOID      *Data,
  IN  UINTN     DataSize,
  IN  UINT32    Attributes OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINT8       *Payload;
  UINTN       PayloadSize;

  if (((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) ||
      ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == 0))
  {
    //
    // DB, DBX and DBT should set EFI_VARIABLE_NON_VOLATILE attribute and should be a time-based
    // authenticated variable.
    //
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  if ((mPlatformMode == USER_MODE) && !(InCustomMode () && UserPhysicalPresent ())) {
    //
    // Time-based, verify against X509 Cert KEK.
    //
    return VerifyTimeBasedPayloadAndUpdate (
             VariableName,
             VendorGuid,
             Data,
             DataSize,
             Attributes,
             AuthVarTypeKek,
             NULL
             );
  } else {
    //
    // If in setup mode or custom secure boot mode, no authentication needed.
    //
    Payload     = (UINT8 *)Data + AUTHINFO2_SIZE (Data);
    PayloadSize = DataSize - AUTHINFO2_SIZE (Data);

    Status = CheckSignatureListFormat (VariableName, VendorGuid, Payload, PayloadSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AuthServiceInternalUpdateVariableWithTimeStamp (
               VariableName,
               VendorGuid,
               Payload,
               PayloadSize,
               Attributes,
               &((EFI_VARIABLE_AUTHENTICATION_2 *)Data)->TimeStamp
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (mPlatformMode != SETUP_MODE) {
      Status = VendorKeyIsModified ();
    }
  }

  return Status;
}

/**
  Check if it is to delete auth variable.

  @param[in] OrgAttributes      Original attribute value of the variable.
  @param[in] Data               Data pointer.
  @param[in] DataSize           Size of Data.
  @param[in] Attributes         Attribute value of the variable.

  @retval TRUE                  It is to delete auth variable.
  @retval FALSE                 It is not to delete auth variable.

**/
BOOLEAN
IsDeleteAuthVariable (
  IN  UINT32  OrgAttributes,
  IN  VOID    *Data,
  IN  UINTN   DataSize,
  IN  UINT32  Attributes
  )
{
  BOOLEAN  Del;
  UINTN    PayloadSize;

  Del = FALSE;

  //
  // To delete a variable created with the EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS
  // or the EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute,
  // SetVariable must be used with attributes matching the existing variable
  // and the DataSize set to the size of the AuthInfo descriptor.
  //
  if ((Attributes == OrgAttributes) &&
      ((Attributes & (EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)) != 0))
  {
    if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) {
      PayloadSize = DataSize - AUTHINFO2_SIZE (Data);
      if (PayloadSize == 0) {
        Del = TRUE;
      }
    } else {
      PayloadSize = DataSize - AUTHINFO_SIZE;
      if (PayloadSize == 0) {
        Del = TRUE;
      }
    }
  }

  return Del;
}

/**
  Process variable with EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.
  This function will check attribute carefully to avoid authentication bypass.

  @param[in]  VariableName                Name of the variable.
  @param[in]  VendorGuid                  Variable vendor GUID.
  @param[in]  Data                        Data pointer.
  @param[in]  DataSize                    Size of Data.
  @param[in]  Attributes                  Attribute value of the variable.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_WRITE_PROTECTED             Variable is write-protected and needs authentication with
                                          EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS or EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS set.
  @return EFI_OUT_OF_RESOURCES            The Database to save the public key is full.
  @return EFI_SECURITY_VIOLATION          The variable is with EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS
                                          set, but the AuthInfo does NOT pass the validation
                                          check carried out by the firmware.
  @return EFI_SUCCESS                     Variable is not write-protected or pass validation successfully.

**/
EFI_STATUS
ProcessVariable (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  IN     VOID      *Data,
  IN     UINTN     DataSize,
  IN     UINT32    Attributes
  )
{
  EFI_STATUS          Status;
  AUTH_VARIABLE_INFO  OrgVariableInfo;

  Status = EFI_SUCCESS;

  ZeroMem (&OrgVariableInfo, sizeof (OrgVariableInfo));
  Status = mAuthVarLibContextIn->FindVariable (
                                   VariableName,
                                   VendorGuid,
                                   &OrgVariableInfo
                                   );

  // If the VariablePolicy engine is disabled, allow deletion of any authenticated variables.
  if ((!EFI_ERROR (Status)) && IsDeleteAuthVariable (OrgVariableInfo.Attributes, Data, DataSize, Attributes) && (UserPhysicalPresent () || !IsVariablePolicyEnabled ())) {
    //
    // Allow the delete operation of common authenticated variable(AT or AW) at user physical presence.
    //
    Status = AuthServiceInternalUpdateVariable (
               VariableName,
               VendorGuid,
               NULL,
               0,
               0
               );
    if (!EFI_ERROR (Status) && ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0)) {
      Status = DeleteCertsFromDb (VariableName, VendorGuid, Attributes);
    }

    return Status;
  }

  if (NeedPhysicallyPresent (VariableName, VendorGuid) && !UserPhysicalPresent ()) {
    //
    // This variable is protected, only physical present user could modify its value.
    //
    return EFI_SECURITY_VIOLATION;
  }

  //
  if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
    //
    // Reject Counter Based Auth Variable processing request.
    //
    return EFI_UNSUPPORTED;
  } else if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) {
    //
    // Process Time-based Authenticated variable.
    //
    return VerifyTimeBasedPayloadAndUpdate (
             VariableName,
             VendorGuid,
             Data,
             DataSize,
             Attributes,
             AuthVarTypePriv,
             NULL
             );
  }

  if ((OrgVariableInfo.Data != NULL) &&
      ((OrgVariableInfo.Attributes & (EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)) != 0))
  {
    //
    // If the variable is already write-protected, it always needs authentication before update.
    //
    return EFI_WRITE_PROTECTED;
  }

  //
  // Not authenticated variable, just update variable as usual.
  //
  Status = AuthServiceInternalUpdateVariable (VariableName, VendorGuid, Data, DataSize, Attributes);
  return Status;
}

/**
  Filter out the duplicated EFI_SIGNATURE_DATA from the new data by comparing to the original data.

  @param[in]        Data          Pointer to original EFI_SIGNATURE_LIST.
  @param[in]        DataSize      Size of Data buffer.
  @param[in, out]   NewData       Pointer to new EFI_SIGNATURE_LIST.
  @param[in, out]   NewDataSize   Size of NewData buffer.

**/
EFI_STATUS
FilterSignatureList (
  IN     VOID   *Data,
  IN     UINTN  DataSize,
  IN OUT VOID   *NewData,
  IN OUT UINTN  *NewDataSize
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
  UINT8               *TempData;
  UINTN               TempDataSize;
  EFI_STATUS          Status;

  if (*NewDataSize == 0) {
    return EFI_SUCCESS;
  }

  TempDataSize = *NewDataSize;
  Status       = mAuthVarLibContextIn->GetScratchBuffer (&TempDataSize, (VOID **)&TempData);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  Tail = TempData;

  NewCertList = (EFI_SIGNATURE_LIST *)NewData;
  while ((*NewDataSize > 0) && (*NewDataSize >= NewCertList->SignatureListSize)) {
    NewCert      = (EFI_SIGNATURE_DATA *)((UINT8 *)NewCertList + sizeof (EFI_SIGNATURE_LIST) + NewCertList->SignatureHeaderSize);
    NewCertCount = (NewCertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - NewCertList->SignatureHeaderSize) / NewCertList->SignatureSize;

    CopiedCount = 0;
    for (Index = 0; Index < NewCertCount; Index++) {
      IsNewCert = TRUE;

      Size     = DataSize;
      CertList = (EFI_SIGNATURE_LIST *)Data;
      while ((Size > 0) && (Size >= CertList->SignatureListSize)) {
        if (CompareGuid (&CertList->SignatureType, &NewCertList->SignatureType) &&
            (CertList->SignatureSize == NewCertList->SignatureSize))
        {
          Cert      = (EFI_SIGNATURE_DATA *)((UINT8 *)CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
          CertCount = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;
          for (Index2 = 0; Index2 < CertCount; Index2++) {
            //
            // Iterate each Signature Data in this Signature List.
            //
            if (CompareMem (NewCert, Cert, CertList->SignatureSize) == 0) {
              IsNewCert = FALSE;
              break;
            }

            Cert = (EFI_SIGNATURE_DATA *)((UINT8 *)Cert + CertList->SignatureSize);
          }
        }

        if (!IsNewCert) {
          break;
        }

        Size    -= CertList->SignatureListSize;
        CertList = (EFI_SIGNATURE_LIST *)((UINT8 *)CertList + CertList->SignatureListSize);
      }

      if (IsNewCert) {
        //
        // New EFI_SIGNATURE_DATA, keep it.
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

      NewCert = (EFI_SIGNATURE_DATA *)((UINT8 *)NewCert + NewCertList->SignatureSize);
    }

    //
    // Update SignatureListSize in the kept EFI_SIGNATURE_LIST.
    //
    if (CopiedCount != 0) {
      SignatureListSize           = sizeof (EFI_SIGNATURE_LIST) + NewCertList->SignatureHeaderSize + (CopiedCount * NewCertList->SignatureSize);
      CertList                    = (EFI_SIGNATURE_LIST *)(Tail - SignatureListSize);
      CertList->SignatureListSize = (UINT32)SignatureListSize;
    }

    *NewDataSize -= NewCertList->SignatureListSize;
    NewCertList   = (EFI_SIGNATURE_LIST *)((UINT8 *)NewCertList + NewCertList->SignatureListSize);
  }

  TempDataSize = (Tail - (UINT8 *)TempData);

  CopyMem (NewData, TempData, TempDataSize);
  *NewDataSize = TempDataSize;

  return EFI_SUCCESS;
}

/**
  Compare two EFI_TIME data.


  @param FirstTime           A pointer to the first EFI_TIME data.
  @param SecondTime          A pointer to the second EFI_TIME data.

  @retval  TRUE              The FirstTime is not later than the SecondTime.
  @retval  FALSE             The FirstTime is later than the SecondTime.

**/
BOOLEAN
AuthServiceInternalCompareTimeStamp (
  IN EFI_TIME  *FirstTime,
  IN EFI_TIME  *SecondTime
  )
{
  if (FirstTime->Year != SecondTime->Year) {
    return (BOOLEAN)(FirstTime->Year < SecondTime->Year);
  } else if (FirstTime->Month != SecondTime->Month) {
    return (BOOLEAN)(FirstTime->Month < SecondTime->Month);
  } else if (FirstTime->Day != SecondTime->Day) {
    return (BOOLEAN)(FirstTime->Day < SecondTime->Day);
  } else if (FirstTime->Hour != SecondTime->Hour) {
    return (BOOLEAN)(FirstTime->Hour < SecondTime->Hour);
  } else if (FirstTime->Minute != SecondTime->Minute) {
    return (BOOLEAN)(FirstTime->Minute < SecondTime->Minute);
  }

  return (BOOLEAN)(FirstTime->Second <= SecondTime->Second);
}

/**
  Calculate SHA digest of SignerCert CommonName + ToplevelCert tbsCertificate.
  SignerCert and ToplevelCert are inside the signer certificate chain.

  @param[in]  HashAlgId           Hash algorithm index.
  @param[in]  SignerCert          A pointer to SignerCert data.
  @param[in]  SignerCertSize      Length of SignerCert data.
  @param[in]  TopLevelCert        A pointer to TopLevelCert data.
  @param[in]  TopLevelCertSize    Length of TopLevelCert data.
  @param[out] ShaDigest           Sha digest calculated.

  @return EFI_ABORTED          Digest process failed.
  @return EFI_SUCCESS          SHA Digest is successfully calculated.

**/
EFI_STATUS
CalculatePrivAuthVarSignChainSHADigest (
  IN     UINT8  HashAlgId,
  IN     UINT8  *SignerCert,
  IN     UINTN  SignerCertSize,
  IN     UINT8  *TopLevelCert,
  IN     UINTN  TopLevelCertSize,
  OUT    UINT8  *ShaDigest
  )
{
  UINT8       *TbsCert;
  UINTN       TbsCertSize;
  CHAR8       CertCommonName[128];
  UINTN       CertCommonNameSize;
  BOOLEAN     CryptoStatus;
  EFI_STATUS  Status;

  if (HashAlgId >= (sizeof (mHashInfo) / sizeof (EFI_HASH_INFO))) {
    DEBUG ((DEBUG_INFO, "%a Unsupported Hash Algorithm %d\n", __func__, HashAlgId));
    return EFI_ABORTED;
  }

  CertCommonNameSize = sizeof (CertCommonName);

  //
  // Get SignerCert CommonName
  //
  Status = X509GetCommonName (SignerCert, SignerCertSize, CertCommonName, &CertCommonNameSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a Get SignerCert CommonName failed with status %x\n", __func__, Status));
    return EFI_ABORTED;
  }

  //
  // Get TopLevelCert tbsCertificate
  //
  if (!X509GetTBSCert (TopLevelCert, TopLevelCertSize, &TbsCert, &TbsCertSize)) {
    DEBUG ((DEBUG_INFO, "%a Get Top-level Cert tbsCertificate failed!\n", __func__));
    return EFI_ABORTED;
  }

  //
  // Digest SignerCert CN + TopLevelCert tbsCertificate
  //
  ZeroMem (ShaDigest, mHashInfo[HashAlgId].HashSize);
  CryptoStatus = mHashInfo[HashAlgId].Init (*(mHashInfo[HashAlgId].HashShaCtx));
  if (!CryptoStatus) {
    return EFI_ABORTED;
  }

  //
  // '\0' is forced in CertCommonName. No overflow issue
  //
  CryptoStatus = mHashInfo[HashAlgId].Update (
                                        *(mHashInfo[HashAlgId].HashShaCtx),
                                        CertCommonName,
                                        AsciiStrLen (CertCommonName)
                                        );
  if (!CryptoStatus) {
    return EFI_ABORTED;
  }

  CryptoStatus = mHashInfo[HashAlgId].Update (*(mHashInfo[HashAlgId].HashShaCtx), TbsCert, TbsCertSize);
  if (!CryptoStatus) {
    return EFI_ABORTED;
  }

  CryptoStatus = mHashInfo[HashAlgId].Final (*(mHashInfo[HashAlgId].HashShaCtx), ShaDigest);
  if (!CryptoStatus) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
  Find matching signer's certificates for common authenticated variable
  by corresponding VariableName and VendorGuid from "certdb" or "certdbv".

  The data format of "certdb" or "certdbv":
  //
  //     UINT32 CertDbListSize;
  // /// AUTH_CERT_DB_DATA Certs1[];
  // /// AUTH_CERT_DB_DATA Certs2[];
  // /// ...
  // /// AUTH_CERT_DB_DATA Certsn[];
  //

  @param[in]  VariableName   Name of authenticated Variable.
  @param[in]  VendorGuid     Vendor GUID of authenticated Variable.
  @param[in]  Data           Pointer to variable "certdb" or "certdbv".
  @param[in]  DataSize       Size of variable "certdb" or "certdbv".
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
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  IN     UINT8     *Data,
  IN     UINTN     DataSize,
  OUT    UINT32    *CertOffset     OPTIONAL,
  OUT    UINT32    *CertDataSize   OPTIONAL,
  OUT    UINT32    *CertNodeOffset OPTIONAL,
  OUT    UINT32    *CertNodeSize   OPTIONAL
  )
{
  UINT32             Offset;
  AUTH_CERT_DB_DATA  *Ptr;
  UINT32             CertSize;
  UINT32             NameSize;
  UINT32             NodeSize;
  UINT32             CertDbListSize;

  if ((VariableName == NULL) || (VendorGuid == NULL) || (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether DataSize matches recorded CertDbListSize.
  //
  if (DataSize < sizeof (UINT32)) {
    return EFI_INVALID_PARAMETER;
  }

  CertDbListSize = ReadUnaligned32 ((UINT32 *)Data);

  if (CertDbListSize != (UINT32)DataSize) {
    return EFI_INVALID_PARAMETER;
  }

  Offset = sizeof (UINT32);

  //
  // Get corresponding certificates by VendorGuid and VariableName.
  //
  while (Offset < (UINT32)DataSize) {
    Ptr = (AUTH_CERT_DB_DATA *)(Data + Offset);
    //
    // Check whether VendorGuid matches.
    //
    if (CompareGuid (&Ptr->VendorGuid, VendorGuid)) {
      NodeSize = ReadUnaligned32 (&Ptr->CertNodeSize);
      NameSize = ReadUnaligned32 (&Ptr->NameSize);
      CertSize = ReadUnaligned32 (&Ptr->CertDataSize);

      if (NodeSize != sizeof (EFI_GUID) + sizeof (UINT32) * 3 + CertSize +
          sizeof (CHAR16) * NameSize)
      {
        return EFI_INVALID_PARAMETER;
      }

      Offset = Offset + sizeof (EFI_GUID) + sizeof (UINT32) * 3;
      //
      // Check whether VariableName matches.
      //
      if ((NameSize == StrLen (VariableName)) &&
          (CompareMem (Data + Offset, VariableName, NameSize * sizeof (CHAR16)) == 0))
      {
        Offset = Offset + NameSize * sizeof (CHAR16);

        if (CertOffset != NULL) {
          *CertOffset = Offset;
        }

        if (CertDataSize != NULL) {
          *CertDataSize = CertSize;
        }

        if (CertNodeOffset != NULL) {
          *CertNodeOffset = (UINT32)((UINT8 *)Ptr - Data);
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
  by corresponding VariableName and VendorGuid from "certdb"
  or "certdbv" according to authenticated variable attributes.

  @param[in]  VariableName   Name of authenticated Variable.
  @param[in]  VendorGuid     Vendor GUID of authenticated Variable.
  @param[in]  Attributes        Attributes of authenticated variable.
  @param[out] CertData       Pointer to signer's certificates.
  @param[out] CertDataSize   Length of CertData in bytes.

  @retval  EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval  EFI_NOT_FOUND         Fail to find "certdb"/"certdbv" or matching certs.
  @retval  EFI_SUCCESS           Get signer's certificates successfully.

**/
EFI_STATUS
GetCertsFromDb (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  IN     UINT32    Attributes,
  OUT    UINT8     **CertData,
  OUT    UINT32    *CertDataSize
  )
{
  EFI_STATUS  Status;
  UINT8       *Data;
  UINTN       DataSize;
  UINT32      CertOffset;
  CHAR16      *DbName;

  if ((VariableName == NULL) || (VendorGuid == NULL) || (CertData == NULL) || (CertDataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
    //
    // Get variable "certdb".
    //
    DbName = EFI_CERT_DB_NAME;
  } else {
    //
    // Get variable "certdbv".
    //
    DbName = EFI_CERT_DB_VOLATILE_NAME;
  }

  //
  // Get variable "certdb" or "certdbv".
  //
  Status = AuthServiceInternalFindVariable (
             DbName,
             &gEfiCertDbGuid,
             (VOID **)&Data,
             &DataSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

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
  variable by corresponding VariableName and VendorGuid from "certdb" or
  "certdbv" according to authenticated variable attributes.

  @param[in]  VariableName   Name of authenticated Variable.
  @param[in]  VendorGuid     Vendor GUID of authenticated Variable.
  @param[in]  Attributes        Attributes of authenticated variable.

  @retval  EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval  EFI_NOT_FOUND         Fail to find "certdb"/"certdbv" or matching certs.
  @retval  EFI_OUT_OF_RESOURCES  The operation is failed due to lack of resources.
  @retval  EFI_SUCCESS           The operation is completed successfully.

**/
EFI_STATUS
DeleteCertsFromDb (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  IN     UINT32    Attributes
  )
{
  EFI_STATUS  Status;
  UINT8       *Data;
  UINTN       DataSize;
  UINT32      VarAttr;
  UINT32      CertNodeOffset;
  UINT32      CertNodeSize;
  UINT8       *NewCertDb;
  UINT32      NewCertDbSize;
  CHAR16      *DbName;

  if ((VariableName == NULL) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
    //
    // Get variable "certdb".
    //
    DbName  = EFI_CERT_DB_NAME;
    VarAttr = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
  } else {
    //
    // Get variable "certdbv".
    //
    DbName  = EFI_CERT_DB_VOLATILE_NAME;
    VarAttr = EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
  }

  Status = AuthServiceInternalFindVariable (
             DbName,
             &gEfiCertDbGuid,
             (VOID **)&Data,
             &DataSize
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((DataSize == 0) || (Data == NULL)) {
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  if (DataSize == sizeof (UINT32)) {
    //
    // There is no certs in "certdb" or "certdbv".
    //
    return EFI_SUCCESS;
  }

  //
  // Get corresponding cert node from "certdb" or "certdbv".
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
  // Construct new data content of variable "certdb" or "certdbv".
  //
  NewCertDbSize = (UINT32)DataSize - CertNodeSize;
  NewCertDb     = (UINT8 *)mCertDbStore;

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
  // Set "certdb" or "certdbv".
  //
  Status = AuthServiceInternalUpdateVariable (
             DbName,
             &gEfiCertDbGuid,
             NewCertDb,
             NewCertDbSize,
             VarAttr
             );

  return Status;
}

/**
  Insert signer's certificates for common authenticated variable with VariableName
  and VendorGuid in AUTH_CERT_DB_DATA to "certdb" or "certdbv" according to
  time based authenticated variable attributes. CertData is the SHA digest of
  SignerCert CommonName + TopLevelCert tbsCertificate.

  @param[in]  HashAlgId         Hash algorithm index.
  @param[in]  VariableName      Name of authenticated Variable.
  @param[in]  VendorGuid        Vendor GUID of authenticated Variable.
  @param[in]  Attributes        Attributes of authenticated variable.
  @param[in]  SignerCert        Signer certificate data.
  @param[in]  SignerCertSize    Length of signer certificate.
  @param[in]  TopLevelCert      Top-level certificate data.
  @param[in]  TopLevelCertSize  Length of top-level certificate.

  @retval  EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval  EFI_ACCESS_DENIED     An AUTH_CERT_DB_DATA entry with same VariableName
                                 and VendorGuid already exists.
  @retval  EFI_OUT_OF_RESOURCES  The operation is failed due to lack of resources.
  @retval  EFI_SUCCESS           Insert an AUTH_CERT_DB_DATA entry to "certdb" or "certdbv"

**/
EFI_STATUS
InsertCertsToDb (
  IN     UINT8     HashAlgId,
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  IN     UINT32    Attributes,
  IN     UINT8     *SignerCert,
  IN     UINTN     SignerCertSize,
  IN     UINT8     *TopLevelCert,
  IN     UINTN     TopLevelCertSize
  )
{
  EFI_STATUS         Status;
  UINT8              *Data;
  UINTN              DataSize;
  UINT32             VarAttr;
  UINT8              *NewCertDb;
  UINT32             NewCertDbSize;
  UINT32             CertNodeSize;
  UINT32             NameSize;
  UINT32             CertDataSize;
  AUTH_CERT_DB_DATA  *Ptr;
  CHAR16             *DbName;
  UINT8              ShaDigest[SHA_DIGEST_SIZE_MAX];

  if ((VariableName == NULL) || (VendorGuid == NULL) || (SignerCert == NULL) || (TopLevelCert == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (HashAlgId >= (sizeof (mHashInfo) / sizeof (EFI_HASH_INFO))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
    //
    // Get variable "certdb".
    //
    DbName  = EFI_CERT_DB_NAME;
    VarAttr = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
  } else {
    //
    // Get variable "certdbv".
    //
    DbName  = EFI_CERT_DB_VOLATILE_NAME;
    VarAttr = EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
  }

  //
  // Get variable "certdb" or "certdbv".
  //
  Status = AuthServiceInternalFindVariable (
             DbName,
             &gEfiCertDbGuid,
             (VOID **)&Data,
             &DataSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((DataSize == 0) || (Data == NULL)) {
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  //
  // Find whether matching cert node already exists in "certdb" or "certdbv".
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
  // Construct new data content of variable "certdb" or "certdbv".
  //
  NameSize      = (UINT32)StrLen (VariableName);
  CertDataSize  = mHashInfo[HashAlgId].HashSize;
  CertNodeSize  = sizeof (AUTH_CERT_DB_DATA) + (UINT32)CertDataSize + NameSize * sizeof (CHAR16);
  NewCertDbSize = (UINT32)DataSize + CertNodeSize;
  if (NewCertDbSize > mMaxCertDbSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = CalculatePrivAuthVarSignChainSHADigest (
             HashAlgId,
             SignerCert,
             SignerCertSize,
             TopLevelCert,
             TopLevelCertSize,
             ShaDigest
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  NewCertDb = (UINT8 *)mCertDbStore;

  //
  // Copy the DB entries before inserting node.
  //
  CopyMem (NewCertDb, Data, DataSize);
  //
  // Update CertDbListSize.
  //
  CopyMem (NewCertDb, &NewCertDbSize, sizeof (UINT32));
  //
  // Construct new cert node.
  //
  Ptr = (AUTH_CERT_DB_DATA *)(NewCertDb + DataSize);
  CopyGuid (&Ptr->VendorGuid, VendorGuid);
  CopyMem (&Ptr->CertNodeSize, &CertNodeSize, sizeof (UINT32));
  CopyMem (&Ptr->NameSize, &NameSize, sizeof (UINT32));
  CopyMem (&Ptr->CertDataSize, &CertDataSize, sizeof (UINT32));

  CopyMem (
    (UINT8 *)Ptr + sizeof (AUTH_CERT_DB_DATA),
    VariableName,
    NameSize * sizeof (CHAR16)
    );

  CopyMem (
    (UINT8 *)Ptr +  sizeof (AUTH_CERT_DB_DATA) + NameSize * sizeof (CHAR16),
    ShaDigest,
    CertDataSize
    );

  //
  // Set "certdb" or "certdbv".
  //
  Status = AuthServiceInternalUpdateVariable (
             DbName,
             &gEfiCertDbGuid,
             NewCertDb,
             NewCertDbSize,
             VarAttr
             );

  return Status;
}

/**
  Clean up signer's certificates for common authenticated variable
  by corresponding VariableName and VendorGuid from "certdb".
  System may break down during Timebased Variable update & certdb update,
  make them inconsistent,  this function is called in AuthVariable Init
  to ensure consistency.

  @retval  EFI_NOT_FOUND         Fail to find variable "certdb".
  @retval  EFI_OUT_OF_RESOURCES  The operation is failed due to lack of resources.
  @retval  EFI_SUCCESS           The operation is completed successfully.

**/
EFI_STATUS
CleanCertsFromDb (
  VOID
  )
{
  UINT32              Offset;
  AUTH_CERT_DB_DATA   *Ptr;
  UINT32              NameSize;
  UINT32              NodeSize;
  CHAR16              *VariableName;
  EFI_STATUS          Status;
  BOOLEAN             CertCleaned;
  UINT8               *Data;
  UINTN               DataSize;
  EFI_GUID            AuthVarGuid;
  AUTH_VARIABLE_INFO  AuthVariableInfo;

  Status = EFI_SUCCESS;

  //
  // Get corresponding certificates by VendorGuid and VariableName.
  //
  do {
    CertCleaned = FALSE;

    //
    // Get latest variable "certdb"
    //
    Status = AuthServiceInternalFindVariable (
               EFI_CERT_DB_NAME,
               &gEfiCertDbGuid,
               (VOID **)&Data,
               &DataSize
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((DataSize == 0) || (Data == NULL)) {
      ASSERT (FALSE);
      return EFI_NOT_FOUND;
    }

    Offset = sizeof (UINT32);

    while (Offset < (UINT32)DataSize) {
      Ptr      = (AUTH_CERT_DB_DATA *)(Data + Offset);
      NodeSize = ReadUnaligned32 (&Ptr->CertNodeSize);
      NameSize = ReadUnaligned32 (&Ptr->NameSize);

      //
      // Get VarName tailed with '\0'
      //
      VariableName = AllocateZeroPool ((NameSize + 1) * sizeof (CHAR16));
      if (VariableName == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      CopyMem (VariableName, (UINT8 *)Ptr + sizeof (AUTH_CERT_DB_DATA), NameSize * sizeof (CHAR16));
      //
      // Keep VarGuid  aligned
      //
      CopyMem (&AuthVarGuid, &Ptr->VendorGuid, sizeof (EFI_GUID));

      //
      // Find corresponding time auth variable
      //
      ZeroMem (&AuthVariableInfo, sizeof (AuthVariableInfo));
      Status = mAuthVarLibContextIn->FindVariable (
                                       VariableName,
                                       &AuthVarGuid,
                                       &AuthVariableInfo
                                       );

      if (EFI_ERROR (Status) || ((AuthVariableInfo.Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == 0)) {
        //
        // While cleaning certdb, always delete the variable in certdb regardless of it attributes.
        //
        Status = DeleteCertsFromDb (
                   VariableName,
                   &AuthVarGuid,
                   AuthVariableInfo.Attributes | EFI_VARIABLE_NON_VOLATILE
                   );
        CertCleaned = TRUE;
        DEBUG ((DEBUG_INFO, "Recovery!! Cert for Auth Variable %s Guid %g is removed for consistency\n", VariableName, &AuthVarGuid));
        FreePool (VariableName);
        break;
      }

      FreePool (VariableName);
      Offset = Offset + NodeSize;
    }
  } while (CertCleaned);

  return Status;
}

/**
  Find hash algorithm index.

  @param[in]  SigData      Pointer to the PKCS#7 message.
  @param[in]  SigDataSize  Length of the PKCS#7 message.

  @retval UINT8        Hash Algorithm Index.
**/
UINT8
FindHashAlgorithmIndex (
  IN     UINT8   *SigData,
  IN     UINT32  SigDataSize
  )
{
  UINT8  i;

  for (i = 0; i < (sizeof (mHashInfo) / sizeof (EFI_HASH_INFO)); i++) {
    if (  (  (SigDataSize >= (13 + mHashInfo[i].OidLength))
          && (  ((*(SigData + 1) & TWO_BYTE_ENCODE) == TWO_BYTE_ENCODE)
             && (CompareMem (SigData + 13, mHashInfo[i].OidValue, mHashInfo[i].OidLength) == 0)))
       || (  ((SigDataSize >= (32 +  mHashInfo[i].OidLength)))
          && (  ((*(SigData + 20) & TWO_BYTE_ENCODE) == TWO_BYTE_ENCODE)
             && (CompareMem (SigData + 32, mHashInfo[i].OidValue, mHashInfo[i].OidLength) == 0))))
    {
      break;
    }
  }

  return i;
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
  @param[in]  Attributes                  Attribute value of the variable.
  @param[in]  AuthVarType                 Verify against PK, KEK database, private database or certificate in data payload.
  @param[in]  OrgTimeStamp                Pointer to original time stamp,
                                          original variable is not found if NULL.
  @param[out]  VarPayloadPtr              Pointer to variable payload address.
  @param[out]  VarPayloadSize             Pointer to variable payload size.

  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval EFI_SECURITY_VIOLATION          The variable does NOT pass the validation
                                          check carried out by the firmware.
  @retval EFI_OUT_OF_RESOURCES            Failed to process variable due to lack
                                          of resources.
  @retval EFI_SUCCESS                     Variable pass validation successfully.

**/
EFI_STATUS
VerifyTimeBasedPayload (
  IN     CHAR16        *VariableName,
  IN     EFI_GUID      *VendorGuid,
  IN     VOID          *Data,
  IN     UINTN         DataSize,
  IN     UINT32        Attributes,
  IN     AUTHVAR_TYPE  AuthVarType,
  IN     EFI_TIME      *OrgTimeStamp,
  OUT    UINT8         **VarPayloadPtr,
  OUT    UINTN         *VarPayloadSize
  )
{
  EFI_VARIABLE_AUTHENTICATION_2  *CertData;
  UINT8                          *SigData;
  UINT32                         SigDataSize;
  UINT8                          *PayloadPtr;
  UINTN                          PayloadSize;
  UINT32                         Attr;
  BOOLEAN                        VerifyStatus;
  EFI_STATUS                     Status;
  EFI_SIGNATURE_LIST             *CertList;
  EFI_SIGNATURE_DATA             *Cert;
  UINTN                          Index;
  UINTN                          CertCount;
  UINT32                         KekDataSize;
  UINT8                          *NewData;
  UINTN                          NewDataSize;
  UINT8                          *Buffer;
  UINTN                          Length;
  UINT8                          *TopLevelCert;
  UINTN                          TopLevelCertSize;
  UINT8                          *TrustedCert;
  UINTN                          TrustedCertSize;
  UINT8                          *SignerCerts;
  UINTN                          CertStackSize;
  UINT8                          *CertsInCertDb;
  UINT32                         CertsSizeinDb;
  UINT8                          ShaDigest[SHA_DIGEST_SIZE_MAX];
  EFI_CERT_DATA                  *CertDataPtr;
  UINT8                          HashAlgId;

  //
  // 1. TopLevelCert is the top-level issuer certificate in signature Signer Cert Chain
  // 2. TrustedCert is the certificate which firmware trusts. It could be saved in protected
  //     storage or PK payload on PK init
  //
  VerifyStatus  = FALSE;
  CertData      = NULL;
  NewData       = NULL;
  Attr          = Attributes;
  SignerCerts   = NULL;
  TopLevelCert  = NULL;
  CertsInCertDb = NULL;
  CertDataPtr   = NULL;

  //
  // When the attribute EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS is
  // set, then the Data buffer shall begin with an instance of a complete (and serialized)
  // EFI_VARIABLE_AUTHENTICATION_2 descriptor. The descriptor shall be followed by the new
  // variable value and DataSize shall reflect the combined size of the descriptor and the new
  // variable value. The authentication descriptor is not part of the variable data and is not
  // returned by subsequent calls to GetVariable().
  //
  CertData = (EFI_VARIABLE_AUTHENTICATION_2 *)Data;

  //
  // Verify that Pad1, Nanosecond, TimeZone, Daylight and Pad2 components of the
  // TimeStamp value are set to zero.
  //
  if ((CertData->TimeStamp.Pad1 != 0) ||
      (CertData->TimeStamp.Nanosecond != 0) ||
      (CertData->TimeStamp.TimeZone != 0) ||
      (CertData->TimeStamp.Daylight != 0) ||
      (CertData->TimeStamp.Pad2 != 0))
  {
    return EFI_SECURITY_VIOLATION;
  }

  if ((OrgTimeStamp != NULL) && ((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0)) {
    if (AuthServiceInternalCompareTimeStamp (&CertData->TimeStamp, OrgTimeStamp)) {
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
      !CompareGuid (&CertData->AuthInfo.CertType, &gEfiCertPkcs7Guid))
  {
    //
    // Invalid AuthInfo type, return EFI_SECURITY_VIOLATION.
    //
    return EFI_SECURITY_VIOLATION;
  }

  //
  // Find out Pkcs7 SignedData which follows the EFI_VARIABLE_AUTHENTICATION_2 descriptor.
  // AuthInfo.Hdr.dwLength is the length of the entire certificate, including the length of the header.
  //
  SigData     = CertData->AuthInfo.CertData;
  SigDataSize = CertData->AuthInfo.Hdr.dwLength - (UINT32)(OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData));

  //
  // SignedData.digestAlgorithms shall contain the digest algorithm used when preparing the
  // signature. Only a digest algorithm of SHA-256, SHA-384 or SHA-512 is accepted.
  //
  //    According to PKCS#7 Definition (https://www.rfc-editor.org/rfc/rfc2315):
  //        SignedData ::= SEQUENCE {
  //            version Version,
  //            digestAlgorithms DigestAlgorithmIdentifiers,
  //            contentInfo ContentInfo,
  //            .... }
  //    The DigestAlgorithmIdentifiers can be used to determine the hash algorithm
  //    in VARIABLE_AUTHENTICATION_2 descriptor.
  //    This field has the fixed offset (+13) or (+32) based on whether the DER-encoded
  //    ContentInfo structure is present or not, and can be calculated based on two
  //    bytes of length encoding.
  //
  //    Both condition can be handled in WrapPkcs7Data() in CryptPkcs7VerifyCommon.c.
  //
  //    See below examples:
  //
  // 1. Without ContentInfo
  //    30 82 0c da // SEQUENCE (5 element) (3294 BYTES) -- SignedData
  //       02 01 01 // INTEGER 1 -- Version
  //       31 0f // SET (1 element) (15 BYTES) -- DigestAlgorithmIdentifiers
  //          30 0d // SEQUENCE (2 element) (13 BYTES) -- AlgorithmIdentifier
  //             06 09 // OBJECT-IDENTIFIER (9 BYTES) -- algorithm
  //                60 86 48 01 65 03 04 02 01 // sha256 [2.16.840.1.101.3.4.2.1]
  //             05 00 // NULL (0 BYTES) -- parameters
  //
  // Example from: https://uefi.org/revocationlistfile
  //
  // 2. With ContentInfo
  //    30 82 05 90 // SEQUENCE (1424 BYTES) -- ContentInfo
  //       06 09 // OBJECT-IDENTIFIER (9 BYTES) -- ContentType
  //          2a 86 48 86 f7 0d 01 07 02 // signedData [1.2.840.113549.1.7.2]
  //       a0 82 05 81 // CONTEXT-SPECIFIC CONSTRUCTED TAG 0 (1409 BYTES) -- content
  //          30 82 05 7d // SEQUENCE (1405 BYTES) -- SignedData
  //             02 01 01 // INTEGER 1 -- Version
  //             31 0f // SET (1 element) (15 BYTES) -- DigestAlgorithmIdentifiers
  //                30 0d // SEQUENCE (13 BYTES) -- AlgorithmIdentifier
  //                   06 09 // OBJECT-IDENTIFIER (9 BYTES) -- algorithm
  //                      60 86 48 01 65 03 04 02 01 // sha256 [2.16.840.1.101.3.4.2.1]
  //                   05 00 // NULL (0 BYTES) -- parameters
  //
  // Example generated with: https://wiki.archlinux.org/title/Unified_Extensible_Firmware_Interface/Secure_Boot#Manual_process
  //
  HashAlgId = FindHashAlgorithmIndex (SigData, SigDataSize);
  if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) {
    if (HashAlgId >= (sizeof (mHashInfo) / sizeof (EFI_HASH_INFO))) {
      return EFI_SECURITY_VIOLATION;
    }
  }

  //
  // Find out the new data payload which follows Pkcs7 SignedData directly.
  //
  PayloadPtr  = SigData + SigDataSize;
  PayloadSize = DataSize - OFFSET_OF_AUTHINFO2_CERT_DATA - (UINTN)SigDataSize;

  // If the VariablePolicy engine is disabled, allow deletion of any authenticated variables.
  if ((PayloadSize == 0) && ((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) && !IsVariablePolicyEnabled ()) {
    VerifyStatus = TRUE;
    goto Exit;
  }

  //
  // Construct a serialization buffer of the values of the VariableName, VendorGuid and Attributes
  // parameters of the SetVariable() call and the TimeStamp component of the
  // EFI_VARIABLE_AUTHENTICATION_2 descriptor followed by the variable's new value
  // i.e. (VariableName, VendorGuid, Attributes, TimeStamp, Data)
  //
  NewDataSize = PayloadSize + sizeof (EFI_TIME) + sizeof (UINT32) +
                sizeof (EFI_GUID) + StrSize (VariableName) - sizeof (CHAR16);

  //
  // Here is to reuse scratch data area(at the end of volatile variable store)
  // to reduce SMRAM consumption for SMM variable driver.
  // The scratch buffer is enough to hold the serialized data and safe to use,
  // because it is only used at here to do verification temporarily first
  // and then used in UpdateVariable() for a time based auth variable set.
  //
  Status = mAuthVarLibContextIn->GetScratchBuffer (&NewDataSize, (VOID **)&NewData);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

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
    // Verify that the signature has been made with the current Platform Key (no chaining for PK).
    // First, get signer's certificates from SignedData.
    //
    VerifyStatus = Pkcs7GetSigners (
                     SigData,
                     SigDataSize,
                     &SignerCerts,
                     &CertStackSize,
                     &TopLevelCert,
                     &TopLevelCertSize
                     );
    if (!VerifyStatus) {
      goto Exit;
    }

    //
    // Second, get the current platform key from variable. Check whether it's identical with signer's certificates
    // in SignedData. If not, return error immediately.
    //
    Status = AuthServiceInternalFindVariable (
               EFI_PLATFORM_KEY_NAME,
               &gEfiGlobalVariableGuid,
               &Data,
               &DataSize
               );
    if (EFI_ERROR (Status)) {
      VerifyStatus = FALSE;
      goto Exit;
    }

    CertList = (EFI_SIGNATURE_LIST *)Data;
    Cert     = (EFI_SIGNATURE_DATA *)((UINT8 *)CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
    if ((TopLevelCertSize != (CertList->SignatureSize - (sizeof (EFI_SIGNATURE_DATA) - 1))) ||
        (CompareMem (Cert->SignatureData, TopLevelCert, TopLevelCertSize) != 0))
    {
      VerifyStatus = FALSE;
      goto Exit;
    }

    //
    // Verify Pkcs7 SignedData via Pkcs7Verify library.
    //
    VerifyStatus = Pkcs7Verify (
                     SigData,
                     SigDataSize,
                     TopLevelCert,
                     TopLevelCertSize,
                     NewData,
                     NewDataSize
                     );
  } else if (AuthVarType == AuthVarTypeKek) {
    //
    // Get KEK database from variable.
    //
    Status = AuthServiceInternalFindVariable (
               EFI_KEY_EXCHANGE_KEY_NAME,
               &gEfiGlobalVariableGuid,
               &Data,
               &DataSize
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Ready to verify Pkcs7 SignedData. Go through KEK Signature Database to find out X.509 CertList.
    //
    KekDataSize = (UINT32)DataSize;
    CertList    = (EFI_SIGNATURE_LIST *)Data;
    while ((KekDataSize > 0) && (KekDataSize >= CertList->SignatureListSize)) {
      if (CompareGuid (&CertList->SignatureType, &gEfiCertX509Guid)) {
        Cert      = (EFI_SIGNATURE_DATA *)((UINT8 *)CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
        CertCount = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;
        for (Index = 0; Index < CertCount; Index++) {
          //
          // Iterate each Signature Data Node within this CertList for a verify
          //
          TrustedCert     = Cert->SignatureData;
          TrustedCertSize = CertList->SignatureSize - (sizeof (EFI_SIGNATURE_DATA) - 1);

          //
          // Verify Pkcs7 SignedData via Pkcs7Verify library.
          //
          VerifyStatus = Pkcs7Verify (
                           SigData,
                           SigDataSize,
                           TrustedCert,
                           TrustedCertSize,
                           NewData,
                           NewDataSize
                           );
          if (VerifyStatus) {
            goto Exit;
          }

          Cert = (EFI_SIGNATURE_DATA *)((UINT8 *)Cert + CertList->SignatureSize);
        }
      }

      KekDataSize -= CertList->SignatureListSize;
      CertList     = (EFI_SIGNATURE_LIST *)((UINT8 *)CertList + CertList->SignatureListSize);
    }
  } else if (AuthVarType == AuthVarTypePriv) {
    //
    // Process common authenticated variable except PK/KEK/DB/DBX/DBT.
    // Get signer's certificates from SignedData.
    //
    VerifyStatus = Pkcs7GetSigners (
                     SigData,
                     SigDataSize,
                     &SignerCerts,
                     &CertStackSize,
                     &TopLevelCert,
                     &TopLevelCertSize
                     );
    if (!VerifyStatus) {
      goto Exit;
    }

    //
    // Get previously stored signer's certificates from certdb or certdbv for existing
    // variable. Check whether they are identical with signer's certificates
    // in SignedData. If not, return error immediately.
    //
    if (OrgTimeStamp != NULL) {
      VerifyStatus = FALSE;

      Status = GetCertsFromDb (VariableName, VendorGuid, Attributes, &CertsInCertDb, &CertsSizeinDb);
      if (EFI_ERROR (Status)) {
        goto Exit;
      }

      if ((HashAlgId < (sizeof (mHashInfo) / sizeof (EFI_HASH_INFO))) && (CertsSizeinDb == mHashInfo[HashAlgId].HashSize)) {
        //
        // Check hash of signer cert CommonName + Top-level issuer tbsCertificate against data in CertDb
        //
        CertDataPtr = (EFI_CERT_DATA *)(SignerCerts + 1);
        Status      = CalculatePrivAuthVarSignChainSHADigest (
                        HashAlgId,
                        CertDataPtr->CertDataBuffer,
                        ReadUnaligned32 ((UINT32 *)&(CertDataPtr->CertDataLength)),
                        TopLevelCert,
                        TopLevelCertSize,
                        ShaDigest
                        );
        if (EFI_ERROR (Status) || (CompareMem (ShaDigest, CertsInCertDb, CertsSizeinDb) != 0)) {
          goto Exit;
        }
      } else {
        //
        // Keep backward compatible with previous solution which saves whole signer certs stack in CertDb
        //
        if ((CertStackSize != CertsSizeinDb) ||
            (CompareMem (SignerCerts, CertsInCertDb, CertsSizeinDb) != 0))
        {
          goto Exit;
        }
      }
    }

    VerifyStatus = Pkcs7Verify (
                     SigData,
                     SigDataSize,
                     TopLevelCert,
                     TopLevelCertSize,
                     NewData,
                     NewDataSize
                     );
    if (!VerifyStatus) {
      goto Exit;
    }

    if ((OrgTimeStamp == NULL) && (PayloadSize != 0)) {
      //
      // When adding a new common authenticated variable, always save Hash of cn of signer cert + tbsCertificate of Top-level issuer
      //
      CertDataPtr = (EFI_CERT_DATA *)(SignerCerts + 1);
      Status      = InsertCertsToDb (
                      HashAlgId,
                      VariableName,
                      VendorGuid,
                      Attributes,
                      CertDataPtr->CertDataBuffer,
                      ReadUnaligned32 ((UINT32 *)&(CertDataPtr->CertDataLength)),
                      TopLevelCert,
                      TopLevelCertSize
                      );
      if (EFI_ERROR (Status)) {
        VerifyStatus = FALSE;
        goto Exit;
      }
    }
  } else if (AuthVarType == AuthVarTypePayload) {
    CertList        = (EFI_SIGNATURE_LIST *)PayloadPtr;
    Cert            = (EFI_SIGNATURE_DATA *)((UINT8 *)CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
    TrustedCert     = Cert->SignatureData;
    TrustedCertSize = CertList->SignatureSize - (sizeof (EFI_SIGNATURE_DATA) - 1);
    //
    // Verify Pkcs7 SignedData via Pkcs7Verify library.
    //
    VerifyStatus = Pkcs7Verify (
                     SigData,
                     SigDataSize,
                     TrustedCert,
                     TrustedCertSize,
                     NewData,
                     NewDataSize
                     );
  } else {
    return EFI_SECURITY_VIOLATION;
  }

Exit:

  if ((AuthVarType == AuthVarTypePk) || (AuthVarType == AuthVarTypePriv)) {
    if (TopLevelCert != NULL) {
      Pkcs7FreeSigners (TopLevelCert);
    }

    if (SignerCerts != NULL) {
      Pkcs7FreeSigners (SignerCerts);
    }
  }

  if (!VerifyStatus) {
    return EFI_SECURITY_VIOLATION;
  }

  Status = CheckSignatureListFormat (VariableName, VendorGuid, PayloadPtr, PayloadSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *VarPayloadPtr  = PayloadPtr;
  *VarPayloadSize = PayloadSize;

  return EFI_SUCCESS;
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
  @param[in]  Attributes                  Attribute value of the variable.
  @param[in]  AuthVarType                 Verify against PK, KEK database, private database or certificate in data payload.
  @param[out] VarDel                      Delete the variable or not.

  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval EFI_SECURITY_VIOLATION          The variable does NOT pass the validation
                                          check carried out by the firmware.
  @retval EFI_OUT_OF_RESOURCES            Failed to process variable due to lack
                                          of resources.
  @retval EFI_SUCCESS                     Variable pass validation successfully.

**/
EFI_STATUS
VerifyTimeBasedPayloadAndUpdate (
  IN     CHAR16        *VariableName,
  IN     EFI_GUID      *VendorGuid,
  IN     VOID          *Data,
  IN     UINTN         DataSize,
  IN     UINT32        Attributes,
  IN     AUTHVAR_TYPE  AuthVarType,
  OUT    BOOLEAN       *VarDel
  )
{
  EFI_STATUS                     Status;
  EFI_STATUS                     FindStatus;
  UINT8                          *PayloadPtr;
  UINTN                          PayloadSize;
  EFI_VARIABLE_AUTHENTICATION_2  *CertData;
  AUTH_VARIABLE_INFO             OrgVariableInfo;
  BOOLEAN                        IsDel;

  ZeroMem (&OrgVariableInfo, sizeof (OrgVariableInfo));
  FindStatus = mAuthVarLibContextIn->FindVariable (
                                       VariableName,
                                       VendorGuid,
                                       &OrgVariableInfo
                                       );

  Status = VerifyTimeBasedPayload (
             VariableName,
             VendorGuid,
             Data,
             DataSize,
             Attributes,
             AuthVarType,
             (!EFI_ERROR (FindStatus)) ? OrgVariableInfo.TimeStamp : NULL,
             &PayloadPtr,
             &PayloadSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (  !EFI_ERROR (FindStatus)
     && (PayloadSize == 0)
     && ((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0))
  {
    IsDel = TRUE;
  } else {
    IsDel = FALSE;
  }

  CertData = (EFI_VARIABLE_AUTHENTICATION_2 *)Data;

  //
  // Final step: Update/Append Variable if it pass Pkcs7Verify
  //
  Status = AuthServiceInternalUpdateVariableWithTimeStamp (
             VariableName,
             VendorGuid,
             PayloadPtr,
             PayloadSize,
             Attributes,
             &CertData->TimeStamp
             );

  //
  // Delete signer's certificates when delete the common authenticated variable.
  //
  if (IsDel && (AuthVarType == AuthVarTypePriv) && !EFI_ERROR (Status)) {
    Status = DeleteCertsFromDb (VariableName, VendorGuid, Attributes);
  }

  if (VarDel != NULL) {
    if (IsDel && !EFI_ERROR (Status)) {
      *VarDel = TRUE;
    } else {
      *VarDel = FALSE;
    }
  }

  return Status;
}

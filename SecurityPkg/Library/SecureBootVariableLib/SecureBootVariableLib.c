/** @file
  This library provides helper functions to set/clear Secure Boot
  keys and databases.

  Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2018 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2021, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2021, Semihalf All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <UefiSecureBoot.h>
#include <Guid/GlobalVariable.h>
#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/ImageAuthentication.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SecureBootVariableLib.h>
#include <Library/PlatformPKProtectionLib.h>

//
// This is the minimum size of a EFI_SIGNATURE_LIST and data and still be valid.
//
// The UEFI specification defines this as:
//  Each signature list is a list of signatures of one type, identified by SignatureType.
//  The signature list contains a header and then an array of zero or more signatures in
//  the format specified by the header. The size of each signature in the signature list
//  is specified by SignatureSize.
//
// This is the size of a signature list with some data in it. Effectively 44 bytes are
// required to set the DBX. However in reality, the size of a real payload will be larger
// since this miniumum size does not consider the SIGNATURE_LIST
//
#define MINIMUM_VALID_SIGNATURE_LIST  (sizeof(EFI_SIGNATURE_LIST) + sizeof(EFI_SIGNATURE_DATA) - 1)

// This time can be used when deleting variables, as it should be greater than any variable time.
EFI_TIME  mMaxTimestamp = {
  0xFFFF,     // Year
  0xFF,       // Month
  0xFF,       // Day
  0xFF,       // Hour
  0xFF,       // Minute
  0xFF,       // Second
  0x00,
  0x00000000, // Nanosecond
  0,
  0,
  0x00
};

//
// This epoch time is the date that is used when creating SecureBoot default variables.
// NOTE: This is a placeholder date that doesn't correspond to anything else.
//
EFI_TIME  mDefaultPayloadTimestamp = {
  1970, // Year (1970)
  1,    // Month (Jan)
  1,    // Day (1)
  0,    // Hour
  0,    // Minute
  0,    // Second
  0,    // Pad1
  0,    // Nanosecond
  0,    // Timezone (Dummy value)
  0,    // Daylight (Dummy value)
  0     // Pad2
};

/** Creates EFI Signature List structure.

  @param[in]      Data     A pointer to signature data.
  @param[in]      Size     Size of signature data.
  @param[out]     SigList  Created Signature List.

  @retval  EFI_SUCCESS           Signature List was created successfully.
  @retval  EFI_OUT_OF_RESOURCES  Failed to allocate memory.
**/
STATIC
EFI_STATUS
CreateSigList (
  IN VOID                 *Data,
  IN UINTN                Size,
  OUT EFI_SIGNATURE_LIST  **SigList
  )
{
  UINTN               SigListSize;
  EFI_SIGNATURE_LIST  *TmpSigList;
  EFI_SIGNATURE_DATA  *SigData;

  //
  // Allocate data for Signature Database
  //
  SigListSize = sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + Size;
  TmpSigList  = (EFI_SIGNATURE_LIST *)AllocateZeroPool (SigListSize);
  if (TmpSigList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Only gEfiCertX509Guid type is supported
  //
  TmpSigList->SignatureListSize   = (UINT32)SigListSize;
  TmpSigList->SignatureSize       = (UINT32)(sizeof (EFI_SIGNATURE_DATA) - 1 + Size);
  TmpSigList->SignatureHeaderSize = 0;
  CopyGuid (&TmpSigList->SignatureType, &gEfiCertX509Guid);

  //
  // Copy key data
  //
  SigData = (EFI_SIGNATURE_DATA *)(TmpSigList + 1);
  CopyGuid (&SigData->SignatureOwner, &gEfiGlobalVariableGuid);
  CopyMem (&SigData->SignatureData[0], Data, Size);

  *SigList = TmpSigList;

  return EFI_SUCCESS;
}

/** Adds new signature list to signature database.

  @param[in]      SigLists        A pointer to signature database.
  @param[in]      SigListAppend  A signature list to be added.
  @param[out]     *SigListOut     Created signature database.
  @param[in, out] SigListsSize    A size of created signature database.

  @retval  EFI_SUCCESS           Signature List was added successfully.
  @retval  EFI_OUT_OF_RESOURCES  Failed to allocate memory.
**/
STATIC
EFI_STATUS
ConcatenateSigList (
  IN  EFI_SIGNATURE_LIST  *SigLists,
  IN  EFI_SIGNATURE_LIST  *SigListAppend,
  OUT EFI_SIGNATURE_LIST  **SigListOut,
  IN OUT UINTN            *SigListsSize
  )
{
  EFI_SIGNATURE_LIST  *TmpSigList;
  UINT8               *Offset;
  UINTN               NewSigListsSize;

  NewSigListsSize = *SigListsSize + SigListAppend->SignatureListSize;

  TmpSigList = (EFI_SIGNATURE_LIST *)AllocateZeroPool (NewSigListsSize);
  if (TmpSigList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (TmpSigList, SigLists, *SigListsSize);

  Offset  = (UINT8 *)TmpSigList;
  Offset += *SigListsSize;
  CopyMem ((VOID *)Offset, SigListAppend, SigListAppend->SignatureListSize);

  *SigListsSize = NewSigListsSize;
  *SigListOut   = TmpSigList;
  return EFI_SUCCESS;
}

/**
  Create a EFI Signature List with data supplied from input argument.
  The input certificates from KeyInfo parameter should be DER-encoded
  format.

  @param[out]       SigListsSize   A pointer to size of signature list
  @param[out]       SigListOut     A pointer to a callee-allocated buffer with signature lists
  @param[in]        KeyInfoCount   The number of certificate pointer and size pairs inside KeyInfo.
  @param[in]        KeyInfo        A pointer to all certificates, in the format of DER-encoded,
                                   to be concatenated into signature lists.

  @retval EFI_SUCCESS              Created signature list from payload successfully.
  @retval EFI_NOT_FOUND            Section with key has not been found.
  @retval EFI_INVALID_PARAMETER    Embedded key has a wrong format or input pointers are NULL.
  @retval Others                   Unexpected error happens.

**/
EFI_STATUS
EFIAPI
SecureBootCreateDataFromInput (
  OUT UINTN                               *SigListsSize,
  OUT EFI_SIGNATURE_LIST                  **SigListOut,
  IN  UINTN                               KeyInfoCount,
  IN  CONST SECURE_BOOT_CERTIFICATE_INFO  *KeyInfo
  )
{
  EFI_SIGNATURE_LIST  *EfiSig;
  EFI_SIGNATURE_LIST  *TmpEfiSig;
  EFI_SIGNATURE_LIST  *TmpEfiSig2;
  EFI_STATUS          Status;
  VOID                *Buffer;
  UINTN               Size;
  UINTN               InputIndex;
  UINTN               KeyIndex;

  if ((SigListOut == NULL) || (SigListsSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((KeyInfoCount == 0) || (KeyInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  InputIndex    = 0;
  KeyIndex      = 0;
  EfiSig        = NULL;
  *SigListsSize = 0;
  while (InputIndex < KeyInfoCount) {
    if (KeyInfo[InputIndex].Data != NULL) {
      Size   = KeyInfo[InputIndex].DataSize;
      Buffer = AllocateCopyPool (Size, KeyInfo[InputIndex].Data);
      if (Buffer == NULL) {
        if (EfiSig != NULL) {
          FreePool (EfiSig);
        }

        return EFI_OUT_OF_RESOURCES;
      }

      Status = CreateSigList (Buffer, Size, &TmpEfiSig);

      if (EFI_ERROR (Status)) {
        FreePool (Buffer);
        break;
      }

      //
      // Concatenate lists if more than one section found
      //
      if (KeyIndex == 0) {
        EfiSig        = TmpEfiSig;
        *SigListsSize = TmpEfiSig->SignatureListSize;
      } else {
        ConcatenateSigList (EfiSig, TmpEfiSig, &TmpEfiSig2, SigListsSize);
        FreePool (EfiSig);
        FreePool (TmpEfiSig);
        EfiSig = TmpEfiSig2;
      }

      KeyIndex++;
      FreePool (Buffer);
    }

    InputIndex++;
  }

  if (KeyIndex == 0) {
    return EFI_NOT_FOUND;
  }

  *SigListOut = EfiSig;

  return EFI_SUCCESS;
}

/**
  Create a time based data payload by concatenating the EFI_VARIABLE_AUTHENTICATION_2
  descriptor with the input data. NO authentication is required in this function.

  @param[in, out]   DataSize       On input, the size of Data buffer in bytes.
                                   On output, the size of data returned in Data
                                   buffer in bytes.
  @param[in, out]   Data           On input, Pointer to data buffer to be wrapped or
                                   pointer to NULL to wrap an empty payload.
                                   On output, Pointer to the new payload date buffer allocated from pool,
                                   it's caller's responsibility to free the memory when finish using it.
  @param[in]        Time           Pointer to time information to created time based payload.

  @retval EFI_SUCCESS              Create time based payload successfully.
  @retval EFI_OUT_OF_RESOURCES     There are not enough memory resources to create time based payload.
  @retval EFI_INVALID_PARAMETER    The parameter is invalid.
  @retval Others                   Unexpected error happens.

--*/
EFI_STATUS
EFIAPI
CreateTimeBasedPayload (
  IN OUT UINTN     *DataSize,
  IN OUT UINT8     **Data,
  IN     EFI_TIME  *Time
  )
{
  UINT8                          *NewData;
  UINT8                          *Payload;
  UINTN                          PayloadSize;
  EFI_VARIABLE_AUTHENTICATION_2  *DescriptorData;
  UINTN                          DescriptorSize;

  if ((Data == NULL) || (DataSize == NULL) || (Time == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a(), invalid arg\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  //
  // In Setup mode or Custom mode, the variable does not need to be signed but the
  // parameters to the SetVariable() call still need to be prepared as authenticated
  // variable. So we create EFI_VARIABLE_AUTHENTICATED_2 descriptor without certificate
  // data in it.
  //
  Payload     = *Data;
  PayloadSize = *DataSize;

  DescriptorSize = OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo) + OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  NewData        = (UINT8 *)AllocateZeroPool (DescriptorSize + PayloadSize);
  if (NewData == NULL) {
    DEBUG ((DEBUG_ERROR, "%a() Out of resources.\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  if ((Payload != NULL) && (PayloadSize != 0)) {
    CopyMem (NewData + DescriptorSize, Payload, PayloadSize);
  }

  DescriptorData = (EFI_VARIABLE_AUTHENTICATION_2 *)(NewData);

  CopyMem (&DescriptorData->TimeStamp, Time, sizeof (EFI_TIME));

  DescriptorData->AuthInfo.Hdr.dwLength         = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  DescriptorData->AuthInfo.Hdr.wRevision        = 0x0200;
  DescriptorData->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyGuid (&DescriptorData->AuthInfo.CertType, &gEfiCertPkcs7Guid);

  if (Payload != NULL) {
    FreePool (Payload);
    Payload = NULL;
  }

  *DataSize = DescriptorSize + PayloadSize;
  *Data     = NewData;
  return EFI_SUCCESS;
}

/**
  Internal helper function to delete a Variable given its name and GUID, NO authentication
  required.

  @param[in]      VariableName            Name of the Variable.
  @param[in]      VendorGuid              GUID of the Variable.

  @retval EFI_SUCCESS              Variable deleted successfully.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
DeleteVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS  Status;
  VOID        *Variable;
  UINT8       *Data;
  UINTN       DataSize;
  UINT32      Attr;

  GetVariable2 (VariableName, VendorGuid, &Variable, NULL);
  if (Variable == NULL) {
    return EFI_SUCCESS;
  }

  FreePool (Variable);

  Data     = NULL;
  DataSize = 0;
  Attr     = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS
             | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;

  Status = CreateTimeBasedPayload (&DataSize, &Data, &mMaxTimestamp);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fail to create time-based data payload: %r\n", Status));
    return Status;
  }

  Status = gRT->SetVariable (
                  VariableName,
                  VendorGuid,
                  Attr,
                  DataSize,
                  Data
                  );
  if (Data != NULL) {
    FreePool (Data);
  }

  return Status;
}

/**

  Set the platform secure boot mode into "Custom" or "Standard" mode.

  @param[in]   SecureBootMode        New secure boot mode: STANDARD_SECURE_BOOT_MODE or
                                     CUSTOM_SECURE_BOOT_MODE.

  @return EFI_SUCCESS                The platform has switched to the special mode successfully.
  @return other                      Fail to operate the secure boot mode.

**/
EFI_STATUS
EFIAPI
SetSecureBootMode (
  IN  UINT8  SecureBootMode
  )
{
  return gRT->SetVariable (
                EFI_CUSTOM_MODE_NAME,
                &gEfiCustomModeEnableGuid,
                EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                sizeof (UINT8),
                &SecureBootMode
                );
}

/**
  Fetches the value of SetupMode variable.

  @param[out] SetupMode             Pointer to UINT8 for SetupMode output

  @retval other                     Retval from GetVariable.
**/
EFI_STATUS
EFIAPI
GetSetupMode (
  OUT UINT8  *SetupMode
  )
{
  UINTN       Size;
  EFI_STATUS  Status;

  Size   = sizeof (*SetupMode);
  Status = gRT->GetVariable (
                  EFI_SETUP_MODE_NAME,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &Size,
                  SetupMode
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Helper function to quickly determine whether SecureBoot is enabled.

  @retval     TRUE    SecureBoot is verifiably enabled.
  @retval     FALSE   SecureBoot is either disabled or an error prevented checking.

**/
BOOLEAN
EFIAPI
IsSecureBootEnabled (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT8       *SecureBoot;

  SecureBoot = NULL;

  Status = GetEfiGlobalVariable2 (EFI_SECURE_BOOT_MODE_NAME, (VOID **)&SecureBoot, NULL);
  //
  // Skip verification if SecureBoot variable doesn't exist.
  //
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot check SecureBoot variable - %r\n", Status));
    return FALSE;
  }

  //
  // Skip verification if SecureBoot is disabled but not AuditMode
  //
  if (*SecureBoot == SECURE_BOOT_MODE_DISABLE) {
    FreePool (SecureBoot);
    return FALSE;
  } else {
    return TRUE;
  }
}

/**
  Clears the content of the 'db' variable.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2 (), GetTime () and SetVariable ()
**/
EFI_STATUS
EFIAPI
DeleteDb (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = DeleteVariable (
             EFI_IMAGE_SECURITY_DATABASE,
             &gEfiImageSecurityDatabaseGuid
             );

  return Status;
}

/**
  Clears the content of the 'dbx' variable.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2 (), GetTime () and SetVariable ()
**/
EFI_STATUS
EFIAPI
DeleteDbx (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = DeleteVariable (
             EFI_IMAGE_SECURITY_DATABASE1,
             &gEfiImageSecurityDatabaseGuid
             );

  return Status;
}

/**
  Clears the content of the 'dbt' variable.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2 (), GetTime () and SetVariable ()
**/
EFI_STATUS
EFIAPI
DeleteDbt (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = DeleteVariable (
             EFI_IMAGE_SECURITY_DATABASE2,
             &gEfiImageSecurityDatabaseGuid
             );

  return Status;
}

/**
  Clears the content of the 'KEK' variable.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2 (), GetTime () and SetVariable ()
**/
EFI_STATUS
EFIAPI
DeleteKEK (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = DeleteVariable (
             EFI_KEY_EXCHANGE_KEY_NAME,
             &gEfiGlobalVariableGuid
             );

  return Status;
}

/**
  Remove the PK variable.

  @retval EFI_SUCCESS    Delete PK successfully.
  @retval Others         Could not allow to delete PK.

**/
EFI_STATUS
EFIAPI
DeletePlatformKey (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = SetSecureBootMode (CUSTOM_SECURE_BOOT_MODE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DeleteVariable (
             EFI_PLATFORM_KEY_NAME,
             &gEfiGlobalVariableGuid
             );
  return Status;
}

/**
  This function will delete the secure boot keys, thus
  disabling secure boot.

  @return EFI_SUCCESS or underlying failure code.
**/
EFI_STATUS
EFIAPI
DeleteSecureBootVariables (
  VOID
  )
{
  EFI_STATUS  Status, TempStatus;

  DEBUG ((DEBUG_INFO, "%a - Attempting to delete the Secure Boot variables.\n", __func__));

  //
  // Step 1: Notify that a PK update is coming shortly...
  Status = DisablePKProtection ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to signal PK update start! %r\n", __func__, Status));
    // Classify this as a PK deletion error.
    Status = EFI_ABORTED;
  }

  //
  // Step 2: Attempt to delete the PK.
  // Let's try to nuke the PK, why not...
  if (!EFI_ERROR (Status)) {
    Status = DeletePlatformKey ();
    DEBUG ((DEBUG_INFO, "%a - PK Delete = %r\n", __func__, Status));
    // If the PK is not found, then our work here is done.
    if (Status == EFI_NOT_FOUND) {
      Status = EFI_SUCCESS;
    }
    // If any other error occurred, let's inform the caller that the PK delete in particular failed.
    else if (EFI_ERROR (Status)) {
      Status = EFI_ABORTED;
    }
  }

  //
  // Step 3: Attempt to delete remaining keys/databases...
  // Now that the PK is deleted (assuming Status == EFI_SUCCESS) the system is in SETUP_MODE.
  // Arguably we could leave these variables in place and let them be deleted by whoever wants to
  // update all the SecureBoot variables. However, for cleanliness sake, let's try to
  // get rid of them here.
  if (!EFI_ERROR (Status)) {
    //
    // If any of THESE steps have an error, report the error but attempt to delete all keys.
    // Using TempStatus will prevent an error from being trampled by an EFI_SUCCESS.
    // Overwrite Status ONLY if TempStatus is an error.
    //
    // If the error is EFI_NOT_FOUND, we can safely ignore it since we were trying to delete
    // the variables anyway.
    //
    TempStatus = DeleteKEK ();
    DEBUG ((DEBUG_INFO, "%a - KEK Delete = %r\n", __func__, TempStatus));
    if (EFI_ERROR (TempStatus) && (TempStatus != EFI_NOT_FOUND)) {
      Status = EFI_ACCESS_DENIED;
    }

    TempStatus = DeleteDb ();
    DEBUG ((DEBUG_INFO, "%a - db Delete = %r\n", __func__, TempStatus));
    if (EFI_ERROR (TempStatus) && (TempStatus != EFI_NOT_FOUND)) {
      Status = EFI_ACCESS_DENIED;
    }

    TempStatus = DeleteDbx ();
    DEBUG ((DEBUG_INFO, "%a - dbx Delete = %r\n", __func__, TempStatus));
    if (EFI_ERROR (TempStatus) && (TempStatus != EFI_NOT_FOUND)) {
      Status = EFI_ACCESS_DENIED;
    }

    TempStatus = DeleteDbt ();
    DEBUG ((DEBUG_INFO, "%a - dbt Delete = %r\n", __func__, TempStatus));
    if (EFI_ERROR (TempStatus) && (TempStatus != EFI_NOT_FOUND)) {
      Status = EFI_ACCESS_DENIED;
    }
  }

  return Status;
}// DeleteSecureBootVariables()

/**
  A helper function to take in a variable payload, wrap it in the
  proper authenticated variable structure, and install it in the
  EFI variable space.

  @param[in]  VariableName  The name of the key/database.
  @param[in]  VendorGuid    The namespace (ie. vendor GUID) of the variable
  @param[in]  DataSize      Size parameter for target secure boot variable.
  @param[in]  Data          Pointer to signature list formatted secure boot variable content.

  @retval EFI_SUCCESS              The enrollment for authenticated variable was successful.
  @retval EFI_OUT_OF_RESOURCES     There are not enough memory resources to create time based payload.
  @retval EFI_INVALID_PARAMETER    The parameter is invalid.
  @retval Others                   Unexpected error happens.
**/
EFI_STATUS
EFIAPI
EnrollFromInput (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  VOID        *Payload;
  UINTN       PayloadSize;
  EFI_STATUS  Status;

  Payload = NULL;

  if ((VariableName == NULL) || (VendorGuid == 0)) {
    DEBUG ((DEBUG_ERROR, "Input vendor variable invalid: %p and %p\n", VariableName, VendorGuid));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if ((Data == NULL) || (DataSize == 0)) {
    // You might as well just use DeleteVariable...
    DEBUG ((DEBUG_ERROR, "Input argument invalid: %p: %x\n", Data, DataSize));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  // Bring in the noise...
  PayloadSize = DataSize;
  Payload     = AllocateZeroPool (DataSize);
  // Bring in the funk...
  if (Payload == NULL) {
    return EFI_OUT_OF_RESOURCES;
  } else {
    CopyMem (Payload, Data, DataSize);
  }

  Status = CreateTimeBasedPayload (&PayloadSize, (UINT8 **)&Payload, &mDefaultPayloadTimestamp);
  if (EFI_ERROR (Status) || (Payload == NULL)) {
    DEBUG ((DEBUG_ERROR, "Fail to create time-based data payload: %r\n", Status));
    Payload = NULL;
    Status  = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Allocate memory for auth variable
  //
  Status = gRT->SetVariable (
                  VariableName,
                  VendorGuid,
                  (EFI_VARIABLE_NON_VOLATILE |
                   EFI_VARIABLE_BOOTSERVICE_ACCESS |
                   EFI_VARIABLE_RUNTIME_ACCESS |
                   EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS),
                  PayloadSize,
                  Payload
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "error: %a (\"%s\", %g): %r\n",
      __func__,
      VariableName,
      VendorGuid,
      Status
      ));
  }

Exit:
  //
  // Always Put Away Your Toys
  // Payload will be reassigned by CreateTimeBasedPayload()...
  if (Payload != NULL) {
    FreePool (Payload);
    Payload = NULL;
  }

  return Status;
}

/**
  Similar to DeleteSecureBootVariables, this function is used to unilaterally
  force the state of related SB variables (db, dbx, dbt, KEK, PK, etc.) to be
  the built-in, hardcoded default vars.

  @param[in]  SecureBootPayload  Payload information for secure boot related keys.

  @retval     EFI_SUCCESS               SecureBoot keys are now set to defaults.
  @retval     EFI_ABORTED               SecureBoot keys are not empty. Please delete keys first
                                        or follow standard methods of altering keys (ie. use the signing system).
  @retval     EFI_SECURITY_VIOLATION    Failed to create the PK.
  @retval     Others                    Something failed in one of the subfunctions.

**/
EFI_STATUS
EFIAPI
SetSecureBootVariablesToDefault (
  IN  CONST SECURE_BOOT_PAYLOAD_INFO  *SecureBootPayload
  )
{
  EFI_STATUS  Status;
  UINT8       *Data;
  UINTN       DataSize;

  DEBUG ((DEBUG_INFO, "%a() Entry\n", __func__));

  if (SecureBootPayload == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Invalid SecureBoot payload is supplied!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Right off the bat, if SecureBoot is currently enabled, bail.
  if (IsSecureBootEnabled ()) {
    DEBUG ((DEBUG_ERROR, "%a - Cannot set default keys while SecureBoot is enabled!\n", __func__));
    return EFI_ABORTED;
  }

  DEBUG ((DEBUG_INFO, "%a - Setting up key %s!\n", __func__, SecureBootPayload->SecureBootKeyName));

  //
  // Start running down the list, creating variables in our wake.
  // dbx is a good place to start.
  Data     = (UINT8 *)SecureBootPayload->DbxPtr;
  DataSize = SecureBootPayload->DbxSize;
  if (DataSize > MINIMUM_VALID_SIGNATURE_LIST) {
    //
    // Ensure that that the DataSize meets a minimum allowed size before being set.
    //
    DEBUG ((DEBUG_INFO, "%a - Setting Dbx\n", __func__));
    Status = EnrollFromInput (
               EFI_IMAGE_SECURITY_DATABASE1,
               &gEfiImageSecurityDatabaseGuid,
               DataSize,
               Data
               );
  } else if ((DataSize == 0) || (DataSize == sizeof (EFI_SIGNATURE_LIST))) {
    //
    // The DBX is allowed to be empty by default
    //
    DEBUG ((DEBUG_INFO, "%a - Skipping Dbx - DataSize(%u)\n", __func__, DataSize));
    Status = EFI_SUCCESS;
  } else {
    DEBUG ((DEBUG_ERROR, "%a - Invalid Dbx size %u\n", __func__, DataSize));
    Status = EFI_INVALID_PARAMETER;
  }

  // If that went well, try the db (make sure to pick the right one!).
  if (!EFI_ERROR (Status)) {
    Data     = (UINT8 *)SecureBootPayload->DbPtr;
    DataSize = SecureBootPayload->DbSize;
    Status   = EnrollFromInput (
                 EFI_IMAGE_SECURITY_DATABASE,
                 &gEfiImageSecurityDatabaseGuid,
                 DataSize,
                 Data
                 );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Failed to enroll DB - %r!\n", __func__, Status));
    }
  } else {
    DEBUG ((DEBUG_ERROR, "%a - Failed to enroll DBX - %r!\n", __func__, Status));
  }

  // Keep it going. Keep it going. dbt if supplied...
  if (!EFI_ERROR (Status) && (SecureBootPayload->DbtPtr != NULL)) {
    Data     = (UINT8 *)SecureBootPayload->DbtPtr;
    DataSize = SecureBootPayload->DbtSize;
    Status   = EnrollFromInput (
                 EFI_IMAGE_SECURITY_DATABASE2,
                 &gEfiImageSecurityDatabaseGuid,
                 DataSize,
                 Data
                 );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Failed to enroll DBT - %r!\n", __func__, Status));
    }
  }

  // Keep it going. Keep it going. KEK...
  if (!EFI_ERROR (Status)) {
    Data     = (UINT8 *)SecureBootPayload->KekPtr;
    DataSize = SecureBootPayload->KekSize;
    Status   = EnrollFromInput (
                 EFI_KEY_EXCHANGE_KEY_NAME,
                 &gEfiGlobalVariableGuid,
                 DataSize,
                 Data
                 );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Failed to enroll KEK - %r!\n", __func__, Status));
    }
  }

  //
  // Finally! The Big Daddy of them all.
  // The PK!
  //
  if (!EFI_ERROR (Status)) {
    //
    // Finally, install the key.
    Data     = (UINT8 *)SecureBootPayload->PkPtr;
    DataSize = SecureBootPayload->PkSize;
    Status   = EnrollFromInput (
                 EFI_PLATFORM_KEY_NAME,
                 &gEfiGlobalVariableGuid,
                 DataSize,
                 Data
                 );

    //
    // Report PK creation errors.
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Failed to update the PK! - %r\n", __func__, Status));
      Status = EFI_SECURITY_VIOLATION;
    }
  }

  return Status;
}

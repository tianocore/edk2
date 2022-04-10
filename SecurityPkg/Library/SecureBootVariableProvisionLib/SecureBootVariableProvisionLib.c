/** @file
  This library provides functions to set/clear Secure Boot
  keys and databases.

  Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2018 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2021, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2021, Semihalf All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <UefiSecureBoot.h>
#include <Guid/GlobalVariable.h>
#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/ImageAuthentication.h>
#include <Library/BaseLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SecureBootVariableLib.h>
#include <Library/SecureBootVariableProvisionLib.h>
#include <Library/DxeServicesLib.h>

/**
  Create a EFI Signature List with data fetched from section specified as a argument.
  Found keys are verified using RsaGetPublicKeyFromX509().

  @param[in]        KeyFileGuid    A pointer to to the FFS filename GUID
  @param[out]       SigListsSize   A pointer to size of signature list
  @param[out]       SigListOut    a pointer to a callee-allocated buffer with signature lists

  @retval EFI_SUCCESS              Create time based payload successfully.
  @retval EFI_NOT_FOUND            Section with key has not been found.
  @retval EFI_INVALID_PARAMETER    Embedded key has a wrong format.
  @retval Others                   Unexpected error happens.

**/
STATIC
EFI_STATUS
SecureBootFetchData (
  IN  EFI_GUID            *KeyFileGuid,
  OUT UINTN               *SigListsSize,
  OUT EFI_SIGNATURE_LIST  **SigListOut
  )
{
  EFI_SIGNATURE_LIST            *EfiSig;
  EFI_STATUS                    Status;
  VOID                          *Buffer;
  VOID                          *RsaPubKey;
  UINTN                         Size;
  UINTN                         KeyIndex;
  UINTN                         Index;
  SECURE_BOOT_CERTIFICATE_INFO  *CertInfo;
  SECURE_BOOT_CERTIFICATE_INFO  *NewCertInfo;

  KeyIndex      = 0;
  EfiSig        = NULL;
  *SigListOut   = NULL;
  *SigListsSize = 0;
  CertInfo      = AllocatePool (sizeof (SECURE_BOOT_CERTIFICATE_INFO));
  NewCertInfo   = CertInfo;
  while (1) {
    if (NewCertInfo == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    } else {
      CertInfo = NewCertInfo;
    }

    Status = GetSectionFromAnyFv (
               KeyFileGuid,
               EFI_SECTION_RAW,
               KeyIndex,
               &Buffer,
               &Size
               );

    if (Status == EFI_SUCCESS) {
      RsaPubKey = NULL;
      if (RsaGetPublicKeyFromX509 (Buffer, Size, &RsaPubKey) == FALSE) {
        DEBUG ((DEBUG_ERROR, "%a: Invalid key format: %d\n", __FUNCTION__, KeyIndex));
        if (EfiSig != NULL) {
          FreePool (EfiSig);
        }

        FreePool (Buffer);
        Status = EFI_INVALID_PARAMETER;
        break;
      }

      CertInfo[KeyIndex].Data     = Buffer;
      CertInfo[KeyIndex].DataSize = Size;
      KeyIndex++;
      NewCertInfo = ReallocatePool (
                      sizeof (SECURE_BOOT_CERTIFICATE_INFO) * KeyIndex,
                      sizeof (SECURE_BOOT_CERTIFICATE_INFO) * (KeyIndex + 1),
                      CertInfo
                      );
    }

    if (Status == EFI_NOT_FOUND) {
      Status = EFI_SUCCESS;
      break;
    }
  }

  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  if (KeyIndex == 0) {
    Status = EFI_NOT_FOUND;
    goto Cleanup;
  }

  // Now that we collected all certs from FV, convert it into sig list
  Status = SecureBootCreateDataFromInput (SigListsSize, SigListOut, KeyIndex, CertInfo);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

Cleanup:
  if (CertInfo) {
    for (Index = 0; Index < KeyIndex; Index++) {
      FreePool ((VOID *)CertInfo[Index].Data);
    }

    FreePool (CertInfo);
  }

  return Status;
}

/**
  Enroll a key/certificate based on a default variable.

  @param[in] VariableName        The name of the key/database.
  @param[in] DefaultName         The name of the default variable.
  @param[in] VendorGuid          The namespace (ie. vendor GUID) of the variable

  @retval EFI_OUT_OF_RESOURCES   Out of memory while allocating AuthHeader.
  @retval EFI_SUCCESS            Successful enrollment.
  @return                        Error codes from GetTime () and SetVariable ().
**/
STATIC
EFI_STATUS
EnrollFromDefault (
  IN CHAR16    *VariableName,
  IN CHAR16    *DefaultName,
  IN EFI_GUID  *VendorGuid
  )
{
  VOID        *Data;
  UINTN       DataSize;
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  DataSize = 0;
  Status   = GetVariable2 (DefaultName, &gEfiGlobalVariableGuid, &Data, &DataSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "error: GetVariable (\"%s): %r\n", DefaultName, Status));
    return Status;
  }

  Status = EnrollFromInput (VariableName, VendorGuid, DataSize, Data);

  if (Data != NULL) {
    FreePool (Data);
  }

  return Status;
}

/** Initializes PKDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
**/
EFI_STATUS
SecureBootInitPKDefault (
  IN VOID
  )
{
  EFI_SIGNATURE_LIST  *EfiSig;
  UINTN               SigListsSize;
  EFI_STATUS          Status;
  UINT8               *Data;
  UINTN               DataSize;

  //
  // Check if variable exists, if so do not change it
  //
  Status = GetVariable2 (EFI_PK_DEFAULT_VARIABLE_NAME, &gEfiGlobalVariableGuid, (VOID **)&Data, &DataSize);
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Variable %s exists. Old value is preserved\n", EFI_PK_DEFAULT_VARIABLE_NAME));
    FreePool (Data);
    return EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    return Status;
  }

  //
  // Variable does not exist, can be initialized
  //
  DEBUG ((DEBUG_INFO, "Variable %s does not exist.\n", EFI_PK_DEFAULT_VARIABLE_NAME));

  Status = SecureBootFetchData (&gDefaultPKFileGuid, &SigListsSize, &EfiSig);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Content for %s not found\n", EFI_PK_DEFAULT_VARIABLE_NAME));
    return Status;
  }

  Status = gRT->SetVariable (
                  EFI_PK_DEFAULT_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  SigListsSize,
                  (VOID *)EfiSig
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Failed to set %s\n", EFI_PK_DEFAULT_VARIABLE_NAME));
  }

  FreePool (EfiSig);

  return Status;
}

/** Initializes KEKDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
**/
EFI_STATUS
SecureBootInitKEKDefault (
  IN VOID
  )
{
  EFI_SIGNATURE_LIST  *EfiSig;
  UINTN               SigListsSize;
  EFI_STATUS          Status;
  UINT8               *Data;
  UINTN               DataSize;

  //
  // Check if variable exists, if so do not change it
  //
  Status = GetVariable2 (EFI_KEK_DEFAULT_VARIABLE_NAME, &gEfiGlobalVariableGuid, (VOID **)&Data, &DataSize);
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Variable %s exists. Old value is preserved\n", EFI_KEK_DEFAULT_VARIABLE_NAME));
    FreePool (Data);
    return EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    return Status;
  }

  //
  // Variable does not exist, can be initialized
  //
  DEBUG ((DEBUG_INFO, "Variable %s does not exist.\n", EFI_KEK_DEFAULT_VARIABLE_NAME));

  Status = SecureBootFetchData (&gDefaultKEKFileGuid, &SigListsSize, &EfiSig);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Content for %s not found\n", EFI_KEK_DEFAULT_VARIABLE_NAME));
    return Status;
  }

  Status = gRT->SetVariable (
                  EFI_KEK_DEFAULT_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  SigListsSize,
                  (VOID *)EfiSig
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Failed to set %s\n", EFI_KEK_DEFAULT_VARIABLE_NAME));
  }

  FreePool (EfiSig);

  return Status;
}

/** Initializes dbDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
**/
EFI_STATUS
SecureBootInitDbDefault (
  IN VOID
  )
{
  EFI_SIGNATURE_LIST  *EfiSig;
  UINTN               SigListsSize;
  EFI_STATUS          Status;
  UINT8               *Data;
  UINTN               DataSize;

  Status = GetVariable2 (EFI_DB_DEFAULT_VARIABLE_NAME, &gEfiGlobalVariableGuid, (VOID **)&Data, &DataSize);
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Variable %s exists. Old value is preserved\n", EFI_DB_DEFAULT_VARIABLE_NAME));
    FreePool (Data);
    return EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Variable %s does not exist.\n", EFI_DB_DEFAULT_VARIABLE_NAME));

  Status = SecureBootFetchData (&gDefaultdbFileGuid, &SigListsSize, &EfiSig);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
                  EFI_DB_DEFAULT_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  SigListsSize,
                  (VOID *)EfiSig
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Failed to set %s\n", EFI_DB_DEFAULT_VARIABLE_NAME));
  }

  FreePool (EfiSig);

  return Status;
}

/** Initializes dbxDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
**/
EFI_STATUS
SecureBootInitDbxDefault (
  IN VOID
  )
{
  EFI_SIGNATURE_LIST  *EfiSig;
  UINTN               SigListsSize;
  EFI_STATUS          Status;
  UINT8               *Data;
  UINTN               DataSize;

  //
  // Check if variable exists, if so do not change it
  //
  Status = GetVariable2 (EFI_DBX_DEFAULT_VARIABLE_NAME, &gEfiGlobalVariableGuid, (VOID **)&Data, &DataSize);
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Variable %s exists. Old value is preserved\n", EFI_DBX_DEFAULT_VARIABLE_NAME));
    FreePool (Data);
    return EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    return Status;
  }

  //
  // Variable does not exist, can be initialized
  //
  DEBUG ((DEBUG_INFO, "Variable %s does not exist.\n", EFI_DBX_DEFAULT_VARIABLE_NAME));

  Status = SecureBootFetchData (&gDefaultdbxFileGuid, &SigListsSize, &EfiSig);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Content for %s not found\n", EFI_DBX_DEFAULT_VARIABLE_NAME));
    return Status;
  }

  Status = gRT->SetVariable (
                  EFI_DBX_DEFAULT_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  SigListsSize,
                  (VOID *)EfiSig
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Failed to set %s\n", EFI_DBX_DEFAULT_VARIABLE_NAME));
  }

  FreePool (EfiSig);

  return Status;
}

/** Initializes dbtDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
**/
EFI_STATUS
SecureBootInitDbtDefault (
  IN VOID
  )
{
  EFI_SIGNATURE_LIST  *EfiSig;
  UINTN               SigListsSize;
  EFI_STATUS          Status;
  UINT8               *Data;
  UINTN               DataSize;

  //
  // Check if variable exists, if so do not change it
  //
  Status = GetVariable2 (EFI_DBT_DEFAULT_VARIABLE_NAME, &gEfiGlobalVariableGuid, (VOID **)&Data, &DataSize);
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Variable %s exists. Old value is preserved\n", EFI_DBT_DEFAULT_VARIABLE_NAME));
    FreePool (Data);
    return EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    return Status;
  }

  //
  // Variable does not exist, can be initialized
  //
  DEBUG ((DEBUG_INFO, "Variable %s does not exist.\n", EFI_DBT_DEFAULT_VARIABLE_NAME));

  Status = SecureBootFetchData (&gDefaultdbtFileGuid, &SigListsSize, &EfiSig);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
                  EFI_DBT_DEFAULT_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  SigListsSize,
                  (VOID *)EfiSig
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Failed to set %s\n", EFI_DBT_DEFAULT_VARIABLE_NAME));
  }

  FreePool (EfiSig);

  return EFI_SUCCESS;
}

/**
  Sets the content of the 'db' variable based on 'dbDefault' variable content.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2 (), GetTime () and SetVariable ()
**/
EFI_STATUS
EFIAPI
EnrollDbFromDefault (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = EnrollFromDefault (
             EFI_IMAGE_SECURITY_DATABASE,
             EFI_DB_DEFAULT_VARIABLE_NAME,
             &gEfiImageSecurityDatabaseGuid
             );

  return Status;
}

/**
  Sets the content of the 'dbx' variable based on 'dbxDefault' variable content.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2 (), GetTime () and SetVariable ()
**/
EFI_STATUS
EFIAPI
EnrollDbxFromDefault (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = EnrollFromDefault (
             EFI_IMAGE_SECURITY_DATABASE1,
             EFI_DBX_DEFAULT_VARIABLE_NAME,
             &gEfiImageSecurityDatabaseGuid
             );

  return Status;
}

/**
  Sets the content of the 'dbt' variable based on 'dbtDefault' variable content.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2 (), GetTime () and SetVariable ()
**/
EFI_STATUS
EFIAPI
EnrollDbtFromDefault (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = EnrollFromDefault (
             EFI_IMAGE_SECURITY_DATABASE2,
             EFI_DBT_DEFAULT_VARIABLE_NAME,
             &gEfiImageSecurityDatabaseGuid
             );

  return Status;
}

/**
  Sets the content of the 'KEK' variable based on 'KEKDefault' variable content.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2 (), GetTime () and SetVariable ()
**/
EFI_STATUS
EFIAPI
EnrollKEKFromDefault (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = EnrollFromDefault (
             EFI_KEY_EXCHANGE_KEY_NAME,
             EFI_KEK_DEFAULT_VARIABLE_NAME,
             &gEfiGlobalVariableGuid
             );

  return Status;
}

/**
  Sets the content of the 'KEK' variable based on 'KEKDefault' variable content.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2 (), GetTime () and SetVariable ()
**/
EFI_STATUS
EFIAPI
EnrollPKFromDefault (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = EnrollFromDefault (
             EFI_PLATFORM_KEY_NAME,
             EFI_PK_DEFAULT_VARIABLE_NAME,
             &gEfiGlobalVariableGuid
             );

  return Status;
}

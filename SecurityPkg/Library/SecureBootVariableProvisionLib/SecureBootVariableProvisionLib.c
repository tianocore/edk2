/** @file
  This library provides functions to set/clear Secure Boot
  keys and databases.

  Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2018 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2021, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2021, Semihalf All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
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
#include <Library/SecureBootVariableProvisionLib.h>

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
  IN CHAR16   *VariableName,
  IN CHAR16   *DefaultName,
  IN EFI_GUID *VendorGuid
  )
{
  VOID       *Data;
  UINTN       DataSize;
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  DataSize = 0;
  Status = GetVariable2 (DefaultName, &gEfiGlobalVariableGuid, &Data, &DataSize);
  if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "error: GetVariable (\"%s): %r\n", DefaultName, Status));
      return Status;
  }

  CreateTimeBasedPayload (&DataSize, (UINT8 **)&Data);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fail to create time-based data payload: %r", Status));
    return Status;
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
                  DataSize,
                  Data
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "error: %a (\"%s\", %g): %r\n", __FUNCTION__, VariableName,
      VendorGuid, Status));
  }

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
  EFI_SIGNATURE_LIST *EfiSig;
  UINTN               SigListsSize;
  EFI_STATUS          Status;
  UINT8               *Data;
  UINTN               DataSize;

  //
  // Check if variable exists, if so do not change it
  //
  Status = GetVariable2 (EFI_PK_DEFAULT_VARIABLE_NAME, &gEfiGlobalVariableGuid, (VOID **) &Data, &DataSize);
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
  EFI_SIGNATURE_LIST *EfiSig;
  UINTN               SigListsSize;
  EFI_STATUS          Status;
  UINT8              *Data;
  UINTN               DataSize;

  //
  // Check if variable exists, if so do not change it
  //
  Status = GetVariable2 (EFI_KEK_DEFAULT_VARIABLE_NAME, &gEfiGlobalVariableGuid, (VOID **) &Data, &DataSize);
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
  EFI_SIGNATURE_LIST *EfiSig;
  UINTN               SigListsSize;
  EFI_STATUS          Status;
  UINT8              *Data;
  UINTN               DataSize;

  Status = GetVariable2 (EFI_DB_DEFAULT_VARIABLE_NAME, &gEfiGlobalVariableGuid, (VOID **) &Data, &DataSize);
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
  EFI_SIGNATURE_LIST *EfiSig;
  UINTN               SigListsSize;
  EFI_STATUS          Status;
  UINT8              *Data;
  UINTN               DataSize;

  //
  // Check if variable exists, if so do not change it
  //
  Status = GetVariable2 (EFI_DBX_DEFAULT_VARIABLE_NAME, &gEfiGlobalVariableGuid, (VOID **) &Data, &DataSize);
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
  EFI_SIGNATURE_LIST *EfiSig;
  UINTN               SigListsSize;
  EFI_STATUS          Status;
  UINT8              *Data;
  UINTN               DataSize;

  //
  // Check if variable exists, if so do not change it
  //
  Status = GetVariable2 (EFI_DBT_DEFAULT_VARIABLE_NAME, &gEfiGlobalVariableGuid, (VOID **) &Data, &DataSize);
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
  EFI_STATUS Status;

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
  EFI_STATUS Status;

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
  EFI_STATUS Status;

  Status = EnrollFromDefault (
             EFI_IMAGE_SECURITY_DATABASE2,
             EFI_DBT_DEFAULT_VARIABLE_NAME,
             &gEfiImageSecurityDatabaseGuid);

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
  EFI_STATUS Status;

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
  EFI_STATUS Status;

  Status = EnrollFromDefault (
             EFI_PLATFORM_KEY_NAME,
             EFI_PK_DEFAULT_VARIABLE_NAME,
             &gEfiGlobalVariableGuid
             );

  return Status;
}

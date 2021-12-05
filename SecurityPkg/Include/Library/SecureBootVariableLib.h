/** @file
  Provides a helper functions for creating variable authenticated
  payloads, signature lists related to secure boot keys.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2018 Hewlett Packard Enterprise Development LP<BR>
Copyright (c) 2021, ARM Ltd. All rights reserved.<BR>
Copyright (c) 2021, Semihalf All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SECURE_BOOT_VARIABLE_LIB_H_
#define SECURE_BOOT_VARIABLE_LIB_H_

/**
  Set the platform secure boot mode into "Custom" or "Standard" mode.

  @param[in]   SecureBootMode        New secure boot mode: STANDARD_SECURE_BOOT_MODE or
                                     CUSTOM_SECURE_BOOT_MODE.

  @return EFI_SUCCESS                The platform has switched to the special mode successfully.
  @return other                      Fail to operate the secure boot mode.

--*/
EFI_STATUS
SetSecureBootMode (
  IN  UINT8  SecureBootMode
  );

/**
  Fetches the value of SetupMode variable.

  @param[out] SetupMode             Pointer to UINT8 for SetupMode output

  @retval other                     Error codes from GetVariable.
--*/
EFI_STATUS
EFIAPI
GetSetupMode (
  OUT UINT8  *SetupMode
  );

/**
  Create a EFI Signature List with data fetched from section specified as a argument.
  Found keys are verified using RsaGetPublicKeyFromX509().

  @param[in]        KeyFileGuid    A pointer to to the FFS filename GUID
  @param[out]       SigListsSize   A pointer to size of signature list
  @param[out]       SigListsOut    a pointer to a callee-allocated buffer with signature lists

  @retval EFI_SUCCESS              Create time based payload successfully.
  @retval EFI_NOT_FOUND            Section with key has not been found.
  @retval EFI_INVALID_PARAMETER    Embedded key has a wrong format.
  @retval Others                   Unexpected error happens.

--*/
EFI_STATUS
SecureBootFetchData (
  IN  EFI_GUID            *KeyFileGuid,
  OUT UINTN               *SigListsSize,
  OUT EFI_SIGNATURE_LIST  **SigListOut
  );

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

  @retval EFI_SUCCESS              Create time based payload successfully.
  @retval EFI_OUT_OF_RESOURCES     There are not enough memory resources to create time based payload.
  @retval EFI_INVALID_PARAMETER    The parameter is invalid.
  @retval Others                   Unexpected error happens.

--*/
EFI_STATUS
CreateTimeBasedPayload (
  IN OUT UINTN  *DataSize,
  IN OUT UINT8  **Data
  );

/**
  Clears the content of the 'db' variable.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2(), GetTime() and SetVariable()
--*/
EFI_STATUS
EFIAPI
DeleteDb (
  VOID
  );

/**
  Clears the content of the 'dbx' variable.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2(), GetTime() and SetVariable()
--*/
EFI_STATUS
EFIAPI
DeleteDbx (
  VOID
  );

/**
  Clears the content of the 'dbt' variable.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2(), GetTime() and SetVariable()
--*/
EFI_STATUS
EFIAPI
DeleteDbt (
  VOID
  );

/**
  Clears the content of the 'KEK' variable.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2(), GetTime() and SetVariable()
--*/
EFI_STATUS
EFIAPI
DeleteKEK (
  VOID
  );

/**
  Clears the content of the 'PK' variable.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2(), GetTime() and SetVariable()
--*/
EFI_STATUS
EFIAPI
DeletePlatformKey (
  VOID
  );

#endif

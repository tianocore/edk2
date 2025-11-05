/** @file
  Provides a helper functions for creating variable authenticated
  payloads, signature lists related to secure boot keys.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2018 Hewlett Packard Enterprise Development LP<BR>
Copyright (c) 2021, ARM Ltd. All rights reserved.<BR>
Copyright (c) 2021, Semihalf All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
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
EFIAPI
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
  Helper function to quickly determine whether SecureBoot is enabled.

  @retval     TRUE    SecureBoot is verifiably enabled.
  @retval     FALSE   SecureBoot is either disabled or an error prevented checking.

**/
BOOLEAN
EFIAPI
IsSecureBootEnabled (
  VOID
  );

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

--*/
EFI_STATUS
EFIAPI
SecureBootCreateDataFromInput (
  OUT UINTN                               *SigListsSize,
  OUT EFI_SIGNATURE_LIST                  **SigListOut,
  IN  UINTN                               KeyInfoCount,
  IN  CONST SECURE_BOOT_CERTIFICATE_INFO  *KeyInfo
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

/**
  This function will delete the secure boot keys, thus
  disabling secure boot.

  @return EFI_SUCCESS or underlying failure code.
**/
EFI_STATUS
EFIAPI
DeleteSecureBootVariables (
  VOID
  );

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
  );

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
  );

#endif

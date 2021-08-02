/** @file
  Provides a functions to enroll keys based on default values.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2018 Hewlett Packard Enterprise Development LP<BR>
Copyright (c) 2021, ARM Ltd. All rights reserved.<BR>
Copyright (c) 2021, Semihalf All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SECURE_BOOT_VARIABLE_PROVISION_LIB_H_
#define SECURE_BOOT_VARIABLE_PROVISION_LIB_H_

/**
  Sets the content of the 'db' variable based on 'dbDefault' variable content.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2(), GetTime() and SetVariable()
--*/
EFI_STATUS
EFIAPI
EnrollDbFromDefault (
  VOID
);

/**
  Sets the content of the 'dbx' variable based on 'dbxDefault' variable content.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2(), GetTime() and SetVariable()
--*/
EFI_STATUS
EFIAPI
EnrollDbxFromDefault (
  VOID
);

/**
  Sets the content of the 'dbt' variable based on 'dbtDefault' variable content.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2(), GetTime() and SetVariable()
--*/
EFI_STATUS
EFIAPI
EnrollDbtFromDefault (
  VOID
);

/**
  Sets the content of the 'KEK' variable based on 'KEKDefault' variable content.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2(), GetTime() and SetVariable()
--*/
EFI_STATUS
EFIAPI
EnrollKEKFromDefault (
  VOID
);

/**
  Sets the content of the 'PK' variable based on 'PKDefault' variable content.

  @retval EFI_OUT_OF_RESOURCES      If memory allocation for EFI_VARIABLE_AUTHENTICATION_2 fails
                                    while VendorGuid is NULL.
  @retval other                     Errors from GetVariable2(), GetTime() and SetVariable()
--*/
EFI_STATUS
EFIAPI
EnrollPKFromDefault (
  VOID
);

/**
  Initializes PKDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
--*/
EFI_STATUS
SecureBootInitPKDefault (
  IN VOID
  );

/**
  Initializes KEKDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
--*/
EFI_STATUS
SecureBootInitKEKDefault (
  IN VOID
  );

/**
  Initializes dbDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
--*/
EFI_STATUS
SecureBootInitDbDefault (
  IN VOID
  );

/**
  Initializes dbtDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
--*/
EFI_STATUS
SecureBootInitDbtDefault (
  IN VOID
  );

/**
  Initializes dbxDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
--*/
EFI_STATUS
SecureBootInitDbxDefault (
  IN VOID
  );
#endif

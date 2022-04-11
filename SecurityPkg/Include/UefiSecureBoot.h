/** @file
  Provides a Secure Boot related data structure definitions.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef UEFI_SECURE_BOOT_H_
#define UEFI_SECURE_BOOT_H_

#pragma pack (push, 1)

/*
  Data structure to provide certificates to setup authenticated secure
  boot variables ('db', 'dbx', 'dbt', 'pk', etc.).

*/
typedef struct {
  //
  // The size, in number of bytes, of supplied certificate in 'Data' field.
  //
  UINTN         DataSize;
  //
  // The pointer to the certificates in DER-encoded format.
  // Note: This certificate data should not contain the EFI_VARIABLE_AUTHENTICATION_2
  //       for authenticated variables.
  //
  CONST VOID    *Data;
} SECURE_BOOT_CERTIFICATE_INFO;

/*
  Data structure to provide all Secure Boot related certificates.

*/
typedef struct {
  //
  // The human readable name for this set of Secure Boot key sets.
  //
  CONST CHAR16    *SecureBootKeyName;
  //
  // The size, in number of bytes, of supplied certificate in 'DbPtr' field.
  //
  UINTN           DbSize;
  //
  // The pointer to the DB certificates in signature list format.
  // Note: This DB certificates should not contain the EFI_VARIABLE_AUTHENTICATION_2
  //       for authenticated variables.
  //
  CONST VOID      *DbPtr;
  //
  // The size, in number of bytes, of supplied certificate in 'DbxPtr' field.
  //
  UINTN           DbxSize;
  //
  // The pointer to the DBX certificates in signature list format.
  // Note: This DBX certificates should not contain the EFI_VARIABLE_AUTHENTICATION_2
  //       for authenticated variables.
  //
  CONST VOID      *DbxPtr;
  //
  // The size, in number of bytes, of supplied certificate in 'DbtPtr' field.
  //
  UINTN           DbtSize;
  //
  // The pointer to the DBT certificates in signature list format.
  // Note: This DBT certificates should not contain the EFI_VARIABLE_AUTHENTICATION_2
  //       for authenticated variables.
  //
  CONST VOID      *DbtPtr;
  //
  // The size, in number of bytes, of supplied certificate in 'KekPtr' field.
  //
  UINTN           KekSize;
  //
  // The pointer to the KEK certificates in signature list format.
  // Note: This KEK certificates should not contain the EFI_VARIABLE_AUTHENTICATION_2
  //       for authenticated variables.
  //
  CONST VOID      *KekPtr;
  //
  // The size, in number of bytes, of supplied certificate in 'PkPtr' field.
  //
  UINTN           PkSize;
  //
  // The pointer to the PK certificates in signature list format.
  // Note: This PK certificates should not contain the EFI_VARIABLE_AUTHENTICATION_2
  //       for authenticated variables.
  //
  CONST VOID      *PkPtr;
} SECURE_BOOT_PAYLOAD_INFO;
#pragma pack (pop)

#endif // UEFI_SECURE_BOOT_H_

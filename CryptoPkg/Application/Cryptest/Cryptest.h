/** @file  
  Application for Cryptographic Primitives Validation.

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __CRYPTEST_H__
#define __CRYPTEST_H__

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/BaseCryptLib.h>

/**
  Validate UEFI-OpenSSL Digest Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptDigest (
  VOID
  );

/**
  Validate UEFI-OpenSSL Message Authentication Codes Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptHmac (
  VOID
  );

/**
  Validate UEFI-OpenSSL Block Ciphers (Symmetric Crypto) Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptBlockCipher (
  VOID
  );

/**
  Validate UEFI-OpenSSL RSA Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptRsa (
  VOID
  );

/**
  Validate UEFI-OpenSSL RSA Key Retrieving (from PEM & X509) & Signature Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptRsa2 (
  VOID
  );

/**
  Validate UEFI-OpenSSL PKCS#7 Signing & Verification Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptPkcs7 (
  VOID
  );

/**
  Validate UEFI-OpenSSL Authenticode Verification Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateAuthenticode (
  VOID
  );

/**
  Validate UEFI-OpenSSL DH Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptDh (
  VOID
  );

/**
  Validate UEFI-OpenSSL pseudorandom number generator interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptPrng (
  VOID
  );

#endif

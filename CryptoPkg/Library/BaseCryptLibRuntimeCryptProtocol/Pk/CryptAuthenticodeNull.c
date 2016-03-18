/** @file
  Authenticode Portable Executable Signature Verification which does not provide
  real capabilities.

Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"

/**
  Verifies the validility of a PE/COFF Authenticode Signature as described in "Windows
  Authenticode Portable Executable Signature Format".

  Return FALSE to indicate this interface is not supported.

  @param[in]  AuthData     Pointer to the Authenticode Signature retrieved from signed
                           PE/COFF image to be verified.
  @param[in]  DataSize     Size of the Authenticode Signature in bytes.
  @param[in]  TrustedCert  Pointer to a trusted/root certificate encoded in DER, which
                           is used for certificate chain verification.
  @param[in]  CertSize     Size of the trusted certificate in bytes.
  @param[in]  ImageHash    Pointer to the original image file hash value. The procudure
                           for calculating the image hash value is described in Authenticode
                           specification.
  @param[in]  HashSize     Size of Image hash value in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
AuthenticodeVerify (
  IN  CONST UINT8  *AuthData,
  IN  UINTN        DataSize,
  IN  CONST UINT8  *TrustedCert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *ImageHash,
  IN  UINTN        HashSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

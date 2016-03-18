/** @file  
  Application for RSA Primitives Validation.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Cryptest.h"

#define  RSA_MODULUS_LENGTH  512

//
// RSA PKCS#1 Validation Data from OpenSSL "Fips_rsa_selftest.c"
//

//
// Public Modulus of RSA Key
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 RsaN[] = {
  0xBB, 0xF8, 0x2F, 0x09, 0x06, 0x82, 0xCE, 0x9C, 0x23, 0x38, 0xAC, 0x2B, 0x9D, 0xA8, 0x71, 0xF7, 
  0x36, 0x8D, 0x07, 0xEE, 0xD4, 0x10, 0x43, 0xA4, 0x40, 0xD6, 0xB6, 0xF0, 0x74, 0x54, 0xF5, 0x1F,
  0xB8, 0xDF, 0xBA, 0xAF, 0x03, 0x5C, 0x02, 0xAB, 0x61, 0xEA, 0x48, 0xCE, 0xEB, 0x6F, 0xCD, 0x48,
  0x76, 0xED, 0x52, 0x0D, 0x60, 0xE1, 0xEC, 0x46, 0x19, 0x71, 0x9D, 0x8A, 0x5B, 0x8B, 0x80, 0x7F,
  0xAF, 0xB8, 0xE0, 0xA3, 0xDF, 0xC7, 0x37, 0x72, 0x3E, 0xE6, 0xB4, 0xB7, 0xD9, 0x3A, 0x25, 0x84,
  0xEE, 0x6A, 0x64, 0x9D, 0x06, 0x09, 0x53, 0x74, 0x88, 0x34, 0xB2, 0x45, 0x45, 0x98, 0x39, 0x4E,
  0xE0, 0xAA, 0xB1, 0x2D, 0x7B, 0x61, 0xA5, 0x1F, 0x52, 0x7A, 0x9A, 0x41, 0xF6, 0xC1, 0x68, 0x7F,
  0xE2, 0x53, 0x72, 0x98, 0xCA, 0x2A, 0x8F, 0x59, 0x46, 0xF8, 0xE5, 0xFD, 0x09, 0x1D, 0xBD, 0xCB
  };

//
// Public Exponent of RSA Key
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 RsaE[] = { 0x11 };

//
// Private Exponent of RSA Key
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 RsaD[] = {
  0xA5, 0xDA, 0xFC, 0x53, 0x41, 0xFA, 0xF2, 0x89, 0xC4, 0xB9, 0x88, 0xDB, 0x30, 0xC1, 0xCD, 0xF8,
  0x3F, 0x31, 0x25, 0x1E, 0x06, 0x68, 0xB4, 0x27, 0x84, 0x81, 0x38, 0x01, 0x57, 0x96, 0x41, 0xB2,
  0x94, 0x10, 0xB3, 0xC7, 0x99, 0x8D, 0x6B, 0xC4, 0x65, 0x74, 0x5E, 0x5C, 0x39, 0x26, 0x69, 0xD6,
  0x87, 0x0D, 0xA2, 0xC0, 0x82, 0xA9, 0x39, 0xE3, 0x7F, 0xDC, 0xB8, 0x2E, 0xC9, 0x3E, 0xDA, 0xC9,
  0x7F, 0xF3, 0xAD, 0x59, 0x50, 0xAC, 0xCF, 0xBC, 0x11, 0x1C, 0x76, 0xF1, 0xA9, 0x52, 0x94, 0x44,
  0xE5, 0x6A, 0xAF, 0x68, 0xC5, 0x6C, 0x09, 0x2C, 0xD3, 0x8D, 0xC3, 0xBE, 0xF5, 0xD2, 0x0A, 0x93,
  0x99, 0x26, 0xED, 0x4F, 0x74, 0xA1, 0x3E, 0xDD, 0xFB, 0xE1, 0xA1, 0xCE, 0xCC, 0x48, 0x94, 0xAF,
  0x94, 0x28, 0xC2, 0xB7, 0xB8, 0x88, 0x3F, 0xE4, 0x46, 0x3A, 0x4B, 0xC8, 0x5B, 0x1C, 0xB3, 0xC1
  };

//
// Known Answer Test (KAT) Data for RSA PKCS#1 Signing
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 RsaSignData[] = "OpenSSL FIPS 140-2 Public Key RSA KAT";

//
// Known Signature for the above message, under SHA-1 Digest
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 RsaPkcs1Signature[] = {
  0x71, 0xEE, 0x1A, 0xC0, 0xFE, 0x01, 0x93, 0x54, 0x79, 0x5C, 0xF2, 0x4C, 0x4A, 0xFD, 0x1A, 0x05,
  0x8F, 0x64, 0xB1, 0x6D, 0x61, 0x33, 0x8D, 0x9B, 0xE7, 0xFD, 0x60, 0xA3, 0x83, 0xB5, 0xA3, 0x51,
  0x55, 0x77, 0x90, 0xCF, 0xDC, 0x22, 0x37, 0x8E, 0xD0, 0xE1, 0xAE, 0x09, 0xE3, 0x3D, 0x1E, 0xF8,
  0x80, 0xD1, 0x8B, 0xC2, 0xEC, 0x0A, 0xD7, 0x6B, 0x88, 0x8B, 0x8B, 0xA1, 0x20, 0x22, 0xBE, 0x59,
  0x5B, 0xE0, 0x23, 0x24, 0xA1, 0x49, 0x30, 0xBA, 0xA9, 0x9E, 0xE8, 0xB1, 0x8A, 0x62, 0x16, 0xBF,
  0x4E, 0xCA, 0x2E, 0x4E, 0xBC, 0x29, 0xA8, 0x67, 0x13, 0xB7, 0x9F, 0x1D, 0x04, 0x44, 0xE5, 0x5F,
  0x35, 0x07, 0x11, 0xBC, 0xED, 0x19, 0x37, 0x21, 0xCF, 0x23, 0x48, 0x1F, 0x72, 0x05, 0xDE, 0xE6,
  0xE8, 0x7F, 0x33, 0x8A, 0x76, 0x4B, 0x2F, 0x95, 0xDF, 0xF1, 0x5F, 0x84, 0x80, 0xD9, 0x46, 0xB4
  };

//
// Default public key 0x10001 = 65537
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 DefaultPublicKey[] = {
  0x01, 0x00, 0x01
};

/**
  Validate UEFI-OpenSSL RSA Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptRsa (
  VOID
  )
{
  VOID     *Rsa;
  UINT8    HashValue[SHA1_DIGEST_SIZE];
  UINTN    HashSize;
  UINTN    CtxSize;
  VOID     *Sha1Ctx;
  UINT8    *Signature;
  UINTN    SigSize;
  BOOLEAN  Status;
  UINTN    KeySize;
  UINT8    *KeyBuffer;

  Print (L"\nUEFI-OpenSSL RSA Engine Testing: ");

  //
  // Generate & Initialize RSA Context
  //
  Rsa = RsaNew ();
  Print (L"\n- Generate RSA Context ... ");
  if (Rsa == NULL) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  //
  // Set/Get RSA Key Components
  //
  Print (L"Set/Get RSA Key Components ... ");

  //
  // Set/Get RSA Key N
  //
  Status = RsaSetKey (Rsa, RsaKeyN, RsaN, sizeof (RsaN));
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  KeySize = 0;
  Status = RsaGetKey (Rsa, RsaKeyN, NULL, &KeySize);
  if (Status || KeySize != sizeof (RsaN)) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  KeyBuffer = AllocatePool (KeySize);
  Status = RsaGetKey (Rsa, RsaKeyN, KeyBuffer, &KeySize);
  if (!Status || KeySize != sizeof (RsaN)) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (KeyBuffer, RsaN, KeySize) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  FreePool (KeyBuffer);

  //
  // Set/Get RSA Key E
  //
  Status = RsaSetKey (Rsa, RsaKeyE, RsaE, sizeof (RsaE));
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  KeySize = 0;
  Status = RsaGetKey (Rsa, RsaKeyE, NULL, &KeySize);
  if (Status || KeySize != sizeof (RsaE)) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  KeyBuffer = AllocatePool (KeySize);
  Status = RsaGetKey (Rsa, RsaKeyE, KeyBuffer, &KeySize);
  if (!Status || KeySize != sizeof (RsaE)) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (KeyBuffer, RsaE, KeySize) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  FreePool (KeyBuffer);

  //
  // Clear/Get RSA Key Components
  //
  Print (L"Clear/Get RSA Key Components ... ");

  //
  // Clear/Get RSA Key N
  //
  Status = RsaSetKey (Rsa, RsaKeyN, NULL, 0);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  KeySize = 1;
  Status = RsaGetKey (Rsa, RsaKeyN, NULL, &KeySize);
  if (!Status || KeySize != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  //
  // Clear/Get RSA Key E
  //
  Status = RsaSetKey (Rsa, RsaKeyE, NULL, 0);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  KeySize = 1;
  Status = RsaGetKey (Rsa, RsaKeyE, NULL, &KeySize);
  if (!Status || KeySize != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  //
  // Generate RSA Key Components
  //
  Print (L"Generate RSA Key Components ... ");

  Status = RsaGenerateKey (Rsa, RSA_MODULUS_LENGTH, NULL, 0);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  KeySize = RSA_MODULUS_LENGTH / 8;
  KeyBuffer = AllocatePool (KeySize);
  Status = RsaGetKey (Rsa, RsaKeyE, KeyBuffer, &KeySize);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }
  
  if (KeySize != 3 ||
      CompareMem (KeyBuffer, DefaultPublicKey, 3) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  KeySize = RSA_MODULUS_LENGTH / 8;
  Status = RsaGetKey (Rsa, RsaKeyN, KeyBuffer, &KeySize);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (KeySize != RSA_MODULUS_LENGTH / 8) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (!RsaCheckKey (Rsa)) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  //
  // Check invalid RSA key components
  //
  Print (L"Check Invalid RSA Key Components ... ");

  Status = RsaSetKey (Rsa, RsaKeyN, RsaN, sizeof (RsaN));
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (RsaCheckKey (Rsa)) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = RsaSetKey (Rsa, RsaKeyN, KeyBuffer, KeySize);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (!RsaCheckKey (Rsa)) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = RsaSetKey (Rsa, RsaKeyE, RsaE, sizeof (RsaE));
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (RsaCheckKey (Rsa)) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  FreePool (KeyBuffer);

  //
  // SHA-1 Digest Message for PKCS#1 Signature 
  //
  Print (L"Hash Original Message ... ");
  HashSize = SHA1_DIGEST_SIZE;
  ZeroMem (HashValue, HashSize);
  CtxSize = Sha1GetContextSize ();
  Sha1Ctx = AllocatePool (CtxSize);

  Status  = Sha1Init (Sha1Ctx);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status  = Sha1Update (Sha1Ctx, RsaSignData, AsciiStrLen (RsaSignData));
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status  = Sha1Final (Sha1Ctx, HashValue);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  FreePool (Sha1Ctx);

  //
  // Sign RSA PKCS#1-encoded Signature
  //
  Print (L"PKCS#1 Signature ... ");

  RsaFree (Rsa);

  Rsa = RsaNew ();
  if (Rsa == NULL) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = RsaSetKey (Rsa, RsaKeyN, RsaN, sizeof (RsaN));
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = RsaSetKey (Rsa, RsaKeyE, RsaE, sizeof (RsaE));
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Status = RsaSetKey (Rsa, RsaKeyD, RsaD, sizeof (RsaD));
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  SigSize = 0;
  Status  = RsaPkcs1Sign (Rsa, HashValue, HashSize, NULL, &SigSize);
  if (Status || SigSize == 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Signature = AllocatePool (SigSize);
  Status  = RsaPkcs1Sign (Rsa, HashValue, HashSize, Signature, &SigSize);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (SigSize != sizeof (RsaPkcs1Signature)) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  if (CompareMem (Signature, RsaPkcs1Signature, SigSize) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  //
  // Verify RSA PKCS#1-encoded Signature
  //

  Print (L"PKCS#1 Signature Verification ... ");

  Status = RsaPkcs1Verify (Rsa, HashValue, HashSize, Signature, SigSize);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  //
  // Release Resources
  //
  RsaFree (Rsa);
  Print (L"Release RSA Context ... [Pass]");

  Print (L"\n");

  return EFI_SUCCESS;
}

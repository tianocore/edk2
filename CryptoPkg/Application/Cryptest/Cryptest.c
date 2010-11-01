/** @file  
  Application for Cryptographic Primitives Validation.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/DebugLib.h>

#include <Library/BaseCryptLib.h>

//
// Max Known Digest Size is SHA512 Output (64 bytes) by far
//
#define MAX_DIGEST_SIZE    64

//
// Message string for digest validation
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *HashData = "abc";

//
// Result for MD5("abc"). (From "A.5 Test suite" of IETF RFC1321)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Md5Digest[MD5_DIGEST_SIZE] = {
  0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0, 0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72
  };

//
// Result for SHA-1("abc"). (From "A.1 SHA-1 Example" of NIST FIPS 180-2)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Sha1Digest[SHA1_DIGEST_SIZE] = {
  0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c,
  0x9c, 0xd0, 0xd8, 0x9d
  };

//
// Result for SHA-256("abc"). (From "B.1 SHA-256 Example" of NIST FIPS 180-2)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Sha256Digest[SHA256_DIGEST_SIZE] = {
  0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
  0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
  };

//
// RSA PKCS#1 Validation Data from OpenSSL "Fips_rsa_selftest.c"
//

// Public Modulus of RSA Key
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

// Public Exponent of RSA Key
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 RsaE[] = { 0x11 };

// Known Answer Test (KAT) Data for RSA PKCS#1 Signing
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 RsaSignData[] = "OpenSSL FIPS 140-2 Public Key RSA KAT";

// Known Signature for the above message, under SHA-1 Digest
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

/**
  Validate MSFT Authenticode using PKCS#7 Verification Interfaces.

  @return  EFI_SUCCESS  Validation succeeds. 

**/
BOOLEAN
AuthenticodeVerify (
  VOID
  );

/**
  Validate UEFI-OpenSSL Digest Interfaces.

  @return  EFI_SUCCESS  Validation succeeded.

**/
EFI_STATUS
ValidateCryptDigest (
  VOID
  )
{
  UINTN    CtxSize;
  VOID     *HashCtx;
  UINTN    DataSize;
  UINT8    Digest[MAX_DIGEST_SIZE];
  UINTN    Index;
  BOOLEAN  Status;

  Print (L" UEFI-OpenSSL Hash Engine Testing (Hashing(\"abc\")): ");
  DataSize = AsciiStrLen (HashData);

  //
  // MD5 Digest Validation
  //
  ZeroMem (Digest, MAX_DIGEST_SIZE);
  CtxSize = Md5GetContextSize ();
  HashCtx = AllocatePool (CtxSize);
  Status  = Md5Init (HashCtx);
  Status  = Md5Update (HashCtx, HashData, DataSize);
  Status  = Md5Final (HashCtx, Digest);
  FreePool (HashCtx);
  Print (L"\n   - MD5 Digest: \n     = 0x");
  for (Index = 0; Index < MD5_DIGEST_SIZE; Index++) {
    Print (L"%02x", Digest[Index]);
  }
  if (CompareMem (Digest, Md5Digest, MD5_DIGEST_SIZE) == 0) {
    Print (L" [Pass]");
  } else {
    Print (L" [Failed]");
  }

  //
  // SHA-1 Digest Validation
  //
  ZeroMem (Digest, MAX_DIGEST_SIZE);
  CtxSize = Sha1GetContextSize ();
  HashCtx = AllocatePool (CtxSize);
  Status  = Sha1Init (HashCtx);
  Status  = Sha1Update (HashCtx, HashData, DataSize);
  Status  = Sha1Final (HashCtx, Digest);
  FreePool (HashCtx);
  Print (L"\n   - SHA-1 Digest: \n     = 0x");
  for (Index = 0; Index < SHA1_DIGEST_SIZE; Index++) {
    Print (L"%02x", Digest[Index]);
  }
  if (CompareMem (Digest, Sha1Digest, SHA1_DIGEST_SIZE) == 0) {
    Print (L" [Pass]");
  } else {
    Print (L" [Failed]");
  }

  //
  // SHA256 Digest Validation
  //
  ZeroMem (Digest, MAX_DIGEST_SIZE);
  CtxSize = Sha256GetContextSize ();
  HashCtx = AllocatePool (CtxSize);
  Status  = Sha256Init (HashCtx);
  Status  = Sha256Update (HashCtx, HashData, DataSize);
  Status  = Sha256Final (HashCtx, Digest);
  FreePool (HashCtx);
  Print (L"\n   - SHA-256 Digest: \n     = 0x");
  for (Index = 0; Index < SHA256_DIGEST_SIZE; Index++) {
    Print (L"%02x", Digest[Index]);
  }
  if (CompareMem (Digest, Sha256Digest, SHA256_DIGEST_SIZE) == 0) {
    Print (L" [Pass]");
  } else {
    Print (L" [Failed]");
  }  

  Print (L"\n");
  
  return EFI_SUCCESS;
}


/**
  Validate UEFI-OpenSSL Message Authentication Codes Interfaces.

  @return  EFI_SUCCESS  Validation succeeded. 

**/
EFI_STATUS
ValidateCryptHmac (
  VOID
  )
{
  Print (L"\n UEFI-OpenSSL HMAC Engine Testing: ");
  Print (L"\n   ==> No HMAC Support in Base Crypto Library!\n");

  return EFI_SUCCESS;
}


/**
  Validate UEFI-OpenSSL Block Ciphers (Symmetric Crypto) Interfaces.

  @return  EFI_SUCCESS  Validation succeeded.

**/
EFI_STATUS
ValidateCryptBlockCipher (
  VOID
  )
{
  Print (L"\n UEFI-OpenSSL Block Cipher Engine Testing: ");
  Print (L"\n   ==> No Block Cipher Support in Base Crypto Library!\n");

  return EFI_SUCCESS;
}


/**
  Validate UEFI-OpenSSL RSA Interfaces.

  @return  EFI_SUCCESS  Validation succeeded.

**/
EFI_STATUS
ValidateCryptRsa (
  VOID
  )
{
  VOID     *Rsa;
  UINT8    mHash[SHA1_DIGEST_SIZE];
  UINTN    HashSize;
  UINTN    CtxSize;
  VOID     *Sha1Ctx;
  UINT8    *Signature;
  UINTN    SigSize;
  BOOLEAN  Status;

  Print (L"\n UEFI-OpenSSL RSA Engine Testing: ");

  //
  // Generate & Initialize RSA Context
  //
  Rsa = RsaNew ();
  Print (L"\n   - Generate RSA Context .............. ");
  if (Rsa != NULL) {
    Print (L"[Pass]");
  } else {
    Print (L"[Failed]");
  }

  //
  // Set RSA Key Components
  // NOTE: Only N and E are needed to be set as RSA public key for signature verification
  //
  Print (L"\n   - Set RSA Key Components ............ ");
  Status = RsaSetKey (Rsa, RsaKeyN, RsaN, sizeof (RsaN));
  Status = RsaSetKey (Rsa, RsaKeyE, RsaE, sizeof (RsaE));
  if (Status) {
    Print (L"[Pass]");
  } else {
    Print (L"[Failed]");
  }

  //
  // SHA-1 Digest Message for PKCS#1 Signature 
  //
  Print (L"\n   - Hash Original Message ............. ");
  HashSize = SHA1_DIGEST_SIZE;
  ZeroMem (mHash, HashSize);
  CtxSize = Sha1GetContextSize ();
  Sha1Ctx = AllocatePool (CtxSize);
  Status  = Sha1Init (Sha1Ctx);
  Status  = Sha1Update (Sha1Ctx, RsaSignData, AsciiStrLen (RsaSignData));
  Status  = Sha1Final (Sha1Ctx, mHash);
  FreePool (Sha1Ctx);
  if (Status) {
    Print (L"[Pass]");
  } else {
    Print (L"[Failed]");
  }

  //
  // Verify RSA PKCS#1-encoded Signature
  //
  Print (L"\n   - PKCS#1 Signature Verification ..... ");
  SigSize   = sizeof (RsaPkcs1Signature);
  Signature = (UINT8 *)AllocatePool (SigSize);
  CopyMem (Signature, RsaPkcs1Signature, SigSize);
  Status = RsaPkcs1Verify (Rsa, mHash, HashSize, Signature, SigSize);
  if (Status) {
    Print (L"[Pass]");
  } else {
    Print (L"[Failed]");
  }   

  //
  // Release Resources
  //
  RsaFree (Rsa);
  Print (L"\n   - Release RSA Context ............... [Pass]");

  Print (L"\n");

  return EFI_SUCCESS;
}

/**
  Validate UEFI-OpenSSL PKCS#7 Verification Interfaces.

  @return  EFI_SUCCESS  Validation succeeded.

**/
EFI_STATUS
ValidateAuthenticode (
  VOID
  )
{
  Print (L"\n UEFI-OpenSSL PKCS#7-Signed-Data Testing: ");

  Print (L"\n   - Authenticode (PKCS#7 Signed Data) Verification ... ");

  if (AuthenticodeVerify ()) {
    Print (L"[Pass]");
  } else {
    Print (L"[Failed]");
  }   

  Print (L"\n");

  return EFI_SUCCESS;
}


/**
  Entry Point of Cryptographic Validation Utility.

  @param  ImageHandle  The image handle of the UEFI Application.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
CryptestMain (
  IN     EFI_HANDLE                 ImageHandle,
  IN     EFI_SYSTEM_TABLE           *SystemTable
  )
{
  EFI_STATUS  Status;

  Print (L"\nUEFI-OpenSSL Wrapper Cryptosystem Testing: \n");
  Print (L"-------------------------------------------- \n");

  Status = EFI_SUCCESS;
  Status = ValidateCryptDigest ();
  Status = ValidateCryptHmac ();
  Status = ValidateCryptBlockCipher ();
  Status = ValidateCryptRsa ();
  Status = ValidateAuthenticode ();

  return Status;
}

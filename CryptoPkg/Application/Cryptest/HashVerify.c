/** @file
  Application for Hash Primitives Validation.

Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Cryptest.h"

//
// Max Known Digest Size is SHA512 Output (64 bytes) by far
//
#define MAX_DIGEST_SIZE    64

//
// Message string for digest validation
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *HashData = "abc";

//
// Result for MD4("abc"). (From "A.5 Test suite" of IETF RFC1320)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Md4Digest[MD4_DIGEST_SIZE] = {
  0xa4, 0x48, 0x01, 0x7a, 0xaf, 0x21, 0xd8, 0x52, 0x5f, 0xc1, 0x0a, 0xe8, 0x7a, 0xa6, 0x72, 0x9d
  };

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
// Result for SHA-384("abc"). (From "D.1 SHA-384 Example" of NIST FIPS 180-2)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Sha384Digest[SHA384_DIGEST_SIZE] = {
  0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b, 0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
  0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63, 0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
  0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23, 0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7
  };

//
// Result for SHA-512("abc"). (From "C.1 SHA-512 Example" of NIST FIPS 180-2)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 Sha512Digest[SHA512_DIGEST_SIZE] = {
  0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba, 0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
  0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2, 0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
  0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8, 0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
  0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e, 0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f
  };

/**
  Validate UEFI-OpenSSL Digest Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

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
  BOOLEAN  Status;

  Print (L" UEFI-OpenSSL Hash Engine Testing:\n");
  DataSize = AsciiStrLen (HashData);

  Print (L"- MD4:    ");

  //
  // MD4 Digest Validation
  //
  ZeroMem (Digest, MAX_DIGEST_SIZE);
  CtxSize = Md4GetContextSize ();
  HashCtx = AllocatePool (CtxSize);

  Print (L"Init... ");
  Status  = Md4Init (HashCtx);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Update... ");
  Status  = Md4Update (HashCtx, HashData, DataSize);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Finalize... ");
  Status  = Md4Final (HashCtx, Digest);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  FreePool (HashCtx);

  Print (L"Check Value... ");
  if (CompareMem (Digest, Md4Digest, MD5_DIGEST_SIZE) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"[Pass]\n");

  Print (L"- MD5:    ");

  //
  // MD5 Digest Validation
  //
  ZeroMem (Digest, MAX_DIGEST_SIZE);
  CtxSize = Md5GetContextSize ();
  HashCtx = AllocatePool (CtxSize);

  Print (L"Init... ");
  Status  = Md5Init (HashCtx);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Update... ");
  Status  = Md5Update (HashCtx, HashData, DataSize);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Finalize... ");
  Status  = Md5Final (HashCtx, Digest);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  FreePool (HashCtx);

  Print (L"Check Value... ");
  if (CompareMem (Digest, Md5Digest, MD5_DIGEST_SIZE) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"[Pass]\n");

  Print (L"- SHA1:   ");

  //
  // SHA-1 Digest Validation
  //
  ZeroMem (Digest, MAX_DIGEST_SIZE);
  CtxSize = Sha1GetContextSize ();
  HashCtx = AllocatePool (CtxSize);

  Print (L"Init... ");
  Status  = Sha1Init (HashCtx);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Update... ");
  Status  = Sha1Update (HashCtx, HashData, DataSize);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Finalize... ");
  Status  = Sha1Final (HashCtx, Digest);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  FreePool (HashCtx);

  Print (L"Check Value... ");
  if (CompareMem (Digest, Sha1Digest, SHA1_DIGEST_SIZE) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"[Pass]\n");

  Print (L"- SHA256: ");

  //
  // SHA256 Digest Validation
  //
  ZeroMem (Digest, MAX_DIGEST_SIZE);
  CtxSize = Sha256GetContextSize ();
  HashCtx = AllocatePool (CtxSize);

  Print (L"Init... ");
  Status  = Sha256Init (HashCtx);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Update... ");
  Status  = Sha256Update (HashCtx, HashData, DataSize);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Finalize... ");
  Status  = Sha256Final (HashCtx, Digest);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  FreePool (HashCtx);

  Print (L"Check Value... ");
  if (CompareMem (Digest, Sha256Digest, SHA256_DIGEST_SIZE) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"[Pass]\n");

  Print (L"- SHA384: ");

  //
  // SHA384 Digest Validation
  //
  ZeroMem (Digest, MAX_DIGEST_SIZE);
  CtxSize = Sha384GetContextSize ();
  HashCtx = AllocatePool (CtxSize);

  Print (L"Init... ");
  Status  = Sha384Init (HashCtx);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Update... ");
  Status  = Sha384Update (HashCtx, HashData, DataSize);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Finalize... ");
  Status  = Sha384Final (HashCtx, Digest);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  FreePool (HashCtx);

  Print (L"Check Value... ");
  if (CompareMem (Digest, Sha384Digest, SHA384_DIGEST_SIZE) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"[Pass]\n");

  Print (L"- SHA512: ");

  //
  // SHA512 Digest Validation
  //
  ZeroMem (Digest, MAX_DIGEST_SIZE);
  CtxSize = Sha512GetContextSize ();
  HashCtx = AllocatePool (CtxSize);

  Print (L"Init... ");
  Status  = Sha512Init (HashCtx);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Update... ");
  Status  = Sha512Update (HashCtx, HashData, DataSize);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Finalize... ");
  Status  = Sha512Final (HashCtx, Digest);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  FreePool (HashCtx);

  Print (L"Check Value... ");
  if (CompareMem (Digest, Sha512Digest, SHA512_DIGEST_SIZE) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"[Pass]\n");

  return EFI_SUCCESS;
}

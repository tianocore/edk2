/** @file  
  Application for HMAC Primitives Validation.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
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
// Data string for HMAC validation
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *HmacData = "Hi There";

//
// Key value for HMAC-MD5 validation. (From "2. Test Cases for HMAC-MD5" of IETF RFC2202)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 HmacMd5Key[16] = {
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
  };

//
// Result for HMAC-MD5("Hi There"). (From "2. Test Cases for HMAC-MD5" of IETF RFC2202)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 HmacMd5Digest[] = {
  0x92, 0x94, 0x72, 0x7a, 0x36, 0x38, 0xbb, 0x1c, 0x13, 0xf4, 0x8e, 0xf8, 0x15, 0x8b, 0xfc, 0x9d
  };

//
// Key value for HMAC-SHA-1 validation. (From "3. Test Cases for HMAC-SHA-1" of IETF RFC2202)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 HmacSha1Key[20] = {
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  0x0b, 0x0b, 0x0b, 0x0b
  };

//
// Result for HMAC-SHA-1 ("Hi There"). (From "3. Test Cases for HMAC-SHA-1" of IETF RFC2202)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 HmacSha1Digest[] = {
  0xb6, 0x17, 0x31, 0x86, 0x55, 0x05, 0x72, 0x64, 0xe2, 0x8b, 0xc0, 0xb6, 0xfb, 0x37, 0x8c, 0x8e,
  0xf1, 0x46, 0xbe, 0x00
  };

/**
  Validate UEFI-OpenSSL Message Authentication Codes Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptHmac (
  VOID
  )
{
  UINTN    CtxSize;
  VOID     *HmacCtx;
  UINT8    Digest[MAX_DIGEST_SIZE];
  BOOLEAN  Status;

  Print (L" \nUEFI-OpenSSL HMAC Engine Testing:\n");

  Print (L"- HMAC-MD5:  ");

  //
  // HMAC-MD5 Digest Validation
  //
  ZeroMem (Digest, MAX_DIGEST_SIZE);
  CtxSize = HmacMd5GetContextSize ();
  HmacCtx = AllocatePool (CtxSize);

  Print (L"Init... ");
  Status  = HmacMd5Init (HmacCtx, HmacMd5Key, sizeof (HmacMd5Key));
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Update... ");
  Status  = HmacMd5Update (HmacCtx, HmacData, 8);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Finalize... ");
  Status  = HmacMd5Final (HmacCtx, Digest);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  FreePool (HmacCtx);

  Print (L"Check Value... ");
  if (CompareMem (Digest, HmacMd5Digest, MD5_DIGEST_SIZE) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"[Pass]\n");

  Print (L"- HMAC-SHA1: ");

  //
  // HMAC-SHA1 Digest Validation
  //
  ZeroMem (Digest, MAX_DIGEST_SIZE);
  CtxSize = HmacSha1GetContextSize ();
  HmacCtx = AllocatePool (CtxSize);

  Print (L"Init... ");
  Status  = HmacSha1Init (HmacCtx, HmacSha1Key, sizeof (HmacSha1Key));
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Update... ");
  Status  = HmacSha1Update (HmacCtx, HmacData, 8);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"Finalize... ");
  Status  = HmacSha1Final (HmacCtx, Digest);
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  FreePool (HmacCtx);

  Print (L"Check Value... ");
  if (CompareMem (Digest, HmacSha1Digest, SHA1_DIGEST_SIZE) != 0) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  Print (L"[Pass]\n");

  return EFI_SUCCESS;
}

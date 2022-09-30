/** @file
  Application for HMAC Primitives Validation.

Copyright (c) 2010 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

//
// Max Known Digest Size is SHA512 Output (64 bytes) by far
//
#define MAX_DIGEST_SIZE  64

//
// Data string for HMAC validation
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8  *HmacData = "Hi There";

//
// Key value for HMAC-MD5 validation. (From "2. Test Cases for HMAC-MD5" of IETF RFC2202)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  HmacMd5Key[16] = {
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
};

//
// Result for HMAC-MD5("Hi There"). (From "2. Test Cases for HMAC-MD5" of IETF RFC2202)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  HmacMd5Digest[] = {
  0x92, 0x94, 0x72, 0x7a, 0x36, 0x38, 0xbb, 0x1c, 0x13, 0xf4, 0x8e, 0xf8, 0x15, 0x8b, 0xfc, 0x9d
};

//
// Key value for HMAC-SHA-1 validation. (From "3. Test Cases for HMAC-SHA-1" of IETF RFC2202)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  HmacSha1Key[20] = {
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  0x0b, 0x0b, 0x0b, 0x0b
};

//
// Result for HMAC-SHA-1 ("Hi There"). (From "3. Test Cases for HMAC-SHA-1" of IETF RFC2202)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  HmacSha1Digest[] = {
  0xb6, 0x17, 0x31, 0x86, 0x55, 0x05, 0x72, 0x64, 0xe2, 0x8b, 0xc0, 0xb6, 0xfb, 0x37, 0x8c, 0x8e,
  0xf1, 0x46, 0xbe, 0x00
};

//
// Key value for HMAC-SHA-256 validation. (From "4. Test Vectors" of IETF RFC4231)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  HmacSha256Key[20] = {
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  0x0b, 0x0b, 0x0b, 0x0b
};

//
// Result for HMAC-SHA-256 ("Hi There"). (From "4. Test Vectors" of IETF RFC4231)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  HmacSha256Digest[] = {
  0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53, 0x5c, 0xa8, 0xaf, 0xce, 0xaf, 0x0b, 0xf1, 0x2b,
  0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7, 0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7
};

//
// Key value for HMAC-SHA-384 validation. (From "4. Test Vectors" of IETF RFC4231)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  HmacSha384Key[20] = {
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  0x0b, 0x0b, 0x0b, 0x0b
};

//
// Result for HMAC-SHA-384 ("Hi There"). (From "4. Test Vectors" of IETF RFC4231)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  HmacSha384Digest[] = {
  0xaf, 0xd0, 0x39, 0x44, 0xd8, 0x48, 0x95, 0x62, 0x6b, 0x08, 0x25, 0xf4, 0xab, 0x46, 0x90, 0x7f,
  0x15, 0xf9, 0xda, 0xdb, 0xe4, 0x10, 0x1e, 0xc6, 0x82, 0xaa, 0x03, 0x4c, 0x7c, 0xeb, 0xc5, 0x9c,
  0xfa, 0xea, 0x9e, 0xa9, 0x07, 0x6e, 0xde, 0x7f, 0x4a, 0xf1, 0x52, 0xe8, 0xb2, 0xfa, 0x9c, 0xb6
};

typedef
VOID *
(EFIAPI *EFI_HMAC_NEW)(
  VOID
  );

typedef
VOID
(EFIAPI *EFI_HMAC_FREE)(
  IN VOID  *HashContext
  );

typedef
BOOLEAN
(EFIAPI *EFI_HMAC_INIT)(
  IN OUT  VOID        *HashContext,
  IN   CONST UINT8    *Key,
  IN   UINTN           KeySize
  );

typedef
BOOLEAN
(EFIAPI *EFI_HMAC_UPDATE)(
  IN OUT  VOID        *HashContext,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

typedef
BOOLEAN
(EFIAPI *EFI_HMAC_FINAL)(
  IN OUT  VOID   *HashContext,
  OUT     UINT8  *HashValue
  );

typedef struct {
  UINT32             DigestSize;
  EFI_HMAC_NEW       HmacNew;
  EFI_HMAC_FREE      HmacFree;
  EFI_HMAC_INIT      HmacInit;
  EFI_HMAC_UPDATE    HmacUpdate;
  EFI_HMAC_FINAL     HmacFinal;
  CONST UINT8        *Key;
  UINTN              KeySize;
  CONST UINT8        *Digest;
  VOID               *HmacCtx;
} HMAC_TEST_CONTEXT;

// These functions have been deprecated but they've been left commented out for future reference
// HMAC_TEST_CONTEXT       mHmacMd5TestCtx    = {MD5_DIGEST_SIZE,    HmacMd5New,    HmacMd5Free,  HmacMd5SetKey,    HmacMd5Update,    HmacMd5Final,    HmacMd5Key,    sizeof(HmacMd5Key),    HmacMd5Digest};
// HMAC_TEST_CONTEXT       mHmacSha1TestCtx   = {SHA1_DIGEST_SIZE,   HmacSha1New,   HmacSha1Free, HmacSha1SetKey,   HmacSha1Update,   HmacSha1Final,   HmacSha1Key,   sizeof(HmacSha1Key),   HmacSha1Digest};
HMAC_TEST_CONTEXT  mHmacSha256TestCtx = { SHA256_DIGEST_SIZE, HmacSha256New, HmacSha256Free, HmacSha256SetKey, HmacSha256Update, HmacSha256Final, HmacSha256Key, sizeof (HmacSha256Key), HmacSha256Digest };
HMAC_TEST_CONTEXT  mHmacSha384TestCtx = { SHA384_DIGEST_SIZE, HmacSha384New, HmacSha384Free, HmacSha384SetKey, HmacSha384Update, HmacSha384Final, HmacSha384Key, sizeof (HmacSha384Key), HmacSha384Digest };

UNIT_TEST_STATUS
EFIAPI
TestVerifyHmacPreReq (
  UNIT_TEST_CONTEXT  Context
  )
{
  HMAC_TEST_CONTEXT  *HmacTestContext;

  HmacTestContext          = Context;
  HmacTestContext->HmacCtx = HmacTestContext->HmacNew ();
  if (HmacTestContext->HmacCtx == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

VOID
EFIAPI
TestVerifyHmacCleanUp (
  UNIT_TEST_CONTEXT  Context
  )
{
  HMAC_TEST_CONTEXT  *HmacTestContext;

  HmacTestContext = Context;
  if (HmacTestContext->HmacCtx != NULL) {
    HmacTestContext->HmacFree (HmacTestContext->HmacCtx);
  }
}

UNIT_TEST_STATUS
EFIAPI
TestVerifyHmac (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8              Digest[MAX_DIGEST_SIZE];
  BOOLEAN            Status;
  HMAC_TEST_CONTEXT  *HmacTestContext;

  HmacTestContext = Context;

  ZeroMem (Digest, MAX_DIGEST_SIZE);

  Status = HmacTestContext->HmacInit (HmacTestContext->HmacCtx, HmacTestContext->Key, HmacTestContext->KeySize);
  UT_ASSERT_TRUE (Status);

  Status = HmacTestContext->HmacUpdate (HmacTestContext->HmacCtx, HmacData, 8);
  UT_ASSERT_TRUE (Status);

  Status = HmacTestContext->HmacFinal (HmacTestContext->HmacCtx, Digest);
  UT_ASSERT_TRUE (Status);

  UT_ASSERT_MEM_EQUAL (Digest, HmacTestContext->Digest, HmacTestContext->DigestSize);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mHmacTest[] = {
  //
  // -----Description---------------------Class---------------------Function---------------Pre------------------Post------------Context
  //
  { "TestVerifyHmacSha256()", "CryptoPkg.BaseCryptLib.Hmac", TestVerifyHmac, TestVerifyHmacPreReq, TestVerifyHmacCleanUp, &mHmacSha256TestCtx },
  { "TestVerifyHmacSha384()", "CryptoPkg.BaseCryptLib.Hmac", TestVerifyHmac, TestVerifyHmacPreReq, TestVerifyHmacCleanUp, &mHmacSha384TestCtx },
  // These functions have been deprecated but they've been left commented out for future reference
  // {"TestVerifyHmacMd5()",    "CryptoPkg.BaseCryptLib.Hmac",   TestVerifyHmac, TestVerifyHmacPreReq, TestVerifyHmacCleanUp, &mHmacMd5TestCtx},
  // {"TestVerifyHmacSha1()",   "CryptoPkg.BaseCryptLib.Hmac",   TestVerifyHmac, TestVerifyHmacPreReq, TestVerifyHmacCleanUp, &mHmacSha1TestCtx},
};

UINTN  mHmacTestNum = ARRAY_SIZE (mHmacTest);

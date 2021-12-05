/** @file
  Application for Hash Primitives Validation.

Copyright (c) 2010 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

//
// Max Known Digest Size is SHA512 Output (64 bytes) by far
//
#define MAX_DIGEST_SIZE  64

//
// Message string for digest validation
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8  *HashData = "abc";

//
// Result for MD5("abc"). (From "A.5 Test suite" of IETF RFC1321)
//
#ifdef ENABLE_MD5_DEPRECATED_INTERFACES
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Md5Digest[MD5_DIGEST_SIZE] = {
  0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0, 0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72
};
#endif

//
// Result for SHA-1("abc"). (From "A.1 SHA-1 Example" of NIST FIPS 180-2)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Sha1Digest[SHA1_DIGEST_SIZE] = {
  0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c,
  0x9c, 0xd0, 0xd8, 0x9d
};

//
// Result for SHA-256("abc"). (From "B.1 SHA-256 Example" of NIST FIPS 180-2)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Sha256Digest[SHA256_DIGEST_SIZE] = {
  0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
  0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
};

//
// Result for SHA-384("abc"). (From "D.1 SHA-384 Example" of NIST FIPS 180-2)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Sha384Digest[SHA384_DIGEST_SIZE] = {
  0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b, 0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
  0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63, 0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
  0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23, 0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7
};

//
// Result for SHA-512("abc"). (From "C.1 SHA-512 Example" of NIST FIPS 180-2)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Sha512Digest[SHA512_DIGEST_SIZE] = {
  0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba, 0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
  0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2, 0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
  0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8, 0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
  0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e, 0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f
};

typedef
UINTN
(EFIAPI *EFI_HASH_GET_CONTEXT_SIZE)(
  VOID
  );

typedef
BOOLEAN
(EFIAPI *EFI_HASH_INIT)(
  OUT  VOID  *HashContext
  );

typedef
BOOLEAN
(EFIAPI *EFI_HASH_UPDATE)(
  IN OUT  VOID        *HashContext,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

typedef
BOOLEAN
(EFIAPI *EFI_HASH_FINAL)(
  IN OUT  VOID   *HashContext,
  OUT     UINT8  *HashValue
  );

typedef
BOOLEAN
(EFIAPI *EFI_HASH_ALL)(
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  );

typedef struct {
  UINT32                       DigestSize;
  EFI_HASH_GET_CONTEXT_SIZE    GetContextSize;
  EFI_HASH_INIT                HashInit;
  EFI_HASH_UPDATE              HashUpdate;
  EFI_HASH_FINAL               HashFinal;
  EFI_HASH_ALL                 HashAll;
  CONST UINT8                  *Digest;
  VOID                         *HashCtx;
} HASH_TEST_CONTEXT;

#ifdef ENABLE_MD5_DEPRECATED_INTERFACES
HASH_TEST_CONTEXT  mMd5TestCtx = { MD5_DIGEST_SIZE, Md5GetContextSize, Md5Init, Md5Update, Md5Final, Md5HashAll, Md5Digest };
#endif
HASH_TEST_CONTEXT  mSha1TestCtx   = { SHA1_DIGEST_SIZE, Sha1GetContextSize, Sha1Init, Sha1Update, Sha1Final, Sha1HashAll, Sha1Digest };
HASH_TEST_CONTEXT  mSha256TestCtx = { SHA256_DIGEST_SIZE, Sha256GetContextSize, Sha256Init, Sha256Update, Sha256Final, Sha256HashAll, Sha256Digest };
HASH_TEST_CONTEXT  mSha384TestCtx = { SHA384_DIGEST_SIZE, Sha384GetContextSize, Sha384Init, Sha384Update, Sha384Final, Sha384HashAll, Sha384Digest };
HASH_TEST_CONTEXT  mSha512TestCtx = { SHA512_DIGEST_SIZE, Sha512GetContextSize, Sha512Init, Sha512Update, Sha512Final, Sha512HashAll, Sha512Digest };

UNIT_TEST_STATUS
EFIAPI
TestVerifyHashPreReq (
  UNIT_TEST_CONTEXT  Context
  )
{
  HASH_TEST_CONTEXT  *HashTestContext;
  UINTN              CtxSize;

  HashTestContext          = Context;
  CtxSize                  = HashTestContext->GetContextSize ();
  HashTestContext->HashCtx = AllocatePool (CtxSize);
  if (HashTestContext->HashCtx == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

VOID
EFIAPI
TestVerifyHashCleanUp (
  UNIT_TEST_CONTEXT  Context
  )
{
  HASH_TEST_CONTEXT  *HashTestContext;

  HashTestContext = Context;
  if (HashTestContext->HashCtx != NULL) {
    FreePool (HashTestContext->HashCtx);
  }
}

UNIT_TEST_STATUS
EFIAPI
TestVerifyHash (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN              DataSize;
  UINT8              Digest[MAX_DIGEST_SIZE];
  BOOLEAN            Status;
  HASH_TEST_CONTEXT  *HashTestContext;

  HashTestContext = Context;

  DataSize = AsciiStrLen (HashData);

  ZeroMem (Digest, MAX_DIGEST_SIZE);

  Status = HashTestContext->HashInit (HashTestContext->HashCtx);
  UT_ASSERT_TRUE (Status);

  Status = HashTestContext->HashUpdate (HashTestContext->HashCtx, HashData, DataSize);
  UT_ASSERT_TRUE (Status);

  Status = HashTestContext->HashFinal (HashTestContext->HashCtx, Digest);
  UT_ASSERT_TRUE (Status);

  UT_ASSERT_MEM_EQUAL (Digest, HashTestContext->Digest, HashTestContext->DigestSize);

  ZeroMem (Digest, MAX_DIGEST_SIZE);
  Status = HashTestContext->HashAll (HashData, DataSize, Digest);
  UT_ASSERT_TRUE (Status);

  UT_ASSERT_MEM_EQUAL (Digest, HashTestContext->Digest, HashTestContext->DigestSize);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mHashTest[] = {
  //
  // -----Description----------------Class---------------------Function---------------Pre------------------Post------------Context
  //
 #ifdef ENABLE_MD5_DEPRECATED_INTERFACES
  { "TestVerifyMd5()",    "CryptoPkg.BaseCryptLib.Hash", TestVerifyHash, TestVerifyHashPreReq, TestVerifyHashCleanUp, &mMd5TestCtx    },
 #endif
  { "TestVerifySha1()",   "CryptoPkg.BaseCryptLib.Hash", TestVerifyHash, TestVerifyHashPreReq, TestVerifyHashCleanUp, &mSha1TestCtx   },
  { "TestVerifySha256()", "CryptoPkg.BaseCryptLib.Hash", TestVerifyHash, TestVerifyHashPreReq, TestVerifyHashCleanUp, &mSha256TestCtx },
  { "TestVerifySha384()", "CryptoPkg.BaseCryptLib.Hash", TestVerifyHash, TestVerifyHashPreReq, TestVerifyHashCleanUp, &mSha384TestCtx },
  { "TestVerifySha512()", "CryptoPkg.BaseCryptLib.Hash", TestVerifyHash, TestVerifyHashPreReq, TestVerifyHashCleanUp, &mSha512TestCtx },
};

UINTN  mHashTestNum = ARRAY_SIZE (mHashTest);

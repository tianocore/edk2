/** @file
  Application for Block Cipher Primitives Validation.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

//
// TDES test vectors are extracted from OpenSSL 0.9.8l, crypto\des\destest.c
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  TdesEcbData[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  TdesEcbKey[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  TdesEcbCipher[] = {
  0x8C, 0xA6, 0x4D, 0xE9, 0xC1, 0xB1, 0x23, 0xA7,
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  TdesEcb2Cipher[] = {
  0x92, 0x95, 0xB5, 0x9B, 0xB3, 0x84, 0x73, 0x6E,
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  TdesCbcData[] = {
  0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x20,
  0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74,
  0x68, 0x65, 0x20, 0x74, 0x69, 0x6D, 0x65, 0x20
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  TdesCbcKey[] = {
  0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
  0xf1, 0xe0, 0xd3, 0xc2, 0xb5, 0xa4, 0x97, 0x86,
  0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
};

GLOBAL_REMOVE_IF_UNREFERENCED UINT8  TdesCbcIvec[] = {
  0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  TdesCbc3Cipher[] = {
  0x3F, 0xE3, 0x01, 0xC9, 0x62, 0xAC, 0x01, 0xD0,
  0x22, 0x13, 0x76, 0x3C, 0x1C, 0xBD, 0x4C, 0xDC,
  0x79, 0x96, 0x57, 0xC0, 0x64, 0xEC, 0xF5, 0xD4
};

//
// AES test vectors are from NIST KAT of AES
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes128EcbData[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes128EcbKey[] = {
  0x10, 0xa5, 0x88, 0x69, 0xd7, 0x4b, 0xe5, 0xa3, 0x74, 0xcf, 0x86, 0x7c, 0xfb, 0x47, 0x38, 0x59
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes128EcbCipher[] = {
  0x6d, 0x25, 0x1e, 0x69, 0x44, 0xb0, 0x51, 0xe0, 0x4e, 0xaa, 0x6f, 0xb4, 0xdb, 0xf7, 0x84, 0x65
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes192EcbData[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes192EcbKey[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes192EcbCipher[] = {
  0xdd, 0x8a, 0x49, 0x35, 0x14, 0x23, 0x1c, 0xbf, 0x56, 0xec, 0xce, 0xe4, 0xc4, 0x08, 0x89, 0xfb
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes256EcbData[] = {
  0x01, 0x47, 0x30, 0xf8, 0x0a, 0xc6, 0x25, 0xfe, 0x84, 0xf0, 0x26, 0xc6, 0x0b, 0xfd, 0x54, 0x7d
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes256EcbKey[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes256EcbCipher[] = {
  0x5c, 0x9d, 0x84, 0x4e, 0xd4, 0x6f, 0x98, 0x85, 0x08, 0x5e, 0x5d, 0x6a, 0x4f, 0x94, 0xc7, 0xd7
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes128CbcData[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes128CbcKey[] = {
  0xc2, 0x86, 0x69, 0x6d, 0x88, 0x7c, 0x9a, 0xa0, 0x61, 0x1b, 0xbb, 0x3e, 0x20, 0x25, 0xa4, 0x5a
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes128CbcIvec[] = {
  0x56, 0x2e, 0x17, 0x99, 0x6d, 0x09, 0x3d, 0x28, 0xdd, 0xb3, 0xba, 0x69, 0x5a, 0x2e, 0x6f, 0x58
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Aes128CbcCipher[] = {
  0xd2, 0x96, 0xcd, 0x94, 0xc2, 0xcc, 0xcf, 0x8a, 0x3a, 0x86, 0x30, 0x28, 0xb5, 0xe1, 0xdc, 0x0a,
  0x75, 0x86, 0x60, 0x2d, 0x25, 0x3c, 0xff, 0xf9, 0x1b, 0x82, 0x66, 0xbe, 0xa6, 0xd6, 0x1a, 0xb1
};

//
// ARC4 Test Vector defined in "Appendix A.1 Test Vectors from [CRYPTLIB]" of
// IETF Draft draft-kaukonen-cipher-arcfour-03 ("A Stream Cipher Encryption Algorithm 'Arcfour'").
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Arc4Data[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Arc4Key[] = {
  0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  Arc4Cipher[] = {
  0x74, 0x94, 0xC2, 0xE7, 0x10, 0x4B, 0x08, 0x79
};

typedef
UINTN
(EFIAPI *EFI_BLOCK_CIPHER_GET_CONTEXT_SIZE)(
  VOID
  );

typedef
BOOLEAN
(EFIAPI *EFI_BLOCK_CIPHER_INIT)(
  OUT  VOID         *BlockCipherContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  );

typedef
BOOLEAN
(EFIAPI *EFI_BLOCK_CIPHER_ECB_ENCRYPT_DECRYPT)(
  IN   VOID         *BlockCipherContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  );

typedef
BOOLEAN
(EFIAPI *EFI_BLOCK_CIPHER_CBC_ENCRYPT_DECRYPT)(
  IN   VOID         *BlockCipherContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  );

typedef
BOOLEAN
(EFIAPI *EFI_BLOCK_CIPHER_RESET)(
  IN OUT  VOID  *BlockCipherContext
  );

typedef struct {
  EFI_BLOCK_CIPHER_GET_CONTEXT_SIZE       GetContextSize;
  EFI_BLOCK_CIPHER_INIT                   Init;
  EFI_BLOCK_CIPHER_ECB_ENCRYPT_DECRYPT    EcbEncrypt;
  EFI_BLOCK_CIPHER_ECB_ENCRYPT_DECRYPT    EcbDecrypt;
  EFI_BLOCK_CIPHER_CBC_ENCRYPT_DECRYPT    CbcEncrypt;
  EFI_BLOCK_CIPHER_CBC_ENCRYPT_DECRYPT    CbcDecrypt;
  EFI_BLOCK_CIPHER_RESET                  Reset;
  CONST UINT8                             *Key;
  UINTN                                   KeySize;
  CONST UINT8                             *Ivec;
  CONST UINT8                             *Data;
  UINTN                                   DataSize;
  CONST UINT8                             *Cipher;
  UINTN                                   CipherSize;
  VOID                                    *Ctx;
} BLOCK_CIPHER_TEST_CONTEXT;

// These are commented out as they are deprecated, but are left in should they be used again
// BLOCK_CIPHER_TEST_CONTEXT mTdesEcbTestCtx   = {TdesGetContextSize, TdesInit, TdesEcbEncrypt, TdesEcbDecrypt, NULL,           NULL,           NULL,      TdesEcbKey,   64,              NULL,          TdesEcbData,   8,                     TdesEcbCipher,   8};
// BLOCK_CIPHER_TEST_CONTEXT mTdesCbcTestCtx   = {TdesGetContextSize, TdesInit, NULL,           NULL,           TdesCbcEncrypt, TdesCbcDecrypt, NULL,      TdesCbcKey,   192,             TdesCbcIvec,   TdesCbcData,   sizeof(TdesCbcData),   TdesCbc3Cipher,  sizeof(TdesCbc3Cipher)};
// BLOCK_CIPHER_TEST_CONTEXT mAes128EcbTestCtx = {AesGetContextSize,  AesInit,  AesEcbEncrypt,  AesEcbDecrypt,  NULL,           NULL,           NULL,      Aes128EcbKey, 128,             NULL,          Aes128EcbData, sizeof(Aes128EcbData), Aes128EcbCipher, sizeof(Aes128EcbCipher)};
// BLOCK_CIPHER_TEST_CONTEXT mAes192EcbTestCtx = {AesGetContextSize,  AesInit,  AesEcbEncrypt,  AesEcbDecrypt,  NULL,           NULL,           NULL,      Aes192EcbKey, 192,             NULL,          Aes192EcbData, sizeof(Aes192EcbData), Aes192EcbCipher, sizeof(Aes192EcbCipher)};
// BLOCK_CIPHER_TEST_CONTEXT mAes256EcbTestCtx = {AesGetContextSize,  AesInit,  AesEcbEncrypt,  AesEcbDecrypt,  NULL,           NULL,           NULL,      Aes256EcbKey, 256,             NULL,          Aes256EcbData, sizeof(Aes256EcbData), Aes256EcbCipher, sizeof(Aes256EcbCipher)};
// BLOCK_CIPHER_TEST_CONTEXT mArc4TestCtx      = {Arc4GetContextSize, Arc4Init, Arc4Encrypt,    (EFI_BLOCK_CIPHER_ECB_ENCRYPT_DECRYPT), Arc4Decrypt,    NULL,           NULL,           Arc4Reset, Arc4Key,      sizeof(Arc4Key), NULL,          Arc4Data,      sizeof(Arc4Data),      Arc4Cipher,      sizeof(Arc4Cipher)};
BLOCK_CIPHER_TEST_CONTEXT  mAes128CbcTestCtx = { AesGetContextSize, AesInit, NULL, NULL, AesCbcEncrypt, AesCbcDecrypt, NULL, Aes128CbcKey, 128, Aes128CbcIvec, Aes128CbcData, sizeof (Aes128CbcData), Aes128CbcCipher, sizeof (Aes128CbcCipher) };

UNIT_TEST_STATUS
EFIAPI
TestVerifyBLockCiperPreReq (
  UNIT_TEST_CONTEXT  Context
  )
{
  BLOCK_CIPHER_TEST_CONTEXT  *TestContext;
  UINTN                      CtxSize;

  TestContext      = Context;
  CtxSize          = TestContext->GetContextSize ();
  TestContext->Ctx = AllocatePool (CtxSize);
  if (TestContext->Ctx == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

VOID
EFIAPI
TestVerifyBLockCiperCleanUp (
  UNIT_TEST_CONTEXT  Context
  )
{
  BLOCK_CIPHER_TEST_CONTEXT  *TestContext;

  TestContext = Context;
  if (TestContext->Ctx != NULL) {
    FreePool (TestContext->Ctx);
  }
}

UNIT_TEST_STATUS
EFIAPI
TestVerifyBLockCiper (
  UNIT_TEST_CONTEXT  Context
  )
{
  UINT8                      Encrypt[256];
  UINT8                      Decrypt[256];
  BOOLEAN                    Status;
  BLOCK_CIPHER_TEST_CONTEXT  *TestContext;

  TestContext = Context;

  ZeroMem (Encrypt, sizeof (Encrypt));
  ZeroMem (Decrypt, sizeof (Decrypt));

  Status = TestContext->Init (TestContext->Ctx, TestContext->Key, TestContext->KeySize);
  UT_ASSERT_TRUE (Status);

  if (TestContext->Ivec == NULL) {
    Status = TestContext->EcbEncrypt (TestContext->Ctx, TestContext->Data, TestContext->DataSize, Encrypt);
    UT_ASSERT_TRUE (Status);

    if (TestContext->Reset != NULL) {
      Status = TestContext->Reset (TestContext->Ctx);
      UT_ASSERT_TRUE (Status);
    }

    Status = TestContext->EcbDecrypt (TestContext->Ctx, Encrypt, TestContext->DataSize, Decrypt);
    UT_ASSERT_TRUE (Status);
  } else {
    Status = TestContext->CbcEncrypt (TestContext->Ctx, TestContext->Data, TestContext->DataSize, TestContext->Ivec, Encrypt);
    UT_ASSERT_TRUE (Status);

    if (TestContext->Reset != NULL) {
      Status = TestContext->Reset (TestContext->Ctx);
      UT_ASSERT_TRUE (Status);
    }

    Status = TestContext->CbcDecrypt (TestContext->Ctx, Encrypt, TestContext->DataSize, TestContext->Ivec, Decrypt);
    UT_ASSERT_TRUE (Status);
  }

  UT_ASSERT_MEM_EQUAL (Encrypt, TestContext->Cipher, TestContext->CipherSize);
  UT_ASSERT_MEM_EQUAL (Decrypt, TestContext->Data, TestContext->DataSize);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mBlockCipherTest[] = {
  //
  // -----Description-------------------------Class-------------------------Function---------------Pre---------------------------Post------------------Context
  //
  { "TestVerifyAes128Cbc()", "CryptoPkg.BaseCryptLib.BlockCipher", TestVerifyBLockCiper, TestVerifyBLockCiperPreReq, TestVerifyBLockCiperCleanUp, &mAes128CbcTestCtx },
  // These are commented out as these functions have been deprecated, but they have been left in for future reference
  // {"TestVerifyTdesEcb()",    "CryptoPkg.BaseCryptLib.BlockCipher",   TestVerifyBLockCiper, TestVerifyBLockCiperPreReq, TestVerifyBLockCiperCleanUp, &mTdesEcbTestCtx},
  // {"TestVerifyTdesCbc()",    "CryptoPkg.BaseCryptLib.BlockCipher",   TestVerifyBLockCiper, TestVerifyBLockCiperPreReq, TestVerifyBLockCiperCleanUp, &mTdesCbcTestCtx},
  // {"TestVerifyAes128Ecb()",  "CryptoPkg.BaseCryptLib.BlockCipher",   TestVerifyBLockCiper, TestVerifyBLockCiperPreReq, TestVerifyBLockCiperCleanUp, &mAes128EcbTestCtx},
  // {"TestVerifyAes192Ecb()",  "CryptoPkg.BaseCryptLib.BlockCipher",   TestVerifyBLockCiper, TestVerifyBLockCiperPreReq, TestVerifyBLockCiperCleanUp, &mAes192EcbTestCtx},
  // {"TestVerifyAes256Ecb()",  "CryptoPkg.BaseCryptLib.BlockCipher",   TestVerifyBLockCiper, TestVerifyBLockCiperPreReq, TestVerifyBLockCiperCleanUp, &mAes256EcbTestCtx},
  // {"TestVerifyArc4()",       "CryptoPkg.BaseCryptLib.BlockCipher",   TestVerifyBLockCiper, TestVerifyBLockCiperPreReq, TestVerifyBLockCiperCleanUp, &mArc4TestCtx},
};

UINTN  mBlockCipherTestNum = ARRAY_SIZE (mBlockCipherTest);

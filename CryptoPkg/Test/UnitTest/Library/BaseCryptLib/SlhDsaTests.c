/** @file
  Application for SLH DSA Primitives Validation.

Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"
#include "SlhDsaTestsSignatures.h"

#define DEBUG_PRINT_SLH_DSA 0

STATIC VOID TestPrintBuffer (UINT8 *Buf, UINTN BufSize, CONST UINT8 *Label OPTIONAL) {
#if DEBUG_PRINT_SLH_DSA
  if (Label != NULL) {
    DEBUG ((DEBUG_INFO, "%a - Size: %lu\n", Label, BufSize));
  } else {
    DEBUG ((DEBUG_INFO, "Buffer Size: %lu\n", BufSize));
  }
  for (UINT32 Index = 0; Index < BufSize; Index++) {
      DEBUG ((DEBUG_INFO, "0x%2x ", Buf[Index]));
      if ((Index + 1) % 16 == 0) {
        DEBUG ((DEBUG_INFO, "\n"));
      }
  }
#endif
}

VOID *mSlhDsa;

UNIT_TEST_STATUS
EFIAPI
TestVerifySlhPreReq (
  UNIT_TEST_CONTEXT  Context
  )
{
  SlhDsaTestSignatures *SlhCtx;

  SlhCtx = (SlhDsaTestSignatures *)Context;
  mSlhDsa = SlhDsaNewByNid (SlhCtx->Nid);
  if (mSlhDsa == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

VOID
EFIAPI
TestVerifySlhCleanUp (
  UNIT_TEST_CONTEXT  Context
  )
{
  if (mSlhDsa != NULL) {
    SlhDsaFree (mSlhDsa);
    mSlhDsa = NULL;
  }
}

UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaVerify (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN              Status;
  SlhDsaTestSignatures *SlhCtx;
  UINT8                PubKey[256];
  UINTN                PubKeySize;

  SlhCtx = (SlhDsaTestSignatures *)Context;

  Status = SlhDsaSetPubKey (mSlhDsa, (UINT8 *)SlhCtx->PublicKey, &SlhCtx->PublicKeyLen);
  UT_ASSERT_TRUE (Status);
  TestPrintBuffer ((UINT8 *)SlhCtx->PublicKey, SlhCtx->PublicKeyLen, "Expected Public Key");
  PubKeySize = sizeof (PubKey);
  Status = SlhDsaGetPubKey (mSlhDsa, PubKey, &PubKeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (SlhCtx->PublicKeyLen, PubKeySize);
  TestPrintBuffer (PubKey, PubKeySize, "Retrieved Public Key");
  UT_ASSERT_MEM_EQUAL (SlhCtx->PublicKey, PubKey, PubKeySize);

  Status = SlhDsaVerify (mSlhDsa,
                        (UINT8 *)SlhCtx->Context, SlhCtx->ContextLen,
                        (UINT8 *)SlhCtx->Msg, SlhCtx->MsgLen,
                        (UINT8 *)SlhCtx->Sig, SlhCtx->SigLen);
  UT_ASSERT_TRUE (Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaSignVerify (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN              Status;
  SlhDsaTestSignatures *SlhCtx;
  UINT8                Key[256];
  UINTN                KeySize;
  UINT8                Sig[49856];
  UINTN                SigSize;

  SlhCtx = (SlhDsaTestSignatures *)Context;

  Status = SlhDsaSetPrivKey (mSlhDsa, (UINT8 *)SlhCtx->PrivateKey, &SlhCtx->PrivateKeyLen);
  UT_ASSERT_TRUE (Status);
  TestPrintBuffer ((UINT8 *)SlhCtx->PrivateKey, SlhCtx->PrivateKeyLen, "Expected Private Key");
  KeySize = sizeof (Key);
  Status = SlhDsaGetPrivKey (mSlhDsa, Key, &KeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (SlhCtx->PrivateKeyLen, KeySize);
  TestPrintBuffer (Key, KeySize, "Retrieved Private Key");
  UT_ASSERT_MEM_EQUAL (SlhCtx->PrivateKey, Key, KeySize);

  SigSize = sizeof (Sig);
  Status = SlhDsaSign (mSlhDsa,
                      (UINT8 *)SlhCtx->Context, SlhCtx->ContextLen,
                      (UINT8 *)SlhCtx->Msg, SlhCtx->MsgLen,
                      Sig, &SigSize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (SlhCtx->SigLen, SigSize);
  TestPrintBuffer (Sig, SigSize, "Generated Signature");

  Status = SlhDsaSetPubKey (mSlhDsa, (UINT8 *)SlhCtx->PublicKey, &SlhCtx->PublicKeyLen);
  UT_ASSERT_TRUE (Status);
  TestPrintBuffer ((UINT8 *)SlhCtx->PublicKey, SlhCtx->PublicKeyLen, "Expected Public Key");
  KeySize = sizeof (Key);
  Status = SlhDsaGetPubKey (mSlhDsa, Key, &KeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (SlhCtx->PublicKeyLen, KeySize);
  TestPrintBuffer (Key, KeySize, "Retrieved Public Key");
  UT_ASSERT_MEM_EQUAL (SlhCtx->PublicKey, Key, KeySize);

  Status = SlhDsaVerify (mSlhDsa,
                        (UINT8 *)SlhCtx->Context, SlhCtx->ContextLen,
                        (UINT8 *)SlhCtx->Msg, SlhCtx->MsgLen,
                        Sig, SigSize);
  UT_ASSERT_TRUE (Status);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mSlhDsaTest[] = {
  //
  // -----Description--------------------------------------Class----------------------Function---------------------------------Pre---------------------Post---------Context
  //
  // { "TestVerifySlhDsaVerify()",     "CryptoPkg.BaseCryptLib.Slh", TestVerifySlhDsaVerify,     TestVerifySlhPreReq, TestVerifySlhCleanUp, &mSlhDsaSha2128sVerifyCtx },
  { "TestVerifySlhDsaVerify()",     "CryptoPkg.BaseCryptLib.Slh", TestVerifySlhDsaVerify,     TestVerifySlhPreReq, TestVerifySlhCleanUp, &mSlhDsaSha2192fVerifyCtx },
  // { "TestVerifySlhDsaVerify()",     "CryptoPkg.BaseCryptLib.Slh", TestVerifySlhDsaVerify,     TestVerifySlhPreReq, TestVerifySlhCleanUp, &mSlhDsaSha2256sVerifyCtx },
  // { "TestVerifySlhDsaSignVerify()", "CryptoPkg.BaseCryptLib.Slh", TestVerifySlhDsaSignVerify, TestVerifySlhPreReq, TestVerifySlhCleanUp, &mSlhDsaSha2128sVerifyCtx },
};

UINTN  mSlhDsaTestNum = ARRAY_SIZE (mSlhDsaTest);

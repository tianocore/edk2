/** @file
  Application for ML DSA Primitives Validation.

Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"
#include "MlDsaTestsSignatures.h"

#define DEBUG_PRINT_ML_DSA 0

STATIC VOID TestPrintBuffer (UINT8 *Buf, UINTN BufSize, CONST UINT8 *Label OPTIONAL) {
#if DEBUG_PRINT_ML_DSA
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

VOID *mMlDsa;

UNIT_TEST_STATUS
EFIAPI
TestVerifyMlPreReq (
  UNIT_TEST_CONTEXT  Context
  )
{
  MlDsaTestSignatures *MlCtx;

  MlCtx = (MlDsaTestSignatures *)Context;
  mMlDsa = MlDsaNewByNid (MlCtx->Nid);
  if (mMlDsa == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

VOID
EFIAPI
TestVerifyMlCleanUp (
  UNIT_TEST_CONTEXT  Context
  )
{
  if (mMlDsa != NULL) {
    MlDsaFree (mMlDsa);
    mMlDsa = NULL;
  }
}

UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaVerify (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN              Status;
  MlDsaTestSignatures *MlCtx;
  UINT8                PubKey[2592];
  UINTN                PubKeySize;

  MlCtx = (MlDsaTestSignatures *)Context;

  Status = MlDsaSetPubKey (mMlDsa, (UINT8 *)MlCtx->PublicKey, &MlCtx->PublicKeyLen);
  UT_ASSERT_TRUE (Status);
  TestPrintBuffer ((UINT8 *)MlCtx->PublicKey, MlCtx->PublicKeyLen, "Expected Public Key");
  PubKeySize = sizeof (PubKey);
  Status = MlDsaGetPubKey (mMlDsa, PubKey, &PubKeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (MlCtx->PublicKeyLen, PubKeySize);
  TestPrintBuffer (PubKey, PubKeySize, "Retrieved Public Key");
  UT_ASSERT_MEM_EQUAL (MlCtx->PublicKey, PubKey, PubKeySize);

  Status = MlDsaVerify (mMlDsa,
                        NULL, 0,
                        (UINT8 *)MlCtx->Msg, MlCtx->MsgLen,
                        (UINT8 *)MlCtx->Sig, MlCtx->SigLen);
  UT_ASSERT_TRUE (Status);

  return UNIT_TEST_PASSED;
}

// UNIT_TEST_STATUS
// EFIAPI
// TestVerifyMlDsaSignVerify (
//   IN UNIT_TEST_CONTEXT  Context
//   )
// {
//   BOOLEAN              Status;
//   MlDsaTestSignatures *MlCtx;
//   UINT8                Key[256];
//   UINTN                KeySize;
//   UINT8                Sig[49856];
//   UINTN                SigSize;

//   MlCtx = (MlDsaTestSignatures *)Context;

//   Status = MlDsaSetPrivKey (mMlDsa, (UINT8 *)MlCtx->PrivateKey, &MlCtx->PrivateKeyLen);
//   UT_ASSERT_TRUE (Status);
//   TestPrintBuffer ((UINT8 *)MlCtx->PrivateKey, MlCtx->PrivateKeyLen, "Expected Private Key");
//   KeySize = sizeof (Key);
//   Status = MlDsaGetPrivKey (mMlDsa, Key, &KeySize);
//   UT_ASSERT_TRUE (Status);
//   UT_ASSERT_EQUAL (MlCtx->PrivateKeyLen, KeySize);
//   TestPrintBuffer (Key, KeySize, "Retrieved Private Key");
//   UT_ASSERT_MEM_EQUAL (MlCtx->PrivateKey, Key, KeySize);

//   SigSize = sizeof (Sig);
//   Status = MlDsaSign (mMlDsa,
//                       (UINT8 *)MlCtx->Context, MlCtx->ContextLen,
//                       (UINT8 *)MlCtx->Msg, MlCtx->MsgLen,
//                       Sig, &SigSize);
//   UT_ASSERT_TRUE (Status);
//   UT_ASSERT_EQUAL (MlCtx->SigLen, SigSize);
//   TestPrintBuffer (Sig, SigSize, "Generated Signature");

//   Status = MlDsaSetPubKey (mMlDsa, (UINT8 *)MlCtx->PublicKey, &MlCtx->PublicKeyLen);
//   UT_ASSERT_TRUE (Status);
//   TestPrintBuffer ((UINT8 *)MlCtx->PublicKey, MlCtx->PublicKeyLen, "Expected Public Key");
//   KeySize = sizeof (Key);
//   Status = MlDsaGetPubKey (mMlDsa, Key, &KeySize);
//   UT_ASSERT_TRUE (Status);
//   UT_ASSERT_EQUAL (MlCtx->PublicKeyLen, KeySize);
//   TestPrintBuffer (Key, KeySize, "Retrieved Public Key");
//   UT_ASSERT_MEM_EQUAL (MlCtx->PublicKey, Key, KeySize);

//   Status = MlDsaVerify (mMlDsa,
//                         (UINT8 *)MlCtx->Context, MlCtx->ContextLen,
//                         (UINT8 *)MlCtx->Msg, MlCtx->MsgLen,
//                         Sig, SigSize);
//   UT_ASSERT_TRUE (Status);

//   return UNIT_TEST_PASSED;
// }

TEST_DESC  mMlDsaTest[] = {
  //
  // -----Description--------------------------------------Class----------------------Function---------------------------------Pre---------------------Post---------Context
  //
  // { "TestVerifyMlDsaVerify()",     "CryptoPkg.BaseCryptLib.Ml", TestVerifyMlDsaVerify,     TestVerifyMlPreReq, TestVerifyMlCleanUp, &mMlDsaSha2128sVerifyCtx },
  { "TestVerifyMlDsaVerify()",     "CryptoPkg.BaseCryptLib.Ml", TestVerifyMlDsaVerify,     TestVerifyMlPreReq, TestVerifyMlCleanUp, &mMlDsa44VerifyCtx },
  // { "TestVerifyMlDsaVerify()",     "CryptoPkg.BaseCryptLib.Ml", TestVerifyMlDsaVerify,     TestVerifyMlPreReq, TestVerifyMlCleanUp, &mMlDsaSha2256sVerifyCtx },
  // { "TestVerifyMlDsaSignVerify()", "CryptoPkg.BaseCryptLib.Ml", TestVerifyMlDsaSignVerify, TestVerifyMlPreReq, TestVerifyMlCleanUp, &mMlDsaSha2128sVerifyCtx },
};

UINTN  mMlDsaTestNum = ARRAY_SIZE (mMlDsaTest);

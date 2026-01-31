/** @file
  Application for Ed-DSA Validation.

Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"
#include <openssl/evp.h>

EVP_PKEY      *PublicKeyObject;
EVP_PKEY_CTX  *pctx;
EVP_MD_CTX    *mctx;
UINTN         CoordLen;

GLOBAL_REMOVE_IF_UNREFERENCED UINT8  EdDsa448PublicKey[] = {
  0x03, 0x3A, 0x00, 0x39, 0x25, 0x00, 0x9C, 0xFD, 0x56, 0x2B, 0xEE, 0xB3, 0xC1, 0xFF, 0x5B, 0xF1, 0x16, 0xC4, 0x09, 0x22,
  0xC7, 0x40, 0x18, 0xB8, 0x7A, 0x43, 0x7C, 0x70, 0x06, 0xDD, 0x9D, 0x2B, 0xD7, 0x9C, 0x60, 0x55, 0x1E, 0x27, 0x17, 0xDC,
  0xB5, 0xEA, 0xEE, 0x1D, 0x36, 0xEF, 0xCE, 0x49, 0x20, 0xF6, 0xB1, 0x95, 0x7E, 0xA6, 0x7C, 0xBF, 0x14, 0xE0
};

GLOBAL_REMOVE_IF_UNREFERENCED UINT8  EdDsa448Signature[] = {
  0xDE, 0x27, 0x3E, 0x38, 0xEE, 0x16, 0xE9, 0xD8, 0xBA, 0x5C, 0x03, 0x98, 0xE2, 0x83, 0x2E, 0x65, 0xA8, 0x2A, 0x38, 0x0F,
  0x0D, 0x5D, 0xB0, 0x18, 0x86, 0x00, 0xC5, 0x09, 0xEA, 0x1D, 0x68, 0x6B, 0x71, 0xC5, 0xED, 0x63, 0xC1, 0x5C, 0x89, 0xE9,
  0x85, 0xB0, 0xFD, 0x62, 0x63, 0x8B, 0x70, 0xFF, 0x6C, 0xA7, 0xFC, 0xE2, 0x95, 0x1C, 0xBD, 0xEB, 0x80, 0x3F, 0xD2, 0x09,
  0x80, 0x03, 0x2D, 0x78, 0x0A, 0x3E, 0x9A, 0xAF, 0x68, 0xDC, 0x65, 0xA1, 0xB1, 0x46, 0x21, 0x30, 0xF8, 0x4A, 0x16, 0x7C,
  0xAB, 0x79, 0x9C, 0x9E, 0x65, 0xB9, 0xB8, 0xD9, 0x02, 0x57, 0x9E, 0x7A, 0x08, 0xA2, 0x05, 0xA8, 0x08, 0x08, 0xB9, 0x09,
  0x67, 0xCE, 0xA7, 0x87, 0x2D, 0x23, 0xC3, 0x23, 0xBE, 0xFD, 0x78, 0x38, 0x34, 0x00
};

UINT8  EdDsa448Data[] = "This is a message";

UNIT_TEST_STATUS
EFIAPI
TestVerifyEdPreReq (
  UNIT_TEST_CONTEXT  Context
  )
{
  pctx            = NULL;
  mctx            = NULL;
  PublicKeyObject = NULL;
  CoordLen        = 57;

  if ((EdDsa448PublicKey == NULL) || (EdDsa448Signature == NULL)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestVerifyEdPublicKey (
  UNIT_TEST_CONTEXT  Context
  )
{
  PublicKeyObject = EVP_PKEY_new_raw_public_key (CRYPTO_NID_ED448, NULL, EdDsa448PublicKey, CoordLen);
  if (PublicKeyObject == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestVerifyEdVerify (
  UNIT_TEST_CONTEXT  Context
  )
{
  PublicKeyObject = EVP_PKEY_new_raw_public_key (CRYPTO_NID_ED448, NULL, EdDsa448PublicKey, CoordLen);
  if (PublicKeyObject == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  mctx = EVP_MD_CTX_new ();
  if (!mctx) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  pctx = EVP_PKEY_CTX_new (PublicKeyObject, NULL);
  if (!pctx) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  } else {
    EVP_MD_CTX_set_pkey_ctx (mctx, pctx);
  }

  if (EVP_DigestVerifyInit (mctx, NULL, NULL, NULL, PublicKeyObject) != 1) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  if (EVP_DigestVerify (mctx, EdDsa448Signature, sizeof (EdDsa448Signature), EdDsa448Data, sizeof (EdDsa448Data)) != 1) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

VOID
EFIAPI
TestVerifyEdCleanUp (
  UNIT_TEST_CONTEXT  Context
  )
{
  if (PublicKeyObject != NULL) {
    EVP_PKEY_free (PublicKeyObject);
  }

  if (mctx != NULL) {
    EVP_MD_CTX_free (mctx);
  }

  if (pctx != NULL) {
    EVP_PKEY_CTX_free (pctx);
  }
}

TEST_DESC  mEdTest[] = {
  //
  // -----Description-----------------Class------------------Function----Pre----Post----Context
  //
  { "TestVerifyEdPublicKey()", "CryptoPkg.BaseCryptLib.Ed", TestVerifyEdPublicKey, TestVerifyEdPreReq, TestVerifyEdCleanUp, NULL },
  { "TestVerifyEdVerify()",    "CryptoPkg.BaseCryptLib.Ed", TestVerifyEdVerify,    TestVerifyEdPreReq, TestVerifyEdCleanUp, NULL },
};

UINTN  mEdTestNum = ARRAY_SIZE (mEdTest);

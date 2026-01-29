/** @file
  Application for Hkdf Primitives Validation.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

/**
 * HKDF KAT from RFC 5869 Appendix A. Test Vectors
 * https://www.rfc-editor.org/rfc/rfc5869.html
 **/
UINT8  mHkdfSha256Ikm[22] = {
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
  0x0b, 0x0b
};

UINT8  mHkdfSha256Salt[13] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
  0x0a, 0x0b, 0x0c,
};

UINT8  mHkdfSha256Info[10] = {
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
};

UINT8  mHkdfSha256Prk[32] = {
  0x07, 0x77, 0x09, 0x36, 0x2c, 0x2e, 0x32, 0xdf, 0x0d, 0xdc,
  0x3f, 0x0d, 0xc4, 0x7b, 0xba, 0x63, 0x90, 0xb6, 0xc7, 0x3b,
  0xb5, 0x0f, 0x9c, 0x31, 0x22, 0xec, 0x84, 0x4a, 0xd7, 0xc2,
  0xb3, 0xe5,
};

UINT8  mHkdfSha256Okm[42] = {
  0x3c, 0xb2, 0x5f, 0x25, 0xfa, 0xac, 0xd5, 0x7a, 0x90, 0x43,
  0x4f, 0x64, 0xd0, 0x36, 0x2f, 0x2a, 0x2d, 0x2d, 0x0a, 0x90,
  0xcf, 0x1a, 0x5a, 0x4c, 0x5d, 0xb0, 0x2d, 0x56, 0xec, 0xc4,
  0xc5, 0xbf, 0x34, 0x00, 0x72, 0x08, 0xd5, 0xb8, 0x87, 0x18,
  0x58, 0x65,
};

/**
 * This Hkdf-Sha384 test vector is form Project Wycheproof
 * developed and maintained by members of Google Security Team.
 * https://github.com/google/wycheproof/blob/master/testvectors/hkdf_sha384_test.json
 **/
UINT8  mHkdfSha384Ikm[16] = {
  0x86, 0x77, 0xdc, 0x79, 0x23, 0x3e, 0xf3, 0x48, 0x07, 0x77,
  0xc4, 0xc6, 0x01, 0xef, 0x4f, 0x0b,
};

UINT8  mHkdfSha384Salt[16] = {
  0xad, 0x88, 0xdb, 0x71, 0x82, 0x44, 0xe2, 0xcb, 0x60, 0xe3,
  0x5f, 0x87, 0x4d, 0x7a, 0xd8, 0x1f,
};

UINT8  mHkdfSha384Info[20] = {
  0xa3, 0x8f, 0x63, 0x4d, 0x94, 0x78, 0x19, 0xa9, 0xbf, 0xa7,
  0x92, 0x17, 0x4b, 0x42, 0xba, 0xa2, 0x0c, 0x9f, 0xce, 0x15,
};

UINT8  mHkdfSha384Prk[48] = {
  0x60, 0xae, 0xa0, 0xde, 0xca, 0x97, 0x62, 0xaa, 0x43, 0xaf,
  0x0e, 0x77, 0xa8, 0x0f, 0xb7, 0x76, 0xd0, 0x08, 0x19, 0x62,
  0xf8, 0x30, 0xb5, 0x0d, 0x92, 0x08, 0x92, 0x7a, 0x8a, 0xd5,
  0x6a, 0x3d, 0xc4, 0x4a, 0x5d, 0xfe, 0xb6, 0xb4, 0x79, 0x2f,
  0x97, 0x92, 0x71, 0xe6, 0xcb, 0x08, 0x86, 0x52,
};

UINT8  mHkdfSha384Okm[64] = {
  0x75, 0x85, 0x46, 0x36, 0x2a, 0x07, 0x0c, 0x0f, 0x13, 0xcb,
  0xfb, 0xf1, 0x75, 0x6e, 0x8f, 0x29, 0xb7, 0x81, 0x9f, 0xb9,
  0x03, 0xc7, 0xed, 0x4f, 0x97, 0xa5, 0x6b, 0xe3, 0xc8, 0xf8,
  0x1e, 0x8c, 0x37, 0xae, 0xf5, 0xc0, 0xf8, 0xe5, 0xd2, 0xb1,
  0x7e, 0xb1, 0xaa, 0x02, 0xec, 0x04, 0xc3, 0x3f, 0x54, 0x6c,
  0xb2, 0xf3, 0xd1, 0x93, 0xe9, 0x30, 0xa9, 0xf8, 0x9e, 0xc9,
  0xce, 0x3a, 0x82, 0xb5
};

UNIT_TEST_STATUS
EFIAPI
TestVerifyHkdfSha256 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    PrkOut[32];
  UINT8    Out[42];
  BOOLEAN  Status;

  /* HKDF-SHA-256 digest Validation*/

  ZeroMem (PrkOut, sizeof (PrkOut));
  Status = HkdfSha256Extract (
             mHkdfSha256Ikm,
             sizeof (mHkdfSha256Ikm),
             mHkdfSha256Salt,
             sizeof (mHkdfSha256Salt),
             PrkOut,
             sizeof (PrkOut)
             );
  UT_ASSERT_TRUE (Status);

  UT_ASSERT_MEM_EQUAL (PrkOut, mHkdfSha256Prk, sizeof (mHkdfSha256Prk));

  ZeroMem (Out, sizeof (Out));
  Status = HkdfSha256Expand (
             mHkdfSha256Prk,
             sizeof (mHkdfSha256Prk),
             mHkdfSha256Info,
             sizeof (mHkdfSha256Info),
             Out,
             sizeof (Out)
             );
  UT_ASSERT_TRUE (Status);

  UT_ASSERT_MEM_EQUAL (Out, mHkdfSha256Okm, sizeof (mHkdfSha256Okm));

  ZeroMem (Out, sizeof (Out));
  Status = HkdfSha256ExtractAndExpand (
             mHkdfSha256Ikm,
             sizeof (mHkdfSha256Ikm),
             mHkdfSha256Salt,
             sizeof (mHkdfSha256Salt),
             mHkdfSha256Info,
             sizeof (mHkdfSha256Info),
             Out,
             sizeof (Out)
             );
  UT_ASSERT_TRUE (Status);

  UT_ASSERT_MEM_EQUAL (Out, mHkdfSha256Okm, sizeof (mHkdfSha256Okm));

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestVerifyHkdfSha384 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    PrkOut[48];
  UINT8    Out[64];
  BOOLEAN  Status;

  /* HKDF-SHA-384 digest Validation*/
  ZeroMem (PrkOut, sizeof (PrkOut));
  Status = HkdfSha384Extract (
             mHkdfSha384Ikm,
             sizeof (mHkdfSha384Ikm),
             mHkdfSha384Salt,
             sizeof (mHkdfSha384Salt),
             PrkOut,
             sizeof (PrkOut)
             );
  UT_ASSERT_TRUE (Status);

  UT_ASSERT_MEM_EQUAL (PrkOut, mHkdfSha384Prk, sizeof (mHkdfSha384Prk));

  ZeroMem (Out, sizeof (Out));
  Status = HkdfSha384Expand (
             mHkdfSha384Prk,
             sizeof (mHkdfSha384Prk),
             mHkdfSha384Info,
             sizeof (mHkdfSha384Info),
             Out,
             sizeof (Out)
             );
  UT_ASSERT_TRUE (Status);

  UT_ASSERT_MEM_EQUAL (Out, mHkdfSha384Okm, sizeof (mHkdfSha384Okm));

  ZeroMem (Out, sizeof (Out));
  Status = HkdfSha384ExtractAndExpand (
             mHkdfSha384Ikm,
             sizeof (mHkdfSha384Ikm),
             mHkdfSha384Salt,
             sizeof (mHkdfSha384Salt),
             mHkdfSha384Info,
             sizeof (mHkdfSha384Info),
             Out,
             sizeof (Out)
             );
  UT_ASSERT_TRUE (Status);

  UT_ASSERT_MEM_EQUAL (Out, mHkdfSha384Okm, sizeof (mHkdfSha384Okm));

  return UNIT_TEST_PASSED;
}

TEST_DESC  mHkdfTest[] = {
  //
  // -----Description--------------------------------------Class----------------------Function---------------------------------Pre---------------------Post---------Context
  //
  { "TestVerifyHkdfSha256()", "CryptoPkg.BaseCryptLib.Hkdf", TestVerifyHkdfSha256, NULL, NULL, NULL },
  { "TestVerifyHkdfSha384()", "CryptoPkg.BaseCryptLib.Hkdf", TestVerifyHkdfSha384, NULL, NULL, NULL },
};

UINTN  mHkdfTestNum = ARRAY_SIZE (mHkdfTest);

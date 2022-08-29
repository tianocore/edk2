/** @file
  Application for Authenticated Encryption with Associated Data
  (AEAD) Validation.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

/* AES-GCM test data from NIST public test vectors */
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  gcm_key[] = {
  0xee, 0xbc, 0x1f, 0x57, 0x48, 0x7f, 0x51, 0x92, 0x1c, 0x04, 0x65, 0x66,
  0x5f, 0x8a, 0xe6, 0xd1, 0x65, 0x8b, 0xb2, 0x6d, 0xe6, 0xf8, 0xa0, 0x69,
  0xa3, 0x52, 0x02, 0x93, 0xa5, 0x72, 0x07, 0x8f
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  gcm_iv[] = {
  0x99, 0xaa, 0x3e, 0x68, 0xed, 0x81, 0x73, 0xa0, 0xee, 0xd0, 0x66, 0x84
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  gcm_pt[] = {
  0xf5, 0x6e, 0x87, 0x05, 0x5b, 0xc3, 0x2d, 0x0e, 0xeb, 0x31, 0xb2, 0xea,
  0xcc, 0x2b, 0xf2, 0xa5
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  gcm_aad[] = {
  0x4d, 0x23, 0xc3, 0xce, 0xc3, 0x34, 0xb4, 0x9b, 0xdb, 0x37, 0x0c, 0x43,
  0x7f, 0xec, 0x78, 0xde
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  gcm_ct[] = {
  0xf7, 0x26, 0x44, 0x13, 0xa8, 0x4c, 0x0e, 0x7c, 0xd5, 0x36, 0x86, 0x7e,
  0xb9, 0xf2, 0x17, 0x36
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  gcm_tag[] = {
  0x67, 0xba, 0x05, 0x10, 0x26, 0x2a, 0xe4, 0x87, 0xd7, 0x37, 0xee, 0x62,
  0x98, 0xf7, 0x7e, 0x0c
};

UNIT_TEST_STATUS
EFIAPI
TestVerifyAeadAesGcm (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    OutBuffer[1024];
  UINTN    OutBufferSize;
  UINT8    OutTag[1024];
  UINTN    OutTagSize;

  OutBufferSize = sizeof (OutBuffer);
  OutTagSize    = sizeof (gcm_tag);
  ZeroMem (OutBuffer, sizeof (OutBuffer));
  ZeroMem (OutTag, sizeof (OutTag));
  Status = AeadAesGcmEncrypt (
             gcm_key,
             sizeof (gcm_key),
             gcm_iv,
             sizeof (gcm_iv),
             gcm_aad,
             sizeof (gcm_aad),
             gcm_pt,
             sizeof (gcm_pt),
             OutTag,
             OutTagSize,
             OutBuffer,
             &OutBufferSize
             );
  UT_ASSERT_TRUE (Status);

  UT_ASSERT_EQUAL (OutBufferSize, sizeof (gcm_ct));

  UT_ASSERT_MEM_EQUAL (OutBuffer, gcm_ct, sizeof (gcm_ct));

  UT_ASSERT_MEM_EQUAL (OutTag, gcm_tag, sizeof (gcm_tag));

  ZeroMem (OutBuffer, sizeof (OutBuffer));
  Status = AeadAesGcmDecrypt (
             gcm_key,
             sizeof (gcm_key),
             gcm_iv,
             sizeof (gcm_iv),
             gcm_aad,
             sizeof (gcm_aad),
             gcm_ct,
             sizeof (gcm_ct),
             gcm_tag,
             sizeof (gcm_tag),
             OutBuffer,
             &OutBufferSize
             );
  UT_ASSERT_TRUE (Status);

  UT_ASSERT_EQUAL (OutBufferSize, sizeof (gcm_pt));

  UT_ASSERT_MEM_EQUAL (OutBuffer, gcm_pt, sizeof (gcm_pt));

  return UNIT_TEST_PASSED;
}

TEST_DESC  mAeadAesGcmTest[] = {
  //
  // -----Description--------------------------------------Class----------------------Function---------------------------------Pre---------------------Post---------Context
  //
  { "TestVerifyAeadAesGcm()", "CryptoPkg.BaseCryptLib.AeadAesGcm", TestVerifyAeadAesGcm, NULL, NULL, NULL },
};

UINTN  mAeadAesGcmTestNum = ARRAY_SIZE (mAeadAesGcmTest);

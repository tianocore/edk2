/** @file
  Application for Diffie-Hellman Primitives Validation.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

#define EC_CURVE_NUM_SUPPORTED  3
UINTN  EcCurveList[EC_CURVE_NUM_SUPPORTED]   = { CRYPTO_NID_SECP256R1, CRYPTO_NID_SECP384R1, CRYPTO_NID_SECP521R1 };
UINTN  EcKeyHalfSize[EC_CURVE_NUM_SUPPORTED] = { 32, 48, 66 };

struct Generator {
  UINT8    X[66];
  UINT8    Y[66];
};

// Generator points of all ec curve
struct Generator  EcCurveGenerator[EC_CURVE_NUM_SUPPORTED] =
{
  // CRYPTO_NID_SECP256R1
  {
    { 0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47, 0xF8, 0xBC, 0xE6, 0xE5,
      0x63, 0xA4, 0x40, 0xF2, 0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0,
      0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96 },

    { 0x4f, 0xe3, 0x42, 0xe2, 0xfe, 0x1a, 0x7f, 0x9b, 0x8e, 0xe7, 0xeb, 0x4a,
      0x7c, 0x0f, 0x9e, 0x16, 0x2b, 0xce, 0x33, 0x57, 0x6b, 0x31, 0x5e, 0xce,
      0xcb, 0xb6, 0x40, 0x68, 0x37, 0xbf, 0x51, 0xf5 }
  },
  // CRYPTO_NID_SECP384R1
  {
    { 0xAA, 0x87, 0xCA, 0x22, 0xBE, 0x8B, 0x05, 0x37, 0x8E, 0xB1, 0xC7, 0x1E,
      0xF3, 0x20, 0xAD, 0x74, 0x6E, 0x1D, 0x3B, 0x62, 0x8B, 0xA7, 0x9B, 0x98,
      0x59, 0xF7, 0x41, 0xE0, 0x82, 0x54, 0x2A, 0x38, 0x55, 0x02, 0xF2, 0x5D,
      0xBF, 0x55, 0x29, 0x6C, 0x3A, 0x54, 0x5E, 0x38, 0x72, 0x76, 0x0A, 0xB7 },

    { 0x36, 0x17, 0xde, 0x4a, 0x96, 0x26, 0x2c, 0x6f, 0x5d, 0x9e, 0x98, 0xbf,
      0x92, 0x92, 0xdc, 0x29, 0xf8, 0xf4, 0x1d, 0xbd, 0x28, 0x9a, 0x14, 0x7c,
      0xe9, 0xda, 0x31, 0x13, 0xb5, 0xf0, 0xb8, 0xc0, 0x0a, 0x60, 0xb1, 0xce,
      0x1d, 0x7e, 0x81, 0x9d, 0x7a, 0x43, 0x1d, 0x7c, 0x90, 0xea, 0x0e, 0x5f }
  },
  // CRYPTO_NID_SECP521R1
  {
    { 0x00, 0xC6, 0x85, 0x8E, 0x06, 0xB7, 0x04, 0x04, 0xE9, 0xCD, 0x9E, 0x3E,
      0xCB, 0x66, 0x23, 0x95, 0xB4, 0x42, 0x9C, 0x64, 0x81, 0x39, 0x05, 0x3F,
      0xB5, 0x21, 0xF8, 0x28, 0xAF, 0x60, 0x6B, 0x4D, 0x3D, 0xBA, 0xA1, 0x4B,
      0x5E, 0x77, 0xEF, 0xE7, 0x59, 0x28, 0xFE, 0x1D, 0xC1, 0x27, 0xA2, 0xFF,
      0xA8, 0xDE, 0x33, 0x48, 0xB3, 0xC1, 0x85, 0x6A, 0x42, 0x9B, 0xF9, 0x7E,
      0x7E, 0x31, 0xC2, 0xE5, 0xBD, 0x66 },

    { 0x01, 0x18, 0x39, 0x29, 0x6a, 0x78, 0x9a, 0x3b, 0xc0, 0x04, 0x5c, 0x8a,
      0x5f, 0xb4, 0x2c, 0x7d, 0x1b, 0xd9, 0x98, 0xf5, 0x44, 0x49, 0x57, 0x9b,
      0x44, 0x68, 0x17, 0xaf, 0xbd, 0x17, 0x27, 0x3e, 0x66, 0x2c, 0x97, 0xee,
      0x72, 0x99, 0x5e, 0xf4, 0x26, 0x40, 0xc5, 0x50, 0xb9, 0x01, 0x3f, 0xad,
      0x07, 0x61, 0x35, 0x3c, 0x70, 0x86, 0xa2, 0x72, 0xc2, 0x40, 0x88, 0xbe,
      0x94, 0x76, 0x9f, 0xd1, 0x66, 0x50 }
  }
};

VOID  *Ec1;
VOID  *Ec2;
VOID  *Group;
VOID  *Point1;
VOID  *Point2;
VOID  *PointRes;
VOID  *BnX;
VOID  *BnY;
VOID  *BnP;
VOID  *BnOrder;

UNIT_TEST_STATUS
EFIAPI
TestVerifyEcPreReq (
  UNIT_TEST_CONTEXT  Context
  )
{
  Ec1      = NULL;
  Ec2      = NULL;
  Group    = NULL;
  Point1   = NULL;
  Point2   = NULL;
  PointRes = NULL;
  BnX      = NULL;
  BnY      = NULL;
  BnP      = BigNumInit ();
  BnOrder  = BigNumInit ();
  if ((BnP == NULL) || (BnOrder == NULL)) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

VOID
EFIAPI
TestVerifyEcCleanUp (
  UNIT_TEST_CONTEXT  Context
  )
{
  BigNumFree (BnX, TRUE);
  BigNumFree (BnY, TRUE);
  BigNumFree (BnP, TRUE);
  BigNumFree (BnOrder, TRUE);
  EcGroupFree (Group);
  EcPointDeInit (Point1, TRUE);
  EcPointDeInit (Point2, TRUE);
  EcPointDeInit (PointRes, TRUE);
  EcFree (Ec1);
  EcFree (Ec2);
}

UNIT_TEST_STATUS
EFIAPI
TestVerifyEcBasic (
  UNIT_TEST_CONTEXT  Context
  )
{
  UINTN    CurveCount;
  BOOLEAN  Status;

  //
  // Initialize BigNumbers
  //
  for (CurveCount = 0; CurveCount < EC_CURVE_NUM_SUPPORTED; CurveCount++) {
    //
    // Basic EC functions unit test
    //
    Group = EcGroupInit (EcCurveList[CurveCount]);
    if (Group == NULL) {
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    Point1   = EcPointInit (Group);
    Point2   = EcPointInit (Group);
    PointRes = EcPointInit (Group);
    BnX      = BigNumFromBin (EcCurveGenerator[CurveCount].X, EcKeyHalfSize[CurveCount]);
    BnY      = BigNumFromBin (EcCurveGenerator[CurveCount].Y, EcKeyHalfSize[CurveCount]);
    if ((Point1 == NULL) || (Point2 == NULL) || (PointRes == NULL) || (BnX == NULL) || (BnY == NULL)) {
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    Status = EcGroupGetCurve (Group, BnP, NULL, NULL, NULL);
    UT_ASSERT_TRUE (Status);

    Status = EcGroupGetOrder (Group, BnOrder);
    UT_ASSERT_TRUE (Status);

    // Point G should on curve
    Status = EcPointSetAffineCoordinates (Group, Point1, BnX, BnY, NULL);
    UT_ASSERT_TRUE (Status);

    Status = EcPointSetAffineCoordinates (Group, Point2, BnX, BnY, NULL);
    UT_ASSERT_TRUE (Status);

    Status = EcPointEqual (Group, Point1, Point2, NULL);
    UT_ASSERT_TRUE (Status);

    Status = EcPointIsOnCurve (Group, Point1, NULL);
    UT_ASSERT_TRUE (Status);

    Status = EcPointIsAtInfinity (Group, Point1);
    UT_ASSERT_FALSE (Status);

    // Point 2G should on curve
    Status = EcPointAdd (Group, PointRes, Point1, Point1, NULL);
    UT_ASSERT_TRUE (Status);

    Status = EcPointIsOnCurve (Group, PointRes, NULL);
    UT_ASSERT_TRUE (Status);

    // Point Order * G should at infinity
    Status = EcPointMul (Group, PointRes, Point1, BnOrder, NULL);
    UT_ASSERT_TRUE (Status);

    Status = EcPointIsAtInfinity (Group, PointRes);
    UT_ASSERT_TRUE (Status);

    // -(-G) == G
    Status = EcPointInvert (Group, Point2, NULL);
    UT_ASSERT_TRUE (Status);

    Status = EcPointEqual (Group, Point2, Point1, NULL);
    UT_ASSERT_FALSE (Status);

    Status = EcPointInvert (Group, Point2, NULL);
    UT_ASSERT_TRUE (Status);

    Status = EcPointEqual (Group, Point2, Point1, NULL);
    UT_ASSERT_TRUE (Status);

    // Compress point test
    Status = EcPointSetCompressedCoordinates (Group, Point1, BnX, 0, NULL);
    UT_ASSERT_TRUE (Status);

    Status = EcPointSetCompressedCoordinates (Group, Point2, BnX, 1, NULL);
    UT_ASSERT_TRUE (Status);

    Status = EcPointEqual (Group, Point2, Point1, NULL);
    UT_ASSERT_FALSE (Status);

    Status = EcPointInvert (Group, Point2, NULL);
    UT_ASSERT_TRUE (Status);

    Status = EcPointEqual (Group, Point2, Point1, NULL);
    UT_ASSERT_TRUE (Status);
  }

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestVerifyEcDh (
  UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    Public1[66 * 2];
  UINTN    Public1Length;
  UINT8    Public2[66 * 2];
  UINTN    Public2Length;
  UINT8    Key1[66];
  UINTN    Key1Length;
  UINT8    Key2[66];
  UINTN    Key2Length;
  UINTN    CurveCount;
  BOOLEAN  Status;

  for (CurveCount = 0; CurveCount < EC_CURVE_NUM_SUPPORTED; CurveCount++) {
    //
    // Initial key length
    //
    Public1Length = sizeof (Public1);
    Public2Length = sizeof (Public2);
    Key1Length    = sizeof (Key1);
    Key2Length    = sizeof (Key2);
    //
    // ECDH functions unit test
    //
    Ec1 = EcNewByNid (EcCurveList[CurveCount]);
    if (Ec1 == NULL) {
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    Ec2 = EcNewByNid (EcCurveList[CurveCount]);
    if (Ec2 == NULL) {
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    Status = EcGenerateKey (Ec1, Public1, &Public1Length);
    UT_ASSERT_TRUE (Status);
    UT_ASSERT_EQUAL (Public1Length, EcKeyHalfSize[CurveCount] * 2);

    Status = EcGenerateKey (Ec2, Public2, &Public2Length);
    UT_ASSERT_TRUE (Status);
    UT_ASSERT_EQUAL (Public2Length, EcKeyHalfSize[CurveCount] * 2);

    Status = EcDhComputeKey (Ec1, Public2, Public2Length, NULL, Key1, &Key1Length);
    UT_ASSERT_TRUE (Status);
    UT_ASSERT_EQUAL (Key1Length, EcKeyHalfSize[CurveCount]);

    Status = EcDhComputeKey (Ec2, Public1, Public1Length, NULL, Key2, &Key2Length);
    UT_ASSERT_TRUE (Status);
    UT_ASSERT_EQUAL (Key2Length, EcKeyHalfSize[CurveCount]);

    UT_ASSERT_EQUAL (Key1Length, Key2Length);
    UT_ASSERT_MEM_EQUAL (Key1, Key2, Key1Length);

    Status = EcGetPubKey (Ec1, Public2, &Public2Length);
    UT_ASSERT_TRUE (Status);
    UT_ASSERT_EQUAL (Public2Length, EcKeyHalfSize[CurveCount] * 2);

    UT_ASSERT_EQUAL (Public1Length, Public2Length);
    UT_ASSERT_MEM_EQUAL (Public1, Public2, Public1Length);
  }

  return UNIT_TEST_PASSED;
}

TEST_DESC  mEcTest[] = {
  //
  // -----Description-----------------Class------------------Function----Pre----Post----Context
  //
  { "TestVerifyEcBasic()", "CryptoPkg.BaseCryptLib.Ec", TestVerifyEcBasic, TestVerifyEcPreReq, TestVerifyEcCleanUp, NULL },
  { "TestVerifyEcDh()",    "CryptoPkg.BaseCryptLib.Ec", TestVerifyEcDh,    TestVerifyEcPreReq, TestVerifyEcCleanUp, NULL },
};

UINTN  mEcTestNum = ARRAY_SIZE (mEcTest);

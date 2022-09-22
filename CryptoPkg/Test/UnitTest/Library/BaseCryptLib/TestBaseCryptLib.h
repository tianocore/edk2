/** @file
  Application for Cryptographic Primitives Validation.

Copyright (c) 2009 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CRYPTEST_H__
#define __CRYPTEST_H__

#include <PiPei.h>
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
// #include <UnitTestTypes.h>
#include <Library/UnitTestLib.h>
// #include <Library/UnitTestAssertLib.h>

#define UNIT_TEST_NAME     "BaseCryptLib Unit Test"
#define UNIT_TEST_VERSION  "1.0"

typedef struct {
  CHAR8                     *Description;
  CHAR8                     *ClassName;
  UNIT_TEST_FUNCTION        Func;
  UNIT_TEST_PREREQUISITE    PreReq;
  UNIT_TEST_CLEANUP         CleanUp;
  UNIT_TEST_CONTEXT         Context;
} TEST_DESC;

typedef struct {
  CHAR8                       *Title;
  CHAR8                       *Package;
  UNIT_TEST_SUITE_SETUP       Sup;
  UNIT_TEST_SUITE_TEARDOWN    Tdn;
  UINTN                       *TestNum;
  TEST_DESC                   *TestDesc;
} SUITE_DESC;

extern UINTN      mPkcs7EkuTestNum;
extern TEST_DESC  mPkcs7EkuTest[];

extern UINTN      mHashTestNum;
extern TEST_DESC  mHashTest[];

extern UINTN      mHmacTestNum;
extern TEST_DESC  mHmacTest[];

extern UINTN      mBlockCipherTestNum;
extern TEST_DESC  mBlockCipherTest[];

extern UINTN      mRsaTestNum;
extern TEST_DESC  mRsaTest[];

extern UINTN      mRsaCertTestNum;
extern TEST_DESC  mRsaCertTest[];

extern UINTN      mPkcs7TestNum;
extern TEST_DESC  mPkcs7Test[];

extern UINTN      mPkcs5TestNum;
extern TEST_DESC  mPkcs5Test[];

extern UINTN      mAuthenticodeTestNum;
extern TEST_DESC  mAuthenticodeTest[];

extern UINTN      mImageTimestampTestNum;
extern TEST_DESC  mImageTimestampTest[];

extern UINTN      mDhTestNum;
extern TEST_DESC  mDhTest[];

extern UINTN      mPrngTestNum;
extern TEST_DESC  mPrngTest[];

extern UINTN      mOaepTestNum;
extern TEST_DESC  mOaepTest[];

extern UINTN      mRsaPssTestNum;
extern TEST_DESC  mRsaPssTest[];

extern UINTN      mHkdfTestNum;
extern TEST_DESC  mHkdfTest[];

extern UINTN      mAeadAesGcmTestNum;
extern TEST_DESC  mAeadAesGcmTest[];

extern UINTN      mBnTestNum;
extern TEST_DESC  mBnTest[];

extern UINTN      mEcTestNum;
extern TEST_DESC  mEcTest[];

extern UINTN      mX509TestNum;
extern TEST_DESC  mX509Test[];

/** Creates a framework you can use */
EFI_STATUS
EFIAPI
CreateUnitTest (
  IN     CHAR8                       *UnitTestName,
  IN     CHAR8                       *UnitTestVersion,
  IN OUT UNIT_TEST_FRAMEWORK_HANDLE  *Framework
  );

/**
  Validate UEFI-OpenSSL DH Interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptDh (
  VOID
  );

/**
  Validate UEFI-OpenSSL pseudorandom number generator interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptPrng (
  VOID
  );

#endif

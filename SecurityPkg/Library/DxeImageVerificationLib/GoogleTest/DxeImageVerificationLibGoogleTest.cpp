/** @file
  Unit tests for the implementation of DxeImageVerificationLib.

  Copyright (c) 2025, Yandex. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockUefiLib.h>
#include <GoogleTest/Library/MockUefiRuntimeServicesTableLib.h>
#include <GoogleTest/Library/MockUefiBootServicesTableLib.h>
#include <GoogleTest/Library/MockDevicePathLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>

  #include "DxeImageVerificationLibGoogleTest.h"
}

//////////////////////////////////////////////////////////////////////////////
class CheckImageTypeResult : public ::testing::Test {
public:
  EFI_DEVICE_PATH_PROTOCOL File;

protected:
  MockUefiRuntimeServicesTableLib RtServicesMock;
  MockUefiBootServicesTableLib BsMock;
  MockDevicePathLib DevicePathMock;

  EFI_STATUS Status;

  UINT32 AuthenticationStatus;
  VOID *FileBuffer;
  UINTN FileSize;
  BOOLEAN BootPolicy;

  virtual void
  SetUp (
    )
  {
    AuthenticationStatus = 0;
    FileBuffer           = NULL;
    FileSize             = 0;
    BootPolicy           = FALSE;
  }
};

TEST_F (CheckImageTypeResult, ImageTypeVerifySanity) {
  // Sanity check
  Status = DxeImageVerificationHandler (AuthenticationStatus, NULL, FileBuffer, FileSize, BootPolicy);
  EXPECT_EQ (Status, EFI_INVALID_PARAMETER);
}

TEST_F (CheckImageTypeResult, ImageTypeVerifyImageFromFv) {
  EXPECT_CALL (BsMock, gBS_LocateDevicePath)
    .WillRepeatedly (testing::Return (EFI_SUCCESS));
  EXPECT_CALL (BsMock, gBS_OpenProtocol)
    .WillRepeatedly (testing::Return (EFI_SUCCESS));

  Status = DxeImageVerificationHandler (AuthenticationStatus, &File, FileBuffer, FileSize, BootPolicy);
  EXPECT_EQ (Status, EFI_SUCCESS);
}

TEST_F (CheckImageTypeResult, ImageTypeVerifyImageFromOptionRom) {
  auto  TestFunc = [&](EFI_STATUS ExpectedStatus) {
                     EXPECT_CALL (BsMock, gBS_LocateDevicePath)
                       .Times (3)
                       .WillRepeatedly (testing::Return (EFI_NOT_FOUND));
                     EXPECT_CALL (BsMock, gBS_OpenProtocol)
                       .WillRepeatedly (testing::Return (EFI_NOT_FOUND));
                     EXPECT_CALL (DevicePathMock, IsDevicePathEndType)
                       .WillOnce (testing::Return ((BOOLEAN)FALSE));
                     EXPECT_CALL (DevicePathMock, DevicePathType)
                       .WillOnce (testing::Return ((UINT8)MEDIA_DEVICE_PATH));
                     EXPECT_CALL (DevicePathMock, DevicePathSubType)
                       .WillOnce (testing::Return ((UINT8)MEDIA_RELATIVE_OFFSET_RANGE_DP));

                     Status = DxeImageVerificationHandler (AuthenticationStatus, &File, FileBuffer, FileSize, BootPolicy);
                     EXPECT_EQ (Status, ExpectedStatus);
                   };

  PatchPcdSet32 (PcdOptionRomImageVerificationPolicy, ALWAYS_EXECUTE);
  TestFunc (EFI_SUCCESS);
  PatchPcdSet32 (PcdOptionRomImageVerificationPolicy, NEVER_EXECUTE);
  TestFunc (EFI_ACCESS_DENIED);
}

TEST_F (CheckImageTypeResult, ImageTypeVerifyImageFromRemovableMedia) {
  auto  TestFunc = [&](EFI_STATUS ExpectedStatus) {
                     EXPECT_CALL (BsMock, gBS_LocateDevicePath)
                       .Times (3)
                       .WillRepeatedly (testing::Return (EFI_NOT_FOUND));
                     EXPECT_CALL (DevicePathMock, IsDevicePathEndType)
                       .WillOnce (testing::Return ((BOOLEAN)FALSE));
                     EXPECT_CALL (DevicePathMock, DevicePathType)
                       .WillOnce (testing::Return ((UINT8)MESSAGING_DEVICE_PATH));
                     EXPECT_CALL (DevicePathMock, DevicePathSubType)
                       .WillOnce (testing::Return ((UINT8)MSG_MAC_ADDR_DP));

                     Status = DxeImageVerificationHandler (AuthenticationStatus, &File, FileBuffer, FileSize, BootPolicy);
                     EXPECT_EQ (Status, ExpectedStatus);
                   };

  PatchPcdSet32 (PcdRemovableMediaImageVerificationPolicy, ALWAYS_EXECUTE);
  TestFunc (EFI_SUCCESS);
  PatchPcdSet32 (PcdRemovableMediaImageVerificationPolicy, NEVER_EXECUTE);
  TestFunc (EFI_ACCESS_DENIED);
}

TEST_F (CheckImageTypeResult, ImageTypeVerifyImageFromFixedMedia) {
  auto  TestFunc = [&](EFI_STATUS ExpectedStatus) {
                     EXPECT_CALL (BsMock, gBS_LocateDevicePath)
                       .WillOnce (testing::Return (EFI_NOT_FOUND))
                       .WillOnce (testing::Return (EFI_NOT_FOUND))
                       .WillOnce (testing::Return (EFI_SUCCESS));

                     Status = DxeImageVerificationHandler (AuthenticationStatus, &File, FileBuffer, FileSize, BootPolicy);
                     EXPECT_EQ (Status, ExpectedStatus);
                   };

  PatchPcdSet32 (PcdFixedMediaImageVerificationPolicy, ALWAYS_EXECUTE);
  TestFunc (EFI_SUCCESS);
  PatchPcdSet32 (PcdFixedMediaImageVerificationPolicy, NEVER_EXECUTE);
  TestFunc (EFI_ACCESS_DENIED);
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}

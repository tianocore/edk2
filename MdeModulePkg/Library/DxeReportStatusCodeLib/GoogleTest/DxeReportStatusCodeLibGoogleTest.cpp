/** @file
  UEFI based application for unit testing the DxeReportStatusCodeLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockUefiLib.h>
#include <GoogleTest/Library/MockUefiBootServicesTableLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/ReportStatusCodeLib.h>
  #include <Library/UefiBootServicesTableLib.h>
}

using namespace testing;

class DxeReportStatusCodeLibGeneralTest : public Test {
protected:
  StrictMock<MockUefiBootServicesTableLib> BsMock;
};

// Verify that ReportStatusCode does not attempt to locate the ReportStatusCode protocol
// on demand if not cached. It should only be updated via events outside of a call to
// ReportStatusCode to prevent possible TPL inversion.
TEST_F (DxeReportStatusCodeLibGeneralTest, DoNotCallLocateProtocol) {
  EFI_STATUS  Status;

  // LocateProtocol should never be called
  EXPECT_CALL (BsMock, gBS_LocateProtocol (_, _, _))
    .Times (0);

  Status = ReportStatusCode (EFI_PROGRESS_CODE, 0x0);
  ASSERT_EQ (Status, EFI_UNSUPPORTED); // We have not located the protocol, so this is expected.
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

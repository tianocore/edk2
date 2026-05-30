/** @file
  Host-based unit test for QemuKernelLoaderFsDxe blob fetching.

  Exercises QemuKernelFetchBlob() against a programmable fw_cfg stub to
  reproduce the integer-overflow heap buffer overflow tracked as
  review-notes D1 (CWE-190 -> CWE-787): the per-blob sizes returned by the
  (host-controlled) fw_cfg interface are summed into a UINT32 with no overflow
  check, so a wrapping sum makes the allocation far smaller than the number of
  bytes the copy loop subsequently writes.

  The test #includes the module under test as a single translation unit so the
  STATIC QemuKernelFetchBlob() and its STATIC fw_cfg item table are reachable.
  The fw_cfg, HOB, device-path and blob-verifier dependencies have no
  host-library instance, so minimal stubs are provided locally; everything else
  links against the standard host libraries.

  Copyright (c) 2026, Canonical Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/HobLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BlobVerifierLib.h>
#include <Library/UnitTestLib.h>

#define UNIT_TEST_APP_NAME     "QemuKernelLoaderFsDxe Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

//
// ---------------------------------------------------------------------------
// Programmable fw_cfg stub.
//
// The module reads each blob's size with QemuFwCfgRead32() after selecting the
// matching size item, then streams the blob bytes with QemuFwCfgReadBytes()
// after selecting the matching data item. The stub returns a caller-chosen
// size per item and, on a data read, actually writes that many bytes into the
// destination buffer so an over-large request overflows the (short) allocation.
// ---------------------------------------------------------------------------
//
STATIC FIRMWARE_CONFIG_ITEM  mSelectedItem;
STATIC UINT32                mKernelSetupSize;
STATIC UINT32                mKernelImageSize;
STATIC UINT64                mTotalBytesRead;

BOOLEAN
EFIAPI
QemuFwCfgIsAvailable (
  VOID
  )
{
  return TRUE;
}

VOID
EFIAPI
QemuFwCfgSelectItem (
  IN FIRMWARE_CONFIG_ITEM  QemuFwCfgItem
  )
{
  mSelectedItem = QemuFwCfgItem;
}

UINT32
EFIAPI
QemuFwCfgRead32 (
  VOID
  )
{
  switch (mSelectedItem) {
    case QemuFwCfgItemKernelSetupSize:
      return mKernelSetupSize;
    case QemuFwCfgItemKernelSize:
      return mKernelImageSize;
    default:
      return 0;
  }
}

VOID
EFIAPI
QemuFwCfgReadBytes (
  IN UINTN  Size,
  IN VOID   *Buffer  OPTIONAL
  )
{
  mTotalBytesRead += Size;
  if (Buffer != NULL) {
    //
    // Faithfully transfer the requested number of host bytes. If the module
    // sized the allocation from a wrapped sum, this write runs past the end of
    // Buffer and AddressSanitizer reports the heap-buffer-overflow.
    //
    SetMem (Buffer, Size, 0xAB);
  }
}

//
// ---------------------------------------------------------------------------
// Minimal stubs for the remaining dependencies that have no host instance.
// None are exercised by QemuKernelFetchBlob(); they only need to link because
// the whole module is compiled as one translation unit.
// ---------------------------------------------------------------------------
//
VOID *
EFIAPI
GetFirstGuidHob (
  IN CONST EFI_GUID  *Guid
  )
{
  return NULL;
}

VOID *
EFIAPI
GetNextGuidHob (
  IN CONST EFI_GUID  *Guid,
  IN CONST VOID      *HobStart
  )
{
  return NULL;
}

BOOLEAN
EFIAPI
IsDevicePathValid (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN       UINTN                     MaxSize
  )
{
  return TRUE;
}

EFI_STATUS
EFIAPI
VerifyBlob (
  IN  CONST CHAR16  *BlobName,
  IN  CONST VOID    *Buf,
  IN  UINT32        BufSize,
  IN  EFI_STATUS    FetchStatus
  )
{
  return EFI_SUCCESS;
}

//
// Pull in the module under test (STATIC QemuKernelFetchBlob + mKernelBlobItems).
//
#include "../QemuKernelLoaderFsDxe.c"

/**
  A host-controlled fw_cfg reports a kernel setup size near UINT32_MAX and a
  small image size whose sum wraps to a tiny value. Pre-fix, the allocation is
  sized from the wrapped sum while the copy loop writes the full (un-wrapped)
  per-item sizes -> heap buffer overflow (caught by ASan). Post-fix,
  QemuKernelFetchBlob() must reject the input and append no blob.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
KernelSizeSumOverflowIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  // Reset module state touched by the function under test.
  mKernelBlobs    = NULL;
  mKernelBlobCount = 0;
  mTotalBlobBytes = 0;
  mTotalBytesRead = 0;

  // 0xFFFFFF00 + 0x200 wraps (UINT32) to 0x100.
  mKernelSetupSize = 0xFFFFFF00;
  mKernelImageSize = 0x00000200;

  Status = QemuKernelFetchBlob (&mKernelBlobItems[0]);

  //
  // Reaching this point means no overflow occurred (post-fix). The contract is:
  // the malformed sizes are rejected and no blob is recorded.
  //
  UT_ASSERT_TRUE (EFI_ERROR (Status));
  UT_ASSERT_EQUAL (mKernelBlobCount, 0);
  UT_ASSERT_TRUE (mKernelBlobs == NULL);

  return UNIT_TEST_PASSED;
}

/**
  Sanity case: a well-formed kernel blob (setup + image) must be fetched
  successfully and recorded with the summed size.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
WellFormedKernelBlobIsFetched (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  mKernelBlobs    = NULL;
  mKernelBlobCount = 0;
  mTotalBlobBytes = 0;
  mTotalBytesRead = 0;

  mKernelSetupSize = 0x1000;
  mKernelImageSize = 0x4000;

  Status = QemuKernelFetchBlob (&mKernelBlobItems[0]);

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (mKernelBlobCount, 1);
  UT_ASSERT_TRUE (mKernelBlobs != NULL);
  UT_ASSERT_EQUAL (mKernelBlobs->Size, 0x5000);
  UT_ASSERT_EQUAL (mTotalBytesRead, 0x5000);

  // The fetched blob was prepended to the module's global list; free it so the
  // test run is leak-clean (the module itself never frees fetched blobs).
  FreePool (mKernelBlobs->Data);
  FreePool (mKernelBlobs);
  mKernelBlobs = NULL;

  return UNIT_TEST_PASSED;
}

/**
  Build and run the test suite.
**/
STATIC
EFI_STATUS
EFIAPI
UnitTestingEntry (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      FetchTests;

  Framework = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  Status = InitUnitTestFramework (&Framework, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  Status = CreateUnitTestSuite (&FetchTests, Framework, "QemuKernelFetchBlob Tests", "QemuKernelLoaderFsDxe.Fetch", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for FetchTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (FetchTests, "Well-formed kernel blob is fetched", "WellFormed", WellFormedKernelBlobIsFetched, NULL, NULL, NULL);
  AddTestCase (FetchTests, "Overflowing size sum is rejected", "SizeOverflow", KernelSizeSumOverflowIsRejected, NULL, NULL, NULL);

  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  return UnitTestingEntry ();
}

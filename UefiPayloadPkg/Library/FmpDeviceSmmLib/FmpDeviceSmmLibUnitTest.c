/** @file
  Unit tests for SMMSTORE-backed firmware update policy.

  Copyright (c) 2026, Star Labs Systems. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>

#include "FmpDeviceSmmFlashRetry.h"
#include "FmpDeviceSmmManifest.h"
#include "FmpDeviceSmmUpdatePolicy.h"

#define UNIT_TEST_APP_NAME     "FmpDeviceSmmLib Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"
#define TEST_BLOCK_SIZE        16

typedef struct {
  UINTN      ReadFailures;
  UINTN      WriteFailures;
  UINTN      EraseFailures;
  UINTN      CorruptReads;
  BOOLEAN    ShortWrites;
  UINTN      ReadCalls;
  UINTN      WriteCalls;
  UINTN      EraseCalls;
  UINTN      StallCalls;
  UINT8      Storage[TEST_BLOCK_SIZE];
} FLASH_IO_TEST_CONTEXT;

STATIC FLASH_IO_TEST_CONTEXT  mFlashContext;

typedef struct {
  UINT8                      Firmware[32];
  REGION_MANIFEST_ENTRY      Entries[2];
  REGION_MANIFEST_TRAILER    Trailer;
} TEST_MANIFEST_IMAGE;

/**
  Consume one injected failure.

  @param[in,out] Failures  Remaining failure count.

  @retval TRUE   A failure was consumed.
  @retval FALSE  No failures remain.
**/
STATIC
BOOLEAN
ConsumeFailure (
  IN OUT UINTN  *Failures
  )
{
  if (*Failures == 0) {
    return FALSE;
  }

  (*Failures)--;
  return TRUE;
}

/**
  Read from the in-memory test flash.

  @param[in]     Lba       Ignored block number.
  @param[in]     Offset    Offset in the test flash.
  @param[in,out] NumBytes  Requested and actual byte count.
  @param[out]    Buffer    Destination buffer.

  @retval EFI_SUCCESS          The data was read.
  @retval EFI_BAD_BUFFER_SIZE  The range is invalid.
  @retval EFI_DEVICE_ERROR     An injected failure occurred.
**/
STATIC
EFI_STATUS
TestFlashRead (
  IN     EFI_LBA  Lba,
  IN     UINTN    Offset,
  IN OUT UINTN    *NumBytes,
  OUT    UINT8    *Buffer
  )
{
  mFlashContext.ReadCalls++;
  if (ConsumeFailure (&mFlashContext.ReadFailures)) {
    return EFI_DEVICE_ERROR;
  }

  if ((Offset > sizeof (mFlashContext.Storage)) || (*NumBytes > (sizeof (mFlashContext.Storage) - Offset))) {
    return EFI_BAD_BUFFER_SIZE;
  }

  CopyMem (Buffer, mFlashContext.Storage + Offset, *NumBytes);
  if (ConsumeFailure (&mFlashContext.CorruptReads) && (*NumBytes > 0)) {
    Buffer[0] ^= 0xff;
  }

  return EFI_SUCCESS;
}

/**
  Write to the in-memory test flash.

  @param[in]     Lba       Ignored block number.
  @param[in]     Offset    Offset in the test flash.
  @param[in,out] NumBytes  Requested and actual byte count.
  @param[in]     Buffer    Source buffer.

  @retval EFI_SUCCESS          The data was written or a short write injected.
  @retval EFI_BAD_BUFFER_SIZE  The range is invalid.
  @retval EFI_DEVICE_ERROR     An injected failure occurred.
**/
STATIC
EFI_STATUS
TestFlashWrite (
  IN     EFI_LBA  Lba,
  IN     UINTN    Offset,
  IN OUT UINTN    *NumBytes,
  IN     UINT8    *Buffer
  )
{
  mFlashContext.WriteCalls++;
  if (ConsumeFailure (&mFlashContext.WriteFailures)) {
    return EFI_DEVICE_ERROR;
  }

  if (mFlashContext.ShortWrites && (*NumBytes > 0)) {
    (*NumBytes)--;
    return EFI_SUCCESS;
  }

  if ((Offset > sizeof (mFlashContext.Storage)) || (*NumBytes > (sizeof (mFlashContext.Storage) - Offset))) {
    return EFI_BAD_BUFFER_SIZE;
  }

  CopyMem (mFlashContext.Storage + Offset, Buffer, *NumBytes);
  return EFI_SUCCESS;
}

/**
  Erase the in-memory test flash.

  @param[in] Lba  Ignored block number.

  @retval EFI_SUCCESS       The block was erased.
  @retval EFI_DEVICE_ERROR  An injected failure occurred.
**/
STATIC
EFI_STATUS
TestFlashErase (
  IN EFI_LBA  Lba
  )
{
  mFlashContext.EraseCalls++;
  if (ConsumeFailure (&mFlashContext.EraseFailures)) {
    return EFI_DEVICE_ERROR;
  }

  SetMem (mFlashContext.Storage, sizeof (mFlashContext.Storage), 0xff);
  return EFI_SUCCESS;
}

/**
  Record a retry stall.

  @param[in] Attempt  Ignored attempt number.
**/
STATIC
VOID
TestFlashStall (
  IN UINTN  Attempt
  )
{
  mFlashContext.StallCalls++;
}

STATIC CONST FMP_DEVICE_FLASH_IO  mTestFlashIo = {
  TestFlashRead,
  TestFlashWrite,
  TestFlashErase,
  TestFlashStall
};

/**
  Reset the in-memory flash test context.
**/
STATIC
VOID
ResetFlashContext (
  VOID
  )
{
  ZeroMem (&mFlashContext, sizeof (mFlashContext));
}

/**
  Verify preserved variable storage keeps variable services enabled.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
PreservedStoreKeepsVariableServices (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_FALSE (FmpDeviceShouldDisableVariableServices (TRUE));
  return UNIT_TEST_PASSED;
}

/**
  Verify unproven variable storage disables variable services.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
UnprovenStoreDisablesVariableServices (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_TRUE (FmpDeviceShouldDisableVariableServices (FALSE));
  return UNIT_TEST_PASSED;
}

/**
  Verify overlapping flash ranges are detected.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
OverlappingRangesAreDetected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_TRUE (FmpDeviceFlashRangesOverlap (0x1000, 0x2000, 0x2000, 0x1000));
  UT_ASSERT_TRUE (FmpDeviceFlashRangesOverlap (0x2000, 0x1000, 0x1000, 0x2000));
  return UNIT_TEST_PASSED;
}

/**
  Verify adjacent flash ranges are not treated as overlapping.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
AdjacentRangesDoNotOverlap (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_FALSE (FmpDeviceFlashRangesOverlap (0x1000, 0x1000, 0x2000, 0x1000));
  UT_ASSERT_FALSE (FmpDeviceFlashRangesOverlap (0x2000, 0x1000, 0x1000, 0x1000));
  return UNIT_TEST_PASSED;
}

/**
  Verify malformed flash ranges fail closed.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
MalformedRangesFailClosed (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_TRUE (FmpDeviceFlashRangesOverlap (0x1000, 0, 0x2000, 0x1000));
  UT_ASSERT_TRUE (FmpDeviceFlashRangesOverlap (MAX_UINTN - 1, 2, 0x2000, 0x1000));
  return UNIT_TEST_PASSED;
}

/**
  Verify flash update steps are calculated across block boundaries.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
FlashRangeStepsAreCalculated (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINTN       StepCount;

  Status = FmpDeviceGetFlashRangeStepCount (15, 2, 16, &StepCount);

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (StepCount, 4);
  return UNIT_TEST_PASSED;
}

/**
  Verify flash update step arithmetic rejects overflow.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
FlashRangeStepOverflowFails (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN  StepCount;

  UT_ASSERT_EQUAL (
    FmpDeviceGetFlashRangeStepCount (MAX_UINTN, 2, 16, &StepCount),
    EFI_BAD_BUFFER_SIZE
    );
  UT_ASSERT_EQUAL (
    FmpDeviceGetFlashRangeStepCount (0, MAX_UINTN, 1, &StepCount),
    EFI_BAD_BUFFER_SIZE
    );
  return UNIT_TEST_PASSED;
}

/**
  Initialize a test image containing an RMAP manifest.

  @param[out] Image       Image to initialize.
  @param[in]  EntryCount  Manifest entry count.
**/
STATIC
VOID
InitializeManifestImage (
  OUT TEST_MANIFEST_IMAGE  *Image,
  IN  UINT16               EntryCount
  )
{
  ZeroMem (Image, sizeof (*Image));
  CopyMem (Image->Entries[0].RegionName, "COREBOOT", sizeof ("COREBOOT"));
  CopyMem (Image->Entries[1].RegionName, "FW_MAIN_A", sizeof ("FW_MAIN_A"));
  Image->Trailer.Signature  = REGION_MANIFEST_SIGNATURE;
  Image->Trailer.Version    = REGION_MANIFEST_VERSION;
  Image->Trailer.EntryCount = EntryCount;
}

/**
  Verify an image without a manifest reports EFI_NOT_FOUND.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
AbsentManifestIsReported (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                   Status;
  UINT8                        Image[32];
  UINTN                        EntryCount;
  CONST REGION_MANIFEST_ENTRY  *Entries;
  UINTN                        FirmwareImageSize;

  ZeroMem (Image, sizeof (Image));
  Status = FmpDeviceLocateRegionManifest (
             Image,
             sizeof (Image),
             &EntryCount,
             &Entries,
             &FirmwareImageSize
             );

  UT_ASSERT_EQUAL (Status, EFI_NOT_FOUND);
  UT_ASSERT_EQUAL (EntryCount, 0);
  UT_ASSERT_EQUAL ((UINTN)Entries, (UINTN)NULL);
  UT_ASSERT_EQUAL (FirmwareImageSize, sizeof (Image));
  return UNIT_TEST_PASSED;
}

/**
  Verify a valid manifest is located and decoded.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
ValidManifestIsLocated (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                   Status;
  TEST_MANIFEST_IMAGE          Image;
  UINTN                        EntryCount;
  CONST REGION_MANIFEST_ENTRY  *Entries;
  UINTN                        FirmwareImageSize;

  InitializeManifestImage (&Image, ARRAY_SIZE (Image.Entries));
  Status = FmpDeviceLocateRegionManifest (
             (CONST UINT8 *)&Image,
             sizeof (Image),
             &EntryCount,
             &Entries,
             &FirmwareImageSize
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (EntryCount, ARRAY_SIZE (Image.Entries));
  UT_ASSERT_EQUAL ((UINTN)Entries, (UINTN)Image.Entries);
  UT_ASSERT_EQUAL (FirmwareImageSize, sizeof (Image.Firmware));
  return UNIT_TEST_PASSED;
}

/**
  Verify malformed manifests are rejected.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
MalformedManifestFailsClosed (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                   Status;
  TEST_MANIFEST_IMAGE          Image;
  UINTN                        EntryCount;
  CONST REGION_MANIFEST_ENTRY  *Entries;
  UINTN                        FirmwareImageSize;

  InitializeManifestImage (&Image, ARRAY_SIZE (Image.Entries));
  Image.Trailer.Version = REGION_MANIFEST_VERSION + 1;
  Status                = FmpDeviceLocateRegionManifest (
                            (CONST UINT8 *)&Image,
                            sizeof (Image),
                            &EntryCount,
                            &Entries,
                            &FirmwareImageSize
                            );
  UT_ASSERT_EQUAL (Status, EFI_COMPROMISED_DATA);

  InitializeManifestImage (&Image, MAX_UINT16);
  Status = FmpDeviceLocateRegionManifest (
             (CONST UINT8 *)&Image,
             sizeof (Image),
             &EntryCount,
             &Entries,
             &FirmwareImageSize
             );
  UT_ASSERT_EQUAL (Status, EFI_COMPROMISED_DATA);

  InitializeManifestImage (&Image, ARRAY_SIZE (Image.Entries));
  CopyMem (Image.Entries[1].RegionName, Image.Entries[0].RegionName, sizeof (Image.Entries[1].RegionName));
  Status = FmpDeviceLocateRegionManifest (
             (CONST UINT8 *)&Image,
             sizeof (Image),
             &EntryCount,
             &Entries,
             &FirmwareImageSize
             );
  UT_ASSERT_EQUAL (Status, EFI_COMPROMISED_DATA);

  InitializeManifestImage (&Image, ARRAY_SIZE (Image.Entries));
  Image.Entries[0].RegionName[9] = 'X';
  Status                         = FmpDeviceLocateRegionManifest (
                                     (CONST UINT8 *)&Image,
                                     sizeof (Image),
                                     &EntryCount,
                                     &Entries,
                                     &FirmwareImageSize
                                     );
  UT_ASSERT_EQUAL (Status, EFI_COMPROMISED_DATA);
  return UNIT_TEST_PASSED;
}

/**
  Verify transient erase, write and verify failures recover.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TransientFlashErrorsRecover (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Expected[TEST_BLOCK_SIZE];
  UINT8       VerifyBuffer[TEST_BLOCK_SIZE];

  ResetFlashContext ();
  SetMem (Expected, sizeof (Expected), 0x5a);
  mFlashContext.EraseFailures = 1;
  mFlashContext.WriteFailures = 1;
  mFlashContext.CorruptReads  = 1;

  Status = FmpDeviceFlashProgramWithRetry (&mTestFlashIo, 0, Expected, VerifyBuffer, sizeof (Expected));

  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (mFlashContext.EraseCalls, 3);
  UT_ASSERT_EQUAL (mFlashContext.WriteCalls, 3);
  UT_ASSERT_EQUAL (mFlashContext.ReadCalls, 2);
  UT_ASSERT_MEM_EQUAL (mFlashContext.Storage, Expected, sizeof (Expected));
  return UNIT_TEST_PASSED;
}

/**
  Verify persistent read failures exhaust the retry limit.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
ReadErrorsExhaustRetryLimit (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINTN       NumBytes;
  UINT8       Buffer[TEST_BLOCK_SIZE];

  ResetFlashContext ();
  mFlashContext.ReadFailures = FMP_DEVICE_SMM_FLASH_RETRY_COUNT;
  NumBytes                   = sizeof (Buffer);

  Status = FmpDeviceFlashReadWithRetry (&mTestFlashIo, 0, 0, &NumBytes, Buffer);

  UT_ASSERT_EQUAL (Status, EFI_DEVICE_ERROR);
  UT_ASSERT_EQUAL (mFlashContext.ReadCalls, FMP_DEVICE_SMM_FLASH_RETRY_COUNT);
  return UNIT_TEST_PASSED;
}

/**
  Verify persistent erase failures exhaust the retry limit.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
EraseErrorsExhaustRetryLimit (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  ResetFlashContext ();
  mFlashContext.EraseFailures = FMP_DEVICE_SMM_FLASH_RETRY_COUNT;

  Status = FmpDeviceFlashEraseWithRetry (&mTestFlashIo, 0);

  UT_ASSERT_EQUAL (Status, EFI_DEVICE_ERROR);
  UT_ASSERT_EQUAL (mFlashContext.EraseCalls, FMP_DEVICE_SMM_FLASH_RETRY_COUNT);
  return UNIT_TEST_PASSED;
}

/**
  Verify persistent write failures exhaust the retry limit.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
WriteErrorsExhaustRetryLimit (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINTN       NumBytes;
  UINT8       Buffer[TEST_BLOCK_SIZE];

  ResetFlashContext ();
  mFlashContext.WriteFailures = FMP_DEVICE_SMM_FLASH_RETRY_COUNT;
  NumBytes                    = sizeof (Buffer);

  Status = FmpDeviceFlashWriteWithRetry (&mTestFlashIo, 0, 0, &NumBytes, Buffer);

  UT_ASSERT_EQUAL (Status, EFI_DEVICE_ERROR);
  UT_ASSERT_EQUAL (mFlashContext.WriteCalls, FMP_DEVICE_SMM_FLASH_RETRY_COUNT);
  return UNIT_TEST_PASSED;
}

/**
  Verify repeated short writes exhaust the retry limit.

  @param[in] Context  Unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
ShortWritesExhaustRetryLimit (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINTN       NumBytes;
  UINT8       Buffer[TEST_BLOCK_SIZE];

  ResetFlashContext ();
  mFlashContext.ShortWrites = TRUE;
  NumBytes                  = sizeof (Buffer);

  Status = FmpDeviceFlashWriteWithRetry (&mTestFlashIo, 0, 0, &NumBytes, Buffer);

  UT_ASSERT_EQUAL (Status, EFI_DEVICE_ERROR);
  UT_ASSERT_EQUAL (mFlashContext.WriteCalls, FMP_DEVICE_SMM_FLASH_RETRY_COUNT);
  UT_ASSERT_EQUAL (NumBytes, sizeof (Buffer) - 1);
  return UNIT_TEST_PASSED;
}

/**
  Create and run the SMMSTORE update policy test suite.

  @retval EFI_SUCCESS  All tests passed.
  @retval Others       Test framework setup or execution failed.
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
  UNIT_TEST_SUITE_HANDLE      PolicyTests;

  Framework = NULL;
  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  Status = InitUnitTestFramework (
             &Framework,
             UNIT_TEST_APP_NAME,
             gEfiCallerBaseName,
             UNIT_TEST_APP_VERSION
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CreateUnitTestSuite (
             &PolicyTests,
             Framework,
             "SMMSTORE update policy",
             "FmpDeviceSmmLib.Policy",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    FreeUnitTestFramework (Framework);
    return Status;
  }

  AddTestCase (PolicyTests, "Preserved store keeps variable services", "PreservedStore", PreservedStoreKeepsVariableServices, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Unproven store disables variable services", "UnprovenStore", UnprovenStoreDisablesVariableServices, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Overlapping ranges are detected", "Overlap", OverlappingRangesAreDetected, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Adjacent ranges do not overlap", "Adjacent", AdjacentRangesDoNotOverlap, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Malformed ranges fail closed", "Malformed", MalformedRangesFailClosed, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Flash range steps are calculated", "StepCount", FlashRangeStepsAreCalculated, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Flash range step overflow fails", "StepOverflow", FlashRangeStepOverflowFails, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Absent manifest is reported", "ManifestAbsent", AbsentManifestIsReported, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Valid manifest is located", "ManifestValid", ValidManifestIsLocated, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Malformed manifest fails closed", "ManifestMalformed", MalformedManifestFailsClosed, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Transient flash errors recover", "TransientIo", TransientFlashErrorsRecover, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Read errors exhaust retry limit", "ReadError", ReadErrorsExhaustRetryLimit, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Erase errors exhaust retry limit", "EraseError", EraseErrorsExhaustRetryLimit, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Write errors exhaust retry limit", "WriteError", WriteErrorsExhaustRetryLimit, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Short writes exhaust retry limit", "ShortWrite", ShortWritesExhaustRetryLimit, NULL, NULL, NULL);

  Status = RunAllTestSuites (Framework);
  FreeUnitTestFramework (Framework);
  return Status;
}

#define FmpDeviceSmmLibUnitTestMain  main

/**
  Host test application entry point.

  @param[in] Argc  Command-line argument count.
  @param[in] Argv  Command-line arguments.

  @retval 0  All tests passed.
  @retval 1  A test failed.
**/
INT32
FmpDeviceSmmLibUnitTestMain (
  IN INT32  Argc,
  IN CHAR8  *Argv[]
  )
{
  return EFI_ERROR (UnitTestingEntry ()) ? 1 : 0;
}

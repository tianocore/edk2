/** @file
  Unit tests for SMMSTORE-backed firmware update policy.

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

STATIC
VOID
ResetFlashContext (
  VOID
  )
{
  ZeroMem (&mFlashContext, sizeof (mFlashContext));
}

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

INT32
FmpDeviceSmmLibUnitTestMain (
  IN INT32  Argc,
  IN CHAR8  *Argv[]
  )
{
  return EFI_ERROR (UnitTestingEntry ()) ? 1 : 0;
}

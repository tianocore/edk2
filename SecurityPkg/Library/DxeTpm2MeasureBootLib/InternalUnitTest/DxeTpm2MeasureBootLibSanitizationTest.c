/** @file
  This file includes the unit test cases for the DxeTpm2MeasureBootLibSanitizationTest.c.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Protocol/BlockIo.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Protocol/Tcg2Protocol.h>

#include "../DxeTpm2MeasureBootLibSanitization.h"

#define UNIT_TEST_NAME     "DxeTpm2MeasureBootLibSanitizationTest"
#define UNIT_TEST_VERSION  "1.0"

#define DEFAULT_PRIMARY_TABLE_HEADER_REVISION                     0x00010000
#define DEFAULT_PRIMARY_TABLE_HEADER_NUMBER_OF_PARTITION_ENTRIES  1
#define DEFAULT_PRIMARY_TABLE_HEADER_SIZE_OF_PARTITION_ENTRY      128

/**
  This function tests the SanitizeEfiPartitionTableHeader function.
  It's intent is to test that a malicious EFI_PARTITION_TABLE_HEADER
  structure will not cause undefined or unexpected behavior.

  In general the TPM should still be able to measure the data, but
  be the header should be sanitized to prevent any unexpected behavior.

  @param[in] Context  The unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
  @retval UNIT_TEST_ERROR_TEST_FAILED  The test failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestSanitizeEfiPartitionTableHeader (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                  Status;
  EFI_PARTITION_TABLE_HEADER  PrimaryHeader;
  EFI_BLOCK_IO_PROTOCOL       BlockIo;
  EFI_BLOCK_IO_MEDIA          BlockMedia;

  // Generate EFI_BLOCK_IO_MEDIA test data
  BlockMedia.MediaId          = 1;
  BlockMedia.RemovableMedia   = FALSE;
  BlockMedia.MediaPresent     = TRUE;
  BlockMedia.LogicalPartition = FALSE;
  BlockMedia.ReadOnly         = FALSE;
  BlockMedia.WriteCaching     = FALSE;
  BlockMedia.BlockSize        = 512;
  BlockMedia.IoAlign          = 1;
  BlockMedia.LastBlock        = 0;

  // Generate EFI_BLOCK_IO_PROTOCOL test data
  BlockIo.Revision    = 1;
  BlockIo.Media       = &BlockMedia;
  BlockIo.Reset       = NULL;
  BlockIo.ReadBlocks  = NULL;
  BlockIo.WriteBlocks = NULL;
  BlockIo.FlushBlocks = NULL;

  // Geneate EFI_PARTITION_TABLE_HEADER test data
  PrimaryHeader.Header.Signature         = EFI_PTAB_HEADER_ID;
  PrimaryHeader.Header.Revision          = DEFAULT_PRIMARY_TABLE_HEADER_REVISION;
  PrimaryHeader.Header.HeaderSize        = sizeof (EFI_PARTITION_TABLE_HEADER);
  PrimaryHeader.MyLBA                    = 1;
  PrimaryHeader.PartitionEntryLBA        = 2;
  PrimaryHeader.AlternateLBA             = 3;
  PrimaryHeader.FirstUsableLBA           = 4;
  PrimaryHeader.LastUsableLBA            = 5;
  PrimaryHeader.NumberOfPartitionEntries = DEFAULT_PRIMARY_TABLE_HEADER_NUMBER_OF_PARTITION_ENTRIES;
  PrimaryHeader.SizeOfPartitionEntry     = DEFAULT_PRIMARY_TABLE_HEADER_SIZE_OF_PARTITION_ENTRY;
  PrimaryHeader.PartitionEntryArrayCRC32 = 0; // Purposely invalid

  // Calculate the CRC32 of the PrimaryHeader
  PrimaryHeader.Header.CRC32 = CalculateCrc32 ((UINT8 *)&PrimaryHeader, PrimaryHeader.Header.HeaderSize);

  // Test that a normal PrimaryHeader passes validation
  Status = Tpm2SanitizeEfiPartitionTableHeader (&PrimaryHeader, &BlockIo);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Test that when number of partition entries is 0, the function returns EFI_DEVICE_ERROR
  // Should print "Invalid Partition Table Header NumberOfPartitionEntries!""
  PrimaryHeader.NumberOfPartitionEntries = 0;
  Status                                 = Tpm2SanitizeEfiPartitionTableHeader (&PrimaryHeader, &BlockIo);
  UT_ASSERT_EQUAL (Status, EFI_DEVICE_ERROR);
  PrimaryHeader.NumberOfPartitionEntries = DEFAULT_PRIMARY_TABLE_HEADER_SIZE_OF_PARTITION_ENTRY;

  // Test that when the header size is too small, the function returns EFI_DEVICE_ERROR
  // Should print "Invalid Partition Table Header Size!"
  PrimaryHeader.Header.HeaderSize = 0;
  Status                          = Tpm2SanitizeEfiPartitionTableHeader (&PrimaryHeader, &BlockIo);
  UT_ASSERT_EQUAL (Status, EFI_DEVICE_ERROR);
  PrimaryHeader.Header.HeaderSize = sizeof (EFI_PARTITION_TABLE_HEADER);

  // Test that when the SizeOfPartitionEntry is too small, the function returns EFI_DEVICE_ERROR
  // should print: "SizeOfPartitionEntry shall be set to a value of 128 x 2^n where n is an integer greater than or equal to zero (e.g., 128, 256, 512, etc.)!"
  PrimaryHeader.SizeOfPartitionEntry = 1;
  Status                             = Tpm2SanitizeEfiPartitionTableHeader (&PrimaryHeader, &BlockIo);
  UT_ASSERT_EQUAL (Status, EFI_DEVICE_ERROR);

  DEBUG ((DEBUG_INFO, "%a: Test passed\n", __func__));

  return UNIT_TEST_PASSED;
}

/**
  This function tests the SanitizePrimaryHeaderAllocationSize function.
  It's intent is to test that the untrusted input from a EFI_PARTITION_TABLE_HEADER
  structure will not cause an overflow when calculating the allocation size.

  @param[in] Context  The unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
  @retval UNIT_TEST_ERROR_TEST_FAILED  The test failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestSanitizePrimaryHeaderAllocationSize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT32  AllocationSize;

  EFI_STATUS                  Status;
  EFI_PARTITION_TABLE_HEADER  PrimaryHeader;

  // Test that a normal PrimaryHeader passes validation
  PrimaryHeader.NumberOfPartitionEntries = 5;
  PrimaryHeader.SizeOfPartitionEntry     = DEFAULT_PRIMARY_TABLE_HEADER_SIZE_OF_PARTITION_ENTRY;

  Status = Tpm2SanitizePrimaryHeaderAllocationSize (&PrimaryHeader, &AllocationSize);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Test that the allocation size is correct compared to the existing logic
  UT_ASSERT_EQUAL (AllocationSize, PrimaryHeader.NumberOfPartitionEntries * PrimaryHeader.SizeOfPartitionEntry);

  // Test that an overflow is detected
  PrimaryHeader.NumberOfPartitionEntries = MAX_UINT32;
  PrimaryHeader.SizeOfPartitionEntry     = 5;
  Status                                 = Tpm2SanitizePrimaryHeaderAllocationSize (&PrimaryHeader, &AllocationSize);
  UT_ASSERT_EQUAL (Status, EFI_BAD_BUFFER_SIZE);

  // Test the inverse
  PrimaryHeader.NumberOfPartitionEntries = 5;
  PrimaryHeader.SizeOfPartitionEntry     = MAX_UINT32;
  Status                                 = Tpm2SanitizePrimaryHeaderAllocationSize (&PrimaryHeader, &AllocationSize);
  UT_ASSERT_EQUAL (Status, EFI_BAD_BUFFER_SIZE);

  // Test the worst case scenario
  PrimaryHeader.NumberOfPartitionEntries = MAX_UINT32;
  PrimaryHeader.SizeOfPartitionEntry     = MAX_UINT32;
  Status                                 = Tpm2SanitizePrimaryHeaderAllocationSize (&PrimaryHeader, &AllocationSize);
  UT_ASSERT_EQUAL (Status, EFI_BAD_BUFFER_SIZE);

  DEBUG ((DEBUG_INFO, "%a: Test passed\n", __func__));

  return UNIT_TEST_PASSED;
}

/**
  This function tests the SanitizePrimaryHeaderGptEventSize function.
  It's intent is to test that the untrusted input from a EFI_GPT_DATA structure
  will not cause an overflow when calculating the event size.

  @param[in] Context  The unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
  @retval UNIT_TEST_ERROR_TEST_FAILED  The test failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestSanitizePrimaryHeaderGptEventSize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT32                      EventSize;
  UINT32                      ExistingLogicEventSize;
  EFI_STATUS                  Status;
  EFI_PARTITION_TABLE_HEADER  PrimaryHeader;
  UINTN                       NumberOfPartition;

  // Test that a normal PrimaryHeader passes validation
  PrimaryHeader.NumberOfPartitionEntries = 5;
  PrimaryHeader.SizeOfPartitionEntry     = DEFAULT_PRIMARY_TABLE_HEADER_SIZE_OF_PARTITION_ENTRY;

  // set the number of partitions
  NumberOfPartition = 13;

  // that the primary event size is correct
  Status = Tpm2SanitizePrimaryHeaderGptEventSize (&PrimaryHeader, NumberOfPartition, &EventSize);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Calculate the existing logic event size
  ExistingLogicEventSize = (UINT32)(OFFSET_OF (EFI_TCG2_EVENT, Event) + OFFSET_OF (EFI_GPT_DATA, Partitions)
                                    + NumberOfPartition * PrimaryHeader.SizeOfPartitionEntry);

  // Check that the event size is correct
  UT_ASSERT_EQUAL (EventSize, ExistingLogicEventSize);

  // Tests that the primary event size may not overflow
  Status = Tpm2SanitizePrimaryHeaderGptEventSize (&PrimaryHeader, MAX_UINT32, &EventSize);
  UT_ASSERT_EQUAL (Status, EFI_BAD_BUFFER_SIZE);

  // Test that the size of partition entries may not overflow
  PrimaryHeader.SizeOfPartitionEntry = MAX_UINT32;
  Status                             = Tpm2SanitizePrimaryHeaderGptEventSize (&PrimaryHeader, NumberOfPartition, &EventSize);
  UT_ASSERT_EQUAL (Status, EFI_BAD_BUFFER_SIZE);

  DEBUG ((DEBUG_INFO, "%a: Test passed\n", __func__));

  return UNIT_TEST_PASSED;
}

/**
  This function tests the SanitizePeImageEventSize function.
  It's intent is to test that the untrusted input from a file path when generating a
  EFI_IMAGE_LOAD_EVENT structure will not cause an overflow when calculating
  the event size when allocating space

  @param[in] Context  The unit test context.

  @retval UNIT_TEST_PASSED  The test passed.
  @retval UNIT_TEST_ERROR_TEST_FAILED  The test failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestSanitizePeImageEventSize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT32      EventSize;
  UINTN       ExistingLogicEventSize;
  UINT32      FilePathSize;
  EFI_STATUS  Status;

  FilePathSize = 255;

  // Test that a normal PE image passes validation
  Status = Tpm2SanitizePeImageEventSize (FilePathSize, &EventSize);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

  // Test that the event size is correct compared to the existing logic
  ExistingLogicEventSize  = OFFSET_OF (EFI_IMAGE_LOAD_EVENT, DevicePath) + FilePathSize;
  ExistingLogicEventSize += OFFSET_OF (EFI_TCG2_EVENT, Event);

  if (EventSize != ExistingLogicEventSize) {
    UT_LOG_ERROR ("SanitizePeImageEventSize returned an incorrect event size. Expected %u, got %u\n", ExistingLogicEventSize, EventSize);
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  // Test that the event size may not overflow
  Status = Tpm2SanitizePeImageEventSize (MAX_UINT32, &EventSize);
  UT_ASSERT_EQUAL (Status, EFI_BAD_BUFFER_SIZE);

  DEBUG ((DEBUG_INFO, "%a: Test passed\n", __func__));

  return UNIT_TEST_PASSED;
}

// *--------------------------------------------------------------------*
// *  Unit Test Code Main Function
// *--------------------------------------------------------------------*

/**
  This function acts as the entry point for the unit tests.

  @retval UNIT_TEST_PASSED  The test passed.
  @retval UNIT_TEST_ERROR_TEST_FAILED  The test failed.
  @retval others The test failed.
**/
EFI_STATUS
EFIAPI
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      Tcg2MeasureBootLibValidationTestSuite;

  Framework = NULL;

  DEBUG ((DEBUG_INFO, "%a: TestMain() - Start\n", UNIT_TEST_NAME));

  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed in InitUnitTestFramework. Status = %r\n", UNIT_TEST_NAME, Status));
    goto EXIT;
  }

  Status = CreateUnitTestSuite (&Tcg2MeasureBootLibValidationTestSuite, Framework, "Tcg2MeasureBootLibValidationTestSuite", "Common.Tcg2MeasureBootLibValidation", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%s: Failed in CreateUnitTestSuite for Tcg2MeasureBootLibValidationTestSuite\n", UNIT_TEST_NAME));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  // -----------Suite---------------------------------Description----------------------------Class----------------------------------Test Function------------------------Pre---Clean-Context
  AddTestCase (Tcg2MeasureBootLibValidationTestSuite, "Tests Validating EFI Partition Table", "Common.Tcg2MeasureBootLibValidation", TestSanitizeEfiPartitionTableHeader, NULL, NULL, NULL);
  AddTestCase (Tcg2MeasureBootLibValidationTestSuite, "Tests Primary header gpt event checks for overflow", "Common.Tcg2MeasureBootLibValidation", TestSanitizePrimaryHeaderAllocationSize, NULL, NULL, NULL);
  AddTestCase (Tcg2MeasureBootLibValidationTestSuite, "Tests Primary header allocation size checks for overflow", "Common.Tcg2MeasureBootLibValidation", TestSanitizePrimaryHeaderGptEventSize, NULL, NULL, NULL);
  AddTestCase (Tcg2MeasureBootLibValidationTestSuite, "Tests PE Image and FileSize checks for overflow", "Common.Tcg2MeasureBootLibValidation", TestSanitizePeImageEventSize, NULL, NULL, NULL);

  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework != NULL) {
    FreeUnitTestFramework (Framework);
  }

  DEBUG ((DEBUG_INFO, "%a: TestMain() - End\n", UNIT_TEST_NAME));
  return Status;
}

///
/// Avoid ECC error for function name that starts with lower case letter
///
#define DxeTpm2MeasureBootLibUnitTestMain  main

/**
  Standard POSIX C entry point for host based unit test execution.

  @param[in] Argc  Number of arguments
  @param[in] Argv  Array of pointers to arguments

  @retval 0      Success
  @retval other  Error
**/
INT32
DxeTpm2MeasureBootLibUnitTestMain (
  IN INT32  Argc,
  IN CHAR8  *Argv[]
  )
{
  return (INT32)UefiTestMain ();
}

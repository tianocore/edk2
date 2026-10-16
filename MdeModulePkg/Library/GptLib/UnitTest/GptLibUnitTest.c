/** @file
  Host-based unit tests for GptLib.

  These tests exercise PartitionValidGptTable(), PartitionCheckGptEntry()
  and PartitionRestoreGptTable() against an in-memory mock disk, covering
  both well-formed GPT structures and malformed ones an attacker may
  present: bad signature/revision, header size
  boundaries, CRC corruption, MyLBA replay, zero/non-power-of-two entry
  sizes, LBA multiplication overflow, out-of-range and overlapping
  entries, and backup/primary restore behavior.

  Copyright (c) 2026, SUSE LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/GptLib.h>
#include "GptLibUnitTestCommon.h"

#define UNIT_TEST_NAME     "GptLibUnitTest"
#define UNIT_TEST_VERSION  "1.0"

// ---------------------------------------------------------------------------
// PartitionValidGptTable() tests
// ---------------------------------------------------------------------------

/**
  Verify that a well-formed primary GPT header is accepted.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestValidPrimaryHeaderIsAccepted (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  UT_ASSERT_TRUE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));
  UT_ASSERT_EQUAL (Header.MyLBA, PRIMARY_PART_HEADER_LBA);
  UT_ASSERT_EQUAL (Header.AlternateLBA, LAST_LBA);
  UT_ASSERT_EQUAL (Header.NumberOfPartitionEntries, NUM_PARTITION_ENTRIES);
  UT_ASSERT_EQUAL (Header.SizeOfPartitionEntry, PARTITION_ENTRY_SIZE);

  return UNIT_TEST_PASSED;
}

/**
  Verify that a well-formed backup GPT header is accepted.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestValidBackupHeaderIsAccepted (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  //
  // Backup header at the last LBA with its entry array after the usable
  // region, as laid out by real partitioning tools. The validator must not
  // reject an entry array located past FirstUsableLBA.
  //
  WriteGptTableAt (
    LAST_LBA,
    PRIMARY_PART_HEADER_LBA,
    LAST_USABLE_LBA + 1,
    NUM_PARTITION_ENTRIES,
    PARTITION_ENTRY_SIZE,
    2
    );

  UT_ASSERT_TRUE (ValidateAt (LAST_LBA, &Header));
  UT_ASSERT_EQUAL (Header.MyLBA, LAST_LBA);
  UT_ASSERT_EQUAL (Header.AlternateLBA, PRIMARY_PART_HEADER_LBA);

  return UNIT_TEST_PASSED;
}

/**
  Verify that the minimum (92-byte) header size is accepted.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestMinimumHeaderSizeIsAccepted (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  GetPrimaryHeader ()->Header.HeaderSize = TEST_GPT_HEADER_MIN_SIZE;
  UpdateGptHeaderCrc (GetPrimaryHeader ());

  UT_ASSERT_TRUE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that a larger power-of-two (256-byte) partition entry size is accepted.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestLargerPowerOfTwoEntrySizeIsAccepted (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  //
  // SizeOfPartitionEntry of 256 (128 * 2^1) is legal per the UEFI spec.
  //
  WriteGptTableAt (PRIMARY_PART_HEADER_LBA, LAST_LBA, PRIMARY_PART_HEADER_LBA + 1, 8, 256, 2);

  UT_ASSERT_TRUE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));
  UT_ASSERT_EQUAL (Header.SizeOfPartitionEntry, 256U);

  return UNIT_TEST_PASSED;
}

/**
  Verify that a GPT header read failure is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestHeaderReadFailureIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  UT_ASSERT_FALSE (ValidateAt (DISK_SECTORS + 10, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that an invalid GPT signature is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestInvalidSignatureIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  GetPrimaryHeader ()->Header.Signature = SIGNATURE_64 ('X', 'X', 'X', 'X', 'X', 'X', 'X', 'X');
  UpdateGptHeaderCrc (GetPrimaryHeader ());

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that an invalid GPT revision is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestInvalidRevisionIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;
  STATIC CONST UINT32         BadRevisions[] = { 0x00000000, 0x00010001, 0x00020000, 0xFFFFFFFF };
  UINTN                       Index;

  for (Index = 0; Index < ARRAY_SIZE (BadRevisions); Index++) {
    SetupDiskWithValidPrimary ();

    GetPrimaryHeader ()->Header.Revision = BadRevisions[Index];
    UpdateGptHeaderCrc (GetPrimaryHeader ());

    UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));
  }

  return UNIT_TEST_PASSED;
}

/**
  Verify that a header size below the minimum is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestHeaderSizeBelowMinimumIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  GetPrimaryHeader ()->Header.HeaderSize = TEST_GPT_HEADER_MIN_SIZE - 1;
  GetPrimaryHeader ()->Header.CRC32      = 0;
  GetPrimaryHeader ()->Header.CRC32      = CalculateCrc32 (GetPrimaryHeader (), TEST_GPT_HEADER_MIN_SIZE - 1);

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that a zero header size is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestZeroHeaderSizeIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  GetPrimaryHeader ()->Header.HeaderSize = 0;

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that a header size beyond the block size is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestHeaderSizeBeyondBlockSizeIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  //
  // HeaderSize larger than the block that was read: the CRC check must
  // refuse to scan past the buffer instead of reading out of bounds.
  //
  GetPrimaryHeader ()->Header.HeaderSize = SECTOR_SIZE + 1;

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that a corrupt header CRC is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestCorruptHeaderCrcIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  GetPrimaryHeader ()->Header.CRC32 ^= 0xFFFFFFFF;

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that a MyLBA mismatch is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestMyLbaMismatchIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  //
  // A header claiming to live at a different LBA than the one it was read
  // from (e.g. a copied/replayed header) must be rejected.
  //
  GetPrimaryHeader ()->MyLBA = 5;
  UpdateGptHeaderCrc (GetPrimaryHeader ());

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that a header reporting zero partition entries is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestZeroPartitionEntriesIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  GetPrimaryHeader ()->NumberOfPartitionEntries = 0;
  UpdateGptHeaderCrc (GetPrimaryHeader ());

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that a partition entry size below 128 bytes is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestEntrySizeTooSmallIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  GetPrimaryHeader ()->SizeOfPartitionEntry = (UINT32)(sizeof (EFI_PARTITION_ENTRY) / 2);
  UpdateGptHeaderCrc (GetPrimaryHeader ());

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that a zero partition entry size is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestZeroEntrySizeIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  //
  // Must be rejected by the size check before the later division by
  // SizeOfPartitionEntry (division by zero) can be reached.
  //
  GetPrimaryHeader ()->SizeOfPartitionEntry = 0;
  UpdateGptHeaderCrc (GetPrimaryHeader ());

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that a non power-of-two partition entry size is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestNonPowerOfTwoEntrySizeIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  //
  // 192 is >= 128 but not 128 * 2^n.
  //
  GetPrimaryHeader ()->SizeOfPartitionEntry = 192;
  UpdateGptHeaderCrc (GetPrimaryHeader ());

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that a PartitionEntryLBA causing multiplication overflow is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestEntryLbaMultiplicationOverflowIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  //
  // PartitionEntryLBA * BlockSize would wrap around UINT64. Must be rejected
  // before any read is attempted with the truncated offset.
  //
  GetPrimaryHeader ()->PartitionEntryLBA = MAX_UINT64;
  UpdateGptHeaderCrc (GetPrimaryHeader ());

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that an entry array CRC mismatch is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestEntryArrayCrcMismatchIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  //
  // Flip one byte inside an unused entry slot without refreshing the
  // entry-array CRC32 recorded in the header.
  //
  GetDiskLba (PRIMARY_PART_HEADER_LBA + 1)[(2U * PARTITION_ENTRY_SIZE) + 7] ^= 0xA5;

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

/**
  Verify that an entry array read failure is rejected.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestEntryArrayReadFailureIsRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;

  SetupDiskWithValidPrimary ();

  //
  // Entry array placed so close to the end of the device that reading
  // NumberOfPartitionEntries * SizeOfPartitionEntry bytes runs off the disk.
  //
  GetPrimaryHeader ()->PartitionEntryLBA = LAST_LBA;
  UpdateGptHeaderCrc (GetPrimaryHeader ());

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &Header));

  return UNIT_TEST_PASSED;
}

// ---------------------------------------------------------------------------
// PartitionCheckGptEntry() tests
// ---------------------------------------------------------------------------

/**
  Verify that valid partition entries report no flags.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestValidEntriesReportNoFlags (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;
  EFI_PARTITION_ENTRY         Entries[4];
  EFI_PARTITION_ENTRY_STATUS  Status[4];
  UINTN                       Index;

  InitCheckEntryHeader (&Header, 4, sizeof (EFI_PARTITION_ENTRY));
  ZeroMem (Entries, sizeof (Entries));
  ZeroMem (Status, sizeof (Status));

  FillPartitionEntry (&Entries[0], &mPartitionGuid1, 34, 100);
  FillPartitionEntry (&Entries[1], &mPartitionGuid2, 101, 200);

  PartitionCheckGptEntry (&Header, Entries, Status);

  for (Index = 0; Index < 4; Index++) {
    UT_ASSERT_FALSE (Status[Index].OutOfRange);
    UT_ASSERT_FALSE (Status[Index].Overlap);
    UT_ASSERT_FALSE (Status[Index].OsSpecific);
  }

  return UNIT_TEST_PASSED;
}

/**
  Verify that an OS-specific partition attribute is reported.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestOsSpecificAttributeIsReported (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;
  EFI_PARTITION_ENTRY         Entries[1];
  EFI_PARTITION_ENTRY_STATUS  Status[1];

  InitCheckEntryHeader (&Header, 1, sizeof (EFI_PARTITION_ENTRY));
  ZeroMem (Entries, sizeof (Entries));
  ZeroMem (Status, sizeof (Status));

  FillPartitionEntry (&Entries[0], &mPartitionGuid1, 40, 100);
  Entries[0].Attributes = BIT1;

  PartitionCheckGptEntry (&Header, Entries, Status);

  UT_ASSERT_TRUE (Status[0].OsSpecific);
  UT_ASSERT_FALSE (Status[0].OutOfRange);

  return UNIT_TEST_PASSED;
}

/**
  Verify that unused partition entries are ignored.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestUnusedEntriesAreIgnored (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;
  EFI_PARTITION_ENTRY         Entries[2];
  EFI_PARTITION_ENTRY_STATUS  Status[2];

  InitCheckEntryHeader (&Header, 2, sizeof (EFI_PARTITION_ENTRY));
  ZeroMem (Entries, sizeof (Entries));
  ZeroMem (Status, sizeof (Status));

  //
  // Unused (zero PartitionTypeGUID) entry with nonsense LBAs must be skipped
  // and must not participate in the overlap scan.
  //
  Entries[0].StartingLBA = MAX_UINT64;
  Entries[0].EndingLBA   = 0;
  FillPartitionEntry (&Entries[1], &mPartitionGuid1, 40, 100);

  PartitionCheckGptEntry (&Header, Entries, Status);

  UT_ASSERT_FALSE (Status[0].OutOfRange);
  UT_ASSERT_FALSE (Status[0].Overlap);
  UT_ASSERT_FALSE (Status[1].OutOfRange);
  UT_ASSERT_FALSE (Status[1].Overlap);

  return UNIT_TEST_PASSED;
}

/**
  Verify that the entry stride follows SizeOfPartitionEntry.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestEntryStrideFollowsSizeOfPartitionEntry (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;
  UINT8                       Buffer[2 * 256];
  EFI_PARTITION_ENTRY_STATUS  Status[2];
  EFI_PARTITION_ENTRY         *DecoyEntry;

  InitCheckEntryHeader (&Header, 2, 256);
  ZeroMem (Buffer, sizeof (Buffer));
  ZeroMem (Status, sizeof (Status));

  //
  // Real entries at 256-byte stride: entry 0 at offset 0, entry 1 at offset
  // 256, deliberately overlapping. A decoy non-overlapping but out-of-range
  // entry is placed at offset 128: a checker that wrongly walked with a
  // 128-byte stride would flag OutOfRange instead of Overlap on index 1.
  //
  FillPartitionEntry ((EFI_PARTITION_ENTRY *)(VOID *)&Buffer[0], &mPartitionGuid1, 40, 100);
  FillPartitionEntry ((EFI_PARTITION_ENTRY *)(VOID *)&Buffer[256], &mPartitionGuid2, 90, 150);
  DecoyEntry = (EFI_PARTITION_ENTRY *)(VOID *)&Buffer[128];
  FillPartitionEntry (DecoyEntry, &mPartitionGuid1, 1000, 2000);

  PartitionCheckGptEntry (&Header, (EFI_PARTITION_ENTRY *)(VOID *)Buffer, Status);

  UT_ASSERT_TRUE (Status[0].Overlap);
  UT_ASSERT_TRUE (Status[1].Overlap);
  UT_ASSERT_FALSE (Status[1].OutOfRange);

  return UNIT_TEST_PASSED;
}

/**
  Verify that an inverted LBA range reports OutOfRange.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestInvertedLbaRangeReportsOutOfRange (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;
  EFI_PARTITION_ENTRY         Entries[1];
  EFI_PARTITION_ENTRY_STATUS  Status[1];

  InitCheckEntryHeader (&Header, 1, sizeof (EFI_PARTITION_ENTRY));
  ZeroMem (Entries, sizeof (Entries));
  ZeroMem (Status, sizeof (Status));

  FillPartitionEntry (&Entries[0], &mPartitionGuid1, 100, 50);

  PartitionCheckGptEntry (&Header, Entries, Status);

  UT_ASSERT_TRUE (Status[0].OutOfRange);

  return UNIT_TEST_PASSED;
}

/**
  Verify that an entry outside the usable region reports OutOfRange.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestEntryOutsideUsableRegionReportsOutOfRange (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;
  EFI_PARTITION_ENTRY         Entries[2];
  EFI_PARTITION_ENTRY_STATUS  Status[2];

  InitCheckEntryHeader (&Header, 2, sizeof (EFI_PARTITION_ENTRY));
  ZeroMem (Entries, sizeof (Entries));
  ZeroMem (Status, sizeof (Status));

  //
  // Entry 0 starts before FirstUsableLBA; entry 1 ends after LastUsableLBA.
  //
  FillPartitionEntry (&Entries[0], &mPartitionGuid1, FIRST_USABLE_LBA - 1, 100);
  FillPartitionEntry (&Entries[1], &mPartitionGuid2, 150, 201);

  PartitionCheckGptEntry (&Header, Entries, Status);

  UT_ASSERT_TRUE (Status[0].OutOfRange);
  UT_ASSERT_TRUE (Status[1].OutOfRange);

  return UNIT_TEST_PASSED;
}

/**
  Verify that overlapping partition entries report Overlap.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestOverlappingEntriesReportOverlap (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  Header;
  EFI_PARTITION_ENTRY         Entries[3];
  EFI_PARTITION_ENTRY_STATUS  Status[3];

  InitCheckEntryHeader (&Header, 3, sizeof (EFI_PARTITION_ENTRY));
  ZeroMem (Entries, sizeof (Entries));
  ZeroMem (Status, sizeof (Status));

  FillPartitionEntry (&Entries[0], &mPartitionGuid1, 40, 100);
  FillPartitionEntry (&Entries[1], &mPartitionGuid2, 90, 150);
  FillPartitionEntry (&Entries[2], &mPartitionGuid1, 160, 200);

  PartitionCheckGptEntry (&Header, Entries, Status);

  UT_ASSERT_TRUE (Status[0].Overlap);
  UT_ASSERT_TRUE (Status[1].Overlap);
  UT_ASSERT_FALSE (Status[2].Overlap);

  return UNIT_TEST_PASSED;
}

// ---------------------------------------------------------------------------
// PartitionRestoreGptTable() tests
// ---------------------------------------------------------------------------

/**
  Verify that a corrupted primary GPT is restored from the backup.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestRestorePrimaryFromBackup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  BackupHeader;
  EFI_PARTITION_TABLE_HEADER  RestoredHeader;

  SetupDiskWithValidPrimary ();

  //
  // Wipe the primary and keep only a valid backup whose entry array follows
  // the usable region.
  //
  WriteGptTableAt (
    LAST_LBA,
    PRIMARY_PART_HEADER_LBA,
    LAST_USABLE_LBA + 1,
    NUM_PARTITION_ENTRIES,
    PARTITION_ENTRY_SIZE,
    2
    );
  ZeroMem (GetDiskLba (PRIMARY_PART_HEADER_LBA), SECTOR_SIZE);
  ZeroMem (GetDiskLba (PRIMARY_PART_HEADER_LBA + 1), PART_ARRAY_SIZE);

  UT_ASSERT_FALSE (ValidateAt (PRIMARY_PART_HEADER_LBA, &RestoredHeader));
  UT_ASSERT_TRUE (ValidateAt (LAST_LBA, &BackupHeader));

  UT_ASSERT_TRUE (PartitionRestoreGptTable (&mBlockIo, &mDiskIo, &BackupHeader));

  UT_ASSERT_TRUE (ValidateAt (PRIMARY_PART_HEADER_LBA, &RestoredHeader));
  UT_ASSERT_EQUAL (RestoredHeader.MyLBA, PRIMARY_PART_HEADER_LBA);
  UT_ASSERT_EQUAL (RestoredHeader.AlternateLBA, LAST_LBA);
  UT_ASSERT_EQUAL (RestoredHeader.PartitionEntryLBA, PRIMARY_PART_HEADER_LBA + 1);

  return UNIT_TEST_PASSED;
}

/**
  Verify that a corrupted backup GPT is restored from the primary.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestRestoreBackupFromPrimary (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  PrimaryHeader;
  EFI_PARTITION_TABLE_HEADER  RestoredHeader;

  SetupDiskWithValidPrimary ();

  UT_ASSERT_TRUE (ValidateAt (PRIMARY_PART_HEADER_LBA, &PrimaryHeader));
  UT_ASSERT_FALSE (ValidateAt (LAST_LBA, &RestoredHeader));

  UT_ASSERT_TRUE (PartitionRestoreGptTable (&mBlockIo, &mDiskIo, &PrimaryHeader));

  UT_ASSERT_TRUE (ValidateAt (LAST_LBA, &RestoredHeader));
  UT_ASSERT_EQUAL (RestoredHeader.MyLBA, LAST_LBA);
  UT_ASSERT_EQUAL (RestoredHeader.AlternateLBA, PRIMARY_PART_HEADER_LBA);
  UT_ASSERT_EQUAL (RestoredHeader.PartitionEntryLBA, LAST_USABLE_LBA + 1);

  return UNIT_TEST_PASSED;
}

/**
  Verify that a restore attempt fails on write-protected media.

  @param[in]  Context  Unit test context.

  @retval  UNIT_TEST_PASSED  The test passed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestRestoreFailsOnWriteProtectedMedia (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_PARTITION_TABLE_HEADER  PrimaryHeader;

  SetupDiskWithValidPrimary ();

  UT_ASSERT_TRUE (ValidateAt (PRIMARY_PART_HEADER_LBA, &PrimaryHeader));

  mWriteProtected = TRUE;

  UT_ASSERT_FALSE (PartitionRestoreGptTable (&mBlockIo, &mDiskIo, &PrimaryHeader));
  UT_ASSERT_FALSE (ValidateAt (LAST_LBA, &PrimaryHeader));

  return UNIT_TEST_PASSED;
}

// ---------------------------------------------------------------------------
// Test runner
// ---------------------------------------------------------------------------

/**
  Configure and run the GptLib unit test suites.

  @retval  EFI_SUCCESS  The unit tests ran to completion.
  @retval  other        An error occurred setting up or running the tests.
**/
EFI_STATUS
EFIAPI
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      ValidSuite;
  UNIT_TEST_SUITE_HANDLE      EntrySuite;
  UNIT_TEST_SUITE_HANDLE      RestoreSuite;

  Framework = NULL;

  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CreateUnitTestSuite (&ValidSuite, Framework, "PartitionValidGptTable tests", "GptLib.ValidGptTable", NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = CreateUnitTestSuite (&EntrySuite, Framework, "PartitionCheckGptEntry tests", "GptLib.CheckGptEntry", NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = CreateUnitTestSuite (&RestoreSuite, Framework, "PartitionRestoreGptTable tests", "GptLib.RestoreGptTable", NULL, NULL);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  AddTestCase (ValidSuite, "Valid primary header is accepted", "ValidPrimary", TestValidPrimaryHeaderIsAccepted, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Valid backup header is accepted", "ValidBackup", TestValidBackupHeaderIsAccepted, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Minimum (92-byte) header size is accepted", "MinHeaderSize", TestMinimumHeaderSizeIsAccepted, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "256-byte partition entry size is accepted", "EntrySize256", TestLargerPowerOfTwoEntrySizeIsAccepted, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Header read failure is rejected", "HeaderReadFailure", TestHeaderReadFailureIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Invalid signature is rejected", "InvalidSignature", TestInvalidSignatureIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Invalid revision is rejected", "InvalidRevision", TestInvalidRevisionIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Header size below minimum is rejected", "HeaderSizeTooSmall", TestHeaderSizeBelowMinimumIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Zero header size is rejected", "ZeroHeaderSize", TestZeroHeaderSizeIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Header size beyond block size is rejected", "HeaderSizeBeyondBlock", TestHeaderSizeBeyondBlockSizeIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Corrupt header CRC is rejected", "CorruptHeaderCrc", TestCorruptHeaderCrcIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "MyLBA mismatch is rejected", "MyLbaMismatch", TestMyLbaMismatchIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Zero partition entries is rejected", "ZeroEntries", TestZeroPartitionEntriesIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Entry size below 128 is rejected", "EntrySizeTooSmall", TestEntrySizeTooSmallIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Zero entry size is rejected", "ZeroEntrySize", TestZeroEntrySizeIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Non power-of-two entry size is rejected", "NonPowerOfTwoEntrySize", TestNonPowerOfTwoEntrySizeIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "PartitionEntryLBA overflow is rejected", "EntryLbaOverflow", TestEntryLbaMultiplicationOverflowIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Entry array CRC mismatch is rejected", "EntryArrayCrcMismatch", TestEntryArrayCrcMismatchIsRejected, NULL, NULL, NULL);
  AddTestCase (ValidSuite, "Entry array read failure is rejected", "EntryArrayReadFailure", TestEntryArrayReadFailureIsRejected, NULL, NULL, NULL);

  AddTestCase (EntrySuite, "Valid entries report no flags", "ValidEntries", TestValidEntriesReportNoFlags, NULL, NULL, NULL);
  AddTestCase (EntrySuite, "OS-specific attribute is reported", "OsSpecificAttribute", TestOsSpecificAttributeIsReported, NULL, NULL, NULL);
  AddTestCase (EntrySuite, "Unused entries are ignored", "UnusedEntriesIgnored", TestUnusedEntriesAreIgnored, NULL, NULL, NULL);
  AddTestCase (EntrySuite, "Entry stride follows SizeOfPartitionEntry", "EntryStride", TestEntryStrideFollowsSizeOfPartitionEntry, NULL, NULL, NULL);
  AddTestCase (EntrySuite, "Inverted LBA range reports OutOfRange", "InvertedRange", TestInvertedLbaRangeReportsOutOfRange, NULL, NULL, NULL);
  AddTestCase (EntrySuite, "Entry outside usable region reports OutOfRange", "OutsideUsableRegion", TestEntryOutsideUsableRegionReportsOutOfRange, NULL, NULL, NULL);
  AddTestCase (EntrySuite, "Overlapping entries report Overlap", "OverlappingEntries", TestOverlappingEntriesReportOverlap, NULL, NULL, NULL);

  AddTestCase (RestoreSuite, "Restore primary from backup", "RestorePrimary", TestRestorePrimaryFromBackup, NULL, NULL, NULL);
  AddTestCase (RestoreSuite, "Restore backup from primary", "RestoreBackup", TestRestoreBackupFromPrimary, NULL, NULL, NULL);
  AddTestCase (RestoreSuite, "Restore fails on write-protected media", "RestoreWriteProtected", TestRestoreFailsOnWriteProtectedMedia, NULL, NULL, NULL);

  Status = RunAllTestSuites (Framework);

Exit:
  if (Framework != NULL) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

///
/// Avoid ECC error for function name that starts with lower case letter
///
#define GptLibUnitTestMain  main

/**
  Standard POSIX C entry point for host based unit test execution.

  @param[in] Argc  Number of arguments
  @param[in] Argv  Array of pointers to arguments

  @retval 0      Success
  @retval other  Error
**/
INT32
GptLibUnitTestMain (
  IN INT32  Argc,
  IN CHAR8  *Argv[]
  )
{
  UefiTestMain ();
  return 0;
}

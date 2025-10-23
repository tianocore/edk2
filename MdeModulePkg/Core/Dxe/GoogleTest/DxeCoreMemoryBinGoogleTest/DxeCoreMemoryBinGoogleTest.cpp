/** @file
  Unit tests for the implementation of UefiSortLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Library/BaseLib.h>
  #include <Library/UefiLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/DebugLib.h>

  #include <Pi/PiDxeCis.h>
  #include <Guid/LoadModuleAtFixedAddress.h>
  #include <Guid/MemoryProfile.h>

  #define PREVIOUS_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
    ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

  typedef struct {
    EFI_PHYSICAL_ADDRESS    BaseAddress;
    EFI_PHYSICAL_ADDRESS    MaximumAddress;
    UINT64                  CurrentNumberOfPages;
    UINT64                  NumberOfPages;
    UINTN                   InformationIndex;
    BOOLEAN                 Special;
    BOOLEAN                 Runtime;
  } EFI_MEMORY_TYPE_STATISTICS;

  EFI_HANDLE                                  gDxeCoreImageHandle                       = NULL;
  LIST_ENTRY                                  mGcdMemorySpaceMap                        = INITIALIZE_LIST_HEAD_VARIABLE (mGcdMemorySpaceMap);
  BOOLEAN                                     mOnGuarding                               = FALSE;
  EFI_LOAD_FIXED_ADDRESS_CONFIGURATION_TABLE  gLoadModuleAtFixAddressConfigurationTable = { 0, 0 };
  extern EFI_MEMORY_TYPE_STATISTICS           mMemoryTypeStatisticsSortedByAddress[EfiMaxMemoryType + 1];

  EFI_STATUS
  SplitIncomingRange (
    IN EFI_MEMORY_TYPE         Type,
    IN EFI_PHYSICAL_ADDRESS    Start,
    IN EFI_PHYSICAL_ADDRESS    End,
    IN UINT64                  Attribute,
    OUT EFI_MEMORY_DESCRIPTOR  *MemoryDescriptors,
    IN OUT UINTN               *MemoryDescriptorCount
    );

  VOID
  CoreReleaseGcdMemoryLock (
    VOID
    )
  {
    // Implementation of the function
  }

  VOID
  CoreNotifySignalList (
    IN EFI_GUID  *EventGroup
    )
  {
    // Implementation of the function
  }

  VOID
  EFIAPI
  CoreRestoreTpl (
    IN EFI_TPL  NewTpl
    )
  {
    // Implementation of the function
  }

  EFI_TPL
  EFIAPI
  CoreRaiseTpl (
    IN EFI_TPL  NewTpl
    )
  {
    // Implementation of the function
    return NewTpl;
  }

  EFI_STATUS
  EFIAPI
  ApplyMemoryProtectionPolicy (
    IN  EFI_MEMORY_TYPE       OldType,
    IN  EFI_MEMORY_TYPE       NewType,
    IN  EFI_PHYSICAL_ADDRESS  Memory,
    IN  UINT64                Length
    )
  {
    return EFI_SUCCESS;
  }

  VOID
  CoreAcquireGcdMemoryLock (
    VOID
    )
  {
    // Implementation of the function
  }

  EFI_STATUS
  EFIAPI
  CoreUpdateProfile (
    IN EFI_PHYSICAL_ADDRESS   CallerAddress,
    IN MEMORY_PROFILE_ACTION  Action,
    IN EFI_MEMORY_TYPE        MemoryType,
    IN UINTN                  Size,       // Valid for AllocatePages/FreePages/AllocatePool
    IN VOID                   *Buffer,
    IN CHAR8                  *ActionString OPTIONAL
    )
  {
    return EFI_UNSUPPORTED;
  }

  EFI_STATUS
  CoreConvertPagesWithGuard (
    IN UINT64           Start,
    IN UINTN            NumberOfPages,
    IN EFI_MEMORY_TYPE  NewType
    )
  {
    return EFI_SUCCESS;
  }

  EFI_STATUS
  EFIAPI
  CoreGetMemorySpaceDescriptor (
    IN  EFI_PHYSICAL_ADDRESS             BaseAddress,
    OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *Descriptor
    )
  {
    return EFI_UNSUPPORTED;
  }

  BOOLEAN
  PromoteGuardedFreePages (
    OUT EFI_PHYSICAL_ADDRESS  *StartAddress,
    OUT EFI_PHYSICAL_ADDRESS  *EndAddress
    )
  {
    return TRUE;
  }

  BOOLEAN
  IsHeapGuardEnabled (
    UINT8  GuardType
    )
  {
    return FALSE;
  }

  VOID
  SetGuardForMemory (
    IN EFI_PHYSICAL_ADDRESS  Memory,
    IN UINTN                 NumberOfPages
    )
  {
  }

  BOOLEAN
  IsPageTypeToGuard (
    IN EFI_MEMORY_TYPE    MemoryType,
    IN EFI_ALLOCATE_TYPE  AllocateType
    )
  {
    return FALSE;
  }

  VOID
  EFIAPI
  GuardFreedPagesChecked (
    IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
    IN  UINTN                 Pages
    )
  {
  }

  UINT64
  AdjustMemoryS (
    IN UINT64  Start,
    IN UINT64  Size,
    IN UINT64  SizeRequested
    )
  {
    return Start;
  }

  VOID
  InstallMemoryAttributesTableOnMemoryAllocation (
    IN EFI_MEMORY_TYPE  MemoryType
    )
  {
    // Implementation of the function
  }

  BOOLEAN
  EFIAPI
  IsMemoryGuarded (
    IN EFI_PHYSICAL_ADDRESS  Address
    )
  {
    return mOnGuarding;
  }

  VOID
  MergeMemoryMap (
    IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
    IN OUT UINTN                  *MemoryMapSize,
    IN UINTN                      DescriptorSize
    )
  {
    EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
    EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
    UINT64                 MemoryBlockLength;
    EFI_MEMORY_DESCRIPTOR  *NewMemoryMapEntry;
    EFI_MEMORY_DESCRIPTOR  *NextMemoryMapEntry;

    MemoryMapEntry    = MemoryMap;
    NewMemoryMapEntry = MemoryMap;
    MemoryMapEnd      = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);
    while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
      CopyMem (NewMemoryMapEntry, MemoryMapEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
      NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);

      do {
        MemoryBlockLength = MemoryMapEntry->NumberOfPages << EFI_PAGE_SHIFT;
        if (((UINTN)NextMemoryMapEntry < (UINTN)MemoryMapEnd) &&
            (MemoryMapEntry->Type == NextMemoryMapEntry->Type) &&
            (MemoryMapEntry->Attribute == NextMemoryMapEntry->Attribute) &&
            ((MemoryMapEntry->PhysicalStart + MemoryBlockLength) == NextMemoryMapEntry->PhysicalStart))
        {
          MemoryMapEntry->NumberOfPages += NextMemoryMapEntry->NumberOfPages;
          if (NewMemoryMapEntry != MemoryMapEntry) {
            NewMemoryMapEntry->NumberOfPages += NextMemoryMapEntry->NumberOfPages;
          }

          NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
          continue;
        } else {
          MemoryMapEntry = PREVIOUS_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
          break;
        }
      } while (TRUE);

      MemoryMapEntry    = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
      NewMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapEntry, DescriptorSize);
    }

    *MemoryMapSize = (UINTN)NewMemoryMapEntry - (UINTN)MemoryMap;

    return;
  }

  VOID
  EFIAPI
  DumpGuardedMemoryBitmap (
    VOID
    )
  {
    // Implementation of the function
  }
}

using namespace testing;

class SplitIncomingRangeTest : public  Test {
protected:
  /* Redefining the Test class's SetUp function for test fixtures. */
  void
  SetUp (
    ) override
  {
    EFI_PHYSICAL_ADDRESS  Start = SIZE_64KB;

    // Initialize the mMemoryTypeStatisticsSortedByAddress array
    for (UINTN Index = 0; Index <= EfiMaxMemoryType; Index++) {
      if ((Index == EfiPersistentMemory) || (Index == EfiUnacceptedMemoryType)) {
        // Skip PMEM and Unaccepted memory types for this test
        continue;
      }

      if (!mMemoryTypeStatisticsSortedByAddress[Index].Special) {
        continue;
      }

      mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress    = Start;
      mMemoryTypeStatisticsSortedByAddress[Index].MaximumAddress = Start + EFI_PAGE_SIZE - 1;
      mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages  = 1;

      // The Special memory types are intentionally sparse
      Start += SIZE_64KB;
    }
  }
};

// Test SplitIncomingRange() API for a single memory range that does not overlap
// with existing ranges.
TEST_F (SplitIncomingRangeTest, AddRangeStandaloneBin) {
  CONST UINTN            MaxMemDescCount = 5;
  EFI_MEMORY_DESCRIPTOR  MemDesc[MaxMemDescCount];
  UINTN                  MemDescCount = MaxMemDescCount;
  EFI_STATUS             Status;

  Status = SplitIncomingRange (
             EfiBootServicesData,
             SIZE_1MB,
             SIZE_1MB + EFI_PAGE_SIZE - 1,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, (UINTN)1);

  EXPECT_EQ (MemDesc[0].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[0].PhysicalStart, (EFI_PHYSICAL_ADDRESS)SIZE_1MB);
  EXPECT_EQ (MemDesc[0].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[0].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
}

// Test SplitIncomingRange() API for a single memory range that overlaps with
// a single existing range.
TEST_F (SplitIncomingRangeTest, AddRangeOverlapSingleBin) {
  CONST UINTN            MaxMemDescCount = 5;
  EFI_MEMORY_DESCRIPTOR  MemDesc[MaxMemDescCount];
  UINTN                  MemDescCount = MaxMemDescCount;
  EFI_STATUS             Status;

  // This range overlaps with EfiACPIReclaimMemory only
  Status = SplitIncomingRange (
             EfiBootServicesData,
             SIZE_64KB - EFI_PAGE_SIZE,
             SIZE_64KB + EFI_PAGE_SIZE - 1,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, (UINTN)2); // One for the part before, one for the part within

  EXPECT_EQ (MemDesc[0].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[0].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB - EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[0].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[0].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[1].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[1].PhysicalStart, (EFI_PHYSICAL_ADDRESS)SIZE_64KB);
  EXPECT_EQ (MemDesc[1].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[1].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
}

// Test SplitIncomingRange() API for a single memory range that is within a single
// existing range.
TEST_F (SplitIncomingRangeTest, AddRangeWithinSingleBin) {
  EFI_MEMORY_DESCRIPTOR  MemDesc;
  UINTN                  MemDescCount = 1;
  EFI_STATUS             Status;

  // This range is fully contained within EfiACPIReclaimMemory
  Status = SplitIncomingRange (
             EfiACPIReclaimMemory,
             SIZE_64KB + EFI_PAGE_SIZE,
             SIZE_64KB + 2 * EFI_PAGE_SIZE - 1,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             &MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, (UINTN)1);

  EXPECT_EQ (MemDesc.Type, (UINT32)EfiACPIReclaimMemory);
  EXPECT_EQ (MemDesc.PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc.NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc.Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
}

// Test SplitIncomingRange() API for a single memory range that is identical to
// an existing range.
TEST_F (SplitIncomingRangeTest, AddRangeIdenticalToExistingBin) {
  EFI_MEMORY_DESCRIPTOR  MemDesc;
  UINTN                  MemDescCount = 1;
  EFI_STATUS             Status;

  // This range is identical to EfiACPIReclaimMemory
  Status = SplitIncomingRange (
             EfiACPIReclaimMemory,
             SIZE_64KB,
             SIZE_64KB + EFI_PAGE_SIZE - 1,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             &MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, (UINTN)1);

  EXPECT_EQ (MemDesc.Type, (UINT32)EfiACPIReclaimMemory);
  EXPECT_EQ (MemDesc.PhysicalStart, (EFI_PHYSICAL_ADDRESS)SIZE_64KB);
  EXPECT_EQ (MemDesc.NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc.Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
}

// Test SplitIncomingRange() API for a single memory range that covers from before
// an existing range to the end of that existing range.
TEST_F (SplitIncomingRangeTest, AddRangeCoverSingleBinFromStart) {
  CONST UINTN            MaxMemDescCount = 2;
  EFI_MEMORY_DESCRIPTOR  MemDesc[MaxMemDescCount];
  UINTN                  MemDescCount = MaxMemDescCount;
  EFI_STATUS             Status;

  // This range covers EfiACPIReclaimMemory
  Status = SplitIncomingRange (
             EfiBootServicesData,
             SIZE_64KB - EFI_PAGE_SIZE,
             SIZE_64KB + EFI_PAGE_SIZE - 1,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, MaxMemDescCount);

  EXPECT_EQ (MemDesc[0].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[0].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB - EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[0].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[0].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[1].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[1].PhysicalStart, (EFI_PHYSICAL_ADDRESS)SIZE_64KB);
  EXPECT_EQ (MemDesc[1].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[1].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
}

// Test SplitIncomingRange() API for a single memory range that covers from the
// start of an existing range to after that existing range.
TEST_F (SplitIncomingRangeTest, AddRangeCoverSingleBinToEnd) {
  CONST UINTN            MaxMemDescCount = 2;
  EFI_MEMORY_DESCRIPTOR  MemDesc[MaxMemDescCount];
  UINTN                  MemDescCount = MaxMemDescCount;
  EFI_STATUS             Status;

  // This range covers EfiACPIReclaimMemory
  Status = SplitIncomingRange (
             EfiBootServicesData,
             SIZE_64KB,
             SIZE_64KB + EFI_PAGE_SIZE * 2 - 1,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, MaxMemDescCount);

  EXPECT_EQ (MemDesc[0].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[0].PhysicalStart, (EFI_PHYSICAL_ADDRESS)SIZE_64KB);
  EXPECT_EQ (MemDesc[0].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[0].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[1].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[1].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[1].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[1].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
}

// Test SplitIncomingRange() API for a single memory range that overlaps with
// multiple existing ranges.
TEST_F (SplitIncomingRangeTest, AddRangeCoverMultipleBinsToEnd) {
  CONST UINTN            MaxMemDescCount = 5;
  EFI_MEMORY_DESCRIPTOR  MemDesc[MaxMemDescCount];
  UINTN                  MemDescCount = MaxMemDescCount;
  EFI_STATUS             Status;

  // This range overlaps with EfiACPIReclaimMemory, EfiACPIMemoryNVS and
  // EfiMemoryMappedIO
  Status = SplitIncomingRange (
             EfiBootServicesData,
             SIZE_64KB - EFI_PAGE_SIZE,
             SIZE_64KB + 2 * SIZE_64KB - 1,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, MaxMemDescCount);

  EXPECT_EQ (MemDesc[0].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[0].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB - EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[0].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[0].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[1].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[1].PhysicalStart, (EFI_PHYSICAL_ADDRESS)SIZE_64KB);
  EXPECT_EQ (MemDesc[1].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[1].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[2].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[2].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[2].NumberOfPages, (UINT64)15);
  EXPECT_EQ (MemDesc[2].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[3].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[3].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + SIZE_64KB));
  EXPECT_EQ (MemDesc[3].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[3].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[4].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[4].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + SIZE_64KB + EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[4].NumberOfPages, (UINT64)15);
  EXPECT_EQ (MemDesc[4].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
}

// Test SplitIncomingRange() API for a single memory range that covers multiple
// existing ranges, starting from one special range to the end of another special range.
TEST_F (SplitIncomingRangeTest, AddRangeCoverMultipleBinsStartToEnd) {
  CONST UINTN            MaxMemDescCount = 5;
  EFI_MEMORY_DESCRIPTOR  MemDesc[MaxMemDescCount];
  UINTN                  MemDescCount = MaxMemDescCount;
  EFI_STATUS             Status;

  // This range covers EfiACPIReclaimMemory, EfiACPIMemoryNVS, and EfiMemoryMappedIO
  Status = SplitIncomingRange (
             EfiBootServicesData,
             SIZE_64KB,
             SIZE_64KB + 2 * SIZE_64KB + EFI_PAGE_SIZE - 1,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, MaxMemDescCount);

  EXPECT_EQ (MemDesc[0].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[0].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB));
  EXPECT_EQ (MemDesc[0].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[0].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[1].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[1].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[1].NumberOfPages, (UINT64)15);
  EXPECT_EQ (MemDesc[1].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[2].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[2].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + SIZE_64KB));
  EXPECT_EQ (MemDesc[2].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[2].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[3].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[3].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + SIZE_64KB + EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[3].NumberOfPages, (UINT64)15);
  EXPECT_EQ (MemDesc[3].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[4].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[4].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + 2 * SIZE_64KB));
  EXPECT_EQ (MemDesc[4].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[4].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
}

// Test SplitIncomingRange() API for a single memory range that overlaps with
// multiple existing ranges, with extra space on both ends.
TEST_F (SplitIncomingRangeTest, AddRangeCoverMultipleBinsExtraSpace) {
  CONST UINTN            MaxMemDescCount = 7;
  EFI_MEMORY_DESCRIPTOR  MemDesc[MaxMemDescCount];
  UINTN                  MemDescCount = MaxMemDescCount;
  EFI_STATUS             Status;

  // This range overlaps with EfiACPIReclaimMemory, EfiACPIMemoryNVS and
  // EfiMemoryMappedIO
  Status = SplitIncomingRange (
             EfiBootServicesData,
             SIZE_64KB - EFI_PAGE_SIZE,
             SIZE_64KB + 2 * SIZE_64KB + 2 * EFI_PAGE_SIZE - 1,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, MaxMemDescCount);

  EXPECT_EQ (MemDesc[0].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[0].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB - EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[0].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[0].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[1].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[1].PhysicalStart, (EFI_PHYSICAL_ADDRESS)SIZE_64KB);
  EXPECT_EQ (MemDesc[1].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[1].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[2].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[2].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[2].NumberOfPages, (UINT64)15);
  EXPECT_EQ (MemDesc[2].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[3].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[3].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + SIZE_64KB));
  EXPECT_EQ (MemDesc[3].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[3].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[4].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[4].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + SIZE_64KB + EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[4].NumberOfPages, (UINT64)15);
  EXPECT_EQ (MemDesc[4].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[5].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[5].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + 2 * SIZE_64KB));
  EXPECT_EQ (MemDesc[5].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[5].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[6].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[6].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + 2 * SIZE_64KB + EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[6].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[6].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
}

// Test SplitIncomingRange() API for a single memory range that overlaps with
// multiple existing ranges, with extra space on the starting end.
TEST_F (SplitIncomingRangeTest, AddRangeCoverMultipleBinsFromStart) {
  CONST UINTN            MaxMemDescCount = 5;
  EFI_MEMORY_DESCRIPTOR  MemDesc[MaxMemDescCount];
  UINTN                  MemDescCount = MaxMemDescCount;
  EFI_STATUS             Status;

  // This range overlaps with EfiACPIReclaimMemory, EfiACPIMemoryNVS and
  // EfiMemoryMappedIO
  Status = SplitIncomingRange (
             EfiBootServicesData,
             SIZE_64KB - EFI_PAGE_SIZE,
             SIZE_64KB + SIZE_64KB + 2 * EFI_PAGE_SIZE - 1,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, MaxMemDescCount);

  EXPECT_EQ (MemDesc[0].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[0].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB - EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[0].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[0].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[1].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[1].PhysicalStart, (EFI_PHYSICAL_ADDRESS)SIZE_64KB);
  EXPECT_EQ (MemDesc[1].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[1].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[2].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[2].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[2].NumberOfPages, (UINT64)15);
  EXPECT_EQ (MemDesc[2].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[3].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[3].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + SIZE_64KB));
  EXPECT_EQ (MemDesc[3].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[3].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  EXPECT_EQ (MemDesc[4].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[4].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB + SIZE_64KB + EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[4].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[4].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
}

// Test SplitIncomingRange() API for a single memory range that covers all
// existing ranges, with extra space on both ends.
TEST_F (SplitIncomingRangeTest, AddRangeCoverAllBinsWithExtraSpace) {
  // This test assumes there are 6 special bins defined in mMemoryTypeStatisticsSortedByAddress
  // in SetUp(), EfiUnacceptedMemoryType and EfiPersistentMemory are skipped.
  CONST UINTN            MaxMemDescCount = 13;
  EFI_MEMORY_DESCRIPTOR  MemDesc[MaxMemDescCount];
  UINTN                  MemDescCount = MaxMemDescCount;
  EFI_STATUS             Status;

  // This range covers all special bins
  Status = SplitIncomingRange (
             EfiBootServicesData,
             SIZE_64KB - EFI_PAGE_SIZE,
             SIZE_64KB + 6 * SIZE_64KB - 1,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, MaxMemDescCount);

  EXPECT_EQ (MemDesc[0].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[0].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB - EFI_PAGE_SIZE));
  EXPECT_EQ (MemDesc[0].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[0].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

  for (UINTN Index = 1; Index < MemDescCount; Index += 2) {
    EXPECT_EQ (MemDesc[Index].Type, (UINT32)EfiBootServicesData);
    EXPECT_EQ (MemDesc[Index].PhysicalStart, (EFI_PHYSICAL_ADDRESS)SIZE_64KB * (Index / 2 + 1));
    EXPECT_EQ (MemDesc[Index].NumberOfPages, (UINT64)1);
    EXPECT_EQ (MemDesc[Index].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

    EXPECT_EQ (MemDesc[Index + 1].Type, (UINT32)EfiBootServicesData);
    EXPECT_EQ (MemDesc[Index + 1].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB * (Index / 2 + 1) + EFI_PAGE_SIZE));
    EXPECT_EQ (MemDesc[Index + 1].NumberOfPages, (UINT64)15);
    EXPECT_EQ (MemDesc[Index + 1].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
  }
}

// Test SplitIncomingRange() API for a single memory range that covers all
// existing ranges, with extra space at the beginning only.
TEST_F (SplitIncomingRangeTest, AddRangeCoverAllBinsWithExtraSpaceAtBeginning) {
  // This test assumes there are 6 special bins defined in mMemoryTypeStatisticsSortedByAddress
  // in SetUp(), EfiUnacceptedMemoryType and EfiPersistentMemory are skipped.
  CONST UINTN            MaxMemDescCount = 12;
  EFI_MEMORY_DESCRIPTOR  MemDesc[MaxMemDescCount];
  UINTN                  MemDescCount = MaxMemDescCount;
  EFI_STATUS             Status;

  // This range covers all special bins
  Status = SplitIncomingRange (
             EfiBootServicesData,
             EFI_PAGE_SIZE,
             SIZE_64KB + 5 * SIZE_64KB + EFI_PAGE_SIZE - 1,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, MaxMemDescCount);

  for (UINTN Index = 0; Index < MemDescCount - 1; Index += 2) {
    EXPECT_EQ (MemDesc[Index].Type, (UINT32)EfiBootServicesData);
    EXPECT_EQ (MemDesc[Index].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(EFI_PAGE_SIZE + SIZE_64KB * (Index / 2)));
    EXPECT_EQ (MemDesc[Index].NumberOfPages, (UINT64)15);
    EXPECT_EQ (MemDesc[Index].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

    EXPECT_EQ (MemDesc[Index + 1].Type, (UINT32)EfiBootServicesData);
    EXPECT_EQ (MemDesc[Index + 1].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB * (Index / 2 + 1)));
    EXPECT_EQ (MemDesc[Index + 1].NumberOfPages, (UINT64)1);
    EXPECT_EQ (MemDesc[Index + 1].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
  }
}

// Test SplitIncomingRange() API for a single memory range that covers all
// existing ranges, with extra space at the end only.
TEST_F (SplitIncomingRangeTest, AddRangeCoverAllBinsWithExtraSpaceAtEnd) {
  // This test assumes there are 6 special bins defined in mMemoryTypeStatisticsSortedByAddress
  // in SetUp(), EfiUnacceptedMemoryType and EfiPersistentMemory are skipped.
  CONST UINTN            MaxMemDescCount = 12;
  EFI_MEMORY_DESCRIPTOR  MemDesc[MaxMemDescCount];
  UINTN                  MemDescCount = MaxMemDescCount;
  EFI_STATUS             Status;

  // This range covers all special bins
  Status = SplitIncomingRange (
             EfiBootServicesData,
             SIZE_64KB,
             SIZE_64KB + 6 * SIZE_64KB - 1,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, MaxMemDescCount);

  for (UINTN Index = 0; Index < MemDescCount; Index += 2) {
    EXPECT_EQ (MemDesc[Index].Type, (UINT32)EfiBootServicesData);
    EXPECT_EQ (MemDesc[Index].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB * (Index / 2 + 1)));
    EXPECT_EQ (MemDesc[Index].NumberOfPages, (UINT64)1);
    EXPECT_EQ (MemDesc[Index].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

    EXPECT_EQ (MemDesc[Index + 1].Type, (UINT32)EfiBootServicesData);
    EXPECT_EQ (MemDesc[Index + 1].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB * (Index / 2 + 1) + EFI_PAGE_SIZE));
    EXPECT_EQ (MemDesc[Index + 1].NumberOfPages, (UINT64)15);
    EXPECT_EQ (MemDesc[Index + 1].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
  }
}

// Test SplitIncomingRange() API for a single memory range that covers all
// existing ranges, with no extra space.
TEST_F (SplitIncomingRangeTest, AddRangeCoverAllBinsNoExtraSpace) {
  // This test assumes there are 6 special bins defined in mMemoryTypeStatisticsSortedByAddress
  // in SetUp(), EfiUnacceptedMemoryType and EfiPersistentMemory are skipped.
  CONST UINTN            MaxMemDescCount = 11;
  EFI_MEMORY_DESCRIPTOR  MemDesc[MaxMemDescCount];
  UINTN                  MemDescCount = MaxMemDescCount;
  EFI_STATUS             Status;

  // This range covers all special bins
  Status = SplitIncomingRange (
             EfiBootServicesData,
             SIZE_64KB,
             SIZE_64KB + 5 * SIZE_64KB - 1 + EFI_PAGE_SIZE,
             EFI_MEMORY_UC | EFI_MEMORY_WP,
             MemDesc,
             &MemDescCount
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_EQ (MemDescCount, MaxMemDescCount);

  for (UINTN Index = 0; Index < MemDescCount - 1; Index += 2) {
    EXPECT_EQ (MemDesc[Index].Type, (UINT32)EfiBootServicesData);
    EXPECT_EQ (MemDesc[Index].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB * (Index / 2 + 1)));
    EXPECT_EQ (MemDesc[Index].NumberOfPages, (UINT64)1);
    EXPECT_EQ (MemDesc[Index].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));

    EXPECT_EQ (MemDesc[Index + 1].Type, (UINT32)EfiBootServicesData);
    EXPECT_EQ (MemDesc[Index + 1].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB * (Index / 2 + 1) + EFI_PAGE_SIZE));
    EXPECT_EQ (MemDesc[Index + 1].NumberOfPages, (UINT64)15);
    EXPECT_EQ (MemDesc[Index + 1].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
  }

  EXPECT_EQ (MemDesc[MemDescCount - 1].Type, (UINT32)EfiBootServicesData);
  EXPECT_EQ (MemDesc[MemDescCount - 1].PhysicalStart, (EFI_PHYSICAL_ADDRESS)(SIZE_64KB * (MemDescCount / 2 + 1)));
  EXPECT_EQ (MemDesc[MemDescCount - 1].NumberOfPages, (UINT64)1);
  EXPECT_EQ (MemDesc[MemDescCount - 1].Attribute, (UINT64)(EFI_MEMORY_UC | EFI_MEMORY_WP));
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

/** @file
  Unit tests for the implementation of UefiSortLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Library/UefiLib.h>
  #include <Library/BaseLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/DebugLib.h>

  #include <Pi/PiDxeCis.h>
  #include <Guid/LoadModuleAtFixedAddress.h>
  #include <Guid/MemoryProfile.h>

  #include "../../Mem/Imem.h"

  /**
    Internal function.  Adds a ranges to the memory map.
    The range must not already exist in the map.

    @param  Type                   The type of memory range to add
    @param  Start                  The starting address in the memory range Must be
                                  paged aligned
    @param  End                    The last address in the range Must be the last
                                  byte of a page
    @param  Attribute              The attributes of the memory range to add

  **/
  VOID
  CoreAddRange (
    IN EFI_MEMORY_TYPE       Type,
    IN EFI_PHYSICAL_ADDRESS  Start,
    IN EFI_PHYSICAL_ADDRESS  End,
    IN UINT64                Attribute
    );

  INTN
  EFIAPI
  CompareMemoryTypeStats (
    IN CONST VOID  *A,
    IN CONST VOID  *B
    );

  /**
    Internal function.  Moves any memory descriptors that are on the
    temporary descriptor stack to heap.

  **/
  VOID
  CoreFreeMemoryMapStack (
    VOID
    );

  typedef struct {
    EFI_PHYSICAL_ADDRESS    BaseAddress;
    EFI_PHYSICAL_ADDRESS    MaximumAddress;
    UINT64                  CurrentNumberOfPages;
    UINT64                  NumberOfPages;
    UINTN                   InformationIndex;
    BOOLEAN                 Special;
    BOOLEAN                 Runtime;
  } EFI_MEMORY_TYPE_STATISTICS;

  extern EFI_MEMORY_TYPE_STATISTICS  mMemoryTypeStatistics[EfiMaxMemoryType + 1];
  extern EFI_MEMORY_TYPE_STATISTICS  mMemoryTypeStatisticsSortedByAddress[EfiMaxMemoryType + 1];
  extern BOOLEAN                     mMemoryTypeInformationInitialized;
  extern LIST_ENTRY                  mFreeMemoryMapEntryList;
  extern UINTN                       mMapDepth;
}

using namespace testing;

class CoreAddRangeTest : public  Test {
protected:
  EFI_STATUS Status;
  EFI_MEMORY_TYPE_STATISTICS MemoryTypeStatisticsCache[EfiMaxMemoryType + 1];

  /* Redefining the Test class's SetUp function for test fixtures. */
  void
  SetUp (
    ) override
  {
    EFI_MEMORY_TYPE_STATISTICS  Dummy;
    EFI_PHYSICAL_ADDRESS        Start = SIZE_64KB;

    CopyMem (
      MemoryTypeStatisticsCache,
      mMemoryTypeStatistics,
      sizeof (MemoryTypeStatisticsCache)
      );

    // Initialize the mMemoryTypeStatisticsSortedByAddress array
    // The special memory types should be sparse in as each bin is a few EFI_PAGE_SIZE
    // in size but the distance between bins is SIZE_64KB.
    for (UINTN Index = 0; Index <= EfiMaxMemoryType; Index++) {
      if ((Index == EfiPersistentMemory) ||
          (Index == EfiUnacceptedMemoryType))
      {
        // Skip PMEM and Unaccepted memory types for this test
        continue;
      }

      if (!mMemoryTypeStatistics[Index].Special) {
        continue;
      }

      mMemoryTypeStatistics[Index].BaseAddress    = Start;
      mMemoryTypeStatistics[Index].MaximumAddress = Start + EFI_PAGE_SIZE - 1;
      mMemoryTypeStatistics[Index].NumberOfPages  = 1;

      mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress    = Start;
      mMemoryTypeStatisticsSortedByAddress[Index].MaximumAddress = Start + EFI_PAGE_SIZE - 1;
      mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages  = 1;

      // The Special memory types are intentionally sparse
      Start += SIZE_64KB;
    }

    QuickSort (
      (VOID *)mMemoryTypeStatisticsSortedByAddress,
      sizeof (mMemoryTypeStatisticsSortedByAddress) / sizeof (EFI_MEMORY_TYPE_STATISTICS),
      sizeof (EFI_MEMORY_TYPE_STATISTICS),
      CompareMemoryTypeStats,
      &Dummy
      );
  }

  void
  TearDown (
    ) override
  {
    // Clear the memory map
    mFreeMemoryMapEntryList = INITIALIZE_LIST_HEAD_VARIABLE (mFreeMemoryMapEntryList);
    gMemoryMap              = INITIALIZE_LIST_HEAD_VARIABLE (gMemoryMap);

    CopyMem (
      mMemoryTypeStatistics,
      MemoryTypeStatisticsCache,
      sizeof (MemoryTypeStatisticsCache)
      );

    for (UINTN Index = 0; Index <= EfiMaxMemoryType; Index++) {
      mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress      = 0;
      mMemoryTypeStatisticsSortedByAddress[Index].MaximumAddress   = MAX_ALLOC_ADDRESS;
      mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages    = 0;
      mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex = Index;
      mMemoryTypeStatisticsSortedByAddress[Index].Special          = MemoryTypeStatisticsCache[Index].Special;
      mMemoryTypeStatisticsSortedByAddress[Index].Runtime          = MemoryTypeStatisticsCache[Index].Runtime;
    }

    mMapDepth = 0;
  }
};

// Verify that the mMemoryTypeStatisticsSortedByAddress array is initialized correctly
TEST_F (CoreAddRangeTest, VerifyMemoryStatisticsSort) {
  EFI_PHYSICAL_ADDRESS  Start = (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB;
  EFI_PHYSICAL_ADDRESS  End   = Start + EFI_PAGE_SIZE - 1;
  UINTN                 Index;

  // Specifically, the Special memory types should be initialized to cover
  // the buffer we set up in SetUp().

  // And as the memory type index increases, the BaseAddress should also increase
  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: bucket %d: %lx - %lx, special: %d\n",
      __func__,
      mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex,
      mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress,
      mMemoryTypeStatisticsSortedByAddress[Index].MaximumAddress,
      mMemoryTypeStatisticsSortedByAddress[Index].Special
      ));

    if (mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages == 0) {
      break;
    }

    ASSERT_GE (mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress, Start);
    ASSERT_GE (mMemoryTypeStatisticsSortedByAddress[Index].MaximumAddress, End);

    Start = mMemoryTypeStatisticsSortedByAddress[Index].MaximumAddress + 1;
    End   = Start + EFI_PAGE_SIZE - 1;
  }

  // Make sure we have at least one special memory type for the test
  ASSERT_GT (Index, (UINTN)0);

  // Make sure the rest of the array is zeroed out
  for ( ; Index <= EfiMaxMemoryType; Index++) {
    ASSERT_EQ (mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages, (UINTN)0);
  }

  return;
}

// Test CoreAddRange() API for a single memory range that does not overlap
// with existing ranges.
TEST_F (CoreAddRangeTest, AddRangeStandaloneBin) {
  LIST_ENTRY  *Link;
  MEMORY_MAP  *MemDescEntry;
  UINTN       Counter = 0;

  CoreAcquireMemoryLock ();
  CoreAddRange (
    EfiBootServicesData,
    (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB - EFI_PAGE_SIZE),
    (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB - 1),
    EFI_MEMORY_UC | EFI_MEMORY_WP
    );
  CoreReleaseMemoryLock ();

  // There should be one and only one entry in the memory map now
  BASE_LIST_FOR_EACH (Link, &gMemoryMap) {
    MemDescEntry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);

    ASSERT_EQ (MemDescEntry->Type, EfiBootServicesData);
    ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)((EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB - EFI_PAGE_SIZE)));
    ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB - 1));
    ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);

    Counter++;
  }

  ASSERT_EQ (Counter, (UINTN)1);
}

// Test CoreAddRange() API that partially overlaps with the head of an existing bin range.
TEST_F (CoreAddRangeTest, AddRangeHeadOverlap) {
  LIST_ENTRY  *Link;
  MEMORY_MAP  *MemDescEntry;
  UINTN       Counter = 0;

  CoreAcquireMemoryLock ();
  CoreAddRange (
    EfiBootServicesData,
    (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB - EFI_PAGE_SIZE,
    (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB + EFI_PAGE_SIZE - 1,
    EFI_MEMORY_UC | EFI_MEMORY_WP
    );
  CoreReleaseMemoryLock ();

  // There should be two entries in the memory map now
  BASE_LIST_FOR_EACH (Link, &gMemoryMap) {
    MemDescEntry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);

    if (Counter == 0) {
      ASSERT_EQ (MemDescEntry->Type, EfiBootServicesData);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)((EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB - EFI_PAGE_SIZE)));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 1) {
      ASSERT_EQ (MemDescEntry->Type, EfiBootServicesData);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB);
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else {
      FAIL () << "Unexpected extra entry in memory map";
    }

    Counter++;
  }

  ASSERT_EQ (Counter, (UINTN)2);
}

// Test CoreAddRange() API that partially overlaps with the tail of an existing bin range.
TEST_F (CoreAddRangeTest, AddRangeTailOverlap) {
  LIST_ENTRY  *Link;
  MEMORY_MAP  *MemDescEntry;
  UINTN       Counter = 0;

  CoreAcquireMemoryLock ();
  CoreAddRange (
    EfiBootServicesData,
    (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB,
    (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE * 2 - 1),
    EFI_MEMORY_UC | EFI_MEMORY_WP
    );

  // There should be two entries in the memory map now
  BASE_LIST_FOR_EACH (Link, &gMemoryMap) {
    MemDescEntry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);

    if (Counter == 0) {
      ASSERT_EQ (MemDescEntry->Type, EfiBootServicesData);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB);
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 1) {
      ASSERT_EQ (MemDescEntry->Type, EfiBootServicesData);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE * 2 - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else {
      FAIL () << "Unexpected extra entry in memory map";
    }

    Counter++;
  }

  ASSERT_EQ (Counter, (UINTN)2);

  CoreReleaseMemoryLock ();
}

// Test CoreAddRange() API that overlaps with both the head and tail of an existing bin range.
TEST_F (CoreAddRangeTest, AddRangeHeadTailOverlap) {
  LIST_ENTRY  *Link;
  MEMORY_MAP  *MemDescEntry;
  UINTN       Counter = 0;

  CoreAcquireMemoryLock ();
  CoreAddRange (
    EfiBootServicesData,
    (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB - EFI_PAGE_SIZE,
    (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE * 2 - 1),
    EFI_MEMORY_UC | EFI_MEMORY_WP
    );
  CoreReleaseMemoryLock ();

  // There should be three entries in the memory map now
  BASE_LIST_FOR_EACH (Link, &gMemoryMap) {
    MemDescEntry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);

    if (Counter == 0) {
      ASSERT_EQ (MemDescEntry->Type, EfiBootServicesData);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)((EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB - EFI_PAGE_SIZE)));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 1) {
      ASSERT_EQ (MemDescEntry->Type, EfiBootServicesData);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB);
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 2) {
      ASSERT_EQ (MemDescEntry->Type, EfiBootServicesData);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE * 2 - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else {
      FAIL () << "Unexpected extra entry in memory map";
    }

    Counter++;
  }

  ASSERT_EQ (Counter, (UINTN)3);
}

// Test CoreAddRange() API that just covers an existing range.
TEST_F (CoreAddRangeTest, AddRangeCoveringExisting) {
  LIST_ENTRY  *Link;
  MEMORY_MAP  *MemDescEntry;
  UINTN       Counter = 0;

  CoreAcquireMemoryLock ();
  CoreAddRange (
    EfiConventionalMemory,
    (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB,
    (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE - 1),
    EFI_MEMORY_UC | EFI_MEMORY_WP
    );
  CoreReleaseMemoryLock ();

  // Go the tail of gMemoryMap and make sure the new range is added correctly
  BASE_LIST_FOR_EACH (Link, &gMemoryMap) {
    MemDescEntry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);

    ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
    ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)((EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB));
    ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE - 1));
    ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    Counter++;
  }

  ASSERT_EQ (Counter, (UINTN)1);
}

// Test CoreAddRange() API that adds multiple ranges that needs to be splitted.
TEST_F (CoreAddRangeTest, AddRangeMultipleSplits) {
  LIST_ENTRY  *Link;
  MEMORY_MAP  *MemDescEntry;
  UINTN       Counter = 0;

  CoreAcquireMemoryLock ();
  CoreAddRange (
    EfiConventionalMemory,
    (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB - EFI_PAGE_SIZE,
    (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB * 2 + EFI_PAGE_SIZE * 2 - 1,
    EFI_MEMORY_UC | EFI_MEMORY_WP
    );
  CoreReleaseMemoryLock ();

  // Go the tail of gMemoryMap and make sure the new range is added correctly
  BASE_LIST_FOR_EACH (Link, &gMemoryMap) {
    MemDescEntry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);

    if (Counter == 0) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)((EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB - EFI_PAGE_SIZE)));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 1) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB);
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 2) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 3) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 + EFI_PAGE_SIZE - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 4) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 + EFI_PAGE_SIZE));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 + EFI_PAGE_SIZE * 2 - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else {
      FAIL () << "Unexpected extra entry in memory map";
    }

    Counter++;
  }

  // Should have created multiple entries due to splits
  ASSERT_EQ (Counter, (UINTN)5);
}

// Test CoreAddRange() API that adds multiple ranges that starts with non-bin range
// but ends at bin range.
TEST_F (CoreAddRangeTest, AddRangeNonBinToBin) {
  LIST_ENTRY  *Link;
  MEMORY_MAP  *MemDescEntry;
  UINTN       Counter = 0;

  CoreAcquireMemoryLock ();
  CoreAddRange (
    EfiConventionalMemory,
    (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB - EFI_PAGE_SIZE),
    (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 + EFI_PAGE_SIZE - 1),
    EFI_MEMORY_UC | EFI_MEMORY_WP
    );
  CoreReleaseMemoryLock ();

  // Go the tail of gMemoryMap and make sure the new range is added correctly
  BASE_LIST_FOR_EACH (Link, &gMemoryMap) {
    MemDescEntry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);

    if (Counter == 0) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)((EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB - EFI_PAGE_SIZE)));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 1) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB);
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 2) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 3) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 + EFI_PAGE_SIZE - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else {
      FAIL () << "Unexpected extra entry in memory map";
    }

    Counter++;
  }

  // Should have created multiple entries due to splits
  ASSERT_EQ (Counter, (UINTN)4);
}

// Test CoreAddRange() API that adds multiple ranges that starts with bin range
// but ends at non-bin range.
TEST_F (CoreAddRangeTest, AddRangeBinToNonBin) {
  LIST_ENTRY  *Link;
  MEMORY_MAP  *MemDescEntry;
  UINTN       Counter = 0;

  CoreAcquireMemoryLock ();
  CoreAddRange (
    EfiConventionalMemory,
    (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB,
    (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 + EFI_PAGE_SIZE * 2 - 1),
    EFI_MEMORY_UC | EFI_MEMORY_WP
    );
  CoreReleaseMemoryLock ();

  // Go the tail of gMemoryMap and make sure the new range is added correctly
  BASE_LIST_FOR_EACH (Link, &gMemoryMap) {
    MemDescEntry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);

    if (Counter == 0) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB);
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 1) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 2) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 + EFI_PAGE_SIZE - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 3) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 + EFI_PAGE_SIZE));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 + EFI_PAGE_SIZE * 2 - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else {
      FAIL () << "Unexpected extra entry in memory map";
    }

    Counter++;
  }

  // Should have created multiple entries due to splits
  ASSERT_EQ (Counter, (UINTN)4);
}

// Test CoreAddRange() API that adds multiple ranges that starts with bin range
// but ends at another bin range.
TEST_F (CoreAddRangeTest, AddRangeBinToBin) {
  LIST_ENTRY  *Link;
  MEMORY_MAP  *MemDescEntry;
  UINTN       Counter = 0;

  CoreAcquireMemoryLock ();
  CoreAddRange (
    EfiConventionalMemory,
    (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB,
    (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 + EFI_PAGE_SIZE - 1),
    EFI_MEMORY_UC | EFI_MEMORY_WP
    );
  CoreReleaseMemoryLock ();

  // Go the tail of gMemoryMap and make sure the new range is added correctly
  BASE_LIST_FOR_EACH (Link, &gMemoryMap) {
    MemDescEntry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);

    if (Counter == 0) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)((EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB)));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB + EFI_PAGE_SIZE - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 1) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB + EFI_PAGE_SIZE);
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else if (Counter == 2) {
      ASSERT_EQ (MemDescEntry->Type, EfiConventionalMemory);
      ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2));
      ASSERT_EQ (MemDescEntry->End, (EFI_PHYSICAL_ADDRESS)(UINTN)(SIZE_64KB * 2 + EFI_PAGE_SIZE - 1));
      ASSERT_EQ (MemDescEntry->Attribute, EFI_MEMORY_UC | EFI_MEMORY_WP);
    } else {
      FAIL () << "Unexpected extra entry in memory map";
    }

    Counter++;
  }

  // Should have created multiple entries due to splits
  ASSERT_EQ (Counter, (UINTN)3);
}

// Test CoreAddRange() API that adds a range that spans all special memory types.
// This will fail the call because the depth is not big enough to handle all splits.
TEST_F (CoreAddRangeTest, AddRangeAllSpecialTypes) {
  CoreAcquireMemoryLock ();
  EXPECT_THROW_MESSAGE (
    CoreAddRange (
      EfiBootServicesData,
      (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB - EFI_PAGE_SIZE,
      (EFI_PHYSICAL_ADDRESS)(UINTN)SIZE_64KB * 5 + EFI_PAGE_SIZE - 1,
      EFI_MEMORY_UC | EFI_MEMORY_WP
      ),
    "!(((RETURN_STATUS)(Status)) >= 0x8000000000000000ULL)"
    );

  CoreReleaseMemoryLock ();
}

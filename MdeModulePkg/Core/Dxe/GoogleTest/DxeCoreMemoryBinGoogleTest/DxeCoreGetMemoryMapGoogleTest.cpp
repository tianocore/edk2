/** @file
  Unit tests for the implementation of UefiSortLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Pi/PiDxeCis.h>

  #include <Library/BaseLib.h>
  #include <Library/UefiLib.h>
  #include <Library/DebugLib.h>
  #include <Library/BaseMemoryLib.h>

  #include <Guid/LoadModuleAtFixedAddress.h>
  #include <Guid/MemoryTypeInformation.h>
  #include <Guid/MemoryProfile.h>

  #include "../../Mem/Imem.h"
  #include "../../DxeMain.h"

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

  // set up a local buffer for the memory manage service
  alignas (EFI_PAGE_SIZE) UINT8 Buffer[SIZE_2MB];

  extern EFI_MEMORY_TYPE_STATISTICS  mMemoryTypeStatistics[EfiMaxMemoryType + 1];
  extern EFI_MEMORY_TYPE_STATISTICS  mMemoryTypeStatisticsSortedByAddress[EfiMaxMemoryType + 1];
  extern BOOLEAN                     mMemoryTypeInformationInitialized;
  extern LIST_ENTRY                  mFreeMemoryMapEntryList;
  extern UINTN                       mMapDepth;

  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *gMemoryAttributeProtocol = NULL;
}

using namespace testing;

class CoreGetMemoryMapTest : public  Test {
protected:
  EFI_STATUS Status;
  EFI_MEMORY_TYPE_STATISTICS MemoryTypeStatisticsCache[EfiMaxMemoryType + 1];

  /* Redefining the Test class's SetUp function for test fixtures. */
  void
  SetUp (
    ) override
  {
    UINTN  SpecialCount = 0;

    // And as the memory type index increases, the BaseAddress should also increase
    for (UINTN Index = 0; Index <= EfiMaxMemoryType; Index++) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: bucket %d: %lx - %lx, special: %d\n",
        __func__,
        mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex,
        mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress,
        mMemoryTypeStatisticsSortedByAddress[Index].MaximumAddress,
        mMemoryTypeStatisticsSortedByAddress[Index].Special
        ));
    }

    // Populate gMemoryTypeInformation to special memory types memory bins
    for (INTN Index = 0; Index <= EfiMaxMemoryType; Index++) {
      if ((Index == EfiPersistentMemory) || (Index == EfiUnacceptedMemoryType)) {
        // Skip PMEM and Unaccepted memory types for this test
        continue;
      }

      if (mMemoryTypeStatisticsSortedByAddress[Index].Special) {
        gMemoryTypeInformation[SpecialCount].Type = (UINT32)Index;
        // Intentionally make each bin size different
        gMemoryTypeInformation[SpecialCount].NumberOfPages = SpecialCount + 1;
        SpecialCount++;
      }
    }

    CopyMem (
      MemoryTypeStatisticsCache,
      mMemoryTypeStatisticsSortedByAddress,
      sizeof (MemoryTypeStatisticsCache)
      );

    CoreAddMemoryDescriptor (
      EfiConventionalMemory,
      (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer,
      EFI_SIZE_TO_PAGES (sizeof (Buffer)),
      EFI_MEMORY_UC | EFI_MEMORY_WP
      );
  }

  void
  TearDown (
    ) override
  {
    // Reset gMemoryTypeInformation
    for (UINTN Index = 0; Index <= EfiMaxMemoryType; Index++) {
      gMemoryTypeInformation[Index].Type          = EfiMaxMemoryType;
      gMemoryTypeInformation[Index].NumberOfPages = 0;
    }

    mMemoryTypeInformationInitialized = FALSE;

    // Clear the memory map
    mFreeMemoryMapEntryList = INITIALIZE_LIST_HEAD_VARIABLE (mFreeMemoryMapEntryList);
    gMemoryMap              = INITIALIZE_LIST_HEAD_VARIABLE (gMemoryMap);

    CopyMem (
      mMemoryTypeStatisticsSortedByAddress,
      MemoryTypeStatisticsCache,
      sizeof (MemoryTypeStatisticsCache)
      );

    for (UINTN Index = 0; Index <= EfiMaxMemoryType; Index++) {
      mMemoryTypeStatistics[Index].BaseAddress      = 0;
      mMemoryTypeStatistics[Index].MaximumAddress   = MAX_ALLOC_ADDRESS;
      mMemoryTypeStatistics[Index].NumberOfPages    = 0;
      mMemoryTypeStatistics[Index].InformationIndex = EfiMaxMemoryType;
      mMemoryTypeStatistics[Index].Special          = MemoryTypeStatisticsCache[Index].Special;
      mMemoryTypeStatistics[Index].Runtime          = MemoryTypeStatisticsCache[Index].Runtime;
    }

    mMapDepth = 0;
  }
};

// Verify that the mMemoryTypeStatisticsSortedByAddress array is initialized correctly
TEST_F (CoreGetMemoryMapTest, VerifyMemoryStatistics) {
  EFI_PHYSICAL_ADDRESS  Start = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
  EFI_PHYSICAL_ADDRESS  End   = Start + sizeof (Buffer) - 1;
  UINTN                 Index;
  MEMORY_MAP            *MemDescEntry;

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

    if (!mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages) {
      break;
    }

    ASSERT_GE (mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress, Start);
    ASSERT_LE (mMemoryTypeStatisticsSortedByAddress[Index].MaximumAddress, End);

    Start = mMemoryTypeStatisticsSortedByAddress[Index].MaximumAddress + 1;
    End  += EFI_PAGE_SIZE;
  }

  // Make sure we have at least one special memory type for the test
  ASSERT_GT (Index, (UINTN)0);

  // Go through the memory map entries, at this point, it should be one conventional
  // memory range per each memory bin, one page for the memory map, and one free
  // range for the remaining conventional memory.
  LIST_ENTRY  *Link;
  UINTN       Idx;

  BASE_LIST_FOR_EACH (Link, &gMemoryMap) {
    MemDescEntry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);

    DEBUG ((
      DEBUG_ERROR,
      "%a: MemDescEntry: Type %d, Phys %lx, Ends %lx, Attr %lx\n",
      __func__,
      MemDescEntry->Type,
      MemDescEntry->Start,
      MemDescEntry->End,
      MemDescEntry->Attribute
      ));

    for (Idx = 0; Idx < Index; Idx++) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Comparing with special bucket %d: %lx - %lx\n",
        __func__,
        mMemoryTypeStatisticsSortedByAddress[Idx].InformationIndex,
        mMemoryTypeStatisticsSortedByAddress[Idx].BaseAddress,
        mMemoryTypeStatisticsSortedByAddress[Idx].MaximumAddress
        ));
      if (MemDescEntry->Start == mMemoryTypeStatisticsSortedByAddress[Idx].BaseAddress) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Found matching special bucket %d\n",
          __func__,
          mMemoryTypeStatisticsSortedByAddress[Idx].InformationIndex
          ));
        ASSERT_EQ (
          MemDescEntry->End,
          mMemoryTypeStatisticsSortedByAddress[Idx].BaseAddress +
          mMemoryTypeStatisticsSortedByAddress[Idx].NumberOfPages * EFI_PAGE_SIZE - 1
          );
        // Clear the number of pages for this memory type to indicate it has been verified
        mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Idx].InformationIndex].NumberOfPages = 0;
        break;
      }
    }

    if (Idx == Index) {
      // Not found in the special memory types, it should be either the memory map page
      // or the remaining conventional memory
      if (MemDescEntry->Type == EfiConventionalMemory) {
        // This is the remaining conventional memory range
        ASSERT_EQ (MemDescEntry->Start, (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer);
        // The end should be before the first special memory type
        ASSERT_LT (MemDescEntry->End, mMemoryTypeStatisticsSortedByAddress[0].BaseAddress);
      } else if (MemDescEntry->Type == EfiBootServicesData) {
        // This should be the memory map page
        ASSERT_EQ (MemDescEntry->Start + EFI_PAGE_SIZE - 1, MemDescEntry->End);
      } else {
        ASSERT_TRUE (FALSE);
      }
    }
  }

  // Make sure all special memory types have been verified
  for (Idx = 0; Idx < Index; Idx++) {
    ASSERT_EQ (mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Idx].InformationIndex].NumberOfPages, (UINTN)0);
  }

  // Make sure the rest of the array is zeroed out
  for ( ; Index <= EfiMaxMemoryType; Index++) {
    ASSERT_EQ (mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages, (UINTN)0);
  }

  return;
}

// Test to allocate a special range, the resultant memory map should not change.
TEST_F (CoreGetMemoryMapTest, AllocateSpecialRange) {
  VOID                   *RtBuffer;
  UINTN                  MemoryMapSize;
  UINT8                  MemoryMap[SIZE_1KB];
  EFI_MEMORY_DESCRIPTOR  *EfiMemNext;
  UINT8                  *EfiMemoryMapEnd;
  UINTN                  EfiDescriptorSize;
  UINTN                  Index;

  Status = CoreAllocatePages (
             AllocateAnyPages,
             EfiRuntimeServicesCode,
             1,
             (EFI_PHYSICAL_ADDRESS *)(UINTN)&RtBuffer
             );

  ASSERT_EFI_ERROR (Status);
  ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)RtBuffer, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

  MemoryMapSize = sizeof (MemoryMap);
  Status        = CoreGetMemoryMap (
                    &MemoryMapSize,
                    (EFI_MEMORY_DESCRIPTOR *)MemoryMap,
                    NULL,
                    &EfiDescriptorSize,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);

  // The memory map should include:
  // 1. All special memory bins, including the one we just allocated
  // 2. The memory map page
  // 3. The remaining conventional memory
  EfiMemNext      = (EFI_MEMORY_DESCRIPTOR *)MemoryMap;
  EfiMemoryMapEnd = (UINT8 *)MemoryMap + MemoryMapSize;

  while ((UINT8 *)EfiMemNext < EfiMemoryMapEnd) {
    for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
      if (mMemoryTypeStatisticsSortedByAddress[Index].Special &&
          (EfiMemNext->PhysicalStart ==
           mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress))
      {
        // This is one of the special memory types
        ASSERT_EQ (
          EfiMemNext->NumberOfPages,
          mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages
          );
        ASSERT_EQ (
          EfiMemNext->Type,
          mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
          );
        break;
      }
    }

    if (Index < EfiMaxMemoryType) {
      // Found the special memory type, mark it as verified
      mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex].NumberOfPages = 0;
    } else {
      // Not found in the special memory types, it should be either the memory map page
      // or the remaining conventional memory
      if (EfiMemNext->Type == EfiConventionalMemory) {
        // This is the remaining conventional memory range
        ASSERT_EQ (EfiMemNext->PhysicalStart, (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer);
        // The end should be before the first special memory type
        ASSERT_LT (
          EfiMemNext->PhysicalStart + LShiftU64 (EfiMemNext->NumberOfPages, EFI_PAGE_SHIFT) - 1,
          mMemoryTypeStatisticsSortedByAddress[0].BaseAddress
          );
      } else if (EfiMemNext->Type == EfiBootServicesData) {
        // This should be the memory map page
        ASSERT_EQ (EfiMemNext->NumberOfPages, (UINT64)1);
      } else {
        ASSERT_TRUE (FALSE);
      }
    }

    // Move on to next descriptor
    EfiMemNext = NEXT_MEMORY_DESCRIPTOR (EfiMemNext, EfiDescriptorSize);

    ASSERT_LE ((UINT8 *)EfiMemNext, EfiMemoryMapEnd);
  }

  // Make sure all special memory types have been verified
  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    if (mMemoryTypeStatisticsSortedByAddress[Index].Special) {
      ASSERT_EQ (
        mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
        ].NumberOfPages,
        (UINTN)0
        );
    }
  }

  return;
}

// Test to allocate multiple special ranges, the resultant memory map should not change.
TEST_F (CoreGetMemoryMapTest, AllocateMultipleSpecialRange) {
  VOID                   *RtBuffer;
  VOID                   *AcpiBuffer;
  UINTN                  MemoryMapSize;
  UINT8                  MemoryMap[SIZE_1KB];
  EFI_MEMORY_DESCRIPTOR  *EfiMemNext;
  UINT8                  *EfiMemoryMapEnd;
  UINTN                  EfiDescriptorSize;
  UINTN                  Index;

  Status = CoreAllocatePages (
             AllocateAnyPages,
             EfiRuntimeServicesCode,
             1,
             (EFI_PHYSICAL_ADDRESS *)(UINTN)&RtBuffer
             );

  ASSERT_EFI_ERROR (Status);
  ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)RtBuffer, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

  Status = CoreAllocatePages (
             AllocateAnyPages,
             EfiACPIReclaimMemory,
             2,
             (EFI_PHYSICAL_ADDRESS *)(UINTN)&AcpiBuffer
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)AcpiBuffer, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

  MemoryMapSize = sizeof (MemoryMap);
  Status        = CoreGetMemoryMap (
                    &MemoryMapSize,
                    (EFI_MEMORY_DESCRIPTOR *)MemoryMap,
                    NULL,
                    &EfiDescriptorSize,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);

  // The memory map should include:
  // 1. All special memory bins, including the one we just allocated
  // 2. The memory map page
  // 3. The remaining conventional memory
  EfiMemNext      = (EFI_MEMORY_DESCRIPTOR *)MemoryMap;
  EfiMemoryMapEnd = (UINT8 *)MemoryMap + MemoryMapSize;

  while ((UINT8 *)EfiMemNext < EfiMemoryMapEnd) {
    for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
      if (mMemoryTypeStatisticsSortedByAddress[Index].Special &&
          (EfiMemNext->PhysicalStart ==
           mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress))
      {
        // This is one of the special memory types
        ASSERT_EQ (
          EfiMemNext->NumberOfPages,
          mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages
          );
        ASSERT_EQ (
          EfiMemNext->Type,
          mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
          );
        break;
      }
    }

    if (Index < EfiMaxMemoryType) {
      // Found the special memory type, mark it as verified
      mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex].NumberOfPages = 0;
    } else {
      // Not found in the special memory types, it should be either the memory map page
      // or the remaining conventional memory
      if (EfiMemNext->Type == EfiConventionalMemory) {
        // This is the remaining conventional memory range
        ASSERT_EQ (EfiMemNext->PhysicalStart, (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer);
        // The end should be before the first special memory type
        ASSERT_LT (
          EfiMemNext->PhysicalStart + LShiftU64 (EfiMemNext->NumberOfPages, EFI_PAGE_SHIFT) - 1,
          mMemoryTypeStatisticsSortedByAddress[0].BaseAddress
          );
      } else if (EfiMemNext->Type == EfiBootServicesData) {
        // This should be the memory map page
        ASSERT_EQ (EfiMemNext->NumberOfPages, (UINT64)1);
      } else {
        ASSERT_TRUE (FALSE);
      }
    }

    // Move on to next descriptor
    EfiMemNext = NEXT_MEMORY_DESCRIPTOR (EfiMemNext, EfiDescriptorSize);

    ASSERT_LE ((UINT8 *)EfiMemNext, EfiMemoryMapEnd);
  }

  // Make sure all special memory types have been verified
  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    if (mMemoryTypeStatisticsSortedByAddress[Index].Special) {
      ASSERT_EQ (
        mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
        ].NumberOfPages,
        (UINTN)0
        );
    }
  }

  return;
}

// Test to allocate multiple special ranges, the resultant memory map should not change.
TEST_F (CoreGetMemoryMapTest, AllocateThenFreeSpecialRange) {
  VOID                   *RtBuffer;
  UINTN                  MemoryMapSize;
  UINT8                  MemoryMap[SIZE_1KB];
  EFI_MEMORY_DESCRIPTOR  *EfiMemNext;
  UINT8                  *EfiMemoryMapEnd;
  UINTN                  EfiDescriptorSize;
  UINTN                  Index;

  Status = CoreAllocatePages (
             AllocateAnyPages,
             EfiRuntimeServicesCode,
             1,
             (EFI_PHYSICAL_ADDRESS *)(UINTN)&RtBuffer
             );

  ASSERT_EFI_ERROR (Status);
  ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)RtBuffer, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

  Status = CoreFreePages (
             (EFI_PHYSICAL_ADDRESS)(UINTN)RtBuffer,
             1
             );
  ASSERT_EFI_ERROR (Status);

  MemoryMapSize = sizeof (MemoryMap);
  Status        = CoreGetMemoryMap (
                    &MemoryMapSize,
                    (EFI_MEMORY_DESCRIPTOR *)MemoryMap,
                    NULL,
                    &EfiDescriptorSize,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);

  // The memory map should include:
  // 1. All special memory bins, including the one we just allocated
  // 2. The memory map page
  // 3. The remaining conventional memory
  EfiMemNext      = (EFI_MEMORY_DESCRIPTOR *)MemoryMap;
  EfiMemoryMapEnd = (UINT8 *)MemoryMap + MemoryMapSize;

  while ((UINT8 *)EfiMemNext < EfiMemoryMapEnd) {
    for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
      if (mMemoryTypeStatisticsSortedByAddress[Index].Special &&
          (EfiMemNext->PhysicalStart ==
           mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress))
      {
        // This is one of the special memory types
        ASSERT_EQ (
          EfiMemNext->NumberOfPages,
          mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages
          );
        ASSERT_EQ (
          EfiMemNext->Type,
          mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
          );
        break;
      }
    }

    if (Index < EfiMaxMemoryType) {
      // Found the special memory type, mark it as verified
      mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex].NumberOfPages = 0;
    } else {
      // Not found in the special memory types, it should be either the memory map page
      // or the remaining conventional memory
      if (EfiMemNext->Type == EfiConventionalMemory) {
        // This is the remaining conventional memory range
        ASSERT_EQ (EfiMemNext->PhysicalStart, (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer);
        // The end should be before the first special memory type
        ASSERT_LT (
          EfiMemNext->PhysicalStart + LShiftU64 (EfiMemNext->NumberOfPages, EFI_PAGE_SHIFT) - 1,
          mMemoryTypeStatisticsSortedByAddress[0].BaseAddress
          );
      } else if (EfiMemNext->Type == EfiBootServicesData) {
        // This should be the memory map page
        ASSERT_EQ (EfiMemNext->NumberOfPages, (UINT64)1);
      } else {
        ASSERT_TRUE (FALSE);
      }
    }

    // Move on to next descriptor
    EfiMemNext = NEXT_MEMORY_DESCRIPTOR (EfiMemNext, EfiDescriptorSize);

    ASSERT_LE ((UINT8 *)EfiMemNext, EfiMemoryMapEnd);
  }

  // Make sure all special memory types have been verified
  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    if (mMemoryTypeStatisticsSortedByAddress[Index].Special) {
      ASSERT_EQ (
        mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
        ].NumberOfPages,
        (UINTN)0
        );
    }
  }

  return;
}

// Test to allocate multiple special ranges, the resultant memory map should include it.
TEST_F (CoreGetMemoryMapTest, AllocateNonSpecialRange) {
  VOID                   *BsBuffer;
  UINTN                  MemoryMapSize;
  UINT8                  MemoryMap[SIZE_1KB];
  EFI_MEMORY_DESCRIPTOR  *EfiMemNext;
  UINT8                  *EfiMemoryMapEnd;
  UINTN                  EfiDescriptorSize;
  UINTN                  Index;
  BOOLEAN                FoundBsBuffer         = FALSE;
  BOOLEAN                FoundFreeConventional = FALSE;
  BOOLEAN                FoundMemoryMapPage    = FALSE;

  Status = CoreAllocatePages (
             AllocateAnyPages,
             EfiBootServicesCode,
             1,
             (EFI_PHYSICAL_ADDRESS *)(UINTN)&BsBuffer
             );

  ASSERT_EFI_ERROR (Status);
  ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)BsBuffer, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

  MemoryMapSize = sizeof (MemoryMap);
  Status        = CoreGetMemoryMap (
                    &MemoryMapSize,
                    (EFI_MEMORY_DESCRIPTOR *)MemoryMap,
                    NULL,
                    &EfiDescriptorSize,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);

  // The memory map should include:
  // 1. All special memory bins, including the one we just allocated
  // 2. The memory map page
  // 3. The remaining conventional memory
  EfiMemNext      = (EFI_MEMORY_DESCRIPTOR *)MemoryMap;
  EfiMemoryMapEnd = (UINT8 *)MemoryMap + MemoryMapSize;

  while ((UINT8 *)EfiMemNext < EfiMemoryMapEnd) {
    for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
      if (mMemoryTypeStatisticsSortedByAddress[Index].Special &&
          (EfiMemNext->PhysicalStart ==
           mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress))
      {
        // This is one of the special memory types
        ASSERT_EQ (
          EfiMemNext->NumberOfPages,
          mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages
          );
        ASSERT_EQ (
          EfiMemNext->Type,
          mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
          );
        break;
      }
    }

    if (Index < EfiMaxMemoryType) {
      // Found the special memory type, mark it as verified
      mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex].NumberOfPages = 0;
    } else {
      // Not found in the special memory types, it should be either the memory map page
      // or the remaining conventional memory
      if (EfiMemNext->Type == EfiConventionalMemory) {
        // This is the remaining conventional memory range
        ASSERT_EQ (EfiMemNext->PhysicalStart, (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer);
        // The end should be before the first special memory type
        ASSERT_LT (
          EfiMemNext->PhysicalStart + LShiftU64 (EfiMemNext->NumberOfPages, EFI_PAGE_SHIFT) - 1,
          mMemoryTypeStatisticsSortedByAddress[0].BaseAddress
          );
        FoundFreeConventional = TRUE;
      } else if (EfiMemNext->Type == EfiBootServicesData) {
        // This should be the memory map page
        ASSERT_EQ (EfiMemNext->NumberOfPages, (UINT64)1);
        FoundMemoryMapPage = TRUE;
      } else if (EfiMemNext->Type == EfiBootServicesCode) {
        // This should be the newly allocated non-special range
        ASSERT_EQ (EfiMemNext->PhysicalStart, (EFI_PHYSICAL_ADDRESS)(UINTN)BsBuffer);
        ASSERT_EQ (EfiMemNext->NumberOfPages, (UINT64)1);
        FoundBsBuffer = TRUE;
      } else {
        ASSERT_TRUE (FALSE);
      }
    }

    // Move on to next descriptor
    EfiMemNext = NEXT_MEMORY_DESCRIPTOR (EfiMemNext, EfiDescriptorSize);

    ASSERT_LE ((UINT8 *)EfiMemNext, EfiMemoryMapEnd);
  }

  ASSERT_TRUE (FoundBsBuffer);
  ASSERT_TRUE (FoundFreeConventional);
  ASSERT_TRUE (FoundMemoryMapPage);

  // Make sure all special memory types have been verified
  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    if (mMemoryTypeStatisticsSortedByAddress[Index].Special) {
      ASSERT_EQ (
        mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
        ].NumberOfPages,
        (UINTN)0
        );
    }
  }

  return;
}

// Test to allocate then free a non-special range, the resultant memory map should include it.
TEST_F (CoreGetMemoryMapTest, AllocateThenFreeNonSpecialRange) {
  VOID                   *BsBuffer;
  UINTN                  MemoryMapSize;
  UINT8                  MemoryMap[SIZE_1KB];
  EFI_MEMORY_DESCRIPTOR  *EfiMemNext;
  UINT8                  *EfiMemoryMapEnd;
  UINTN                  EfiDescriptorSize;
  UINTN                  Index;
  BOOLEAN                FoundFreeConventional = FALSE;
  BOOLEAN                FoundMemoryMapPage    = FALSE;

  Status = CoreAllocatePages (
             AllocateAnyPages,
             EfiBootServicesCode,
             1,
             (EFI_PHYSICAL_ADDRESS *)(UINTN)&BsBuffer
             );

  ASSERT_EFI_ERROR (Status);
  ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)BsBuffer, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

  Status = CoreFreePages (
             (EFI_PHYSICAL_ADDRESS)(UINTN)BsBuffer,
             1
             );
  ASSERT_EFI_ERROR (Status);

  MemoryMapSize = sizeof (MemoryMap);
  Status        = CoreGetMemoryMap (
                    &MemoryMapSize,
                    (EFI_MEMORY_DESCRIPTOR *)MemoryMap,
                    NULL,
                    &EfiDescriptorSize,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);

  // The memory map should include:
  // 1. All special memory bins, including the one we just allocated
  // 2. The memory map page
  // 3. The remaining conventional memory
  EfiMemNext      = (EFI_MEMORY_DESCRIPTOR *)MemoryMap;
  EfiMemoryMapEnd = (UINT8 *)MemoryMap + MemoryMapSize;

  while ((UINT8 *)EfiMemNext < EfiMemoryMapEnd) {
    for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
      if (mMemoryTypeStatisticsSortedByAddress[Index].Special &&
          (EfiMemNext->PhysicalStart ==
           mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress))
      {
        // This is one of the special memory types
        ASSERT_EQ (
          EfiMemNext->NumberOfPages,
          mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages
          );
        ASSERT_EQ (
          EfiMemNext->Type,
          mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
          );
        break;
      }
    }

    if (Index < EfiMaxMemoryType) {
      // Found the special memory type, mark it as verified
      mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex].NumberOfPages = 0;
    } else {
      // Not found in the special memory types, it should be either the memory map page
      // or the remaining conventional memory
      if (EfiMemNext->Type == EfiConventionalMemory) {
        // This is the remaining conventional memory range
        ASSERT_EQ (EfiMemNext->PhysicalStart, (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer);
        // The end should be before the first special memory type
        ASSERT_LT (
          EfiMemNext->PhysicalStart + LShiftU64 (EfiMemNext->NumberOfPages, EFI_PAGE_SHIFT) - 1,
          mMemoryTypeStatisticsSortedByAddress[0].BaseAddress
          );
        FoundFreeConventional = TRUE;
      } else if (EfiMemNext->Type == EfiBootServicesData) {
        // This should be the memory map page
        ASSERT_EQ (EfiMemNext->NumberOfPages, (UINT64)1);
        FoundMemoryMapPage = TRUE;
      } else {
        ASSERT_TRUE (FALSE);
      }
    }

    // Move on to next descriptor
    EfiMemNext = NEXT_MEMORY_DESCRIPTOR (EfiMemNext, EfiDescriptorSize);

    ASSERT_LE ((UINT8 *)EfiMemNext, EfiMemoryMapEnd);
  }

  ASSERT_TRUE (FoundFreeConventional);
  ASSERT_TRUE (FoundMemoryMapPage);

  // Make sure all special memory types have been verified
  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    if (mMemoryTypeStatisticsSortedByAddress[Index].Special) {
      ASSERT_EQ (
        mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
        ].NumberOfPages,
        (UINTN)0
        );
    }
  }

  return;
}

// Test to allocate a non-special range followed by a special range, the resultant memory map should only include the non special range.
TEST_F (CoreGetMemoryMapTest, AllocateNonSpecialThenSpecialRange) {
  VOID                   *BsBuffer;
  VOID                   *RtBuffer;
  UINTN                  MemoryMapSize;
  UINT8                  MemoryMap[SIZE_1KB];
  EFI_MEMORY_DESCRIPTOR  *EfiMemNext;
  UINT8                  *EfiMemoryMapEnd;
  UINTN                  EfiDescriptorSize;
  UINTN                  Index;
  BOOLEAN                FoundBsBuffer         = FALSE;
  BOOLEAN                FoundFreeConventional = FALSE;
  BOOLEAN                FoundMemoryMapPage    = FALSE;

  Status = CoreAllocatePages (
             AllocateAnyPages,
             EfiBootServicesCode,
             1,
             (EFI_PHYSICAL_ADDRESS *)(UINTN)&BsBuffer
             );

  ASSERT_EFI_ERROR (Status);
  ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)BsBuffer, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

  Status = CoreAllocatePages (
             AllocateAnyPages,
             EfiRuntimeServicesCode,
             1,
             (EFI_PHYSICAL_ADDRESS *)(UINTN)&RtBuffer
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)RtBuffer, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

  MemoryMapSize = sizeof (MemoryMap);
  Status        = CoreGetMemoryMap (
                    &MemoryMapSize,
                    (EFI_MEMORY_DESCRIPTOR *)MemoryMap,
                    NULL,
                    &EfiDescriptorSize,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);

  // The memory map should include:
  // 1. All special memory bins, including the one we just allocated
  // 2. The memory map page
  // 3. The remaining conventional memory
  EfiMemNext      = (EFI_MEMORY_DESCRIPTOR *)MemoryMap;
  EfiMemoryMapEnd = (UINT8 *)MemoryMap + MemoryMapSize;

  while ((UINT8 *)EfiMemNext < EfiMemoryMapEnd) {
    for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
      if (mMemoryTypeStatisticsSortedByAddress[Index].Special &&
          (EfiMemNext->PhysicalStart ==
           mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress))
      {
        // This is one of the special memory types
        ASSERT_EQ (
          EfiMemNext->NumberOfPages,
          mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages
          );
        ASSERT_EQ (
          EfiMemNext->Type,
          mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
          );
        break;
      }
    }

    if (Index < EfiMaxMemoryType) {
      // Found the special memory type, mark it as verified
      mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex].NumberOfPages = 0;
    } else {
      // Not found in the special memory types, it should be either the memory map page
      // or the remaining conventional memory
      if (EfiMemNext->Type == EfiConventionalMemory) {
        // This is the remaining conventional memory range
        ASSERT_EQ (EfiMemNext->PhysicalStart, (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer);
        // The end should be before the first special memory type
        ASSERT_LT (
          EfiMemNext->PhysicalStart + LShiftU64 (EfiMemNext->NumberOfPages, EFI_PAGE_SHIFT) - 1,
          mMemoryTypeStatisticsSortedByAddress[0].BaseAddress
          );
        FoundFreeConventional = TRUE;
      } else if (EfiMemNext->Type == EfiBootServicesData) {
        // This should be the memory map page
        ASSERT_EQ (EfiMemNext->NumberOfPages, (UINT64)1);
        FoundMemoryMapPage = TRUE;
      } else if (EfiMemNext->Type == EfiBootServicesCode) {
        // This should be the newly allocated non-special range
        ASSERT_EQ (EfiMemNext->PhysicalStart, (EFI_PHYSICAL_ADDRESS)(UINTN)BsBuffer);
        ASSERT_EQ (EfiMemNext->NumberOfPages, (UINT64)1);
        FoundBsBuffer = TRUE;
      } else {
        ASSERT_TRUE (FALSE);
      }
    }

    // Move on to next descriptor
    EfiMemNext = NEXT_MEMORY_DESCRIPTOR (EfiMemNext, EfiDescriptorSize);

    ASSERT_LE ((UINT8 *)EfiMemNext, EfiMemoryMapEnd);
  }

  ASSERT_TRUE (FoundBsBuffer);
  ASSERT_TRUE (FoundFreeConventional);
  ASSERT_TRUE (FoundMemoryMapPage);

  // Make sure all special memory types have been verified
  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    if (mMemoryTypeStatisticsSortedByAddress[Index].Special) {
      ASSERT_EQ (
        mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
        ].NumberOfPages,
        (UINTN)0
        );
    }
  }

  return;
}

// Test to allocate a non-special range followed by a special range and its freeing,
// the resultant memory map should only include the non special range.
TEST_F (CoreGetMemoryMapTest, AllocateNonSpecialThenSpecialAndFreeRange) {
  VOID                   *BsBuffer;
  VOID                   *RtBuffer;
  UINTN                  MemoryMapSize;
  UINT8                  MemoryMap[SIZE_1KB];
  EFI_MEMORY_DESCRIPTOR  *EfiMemNext;
  UINT8                  *EfiMemoryMapEnd;
  UINTN                  EfiDescriptorSize;
  UINTN                  Index;
  BOOLEAN                FoundBsBuffer         = FALSE;
  BOOLEAN                FoundFreeConventional = FALSE;
  BOOLEAN                FoundMemoryMapPage    = FALSE;

  Status = CoreAllocatePages (
             AllocateAnyPages,
             EfiBootServicesCode,
             1,
             (EFI_PHYSICAL_ADDRESS *)(UINTN)&BsBuffer
             );

  ASSERT_EFI_ERROR (Status);
  ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)BsBuffer, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

  Status = CoreAllocatePages (
             AllocateAnyPages,
             EfiRuntimeServicesCode,
             1,
             (EFI_PHYSICAL_ADDRESS *)(UINTN)&RtBuffer
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)RtBuffer, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

  Status = CoreFreePages (
             (EFI_PHYSICAL_ADDRESS)(UINTN)RtBuffer,
             1
             );
  ASSERT_EFI_ERROR (Status);

  MemoryMapSize = sizeof (MemoryMap);
  Status        = CoreGetMemoryMap (
                    &MemoryMapSize,
                    (EFI_MEMORY_DESCRIPTOR *)MemoryMap,
                    NULL,
                    &EfiDescriptorSize,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);

  // The memory map should include:
  // 1. All special memory bins, including the one we just allocated
  // 2. The memory map page
  // 3. The remaining conventional memory
  EfiMemNext      = (EFI_MEMORY_DESCRIPTOR *)MemoryMap;
  EfiMemoryMapEnd = (UINT8 *)MemoryMap + MemoryMapSize;

  while ((UINT8 *)EfiMemNext < EfiMemoryMapEnd) {
    for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
      if (mMemoryTypeStatisticsSortedByAddress[Index].Special &&
          (EfiMemNext->PhysicalStart ==
           mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress))
      {
        // This is one of the special memory types
        ASSERT_EQ (
          EfiMemNext->NumberOfPages,
          mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages
          );
        ASSERT_EQ (
          EfiMemNext->Type,
          mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
          );
        break;
      }
    }

    if (Index < EfiMaxMemoryType) {
      // Found the special memory type, mark it as verified
      mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex].NumberOfPages = 0;
    } else {
      // Not found in the special memory types, it should be either the memory map page
      // or the remaining conventional memory
      if (EfiMemNext->Type == EfiConventionalMemory) {
        // This is the remaining conventional memory range
        ASSERT_EQ (EfiMemNext->PhysicalStart, (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer);
        // The end should be before the first special memory type
        ASSERT_LT (
          EfiMemNext->PhysicalStart + LShiftU64 (EfiMemNext->NumberOfPages, EFI_PAGE_SHIFT) - 1,
          mMemoryTypeStatisticsSortedByAddress[0].BaseAddress
          );
        FoundFreeConventional = TRUE;
      } else if (EfiMemNext->Type == EfiBootServicesData) {
        // This should be the memory map page
        ASSERT_EQ (EfiMemNext->NumberOfPages, (UINT64)1);
        FoundMemoryMapPage = TRUE;
      } else if (EfiMemNext->Type == EfiBootServicesCode) {
        // This should be the newly allocated non-special range
        ASSERT_EQ (EfiMemNext->PhysicalStart, (EFI_PHYSICAL_ADDRESS)(UINTN)BsBuffer);
        ASSERT_EQ (EfiMemNext->NumberOfPages, (UINT64)1);
        FoundBsBuffer = TRUE;
      } else {
        ASSERT_TRUE (FALSE);
      }
    }

    // Move on to next descriptor
    EfiMemNext = NEXT_MEMORY_DESCRIPTOR (EfiMemNext, EfiDescriptorSize);

    ASSERT_LE ((UINT8 *)EfiMemNext, EfiMemoryMapEnd);
  }

  ASSERT_TRUE (FoundBsBuffer);
  ASSERT_TRUE (FoundFreeConventional);
  ASSERT_TRUE (FoundMemoryMapPage);

  // Make sure all special memory types have been verified
  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    if (mMemoryTypeStatisticsSortedByAddress[Index].Special) {
      ASSERT_EQ (
        mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
        ].NumberOfPages,
        (UINTN)0
        );
    }
  }

  return;
}

// Test to allocate a non-special range and its free followed by a special range allocation,
// the resultant memory map should only include the non special range.
TEST_F (CoreGetMemoryMapTest, AllocateNonSpecialAndFreeThenSpecialRange) {
  VOID                   *BsBuffer;
  VOID                   *RtBuffer;
  UINTN                  MemoryMapSize;
  UINT8                  MemoryMap[SIZE_1KB];
  EFI_MEMORY_DESCRIPTOR  *EfiMemNext;
  UINT8                  *EfiMemoryMapEnd;
  UINTN                  EfiDescriptorSize;
  UINTN                  Index;
  BOOLEAN                FoundFreeConventional = FALSE;
  BOOLEAN                FoundMemoryMapPage    = FALSE;

  Status = CoreAllocatePages (
             AllocateAnyPages,
             EfiBootServicesCode,
             1,
             (EFI_PHYSICAL_ADDRESS *)(UINTN)&BsBuffer
             );

  ASSERT_EFI_ERROR (Status);
  ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)BsBuffer, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

  Status = CoreAllocatePages (
             AllocateAnyPages,
             EfiRuntimeServicesCode,
             1,
             (EFI_PHYSICAL_ADDRESS *)(UINTN)&RtBuffer
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)RtBuffer, (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

  Status = CoreFreePages (
             (EFI_PHYSICAL_ADDRESS)(UINTN)BsBuffer,
             1
             );
  ASSERT_EFI_ERROR (Status);

  MemoryMapSize = sizeof (MemoryMap);
  Status        = CoreGetMemoryMap (
                    &MemoryMapSize,
                    (EFI_MEMORY_DESCRIPTOR *)MemoryMap,
                    NULL,
                    &EfiDescriptorSize,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);

  // The memory map should include:
  // 1. All special memory bins, including the one we just allocated
  // 2. The memory map page
  // 3. The remaining conventional memory
  EfiMemNext      = (EFI_MEMORY_DESCRIPTOR *)MemoryMap;
  EfiMemoryMapEnd = (UINT8 *)MemoryMap + MemoryMapSize;

  while ((UINT8 *)EfiMemNext < EfiMemoryMapEnd) {
    for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
      if (mMemoryTypeStatisticsSortedByAddress[Index].Special &&
          (EfiMemNext->PhysicalStart ==
           mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress))
      {
        // This is one of the special memory types
        ASSERT_EQ (
          EfiMemNext->NumberOfPages,
          mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages
          );
        ASSERT_EQ (
          EfiMemNext->Type,
          mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
          );
        break;
      }
    }

    if (Index < EfiMaxMemoryType) {
      // Found the special memory type, mark it as verified
      mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex].NumberOfPages = 0;
    } else {
      // Not found in the special memory types, it should be either the memory map page
      // or the remaining conventional memory
      if (EfiMemNext->Type == EfiConventionalMemory) {
        // This is the remaining conventional memory range
        ASSERT_EQ (EfiMemNext->PhysicalStart, (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer);
        // The end should be before the first special memory type
        ASSERT_LT (
          EfiMemNext->PhysicalStart + LShiftU64 (EfiMemNext->NumberOfPages, EFI_PAGE_SHIFT) - 1,
          mMemoryTypeStatisticsSortedByAddress[0].BaseAddress
          );
        FoundFreeConventional = TRUE;
      } else if (EfiMemNext->Type == EfiBootServicesData) {
        // This should be the memory map page
        ASSERT_EQ (EfiMemNext->NumberOfPages, (UINT64)1);
        FoundMemoryMapPage = TRUE;
      } else {
        ASSERT_TRUE (FALSE);
      }
    }

    // Move on to next descriptor
    EfiMemNext = NEXT_MEMORY_DESCRIPTOR (EfiMemNext, EfiDescriptorSize);

    ASSERT_LE ((UINT8 *)EfiMemNext, EfiMemoryMapEnd);
  }

  ASSERT_TRUE (FoundFreeConventional);
  ASSERT_TRUE (FoundMemoryMapPage);

  // Make sure all special memory types have been verified
  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    if (mMemoryTypeStatisticsSortedByAddress[Index].Special) {
      ASSERT_EQ (
        mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
        ].NumberOfPages,
        (UINTN)0
        );
    }
  }

  return;
}

// Test to allocate all special ranges, the resultant memory map should not be merged into conventional memory.
TEST_F (CoreGetMemoryMapTest, AllocateAllSpecialRanges) {
  VOID                   *SpecialBuffer[EfiMaxMemoryType + 1];
  UINTN                  MemoryMapSize;
  UINT8                  MemoryMap[SIZE_1KB];
  EFI_MEMORY_DESCRIPTOR  *EfiMemNext;
  UINT8                  *EfiMemoryMapEnd;
  UINTN                  EfiDescriptorSize;
  UINTN                  Index;
  BOOLEAN                FoundFreeConventional = FALSE;
  BOOLEAN                FoundMemoryMapPage    = FALSE;

  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    if ((Index == EfiPersistentMemory) || (Index == EfiUnacceptedMemoryType)) {
      continue;
    }

    if (!mMemoryTypeStatisticsSortedByAddress[Index].Special) {
      continue;
    }

    Status = CoreAllocatePages (
               AllocateAnyPages,
               (EFI_MEMORY_TYPE)mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex,
               1,
               (EFI_PHYSICAL_ADDRESS *)(UINTN)&SpecialBuffer[Index]
               );
    ASSERT_EFI_ERROR (Status);
    ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)SpecialBuffer[Index], (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);
  }

  MemoryMapSize = sizeof (MemoryMap);
  Status        = CoreGetMemoryMap (
                    &MemoryMapSize,
                    (EFI_MEMORY_DESCRIPTOR *)MemoryMap,
                    NULL,
                    &EfiDescriptorSize,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);

  // The memory map should include:
  // 1. All special memory bins, including the one we just allocated
  // 2. The memory map page
  // 3. The remaining conventional memory
  EfiMemNext      = (EFI_MEMORY_DESCRIPTOR *)MemoryMap;
  EfiMemoryMapEnd = (UINT8 *)MemoryMap + MemoryMapSize;

  while ((UINT8 *)EfiMemNext < EfiMemoryMapEnd) {
    for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
      if (mMemoryTypeStatisticsSortedByAddress[Index].Special &&
          (EfiMemNext->PhysicalStart ==
           mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress))
      {
        // This is one of the special memory types
        ASSERT_EQ (
          EfiMemNext->NumberOfPages,
          mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages
          );
        ASSERT_EQ (
          EfiMemNext->Type,
          mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
          );
        break;
      }
    }

    if (Index < EfiMaxMemoryType) {
      // Found the special memory type, mark it as verified
      mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex].NumberOfPages = 0;
    } else {
      // Not found in the special memory types, it should be either the memory map page
      // or the remaining conventional memory
      if (EfiMemNext->Type == EfiConventionalMemory) {
        // This is the remaining conventional memory range
        ASSERT_EQ (EfiMemNext->PhysicalStart, (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer);
        // The end should be before the first special memory type
        ASSERT_LT (
          EfiMemNext->PhysicalStart + LShiftU64 (EfiMemNext->NumberOfPages, EFI_PAGE_SHIFT) - 1,
          mMemoryTypeStatisticsSortedByAddress[0].BaseAddress
          );
        FoundFreeConventional = TRUE;
      } else if (EfiMemNext->Type == EfiBootServicesData) {
        // This should be the memory map page
        ASSERT_EQ (EfiMemNext->NumberOfPages, (UINT64)1);
        FoundMemoryMapPage = TRUE;
      } else {
        ASSERT_TRUE (FALSE);
      }
    }

    // Move on to next descriptor
    EfiMemNext = NEXT_MEMORY_DESCRIPTOR (EfiMemNext, EfiDescriptorSize);

    ASSERT_LE ((UINT8 *)EfiMemNext, EfiMemoryMapEnd);
  }

  ASSERT_TRUE (FoundFreeConventional);
  ASSERT_TRUE (FoundMemoryMapPage);

  // Make sure all special memory types have been verified
  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    if (mMemoryTypeStatisticsSortedByAddress[Index].Special) {
      ASSERT_EQ (
        mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
        ].NumberOfPages,
        (UINTN)0
        );
    }
  }

  return;
}

// Test to allocate all special ranges, the resultant memory map should not be merged into conventional memory.
TEST_F (CoreGetMemoryMapTest, AllocateAndFreeAllSpecialRanges) {
  VOID                   *SpecialBuffer[EfiMaxMemoryType + 1];
  UINTN                  MemoryMapSize;
  UINT8                  MemoryMap[SIZE_1KB];
  EFI_MEMORY_DESCRIPTOR  *EfiMemNext;
  UINT8                  *EfiMemoryMapEnd;
  UINTN                  EfiDescriptorSize;
  UINTN                  Index;
  BOOLEAN                FoundFreeConventional = FALSE;
  BOOLEAN                FoundMemoryMapPage    = FALSE;

  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    if ((Index == EfiPersistentMemory) || (Index == EfiUnacceptedMemoryType)) {
      continue;
    }

    if (!mMemoryTypeStatisticsSortedByAddress[Index].Special) {
      continue;
    }

    Status = CoreAllocatePages (
               AllocateAnyPages,
               (EFI_MEMORY_TYPE)mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex,
               1,
               (EFI_PHYSICAL_ADDRESS *)(UINTN)&SpecialBuffer[Index]
               );
    ASSERT_EFI_ERROR (Status);
    ASSERT_NE ((EFI_PHYSICAL_ADDRESS)(UINTN)SpecialBuffer[Index], (EFI_PHYSICAL_ADDRESS)(UINTN)NULL);

    Status = CoreFreePages (
               (EFI_PHYSICAL_ADDRESS)(UINTN)SpecialBuffer[Index],
               1
               );
    ASSERT_EFI_ERROR (Status);
  }

  MemoryMapSize = sizeof (MemoryMap);
  Status        = CoreGetMemoryMap (
                    &MemoryMapSize,
                    (EFI_MEMORY_DESCRIPTOR *)MemoryMap,
                    NULL,
                    &EfiDescriptorSize,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);

  // The memory map should include:
  // 1. All special memory bins, including the one we just allocated
  // 2. The memory map page
  // 3. The remaining conventional memory
  EfiMemNext      = (EFI_MEMORY_DESCRIPTOR *)MemoryMap;
  EfiMemoryMapEnd = (UINT8 *)MemoryMap + MemoryMapSize;

  while ((UINT8 *)EfiMemNext < EfiMemoryMapEnd) {
    for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
      if (mMemoryTypeStatisticsSortedByAddress[Index].Special &&
          (EfiMemNext->PhysicalStart ==
           mMemoryTypeStatisticsSortedByAddress[Index].BaseAddress))
      {
        // This is one of the special memory types
        ASSERT_EQ (
          EfiMemNext->NumberOfPages,
          mMemoryTypeStatisticsSortedByAddress[Index].NumberOfPages
          );
        ASSERT_EQ (
          EfiMemNext->Type,
          mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
          );
        break;
      }
    }

    if (Index < EfiMaxMemoryType) {
      // Found the special memory type, mark it as verified
      mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex].NumberOfPages = 0;
    } else {
      // Not found in the special memory types, it should be either the memory map page
      // or the remaining conventional memory
      if (EfiMemNext->Type == EfiConventionalMemory) {
        // This is the remaining conventional memory range
        ASSERT_EQ (EfiMemNext->PhysicalStart, (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer);
        // The end should be before the first special memory type
        ASSERT_LT (
          EfiMemNext->PhysicalStart + LShiftU64 (EfiMemNext->NumberOfPages, EFI_PAGE_SHIFT) - 1,
          mMemoryTypeStatisticsSortedByAddress[0].BaseAddress
          );
        FoundFreeConventional = TRUE;
      } else if (EfiMemNext->Type == EfiBootServicesData) {
        // This should be the memory map page
        ASSERT_EQ (EfiMemNext->NumberOfPages, (UINT64)1);
        FoundMemoryMapPage = TRUE;
      } else {
        ASSERT_TRUE (FALSE);
      }
    }

    // Move on to next descriptor
    EfiMemNext = NEXT_MEMORY_DESCRIPTOR (EfiMemNext, EfiDescriptorSize);

    ASSERT_LE ((UINT8 *)EfiMemNext, EfiMemoryMapEnd);
  }

  ASSERT_TRUE (FoundFreeConventional);
  ASSERT_TRUE (FoundMemoryMapPage);

  // Make sure all special memory types have been verified
  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    if (mMemoryTypeStatisticsSortedByAddress[Index].Special) {
      ASSERT_EQ (
        mMemoryTypeStatistics[mMemoryTypeStatisticsSortedByAddress[Index].InformationIndex
        ].NumberOfPages,
        (UINTN)0
        );
    }
  }

  return;
}

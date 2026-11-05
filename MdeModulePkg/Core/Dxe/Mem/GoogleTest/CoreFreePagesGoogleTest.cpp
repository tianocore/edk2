/** @file
  Tests for CoreFreePages().

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockHobLib.h>

extern "C" {
  #include "../../DxeMain.h"
  #include "../Imem.h"
  #include "../HeapGuard.h"

  extern UINTN                       mMemoryMapKey;
  extern UINTN                       mMapDepth;
  extern BOOLEAN                     mMemoryTypeInformationInitialized;
  extern EFI_MEMORY_TYPE_STATISTICS  mMemoryTypeStatistics[EfiMaxMemoryType + 1];

  EFI_LOCK                                    gMemoryLock = { TPL_NOTIFY, 0, EfiLockReleased };
  LIST_ENTRY                                  gMemoryMap  = INITIALIZE_LIST_HEAD_VARIABLE (gMemoryMap);
  EFI_MEMORY_ATTRIBUTE_PROTOCOL               *gMemoryAttributeProtocol;
  EFI_HANDLE                                  gDxeCoreImageHandle;
  EFI_LOAD_FIXED_ADDRESS_CONFIGURATION_TABLE  gLoadModuleAtFixAddressConfigurationTable;
  LIST_ENTRY                                  mGcdMemorySpaceMap = INITIALIZE_LIST_HEAD_VARIABLE (mGcdMemorySpaceMap);
  BOOLEAN                                     mOnGuarding        = FALSE;

  VOID
  CoreAcquireLock (
    IN EFI_LOCK  *Lock
    )
  {
    EXPECT_EQ (Lock->Lock, EfiLockReleased);
    Lock->Lock = EfiLockAcquired;
  }

  VOID
  CoreReleaseLock (
    IN EFI_LOCK  *Lock
    )
  {
    EXPECT_EQ (Lock->Lock, EfiLockAcquired);
    Lock->Lock = EfiLockReleased;
  }

  VOID
  CoreAcquireGcdMemoryLock (
    VOID
    )
  {
    ADD_FAILURE ();
  }

  VOID
  CoreReleaseGcdMemoryLock (
    VOID
    )
  {
    ADD_FAILURE ();
  }

  VOID
  CoreNotifySignalList (
    IN EFI_GUID  *EventGroup
    )
  {
    ADD_FAILURE ();
  }

  EFI_STATUS EFIAPI
  CoreGetMemorySpaceDescriptor (
    IN EFI_PHYSICAL_ADDRESS              BaseAddress,
    OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *Descriptor
    )
  {
    ADD_FAILURE ();
    return EFI_UNSUPPORTED;
  }

  EFI_STATUS EFIAPI
  CoreUpdateProfile (
    IN EFI_PHYSICAL_ADDRESS   CallerAddress,
    IN MEMORY_PROFILE_ACTION  Action,
    IN EFI_MEMORY_TYPE        MemoryType,
    IN UINTN                  Size,
    IN VOID                   *Buffer,
    IN CHAR8                  *ActionString OPTIONAL
    )
  {
    ADD_FAILURE ();
    return EFI_UNSUPPORTED;
  }

  VOID
  InstallMemoryAttributesTableOnMemoryAllocation (
    IN EFI_MEMORY_TYPE  MemoryType
    )
  {
    ADD_FAILURE ();
  }

  EFI_STATUS EFIAPI
  ApplyMemoryProtectionPolicy (
    IN EFI_MEMORY_TYPE       OldType,
    IN EFI_MEMORY_TYPE       NewType,
    IN EFI_PHYSICAL_ADDRESS  Memory,
    IN UINT64                Length
    )
  {
    ADD_FAILURE ();
    return EFI_UNSUPPORTED;
  }

  VOID
  MergeMemoryMap (
    IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
    IN OUT UINTN                  *MemoryMapSize,
    IN UINTN                      DescriptorSize
    )
  {
    ADD_FAILURE ();
  }

  EFI_STATUS
  CoreConvertPagesWithGuard (
    IN UINT64           Start,
    IN UINTN            NumberOfPages,
    IN EFI_MEMORY_TYPE  NewType
    )
  {
    ADD_FAILURE ();
    return EFI_UNSUPPORTED;
  }

  VOID
  SetGuardForMemory (
    IN EFI_PHYSICAL_ADDRESS  Memory,
    IN UINTN                 NumberOfPages
    )
  {
    ADD_FAILURE ();
  }

  UINT64
  AdjustMemoryS (
    IN OUT UINT64  *Start,
    IN UINT64      Size,
    IN OUT UINT64  *SizeRequested
    )
  {
    ADD_FAILURE ();
    return 0;
  }

  BOOLEAN
  IsPageTypeToGuard (
    IN EFI_MEMORY_TYPE    MemoryType,
    IN EFI_ALLOCATE_TYPE  AllocateType
    )
  {
    ADD_FAILURE ();
    return FALSE;
  }

  BOOLEAN EFIAPI
  IsMemoryGuarded (
    IN EFI_PHYSICAL_ADDRESS  Address
    )
  {
    ADD_FAILURE ();
    return FALSE;
  }

  VOID EFIAPI
  DumpGuardedMemoryBitmap (
    VOID
    )
  {
    ADD_FAILURE ();
  }

  BOOLEAN
  IsHeapGuardEnabled (
    UINT8  GuardType
    )
  {
    ADD_FAILURE ();
    return FALSE;
  }

  VOID EFIAPI
  GuardFreedPagesChecked (
    IN EFI_PHYSICAL_ADDRESS  BaseAddress,
    IN UINTN                 Pages
    )
  {
    ADD_FAILURE ();
  }

  BOOLEAN
  PromoteGuardedFreePages (
    OUT EFI_PHYSICAL_ADDRESS  *StartAddress,
    OUT EFI_PHYSICAL_ADDRESS  *EndAddress
    )
  {
    ADD_FAILURE ();
    return FALSE;
  }
}

struct AttributeResult {
  EFI_STATUS    Status;
  UINT64        Attributes;
};

class CoreFreePagesTest : public testing::TestWithParam<AttributeResult> {
protected:
  testing::StrictMock<MockHobLib> HobLib;
  MEMORY_MAP Entries[2];
  EFI_MEMORY_ATTRIBUTE_PROTOCOL Protocol;
  static UINTN AttributeQueries;
  static constexpr UINT64 Base = 0x100000;

  static EFI_STATUS EFIAPI
  GetMemoryAttributes (
    IN EFI_MEMORY_ATTRIBUTE_PROTOCOL  *This,
    IN EFI_PHYSICAL_ADDRESS           Address,
    IN UINT64                         Length,
    OUT UINT64                        *Attributes
    )
  {
    ++AttributeQueries;
    if (!EFI_ERROR (GetParam ().Status)) {
      *Attributes = GetParam ().Attributes;
    }

    return GetParam ().Status;
  }

  void
  SetUp (
    ) override
  {
    InitializeListHead (&gMemoryMap);
    ZeroMem (Entries, sizeof (Entries));
    ZeroMem (&Protocol, sizeof (Protocol));
    Protocol.GetMemoryAttributes      = GetMemoryAttributes;
    gMemoryAttributeProtocol          = &Protocol;
    gMemoryLock.Lock                  = EfiLockReleased;
    AttributeQueries                  = 0;
    mMemoryMapKey                     = 0;
    mMapDepth                         = 0;
    mMemoryTypeInformationInitialized = FALSE;
  }

  void
  TearDown (
    ) override
  {
    EXPECT_EQ (gMemoryLock.Lock, EfiLockReleased);
    gMemoryAttributeProtocol = NULL;
    InitializeListHead (&gMemoryMap);
  }

  void
  AddRange (
    UINTN            Index,
    EFI_MEMORY_TYPE  Type,
    UINT64           Start,
    UINTN            Pages
    )
  {
    Entries[Index].Signature = MEMORY_MAP_SIGNATURE;
    Entries[Index].Type      = Type;
    Entries[Index].Start     = Start;
    Entries[Index].End       = Start + EFI_PAGES_TO_SIZE (Pages) - 1;
    Entries[Index].Attribute = EFI_MEMORY_WB;
    InsertTailList (&gMemoryMap, &Entries[Index].Link);
  }

  void
  ExpectFree (
    UINT64      Address,
    UINTN       Pages,
    EFI_STATUS  ExpectedStatus,
    UINTN       ExpectedQueries = 1
    )
  {
    MEMORY_MAP                  OriginalEntries[2];
    LIST_ENTRY                  OriginalMap;
    EFI_MEMORY_TYPE_STATISTICS  OriginalStatistics[EfiMaxMemoryType + 1];

    CopyMem (OriginalEntries, Entries, sizeof (Entries));
    CopyMem (&OriginalMap, &gMemoryMap, sizeof (gMemoryMap));
    CopyMem (OriginalStatistics, mMemoryTypeStatistics, sizeof (OriginalStatistics));

    EXPECT_EQ (CoreFreePages (Address, Pages), ExpectedStatus);
    EXPECT_EQ (gMemoryLock.Lock, EfiLockReleased);
    EXPECT_EQ (AttributeQueries, ExpectedQueries);
    EXPECT_EQ (CompareMem (Entries, OriginalEntries, sizeof (Entries)), 0);
    EXPECT_EQ (CompareMem (&gMemoryMap, &OriginalMap, sizeof (gMemoryMap)), 0);
    EXPECT_EQ (CompareMem (mMemoryTypeStatistics, OriginalStatistics, sizeof (OriginalStatistics)), 0);
    EXPECT_EQ (mMemoryMapKey, 0U);
    EXPECT_EQ (mMapDepth, 0U);
    EXPECT_FALSE (mMemoryTypeInformationInitialized);
  }
};

UINTN  CoreFreePagesTest::AttributeQueries;

TEST_P (CoreFreePagesTest, RejectsUnalignedAddress) {
  AddRange (0, EfiBootServicesData, Base, 2);
  ExpectFree (Base + 1, 1, EFI_INVALID_PARAMETER, 0);
}

TEST_P (CoreFreePagesTest, RejectsZeroPages) {
  AddRange (0, EfiBootServicesData, Base, 2);
  ExpectFree (Base, 0, EFI_INVALID_PARAMETER, 0);
}

TEST_P (CoreFreePagesTest, RejectsUnmappedAddress) {
  ExpectFree (Base, 1, EFI_NOT_FOUND);
}

TEST_P (CoreFreePagesTest, RejectsAlreadyFreeMemory) {
  AddRange (0, EfiConventionalMemory, Base, 2);
  ExpectFree (Base, 1, EFI_NOT_FOUND);
}

TEST_P (CoreFreePagesTest, RejectsRangeExtendingPastAllocation) {
  AddRange (0, EfiBootServicesData, Base, 1);
  ExpectFree (Base, 2, EFI_NOT_FOUND);
}

TEST_P (CoreFreePagesTest, RejectsRangeContainingFreeMemory) {
  AddRange (0, EfiBootServicesData, Base, 1);
  AddRange (1, EfiConventionalMemory, Base + EFI_PAGE_SIZE, 1);
  ExpectFree (Base, 2, EFI_NOT_FOUND);
}

TEST_P (CoreFreePagesTest, RejectsRangeContainingGap) {
  AddRange (0, EfiBootServicesData, Base, 1);
  AddRange (1, EfiBootServicesData, Base + 2 * EFI_PAGE_SIZE, 1);
  ExpectFree (Base, 3, EFI_NOT_FOUND);
}

TEST_P (CoreFreePagesTest, LeaksWholeAllocationSuccessfully) {
  AddRange (0, EfiBootServicesData, Base, 2);
  ExpectFree (Base, 2, EFI_SUCCESS);
}

TEST_P (CoreFreePagesTest, LeaksPartialAllocationSuccessfully) {
  AddRange (0, EfiBootServicesData, Base, 3);
  ExpectFree (Base + EFI_PAGE_SIZE, 1, EFI_SUCCESS);
}

TEST_P (CoreFreePagesTest, LeaksRangeAcrossAllocatedDescriptorsSuccessfully) {
  AddRange (0, EfiLoaderData, Base, 1);
  AddRange (1, EfiBootServicesData, Base + EFI_PAGE_SIZE, 1);
  ExpectFree (Base, 2, EFI_SUCCESS);
}

TEST_P (CoreFreePagesTest, RejectsRuntimeUnalignedAddress) {
  if (RUNTIME_PAGE_ALLOCATION_GRANULARITY == EFI_PAGE_SIZE) {
    GTEST_SKIP () << "Runtime alignment equals page alignment on this architecture";
  }

  AddRange (0, EfiRuntimeServicesData, Base, EFI_SIZE_TO_PAGES (RUNTIME_PAGE_ALLOCATION_GRANULARITY));
  ExpectFree (Base + EFI_PAGE_SIZE, 1, EFI_INVALID_PARAMETER);
}

TEST_P (CoreFreePagesTest, LeaksRoundedRuntimeAllocationSuccessfully) {
  AddRange (0, EfiRuntimeServicesData, Base, EFI_SIZE_TO_PAGES (RUNTIME_PAGE_ALLOCATION_GRANULARITY));
  ExpectFree (Base, 1, EFI_SUCCESS);
}

TEST_P (CoreFreePagesTest, RejectsRoundedRuntimeRangeExtendingPastAllocation) {
  if (RUNTIME_PAGE_ALLOCATION_GRANULARITY == EFI_PAGE_SIZE) {
    GTEST_SKIP () << "Runtime rounding equals page rounding on this architecture";
  }

  AddRange (0, EfiRuntimeServicesData, Base, 1);
  ExpectFree (Base, 1, EFI_NOT_FOUND);
}

TEST_P (CoreFreePagesTest, RejectsRoundedRuntimeRangeContainingFreeMemory) {
  if (RUNTIME_PAGE_ALLOCATION_GRANULARITY == EFI_PAGE_SIZE) {
    GTEST_SKIP () << "Runtime rounding equals page rounding on this architecture";
  }

  AddRange (0, EfiRuntimeServicesData, Base, 1);
  AddRange (1, EfiConventionalMemory, Base + EFI_PAGE_SIZE, EFI_SIZE_TO_PAGES (RUNTIME_PAGE_ALLOCATION_GRANULARITY) - 1);
  ExpectFree (Base, 1, EFI_NOT_FOUND);
}

TEST_P (CoreFreePagesTest, LeaksRoundedRuntimeRangeAcrossAllocatedDescriptorsSuccessfully) {
  UINTN  RuntimePages;

  RuntimePages = EFI_SIZE_TO_PAGES (RUNTIME_PAGE_ALLOCATION_GRANULARITY);
  AddRange (0, EfiRuntimeServicesCode, Base, RuntimePages);
  AddRange (1, EfiRuntimeServicesData, Base + RUNTIME_PAGE_ALLOCATION_GRANULARITY, RuntimePages);
  ExpectFree (Base, RuntimePages + 1, EFI_SUCCESS);
}

INSTANTIATE_TEST_SUITE_P (
  ProtectedMemory,
  CoreFreePagesTest,
  testing::Values (
             AttributeResult { EFI_SUCCESS, EFI_MEMORY_RO },
             AttributeResult { EFI_SUCCESS, EFI_MEMORY_RP },
             AttributeResult { EFI_SUCCESS, EFI_MEMORY_RO | EFI_MEMORY_RP },
             AttributeResult { EFI_UNSUPPORTED, 0 },
             AttributeResult { EFI_NO_MAPPING, 0 },
             AttributeResult { EFI_INVALID_PARAMETER, 0 }
             )
  );

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}

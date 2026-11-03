/** @file
  Unit tests for PEI Core page allocation.

  Test cases:
  - TemporaryMemory_ReuseAfterFree:
    reuses a freed range before consuming Temporary Memory top-down pages.
  - StackSwitch_ActiveRangeFirst:
    uses the active physical-memory top after the stack-switch signal, even when a suitable freed HOB exists.
  - StackSwitch_FinalFallback:
    retains the unrestricted final HOB fallback when the active physical-memory range is exhausted.
  - TemporaryMemory_BestFit:
    prefers the smallest suitable HOB over a larger earlier HOB.
  - TemporaryMemory_PaddingHobReuse:
    sets the hint when larger-granularity padding is created and reuses it on the next allocation.
  - Granularity_AlignedSplit:
    checks HOB allocation alignment and splitting at 4 KiB, 16 KiB, and 64 KiB.
  - TemporaryMemory_NoHintSkipsSearch:
    skips the early HOB scan when FreePages() has not succeeded.
  - PermanentMemory_TopDownFirst:
    leaves Permanent Memory allocation order unchanged.
  - PermanentMemory_SmallHobTopDown:
    top-down allocation when free HOBs are too small.
  - Validation_InvalidType:
    rejects invalid types without changing the PHIT.
  - Fallback_OutsidePhit:
    final fallback reuses an out-of-PHIT range without bins.
  - Fallback_CoalescesAdjacentRanges:
    final fallback merges adjacent free HOBs when neither range can satisfy the request alone.
  - Fallback_BestFitTieOrder:
    selects the smallest suitable HOB during final fallback and preserves HOB-list order for ties.

  Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockHobLib.h>

extern "C" {
  #include "PeiMain.h"
  #include "MemoryBin.h"
  #include <Library/PeiServicesLib.h>

  EFI_STATUS
  FindFreeMemoryFromMemoryAllocationHob (
    IN  CONST EFI_PEI_SERVICES  **PeiServices,
    IN  EFI_MEMORY_TYPE         MemoryType,
    IN  UINTN                   Pages,
    IN  UINTN                   Granularity,
    OUT EFI_PHYSICAL_ADDRESS    *Memory
    );
}

using namespace testing;

/** Maximum number of synthetic allocation HOBs used by the fixture. **/
static constexpr UINTN  MAX_MEMORY_ALLOCATION_HOBS = 4;
/** Top of the PHIT free-memory range used by allocation tests. **/
static constexpr EFI_PHYSICAL_ADDRESS  FREE_MEMORY_TOP = 0x10000000;

/**
  Fixture for exercising PEI Core page allocation with controlled HOB state.

  The fixture provides mock HOB lookup over synthetic allocation HOBs and a
  valid PEI core instance/PHIT for calls to PeiAllocatePages().
**/
class PeiMemoryServicesTest : public ::testing::Test {
protected:
  // Intercepts production HOB lookups and maps them to the fixture HOBs.
  StrictMock<MockHobLib> HobLib;
  // PEI core state consumed by PeiAllocatePages().
  PEI_CORE_INSTANCE PrivateData;
  // Handoff information table containing the default free-memory range.
  EFI_HOB_HANDOFF_INFO_TABLE HobList;
  // Synthetic memory allocation HOBs returned by the mocked HOB library.
  EFI_HOB_MEMORY_ALLOCATION MemoryAllocationHobs[MAX_MEMORY_ALLOCATION_HOBS];
  // Reusable HOB consumed when the allocator creates a memory allocation HOB.
  EFI_HOB_MEMORY_ALLOCATION UnusedHob;
  // Number of valid entries in MemoryAllocationHobs.
  UINTN MemoryAllocationHobCount;

  /**
    Initialize the fixture's PEI state and mock HOB lookup behavior.
  **/
  VOID
  SetUp (
    ) override
  {
    ZeroMem (&PrivateData, sizeof (PrivateData));
    ZeroMem (&HobList, sizeof (HobList));
    ZeroMem (MemoryAllocationHobs, sizeof (MemoryAllocationHobs));
    ZeroMem (&UnusedHob, sizeof (UnusedHob));

    PrivateData.Signature          = PEI_CORE_HANDLE_SIGNATURE;
    PrivateData.HobList.Raw        = (UINT8 *)&HobList;
    PrivateData.PeiMemoryInstalled = TRUE;
    HobList.EfiFreeMemoryTop       = FREE_MEMORY_TOP;
    HobList.EfiFreeMemoryBottom    = FREE_MEMORY_TOP - (8 * DEFAULT_PAGE_ALLOCATION_GRANULARITY);
    UnusedHob.Header.HobType       = EFI_HOB_TYPE_UNUSED;
    UnusedHob.Header.HobLength     = sizeof (UnusedHob);
    MemoryAllocationHobCount       = 0;

    EXPECT_CALL (HobLib, GetFirstHob (_))
      .Times (AnyNumber ())
      .WillRepeatedly (
         [this](UINT16 Type) {
      return FindFirstHob (Type);
    }
         );
    EXPECT_CALL (HobLib, GetNextHob (_, _))
      .Times (AnyNumber ())
      .WillRepeatedly (
         [this](UINT16 Type, CONST VOID *HobStart) {
      return FindNextHob (Type, HobStart);
    }
         );
  }

  /**
    Return the PEI services pointer associated with the fixture instance.

    @return  The PEI services pointer expected by PeiAllocatePages().
  **/
  CONST EFI_PEI_SERVICES **
  PeiServices (
    )
  {
    return (CONST EFI_PEI_SERVICES **)&PrivateData.Ps;
  }

  /**
    Add a synthetic conventional-memory allocation HOB to the fixture.

    @param[in] BaseAddress  Start address of the free range.
    @param[in] Length       Length of the free range in bytes.

    @return  The newly initialized memory allocation HOB.
  **/
  EFI_HOB_MEMORY_ALLOCATION *
  AddFreeRange (
    IN EFI_PHYSICAL_ADDRESS  BaseAddress,
    IN UINT64                Length
    )
  {
    EFI_HOB_MEMORY_ALLOCATION  *Hob;

    Hob = &MemoryAllocationHobs[MemoryAllocationHobCount++];
    ZeroMem (Hob, sizeof (*Hob));
    Hob->Header.HobType                    = EFI_HOB_TYPE_MEMORY_ALLOCATION;
    Hob->Header.HobLength                  = sizeof (*Hob);
    Hob->AllocDescriptor.MemoryBaseAddress = BaseAddress;
    Hob->AllocDescriptor.MemoryLength      = Length;
    Hob->AllocDescriptor.MemoryType        = EfiConventionalMemory;
    return Hob;
  }

private:

  /**
    Find the first fixture HOB matching the requested type.

    @param[in] Type  HOB type to find.

    @return  The first matching HOB, or NULL if none is available.
  **/
  VOID *
  FindFirstHob (
    IN UINT16  Type
    )
  {
    UINTN  Index;

    if (UnusedHob.Header.HobType == Type) {
      return &UnusedHob;
    }

    for (Index = 0; Index < MAX_MEMORY_ALLOCATION_HOBS; Index++) {
      if (MemoryAllocationHobs[Index].Header.HobType == Type) {
        return &MemoryAllocationHobs[Index];
      }
    }

    return NULL;
  }

  /**
    Find the fixture HOB beginning at HobStart if it matches Type.

    @param[in] Type      HOB type to find.
    @param[in] HobStart  Starting HOB address supplied by HobLib.

    @return  The matching HOB, or NULL if none is available.
  **/
  VOID *
  FindNextHob (
    IN UINT16      Type,
    IN CONST VOID  *HobStart
    )
  {
    UINTN  Index;
    UINTN  StartIndex;

    if (HobStart == ((CONST UINT8 *)&UnusedHob + UnusedHob.Header.HobLength)) {
      StartIndex = 0;
    } else {
      for (StartIndex = 0; StartIndex < MAX_MEMORY_ALLOCATION_HOBS; StartIndex++) {
        if (HobStart == &MemoryAllocationHobs[StartIndex]) {
          break;
        }
      }
    }

    for (Index = StartIndex; Index < MAX_MEMORY_ALLOCATION_HOBS; Index++) {
      if (MemoryAllocationHobs[Index].Header.HobType == Type) {
        return &MemoryAllocationHobs[Index];
      }
    }

    return NULL;
  }
};

/**
  Verify that allocation reuses the smallest suitable freed range first.

  The PHIT free-memory top remains unchanged when a freed range can satisfy
  the request.
**/
TEST_F (PeiMemoryServicesTest, TemporaryMemory_ReuseAfterFree) {
  EFI_PHYSICAL_ADDRESS       Memory;
  EFI_HOB_MEMORY_ALLOCATION  *FreedRange;
  EFI_STATUS                 Status;
  UINT64                     Granularity;

  Granularity                            = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
  PrivateData.PeiMemoryInstalled         = FALSE;
  FreedRange                             = AddFreeRange (HobList.EfiFreeMemoryBottom, 2 * Granularity);
  FreedRange->AllocDescriptor.MemoryType = EfiLoaderData;

  Status = PeiFreePages (
             PeiServices (),
             FreedRange->AllocDescriptor.MemoryBaseAddress,
             EFI_SIZE_TO_PAGES (2 * Granularity)
             );

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_TRUE (PrivateData.TemporaryMemoryPagesFreed);

  Status = PeiAllocatePages (PeiServices (), EfiLoaderData, 1, &Memory);

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Memory, HobList.EfiFreeMemoryBottom + Granularity);
  EXPECT_EQ (HobList.EfiFreeMemoryTop, FREE_MEMORY_TOP);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryBaseAddress, Memory);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryLength, Granularity);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryType, EfiLoaderData);
  EXPECT_EQ (UnusedHob.Header.HobType, EFI_HOB_TYPE_MEMORY_ALLOCATION);
  EXPECT_EQ (UnusedHob.AllocDescriptor.MemoryBaseAddress, HobList.EfiFreeMemoryBottom);
  EXPECT_EQ (UnusedHob.AllocDescriptor.MemoryLength, Granularity);
  EXPECT_TRUE (PrivateData.TemporaryMemoryPagesFreed);
}

/**
  Verify a suitable free HOB does not take priority after stack switching begins.

  The active physical-memory range is used even when the early-search hint is set.
**/
TEST_F (PeiMemoryServicesTest, StackSwitch_ActiveRangeFirst) {
  EFI_PHYSICAL_ADDRESS       Memory;
  EFI_HOB_MEMORY_ALLOCATION  *FreedRange;
  EFI_STATUS                 Status;
  UINT64                     Granularity;

  Granularity                            = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
  PrivateData.PeiMemoryInstalled         = FALSE;
  FreedRange                             = AddFreeRange (HobList.EfiFreeMemoryBottom, 2 * Granularity);
  FreedRange->AllocDescriptor.MemoryType = EfiLoaderData;

  Status = PeiFreePages (
             PeiServices (),
             FreedRange->AllocDescriptor.MemoryBaseAddress,
             EFI_SIZE_TO_PAGES (2 * Granularity)
             );

  ASSERT_EQ (Status, EFI_SUCCESS);
  ASSERT_TRUE (PrivateData.TemporaryMemoryPagesFreed);

  PrivateData.SwitchStackSignal     = TRUE;
  PrivateData.PhysicalMemoryBegin   = 0x20000000;
  PrivateData.FreePhysicalMemoryTop = 0x21000000;

  Status = PeiAllocatePages (PeiServices (), EfiLoaderData, 1, &Memory);

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Memory, (EFI_PHYSICAL_ADDRESS)(0x21000000 - Granularity));
  EXPECT_EQ (PrivateData.FreePhysicalMemoryTop, Memory);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryBaseAddress, HobList.EfiFreeMemoryBottom);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryLength, 2 * Granularity);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryType, EfiConventionalMemory);
}

/**
  Verify the unrestricted final fallback remains available while stack switching is pending.

  The active physical-memory range is exhausted, so allocation must use the
  suitable free HOB without consuming either top-down free-memory top.
**/
TEST_F (PeiMemoryServicesTest, StackSwitch_FinalFallback) {
  EFI_PHYSICAL_ADDRESS       Memory;
  EFI_HOB_MEMORY_ALLOCATION  *FreedRange;
  EFI_STATUS                 Status;
  UINT64                     Granularity;
  EFI_PHYSICAL_ADDRESS       RangeBase;

  Granularity                           = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
  RangeBase                             = 0x30000000;
  PrivateData.PeiMemoryInstalled        = FALSE;
  PrivateData.SwitchStackSignal         = TRUE;
  PrivateData.TemporaryMemoryPagesFreed = TRUE;
  PrivateData.PhysicalMemoryBegin       = 0x20000000;
  PrivateData.FreePhysicalMemoryTop     = PrivateData.PhysicalMemoryBegin;
  HobList.EfiFreeMemoryBottom           = FREE_MEMORY_TOP;
  FreedRange                            = AddFreeRange (RangeBase, 2 * Granularity);

  Status = PeiAllocatePages (PeiServices (), EfiLoaderData, 1, &Memory);

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Memory, RangeBase + Granularity);
  EXPECT_EQ (PrivateData.FreePhysicalMemoryTop, PrivateData.PhysicalMemoryBegin);
  EXPECT_EQ (HobList.EfiFreeMemoryTop, FREE_MEMORY_TOP);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryBaseAddress, Memory);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryLength, Granularity);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryType, EfiLoaderData);
}

/**
  Verify early reuse selects the smallest suitable HOB rather than the first suitable range.
**/
TEST_F (PeiMemoryServicesTest, TemporaryMemory_BestFit) {
  EFI_PHYSICAL_ADDRESS       Memory;
  EFI_HOB_MEMORY_ALLOCATION  *FirstRange;
  EFI_HOB_MEMORY_ALLOCATION  *SecondRange;
  EFI_STATUS                 Status;
  UINT64                     Granularity;

  Granularity                            = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
  PrivateData.PeiMemoryInstalled         = FALSE;
  FirstRange                             = AddFreeRange (HobList.EfiFreeMemoryBottom, 3 * Granularity);
  FirstRange->AllocDescriptor.MemoryType = EfiLoaderData;
  SecondRange                            = AddFreeRange (
                                             HobList.EfiFreeMemoryBottom + (4 * Granularity),
                                             2 * Granularity
                                             );

  Status = PeiFreePages (
             PeiServices (),
             FirstRange->AllocDescriptor.MemoryBaseAddress,
             EFI_SIZE_TO_PAGES (3 * Granularity)
             );

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_TRUE (PrivateData.TemporaryMemoryPagesFreed);

  Status = PeiAllocatePages (PeiServices (), EfiLoaderData, 1, &Memory);

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Memory, HobList.EfiFreeMemoryBottom + (5 * Granularity));
  EXPECT_EQ (HobList.EfiFreeMemoryTop, FREE_MEMORY_TOP);
  EXPECT_EQ (FirstRange->AllocDescriptor.MemoryBaseAddress, HobList.EfiFreeMemoryBottom);
  EXPECT_EQ (FirstRange->AllocDescriptor.MemoryLength, 3 * Granularity);
  EXPECT_EQ (FirstRange->AllocDescriptor.MemoryType, EfiConventionalMemory);
  EXPECT_EQ (SecondRange->AllocDescriptor.MemoryBaseAddress, Memory);
  EXPECT_EQ (SecondRange->AllocDescriptor.MemoryLength, Granularity);
  EXPECT_EQ (SecondRange->AllocDescriptor.MemoryType, EfiLoaderData);
}

/**
  Verify larger-granularity padding sets the hint and can be reused immediately.
**/
TEST_F (PeiMemoryServicesTest, TemporaryMemory_PaddingHobReuse) {
  EFI_PHYSICAL_ADDRESS       Memory;
  EFI_PHYSICAL_ADDRESS       RuntimeMemory;
  EFI_HOB_MEMORY_ALLOCATION  *RuntimeAllocation;
  EFI_STATUS                 Status;
  UINTN                      Granularity;
  UINTN                      Index;
  UINTN                      Padding;

  if (RUNTIME_PAGE_ALLOCATION_GRANULARITY <= DEFAULT_PAGE_ALLOCATION_GRANULARITY) {
    GTEST_SKIP () << "Requires a runtime allocation granularity larger than the default";
  }

  Granularity                    = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
  Padding                        = EFI_PAGE_SIZE;
  PrivateData.PeiMemoryInstalled = FALSE;
  HobList.EfiFreeMemoryTop       = FREE_MEMORY_TOP + Padding;
  HobList.EfiFreeMemoryBottom    = HobList.EfiFreeMemoryTop - (4 * Granularity);
  MemoryAllocationHobCount       = MAX_MEMORY_ALLOCATION_HOBS;
  for (Index = 0; Index < MAX_MEMORY_ALLOCATION_HOBS; Index++) {
    MemoryAllocationHobs[Index].Header.HobType   = EFI_HOB_TYPE_UNUSED;
    MemoryAllocationHobs[Index].Header.HobLength = sizeof (MemoryAllocationHobs[Index]);
  }

  Status = PeiAllocatePages (
             PeiServices (),
             EfiRuntimeServicesData,
             EFI_SIZE_TO_PAGES (Granularity),
             &RuntimeMemory
             );

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_TRUE (PrivateData.TemporaryMemoryPagesFreed);
  ASSERT_EQ (UnusedHob.Header.HobType, EFI_HOB_TYPE_MEMORY_ALLOCATION);
  EXPECT_EQ (UnusedHob.AllocDescriptor.MemoryBaseAddress, FREE_MEMORY_TOP);
  EXPECT_EQ (UnusedHob.AllocDescriptor.MemoryLength, Padding);
  EXPECT_EQ (UnusedHob.AllocDescriptor.MemoryType, EfiConventionalMemory);

  RuntimeAllocation = &MemoryAllocationHobs[0];
  ASSERT_EQ (RuntimeAllocation->Header.HobType, EFI_HOB_TYPE_MEMORY_ALLOCATION);
  EXPECT_EQ (RuntimeAllocation->AllocDescriptor.MemoryBaseAddress, RuntimeMemory);
  EXPECT_EQ (RuntimeAllocation->AllocDescriptor.MemoryLength, Granularity);
  EXPECT_EQ (RuntimeAllocation->AllocDescriptor.MemoryType, EfiRuntimeServicesData);

  Status = PeiAllocatePages (PeiServices (), EfiLoaderData, 1, &Memory);

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Memory, FREE_MEMORY_TOP);
  EXPECT_EQ (HobList.EfiFreeMemoryTop, RuntimeMemory);
  EXPECT_EQ (UnusedHob.AllocDescriptor.MemoryBaseAddress, Memory);
  EXPECT_EQ (UnusedHob.AllocDescriptor.MemoryLength, (UINT64)EFI_PAGE_SIZE);
  EXPECT_EQ (UnusedHob.AllocDescriptor.MemoryType, EfiLoaderData);
  EXPECT_TRUE (PrivateData.TemporaryMemoryPagesFreed);
}

/**
  Verify free-HOB allocations honor common page allocation granularities.
**/
TEST_F (PeiMemoryServicesTest, Granularity_AlignedSplit) {
  const UINTN                Granularities[] = { 0x1000, 0x4000, 0x10000 };
  EFI_PHYSICAL_ADDRESS       Memory;
  EFI_HOB_MEMORY_ALLOCATION  *FreedRange;
  EFI_STATUS                 Status;
  UINTN                      Granularity;
  UINTN                      Index;

  for (Index = 0; Index < ARRAY_SIZE (Granularities); Index++) {
    Granularity = Granularities[Index];
    SCOPED_TRACE (Granularity);

    ZeroMem (MemoryAllocationHobs, sizeof (MemoryAllocationHobs));
    ZeroMem (&UnusedHob, sizeof (UnusedHob));
    MemoryAllocationHobCount   = 0;
    UnusedHob.Header.HobType   = EFI_HOB_TYPE_UNUSED;
    UnusedHob.Header.HobLength = sizeof (UnusedHob);
    FreedRange                 = AddFreeRange (0x40000000, Granularity + (Granularity / 2));
    Memory                     = 0;

    Status = FindFreeMemoryFromMemoryAllocationHob (
               PeiServices (),
               EfiLoaderData,
               EFI_SIZE_TO_PAGES (Granularity),
               Granularity,
               &Memory
               );

    ASSERT_EQ (Status, EFI_SUCCESS);
    EXPECT_EQ (Memory, (EFI_PHYSICAL_ADDRESS)0x40000000);
    EXPECT_EQ (Memory % Granularity, (EFI_PHYSICAL_ADDRESS)0);
    EXPECT_EQ (FreedRange->AllocDescriptor.MemoryBaseAddress, Memory);
    EXPECT_EQ (FreedRange->AllocDescriptor.MemoryLength, Granularity);
    EXPECT_EQ (FreedRange->AllocDescriptor.MemoryType, EfiLoaderData);
    EXPECT_EQ (UnusedHob.Header.HobType, EFI_HOB_TYPE_MEMORY_ALLOCATION);
    EXPECT_EQ (UnusedHob.AllocDescriptor.MemoryBaseAddress, (EFI_PHYSICAL_ADDRESS)(0x40000000 + Granularity));
    EXPECT_EQ (UnusedHob.AllocDescriptor.MemoryLength, Granularity / 2);
  }
}

/**
  Verify that allocation from Temporary Memory skips the early HOB search when FreePages() has not succeeded.

  The existing top-down path is used while the free range remains untouched.
**/
TEST_F (PeiMemoryServicesTest, TemporaryMemory_NoHintSkipsSearch) {
  EFI_PHYSICAL_ADDRESS       Memory;
  EFI_HOB_MEMORY_ALLOCATION  *FreeRange;
  EFI_STATUS                 Status;
  UINT64                     Granularity;

  Granularity                    = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
  PrivateData.PeiMemoryInstalled = FALSE;
  FreeRange                      = AddFreeRange (HobList.EfiFreeMemoryBottom, 2 * Granularity);

  Status = PeiFreePages (PeiServices (), FreeRange->AllocDescriptor.MemoryBaseAddress, 1);

  EXPECT_EQ (Status, EFI_NOT_FOUND);
  EXPECT_FALSE (PrivateData.TemporaryMemoryPagesFreed);

  Status = PeiAllocatePages (PeiServices (), EfiLoaderData, 1, &Memory);

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_FALSE (PrivateData.TemporaryMemoryPagesFreed);
  EXPECT_EQ (Memory, FREE_MEMORY_TOP - Granularity);
  EXPECT_EQ (HobList.EfiFreeMemoryTop, Memory);
  EXPECT_EQ (FreeRange->AllocDescriptor.MemoryBaseAddress, HobList.EfiFreeMemoryBottom);
  EXPECT_EQ (FreeRange->AllocDescriptor.MemoryLength, 2 * Granularity);
  EXPECT_EQ (FreeRange->AllocDescriptor.MemoryType, EfiConventionalMemory);
}

/**
  Verify Permanent Memory allocation remains top-down-first.

  A suitable free HOB is left untouched while the PHIT can satisfy the request.
**/
TEST_F (PeiMemoryServicesTest, PermanentMemory_TopDownFirst) {
  EFI_PHYSICAL_ADDRESS       Memory;
  EFI_HOB_MEMORY_ALLOCATION  *FreedRange;
  EFI_STATUS                 Status;
  UINT64                     Granularity;

  Granularity                           = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
  PrivateData.TemporaryMemoryPagesFreed = TRUE;
  FreedRange                            = AddFreeRange (HobList.EfiFreeMemoryBottom, 2 * Granularity);

  Status = PeiAllocatePages (PeiServices (), EfiLoaderData, 1, &Memory);

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Memory, FREE_MEMORY_TOP - Granularity);
  EXPECT_EQ (HobList.EfiFreeMemoryTop, Memory);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryBaseAddress, HobList.EfiFreeMemoryBottom);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryLength, 2 * Granularity);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryType, EfiConventionalMemory);
}

/**
  Verify that Permanent Memory allocation uses the PHIT top when free HOBs are too small.

  The request is rounded to the configured page-allocation granularity before
  the PHIT range is updated.
**/
TEST_F (PeiMemoryServicesTest, PermanentMemory_SmallHobTopDown) {
  EFI_PHYSICAL_ADDRESS       Memory;
  EFI_HOB_MEMORY_ALLOCATION  *FreedRange;
  EFI_STATUS                 Status;
  UINTN                      GranularityPages;
  UINTN                      RequestedPages;
  UINTN                      AllocatedPages;

  GranularityPages = EFI_SIZE_TO_PAGES (DEFAULT_PAGE_ALLOCATION_GRANULARITY);
  RequestedPages   = GranularityPages + 1;
  AllocatedPages   = ALIGN_VALUE (RequestedPages, GranularityPages);
  FreedRange       = AddFreeRange (0x40000000, DEFAULT_PAGE_ALLOCATION_GRANULARITY);

  Status = PeiAllocatePages (PeiServices (), EfiLoaderData, RequestedPages, &Memory);

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Memory, FREE_MEMORY_TOP - (AllocatedPages * EFI_PAGE_SIZE));
  EXPECT_EQ (HobList.EfiFreeMemoryTop, Memory);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryType, EfiConventionalMemory);
  EXPECT_EQ (UnusedHob.Header.HobType, EFI_HOB_TYPE_MEMORY_ALLOCATION);
  EXPECT_EQ (UnusedHob.AllocDescriptor.MemoryBaseAddress, Memory);
  EXPECT_EQ (UnusedHob.AllocDescriptor.MemoryLength, AllocatedPages * EFI_PAGE_SIZE);
}

/**
  Verify that an invalid memory type is rejected without changing free memory.
**/
TEST_F (PeiMemoryServicesTest, Validation_InvalidType) {
  EFI_PHYSICAL_ADDRESS  Memory;
  EFI_STATUS            Status;

  Memory = 0;
  Status = PeiAllocatePages (PeiServices (), EfiConventionalMemory, 1, &Memory);

  EXPECT_EQ (Status, EFI_INVALID_PARAMETER);
  EXPECT_EQ (HobList.EfiFreeMemoryTop, FREE_MEMORY_TOP);
  EXPECT_EQ (Memory, (EFI_PHYSICAL_ADDRESS)0);
}

/**
  Verify the final unrestricted search can reuse a free range outside the PHIT.

  The PHIT has no available top-down memory, so allocation succeeds only if
  the final free-HOB search is not restricted to the PHIT range.
**/
TEST_F (PeiMemoryServicesTest, Fallback_OutsidePhit) {
  EFI_PHYSICAL_ADDRESS       Memory;
  EFI_HOB_MEMORY_ALLOCATION  *FreedRange;
  EFI_STATUS                 Status;
  UINT64                     Granularity;
  EFI_PHYSICAL_ADDRESS       RangeBase;

  Granularity                 = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
  RangeBase                   = FREE_MEMORY_TOP + Granularity;
  HobList.EfiFreeMemoryBottom = FREE_MEMORY_TOP;
  FreedRange                  = AddFreeRange (RangeBase, 2 * Granularity);

  Status = PeiAllocatePages (PeiServices (), EfiLoaderData, 1, &Memory);

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Memory, RangeBase + Granularity);
  EXPECT_EQ (HobList.EfiFreeMemoryTop, FREE_MEMORY_TOP);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryBaseAddress, Memory);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryLength, Granularity);
  EXPECT_EQ (FreedRange->AllocDescriptor.MemoryType, EfiLoaderData);
}

/**
  Verify the final unrestricted fallback coalesces adjacent free HOBs.

  Neither range can satisfy the request individually, but their combined
  range can.
**/
TEST_F (PeiMemoryServicesTest, Fallback_CoalescesAdjacentRanges) {
  EFI_PHYSICAL_ADDRESS       Memory;
  EFI_HOB_MEMORY_ALLOCATION  *FirstRange;
  EFI_HOB_MEMORY_ALLOCATION  *SecondRange;
  EFI_STATUS                 Status;
  UINT64                     Granularity;
  EFI_PHYSICAL_ADDRESS       RangeBase;

  Granularity                 = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
  RangeBase                   = 0x80000000;
  HobList.EfiFreeMemoryBottom = FREE_MEMORY_TOP;
  FirstRange                  = AddFreeRange (RangeBase, Granularity);
  SecondRange                 = AddFreeRange (RangeBase + Granularity, Granularity);

  Status = PeiAllocatePages (PeiServices (), EfiLoaderData, EFI_SIZE_TO_PAGES (2 * Granularity), &Memory);

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Memory, RangeBase);
  EXPECT_EQ (HobList.EfiFreeMemoryTop, FREE_MEMORY_TOP);
  EXPECT_EQ (FirstRange->Header.HobType, EFI_HOB_TYPE_UNUSED);
  EXPECT_EQ (SecondRange->AllocDescriptor.MemoryBaseAddress, Memory);
  EXPECT_EQ (SecondRange->AllocDescriptor.MemoryLength, 2 * Granularity);
  EXPECT_EQ (SecondRange->AllocDescriptor.MemoryType, EfiLoaderData);
}

/**
  Verify the final unrestricted fallback selects the smallest suitable HOB.

  Equal-sized candidates are resolved by HOB-list order after the PHIT is exhausted.
**/
TEST_F (PeiMemoryServicesTest, Fallback_BestFitTieOrder) {
  EFI_PHYSICAL_ADDRESS       Memory;
  EFI_HOB_MEMORY_ALLOCATION  *FirstRange;
  EFI_HOB_MEMORY_ALLOCATION  *SecondRange;
  EFI_HOB_MEMORY_ALLOCATION  *ThirdRange;
  EFI_STATUS                 Status;
  UINT64                     Granularity;
  EFI_PHYSICAL_ADDRESS       FirstBase;
  EFI_PHYSICAL_ADDRESS       SecondBase;
  EFI_PHYSICAL_ADDRESS       ThirdBase;

  Granularity                 = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
  FirstBase                   = 0x80000000;
  SecondBase                  = 0x81000000;
  ThirdBase                   = 0x82000000;
  HobList.EfiFreeMemoryBottom = FREE_MEMORY_TOP;
  FirstRange                  = AddFreeRange (FirstBase, 3 * Granularity);
  SecondRange                 = AddFreeRange (SecondBase, 2 * Granularity);
  ThirdRange                  = AddFreeRange (ThirdBase, 2 * Granularity);

  Status = PeiAllocatePages (PeiServices (), EfiLoaderData, 1, &Memory);

  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (Memory, SecondBase + Granularity);
  EXPECT_EQ (HobList.EfiFreeMemoryTop, FREE_MEMORY_TOP);
  EXPECT_EQ (FirstRange->AllocDescriptor.MemoryBaseAddress, FirstBase);
  EXPECT_EQ (FirstRange->AllocDescriptor.MemoryLength, 3 * Granularity);
  EXPECT_EQ (FirstRange->AllocDescriptor.MemoryType, EfiConventionalMemory);
  EXPECT_EQ (SecondRange->AllocDescriptor.MemoryBaseAddress, Memory);
  EXPECT_EQ (SecondRange->AllocDescriptor.MemoryLength, Granularity);
  EXPECT_EQ (SecondRange->AllocDescriptor.MemoryType, EfiLoaderData);
  EXPECT_EQ (ThirdRange->AllocDescriptor.MemoryBaseAddress, ThirdBase);
  EXPECT_EQ (ThirdRange->AllocDescriptor.MemoryLength, 2 * Granularity);
  EXPECT_EQ (ThirdRange->AllocDescriptor.MemoryType, EfiConventionalMemory);
}

/**
  Provide a test stub for PEI Core HOB creation.

  This entry point is unrelated to the AllocatePages() paths under test.

  @param[in] PeiServices  PEI services table pointer.
  @param[in] Type         Type of HOB to create.
  @param[in] Length       Requested HOB length.
  @param[out] Hob         Receives the created HOB pointer.

  @retval EFI_OUT_OF_RESOURCES  HOB creation is not supported by this stub.
**/
extern "C" EFI_STATUS
EFIAPI
PeiCreateHob (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN UINT16                  Type,
  IN UINT16                  Length,
  IN OUT VOID                **Hob
  )
{
  (VOID)PeiServices;
  (VOID)Type;
  (VOID)Length;
  (VOID)Hob;
  return EFI_OUT_OF_RESOURCES;
}

/**
  Provide a test stub for the PEI service-table HOB creation service.

  This entry point is referenced by unrelated pool-allocation code.

  @param[in] Type   Type of HOB to create.
  @param[in] Length Requested HOB length.
  @param[out] Hob   Receives the created HOB pointer.

  @retval EFI_OUT_OF_RESOURCES  HOB creation is not supported by this stub.
**/
extern "C" EFI_STATUS
EFIAPI
PeiServicesCreateHob (
  IN UINT16  Type,
  IN UINT16  Length,
  OUT VOID   **Hob
  )
{
  (VOID)Type;
  (VOID)Length;
  (VOID)Hob;
  return EFI_OUT_OF_RESOURCES;
}

/**
  Provide a test stub for memory-type information discovery.

  @param[out] MemoryTypeInformation  Receives the discovered memory types.

  @retval EFI_NOT_FOUND  Memory-type discovery is outside this test's scope.
**/
extern "C" EFI_STATUS
EFIAPI
PopulateMemoryTypeInformation (
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInformation
  )
{
  (VOID)MemoryTypeInformation;
  return EFI_NOT_FOUND;
}

/**
  Provide a test stub for memory-type information resource-HOB lookup.

  @param[in] HobStart                 Start of the HOB list.
  @param[in] MemoryTypeInformation    Memory-type information to locate.

  @return  NULL because resource-HOB lookup is outside this test's scope.
**/
extern "C" EFI_HOB_RESOURCE_DESCRIPTOR *
EFIAPI
GetMemoryTypeInformationResourceHob (
  IN VOID                         **HobStart,
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInformation
  )
{
  (VOID)HobStart;
  (VOID)MemoryTypeInformation;
  return NULL;
}

/**
  Provide a test stub for memory-bin range initialization.

  @param[in] Start                             Start of the memory-bin range.
  @param[in] Length                            Length of the memory-bin range.
  @param[in] MemoryTypeInformation              Memory-type allocation counts.
  @param[in, out] MemoryTypeInformationInitialized  Memory-bin initialization state.
  @param[out] MemoryTypeStatistics             Memory-bin address statistics.
  @param[in, out] DefaultMaximumAddress         Default bin maximum address.
**/
extern "C" VOID
EFIAPI
CoreSetMemoryTypeInformationRange (
  IN EFI_PHYSICAL_ADDRESS         Start,
  IN UINT64                       Length,
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInformation,
  IN BOOLEAN                      *MemoryTypeInformationInitialized,
  IN EFI_MEMORY_TYPE_STATISTICS   *MemoryTypeStatistics,
  IN EFI_PHYSICAL_ADDRESS         *DefaultMaximumAddress
  )
{
  (VOID)Start;
  (VOID)Length;
  (VOID)MemoryTypeInformation;
  (VOID)MemoryTypeInformationInitialized;
  (VOID)MemoryTypeStatistics;
  (VOID)DefaultMaximumAddress;
}

/**
  Provide a test stub for memory-bin allocation.

  @param[in, out] MemoryTypeInformationInitialized  Memory-bin initialization state.
  @param[in] MemoryTypeInformation                  Memory-type allocation counts.
  @param[out] MemoryTypeStatistics                 Memory-bin address statistics.
  @param[in, out] DefaultMaximumAddress             Default bin maximum address.
  @param[in] CreateHob                              Whether to create the resource HOB.
**/
extern "C" VOID
EFIAPI
AllocateMemoryTypeInformationBins (
  IN BOOLEAN                      *MemoryTypeInformationInitialized,
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInformation,
  IN EFI_MEMORY_TYPE_STATISTICS   *MemoryTypeStatistics,
  IN EFI_PHYSICAL_ADDRESS         *DefaultMaximumAddress,
  IN BOOLEAN                      CreateHob
  )
{
  (VOID)MemoryTypeInformationInitialized;
  (VOID)MemoryTypeInformation;
  (VOID)MemoryTypeStatistics;
  (VOID)DefaultMaximumAddress;
  (VOID)CreateHob;
}

/**
  Provide a test stub for PEI HOB-list retrieval.

  @param[in] PeiServices  PEI services table pointer.
  @param[out] HobList     Receives the HOB list pointer.

  @retval EFI_NOT_AVAILABLE_YET  HOB-list retrieval is outside this test's scope.
**/
extern "C" EFI_STATUS
EFIAPI
PeiGetHobList (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN OUT VOID                **HobList
  )
{
  (VOID)PeiServices;
  (VOID)HobList;
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Provide a test stub for PHIT construction.

  @param[in] BootMode      Current boot mode.
  @param[in] MemoryBegin   Start address of the memory region.
  @param[in] MemoryLength  Length of the memory region in bytes.
**/
extern "C" VOID
PeiCoreBuildHobHandoffInfoTable (
  IN EFI_BOOT_MODE         BootMode,
  IN EFI_PHYSICAL_ADDRESS  MemoryBegin,
  IN UINT64                MemoryLength
  )
{
  (VOID)BootMode;
  (VOID)MemoryBegin;
  (VOID)MemoryLength;
}

/**
  Initialize GoogleTest and execute the PEI memory-service test cases.

  @param[in] argc  Number of command-line arguments.
  @param[in] argv  Command-line argument array.

  @return  GoogleTest's result code: zero if all tests pass, nonzero otherwise.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}

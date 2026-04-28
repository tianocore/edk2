/** @file
  Unit tests for BaseMemoryBinLib library.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockHobLib.h>

extern "C" {
  #include <Uefi/UefiBaseType.h>
  #include <Uefi/UefiMultiPhase.h>
  #include <Pi/PiMultiPhase.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Guid/MemoryTypeInformation.h>
  #include <Pi/PiHob.h>
  #include <Pi/PiDxeCis.h>
  #include <MemoryBin.h>

  BOOLEAN  mMemoryTypeInformationInitialized = FALSE;

  EFI_MEMORY_TYPE_STATISTICS  mMemoryTypeStatistics[EfiMaxMemoryType + 1] = {
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE, FALSE }, // EfiReservedMemoryType
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE }, // EfiLoaderCode
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE }, // EfiLoaderData
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE }, // EfiBootServicesCode
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE }, // EfiBootServicesData
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  TRUE,  FALSE }, // EfiRuntimeServicesCode
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  TRUE,  FALSE }, // EfiRuntimeServicesData
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE }, // EfiConventionalMemory
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE }, // EfiUnusableMemory
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE, FALSE }, // EfiACPIReclaimMemory
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE, FALSE }, // EfiACPIMemoryNVS
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE }, // EfiMemoryMappedIO
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE }, // EfiMemoryMappedIOPortSpace
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  TRUE,  FALSE }, // EfiPalCode
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE }, // EfiPersistentMemory
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE, FALSE }, // EfiUnacceptedMemoryType
    { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE } // EfiMaxMemoryType
  };

  EFI_PHYSICAL_ADDRESS  mDefaultMaximumAddress = MAX_ALLOC_ADDRESS;
  EFI_PHYSICAL_ADDRESS  mDefaultBaseAddress    = MAX_ALLOC_ADDRESS;

  EFI_MEMORY_TYPE_INFORMATION  gMemoryTypeInformation[EfiMaxMemoryType + 1] = {
    { EfiReservedMemoryType,      0 },
    { EfiLoaderCode,              0 },
    { EfiLoaderData,              0 },
    { EfiBootServicesCode,        0 },
    { EfiBootServicesData,        0 },
    { EfiRuntimeServicesCode,     0 },
    { EfiRuntimeServicesData,     0 },
    { EfiConventionalMemory,      0 },
    { EfiUnusableMemory,          0 },
    { EfiACPIReclaimMemory,       0 },
    { EfiACPIMemoryNVS,           0 },
    { EfiMemoryMappedIO,          0 },
    { EfiMemoryMappedIOPortSpace, 0 },
    { EfiPalCode,                 0 },
    { EfiPersistentMemory,        0 },
    { EfiGcdMemoryTypeUnaccepted, 0 },
    { EfiMaxMemoryType,           0 }
  };
}

using namespace testing;

class BaseMemoryBinLibTest : public ::testing::Test {
protected:
  StrictMock<MockHobLib> HobLib;
  void
  SetUp (
    ) override
  {
    //
    // Reset global state before each test
    //
    mMemoryTypeInformationInitialized = FALSE;
    mDefaultMaximumAddress            = MAX_ALLOC_ADDRESS;
    mDefaultBaseAddress               = MAX_ALLOC_ADDRESS;

    for (UINTN i = 0; i < EfiMaxMemoryType + 1; i++) {
      gMemoryTypeInformation[i].NumberOfPages = 0;
    }

    for (UINTN i = 0; i < EfiMaxMemoryType + 1; i++) {
      mMemoryTypeStatistics[i].BaseAddress          = 0;
      mMemoryTypeStatistics[i].MaximumAddress       = MAX_ALLOC_ADDRESS;
      mMemoryTypeStatistics[i].CurrentNumberOfPages = 0;
      mMemoryTypeStatistics[i].NumberOfPages        = 0;
      mMemoryTypeStatistics[i].InformationIndex     = EfiMaxMemoryType;
      mMemoryTypeStatistics[i].DefaultBin           = FALSE;
    }

    mMemoryTypeStatistics[EfiReservedMemoryType].Special   = TRUE;
    mMemoryTypeStatistics[EfiRuntimeServicesCode].Special  = TRUE;
    mMemoryTypeStatistics[EfiRuntimeServicesCode].Runtime  = TRUE;
    mMemoryTypeStatistics[EfiRuntimeServicesData].Special  = TRUE;
    mMemoryTypeStatistics[EfiRuntimeServicesData].Runtime  = TRUE;
    mMemoryTypeStatistics[EfiACPIReclaimMemory].Special    = TRUE;
    mMemoryTypeStatistics[EfiACPIMemoryNVS].Special        = TRUE;
    mMemoryTypeStatistics[EfiPalCode].Special              = TRUE;
    mMemoryTypeStatistics[EfiPalCode].Runtime              = TRUE;
    mMemoryTypeStatistics[EfiUnacceptedMemoryType].Special = TRUE;
  }
};

//
// Test: CalculateTotalMemoryBinSizeNeeded with no pages allocated
//
TEST_F (BaseMemoryBinLibTest, CalculateTotalMemoryBinSizeNeededReturnsZeroWhenNoPagesAllocated) {
  UINT64  TotalSize;

  TotalSize = CalculateTotalMemoryBinSizeNeeded (0, gMemoryTypeInformation);

  ASSERT_EQ (TotalSize, (UINT64)0);
}

//
// Test: CalculateTotalMemoryBinSizeNeeded with pages allocated
//
TEST_F (BaseMemoryBinLibTest, CalculateTotalMemoryBinSizeNeededCalculatesCorrectSize) {
  UINT64  TotalSize;

  gMemoryTypeInformation[0].Type          = EfiRuntimeServicesCode;
  gMemoryTypeInformation[0].NumberOfPages = 10;
  gMemoryTypeInformation[1].Type          = EfiRuntimeServicesData;
  gMemoryTypeInformation[1].NumberOfPages = 20;
  gMemoryTypeInformation[2].Type          = EfiMaxMemoryType;

  TotalSize = CalculateTotalMemoryBinSizeNeeded (0, gMemoryTypeInformation);

  ASSERT_EQ (TotalSize, (UINT64)(30 * EFI_PAGE_SIZE));
}

//
// Test: CoreSetMemoryTypeInformationRange initializes bins correctly
//
TEST_F (BaseMemoryBinLibTest, CoreSetMemoryTypeInformationRangeSetsUpBinsCorrectly) {
  EFI_PHYSICAL_ADDRESS  Start;
  UINT64                Length;

  Start  = 0x100000;
  Length = 0x100000;

  gMemoryTypeInformation[0].Type          = EfiRuntimeServicesCode;
  gMemoryTypeInformation[0].NumberOfPages = 20;
  gMemoryTypeInformation[1].Type          = EfiRuntimeServicesData;
  gMemoryTypeInformation[1].NumberOfPages = 10;
  gMemoryTypeInformation[2].Type          = EfiReservedMemoryType;
  gMemoryTypeInformation[2].NumberOfPages = 5;
  gMemoryTypeInformation[3].Type          = EfiACPIReclaimMemory;
  gMemoryTypeInformation[3].NumberOfPages = 15;
  gMemoryTypeInformation[4].Type          = EfiACPIMemoryNVS;
  gMemoryTypeInformation[4].NumberOfPages = 25;
  gMemoryTypeInformation[5].Type          = EfiMaxMemoryType;

  CoreSetMemoryTypeInformationRange (Start, Length, gMemoryTypeInformation, &mMemoryTypeInformationInitialized, mMemoryTypeStatistics, &mDefaultMaximumAddress);

  ASSERT_TRUE (mMemoryTypeInformationInitialized);

  ASSERT_EQ (mMemoryTypeStatistics[EfiRuntimeServicesCode].NumberOfPages, (UINT32)20);
  ASSERT_EQ (mMemoryTypeStatistics[EfiRuntimeServicesData].NumberOfPages, (UINT32)10);
  ASSERT_EQ (mMemoryTypeStatistics[EfiReservedMemoryType].NumberOfPages, (UINT32)5);
  ASSERT_EQ (mMemoryTypeStatistics[EfiACPIReclaimMemory].NumberOfPages, (UINT32)15);
  ASSERT_EQ (mMemoryTypeStatistics[EfiACPIMemoryNVS].NumberOfPages, (UINT32)25);

  // Confirm contiguous bins
  ASSERT_EQ (mMemoryTypeStatistics[EfiRuntimeServicesData].MaximumAddress + 1, mMemoryTypeStatistics[EfiRuntimeServicesCode].BaseAddress);
  ASSERT_EQ (mMemoryTypeStatistics[EfiReservedMemoryType].MaximumAddress + 1, mMemoryTypeStatistics[EfiRuntimeServicesData].BaseAddress);
  ASSERT_EQ (mMemoryTypeStatistics[EfiACPIReclaimMemory].MaximumAddress + 1, mMemoryTypeStatistics[EfiReservedMemoryType].BaseAddress);
  ASSERT_EQ (mMemoryTypeStatistics[EfiACPIMemoryNVS].MaximumAddress + 1, mMemoryTypeStatistics[EfiACPIReclaimMemory].BaseAddress);
}

//
// Test: CoreSetMemoryTypeInformationRange rejects insufficient space
//
TEST_F (BaseMemoryBinLibTest, CoreSetMemoryTypeInformationRangeRejectsInsufficientSpace) {
  EFI_PHYSICAL_ADDRESS  Start;
  UINT64                Length;

  Start  = 0x100000;
  Length = 0x1000;

  gMemoryTypeInformation[0].Type          = EfiRuntimeServicesCode;
  gMemoryTypeInformation[0].NumberOfPages = 100;  // Requires more than 4KB
  gMemoryTypeInformation[1].Type          = EfiMaxMemoryType;

  CoreSetMemoryTypeInformationRange (Start, Length, gMemoryTypeInformation, &mMemoryTypeInformationInitialized, mMemoryTypeStatistics, &mDefaultMaximumAddress);

  ASSERT_FALSE (mMemoryTypeInformationInitialized);
}

//
// Test: CoreSetMemoryTypeInformationRange respects already initialized state
//
TEST_F (BaseMemoryBinLibTest, CoreSetMemoryTypeInformationRangeRespectsInitializedState) {
  EFI_PHYSICAL_ADDRESS  Start;
  UINT64                Length;

  mMemoryTypeInformationInitialized = TRUE;

  Start  = 0x100000;
  Length = 0x100000;

  gMemoryTypeInformation[0].Type                            = EfiRuntimeServicesCode;
  gMemoryTypeInformation[0].NumberOfPages                   = 10;
  mMemoryTypeStatistics[EfiRuntimeServicesCode].BaseAddress = 0x1000;
  gMemoryTypeInformation[1].Type                            = EfiMaxMemoryType;

  CoreSetMemoryTypeInformationRange (Start, Length, gMemoryTypeInformation, &mMemoryTypeInformationInitialized, mMemoryTypeStatistics, &mDefaultMaximumAddress);

  ASSERT_TRUE (mMemoryTypeInformationInitialized);
  ASSERT_EQ (mMemoryTypeStatistics[EfiRuntimeServicesCode].BaseAddress, (EFI_PHYSICAL_ADDRESS)0x1000);
}

//
// Test: UpdateMemoryStatistics updates in-bin statistics correctly
//
TEST_F (BaseMemoryBinLibTest, UpdateMemoryStatisticsUpdatesInBinCorrectly) {
  mMemoryTypeInformationInitialized = TRUE;

  mMemoryTypeStatistics[EfiRuntimeServicesCode].BaseAddress      = 0x100000;
  mMemoryTypeStatistics[EfiRuntimeServicesCode].MaximumAddress   = 0x200000;
  mMemoryTypeStatistics[EfiRuntimeServicesCode].InformationIndex = 0;

  gMemoryTypeInformation[0].Type          = EfiRuntimeServicesCode;
  gMemoryTypeInformation[0].NumberOfPages = 0;

  UpdateMemoryStatistics (
    EfiConventionalMemory,
    EfiRuntimeServicesCode,
    0x150000,  // Within bin range
    10,
    &mMemoryTypeInformationInitialized,
    mMemoryTypeStatistics,
    gMemoryTypeInformation,
    0x10000, // Just set these outside of the range
    0x20000
    );

  ASSERT_EQ (mMemoryTypeStatistics[EfiRuntimeServicesCode].CurrentNumberOfPages, (UINT32)10);
}

//
// Test: UpdateMemoryStatistics updates out-of-bin statistics correctly
//
TEST_F (BaseMemoryBinLibTest, UpdateMemoryStatisticsUpdatesOutOfBinCorrectly) {
  mMemoryTypeInformationInitialized = TRUE;

  mMemoryTypeStatistics[EfiRuntimeServicesCode].BaseAddress      = 0x100000;
  mMemoryTypeStatistics[EfiRuntimeServicesCode].MaximumAddress   = 0x200000;
  mMemoryTypeStatistics[EfiRuntimeServicesCode].InformationIndex = 0;

  gMemoryTypeInformation[0].Type          = EfiRuntimeServicesCode;
  gMemoryTypeInformation[0].NumberOfPages = 0;

  UpdateMemoryStatistics (
    EfiConventionalMemory,
    EfiRuntimeServicesCode,
    0x300000,  // Outside bin range
    10,
    &mMemoryTypeInformationInitialized,
    mMemoryTypeStatistics,
    gMemoryTypeInformation,
    0x10000, // Just set these outside of the range
    0x20000
    );

  ASSERT_EQ (mMemoryTypeStatistics[EfiRuntimeServicesCode].CurrentNumberOfPages, (UINT32)0);
}

//
// Test: UpdateMemoryStatistics handles freeing memory
//
TEST_F (BaseMemoryBinLibTest, UpdateMemoryStatisticsHandlesFreeingMemory) {
  mMemoryTypeInformationInitialized = TRUE;

  mMemoryTypeStatistics[EfiRuntimeServicesCode].BaseAddress          = 0x100000;
  mMemoryTypeStatistics[EfiRuntimeServicesCode].MaximumAddress       = 0x200000;
  mMemoryTypeStatistics[EfiRuntimeServicesCode].CurrentNumberOfPages = 20;
  mMemoryTypeStatistics[EfiRuntimeServicesCode].InformationIndex     = 0;

  gMemoryTypeInformation[0].Type          = EfiRuntimeServicesCode;
  gMemoryTypeInformation[0].NumberOfPages = 20;

  UpdateMemoryStatistics (
    EfiRuntimeServicesCode,
    EfiConventionalMemory,
    0x150000,  // Within bin range
    10,
    &mMemoryTypeInformationInitialized,
    mMemoryTypeStatistics,
    gMemoryTypeInformation,
    0x10000, // Just set these outside of the range
    0x20000
    );

  ASSERT_EQ (mMemoryTypeStatistics[EfiRuntimeServicesCode].CurrentNumberOfPages, (UINT32)10);
}

//
// Tests for AllocateMemoryTypeInformationBins
//

//
// Test: AllocateMemoryTypeInformationBins does nothing when already initialized
//
TEST_F (BaseMemoryBinLibTest, DoesNothingWhenAlreadyInitialized) {
  mMemoryTypeInformationInitialized                         = TRUE;
  mMemoryTypeStatistics[EfiRuntimeServicesCode].BaseAddress = 0x1000;

  AllocateMemoryTypeInformationBins (&mMemoryTypeInformationInitialized, gMemoryTypeInformation, mMemoryTypeStatistics, &mDefaultMaximumAddress, FALSE);

  // Should remain unchanged since already initialized
  ASSERT_TRUE (mMemoryTypeInformationInitialized);
  ASSERT_EQ (mMemoryTypeStatistics[EfiRuntimeServicesCode].BaseAddress, (EFI_PHYSICAL_ADDRESS)0x1000);
}

//
// Test: AllocateMemoryTypeInformationBins does nothing when no pages needed
//
TEST_F (BaseMemoryBinLibTest, DoesNothingWhenNoPagesNeeded) {
  AllocateMemoryTypeInformationBins (&mMemoryTypeInformationInitialized, gMemoryTypeInformation, mMemoryTypeStatistics, &mDefaultMaximumAddress, FALSE);

  ASSERT_TRUE (mMemoryTypeInformationInitialized);
  ASSERT_EQ (mMemoryTypeStatistics[EfiReservedMemoryType].NumberOfPages, (UINT32)0);
  ASSERT_EQ (mMemoryTypeStatistics[EfiRuntimeServicesCode].NumberOfPages, (UINT32)0);
  ASSERT_EQ (mMemoryTypeStatistics[EfiRuntimeServicesData].NumberOfPages, (UINT32)0);
  ASSERT_EQ (mMemoryTypeStatistics[EfiACPIReclaimMemory].NumberOfPages, (UINT32)0);
  ASSERT_EQ (mMemoryTypeStatistics[EfiACPIMemoryNVS].NumberOfPages, (UINT32)0);
}

//
// Test: AllocateMemoryTypeInformationBins allocates pages and initializes bins
//
TEST_F (BaseMemoryBinLibTest, AllocatesPagesAndInitializesBins) {
  gMemoryTypeInformation[0].Type          = EfiRuntimeServicesCode;
  gMemoryTypeInformation[0].NumberOfPages = 20;
  gMemoryTypeInformation[1].Type          = EfiRuntimeServicesData;
  gMemoryTypeInformation[1].NumberOfPages = 10;
  gMemoryTypeInformation[2].Type          = EfiReservedMemoryType;
  gMemoryTypeInformation[2].NumberOfPages = 5;
  gMemoryTypeInformation[3].Type          = EfiACPIReclaimMemory;
  gMemoryTypeInformation[3].NumberOfPages = 15;
  gMemoryTypeInformation[4].Type          = EfiACPIMemoryNVS;
  gMemoryTypeInformation[4].NumberOfPages = 25;
  gMemoryTypeInformation[5].Type          = EfiMaxMemoryType;

  // HOB should not be built in this case
  EXPECT_CALL (HobLib, BuildResourceDescriptorWithOwnerHob (_, _, _, _, _))
    .Times (0);

  AllocateMemoryTypeInformationBins (&mMemoryTypeInformationInitialized, gMemoryTypeInformation, mMemoryTypeStatistics, &mDefaultMaximumAddress, FALSE);

  ASSERT_TRUE (mMemoryTypeInformationInitialized);

  ASSERT_EQ (mMemoryTypeStatistics[EfiRuntimeServicesCode].NumberOfPages, (UINT32)20);
  ASSERT_EQ (mMemoryTypeStatistics[EfiRuntimeServicesData].NumberOfPages, (UINT32)10);
  ASSERT_EQ (mMemoryTypeStatistics[EfiReservedMemoryType].NumberOfPages, (UINT32)5);
  ASSERT_EQ (mMemoryTypeStatistics[EfiACPIReclaimMemory].NumberOfPages, (UINT32)15);
  ASSERT_EQ (mMemoryTypeStatistics[EfiACPIMemoryNVS].NumberOfPages, (UINT32)25);

  // Confirm contiguous bins
  ASSERT_EQ (mMemoryTypeStatistics[EfiRuntimeServicesData].MaximumAddress + 1, mMemoryTypeStatistics[EfiRuntimeServicesCode].BaseAddress);
  ASSERT_EQ (mMemoryTypeStatistics[EfiReservedMemoryType].MaximumAddress + 1, mMemoryTypeStatistics[EfiRuntimeServicesData].BaseAddress);
  ASSERT_EQ (mMemoryTypeStatistics[EfiACPIReclaimMemory].MaximumAddress + 1, mMemoryTypeStatistics[EfiReservedMemoryType].BaseAddress);
  ASSERT_EQ (mMemoryTypeStatistics[EfiACPIMemoryNVS].MaximumAddress + 1, mMemoryTypeStatistics[EfiACPIReclaimMemory].BaseAddress);
}

//
// Test: AllocateMemoryTypeInformationBins creates HOB when requested
//
TEST_F (BaseMemoryBinLibTest, CreatesHobWhenRequested) {
  UINT64  TotalPages = 30;

  gMemoryTypeInformation[0].Type          = EfiRuntimeServicesCode;
  gMemoryTypeInformation[0].NumberOfPages = 10;
  gMemoryTypeInformation[1].Type          = EfiRuntimeServicesData;
  gMemoryTypeInformation[1].NumberOfPages = 20;
  gMemoryTypeInformation[2].Type          = EfiMaxMemoryType;

  EXPECT_CALL (
    HobLib,
    BuildResourceDescriptorWithOwnerHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_TESTED,
      _,
      TotalPages * EFI_PAGE_SIZE,
      &gEfiMemoryTypeInformationGuid
      )
    )
    .Times (1);

  AllocateMemoryTypeInformationBins (&mMemoryTypeInformationInitialized, gMemoryTypeInformation, mMemoryTypeStatistics, &mDefaultMaximumAddress, TRUE);

  ASSERT_TRUE (mMemoryTypeInformationInitialized);
}

//
// Test: PopulateMemoryTypeInformation returns NOT_FOUND when Memory Type Information HOB is missing
//
TEST_F (BaseMemoryBinLibTest, ReturnsNotFoundWhenMemoryTypeInformationHobMissing) {
  EFI_STATUS  Status;

  EXPECT_CALL (HobLib, GetFirstGuidHob (&gEfiMemoryTypeInformationGuid))
    .WillOnce (Return ((VOID *)NULL));

  Status = PopulateMemoryTypeInformation (gMemoryTypeInformation);

  ASSERT_EQ (Status, EFI_NOT_FOUND);
}

//
// Test: PopulateMemoryTypeInformation populates from valid HOB
//
TEST_F (BaseMemoryBinLibTest, PopulatesFromValidHob) {
  EFI_STATUS                   Status;
  UINT8                        GuidHobBuffer[sizeof (EFI_HOB_GUID_TYPE) + sizeof (EFI_MEMORY_TYPE_INFORMATION) * 7];
  EFI_HOB_GUID_TYPE            *GuidHob;
  EFI_MEMORY_TYPE_INFORMATION  *MemTypeInfo;

  ZeroMem (GuidHobBuffer, sizeof (GuidHobBuffer));
  GuidHob                   = (EFI_HOB_GUID_TYPE *)GuidHobBuffer;
  GuidHob->Header.HobType   = EFI_HOB_TYPE_GUID_EXTENSION;
  GuidHob->Header.HobLength = sizeof (GuidHobBuffer);
  CopyGuid (&GuidHob->Name, &gEfiMemoryTypeInformationGuid);

  MemTypeInfo                  = (EFI_MEMORY_TYPE_INFORMATION *)(GuidHob + 1);
  MemTypeInfo[0].Type          = EfiReservedMemoryType;
  MemTypeInfo[0].NumberOfPages = 5;
  MemTypeInfo[1].Type          = EfiRuntimeServicesCode;
  MemTypeInfo[1].NumberOfPages = 10;
  MemTypeInfo[2].Type          = EfiRuntimeServicesData;
  MemTypeInfo[2].NumberOfPages = 15;
  MemTypeInfo[3].Type          = EfiACPIReclaimMemory;
  MemTypeInfo[3].NumberOfPages = 20;
  MemTypeInfo[4].Type          = EfiACPIMemoryNVS;
  MemTypeInfo[4].NumberOfPages = 25;
  MemTypeInfo[5].Type          = EfiPalCode;
  MemTypeInfo[5].NumberOfPages = 8;
  MemTypeInfo[6].Type          = EfiMaxMemoryType;
  MemTypeInfo[6].NumberOfPages = 0;

  EXPECT_CALL (HobLib, GetFirstGuidHob (&gEfiMemoryTypeInformationGuid))
    .WillOnce (Return ((VOID *)GuidHob));

  Status = PopulateMemoryTypeInformation (gMemoryTypeInformation);

  ASSERT_EQ (Status, EFI_SUCCESS);
  ASSERT_EQ (gMemoryTypeInformation[0].Type, (UINT32)EfiReservedMemoryType);
  ASSERT_EQ (gMemoryTypeInformation[0].NumberOfPages, (UINT32)5);
  ASSERT_EQ (gMemoryTypeInformation[1].Type, (UINT32)EfiRuntimeServicesCode);
  ASSERT_EQ (gMemoryTypeInformation[1].NumberOfPages, (UINT32)10);
  ASSERT_EQ (gMemoryTypeInformation[2].Type, (UINT32)EfiRuntimeServicesData);
  ASSERT_EQ (gMemoryTypeInformation[2].NumberOfPages, (UINT32)15);
  ASSERT_EQ (gMemoryTypeInformation[3].Type, (UINT32)EfiACPIReclaimMemory);
  ASSERT_EQ (gMemoryTypeInformation[3].NumberOfPages, (UINT32)20);
  ASSERT_EQ (gMemoryTypeInformation[4].Type, (UINT32)EfiACPIMemoryNVS);
  ASSERT_EQ (gMemoryTypeInformation[4].NumberOfPages, (UINT32)25);
  ASSERT_EQ (gMemoryTypeInformation[5].Type, (UINT32)EfiPalCode);
  ASSERT_EQ (gMemoryTypeInformation[5].NumberOfPages, (UINT32)8);
}

//
// Test: GetMemoryTypeInformationResourceHob returns NOT_FOUND when no resource HOB exists
//
TEST_F (BaseMemoryBinLibTest, GetMemoryTypeInformationResourceHobReturnsNotFoundWhenNoResourceHob) {
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  UINT8                        HobListBuffer[sizeof (EFI_HOB_HANDOFF_INFO_TABLE) + sizeof (EFI_HOB_GENERIC_HEADER)];
  EFI_HOB_HANDOFF_INFO_TABLE   *HandoffHob;
  EFI_HOB_GENERIC_HEADER       *EndOfHobList;
  VOID                         *HobStart;

  ZeroMem (HobListBuffer, sizeof (HobListBuffer));

  HandoffHob                   = (EFI_HOB_HANDOFF_INFO_TABLE *)HobListBuffer;
  HandoffHob->Header.HobType   = EFI_HOB_TYPE_HANDOFF;
  HandoffHob->Header.HobLength = sizeof (EFI_HOB_HANDOFF_INFO_TABLE);

  EndOfHobList            = (EFI_HOB_GENERIC_HEADER *)(HandoffHob + 1);
  EndOfHobList->HobType   = EFI_HOB_TYPE_END_OF_HOB_LIST;
  EndOfHobList->HobLength = sizeof (EFI_HOB_GENERIC_HEADER);

  HobStart = (VOID *)HobListBuffer;

  ResourceHob = GetMemoryTypeInformationResourceHob (&HobStart, gMemoryTypeInformation);

  ASSERT_EQ (ResourceHob, NULL);
}

//
// Test: GetMemoryTypeInformationResourceHob finds matching resource HOB
//
TEST_F (BaseMemoryBinLibTest, GetMemoryTypeInformationResourceHobFindsMatchingHob) {
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  UINT8                        HobListBuffer[sizeof (EFI_HOB_HANDOFF_INFO_TABLE) + 3 *sizeof (EFI_HOB_RESOURCE_DESCRIPTOR) + sizeof (EFI_HOB_GENERIC_HEADER)];
  EFI_HOB_HANDOFF_INFO_TABLE   *HandoffHob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceDescriptor;
  EFI_HOB_GENERIC_HEADER       *EndOfHobList;
  VOID                         *HobStart;
  EFI_GUID                     OtherGuid = { 0x12345678, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 }
  };

  ZeroMem (HobListBuffer, sizeof (HobListBuffer));

  HandoffHob                   = (EFI_HOB_HANDOFF_INFO_TABLE *)HobListBuffer;
  HandoffHob->Header.HobType   = EFI_HOB_TYPE_HANDOFF;
  HandoffHob->Header.HobLength = sizeof (EFI_HOB_HANDOFF_INFO_TABLE);

  ResourceDescriptor                   = (EFI_HOB_RESOURCE_DESCRIPTOR *)(HandoffHob + 1);
  ResourceDescriptor->Header.HobType   = EFI_HOB_TYPE_RESOURCE_DESCRIPTOR;
  ResourceDescriptor->Header.HobLength = sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
  ResourceDescriptor->ResourceType     = EFI_RESOURCE_SYSTEM_MEMORY;
  CopyGuid (&ResourceDescriptor->Owner, &OtherGuid);
  ResourceDescriptor->PhysicalStart  = 0x100000;
  ResourceDescriptor->ResourceLength = 0x50000;

  ResourceDescriptor                   = (EFI_HOB_RESOURCE_DESCRIPTOR *)(ResourceDescriptor + 1);
  ResourceDescriptor->Header.HobType   = EFI_HOB_TYPE_RESOURCE_DESCRIPTOR;
  ResourceDescriptor->Header.HobLength = sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
  ResourceDescriptor->ResourceType     = EFI_RESOURCE_SYSTEM_MEMORY;
  CopyGuid (&ResourceDescriptor->Owner, &gEfiMemoryTypeInformationGuid);
  ResourceDescriptor->PhysicalStart     = 0x200000;
  ResourceDescriptor->ResourceLength    = 0x75000;
  ResourceDescriptor->ResourceAttribute = EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_TESTED;

  ResourceDescriptor                   = (EFI_HOB_RESOURCE_DESCRIPTOR *)(ResourceDescriptor + 1);
  ResourceDescriptor->Header.HobType   = EFI_HOB_TYPE_RESOURCE_DESCRIPTOR;
  ResourceDescriptor->Header.HobLength = sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
  ResourceDescriptor->ResourceType     = EFI_RESOURCE_SYSTEM_MEMORY;
  CopyGuid (&ResourceDescriptor->Owner, &OtherGuid);
  ResourceDescriptor->PhysicalStart  = 0x300000;
  ResourceDescriptor->ResourceLength = 0x50000;

  EndOfHobList            = (EFI_HOB_GENERIC_HEADER *)(ResourceDescriptor + 1);
  EndOfHobList->HobType   = EFI_HOB_TYPE_END_OF_HOB_LIST;
  EndOfHobList->HobLength = sizeof (EFI_HOB_GENERIC_HEADER);

  HobStart = (VOID *)HobListBuffer;

  ResourceHob = GetMemoryTypeInformationResourceHob (&HobStart, gMemoryTypeInformation);

  ASSERT_EQ (ResourceHob->PhysicalStart, (EFI_PHYSICAL_ADDRESS)0x200000);
  ASSERT_EQ (ResourceHob->ResourceLength, (UINT64)0x75000);
}

//
// Test: GetMemoryTypeInformationResourceHob Fails When Multiple Matching HOBs Exist
//
TEST_F (BaseMemoryBinLibTest, GetMemoryTypeInformationResourceHobFailsWithMultipleMatchingHobs) {
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  UINT8                        HobListBuffer[sizeof (EFI_HOB_HANDOFF_INFO_TABLE) + 2 * sizeof (EFI_HOB_RESOURCE_DESCRIPTOR) + sizeof (EFI_HOB_GENERIC_HEADER)];
  EFI_HOB_HANDOFF_INFO_TABLE   *HandoffHob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *FirstMatchingResource;
  EFI_HOB_RESOURCE_DESCRIPTOR  *SecondMatchingResource;
  EFI_HOB_GENERIC_HEADER       *EndOfHobList;
  VOID                         *HobStart;

  ZeroMem (HobListBuffer, sizeof (HobListBuffer));

  HandoffHob                   = (EFI_HOB_HANDOFF_INFO_TABLE *)HobListBuffer;
  HandoffHob->Header.HobType   = EFI_HOB_TYPE_HANDOFF;
  HandoffHob->Header.HobLength = sizeof (EFI_HOB_HANDOFF_INFO_TABLE);

  // First matching resource HOB
  FirstMatchingResource                   = (EFI_HOB_RESOURCE_DESCRIPTOR *)(HandoffHob + 1);
  FirstMatchingResource->Header.HobType   = EFI_HOB_TYPE_RESOURCE_DESCRIPTOR;
  FirstMatchingResource->Header.HobLength = sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
  FirstMatchingResource->ResourceType     = EFI_RESOURCE_SYSTEM_MEMORY;
  CopyGuid (&FirstMatchingResource->Owner, &gEfiMemoryTypeInformationGuid);
  FirstMatchingResource->PhysicalStart     = 0x100000;
  FirstMatchingResource->ResourceLength    = 0x50000;
  FirstMatchingResource->ResourceAttribute = EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_TESTED;

  // Second matching resource HOB
  SecondMatchingResource                   = (EFI_HOB_RESOURCE_DESCRIPTOR *)(FirstMatchingResource + 1);
  SecondMatchingResource->Header.HobType   = EFI_HOB_TYPE_RESOURCE_DESCRIPTOR;
  SecondMatchingResource->Header.HobLength = sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
  SecondMatchingResource->ResourceType     = EFI_RESOURCE_SYSTEM_MEMORY;
  CopyGuid (&SecondMatchingResource->Owner, &gEfiMemoryTypeInformationGuid);
  SecondMatchingResource->PhysicalStart     = 0x400000;
  SecondMatchingResource->ResourceLength    = 0x80000;
  SecondMatchingResource->ResourceAttribute = EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_TESTED;

  EndOfHobList            = (EFI_HOB_GENERIC_HEADER *)(SecondMatchingResource + 1);
  EndOfHobList->HobType   = EFI_HOB_TYPE_END_OF_HOB_LIST;
  EndOfHobList->HobLength = sizeof (EFI_HOB_GENERIC_HEADER);

  HobStart = (VOID *)HobListBuffer;

  ResourceHob = GetMemoryTypeInformationResourceHob (&HobStart, gMemoryTypeInformation);

  ASSERT_EQ (ResourceHob, NULL);
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

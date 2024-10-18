/** @file
  Internal ARCH Specific file of MM memory check library.

  MM memory check library implementation. This library consumes MM_ACCESS_PROTOCOL
  to get MMRAM information. In order to use this library instance, the platform should produce
  all MMRAM range via MM_ACCESS_PROTOCOL, including the range for firmware (like MM Core
  and MM driver) and/or specific dedicated hardware.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <StandaloneMmMemLib.h>
#include <PiMm.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>

#include <Guid/MmramMemoryReserve.h>

NON_MM_MEMORY_MAP  *mValidNonMmramRanges;
UINTN              mValidNonMmramCount;

//
// Maximum support address used to check input buffer
//
extern EFI_PHYSICAL_ADDRESS  mMmMemLibInternalMaximumSupportAddress;

/**
  Calculate and save the maximum support address.

**/
VOID
MmMemLibInternalCalculateMaximumSupportAddress (
  VOID
  )
{
  VOID    *Hob;
  UINT32  RegEax;
  UINT8   PhysicalAddressBits;

  //
  // Get physical address bits supported.
  //
  Hob = GetFirstHob (EFI_HOB_TYPE_CPU);
  if (Hob != NULL) {
    PhysicalAddressBits = ((EFI_HOB_CPU *)Hob)->SizeOfMemorySpace;
  } else {
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000008) {
      AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
      PhysicalAddressBits = (UINT8)RegEax;
    } else {
      PhysicalAddressBits = 36;
    }
  }

  //
  // IA-32e paging translates 48-bit linear addresses to 52-bit physical addresses.
  //
  ASSERT (PhysicalAddressBits <= 52);
  if (PhysicalAddressBits > 48) {
    PhysicalAddressBits = 48;
  }

  //
  // Save the maximum support address in one global variable
  //
  mMmMemLibInternalMaximumSupportAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)(LShiftU64 (1, PhysicalAddressBits) - 1);
  DEBUG ((DEBUG_INFO, "mMmMemLibInternalMaximumSupportAddress = 0x%lx\n", mMmMemLibInternalMaximumSupportAddress));
}

/**
  Merge the overlapped or continuous ranges in input MemoryMap. This function is to optimize
  the process of checking whether a buffer range belongs to the range reported by resource HOB,
  since the buffer to be checked may be covered by multi resource HOB.

  @param[in, out]  MemoryMap              A pointer to the NonMmramRanges reported by resource HOB.
  @param[in, out]  MemoryMapSize          A pointer to the size, in bytes, of the MemoryMap buffer.
                                          On input, it is the size of the current memory map.
                                          On output, it is the size of new memory map after merge.
  @param[in]       DescriptorSize         Size, in bytes, of an individual NON_MM_MEMORY_MAP.
**/
STATIC
VOID
MergeOverlappedOrContinuousRanges (
  IN OUT NON_MM_MEMORY_MAP  *MemoryMap,
  IN OUT UINTN              *MemoryMapSize,
  IN UINTN                  DescriptorSize
  )
{
  NON_MM_MEMORY_MAP     *MemoryMapEntry;
  NON_MM_MEMORY_MAP     *MemoryMapEnd;
  NON_MM_MEMORY_MAP     *NewMemoryMapEntry;
  NON_MM_MEMORY_MAP     *NextMemoryMapEntry;
  EFI_PHYSICAL_ADDRESS  End;

  MemoryMapEntry    = MemoryMap;
  NewMemoryMapEntry = MemoryMap;
  MemoryMapEnd      = (NON_MM_MEMORY_MAP *)((UINT8 *)MemoryMap + *MemoryMapSize);
  while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
    NextMemoryMapEntry = NEXT_NON_MM_MEMORY_MAP (MemoryMapEntry, DescriptorSize);

    do {
      if (((UINTN)NextMemoryMapEntry < (UINTN)MemoryMapEnd) &&
          ((MemoryMapEntry->Base + MemoryMapEntry->Length) >= NextMemoryMapEntry->Base))
      {
        //
        // Merge the overlapped or continuous ranges.
        //
        End = MAX (
                MemoryMapEntry->Base + MemoryMapEntry->Length,
                NextMemoryMapEntry->Base + NextMemoryMapEntry->Length
                );
        MemoryMapEntry->Length = End - MemoryMapEntry->Base;

        NextMemoryMapEntry = NEXT_NON_MM_MEMORY_MAP (NextMemoryMapEntry, DescriptorSize);
        continue;
      } else {
        //
        // Copy the processed independent range to the new index location.
        //
        CopyMem (NewMemoryMapEntry, MemoryMapEntry, sizeof (NON_MM_MEMORY_MAP));
        break;
      }
    } while (TRUE);

    MemoryMapEntry    = NextMemoryMapEntry;
    NewMemoryMapEntry = NEXT_NON_MM_MEMORY_MAP (NewMemoryMapEntry, DescriptorSize);
  }

  *MemoryMapSize = (UINTN)NewMemoryMapEntry - (UINTN)MemoryMap;
}

/**
  Function to compare 2 NON_MM_MEMORY_MAP pointer based on Base.

  @param[in] Buffer1            pointer to NON_MM_MEMORY_MAP pointer to compare
  @param[in] Buffer2            pointer to second NON_MM_MEMORY_MAP pointer to compare

  @retval 0                     Buffer1 equal to Buffer2
  @retval <0                    Buffer1 is less than Buffer2
  @retval >0                    Buffer1 is greater than Buffer2
**/
INTN
EFIAPI
NonMmMapCompare (
  IN  CONST VOID  *Buffer1,
  IN  CONST VOID  *Buffer2
  )
{
  if (((NON_MM_MEMORY_MAP *)Buffer1)->Base > ((NON_MM_MEMORY_MAP *)Buffer2)->Base) {
    return 1;
  } else if (((NON_MM_MEMORY_MAP *)Buffer1)->Base < ((NON_MM_MEMORY_MAP *)Buffer2)->Base) {
    return -1;
  }

  return 0;
}

/**
  Initialize valid non-Mmram Ranges from Resource HOB.

**/
VOID
MmMemLibInternalPopulateValidNonMmramRanges (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 Count;
  UINTN                 Index;
  UINTN                 RangeSize;
  NON_MM_MEMORY_MAP     SortBuffer;

  mValidNonMmramRanges = NULL;
  mValidNonMmramCount  = 0;

  Count     = 0;
  Index     = 0;
  RangeSize = 0;

  //
  // 1. Get the count.
  //
  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  while (Hob.Raw != NULL) {
    Count++;
    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);
  }

  //
  // 2. Store the initial data.
  //
  RangeSize            = sizeof (NON_MM_MEMORY_MAP) * Count;
  mValidNonMmramRanges = (NON_MM_MEMORY_MAP *)AllocateZeroPool (RangeSize);
  ASSERT (mValidNonMmramRanges != NULL);

  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  while (Hob.Raw != NULL) {
    mValidNonMmramRanges[Index].Base   = Hob.ResourceDescriptor->PhysicalStart;
    mValidNonMmramRanges[Index].Length = Hob.ResourceDescriptor->ResourceLength;
    Index++;

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);
  }

  ASSERT (Index == Count);

  //
  // 3. Sort the data.
  //
  QuickSort (mValidNonMmramRanges, Count, sizeof (NON_MM_MEMORY_MAP), (BASE_SORT_COMPARE)NonMmMapCompare, &SortBuffer);

  //
  // 4. Merge the overlapped or continuous ranges.
  //
  MergeOverlappedOrContinuousRanges (mValidNonMmramRanges, &RangeSize, sizeof (NON_MM_MEMORY_MAP));
  mValidNonMmramCount = RangeSize/sizeof (NON_MM_MEMORY_MAP);
}

/**
  Deinitialize cached non-Mmram Ranges.

**/
VOID
MmMemLibInternalFreeNonMmramRanges (
  VOID
  )
{
  if (mValidNonMmramRanges != NULL) {
    FreePool (mValidNonMmramRanges);
  }
}

/**
  This function check if the buffer is valid non-MMRAM memory range.

  @param[in] Buffer  The buffer start address to be checked.
  @param[in] Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid non-MMRAM memory range.
  @retval FALSE This buffer is not valid non-MMRAM memory range.
**/
BOOLEAN
MmMemLibInternalIsValidNonMmramRange (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  UINTN  Index;

  for (Index = 0; Index < mValidNonMmramCount; Index++) {
    if ((Buffer >= mValidNonMmramRanges[Index].Base) &&
        (Buffer + Length <= mValidNonMmramRanges[Index].Base + mValidNonMmramRanges[Index].Length))
    {
      return TRUE;
    }
  }

  return FALSE;
}

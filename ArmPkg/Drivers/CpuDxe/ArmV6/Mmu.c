/*++

Copyright (c) 2009, Hewlett-Packard Company. All rights reserved.<BR>
Portions copyright (c) 2010, Apple Inc. All rights reserved.<BR>
Portions copyright (c) 2013, ARM Ltd. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


--*/

#include <Library/MemoryAllocationLib.h>
#include "CpuDxe.h"

// First Level Descriptors
typedef UINT32    ARM_FIRST_LEVEL_DESCRIPTOR;

// Second Level Descriptors
typedef UINT32    ARM_PAGE_TABLE_ENTRY;

EFI_STATUS
SectionToGcdAttributes (
  IN  UINT32  SectionAttributes,
  OUT UINT64  *GcdAttributes
  )
{
  *GcdAttributes = 0;

  // determine cacheability attributes
  switch(SectionAttributes & TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK) {
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_STRONGLY_ORDERED:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_SHAREABLE_DEVICE:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC:
      *GcdAttributes |= EFI_MEMORY_WT;
      break;
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK_NO_ALLOC:
      *GcdAttributes |= EFI_MEMORY_WB;
      break;
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_NON_CACHEABLE:
      *GcdAttributes |= EFI_MEMORY_WC;
      break;
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK_ALLOC:
      *GcdAttributes |= EFI_MEMORY_WB;
      break;
    case TT_DESCRIPTOR_SECTION_CACHE_POLICY_NON_SHAREABLE_DEVICE:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    default:
      return EFI_UNSUPPORTED;
  }

  // determine protection attributes
  switch(SectionAttributes & TT_DESCRIPTOR_SECTION_AP_MASK) {
    case TT_DESCRIPTOR_SECTION_AP_NO_NO: // no read, no write
      //*GcdAttributes |= EFI_MEMORY_WP | EFI_MEMORY_RP;
      break;

    case TT_DESCRIPTOR_SECTION_AP_RW_NO:
    case TT_DESCRIPTOR_SECTION_AP_RW_RW:
      // normal read/write access, do not add additional attributes
      break;

    // read only cases map to write-protect
    case TT_DESCRIPTOR_SECTION_AP_RO_NO:
    case TT_DESCRIPTOR_SECTION_AP_RO_RO:
      *GcdAttributes |= EFI_MEMORY_WP;
      break;

    default:
      return EFI_UNSUPPORTED;
  }

  // now process eXectue Never attribute
  if ((SectionAttributes & TT_DESCRIPTOR_SECTION_XN_MASK) != 0 ) {
    *GcdAttributes |= EFI_MEMORY_XP;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PageToGcdAttributes (
  IN  UINT32  PageAttributes,
  OUT UINT64  *GcdAttributes
  )
{
  *GcdAttributes = 0;

  // determine cacheability attributes
  switch(PageAttributes & TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK) {
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_STRONGLY_ORDERED:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_SHAREABLE_DEVICE:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC:
      *GcdAttributes |= EFI_MEMORY_WT;
      break;
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_WRITE_BACK_NO_ALLOC:
      *GcdAttributes |= EFI_MEMORY_WB;
      break;
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_NON_CACHEABLE:
      *GcdAttributes |= EFI_MEMORY_WC;
      break;
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_WRITE_BACK_ALLOC:
      *GcdAttributes |= EFI_MEMORY_WB;
      break;
    case TT_DESCRIPTOR_PAGE_CACHE_POLICY_NON_SHAREABLE_DEVICE:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    default:
      return EFI_UNSUPPORTED;
  }

  // determine protection attributes
  switch(PageAttributes & TT_DESCRIPTOR_PAGE_AP_MASK) {
    case TT_DESCRIPTOR_PAGE_AP_NO_NO: // no read, no write
      //*GcdAttributes |= EFI_MEMORY_WP | EFI_MEMORY_RP;
      break;

    case TT_DESCRIPTOR_PAGE_AP_RW_NO:
    case TT_DESCRIPTOR_PAGE_AP_RW_RW:
      // normal read/write access, do not add additional attributes
      break;

    // read only cases map to write-protect
    case TT_DESCRIPTOR_PAGE_AP_RO_NO:
    case TT_DESCRIPTOR_PAGE_AP_RO_RO:
      *GcdAttributes |= EFI_MEMORY_WP;
      break;

    default:
      return EFI_UNSUPPORTED;
  }

  // now process eXectue Never attribute
  if ((PageAttributes & TT_DESCRIPTOR_PAGE_XN_MASK) != 0 ) {
    *GcdAttributes |= EFI_MEMORY_XP;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SyncCacheConfigPage (
  IN     UINT32                             SectionIndex,
  IN     UINT32                             FirstLevelDescriptor,
  IN     UINTN                              NumberOfDescriptors,
  IN     EFI_GCD_MEMORY_SPACE_DESCRIPTOR    *MemorySpaceMap,
  IN OUT EFI_PHYSICAL_ADDRESS               *NextRegionBase,
  IN OUT UINT64                             *NextRegionLength,
  IN OUT UINT32                             *NextSectionAttributes
  )
{
  EFI_STATUS                          Status;
  UINT32                              i;
  volatile ARM_PAGE_TABLE_ENTRY       *SecondLevelTable;
  UINT32                              NextPageAttributes = 0;
  UINT32                              PageAttributes = 0;
  UINT32                              BaseAddress;
  UINT64                              GcdAttributes;

  // Get the Base Address from FirstLevelDescriptor;
  BaseAddress = TT_DESCRIPTOR_PAGE_BASE_ADDRESS(SectionIndex << TT_DESCRIPTOR_SECTION_BASE_SHIFT);

  // Convert SectionAttributes into PageAttributes
  NextPageAttributes =
      TT_DESCRIPTOR_CONVERT_TO_PAGE_CACHE_POLICY(*NextSectionAttributes,0) |
      TT_DESCRIPTOR_CONVERT_TO_PAGE_AP(*NextSectionAttributes);

  // obtain page table base
  SecondLevelTable = (ARM_PAGE_TABLE_ENTRY *)(FirstLevelDescriptor & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK);

  for (i=0; i < TRANSLATION_TABLE_PAGE_COUNT; i++) {
    if ((SecondLevelTable[i] & TT_DESCRIPTOR_PAGE_TYPE_MASK) == TT_DESCRIPTOR_PAGE_TYPE_PAGE) {
      // extract attributes (cacheability and permissions)
      PageAttributes = SecondLevelTable[i] & (TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK | TT_DESCRIPTOR_PAGE_AP_MASK);

      if (NextPageAttributes == 0) {
        // start on a new region
        *NextRegionLength = 0;
        *NextRegionBase = BaseAddress | (i << TT_DESCRIPTOR_PAGE_BASE_SHIFT);
        NextPageAttributes = PageAttributes;
      } else if (PageAttributes != NextPageAttributes) {
        // Convert Section Attributes into GCD Attributes
        Status = PageToGcdAttributes (NextPageAttributes, &GcdAttributes);
        ASSERT_EFI_ERROR (Status);

        // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
        SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, *NextRegionBase, *NextRegionLength, GcdAttributes);

        // start on a new region
        *NextRegionLength = 0;
        *NextRegionBase = BaseAddress | (i << TT_DESCRIPTOR_PAGE_BASE_SHIFT);
        NextPageAttributes = PageAttributes;
      }
    } else if (NextPageAttributes != 0) {
      // Convert Page Attributes into GCD Attributes
      Status = PageToGcdAttributes (NextPageAttributes, &GcdAttributes);
      ASSERT_EFI_ERROR (Status);

      // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
      SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, *NextRegionBase, *NextRegionLength, GcdAttributes);

      *NextRegionLength = 0;
      *NextRegionBase = BaseAddress | (i << TT_DESCRIPTOR_PAGE_BASE_SHIFT);
      NextPageAttributes = 0;
    }
    *NextRegionLength += TT_DESCRIPTOR_PAGE_SIZE;
  }

  // Convert back PageAttributes into SectionAttributes
  *NextSectionAttributes =
      TT_DESCRIPTOR_CONVERT_TO_SECTION_CACHE_POLICY(NextPageAttributes,0) |
      TT_DESCRIPTOR_CONVERT_TO_SECTION_AP(NextPageAttributes);

  return EFI_SUCCESS;
}

EFI_STATUS
SyncCacheConfig (
  IN  EFI_CPU_ARCH_PROTOCOL *CpuProtocol
  )
{
  EFI_STATUS                          Status;
  UINT32                              i;
  EFI_PHYSICAL_ADDRESS                NextRegionBase;
  UINT64                              NextRegionLength;
  UINT32                              NextSectionAttributes = 0;
  UINT32                              SectionAttributes = 0;
  UINT64                              GcdAttributes;
  volatile ARM_FIRST_LEVEL_DESCRIPTOR   *FirstLevelTable;
  UINTN                               NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR     *MemorySpaceMap;


  DEBUG ((EFI_D_PAGE, "SyncCacheConfig()\n"));

  // This code assumes MMU is enabled and filed with section translations
  ASSERT (ArmMmuEnabled ());

  //
  // Get the memory space map from GCD
  //
  MemorySpaceMap = NULL;
  Status = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  ASSERT_EFI_ERROR (Status);


  // The GCD implementation maintains its own copy of the state of memory space attributes.  GCD needs
  // to know what the initial memory space attributes are.  The CPU Arch. Protocol does not provide a
  // GetMemoryAttributes function for GCD to get this so we must resort to calling GCD (as if we were
  // a client) to update its copy of the attributes.  This is bad architecture and should be replaced
  // with a way for GCD to query the CPU Arch. driver of the existing memory space attributes instead.

  // obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)(ArmGetTTBR0BaseAddress ());

  // Get the first region
  NextSectionAttributes = FirstLevelTable[0] & (TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK | TT_DESCRIPTOR_SECTION_AP_MASK);

  // iterate through each 1MB descriptor
  NextRegionBase = NextRegionLength = 0;
  for (i=0; i < TRANSLATION_TABLE_SECTION_COUNT; i++) {
    if ((FirstLevelTable[i] & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SECTION) {
      // extract attributes (cacheability and permissions)
      SectionAttributes = FirstLevelTable[i] & (TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK | TT_DESCRIPTOR_SECTION_AP_MASK);

      if (NextSectionAttributes == 0) {
        // start on a new region
        NextRegionLength = 0;
        NextRegionBase = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(i << TT_DESCRIPTOR_SECTION_BASE_SHIFT);
        NextSectionAttributes = SectionAttributes;
      } else if (SectionAttributes != NextSectionAttributes) {
        // Convert Section Attributes into GCD Attributes
        Status = SectionToGcdAttributes (NextSectionAttributes, &GcdAttributes);
        ASSERT_EFI_ERROR (Status);

        // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
        SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, NextRegionBase, NextRegionLength, GcdAttributes);

        // start on a new region
        NextRegionLength = 0;
        NextRegionBase = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(i << TT_DESCRIPTOR_SECTION_BASE_SHIFT);
        NextSectionAttributes = SectionAttributes;
      }
      NextRegionLength += TT_DESCRIPTOR_SECTION_SIZE;
    } else if (TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE(FirstLevelTable[i])) {
      Status = SyncCacheConfigPage (
          i,FirstLevelTable[i],
          NumberOfDescriptors, MemorySpaceMap,
          &NextRegionBase,&NextRegionLength,&NextSectionAttributes);
      ASSERT_EFI_ERROR (Status);
    } else {
      // We do not support yet 16MB sections
      ASSERT ((FirstLevelTable[i] & TT_DESCRIPTOR_SECTION_TYPE_MASK) != TT_DESCRIPTOR_SECTION_TYPE_SUPERSECTION);

      // start on a new region
      if (NextSectionAttributes != 0) {
        // Convert Section Attributes into GCD Attributes
        Status = SectionToGcdAttributes (NextSectionAttributes, &GcdAttributes);
        ASSERT_EFI_ERROR (Status);

        // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
        SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, NextRegionBase, NextRegionLength, GcdAttributes);

        NextRegionLength = 0;
        NextRegionBase = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(i << TT_DESCRIPTOR_SECTION_BASE_SHIFT);
        NextSectionAttributes = 0;
      }
      NextRegionLength += TT_DESCRIPTOR_SECTION_SIZE;
    }
  } // section entry loop

  if (NextSectionAttributes != 0) {
    // Convert Section Attributes into GCD Attributes
    Status = SectionToGcdAttributes (NextSectionAttributes, &GcdAttributes);
    ASSERT_EFI_ERROR (Status);

    // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
    SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, NextRegionBase, NextRegionLength, GcdAttributes);
  }

  FreePool (MemorySpaceMap);

  return EFI_SUCCESS;
}



EFI_STATUS
UpdatePageEntries (
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes,
  IN EFI_PHYSICAL_ADDRESS      VirtualMask
  )
{
  EFI_STATUS    Status;
  UINT32        EntryValue;
  UINT32        EntryMask;
  UINT32        FirstLevelIdx;
  UINT32        Offset;
  UINT32        NumPageEntries;
  UINT32        Descriptor;
  UINT32        p;
  UINT32        PageTableIndex;
  UINT32        PageTableEntry;
  UINT32        CurrentPageTableEntry;
  VOID          *Mva;

  volatile ARM_FIRST_LEVEL_DESCRIPTOR   *FirstLevelTable;
  volatile ARM_PAGE_TABLE_ENTRY         *PageTable;

  Status = EFI_SUCCESS;

  // EntryMask: bitmask of values to change (1 = change this value, 0 = leave alone)
  // EntryValue: values at bit positions specified by EntryMask
  EntryMask = TT_DESCRIPTOR_PAGE_TYPE_MASK;
  EntryValue = TT_DESCRIPTOR_PAGE_TYPE_PAGE;
  // Although the PI spec is unclear on this the GCD guarantees that only
  // one Attribute bit is set at a time, so we can safely use a switch statement
  switch (Attributes) {
    case EFI_MEMORY_UC:
      // modify cacheability attributes
      EntryMask |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK;
      // map to strongly ordered
      EntryValue |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_STRONGLY_ORDERED; // TEX[2:0] = 0, C=0, B=0
      break;

    case EFI_MEMORY_WC:
      // modify cacheability attributes
      EntryMask |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK;
      // map to normal non-cachable
      EntryValue |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_NON_CACHEABLE; // TEX [2:0]= 001 = 0x2, B=0, C=0
      break;

    case EFI_MEMORY_WT:
      // modify cacheability attributes
      EntryMask |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK;
      // write through with no-allocate
      EntryValue |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC; // TEX [2:0] = 0, C=1, B=0
      break;

    case EFI_MEMORY_WB:
      // modify cacheability attributes
      EntryMask |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_MASK;
      // write back (with allocate)
      EntryValue |= TT_DESCRIPTOR_PAGE_CACHE_POLICY_WRITE_BACK_ALLOC; // TEX [2:0] = 001, C=1, B=1
      break;

    case EFI_MEMORY_WP:
    case EFI_MEMORY_XP:
    case EFI_MEMORY_UCE:
      // cannot be implemented UEFI definition unclear for ARM
      // Cause a page fault if these ranges are accessed.
      EntryValue = TT_DESCRIPTOR_PAGE_TYPE_FAULT;
      DEBUG ((EFI_D_PAGE, "SetMemoryAttributes(): setting page %lx with unsupported attribute %x will page fault on access\n", BaseAddress, Attributes));
      break;

    default:
      return EFI_UNSUPPORTED;
  }

  // Obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTTBR0BaseAddress ();

  // Calculate number of 4KB page table entries to change
  NumPageEntries = Length / TT_DESCRIPTOR_PAGE_SIZE;

  // Iterate for the number of 4KB pages to change
  Offset = 0;
  for(p = 0; p < NumPageEntries; p++) {
    // Calculate index into first level translation table for page table value

    FirstLevelIdx = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(BaseAddress + Offset) >> TT_DESCRIPTOR_SECTION_BASE_SHIFT;
    ASSERT (FirstLevelIdx < TRANSLATION_TABLE_SECTION_COUNT);

    // Read the descriptor from the first level page table
    Descriptor = FirstLevelTable[FirstLevelIdx];

    // Does this descriptor need to be converted from section entry to 4K pages?
    if (!TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE(Descriptor)) {
      Status = ConvertSectionToPages (FirstLevelIdx << TT_DESCRIPTOR_SECTION_BASE_SHIFT);
      if (EFI_ERROR(Status)) {
        // Exit for loop
        break;
      }

      // Re-read descriptor
      Descriptor = FirstLevelTable[FirstLevelIdx];
    }

    // Obtain page table base address
    PageTable = (ARM_PAGE_TABLE_ENTRY *)TT_DESCRIPTOR_PAGE_BASE_ADDRESS(Descriptor);

    // Calculate index into the page table
    PageTableIndex = ((BaseAddress + Offset) & TT_DESCRIPTOR_PAGE_INDEX_MASK) >> TT_DESCRIPTOR_PAGE_BASE_SHIFT;
    ASSERT (PageTableIndex < TRANSLATION_TABLE_PAGE_COUNT);

    // Get the entry
    CurrentPageTableEntry = PageTable[PageTableIndex];

    // Mask off appropriate fields
    PageTableEntry = CurrentPageTableEntry & ~EntryMask;

    // Mask in new attributes and/or permissions
    PageTableEntry |= EntryValue;

    if (VirtualMask != 0) {
      // Make this virtual address point at a physical page
      PageTableEntry &= ~VirtualMask;
    }

    if (CurrentPageTableEntry  != PageTableEntry) {
      Mva = (VOID *)(UINTN)((((UINTN)FirstLevelIdx) << TT_DESCRIPTOR_SECTION_BASE_SHIFT) + (PageTableIndex << TT_DESCRIPTOR_PAGE_BASE_SHIFT));
      if ((CurrentPageTableEntry & TT_DESCRIPTOR_PAGE_CACHEABLE_MASK) == TT_DESCRIPTOR_PAGE_CACHEABLE_MASK) {
        // The current section mapping is cacheable so Clean/Invalidate the MVA of the page
        // Note assumes switch(Attributes), not ARMv7 possibilities
        WriteBackInvalidateDataCacheRange (Mva, TT_DESCRIPTOR_PAGE_SIZE);
      }

      // Only need to update if we are changing the entry
      PageTable[PageTableIndex] = PageTableEntry;
      ArmUpdateTranslationTableEntry ((VOID *)&PageTable[PageTableIndex], Mva);
    }

    Status = EFI_SUCCESS;
    Offset += TT_DESCRIPTOR_PAGE_SIZE;

  } // End first level translation table loop

  return Status;
}



EFI_STATUS
UpdateSectionEntries (
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes,
  IN EFI_PHYSICAL_ADDRESS      VirtualMask
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  UINT32        EntryMask;
  UINT32        EntryValue;
  UINT32        FirstLevelIdx;
  UINT32        NumSections;
  UINT32        i;
  UINT32        CurrentDescriptor;
  UINT32        Descriptor;
  VOID          *Mva;
  volatile ARM_FIRST_LEVEL_DESCRIPTOR   *FirstLevelTable;

  // EntryMask: bitmask of values to change (1 = change this value, 0 = leave alone)
  // EntryValue: values at bit positions specified by EntryMask

  // Make sure we handle a section range that is unmapped
  EntryMask = TT_DESCRIPTOR_SECTION_TYPE_MASK;
  EntryValue = TT_DESCRIPTOR_SECTION_TYPE_SECTION;

  // Although the PI spec is unclear on this the GCD guarantees that only
  // one Attribute bit is set at a time, so we can safely use a switch statement
  switch(Attributes) {
    case EFI_MEMORY_UC:
      // modify cacheability attributes
      EntryMask |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK;
      // map to strongly ordered
      EntryValue |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_STRONGLY_ORDERED; // TEX[2:0] = 0, C=0, B=0
      break;

    case EFI_MEMORY_WC:
      // modify cacheability attributes
      EntryMask |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK;
      // map to normal non-cachable
      EntryValue |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_NON_CACHEABLE; // TEX [2:0]= 001 = 0x2, B=0, C=0
      break;

    case EFI_MEMORY_WT:
      // modify cacheability attributes
      EntryMask |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK;
      // write through with no-allocate
      EntryValue |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC; // TEX [2:0] = 0, C=1, B=0
      break;

    case EFI_MEMORY_WB:
      // modify cacheability attributes
      EntryMask |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_MASK;
      // write back (with allocate)
      EntryValue |= TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK_ALLOC; // TEX [2:0] = 001, C=1, B=1
      break;

    case EFI_MEMORY_WP:
    case EFI_MEMORY_XP:
    case EFI_MEMORY_RP:
    case EFI_MEMORY_UCE:
      // cannot be implemented UEFI definition unclear for ARM
      // Cause a page fault if these ranges are accessed.
      EntryValue = TT_DESCRIPTOR_SECTION_TYPE_FAULT;
      DEBUG ((EFI_D_PAGE, "SetMemoryAttributes(): setting section %lx with unsupported attribute %x will page fault on access\n", BaseAddress, Attributes));
      break;


    default:
      return EFI_UNSUPPORTED;
  }

  // obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTTBR0BaseAddress ();

  // calculate index into first level translation table for start of modification
  FirstLevelIdx = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(BaseAddress) >> TT_DESCRIPTOR_SECTION_BASE_SHIFT;
  ASSERT (FirstLevelIdx < TRANSLATION_TABLE_SECTION_COUNT);

  // calculate number of 1MB first level entries this applies to
  NumSections = Length / TT_DESCRIPTOR_SECTION_SIZE;

  // iterate through each descriptor
  for(i=0; i<NumSections; i++) {
    CurrentDescriptor = FirstLevelTable[FirstLevelIdx + i];

    // has this descriptor already been coverted to pages?
    if (TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE(CurrentDescriptor)) {
      // forward this 1MB range to page table function instead
      Status = UpdatePageEntries ((FirstLevelIdx + i) << TT_DESCRIPTOR_SECTION_BASE_SHIFT, TT_DESCRIPTOR_SECTION_SIZE, Attributes, VirtualMask);
    } else {
      // still a section entry

      // mask off appropriate fields
      Descriptor = CurrentDescriptor & ~EntryMask;

      // mask in new attributes and/or permissions
      Descriptor |= EntryValue;
      if (VirtualMask != 0) {
        Descriptor &= ~VirtualMask;
      }

      if (CurrentDescriptor  != Descriptor) {
        Mva = (VOID *)(UINTN)(((UINTN)FirstLevelTable) << TT_DESCRIPTOR_SECTION_BASE_SHIFT);
        if ((CurrentDescriptor & TT_DESCRIPTOR_SECTION_CACHEABLE_MASK) == TT_DESCRIPTOR_SECTION_CACHEABLE_MASK) {
          // The current section mapping is cacheable so Clean/Invalidate the MVA of the section
          // Note assumes switch(Attributes), not ARMv7 possabilities
          WriteBackInvalidateDataCacheRange (Mva, SIZE_1MB);
        }

        // Only need to update if we are changing the descriptor
        FirstLevelTable[FirstLevelIdx + i] = Descriptor;
        ArmUpdateTranslationTableEntry ((VOID *)&FirstLevelTable[FirstLevelIdx + i], Mva);
      }

      Status = EFI_SUCCESS;
    }
  }

  return Status;
}

EFI_STATUS
ConvertSectionToPages (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress
  )
{
  EFI_STATUS              Status;
  EFI_PHYSICAL_ADDRESS    PageTableAddr;
  UINT32                  FirstLevelIdx;
  UINT32                  SectionDescriptor;
  UINT32                  PageTableDescriptor;
  UINT32                  PageDescriptor;
  UINT32                  Index;

  volatile ARM_FIRST_LEVEL_DESCRIPTOR   *FirstLevelTable;
  volatile ARM_PAGE_TABLE_ENTRY         *PageTable;

  DEBUG ((EFI_D_PAGE, "Converting section at 0x%x to pages\n", (UINTN)BaseAddress));

  // Obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTTBR0BaseAddress ();

  // Calculate index into first level translation table for start of modification
  FirstLevelIdx = TT_DESCRIPTOR_SECTION_BASE_ADDRESS(BaseAddress) >> TT_DESCRIPTOR_SECTION_BASE_SHIFT;
  ASSERT (FirstLevelIdx < TRANSLATION_TABLE_SECTION_COUNT);

  // Get section attributes and convert to page attributes
  SectionDescriptor = FirstLevelTable[FirstLevelIdx];
  PageDescriptor = TT_DESCRIPTOR_PAGE_TYPE_PAGE | ConvertSectionAttributesToPageAttributes (SectionDescriptor, FALSE);

  // Allocate a page table for the 4KB entries (we use up a full page even though we only need 1KB)
  Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesData, 1, &PageTableAddr);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  PageTable = (volatile ARM_PAGE_TABLE_ENTRY *)(UINTN)PageTableAddr;

  // Write the page table entries out
  for (Index = 0; Index < TRANSLATION_TABLE_PAGE_COUNT; Index++) {
    PageTable[Index] = TT_DESCRIPTOR_PAGE_BASE_ADDRESS(BaseAddress + (Index << 12)) | PageDescriptor;
  }

  // Flush d-cache so descriptors make it back to uncached memory for subsequent table walks
  WriteBackInvalidateDataCacheRange ((VOID *)(UINTN)PageTableAddr, TT_DESCRIPTOR_PAGE_SIZE);

  // Formulate page table entry, Domain=0, NS=0
  PageTableDescriptor = (((UINTN)PageTableAddr) & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK) | TT_DESCRIPTOR_SECTION_TYPE_PAGE_TABLE;

  // Write the page table entry out, replacing section entry
  FirstLevelTable[FirstLevelIdx] = PageTableDescriptor;

  return EFI_SUCCESS;
}



EFI_STATUS
SetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes,
  IN EFI_PHYSICAL_ADDRESS      VirtualMask
  )
{
  EFI_STATUS    Status;

  if(((BaseAddress & 0xFFFFF) == 0) && ((Length & 0xFFFFF) == 0)) {
    // Is the base and length a multiple of 1 MB?
    DEBUG ((EFI_D_PAGE, "SetMemoryAttributes(): MMU section 0x%x length 0x%x to %lx\n", (UINTN)BaseAddress, (UINTN)Length, Attributes));
    Status = UpdateSectionEntries (BaseAddress, Length, Attributes, VirtualMask);
  } else {
    // Base and/or length is not a multiple of 1 MB
    DEBUG ((EFI_D_PAGE, "SetMemoryAttributes(): MMU page 0x%x length 0x%x to %lx\n", (UINTN)BaseAddress, (UINTN)Length, Attributes));
    Status = UpdatePageEntries (BaseAddress, Length, Attributes, VirtualMask);
  }

  // Flush d-cache so descriptors make it back to uncached memory for subsequent table walks
  // flush and invalidate pages
  //TODO: Do we really need to invalidate the caches everytime we change the memory attributes ?
  ArmCleanInvalidateDataCache ();

  ArmInvalidateInstructionCache ();

  // Invalidate all TLB entries so changes are synced
  ArmInvalidateTlb ();

  return Status;
}

UINT64
EfiAttributeToArmAttribute (
  IN UINT64                    EfiAttributes
  )
{
  UINT64 ArmAttributes;

  switch (EfiAttributes & EFI_MEMORY_CACHETYPE_MASK) {
    case EFI_MEMORY_UC:
      // Map to strongly ordered
      ArmAttributes = TT_DESCRIPTOR_SECTION_CACHE_POLICY_STRONGLY_ORDERED; // TEX[2:0] = 0, C=0, B=0
      break;

    case EFI_MEMORY_WC:
      // Map to normal non-cachable
      ArmAttributes = TT_DESCRIPTOR_SECTION_CACHE_POLICY_NON_CACHEABLE; // TEX [2:0]= 001 = 0x2, B=0, C=0
      break;

    case EFI_MEMORY_WT:
      // Write through with no-allocate
      ArmAttributes = TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC; // TEX [2:0] = 0, C=1, B=0
      break;

    case EFI_MEMORY_WB:
      // Write back (with allocate)
      ArmAttributes = TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK_ALLOC; // TEX [2:0] = 001, C=1, B=1
      break;

    case EFI_MEMORY_WP:
    case EFI_MEMORY_XP:
    case EFI_MEMORY_RP:
    case EFI_MEMORY_UCE:
    default:
      // Cannot be implemented UEFI definition unclear for ARM
      // Cause a page fault if these ranges are accessed.
      ArmAttributes = TT_DESCRIPTOR_SECTION_TYPE_FAULT;
      DEBUG ((EFI_D_PAGE, "SetMemoryAttributes(): Unsupported attribute %x will page fault on access\n", EfiAttributes));
      break;
  }

  // Determine protection attributes
  if (EfiAttributes & EFI_MEMORY_WP) {
    ArmAttributes |= TT_DESCRIPTOR_SECTION_AP_RO_RO;
  } else {
    ArmAttributes |= TT_DESCRIPTOR_SECTION_AP_RW_RW;
  }

  // Determine eXecute Never attribute
  if (EfiAttributes & EFI_MEMORY_XP) {
    ArmAttributes |= TT_DESCRIPTOR_SECTION_XN_MASK;
  }

  return ArmAttributes;
}

EFI_STATUS
GetMemoryRegionPage (
  IN     UINT32                  *PageTable,
  IN OUT UINTN                   *BaseAddress,
  OUT    UINTN                   *RegionLength,
  OUT    UINTN                   *RegionAttributes
  )
{
  UINT32      PageAttributes;
  UINT32      TableIndex;
  UINT32      PageDescriptor;

  // Convert the section attributes into page attributes
  PageAttributes = ConvertSectionAttributesToPageAttributes (*RegionAttributes, 0);

  // Calculate index into first level translation table for start of modification
  TableIndex = ((*BaseAddress) & TT_DESCRIPTOR_PAGE_INDEX_MASK)  >> TT_DESCRIPTOR_PAGE_BASE_SHIFT;
  ASSERT (TableIndex < TRANSLATION_TABLE_PAGE_COUNT);

  // Go through the page table to find the end of the section
  for (; TableIndex < TRANSLATION_TABLE_PAGE_COUNT; TableIndex++) {
    // Get the section at the given index
    PageDescriptor = PageTable[TableIndex];

    if ((PageDescriptor & TT_DESCRIPTOR_PAGE_TYPE_MASK) == TT_DESCRIPTOR_PAGE_TYPE_FAULT) {
      // Case: End of the boundary of the region
      return EFI_SUCCESS;
    } else if ((PageDescriptor & TT_DESCRIPTOR_PAGE_TYPE_PAGE) == TT_DESCRIPTOR_PAGE_TYPE_PAGE) {
      if ((PageDescriptor & TT_DESCRIPTOR_PAGE_ATTRIBUTE_MASK) == PageAttributes) {
        *RegionLength = *RegionLength + TT_DESCRIPTOR_PAGE_SIZE;
      } else {
        // Case: End of the boundary of the region
        return EFI_SUCCESS;
      }
    } else {
      // We do not support Large Page yet. We return EFI_SUCCESS that means end of the region.
      ASSERT(0);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
GetMemoryRegion (
  IN OUT UINTN                   *BaseAddress,
  OUT    UINTN                   *RegionLength,
  OUT    UINTN                   *RegionAttributes
  )
{
  EFI_STATUS                  Status;
  UINT32                      TableIndex;
  UINT32                      PageAttributes;
  UINT32                      PageTableIndex;
  UINT32                      SectionDescriptor;
  ARM_FIRST_LEVEL_DESCRIPTOR *FirstLevelTable;
  UINT32                     *PageTable;

  // Initialize the arguments
  *RegionLength = 0;

  // Obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTTBR0BaseAddress ();

  // Calculate index into first level translation table for start of modification
  TableIndex = TT_DESCRIPTOR_SECTION_BASE_ADDRESS (*BaseAddress) >> TT_DESCRIPTOR_SECTION_BASE_SHIFT;
  ASSERT (TableIndex < TRANSLATION_TABLE_SECTION_COUNT);

  // Get the section at the given index
  SectionDescriptor = FirstLevelTable[TableIndex];

  // If 'BaseAddress' belongs to the section then round it to the section boundary
  if (((SectionDescriptor & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SECTION) ||
      ((SectionDescriptor & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SUPERSECTION))
  {
    *BaseAddress = (*BaseAddress) & TT_DESCRIPTOR_SECTION_BASE_ADDRESS_MASK;
    *RegionAttributes = SectionDescriptor & TT_DESCRIPTOR_SECTION_ATTRIBUTE_MASK;
  } else {
    // Otherwise, we round it to the page boundary
    *BaseAddress = (*BaseAddress) & TT_DESCRIPTOR_PAGE_BASE_ADDRESS_MASK;

    // Get the attribute at the page table level (Level 2)
    PageTable = (UINT32*)(SectionDescriptor & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK);

    // Calculate index into first level translation table for start of modification
    PageTableIndex = ((*BaseAddress) & TT_DESCRIPTOR_PAGE_INDEX_MASK)  >> TT_DESCRIPTOR_PAGE_BASE_SHIFT;
    ASSERT (PageTableIndex < TRANSLATION_TABLE_PAGE_COUNT);

    PageAttributes = PageTable[PageTableIndex] & TT_DESCRIPTOR_PAGE_ATTRIBUTE_MASK;
    *RegionAttributes = TT_DESCRIPTOR_CONVERT_TO_SECTION_CACHE_POLICY (PageAttributes, 0) |
                        TT_DESCRIPTOR_CONVERT_TO_SECTION_AP (PageAttributes);
  }

  for (;TableIndex < TRANSLATION_TABLE_SECTION_COUNT; TableIndex++) {
    // Get the section at the given index
    SectionDescriptor = FirstLevelTable[TableIndex];

    // If the entry is a level-2 page table then we scan it to find the end of the region
    if (TT_DESCRIPTOR_SECTION_TYPE_IS_PAGE_TABLE (SectionDescriptor)) {
      // Extract the page table location from the descriptor
      PageTable = (UINT32*)(SectionDescriptor & TT_DESCRIPTOR_SECTION_PAGETABLE_ADDRESS_MASK);

      // Scan the page table to find the end of the region.
      Status = GetMemoryRegionPage (PageTable, BaseAddress, RegionLength, RegionAttributes);

      // If we have found the end of the region (Status == EFI_SUCCESS) then we exit the for-loop
      if (Status == EFI_SUCCESS) {
        break;
      }
    } else if (((SectionDescriptor & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SECTION) ||
               ((SectionDescriptor & TT_DESCRIPTOR_SECTION_TYPE_MASK) == TT_DESCRIPTOR_SECTION_TYPE_SUPERSECTION)) {
      if ((SectionDescriptor & TT_DESCRIPTOR_SECTION_ATTRIBUTE_MASK) != *RegionAttributes) {
        // If the attributes of the section differ from the one targeted then we exit the loop
        break;
      } else {
        *RegionLength = *RegionLength + TT_DESCRIPTOR_SECTION_SIZE;
      }
    } else {
      // If we are on an invalid section then it means it is the end of our section.
      break;
    }
  }

  return EFI_SUCCESS;
}

/** @file
  x64 Virtual Memory Management Services in the form of an IA-32 driver.  
  Used to establish a 1:1 Virtual to Physical Mapping that is required to
  enter Long Mode (x64 64-bit mode).

  While we make a 1:1 mapping (identity mapping) for all physical pages 
  we still need to use the MTRR's to ensure that the cachability attirbutes
  for all memory regions is correct.

  The basic idea is to use 2MB page table entries where ever possible. If
  more granularity of cachability is required then 4K page tables are used.

  References:
    1) IA-32 Intel(R) Atchitecture Software Developer's Manual Volume 1:Basic Architecture, Intel
    2) IA-32 Intel(R) Atchitecture Software Developer's Manual Volume 2:Instruction Set Reference, Intel
    3) IA-32 Intel(R) Atchitecture Software Developer's Manual Volume 3:System Programmer's Guide, Intel

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/  

#include "DxeIpl.h"
#include "VirtualMemory.h"






/**
  Allocates and fills in the Page Directory and Page Table Entries to
  establish a 1:1 Virtual to Physical mapping.

  @param  NumberOfProcessorPhysicalAddressBits  Number of processor address bits 
                                                to use. Limits the number of page 
                                                table entries  to the physical 
                                                address space. 

  @return EFI_SUCCESS           The 1:1 Virtual to Physical identity mapping was created

**/
UINTN
CreateIdentityMappingPageTables (
  VOID
  )
{  
  UINT8                                         PhysicalAddressBits;
  EFI_PHYSICAL_ADDRESS                          PageAddress;
  UINTN                                         IndexOfPml4Entries;
  UINTN                                         IndexOfPdpEntries;
  UINTN                                         IndexOfPageDirectoryEntries;
  UINTN                                         NumberOfPml4EntriesNeeded;
  UINTN                                         NumberOfPdpEntriesNeeded;
  PAGE_MAP_AND_DIRECTORY_POINTER                *PageMapLevel4Entry;
  PAGE_MAP_AND_DIRECTORY_POINTER                *PageMap;
  PAGE_MAP_AND_DIRECTORY_POINTER                *PageDirectoryPointerEntry;
  PAGE_TABLE_ENTRY                              *PageDirectoryEntry;
  UINTN                                         TotalPagesNum;
  UINTN                                         BigPageAddress;
  VOID                                          *Hob;

  //
  // Get physical address bits supported from CPU HOB.
  //
  PhysicalAddressBits = 36;
  
  Hob = GetFirstHob (EFI_HOB_TYPE_CPU);
  if (Hob != NULL) {
    PhysicalAddressBits = ((EFI_HOB_CPU *) Hob)->SizeOfMemorySpace;    
  }

  //
  // Calculate the table entries needed.
  //
  if (PhysicalAddressBits <= 39 ) {
    NumberOfPml4EntriesNeeded = 1;
    NumberOfPdpEntriesNeeded =  1 << (PhysicalAddressBits - 30);
  } else {
    NumberOfPml4EntriesNeeded = 1 << (PhysicalAddressBits - 39);
    NumberOfPdpEntriesNeeded = 512;
  }

  //
  // Pre-allocate big pages to avoid later allocations. 
  //
  TotalPagesNum = (NumberOfPdpEntriesNeeded + 1) * NumberOfPml4EntriesNeeded + 1;
  BigPageAddress = (UINTN) AllocatePages (TotalPagesNum);
  ASSERT (BigPageAddress != 0);

  //
  // By architecture only one PageMapLevel4 exists - so lets allocate storage for it.
  //
  PageMap         = (VOID *) BigPageAddress;
  BigPageAddress += EFI_PAGE_SIZE;

  PageMapLevel4Entry = PageMap;
  PageAddress        = 0;
  for (IndexOfPml4Entries = 0; IndexOfPml4Entries < NumberOfPml4EntriesNeeded; IndexOfPml4Entries++, PageMapLevel4Entry++) {
    //
    // Each PML4 entry points to a page of Page Directory Pointer entires.
    // So lets allocate space for them and fill them in in the IndexOfPdpEntries loop.
    //
    PageDirectoryPointerEntry = (VOID *) BigPageAddress;
    BigPageAddress += EFI_PAGE_SIZE;

    //
    // Make a PML4 Entry
    //
    PageMapLevel4Entry->Uint64 = (UINT64)(UINTN)PageDirectoryPointerEntry;
    PageMapLevel4Entry->Bits.ReadWrite = 1;
    PageMapLevel4Entry->Bits.Present = 1;

    for (IndexOfPdpEntries = 0; IndexOfPdpEntries < NumberOfPdpEntriesNeeded; IndexOfPdpEntries++, PageDirectoryPointerEntry++) {
      //
      // Each Directory Pointer entries points to a page of Page Directory entires.
      // So allocate space for them and fill them in in the IndexOfPageDirectoryEntries loop.
      //       
      PageDirectoryEntry = (VOID *) BigPageAddress;
      BigPageAddress += EFI_PAGE_SIZE;

      //
      // Fill in a Page Directory Pointer Entries
      //
      PageDirectoryPointerEntry->Uint64 = (UINT64)(UINTN)PageDirectoryEntry;
      PageDirectoryPointerEntry->Bits.ReadWrite = 1;
      PageDirectoryPointerEntry->Bits.Present = 1;

      for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectoryEntry++, PageAddress += 0x200000) {
        //
        // Fill in the Page Directory entries
        //
        PageDirectoryEntry->Uint64 = (UINT64)PageAddress;
        PageDirectoryEntry->Bits.ReadWrite = 1;
        PageDirectoryEntry->Bits.Present = 1;
        PageDirectoryEntry->Bits.MustBe1 = 1;

      }
    }
  }

  //
  // For the PML4 entries we are not using fill in a null entry.
  // For now we just copy the first entry.
  //
  for (; IndexOfPml4Entries < 512; IndexOfPml4Entries++, PageMapLevel4Entry++) {
     CopyMem (
       PageMapLevel4Entry,
       PageMap,
       sizeof (PAGE_MAP_AND_DIRECTORY_POINTER)
       );
  }

  return (UINTN)PageMap;
}


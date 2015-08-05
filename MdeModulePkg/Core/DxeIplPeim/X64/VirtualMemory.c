/** @file
  x64 Virtual Memory Management Services in the form of an IA-32 driver.  
  Used to establish a 1:1 Virtual to Physical Mapping that is required to
  enter Long Mode (x64 64-bit mode).

  While we make a 1:1 mapping (identity mapping) for all physical pages 
  we still need to use the MTRR's to ensure that the cachability attributes
  for all memory regions is correct.

  The basic idea is to use 2MB page table entries where ever possible. If
  more granularity of cachability is required then 4K page tables are used.

  References:
    1) IA-32 Intel(R) Architecture Software Developer's Manual Volume 1:Basic Architecture, Intel
    2) IA-32 Intel(R) Architecture Software Developer's Manual Volume 2:Instruction Set Reference, Intel
    3) IA-32 Intel(R) Architecture Software Developer's Manual Volume 3:System Programmer's Guide, Intel

Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/  

#include "DxeIpl.h"
#include "VirtualMemory.h"

/**
  Enable Execute Disable Bit.

**/
VOID
EnableExecuteDisableBit (
  VOID
  )
{
  UINT64           MsrRegisters;

  MsrRegisters = AsmReadMsr64 (0xC0000080);
  MsrRegisters |= BIT11;
  AsmWriteMsr64 (0xC0000080, MsrRegisters);
}

/**
  Split 2M page to 4K.

  @param[in]      PhysicalAddress       Start physical address the 2M page covered.
  @param[in, out] PageEntry2M           Pointer to 2M page entry.
  @param[in]      StackBase             Stack base address.
  @param[in]      StackSize             Stack size.

**/
VOID
Split2MPageTo4K (
  IN EFI_PHYSICAL_ADDRESS               PhysicalAddress,
  IN OUT UINT64                         *PageEntry2M,
  IN EFI_PHYSICAL_ADDRESS               StackBase,
  IN UINTN                              StackSize
  )
{
  EFI_PHYSICAL_ADDRESS                  PhysicalAddress4K;
  UINTN                                 IndexOfPageTableEntries;
  PAGE_TABLE_4K_ENTRY                   *PageTableEntry;

  PageTableEntry = AllocatePages (1);
  //
  // Fill in 2M page entry.
  //
  *PageEntry2M = (UINT64) (UINTN) PageTableEntry | IA32_PG_P | IA32_PG_RW;

  PhysicalAddress4K = PhysicalAddress;
  for (IndexOfPageTableEntries = 0; IndexOfPageTableEntries < 512; IndexOfPageTableEntries++, PageTableEntry++, PhysicalAddress4K += SIZE_4KB) {
    //
    // Fill in the Page Table entries
    //
    PageTableEntry->Uint64 = (UINT64) PhysicalAddress4K;
    PageTableEntry->Bits.ReadWrite = 1;
    PageTableEntry->Bits.Present = 1;
    if ((PhysicalAddress4K >= StackBase) && (PhysicalAddress4K < StackBase + StackSize)) {
      //
      // Set Nx bit for stack.
      //
      PageTableEntry->Bits.Nx = 1;
    }
  }
}

/**
  Split 1G page to 2M.

  @param[in]      PhysicalAddress       Start physical address the 1G page covered.
  @param[in, out] PageEntry1G           Pointer to 1G page entry.
  @param[in]      StackBase             Stack base address.
  @param[in]      StackSize             Stack size.

**/
VOID
Split1GPageTo2M (
  IN EFI_PHYSICAL_ADDRESS               PhysicalAddress,
  IN OUT UINT64                         *PageEntry1G,
  IN EFI_PHYSICAL_ADDRESS               StackBase,
  IN UINTN                              StackSize
  )
{
  EFI_PHYSICAL_ADDRESS                  PhysicalAddress2M;
  UINTN                                 IndexOfPageDirectoryEntries;
  PAGE_TABLE_ENTRY                      *PageDirectoryEntry;

  PageDirectoryEntry = AllocatePages (1);
  //
  // Fill in 1G page entry.
  //
  *PageEntry1G = (UINT64) (UINTN) PageDirectoryEntry | IA32_PG_P | IA32_PG_RW;

  PhysicalAddress2M = PhysicalAddress;
  for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectoryEntry++, PhysicalAddress2M += SIZE_2MB) {
    if ((PhysicalAddress2M < StackBase + StackSize) && ((PhysicalAddress2M + SIZE_2MB) > StackBase)) {
      //
      // Need to split this 2M page that covers stack range.
      //
      Split2MPageTo4K (PhysicalAddress2M, (UINT64 *) PageDirectoryEntry, StackBase, StackSize);
    } else {
      //
      // Fill in the Page Directory entries
      //
      PageDirectoryEntry->Uint64 = (UINT64) PhysicalAddress2M;
      PageDirectoryEntry->Bits.ReadWrite = 1;
      PageDirectoryEntry->Bits.Present = 1;
      PageDirectoryEntry->Bits.MustBe1 = 1;
    }
  }
}

/**
  Allocates and fills in the Page Directory and Page Table Entries to
  establish a 1:1 Virtual to Physical mapping.

  @param[in] StackBase  Stack base address.
  @param[in] StackSize  Stack size.

  @return The address of 4 level page map.

**/
UINTN
CreateIdentityMappingPageTables (
  IN EFI_PHYSICAL_ADDRESS   StackBase,
  IN UINTN                  StackSize
  )
{  
  UINT32                                        RegEax;
  UINT32                                        RegEdx;
  UINT8                                         PhysicalAddressBits;
  EFI_PHYSICAL_ADDRESS                          PageAddress;
  UINTN                                         IndexOfPml4Entries;
  UINTN                                         IndexOfPdpEntries;
  UINTN                                         IndexOfPageDirectoryEntries;
  UINT32                                        NumberOfPml4EntriesNeeded;
  UINT32                                        NumberOfPdpEntriesNeeded;
  PAGE_MAP_AND_DIRECTORY_POINTER                *PageMapLevel4Entry;
  PAGE_MAP_AND_DIRECTORY_POINTER                *PageMap;
  PAGE_MAP_AND_DIRECTORY_POINTER                *PageDirectoryPointerEntry;
  PAGE_TABLE_ENTRY                              *PageDirectoryEntry;
  UINTN                                         TotalPagesNum;
  UINTN                                         BigPageAddress;
  VOID                                          *Hob;
  BOOLEAN                                       Page1GSupport;
  PAGE_TABLE_1G_ENTRY                           *PageDirectory1GEntry;

  Page1GSupport = FALSE;
  if (PcdGetBool(PcdUse1GPageTable)) {
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000001) {
      AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
      if ((RegEdx & BIT26) != 0) {
        Page1GSupport = TRUE;
      }
    }
  }

  //
  // Get physical address bits supported.
  //
  Hob = GetFirstHob (EFI_HOB_TYPE_CPU);
  if (Hob != NULL) {
    PhysicalAddressBits = ((EFI_HOB_CPU *) Hob)->SizeOfMemorySpace;
  } else {
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000008) {
      AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
      PhysicalAddressBits = (UINT8) RegEax;
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
  // Calculate the table entries needed.
  //
  if (PhysicalAddressBits <= 39 ) {
    NumberOfPml4EntriesNeeded = 1;
    NumberOfPdpEntriesNeeded = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 30));
  } else {
    NumberOfPml4EntriesNeeded = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 39));
    NumberOfPdpEntriesNeeded = 512;
  }

  //
  // Pre-allocate big pages to avoid later allocations. 
  //
  if (!Page1GSupport) {
    TotalPagesNum = (NumberOfPdpEntriesNeeded + 1) * NumberOfPml4EntriesNeeded + 1;
  } else {
    TotalPagesNum = NumberOfPml4EntriesNeeded + 1;
  }
  BigPageAddress = (UINTN) AllocatePages (TotalPagesNum);
  ASSERT (BigPageAddress != 0);

  //
  // By architecture only one PageMapLevel4 exists - so lets allocate storage for it.
  //
  PageMap         = (VOID *) BigPageAddress;
  BigPageAddress += SIZE_4KB;

  PageMapLevel4Entry = PageMap;
  PageAddress        = 0;
  for (IndexOfPml4Entries = 0; IndexOfPml4Entries < NumberOfPml4EntriesNeeded; IndexOfPml4Entries++, PageMapLevel4Entry++) {
    //
    // Each PML4 entry points to a page of Page Directory Pointer entires.
    // So lets allocate space for them and fill them in in the IndexOfPdpEntries loop.
    //
    PageDirectoryPointerEntry = (VOID *) BigPageAddress;
    BigPageAddress += SIZE_4KB;

    //
    // Make a PML4 Entry
    //
    PageMapLevel4Entry->Uint64 = (UINT64)(UINTN)PageDirectoryPointerEntry;
    PageMapLevel4Entry->Bits.ReadWrite = 1;
    PageMapLevel4Entry->Bits.Present = 1;

    if (Page1GSupport) {
      PageDirectory1GEntry = (VOID *) PageDirectoryPointerEntry;
    
      for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectory1GEntry++, PageAddress += SIZE_1GB) {
        if (PcdGetBool (PcdSetNxForStack) && (PageAddress < StackBase + StackSize) && ((PageAddress + SIZE_1GB) > StackBase)) {
          Split1GPageTo2M (PageAddress, (UINT64 *) PageDirectory1GEntry, StackBase, StackSize);
        } else {
          //
          // Fill in the Page Directory entries
          //
          PageDirectory1GEntry->Uint64 = (UINT64)PageAddress;
          PageDirectory1GEntry->Bits.ReadWrite = 1;
          PageDirectory1GEntry->Bits.Present = 1;
          PageDirectory1GEntry->Bits.MustBe1 = 1;
        }
      }
    } else {
      for (IndexOfPdpEntries = 0; IndexOfPdpEntries < NumberOfPdpEntriesNeeded; IndexOfPdpEntries++, PageDirectoryPointerEntry++) {
        //
        // Each Directory Pointer entries points to a page of Page Directory entires.
        // So allocate space for them and fill them in in the IndexOfPageDirectoryEntries loop.
        //       
        PageDirectoryEntry = (VOID *) BigPageAddress;
        BigPageAddress += SIZE_4KB;

        //
        // Fill in a Page Directory Pointer Entries
        //
        PageDirectoryPointerEntry->Uint64 = (UINT64)(UINTN)PageDirectoryEntry;
        PageDirectoryPointerEntry->Bits.ReadWrite = 1;
        PageDirectoryPointerEntry->Bits.Present = 1;

        for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectoryEntry++, PageAddress += SIZE_2MB) {
          if (PcdGetBool (PcdSetNxForStack) && (PageAddress < StackBase + StackSize) && ((PageAddress + SIZE_2MB) > StackBase)) {
            //
            // Need to split this 2M page that covers stack range.
            //
            Split2MPageTo4K (PageAddress, (UINT64 *) PageDirectoryEntry, StackBase, StackSize);
          } else {
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

      for (; IndexOfPdpEntries < 512; IndexOfPdpEntries++, PageDirectoryPointerEntry++) {
        ZeroMem (
          PageDirectoryPointerEntry,
          sizeof(PAGE_MAP_AND_DIRECTORY_POINTER)
          );
      }
    }
  }

  //
  // For the PML4 entries we are not using fill in a null entry.
  //
  for (; IndexOfPml4Entries < 512; IndexOfPml4Entries++, PageMapLevel4Entry++) {
    ZeroMem (
      PageMapLevel4Entry,
      sizeof (PAGE_MAP_AND_DIRECTORY_POINTER)
      );
  }

  if (PcdGetBool (PcdSetNxForStack)) {
    EnableExecuteDisableBit ();
  }

  return (UINTN)PageMap;
}


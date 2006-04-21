/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  VirtualMemory.c
  
Abstract:

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
  
--*/  

#include "VirtualMemory.h"

x64_MTRR_VARIABLE_RANGE     *mMTRRVariableRange;
x64_MTRR_FIXED_RANGE        mMTRRFixedRange;


//
// Physial memory limit values for each of the 11 fixed MTRRs
//
UINTN mFixedRangeLimit[] = {
  0x7FFFF,  // Fixed MTRR  #0 describes 0x00000..0x7FFFF
  0x9FFFF,  // Fixed MTRR  #1 describes 0x80000..0x9FFFF
  0xBFFFF,  // Fixed MTRR  #2 describes 0xA0000..0xBFFFF
  0xC7FFF,  // Fixed MTRR  #3 describes 0xC0000..0xC7FFF
  0xCFFFF,  // Fixed MTRR  #4 describes 0xC8000..0xCFFFF
  0xD7FFF,  // Fixed MTRR  #5 describes 0xD0000..0xD7FFF
  0xDFFFF,  // Fixed MTRR  #6 describes 0xD8000..0xDFFFF
  0xE7FFF,  // Fixed MTRR  #7 describes 0xE0000..0xE7FFF
  0xEFFFF,  // Fixed MTRR  #8 describes 0xE8000..0xEFFFF
  0xF7FFF,  // Fixed MTRR  #9 describes 0xF0000..0xF7FFF
  0xFFFFF   // Fixed MTRR #10 describes 0xF8000..0xFFFFF
};

//
// The size, in bits, of each of the 11 fixed MTRR.
//
UINTN mFixedRangeShift[] = {
  16,   // Fixed MTRR  #0 describes 8, 64 KB ranges
  14,   // Fixed MTRR  #1 describes 8, 16 KB ranges
  14,   // Fixed MTRR  #2 describes 8, 16 KB ranges
  12,   // Fixed MTRR  #3 describes 8, 4 KB ranges
  12,   // Fixed MTRR  #4 describes 8, 4 KB ranges
  12,   // Fixed MTRR  #5 describes 8, 4 KB ranges
  12,   // Fixed MTRR  #6 describes 8, 4 KB ranges
  12,   // Fixed MTRR  #7 describes 8, 4 KB ranges
  12,   // Fixed MTRR  #8 describes 8, 4 KB ranges
  12,   // Fixed MTRR  #9 describes 8, 4 KB ranges
  12    // Fixed MTRR #10 describes 8, 4 KB ranges
};


UINTN mPowerOf2[] = {
  1,
  2,
  4,
  8,
  16,
  32,
  64,
  128,
  256,
  512
};

x64_MTRR_MEMORY_TYPE
EfiGetMTRRMemoryType (
  IN  EFI_PHYSICAL_ADDRESS      Address
  )
/*++

Routine Description:

  Retrieves the memory type from the MTRR that describes a physical address.

Arguments:

  VariableRange - Set of Variable MTRRs

  FixedRange    - Set of Fixed MTRRs

  Address       - The physical address for which the MTRR memory type is being retrieved

Returns:

  The MTRR Memory Type for the physical memory specified by Address.

--*/
{
  UINTN                   Index;
  UINTN                   TypeIndex;
  BOOLEAN                 Found;
  x64_MTRR_MEMORY_TYPE  VariableType;
  EFI_PHYSICAL_ADDRESS    MaskBase;
  EFI_PHYSICAL_ADDRESS    PhysMask;

  //
  // If the MTRRs are disabled, then return the Uncached Memory Type
  //
  if (mMTRRFixedRange.DefaultType.Bits.E == 0) {
    return Uncached;
  }

  //
  // If the CPU supports Fixed MTRRs and the Fixed MTRRs are enabled, then 
  // see if Address falls into one of the Fixed MTRRs
  //
  if (mMTRRFixedRange.Capabilities.Bits.FIX && mMTRRFixedRange.DefaultType.Bits.FE) {
    //
    // Loop though 11 fixed MTRRs
    //
    for (Index = 0; Index < 11; Index++) {
      //
      // Check for a matching range
      //
      if (Address <= mFixedRangeLimit[Index]) {
        //
        // Compute the offset address into the MTRR bu subtrating the base address of the MTRR
        //
        if (Index > 0) {
          Address = Address - (mFixedRangeLimit[Index-1] + 1);
        }
        //
        // Retrieve the index into the MTRR to extract the memory type.  The range is 0..7
        //
        TypeIndex = (UINTN)RShiftU64 (Address, mFixedRangeShift[Index]);
        
        //
        // Retrieve and return the memory type for the matching range
        //
        return mMTRRFixedRange.Fixed[Index].Type[TypeIndex];
      }
    }
  }

  //
  // If Address was not found in a Fixed MTRR, then search the Variable MTRRs
  //
  for (Index = 0, Found = FALSE, VariableType = WriteBack; Index < mMTRRFixedRange.Capabilities.Bits.VCNT; Index++) {
    //
    // BugBug: __aullshr complier error
    //
    if ((mMTRRVariableRange[Index].PhysMask.Uint64 & 0x800) == 0x800) {    
    //if (mMTRRVariableRange[Index].PhysMask.Bits.Valid == 1) {
      PhysMask = mMTRRVariableRange[Index].PhysMask.Uint64 & ~0xfff;
      MaskBase = PhysMask & (mMTRRVariableRange[Index].PhysBase.Uint64 & ~0xfff);
      if (MaskBase == (PhysMask & Address)) {
        //
        // Check to see how many matches we find
        //
        Found = TRUE;
        if ((mMTRRVariableRange[Index].PhysBase.Bits.Type == Uncached) || (VariableType == Uncached)) {
          //
          // If any matching region uses UC, the memory region is UC
          //
          VariableType = Uncached;
        } else if ((mMTRRVariableRange[Index].PhysBase.Bits.Type == WriteThrough) || (VariableType == WriteThrough)){
          //
          // If it's WT and WB then set it to WT. If it's WT and other type it's undefined
          //
          VariableType = WriteThrough;
        } else {
          VariableType = mMTRRVariableRange[Index].PhysBase.Bits.Type;
        }
      }
    }
  }
  
  if (Found) {
    return VariableType;
  }

  //
  // Address was not found in the Fixed or Variable MTRRs, so return the default memory type
  //
  return mMTRRFixedRange.DefaultType.Bits.Type;
}


BOOLEAN
CanNotUse2MBPage (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress
  )
/*++

Routine Description:
  Test to see if a 2MB aligned page has all the same attributes. If a 2MB page
  has more than one attibute type it needs to be split into multiple 4K pages.

Arguments:
  BaseAddress - 2MB aligned address to check out

Returns:
  TRUE  - This 2MB address range (BaseAddress) can NOT be mapped by a 2MB page
  FALSE - This 2MB address range can be mapped by a 2MB page

--*/
{
  UINTN                   Index;
  x64_MTRR_MEMORY_TYPE  MemoryType;
  x64_MTRR_MEMORY_TYPE  PreviousMemoryType;
  
  //
  // Address needs to be 2MB aligned
  //
  ASSERT ((BaseAddress & 0x1fffff) == 0);

  PreviousMemoryType = -1;
  for (Index = 0; Index < 512; Index++, BaseAddress += 0x1000) {
    MemoryType = EfiGetMTRRMemoryType (BaseAddress);
    if ((Index != 0) && (MemoryType != PreviousMemoryType)) {
      return TRUE;
    }

    PreviousMemoryType = MemoryType;
  }

  //
  // All the pages had the same type
  //
  return FALSE;
}




VOID
Convert2MBPageTo4KPages (  
  IN  x64_PAGE_TABLE_ENTRY_2M   *PageDirectoryEntry2MB, 
  IN  EFI_PHYSICAL_ADDRESS        PageAddress
  )
/*++

Routine Description:
 Convert a single 2MB page entry to 512 4K page entries. The attributes for 
 the 4K pages are read from the MTRR registers.

Arguments:
  PageDirectoryEntry2MB - Page directory entry for PageAddress
  PageAddress           - 2MB algined address of region to convert

Returns:
  None

--*/
{
  EFI_PHYSICAL_ADDRESS                          Address;
  x64_PAGE_DIRECTORY_ENTRY_4K                   *PageDirectoryEntry4k;
  x64_PAGE_TABLE_ENTRY_4K                       *PageTableEntry;
  UINTN                                         Index1;

  //
  // Allocate the page table entry for the 4K pages
  //
  PageTableEntry = (x64_PAGE_TABLE_ENTRY_4K *) AllocatePages (1);

  ASSERT (PageTableEntry != NULL);

  //
  // Convert PageDirectoryEntry2MB into a 4K Page Directory
  //
  PageDirectoryEntry4k = (x64_PAGE_DIRECTORY_ENTRY_4K *)PageDirectoryEntry2MB;
  PageDirectoryEntry2MB->Uint64 = (UINT64)PageTableEntry;
  PageDirectoryEntry2MB->Bits.ReadWrite = 1;
  PageDirectoryEntry2MB->Bits.Present = 1;
  
  //
  // Fill in the 4K page entries with the attributes from the MTRRs
  //
  for (Index1 = 0, Address = PageAddress; Index1 < 512; Index1++, PageTableEntry++, Address += 0x1000) {
    PageTableEntry->Uint64 = (UINT64)Address;
    PageTableEntry->Bits.ReadWrite = 1;
    PageTableEntry->Bits.Present = 1;
  }
}


EFI_PHYSICAL_ADDRESS
CreateIdentityMappingPageTables (
  IN UINT32                NumberOfProcessorPhysicalAddressBits
  )
/*++

Routine Description:

  Allocates and fills in the Page Directory and Page Table Entries to
  establish a 1:1 Virtual to Physical mapping for physical memory from
  0 to 4GB.  Memory above 4GB is not mapped.  The MTRRs are used to 
  determine the cachability of the physical memory regions

Arguments:

  NumberOfProcessorPhysicalAddressBits - Number of processor address bits to use.
                                         Limits the number of page table entries 
                                         to the physical address space.

Returns:
  EFI_OUT_OF_RESOURCES  There are not enough resources to allocate the Page Tables

  EFI_SUCCESS           The 1:1 Virtual to Physical identity mapping was created

--*/
{  
  EFI_PHYSICAL_ADDRESS                          PageAddress;
  UINTN                                         Index;
  UINTN                                         MaxBitsSupported;
  UINTN                                         Index1;
  UINTN                                         Index2;
  x64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K     *PageMapLevel4Entry;
  x64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K     *PageMap;
  x64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K     *PageDirectoryPointerEntry;
  x64_PAGE_TABLE_ENTRY_2M                       *PageDirectoryEntry2MB;


  //
  //  Page Table structure 4 level 4K, 3 level 2MB.
  //
  //                   PageMapLevel4Entry        : bits 47-39
  //                   PageDirectoryPointerEntry : bits 38-30
  //  Page Table 2MB : PageDirectoryEntry2M      : bits 29-21
  //  Page Table 4K  : PageDirectoryEntry4K      : bits 29 - 21
  //                   PageTableEntry            : bits 20 - 12
  //
  // Strategy is to map every thing in the processor address space using 
  //  2MB pages. If more  granularity is required the 2MB page will get 
  //  converted to set of 4K pages. 
  //

  //
  // By architecture only one PageMapLevel4 exists - so lets allocate storgage for it.
  //
  PageMap = PageMapLevel4Entry = (x64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K *) AllocatePages (1);
  ASSERT (PageMap != NULL);
  PageAddress = 0;

  //
  // The number of page-map Level-4 Offset entries is based on the number of 
  // physical address bits. Less than equal to 38 bits only takes one entry.
  // 512 entries represents 48 address bits. 
  //
  if (NumberOfProcessorPhysicalAddressBits <= 38) {
    MaxBitsSupported = 1;
  } else {
    MaxBitsSupported = mPowerOf2[NumberOfProcessorPhysicalAddressBits - 39];
  }

  for (Index = 0; Index < MaxBitsSupported; Index++, PageMapLevel4Entry++) {
    //
    // Each PML4 entry points to a page of Page Directory Pointer entires.
    //  So lets allocate space for them and fill them in in the Index1 loop.
    //  
    PageDirectoryPointerEntry = (x64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K *) AllocatePages (1);
    ASSERT (PageDirectoryPointerEntry != NULL);

    //
    // Make a PML4 Entry
    //
    PageMapLevel4Entry->Uint64 = (UINT64)(UINTN)PageDirectoryPointerEntry;
    PageMapLevel4Entry->Bits.ReadWrite = 1;
    PageMapLevel4Entry->Bits.Present = 1;

    for (Index1 = 0; Index1 < 512; Index1++, PageDirectoryPointerEntry++) {
      //
      // Each Directory Pointer entries points to a page of Page Directory entires.
      //  So lets allocate space for them and fill them in in the Index2 loop.
      //       
      PageDirectoryEntry2MB = (x64_PAGE_TABLE_ENTRY_2M *) AllocatePages (1);
      ASSERT (PageDirectoryEntry2MB != NULL);

      //
      // Fill in a Page Directory Pointer Entries
      //
      PageDirectoryPointerEntry->Uint64 = (UINT64)(UINTN)PageDirectoryEntry2MB;
      PageDirectoryPointerEntry->Bits.ReadWrite = 1;
      PageDirectoryPointerEntry->Bits.Present = 1;

      for (Index2 = 0; Index2 < 512; Index2++, PageDirectoryEntry2MB++, PageAddress += 0x200000) {
        //
        // Fill in the Page Directory entries
        //
        PageDirectoryEntry2MB->Uint64 = (UINT64)PageAddress;
        PageDirectoryEntry2MB->Bits.ReadWrite = 1;
        PageDirectoryEntry2MB->Bits.Present = 1;
        PageDirectoryEntry2MB->Bits.MustBe1 = 1;

        if (CanNotUse2MBPage (PageAddress)) {
          //
          // Check to see if all 2MB has the same mapping. If not convert
          //  to 4K pages by adding the 4th level of page table entries
          //
          Convert2MBPageTo4KPages (PageDirectoryEntry2MB, PageAddress);
        }
      }
    }
  }

  //
  // For the PML4 entries we are not using fill in a null entry.
  //  for now we just copy the first entry.
  //
  for (; Index < 512; Index++, PageMapLevel4Entry++) {
  //    EfiCopyMem (PageMapLevel4Entry, PageMap, sizeof (x64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K));
     CopyMem (PageMapLevel4Entry,
              PageMap,
              sizeof (x64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K)
              );
  }

  return (EFI_PHYSICAL_ADDRESS)PageMap;
}


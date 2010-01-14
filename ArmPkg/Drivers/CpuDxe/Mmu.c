/*++

Copyright (c) 2009, Hewlett-Packard Company  
Portions copyright (c) 2010, Apple Inc.  All rights reserved.

All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


--*/

#include "CpuDxe.h"

//
// For debug switch me back to to EFI_D_PAGE when done
//
#define L_EFI_D_PAGE EFI_D_ERROR

//
// Translation/page table definitions
//

// First Level Descriptors
typedef UINT32    ARM_FIRST_LEVEL_DESCRIPTOR;

// memory space covered by a first level descriptor
#define ARM_PAGE_DESC_ENTRY_MVA_SIZE  0x00100000  // 1MB

// number of first level descriptors to cover entire 32-bit memory space
#define FIRST_LEVEL_ENTRY_COUNT       (0xFFFFFFFF / ARM_PAGE_DESC_ENTRY_MVA_SIZE + 1)


// page table 1st level descriptor entries
#define ARM_PAGE_DESC_BASE_MASK       0xFFFFFC00
#define ARM_PAGE_DESC_BASE_SHFIT      10
#define ARM_PAGE_DESC_DOMAIN_MASK     0x000001E0
#define ARM_PAGE_DESC_DOMAIN_SHIFT    5
#define ARM_PAGE_DESC_NS              0x00000008

#define ARM_FIRST_LEVEL_DESC_ALIGN    0x00004000  // 16KB

// section 1st level desriptor entries
#define ARM_SECTION_BASE_MASK         0xFFF00000
#define ARM_SECTION_BASE_SHIFT        20
#define ARM_SECTION_NS                0x00080000
#define ARM_SECTION_nG                0x00020000
#define ARM_SECTION_S                 0x00010000
#define ARM_SECTION_AP2               0x00008000
#define ARM_SECTION_TEX_MASK          0x00007000
#define ARM_SECTION_TEX_SHIFT         12
#define ARM_SECTION_AP10_MASK         0x00000C00
#define ARM_SECTION_AP10_SHIFT        10
#define ARM_SECTION_DOMAIN_MASK       0x000001E0
#define ARM_SECTION_DOMAIN_SHIFT      5
#define ARM_SECTION_XN                0x00000010
#define ARM_SECTION_C                 0x00000008
#define ARM_SECTION_B                 0x00000004

// section level AP[2:0] definitions
#define ARM_SECTION_AP_NO_ACCESS      0   // AP[2:0] = 0
#define ARM_SECTION_AP_READ_WRITE     ARM_SECTION_AP10_MASK // AP[2:0] = 011
#define ARM_SECTION_AP_READ_ONLY      (ARM_SECTION_AP2 | ARM_SECTION_AP10_MASK) // AP[2:0] = 111

// common 1st level descriptor fields
#define ARM_DESC_TYPE_MASK            0x00000003

// descriptor type values
#define ARM_DESC_TYPE_FAULT           0x0
#define ARM_DESC_TYPE_PAGE_TABLE      0x1
#define ARM_DESC_TYPE_SECTION         0x2


// Second Level Descriptors
typedef UINT32    ARM_PAGE_TABLE_ENTRY;

// small page 2nd level descriptor entries
#define ARM_SMALL_PAGE_BASE_MASK      0xFFFFF000
#define ARM_SMALL_PAGE_INDEX_MASK     0x000FF000
#define ARM_SMALL_PAGE_BASE_SHIFT     12
#define ARM_SMALL_PAGE_TEX_MASK       0x000001C0
#define ARM_SMALL_PAGE_TEX_SHIFT      6
#define ARM_SMALL_PAGE_XN             0x00000001

// large page 2nd level descriptor entries
#define ARM_LARGE_PAGE_BASE_MASK      0xFFFF0000
#define ARM_LARGE_PAGE_BASE_SHIFT     16
#define ARM_LARGE_PAGE_TEX_MASK       0x00007000
#define ARM_LARGE_PAGE_TEX_SHIFT      12
#define ARM_LARGE_PAGE_XN             0x00008000

// common 2nd level desriptor fields
#define ARM_PAGE_nG                   0x00000800
#define ARM_PAGE_S                    0x00000400
#define ARM_PAGE_AP2                  0x00000200
#define ARM_PAGE_AP10_MASK            0x00000030
#define ARM_PAGE_AP10_SHIFT           4
#define ARM_PAGE_C                    0x00000008
#define ARM_PAGE_B                    0x00000004
#define ARM_PAGE_DESC_TYPE_MASK       0x00000003

// descriptor type values
#define ARM_PAGE_TYPE_FAULT           0x0
#define ARM_PAGE_TYPE_LARGE           0x1
#define ARM_PAGE_TYPE_SMALL           0x2
#define ARM_PAGE_TYPE_SMALL_XN        0x3

#define SMALL_PAGE_TABLE_ENTRY_COUNT  (ARM_PAGE_DESC_ENTRY_MVA_SIZE / EFI_PAGE_SIZE)


// Translation Table Base 0 fields
#define ARM_TTBR0_BASE_MASK           0xFFFFC000
#define ARM_TTBR0_BASE_SHIFT          14
#define ARM_TTRB0_NOS                 0x00000020

// define the combination of interesting attributes: cacheability and access permissions
#define ARM_SECTION_CACHEABILITY_MASK   ( ARM_SECTION_TEX_MASK | ARM_SECTION_C | ARM_SECTION_B )
#define ARM_SECTION_RW_PERMISSIONS_MASK ( ARM_SECTION_AP2 | ARM_SECTION_AP10_MASK )
#define ARM_DESCRIPTOR_ATTRIBUTES       ( ARM_SECTION_CACHEABILITY_MASK | ARM_SECTION_RW_PERMISSIONS_MASK | ARM_SECTION_XN )

// cacheability values for section entries
#define ARM_SECTION_STRONGLY_ORDERED    0
#define ARM_SECTION_SHAREABLE_DEVICE    ARM_SECTION_B
#define ARM_SECTION_WRITE_THROUGH       ARM_SECTION_C
#define ARM_SECTION_WRITE_BACK_NWA      ( ARM_SECTION_C| ARM_SECTION_B )
#define ARM_SECTION_NORMAL_UNCACHEABLE  ( 0x1 << ARM_SECTION_TEX_SHIFT )
#define ARM_SECTION_WRITE_BACK          ( ( 0x1 << ARM_SECTION_TEX_SHIFT ) | ARM_SECTION_C | ARM_SECTION_B )
#define ARM_SECTION_NONSHAREABLE_DEVICE ( 0x2 << ARM_SECTION_TEX_SHIFT )

// permissions values for section entries
#define ARM_SECTION_NO_ACCESS           0
#define ARM_SECTION_PRIV_ACCESS_ONLY    ( 0x1 << ARM_SECTION_AP10_SHIFT)
#define ARM_SECTION_USER_READ_ONLY      ( 0x2 << ARM_SECTION_AP10_SHIFT)
#define ARM_SECTION_FULL_ACCESS         ( 0x3 << ARM_SECTION_AP10_SHIFT)
#define ARM_SECTION_PRIV_READ_ONLY      ( ARM_SECTION_AP2 | (0x1 << ARM_SECTION_AP10_SHIFT) )
#define ARM_SECTION_READ_ONLY_DEP       ( ARM_SECTION_AP2 | (0x2 << ARM_SECTION_AP10_SHIFT) )
#define ARM_SECTION_READ_ONLY           ( ARM_SECTION_AP2 | (0x3 << ARM_SECTION_AP10_SHIFT) )



EFI_STATUS 
SectionToGcdAttributes (
  IN  UINT32  SectionAttributes,
  OUT UINT64  *GcdAttributes
  )
{
  *GcdAttributes = 0;

  // determine cacheability attributes
  switch(SectionAttributes & ARM_SECTION_CACHEABILITY_MASK) {
    case ARM_SECTION_STRONGLY_ORDERED:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    case ARM_SECTION_SHAREABLE_DEVICE:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    case ARM_SECTION_WRITE_THROUGH:
      *GcdAttributes |= EFI_MEMORY_WT;
      break;
    case ARM_SECTION_WRITE_BACK_NWA:
      *GcdAttributes |= EFI_MEMORY_WB;
      break;
    case ARM_SECTION_NORMAL_UNCACHEABLE:
      *GcdAttributes |= EFI_MEMORY_WC;
      break;
    case ARM_SECTION_WRITE_BACK:
      *GcdAttributes |= EFI_MEMORY_WB;
      break;
    case ARM_SECTION_NONSHAREABLE_DEVICE:
      *GcdAttributes |= EFI_MEMORY_UC;
      break;
    default:
      return EFI_UNSUPPORTED;
  }
    
  // determine protection attributes
  switch(SectionAttributes & ARM_SECTION_RW_PERMISSIONS_MASK) {
    case ARM_SECTION_NO_ACCESS: // no read, no write
      //*GcdAttributes |= EFI_MEMORY_WP | EFI_MEMORY_RP;
      break;

    case ARM_SECTION_PRIV_ACCESS_ONLY:
    case ARM_SECTION_FULL_ACCESS:
      // normal read/write access, do not add additional attributes
      break;

    // read only cases map to write-protect
    case ARM_SECTION_PRIV_READ_ONLY:
    case ARM_SECTION_READ_ONLY_DEP:
    case ARM_SECTION_READ_ONLY:
      *GcdAttributes |= EFI_MEMORY_WP;
      break;

    default:
      return EFI_UNSUPPORTED;
  }

  // now process eXectue Never attribute
  if ((SectionAttributes & ARM_SECTION_XN) != 0 ) {
    *GcdAttributes |= EFI_MEMORY_XP;
  }

  return EFI_SUCCESS;
}

/**
  Searches memory descriptors covered by given memory range.

  This function searches into the Gcd Memory Space for descriptors
  (from StartIndex to EndIndex) that contains the memory range
  specified by BaseAddress and Length.

  @param  MemorySpaceMap       Gcd Memory Space Map as array.
  @param  NumberOfDescriptors  Number of descriptors in map.
  @param  BaseAddress          BaseAddress for the requested range.
  @param  Length               Length for the requested range.
  @param  StartIndex           Start index into the Gcd Memory Space Map.
  @param  EndIndex             End index into the Gcd Memory Space Map.

  @retval EFI_SUCCESS          Search successfully.
  @retval EFI_NOT_FOUND        The requested descriptors does not exist.

**/
EFI_STATUS
SearchGcdMemorySpaces (
  IN EFI_GCD_MEMORY_SPACE_DESCRIPTOR     *MemorySpaceMap,
  IN UINTN                               NumberOfDescriptors,
  IN EFI_PHYSICAL_ADDRESS                BaseAddress,
  IN UINT64                              Length,
  OUT UINTN                              *StartIndex,
  OUT UINTN                              *EndIndex
  )
{
  UINTN           Index;

  *StartIndex = 0;
  *EndIndex   = 0;
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if (BaseAddress >= MemorySpaceMap[Index].BaseAddress &&
        BaseAddress < MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length) {
      *StartIndex = Index;
    }
    if (BaseAddress + Length - 1 >= MemorySpaceMap[Index].BaseAddress &&
        BaseAddress + Length - 1 < MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length) {
      *EndIndex = Index;
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}


/**
  Sets the attributes for a specified range in Gcd Memory Space Map.

  This function sets the attributes for a specified range in
  Gcd Memory Space Map.

  @param  MemorySpaceMap       Gcd Memory Space Map as array
  @param  NumberOfDescriptors  Number of descriptors in map
  @param  BaseAddress          BaseAddress for the range
  @param  Length               Length for the range
  @param  Attributes           Attributes to set

  @retval EFI_SUCCESS          Memory attributes set successfully
  @retval EFI_NOT_FOUND        The specified range does not exist in Gcd Memory Space

**/
EFI_STATUS
SetGcdMemorySpaceAttributes (
  IN EFI_GCD_MEMORY_SPACE_DESCRIPTOR     *MemorySpaceMap,
  IN UINTN                               NumberOfDescriptors,
  IN EFI_PHYSICAL_ADDRESS                BaseAddress,
  IN UINT64                              Length,
  IN UINT64                              Attributes
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  UINTN                 StartIndex;
  UINTN                 EndIndex;
  EFI_PHYSICAL_ADDRESS  RegionStart;
  UINT64                RegionLength;

  //
  // Get all memory descriptors covered by the memory range
  //
  Status = SearchGcdMemorySpaces (
             MemorySpaceMap,
             NumberOfDescriptors,
             BaseAddress,
             Length,
             &StartIndex,
             &EndIndex
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Go through all related descriptors and set attributes accordingly
  //
  for (Index = StartIndex; Index <= EndIndex; Index++) {
    if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
      continue;
    }
    //
    // Calculate the start and end address of the overlapping range
    //
    if (BaseAddress >= MemorySpaceMap[Index].BaseAddress) {
      RegionStart = BaseAddress;
    } else {
      RegionStart = MemorySpaceMap[Index].BaseAddress;
    }
    if (BaseAddress + Length - 1 < MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length) {
      RegionLength = BaseAddress + Length - RegionStart;
    } else {
      RegionLength = MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length - RegionStart;
    }
    //
    // Set memory attributes according to MTRR attribute and the original attribute of descriptor
    //
    gDS->SetMemorySpaceAttributes (
           RegionStart,
           RegionLength,
           (MemorySpaceMap[Index].Attributes & ~EFI_MEMORY_CACHETYPE_MASK) | (MemorySpaceMap[Index].Capabilities & Attributes)
           );
  }

  return EFI_SUCCESS;
}


EFI_STATUS
SyncCacheConfig (
  IN  EFI_CPU_ARCH_PROTOCOL *CpuProtocol
  )
{
  EFI_STATUS                          Status;
  UINT32                              i;
  UINT32                              Descriptor;
  UINT32                              SectionAttributes;
  EFI_PHYSICAL_ADDRESS                NextRegionBase;
  UINT64                              NextRegionLength;
  UINT64                              GcdAttributes;
  UINT32                              NextRegionAttributes = 0;
  volatile ARM_FIRST_LEVEL_DESCRIPTOR   *FirstLevelTable;
  UINTN                               NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR     *MemorySpaceMap;


  DEBUG ((L_EFI_D_PAGE, "SyncCacheConfig()\n"));

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
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)(ArmGetTranslationTableBaseAddress ());


  // iterate through each 1MB descriptor
  NextRegionBase = NextRegionLength = 0;
  for (i=0; i< FIRST_LEVEL_ENTRY_COUNT; i++) {

    // obtain existing descriptor and make sure it contains a valid Base Address even if it is a fault section
    Descriptor = FirstLevelTable[i] | (ARM_SECTION_BASE_MASK & (i << ARM_SECTION_BASE_SHIFT));

    // extract attributes (cacheability and permissions)
    SectionAttributes = Descriptor & 0xDEC;

    // do we already have an existing region (or are we about to finish)?
    // Skip the first entry, and make sure we close on the last entry
    if ( (NextRegionLength > 0) || (i == (FIRST_LEVEL_ENTRY_COUNT-1)) ) {
      // attributes are changing, update attributes in GCD
      if (SectionAttributes != NextRegionAttributes) {
        
        // convert section entry attributes to GCD bitmask
        Status = SectionToGcdAttributes (NextRegionAttributes, &GcdAttributes);
        ASSERT_EFI_ERROR (Status);

        // update GCD with these changes (this will recurse into our own CpuSetMemoryAttributes below which is OK)
        SetGcdMemorySpaceAttributes (MemorySpaceMap, NumberOfDescriptors, NextRegionBase, NextRegionLength, GcdAttributes);


        // start on a new region
        NextRegionLength = 0;
        NextRegionBase = Descriptor & ARM_SECTION_BASE_MASK;
      }
    }

    // starting a new region?
    if (NextRegionLength == 0) {
      NextRegionAttributes = SectionAttributes;
    }

    NextRegionLength += ARM_PAGE_DESC_ENTRY_MVA_SIZE;

  } // section entry loop

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

  volatile ARM_FIRST_LEVEL_DESCRIPTOR   *FirstLevelTable;
  volatile ARM_PAGE_TABLE_ENTRY         *PageTable;

  // EntryMask: bitmask of values to change (1 = change this value, 0 = leave alone)
  // EntryValue: values at bit positions specified by EntryMask

  // Although the PI spec is unclear on this the GCD guarantees that only
  // one Attribute bit is set at a time, so we can safely use a switch statement
  switch (Attributes) {
    case EFI_MEMORY_UC:
      // modify cacheability attributes
      EntryMask = ARM_SMALL_PAGE_TEX_MASK | ARM_PAGE_C | ARM_PAGE_B;
      // map to strongly ordered
      EntryValue = 0; // TEX[2:0] = 0, C=0, B=0
      break;

    case EFI_MEMORY_WC:
      // modify cacheability attributes
      EntryMask = ARM_SMALL_PAGE_TEX_MASK | ARM_PAGE_C | ARM_PAGE_B;
      // map to normal non-cachable
      EntryValue = (0x1 << ARM_SMALL_PAGE_TEX_SHIFT); // TEX [2:0]= 001 = 0x2, B=0, C=0
      break;

    case EFI_MEMORY_WT:
      // modify cacheability attributes
      EntryMask = ARM_SMALL_PAGE_TEX_MASK | ARM_PAGE_C | ARM_PAGE_B;
      // write through with no-allocate
      EntryValue = ARM_PAGE_C; // TEX [2:0] = 0, C=1, B=0
      break;

    case EFI_MEMORY_WB:
      // modify cacheability attributes
      EntryMask = ARM_SMALL_PAGE_TEX_MASK | ARM_PAGE_C | ARM_PAGE_B;
      // write back (with allocate)
      EntryValue = (0x1 << ARM_SMALL_PAGE_TEX_SHIFT) | ARM_PAGE_C | ARM_PAGE_B; // TEX [2:0] = 001, C=1, B=1
      break;

    case EFI_MEMORY_WP:
    case EFI_MEMORY_XP:
    case EFI_MEMORY_UCE:
      // cannot be implemented UEFI definition unclear for ARM
      // Cause a page fault if these ranges are accessed.
      EntryMask   = 0x3;
      EntryValue = 0;
      DEBUG ((L_EFI_D_PAGE, "SetMemoryAttributes(): setting page %lx with unsupported attribute %x will page fault on access\n", BaseAddress, Attributes));
      break;

    default:
      return EFI_UNSUPPORTED;
  }

  // obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTranslationTableBaseAddress ();

  // calculate number of 4KB page table entries to change
  NumPageEntries = Length/EFI_PAGE_SIZE;
  
  // iterate for the number of 4KB pages to change
  Offset = 0;
  for(p=0; p<NumPageEntries; p++) {
    // calculate index into first level translation table for page table value
    
    FirstLevelIdx = ((BaseAddress + Offset) & ARM_SECTION_BASE_MASK) >> ARM_SECTION_BASE_SHIFT;
    ASSERT (FirstLevelIdx < FIRST_LEVEL_ENTRY_COUNT);

    // read the descriptor from the first level page table
    Descriptor = FirstLevelTable[FirstLevelIdx];

    // does this descriptor need to be converted from section entry to 4K pages?
    if ((Descriptor & ARM_DESC_TYPE_MASK) != ARM_DESC_TYPE_PAGE_TABLE ) {
      Status = ConvertSectionToPages (FirstLevelIdx << ARM_SECTION_BASE_SHIFT);
      if (EFI_ERROR(Status)) {
        // exit for loop
        break; 
      } 
      
      // re-read descriptor
      Descriptor = FirstLevelTable[FirstLevelIdx];
    }

    // obtain page table base address
    PageTable = (ARM_PAGE_TABLE_ENTRY *)(Descriptor & ARM_SMALL_PAGE_BASE_MASK);

    // calculate index into the page table
    PageTableIndex = ((BaseAddress + Offset) & ARM_SMALL_PAGE_INDEX_MASK) >> ARM_SMALL_PAGE_BASE_SHIFT;
    ASSERT (PageTableIndex < SMALL_PAGE_TABLE_ENTRY_COUNT);

    // get the entry
    PageTableEntry = PageTable[PageTableIndex];

    // mask off appropriate fields
    PageTableEntry &= ~EntryMask;

    // mask in new attributes and/or permissions
    PageTableEntry |= EntryValue;

    if (VirtualMask != 0) {
      // Make this virtual address point at a physical page
      PageTableEntry &= ~VirtualMask;
    }
    
    // update the entry
    PageTable[PageTableIndex] = PageTableEntry; 
   

    Status = EFI_SUCCESS;
    Offset += EFI_PAGE_SIZE;
    
  } // end first level translation table loop

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
  UINT32        Descriptor;

  volatile ARM_FIRST_LEVEL_DESCRIPTOR   *FirstLevelTable;

  // EntryMask: bitmask of values to change (1 = change this value, 0 = leave alone)
  // EntryValue: values at bit positions specified by EntryMask

  // Make sure we handle a section range that is unmapped 
  EntryMask = ARM_DESC_TYPE_MASK;
  EntryValue = ARM_DESC_TYPE_SECTION;

  // Although the PI spec is unclear on this the GCD guarantees that only
  // one Attribute bit is set at a time, so we can safely use a switch statement
  switch(Attributes) {
    case EFI_MEMORY_UC:
      // modify cacheability attributes
      EntryMask |= ARM_SECTION_TEX_MASK | ARM_SECTION_C | ARM_SECTION_B;
      // map to strongly ordered
      EntryValue |= 0; // TEX[2:0] = 0, C=0, B=0
      break;

    case EFI_MEMORY_WC:
      // modify cacheability attributes
      EntryMask |= ARM_SECTION_TEX_MASK | ARM_SECTION_C | ARM_SECTION_B;
      // map to normal non-cachable
      EntryValue |= (0x1 << ARM_SECTION_TEX_SHIFT); // TEX [2:0]= 001 = 0x2, B=0, C=0
      break;

    case EFI_MEMORY_WT:
      // modify cacheability attributes
      EntryMask |= ARM_SECTION_TEX_MASK | ARM_SECTION_C | ARM_SECTION_B;
      // write through with no-allocate
      EntryValue |= ARM_SECTION_C; // TEX [2:0] = 0, C=1, B=0
      break;

    case EFI_MEMORY_WB:
      // modify cacheability attributes
      EntryMask |= ARM_SECTION_TEX_MASK | ARM_SECTION_C | ARM_SECTION_B;
      // write back (with allocate)
      EntryValue |= (0x1 << ARM_SECTION_TEX_SHIFT) | ARM_SECTION_C | ARM_SECTION_B; // TEX [2:0] = 001, C=1, B=1
      break;

    case EFI_MEMORY_WP:
    case EFI_MEMORY_XP:
    case EFI_MEMORY_RP:
    case EFI_MEMORY_UCE:
      // cannot be implemented UEFI definition unclear for ARM
      // Cause a page fault if these ranges are accessed.
      EntryValue = ARM_DESC_TYPE_FAULT;
      DEBUG ((L_EFI_D_PAGE, "SetMemoryAttributes(): setting section %lx with unsupported attribute %x will page fault on access\n", BaseAddress, Attributes));
      break;


    default:
      return EFI_UNSUPPORTED;
  }

  // obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTranslationTableBaseAddress ();

  // calculate index into first level translation table for start of modification
  FirstLevelIdx = (BaseAddress & ARM_SECTION_BASE_MASK) >> ARM_SECTION_BASE_SHIFT;
  ASSERT (FirstLevelIdx < FIRST_LEVEL_ENTRY_COUNT);

  // calculate number of 1MB first level entries this applies to
  NumSections = Length / ARM_PAGE_DESC_ENTRY_MVA_SIZE;
  
  // iterate through each descriptor
  for(i=0; i<NumSections; i++) {
    Descriptor = FirstLevelTable[FirstLevelIdx + i];

    // has this descriptor already been coverted to pages?
    if ((Descriptor & ARM_DESC_TYPE_MASK) != ARM_DESC_TYPE_PAGE_TABLE ) {
      // forward this 1MB range to page table function instead
      Status = UpdatePageEntries ((FirstLevelIdx + i) << ARM_SECTION_BASE_SHIFT, ARM_PAGE_DESC_ENTRY_MVA_SIZE, Attributes, VirtualMask);
    } else {
      // still a section entry
      
      // mask off appropriate fields
      Descriptor &= ~EntryMask;

      // mask in new attributes and/or permissions
      Descriptor |= EntryValue;
      if (VirtualMask != 0) {
        Descriptor &= ~VirtualMask;
      }

      FirstLevelTable[FirstLevelIdx + i] = Descriptor;

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
  UINT32                  i;

  volatile ARM_FIRST_LEVEL_DESCRIPTOR   *FirstLevelTable;
  volatile ARM_PAGE_TABLE_ENTRY         *PageTable;

  DEBUG ((L_EFI_D_PAGE, "Converting section at 0x%x to pages\n", (UINTN)BaseAddress));

  // obtain page table base
  FirstLevelTable = (ARM_FIRST_LEVEL_DESCRIPTOR *)ArmGetTranslationTableBaseAddress ();

  // calculate index into first level translation table for start of modification
  FirstLevelIdx = (BaseAddress & ARM_SECTION_BASE_MASK) >> ARM_SECTION_BASE_SHIFT;
  ASSERT (FirstLevelIdx < FIRST_LEVEL_ENTRY_COUNT);

  // get section attributes and convert to page attributes
  SectionDescriptor = FirstLevelTable[FirstLevelIdx];
  PageDescriptor = ARM_PAGE_TYPE_SMALL;
  PageDescriptor |= ((SectionDescriptor & ARM_SECTION_TEX_MASK) >> ARM_SECTION_TEX_SHIFT) << ARM_SMALL_PAGE_TEX_SHIFT;
  if ((SectionDescriptor & ARM_SECTION_B) != 0) {
    PageDescriptor |= ARM_PAGE_B;
  }
  if ((SectionDescriptor & ARM_SECTION_C) != 0) {
    PageDescriptor |= ARM_PAGE_C;
  }
  PageDescriptor |= ((SectionDescriptor & ARM_SECTION_AP10_MASK) >> ARM_SECTION_AP10_SHIFT) << ARM_PAGE_AP10_SHIFT;
  if ((SectionDescriptor & ARM_SECTION_AP2) != 0) {
    PageDescriptor |= ARM_PAGE_AP2;
  }
  if ((SectionDescriptor & ARM_SECTION_XN) != 0) {
    PageDescriptor |= ARM_PAGE_TYPE_SMALL_XN;
  }
  if ((SectionDescriptor & ARM_SECTION_nG) != 0) {
    PageDescriptor |= ARM_PAGE_nG;
  }
  if ((SectionDescriptor & ARM_SECTION_S) != 0) {
    PageDescriptor |= ARM_PAGE_S;
  }

  // allocate a page table for the 4KB entries (we use up a full page even though we only need 1KB)
  Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesData, 1, &PageTableAddr);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  PageTable = (volatile ARM_PAGE_TABLE_ENTRY *)(UINTN)PageTableAddr;

  // write the page table entries out
  for (i=0; i<(ARM_PAGE_DESC_ENTRY_MVA_SIZE/EFI_PAGE_SIZE); i++) {
    PageTable[i] = ((BaseAddress + (i << 12)) & ARM_SMALL_PAGE_BASE_MASK) | PageDescriptor;
  }

  // flush d-cache so descriptors make it back to uncached memory for subsequent table walks
  // TODO: change to use only PageTable base and length
  // ArmInvalidateDataCache ();
DEBUG ((EFI_D_ERROR, "InvalidateDataCacheRange (%x, %x)\n", (UINTN)PageTableAddr, EFI_PAGE_SIZE));

  InvalidateDataCacheRange ((VOID *)(UINTN)PageTableAddr, EFI_PAGE_SIZE);

  // formulate page table entry, Domain=0, NS=0
  PageTableDescriptor = (((UINTN)PageTableAddr) & ARM_PAGE_DESC_BASE_MASK) | ARM_DESC_TYPE_PAGE_TABLE;

  // write the page table entry out, repalcing section entry
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
    // is the base and length a multiple of 1 MB?
    DEBUG ((L_EFI_D_PAGE, "SetMemoryAttributes(): MMU section 0x%x length 0x%x to %lx\n", (UINTN)BaseAddress, (UINTN)Length, Attributes));
    Status = UpdateSectionEntries (BaseAddress, Length, Attributes, VirtualMask);
  } else {
    // base and/or length is not a multiple of 1 MB
    DEBUG ((L_EFI_D_PAGE, "SetMemoryAttributes(): MMU page 0x%x length 0x%x to %lx\n", (UINTN)BaseAddress, (UINTN)Length, Attributes));
    Status = UpdatePageEntries (BaseAddress, Length, Attributes, VirtualMask);
  }

  // flush d-cache so descriptors make it back to uncached memory for subsequent table walks
  // flush and invalidate pages
  ArmCleanInvalidateDataCache ();
  
  ArmInvalidateInstructionCache ();

  // invalidate all TLB entries so changes are synced
  ArmInvalidateTlb (); 

  return Status;
}


/**
  This function modifies the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  BaseAddress      The physical address that is the start address of a memory region.
  @param  Length           The size in bytes of the memory region.
  @param  Attributes       The bit mask of attributes to set for the memory region.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not support for the memory resource
                                range specified by BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
CpuSetMemoryAttributes (
  IN EFI_CPU_ARCH_PROTOCOL     *This,
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes
  )
{
  DEBUG ((L_EFI_D_PAGE, "SetMemoryAttributes(%lx, %lx, %lx)\n", BaseAddress, Length, Attributes));
  if ( ((BaseAddress & (EFI_PAGE_SIZE-1)) != 0) || ((Length & (EFI_PAGE_SIZE-1)) != 0)){
    // minimum granularity is EFI_PAGE_SIZE (4KB on ARM)
    DEBUG ((L_EFI_D_PAGE, "SetMemoryAttributes(%lx, %lx, %lx): minimum ganularity is EFI_PAGE_SIZE\n", BaseAddress, Length, Attributes));
    return EFI_UNSUPPORTED;
  }
  
  return SetMemoryAttributes (BaseAddress, Length, Attributes, 0);
}



//
// Add a new protocol to support 
//

EFI_STATUS
EFIAPI
CpuConvertPagesToUncachedVirtualAddress (
  IN  VIRTUAL_UNCACHED_PAGES_PROTOCOL   *This,
  IN  EFI_PHYSICAL_ADDRESS              Address,
  IN  UINTN                             Length,
  IN  EFI_PHYSICAL_ADDRESS              VirtualMask,
  OUT UINT64                            *Attributes     OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR GcdDescriptor;
  
  
  if (Attributes != NULL) {
    Status = gDS->GetMemorySpaceDescriptor (Address, &GcdDescriptor);
    if (!EFI_ERROR (Status)) {
      *Attributes = GcdDescriptor.Attributes;
    }
  }
ASSERT (FALSE);  
  //
  // Make this address range page fault if accessed. If it is a DMA buffer than this would 
  // be the PCI address. Code should always use the CPU address, and we will or in VirtualMask
  // to that address. 
  //
  Status = SetMemoryAttributes (Address, Length, EFI_MEMORY_WP, 0);
  if (!EFI_ERROR (Status)) {
    Status = SetMemoryAttributes (Address | VirtualMask, Length, EFI_MEMORY_UC, VirtualMask);
  }

  return Status;
}


EFI_STATUS
EFIAPI
CpuReconvertPagesPages (
  IN  VIRTUAL_UNCACHED_PAGES_PROTOCOL   *This,
  IN  EFI_PHYSICAL_ADDRESS              Address,
  IN  UINTN                             Length,
  IN  EFI_PHYSICAL_ADDRESS              VirtualMask,
  IN  UINT64                            Attributes
  )
{
  EFI_STATUS      Status;
  
  //
  // Unmap the alaised Address
  //
  Status = SetMemoryAttributes (Address | VirtualMask, Length, EFI_MEMORY_WP, 0);
  if (!EFI_ERROR (Status)) {
    //
    // Restore atttributes
    //
    Status = SetMemoryAttributes (Address, Length, Attributes, 0);
  }
  
  return Status;
}


VIRTUAL_UNCACHED_PAGES_PROTOCOL  gVirtualUncachedPages = {
  CpuConvertPagesToUncachedVirtualAddress,
  CpuReconvertPagesPages
};





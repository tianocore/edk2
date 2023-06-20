/** @file
  SMM IPL that load the SMM Core into SMRAM

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <PiSmm.h>
#include <StandaloneMm.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <StandaloneMmIplPei.h>

#pragma pack(1)

//
// Page-Map Level-4 Offset (PML4) and
// Page-Directory-Pointer Offset (PDPE) entries 4K & 2MB
//

typedef union {
  struct {
    UINT64    Present              : 1;   // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;   // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;   // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;   // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;   // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;   // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Reserved             : 1;   // Reserved
    UINT64    MustBeZero           : 2;   // Must Be Zero
    UINT64    Available            : 3;   // Available for use by system software
    UINT64    PageTableBaseAddress : 40;  // Page Table Base Address
    UINT64    AvailableHigh        : 11;  // Available for use by system software
    UINT64    Nx                   : 1;   // No Execute bit
  } Bits;
  UINT64    Uint64;
} PAGE_MAP_AND_DIRECTORY_POINTER;

//
// Page Table Entry 2MB
//
typedef union {
  struct {
    UINT64    Present              : 1;   // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;   // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;   // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;   // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;   // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;   // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Dirty                : 1;   // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64    MustBe1              : 1;   // Must be 1
    UINT64    Global               : 1;   // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64    Available            : 3;   // Available for use by system software
    UINT64    Pat                  : 1;   //
    UINT64    MustBeZero           : 8;   // Must be zero
    UINT64    PageTableBaseAddress : 31;  // Page Table Base Address
    UINT64    AvailableHigh        : 11;  // Available for use by system software
    UINT64    Nx                   : 1;   // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_ENTRY;

//
// Page Table Entry 1GB
//
typedef union {
  struct {
    UINT64    Present              : 1;   // 0 = Not present in memory, 1 = Present in memory
    UINT64    ReadWrite            : 1;   // 0 = Read-Only, 1= Read/Write
    UINT64    UserSupervisor       : 1;   // 0 = Supervisor, 1=User
    UINT64    WriteThrough         : 1;   // 0 = Write-Back caching, 1=Write-Through caching
    UINT64    CacheDisabled        : 1;   // 0 = Cached, 1=Non-Cached
    UINT64    Accessed             : 1;   // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64    Dirty                : 1;   // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64    MustBe1              : 1;   // Must be 1
    UINT64    Global               : 1;   // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64    Available            : 3;   // Available for use by system software
    UINT64    Pat                  : 1;   //
    UINT64    MustBeZero           : 17;  // Must be zero;
    UINT64    PageTableBaseAddress : 22;  // Page Table Base Address
    UINT64    AvailableHigh        : 11;  // Available for use by system software
    UINT64    Nx                   : 1;   // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_1G_ENTRY;

#pragma pack()

//
// Global Descriptor Table (GDT)
//
GLOBAL_REMOVE_IF_UNREFERENCED IA32_SEGMENT_DESCRIPTOR  mGdtEntries[] = {
  /* selector { Global Segment Descriptor                              } */
  /* 0x00 */ {
    { 0,      0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0 }
  },                                                                      // null descriptor
  /* 0x08 */ {
    { 0xffff, 0, 0, 0x3, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // linear data segment descriptor
  /* 0x10 */ {
    { 0xffff, 0, 0, 0xf, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // linear code segment descriptor
  /* 0x18 */ {
    { 0xffff, 0, 0, 0x3, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // system data segment descriptor
  /* 0x20 */ {
    { 0xffff, 0, 0, 0xb, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // system code segment descriptor
  /* 0x28 */ {
    { 0,      0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0 }
  },                                                                      // spare segment descriptor
  /* 0x30 */ {
    { 0xffff, 0, 0, 0x3, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // system data segment descriptor
  /* 0x38 */ {
    { 0xffff, 0, 0, 0xb, 1, 0, 1, 0xf, 0, 1, 0, 1, 0 }
  },                                                                      // system code segment descriptor
  /* 0x40 */ {
    { 0,      0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0 }
  },                                                                      // spare segment descriptor
};

//
// IA32 Gdt register
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST IA32_DESCRIPTOR  mGdt = {
  sizeof (mGdtEntries) - 1,
  (UINTN)mGdtEntries
};

/**
  Calculate the total size of page table.

  @return The size of page table.

**/
UINTN
CalculatePageTableSize (
  VOID
  )
{
  UINT32   RegEax;
  UINT32   RegEdx;
  UINTN    TotalPagesNum;
  UINT8    PhysicalAddressBits;
  VOID     *Hob;
  UINT32   NumberOfPml4EntriesNeeded;
  UINT32   NumberOfPdpEntriesNeeded;
  BOOLEAN  Page1GSupport;

  Page1GSupport = FALSE;
  if (PcdGetBool (PcdUse1GPageTable)) {
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
  // Calculate the table entries needed.
  //
  if (PhysicalAddressBits <= 39 ) {
    NumberOfPml4EntriesNeeded = 1;
    NumberOfPdpEntriesNeeded  = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 30));
  } else {
    NumberOfPml4EntriesNeeded = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 39));
    NumberOfPdpEntriesNeeded  = 512;
  }

  if (!Page1GSupport) {
    TotalPagesNum = (NumberOfPdpEntriesNeeded + 1) * NumberOfPml4EntriesNeeded + 1;
  } else {
    TotalPagesNum = NumberOfPml4EntriesNeeded + 1;
  }

  return EFI_PAGES_TO_SIZE (TotalPagesNum);
}

/**
  Allocates and fills in the Page Directory and Page Table Entries to
  establish a 1:1 Virtual to Physical mapping.

  @param[in]  PageTablesAddress  The base address of page table.

**/
VOID
CreateIdentityMappingPageTables (
  IN  EFI_PHYSICAL_ADDRESS  PageTablesAddress
  )
{
  UINT32                          RegEax;
  UINT32                          RegEdx;
  UINT8                           PhysicalAddressBits;
  EFI_PHYSICAL_ADDRESS            PageAddress;
  UINTN                           IndexOfPml4Entries;
  UINTN                           IndexOfPdpEntries;
  UINTN                           IndexOfPageDirectoryEntries;
  UINT32                          NumberOfPml4EntriesNeeded;
  UINT32                          NumberOfPdpEntriesNeeded;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageMapLevel4Entry;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageMap;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageDirectoryPointerEntry;
  PAGE_TABLE_ENTRY                *PageDirectoryEntry;
  UINTN                           BigPageAddress;
  VOID                            *Hob;
  BOOLEAN                         Page1GSupport;
  PAGE_TABLE_1G_ENTRY             *PageDirectory1GEntry;

  Page1GSupport = FALSE;
  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000001) {
    AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
    if ((RegEdx & BIT26) != 0) {
      Page1GSupport = TRUE;
    }
  }

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
  // Calculate the table entries needed.
  //
  if (PhysicalAddressBits <= 39 ) {
    NumberOfPml4EntriesNeeded = 1;
    NumberOfPdpEntriesNeeded  = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 30));
  } else {
    NumberOfPml4EntriesNeeded = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 39));
    NumberOfPdpEntriesNeeded  = 512;
  }

  //
  // Pre-allocate big pages to avoid later allocations.
  //
  BigPageAddress = (UINTN)PageTablesAddress;

  //
  // By architecture only one PageMapLevel4 exists - so lets allocate storage for it.
  //
  PageMap         = (VOID *)BigPageAddress;
  BigPageAddress += SIZE_4KB;

  PageMapLevel4Entry = PageMap;
  PageAddress        = 0;
  for (IndexOfPml4Entries = 0; IndexOfPml4Entries < NumberOfPml4EntriesNeeded; IndexOfPml4Entries++, PageMapLevel4Entry++) {
    //
    // Each PML4 entry points to a page of Page Directory Pointer entires.
    // So lets allocate space for them and fill them in in the IndexOfPdpEntries loop.
    //
    PageDirectoryPointerEntry = (VOID *)BigPageAddress;
    BigPageAddress           += SIZE_4KB;

    //
    // Make a PML4 Entry
    //
    PageMapLevel4Entry->Uint64         = (UINT64)(UINTN)PageDirectoryPointerEntry;
    PageMapLevel4Entry->Bits.ReadWrite = 1;
    PageMapLevel4Entry->Bits.Present   = 1;

    if (Page1GSupport) {
      PageDirectory1GEntry = (VOID *)PageDirectoryPointerEntry;

      for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectory1GEntry++, PageAddress += SIZE_1GB) {
        //
        // Fill in the Page Directory entries
        //
        PageDirectory1GEntry->Uint64         = (UINT64)PageAddress;
        PageDirectory1GEntry->Bits.ReadWrite = 1;
        PageDirectory1GEntry->Bits.Present   = 1;
        PageDirectory1GEntry->Bits.MustBe1   = 1;
      }
    } else {
      for (IndexOfPdpEntries = 0; IndexOfPdpEntries < NumberOfPdpEntriesNeeded; IndexOfPdpEntries++, PageDirectoryPointerEntry++) {
        //
        // Each Directory Pointer entries points to a page of Page Directory entires.
        // So allocate space for them and fill them in in the IndexOfPageDirectoryEntries loop.
        //
        PageDirectoryEntry = (VOID *)BigPageAddress;
        BigPageAddress    += SIZE_4KB;

        //
        // Fill in a Page Directory Pointer Entries
        //
        PageDirectoryPointerEntry->Uint64         = (UINT64)(UINTN)PageDirectoryEntry;
        PageDirectoryPointerEntry->Bits.ReadWrite = 1;
        PageDirectoryPointerEntry->Bits.Present   = 1;

        for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectoryEntry++, PageAddress += SIZE_2MB) {
          //
          // Fill in the Page Directory entries
          //
          PageDirectoryEntry->Uint64         = (UINT64)PageAddress;
          PageDirectoryEntry->Bits.ReadWrite = 1;
          PageDirectoryEntry->Bits.Present   = 1;
          PageDirectoryEntry->Bits.MustBe1   = 1;
        }
      }

      for ( ; IndexOfPdpEntries < 512; IndexOfPdpEntries++, PageDirectoryPointerEntry++) {
        ZeroMem (
          PageDirectoryPointerEntry,
          sizeof (PAGE_MAP_AND_DIRECTORY_POINTER)
          );
      }
    }
  }

  //
  // For the PML4 entries we are not using fill in a null entry.
  //
  for ( ; IndexOfPml4Entries < 512; IndexOfPml4Entries++, PageMapLevel4Entry++) {
    ZeroMem (
      PageMapLevel4Entry,
      sizeof (PAGE_MAP_AND_DIRECTORY_POINTER)
      );
  }
}

/**
  If in 32 bit protection mode, and coalesce image is of X64, switch to long mode.

  @param  Entry                     Entry of Standalone MM Foundation.
  @param  Context1                  A pointer to the context to pass into the EntryPoint
                                    function.
  @param  Context2                  A pointer to the context to pass into the EntryPoint
                                    function.
  @retval EFI_SUCCESS               Successfully switched to long mode and execute coalesce.
  @retval Others                    Failed to execute coalesce in long mode.

**/
EFI_STATUS
ModeSwitch (
  IN EFI_PHYSICAL_ADDRESS  Entry,
  IN VOID                  *Context1,
  IN VOID                  *Context2
  )
{
  UINTN            PageTableAddress;
  UINTN            PageTableSize;
  EFI_STATUS       Status;
  IA32_DESCRIPTOR  Gdtr;

  DEBUG ((DEBUG_INFO, "ModeSwitch\n"));

  //
  // Save IA32 GDTR
  //
  AsmReadGdtr (&Gdtr);

  //
  // Set X64 GDTR
  //
  AsmWriteGdtr (&mGdt);

  PageTableAddress = AsmReadCr3 () & 0xFFFFF000;

  //
  // If page table was created, no need to create
  //
  if (PageTableAddress == 0) {
    PageTableSize = CalculatePageTableSize ();

    PageTableAddress = (UINTN)AllocatePages (EFI_SIZE_TO_PAGES (PageTableSize));
    ASSERT (PageTableAddress != 0);

    CreateIdentityMappingPageTables (PageTableAddress);

    AsmWriteCr3 ((UINTN)PageTableAddress);
  }

  DEBUG ((DEBUG_INFO, "AsmExecute64BitCode ...\n"));

  Status = AsmExecute64BitCode (Entry, (UINT64)(UINTN)Context1, (UINT64)(UINTN)Context2, NULL);
  if (Status != 0) {
    Status = Status | MAX_BIT;
  }

  DEBUG ((DEBUG_INFO, "AsmExecute64BitCode - %r\n", Status));

  //
  // Restore IA32 GDTR
  //
  AsmWriteGdtr (&Gdtr);

  return Status;
}

/**
  Load SMM core to dispatch other Standalone MM drivers after IA32 mode to X64 mode.

  @param  Entry                     Entry of Standalone MM Foundation.
  @param  Context1                  A pointer to the context to pass into the EntryPoint
                                    function.
  @retval EFI_SUCCESS               Successfully loaded SMM core.
  @retval Others                    Failed to load SMM core.
**/
EFI_STATUS
LoadSmmCoreIA32ToX64 (
  IN EFI_PHYSICAL_ADDRESS  Entry,
  IN VOID                  *Context1
  )
{
  return ModeSwitch (Entry, Context1, NULL);
}

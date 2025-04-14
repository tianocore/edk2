/** @file
X64 processor specific functions to enable SMM profile.

Copyright (c) 2012 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"
#include "SmmProfileInternal.h"

//
// Current page index.
//
UINTN  mPFPageIndex;

//
// Pool for dynamically creating page table in page fault handler.
//
UINT64  mPFPageBuffer;

//
// Store the uplink information for each page being used.
//
UINT64  *mPFPageUplink[MAX_PF_PAGE_COUNT];

/**
  Create SMM page table for S3 path.

  @param[out] Cr3    The base address of the page tables.

**/
VOID
InitSmmS3Cr3 (
  OUT UINTN  *Cr3
  )
{
  ASSERT (Cr3 != NULL);

  //
  // Generate level4 page table for the first 4GB memory space
  // Return the address of PML4 (to set CR3)
  //
  *Cr3 = GenSmmPageTable (Paging4Level, 32);

  return;
}

/**
  Allocate pages for creating 4KB-page based on 2MB-page when page fault happens.

**/
VOID
InitPagesForPFHandler (
  VOID
  )
{
  VOID  *Address;

  //
  // Pre-Allocate memory for page fault handler
  //
  Address = NULL;
  Address = AllocatePages (MAX_PF_PAGE_COUNT);
  ASSERT (Address != NULL);

  mPFPageBuffer =  (UINT64)(UINTN)Address;
  mPFPageIndex  = 0;
  ZeroMem ((VOID *)(UINTN)mPFPageBuffer, EFI_PAGE_SIZE * MAX_PF_PAGE_COUNT);
  ZeroMem (mPFPageUplink, sizeof (mPFPageUplink));

  return;
}

/**
  Allocate one page for creating 4KB-page based on 2MB-page.

  @param  Uplink   The address of Page-Directory entry.

**/
VOID
AcquirePage (
  UINT64  *Uplink
  )
{
  UINT64  Address;

  //
  // Get the buffer
  //
  Address = mPFPageBuffer + EFI_PAGES_TO_SIZE (mPFPageIndex);
  ZeroMem ((VOID *)(UINTN)Address, EFI_PAGE_SIZE);

  //
  // Cut the previous uplink if it exists and wasn't overwritten
  //
  if ((mPFPageUplink[mPFPageIndex] != NULL) && ((*mPFPageUplink[mPFPageIndex] & ~mAddressEncMask & PHYSICAL_ADDRESS_MASK) == Address)) {
    *mPFPageUplink[mPFPageIndex] = 0;
  }

  //
  // Link & Record the current uplink. Not a leaf entry so do not include the
  // encryption mask.
  //
  *Uplink                     = Address | PAGE_ATTRIBUTE_BITS;
  mPFPageUplink[mPFPageIndex] = Uplink;

  mPFPageIndex = (mPFPageIndex + 1) % MAX_PF_PAGE_COUNT;
}

/**
  Create new entry in page table for page fault address in SmmProfilePFHandler.

**/
VOID
SmmProfileMapPFAddress (
  VOID
  )
{
  UINT64              *PageTable;
  UINT64              *PageTableTop;
  UINT64              PFAddress;
  UINTN               StartBit;
  UINTN               EndBit;
  UINT64              PTIndex;
  UINTN               Index;
  SMM_PAGE_SIZE_TYPE  PageSize;
  UINTN               NumOfPages;
  UINTN               PageAttribute;
  EFI_STATUS          Status;
  UINT64              *UpperEntry;
  BOOLEAN             Enable5LevelPaging;
  IA32_CR4            Cr4;

  //
  // Set default SMM page attribute
  //
  PageSize      = SmmPageSize2M;
  NumOfPages    = 1;
  PageAttribute = 0;

  EndBit       = 0;
  PageTableTop = (UINT64 *)(AsmReadCr3 () & gPhyMask);
  PFAddress    = AsmReadCr2 ();

  Cr4.UintN          = AsmReadCr4 ();
  Enable5LevelPaging = (BOOLEAN)(Cr4.Bits.LA57 != 0);

  Status = GetPlatformPageTableAttribute (PFAddress, &PageSize, &NumOfPages, &PageAttribute);
  //
  // If platform not support page table attribute, set default SMM page attribute
  //
  if (Status != EFI_SUCCESS) {
    PageSize      = SmmPageSize2M;
    NumOfPages    = 1;
    PageAttribute = 0;
  }

  if (PageSize >= MaxSmmPageSizeType) {
    PageSize = SmmPageSize2M;
  }

  if (NumOfPages > 512) {
    NumOfPages = 512;
  }

  switch (PageSize) {
    case SmmPageSize4K:
      //
      // BIT12 to BIT20 is Page Table index
      //
      EndBit = 12;
      break;
    case SmmPageSize2M:
      //
      // BIT21 to BIT29 is Page Directory index
      //
      EndBit         = 21;
      PageAttribute |= (UINTN)IA32_PG_PS;
      break;
    case SmmPageSize1G:
      if (!m1GPageTableSupport) {
        DEBUG ((DEBUG_ERROR, "1-GByte pages is not supported!"));
        ASSERT (FALSE);
      }

      //
      // BIT30 to BIT38 is Page Directory Pointer Table index
      //
      EndBit         = 30;
      PageAttribute |= (UINTN)IA32_PG_PS;
      break;
    default:
      ASSERT (FALSE);
  }

  //
  // If execute-disable is enabled, set NX bit
  //
  if (mXdEnabled) {
    PageAttribute |= IA32_PG_NX;
  }

  for (Index = 0; Index < NumOfPages; Index++) {
    PageTable  = PageTableTop;
    UpperEntry = NULL;
    for (StartBit = Enable5LevelPaging ? 48 : 39; StartBit > 12; StartBit -= 9) {
      PTIndex = BitFieldRead64 (PFAddress, StartBit, StartBit + 8);

      //
      // Iterate through the page table to find the appropriate page table entry for page creation if one of the following cases is met:
      // 1) StartBit > EndBit: The PageSize of current entry is bigger than the platform-specified PageSize granularity.
      // 2) IA32_PG_P bit is 0 & IA32_PG_PS bit is not 0: The current entry is present and it's a non-leaf entry.
      //
      if ((StartBit > EndBit) || ((((PageTable[PTIndex] & IA32_PG_P) != 0) && ((PageTable[PTIndex] & IA32_PG_PS) == 0)))) {
        if ((PageTable[PTIndex] & IA32_PG_P) == 0) {
          //
          // If the entry is not present, allocate one page from page pool for it
          //
          PageTable[PTIndex] = AllocPage () | mAddressEncMask | PAGE_ATTRIBUTE_BITS;
        } else {
          //
          // Save the upper entry address
          //
          UpperEntry = PageTable + PTIndex;
        }

        //
        // BIT9 to BIT11 of entry is used to save access record,
        // initialize value is 7
        //
        PageTable[PTIndex] |= (UINT64)IA32_PG_A;
        SetAccNum (PageTable + PTIndex, 7);
        PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & ~mAddressEncMask & gPhyMask);
      } else {
        //
        // Found the appropriate entry.
        //
        break;
      }
    }

    PTIndex = BitFieldRead64 (PFAddress, StartBit, StartBit + 8);

    //
    // Fill the new entry
    //
    PageTable[PTIndex] = ((PFAddress | mAddressEncMask) & gPhyMask & ~((1ull << StartBit) - 1)) |
                         PageAttribute | IA32_PG_A | PAGE_ATTRIBUTE_BITS;
    if (UpperEntry != NULL) {
      SetSubEntriesNum (UpperEntry, (GetSubEntriesNum (UpperEntry) + 1) & 0x1FF);
    }

    //
    // Get the next page address if we need to create more page tables
    //
    PFAddress += (1ull << StartBit);
  }
}

/**
  Update page table to map the memory correctly in order to make the instruction
  which caused page fault execute successfully. And it also save the original page
  table to be restored in single-step exception.

  @param  PageTable           PageTable Address.
  @param  PFAddress           The memory address which caused page fault exception.
  @param  CpuIndex            The index of the processor.
  @param  ErrorCode           The Error code of exception.
  @param  IsValidPFAddress    The flag indicates if SMM profile data need be added.

**/
VOID
RestorePageTableAbove4G (
  UINT64   *PageTable,
  UINT64   PFAddress,
  UINTN    CpuIndex,
  UINTN    ErrorCode,
  BOOLEAN  *IsValidPFAddress
  )
{
  UINTN     PTIndex;
  UINT64    Address;
  BOOLEAN   Nx;
  BOOLEAN   Existed;
  UINTN     Index;
  UINTN     PFIndex;
  IA32_CR4  Cr4;
  BOOLEAN   Enable5LevelPaging;

  ASSERT ((PageTable != NULL) && (IsValidPFAddress != NULL));

  Cr4.UintN          = AsmReadCr4 ();
  Enable5LevelPaging = (BOOLEAN)(Cr4.Bits.LA57 == 1);

  //
  // If page fault address is 4GB above.
  //

  //
  // Check if page fault address has existed in page table.
  // If it exists in page table but page fault is generated,
  // there are 2 possible reasons: 1. present flag is set to 0; 2. instruction fetch in protected memory range.
  //
  Existed   = FALSE;
  PageTable = (UINT64 *)(AsmReadCr3 () & PHYSICAL_ADDRESS_MASK);
  PTIndex   = 0;
  if (Enable5LevelPaging) {
    PTIndex = BitFieldRead64 (PFAddress, 48, 56);
  }

  if ((!Enable5LevelPaging) || ((PageTable[PTIndex] & IA32_PG_P) != 0)) {
    // PML5E
    if (Enable5LevelPaging) {
      PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & PHYSICAL_ADDRESS_MASK);
    }

    PTIndex = BitFieldRead64 (PFAddress, 39, 47);
    if ((PageTable[PTIndex] & IA32_PG_P) != 0) {
      // PML4E
      PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & PHYSICAL_ADDRESS_MASK);
      PTIndex   = BitFieldRead64 (PFAddress, 30, 38);
      if ((PageTable[PTIndex] & IA32_PG_P) != 0) {
        // PDPTE
        PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & PHYSICAL_ADDRESS_MASK);
        PTIndex   = BitFieldRead64 (PFAddress, 21, 29);
        // PD
        if ((PageTable[PTIndex] & IA32_PG_PS) != 0) {
          //
          // 2MB page
          //
          Address = (UINT64)(PageTable[PTIndex] & ~mAddressEncMask & PHYSICAL_ADDRESS_MASK);
          if ((Address & ~((1ull << 21) - 1)) == ((PFAddress & PHYSICAL_ADDRESS_MASK & ~((1ull << 21) - 1)))) {
            Existed = TRUE;
          }
        } else {
          //
          // 4KB page
          //
          PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & PHYSICAL_ADDRESS_MASK);
          if (PageTable != 0) {
            //
            // When there is a valid entry to map to 4KB page, need not create a new entry to map 2MB.
            //
            PTIndex = BitFieldRead64 (PFAddress, 12, 20);
            Address = (UINT64)(PageTable[PTIndex] & ~mAddressEncMask & PHYSICAL_ADDRESS_MASK);
            if ((Address & ~((1ull << 12) - 1)) == (PFAddress & PHYSICAL_ADDRESS_MASK & ~((1ull << 12) - 1))) {
              Existed = TRUE;
            }
          }
        }
      }
    }
  }

  //
  // If page entry does not existed in page table at all, create a new entry.
  //
  if (!Existed) {
    if (IsSmmProfilePFAddressAbove4GValid (PFAddress, &Nx)) {
      //
      // If page fault address above 4GB is in protected range but it causes a page fault exception,
      // Will create a page entry for this page fault address, make page table entry as present/rw and execution-disable.
      // this access is not saved into SMM profile data.
      //
      *IsValidPFAddress = TRUE;
    }

    //
    // Create one entry in page table for page fault address.
    //
    SmmProfileMapPFAddress ();
    //
    // Find the page table entry created just now.
    //
    PageTable = (UINT64 *)(AsmReadCr3 () & PHYSICAL_ADDRESS_MASK);
    PFAddress = AsmReadCr2 ();
    // PML5E
    if (Enable5LevelPaging) {
      PTIndex   = BitFieldRead64 (PFAddress, 48, 56);
      PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & PHYSICAL_ADDRESS_MASK);
    }

    // PML4E
    PTIndex   = BitFieldRead64 (PFAddress, 39, 47);
    PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & PHYSICAL_ADDRESS_MASK);
    // PDPTE
    PTIndex   = BitFieldRead64 (PFAddress, 30, 38);
    PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & PHYSICAL_ADDRESS_MASK);
    // PD
    PTIndex = BitFieldRead64 (PFAddress, 21, 29);
    Address = PageTable[PTIndex] & ~mAddressEncMask & PHYSICAL_ADDRESS_MASK;
    //
    // Check if 2MB-page entry need be changed to 4KB-page entry.
    //
    if (IsAddressSplit (Address)) {
      AcquirePage (&PageTable[PTIndex]);

      // PTE
      PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & PHYSICAL_ADDRESS_MASK);
      for (Index = 0; Index < 512; Index++) {
        PageTable[Index] = Address | mAddressEncMask | PAGE_ATTRIBUTE_BITS;
        if (!IsSmmProfilePFAddressAbove4GValid (Address, &Nx)) {
          PageTable[Index] = PageTable[Index] & (INTN)(INT32)(~PAGE_ATTRIBUTE_BITS);
        }

        if (Nx && mXdSupported) {
          PageTable[Index] = PageTable[Index] | IA32_PG_NX;
        }

        if (Address == (PFAddress & PHYSICAL_ADDRESS_MASK & ~((1ull << 12) - 1))) {
          PTIndex = Index;
        }

        Address += SIZE_4KB;
      } // end for PT
    } else {
      //
      // Update 2MB page entry.
      //
      if (!IsSmmProfilePFAddressAbove4GValid (Address, &Nx)) {
        //
        // Patch to remove present flag and rw flag.
        //
        PageTable[PTIndex] = PageTable[PTIndex] & (INTN)(INT32)(~PAGE_ATTRIBUTE_BITS);
      }

      //
      // Set XD bit to 1
      //
      if (Nx && mXdSupported) {
        PageTable[PTIndex] = PageTable[PTIndex] | IA32_PG_NX;
      }
    }
  }

  //
  // Record old entries with non-present status
  // Old entries include the memory which instruction is at and the memory which instruction access.
  //
  //
  ASSERT (mPFEntryCount[CpuIndex] < MAX_PF_ENTRY_COUNT);
  if (mPFEntryCount[CpuIndex] < MAX_PF_ENTRY_COUNT) {
    PFIndex                                = mPFEntryCount[CpuIndex];
    mLastPFEntryValue[CpuIndex][PFIndex]   = PageTable[PTIndex];
    mLastPFEntryPointer[CpuIndex][PFIndex] = &PageTable[PTIndex];
    mPFEntryCount[CpuIndex]++;
  }

  //
  // Add present flag or clear XD flag to make page fault handler succeed.
  //
  PageTable[PTIndex] |= (UINT64)(PAGE_ATTRIBUTE_BITS);
  if ((ErrorCode & IA32_PF_EC_ID) != 0) {
    //
    // If page fault is caused by instruction fetch, clear XD bit in the entry.
    //
    PageTable[PTIndex] &= ~IA32_PG_NX;
  }

  return;
}

/**
  Clear TF in FLAGS.

  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.

**/
VOID
ClearTrapFlag (
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  SystemContext.SystemContextX64->Rflags &= (UINTN) ~BIT8;
}

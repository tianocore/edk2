/** @file
X64 processor specific functions to enable SMM profile.

Copyright (c) 2012 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuDxeSmm.h"
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

**/
VOID
InitSmmS3Cr3 (
  VOID
  )
{
  EFI_PHYSICAL_ADDRESS  Pages;
  UINT64                *PTEntry;

  //
  // Generate PAE page table for the first 4GB memory space
  //
  Pages = Gen4GPageTable (FALSE);

  //
  // Fill Page-Table-Level4 (PML4) entry
  //
  PTEntry = (UINT64 *)AllocatePageTableMemory (1);
  ASSERT (PTEntry != NULL);
  *PTEntry = Pages | mAddressEncMask | PAGE_ATTRIBUTE_BITS;
  ZeroMem (PTEntry + 1, EFI_PAGE_SIZE - sizeof (*PTEntry));

  //
  // Return the address of PML4 (to set CR3)
  //
  mSmmS3ResumeState->SmmS3Cr3 = (UINT32)(UINTN)PTEntry;

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
  // Link & Record the current uplink
  //
  *Uplink                     = Address | mAddressEncMask | PAGE_ATTRIBUTE_BITS;
  mPFPageUplink[mPFPageIndex] = Uplink;

  mPFPageIndex = (mPFPageIndex + 1) % MAX_PF_PAGE_COUNT;
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
      PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & ~mAddressEncMask & PHYSICAL_ADDRESS_MASK);
    }

    PTIndex = BitFieldRead64 (PFAddress, 39, 47);
    if ((PageTable[PTIndex] & IA32_PG_P) != 0) {
      // PML4E
      PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & ~mAddressEncMask & PHYSICAL_ADDRESS_MASK);
      PTIndex   = BitFieldRead64 (PFAddress, 30, 38);
      if ((PageTable[PTIndex] & IA32_PG_P) != 0) {
        // PDPTE
        PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & ~mAddressEncMask & PHYSICAL_ADDRESS_MASK);
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
          PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & ~mAddressEncMask& PHYSICAL_ADDRESS_MASK);
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
    if (IsAddressValid (PFAddress, &Nx)) {
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
    SmiDefaultPFHandler ();
    //
    // Find the page table entry created just now.
    //
    PageTable = (UINT64 *)(AsmReadCr3 () & PHYSICAL_ADDRESS_MASK);
    PFAddress = AsmReadCr2 ();
    // PML5E
    if (Enable5LevelPaging) {
      PTIndex   = BitFieldRead64 (PFAddress, 48, 56);
      PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & ~mAddressEncMask & PHYSICAL_ADDRESS_MASK);
    }

    // PML4E
    PTIndex   = BitFieldRead64 (PFAddress, 39, 47);
    PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & ~mAddressEncMask & PHYSICAL_ADDRESS_MASK);
    // PDPTE
    PTIndex   = BitFieldRead64 (PFAddress, 30, 38);
    PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & ~mAddressEncMask & PHYSICAL_ADDRESS_MASK);
    // PD
    PTIndex = BitFieldRead64 (PFAddress, 21, 29);
    Address = PageTable[PTIndex] & ~mAddressEncMask & PHYSICAL_ADDRESS_MASK;
    //
    // Check if 2MB-page entry need be changed to 4KB-page entry.
    //
    if (IsAddressSplit (Address)) {
      AcquirePage (&PageTable[PTIndex]);

      // PTE
      PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & ~mAddressEncMask & PHYSICAL_ADDRESS_MASK);
      for (Index = 0; Index < 512; Index++) {
        PageTable[Index] = Address | mAddressEncMask | PAGE_ATTRIBUTE_BITS;
        if (!IsAddressValid (Address, &Nx)) {
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
      if (!IsAddressValid (Address, &Nx)) {
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

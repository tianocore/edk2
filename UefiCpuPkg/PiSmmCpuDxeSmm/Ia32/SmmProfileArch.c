/** @file
IA-32 processor specific functions to enable SMM profile.

Copyright (c) 2012 - 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"
#include "SmmProfileInternal.h"

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

  *Cr3 = GenSmmPageTable (PagingPae, mPhysicalAddressBits);

  return;
}

/**
  Allocate pages for creating 4KB-page based on 2MB-page when page fault happens.
  32-bit firmware does not need it.

**/
VOID
InitPagesForPFHandler (
  VOID
  )
{
}

/**
  Update page table to map the memory correctly in order to make the instruction
  which caused page fault execute successfully. And it also save the original page
  table to be restored in single-step exception. 32-bit firmware does not need it.

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
  SystemContext.SystemContextIa32->Eflags &= (UINTN) ~BIT8;
}

/**
  Create new entry in page table for page fault address in SmmProfilePFHandler.

**/
VOID
SmmProfileMapPFAddress (
  VOID
  )
{
  CpuDeadLoop ();
}

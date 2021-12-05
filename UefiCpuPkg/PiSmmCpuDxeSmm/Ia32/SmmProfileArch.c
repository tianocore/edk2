/** @file
IA-32 processor specific functions to enable SMM profile.

Copyright (c) 2012 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuDxeSmm.h"
#include "SmmProfileInternal.h"

/**
  Create SMM page table for S3 path.

**/
VOID
InitSmmS3Cr3 (
  VOID
  )
{
  mSmmS3ResumeState->SmmS3Cr3 = Gen4GPageTable (TRUE);

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

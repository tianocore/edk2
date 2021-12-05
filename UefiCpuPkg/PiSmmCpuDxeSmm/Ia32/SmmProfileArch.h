/** @file
IA-32 processor specific header file to enable SMM profile.

Copyright (c) 2012 - 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_PROFILE_ARCH_H_
#define _SMM_PROFILE_ARCH_H_

#pragma pack (1)

typedef struct _MSR_DS_AREA_STRUCT {
  UINT32    BTSBufferBase;
  UINT32    BTSIndex;
  UINT32    BTSAbsoluteMaximum;
  UINT32    BTSInterruptThreshold;
  UINT32    PEBSBufferBase;
  UINT32    PEBSIndex;
  UINT32    PEBSAbsoluteMaximum;
  UINT32    PEBSInterruptThreshold;
  UINT32    PEBSCounterReset[4];
  UINT32    Reserved;
} MSR_DS_AREA_STRUCT;

typedef struct _BRANCH_TRACE_RECORD {
  UINT32    LastBranchFrom;
  UINT32    LastBranchTo;
  UINT32    Rsvd0           : 4;
  UINT32    BranchPredicted : 1;
  UINT32    Rsvd1           : 27;
} BRANCH_TRACE_RECORD;

typedef struct _PEBS_RECORD {
  UINT32    Eflags;
  UINT32    LinearIP;
  UINT32    Eax;
  UINT32    Ebx;
  UINT32    Ecx;
  UINT32    Edx;
  UINT32    Esi;
  UINT32    Edi;
  UINT32    Ebp;
  UINT32    Esp;
} PEBS_RECORD;

#pragma pack ()

#define PHYSICAL_ADDRESS_MASK  ((1ull << 32) - SIZE_4KB)

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
  );

/**
  Create SMM page table for S3 path.

**/
VOID
InitSmmS3Cr3 (
  VOID
  );

/**
  Allocate pages for creating 4KB-page based on 2MB-page when page fault happens.

**/
VOID
InitPagesForPFHandler (
  VOID
  );

#endif // _SMM_PROFILE_ARCH_H_

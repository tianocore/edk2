/** @file
X64 processor specific header file to enable SMM profile.

Copyright (c) 2012 - 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_PROFILE_ARCH_H_
#define _SMM_PROFILE_ARCH_H_

#pragma pack (1)

typedef struct _MSR_DS_AREA_STRUCT {
  UINT64    BTSBufferBase;
  UINT64    BTSIndex;
  UINT64    BTSAbsoluteMaximum;
  UINT64    BTSInterruptThreshold;
  UINT64    PEBSBufferBase;
  UINT64    PEBSIndex;
  UINT64    PEBSAbsoluteMaximum;
  UINT64    PEBSInterruptThreshold;
  UINT64    PEBSCounterReset[2];
  UINT64    Reserved;
} MSR_DS_AREA_STRUCT;

typedef struct _BRANCH_TRACE_RECORD {
  UINT64    LastBranchFrom;
  UINT64    LastBranchTo;
  UINT64    Rsvd0           : 4;
  UINT64    BranchPredicted : 1;
  UINT64    Rsvd1           : 59;
} BRANCH_TRACE_RECORD;

typedef struct _PEBS_RECORD {
  UINT64    Rflags;
  UINT64    LinearIP;
  UINT64    Rax;
  UINT64    Rbx;
  UINT64    Rcx;
  UINT64    Rdx;
  UINT64    Rsi;
  UINT64    Rdi;
  UINT64    Rbp;
  UINT64    Rsp;
  UINT64    R8;
  UINT64    R9;
  UINT64    R10;
  UINT64    R11;
  UINT64    R12;
  UINT64    R13;
  UINT64    R14;
  UINT64    R15;
} PEBS_RECORD;

#pragma pack ()

extern BOOLEAN  m1GPageTableSupport;

#define PHYSICAL_ADDRESS_MASK  ((1ull << 52) - SIZE_4KB)

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
  );

/**
  Create SMM page table for S3 path.

  @param[out] Cr3    The base address of the page tables.

**/
VOID
InitSmmS3Cr3 (
  OUT UINTN  *Cr3
  );

/**
  Allocate pages for creating 4KB-page based on 2MB-page when page fault happens.

**/
VOID
InitPagesForPFHandler (
  VOID
  );

/**
  Set sub-entries number in entry.

  @param[in, out] Entry        Pointer to entry
  @param[in]      SubEntryNum  Sub-entries number based on 0:
                               0 means there is 1 sub-entry under this entry
                               0x1ff means there is 512 sub-entries under this entry

**/
VOID
SetSubEntriesNum (
  IN OUT UINT64  *Entry,
  IN     UINT64  SubEntryNum
  );

/**
  Return sub-entries number in entry.

  @param[in] Entry        Pointer to entry

  @return Sub-entries number based on 0:
          0 means there is 1 sub-entry under this entry
          0x1ff means there is 512 sub-entries under this entry
**/
UINT64
GetSubEntriesNum (
  IN UINT64  *Entry
  );

/**
  Allocate free Page for PageFault handler use.

  @return Page address.

**/
UINT64
AllocPage (
  VOID
  );

/**
  Set access record in entry.

  @param[in, out] Entry        Pointer to entry
  @param[in]      Acc          Access record value

**/
VOID
SetAccNum (
  IN OUT UINT64  *Entry,
  IN     UINT64  Acc
  );

#endif // _SMM_PROFILE_ARCH_H_

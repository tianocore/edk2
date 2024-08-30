/** @file
SMM profile internal header file.

Copyright (c) 2012 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2020, AMD Incorporated. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_PROFILE_INTERNAL_H_
#define _SMM_PROFILE_INTERNAL_H_

#include <Protocol/SmmReadyToLock.h>
#include <Library/CpuLib.h>
#include <IndustryStandard/Acpi.h>

#include "SmmProfileArch.h"

//
// Configure the SMM_PROFILE DTS region size
//
#define SMM_PROFILE_DTS_SIZE  (4 * 1024 * 1024)      // 4M

#define MAX_PF_PAGE_COUNT  0x2

#define PEBS_RECORD_NUMBER  0x2

#define MAX_PF_ENTRY_COUNT  10

//
// This MACRO just enable unit test for the profile
// Please disable it.
//

#define IA32_PF_EC_ID  (1u << 4)

#define SMM_PROFILE_NAME  L"SmmProfileData"

//
// CPU generic definition
//
#define   MSR_EFER     0xc0000080
#define   MSR_EFER_XD  0x800

#define   CPUID1_EDX_BTS_AVAILABLE  0x200000

#define   DR6_SINGLE_STEP  0x4000
#define   RFLAG_TF         0x100

#define MSR_DEBUG_CTL          0x1D9
#define   MSR_DEBUG_CTL_LBR    0x1
#define   MSR_DEBUG_CTL_TR     0x40
#define   MSR_DEBUG_CTL_BTS    0x80
#define   MSR_DEBUG_CTL_BTINT  0x100
#define MSR_DS_AREA            0x600

#define HEAP_GUARD_NONSTOP_MODE      \
        ((PcdGet8 (PcdHeapGuardPropertyMask) & (BIT6|BIT3|BIT2)) > BIT6)

#define NULL_DETECTION_NONSTOP_MODE  \
        ((PcdGet8 (PcdNullPointerDetectionPropertyMask) & (BIT6|BIT1)) > BIT6)

typedef struct {
  EFI_PHYSICAL_ADDRESS    Base;
  EFI_PHYSICAL_ADDRESS    Top;
} MEMORY_RANGE;

typedef struct {
  MEMORY_RANGE    Range;
  BOOLEAN         Present;
  BOOLEAN         Nx;
} MEMORY_PROTECTION_RANGE;

typedef struct {
  UINT64    HeaderSize;
  UINT64    MaxDataEntries;
  UINT64    MaxDataSize;
  UINT64    CurDataEntries;
  UINT64    CurDataSize;
  UINT64    TsegStart;
  UINT64    TsegSize;
  UINT64    NumSmis;
  UINT64    NumCpus;
} SMM_PROFILE_HEADER;

typedef struct {
  UINT64    SmiNum;
  UINT64    CpuNum;
  UINT64    ApicId;
  UINT64    ErrorCode;
  UINT64    Instruction;
  UINT64    Address;
  UINT64    SmiCmd;
} SMM_PROFILE_ENTRY;

extern UINTN              gSmiExceptionHandlers[];
extern BOOLEAN            mXdSupported;
X86_ASSEMBLY_PATCH_LABEL  gPatchXdSupported;
X86_ASSEMBLY_PATCH_LABEL  gPatchMsrIa32MiscEnableSupported;
extern UINTN              *mPFEntryCount;
extern UINT64 (*mLastPFEntryValue)[MAX_PF_ENTRY_COUNT];
extern UINT64                    *(*mLastPFEntryPointer)[MAX_PF_ENTRY_COUNT];

//
// Internal functions
//

/**
  Update IDT table to replace page fault handler and INT 1 handler.

**/
VOID
InitIdtr (
  VOID
  );

/**
  Check if the memory address will be mapped by 4KB-page.

  @param  Address  The address of Memory.

**/
BOOLEAN
IsAddressSplit (
  IN EFI_PHYSICAL_ADDRESS  Address
  );

/**
  Check if the SMM profile page fault address above 4GB is in protected range or not.

  @param[in]   Address  The address of Memory.
  @param[out]  Nx       The flag indicates if the memory is execute-disable.

  @retval TRUE     The input address is in protected range.
  @retval FALSE    The input address is not in protected range.

**/
BOOLEAN
IsSmmProfilePFAddressAbove4GValid (
  IN  EFI_PHYSICAL_ADDRESS  Address,
  OUT BOOLEAN               *Nx
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
  Create new entry in page table for page fault address in SmmProfilePFHandler.

**/
VOID
SmmProfileMapPFAddress (
  VOID
  );

/**
  Clear TF in FLAGS.

  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.

**/
VOID
ClearTrapFlag (
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  );

#endif // _SMM_PROFILE_H_

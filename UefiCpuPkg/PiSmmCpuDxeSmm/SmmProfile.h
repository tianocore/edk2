/** @file
SMM profile header file.

Copyright (c) 2012 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_PROFILE_H_
#define _SMM_PROFILE_H_

#include "SmmProfileInternal.h"

//
// External functions
//

/**
  Initialize processor environment for SMM profile.

  @param  CpuIndex  The index of the processor.

**/
VOID
ActivateSmmProfile (
  IN UINTN  CpuIndex
  );

/**
  Initialize SMM profile in SMM CPU entry point.

  @param[in] Cr3  The base address of the page tables to use in SMM.

**/
VOID
InitSmmProfile (
  UINT32  Cr3
  );

/**
  Increase SMI number in each SMI entry.

**/
VOID
SmmProfileRecordSmiNum (
  VOID
  );

/**
  The Page fault handler to save SMM profile data.

  @param  Rip        The RIP when exception happens.
  @param  ErrorCode  The Error code of exception.

**/
VOID
SmmProfilePFHandler (
  UINTN  Rip,
  UINTN  ErrorCode
  );

/**
  Updates page table to make some memory ranges (like system memory) absent
  and make some memory ranges (like MMIO) present and execute disable. It also
  update 2MB-page to 4KB-page for some memory ranges.

**/
VOID
SmmProfileStart (
  VOID
  );

/**
  Page fault IDT handler for SMM Profile.

**/
VOID
EFIAPI
PageFaultIdtHandlerSmmProfile (
  VOID
  );

/**
  Check if feature is supported by a processor.

  @param CpuIndex        The index of the CPU.
**/
VOID
CheckFeatureSupported (
  IN UINTN  CpuIndex
  );

/**
  Initialize the protected memory ranges and the 4KB-page mapped memory ranges.

**/
VOID
InitProtectedMemRange (
  VOID
  );

/**
  This function updates memory attribute according to mProtectionMemRangeCount.

**/
VOID
SmmProfileUpdateMemoryAttributes (
  VOID
  );

/**
  Get CPU Index from APIC ID.

**/
UINTN
GetCpuIndex (
  VOID
  );

/**
  Handler for Page Fault triggered by Guard page.

  @param  ErrorCode  The Error code of exception.

**/
VOID
GuardPagePFHandler (
  UINTN  ErrorCode
  );

//
// The flag indicates if execute-disable is supported by processor.
//
extern BOOLEAN  mXdSupported;
//
// The flag indicates if execute-disable is enabled on processor.
//
extern BOOLEAN  mXdEnabled;
//
// The flag indicates if SMM profile is enabled.
//
extern BOOLEAN  mSmmProfileEnabled;
//
// The flag indicates if #DB will be setup in #PF handler.
//
extern BOOLEAN  mSetupDebugTrap;
//
// SMI command port.
//
extern UINT32  mSmiCommandPort;

#endif // _SMM_PROFILE_H_

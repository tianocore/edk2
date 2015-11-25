/** @file
SMM profile header file.

Copyright (c) 2012 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMM_PROFILE_H_
#define _SMM_PROFILE_H_

#include "SmmProfileInternal.h"

///
/// MSR Register Index
///
#define MSR_IA32_MISC_ENABLE                  0x1A0

//
// External functions
//

/**
  Initialize processor environment for SMM profile.

  @param  CpuIndex  The index of the processor.

**/
VOID
ActivateSmmProfile (
  IN UINTN CpuIndex
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
  UINTN Rip,
  UINTN ErrorCode
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
  Check if XD feature is supported by a processor.

**/
VOID
CheckFeatureSupported (
  VOID
  );

/**
  Enable XD feature.

**/
VOID
ActivateXd (
  VOID
  );

/**
  Update page table according to protected memory ranges and the 4KB-page mapped memory ranges.

**/
VOID
InitPaging (
  VOID
  );

/**
  Check if XD and BTS features are supported by all processors.

**/
VOID
CheckProcessorFeature (
  VOID
  );

extern BOOLEAN    mXdSupported;
extern BOOLEAN    mXdEnabled;

#endif // _SMM_PROFILE_H_

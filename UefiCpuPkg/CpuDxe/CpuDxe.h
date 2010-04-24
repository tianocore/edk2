/** @file
  CPU DXE Module.

  Copyright (c) 2008 - 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CPU_DXE_H
#define _CPU_DXE_H

#include <PiDxe.h>

#include <Protocol/Cpu.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/CpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/MtrrLib.h>

//
//
//
#define INTERRUPT_VECTOR_NUMBER   256

#define EFI_MEMORY_CACHETYPE_MASK     (EFI_MEMORY_UC  | \
                                       EFI_MEMORY_WC  | \
                                       EFI_MEMORY_WT  | \
                                       EFI_MEMORY_WB  | \
                                       EFI_MEMORY_UCE   \
                                       )


//
// Function declarations
//
EFI_STATUS
EFIAPI
CpuFlushCpuDataCache (
  IN EFI_CPU_ARCH_PROTOCOL     *This,
  IN EFI_PHYSICAL_ADDRESS      Start,
  IN UINT64                    Length,
  IN EFI_CPU_FLUSH_TYPE        FlushType
  );

EFI_STATUS
EFIAPI
CpuEnableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL     *This
  );

EFI_STATUS
EFIAPI
CpuDisableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL     *This
  );

EFI_STATUS
EFIAPI
CpuGetInterruptState (
  IN  EFI_CPU_ARCH_PROTOCOL     *This,
  OUT BOOLEAN                   *State
  );

EFI_STATUS
EFIAPI
CpuInit (
  IN EFI_CPU_ARCH_PROTOCOL     *This,
  IN EFI_CPU_INIT_TYPE         InitType
  );

EFI_STATUS
EFIAPI
CpuRegisterInterruptHandler (
  IN EFI_CPU_ARCH_PROTOCOL         *This,
  IN EFI_EXCEPTION_TYPE            InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER     InterruptHandler
  );

EFI_STATUS
EFIAPI
CpuGetTimerValue (
  IN  EFI_CPU_ARCH_PROTOCOL       *This,
  IN  UINT32                      TimerIndex,
  OUT UINT64                      *TimerValue,
  OUT UINT64                      *TimerPeriod OPTIONAL
  );

EFI_STATUS
EFIAPI
CpuSetMemoryAttributes (
  IN EFI_CPU_ARCH_PROTOCOL      *This,
  IN EFI_PHYSICAL_ADDRESS       BaseAddress,
  IN UINT64                     Length,
  IN UINT64                     Attributes
  );

VOID
EFIAPI
AsmIdtVector00 (
  VOID
  );

VOID
EFIAPI
InitializeExternalVectorTablePtr (
  EFI_CPU_INTERRUPT_HANDLER  *VectorTable
  );

VOID
InitGlobalDescriptorTable (
  VOID
  );

VOID
EFIAPI
SetCodeSelector (
  UINT16 Selector
  );

VOID
EFIAPI
SetDataSelectors (
  UINT16 Selector
  );


#endif


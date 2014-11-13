/** @file
  CPU DXE MP support

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CPU_MP_H_
#define _CPU_MP_H_

#include <Protocol/MpService.h>
#include <Library/SynchronizationLib.h>

/**
  Initialize Multi-processor support.

**/
VOID
InitializeMpSupport (
  VOID
  );

typedef
VOID
(EFIAPI *STACKLESS_AP_ENTRY_POINT)(
  VOID
  );

/**
  Starts the Application Processors and directs them to jump to the
  specified routine.

  The processor jumps to this code in flat mode, but the processor's
  stack is not initialized.

  @param ApEntryPoint    Pointer to the Entry Point routine

  @retval EFI_SUCCESS           The APs were started
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate memory to start APs

**/
EFI_STATUS
StartApsStackless (
  IN STACKLESS_AP_ENTRY_POINT ApEntryPoint
  );

/**
  The AP entry point that the Startup-IPI target code will jump to.

  The processor jumps to this code in flat mode, but the processor's
  stack is not initialized.

**/
VOID
EFIAPI
AsmApEntryPoint (
  VOID
  );

/**
  Releases the lock preventing other APs from using the shared AP
  stack.

  Once the AP has transitioned to using a new stack, it can call this
  function to allow another AP to proceed with using the shared stack.

**/
VOID
EFIAPI
AsmApDoneWithCommonStack (
  VOID
  );

typedef enum {
  CpuStateIdle,
  CpuStateBlocked,
  CpuStateReady,
  CpuStateBuzy,
  CpuStateFinished
} CPU_STATE;

/**
  Define Individual Processor Data block.

**/
typedef struct {
  EFI_PROCESSOR_INFORMATION      Info;
  SPIN_LOCK                      CpuDataLock;
  volatile CPU_STATE             State;

  EFI_AP_PROCEDURE               Procedure;
  VOID                           *Parameter;
} CPU_DATA_BLOCK;

/**
  Define MP data block which consumes individual processor block.

**/
typedef struct {
  CPU_DATA_BLOCK              *CpuDatas;
  UINTN                       NumberOfProcessors;
  UINTN                       NumberOfEnabledProcessors;
} MP_SYSTEM_DATA;

/**
  This function is called by all processors (both BSP and AP) once and collects MP related data.

  @param Bsp             TRUE if the CPU is BSP
  @param ProcessorNumber The specific processor number

  @retval EFI_SUCCESS    Data for the processor collected and filled in

**/
EFI_STATUS
FillInProcessorInformation (
  IN     BOOLEAN              Bsp,
  IN     UINTN                ProcessorNumber
  );

#endif // _CPU_MP_H_


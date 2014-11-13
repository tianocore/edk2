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

/**
  This service retrieves the number of logical processor in the platform
  and the number of those logical processors that are enabled on this boot.
  This service may only be called from the BSP.

  This function is used to retrieve the following information:
    - The number of logical processors that are present in the system.
    - The number of enabled logical processors in the system at the instant
      this call is made.

  Because MP Service Protocol provides services to enable and disable processors
  dynamically, the number of enabled logical processors may vary during the
  course of a boot session.

  If this service is called from an AP, then EFI_DEVICE_ERROR is returned.
  If NumberOfProcessors or NumberOfEnabledProcessors is NULL, then
  EFI_INVALID_PARAMETER is returned. Otherwise, the total number of processors
  is returned in NumberOfProcessors, the number of currently enabled processor
  is returned in NumberOfEnabledProcessors, and EFI_SUCCESS is returned.

  @param[in]  This                        A pointer to the EFI_MP_SERVICES_PROTOCOL
                                          instance.
  @param[out] NumberOfProcessors          Pointer to the total number of logical
                                          processors in the system, including the BSP
                                          and disabled APs.
  @param[out] NumberOfEnabledProcessors   Pointer to the number of enabled logical
                                          processors that exist in system, including
                                          the BSP.

  @retval EFI_SUCCESS             The number of logical processors and enabled
                                  logical processors was retrieved.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   NumberOfProcessors is NULL.
  @retval EFI_INVALID_PARAMETER   NumberOfEnabledProcessors is NULL.

**/
EFI_STATUS
EFIAPI
GetNumberOfProcessors (
  IN  EFI_MP_SERVICES_PROTOCOL  *This,
  OUT UINTN                     *NumberOfProcessors,
  OUT UINTN                     *NumberOfEnabledProcessors
  );

/**
  This return the handle number for the calling processor.  This service may be
  called from the BSP and APs.

  This service returns the processor handle number for the calling processor.
  The returned value is in the range from 0 to the total number of logical
  processors minus 1. The total number of logical processors can be retrieved
  with EFI_MP_SERVICES_PROTOCOL.GetNumberOfProcessors(). This service may be
  called from the BSP and APs. If ProcessorNumber is NULL, then EFI_INVALID_PARAMETER
  is returned. Otherwise, the current processors handle number is returned in
  ProcessorNumber, and EFI_SUCCESS is returned.

  @param[in]  This             A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param[out] ProcessorNumber  The handle number of AP that is to become the new
                               BSP. The range is from 0 to the total number of
                               logical processors minus 1. The total number of
                               logical processors can be retrieved by
                               EFI_MP_SERVICES_PROTOCOL.GetNumberOfProcessors().

  @retval EFI_SUCCESS             The current processor handle number was returned
                                  in ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.

**/
EFI_STATUS
EFIAPI
WhoAmI (
  IN EFI_MP_SERVICES_PROTOCOL  *This,
  OUT UINTN                    *ProcessorNumber
  );

#endif // _CPU_MP_H_


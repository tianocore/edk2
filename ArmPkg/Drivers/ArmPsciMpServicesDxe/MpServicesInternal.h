/** @file

Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.<BR>
Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MP_SERVICES_INTERNAL_H_
#define MP_SERVICES_INTERNAL_H_

#include <Protocol/Cpu.h>
#include <Protocol/MpService.h>

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>

#define AP_STACK_SIZE  0x1000

//
// Internal Data Structures
//

//
// AP state
//
// The state transitions for an AP when it processes a procedure are:
//  Idle ----> Ready ----> Busy ----> Finished ----> Idle
//       [BSP]       [BSP]      [AP]           [BSP]
//
typedef enum {
  CpuStateIdle,
  CpuStateReady,
  CpuStateBlocked,
  CpuStateBusy,
  CpuStateFinished,
  CpuStateDisabled
} CPU_STATE;

//
// Define Individual Processor Data block.
//
typedef struct {
  EFI_PROCESSOR_INFORMATION    Info;
  EFI_AP_PROCEDURE             Procedure;
  VOID                         *Parameter;
  CPU_STATE                    State;
  EFI_EVENT                    CheckThisAPEvent;
  EFI_EVENT                    WaitEvent;
  UINTN                        Timeout;
  UINTN                        TimeTaken;
  BOOLEAN                      TimeoutActive;
  BOOLEAN                      *SingleApFinished;
} CPU_AP_DATA;

//
// Define MP data block which consumes individual processor block.
//
typedef struct {
  UINTN               NumberOfProcessors;
  UINTN               NumberOfEnabledProcessors;
  EFI_EVENT           CheckAllAPsEvent;
  EFI_EVENT           AllWaitEvent;
  UINTN               FinishCount;
  UINTN               StartCount;
  EFI_AP_PROCEDURE    Procedure;
  VOID                *ProcedureArgument;
  BOOLEAN             SingleThread;
  UINTN               StartedNumber;
  CPU_AP_DATA         *CpuData;
  UINTN               *FailedList;
  UINTN               FailedListIndex;
  UINTN               AllTimeout;
  UINTN               AllTimeTaken;
  BOOLEAN             AllTimeoutActive;
} CPU_MP_DATA;

/** Secondary core entry point.

**/
VOID
ApEntryPoint (
  VOID
  );

/** C entry-point for the AP.
    This function gets called from the assembly function ApEntryPoint.
**/
VOID
ApProcedure (
  VOID
  );

/** Turns on the specified core using PSCI and executes the user-supplied
    function that's been configured via a previous call to SetApProcedure.

   @param ProcessorIndex The index of the core to turn on.

   @retval EFI_SUCCESS       The processor was successfully turned on.
   @retval EFI_DEVICE_ERROR  An error occurred turning the processor on.

**/
STATIC
EFI_STATUS
EFIAPI
DispatchCpu (
  IN UINTN  ProcessorIndex
  );

/** Returns whether the specified processor is the BSP.

   @param[in] ProcessorIndex The index the processor to check.

   @return TRUE if the processor is the BSP, FALSE otherwise.
**/
STATIC
BOOLEAN
IsProcessorBSP (
  UINTN  ProcessorIndex
  );

/** Returns whether the processor executing this function is the BSP.

   @return Whether the current processor is the BSP.
**/
STATIC
BOOLEAN
IsCurrentProcessorBSP (
  VOID
  );

/** Returns whether the specified processor is enabled.

   @param[in] ProcessorIndex The index of the processor to check.

   @return TRUE if the processor is enabled, FALSE otherwise.
**/
STATIC
BOOLEAN
IsProcessorEnabled (
  UINTN  ProcessorIndex
  );

/** Configures the processor context with the user-supplied procedure and
    argument.

   @param CpuData           The processor context.
   @param Procedure         The user-supplied procedure.
   @param ProcedureArgument The user-supplied procedure argument.

**/
STATIC
VOID
SetApProcedure (
  IN   CPU_AP_DATA       *CpuData,
  IN   EFI_AP_PROCEDURE  Procedure,
  IN   VOID              *ProcedureArgument
  );

/**
  Get the Application Processors state.

  @param[in]  CpuData    The pointer to CPU_AP_DATA of specified AP

  @return  The AP status
**/
CPU_STATE
GetApState (
  IN  CPU_AP_DATA  *CpuData
  );

/** Returns the index of the next processor that is blocked.

   @param[out] NextNumber The index of the next blocked processor.

   @retval EFI_SUCCESS   Successfully found the next blocked processor.
   @retval EFI_NOT_FOUND There are no blocked processors.

**/
STATIC
EFI_STATUS
GetNextBlockedNumber (
  OUT UINTN  *NextNumber
  );

/** Stalls the BSP for the minimum of gPollInterval and Timeout.

   @param[in]  Timeout    The time limit in microseconds remaining for
                          APs to return from Procedure.

   @retval     StallTime  Time of execution stall.
**/
STATIC
UINTN
CalculateAndStallInterval (
  IN UINTN  Timeout
  );

/** Sets up the state for the StartupAllAPs function.

   @param SingleThread Whether the APs will execute sequentially.

**/
STATIC
VOID
StartupAllAPsPrepareState (
  IN BOOLEAN  SingleThread
  );

/** Handles execution of StartupAllAPs when a WaitEvent has been specified.

  @param Procedure         The user-supplied procedure.
  @param ProcedureArgument The user-supplied procedure argument.
  @param WaitEvent         The wait event to be signaled when the work is
                           complete or a timeout has occurred.
  @param TimeoutInMicroseconds The timeout for the work to be completed. Zero
                               indicates an infinite timeout.
  @param SingleThread          Whether the APs will execute sequentially.
  @param FailedCpuList         User-supplied pointer for list of failed CPUs.

   @return EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
StartupAllAPsWithWaitEvent (
  IN EFI_AP_PROCEDURE  Procedure,
  IN VOID              *ProcedureArgument,
  IN EFI_EVENT         WaitEvent,
  IN UINTN             TimeoutInMicroseconds,
  IN BOOLEAN           SingleThread,
  IN UINTN             **FailedCpuList
  );

/** Handles execution of StartupAllAPs when no wait event has been specified.

   @param Procedure             The user-supplied procedure.
   @param ProcedureArgument     The user-supplied procedure argument.
   @param TimeoutInMicroseconds The timeout for the work to be completed. Zero
                                indicates an infinite timeout.
   @param SingleThread          Whether the APs will execute sequentially.
   @param FailedCpuList         User-supplied pointer for list of failed CPUs.

   @return EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
StartupAllAPsNoWaitEvent (
  IN EFI_AP_PROCEDURE  Procedure,
  IN VOID              *ProcedureArgument,
  IN UINTN             TimeoutInMicroseconds,
  IN BOOLEAN           SingleThread,
  IN UINTN             **FailedCpuList
  );

/** Adds the specified processor the list of failed processors.

   @param ProcessorIndex The processor index to add.
   @param ApState         Processor state.

**/
STATIC
VOID
AddProcessorToFailedList (
  UINTN      ProcessorIndex,
  CPU_STATE  ApState
  );

/** Handles the StartupAllAPs case where the timeout has occurred.

**/
STATIC
VOID
ProcessStartupAllAPsTimeout (
  VOID
  );

/**
  If a timeout is specified in StartupAllAps(), a timer is set, which invokes
  this procedure periodically to check whether all APs have finished.

  @param[in] Event   The WaitEvent the user supplied.
  @param[in] Context The event context.
**/
STATIC
VOID
EFIAPI
CheckAllAPsStatus (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

/** Invoked periodically via a timer to check the state of the processor.

   @param Event   The event supplied by the timer expiration.
   @param Context The processor context.

**/
STATIC
VOID
EFIAPI
CheckThisAPStatus (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

/**
  This function is called by all processors (both BSP and AP) once and collects
  MP related data.

  @param BSP            TRUE if the processor is the BSP.
  @param Mpidr          The MPIDR for the specified processor. This should be
                        the full MPIDR and not only the affinity bits.
  @param ProcessorIndex The index of the processor.

  @return EFI_SUCCESS if the data for the processor collected and filled in.

**/
STATIC
EFI_STATUS
FillInProcessorInformation (
  IN BOOLEAN  BSP,
  IN UINTN    Mpidr,
  IN UINTN    ProcessorIndex
  );

/**
  Event notification function called when the EFI_EVENT_GROUP_READY_TO_BOOT is
  signaled. After this point, non-blocking mode is no longer allowed.

  @param  Event     Event whose notification function is being invoked.
  @param  Context   The pointer to the notification function's context,
                    which is implementation-dependent.

**/
STATIC
VOID
EFIAPI
ReadyToBootSignaled (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

#endif /* MP_SERVICES_INTERNAL_H_ */

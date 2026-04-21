/** @file
  Common header file for MP Initialize Library.

  Copyright (c) 2016 - 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2020 - 2024, AMD Inc. All rights reserved.<BR>
  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>
  Copyright (C) 2026 Qualcomm Technologies, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <PiPei.h>
#include <Guid/RiscVSecHobData.h>
#include <Library/PeiServicesLib.h>

#include <Library/MpInitLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/CpuLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/TimerLib.h>
#include <Library/HobLib.h>
#include <Library/FdtLib.h>

#define CPU_INIT_MP_LIB_HOB_GUID \
  { \
    0x58eb6a19, 0x3699, 0x4c68, { 0xa8, 0x36, 0xda, 0xcd, 0x8e, 0xdc, 0xad, 0x4a } \
  }

//
// AP state
//
// The state transitions for an AP when it process a procedure are:
//  Idle ----> Ready ----> Busy ----> Finished ----> Idle
//       [BSP]       [AP]       [AP]           [BSP]
//
typedef enum {
  CpuStateIdle,
  CpuStateReady,
  CpuStateBusy,
  CpuStateFinished
} CPU_STATE;

typedef struct _CPU_AP_DATA CPU_AP_DATA;

//
// CPU MP Data
//
typedef struct {
  UINT32              CpuCount;
  UINT32              BspNumber;
  volatile UINT32     FinishedCount;
  UINT32              RunningCount;
  BOOLEAN             SingleThread;
  EFI_AP_PROCEDURE    Procedure;
  VOID                *ProcArguments;
  UINT64              ExpectedTime;
  UINT64              CurrentTime;
  UINT64              TotalTime;
  EFI_EVENT           WaitEvent;
  UINTN               **FailedCpuList;

  CPU_AP_DATA         *CpuData;
} CPU_MP_DATA;

//
// AP related data
//
struct _CPU_AP_DATA {
  UINT32                ProcessorNumber;     // The logical processor number of AP.
  UINT32                HartId;
  UINTN                 ScratchRegValue;     // unique per AP
  SPIN_LOCK             ApLock;
  volatile UINTN        ApStack;
  volatile UINTN        ApFunction;
  volatile UINTN        ApFunctionArgument;
  volatile CPU_STATE    State;
  BOOLEAN               Waiting;
  BOOLEAN               *Finished;
  UINT64                ExpectedTime;
  UINT64                CurrentTime;
  UINT64                TotalTime;
  EFI_EVENT             WaitEvent;

  CPU_MP_DATA           *CpuMpData;
};

extern EFI_GUID  mCpuInitMpLibHobGuid;

/**
  Get the pointer to CPU MP Data structure.

  @return  The pointer to CPU MP Data structure.
**/
CPU_MP_DATA *
EFIAPI
GetCpuMpData (
  VOID
  );

/**
  Save the pointer to CPU MP Data structure.

  @param[in] CpuMpData  The pointer to CPU MP Data structure will be saved.
**/
VOID
EFIAPI
SaveCpuMpData (
  IN CPU_MP_DATA  *CpuMpData
  );

/**
  This function will be called by BSP to wakeup AP.

  @param[in] CpuMpData          Pointer to CPU MP Data.
  @param[in] Broadcast          TRUE:  Send broadcast IPI to all APs.
                                FALSE: Send IPI to AP by ApicId.
  @param[in] ProcessorNumber    The handle number of specified processor.
  @param[in] Procedure          The function to be invoked by AP.
  @param[in] ProcedureArgument  The argument to be passed into AP function.
  @param[in] WakeUpDisabledAps  Whether need to wake up disabled APs in broadcast mode.
**/
VOID
EFIAPI
WakeUpAP (
  IN CPU_MP_DATA       *CpuMpData,
  IN BOOLEAN           Broadcast,
  IN UINTN             ProcessorNumber,
  IN EFI_AP_PROCEDURE  Procedure               OPTIONAL,
  IN VOID              *ProcedureArgument      OPTIONAL,
  IN BOOLEAN           WakeUpDisabledAps
  );

/**
  Initialize global data for MP support.

  @param[in] CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
EFIAPI
InitMpGlobalData (
  IN CPU_MP_DATA  *CpuMpData
  );

/**
  Worker function to execute a caller provided function on all enabled APs.

  @param[in]  Procedure               A pointer to the function to be run on
                                      enabled APs of the system.
  @param[in]  SingleThread            If TRUE, then all the enabled APs execute
                                      the function specified by Procedure one by
                                      one, in ascending order of processor handle
                                      number.  If FALSE, then all the enabled APs
                                      execute the function specified by Procedure
                                      simultaneously.
  @param[in]  WaitEvent               The event created by the caller with CreateEvent()
                                      service.
  @param[in]  TimeoutInMicroseconds   Indicates the time limit in microseconds for
                                      APs to return from Procedure, either for
                                      blocking or non-blocking mode.
  @param[in]  ProcedureArgument       The parameter passed into Procedure for
                                      all APs.
  @param[out] FailedCpuList           If all APs finish successfully, then its
                                      content is set to NULL. If not all APs
                                      finish before timeout expires, then its
                                      content is set to address of the buffer
                                      holding handle numbers of the failed APs.

  @retval EFI_SUCCESS             In blocking mode, all APs have finished before
                                  the timeout expired.
  @retval EFI_SUCCESS             In non-blocking mode, function has been dispatched
                                  to all enabled APs.
  @retval others                  Failed to Startup all APs.

**/
EFI_STATUS
EFIAPI
StartupAllCPUsWorker (
  IN  EFI_AP_PROCEDURE  Procedure,
  IN  BOOLEAN           SingleThread,
  IN  BOOLEAN           ExcludeBsp,
  IN  EFI_EVENT         WaitEvent               OPTIONAL,
  IN  UINTN             TimeoutInMicroseconds,
  IN  VOID              *ProcedureArgument      OPTIONAL,
  OUT UINTN             **FailedCpuList         OPTIONAL
  );

/**
  Worker function to let the caller get one enabled AP to execute a caller-provided
  function.

  @param[in]  Procedure               A pointer to the function to be run on
                                      enabled APs of the system.
  @param[in]  ProcessorNumber         The handle number of the AP.
  @param[in]  WaitEvent               The event created by the caller with CreateEvent()
                                      service.
  @param[in]  TimeoutInMicroseconds   Indicates the time limit in microseconds for
                                      APs to return from Procedure, either for
                                      blocking or non-blocking mode.
  @param[in]  ProcedureArgument       The parameter passed into Procedure for
                                      all APs.
  @param[out] Finished                If AP returns from Procedure before the
                                      timeout expires, its content is set to TRUE.
                                      Otherwise, the value is set to FALSE.

  @retval EFI_SUCCESS             In blocking mode, specified AP finished before
                                  the timeout expires.
  @retval others                  Failed to Startup AP.

**/
EFI_STATUS
EFIAPI
StartupThisAPWorker (
  IN  EFI_AP_PROCEDURE  Procedure,
  IN  UINTN             ProcessorNumber,
  IN  EFI_EVENT         WaitEvent               OPTIONAL,
  IN  UINTN             TimeoutInMicroseconds,
  IN  VOID              *ProcedureArgument      OPTIONAL,
  OUT BOOLEAN           *Finished               OPTIONAL
  );

/** Checks status of specified AP.

  This function checks whether the specified AP has finished the task assigned
  by StartupThisAP(), and whether timeout expires.

  @param[in]  CpuData         Pointer to CPU AP Data.

  @retval EFI_SUCCESS           Specified AP has finished task assigned by StartupThisAPs().
  @retval EFI_TIMEOUT           The timeout expires.
  @retval EFI_NOT_READY         Specified AP has not finished task and timeout has not expired.
**/
EFI_STATUS
EFIAPI
CheckThisAP (
  IN CPU_AP_DATA  *CpuData
  );

/**
  Checks status of all APs.

  This function checks whether all APs have finished task assigned by StartupAllAPs(),
  and whether timeout expires.

  @retval EFI_SUCCESS           All APs have finished task assigned by StartupAllAPs().
  @retval EFI_TIMEOUT           The timeout expires.
  @retval EFI_NOT_READY         APs have not finished task and timeout has not expired.
**/
EFI_STATUS
EFIAPI
CheckAllAPs (
  VOID
  );

/**
  Checks APs status and updates APs status if needed.

**/
VOID
EFIAPI
CheckAndUpdateApsStatus (
  VOID
  );

/**
  Enable Debug Agent to support source debugging on AP function.
  This instance will added in the future.

**/
VOID
EFIAPI
EnableDebugAgent (
  VOID
  );

/**
  Ap entry point function.

  Ap wake up and jump to this entry point.

  @param[in] StackBase  The base address of AP's stack.
**/
VOID
EFIAPI
ApEntryPoint (
  IN UINTN  StackBase
  );

/**
  Main Ap wake up function.

  Ap wake up and will perform the corresponding functions.

  @param[in, out] CpuData        Pointer to CPU AP Data
**/
VOID
EFIAPI
ApWakeupFunction (
  IN OUT CPU_AP_DATA  *CpuData
  );

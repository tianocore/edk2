/** @file
  Common header file for LoongArch MP Initialize Library.

  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MP_LIB_H_
#define MP_LIB_H_

#include <PiPei.h>
#include <Guid/ProcessorResourceHob.h>
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

#define WAKEUP_AP_SIGNAL  SIGNATURE_32 ('S', 'T', 'A', 'P')

#define CPU_INIT_MP_LIB_HOB_GUID \
  { \
    0x58eb6a19, 0x3699, 0x4c68, { 0xa8, 0x36, 0xda, 0xcd, 0x8e, 0xdc, 0xad, 0x4a } \
  }

//
// AP loop state when APs are in idle state
// It's value is the same with PcdCpuApLoopMode
//
typedef enum {
  ApInHltLoop = 1,
  ApInRunLoop = 2
} AP_LOOP_MODE;

//
// AP initialization state during APs wakeup
//
typedef enum {
  ApInitConfig   = 1,
  ApInitReconfig = 2,
  ApInitDone     = 3
} AP_INIT_STATE;

//
// AP state
//
typedef enum {
  CpuStateIdle,
  CpuStateReady,
  CpuStateBusy,
  CpuStateFinished,
  CpuStateDisabled
} CPU_STATE;

//
// AP related data
//
typedef struct {
  SPIN_LOCK             ApLock;
  volatile UINT32       *StartupApSignal;
  volatile UINTN        ApFunction;
  volatile UINTN        ApFunctionArgument;
  BOOLEAN               CpuHealthy;
  volatile CPU_STATE    State;
  BOOLEAN               Waiting;
  BOOLEAN               *Finished;
  UINT64                ExpectedTime;
  UINT64                CurrentTime;
  UINT64                TotalTime;
  EFI_EVENT             WaitEvent;
} CPU_AP_DATA;

//
// Basic CPU information saved in Guided HOB.
// Because the contents will be shard between PEI and DXE,
// we need to make sure the each fields offset same in different
// architecture.
//
#pragma pack (1)
typedef struct {
  UINT32    ApicId;
  UINT32    Health;
} CPU_INFO_IN_HOB;
#pragma pack ()

typedef struct MP_CPU_DATA CPU_MP_DATA;

#pragma pack(1)

//
// MP CPU exchange information for AP reset code
// This structure is required to be packed because fixed field offsets
// into this structure are used in assembly code in this module
//
typedef struct {
  CPU_MP_DATA    *CpuMpData;
} MP_CPU_EXCHANGE_INFO;

#pragma pack()

//
// CPU MP Data save in memory
//
struct MP_CPU_DATA {
  UINT64                           CpuInfoInHob;
  UINT32                           CpuCount;
  UINT32                           BspNumber;
  //
  // The above fields data will be passed from PEI to DXE
  // Please make sure the fields offset same in the different
  // architecture.
  //
  SPIN_LOCK                        MpLock;

  volatile UINT32                  FinishedCount;
  UINT32                           RunningCount;
  BOOLEAN                          SingleThread;
  EFI_AP_PROCEDURE                 Procedure;
  VOID                             *ProcArguments;
  BOOLEAN                          *Finished;
  UINT64                           ExpectedTime;
  UINT64                           CurrentTime;
  UINT64                           TotalTime;
  EFI_EVENT                        WaitEvent;

  AP_INIT_STATE                    InitFlag;
  UINT8                            ApLoopMode;
  CPU_AP_DATA                      *CpuData;
  volatile MP_CPU_EXCHANGE_INFO    *MpCpuExchangeInfo;
};

extern EFI_GUID  mCpuInitMpLibHobGuid;

/**
  Get the pointer to CPU MP Data structure.

  @return  The pointer to CPU MP Data structure.
**/
CPU_MP_DATA *
GetCpuMpData (
  VOID
  );

/**
  Save the pointer to CPU MP Data structure.

  @param[in] CpuMpData  The pointer to CPU MP Data structure will be saved.
**/
VOID
SaveCpuMpData (
  IN CPU_MP_DATA  *CpuMpData
  );

/**
  This function will be called by BSP to wakeup AP.

  @param[in] CpuMpData          Pointer to CPU MP Data
  @param[in] Broadcast          TRUE:  Send broadcast IPI to all APs
                                FALSE: Send IPI to AP by ApicId
  @param[in] ProcessorNumber    The handle number of specified processor
  @param[in] Procedure          The function to be invoked by AP
  @param[in] ProcedureArgument  The argument to be passed into AP function
  @param[in] WakeUpDisabledAps  Whether need to wake up disabled APs in broadcast mode.
**/
VOID
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
StartupThisAPWorker (
  IN  EFI_AP_PROCEDURE  Procedure,
  IN  UINTN             ProcessorNumber,
  IN  EFI_EVENT         WaitEvent               OPTIONAL,
  IN  UINTN             TimeoutInMicroseconds,
  IN  VOID              *ProcedureArgument      OPTIONAL,
  OUT BOOLEAN           *Finished               OPTIONAL
  );

/**
  Worker function to let the caller enable or disable an AP from this point onward.
  This service may only be called from the BSP.
  This instance will be added in the future.

  @param[in] ProcessorNumber   The handle number of AP.
  @param[in] EnableAP          Specifies the new state for the processor for
                               enabled, FALSE for disabled.
  @param[in] HealthFlag        If not NULL, a pointer to a value that specifies
                               the new health status of the AP.

  @retval EFI_SUCCESS          The specified AP was enabled or disabled successfully.
  @retval others               Failed to Enable/Disable AP.

**/
EFI_STATUS
EnableDisableApWorker (
  IN  UINTN    ProcessorNumber,
  IN  BOOLEAN  EnableAP,
  IN  UINT32   *HealthFlag OPTIONAL
  );

/**
  Get pointer to CPU MP Data structure from GUIDed HOB.

  @return  The pointer to CPU MP Data structure.
**/
CPU_MP_DATA *
GetCpuMpDataFromGuidedHob (
  VOID
  );

/** Checks status of specified AP.

  This function checks whether the specified AP has finished the task assigned
  by StartupThisAP(), and whether timeout expires.

  @param[in]  ProcessorNumber       The handle number of processor.

  @retval EFI_SUCCESS           Specified AP has finished task assigned by StartupThisAPs().
  @retval EFI_TIMEOUT           The timeout expires.
  @retval EFI_NOT_READY         Specified AP has not finished task and timeout has not expired.
**/
EFI_STATUS
CheckThisAP (
  IN UINTN  ProcessorNumber
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
CheckAllAPs (
  VOID
  );

/**
  Checks APs status and updates APs status if needed.

**/
VOID
CheckAndUpdateApsStatus (
  VOID
  );

/**
  Enable Debug Agent to support source debugging on AP function.
  This instance will added in the future.

**/
VOID
EnableDebugAgent (
  VOID
  );

#endif

/** @file
  CPU MP Initialize Library common functions.

  Copyright (c) 2016 - 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2020 - 2024, AMD Inc. All rights reserved.<BR>
  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>
  Copyright (C) 2026 Qualcomm Technologies, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseRiscVSbiLib.h>
#include "MpLib.h"

/**
  Get the Application Processors state.

  @param[in]  CpuData    The pointer to CPU_AP_DATA of specified AP.

  @return  The AP status
**/
STATIC
CPU_STATE
GetApState (
  IN  CPU_AP_DATA  *CpuData
  )
{
  return CpuData->State;
}

/**
  Set the Application Processors state.

  @param[in]   CpuData    The pointer to CPU_AP_DATA of specified AP.
  @param[in]   State      The AP status.
**/
STATIC
VOID
SetApState (
  IN  CPU_AP_DATA  *CpuData,
  IN  CPU_STATE    State
  )
{
  AcquireSpinLock (&CpuData->ApLock);
  CpuData->State = State;
  ReleaseSpinLock (&CpuData->ApLock);
}

/**
  Initialize CPU AP Data on each processor.

  @param[in, out] CpuData        Pointer to CPU AP Data.

**/
STATIC
VOID
InitializeApData (
  IN OUT CPU_AP_DATA  *CpuData
  )
{
  CPU_MP_DATA  *CpuMpData;

  CpuMpData = CpuData->CpuMpData;

  //
  // There are no ways to directly get HartId from current executing processor,
  // so save the HartId in ScratchRegValue and find the processor number by
  // matching the HartId later.
  //
  // The BSP's scratch register value is reserved for PeiServicesTablePointer,
  // so save the value (HartId + BSP's scratch register value + 1) in AP's ScratchRegValue
  // to make sure the value is unique in each processor.

  if (CpuData->ProcessorNumber == CpuMpData->BspNumber) {
    CpuData->ScratchRegValue = RiscVGetSupervisorScratch ();
  } else {
    CpuData->ScratchRegValue = CpuMpData->CpuData[CpuMpData->BspNumber].ScratchRegValue + CpuData->HartId + 1;
    RiscVSetSupervisorScratch (CpuData->ScratchRegValue);
  }
}

/**
  Calculate timeout value and return the current performance counter value.

  Calculate the number of performance counter ticks required for a timeout.
  If TimeoutInMicroseconds is 0, return value is also 0, which is recognized
  as infinity.

  @param[in]  TimeoutInMicroseconds   Timeout value in microseconds.
  @param[out] CurrentTime             Returns the current value of the performance counter.

  @return Expected time stamp counter for timeout.
          If TimeoutInMicroseconds is 0, return value is also 0, which is recognized
          as infinity.

**/
STATIC
UINT64
CalculateTimeout (
  IN  UINTN   TimeoutInMicroseconds,
  OUT UINT64  *CurrentTime
  )
{
  UINT64  TimeoutInSeconds;
  UINT64  TimestampCounterFreq;

  //
  // Read the current value of the performance counter
  //
  *CurrentTime = GetPerformanceCounter ();

  //
  // If TimeoutInMicroseconds is 0, return value is also 0, which is recognized
  // as infinity.
  //
  if (TimeoutInMicroseconds == 0) {
    return 0;
  }

  //
  // GetPerformanceCounterProperties () returns the timestamp counter's frequency
  // in Hz.
  //
  TimestampCounterFreq = GetPerformanceCounterProperties (NULL, NULL);

  //
  // Check the potential overflow before calculate the number of ticks for the timeout value.
  //
  if (DivU64x64Remainder (MAX_UINT64, TimeoutInMicroseconds, NULL) < TimestampCounterFreq) {
    //
    // Convert microseconds into seconds if direct multiplication overflows
    //
    TimeoutInSeconds = DivU64x32 (TimeoutInMicroseconds, 1000000);
    //
    // Assertion if the final tick count exceeds MAX_UINT64
    //
    ASSERT (DivU64x64Remainder (MAX_UINT64, TimeoutInSeconds, NULL) >= TimestampCounterFreq);
    return MultU64x64 (TimestampCounterFreq, TimeoutInSeconds);
  } else {
    //
    // No overflow case, multiply the return value with TimeoutInMicroseconds and then divide
    // it by 1,000,000, to get the number of ticks for the timeout value.
    //
    return DivU64x32 (
             MultU64x64 (
               TimestampCounterFreq,
               TimeoutInMicroseconds
               ),
             1000000
             );
  }
}

/**
  Checks whether timeout expires.

  Check whether the number of elapsed performance counter ticks required for
  a timeout condition has been reached.
  If Timeout is zero, which means infinity, return value is always FALSE.

  @param[in, out]  PreviousTime   On input,  the value of the performance counter
                                  when it was last read.
                                  On output, the current value of the performance
                                  counter
  @param[in]       TotalTime      The total amount of elapsed time in performance
                                  counter ticks.
  @param[in]       Timeout        The number of performance counter ticks required
                                  to reach a timeout condition.

  @retval TRUE                    A timeout condition has been reached.
  @retval FALSE                   A timeout condition has not been reached.

**/
STATIC
BOOLEAN
CheckTimeout (
  IN OUT UINT64  *PreviousTime,
  IN     UINT64  *TotalTime,
  IN     UINT64  Timeout
  )
{
  UINT64  Start;
  UINT64  End;
  UINT64  CurrentTime;
  INT64   Delta;
  INT64   Cycle;

  if (Timeout == 0) {
    return FALSE;
  }

  GetPerformanceCounterProperties (&Start, &End);
  Cycle = End - Start;
  if (Cycle < 0) {
    Cycle = -Cycle;
  }

  Cycle++;
  CurrentTime = GetPerformanceCounter ();
  Delta       = (INT64)(CurrentTime - *PreviousTime);
  if (Start > End) {
    Delta = -Delta;
  }

  if (Delta < 0) {
    Delta += Cycle;
  }

  *TotalTime   += Delta;
  *PreviousTime = CurrentTime;
  if (*TotalTime > Timeout) {
    return TRUE;
  }

  return FALSE;
}

/**
  Helper function that waits until the finished AP count reaches the specified
  limit, or the specified timeout elapses (whichever comes first).

  @param[in] CpuMpData        Pointer to CPU MP Data.
  @param[in] FinishedApLimit  The number of finished APs to wait for.
  @param[in] TimeLimit        The number of microseconds to wait for.
**/
STATIC
VOID
TimedWaitForApFinish (
  IN CPU_MP_DATA  *CpuMpData,
  IN UINT32       FinishedApLimit,
  IN UINT32       TimeLimit
  )
{
  //
  // CalculateTimeout() and CheckTimeout() consider a TimeLimit of 0
  // "infinity", so check for (TimeLimit == 0) explicitly.
  //
  if (TimeLimit == 0) {
    return;
  }

  CpuMpData->TotalTime    = 0;
  CpuMpData->ExpectedTime = CalculateTimeout (
                              TimeLimit,
                              &CpuMpData->CurrentTime
                              );
  while (CpuMpData->FinishedCount < FinishedApLimit &&
         !CheckTimeout (
            &CpuMpData->CurrentTime,
            &CpuMpData->TotalTime,
            CpuMpData->ExpectedTime
            ))
  {
    CpuPause ();
  }

  if (CpuMpData->FinishedCount >= FinishedApLimit) {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: reached FinishedApLimit=%u in %Lu microseconds\n",
      __func__,
      FinishedApLimit,
      DivU64x64Remainder (
        MultU64x32 (CpuMpData->TotalTime, 1000000),
        GetPerformanceCounterProperties (NULL, NULL),
        NULL
        )
      ));
  }
}

/**
  This function find all CPU nodes in FDT and fill the HartId for each AP in CpuMpData.

  @param[in, out] CpuMpData        Pointer to CPU MP Data.
**/
STATIC
VOID
FindAllCpuNodesInFdt (
  IN OUT CPU_MP_DATA  *CpuMpData
  )
{
  VOID                    *Hob;
  RISCV_SEC_HANDOFF_DATA  *SecData;
  CONST VOID              *FdtBase;
  UINTN                   Index;
  INT32                   Node;
  CONST VOID              *Prop;
  INT32                   Len;
  CONST EFI_GUID          SecHobDataGuid = RISCV_SEC_HANDOFF_HOB_GUID;

  Hob = GetFirstGuidHob (&gFdtHobGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64))) {
    DEBUG ((DEBUG_ERROR, "MpInitLib: Failed to retrieve FDT HOB.\n"));
    return;
  }

  FdtBase = (VOID *)(UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);

  Hob = GetFirstGuidHob (&SecHobDataGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (RISCV_SEC_HANDOFF_DATA))) {
    DEBUG ((DEBUG_ERROR, "MpInitLib: Failed to retrieve SEC_HANDOFF_DATA HOB.\n"));
    return;
  }

  SecData = (RISCV_SEC_HANDOFF_DATA *)GET_GUID_HOB_DATA (Hob);

  CpuMpData->CpuCount = 1;

  Node = FdtPathOffset (FdtBase, "/cpus");
  if (Node < 0) {
    DEBUG ((DEBUG_ERROR, "MpInitLib: Failed to find /cpus node in FDT.\n"));
    return;
  }

  CpuMpData->CpuData[CpuMpData->BspNumber].HartId = SecData->BootHartId;
  Node                                            = FdtFirstSubnode (FdtBase, Node);
  Index                                           = 0;
  while (Node >= 0) {
    Prop = FdtGetProp (FdtBase, Node, "status", &Len);
    if ((Prop != NULL) && (Len == 5) && (Fdt32ToCpu (*(UINT32 *)Prop) == 0x6F6B6179)) {
      // "okay"
      Prop = FdtGetProp (FdtBase, Node, "reg", &Len);
      if ((Prop != NULL) && (Len == sizeof (UINT32))) {
        if (Index == CpuMpData->BspNumber) {
          Index++;
        }

        if (Fdt32ToCpu (*(UINT32 *)Prop) != SecData->BootHartId) {
          CpuMpData->CpuData[Index++].HartId = Fdt32ToCpu (*(UINT32 *)Prop);
          CpuMpData->CpuCount++;
        }
      }
    }

    Node = FdtNextSubnode (FdtBase, Node);
  }
}

/**
  This function will initialize all Application Processors in the system.

  @param[in, out] CpuMpData        Pointer to CPU MP Data

  @return  EFI_SUCCESS          All APs are initialized successfully.
  @return  EFI_TIMEOUT          Timeout while waiting for APs to finish initialization.
**/
STATIC
EFI_STATUS
InitializeAllAps (
  IN OUT CPU_MP_DATA  *CpuMpData
  )
{
  UINTN  Index;

  FindAllCpuNodesInFdt (CpuMpData);

  //
  // Send 1st broadcast to wakeup APs
  //
  WakeUpAP (CpuMpData, TRUE, 0, NULL, NULL, FALSE);

  //
  // Wait for all APs finished the initialization
  //
  TimedWaitForApFinish (
    CpuMpData,
    CpuMpData->CpuCount - 1,
    PcdGet32 (PcdCpuApInitTimeOutInMicroSeconds)
    );

  //
  // Check and reset idle state for each AP.
  //
  for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
    if (Index != CpuMpData->BspNumber) {
      CheckThisAP (&CpuMpData->CpuData[Index]);
    }
  }

  if (CpuMpData->FinishedCount != CpuMpData->CpuCount - 1) {
    DEBUG ((DEBUG_ERROR, "MpInitLib: Only %d APs are initialized successfully.\n", CpuMpData->FinishedCount));
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Main Ap wake up function.

  Ap wake up and will perform the corresponding function.

  @param[in, out] CpuData        Pointer to CPU AP Data
**/
VOID
EFIAPI
ApWakeupFunction (
  IN OUT CPU_AP_DATA  *CpuData
  )
{
  EFI_AP_PROCEDURE  Procedure;
  VOID              *Parameter;
  CPU_MP_DATA       *CpuMpData;

  CpuMpData = CpuData->CpuMpData;

  InitializeApData (CpuData);

  //
  // Invoke AP function here
  //
  Procedure = (EFI_AP_PROCEDURE)CpuData->ApFunction;
  Parameter = (VOID *)CpuData->ApFunctionArgument;
  if (Procedure != NULL) {
    SetApState (CpuData, CpuStateBusy);
    //
    // Enable source debugging on AP function
    //
    EnableDebugAgent ();
    Procedure (Parameter);
  }

  SetApState (CpuData, CpuStateFinished);

  //
  // Updates the finished count.
  //
  InterlockedIncrement ((UINT32 *)&CpuMpData->FinishedCount);

  //
  // Put this AP into stop state to wait for next wakeup.
  //
  SbiCall (SBI_EXT_HSM, SBI_HSM_HART_STOP, 0);

  //
  // Should not reach here.
  //
  ASSERT (FALSE);
}

/**
  This function will be called by BSP to wakeup AP.

  @param[in] CpuMpData          Pointer to CPU MP Data
  @param[in] Broadcast          TRUE:  Wake up all APs
                                FALSE: Wake up an AP by ProcessorNumber
  @param[in] ProcessorNumber    The handle number of specified processor
  @param[in] Procedure          The function to be invoked by AP
  @param[in] ProcedureArgument  The argument to be passed into AP function
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
  )
{
  UINTN        Index;
  CPU_AP_DATA  *CpuData;

  CpuMpData->FinishedCount = 0;

  if (Broadcast) {
    for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
      if (Index != CpuMpData->BspNumber) {
        CpuData                     = &CpuMpData->CpuData[Index];
        CpuData->ApFunction         = (UINTN)Procedure;
        CpuData->ApFunctionArgument = (UINTN)ProcedureArgument;
        SetApState (CpuData, CpuStateReady);
        //
        // Wake up AP
        //
        MemoryFence ();
        SbiCall (SBI_EXT_HSM, SBI_HSM_HART_START, 3, CpuData->HartId, (UINTN)ApEntryPoint, CpuData->ApStack);
      }
    }
  } else {
    CpuData                     = &CpuMpData->CpuData[ProcessorNumber];
    CpuData->ApFunction         = (UINTN)Procedure;
    CpuData->ApFunctionArgument = (UINTN)ProcedureArgument;
    SetApState (CpuData, CpuStateReady);
    //
    // Wake up AP
    //
    MemoryFence ();
    SbiCall (SBI_EXT_HSM, SBI_HSM_HART_START, 3, CpuData->HartId, (UINTN)ApEntryPoint, CpuData->ApStack);
  }
}

/**
  Searches for the next waiting AP.

  Search for the next AP that is put in waiting state by single-threaded StartupAllAPs().

  @param[out]  NextProcessorNumber  Pointer to the processor number of the next waiting AP.

  @retval EFI_SUCCESS          The next waiting AP has been found.
  @retval EFI_NOT_FOUND        No waiting AP exists.

**/
STATIC
EFI_STATUS
GetNextWaitingProcessorNumber (
  OUT UINTN  *NextProcessorNumber
  )
{
  UINTN        ProcessorNumber;
  CPU_MP_DATA  *CpuMpData;

  CpuMpData = GetCpuMpData ();

  if (CpuMpData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get CpuMpData.\n", __func__));
    return EFI_LOAD_ERROR;
  }

  for (ProcessorNumber = 0; ProcessorNumber < CpuMpData->CpuCount; ProcessorNumber++) {
    if (CpuMpData->CpuData[ProcessorNumber].Waiting) {
      *NextProcessorNumber = ProcessorNumber;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

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
  )
{
  SBI_RET  SbiRet;

  //
  //  Check the CPU state of AP. If it is CpuStateIdle, then the AP has finished its task.
  //  Only BSP and corresponding AP access this unit of CPU Data. This means the AP will not modify the
  //  value of state after setting the it to CpuStateIdle, so BSP can safely make use of its value.
  //
  //
  // If the AP finishes for StartupThisAP(), return EFI_SUCCESS.
  //
  if (GetApState (CpuData) == CpuStateFinished) {
    if (CpuData->Finished != NULL) {
      *(CpuData->Finished) = TRUE;
    }

    //
    // Make sure AP in HSM stop state after finishing task, and set AP state to idle to wait for next wakeup.
    //
    do {
      SbiRet = SbiCall (SBI_EXT_HSM, SBI_HSM_HART_STATUS, 1, CpuData->HartId);
      CpuPause ();
    } while (SbiRet.Value != SBI_HSM_STATE_STOPPED);

    SetApState (CpuData, CpuStateIdle);
    return EFI_SUCCESS;
  } else {
    //
    // If timeout expires for StartupThisAP(), report timeout.
    //
    if (CheckTimeout (&CpuData->CurrentTime, &CpuData->TotalTime, CpuData->ExpectedTime)) {
      if (CpuData->Finished != NULL) {
        *(CpuData->Finished) = FALSE;
      }

      return EFI_TIMEOUT;
    }
  }

  return EFI_NOT_READY;
}

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
  )
{
  UINTN        ProcessorNumber;
  UINTN        NextProcessorNumber;
  UINTN        ListIndex;
  EFI_STATUS   Status;
  CPU_MP_DATA  *CpuMpData;
  CPU_AP_DATA  *CpuData;

  CpuMpData = GetCpuMpData ();

  if (CpuMpData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get CpuMpData.\n", __func__));
    return EFI_LOAD_ERROR;
  }

  NextProcessorNumber = 0;

  //
  // Go through all APs that are responsible for the StartupAllAPs().
  //
  for (ProcessorNumber = 0; ProcessorNumber < CpuMpData->CpuCount; ProcessorNumber++) {
    if (!CpuMpData->CpuData[ProcessorNumber].Waiting) {
      continue;
    }

    CpuData = &CpuMpData->CpuData[ProcessorNumber];
    //
    // Check the CPU state of AP. If it is CpuStateIdle, then the AP has finished its task.
    // Only BSP and corresponding AP access this unit of CPU Data. This means the AP will not modify the
    // value of state after setting the it to CpuStateIdle, so BSP can safely make use of its value.
    //
    if (GetApState (CpuData) == CpuStateFinished) {
      CpuMpData->RunningCount--;
      CpuMpData->CpuData[ProcessorNumber].Waiting = FALSE;
      SetApState (CpuData, CpuStateIdle);

      //
      // If in Single Thread mode, then search for the next waiting AP for execution.
      //
      if (CpuMpData->SingleThread) {
        Status = GetNextWaitingProcessorNumber (&NextProcessorNumber);

        if (!EFI_ERROR (Status)) {
          WakeUpAP (
            CpuMpData,
            FALSE,
            (UINT32)NextProcessorNumber,
            CpuMpData->Procedure,
            CpuMpData->ProcArguments,
            TRUE
            );
        }
      }
    }
  }

  //
  // If all APs finish, return EFI_SUCCESS.
  //
  if (CpuMpData->RunningCount == 0) {
    return EFI_SUCCESS;
  }

  //
  // If timeout expires, report timeout.
  //
  if (CheckTimeout (
        &CpuMpData->CurrentTime,
        &CpuMpData->TotalTime,
        CpuMpData->ExpectedTime
        )
      )
  {
    //
    // If FailedCpuList is not NULL, record all failed APs in it.
    //
    if (CpuMpData->FailedCpuList != NULL) {
      *CpuMpData->FailedCpuList =
        AllocatePool ((CpuMpData->RunningCount + 1) * sizeof (UINTN));
      ASSERT (*CpuMpData->FailedCpuList != NULL);
    }

    ListIndex = 0;

    for (ProcessorNumber = 0; ProcessorNumber < CpuMpData->CpuCount; ProcessorNumber++) {
      //
      // Check whether this processor is responsible for StartupAllAPs().
      //
      if (CpuMpData->CpuData[ProcessorNumber].Waiting) {
        if (CpuMpData->FailedCpuList != NULL) {
          (*CpuMpData->FailedCpuList)[ListIndex++] = ProcessorNumber;
        }
      }
    }

    if (CpuMpData->FailedCpuList != NULL) {
      (*CpuMpData->FailedCpuList)[ListIndex] = END_OF_CPU_LIST;
    }

    return EFI_TIMEOUT;
  }

  return EFI_NOT_READY;
}

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
  @param[in]  ExcludeBsp              Whether let BSP also trig this task.
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
  )
{
  EFI_STATUS   Status;
  CPU_MP_DATA  *CpuMpData;
  UINTN        ProcessorCount;
  UINTN        ProcessorNumber;
  UINTN        CallerNumber;
  CPU_AP_DATA  *CpuData;
  CPU_STATE    ApState;

  CpuMpData = GetCpuMpData ();

  if (FailedCpuList != NULL) {
    *FailedCpuList = NULL;
  }

  if (CpuMpData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get CpuMpData.\n", __func__));
    return EFI_LOAD_ERROR;
  }

  if ((CpuMpData->CpuCount == 1) && ExcludeBsp) {
    return EFI_NOT_STARTED;
  }

  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether caller processor is BSP
  //
  MpInitLibWhoAmI (&CallerNumber);
  if (CallerNumber != CpuMpData->BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Update AP state
  //
  CheckAndUpdateApsStatus ();

  ProcessorCount = CpuMpData->CpuCount;

  //
  // Check whether all enabled APs are idle.
  // If any enabled AP is not idle, return EFI_NOT_READY.
  //
  for (ProcessorNumber = 0; ProcessorNumber < ProcessorCount; ProcessorNumber++) {
    CpuData = &CpuMpData->CpuData[ProcessorNumber];
    if (ProcessorNumber != CpuMpData->BspNumber) {
      ApState = GetApState (CpuData);
      if (ApState != CpuStateIdle) {
        //
        // If any enabled APs are busy, return EFI_NOT_READY.
        //
        return EFI_NOT_READY;
      }
    }
  }

  CpuMpData->RunningCount = 0;
  for (ProcessorNumber = 0; ProcessorNumber < ProcessorCount; ProcessorNumber++) {
    CpuData          = &CpuMpData->CpuData[ProcessorNumber];
    CpuData->Waiting = FALSE;
    if (ProcessorNumber != CpuMpData->BspNumber) {
      //
      // Mark this processor as responsible for current calling.
      //
      CpuData->Waiting = TRUE;
      CpuMpData->RunningCount++;
    }
  }

  CpuMpData->Procedure     = Procedure;
  CpuMpData->ProcArguments = ProcedureArgument;
  CpuMpData->SingleThread  = SingleThread;
  CpuMpData->FinishedCount = 0;
  CpuMpData->FailedCpuList = FailedCpuList;
  CpuMpData->ExpectedTime  = CalculateTimeout (
                               TimeoutInMicroseconds,
                               &CpuMpData->CurrentTime
                               );
  CpuMpData->TotalTime = 0;
  CpuMpData->WaitEvent = WaitEvent;

  if (!SingleThread) {
    WakeUpAP (CpuMpData, TRUE, 0, Procedure, ProcedureArgument, FALSE);
  } else {
    for (ProcessorNumber = 0; ProcessorNumber < ProcessorCount; ProcessorNumber++) {
      if ((ProcessorNumber != CpuMpData->BspNumber) && CpuMpData->CpuData[ProcessorNumber].Waiting) {
        WakeUpAP (CpuMpData, FALSE, ProcessorNumber, Procedure, ProcedureArgument, TRUE);
        break;
      }
    }
  }

  if (!ExcludeBsp) {
    //
    // Start BSP.
    //
    Procedure (ProcedureArgument);
  }

  Status = EFI_SUCCESS;
  if (WaitEvent == NULL) {
    do {
      Status = CheckAllAPs ();
    } while (Status == EFI_NOT_READY);
  }

  return Status;
}

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
  )
{
  EFI_STATUS   Status;
  CPU_MP_DATA  *CpuMpData;
  CPU_AP_DATA  *CpuData;
  UINTN        CallerNumber;

  CpuMpData = GetCpuMpData ();

  if (Finished != NULL) {
    *Finished = FALSE;
  }

  //
  // Check whether caller processor is BSP
  //
  MpInitLibWhoAmI (&CallerNumber);
  if (CallerNumber != CpuMpData->BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  if (CpuMpData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get CpuMpData.\n", __func__));
    return EFI_LOAD_ERROR;
  }

  //
  // Check whether processor with the handle specified by ProcessorNumber exists
  //
  if (ProcessorNumber >= CpuMpData->CpuCount) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether specified processor is BSP
  //
  if (ProcessorNumber == CpuMpData->BspNumber) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check parameter Procedure
  //
  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Update AP state
  //
  CheckAndUpdateApsStatus ();

  //
  // If WaitEvent is not NULL, execute in non-blocking mode.
  // BSP saves data for CheckAPsStatus(), and returns EFI_SUCCESS.
  // CheckAPsStatus() will check completion and timeout periodically.
  //
  CpuData               = &CpuMpData->CpuData[ProcessorNumber];
  CpuData->WaitEvent    = WaitEvent;
  CpuData->Finished     = Finished;
  CpuData->ExpectedTime = CalculateTimeout (TimeoutInMicroseconds, &CpuData->CurrentTime);
  CpuData->TotalTime    = 0;

  WakeUpAP (CpuMpData, FALSE, ProcessorNumber, Procedure, ProcedureArgument, TRUE);

  //
  // If WaitEvent is NULL, execute in blocking mode.
  // BSP checks AP's state until it finishes or TimeoutInMicrosecsond expires.
  //
  Status = EFI_SUCCESS;
  if (WaitEvent == NULL) {
    do {
      Status = CheckThisAP (CpuData);
    } while (Status == EFI_NOT_READY);
  }

  return Status;
}

/**
  This service executes a caller provided function on all enabled CPUs.

  @param[in]  Procedure               A pointer to the function to be run on
                                      enabled APs of the system. See type
                                      EFI_AP_PROCEDURE.
  @param[in]  TimeoutInMicroseconds   Indicates the time limit in microseconds for
                                      APs to return from Procedure, either for
                                      blocking or non-blocking mode. Zero means
                                      infinity. TimeoutInMicroseconds is ignored
                                      for BSP.
  @param[in]  ProcedureArgument       The parameter passed into Procedure for
                                      all APs.

  @retval EFI_SUCCESS             In blocking mode, all CPUs have finished before
                                  the timeout expired.
  @retval EFI_SUCCESS             In non-blocking mode, function has been dispatched
                                  to all enabled CPUs.
  @retval EFI_DEVICE_ERROR        Caller processor is AP.
  @retval EFI_NOT_READY           Any enabled APs are busy.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before
                                  all enabled APs have finished.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.

**/
EFI_STATUS
EFIAPI
MpInitLibStartupAllCPUs (
  IN  EFI_AP_PROCEDURE  Procedure,
  IN  UINTN             TimeoutInMicroseconds,
  IN  VOID              *ProcedureArgument      OPTIONAL
  )
{
  return StartupAllCPUsWorker (
           Procedure,
           FALSE,
           FALSE,
           NULL,
           TimeoutInMicroseconds,
           ProcedureArgument,
           NULL
           );
}

/**
  Gets detailed MP-related information on the requested processor at the
  instant this call is made. This service may only be called from the BSP.

  @param[in]  ProcessorNumber       The handle number of processor.
                                    Lower 24 bits contains the actual processor number.
                                    BIT24 indicates if the EXTENDED_PROCESSOR_INFORMATION will be retrieved.
  @param[out] ProcessorInfoBuffer   A pointer to the buffer where information for
                                    the requested processor is deposited.
  @param[out] HealthData            Return processor health data.

  @retval EFI_SUCCESS             Processor information was returned.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   ProcessorInfoBuffer is NULL.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist in the platform.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
EFIAPI
MpInitLibGetProcessorInfo (
  IN  UINTN                      ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer,
  OUT EFI_HEALTH_FLAGS           *HealthData  OPTIONAL
  )
{
  CPU_MP_DATA  *CpuMpData;
  UINTN        CallerNumber;

  CpuMpData = GetCpuMpData ();

  //
  // Check whether caller processor is BSP
  //
  MpInitLibWhoAmI (&CallerNumber);
  if (CallerNumber != CpuMpData->BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  if (ProcessorInfoBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ProcessorNumber >= CpuMpData->CpuCount) {
    return EFI_NOT_FOUND;
  }

  ProcessorInfoBuffer->ProcessorId = (UINT64)CpuMpData->CpuData[ProcessorNumber].HartId;
  ProcessorInfoBuffer->StatusFlag  = 0;
  if (ProcessorNumber == CpuMpData->BspNumber) {
    ProcessorInfoBuffer->StatusFlag |= PROCESSOR_AS_BSP_BIT;
  }

  //
  // All processors are healthy in current implementation,
  // so set PROCESSOR_HEALTH_STATUS_BIT for all processors.
  // If there is any platform specific way to determine the health status
  // of processor, the bit can be set or cleared accordingly.
  //
  ProcessorInfoBuffer->StatusFlag |= PROCESSOR_HEALTH_STATUS_BIT | PROCESSOR_ENABLED_BIT;

  return EFI_SUCCESS;
}

/**
  This return the handle number for the calling processor.  This service may be
  called from the BSP and APs.

  @param[out] ProcessorNumber  Pointer to the handle number of AP.
                               The range is from 0 to the total number of
                               logical processors minus 1. The total number of
                               logical processors can be retrieved by
                               MpInitLibGetNumberOfProcessors().

  @retval EFI_SUCCESS             The current processor handle number was returned
                                  in ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist in the platform.

**/
EFI_STATUS
EFIAPI
MpInitLibWhoAmI (
  OUT UINTN  *ProcessorNumber
  )
{
  CPU_MP_DATA  *CpuMpData;
  UINTN        Index;

  if (ProcessorNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CpuMpData = GetCpuMpData ();
  if (CpuMpData == NULL) {
    return EFI_NOT_READY;
  }

  for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
    if (CpuMpData->CpuData[Index].ScratchRegValue == RiscVGetSupervisorScratch ()) {
      *ProcessorNumber = CpuMpData->CpuData[Index].ProcessorNumber;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Retrieves the number of logical processor in the platform and the number of
  those logical processors that are enabled on this boot. This service may only
  be called from the BSP.

  @param[out] NumberOfProcessors          Pointer to the total number of logical
                                          processors in the system, including the BSP
                                          and disabled APs.
  @param[out] NumberOfEnabledProcessors   Pointer to the number of enabled logical
                                          processors that exist in system, including
                                          the BSP.

  @retval EFI_SUCCESS             The number of logical processors and enabled
                                  logical processors was retrieved.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   NumberOfProcessors is NULL and NumberOfEnabledProcessors
                                  is NULL.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
EFIAPI
MpInitLibGetNumberOfProcessors (
  OUT UINTN  *NumberOfProcessors        OPTIONAL,
  OUT UINTN  *NumberOfEnabledProcessors OPTIONAL
  )
{
  CPU_MP_DATA  *CpuMpData;
  UINTN        CallerNumber;
  UINTN        ProcessorNumber;
  UINTN        EnabledProcessorNumber;

  CpuMpData = GetCpuMpData ();

  if ((NumberOfProcessors == NULL) && (NumberOfEnabledProcessors == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether caller processor is BSP
  //
  MpInitLibWhoAmI (&CallerNumber);
  if (CallerNumber != CpuMpData->BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  ProcessorNumber = CpuMpData->CpuCount;
  //
  // All APs are enabled in current implementation.
  //
  EnabledProcessorNumber = ProcessorNumber;

  if (NumberOfProcessors != NULL) {
    *NumberOfProcessors = ProcessorNumber;
  }

  if (NumberOfEnabledProcessors != NULL) {
    *NumberOfEnabledProcessors = EnabledProcessorNumber;
  }

  return EFI_SUCCESS;
}

/**
  MP Initialize Library initialization.

  This service must be invoked before all other MP Initialize Library
  service are invoked.

  @retval  EFI_SUCCESS           MP initialization succeeds.
  @retval  Others                MP initialization fails.

**/
EFI_STATUS
EFIAPI
MpInitLibInitialize (
  VOID
  )
{
  UINT32       MaxLogicalProcessorNumber;
  UINTN        BufferSize;
  UINT32       ApStackSize;
  VOID         *MpBuffer;
  CPU_MP_DATA  *CpuMpData;
  UINTN        CpuMpDataOffset;
  UINTN        Index;

  MaxLogicalProcessorNumber = PcdGet32 (PcdCpuMaxLogicalProcessorNumber);
  ASSERT (MaxLogicalProcessorNumber != 0);

  ApStackSize = PcdGet32 (PcdCpuApStackSize);
  //
  // ApStackSize must be power of 2
  //
  ASSERT ((ApStackSize & (ApStackSize - 1)) == 0);

  //
  //  The layout of the Buffer is as below (lower address on top):
  //  Pointer of CpuData is stored in the top of each AP's stack.
  //
  //    +--------------------+ <-- Buffer
  //        AP Stacks (N)
  //    +--------------------+ <-- CpuMpData
  //         CPU_MP_DATA
  //    +--------------------+ <-- CpuMpData->CpuData
  //        CPU_AP_DATA (N)
  //    +--------------------+
  //
  BufferSize      = ApStackSize * MaxLogicalProcessorNumber;
  CpuMpDataOffset = BufferSize;
  BufferSize     += sizeof (CPU_MP_DATA);
  BufferSize     += sizeof (CPU_AP_DATA) * MaxLogicalProcessorNumber;
  MpBuffer        = AllocatePages (EFI_SIZE_TO_PAGES (BufferSize));
  if (MpBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Not enough memory.\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (MpBuffer, BufferSize);

  CpuMpData                                                = (CPU_MP_DATA *)((UINTN)MpBuffer + CpuMpDataOffset);
  CpuMpData->BspNumber                                     = 0;
  CpuMpData->CpuData                                       = (CPU_AP_DATA *)(CpuMpData + 1);
  CpuMpData->CpuData[CpuMpData->BspNumber].ProcessorNumber = CpuMpData->BspNumber;

  //
  // Set up APs data.
  //
  for (Index = 0; Index < MaxLogicalProcessorNumber; Index++) {
    InitializeSpinLock (&CpuMpData->CpuData[Index].ApLock);
    CpuMpData->CpuData[Index].CpuMpData = CpuMpData;
    if (Index != CpuMpData->BspNumber) {
      CpuMpData->CpuData[Index].ProcessorNumber                      = Index;
      CpuMpData->CpuData[Index].ApStack                              = (UINTN)MpBuffer + (Index * ApStackSize) + ApStackSize;
      *(UINTN *)(CpuMpData->CpuData[Index].ApStack - sizeof (UINTN)) = (UINTN)&CpuMpData->CpuData[Index];
    }
  }

  //
  // Set BSP basic information.
  //
  InitializeApData (&CpuMpData->CpuData[CpuMpData->BspNumber]);

  //
  // Wakeup all APs in system for the first time.
  //
  if (EFI_ERROR (InitializeAllAps (CpuMpData))) {
    //
    // Some APs failed to wakeup successfully.
    //
    FreePages (MpBuffer, EFI_SIZE_TO_PAGES (BufferSize));
    return EFI_NOT_READY;
  }

  //
  // Initialize global data for MP support
  //
  InitMpGlobalData (CpuMpData);

  return EFI_SUCCESS;
}

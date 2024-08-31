/** @file
  LoongArch64 CPU MP Initialize Library common functions.

  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"

#include <Library/BaseLib.h>
#include <Register/LoongArch64/Csr.h>

#define INVALID_APIC_ID  0xFFFFFFFF

EFI_GUID  mCpuInitMpLibHobGuid = CPU_INIT_MP_LIB_HOB_GUID;

/**
  Get the Application Processors state.

  @param[in]  CpuData    The pointer to CPU_AP_DATA of specified AP

  @return  The AP status
**/
CPU_STATE
GetApState (
  IN  CPU_AP_DATA  *CpuData
  )
{
  return CpuData->State;
}

/**
  Set the Application Processors state.

  @param[in]   CpuData    The pointer to CPU_AP_DATA of specified AP
  @param[in]   State      The AP status
**/
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
  Get APIC ID of the executing processor.

  @return  32-bit APIC ID of the executing processor.
**/
UINT32
GetApicId (
  VOID
  )
{
  UINTN  CpuNum;

  CpuNum = CsrRead (LOONGARCH_CSR_CPUID);

  return CpuNum & 0x3ff;
}

/**
  Find the current Processor number by APIC ID.

  @param[in]  CpuMpData         Pointer to PEI CPU MP Data
  @param[out] ProcessorNumber   Return the pocessor number found

  @retval EFI_SUCCESS          ProcessorNumber is found and returned.
  @retval EFI_NOT_FOUND        ProcessorNumber is not found.
**/
EFI_STATUS
GetProcessorNumber (
  IN CPU_MP_DATA  *CpuMpData,
  OUT UINTN       *ProcessorNumber
  )
{
  UINTN            TotalProcessorNumber;
  UINTN            Index;
  CPU_INFO_IN_HOB  *CpuInfoInHob;

  CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;

  TotalProcessorNumber = CpuMpData->CpuCount;
  for (Index = 0; Index < TotalProcessorNumber; Index++) {
    if (CpuInfoInHob[Index].ApicId == GetApicId ()) {
      *ProcessorNumber = Index;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Sort the APIC ID of all processors.

  This function sorts the APIC ID of all processors so that processor number is
  assigned in the ascending order of APIC ID which eases MP debugging.

  @param[in] CpuMpData        Pointer to PEI CPU MP Data
**/
VOID
SortApicId (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  UINTN            Index1;
  UINTN            Index2;
  UINTN            Index3;
  UINT32           ApicId;
  CPU_INFO_IN_HOB  CpuInfo;
  UINT32           ApCount;
  CPU_INFO_IN_HOB  *CpuInfoInHob;
  volatile UINT32  *StartupApSignal;

  ApCount      = CpuMpData->CpuCount - 1;
  CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;
  if (ApCount != 0) {
    Index2 = 0;
    for (Index1 = (PcdGet32 (PcdCpuMaxLogicalProcessorNumber) - 1); Index1 > 0; Index1--) {
      if (CpuInfoInHob[Index1].ApicId != INVALID_APIC_ID) {
        if (Index1 == ApCount) {
          break;
        } else {
          for ( ; Index2 <= ApCount; Index2++) {
            if (CpuInfoInHob[Index2].ApicId == INVALID_APIC_ID) {
              CopyMem (CpuInfoInHob + Index2, CpuInfoInHob + Index1, sizeof (CPU_INFO_IN_HOB));
              CopyMem (CpuMpData->CpuData + Index2, CpuMpData->CpuData + Index1, sizeof (CPU_AP_DATA));
              CpuInfoInHob[Index1].ApicId = INVALID_APIC_ID;
              break;
            }
          }
        }
      } else {
        continue;
      }
    }

    for (Index1 = 0; Index1 < ApCount; Index1++) {
      Index3 = Index1;
      //
      // Sort key is the hardware default APIC ID
      //
      ApicId = CpuInfoInHob[Index1].ApicId;
      for (Index2 = Index1 + 1; Index2 <= ApCount; Index2++) {
        if (ApicId > CpuInfoInHob[Index2].ApicId) {
          Index3 = Index2;
          ApicId = CpuInfoInHob[Index2].ApicId;
        }
      }

      if (Index3 != Index1) {
        CopyMem (&CpuInfo, &CpuInfoInHob[Index3], sizeof (CPU_INFO_IN_HOB));
        CopyMem (
          &CpuInfoInHob[Index3],
          &CpuInfoInHob[Index1],
          sizeof (CPU_INFO_IN_HOB)
          );
        CopyMem (&CpuInfoInHob[Index1], &CpuInfo, sizeof (CPU_INFO_IN_HOB));

        //
        // Also exchange the StartupApSignal.
        //
        StartupApSignal                            = CpuMpData->CpuData[Index3].StartupApSignal;
        CpuMpData->CpuData[Index3].StartupApSignal =
          CpuMpData->CpuData[Index1].StartupApSignal;
        CpuMpData->CpuData[Index1].StartupApSignal = StartupApSignal;
      }
    }

    //
    // Get the processor number for the BSP
    //
    ApicId = GetApicId ();
    for (Index1 = 0; Index1 < CpuMpData->CpuCount; Index1++) {
      if (CpuInfoInHob[Index1].ApicId == ApicId) {
        CpuMpData->BspNumber = (UINT32)Index1;
        break;
      }
    }
  }
}

/**
  Get pointer to Processor Resource Data structure from GUIDd HOB.

  @return  The pointer to Processor Resource Data structure.
**/
PROCESSOR_RESOURCE_DATA *
GetProcessorResourceDataFromGuidedHob (
  VOID
  )
{
  EFI_HOB_GUID_TYPE        *GuidHob;
  VOID                     *DataInHob;
  PROCESSOR_RESOURCE_DATA  *ResourceData;

  ResourceData = NULL;
  GuidHob      = GetFirstGuidHob (&gProcessorResourceHobGuid);
  if (GuidHob != NULL) {
    DataInHob    = GET_GUID_HOB_DATA (GuidHob);
    ResourceData = (PROCESSOR_RESOURCE_DATA *)(*(UINTN *)DataInHob);
  }

  return ResourceData;
}

/**
  This function will get CPU count in the system.

  @param[in] CpuMpData        Pointer to PEI CPU MP Data

  @return  CPU count detected
**/
UINTN
CollectProcessorCount (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  PROCESSOR_RESOURCE_DATA  *ProcessorResourceData;
  CPU_INFO_IN_HOB          *CpuInfoInHob;
  UINTN                    Index;

  ProcessorResourceData = NULL;

  //
  // Set the default loop mode for APs.
  //
  CpuMpData->ApLoopMode = ApInRunLoop;

  //
  // Beacuse LoongArch does not have SIPI now, the APIC ID must be obtained before
  // calling IPI to wake up the APs. If NULL is obtained, NODE0 Core0 Mailbox0 is used
  // as the first broadcast method to wake up all APs, and all of APs will read NODE0
  // Core0 Mailbox0 in an infinit loop.
  //
  ProcessorResourceData = GetProcessorResourceDataFromGuidedHob ();

  if (ProcessorResourceData != NULL) {
    CpuMpData->ApLoopMode = ApInHltLoop;
    CpuMpData->CpuCount   = ProcessorResourceData->NumberOfProcessor;
    CpuInfoInHob          = (CPU_INFO_IN_HOB *)(UINTN)(CpuMpData->CpuInfoInHob);

    for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
      CpuInfoInHob[Index].ApicId = ProcessorResourceData->ApicId[Index];
    }
  }

  //
  // Send 1st broadcast IPI to APs to wakeup APs
  //
  CpuMpData->InitFlag = ApInitConfig;
  WakeUpAP (CpuMpData, TRUE, 0, NULL, NULL, FALSE);
  CpuMpData->InitFlag = ApInitDone;

  //
  // When InitFlag == ApInitConfig, WakeUpAP () guarantees all APs are checked in.
  // FinishedCount is the number of check-in APs.
  //
  CpuMpData->CpuCount = CpuMpData->FinishedCount + 1;
  ASSERT (CpuMpData->CpuCount <= PcdGet32 (PcdCpuMaxLogicalProcessorNumber));

  //
  // Wait for all APs finished the initialization
  //
  while (CpuMpData->FinishedCount < (CpuMpData->CpuCount - 1)) {
    CpuPause ();
  }

  //
  // Sort BSP/Aps by CPU APIC ID in ascending order
  //
  SortApicId (CpuMpData);

  DEBUG ((DEBUG_INFO, "MpInitLib: Find %d processors in system.\n", CpuMpData->CpuCount));

  return CpuMpData->CpuCount;
}

/**
  Initialize CPU AP Data when AP is wakeup at the first time.

  @param[in, out] CpuMpData        Pointer to PEI CPU MP Data
  @param[in]      ProcessorNumber  The handle number of processor
  @param[in]      BistData         Processor BIST data

**/
VOID
InitializeApData (
  IN OUT CPU_MP_DATA  *CpuMpData,
  IN     UINTN        ProcessorNumber,
  IN     UINT32       BistData
  )
{
  CPU_INFO_IN_HOB  *CpuInfoInHob;

  CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)(CpuMpData->CpuInfoInHob);

  CpuInfoInHob[ProcessorNumber].ApicId = GetApicId ();
  CpuInfoInHob[ProcessorNumber].Health = BistData;

  CpuMpData->CpuData[ProcessorNumber].Waiting    = FALSE;
  CpuMpData->CpuData[ProcessorNumber].CpuHealthy = (BistData == 0) ? TRUE : FALSE;

  InitializeSpinLock (&CpuMpData->CpuData[ProcessorNumber].ApLock);
  SetApState (&CpuMpData->CpuData[ProcessorNumber], CpuStateIdle);
}

/**
  Ap wake up function.

  Ap will wait for scheduling here, and if the IPI or wake-up signal is enabled,
  Ap will preform the corresponding functions.

  @param[in] ApIndex          Number of current executing AP
  @param[in] ExchangeInfo     Pointer to the MP exchange info buffer
**/
VOID
EFIAPI
ApWakeupFunction (
  IN UINTN                 ApIndex,
  IN MP_CPU_EXCHANGE_INFO  *ExchangeInfo
  )
{
  CPU_MP_DATA       *CpuMpData;
  UINTN             ProcessorNumber;
  volatile UINT32   *ApStartupSignalBuffer;
  EFI_AP_PROCEDURE  Procedure;
  VOID              *Parameter;

  CpuMpData = ExchangeInfo->CpuMpData;

  while (TRUE) {
    if (CpuMpData->InitFlag == ApInitConfig) {
      ProcessorNumber = ApIndex;
      //
      // If the AP can running to here, then the BIST must be zero.
      //
      InitializeApData (CpuMpData, ProcessorNumber, 0);
      ApStartupSignalBuffer = CpuMpData->CpuData[ProcessorNumber].StartupApSignal;
    } else {
      //
      // Execute AP function if AP is ready
      //
      GetProcessorNumber (CpuMpData, &ProcessorNumber);

      //
      // Clear AP start-up signal when AP waken up
      //
      ApStartupSignalBuffer = CpuMpData->CpuData[ProcessorNumber].StartupApSignal;
      InterlockedCompareExchange32 (
        (UINT32 *)ApStartupSignalBuffer,
        WAKEUP_AP_SIGNAL,
        0
        );

      //
      // Invoke AP function here
      //
      if (GetApState (&CpuMpData->CpuData[ProcessorNumber]) == CpuStateReady) {
        Procedure = (EFI_AP_PROCEDURE)CpuMpData->CpuData[ProcessorNumber].ApFunction;
        Parameter = (VOID *)CpuMpData->CpuData[ProcessorNumber].ApFunctionArgument;
        if (Procedure != NULL) {
          SetApState (&CpuMpData->CpuData[ProcessorNumber], CpuStateBusy);
          Procedure (Parameter);
        }

        SetApState (&CpuMpData->CpuData[ProcessorNumber], CpuStateFinished);
      }
    }

    //
    // Updates the finished count
    //
    InterlockedIncrement ((UINT32 *)&CpuMpData->FinishedCount);

    while (TRUE) {
      //
      // Clean per-core mail box registers.
      //
      IoCsrWrite64 (LOONGARCH_IOCSR_MBUF0, 0x0);
      IoCsrWrite64 (LOONGARCH_IOCSR_MBUF1, 0x0);
      IoCsrWrite64 (LOONGARCH_IOCSR_MBUF2, 0x0);
      IoCsrWrite64 (LOONGARCH_IOCSR_MBUF3, 0x0);

      //
      // Enable IPI interrupt and global interrupt
      //
      EnableLocalInterrupts (BIT12);
      IoCsrWrite32 (LOONGARCH_IOCSR_IPI_EN, 0xFFFFFFFFU);
      EnableInterrupts ();

      //
      // Ap entry HLT mode
      //
      CpuSleep ();

      //
      // Disable global interrupts when wake up
      //
      DisableInterrupts ();

      //
      // Update CpuMpData
      //
      if (CpuMpData != ExchangeInfo->CpuMpData) {
        CpuMpData = ExchangeInfo->CpuMpData;
        GetProcessorNumber (CpuMpData, &ProcessorNumber);
        ApStartupSignalBuffer = CpuMpData->CpuData[ProcessorNumber].StartupApSignal;
      }

      //
      // Break out of the loop if wake up signal is not NULL.
      //
      if (*ApStartupSignalBuffer == WAKEUP_AP_SIGNAL) {
        break;
      }
    }
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
  Wait for AP wakeup and write AP start-up signal till AP is waken up.

  @param[in] ApStartupSignalBuffer  Pointer to AP wakeup signal
**/
VOID
WaitApWakeup (
  IN volatile UINT32  *ApStartupSignalBuffer
  )
{
  //
  // If AP is waken up, StartupApSignal should be cleared.
  // Otherwise, write StartupApSignal again till AP waken up.
  //
  while (InterlockedCompareExchange32 (
           (UINT32 *)ApStartupSignalBuffer,
           WAKEUP_AP_SIGNAL,
           WAKEUP_AP_SIGNAL
           ) != 0)
  {
    CpuPause ();
  }
}

/**
  This function will fill the exchange info structure.

  @param[in] CpuMpData          Pointer to CPU MP Data

**/
VOID
FillExchangeInfoData (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  volatile MP_CPU_EXCHANGE_INFO  *ExchangeInfo;

  if (!CpuMpData->MpCpuExchangeInfo) {
    CpuMpData->MpCpuExchangeInfo = (MP_CPU_EXCHANGE_INFO *)AllocatePool (sizeof (MP_CPU_EXCHANGE_INFO));
  }

  ExchangeInfo            = CpuMpData->MpCpuExchangeInfo;
  ExchangeInfo->CpuMpData = CpuMpData;
}

/**
  This function will be called by BSP to wakeup AP.

  @param[in] CpuMpData          Pointer to CPU MP Data
  @param[in] Broadcast          TRUE:  Send broadcast IPI to all APs
                                FALSE: Send IPI to AP by ApicId
  @param[in] ProcessorNumber    The handle number of specified processor
  @param[in] Procedure          The function to be invoked by AP
  @param[in] ProcedureArgument  The argument to be passed into AP function
  @param[in] WakeUpDisabledAps  Whether need to wake up disabled APs in broadcast mode. Currently not used on LoongArch.
**/
VOID
WakeUpAP (
  IN CPU_MP_DATA       *CpuMpData,
  IN BOOLEAN           Broadcast,
  IN UINTN             ProcessorNumber,
  IN EFI_AP_PROCEDURE  Procedure               OPTIONAL,
  IN VOID              *ProcedureArgument      OPTIONAL,
  IN BOOLEAN           WakeUpDisabledAps
  )
{
  volatile MP_CPU_EXCHANGE_INFO  *ExchangeInfo;
  UINTN                          Index;
  CPU_AP_DATA                    *CpuData;
  CPU_INFO_IN_HOB                *CpuInfoInHob;

  CpuMpData->FinishedCount = 0;

  CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;

  if (CpuMpData->InitFlag != ApInitDone) {
    FillExchangeInfoData (CpuMpData);
  }

  ExchangeInfo = CpuMpData->MpCpuExchangeInfo;
  //
  // If InitFlag is ApInitConfig, broadcasts all APs to initize themselves.
  //
  if (CpuMpData->InitFlag == ApInitConfig) {
    DEBUG ((DEBUG_INFO, "%a: func 0x%llx, ExchangeInfo 0x%llx\n", __func__, ApWakeupFunction, (UINTN)ExchangeInfo));
    if (CpuMpData->ApLoopMode == ApInHltLoop) {
      for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
        if (Index != CpuMpData->BspNumber) {
          IoCsrWrite64 (
            LOONGARCH_IOCSR_MBUF_SEND,
            (IOCSR_MBUF_SEND_BLOCKING |
             (IOCSR_MBUF_SEND_BOX_HI (0x3) << IOCSR_MBUF_SEND_BOX_SHIFT) |
             (CpuInfoInHob[Index].ApicId << IOCSR_MBUF_SEND_CPU_SHIFT) |
             ((UINTN)(ExchangeInfo) & IOCSR_MBUF_SEND_H32_MASK))
            );
          IoCsrWrite64 (
            LOONGARCH_IOCSR_MBUF_SEND,
            (IOCSR_MBUF_SEND_BLOCKING |
             (IOCSR_MBUF_SEND_BOX_LO (0x3) << IOCSR_MBUF_SEND_BOX_SHIFT) |
             (CpuInfoInHob[Index].ApicId << IOCSR_MBUF_SEND_CPU_SHIFT) |
             ((UINTN)ExchangeInfo) << IOCSR_MBUF_SEND_BUF_SHIFT)
            );

          IoCsrWrite64 (
            LOONGARCH_IOCSR_MBUF_SEND,
            (IOCSR_MBUF_SEND_BLOCKING |
             (IOCSR_MBUF_SEND_BOX_HI (0x0) << IOCSR_MBUF_SEND_BOX_SHIFT) |
             (CpuInfoInHob[Index].ApicId << IOCSR_MBUF_SEND_CPU_SHIFT) |
             ((UINTN)(ApWakeupFunction) & IOCSR_MBUF_SEND_H32_MASK))
            );
          IoCsrWrite64 (
            LOONGARCH_IOCSR_MBUF_SEND,
            (IOCSR_MBUF_SEND_BLOCKING |
             (IOCSR_MBUF_SEND_BOX_LO (0x0) << IOCSR_MBUF_SEND_BOX_SHIFT) |
             (CpuInfoInHob[Index].ApicId << IOCSR_MBUF_SEND_CPU_SHIFT) |
             ((UINTN)ApWakeupFunction) << IOCSR_MBUF_SEND_BUF_SHIFT)
            );

          //
          // Send IPI 4 interrupt to wake up APs.
          //
          IoCsrWrite64 (
            LOONGARCH_IOCSR_IPI_SEND,
            (IOCSR_MBUF_SEND_BLOCKING |
             (CpuInfoInHob[Index].ApicId << IOCSR_MBUF_SEND_CPU_SHIFT) |
             0x2 // Bit 2
            )
            );
        }
      }
    } else {
      IoCsrWrite64 (LOONGARCH_IOCSR_MBUF3, (UINTN)ExchangeInfo);
      IoCsrWrite64 (LOONGARCH_IOCSR_MBUF0, (UINTN)ApWakeupFunction);
    }

    TimedWaitForApFinish (
      CpuMpData,
      PcdGet32 (PcdCpuMaxLogicalProcessorNumber) - 1,
      PcdGet32 (PcdCpuApInitTimeOutInMicroSeconds)
      );
  } else {
    if (Broadcast) {
      for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
        if (Index != CpuMpData->BspNumber) {
          CpuData = &CpuMpData->CpuData[Index];
          if ((GetApState (CpuData) == CpuStateDisabled) && !WakeUpDisabledAps) {
            continue;
          }

          CpuData->ApFunction         = (UINTN)Procedure;
          CpuData->ApFunctionArgument = (UINTN)ProcedureArgument;
          SetApState (CpuData, CpuStateReady);
          *(UINT32 *)CpuData->StartupApSignal = WAKEUP_AP_SIGNAL;

          //
          // Send IPI 4 interrupt to wake up APs.
          //
          IoCsrWrite64 (
            LOONGARCH_IOCSR_IPI_SEND,
            (IOCSR_MBUF_SEND_BLOCKING |
             (CpuInfoInHob[Index].ApicId << IOCSR_MBUF_SEND_CPU_SHIFT) |
             0x2 // Bit 2
            )
            );
        }
      }

      //
      // Wait all APs waken up.
      //
      for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
        CpuData = &CpuMpData->CpuData[Index];
        if (Index != CpuMpData->BspNumber) {
          WaitApWakeup (CpuData->StartupApSignal);
        }
      }
    } else {
      CpuData                     = &CpuMpData->CpuData[ProcessorNumber];
      CpuData->ApFunction         = (UINTN)Procedure;
      CpuData->ApFunctionArgument = (UINTN)ProcedureArgument;
      SetApState (CpuData, CpuStateReady);
      //
      // Wakeup specified AP
      //
      *(UINT32 *)CpuData->StartupApSignal = WAKEUP_AP_SIGNAL;

      //
      // Send IPI 4 interrupt to wake up APs.
      //
      IoCsrWrite64 (
        LOONGARCH_IOCSR_IPI_SEND,
        (IOCSR_MBUF_SEND_BLOCKING |
         (CpuInfoInHob[ProcessorNumber].ApicId << IOCSR_MBUF_SEND_CPU_SHIFT) |
         0x2 // Bit 2
        )
        );

      //
      // Wait specified AP waken up
      //
      WaitApWakeup (CpuData->StartupApSignal);
    }
  }
}

/**
  Searches for the next waiting AP.

  Search for the next AP that is put in waiting state by single-threaded StartupAllAPs().

  @param[out]  NextProcessorNumber  Pointer to the processor number of the next waiting AP.

  @retval EFI_SUCCESS          The next waiting AP has been found.
  @retval EFI_NOT_FOUND        No waiting AP exists.

**/
EFI_STATUS
GetNextWaitingProcessorNumber (
  OUT UINTN  *NextProcessorNumber
  )
{
  UINTN        ProcessorNumber;
  CPU_MP_DATA  *CpuMpData;

  CpuMpData = GetCpuMpData ();

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

  @param[in]  ProcessorNumber       The handle number of processor.

  @retval EFI_SUCCESS           Specified AP has finished task assigned by StartupThisAPs().
  @retval EFI_TIMEOUT           The timeout expires.
  @retval EFI_NOT_READY         Specified AP has not finished task and timeout has not expired.
**/
EFI_STATUS
CheckThisAP (
  IN UINTN  ProcessorNumber
  )
{
  CPU_MP_DATA  *CpuMpData;
  CPU_AP_DATA  *CpuData;

  CpuMpData = GetCpuMpData ();
  CpuData   = &CpuMpData->CpuData[ProcessorNumber];

  //
  // If the AP finishes for StartupThisAP(), return EFI_SUCCESS.
  //
  if (GetApState (CpuData) == CpuStateFinished) {
    if (CpuData->Finished != NULL) {
      *(CpuData->Finished) = TRUE;
    }

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
CheckAllAPs (
  VOID
  )
{
  UINTN        ProcessorNumber;
  UINTN        NextProcessorNumber;
  EFI_STATUS   Status;
  CPU_MP_DATA  *CpuMpData;
  CPU_AP_DATA  *CpuData;

  CpuMpData = GetCpuMpData ();

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
  BOOLEAN      HasEnabledAp;
  CPU_STATE    ApState;

  CpuMpData = GetCpuMpData ();

  if (FailedCpuList != NULL) {
    *FailedCpuList = NULL;
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
  HasEnabledAp   = FALSE;
  //
  // Check whether all enabled APs are idle.
  // If any enabled AP is not idle, return EFI_NOT_READY.
  //
  for (ProcessorNumber = 0; ProcessorNumber < ProcessorCount; ProcessorNumber++) {
    CpuData = &CpuMpData->CpuData[ProcessorNumber];
    if (ProcessorNumber != CpuMpData->BspNumber) {
      ApState = GetApState (CpuData);
      if (ApState != CpuStateDisabled) {
        HasEnabledAp = TRUE;
        if (ApState != CpuStateIdle) {
          //
          // If any enabled APs are busy, return EFI_NOT_READY.
          //
          return EFI_NOT_READY;
        }
      }
    }
  }

  if (!HasEnabledAp && ExcludeBsp) {
    //
    // If no enabled AP exists and not include Bsp to do the procedure, return EFI_NOT_STARTED.
    //
    return EFI_NOT_STARTED;
  }

  CpuMpData->RunningCount = 0;
  for (ProcessorNumber = 0; ProcessorNumber < ProcessorCount; ProcessorNumber++) {
    CpuData          = &CpuMpData->CpuData[ProcessorNumber];
    CpuData->Waiting = FALSE;
    if (ProcessorNumber != CpuMpData->BspNumber) {
      if (CpuData->State == CpuStateIdle) {
        //
        // Mark this processor as responsible for current calling.
        //
        CpuData->Waiting = TRUE;
        CpuMpData->RunningCount++;
      }
    }
  }

  CpuMpData->Procedure     = Procedure;
  CpuMpData->ProcArguments = ProcedureArgument;
  CpuMpData->SingleThread  = SingleThread;
  CpuMpData->FinishedCount = 0;
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
      if (ProcessorNumber == CallerNumber) {
        continue;
      }

      if (CpuMpData->CpuData[ProcessorNumber].Waiting) {
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
  // Check whether specified AP is disabled
  //
  if (GetApState (&CpuMpData->CpuData[ProcessorNumber]) == CpuStateDisabled) {
    return EFI_INVALID_PARAMETER;
  }

  CpuData               = &CpuMpData->CpuData[ProcessorNumber];
  CpuData->WaitEvent    = WaitEvent;
  CpuData->Finished     = Finished;
  CpuData->ExpectedTime = CalculateTimeout (TimeoutInMicroseconds, &CpuData->CurrentTime);
  CpuData->TotalTime    = 0;

  WakeUpAP (CpuMpData, FALSE, ProcessorNumber, Procedure, ProcedureArgument, FALSE);

  //
  // If WaitEvent is NULL, execute in blocking mode.
  // BSP checks AP's state until it finishes or TimeoutInMicrosecsond expires.
  //
  Status = EFI_SUCCESS;
  if (WaitEvent == NULL) {
    do {
      Status = CheckThisAP (ProcessorNumber);
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
           TRUE,
           FALSE,
           NULL,
           TimeoutInMicroseconds,
           ProcedureArgument,
           NULL
           );
}

/**
  MP Initialize Library initialization.

  This service will allocate AP reset vector and wakeup all APs to do APs
  initialization.

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
  CPU_MP_DATA      *OldCpuMpData;
  CPU_INFO_IN_HOB  *CpuInfoInHob;
  UINT32           MaxLogicalProcessorNumber;
  UINTN            BufferSize;
  UINTN            MonitorBufferSize;
  VOID             *MpBuffer;
  CPU_MP_DATA      *CpuMpData;
  UINTN            Index;

  OldCpuMpData = GetCpuMpDataFromGuidedHob ();
  if (OldCpuMpData == NULL) {
    MaxLogicalProcessorNumber = PcdGet32 (PcdCpuMaxLogicalProcessorNumber);
  } else {
    MaxLogicalProcessorNumber = OldCpuMpData->CpuCount;
  }

  ASSERT (MaxLogicalProcessorNumber != 0);

  MonitorBufferSize = sizeof (WAKEUP_AP_SIGNAL) * MaxLogicalProcessorNumber;

  BufferSize  = 0;
  BufferSize += MonitorBufferSize;
  BufferSize += sizeof (CPU_MP_DATA);
  BufferSize += (sizeof (CPU_AP_DATA) + sizeof (CPU_INFO_IN_HOB))* MaxLogicalProcessorNumber;
  MpBuffer    = AllocatePages (EFI_SIZE_TO_PAGES (BufferSize));
  ASSERT (MpBuffer != NULL);
  ZeroMem (MpBuffer, BufferSize);

  CpuMpData = (CPU_MP_DATA *)MpBuffer;

  CpuMpData->CpuCount     = 1;
  CpuMpData->BspNumber    = 0;
  CpuMpData->CpuData      = (CPU_AP_DATA *)(CpuMpData + 1);
  CpuMpData->CpuInfoInHob = (UINT64)(UINTN)(CpuMpData->CpuData + MaxLogicalProcessorNumber);

  InitializeSpinLock (&CpuMpData->MpLock);

  //
  // Set BSP basic information
  //
  InitializeApData (CpuMpData, 0, 0);

  //
  // Set up APs wakeup signal buffer and initialization APs ApicId status.
  //
  for (Index = 0; Index < MaxLogicalProcessorNumber; Index++) {
    CpuMpData->CpuData[Index].StartupApSignal =
      (UINT32 *)((MpBuffer + BufferSize - MonitorBufferSize) + (sizeof (WAKEUP_AP_SIGNAL) * Index));
    if ((OldCpuMpData == NULL) && (Index != CpuMpData->BspNumber)) {
      ((CPU_INFO_IN_HOB  *)CpuMpData->CpuInfoInHob)[Index].ApicId = INVALID_APIC_ID;
    }
  }

  if (OldCpuMpData == NULL) {
    if (MaxLogicalProcessorNumber > 1) {
      //
      // Wakeup all APs and calculate the processor count in system
      //
      CollectProcessorCount (CpuMpData);
    }
  } else {
    //
    // APs have been wakeup before, just get the CPU Information
    // from HOB
    //
    CpuMpData->CpuCount          = OldCpuMpData->CpuCount;
    CpuMpData->BspNumber         = OldCpuMpData->BspNumber;
    CpuMpData->CpuInfoInHob      = OldCpuMpData->CpuInfoInHob;
    CpuMpData->MpCpuExchangeInfo = OldCpuMpData->MpCpuExchangeInfo;

    CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;
    for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
      InitializeSpinLock (&CpuMpData->CpuData[Index].ApLock);
      CpuMpData->CpuData[Index].CpuHealthy = (CpuInfoInHob[Index].Health == 0) ? TRUE : FALSE;
    }

    if (CpuMpData->CpuCount > 1) {
      //
      // Only needs to use this flag for DXE phase to update the wake up
      // buffer. Wakeup buffer allocated in PEI phase is no longer valid
      // in DXE.
      //
      CpuMpData->InitFlag = ApInitReconfig;
      WakeUpAP (CpuMpData, TRUE, 0, NULL, NULL, TRUE);

      //
      // Wait for all APs finished initialization
      //
      while (CpuMpData->FinishedCount < (CpuMpData->CpuCount - 1)) {
        CpuPause ();
      }

      CpuMpData->InitFlag = ApInitDone;
    }

    if (MaxLogicalProcessorNumber > 1) {
      for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
        SetApState (&CpuMpData->CpuData[Index], CpuStateIdle);
      }
    }
  }

  //
  // Initialize global data for MP support
  //
  InitMpGlobalData (CpuMpData);

  return EFI_SUCCESS;
}

/**
  Gets detailed MP-related information on the requested processor at the
  instant this call is made. This service may only be called from the BSP.

  @param[in]  ProcessorNumber       The handle number of processor.
  @param[out] ProcessorInfoBuffer   A pointer to the buffer where information for
                                    the requested processor is deposited.
  @param[out]  HealthData            Return processor health data.

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
  CPU_MP_DATA      *CpuMpData;
  UINTN            CallerNumber;
  CPU_INFO_IN_HOB  *CpuInfoInHob;

  CpuMpData    = GetCpuMpData ();
  CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;

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

  ProcessorInfoBuffer->ProcessorId = (UINT64)CpuInfoInHob[ProcessorNumber].ApicId;
  ProcessorInfoBuffer->StatusFlag  = 0;
  if (ProcessorNumber == CpuMpData->BspNumber) {
    ProcessorInfoBuffer->StatusFlag |= PROCESSOR_AS_BSP_BIT;
  }

  if (CpuMpData->CpuData[ProcessorNumber].CpuHealthy) {
    ProcessorInfoBuffer->StatusFlag |= PROCESSOR_HEALTH_STATUS_BIT;
  }

  if (GetApState (&CpuMpData->CpuData[ProcessorNumber]) == CpuStateDisabled) {
    ProcessorInfoBuffer->StatusFlag &= ~PROCESSOR_ENABLED_BIT;
  } else {
    ProcessorInfoBuffer->StatusFlag |= PROCESSOR_ENABLED_BIT;
  }

  if (HealthData != NULL) {
    HealthData->Uint32 = CpuInfoInHob[ProcessorNumber].Health;
  }

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

**/
EFI_STATUS
EFIAPI
MpInitLibWhoAmI (
  OUT UINTN  *ProcessorNumber
  )
{
  CPU_MP_DATA  *CpuMpData;

  if (ProcessorNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CpuMpData = GetCpuMpData ();

  return GetProcessorNumber (CpuMpData, ProcessorNumber);
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
  UINTN        Index;

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

  ProcessorNumber        = CpuMpData->CpuCount;
  EnabledProcessorNumber = 0;
  for (Index = 0; Index < ProcessorNumber; Index++) {
    if (GetApState (&CpuMpData->CpuData[Index]) != CpuStateDisabled) {
      EnabledProcessorNumber++;
    }
  }

  if (NumberOfProcessors != NULL) {
    *NumberOfProcessors = ProcessorNumber;
  }

  if (NumberOfEnabledProcessors != NULL) {
    *NumberOfEnabledProcessors = EnabledProcessorNumber;
  }

  return EFI_SUCCESS;
}

/**
  Get pointer to CPU MP Data structure from GUIDed HOB.

  @return  The pointer to CPU MP Data structure.
**/
CPU_MP_DATA *
GetCpuMpDataFromGuidedHob (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  VOID               *DataInHob;
  CPU_MP_DATA        *CpuMpData;

  CpuMpData = NULL;
  GuidHob   = GetFirstGuidHob (&mCpuInitMpLibHobGuid);

  if (GuidHob != NULL) {
    DataInHob = GET_GUID_HOB_DATA (GuidHob);
    CpuMpData = (CPU_MP_DATA *)(*(UINTN *)DataInHob);
  }

  return CpuMpData;
}

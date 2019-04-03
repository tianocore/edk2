/** @file
  CPU MP Initialize Library common functions.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"

EFI_GUID mCpuInitMpLibHobGuid = CPU_INIT_MP_LIB_HOB_GUID;

/**
  The function will check if BSP Execute Disable is enabled.

  DxeIpl may have enabled Execute Disable for BSP, APs need to
  get the status and sync up the settings.
  If BSP's CR0.Paging is not set, BSP execute Disble feature is
  not working actually.

  @retval TRUE      BSP Execute Disable is enabled.
  @retval FALSE     BSP Execute Disable is not enabled.
**/
BOOLEAN
IsBspExecuteDisableEnabled (
  VOID
  )
{
  UINT32                      Eax;
  CPUID_EXTENDED_CPU_SIG_EDX  Edx;
  MSR_IA32_EFER_REGISTER      EferMsr;
  BOOLEAN                     Enabled;
  IA32_CR0                    Cr0;

  Enabled = FALSE;
  Cr0.UintN = AsmReadCr0 ();
  if (Cr0.Bits.PG != 0) {
    //
    // If CR0 Paging bit is set
    //
    AsmCpuid (CPUID_EXTENDED_FUNCTION, &Eax, NULL, NULL, NULL);
    if (Eax >= CPUID_EXTENDED_CPU_SIG) {
      AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, NULL, &Edx.Uint32);
      //
      // CPUID 0x80000001
      // Bit 20: Execute Disable Bit available.
      //
      if (Edx.Bits.NX != 0) {
        EferMsr.Uint64 = AsmReadMsr64 (MSR_IA32_EFER);
        //
        // MSR 0xC0000080
        // Bit 11: Execute Disable Bit enable.
        //
        if (EferMsr.Bits.NXE != 0) {
          Enabled = TRUE;
        }
      }
    }
  }

  return Enabled;
}

/**
  Worker function for SwitchBSP().

  Worker function for SwitchBSP(), assigned to the AP which is intended
  to become BSP.

  @param[in] Buffer   Pointer to CPU MP Data
**/
VOID
EFIAPI
FutureBSPProc (
  IN  VOID            *Buffer
  )
{
  CPU_MP_DATA         *DataInHob;

  DataInHob = (CPU_MP_DATA *) Buffer;
  AsmExchangeRole (&DataInHob->APInfo, &DataInHob->BSPInfo);
}

/**
  Get the Application Processors state.

  @param[in]  CpuData    The pointer to CPU_AP_DATA of specified AP

  @return  The AP status
**/
CPU_STATE
GetApState (
  IN  CPU_AP_DATA     *CpuData
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
  IN  CPU_AP_DATA     *CpuData,
  IN  CPU_STATE       State
  )
{
  AcquireSpinLock (&CpuData->ApLock);
  CpuData->State = State;
  ReleaseSpinLock (&CpuData->ApLock);
}

/**
  Save BSP's local APIC timer setting.

  @param[in] CpuMpData          Pointer to CPU MP Data
**/
VOID
SaveLocalApicTimerSetting (
  IN CPU_MP_DATA   *CpuMpData
  )
{
  //
  // Record the current local APIC timer setting of BSP
  //
  GetApicTimerState (
    &CpuMpData->DivideValue,
    &CpuMpData->PeriodicMode,
    &CpuMpData->Vector
    );
  CpuMpData->CurrentTimerCount   = GetApicTimerCurrentCount ();
  CpuMpData->TimerInterruptState = GetApicTimerInterruptState ();
}

/**
  Sync local APIC timer setting from BSP to AP.

  @param[in] CpuMpData          Pointer to CPU MP Data
**/
VOID
SyncLocalApicTimerSetting (
  IN CPU_MP_DATA   *CpuMpData
  )
{
  //
  // Sync local APIC timer setting from BSP to AP
  //
  InitializeApicTimer (
    CpuMpData->DivideValue,
    CpuMpData->CurrentTimerCount,
    CpuMpData->PeriodicMode,
    CpuMpData->Vector
    );
  //
  // Disable AP's local APIC timer interrupt
  //
  DisableApicTimerInterrupt ();
}

/**
  Save the volatile registers required to be restored following INIT IPI.

  @param[out]  VolatileRegisters    Returns buffer saved the volatile resisters
**/
VOID
SaveVolatileRegisters (
  OUT CPU_VOLATILE_REGISTERS    *VolatileRegisters
  )
{
  CPUID_VERSION_INFO_EDX        VersionInfoEdx;

  VolatileRegisters->Cr0 = AsmReadCr0 ();
  VolatileRegisters->Cr3 = AsmReadCr3 ();
  VolatileRegisters->Cr4 = AsmReadCr4 ();

  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &VersionInfoEdx.Uint32);
  if (VersionInfoEdx.Bits.DE != 0) {
    //
    // If processor supports Debugging Extensions feature
    // by CPUID.[EAX=01H]:EDX.BIT2
    //
    VolatileRegisters->Dr0 = AsmReadDr0 ();
    VolatileRegisters->Dr1 = AsmReadDr1 ();
    VolatileRegisters->Dr2 = AsmReadDr2 ();
    VolatileRegisters->Dr3 = AsmReadDr3 ();
    VolatileRegisters->Dr6 = AsmReadDr6 ();
    VolatileRegisters->Dr7 = AsmReadDr7 ();
  }

  AsmReadGdtr (&VolatileRegisters->Gdtr);
  AsmReadIdtr (&VolatileRegisters->Idtr);
  VolatileRegisters->Tr = AsmReadTr ();
}

/**
  Restore the volatile registers following INIT IPI.

  @param[in]  VolatileRegisters   Pointer to volatile resisters
  @param[in]  IsRestoreDr         TRUE:  Restore DRx if supported
                                  FALSE: Do not restore DRx
**/
VOID
RestoreVolatileRegisters (
  IN CPU_VOLATILE_REGISTERS    *VolatileRegisters,
  IN BOOLEAN                   IsRestoreDr
  )
{
  CPUID_VERSION_INFO_EDX        VersionInfoEdx;
  IA32_TSS_DESCRIPTOR           *Tss;

  AsmWriteCr3 (VolatileRegisters->Cr3);
  AsmWriteCr4 (VolatileRegisters->Cr4);
  AsmWriteCr0 (VolatileRegisters->Cr0);

  if (IsRestoreDr) {
    AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &VersionInfoEdx.Uint32);
    if (VersionInfoEdx.Bits.DE != 0) {
      //
      // If processor supports Debugging Extensions feature
      // by CPUID.[EAX=01H]:EDX.BIT2
      //
      AsmWriteDr0 (VolatileRegisters->Dr0);
      AsmWriteDr1 (VolatileRegisters->Dr1);
      AsmWriteDr2 (VolatileRegisters->Dr2);
      AsmWriteDr3 (VolatileRegisters->Dr3);
      AsmWriteDr6 (VolatileRegisters->Dr6);
      AsmWriteDr7 (VolatileRegisters->Dr7);
    }
  }

  AsmWriteGdtr (&VolatileRegisters->Gdtr);
  AsmWriteIdtr (&VolatileRegisters->Idtr);
  if (VolatileRegisters->Tr != 0 &&
      VolatileRegisters->Tr < VolatileRegisters->Gdtr.Limit) {
    Tss = (IA32_TSS_DESCRIPTOR *)(VolatileRegisters->Gdtr.Base +
                                  VolatileRegisters->Tr);
    if (Tss->Bits.P == 1) {
      Tss->Bits.Type &= 0xD;  // 1101 - Clear busy bit just in case
      AsmWriteTr (VolatileRegisters->Tr);
    }
  }
}

/**
  Detect whether Mwait-monitor feature is supported.

  @retval TRUE    Mwait-monitor feature is supported.
  @retval FALSE   Mwait-monitor feature is not supported.
**/
BOOLEAN
IsMwaitSupport (
  VOID
  )
{
  CPUID_VERSION_INFO_ECX        VersionInfoEcx;

  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, &VersionInfoEcx.Uint32, NULL);
  return (VersionInfoEcx.Bits.MONITOR == 1) ? TRUE : FALSE;
}

/**
  Get AP loop mode.

  @param[out] MonitorFilterSize  Returns the largest monitor-line size in bytes.

  @return The AP loop mode.
**/
UINT8
GetApLoopMode (
  OUT UINT32     *MonitorFilterSize
  )
{
  UINT8                         ApLoopMode;
  CPUID_MONITOR_MWAIT_EBX       MonitorMwaitEbx;

  ASSERT (MonitorFilterSize != NULL);

  ApLoopMode = PcdGet8 (PcdCpuApLoopMode);
  ASSERT (ApLoopMode >= ApInHltLoop && ApLoopMode <= ApInRunLoop);
  if (ApLoopMode == ApInMwaitLoop) {
    if (!IsMwaitSupport ()) {
      //
      // If processor does not support MONITOR/MWAIT feature,
      // force AP in Hlt-loop mode
      //
      ApLoopMode = ApInHltLoop;
    }
  }

  if (ApLoopMode != ApInMwaitLoop) {
    *MonitorFilterSize = sizeof (UINT32);
  } else {
    //
    // CPUID.[EAX=05H]:EBX.BIT0-15: Largest monitor-line size in bytes
    // CPUID.[EAX=05H].EDX: C-states supported using MWAIT
    //
    AsmCpuid (CPUID_MONITOR_MWAIT, NULL, &MonitorMwaitEbx.Uint32, NULL, NULL);
    *MonitorFilterSize = MonitorMwaitEbx.Bits.LargestMonitorLineSize;
  }

  return ApLoopMode;
}

/**
  Sort the APIC ID of all processors.

  This function sorts the APIC ID of all processors so that processor number is
  assigned in the ascending order of APIC ID which eases MP debugging.

  @param[in] CpuMpData        Pointer to PEI CPU MP Data
**/
VOID
SortApicId (
  IN CPU_MP_DATA   *CpuMpData
  )
{
  UINTN             Index1;
  UINTN             Index2;
  UINTN             Index3;
  UINT32            ApicId;
  CPU_INFO_IN_HOB   CpuInfo;
  UINT32            ApCount;
  CPU_INFO_IN_HOB   *CpuInfoInHob;
  volatile UINT32   *StartupApSignal;

  ApCount = CpuMpData->CpuCount - 1;
  CpuInfoInHob = (CPU_INFO_IN_HOB *) (UINTN) CpuMpData->CpuInfoInHob;
  if (ApCount != 0) {
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
        StartupApSignal = CpuMpData->CpuData[Index3].StartupApSignal;
        CpuMpData->CpuData[Index3].StartupApSignal =
          CpuMpData->CpuData[Index1].StartupApSignal;
        CpuMpData->CpuData[Index1].StartupApSignal = StartupApSignal;
      }
    }

    //
    // Get the processor number for the BSP
    //
    ApicId = GetInitialApicId ();
    for (Index1 = 0; Index1 < CpuMpData->CpuCount; Index1++) {
      if (CpuInfoInHob[Index1].ApicId == ApicId) {
        CpuMpData->BspNumber = (UINT32) Index1;
        break;
      }
    }
  }
}

/**
  Enable x2APIC mode on APs.

  @param[in, out] Buffer  Pointer to private data buffer.
**/
VOID
EFIAPI
ApFuncEnableX2Apic (
  IN OUT VOID  *Buffer
  )
{
  SetApicMode (LOCAL_APIC_MODE_X2APIC);
}

/**
  Do sync on APs.

  @param[in, out] Buffer  Pointer to private data buffer.
**/
VOID
EFIAPI
ApInitializeSync (
  IN OUT VOID  *Buffer
  )
{
  CPU_MP_DATA  *CpuMpData;

  CpuMpData = (CPU_MP_DATA *) Buffer;
  //
  // Load microcode on AP
  //
  MicrocodeDetect (CpuMpData, FALSE);
  //
  // Sync BSP's MTRR table to AP
  //
  MtrrSetAllMtrrs (&CpuMpData->MtrrTable);
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
  IN CPU_MP_DATA               *CpuMpData,
  OUT UINTN                    *ProcessorNumber
  )
{
  UINTN                   TotalProcessorNumber;
  UINTN                   Index;
  CPU_INFO_IN_HOB         *CpuInfoInHob;
  UINT32                  CurrentApicId;

  CpuInfoInHob = (CPU_INFO_IN_HOB *) (UINTN) CpuMpData->CpuInfoInHob;

  TotalProcessorNumber = CpuMpData->CpuCount;
  CurrentApicId = GetApicId ();
  for (Index = 0; Index < TotalProcessorNumber; Index ++) {
    if (CpuInfoInHob[Index].ApicId == CurrentApicId) {
      *ProcessorNumber = Index;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  This function will get CPU count in the system.

  @param[in] CpuMpData        Pointer to PEI CPU MP Data

  @return  CPU count detected
**/
UINTN
CollectProcessorCount (
  IN CPU_MP_DATA         *CpuMpData
  )
{
  UINTN                  Index;

  //
  // Send 1st broadcast IPI to APs to wakeup APs
  //
  CpuMpData->InitFlag     = ApInitConfig;
  CpuMpData->X2ApicEnable = FALSE;
  WakeUpAP (CpuMpData, TRUE, 0, NULL, NULL, TRUE);
  CpuMpData->InitFlag = ApInitDone;
  ASSERT (CpuMpData->CpuCount <= PcdGet32 (PcdCpuMaxLogicalProcessorNumber));
  //
  // Wait for all APs finished the initialization
  //
  while (CpuMpData->FinishedCount < (CpuMpData->CpuCount - 1)) {
    CpuPause ();
  }

  if (CpuMpData->CpuCount > 255) {
    //
    // If there are more than 255 processor found, force to enable X2APIC
    //
    CpuMpData->X2ApicEnable = TRUE;
  }
  if (CpuMpData->X2ApicEnable) {
    DEBUG ((DEBUG_INFO, "Force x2APIC mode!\n"));
    //
    // Wakeup all APs to enable x2APIC mode
    //
    WakeUpAP (CpuMpData, TRUE, 0, ApFuncEnableX2Apic, NULL, TRUE);
    //
    // Wait for all known APs finished
    //
    while (CpuMpData->FinishedCount < (CpuMpData->CpuCount - 1)) {
      CpuPause ();
    }
    //
    // Enable x2APIC on BSP
    //
    SetApicMode (LOCAL_APIC_MODE_X2APIC);
    //
    // Set BSP/Aps state to IDLE
    //
    for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
      SetApState (&CpuMpData->CpuData[Index], CpuStateIdle);
    }
  }
  DEBUG ((DEBUG_INFO, "APIC MODE is %d\n", GetApicMode ()));
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
  @param[in]      ApTopOfStack     Top of AP stack

**/
VOID
InitializeApData (
  IN OUT CPU_MP_DATA      *CpuMpData,
  IN     UINTN            ProcessorNumber,
  IN     UINT32           BistData,
  IN     UINT64           ApTopOfStack
  )
{
  CPU_INFO_IN_HOB          *CpuInfoInHob;

  CpuInfoInHob = (CPU_INFO_IN_HOB *) (UINTN) CpuMpData->CpuInfoInHob;
  CpuInfoInHob[ProcessorNumber].InitialApicId = GetInitialApicId ();
  CpuInfoInHob[ProcessorNumber].ApicId        = GetApicId ();
  CpuInfoInHob[ProcessorNumber].Health        = BistData;
  CpuInfoInHob[ProcessorNumber].ApTopOfStack  = ApTopOfStack;

  CpuMpData->CpuData[ProcessorNumber].Waiting    = FALSE;
  CpuMpData->CpuData[ProcessorNumber].CpuHealthy = (BistData == 0) ? TRUE : FALSE;
  if (CpuInfoInHob[ProcessorNumber].InitialApicId >= 0xFF) {
    //
    // Set x2APIC mode if there are any logical processor reporting
    // an Initial APIC ID of 255 or greater.
    //
    AcquireSpinLock(&CpuMpData->MpLock);
    CpuMpData->X2ApicEnable = TRUE;
    ReleaseSpinLock(&CpuMpData->MpLock);
  }

  InitializeSpinLock(&CpuMpData->CpuData[ProcessorNumber].ApLock);
  SetApState (&CpuMpData->CpuData[ProcessorNumber], CpuStateIdle);
}

/**
  This function will be called from AP reset code if BSP uses WakeUpAP.

  @param[in] ExchangeInfo     Pointer to the MP exchange info buffer
  @param[in] ApIndex          Number of current executing AP
**/
VOID
EFIAPI
ApWakeupFunction (
  IN MP_CPU_EXCHANGE_INFO      *ExchangeInfo,
  IN UINTN                     ApIndex
  )
{
  CPU_MP_DATA                *CpuMpData;
  UINTN                      ProcessorNumber;
  EFI_AP_PROCEDURE           Procedure;
  VOID                       *Parameter;
  UINT32                     BistData;
  volatile UINT32            *ApStartupSignalBuffer;
  CPU_INFO_IN_HOB            *CpuInfoInHob;
  UINT64                     ApTopOfStack;
  UINTN                      CurrentApicMode;

  //
  // AP finished assembly code and begin to execute C code
  //
  CpuMpData = ExchangeInfo->CpuMpData;

  //
  // AP's local APIC settings will be lost after received INIT IPI
  // We need to re-initialize them at here
  //
  ProgramVirtualWireMode ();
  //
  // Mask the LINT0 and LINT1 so that AP doesn't enter the system timer interrupt handler.
  //
  DisableLvtInterrupts ();
  SyncLocalApicTimerSetting (CpuMpData);

  CurrentApicMode = GetApicMode ();
  while (TRUE) {
    if (CpuMpData->InitFlag == ApInitConfig) {
      //
      // Add CPU number
      //
      InterlockedIncrement ((UINT32 *) &CpuMpData->CpuCount);
      ProcessorNumber = ApIndex;
      //
      // This is first time AP wakeup, get BIST information from AP stack
      //
      ApTopOfStack  = CpuMpData->Buffer + (ProcessorNumber + 1) * CpuMpData->CpuApStackSize;
      BistData = *(UINT32 *) ((UINTN) ApTopOfStack - sizeof (UINTN));
      //
      // Do some AP initialize sync
      //
      ApInitializeSync (CpuMpData);
      //
      // CpuMpData->CpuData[0].VolatileRegisters is initialized based on BSP environment,
      //   to initialize AP in InitConfig path.
      // NOTE: IDTR.BASE stored in CpuMpData->CpuData[0].VolatileRegisters points to a different IDT shared by all APs.
      //
      RestoreVolatileRegisters (&CpuMpData->CpuData[0].VolatileRegisters, FALSE);
      InitializeApData (CpuMpData, ProcessorNumber, BistData, ApTopOfStack);
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
        (UINT32 *) ApStartupSignalBuffer,
        WAKEUP_AP_SIGNAL,
        0
        );
      if (CpuMpData->ApLoopMode == ApInHltLoop) {
        //
        // Restore AP's volatile registers saved
        //
        RestoreVolatileRegisters (&CpuMpData->CpuData[ProcessorNumber].VolatileRegisters, TRUE);
      } else {
        //
        // The CPU driver might not flush TLB for APs on spot after updating
        // page attributes. AP in mwait loop mode needs to take care of it when
        // woken up.
        //
        CpuFlushTlb ();
      }

      if (GetApState (&CpuMpData->CpuData[ProcessorNumber]) == CpuStateReady) {
        Procedure = (EFI_AP_PROCEDURE)CpuMpData->CpuData[ProcessorNumber].ApFunction;
        Parameter = (VOID *) CpuMpData->CpuData[ProcessorNumber].ApFunctionArgument;
        if (Procedure != NULL) {
          SetApState (&CpuMpData->CpuData[ProcessorNumber], CpuStateBusy);
          //
          // Enable source debugging on AP function
          //
          EnableDebugAgent ();
          //
          // Invoke AP function here
          //
          Procedure (Parameter);
          CpuInfoInHob = (CPU_INFO_IN_HOB *) (UINTN) CpuMpData->CpuInfoInHob;
          if (CpuMpData->SwitchBspFlag) {
            //
            // Re-get the processor number due to BSP/AP maybe exchange in AP function
            //
            GetProcessorNumber (CpuMpData, &ProcessorNumber);
            CpuMpData->CpuData[ProcessorNumber].ApFunction = 0;
            CpuMpData->CpuData[ProcessorNumber].ApFunctionArgument = 0;
            ApStartupSignalBuffer = CpuMpData->CpuData[ProcessorNumber].StartupApSignal;
            CpuInfoInHob[ProcessorNumber].ApTopOfStack = CpuInfoInHob[CpuMpData->NewBspNumber].ApTopOfStack;
          } else {
            if (CpuInfoInHob[ProcessorNumber].ApicId != GetApicId () ||
                CpuInfoInHob[ProcessorNumber].InitialApicId != GetInitialApicId ()) {
              if (CurrentApicMode != GetApicMode ()) {
                //
                // If APIC mode change happened during AP function execution,
                // we do not support APIC ID value changed.
                //
                ASSERT (FALSE);
                CpuDeadLoop ();
              } else {
                //
                // Re-get the CPU APICID and Initial APICID if they are changed
                //
                CpuInfoInHob[ProcessorNumber].ApicId        = GetApicId ();
                CpuInfoInHob[ProcessorNumber].InitialApicId = GetInitialApicId ();
              }
            }
          }
        }
        SetApState (&CpuMpData->CpuData[ProcessorNumber], CpuStateFinished);
      }
    }

    //
    // AP finished executing C code
    //
    InterlockedIncrement ((UINT32 *) &CpuMpData->FinishedCount);
    InterlockedDecrement ((UINT32 *) &CpuMpData->MpCpuExchangeInfo->NumApsExecuting);

    //
    // Place AP is specified loop mode
    //
    if (CpuMpData->ApLoopMode == ApInHltLoop) {
      //
      // Save AP volatile registers
      //
      SaveVolatileRegisters (&CpuMpData->CpuData[ProcessorNumber].VolatileRegisters);
      //
      // Place AP in HLT-loop
      //
      while (TRUE) {
        DisableInterrupts ();
        CpuSleep ();
        CpuPause ();
      }
    }
    while (TRUE) {
      DisableInterrupts ();
      if (CpuMpData->ApLoopMode == ApInMwaitLoop) {
        //
        // Place AP in MWAIT-loop
        //
        AsmMonitor ((UINTN) ApStartupSignalBuffer, 0, 0);
        if (*ApStartupSignalBuffer != WAKEUP_AP_SIGNAL) {
          //
          // Check AP start-up signal again.
          // If AP start-up signal is not set, place AP into
          // the specified C-state
          //
          AsmMwait (CpuMpData->ApTargetCState << 4, 0);
        }
      } else if (CpuMpData->ApLoopMode == ApInRunLoop) {
        //
        // Place AP in Run-loop
        //
        CpuPause ();
      } else {
        ASSERT (FALSE);
      }

      //
      // If AP start-up signal is written, AP is waken up
      // otherwise place AP in loop again
      //
      if (*ApStartupSignalBuffer == WAKEUP_AP_SIGNAL) {
        break;
      }
    }
  }
}

/**
  Wait for AP wakeup and write AP start-up signal till AP is waken up.

  @param[in] ApStartupSignalBuffer  Pointer to AP wakeup signal
**/
VOID
WaitApWakeup (
  IN volatile UINT32        *ApStartupSignalBuffer
  )
{
  //
  // If AP is waken up, StartupApSignal should be cleared.
  // Otherwise, write StartupApSignal again till AP waken up.
  //
  while (InterlockedCompareExchange32 (
          (UINT32 *) ApStartupSignalBuffer,
          WAKEUP_AP_SIGNAL,
          WAKEUP_AP_SIGNAL
          ) != 0) {
    CpuPause ();
  }
}

/**
  This function will fill the exchange info structure.

  @param[in] CpuMpData          Pointer to CPU MP Data

**/
VOID
FillExchangeInfoData (
  IN CPU_MP_DATA               *CpuMpData
  )
{
  volatile MP_CPU_EXCHANGE_INFO    *ExchangeInfo;
  UINTN                            Size;
  IA32_SEGMENT_DESCRIPTOR          *Selector;

  ExchangeInfo                  = CpuMpData->MpCpuExchangeInfo;
  ExchangeInfo->Lock            = 0;
  ExchangeInfo->StackStart      = CpuMpData->Buffer;
  ExchangeInfo->StackSize       = CpuMpData->CpuApStackSize;
  ExchangeInfo->BufferStart     = CpuMpData->WakeupBuffer;
  ExchangeInfo->ModeOffset      = CpuMpData->AddressMap.ModeEntryOffset;

  ExchangeInfo->CodeSegment     = AsmReadCs ();
  ExchangeInfo->DataSegment     = AsmReadDs ();

  ExchangeInfo->Cr3             = AsmReadCr3 ();

  ExchangeInfo->CFunction       = (UINTN) ApWakeupFunction;
  ExchangeInfo->ApIndex         = 0;
  ExchangeInfo->NumApsExecuting = 0;
  ExchangeInfo->InitFlag        = (UINTN) CpuMpData->InitFlag;
  ExchangeInfo->CpuInfo         = (CPU_INFO_IN_HOB *) (UINTN) CpuMpData->CpuInfoInHob;
  ExchangeInfo->CpuMpData       = CpuMpData;

  ExchangeInfo->EnableExecuteDisable = IsBspExecuteDisableEnabled ();

  ExchangeInfo->InitializeFloatingPointUnitsAddress = (UINTN)InitializeFloatingPointUnits;

  //
  // Get the BSP's data of GDT and IDT
  //
  AsmReadGdtr ((IA32_DESCRIPTOR *) &ExchangeInfo->GdtrProfile);
  AsmReadIdtr ((IA32_DESCRIPTOR *) &ExchangeInfo->IdtrProfile);

  //
  // Find a 32-bit code segment
  //
  Selector = (IA32_SEGMENT_DESCRIPTOR *)ExchangeInfo->GdtrProfile.Base;
  Size = ExchangeInfo->GdtrProfile.Limit + 1;
  while (Size > 0) {
    if (Selector->Bits.L == 0 && Selector->Bits.Type >= 8) {
      ExchangeInfo->ModeTransitionSegment =
        (UINT16)((UINTN)Selector - ExchangeInfo->GdtrProfile.Base);
      break;
    }
    Selector += 1;
    Size -= sizeof (IA32_SEGMENT_DESCRIPTOR);
  }

  //
  // Copy all 32-bit code and 64-bit code into memory with type of
  // EfiBootServicesCode to avoid page fault if NX memory protection is enabled.
  //
  if (CpuMpData->WakeupBufferHigh != 0) {
    Size = CpuMpData->AddressMap.RendezvousFunnelSize -
           CpuMpData->AddressMap.ModeTransitionOffset;
    CopyMem (
      (VOID *)CpuMpData->WakeupBufferHigh,
      CpuMpData->AddressMap.RendezvousFunnelAddress +
      CpuMpData->AddressMap.ModeTransitionOffset,
      Size
      );

    ExchangeInfo->ModeTransitionMemory = (UINT32)CpuMpData->WakeupBufferHigh;
  } else {
    ExchangeInfo->ModeTransitionMemory = (UINT32)
      (ExchangeInfo->BufferStart + CpuMpData->AddressMap.ModeTransitionOffset);
  }

  ExchangeInfo->ModeHighMemory = ExchangeInfo->ModeTransitionMemory +
                         (UINT32)ExchangeInfo->ModeOffset -
                         (UINT32)CpuMpData->AddressMap.ModeTransitionOffset;
  ExchangeInfo->ModeHighSegment = (UINT16)ExchangeInfo->CodeSegment;
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
  IN CPU_MP_DATA               *CpuMpData,
  IN UINT32                    FinishedApLimit,
  IN UINT32                    TimeLimit
  );

/**
  Get available system memory below 1MB by specified size.

  @param[in]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
BackupAndPrepareWakeupBuffer(
  IN CPU_MP_DATA              *CpuMpData
  )
{
  CopyMem (
    (VOID *) CpuMpData->BackupBuffer,
    (VOID *) CpuMpData->WakeupBuffer,
    CpuMpData->BackupBufferSize
    );
  CopyMem (
    (VOID *) CpuMpData->WakeupBuffer,
    (VOID *) CpuMpData->AddressMap.RendezvousFunnelAddress,
    CpuMpData->AddressMap.RendezvousFunnelSize
    );
}

/**
  Restore wakeup buffer data.

  @param[in]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
RestoreWakeupBuffer(
  IN CPU_MP_DATA              *CpuMpData
  )
{
  CopyMem (
    (VOID *) CpuMpData->WakeupBuffer,
    (VOID *) CpuMpData->BackupBuffer,
    CpuMpData->BackupBufferSize
    );
}

/**
  Allocate reset vector buffer.

  @param[in, out]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
AllocateResetVector (
  IN OUT CPU_MP_DATA          *CpuMpData
  )
{
  UINTN           ApResetVectorSize;

  if (CpuMpData->WakeupBuffer == (UINTN) -1) {
    ApResetVectorSize = CpuMpData->AddressMap.RendezvousFunnelSize +
                          sizeof (MP_CPU_EXCHANGE_INFO);

    CpuMpData->WakeupBuffer      = GetWakeupBuffer (ApResetVectorSize);
    CpuMpData->MpCpuExchangeInfo = (MP_CPU_EXCHANGE_INFO *) (UINTN)
                    (CpuMpData->WakeupBuffer + CpuMpData->AddressMap.RendezvousFunnelSize);
    CpuMpData->WakeupBufferHigh  = GetModeTransitionBuffer (
                                    CpuMpData->AddressMap.RendezvousFunnelSize -
                                    CpuMpData->AddressMap.ModeTransitionOffset
                                    );
  }
  BackupAndPrepareWakeupBuffer (CpuMpData);
}

/**
  Free AP reset vector buffer.

  @param[in]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
FreeResetVector (
  IN CPU_MP_DATA              *CpuMpData
  )
{
  RestoreWakeupBuffer (CpuMpData);
}

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
  IN CPU_MP_DATA               *CpuMpData,
  IN BOOLEAN                   Broadcast,
  IN UINTN                     ProcessorNumber,
  IN EFI_AP_PROCEDURE          Procedure,              OPTIONAL
  IN VOID                      *ProcedureArgument,     OPTIONAL
  IN BOOLEAN                   WakeUpDisabledAps
  )
{
  volatile MP_CPU_EXCHANGE_INFO    *ExchangeInfo;
  UINTN                            Index;
  CPU_AP_DATA                      *CpuData;
  BOOLEAN                          ResetVectorRequired;
  CPU_INFO_IN_HOB                  *CpuInfoInHob;

  CpuMpData->FinishedCount = 0;
  ResetVectorRequired = FALSE;

  if (CpuMpData->WakeUpByInitSipiSipi ||
      CpuMpData->InitFlag   != ApInitDone) {
    ResetVectorRequired = TRUE;
    AllocateResetVector (CpuMpData);
    FillExchangeInfoData (CpuMpData);
    SaveLocalApicTimerSetting (CpuMpData);
  }

  if (CpuMpData->ApLoopMode == ApInMwaitLoop) {
    //
    // Get AP target C-state each time when waking up AP,
    // for it maybe updated by platform again
    //
    CpuMpData->ApTargetCState = PcdGet8 (PcdCpuApTargetCstate);
  }

  ExchangeInfo = CpuMpData->MpCpuExchangeInfo;

  if (Broadcast) {
    for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
      if (Index != CpuMpData->BspNumber) {
        CpuData = &CpuMpData->CpuData[Index];
        //
        // All AP(include disabled AP) will be woke up by INIT-SIPI-SIPI, but
        // the AP procedure will be skipped for disabled AP because AP state
        // is not CpuStateReady.
        //
        if (GetApState (CpuData) == CpuStateDisabled && !WakeUpDisabledAps) {
          continue;
        }

        CpuData->ApFunction         = (UINTN) Procedure;
        CpuData->ApFunctionArgument = (UINTN) ProcedureArgument;
        SetApState (CpuData, CpuStateReady);
        if (CpuMpData->InitFlag != ApInitConfig) {
          *(UINT32 *) CpuData->StartupApSignal = WAKEUP_AP_SIGNAL;
        }
      }
    }
    if (ResetVectorRequired) {
      //
      // Wakeup all APs
      //
      SendInitSipiSipiAllExcludingSelf ((UINT32) ExchangeInfo->BufferStart);
    }
    if (CpuMpData->InitFlag == ApInitConfig) {
      //
      // Here support two methods to collect AP count through adjust
      // PcdCpuApInitTimeOutInMicroSeconds values.
      //
      // one way is set a value to just let the first AP to start the
      // initialization, then through the later while loop to wait all Aps
      // finsh the initialization.
      // The other way is set a value to let all APs finished the initialzation.
      // In this case, the later while loop is useless.
      //
      TimedWaitForApFinish (
        CpuMpData,
        PcdGet32 (PcdCpuMaxLogicalProcessorNumber) - 1,
        PcdGet32 (PcdCpuApInitTimeOutInMicroSeconds)
        );

      while (CpuMpData->MpCpuExchangeInfo->NumApsExecuting != 0) {
        CpuPause();
      }
    } else {
      //
      // Wait all APs waken up if this is not the 1st broadcast of SIPI
      //
      for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
        CpuData = &CpuMpData->CpuData[Index];
        if (Index != CpuMpData->BspNumber) {
          WaitApWakeup (CpuData->StartupApSignal);
        }
      }
    }
  } else {
    CpuData = &CpuMpData->CpuData[ProcessorNumber];
    CpuData->ApFunction         = (UINTN) Procedure;
    CpuData->ApFunctionArgument = (UINTN) ProcedureArgument;
    SetApState (CpuData, CpuStateReady);
    //
    // Wakeup specified AP
    //
    ASSERT (CpuMpData->InitFlag != ApInitConfig);
    *(UINT32 *) CpuData->StartupApSignal = WAKEUP_AP_SIGNAL;
    if (ResetVectorRequired) {
      CpuInfoInHob = (CPU_INFO_IN_HOB *) (UINTN) CpuMpData->CpuInfoInHob;
      SendInitSipiSipi (
        CpuInfoInHob[ProcessorNumber].ApicId,
        (UINT32) ExchangeInfo->BufferStart
        );
    }
    //
    // Wait specified AP waken up
    //
    WaitApWakeup (CpuData->StartupApSignal);
  }

  if (ResetVectorRequired) {
    FreeResetVector (CpuMpData);
  }

  //
  // After one round of Wakeup Ap actions, need to re-sync ApLoopMode with
  // WakeUpByInitSipiSipi flag. WakeUpByInitSipiSipi flag maybe changed by
  // S3SmmInitDone Ppi.
  //
  CpuMpData->WakeUpByInitSipiSipi = (CpuMpData->ApLoopMode == ApInHltLoop);
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
  UINT64 TimeoutInSeconds;
  UINT64 TimestampCounterFreq;

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
  CurrentTime = GetPerformanceCounter();
  Delta = (INT64) (CurrentTime - *PreviousTime);
  if (Start > End) {
    Delta = -Delta;
  }
  if (Delta < 0) {
    Delta += Cycle;
  }
  *TotalTime += Delta;
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
  IN CPU_MP_DATA               *CpuMpData,
  IN UINT32                    FinishedApLimit,
  IN UINT32                    TimeLimit
  )
{
  //
  // CalculateTimeout() and CheckTimeout() consider a TimeLimit of 0
  // "infinity", so check for (TimeLimit == 0) explicitly.
  //
  if (TimeLimit == 0) {
    return;
  }

  CpuMpData->TotalTime = 0;
  CpuMpData->ExpectedTime = CalculateTimeout (
                              TimeLimit,
                              &CpuMpData->CurrentTime
                              );
  while (CpuMpData->FinishedCount < FinishedApLimit &&
         !CheckTimeout (
            &CpuMpData->CurrentTime,
            &CpuMpData->TotalTime,
            CpuMpData->ExpectedTime
            )) {
    CpuPause ();
  }

  if (CpuMpData->FinishedCount >= FinishedApLimit) {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: reached FinishedApLimit=%u in %Lu microseconds\n",
      __FUNCTION__,
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
  Reset an AP to Idle state.

  Any task being executed by the AP will be aborted and the AP
  will be waiting for a new task in Wait-For-SIPI state.

  @param[in] ProcessorNumber  The handle number of processor.
**/
VOID
ResetProcessorToIdleState (
  IN UINTN                     ProcessorNumber
  )
{
  CPU_MP_DATA           *CpuMpData;

  CpuMpData = GetCpuMpData ();

  CpuMpData->InitFlag = ApInitReconfig;
  WakeUpAP (CpuMpData, FALSE, ProcessorNumber, NULL, NULL, TRUE);
  while (CpuMpData->FinishedCount < 1) {
    CpuPause ();
  }
  CpuMpData->InitFlag = ApInitDone;

  SetApState (&CpuMpData->CpuData[ProcessorNumber], CpuStateIdle);
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
  OUT UINTN                    *NextProcessorNumber
  )
{
  UINTN           ProcessorNumber;
  CPU_MP_DATA     *CpuMpData;

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
  IN UINTN        ProcessorNumber
  )
{
  CPU_MP_DATA     *CpuMpData;
  CPU_AP_DATA     *CpuData;

  CpuMpData = GetCpuMpData ();
  CpuData   = &CpuMpData->CpuData[ProcessorNumber];

  //
  //  Check the CPU state of AP. If it is CpuStateIdle, then the AP has finished its task.
  //  Only BSP and corresponding AP access this unit of CPU Data. This means the AP will not modify the
  //  value of state after setting the it to CpuStateIdle, so BSP can safely make use of its value.
  //
  //
  // If the AP finishes for StartupThisAP(), return EFI_SUCCESS.
  //
  if (GetApState(CpuData) == CpuStateFinished) {
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
      //
      // Reset failed AP to idle state
      //
      ResetProcessorToIdleState (ProcessorNumber);

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
  UINTN           ProcessorNumber;
  UINTN           NextProcessorNumber;
  UINTN           ListIndex;
  EFI_STATUS      Status;
  CPU_MP_DATA     *CpuMpData;
  CPU_AP_DATA     *CpuData;

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
    if (GetApState(CpuData) == CpuStateFinished) {
      CpuMpData->RunningCount --;
      CpuMpData->CpuData[ProcessorNumber].Waiting = FALSE;
      SetApState(CpuData, CpuStateIdle);

      //
      // If in Single Thread mode, then search for the next waiting AP for execution.
      //
      if (CpuMpData->SingleThread) {
        Status = GetNextWaitingProcessorNumber (&NextProcessorNumber);

        if (!EFI_ERROR (Status)) {
          WakeUpAP (
            CpuMpData,
            FALSE,
            (UINT32) NextProcessorNumber,
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
       CpuMpData->ExpectedTime)
       ) {
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
        //
        // Reset failed APs to idle state
        //
        ResetProcessorToIdleState (ProcessorNumber);
        CpuMpData->CpuData[ProcessorNumber].Waiting = FALSE;
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
  CPU_MP_DATA              *OldCpuMpData;
  CPU_INFO_IN_HOB          *CpuInfoInHob;
  UINT32                   MaxLogicalProcessorNumber;
  UINT32                   ApStackSize;
  MP_ASSEMBLY_ADDRESS_MAP  AddressMap;
  CPU_VOLATILE_REGISTERS   VolatileRegisters;
  UINTN                    BufferSize;
  UINT32                   MonitorFilterSize;
  VOID                     *MpBuffer;
  UINTN                    Buffer;
  CPU_MP_DATA              *CpuMpData;
  UINT8                    ApLoopMode;
  UINT8                    *MonitorBuffer;
  UINTN                    Index;
  UINTN                    ApResetVectorSize;
  UINTN                    BackupBufferAddr;
  UINTN                    ApIdtBase;
  VOID                     *MicrocodePatchInRam;

  OldCpuMpData = GetCpuMpDataFromGuidedHob ();
  if (OldCpuMpData == NULL) {
    MaxLogicalProcessorNumber = PcdGet32(PcdCpuMaxLogicalProcessorNumber);
  } else {
    MaxLogicalProcessorNumber = OldCpuMpData->CpuCount;
  }
  ASSERT (MaxLogicalProcessorNumber != 0);

  AsmGetAddressMap (&AddressMap);
  ApResetVectorSize = AddressMap.RendezvousFunnelSize + sizeof (MP_CPU_EXCHANGE_INFO);
  ApStackSize = PcdGet32(PcdCpuApStackSize);
  ApLoopMode  = GetApLoopMode (&MonitorFilterSize);

  //
  // Save BSP's Control registers for APs.
  //
  SaveVolatileRegisters (&VolatileRegisters);

  BufferSize  = ApStackSize * MaxLogicalProcessorNumber;
  BufferSize += MonitorFilterSize * MaxLogicalProcessorNumber;
  BufferSize += ApResetVectorSize;
  BufferSize  = ALIGN_VALUE (BufferSize, 8);
  BufferSize += VolatileRegisters.Idtr.Limit + 1;
  BufferSize += sizeof (CPU_MP_DATA);
  BufferSize += (sizeof (CPU_AP_DATA) + sizeof (CPU_INFO_IN_HOB))* MaxLogicalProcessorNumber;
  MpBuffer    = AllocatePages (EFI_SIZE_TO_PAGES (BufferSize));
  ASSERT (MpBuffer != NULL);
  ZeroMem (MpBuffer, BufferSize);
  Buffer = (UINTN) MpBuffer;

  //
  //  The layout of the Buffer is as below:
  //
  //    +--------------------+ <-- Buffer
  //        AP Stacks (N)
  //    +--------------------+ <-- MonitorBuffer
  //    AP Monitor Filters (N)
  //    +--------------------+ <-- BackupBufferAddr (CpuMpData->BackupBuffer)
  //         Backup Buffer
  //    +--------------------+
  //           Padding
  //    +--------------------+ <-- ApIdtBase (8-byte boundary)
  //           AP IDT          All APs share one separate IDT. So AP can get address of CPU_MP_DATA from IDT Base.
  //    +--------------------+ <-- CpuMpData
  //         CPU_MP_DATA
  //    +--------------------+ <-- CpuMpData->CpuData
  //        CPU_AP_DATA (N)
  //    +--------------------+ <-- CpuMpData->CpuInfoInHob
  //      CPU_INFO_IN_HOB (N)
  //    +--------------------+
  //
  MonitorBuffer    = (UINT8 *) (Buffer + ApStackSize * MaxLogicalProcessorNumber);
  BackupBufferAddr = (UINTN) MonitorBuffer + MonitorFilterSize * MaxLogicalProcessorNumber;
  ApIdtBase        = ALIGN_VALUE (BackupBufferAddr + ApResetVectorSize, 8);
  CpuMpData        = (CPU_MP_DATA *) (ApIdtBase + VolatileRegisters.Idtr.Limit + 1);
  CpuMpData->Buffer           = Buffer;
  CpuMpData->CpuApStackSize   = ApStackSize;
  CpuMpData->BackupBuffer     = BackupBufferAddr;
  CpuMpData->BackupBufferSize = ApResetVectorSize;
  CpuMpData->WakeupBuffer     = (UINTN) -1;
  CpuMpData->CpuCount         = 1;
  CpuMpData->BspNumber        = 0;
  CpuMpData->WaitEvent        = NULL;
  CpuMpData->SwitchBspFlag    = FALSE;
  CpuMpData->CpuData          = (CPU_AP_DATA *) (CpuMpData + 1);
  CpuMpData->CpuInfoInHob     = (UINT64) (UINTN) (CpuMpData->CpuData + MaxLogicalProcessorNumber);
  CpuMpData->MicrocodePatchRegionSize = PcdGet64 (PcdCpuMicrocodePatchRegionSize);
  //
  // If platform has more than one CPU, relocate microcode to memory to reduce
  // loading microcode time.
  //
  MicrocodePatchInRam = NULL;
  if (MaxLogicalProcessorNumber > 1) {
    MicrocodePatchInRam = AllocatePages (
                            EFI_SIZE_TO_PAGES (
                              (UINTN)CpuMpData->MicrocodePatchRegionSize
                              )
                            );
  }
  if (MicrocodePatchInRam == NULL) {
    //
    // there is only one processor, or no microcode patch is available, or
    // memory allocation failed
    //
    CpuMpData->MicrocodePatchAddress = PcdGet64 (PcdCpuMicrocodePatchAddress);
  } else {
    //
    // there are multiple processors, and a microcode patch is available, and
    // memory allocation succeeded
    //
    CopyMem (
      MicrocodePatchInRam,
      (VOID *)(UINTN)PcdGet64 (PcdCpuMicrocodePatchAddress),
      (UINTN)CpuMpData->MicrocodePatchRegionSize
      );
    CpuMpData->MicrocodePatchAddress = (UINTN)MicrocodePatchInRam;
  }

  InitializeSpinLock(&CpuMpData->MpLock);

  //
  // Make sure no memory usage outside of the allocated buffer.
  //
  ASSERT ((CpuMpData->CpuInfoInHob + sizeof (CPU_INFO_IN_HOB) * MaxLogicalProcessorNumber) ==
          Buffer + BufferSize);

  //
  // Duplicate BSP's IDT to APs.
  // All APs share one separate IDT. So AP can get the address of CpuMpData by using IDTR.BASE + IDTR.LIMIT + 1
  //
  CopyMem ((VOID *)ApIdtBase, (VOID *)VolatileRegisters.Idtr.Base, VolatileRegisters.Idtr.Limit + 1);
  VolatileRegisters.Idtr.Base = ApIdtBase;
  //
  // Don't pass BSP's TR to APs to avoid AP init failure.
  //
  VolatileRegisters.Tr = 0;
  CopyMem (&CpuMpData->CpuData[0].VolatileRegisters, &VolatileRegisters, sizeof (VolatileRegisters));
  //
  // Set BSP basic information
  //
  InitializeApData (CpuMpData, 0, 0, CpuMpData->Buffer + ApStackSize);
  //
  // Save assembly code information
  //
  CopyMem (&CpuMpData->AddressMap, &AddressMap, sizeof (MP_ASSEMBLY_ADDRESS_MAP));
  //
  // Finally set AP loop mode
  //
  CpuMpData->ApLoopMode = ApLoopMode;
  DEBUG ((DEBUG_INFO, "AP Loop Mode is %d\n", CpuMpData->ApLoopMode));

  CpuMpData->WakeUpByInitSipiSipi = (CpuMpData->ApLoopMode == ApInHltLoop);

  //
  // Set up APs wakeup signal buffer
  //
  for (Index = 0; Index < MaxLogicalProcessorNumber; Index++) {
    CpuMpData->CpuData[Index].StartupApSignal =
      (UINT32 *)(MonitorBuffer + MonitorFilterSize * Index);
  }
  //
  // Load Microcode on BSP
  //
  MicrocodeDetect (CpuMpData, TRUE);
  //
  // Store BSP's MTRR setting
  //
  MtrrGetAllMtrrs (&CpuMpData->MtrrTable);
  //
  // Enable the local APIC for Virtual Wire Mode.
  //
  ProgramVirtualWireMode ();

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
    CpuMpData->CpuCount  = OldCpuMpData->CpuCount;
    CpuMpData->BspNumber = OldCpuMpData->BspNumber;
    CpuMpData->InitFlag  = ApInitReconfig;
    CpuMpData->CpuInfoInHob = OldCpuMpData->CpuInfoInHob;
    CpuInfoInHob = (CPU_INFO_IN_HOB *) (UINTN) CpuMpData->CpuInfoInHob;
    for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
      InitializeSpinLock(&CpuMpData->CpuData[Index].ApLock);
      if (CpuInfoInHob[Index].InitialApicId >= 255 || Index > 254) {
        CpuMpData->X2ApicEnable = TRUE;
      }
      CpuMpData->CpuData[Index].CpuHealthy = (CpuInfoInHob[Index].Health == 0)? TRUE:FALSE;
      CpuMpData->CpuData[Index].ApFunction = 0;
      CopyMem (&CpuMpData->CpuData[Index].VolatileRegisters, &VolatileRegisters, sizeof (CPU_VOLATILE_REGISTERS));
    }
    if (MaxLogicalProcessorNumber > 1) {
      //
      // Wakeup APs to do some AP initialize sync
      //
      WakeUpAP (CpuMpData, TRUE, 0, ApInitializeSync, CpuMpData, TRUE);
      //
      // Wait for all APs finished initialization
      //
      while (CpuMpData->FinishedCount < (CpuMpData->CpuCount - 1)) {
        CpuPause ();
      }
      CpuMpData->InitFlag = ApInitDone;
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
  CPU_MP_DATA            *CpuMpData;
  UINTN                  CallerNumber;
  CPU_INFO_IN_HOB        *CpuInfoInHob;

  CpuMpData = GetCpuMpData ();
  CpuInfoInHob = (CPU_INFO_IN_HOB *) (UINTN) CpuMpData->CpuInfoInHob;

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

  ProcessorInfoBuffer->ProcessorId = (UINT64) CpuInfoInHob[ProcessorNumber].ApicId;
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

  //
  // Get processor location information
  //
  GetProcessorLocationByApicId (
    CpuInfoInHob[ProcessorNumber].ApicId,
    &ProcessorInfoBuffer->Location.Package,
    &ProcessorInfoBuffer->Location.Core,
    &ProcessorInfoBuffer->Location.Thread
    );

  if (HealthData != NULL) {
    HealthData->Uint32 = CpuInfoInHob[ProcessorNumber].Health;
  }

  return EFI_SUCCESS;
}

/**
  Worker function to switch the requested AP to be the BSP from that point onward.

  @param[in] ProcessorNumber   The handle number of AP that is to become the new BSP.
  @param[in] EnableOldBSP      If TRUE, then the old BSP will be listed as an
                               enabled AP. Otherwise, it will be disabled.

  @retval EFI_SUCCESS          BSP successfully switched.
  @retval others               Failed to switch BSP.

**/
EFI_STATUS
SwitchBSPWorker (
  IN UINTN                     ProcessorNumber,
  IN BOOLEAN                   EnableOldBSP
  )
{
  CPU_MP_DATA                  *CpuMpData;
  UINTN                        CallerNumber;
  CPU_STATE                    State;
  MSR_IA32_APIC_BASE_REGISTER  ApicBaseMsr;
  BOOLEAN                      OldInterruptState;
  BOOLEAN                      OldTimerInterruptState;

  //
  // Save and Disable Local APIC timer interrupt
  //
  OldTimerInterruptState = GetApicTimerInterruptState ();
  DisableApicTimerInterrupt ();
  //
  // Before send both BSP and AP to a procedure to exchange their roles,
  // interrupt must be disabled. This is because during the exchange role
  // process, 2 CPU may use 1 stack. If interrupt happens, the stack will
  // be corrupted, since interrupt return address will be pushed to stack
  // by hardware.
  //
  OldInterruptState = SaveAndDisableInterrupts ();

  //
  // Mask LINT0 & LINT1 for the old BSP
  //
  DisableLvtInterrupts ();

  CpuMpData = GetCpuMpData ();

  //
  // Check whether caller processor is BSP
  //
  MpInitLibWhoAmI (&CallerNumber);
  if (CallerNumber != CpuMpData->BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  if (ProcessorNumber >= CpuMpData->CpuCount) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether specified AP is disabled
  //
  State = GetApState (&CpuMpData->CpuData[ProcessorNumber]);
  if (State == CpuStateDisabled) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether ProcessorNumber specifies the current BSP
  //
  if (ProcessorNumber == CpuMpData->BspNumber) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether specified AP is busy
  //
  if (State == CpuStateBusy) {
    return EFI_NOT_READY;
  }

  CpuMpData->BSPInfo.State = CPU_SWITCH_STATE_IDLE;
  CpuMpData->APInfo.State  = CPU_SWITCH_STATE_IDLE;
  CpuMpData->SwitchBspFlag = TRUE;
  CpuMpData->NewBspNumber  = ProcessorNumber;

  //
  // Clear the BSP bit of MSR_IA32_APIC_BASE
  //
  ApicBaseMsr.Uint64 = AsmReadMsr64 (MSR_IA32_APIC_BASE);
  ApicBaseMsr.Bits.BSP = 0;
  AsmWriteMsr64 (MSR_IA32_APIC_BASE, ApicBaseMsr.Uint64);

  //
  // Need to wakeUp AP (future BSP).
  //
  WakeUpAP (CpuMpData, FALSE, ProcessorNumber, FutureBSPProc, CpuMpData, TRUE);

  AsmExchangeRole (&CpuMpData->BSPInfo, &CpuMpData->APInfo);

  //
  // Set the BSP bit of MSR_IA32_APIC_BASE on new BSP
  //
  ApicBaseMsr.Uint64 = AsmReadMsr64 (MSR_IA32_APIC_BASE);
  ApicBaseMsr.Bits.BSP = 1;
  AsmWriteMsr64 (MSR_IA32_APIC_BASE, ApicBaseMsr.Uint64);
  ProgramVirtualWireMode ();

  //
  // Wait for old BSP finished AP task
  //
  while (GetApState (&CpuMpData->CpuData[CallerNumber]) != CpuStateFinished) {
    CpuPause ();
  }

  CpuMpData->SwitchBspFlag = FALSE;
  //
  // Set old BSP enable state
  //
  if (!EnableOldBSP) {
    SetApState (&CpuMpData->CpuData[CallerNumber], CpuStateDisabled);
  } else {
    SetApState (&CpuMpData->CpuData[CallerNumber], CpuStateIdle);
  }
  //
  // Save new BSP number
  //
  CpuMpData->BspNumber = (UINT32) ProcessorNumber;

  //
  // Restore interrupt state.
  //
  SetInterruptState (OldInterruptState);

  if (OldTimerInterruptState) {
    EnableApicTimerInterrupt ();
  }

  return EFI_SUCCESS;
}

/**
  Worker function to let the caller enable or disable an AP from this point onward.
  This service may only be called from the BSP.

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
  IN  UINTN                     ProcessorNumber,
  IN  BOOLEAN                   EnableAP,
  IN  UINT32                    *HealthFlag OPTIONAL
  )
{
  CPU_MP_DATA               *CpuMpData;
  UINTN                     CallerNumber;

  CpuMpData = GetCpuMpData ();

  //
  // Check whether caller processor is BSP
  //
  MpInitLibWhoAmI (&CallerNumber);
  if (CallerNumber != CpuMpData->BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  if (ProcessorNumber == CpuMpData->BspNumber) {
    return EFI_INVALID_PARAMETER;
  }

  if (ProcessorNumber >= CpuMpData->CpuCount) {
    return EFI_NOT_FOUND;
  }

  if (!EnableAP) {
    SetApState (&CpuMpData->CpuData[ProcessorNumber], CpuStateDisabled);
  } else {
    ResetProcessorToIdleState (ProcessorNumber);
  }

  if (HealthFlag != NULL) {
    CpuMpData->CpuData[ProcessorNumber].CpuHealthy =
          (BOOLEAN) ((*HealthFlag & PROCESSOR_HEALTH_STATUS_BIT) != 0);
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
  OUT UINTN                    *ProcessorNumber
  )
{
  CPU_MP_DATA           *CpuMpData;

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
  OUT UINTN                     *NumberOfProcessors,       OPTIONAL
  OUT UINTN                     *NumberOfEnabledProcessors OPTIONAL
  )
{
  CPU_MP_DATA             *CpuMpData;
  UINTN                   CallerNumber;
  UINTN                   ProcessorNumber;
  UINTN                   EnabledProcessorNumber;
  UINTN                   Index;

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
      EnabledProcessorNumber ++;
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
StartupAllAPsWorker (
  IN  EFI_AP_PROCEDURE          Procedure,
  IN  BOOLEAN                   SingleThread,
  IN  EFI_EVENT                 WaitEvent               OPTIONAL,
  IN  UINTN                     TimeoutInMicroseconds,
  IN  VOID                      *ProcedureArgument      OPTIONAL,
  OUT UINTN                     **FailedCpuList         OPTIONAL
  )
{
  EFI_STATUS              Status;
  CPU_MP_DATA             *CpuMpData;
  UINTN                   ProcessorCount;
  UINTN                   ProcessorNumber;
  UINTN                   CallerNumber;
  CPU_AP_DATA             *CpuData;
  BOOLEAN                 HasEnabledAp;
  CPU_STATE               ApState;

  CpuMpData = GetCpuMpData ();

  if (FailedCpuList != NULL) {
    *FailedCpuList = NULL;
  }

  if (CpuMpData->CpuCount == 1) {
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

  if (!HasEnabledAp) {
    //
    // If no enabled AP exists, return EFI_NOT_STARTED.
    //
    return EFI_NOT_STARTED;
  }

  CpuMpData->RunningCount = 0;
  for (ProcessorNumber = 0; ProcessorNumber < ProcessorCount; ProcessorNumber++) {
    CpuData = &CpuMpData->CpuData[ProcessorNumber];
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
  CpuMpData->FailedCpuList = FailedCpuList;
  CpuMpData->ExpectedTime  = CalculateTimeout (
                               TimeoutInMicroseconds,
                               &CpuMpData->CurrentTime
                               );
  CpuMpData->TotalTime     = 0;
  CpuMpData->WaitEvent     = WaitEvent;

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
  IN  EFI_AP_PROCEDURE          Procedure,
  IN  UINTN                     ProcessorNumber,
  IN  EFI_EVENT                 WaitEvent               OPTIONAL,
  IN  UINTN                     TimeoutInMicroseconds,
  IN  VOID                      *ProcedureArgument      OPTIONAL,
  OUT BOOLEAN                   *Finished               OPTIONAL
  )
{
  EFI_STATUS              Status;
  CPU_MP_DATA             *CpuMpData;
  CPU_AP_DATA             *CpuData;
  UINTN                   CallerNumber;

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

  //
  // If WaitEvent is not NULL, execute in non-blocking mode.
  // BSP saves data for CheckAPsStatus(), and returns EFI_SUCCESS.
  // CheckAPsStatus() will check completion and timeout periodically.
  //
  CpuData = &CpuMpData->CpuData[ProcessorNumber];
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
      Status = CheckThisAP (ProcessorNumber);
    } while (Status == EFI_NOT_READY);
  }

  return Status;
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
  EFI_HOB_GUID_TYPE       *GuidHob;
  VOID                    *DataInHob;
  CPU_MP_DATA             *CpuMpData;

  CpuMpData = NULL;
  GuidHob = GetFirstGuidHob (&mCpuInitMpLibHobGuid);
  if (GuidHob != NULL) {
    DataInHob = GET_GUID_HOB_DATA (GuidHob);
    CpuMpData = (CPU_MP_DATA *) (*(UINTN *) DataInHob);
  }
  return CpuMpData;
}


/** @file
  CPU MP Initialize Library common functions.

  Copyright (c) 2016 - 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2020 - 2024, AMD Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"
#include <Library/CcExitLib.h>
#include <Register/Amd/SevSnpMsr.h>
#include <Register/Amd/Ghcb.h>

EFI_GUID  mCpuInitMpLibHobGuid = CPU_INIT_MP_LIB_HOB_GUID;
EFI_GUID  mMpHandOffGuid       = MP_HANDOFF_GUID;
EFI_GUID  mMpHandOffConfigGuid = MP_HANDOFF_CONFIG_GUID;

RELOCATE_AP_LOOP_ENTRY  mReservedApLoop;
UINTN                   mReservedTopOfApStack;
volatile UINT32         mNumberToFinish = 0;
UINTN                   mApPageTable;

/**
  Save the volatile registers required to be restored following INIT IPI.

  @param[out]  VolatileRegisters    Returns buffer saved the volatile resisters
**/
VOID
SaveVolatileRegisters (
  OUT CPU_VOLATILE_REGISTERS  *VolatileRegisters
  );

/**
  Restore the volatile registers following INIT IPI.

  @param[in]  VolatileRegisters   Pointer to volatile resisters
  @param[in]  IsRestoreDr         TRUE:  Restore DRx if supported
                                  FALSE: Do not restore DRx
**/
VOID
RestoreVolatileRegisters (
  IN CPU_VOLATILE_REGISTERS  *VolatileRegisters
  );

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

  Enabled   = FALSE;
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
  IN  VOID  *Buffer
  )
{
  CPU_MP_DATA  *DataInHob;

  DataInHob = (CPU_MP_DATA *)Buffer;
  //
  // Save and restore volatile registers when switch BSP
  //
  SaveVolatileRegisters (&DataInHob->APInfo.VolatileRegisters);
  AsmExchangeRole (&DataInHob->APInfo, &DataInHob->BSPInfo);
  RestoreVolatileRegisters (&DataInHob->APInfo.VolatileRegisters);
}

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
  Save BSP's local APIC timer setting.

  @param[in] CpuMpData          Pointer to CPU MP Data
**/
VOID
SaveLocalApicTimerSetting (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  CpuMpData->InitTimerCount = GetApicTimerInitCount ();
  if (CpuMpData->InitTimerCount != 0) {
    //
    // Record the current local APIC timer setting of BSP
    //
    GetApicTimerState (
      &CpuMpData->DivideValue,
      &CpuMpData->PeriodicMode,
      &CpuMpData->Vector
      );

    CpuMpData->TimerInterruptState = GetApicTimerInterruptState ();
  }
}

/**
  Sync local APIC timer setting from BSP to AP.

  @param[in] CpuMpData          Pointer to CPU MP Data
**/
VOID
SyncLocalApicTimerSetting (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  if (CpuMpData->InitTimerCount != 0) {
    //
    // Sync local APIC timer setting from BSP to AP
    //
    InitializeApicTimer (
      CpuMpData->DivideValue,
      CpuMpData->InitTimerCount,
      CpuMpData->PeriodicMode,
      CpuMpData->Vector
      );
    //
    // Disable AP's local APIC timer interrupt
    //
    DisableApicTimerInterrupt ();
  }
}

/**
  Save the volatile registers required to be restored following INIT IPI.

  @param[out]  VolatileRegisters    Returns buffer saved the volatile resisters
**/
VOID
SaveVolatileRegisters (
  OUT CPU_VOLATILE_REGISTERS  *VolatileRegisters
  )
{
  CPUID_VERSION_INFO_EDX  VersionInfoEdx;

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

**/
VOID
RestoreVolatileRegisters (
  IN CPU_VOLATILE_REGISTERS  *VolatileRegisters
  )
{
  CPUID_VERSION_INFO_EDX  VersionInfoEdx;
  IA32_TSS_DESCRIPTOR     *Tss;

  AsmWriteCr3 (VolatileRegisters->Cr3);
  AsmWriteCr4 (VolatileRegisters->Cr4);
  AsmWriteCr0 (VolatileRegisters->Cr0);

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

  AsmWriteGdtr (&VolatileRegisters->Gdtr);
  AsmWriteIdtr (&VolatileRegisters->Idtr);
  if ((VolatileRegisters->Tr != 0) &&
      (VolatileRegisters->Tr < VolatileRegisters->Gdtr.Limit))
  {
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
  CPUID_VERSION_INFO_ECX  VersionInfoEcx;

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
  OUT UINT32  *MonitorFilterSize
  )
{
  UINT8                    ApLoopMode;
  CPUID_MONITOR_MWAIT_EBX  MonitorMwaitEbx;

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

    if (ConfidentialComputingGuestHas (CCAttrAmdSevEs) &&
        !ConfidentialComputingGuestHas (CCAttrAmdSevSnp))
    {
      //
      // For SEV-ES (SEV-SNP is also considered SEV-ES), force AP in Hlt-loop
      // mode in order to use the GHCB protocol for starting APs
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
  IN CPU_MP_DATA  *CpuMpData
  )
{
  UINTN            Index1;
  UINTN            Index2;
  UINTN            Index3;
  UINT32           ApicId;
  CPU_INFO_IN_HOB  CpuInfo;
  CPU_AP_DATA      CpuApData;
  UINT32           ApCount;
  CPU_INFO_IN_HOB  *CpuInfoInHob;

  ApCount      = CpuMpData->CpuCount - 1;
  CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;
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

        CopyMem (&CpuApData, &CpuMpData->CpuData[Index3], sizeof (CPU_AP_DATA));
        CopyMem (
          &CpuMpData->CpuData[Index3],
          &CpuMpData->CpuData[Index1],
          sizeof (CPU_AP_DATA)
          );
        CopyMem (&CpuMpData->CpuData[Index1], &CpuApData, sizeof (CPU_AP_DATA));
      }
    }

    //
    // Get the processor number for the BSP
    //
    ApicId = GetInitialApicId ();
    for (Index1 = 0; Index1 < CpuMpData->CpuCount; Index1++) {
      if (CpuInfoInHob[Index1].ApicId == ApicId) {
        CpuMpData->BspNumber = (UINT32)Index1;
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
  UINTN        ProcessorNumber;
  EFI_STATUS   Status;

  CpuMpData = (CPU_MP_DATA *)Buffer;
  Status    = GetProcessorNumber (CpuMpData, &ProcessorNumber);
  ASSERT_EFI_ERROR (Status);
  //
  // Load microcode on AP
  //
  MicrocodeDetect (CpuMpData, ProcessorNumber);
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
  IN CPU_MP_DATA  *CpuMpData,
  OUT UINTN       *ProcessorNumber
  )
{
  UINTN            TotalProcessorNumber;
  UINTN            Index;
  CPU_INFO_IN_HOB  *CpuInfoInHob;
  UINT32           CurrentApicId;

  CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;

  TotalProcessorNumber = CpuMpData->CpuCount;
  CurrentApicId        = GetApicId ();
  for (Index = 0; Index < TotalProcessorNumber; Index++) {
    if (CpuInfoInHob[Index].ApicId == CurrentApicId) {
      *ProcessorNumber = Index;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Enable x2APIC mode if
  1. Number of CPU is greater than 255; or
  2. There are any logical processors reporting an Initial APIC ID of 255 or greater.

  @param[in] CpuMpData        Pointer to PEI CPU MP Data
**/
VOID
AutoEnableX2Apic (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  BOOLEAN          X2Apic;
  UINTN            Index;
  CPU_INFO_IN_HOB  *CpuInfoInHob;

  //
  // Enable x2APIC mode if
  //  1. Number of CPU is greater than 255; or
  //  2. There are any logical processors reporting an Initial APIC ID of 255 or greater.
  //
  X2Apic = FALSE;
  if (CpuMpData->CpuCount > 255) {
    //
    // If there are more than 255 processor found, force to enable X2APIC
    //
    X2Apic = TRUE;
  } else {
    CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;
    for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
      if (CpuInfoInHob[Index].InitialApicId >= 0xFF) {
        X2Apic = TRUE;
        break;
      }
    }
  }

  if (X2Apic) {
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
  //
  // Send 1st broadcast IPI to APs to wakeup APs
  //
  CpuMpData->InitFlag = ApInitConfig;
  WakeUpAP (CpuMpData, TRUE, 0, NULL, NULL, TRUE);
  CpuMpData->InitFlag = ApInitDone;
  //
  // When InitFlag == ApInitConfig, WakeUpAP () guarantees all APs are checked in.
  // FinishedCount is the number of check-in APs.
  //
  CpuMpData->CpuCount = CpuMpData->FinishedCount + 1;
  ASSERT (CpuMpData->CpuCount <= PcdGet32 (PcdCpuMaxLogicalProcessorNumber));

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
  IN OUT CPU_MP_DATA  *CpuMpData,
  IN     UINTN        ProcessorNumber,
  IN     UINT32       BistData,
  IN     UINT64       ApTopOfStack
  )
{
  CPU_INFO_IN_HOB                *CpuInfoInHob;
  MSR_IA32_PLATFORM_ID_REGISTER  PlatformIdMsr;
  AP_STACK_DATA                  *ApStackData;

  CpuInfoInHob                                = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;
  CpuInfoInHob[ProcessorNumber].InitialApicId = GetInitialApicId ();
  CpuInfoInHob[ProcessorNumber].ApicId        = GetApicId ();
  CpuInfoInHob[ProcessorNumber].Health        = BistData;
  CpuInfoInHob[ProcessorNumber].ApTopOfStack  = ApTopOfStack;

  //
  // AP_STACK_DATA is stored at the top of AP Stack
  //
  ApStackData         = (AP_STACK_DATA *)((UINTN)ApTopOfStack - sizeof (AP_STACK_DATA));
  ApStackData->MpData = CpuMpData;

  CpuMpData->CpuData[ProcessorNumber].Waiting    = FALSE;
  CpuMpData->CpuData[ProcessorNumber].CpuHealthy = (BistData == 0) ? TRUE : FALSE;

  //
  // NOTE: PlatformId is not relevant on AMD platforms.
  //
  if (!StandardSignatureIsAuthenticAMD ()) {
    PlatformIdMsr.Uint64                           = AsmReadMsr64 (MSR_IA32_PLATFORM_ID);
    CpuMpData->CpuData[ProcessorNumber].PlatformId = (UINT8)PlatformIdMsr.Bits.PlatformId;
  }

  AsmCpuid (
    CPUID_VERSION_INFO,
    &CpuMpData->CpuData[ProcessorNumber].ProcessorSignature,
    NULL,
    NULL,
    NULL
    );

  InitializeSpinLock (&CpuMpData->CpuData[ProcessorNumber].ApLock);
  SetApState (&CpuMpData->CpuData[ProcessorNumber], CpuStateIdle);
}

/**
  This function place APs in Halt loop.

  @param[in] CpuMpData        Pointer to CPU MP Data
**/
VOID
PlaceAPInHltLoop (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  while (TRUE) {
    DisableInterrupts ();
    if (CpuMpData->UseSevEsAPMethod) {
      SevEsPlaceApHlt (CpuMpData);
    } else {
      CpuSleep ();
    }

    CpuPause ();
  }
}

/**
  This function place APs in Mwait or Run loop.

  @param[in] ApLoopMode                   Ap Loop Mode
  @param[in] ApStartupSignalBuffer        Pointer to Ap Startup Signal Buffer
  @param[in] ApTargetCState               Ap Target CState
**/
VOID
PlaceAPInMwaitLoopOrRunLoop (
  IN UINT8            ApLoopMode,
  IN volatile UINT32  *ApStartupSignalBuffer,
  IN UINT8            ApTargetCState
  )
{
  while (TRUE) {
    DisableInterrupts ();
    if (ApLoopMode == ApInMwaitLoop) {
      //
      // Place AP in MWAIT-loop
      //
      AsmMonitor ((UINTN)ApStartupSignalBuffer, 0, 0);
      if ((*ApStartupSignalBuffer != WAKEUP_AP_SIGNAL) && (*ApStartupSignalBuffer != MP_HAND_OFF_SIGNAL)) {
        //
        // Check AP start-up signal again.
        // If AP start-up signal is not set, place AP into
        // the specified C-state
        //
        AsmMwait (ApTargetCState << 4, 0);
      }
    } else if (ApLoopMode == ApInRunLoop) {
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
    if ((*ApStartupSignalBuffer == WAKEUP_AP_SIGNAL) || (*ApStartupSignalBuffer == MP_HAND_OFF_SIGNAL)) {
      break;
    }
  }
}

/**
  This function will be called from AP reset code if BSP uses WakeUpAP.

  @param[in] CpuMpData        Pointer to CPU MP Data
  @param[in] ApIndex          Number of current executing AP
**/
VOID
EFIAPI
ApWakeupFunction (
  IN CPU_MP_DATA  *CpuMpData,
  IN UINTN        ApIndex
  )
{
  UINTN             ProcessorNumber;
  EFI_AP_PROCEDURE  Procedure;
  VOID              *Parameter;
  UINT32            BistData;
  volatile UINT32   *ApStartupSignalBuffer;
  CPU_INFO_IN_HOB   *CpuInfoInHob;
  UINT64            ApTopOfStack;
  UINTN             CurrentApicMode;
  AP_STACK_DATA     *ApStackData;
  UINT32            OriginalValue;

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
      // Synchronize APIC mode with BSP in the first time AP wakeup ONLY.
      //
      SetApicMode (CpuMpData->InitialBspApicMode);
      CurrentApicMode = CpuMpData->InitialBspApicMode;

      ProcessorNumber = ApIndex;
      //
      // This is first time AP wakeup, get BIST information from AP stack
      //
      ApTopOfStack = CpuMpData->Buffer + (ProcessorNumber + 1) * CpuMpData->CpuApStackSize;
      ApStackData  = (AP_STACK_DATA *)((UINTN)ApTopOfStack - sizeof (AP_STACK_DATA));
      BistData     = (UINT32)ApStackData->Bist;

      //
      // CpuMpData->CpuData[ProcessorNumber].VolatileRegisters is initialized based on BSP environment,
      //   to initialize AP in InitConfig path.
      // NOTE: IDTR.BASE stored in CpuMpData->CpuData[ProcessorNumber].VolatileRegisters points to a different IDT shared by all APs.
      //
      RestoreVolatileRegisters (&CpuMpData->CpuData[ProcessorNumber].VolatileRegisters);
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
      OriginalValue         = InterlockedCompareExchange32 (
                                (UINT32 *)ApStartupSignalBuffer,
                                MP_HAND_OFF_SIGNAL,
                                0
                                );
      if (OriginalValue == MP_HAND_OFF_SIGNAL) {
        SetApState (&CpuMpData->CpuData[ProcessorNumber], CpuStateReady);
      }

      InterlockedCompareExchange32 (
        (UINT32 *)ApStartupSignalBuffer,
        WAKEUP_AP_SIGNAL,
        0
        );

      RestoreVolatileRegisters (&CpuMpData->CpuData[ProcessorNumber].VolatileRegisters);

      if (GetApState (&CpuMpData->CpuData[ProcessorNumber]) == CpuStateReady) {
        Procedure = (EFI_AP_PROCEDURE)CpuMpData->CpuData[ProcessorNumber].ApFunction;
        Parameter = (VOID *)CpuMpData->CpuData[ProcessorNumber].ApFunctionArgument;
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
          CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;
          if (CpuMpData->SwitchBspFlag) {
            //
            // Re-get the processor number due to BSP/AP maybe exchange in AP function
            //
            GetProcessorNumber (CpuMpData, &ProcessorNumber);
            CpuMpData->CpuData[ProcessorNumber].ApFunction         = 0;
            CpuMpData->CpuData[ProcessorNumber].ApFunctionArgument = 0;
            ApStartupSignalBuffer                                  = CpuMpData->CpuData[ProcessorNumber].StartupApSignal;
            CpuInfoInHob[ProcessorNumber].ApTopOfStack             = CpuInfoInHob[CpuMpData->NewBspNumber].ApTopOfStack;
          } else {
            if ((CpuInfoInHob[ProcessorNumber].ApicId != GetApicId ()) ||
                (CpuInfoInHob[ProcessorNumber].InitialApicId != GetInitialApicId ()))
            {
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

    SaveVolatileRegisters (&CpuMpData->CpuData[ProcessorNumber].VolatileRegisters);

    //
    // AP finished executing C code
    //
    InterlockedIncrement ((UINT32 *)&CpuMpData->FinishedCount);

    if (CpuMpData->InitFlag == ApInitConfig) {
      //
      // Delay decrementing the APs executing count when SEV-ES is enabled
      // to allow the APs to issue an AP_RESET_HOLD before the BSP possibly
      // performs another INIT-SIPI-SIPI sequence.
      //
      if (!CpuMpData->UseSevEsAPMethod) {
        InterlockedDecrement ((UINT32 *)&CpuMpData->MpCpuExchangeInfo->NumApsExecuting);
      }
    }

    //
    // Place AP is specified loop mode
    //
    if (CpuMpData->ApLoopMode == ApInHltLoop) {
      PlaceAPInHltLoop (CpuMpData);
      //
      // Never run here
      //
    } else {
      PlaceAPInMwaitLoopOrRunLoop (CpuMpData->ApLoopMode, ApStartupSignalBuffer, CpuMpData->ApTargetCState);
    }
  }
}

/**
  This function serves as the entry point for APs when
  they are awakened by the stores in the memory address
  indicated by the MP_HANDOFF_INFO structure.

  @param[in] CpuMpData        Pointer to PEI CPU MP Data
**/
VOID
EFIAPI
DxeApEntryPoint (
  CPU_MP_DATA  *CpuMpData
  )
{
  UINTN                   ProcessorNumber;
  MSR_IA32_EFER_REGISTER  EferMsr;

  GetProcessorNumber (CpuMpData, &ProcessorNumber);
  if (CpuMpData->EnableExecuteDisableForSwitchContext) {
    EferMsr.Uint64   = AsmReadMsr64 (MSR_IA32_EFER);
    EferMsr.Bits.NXE = 1;
    AsmWriteMsr64 (MSR_IA32_EFER, EferMsr.Uint64);
  }

  RestoreVolatileRegisters (&CpuMpData->CpuData[ProcessorNumber].VolatileRegisters);
  InterlockedIncrement ((UINT32 *)&CpuMpData->FinishedCount);
  PlaceAPInMwaitLoopOrRunLoop (
    CpuMpData->ApLoopMode,
    CpuMpData->CpuData[ProcessorNumber].StartupApSignal,
    CpuMpData->ApTargetCState
    );
  ApWakeupFunction (CpuMpData, ProcessorNumber);
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
  Calculate the size of the reset vector.

  @param[in]  AddressMap   The pointer to Address Map structure.
  @param[out] SizeBelow1Mb Return the size of below 1MB memory for AP reset area.
  @param[out] SizeAbove1Mb Return the size of abvoe 1MB memory for AP reset area.
**/
STATIC
VOID
GetApResetVectorSize (
  IN  MP_ASSEMBLY_ADDRESS_MAP  *AddressMap,
  OUT UINTN                    *SizeBelow1Mb OPTIONAL,
  OUT UINTN                    *SizeAbove1Mb OPTIONAL
  )
{
  if (SizeBelow1Mb != NULL) {
    *SizeBelow1Mb = ALIGN_VALUE (AddressMap->ModeTransitionOffset, sizeof (UINTN)) + sizeof (MP_CPU_EXCHANGE_INFO);
  }

  if (SizeAbove1Mb != NULL) {
    *SizeAbove1Mb = AddressMap->RendezvousFunnelSize - AddressMap->ModeTransitionOffset;
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
  UINTN                          Size;
  IA32_SEGMENT_DESCRIPTOR        *Selector;
  IA32_CR4                       Cr4;

  ExchangeInfo              = CpuMpData->MpCpuExchangeInfo;
  ExchangeInfo->StackStart  = CpuMpData->Buffer;
  ExchangeInfo->StackSize   = CpuMpData->CpuApStackSize;
  ExchangeInfo->BufferStart = CpuMpData->WakeupBuffer;
  ExchangeInfo->ModeOffset  = CpuMpData->AddressMap.ModeEntryOffset;

  ExchangeInfo->CodeSegment = AsmReadCs ();
  ExchangeInfo->DataSegment = AsmReadDs ();

  ExchangeInfo->Cr3 = AsmReadCr3 ();

  ExchangeInfo->CFunction       = (UINTN)ApWakeupFunction;
  ExchangeInfo->ApIndex         = 0;
  ExchangeInfo->NumApsExecuting = 0;
  ExchangeInfo->InitFlag        = (UINTN)CpuMpData->InitFlag;
  ExchangeInfo->CpuInfo         = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;
  ExchangeInfo->CpuMpData       = CpuMpData;

  ExchangeInfo->EnableExecuteDisable = IsBspExecuteDisableEnabled ();

  ExchangeInfo->InitializeFloatingPointUnitsAddress = (UINTN)InitializeFloatingPointUnits;

  //
  // We can check either CPUID(7).ECX[bit16] or check CR4.LA57[bit12]
  //  to determin whether 5-Level Paging is enabled.
  // CPUID(7).ECX[bit16] shows CPU's capability, CR4.LA57[bit12] shows
  // current system setting.
  // Using latter way is simpler because it also eliminates the needs to
  //  check whether platform wants to enable it.
  //
  Cr4.UintN                        = AsmReadCr4 ();
  ExchangeInfo->Enable5LevelPaging = (BOOLEAN)(Cr4.Bits.LA57 == 1);
  DEBUG ((DEBUG_INFO, "%a: 5-Level Paging = %d\n", gEfiCallerBaseName, ExchangeInfo->Enable5LevelPaging));

  ExchangeInfo->SevEsIsEnabled  = CpuMpData->SevEsIsEnabled;
  ExchangeInfo->SevSnpIsEnabled = CpuMpData->SevSnpIsEnabled;
  ExchangeInfo->GhcbBase        = (UINTN)CpuMpData->GhcbBase;

  //
  // Populate SEV-ES specific exchange data.
  //
  if (ExchangeInfo->SevSnpIsEnabled) {
    FillExchangeInfoDataSevEs (ExchangeInfo);
  }

  //
  // Get the BSP's data of GDT and IDT
  //
  AsmReadGdtr ((IA32_DESCRIPTOR *)&ExchangeInfo->GdtrProfile);
  AsmReadIdtr ((IA32_DESCRIPTOR *)&ExchangeInfo->IdtrProfile);

  //
  // Find a 32-bit code segment
  //
  Selector = (IA32_SEGMENT_DESCRIPTOR *)ExchangeInfo->GdtrProfile.Base;
  Size     = ExchangeInfo->GdtrProfile.Limit + 1;
  while (Size > 0) {
    if ((Selector->Bits.L == 0) && (Selector->Bits.Type >= 8)) {
      ExchangeInfo->ModeTransitionSegment =
        (UINT16)((UINTN)Selector - ExchangeInfo->GdtrProfile.Base);
      break;
    }

    Selector += 1;
    Size     -= sizeof (IA32_SEGMENT_DESCRIPTOR);
  }

  ExchangeInfo->ModeTransitionMemory = (UINT32)CpuMpData->WakeupBufferHigh;

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
  IN CPU_MP_DATA  *CpuMpData,
  IN UINT32       FinishedApLimit,
  IN UINT32       TimeLimit
  );

/**
  Get available system memory below 1MB by specified size.

  @param[in]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
BackupAndPrepareWakeupBuffer (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  CopyMem (
    (VOID *)CpuMpData->BackupBuffer,
    (VOID *)CpuMpData->WakeupBuffer,
    CpuMpData->BackupBufferSize
    );
  CopyMem (
    (VOID *)CpuMpData->WakeupBuffer,
    (VOID *)CpuMpData->AddressMap.RendezvousFunnelAddress,
    CpuMpData->AddressMap.ModeTransitionOffset
    );
}

/**
  Restore wakeup buffer data.

  @param[in]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
RestoreWakeupBuffer (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  CopyMem (
    (VOID *)CpuMpData->WakeupBuffer,
    (VOID *)CpuMpData->BackupBuffer,
    CpuMpData->BackupBufferSize
    );
}

/**
  Allocate reset vector buffer.

  @param[in, out]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
AllocateResetVectorBelow1Mb (
  IN OUT CPU_MP_DATA  *CpuMpData
  )
{
  UINTN  ApResetStackSize;

  if (CpuMpData->WakeupBuffer == (UINTN)-1) {
    CpuMpData->WakeupBuffer = GetWakeupBuffer (CpuMpData->BackupBufferSize);
    //
    // Align MpCpuExchangeInfo to avoid split-lock violations.
    //
    CpuMpData->MpCpuExchangeInfo = (MP_CPU_EXCHANGE_INFO *)(UINTN)
                                   (CpuMpData->WakeupBuffer +
                                    ALIGN_VALUE (CpuMpData->AddressMap.ModeTransitionOffset, sizeof (UINTN)));
    DEBUG ((
      DEBUG_INFO,
      "AP Vector: 16-bit = %p/%x, ExchangeInfo = %p/%x\n",
      CpuMpData->WakeupBuffer,
      CpuMpData->AddressMap.ModeTransitionOffset,
      CpuMpData->MpCpuExchangeInfo,
      sizeof (MP_CPU_EXCHANGE_INFO)
      ));

    //
    // The AP reset stack is only used by SEV-ES guests. Do not allocate it
    // if SEV-ES is not enabled. An SEV-SNP guest is also considered
    // an SEV-ES guest, but uses a different method of AP startup, eliminating
    // the need for the allocation.
    //
    if (ConfidentialComputingGuestHas (CCAttrAmdSevEs) &&
        !ConfidentialComputingGuestHas (CCAttrAmdSevSnp))
    {
      //
      // Stack location is based on ProcessorNumber, so use the total number
      // of processors for calculating the total stack area.
      //
      ApResetStackSize = (AP_RESET_STACK_SIZE *
                          PcdGet32 (PcdCpuMaxLogicalProcessorNumber));

      //
      // Invoke GetWakeupBuffer a second time to allocate the stack area
      // below 1MB. The returned buffer will be page aligned and sized and
      // below the previously allocated buffer.
      //
      CpuMpData->SevEsAPResetStackStart = GetWakeupBuffer (ApResetStackSize);

      //
      // Check to be sure that the "allocate below" behavior hasn't changed.
      // This will also catch a failed allocation, as "-1" is returned on
      // failure.
      //
      if (CpuMpData->SevEsAPResetStackStart >= CpuMpData->WakeupBuffer) {
        DEBUG ((
          DEBUG_ERROR,
          "SEV-ES AP reset stack is not below wakeup buffer\n"
          ));

        ASSERT (FALSE);
        CpuDeadLoop ();
      }
    }
  }

  BackupAndPrepareWakeupBuffer (CpuMpData);
}

/**
  Free AP reset vector buffer.

  @param[in]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
FreeResetVector (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  //
  // If SEV-ES is enabled, the reset area is needed for AP parking and
  // and AP startup in the OS, so the reset area is reserved. Do not
  // perform the restore as this will overwrite memory which has data
  // needed by SEV-ES.
  //
  if (!CpuMpData->UseSevEsAPMethod) {
    RestoreWakeupBuffer (CpuMpData);
  }
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
  BOOLEAN                        ResetVectorRequired;
  CPU_INFO_IN_HOB                *CpuInfoInHob;

  CpuMpData->FinishedCount = 0;
  ResetVectorRequired      = FALSE;

  if (CpuMpData->WakeUpByInitSipiSipi ||
      (CpuMpData->InitFlag == ApInitConfig))
  {
    ResetVectorRequired = TRUE;
    AllocateResetVectorBelow1Mb (CpuMpData);
    AllocateSevEsAPMemory (CpuMpData);
    FillExchangeInfoData (CpuMpData);
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
        if ((GetApState (CpuData) == CpuStateDisabled) && !WakeUpDisabledAps) {
          continue;
        }

        CpuData->ApFunction         = (UINTN)Procedure;
        CpuData->ApFunctionArgument = (UINTN)ProcedureArgument;
        SetApState (CpuData, CpuStateReady);
        if (CpuMpData->InitFlag == ApInitDone) {
          *(UINT32 *)CpuData->StartupApSignal = WAKEUP_AP_SIGNAL;
        }
      }
    }

    if (ResetVectorRequired) {
      //
      // For SEV-ES and SEV-SNP, the initial AP boot address will be defined by
      // PcdSevEsWorkAreaBase. The Segment/Rip must be the jump address
      // from the original INIT-SIPI-SIPI.
      //
      if (CpuMpData->SevEsIsEnabled) {
        SetSevEsJumpTable (ExchangeInfo->BufferStart);
      }

      //
      // Wakeup all APs
      //   Must use the INIT-SIPI-SIPI method for initial configuration in
      //   order to obtain the APIC ID if not an SEV-SNP guest and the
      //   list of APIC IDs is not available.
      //
      if (CanUseSevSnpCreateAP (CpuMpData)) {
        SevSnpCreateAP (CpuMpData, -1);
      } else {
        if ((CpuMpData->InitFlag == ApInitConfig) && FixedPcdGetBool (PcdFirstTimeWakeUpAPsBySipi)) {
          //
          // SIPI can be used for the first time wake up after reset to reduce boot time.
          //
          SendStartupIpiAllExcludingSelf ((UINT32)ExchangeInfo->BufferStart);
        } else {
          SendInitSipiSipiAllExcludingSelf ((UINT32)ExchangeInfo->BufferStart);
        }
      }
    }

    if (CpuMpData->InitFlag == ApInitConfig) {
      if (PcdGet32 (PcdCpuBootLogicalProcessorNumber) > 0) {
        //
        // The AP enumeration algorithm below is suitable only when the
        // platform can tell us the *exact* boot CPU count in advance.
        //
        // The wait below finishes only when the detected AP count reaches
        // (PcdCpuBootLogicalProcessorNumber - 1), regardless of how long that
        // takes. If at least one AP fails to check in (meaning a platform
        // hardware bug), the detection hangs forever, by design. If the actual
        // boot CPU count in the system is higher than
        // PcdCpuBootLogicalProcessorNumber (meaning a platform
        // misconfiguration), then some APs may complete initialization after
        // the wait finishes, and cause undefined behavior.
        //
        TimedWaitForApFinish (
          CpuMpData,
          PcdGet32 (PcdCpuBootLogicalProcessorNumber) - 1,
          MAX_UINT32 // approx. 71 minutes
          );
      } else {
        //
        // The AP enumeration algorithm below is suitable for two use cases.
        //
        // (1) The check-in time for an individual AP is bounded, and APs run
        //     through their initialization routines strongly concurrently. In
        //     particular, the number of concurrently running APs
        //     ("NumApsExecuting") is never expected to fall to zero
        //     *temporarily* -- it is expected to fall to zero only when all
        //     APs have checked-in.
        //
        //     In this case, the platform is supposed to set
        //     PcdCpuApInitTimeOutInMicroSeconds to a low-ish value (just long
        //     enough for one AP to start initialization). The timeout will be
        //     reached soon, and remaining APs are collected by watching
        //     NumApsExecuting fall to zero. If NumApsExecuting falls to zero
        //     mid-process, while some APs have not completed initialization,
        //     the behavior is undefined.
        //
        // (2) The check-in time for an individual AP is unbounded, and/or APs
        //     may complete their initializations widely spread out. In
        //     particular, some APs may finish initialization before some APs
        //     even start.
        //
        //     In this case, the platform is supposed to set
        //     PcdCpuApInitTimeOutInMicroSeconds to a high-ish value. The AP
        //     enumeration will always take that long (except when the boot CPU
        //     count happens to be maximal, that is,
        //     PcdCpuMaxLogicalProcessorNumber). All APs are expected to
        //     check-in before the timeout, and NumApsExecuting is assumed zero
        //     at timeout. APs that miss the time-out may cause undefined
        //     behavior.
        //
        TimedWaitForApFinish (
          CpuMpData,
          PcdGet32 (PcdCpuMaxLogicalProcessorNumber) - 1,
          PcdGet32 (PcdCpuApInitTimeOutInMicroSeconds)
          );

        while (CpuMpData->MpCpuExchangeInfo->NumApsExecuting != 0) {
          CpuPause ();
        }
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
    CpuData                     = &CpuMpData->CpuData[ProcessorNumber];
    CpuData->ApFunction         = (UINTN)Procedure;
    CpuData->ApFunctionArgument = (UINTN)ProcedureArgument;
    SetApState (CpuData, CpuStateReady);
    //
    // Wakeup specified AP
    //
    ASSERT (CpuMpData->InitFlag == ApInitDone);
    *(UINT32 *)CpuData->StartupApSignal = WAKEUP_AP_SIGNAL;
    if (ResetVectorRequired) {
      CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;

      //
      // For SEV-ES and SEV-SNP, the initial AP boot address will be defined by
      // PcdSevEsWorkAreaBase. The Segment/Rip must be the jump address
      // from the original INIT-SIPI-SIPI.
      //
      if (CpuMpData->SevEsIsEnabled) {
        SetSevEsJumpTable (ExchangeInfo->BufferStart);
      }

      if (CanUseSevSnpCreateAP (CpuMpData)) {
        SevSnpCreateAP (CpuMpData, (INTN)ProcessorNumber);
      } else {
        SendInitSipiSipi (
          CpuInfoInHob[ProcessorNumber].ApicId,
          (UINT32)ExchangeInfo->BufferStart
          );
      }
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
  Switch Context for each AP.

**/
VOID
EFIAPI
SwitchContextPerAp (
  VOID
  )
{
  UINTN            ProcessorNumber;
  CPU_MP_DATA      *CpuMpData;
  CPU_INFO_IN_HOB  *CpuInfoInHob;

  CpuMpData    = GetCpuMpData ();
  CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;
  GetProcessorNumber (CpuMpData, &ProcessorNumber);

  SwitchStack (
    (SWITCH_STACK_ENTRY_POINT)(UINTN)DxeApEntryPoint,
    (VOID *)(UINTN)CpuMpData,
    NULL,
    (VOID *)((UINTN)CpuInfoInHob[ProcessorNumber].ApTopOfStack)
    );
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
  Reset an AP to Idle state.

  Any task being executed by the AP will be aborted and the AP
  will be waiting for a new task in Wait-For-SIPI state.

  @param[in] ProcessorNumber  The handle number of processor.
**/
VOID
ResetProcessorToIdleState (
  IN UINTN  ProcessorNumber
  )
{
  CPU_MP_DATA  *CpuMpData;

  CpuMpData = GetCpuMpData ();

  CpuMpData->WakeUpByInitSipiSipi = TRUE;
  if (CpuMpData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get CpuMpData.  Aborting the AP reset to idle.\n", __func__));
    return;
  }

  WakeUpAP (CpuMpData, FALSE, ProcessorNumber, NULL, NULL, TRUE);
  while (CpuMpData->FinishedCount < 1) {
    CpuPause ();
  }

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

  if (CpuMpData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get CpuMpData.\n", __func__));
    return EFI_LOAD_ERROR;
  }

  CpuData = &CpuMpData->CpuData[ProcessorNumber];

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
  This function Get BspNumber.

  @param[in] FirstMpHandOff   Pointer to first MpHandOff HOB body.
  @return                     BspNumber
**/
UINT32
GetBspNumber (
  IN CONST MP_HAND_OFF  *FirstMpHandOff
  )
{
  UINT32             ApicId;
  UINT32             Index;
  CONST MP_HAND_OFF  *MpHandOff;

  //
  // Get the processor number for the BSP
  //
  ApicId = GetInitialApicId ();

  for (MpHandOff = FirstMpHandOff;
       MpHandOff != NULL;
       MpHandOff = GetNextMpHandOffHob (MpHandOff))
  {
    for (Index = 0; Index < MpHandOff->CpuCount; Index++) {
      if (MpHandOff->Info[Index].ApicId == ApicId) {
        return MpHandOff->ProcessorIndex + Index;
      }
    }
  }

  ASSERT_EFI_ERROR (EFI_NOT_FOUND);
  return 0;
}

/**
  This function is intended to be invoked by the BSP in order
  to wake up the AP. The BSP accomplishes this by triggering a
  start-up signal, which in turn causes any APs that are
  currently in a loop on the PEI-prepared memory to awaken and
  begin running the procedure called SwitchContextPerAp.
  This procedure allows the AP to switch to another section of
  memory and continue its loop there.

  @param[in] MpHandOffConfig  Pointer to MP hand-off config HOB body.
  @param[in] FirstMpHandOff   Pointer to first MP hand-off HOB body.
**/
VOID
SwitchApContext (
  IN CONST MP_HAND_OFF_CONFIG  *MpHandOffConfig,
  IN CONST MP_HAND_OFF         *FirstMpHandOff
  )
{
  UINTN              Index;
  UINT32             BspNumber;
  CONST MP_HAND_OFF  *MpHandOff;

  BspNumber = GetBspNumber (FirstMpHandOff);

  for (MpHandOff = FirstMpHandOff;
       MpHandOff != NULL;
       MpHandOff = GetNextMpHandOffHob (MpHandOff))
  {
    for (Index = 0; Index < MpHandOff->CpuCount; Index++) {
      if (MpHandOff->ProcessorIndex + Index != BspNumber) {
        *(UINTN *)(UINTN)MpHandOff->Info[Index].StartupProcedureAddress = (UINTN)SwitchContextPerAp;
        *(UINT32 *)(UINTN)MpHandOff->Info[Index].StartupSignalAddress   = MpHandOffConfig->StartupSignalValue;
      }
    }
  }

  //
  // Wait all APs waken up if this is not the 1st broadcast of SIPI
  //
  for (MpHandOff = FirstMpHandOff;
       MpHandOff != NULL;
       MpHandOff = GetNextMpHandOffHob (MpHandOff))
  {
    for (Index = 0; Index < MpHandOff->CpuCount; Index++) {
      if (MpHandOff->ProcessorIndex + Index != BspNumber) {
        WaitApWakeup ((UINT32 *)(UINTN)(MpHandOff->Info[Index].StartupSignalAddress));
      }
    }
  }
}

/**
  Get pointer to MP_HAND_OFF_CONFIG GUIDed HOB body.

  @return  The pointer to MP_HAND_OFF_CONFIG structure.
**/
MP_HAND_OFF_CONFIG *
GetMpHandOffConfigHob (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  GuidHob = GetFirstGuidHob (&mMpHandOffConfigGuid);
  if (GuidHob == NULL) {
    return NULL;
  }

  return (MP_HAND_OFF_CONFIG *)GET_GUID_HOB_DATA (GuidHob);
}

/**
  Get pointer to next MP_HAND_OFF GUIDed HOB body.

  @param[in] MpHandOff  Previous HOB body.  Pass NULL to get the first HOB.

  @return  The pointer to MP_HAND_OFF structure.
**/
MP_HAND_OFF *
GetNextMpHandOffHob (
  IN CONST MP_HAND_OFF  *MpHandOff
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  if (MpHandOff == NULL) {
    GuidHob = GetFirstGuidHob (&mMpHandOffGuid);
  } else {
    GuidHob = (VOID *)(((UINT8 *)MpHandOff) - sizeof (EFI_HOB_GUID_TYPE));
    GuidHob = GetNextGuidHob (&mMpHandOffGuid, GET_NEXT_HOB (GuidHob));
  }

  if (GuidHob == NULL) {
    return NULL;
  }

  return (MP_HAND_OFF *)GET_GUID_HOB_DATA (GuidHob);
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
  MP_HAND_OFF_CONFIG       *MpHandOffConfig;
  MP_HAND_OFF              *FirstMpHandOff;
  MP_HAND_OFF              *MpHandOff;
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
  UINT32                   Index, HobIndex;
  UINTN                    ApResetVectorSizeBelow1Mb;
  UINTN                    ApResetVectorSizeAbove1Mb;
  UINTN                    BackupBufferAddr;
  UINTN                    ApIdtBase;
  IA32_CR0                 Cr0;

  FirstMpHandOff = GetNextMpHandOffHob (NULL);
  if (FirstMpHandOff != NULL) {
    MaxLogicalProcessorNumber = 0;
    for (MpHandOff = FirstMpHandOff;
         MpHandOff != NULL;
         MpHandOff = GetNextMpHandOffHob (MpHandOff))
    {
      DEBUG ((
        DEBUG_INFO,
        "%a: ProcessorIndex=%u CpuCount=%u\n",
        __func__,
        MpHandOff->ProcessorIndex,
        MpHandOff->CpuCount
        ));
      ASSERT (MaxLogicalProcessorNumber == MpHandOff->ProcessorIndex);
      MaxLogicalProcessorNumber += MpHandOff->CpuCount;
    }
  } else {
    MaxLogicalProcessorNumber = PcdGet32 (PcdCpuMaxLogicalProcessorNumber);
  }

  ASSERT (MaxLogicalProcessorNumber != 0);

  AsmGetAddressMap (&AddressMap);
  GetApResetVectorSize (&AddressMap, &ApResetVectorSizeBelow1Mb, &ApResetVectorSizeAbove1Mb);
  ApStackSize = PcdGet32 (PcdCpuApStackSize);
  //
  // ApStackSize must be power of 2
  //
  ASSERT ((ApStackSize & (ApStackSize - 1)) == 0);
  ApLoopMode = GetApLoopMode (&MonitorFilterSize);

  //
  // Save BSP's Control registers for APs.
  //
  SaveVolatileRegisters (&VolatileRegisters);

  BufferSize = ApStackSize * MaxLogicalProcessorNumber;
  //
  // Allocate extra ApStackSize to let AP stack align on ApStackSize bounday
  //
  BufferSize += ApStackSize;
  BufferSize += MonitorFilterSize * MaxLogicalProcessorNumber;
  BufferSize += ApResetVectorSizeBelow1Mb;
  BufferSize  = ALIGN_VALUE (BufferSize, 8);
  BufferSize += VolatileRegisters.Idtr.Limit + 1;
  BufferSize += sizeof (CPU_MP_DATA);
  BufferSize += (sizeof (CPU_AP_DATA) + sizeof (CPU_INFO_IN_HOB))* MaxLogicalProcessorNumber;
  MpBuffer    = AllocatePages (EFI_SIZE_TO_PAGES (BufferSize));
  ASSERT (MpBuffer != NULL);
  if (MpBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (MpBuffer, BufferSize);
  Buffer = ALIGN_VALUE ((UINTN)MpBuffer, ApStackSize);

  //
  //  The layout of the Buffer is as below (lower address on top):
  //
  //    +--------------------+ <-- Buffer (Pointer of CpuMpData is stored in the top of each AP's stack.)
  //        AP Stacks (N)                 (StackTop = (RSP + ApStackSize) & ~ApStackSize))
  //    +--------------------+ <-- MonitorBuffer
  //    AP Monitor Filters (N)
  //    +--------------------+ <-- BackupBufferAddr (CpuMpData->BackupBuffer)
  //         Backup Buffer
  //    +--------------------+
  //           Padding
  //    +--------------------+ <-- ApIdtBase (8-byte boundary)
  //           AP IDT          All APs share one separate IDT.
  //    +--------------------+ <-- CpuMpData
  //         CPU_MP_DATA
  //    +--------------------+ <-- CpuMpData->CpuData
  //        CPU_AP_DATA (N)
  //    +--------------------+ <-- CpuMpData->CpuInfoInHob
  //      CPU_INFO_IN_HOB (N)
  //    +--------------------+
  //
  MonitorBuffer               = (UINT8 *)(Buffer + ApStackSize * MaxLogicalProcessorNumber);
  BackupBufferAddr            = (UINTN)MonitorBuffer + MonitorFilterSize * MaxLogicalProcessorNumber;
  ApIdtBase                   = ALIGN_VALUE (BackupBufferAddr + ApResetVectorSizeBelow1Mb, 8);
  CpuMpData                   = (CPU_MP_DATA *)(ApIdtBase + VolatileRegisters.Idtr.Limit + 1);
  CpuMpData->Buffer           = Buffer;
  CpuMpData->CpuApStackSize   = ApStackSize;
  CpuMpData->BackupBuffer     = BackupBufferAddr;
  CpuMpData->BackupBufferSize = ApResetVectorSizeBelow1Mb;
  CpuMpData->WakeupBuffer     = (UINTN)-1;
  CpuMpData->CpuCount         = 1;
  if (FirstMpHandOff == NULL) {
    CpuMpData->BspNumber = 0;
  } else {
    CpuMpData->BspNumber = GetBspNumber (FirstMpHandOff);
  }

  CpuMpData->WaitEvent     = NULL;
  CpuMpData->SwitchBspFlag = FALSE;
  CpuMpData->CpuData       = (CPU_AP_DATA *)(CpuMpData + 1);
  CpuMpData->CpuInfoInHob  = (UINT64)(UINTN)(CpuMpData->CpuData + MaxLogicalProcessorNumber);
  InitializeSpinLock (&CpuMpData->MpLock);
  CpuMpData->SevEsIsEnabled   = ConfidentialComputingGuestHas (CCAttrAmdSevEs);
  CpuMpData->SevSnpIsEnabled  = ConfidentialComputingGuestHas (CCAttrAmdSevSnp);
  CpuMpData->SevEsAPBuffer    = (UINTN)-1;
  CpuMpData->GhcbBase         = PcdGet64 (PcdGhcbBase);
  CpuMpData->UseSevEsAPMethod = CpuMpData->SevEsIsEnabled && !CpuMpData->SevSnpIsEnabled;

  if (CpuMpData->SevSnpIsEnabled) {
    ASSERT ((PcdGet64 (PcdGhcbHypervisorFeatures) & GHCB_HV_FEATURES_SNP_AP_CREATE) == GHCB_HV_FEATURES_SNP_AP_CREATE);
  }

  //
  // Make sure no memory usage outside of the allocated buffer.
  // (ApStackSize - (Buffer - (UINTN)MpBuffer)) is the redundant caused by alignment
  //
  ASSERT (
    (CpuMpData->CpuInfoInHob + sizeof (CPU_INFO_IN_HOB) * MaxLogicalProcessorNumber) ==
    (UINTN)MpBuffer + BufferSize - (ApStackSize - Buffer + (UINTN)MpBuffer)
    );

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
  //
  // Set DR as 0 since DR is set only for BSP.
  //
  VolatileRegisters.Dr0 = 0;
  VolatileRegisters.Dr1 = 0;
  VolatileRegisters.Dr2 = 0;
  VolatileRegisters.Dr3 = 0;
  VolatileRegisters.Dr6 = 0;
  VolatileRegisters.Dr7 = 0;

  //
  // Copy volatile registers since either APs are the first time to bring up,
  // or BSP is in DXE phase but APs are still running in PEI context.
  // In both cases, APs need use volatile registers from BSP
  //
  for (Index = 0; Index < MaxLogicalProcessorNumber; Index++) {
    CopyMem (&CpuMpData->CpuData[Index].VolatileRegisters, &VolatileRegisters, sizeof (VolatileRegisters));
  }

  //
  // Set BSP basic information
  //
  InitializeApData (CpuMpData, CpuMpData->BspNumber, 0, CpuMpData->Buffer + ApStackSize * (CpuMpData->BspNumber + 1));
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
  // Copy all 32-bit code and 64-bit code into memory with type of
  // EfiBootServicesCode to avoid page fault if NX memory protection is enabled.
  //
  CpuMpData->WakeupBufferHigh = AllocateCodePage (ApResetVectorSizeAbove1Mb);

  Cr0.UintN = AsmReadCr0 ();
  if (Cr0.Bits.PG != 0) {
    RemoveNxProtection ((EFI_PHYSICAL_ADDRESS)(UINTN)CpuMpData->WakeupBufferHigh, ALIGN_VALUE (ApResetVectorSizeAbove1Mb, EFI_PAGE_SIZE));
  }

  CopyMem (
    (VOID *)CpuMpData->WakeupBufferHigh,
    CpuMpData->AddressMap.RendezvousFunnelAddress +
    CpuMpData->AddressMap.ModeTransitionOffset,
    ApResetVectorSizeAbove1Mb
    );
  DEBUG ((DEBUG_INFO, "AP Vector: non-16-bit = %p/%x\n", CpuMpData->WakeupBufferHigh, ApResetVectorSizeAbove1Mb));
  if (Cr0.Bits.PG != 0) {
    ApplyRoProtection ((EFI_PHYSICAL_ADDRESS)(UINTN)CpuMpData->WakeupBufferHigh, ALIGN_VALUE (ApResetVectorSizeAbove1Mb, EFI_PAGE_SIZE));
  }

  //
  // Save APIC mode for AP to sync
  //
  CpuMpData->InitialBspApicMode = GetApicMode ();

  //
  // Enable the local APIC for Virtual Wire Mode.
  //
  ProgramVirtualWireMode ();
  SaveLocalApicTimerSetting (CpuMpData);

  if (FirstMpHandOff == NULL) {
    if (MaxLogicalProcessorNumber > 1) {
      //
      // Wakeup all APs and calculate the processor count in system
      //
      CollectProcessorCount (CpuMpData);

      //
      // Enable X2APIC if needed.
      //
      if (CpuMpData->InitialBspApicMode == LOCAL_APIC_MODE_XAPIC) {
        AutoEnableX2Apic (CpuMpData);
      }

      //
      // Sort BSP/Aps by CPU APIC ID in ascending order
      //
      SortApicId (CpuMpData);

      DEBUG ((DEBUG_INFO, "MpInitLib: Find %d processors in system.\n", CpuMpData->CpuCount));
    }
  } else {
    //
    // APs have been wakeup before, just get the CPU Information
    // from HOB
    //
    CpuMpData->InitFlag = ApInitDone;
    if (CpuMpData->UseSevEsAPMethod) {
      AmdSevUpdateCpuMpData (CpuMpData);
    }

    CpuMpData->CpuCount = MaxLogicalProcessorNumber;
    CpuInfoInHob        = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;
    for (MpHandOff = FirstMpHandOff;
         MpHandOff != NULL;
         MpHandOff = GetNextMpHandOffHob (MpHandOff))
    {
      for (HobIndex = 0; HobIndex < MpHandOff->CpuCount; HobIndex++) {
        Index = MpHandOff->ProcessorIndex + HobIndex;
        InitializeSpinLock (&CpuMpData->CpuData[Index].ApLock);
        CpuMpData->CpuData[Index].CpuHealthy = (MpHandOff->Info[HobIndex].Health == 0) ? TRUE : FALSE;
        CpuMpData->CpuData[Index].ApFunction = 0;
        CpuInfoInHob[Index].InitialApicId    = MpHandOff->Info[HobIndex].ApicId;
        CpuInfoInHob[Index].ApTopOfStack     = CpuMpData->Buffer + (Index + 1) * CpuMpData->CpuApStackSize;
        CpuInfoInHob[Index].ApicId           = MpHandOff->Info[HobIndex].ApicId;
        CpuInfoInHob[Index].Health           = MpHandOff->Info[HobIndex].Health;
      }
    }

    MpHandOffConfig = GetMpHandOffConfigHob ();
    if (MpHandOffConfig == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: at least one MpHandOff HOB, but no MpHandOffConfig HOB\n",
        __func__
        ));
      ASSERT (MpHandOffConfig != NULL);
      CpuDeadLoop ();
    }

    DEBUG ((
      DEBUG_INFO,
      "FirstMpHandOff->WaitLoopExecutionMode: %04d, sizeof (VOID *): %04d\n",
      MpHandOffConfig->WaitLoopExecutionMode,
      sizeof (VOID *)
      ));
    if (MpHandOffConfig->WaitLoopExecutionMode == sizeof (VOID *)) {
      ASSERT (CpuMpData->ApLoopMode != ApInHltLoop);

      CpuMpData->FinishedCount                        = 0;
      CpuMpData->EnableExecuteDisableForSwitchContext = IsBspExecuteDisableEnabled ();
      SaveCpuMpData (CpuMpData);
      //
      // In scenarios where both the PEI and DXE phases run in the same
      // execution mode (32bit or 64bit), the BSP triggers
      // a start-up signal during the DXE phase to wake up the APs. This causes any
      // APs that are currently in a loop on the memory prepared during the PEI
      // phase to awaken and run the SwitchContextPerAp procedure. This procedure
      // enables the APs to switch to a different memory section and continue their
      // looping process there.
      //
      SwitchApContext (MpHandOffConfig, FirstMpHandOff);
      //
      // Wait for all APs finished initialization
      //
      while (CpuMpData->FinishedCount < (CpuMpData->CpuCount - 1)) {
        CpuPause ();
      }

      //
      // Set Apstate as Idle, otherwise Aps cannot be waken-up again.
      // If any enabled AP is not idle, return EFI_NOT_READY during waken-up.
      //
      for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
        SetApState (&CpuMpData->CpuData[Index], CpuStateIdle);
      }

      //
      // Initialize global data for MP support
      //
      InitMpGlobalData (CpuMpData);
      return EFI_SUCCESS;
    } else {
      //
      // PEI and DXE are in different Execution Mode
      // Use Init Sipi Sipi for the first AP wake up in DXE phase.
      //
      CpuMpData->WakeUpByInitSipiSipi = TRUE;
    }
  }

  if (!GetMicrocodePatchInfoFromHob (
         &CpuMpData->MicrocodePatchAddress,
         &CpuMpData->MicrocodePatchRegionSize
         ))
  {
    //
    // The microcode patch information cache HOB does not exist, which means
    // the microcode patches data has not been loaded into memory yet
    //
    ShadowMicrocodeUpdatePatch (CpuMpData);
  }

  //
  // Detect and apply Microcode on BSP
  //
  MicrocodeDetect (CpuMpData, CpuMpData->BspNumber);
  //
  // Store BSP's MTRR setting
  //
  MtrrGetAllMtrrs (&CpuMpData->MtrrTable);

  //
  // Wakeup APs to do some AP initialize sync (Microcode & MTRR)
  //
  if (CpuMpData->CpuCount > 1) {
    WakeUpAP (CpuMpData, TRUE, 0, ApInitializeSync, CpuMpData, TRUE);
    //
    // Wait for all APs finished initialization
    //
    while (CpuMpData->FinishedCount < (CpuMpData->CpuCount - 1)) {
      CpuPause ();
    }

    for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
      SetApState (&CpuMpData->CpuData[Index], CpuStateIdle);
    }
  }

  //
  // Dump the microcode revision for each core.
  //
  DEBUG_CODE_BEGIN ();
  UINT32  ThreadId;
  UINT32  ExpectedMicrocodeRevision;

  CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;
  for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
    GetProcessorLocationByApicId (CpuInfoInHob[Index].InitialApicId, NULL, NULL, &ThreadId);
    if (ThreadId == 0) {
      //
      // MicrocodeDetect() loads microcode in first thread of each core, so,
      // CpuMpData->CpuData[Index].MicrocodeEntryAddr is initialized only for first thread of each core.
      //
      ExpectedMicrocodeRevision = 0;
      if (CpuMpData->CpuData[Index].MicrocodeEntryAddr != 0) {
        ExpectedMicrocodeRevision = ((CPU_MICROCODE_HEADER *)(UINTN)CpuMpData->CpuData[Index].MicrocodeEntryAddr)->UpdateRevision;
      }

      DEBUG ((
        DEBUG_INFO,
        "CPU[%04d]: Microcode revision = %08x, expected = %08x\n",
        Index,
        CpuMpData->CpuData[Index].MicrocodeRevision,
        ExpectedMicrocodeRevision
        ));
    }
  }

  DEBUG_CODE_END ();
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
                                    Lower 24 bits contains the actual processor number.
                                    BIT24 indicates if the EXTENDED_PROCESSOR_INFORMATION will be retrived.
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
  UINTN            OriginalProcessorNumber;
  EFI_STATUS       Status;

  CpuMpData = GetCpuMpData ();

  if (CpuMpData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get CpuMpData.\n", __func__));
    return EFI_LOAD_ERROR;
  }

  CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;

  //
  // Lower 24 bits contains the actual processor number.
  //
  OriginalProcessorNumber = ProcessorNumber;
  ProcessorNumber        &= BIT24 - 1;

  //
  // Check whether caller processor is BSP
  //
  Status = MpInitLibWhoAmI (&CallerNumber);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get processor number.  Failed to get MpInit Processor info.\n", __func__));
    return Status;
  }

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

  //
  // Get processor location information
  //
  GetProcessorLocationByApicId (
    CpuInfoInHob[ProcessorNumber].ApicId,
    &ProcessorInfoBuffer->Location.Package,
    &ProcessorInfoBuffer->Location.Core,
    &ProcessorInfoBuffer->Location.Thread
    );

  if ((OriginalProcessorNumber & CPU_V2_EXTENDED_TOPOLOGY) != 0) {
    GetProcessorLocation2ByApicId (
      CpuInfoInHob[ProcessorNumber].ApicId,
      &ProcessorInfoBuffer->ExtendedInformation.Location2.Package,
      &ProcessorInfoBuffer->ExtendedInformation.Location2.Die,
      &ProcessorInfoBuffer->ExtendedInformation.Location2.Tile,
      &ProcessorInfoBuffer->ExtendedInformation.Location2.Module,
      &ProcessorInfoBuffer->ExtendedInformation.Location2.Core,
      &ProcessorInfoBuffer->ExtendedInformation.Location2.Thread
      );
  }

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
  IN UINTN    ProcessorNumber,
  IN BOOLEAN  EnableOldBSP
  )
{
  CPU_MP_DATA                  *CpuMpData;
  UINTN                        CallerNumber;
  CPU_STATE                    State;
  MSR_IA32_APIC_BASE_REGISTER  ApicBaseMsr;
  BOOLEAN                      OldInterruptState;
  BOOLEAN                      OldTimerInterruptState;
  EFI_STATUS                   Status;

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

  if (CpuMpData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get CpuMpData.\n", __func__));
    return EFI_LOAD_ERROR;
  }

  //
  // Check whether caller processor is BSP
  //
  Status = MpInitLibWhoAmI (&CallerNumber);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get processor number.  Failed to get MpInit Processor info.\n", __func__));
    return Status;
  }

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
  ApicBaseMsr.Uint64   = AsmReadMsr64 (MSR_IA32_APIC_BASE);
  ApicBaseMsr.Bits.BSP = 0;
  AsmWriteMsr64 (MSR_IA32_APIC_BASE, ApicBaseMsr.Uint64);

  //
  // Save BSP's local APIC timer setting.
  //
  SaveLocalApicTimerSetting (CpuMpData);

  //
  // Need to wakeUp AP (future BSP).
  //
  WakeUpAP (CpuMpData, FALSE, ProcessorNumber, FutureBSPProc, CpuMpData, TRUE);

  //
  // Save and restore volatile registers when switch BSP
  //
  SaveVolatileRegisters (&CpuMpData->BSPInfo.VolatileRegisters);
  AsmExchangeRole (&CpuMpData->BSPInfo, &CpuMpData->APInfo);
  RestoreVolatileRegisters (&CpuMpData->BSPInfo.VolatileRegisters);
  //
  // Set the BSP bit of MSR_IA32_APIC_BASE on new BSP
  //
  ApicBaseMsr.Uint64   = AsmReadMsr64 (MSR_IA32_APIC_BASE);
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
  CpuMpData->BspNumber = (UINT32)ProcessorNumber;

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
  IN  UINTN    ProcessorNumber,
  IN  BOOLEAN  EnableAP,
  IN  UINT32   *HealthFlag OPTIONAL
  )
{
  CPU_MP_DATA  *CpuMpData;
  UINTN        CallerNumber;
  EFI_STATUS   Status;

  CpuMpData = GetCpuMpData ();

  if (CpuMpData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get CpuMpData.\n", __func__));
    return EFI_LOAD_ERROR;
  }

  //
  // Check whether caller processor is BSP
  //
  Status = MpInitLibWhoAmI (&CallerNumber);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get processor number.  Failed to get MpInit Processor info.\n", __func__));
    return Status;
  }

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
      (BOOLEAN)((*HealthFlag & PROCESSOR_HEALTH_STATUS_BIT) != 0);
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

  if (CpuMpData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get CpuMpData.\n", __func__));
    return EFI_LOAD_ERROR;
  }

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
  EFI_STATUS   Status;

  CpuMpData = GetCpuMpData ();

  if (CpuMpData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get CpuMpData.\n", __func__));
    return EFI_LOAD_ERROR;
  }

  if ((NumberOfProcessors == NULL) && (NumberOfEnabledProcessors == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether caller processor is BSP
  //
  Status = MpInitLibWhoAmI (&CallerNumber);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get processor number.  Failed to get MpInit Processor info.\n", __func__));
    return Status;
  }

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
  Status = MpInitLibWhoAmI (&CallerNumber);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get processor number.  Failed to get MpInit Processor info.\n", __func__));
    return Status;
  }

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

  if (CpuMpData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get CpuMpData.\n", __func__));
    return EFI_LOAD_ERROR;
  }

  //
  // Check whether caller processor is BSP
  //
  Status = MpInitLibWhoAmI (&CallerNumber);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get processor number.  Failed to get MpInit Processor info.\n", __func__));
    return Status;
  }

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
  The function check if the specified Attr is set.

  @param[in]  CurrentAttr   The current attribute.
  @param[in]  Attr          The attribute to check.

  @retval  TRUE      The specified Attr is set.
  @retval  FALSE     The specified Attr is not set.

**/
STATIC
BOOLEAN
AmdMemEncryptionAttrCheck (
  IN  UINT64                             CurrentAttr,
  IN  CONFIDENTIAL_COMPUTING_GUEST_ATTR  Attr
  )
{
  UINT64  CurrentLevel;

  CurrentLevel = CurrentAttr & CCAttrTypeMask;

  switch (Attr) {
    case CCAttrAmdSev:
      //
      // SEV is automatically enabled if SEV-ES or SEV-SNP is active.
      //
      return CurrentLevel >= CCAttrAmdSev;
    case CCAttrAmdSevEs:
      //
      // SEV-ES is automatically enabled if SEV-SNP is active.
      //
      return CurrentLevel >= CCAttrAmdSevEs;
    case CCAttrAmdSevSnp:
      return CurrentLevel == CCAttrAmdSevSnp;
    case CCAttrFeatureAmdSevEsDebugVirtualization:
      return !!(CurrentAttr & CCAttrFeatureAmdSevEsDebugVirtualization);
    case CCAttrFeatureAmdSevSnpAlternateInjection:
      return !!(CurrentAttr & CCAttrFeatureAmdSevSnpAlternateInjection);
    default:
      return FALSE;
  }
}

/**
  Check if the specified confidential computing attribute is active.

  @param[in]  Attr          The attribute to check.

  @retval TRUE   The specified Attr is active.
  @retval FALSE  The specified Attr is not active.

**/
BOOLEAN
EFIAPI
ConfidentialComputingGuestHas (
  IN  CONFIDENTIAL_COMPUTING_GUEST_ATTR  Attr
  )
{
  UINT64  CurrentAttr;

  //
  // Get the current CC attribute.
  //
  CurrentAttr = PcdGet64 (PcdConfidentialComputingGuestAttr);

  //
  // If attr is for the AMD group then call AMD specific checks.
  //
  if (((RShiftU64 (CurrentAttr, 8)) & 0xff) == 1) {
    return AmdMemEncryptionAttrCheck (CurrentAttr, Attr);
  }

  return (CurrentAttr == Attr);
}

/**
  Do sync on APs.

  @param[in, out] Buffer  Pointer to private data buffer.
**/
VOID
EFIAPI
RelocateApLoop (
  IN OUT VOID  *Buffer
  )
{
  CPU_MP_DATA  *CpuMpData;
  BOOLEAN      MwaitSupport;
  UINTN        ProcessorNumber;
  UINTN        StackStart;
  EFI_STATUS   Status;

  Status = MpInitLibWhoAmI (&ProcessorNumber);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get processor number.  Aborting AP sync.\n", __func__));
    return;
  }

  CpuMpData    = GetCpuMpData ();
  MwaitSupport = IsMwaitSupport ();
  if (CpuMpData->UseSevEsAPMethod) {
    //
    // 64-bit AMD processors with SEV-ES
    //
    StackStart = CpuMpData->SevEsAPResetStackStart;
    mReservedApLoop.AmdSevEntry (
                      MwaitSupport,
                      CpuMpData->ApTargetCState,
                      CpuMpData->PmCodeSegment,
                      StackStart - ProcessorNumber * AP_SAFE_STACK_SIZE,
                      (UINTN)&mNumberToFinish,
                      CpuMpData->Pm16CodeSegment,
                      CpuMpData->SevEsAPBuffer,
                      CpuMpData->WakeupBuffer
                      );
  } else {
    //
    // Intel processors (32-bit or 64-bit), 32-bit AMD processors, or 64-bit AMD processors without SEV-ES
    //
    StackStart = mReservedTopOfApStack;
    mReservedApLoop.GenericEntry (
                      MwaitSupport,
                      CpuMpData->ApTargetCState,
                      StackStart - ProcessorNumber * AP_SAFE_STACK_SIZE,
                      (UINTN)&mNumberToFinish,
                      mApPageTable
                      );
  }

  //
  // It should never reach here
  //
  ASSERT (FALSE);
}

/**
  Prepare ApLoopCode.

  @param[in] CpuMpData  Pointer to CpuMpData.
**/
VOID
PrepareApLoopCode (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  EFI_PHYSICAL_ADDRESS     Address;
  MP_ASSEMBLY_ADDRESS_MAP  *AddressMap;
  UINT8                    *ApLoopFunc;
  UINTN                    ApLoopFuncSize;
  UINTN                    StackPages;
  UINTN                    FuncPages;
  IA32_CR0                 Cr0;

  AddressMap = &CpuMpData->AddressMap;
  if (CpuMpData->UseSevEsAPMethod) {
    //
    // 64-bit AMD processors with SEV-ES
    //
    Address        = BASE_4GB - 1;
    ApLoopFunc     = AddressMap->RelocateApLoopFuncAddressAmdSev;
    ApLoopFuncSize = AddressMap->RelocateApLoopFuncSizeAmdSev;
  } else {
    //
    // Intel processors (32-bit or 64-bit), 32-bit AMD processors, or 64-bit AMD processors without SEV-ES
    //
    Address        = MAX_ADDRESS;
    ApLoopFunc     = AddressMap->RelocateApLoopFuncAddressGeneric;
    ApLoopFuncSize = AddressMap->RelocateApLoopFuncSizeGeneric;
  }

  //
  // Avoid APs access invalid buffer data which allocated by BootServices,
  // so we will allocate reserved data for AP loop code. We also need to
  // allocate this buffer below 4GB due to APs may be transferred to 32bit
  // protected mode on long mode DXE.
  // Allocating it in advance since memory services are not available in
  // Exit Boot Services callback function.
  //
  // +------------+ (TopOfApStack)
  // |  Stack * N |
  // +------------+ (stack base, 4k aligned)
  // |  Padding   |
  // +------------+
  // |  Ap Loop   |
  // +------------+ ((low address, 4k-aligned)
  //

  StackPages = EFI_SIZE_TO_PAGES (CpuMpData->CpuCount * AP_SAFE_STACK_SIZE);
  FuncPages  = EFI_SIZE_TO_PAGES (ApLoopFuncSize);

  AllocateApLoopCodeBuffer (StackPages + FuncPages, &Address);
  ASSERT (Address != 0);

  Cr0.UintN = AsmReadCr0 ();
  if (Cr0.Bits.PG != 0) {
    //
    // Make sure that the buffer memory is executable if NX protection is enabled
    // for EfiReservedMemoryType.
    //
    RemoveNxProtection (Address, EFI_PAGES_TO_SIZE (FuncPages));
  }

  mReservedTopOfApStack = (UINTN)Address + EFI_PAGES_TO_SIZE (StackPages+FuncPages);
  ASSERT ((mReservedTopOfApStack & (UINTN)(CPU_STACK_ALIGNMENT - 1)) == 0);
  mReservedApLoop.Data = (VOID *)(UINTN)Address;
  ASSERT (mReservedApLoop.Data != NULL);
  CopyMem (mReservedApLoop.Data, ApLoopFunc, ApLoopFuncSize);
  if (Cr0.Bits.PG != 0) {
    ApplyRoProtection (Address, EFI_PAGES_TO_SIZE (FuncPages));
  }

  if (!CpuMpData->UseSevEsAPMethod) {
    //
    // processors without SEV-ES and paging is enabled
    //
    mApPageTable = CreatePageTable (
                     (UINTN)Address,
                     EFI_PAGES_TO_SIZE (StackPages+FuncPages)
                     );
  }
}

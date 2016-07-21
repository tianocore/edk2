/** @file
  CPU MP Initialize Library common functions.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MpLib.h"

EFI_GUID mCpuInitMpLibHobGuid = CPU_INIT_MP_LIB_HOB_GUID;

/**
  The function will check if BSP Execute Disable is enabled.
  DxeIpl may have enabled Execute Disable for BSP,
  APs need to get the status and sync up the settings.

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

  Enabled = FALSE;
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

  return Enabled;
}

/**
  Get CPU Package/Core/Thread location information.

  @param[in]  InitialApicId     CPU APIC ID
  @param[out] Location          Pointer to CPU location information
**/
VOID
ExtractProcessorLocation (
  IN  UINT32                     InitialApicId,
  OUT EFI_CPU_PHYSICAL_LOCATION  *Location
  )
{
  BOOLEAN                        TopologyLeafSupported;
  UINTN                          ThreadBits;
  UINTN                          CoreBits;
  CPUID_VERSION_INFO_EBX         VersionInfoEbx;
  CPUID_VERSION_INFO_EDX         VersionInfoEdx;
  CPUID_CACHE_PARAMS_EAX         CacheParamsEax;
  CPUID_EXTENDED_TOPOLOGY_EAX    ExtendedTopologyEax;
  CPUID_EXTENDED_TOPOLOGY_EBX    ExtendedTopologyEbx;
  CPUID_EXTENDED_TOPOLOGY_ECX    ExtendedTopologyEcx;
  UINT32                         MaxCpuIdIndex;
  UINT32                         SubIndex;
  UINTN                          LevelType;
  UINT32                         MaxLogicProcessorsPerPackage;
  UINT32                         MaxCoresPerPackage;

  //
  // Check if the processor is capable of supporting more than one logical processor.
  //
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &VersionInfoEdx.Uint32);
  if (VersionInfoEdx.Bits.HTT == 0) {
    Location->Thread  = 0;
    Location->Core    = 0;
    Location->Package = 0;
    return;
  }

  ThreadBits = 0;
  CoreBits = 0;

  //
  // Assume three-level mapping of APIC ID: Package:Core:SMT.
  //

  TopologyLeafSupported = FALSE;
  //
  // Get the max index of basic CPUID
  //
  AsmCpuid (CPUID_SIGNATURE, &MaxCpuIdIndex, NULL, NULL, NULL);

  //
  // If the extended topology enumeration leaf is available, it
  // is the preferred mechanism for enumerating topology.
  //
  if (MaxCpuIdIndex >= CPUID_EXTENDED_TOPOLOGY) {
    AsmCpuidEx (
      CPUID_EXTENDED_TOPOLOGY,
      0,
      &ExtendedTopologyEax.Uint32,
      &ExtendedTopologyEbx.Uint32,
      &ExtendedTopologyEcx.Uint32,
      NULL
      );
    //
    // If CPUID.(EAX=0BH, ECX=0H):EBX returns zero and maximum input value for
    // basic CPUID information is greater than 0BH, then CPUID.0BH leaf is not
    // supported on that processor.
    //
    if (ExtendedTopologyEbx.Uint32 != 0) {
      TopologyLeafSupported = TRUE;

      //
      // Sub-leaf index 0 (ECX= 0 as input) provides enumeration parameters to extract
      // the SMT sub-field of x2APIC ID.
      //
      LevelType = ExtendedTopologyEcx.Bits.LevelType;
      ASSERT (LevelType == CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_SMT);
      ThreadBits = ExtendedTopologyEax.Bits.ApicIdShift;

      //
      // Software must not assume any "level type" encoding
      // value to be related to any sub-leaf index, except sub-leaf 0.
      //
      SubIndex = 1;
      do {
        AsmCpuidEx (
          CPUID_EXTENDED_TOPOLOGY,
          SubIndex,
          &ExtendedTopologyEax.Uint32,
          NULL,
          &ExtendedTopologyEcx.Uint32,
          NULL
          );
        LevelType = ExtendedTopologyEcx.Bits.LevelType;
        if (LevelType == CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_CORE) {
          CoreBits = ExtendedTopologyEax.Bits.ApicIdShift - ThreadBits;
          break;
        }
        SubIndex++;
      } while (LevelType != CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_INVALID);
    }
  }

  if (!TopologyLeafSupported) {
    AsmCpuid (CPUID_VERSION_INFO, NULL, &VersionInfoEbx.Uint32, NULL, NULL);
    MaxLogicProcessorsPerPackage = VersionInfoEbx.Bits.MaximumAddressableIdsForLogicalProcessors;
    if (MaxCpuIdIndex >= CPUID_CACHE_PARAMS) {
      AsmCpuidEx (CPUID_CACHE_PARAMS, 0, &CacheParamsEax.Uint32, NULL, NULL, NULL);
      MaxCoresPerPackage = CacheParamsEax.Bits.MaximumAddressableIdsForLogicalProcessors + 1;
    } else {
      //
      // Must be a single-core processor.
      //
      MaxCoresPerPackage = 1;
    }

    ThreadBits = (UINTN) (HighBitSet32 (MaxLogicProcessorsPerPackage / MaxCoresPerPackage - 1) + 1);
    CoreBits = (UINTN) (HighBitSet32 (MaxCoresPerPackage - 1) + 1);
  }

  Location->Thread  = InitialApicId & ((1 << ThreadBits) - 1);
  Location->Core    = (InitialApicId >> ThreadBits) & ((1 << CoreBits) - 1);
  Location->Package = (InitialApicId >> (ThreadBits + CoreBits));
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

  AsmWriteCr0 (VolatileRegisters->Cr0);
  AsmWriteCr3 (VolatileRegisters->Cr3);
  AsmWriteCr4 (VolatileRegisters->Cr4);

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
  CPU_AP_DATA       CpuData;
  UINT32            ApCount;
  CPU_INFO_IN_HOB   *CpuInfoInHob;

  ApCount = CpuMpData->CpuCount - 1;

  if (ApCount != 0) {
    for (Index1 = 0; Index1 < ApCount; Index1++) {
      Index3 = Index1;
      //
      // Sort key is the hardware default APIC ID
      //
      ApicId = CpuMpData->CpuData[Index1].ApicId;
      for (Index2 = Index1 + 1; Index2 <= ApCount; Index2++) {
        if (ApicId > CpuMpData->CpuData[Index2].ApicId) {
          Index3 = Index2;
          ApicId = CpuMpData->CpuData[Index2].ApicId;
        }
      }
      if (Index3 != Index1) {
        CopyMem (&CpuData, &CpuMpData->CpuData[Index3], sizeof (CPU_AP_DATA));
        CopyMem (
          &CpuMpData->CpuData[Index3],
          &CpuMpData->CpuData[Index1],
          sizeof (CPU_AP_DATA)
          );
        CopyMem (&CpuMpData->CpuData[Index1], &CpuData, sizeof (CPU_AP_DATA));
      }
    }

    //
    // Get the processor number for the BSP
    //
    ApicId = GetInitialApicId ();
    for (Index1 = 0; Index1 < CpuMpData->CpuCount; Index1++) {
      if (CpuMpData->CpuData[Index1].ApicId == ApicId) {
        CpuMpData->BspNumber = (UINT32) Index1;
        break;
      }
    }

    CpuInfoInHob = (CPU_INFO_IN_HOB *) (UINTN) CpuMpData->CpuInfoInHob;
    for (Index1 = 0; Index1 < CpuMpData->CpuCount; Index1++) {
      CpuInfoInHob[Index1].InitialApicId = CpuMpData->CpuData[Index1].InitialApicId;
      CpuInfoInHob[Index1].ApicId        = CpuMpData->CpuData[Index1].ApicId;
      CpuInfoInHob[Index1].Health        = CpuMpData->CpuData[Index1].Health;
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
  // Sync BSP's MTRR table to AP
  //
  MtrrSetAllMtrrs (&CpuMpData->MtrrTable);
  //
  // Load microcode on AP
  //
  MicrocodeDetect (CpuMpData);
}

/**
  Find the current Processor number by APIC ID.

  @param[in] CpuMpData         Pointer to PEI CPU MP Data
  @param[in] ProcessorNumber   Return the pocessor number found

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

  TotalProcessorNumber = CpuMpData->CpuCount;
  for (Index = 0; Index < TotalProcessorNumber; Index ++) {
    if (CpuMpData->CpuData[Index].ApicId == GetApicId ()) {
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
  //
  // Send 1st broadcast IPI to APs to wakeup APs
  //
  CpuMpData->InitFlag     = ApInitConfig;
  CpuMpData->X2ApicEnable = FALSE;
  WakeUpAP (CpuMpData, TRUE, 0, NULL, NULL);
  //
  // Wait for AP task to complete and then exit.
  //
  MicroSecondDelay (PcdGet32(PcdCpuApInitTimeOutInMicroSeconds));
  CpuMpData->InitFlag = ApInitDone;
  ASSERT (CpuMpData->CpuCount <= PcdGet32 (PcdCpuMaxLogicalProcessorNumber));
  //
  // Wait for all APs finished the initialization
  //
  while (CpuMpData->FinishedCount < (CpuMpData->CpuCount - 1)) {
    CpuPause ();
  }

  if (CpuMpData->X2ApicEnable) {
    DEBUG ((DEBUG_INFO, "Force x2APIC mode!\n"));
    //
    // Wakeup all APs to enable x2APIC mode
    //
    WakeUpAP (CpuMpData, TRUE, 0, ApFuncEnableX2Apic, NULL);
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
  }
  DEBUG ((DEBUG_INFO, "APIC MODE is %d\n", GetApicMode ()));
  //
  // Sort BSP/Aps by CPU APIC ID in ascending order
  //
  SortApicId (CpuMpData);

  DEBUG ((DEBUG_INFO, "MpInitLib: Find %d processors in system.\n", CpuMpData->CpuCount));

  return CpuMpData->CpuCount;
}

/*
  Initialize CPU AP Data when AP is wakeup at the first time.

  @param[in, out] CpuMpData        Pointer to PEI CPU MP Data
  @param[in]      ProcessorNumber  The handle number of processor
  @param[in]      BistData         Processor BIST data

**/
VOID
InitializeApData (
  IN OUT CPU_MP_DATA      *CpuMpData,
  IN     UINTN            ProcessorNumber,
  IN     UINT32           BistData
  )
{
  CpuMpData->CpuData[ProcessorNumber].Waiting    = FALSE;
  CpuMpData->CpuData[ProcessorNumber].Health     = BistData;
  CpuMpData->CpuData[ProcessorNumber].CpuHealthy = (BistData == 0) ? TRUE : FALSE;
  CpuMpData->CpuData[ProcessorNumber].ApicId     = GetApicId ();
  CpuMpData->CpuData[ProcessorNumber].InitialApicId = GetInitialApicId ();
  if (CpuMpData->CpuData[ProcessorNumber].InitialApicId >= 0xFF) {
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
  @param[in] NumApsExecuting  Number of current executing AP
**/
VOID
EFIAPI
ApWakeupFunction (
  IN MP_CPU_EXCHANGE_INFO      *ExchangeInfo,
  IN UINTN                     NumApsExecuting
  )
{
  CPU_MP_DATA                *CpuMpData;
  UINTN                      ProcessorNumber;
  EFI_AP_PROCEDURE           Procedure;
  VOID                       *Parameter;
  UINT32                     BistData;
  volatile UINT32            *ApStartupSignalBuffer;

  //
  // AP finished assembly code and begin to execute C code
  //
  CpuMpData = ExchangeInfo->CpuMpData;

  ProgramVirtualWireMode (); 

  while (TRUE) {
    if (CpuMpData->InitFlag == ApInitConfig) {
      //
      // Add CPU number
      //
      InterlockedIncrement ((UINT32 *) &CpuMpData->CpuCount);
      ProcessorNumber = NumApsExecuting;
      //
      // This is first time AP wakeup, get BIST information from AP stack
      //
      BistData = *(UINT32 *) (CpuMpData->Buffer + ProcessorNumber * CpuMpData->CpuApStackSize - sizeof (UINTN));
      //
      // Do some AP initialize sync
      //
      ApInitializeSync (CpuMpData);
      //
      // Sync BSP's Control registers to APs
      //
      RestoreVolatileRegisters (&CpuMpData->CpuData[0].VolatileRegisters, FALSE);
      InitializeApData (CpuMpData, ProcessorNumber, BistData);
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
      }

      if (GetApState (&CpuMpData->CpuData[ProcessorNumber]) == CpuStateReady) {
        Procedure = (EFI_AP_PROCEDURE)CpuMpData->CpuData[ProcessorNumber].ApFunction;
        Parameter = (VOID *) CpuMpData->CpuData[ProcessorNumber].ApFunctionArgument;
        if (Procedure != NULL) {
          SetApState (&CpuMpData->CpuData[ProcessorNumber], CpuStateBusy);
          //
          // Invoke AP function here
          //
          Procedure (Parameter);
          //
          // Re-get the CPU APICID and Initial APICID
          //
          CpuMpData->CpuData[ProcessorNumber].ApicId        = GetApicId ();
          CpuMpData->CpuData[ProcessorNumber].InitialApicId = GetInitialApicId ();
        }
        SetApState (&CpuMpData->CpuData[ProcessorNumber], CpuStateFinished);
      }
    }

    //
    // AP finished executing C code
    //
    InterlockedIncrement ((UINT32 *) &CpuMpData->FinishedCount);

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
  ExchangeInfo->NumApsExecuting = 0;
  ExchangeInfo->CpuMpData       = CpuMpData;

  ExchangeInfo->EnableExecuteDisable = IsBspExecuteDisableEnabled ();

  //
  // Get the BSP's data of GDT and IDT
  //
  AsmReadGdtr ((IA32_DESCRIPTOR *) &ExchangeInfo->GdtrProfile);
  AsmReadIdtr ((IA32_DESCRIPTOR *) &ExchangeInfo->IdtrProfile);
}

/**
  This function will be called by BSP to wakeup AP.

  @param[in] CpuMpData          Pointer to CPU MP Data
  @param[in] Broadcast          TRUE:  Send broadcast IPI to all APs
                                FALSE: Send IPI to AP by ApicId
  @param[in] ProcessorNumber    The handle number of specified processor
  @param[in] Procedure          The function to be invoked by AP
  @param[in] ProcedureArgument  The argument to be passed into AP function
**/
VOID
WakeUpAP (
  IN CPU_MP_DATA               *CpuMpData,
  IN BOOLEAN                   Broadcast,
  IN UINTN                     ProcessorNumber,
  IN EFI_AP_PROCEDURE          Procedure,              OPTIONAL
  IN VOID                      *ProcedureArgument      OPTIONAL
  )
{
  volatile MP_CPU_EXCHANGE_INFO    *ExchangeInfo;
  UINTN                            Index;
  CPU_AP_DATA                      *CpuData;
  BOOLEAN                          ResetVectorRequired;

  CpuMpData->FinishedCount = 0;
  ResetVectorRequired = FALSE;

  if (CpuMpData->ApLoopMode == ApInHltLoop ||
      CpuMpData->InitFlag   != ApInitDone) {
    ResetVectorRequired = TRUE;
    AllocateResetVector (CpuMpData);
    FillExchangeInfoData (CpuMpData);
  } else if (CpuMpData->ApLoopMode == ApInMwaitLoop) {
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
    if (CpuMpData->InitFlag != ApInitConfig) {
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
      SendInitSipiSipi (
        CpuData->ApicId,
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

  OldCpuMpData = GetCpuMpDataFromGuidedHob ();
  if (OldCpuMpData == NULL) {
    MaxLogicalProcessorNumber = PcdGet32(PcdCpuMaxLogicalProcessorNumber);
  } else {
    MaxLogicalProcessorNumber = OldCpuMpData->CpuCount;
  }

  AsmGetAddressMap (&AddressMap);
  ApResetVectorSize = AddressMap.RendezvousFunnelSize + sizeof (MP_CPU_EXCHANGE_INFO);
  ApStackSize = PcdGet32(PcdCpuApStackSize);
  ApLoopMode  = GetApLoopMode (&MonitorFilterSize);

  BufferSize  = ApStackSize * MaxLogicalProcessorNumber;
  BufferSize += MonitorFilterSize * MaxLogicalProcessorNumber;
  BufferSize += sizeof (CPU_MP_DATA);
  BufferSize += ApResetVectorSize;
  BufferSize += (sizeof (CPU_AP_DATA) + sizeof (CPU_INFO_IN_HOB))* MaxLogicalProcessorNumber;
  MpBuffer    = AllocatePages (EFI_SIZE_TO_PAGES (BufferSize));
  ASSERT (MpBuffer != NULL);
  ZeroMem (MpBuffer, BufferSize);
  Buffer = (UINTN) MpBuffer;

  MonitorBuffer    = (UINT8 *) (Buffer + ApStackSize * MaxLogicalProcessorNumber);
  BackupBufferAddr = (UINTN) MonitorBuffer + MonitorFilterSize * MaxLogicalProcessorNumber;
  CpuMpData = (CPU_MP_DATA *) (BackupBufferAddr + ApResetVectorSize);
  CpuMpData->Buffer           = Buffer;
  CpuMpData->CpuApStackSize   = ApStackSize;
  CpuMpData->BackupBuffer     = BackupBufferAddr;
  CpuMpData->BackupBufferSize = ApResetVectorSize;
  CpuMpData->EndOfPeiFlag     = FALSE;
  CpuMpData->WakeupBuffer     = (UINTN) -1;
  CpuMpData->CpuCount         = 1;
  CpuMpData->BspNumber        = 0;
  CpuMpData->WaitEvent        = NULL;
  CpuMpData->CpuData          = (CPU_AP_DATA *) (CpuMpData + 1);
  CpuMpData->CpuInfoInHob     = (UINT64) (UINTN) (CpuMpData->CpuData + MaxLogicalProcessorNumber);
  InitializeSpinLock(&CpuMpData->MpLock);
  //
  // Save BSP's Control registers to APs
  //
  SaveVolatileRegisters (&CpuMpData->CpuData[0].VolatileRegisters);
  //
  // Set BSP basic information
  //
  InitializeApData (CpuMpData, 0, 0);
  //
  // Save assembly code information
  //
  CopyMem (&CpuMpData->AddressMap, &AddressMap, sizeof (MP_ASSEMBLY_ADDRESS_MAP));
  //
  // Finally set AP loop mode
  //
  CpuMpData->ApLoopMode = ApLoopMode;
  DEBUG ((DEBUG_INFO, "AP Loop Mode is %d\n", CpuMpData->ApLoopMode));
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
  MicrocodeDetect (CpuMpData);
  //
  // Store BSP's MTRR setting
  //
  MtrrGetAllMtrrs (&CpuMpData->MtrrTable);

  if (OldCpuMpData == NULL) {
    //
    // Wakeup all APs and calculate the processor count in system
    //
    CollectProcessorCount (CpuMpData);
  } else {
    //
    // APs have been wakeup before, just get the CPU Information
    // from HOB
    //
    CpuMpData->CpuCount  = OldCpuMpData->CpuCount;
    CpuMpData->BspNumber = OldCpuMpData->BspNumber;
    CpuMpData->InitFlag  = ApInitReconfig;
    CpuInfoInHob = (CPU_INFO_IN_HOB *) (UINTN) OldCpuMpData->CpuInfoInHob;
    for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
      InitializeSpinLock(&CpuMpData->CpuData[Index].ApLock);
      CpuMpData->CpuData[Index].ApicId        = CpuInfoInHob[Index].ApicId;
      CpuMpData->CpuData[Index].InitialApicId = CpuInfoInHob[Index].InitialApicId;
      if (CpuMpData->CpuData[Index].InitialApicId >= 255) {
        CpuMpData->X2ApicEnable = TRUE;
      }
      CpuMpData->CpuData[Index].Health     = CpuInfoInHob[Index].Health;
      CpuMpData->CpuData[Index].CpuHealthy = (CpuMpData->CpuData[Index].Health == 0)? TRUE:FALSE;
      CpuMpData->CpuData[Index].ApFunction = 0;
      CopyMem (
        &CpuMpData->CpuData[Index].VolatileRegisters,
        &CpuMpData->CpuData[0].VolatileRegisters,
        sizeof (CPU_VOLATILE_REGISTERS)
        );
    }
    //
    // Wakeup APs to do some AP initialize sync
    //
    WakeUpAP (CpuMpData, TRUE, 0, ApInitializeSync, CpuMpData);
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

  ProcessorInfoBuffer->ProcessorId = (UINT64) CpuMpData->CpuData[ProcessorNumber].ApicId;
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
  ExtractProcessorLocation (CpuMpData->CpuData[ProcessorNumber].ApicId, &ProcessorInfoBuffer->Location);

  if (HealthData != NULL) {
    HealthData->Uint32 = CpuMpData->CpuData[ProcessorNumber].Health;
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

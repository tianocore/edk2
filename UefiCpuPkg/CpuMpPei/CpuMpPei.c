/** @file
  CPU PEI Module installs CPU Multiple Processor PPI.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuMpPei.h"

//
// Global Descriptor Table (GDT)
//
GLOBAL_REMOVE_IF_UNREFERENCED IA32_GDT mGdtEntries[] = {
/* selector { Global Segment Descriptor                              } */
/* 0x00 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}}, //null descriptor
/* 0x08 */  {{0xffff, 0,  0,  0x2,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //linear data segment descriptor
/* 0x10 */  {{0xffff, 0,  0,  0xf,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //linear code segment descriptor
/* 0x18 */  {{0xffff, 0,  0,  0x3,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //system data segment descriptor
/* 0x20 */  {{0xffff, 0,  0,  0xa,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //system code segment descriptor
/* 0x28 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}}, //spare segment descriptor
/* 0x30 */  {{0xffff, 0,  0,  0x2,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //system data segment descriptor
/* 0x38 */  {{0xffff, 0,  0,  0xa,  1,  0,  1,  0xf,  0,  1, 0,  1,  0}}, //system code segment descriptor
/* 0x40 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}}, //spare segment descriptor
};

//
// IA32 Gdt register
//
GLOBAL_REMOVE_IF_UNREFERENCED IA32_DESCRIPTOR mGdt = {
  sizeof (mGdtEntries) - 1,
  (UINTN) mGdtEntries
  };

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_NOTIFY_DESCRIPTOR mNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  CpuMpEndOfPeiCallback
};

/**
  Sort the APIC ID of all processors.

  This function sorts the APIC ID of all processors so that processor number is
  assigned in the ascending order of APIC ID which eases MP debugging.

  @param PeiCpuMpData        Pointer to PEI CPU MP Data
**/
VOID
SortApicId (
  IN PEI_CPU_MP_DATA   *PeiCpuMpData
  )
{
  UINTN             Index1;
  UINTN             Index2;
  UINTN             Index3;
  UINT32            ApicId;
  PEI_CPU_DATA      CpuData;
  UINT32            ApCount;

  ApCount = PeiCpuMpData->CpuCount - 1;

  if (ApCount != 0) {
    for (Index1 = 0; Index1 < ApCount; Index1++) {
      Index3 = Index1;
      //
      // Sort key is the hardware default APIC ID
      //
      ApicId = PeiCpuMpData->CpuData[Index1].ApicId;
      for (Index2 = Index1 + 1; Index2 <= ApCount; Index2++) {
        if (ApicId > PeiCpuMpData->CpuData[Index2].ApicId) {
          Index3 = Index2;
          ApicId = PeiCpuMpData->CpuData[Index2].ApicId;
        }
      }
      if (Index3 != Index1) {
        CopyMem (&CpuData, &PeiCpuMpData->CpuData[Index3], sizeof (PEI_CPU_DATA));
        CopyMem (
          &PeiCpuMpData->CpuData[Index3],
          &PeiCpuMpData->CpuData[Index1],
          sizeof (PEI_CPU_DATA)
          );
        CopyMem (&PeiCpuMpData->CpuData[Index1], &CpuData, sizeof (PEI_CPU_DATA));
      }
    }

    //
    // Get the processor number for the BSP
    //
    ApicId = GetInitialApicId ();
    for (Index1 = 0; Index1 < PeiCpuMpData->CpuCount; Index1++) {
      if (PeiCpuMpData->CpuData[Index1].ApicId == ApicId) {
        PeiCpuMpData->BspNumber = (UINT32) Index1;
        break;
      }
    }
  }
}

/**
  Enable x2APIC mode on APs.

  @param Buffer  Pointer to private data buffer.
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
  Get AP loop mode.

  @param MonitorFilterSize  Returns the largest monitor-line size in bytes.

  @return The AP loop mode.
**/
UINT8
GetApLoopMode (
  OUT UINT16     *MonitorFilterSize
  )
{
  UINT8          ApLoopMode;
  UINT32         RegEbx;
  UINT32         RegEcx;
  UINT32         RegEdx;

  ASSERT (MonitorFilterSize != NULL);

  ApLoopMode = PcdGet8 (PcdCpuApLoopMode);
  ASSERT (ApLoopMode >= ApInHltLoop && ApLoopMode <= ApInRunLoop);
  if (ApLoopMode == ApInMwaitLoop) {
    AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, &RegEcx, NULL);
    if ((RegEcx & BIT3) == 0) {
      //
      // If processor does not support MONITOR/MWAIT feature
      // by CPUID.[EAX=01H]:ECX.BIT3, force AP in Hlt-loop mode
      //
      ApLoopMode = ApInHltLoop;
    }
  }

  if (ApLoopMode == ApInHltLoop) {
    *MonitorFilterSize = 0;
  } else if (ApLoopMode == ApInRunLoop) {
    *MonitorFilterSize = sizeof (UINT32);
  } else if (ApLoopMode == ApInMwaitLoop) {
    //
    // CPUID.[EAX=05H]:EBX.BIT0-15: Largest monitor-line size in bytes
    // CPUID.[EAX=05H].EDX: C-states supported using MWAIT
    //
    AsmCpuid (CPUID_MONITOR_MWAIT, NULL, &RegEbx, NULL, &RegEdx);
    *MonitorFilterSize = RegEbx & 0xFFFF;
  }

  return ApLoopMode;
}

/**
  Get CPU MP Data pointer from the Guided HOB.

  @return  Pointer to Pointer to PEI CPU MP Data
**/
PEI_CPU_MP_DATA *
GetMpHobData (
  VOID
  )
{
  EFI_HOB_GUID_TYPE       *GuidHob;
  VOID                    *DataInHob;
  PEI_CPU_MP_DATA         *CpuMpData;

  CpuMpData = NULL;
  GuidHob = GetFirstGuidHob (&gEfiCallerIdGuid);
  if (GuidHob != NULL) {
    DataInHob = GET_GUID_HOB_DATA (GuidHob);
    CpuMpData = (PEI_CPU_MP_DATA *)(*(UINTN *)DataInHob);
  }
  ASSERT (CpuMpData != NULL);
  return CpuMpData;
}

/**
  Save the volatile registers required to be restored following INIT IPI.
  
  @param  VolatileRegisters    Returns buffer saved the volatile resisters
**/
VOID
SaveVolatileRegisters (
  OUT CPU_VOLATILE_REGISTERS    *VolatileRegisters
  )
{
  UINT32                        RegEdx;

  VolatileRegisters->Cr0 = AsmReadCr0 ();
  VolatileRegisters->Cr3 = AsmReadCr3 ();
  VolatileRegisters->Cr4 = AsmReadCr4 ();

  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &RegEdx);
  if ((RegEdx & BIT2) != 0) {
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
  
  @param  VolatileRegisters   Pointer to volatile resisters
  @param  IsRestoreDr         TRUE:  Restore DRx if supported
                              FALSE: Do not restore DRx
**/
VOID
RestoreVolatileRegisters (
  IN CPU_VOLATILE_REGISTERS    *VolatileRegisters,
  IN BOOLEAN                   IsRestoreDr
  )
{
  UINT32                        RegEdx;

  AsmWriteCr0 (VolatileRegisters->Cr0);
  AsmWriteCr3 (VolatileRegisters->Cr3);
  AsmWriteCr4 (VolatileRegisters->Cr4);

  if (IsRestoreDr) {
    AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &RegEdx);
    if ((RegEdx & BIT2) != 0) {
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
  This function will be called from AP reset code if BSP uses WakeUpAP.

  @param ExchangeInfo     Pointer to the MP exchange info buffer
  @param NumApsExecuting  Number of current executing AP
**/
VOID
EFIAPI
ApCFunction (
  IN MP_CPU_EXCHANGE_INFO      *ExchangeInfo,
  IN UINTN                     NumApsExecuting
  )
{
  PEI_CPU_MP_DATA            *PeiCpuMpData;
  UINTN                      ProcessorNumber;
  EFI_AP_PROCEDURE           Procedure;
  UINTN                      BistData;
  volatile UINT32            *ApStartupSignalBuffer;

  PeiCpuMpData = ExchangeInfo->PeiCpuMpData;
  while (TRUE) {
    if (PeiCpuMpData->InitFlag) {
      ProcessorNumber = NumApsExecuting;
      //
      // Sync BSP's Control registers to APs
      //
      RestoreVolatileRegisters (&PeiCpuMpData->CpuData[0].VolatileRegisters, FALSE);
      //
      // This is first time AP wakeup, get BIST information from AP stack
      //
      BistData = *(UINTN *) (PeiCpuMpData->Buffer + ProcessorNumber * PeiCpuMpData->CpuApStackSize - sizeof (UINTN));
      PeiCpuMpData->CpuData[ProcessorNumber].Health.Uint32 = (UINT32) BistData;
      PeiCpuMpData->CpuData[ProcessorNumber].ApicId = GetInitialApicId ();
      if (PeiCpuMpData->CpuData[ProcessorNumber].ApicId >= 0xFF) {
        //
        // Set x2APIC mode if there are any logical processor reporting
        // an APIC ID of 255 or greater.
        //
        AcquireSpinLock(&PeiCpuMpData->MpLock);
        PeiCpuMpData->X2ApicEnable = TRUE;
        ReleaseSpinLock(&PeiCpuMpData->MpLock);
      }
      //
      // Sync BSP's Mtrr table to all wakeup APs and load microcode on APs.
      //
      MtrrSetAllMtrrs (&PeiCpuMpData->MtrrTable);
      MicrocodeDetect ();
      PeiCpuMpData->CpuData[ProcessorNumber].State = CpuStateIdle;
    } else {
      //
      // Execute AP function if AP is not disabled
      //
      GetProcessorNumber (PeiCpuMpData, &ProcessorNumber);
      if (PeiCpuMpData->ApLoopMode == ApInHltLoop) {
        //
        // Restore AP's volatile registers saved
        //
        RestoreVolatileRegisters (&PeiCpuMpData->CpuData[ProcessorNumber].VolatileRegisters, TRUE);
      }

      if ((PeiCpuMpData->CpuData[ProcessorNumber].State != CpuStateDisabled) &&
          (PeiCpuMpData->ApFunction != 0)) {
        PeiCpuMpData->CpuData[ProcessorNumber].State = CpuStateBusy;
        Procedure = (EFI_AP_PROCEDURE)(UINTN)PeiCpuMpData->ApFunction;
        //
        // Invoke AP function here
        //
        Procedure ((VOID *)(UINTN)PeiCpuMpData->ApFunctionArgument);
        //
        // Re-get the processor number due to BSP/AP maybe exchange in AP function
        //
        GetProcessorNumber (PeiCpuMpData, &ProcessorNumber);
        PeiCpuMpData->CpuData[ProcessorNumber].State = CpuStateIdle;
      }
    }

    //
    // AP finished executing C code
    //
    InterlockedIncrement ((UINT32 *)&PeiCpuMpData->FinishedCount);

    //
    // Place AP is specified loop mode
    //
    if (PeiCpuMpData->ApLoopMode == ApInHltLoop) {
      //
      // Save AP volatile registers
      //
      SaveVolatileRegisters (&PeiCpuMpData->CpuData[ProcessorNumber].VolatileRegisters);
      //
      // Place AP in Hlt-loop
      //
      while (TRUE) {
        DisableInterrupts ();
        CpuSleep ();
        CpuPause ();
      }
    }
    ApStartupSignalBuffer = PeiCpuMpData->CpuData[ProcessorNumber].StartupApSignal;
    //
    // Clear AP start-up signal
    //
    *ApStartupSignalBuffer = 0;
    while (TRUE) {
      DisableInterrupts ();
      if (PeiCpuMpData->ApLoopMode == ApInMwaitLoop) {
        //
        // Place AP in Mwait-loop
        //
        AsmMonitor ((UINTN)ApStartupSignalBuffer, 0, 0);
        if (*ApStartupSignalBuffer != WAKEUP_AP_SIGNAL) {
          //
          // If AP start-up signal is not set, place AP into
          // the maximum C-state
          //
          AsmMwait (PeiCpuMpData->ApTargetCState << 4, 0);
        }
      } else if (PeiCpuMpData->ApLoopMode == ApInRunLoop) {
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
  This function will be called by BSP to wakeup AP.

  @param PeiCpuMpData       Pointer to PEI CPU MP Data
  @param Broadcast          TRUE:  Send broadcast IPI to all APs
                            FALSE: Send IPI to AP by ApicId
  @param ProcessorNumber    The handle number of specified processor
  @param Procedure          The function to be invoked by AP
  @param ProcedureArgument  The argument to be passed into AP function
**/
VOID
WakeUpAP (
  IN PEI_CPU_MP_DATA           *PeiCpuMpData,
  IN BOOLEAN                   Broadcast,
  IN UINTN                     ProcessorNumber,
  IN EFI_AP_PROCEDURE          Procedure,              OPTIONAL
  IN VOID                      *ProcedureArgument      OPTIONAL
  )
{
  volatile MP_CPU_EXCHANGE_INFO    *ExchangeInfo;
  UINTN                            Index;

  PeiCpuMpData->ApFunction         = (UINTN) Procedure;
  PeiCpuMpData->ApFunctionArgument = (UINTN) ProcedureArgument;
  PeiCpuMpData->FinishedCount      = 0;

  ExchangeInfo                     = PeiCpuMpData->MpCpuExchangeInfo;
  ExchangeInfo->Lock               = 0;
  ExchangeInfo->StackStart         = PeiCpuMpData->Buffer;
  ExchangeInfo->StackSize          = PeiCpuMpData->CpuApStackSize;
  ExchangeInfo->BufferStart        = PeiCpuMpData->WakeupBuffer;
  ExchangeInfo->PmodeOffset        = PeiCpuMpData->AddressMap.PModeEntryOffset;
  ExchangeInfo->LmodeOffset        = PeiCpuMpData->AddressMap.LModeEntryOffset;
  ExchangeInfo->Cr3                = AsmReadCr3 ();
  ExchangeInfo->CFunction          = (UINTN) ApCFunction;
  ExchangeInfo->NumApsExecuting    = 0;
  ExchangeInfo->PeiCpuMpData       = PeiCpuMpData;

  //
  // Get the BSP's data of GDT and IDT
  //
  CopyMem ((VOID *)&ExchangeInfo->GdtrProfile, &mGdt, sizeof(mGdt));
  AsmReadIdtr ((IA32_DESCRIPTOR *) &ExchangeInfo->IdtrProfile);

  if (PeiCpuMpData->ApLoopMode == ApInMwaitLoop) {
    //
    // Get AP target C-state each time when waking up AP,
    // for it maybe updated by platform again
    //
    PeiCpuMpData->ApTargetCState = PcdGet8 (PcdCpuApTargetCstate);
  }

  //
  // Wakeup APs per AP loop state
  //
  if (PeiCpuMpData->ApLoopMode == ApInHltLoop || PeiCpuMpData->InitFlag) {
    if (Broadcast) {
      SendInitSipiSipiAllExcludingSelf ((UINT32) ExchangeInfo->BufferStart);
    } else {
      SendInitSipiSipi (
        PeiCpuMpData->CpuData[ProcessorNumber].ApicId,
        (UINT32) ExchangeInfo->BufferStart
        );
    }
  } else if ((PeiCpuMpData->ApLoopMode == ApInMwaitLoop) ||
             (PeiCpuMpData->ApLoopMode == ApInRunLoop)) {
    if (Broadcast) {
      for (Index = 0; Index < PeiCpuMpData->CpuCount; Index++) {
        if (Index != PeiCpuMpData->BspNumber) {
          *(PeiCpuMpData->CpuData[Index].StartupApSignal) = WAKEUP_AP_SIGNAL;
        }
      }
    } else {
      *(PeiCpuMpData->CpuData[ProcessorNumber].StartupApSignal) = WAKEUP_AP_SIGNAL;
    }
  } else {
    ASSERT (FALSE);
  }
  return ;
}

/**
  Get available system memory below 1MB by specified size.

  @param  WakeupBufferSize   Wakeup buffer size required

  @retval other   Return wakeup buffer address below 1MB.
  @retval -1      Cannot find free memory below 1MB.
**/
UINTN
GetWakeupBuffer (
  IN UINTN                WakeupBufferSize
  )
{
  EFI_PEI_HOB_POINTERS    Hob;
  UINTN                   WakeupBufferStart;
  UINTN                   WakeupBufferEnd;

  //
  // Get the HOB list for processing
  //
  Hob.Raw = GetHobList ();

  //
  // Collect memory ranges
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if ((Hob.ResourceDescriptor->PhysicalStart < BASE_1MB) &&
          (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) &&
          ((Hob.ResourceDescriptor->ResourceAttribute &
            (EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED |
             EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED |
             EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED
             )) == 0)
           ) {
        //
        // Need memory under 1MB to be collected here
        //
        WakeupBufferEnd = (UINTN) (Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength);
        if (WakeupBufferEnd > BASE_1MB) {
          //
          // Wakeup buffer should be under 1MB
          //
          WakeupBufferEnd = BASE_1MB;
        }
        //
        // Wakeup buffer should be aligned on 4KB
        //
        WakeupBufferStart = (WakeupBufferEnd - WakeupBufferSize) & ~(SIZE_4KB - 1);
        if (WakeupBufferStart < Hob.ResourceDescriptor->PhysicalStart) {
          continue;
        }
        //
        // Create a memory allocation HOB.
        //
        BuildMemoryAllocationHob (
          WakeupBufferStart,
          WakeupBufferSize,
          EfiBootServicesData
          );
        return WakeupBufferStart;
      }
    }
    //
    // Find the next HOB
    //
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return (UINTN) -1;
}

/**
  Get available system memory below 1MB by specified size.

  @param PeiCpuMpData        Pointer to PEI CPU MP Data
**/
VOID
BackupAndPrepareWakeupBuffer(
  IN PEI_CPU_MP_DATA         *PeiCpuMpData
  )
{
  CopyMem (
    (VOID *) PeiCpuMpData->BackupBuffer,
    (VOID *) PeiCpuMpData->WakeupBuffer,
    PeiCpuMpData->BackupBufferSize
    );
  CopyMem (
    (VOID *) PeiCpuMpData->WakeupBuffer,
    (VOID *) PeiCpuMpData->AddressMap.RendezvousFunnelAddress,
    PeiCpuMpData->AddressMap.RendezvousFunnelSize
    );
}

/**
  Restore wakeup buffer data.

  @param PeiCpuMpData        Pointer to PEI CPU MP Data
**/
VOID
RestoreWakeupBuffer(
  IN PEI_CPU_MP_DATA         *PeiCpuMpData
  )
{
  CopyMem ((VOID *) PeiCpuMpData->WakeupBuffer, (VOID *) PeiCpuMpData->BackupBuffer, PeiCpuMpData->BackupBufferSize);
}

/**
  This function will get CPU count in the system.

  @param PeiCpuMpData        Pointer to PEI CPU MP Data

  @return  AP processor count
**/
UINT32
CountProcessorNumber (
  IN PEI_CPU_MP_DATA            *PeiCpuMpData
  )
{
  //
  // Load Microcode on BSP
  //
  MicrocodeDetect ();
  //
  // Store BSP's MTRR setting
  //
  MtrrGetAllMtrrs (&PeiCpuMpData->MtrrTable);

  //
  // Only perform AP detection if PcdCpuMaxLogicalProcessorNumber is greater than 1
  //
  if (PcdGet32 (PcdCpuMaxLogicalProcessorNumber) > 1) {
    //
    // Send 1st broadcast IPI to APs to wakeup APs
    //
    PeiCpuMpData->InitFlag     = TRUE;
    PeiCpuMpData->X2ApicEnable = FALSE;
    WakeUpAP (PeiCpuMpData, TRUE, 0, NULL, NULL);
    //
    // Wait for AP task to complete and then exit.
    //
    MicroSecondDelay (PcdGet32 (PcdCpuApInitTimeOutInMicroSeconds));
    PeiCpuMpData->InitFlag  = FALSE;
    PeiCpuMpData->CpuCount += (UINT32)PeiCpuMpData->MpCpuExchangeInfo->NumApsExecuting;
    ASSERT (PeiCpuMpData->CpuCount <= PcdGet32 (PcdCpuMaxLogicalProcessorNumber));
    //
    // Wait for all APs finished the initialization
    //
    while (PeiCpuMpData->FinishedCount < (PeiCpuMpData->CpuCount - 1)) {
      CpuPause ();
    }

    if (PeiCpuMpData->X2ApicEnable) {
      DEBUG ((EFI_D_INFO, "Force x2APIC mode!\n"));
      //
      // Wakeup all APs to enable x2APIC mode
      //
      WakeUpAP (PeiCpuMpData, TRUE, 0, ApFuncEnableX2Apic, NULL);
      //
      // Wait for all known APs finished
      //
      while (PeiCpuMpData->FinishedCount < (PeiCpuMpData->CpuCount - 1)) {
        CpuPause ();
      }
      //
      // Enable x2APIC on BSP
      //
      SetApicMode (LOCAL_APIC_MODE_X2APIC);
    }
    DEBUG ((EFI_D_INFO, "APIC MODE is %d\n", GetApicMode ()));
    //
    // Sort BSP/Aps by CPU APIC ID in ascending order
    //
    SortApicId (PeiCpuMpData);
  }

  DEBUG ((EFI_D_INFO, "CpuMpPei: Find %d processors in system.\n", PeiCpuMpData->CpuCount));
  return PeiCpuMpData->CpuCount;
}

/**
  Prepare for AP wakeup buffer and copy AP reset code into it.

  Get wakeup buffer below 1MB. Allocate memory for CPU MP Data and APs Stack.

  @return   Pointer to PEI CPU MP Data
**/
PEI_CPU_MP_DATA *
PrepareAPStartupVector (
  VOID
  )
{
  EFI_STATUS                    Status;
  UINT32                        MaxCpuCount;
  PEI_CPU_MP_DATA               *PeiCpuMpData;
  EFI_PHYSICAL_ADDRESS          Buffer;
  UINTN                         BufferSize;
  UINTN                         WakeupBuffer;
  UINTN                         WakeupBufferSize;
  MP_ASSEMBLY_ADDRESS_MAP       AddressMap;
  UINT8                         ApLoopMode;
  UINT16                        MonitorFilterSize;
  UINT8                         *MonitorBuffer;
  UINTN                         Index;

  AsmGetAddressMap (&AddressMap);
  WakeupBufferSize = AddressMap.RendezvousFunnelSize + sizeof (MP_CPU_EXCHANGE_INFO);
  WakeupBuffer     = GetWakeupBuffer ((WakeupBufferSize + SIZE_4KB - 1) & ~(SIZE_4KB - 1));
  ASSERT (WakeupBuffer != (UINTN) -1);
  DEBUG ((EFI_D_INFO, "CpuMpPei: WakeupBuffer = 0x%x\n", WakeupBuffer));

  //
  // Allocate Pages for APs stack, CPU MP Data, backup buffer for wakeup buffer,
  // and monitor buffer if required.
  //
  MaxCpuCount = PcdGet32(PcdCpuMaxLogicalProcessorNumber);
  BufferSize  = PcdGet32 (PcdCpuApStackSize) * MaxCpuCount + sizeof (PEI_CPU_MP_DATA)
                  + WakeupBufferSize + sizeof (PEI_CPU_DATA) * MaxCpuCount;
  ApLoopMode = GetApLoopMode (&MonitorFilterSize);
  BufferSize += MonitorFilterSize * MaxCpuCount;
  Status = PeiServicesAllocatePages (
             EfiBootServicesData,
             EFI_SIZE_TO_PAGES (BufferSize),
             &Buffer
             );
  ASSERT_EFI_ERROR (Status);

  PeiCpuMpData = (PEI_CPU_MP_DATA *) (UINTN) (Buffer + PcdGet32 (PcdCpuApStackSize) * MaxCpuCount);
  PeiCpuMpData->Buffer            = (UINTN) Buffer;
  PeiCpuMpData->CpuApStackSize    = PcdGet32 (PcdCpuApStackSize);
  PeiCpuMpData->WakeupBuffer      = WakeupBuffer;
  PeiCpuMpData->BackupBuffer      = (UINTN)PeiCpuMpData + sizeof (PEI_CPU_MP_DATA);
  PeiCpuMpData->BackupBufferSize  = WakeupBufferSize;
  PeiCpuMpData->MpCpuExchangeInfo = (MP_CPU_EXCHANGE_INFO *) (UINTN) (WakeupBuffer + AddressMap.RendezvousFunnelSize);

  PeiCpuMpData->CpuCount                 = 1;
  PeiCpuMpData->BspNumber                = 0;
  PeiCpuMpData->CpuData                  = (PEI_CPU_DATA *) (PeiCpuMpData->BackupBuffer +
                                                             PeiCpuMpData->BackupBufferSize);
  PeiCpuMpData->CpuData[0].ApicId        = GetInitialApicId ();
  PeiCpuMpData->CpuData[0].Health.Uint32 = 0;
  PeiCpuMpData->EndOfPeiFlag             = FALSE;
  InitializeSpinLock(&PeiCpuMpData->MpLock);
  SaveVolatileRegisters (&PeiCpuMpData->CpuData[0].VolatileRegisters);
  CopyMem (&PeiCpuMpData->AddressMap, &AddressMap, sizeof (MP_ASSEMBLY_ADDRESS_MAP));
  //
  // Initialize AP loop mode
  //
  PeiCpuMpData->ApLoopMode = ApLoopMode;
  DEBUG ((EFI_D_INFO, "AP Loop Mode is %d\n", PeiCpuMpData->ApLoopMode));
  MonitorBuffer = (UINT8 *)(PeiCpuMpData->CpuData + MaxCpuCount);
  if (PeiCpuMpData->ApLoopMode != ApInHltLoop) {
    //
    // Set up APs wakeup signal buffer
    //
    for (Index = 0; Index < MaxCpuCount; Index++) {
      PeiCpuMpData->CpuData[Index].StartupApSignal = 
        (UINT32 *)(MonitorBuffer + MonitorFilterSize * Index);
    }
  }
  //
  // Backup original data and copy AP reset code in it
  //
  BackupAndPrepareWakeupBuffer(PeiCpuMpData);

  return PeiCpuMpData;
}

/**
  Notify function on End Of Pei PPI.

  On S3 boot, this function will restore wakeup buffer data.
  On normal boot, this function will flag wakeup buffer to be un-used type.

  @param  PeiServices        The pointer to the PEI Services Table.
  @param  NotifyDescriptor   Address of the notification descriptor data structure.
  @param  Ppi                Address of the PPI that was installed.

  @retval EFI_SUCCESS        When everything is OK.

**/
EFI_STATUS
EFIAPI
CpuMpEndOfPeiCallback (
  IN      EFI_PEI_SERVICES        **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR    *NotifyDescriptor,
  IN VOID                         *Ppi
  )
{
  EFI_STATUS                Status;
  EFI_BOOT_MODE             BootMode;
  PEI_CPU_MP_DATA           *PeiCpuMpData;
  EFI_PEI_HOB_POINTERS      Hob;
  EFI_HOB_MEMORY_ALLOCATION *MemoryHob;

  DEBUG ((EFI_D_INFO, "CpuMpPei: CpuMpEndOfPeiCallback () invoked\n"));

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  PeiCpuMpData = GetMpHobData ();
  ASSERT (PeiCpuMpData != NULL);

  if (BootMode != BOOT_ON_S3_RESUME) {
    //
    // Get the HOB list for processing
    //
    Hob.Raw = GetHobList ();
    //
    // Collect memory ranges
    //
    while (!END_OF_HOB_LIST (Hob)) {
      if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
        MemoryHob = Hob.MemoryAllocation;
        if(MemoryHob->AllocDescriptor.MemoryBaseAddress == PeiCpuMpData->WakeupBuffer) {
          //
          // Flag this HOB type to un-used
          //
          GET_HOB_TYPE (Hob) = EFI_HOB_TYPE_UNUSED;
          break;
        }
      }
      Hob.Raw = GET_NEXT_HOB (Hob);
    }
  } else {
    RestoreWakeupBuffer (PeiCpuMpData);
    PeiCpuMpData->EndOfPeiFlag = TRUE;
  }
  return EFI_SUCCESS;
}

/**
  The Entry point of the MP CPU PEIM.

  This function will wakeup APs and collect CPU AP count and install the
  Mp Service Ppi.

  @param  FileHandle    Handle of the file being invoked.
  @param  PeiServices   Describes the list of possible PEI Services.

  @retval EFI_SUCCESS   MpServicePpi is installed successfully.

**/
EFI_STATUS
EFIAPI
CpuMpPeimInit (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS           Status;
  PEI_CPU_MP_DATA      *PeiCpuMpData;
  UINT32               ProcessorCount;

  //
  // Load new GDT table on BSP
  //
  AsmInitializeGdt (&mGdt);
  //
  // Get wakeup buffer and copy AP reset code in it
  //
  PeiCpuMpData = PrepareAPStartupVector ();
  //
  // Count processor number and collect processor information
  //
  ProcessorCount = CountProcessorNumber (PeiCpuMpData);
  //
  // Build location of PEI CPU MP DATA buffer in HOB
  //
  BuildGuidDataHob (
    &gEfiCallerIdGuid,
    (VOID *)&PeiCpuMpData,
    sizeof(UINT64)
    );
  //
  // Update and publish CPU BIST information
  //
  CollectBistDataFromPpi (PeiServices, PeiCpuMpData);
  //
  // register an event for EndOfPei
  //
  Status  = PeiServicesNotifyPpi (&mNotifyList);
  ASSERT_EFI_ERROR (Status);
  //
  // Install CPU MP PPI
  //
  Status = PeiServicesInstallPpi(&mPeiCpuMpPpiDesc);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

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
  MaxLogicalProcessorNumber = PcdGet32(PcdCpuMaxLogicalProcessorNumber);

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
  return EFI_UNSUPPORTED;
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
  return EFI_UNSUPPORTED;
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
  return EFI_UNSUPPORTED;
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

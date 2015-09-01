/** @file
  Implementation of Multiple Processor PPI services.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PeiMpServices.h"

//
// CPU MP PPI to be installed
//
EFI_PEI_MP_SERVICES_PPI                mMpServicesPpi = {
  PeiGetNumberOfProcessors,
  PeiGetProcessorInfo,
  PeiStartupAllAPs,
  PeiStartupThisAP,
  PeiSwitchBSP,
  PeiEnableDisableAP,
  PeiWhoAmI,
};

EFI_PEI_PPI_DESCRIPTOR           mPeiCpuMpPpiDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMpServicesPpiGuid,
  &mMpServicesPpi
};

/**
  Get CPU Package/Core/Thread location information.

  @param InitialApicId     CPU APIC ID
  @param Location          Pointer to CPU location information
**/
VOID
ExtractProcessorLocation (
  IN  UINT32                     InitialApicId,
  OUT EFI_CPU_PHYSICAL_LOCATION  *Location
  )
{
  BOOLEAN  TopologyLeafSupported;
  UINTN    ThreadBits;
  UINTN    CoreBits;
  UINT32   RegEax;
  UINT32   RegEbx;
  UINT32   RegEcx;
  UINT32   RegEdx;
  UINT32   MaxCpuIdIndex;
  UINT32   SubIndex;
  UINTN    LevelType;
  UINT32   MaxLogicProcessorsPerPackage;
  UINT32   MaxCoresPerPackage;

  //
  // Check if the processor is capable of supporting more than one logical processor.
  //
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &RegEdx);
  if ((RegEdx & BIT28) == 0) {
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
    AsmCpuidEx (CPUID_EXTENDED_TOPOLOGY, 0, &RegEax, &RegEbx, &RegEcx, NULL);
    //
    // If CPUID.(EAX=0BH, ECX=0H):EBX returns zero and maximum input value for
    // basic CPUID information is greater than 0BH, then CPUID.0BH leaf is not
    // supported on that processor.
    //
    if (RegEbx != 0) {
      TopologyLeafSupported = TRUE;

      //
      // Sub-leaf index 0 (ECX= 0 as input) provides enumeration parameters to extract
      // the SMT sub-field of x2APIC ID.
      //
      LevelType = (RegEcx >> 8) & 0xff;
      ASSERT (LevelType == CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_SMT);
      ThreadBits = RegEax & 0x1f;

      //
      // Software must not assume any "level type" encoding
      // value to be related to any sub-leaf index, except sub-leaf 0.
      //
      SubIndex = 1;
      do {
        AsmCpuidEx (CPUID_EXTENDED_TOPOLOGY, SubIndex, &RegEax, NULL, &RegEcx, NULL);
        LevelType = (RegEcx >> 8) & 0xff;
        if (LevelType == CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_CORE) {
          CoreBits = (RegEax & 0x1f) - ThreadBits;
          break;
        }
        SubIndex++;
      } while (LevelType != CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_INVALID);
    }
  }

  if (!TopologyLeafSupported) {
    AsmCpuid (CPUID_VERSION_INFO, NULL, &RegEbx, NULL, NULL);
    MaxLogicProcessorsPerPackage = (RegEbx >> 16) & 0xff;
    if (MaxCpuIdIndex >= CPUID_CACHE_PARAMS) {
      AsmCpuidEx (CPUID_CACHE_PARAMS, 0, &RegEax, NULL, NULL, NULL);
      MaxCoresPerPackage = (RegEax >> 26) + 1;
    } else {
      //
      // Must be a single-core processor.
      //
      MaxCoresPerPackage = 1;
    }

    ThreadBits = (UINTN) (HighBitSet32 (MaxLogicProcessorsPerPackage / MaxCoresPerPackage - 1) + 1);
    CoreBits = (UINTN) (HighBitSet32 (MaxCoresPerPackage - 1) + 1);
  }

  Location->Thread  = InitialApicId & ~((-1) << ThreadBits);
  Location->Core    = (InitialApicId >> ThreadBits) & ~((-1) << CoreBits);
  Location->Package = (InitialApicId >> (ThreadBits + CoreBits));
}

/**
  Find the current Processor number by APIC ID.

  @param PeiCpuMpData        Pointer to PEI CPU MP Data
  @param ProcessorNumber     Return the pocessor number found

  @retval EFI_SUCCESS        ProcessorNumber is found and returned.
  @retval EFI_NOT_FOUND      ProcessorNumber is not found.
**/
EFI_STATUS
GetProcessorNumber (
  IN PEI_CPU_MP_DATA         *PeiCpuMpData,
  OUT UINTN                  *ProcessorNumber
  )
{
  UINTN                   TotalProcessorNumber;
  UINTN                   Index;

  TotalProcessorNumber = PeiCpuMpData->CpuCount;
  for (Index = 0; Index < TotalProcessorNumber; Index ++) {
    if (PeiCpuMpData->CpuData[Index].ApicId == GetInitialApicId ()) {
      *ProcessorNumber = Index;
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}

/**
  Worker function for SwitchBSP().

  Worker function for SwitchBSP(), assigned to the AP which is intended to become BSP.

  @param Buffer        Pointer to CPU MP Data
**/
VOID
EFIAPI
FutureBSPProc (
  IN  VOID                *Buffer
  )
{
  PEI_CPU_MP_DATA         *DataInHob;

  DataInHob = (PEI_CPU_MP_DATA *) Buffer;
  AsmExchangeRole (&DataInHob->APInfo, &DataInHob->BSPInfo);
}

/**
  This service retrieves the number of logical processor in the platform
  and the number of those logical processors that are enabled on this boot.
  This service may only be called from the BSP.

  This function is used to retrieve the following information:
    - The number of logical processors that are present in the system.
    - The number of enabled logical processors in the system at the instant
      this call is made.

  Because MP Service Ppi provides services to enable and disable processors
  dynamically, the number of enabled logical processors may vary during the
  course of a boot session.

  If this service is called from an AP, then EFI_DEVICE_ERROR is returned.
  If NumberOfProcessors or NumberOfEnabledProcessors is NULL, then
  EFI_INVALID_PARAMETER is returned. Otherwise, the total number of processors
  is returned in NumberOfProcessors, the number of currently enabled processor
  is returned in NumberOfEnabledProcessors, and EFI_SUCCESS is returned.

  @param[in]  PeiServices         An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in]  This                Pointer to this instance of the PPI.
  @param[out] NumberOfProcessors  Pointer to the total number of logical processors in
                                  the system, including the BSP and disabled APs.
  @param[out] NumberOfEnabledProcessors
                                  Number of processors in the system that are enabled.

  @retval EFI_SUCCESS             The number of logical processors and enabled
                                  logical processors was retrieved.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   NumberOfProcessors is NULL.
                                  NumberOfEnabledProcessors is NULL.
**/
EFI_STATUS
EFIAPI
PeiGetNumberOfProcessors (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI   *This,
  OUT UINTN                     *NumberOfProcessors,
  OUT UINTN                     *NumberOfEnabledProcessors
  )
{
  PEI_CPU_MP_DATA         *PeiCpuMpData;
  UINTN                   CallerNumber;
  UINTN                   ProcessorNumber;
  UINTN                   EnabledProcessorNumber;
  UINTN                   Index;

  PeiCpuMpData = GetMpHobData ();
  if (PeiCpuMpData == NULL) {
    return EFI_NOT_FOUND;
  }

  if ((NumberOfProcessors == NULL) || (NumberOfEnabledProcessors == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether caller processor is BSP
  //
  PeiWhoAmI (PeiServices, This, &CallerNumber);
  if (CallerNumber != PeiCpuMpData->BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  ProcessorNumber        = PeiCpuMpData->CpuCount;
  EnabledProcessorNumber = 0;
  for (Index = 0; Index < ProcessorNumber; Index++) {
    if (PeiCpuMpData->CpuData[Index].State != CpuStateDisabled) {
      EnabledProcessorNumber ++;
    }
  }

  *NumberOfProcessors = ProcessorNumber;
  *NumberOfEnabledProcessors = EnabledProcessorNumber;

  return EFI_SUCCESS;
}

/**
  Gets detailed MP-related information on the requested processor at the
  instant this call is made. This service may only be called from the BSP.

  This service retrieves detailed MP-related information about any processor
  on the platform. Note the following:
    - The processor information may change during the course of a boot session.
    - The information presented here is entirely MP related.

  Information regarding the number of caches and their sizes, frequency of operation,
  slot numbers is all considered platform-related information and is not provided
  by this service.

  @param[in]  PeiServices         An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in]  This                Pointer to this instance of the PPI.
  @param[in]  ProcessorNumber     Pointer to the total number of logical processors in
                                  the system, including the BSP and disabled APs.
  @param[out] ProcessorInfoBuffer Number of processors in the system that are enabled.

  @retval EFI_SUCCESS             Processor information was returned.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   ProcessorInfoBuffer is NULL.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist in the platform.
**/
EFI_STATUS
EFIAPI
PeiGetProcessorInfo (
  IN  CONST EFI_PEI_SERVICES     **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI    *This,
  IN  UINTN                      ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer
  )
{
  PEI_CPU_MP_DATA         *PeiCpuMpData;
  UINTN                   CallerNumber;

  PeiCpuMpData = GetMpHobData ();
  if (PeiCpuMpData == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether caller processor is BSP
  //
  PeiWhoAmI (PeiServices, This, &CallerNumber);
  if (CallerNumber != PeiCpuMpData->BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  if (ProcessorInfoBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ProcessorNumber >= PeiCpuMpData->CpuCount) {
    return EFI_NOT_FOUND;
  }

  ProcessorInfoBuffer->ProcessorId = (UINT64) PeiCpuMpData->CpuData[ProcessorNumber].ApicId;
  ProcessorInfoBuffer->StatusFlag  = 0;
  if (PeiCpuMpData->CpuData[ProcessorNumber].ApicId == GetInitialApicId()) {
    ProcessorInfoBuffer->StatusFlag |= PROCESSOR_AS_BSP_BIT;
  }
  if (PeiCpuMpData->CpuData[ProcessorNumber].CpuHealthy) {
    ProcessorInfoBuffer->StatusFlag |= PROCESSOR_HEALTH_STATUS_BIT;
  }
  if (PeiCpuMpData->CpuData[ProcessorNumber].State == CpuStateDisabled) {
    ProcessorInfoBuffer->StatusFlag &= ~PROCESSOR_ENABLED_BIT;
  } else {
    ProcessorInfoBuffer->StatusFlag |= PROCESSOR_ENABLED_BIT;
  }

  //
  // Get processor location information
  //
  ExtractProcessorLocation (PeiCpuMpData->CpuData[ProcessorNumber].ApicId, &ProcessorInfoBuffer->Location);

  return EFI_SUCCESS;
}

/**
  This service executes a caller provided function on all enabled APs. APs can
  run either simultaneously or one at a time in sequence. This service supports
  both blocking requests only. This service may only
  be called from the BSP.

  This function is used to dispatch all the enabled APs to the function specified
  by Procedure.  If any enabled AP is busy, then EFI_NOT_READY is returned
  immediately and Procedure is not started on any AP.

  If SingleThread is TRUE, all the enabled APs execute the function specified by
  Procedure one by one, in ascending order of processor handle number. Otherwise,
  all the enabled APs execute the function specified by Procedure simultaneously.

  If the timeout specified by TimeoutInMicroSeconds expires before all APs return
  from Procedure, then Procedure on the failed APs is terminated. All enabled APs
  are always available for further calls to EFI_PEI_MP_SERVICES_PPI.StartupAllAPs()
  and EFI_PEI_MP_SERVICES_PPI.StartupThisAP(). If FailedCpuList is not NULL, its
  content points to the list of processor handle numbers in which Procedure was
  terminated.

  Note: It is the responsibility of the consumer of the EFI_PEI_MP_SERVICES_PPI.StartupAllAPs()
  to make sure that the nature of the code that is executed on the BSP and the
  dispatched APs is well controlled. The MP Services Ppi does not guarantee
  that the Procedure function is MP-safe. Hence, the tasks that can be run in
  parallel are limited to certain independent tasks and well-controlled exclusive
  code. PEI services and Ppis may not be called by APs unless otherwise
  specified.

  In blocking execution mode, BSP waits until all APs finish or
  TimeoutInMicroSeconds expires.

  @param[in] PeiServices          An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[in] Procedure            A pointer to the function to be run on enabled APs of
                                  the system.
  @param[in] SingleThread         If TRUE, then all the enabled APs execute the function
                                  specified by Procedure one by one, in ascending order
                                  of processor handle number. If FALSE, then all the
                                  enabled APs execute the function specified by Procedure
                                  simultaneously.
  @param[in] TimeoutInMicroSeconds
                                  Indicates the time limit in microseconds for APs to
                                  return from Procedure, for blocking mode only. Zero
                                  means infinity. If the timeout expires before all APs
                                  return from Procedure, then Procedure on the failed APs
                                  is terminated. All enabled APs are available for next
                                  function assigned by EFI_PEI_MP_SERVICES_PPI.StartupAllAPs()
                                  or EFI_PEI_MP_SERVICES_PPI.StartupThisAP(). If the
                                  timeout expires in blocking mode, BSP returns
                                  EFI_TIMEOUT.
  @param[in] ProcedureArgument    The parameter passed into Procedure for all APs.

  @retval EFI_SUCCESS             In blocking mode, all APs have finished before the
                                  timeout expired.
  @retval EFI_DEVICE_ERROR        Caller processor is AP.
  @retval EFI_NOT_STARTED         No enabled APs exist in the system.
  @retval EFI_NOT_READY           Any enabled APs are busy.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before all
                                  enabled APs have finished.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.
**/
EFI_STATUS
EFIAPI
PeiStartupAllAPs (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI   *This,
  IN  EFI_AP_PROCEDURE          Procedure,
  IN  BOOLEAN                   SingleThread,
  IN  UINTN                     TimeoutInMicroSeconds,
  IN  VOID                      *ProcedureArgument      OPTIONAL
  )
{
  PEI_CPU_MP_DATA         *PeiCpuMpData;
  UINTN                   ProcessorNumber;
  UINTN                   Index;
  UINTN                   CallerNumber;
  BOOLEAN                 HasEnabledAp;
  BOOLEAN                 HasEnabledIdleAp;
  volatile UINT32         *FinishedCount;
  EFI_STATUS              Status;
  UINTN                   WaitCountIndex;
  UINTN                   WaitCountNumber;

  PeiCpuMpData = GetMpHobData ();
  if (PeiCpuMpData == NULL) {
    return EFI_NOT_FOUND;
  }

  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether caller processor is BSP
  //
  PeiWhoAmI (PeiServices, This, &CallerNumber);
  if (CallerNumber != PeiCpuMpData->BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  ProcessorNumber = PeiCpuMpData->CpuCount;

  HasEnabledAp     = FALSE;
  HasEnabledIdleAp = FALSE;
  for (Index = 0; Index < ProcessorNumber; Index ++) {
    if (Index == CallerNumber) {
      //
      // Skip BSP
      //
      continue;
    }
    if (PeiCpuMpData->CpuData[Index].State != CpuStateDisabled) {
      HasEnabledAp = TRUE;
      if (PeiCpuMpData->CpuData[Index].State != CpuStateBusy) {
        HasEnabledIdleAp = TRUE;
      }
    }
  }
  if (!HasEnabledAp) {
    //
    // If no enabled AP exists, return EFI_NOT_STARTED.
    //
    return EFI_NOT_STARTED;
  }
  if (!HasEnabledIdleAp) {
    //
    // If any enabled APs are busy, return EFI_NOT_READY.
    //
    return EFI_NOT_READY;
  }

  if (PeiCpuMpData->EndOfPeiFlag) {
    //
    // Backup original data and copy AP reset vector in it
    //
    BackupAndPrepareWakeupBuffer(PeiCpuMpData);
  }

  WaitCountNumber = TimeoutInMicroSeconds / CPU_CHECK_AP_INTERVAL + 1;
  WaitCountIndex = 0;
  FinishedCount = &PeiCpuMpData->FinishedCount;
  if (!SingleThread) {
    WakeUpAP (PeiCpuMpData, TRUE, 0, Procedure, ProcedureArgument);
    //
    // Wait to finish
    //
    if (TimeoutInMicroSeconds == 0) {
      while (*FinishedCount < ProcessorNumber - 1) {
        CpuPause ();
      }
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_TIMEOUT;
      for (WaitCountIndex = 0; WaitCountIndex < WaitCountNumber; WaitCountIndex++) {
        MicroSecondDelay (CPU_CHECK_AP_INTERVAL);
        if (*FinishedCount >= ProcessorNumber - 1) {
          Status = EFI_SUCCESS;
          break;
        }
      }
    }
  } else {
    Status = EFI_SUCCESS;
    for (Index = 0; Index < ProcessorNumber; Index++) {
      if (Index == CallerNumber) {
        continue;
      }
      WakeUpAP (PeiCpuMpData, FALSE, PeiCpuMpData->CpuData[Index].ApicId, Procedure, ProcedureArgument);
      //
      // Wait to finish
      //
      if (TimeoutInMicroSeconds == 0) {
        while (*FinishedCount < 1) {
          CpuPause ();
        }
      } else {
        for (WaitCountIndex = 0; WaitCountIndex < WaitCountNumber; WaitCountIndex++) {
          MicroSecondDelay (CPU_CHECK_AP_INTERVAL);
          if (*FinishedCount >= 1) {
            break;
          }
        }
        if (WaitCountIndex == WaitCountNumber) {
          Status = EFI_TIMEOUT;
        }
      }
    }
  }

  if (PeiCpuMpData->EndOfPeiFlag) {
    //
    // Restore original data
    //
    RestoreWakeupBuffer(PeiCpuMpData);
  }

  return Status;
}

/**
  This service lets the caller get one enabled AP to execute a caller-provided
  function. The caller can request the BSP to wait for the completion
  of the AP. This service may only be called from the BSP.

  This function is used to dispatch one enabled AP to the function specified by
  Procedure passing in the argument specified by ProcedureArgument.
  The execution is in blocking mode. The BSP waits until the AP finishes or
  TimeoutInMicroSecondss expires.

  If the timeout specified by TimeoutInMicroseconds expires before the AP returns
  from Procedure, then execution of Procedure by the AP is terminated. The AP is
  available for subsequent calls to EFI_PEI_MP_SERVICES_PPI.StartupAllAPs() and
  EFI_PEI_MP_SERVICES_PPI.StartupThisAP().

  @param[in] PeiServices          An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[in] Procedure            A pointer to the function to be run on enabled APs of
                                  the system.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().
  @param[in] TimeoutInMicroseconds
                                  Indicates the time limit in microseconds for APs to
                                  return from Procedure, for blocking mode only. Zero
                                  means infinity. If the timeout expires before all APs
                                  return from Procedure, then Procedure on the failed APs
                                  is terminated. All enabled APs are available for next
                                  function assigned by EFI_PEI_MP_SERVICES_PPI.StartupAllAPs()
                                  or EFI_PEI_MP_SERVICES_PPI.StartupThisAP(). If the
                                  timeout expires in blocking mode, BSP returns
                                  EFI_TIMEOUT.
  @param[in] ProcedureArgument    The parameter passed into Procedure for all APs.

  @retval EFI_SUCCESS             In blocking mode, specified AP finished before the
                                  timeout expires.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before the
                                  specified AP has finished.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.
**/
EFI_STATUS
EFIAPI
PeiStartupThisAP (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI   *This,
  IN  EFI_AP_PROCEDURE          Procedure,
  IN  UINTN                     ProcessorNumber,
  IN  UINTN                     TimeoutInMicroseconds,
  IN  VOID                      *ProcedureArgument      OPTIONAL
  )
{
  PEI_CPU_MP_DATA         *PeiCpuMpData;
  UINTN                   CallerNumber;
  volatile UINT32         *FinishedCount;
  EFI_STATUS              Status;
  UINTN                   WaitCountIndex;
  UINTN                   WaitCountNumber;

  PeiCpuMpData = GetMpHobData ();
  if (PeiCpuMpData == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether caller processor is BSP
  //
  PeiWhoAmI (PeiServices, This, &CallerNumber);
  if (CallerNumber != PeiCpuMpData->BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  if (ProcessorNumber >= PeiCpuMpData->CpuCount) {
    return EFI_NOT_FOUND;
  }

  if (ProcessorNumber == PeiCpuMpData->BspNumber || Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether specified AP is disabled
  //
  if (PeiCpuMpData->CpuData[ProcessorNumber].State == CpuStateDisabled) {
    return EFI_INVALID_PARAMETER;
  }

  if (PeiCpuMpData->EndOfPeiFlag) {
    //
    // Backup original data and copy AP reset vector in it
    //
    BackupAndPrepareWakeupBuffer(PeiCpuMpData);
  }

  WaitCountNumber = TimeoutInMicroseconds / CPU_CHECK_AP_INTERVAL + 1;
  WaitCountIndex = 0;
  FinishedCount = &PeiCpuMpData->FinishedCount;

  WakeUpAP (PeiCpuMpData, FALSE, PeiCpuMpData->CpuData[ProcessorNumber].ApicId, Procedure, ProcedureArgument);

  //
  // Wait to finish
  //
  if (TimeoutInMicroseconds == 0) {
    while (*FinishedCount < 1) {
      CpuPause() ;
    }
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_TIMEOUT;
    for (WaitCountIndex = 0; WaitCountIndex < WaitCountNumber; WaitCountIndex++) {
      MicroSecondDelay (CPU_CHECK_AP_INTERVAL);
      if (*FinishedCount >= 1) {
        Status = EFI_SUCCESS;
        break;
      }
    }
  }

  if (PeiCpuMpData->EndOfPeiFlag) {
    //
    // Backup original data and copy AP reset vector in it
    //
    RestoreWakeupBuffer(PeiCpuMpData);
  }

  return Status;
}

/**
  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes.   This call can only be performed
  by the current BSP.

  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes. The new BSP can take over the
  execution of the old BSP and continue seamlessly from where the old one left
  off.

  If the BSP cannot be switched prior to the return from this service, then
  EFI_UNSUPPORTED must be returned.

  @param[in] PeiServices          An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().
  @param[in] EnableOldBSP         If TRUE, then the old BSP will be listed as an enabled
                                  AP. Otherwise, it will be disabled.

  @retval EFI_SUCCESS             BSP successfully switched.
  @retval EFI_UNSUPPORTED         Switching the BSP cannot be completed prior to this
                                  service returning.
  @retval EFI_UNSUPPORTED         Switching the BSP is not supported.
  @retval EFI_SUCCESS             The calling processor is an AP.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the current BSP or a disabled
                                  AP.
  @retval EFI_NOT_READY           The specified AP is busy.
**/
EFI_STATUS
EFIAPI
PeiSwitchBSP (
  IN  CONST EFI_PEI_SERVICES   **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI  *This,
  IN  UINTN                    ProcessorNumber,
  IN  BOOLEAN                  EnableOldBSP
  )
{
  PEI_CPU_MP_DATA         *PeiCpuMpData;
  UINTN                   CallerNumber;
  MSR_IA32_APIC_BASE      ApicBaseMsr;

  PeiCpuMpData = GetMpHobData ();
  if (PeiCpuMpData == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether caller processor is BSP
  //
  PeiWhoAmI (PeiServices, This, &CallerNumber);
  if (CallerNumber != PeiCpuMpData->BspNumber) {
    return EFI_SUCCESS;
  }

  if (ProcessorNumber >= PeiCpuMpData->CpuCount) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether specified AP is disabled
  //
  if (PeiCpuMpData->CpuData[ProcessorNumber].State == CpuStateDisabled) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether ProcessorNumber specifies the current BSP
  //
  if (ProcessorNumber == PeiCpuMpData->BspNumber) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether specified AP is busy
  //
  if (PeiCpuMpData->CpuData[ProcessorNumber].State == CpuStateBusy) {
    return EFI_NOT_READY;
  }

  //
  // Clear the BSP bit of MSR_IA32_APIC_BASE
  //
  ApicBaseMsr.Uint64 = AsmReadMsr64 (MSR_IA32_APIC_BASE_ADDRESS);
  ApicBaseMsr.Bits.Bsp = 0;
  AsmWriteMsr64 (MSR_IA32_APIC_BASE_ADDRESS, ApicBaseMsr.Uint64);

  PeiCpuMpData->BSPInfo.State = CPU_SWITCH_STATE_IDLE;
  PeiCpuMpData->APInfo.State  = CPU_SWITCH_STATE_IDLE;

  if (PeiCpuMpData->EndOfPeiFlag) {
    //
    // Backup original data and copy AP reset vector in it
    //
    BackupAndPrepareWakeupBuffer(PeiCpuMpData);
  }

  //
  // Need to wakeUp AP (future BSP).
  //
  WakeUpAP (PeiCpuMpData, FALSE, PeiCpuMpData->CpuData[ProcessorNumber].ApicId, FutureBSPProc, PeiCpuMpData);

  AsmExchangeRole (&PeiCpuMpData->BSPInfo, &PeiCpuMpData->APInfo);

  if (PeiCpuMpData->EndOfPeiFlag) {
    //
    // Backup original data and copy AP reset vector in it
    //
    RestoreWakeupBuffer(PeiCpuMpData);
  }

  //
  // Set the BSP bit of MSR_IA32_APIC_BASE on new BSP
  //
  ApicBaseMsr.Uint64 = AsmReadMsr64 (MSR_IA32_APIC_BASE_ADDRESS);
  ApicBaseMsr.Bits.Bsp = 1;
  AsmWriteMsr64 (MSR_IA32_APIC_BASE_ADDRESS, ApicBaseMsr.Uint64);
  //
  // Set old BSP enable state
  //
  if (!EnableOldBSP) {
    PeiCpuMpData->CpuData[PeiCpuMpData->BspNumber].State = CpuStateDisabled;
  }
  //
  // Save new BSP number
  //
  PeiCpuMpData->BspNumber = (UINT32) ProcessorNumber;

  return EFI_SUCCESS;
}

/**
  This service lets the caller enable or disable an AP from this point onward.
  This service may only be called from the BSP.

  This service allows the caller enable or disable an AP from this point onward.
  The caller can optionally specify the health status of the AP by Health. If
  an AP is being disabled, then the state of the disabled AP is implementation
  dependent. If an AP is enabled, then the implementation must guarantee that a
  complete initialization sequence is performed on the AP, so the AP is in a state
  that is compatible with an MP operating system.

  If the enable or disable AP operation cannot be completed prior to the return
  from this service, then EFI_UNSUPPORTED must be returned.

  @param[in] PeiServices          An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().
  @param[in] EnableAP             Specifies the new state for the processor for enabled,
                                  FALSE for disabled.
  @param[in] HealthFlag           If not NULL, a pointer to a value that specifies the
                                  new health status of the AP. This flag corresponds to
                                  StatusFlag defined in EFI_PEI_MP_SERVICES_PPI.GetProcessorInfo().
                                  Only the PROCESSOR_HEALTH_STATUS_BIT is used. All other
                                  bits are ignored. If it is NULL, this parameter is
                                  ignored.

  @retval EFI_SUCCESS             The specified AP was enabled or disabled successfully.
  @retval EFI_UNSUPPORTED         Enabling or disabling an AP cannot be completed prior
                                  to this service returning.
  @retval EFI_UNSUPPORTED         Enabling or disabling an AP is not supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_NOT_FOUND           Processor with the handle specified by ProcessorNumber
                                  does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP.
**/
EFI_STATUS
EFIAPI
PeiEnableDisableAP (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI   *This,
  IN  UINTN                     ProcessorNumber,
  IN  BOOLEAN                   EnableAP,
  IN  UINT32                    *HealthFlag OPTIONAL
  )
{
  PEI_CPU_MP_DATA         *PeiCpuMpData;
  UINTN                   CallerNumber;

  PeiCpuMpData = GetMpHobData ();
  if (PeiCpuMpData == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether caller processor is BSP
  //
  PeiWhoAmI (PeiServices, This, &CallerNumber);
  if (CallerNumber != PeiCpuMpData->BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  if (ProcessorNumber == PeiCpuMpData->BspNumber) {
    return EFI_INVALID_PARAMETER;
  }

  if (ProcessorNumber >= PeiCpuMpData->CpuCount) {
    return EFI_NOT_FOUND;
  }

  if (!EnableAP) {
    PeiCpuMpData->CpuData[ProcessorNumber].State = CpuStateDisabled;
  } else {
    PeiCpuMpData->CpuData[ProcessorNumber].State = CpuStateIdle;
  }

  if (HealthFlag != NULL) {
    PeiCpuMpData->CpuData[ProcessorNumber].CpuHealthy =
          (BOOLEAN) ((*HealthFlag & PROCESSOR_HEALTH_STATUS_BIT) != 0);
  }
  return EFI_SUCCESS;
}

/**
  This return the handle number for the calling processor.  This service may be
  called from the BSP and APs.

  This service returns the processor handle number for the calling processor.
  The returned value is in the range from 0 to the total number of logical
  processors minus 1. The total number of logical processors can be retrieved
  with EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors(). This service may be
  called from the BSP and APs. If ProcessorNumber is NULL, then EFI_INVALID_PARAMETER
  is returned. Otherwise, the current processors handle number is returned in
  ProcessorNumber, and EFI_SUCCESS is returned.

  @param[in]  PeiServices         An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in]  This                A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[out] ProcessorNumber     The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().

  @retval EFI_SUCCESS             The current processor handle number was returned in
                                  ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.
**/
EFI_STATUS
EFIAPI
PeiWhoAmI (
  IN  CONST EFI_PEI_SERVICES   **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI  *This,
  OUT UINTN                    *ProcessorNumber
  )
{
  PEI_CPU_MP_DATA         *PeiCpuMpData;

  PeiCpuMpData = GetMpHobData ();
  if (PeiCpuMpData == NULL) {
    return EFI_NOT_FOUND;
  }

  if (ProcessorNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return GetProcessorNumber (PeiCpuMpData, ProcessorNumber);
}


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
  if (PeiCpuMpData->CpuData[ProcessorNumber].Health.Uint32 == 0) {
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

  return Status;
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


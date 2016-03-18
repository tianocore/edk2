/** @file
Implementation of SMM CPU Services Protocol.

Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PiSmmCpuDxeSmm.h"

//
// SMM CPU Service Protocol instance
//
EFI_SMM_CPU_SERVICE_PROTOCOL  mSmmCpuService = {
  SmmGetProcessorInfo,
  SmmSwitchBsp,
  SmmAddProcessor,
  SmmRemoveProcessor,
  SmmWhoAmI,
  SmmRegisterExceptionHandler
};

/**
  Get Package ID/Core ID/Thread ID of a processor.

  APIC ID must be an initial APIC ID.

  The algorithm below assumes the target system has symmetry across physical package boundaries
  with respect to the number of logical processors per package, number of cores per package.

  @param  ApicId    APIC ID of the target logical processor.
  @param  Location    Returns the processor location information.
**/
VOID
SmmGetProcessorLocation (
  IN UINT32 ApicId,
  OUT EFI_CPU_PHYSICAL_LOCATION *Location
  )
{
  UINTN   ThreadBits;
  UINTN   CoreBits;
  UINT32  RegEax;
  UINT32  RegEbx;
  UINT32  RegEcx;
  UINT32  RegEdx;
  UINT32  MaxCpuIdIndex;
  UINT32  SubIndex;
  UINTN   LevelType;
  UINT32  MaxLogicProcessorsPerPackage;
  UINT32  MaxCoresPerPackage;
  BOOLEAN TopologyLeafSupported;

  ASSERT (Location != NULL);

  ThreadBits            = 0;
  CoreBits              = 0;
  TopologyLeafSupported = FALSE;

  //
  // Check if the processor is capable of supporting more than one logical processor.
  //
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &RegEdx);
  ASSERT ((RegEdx & BIT28) != 0);

  //
  // Assume three-level mapping of APIC ID: Package:Core:SMT.
  //

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
    if ((RegEbx & 0xffff) != 0) {
      TopologyLeafSupported = TRUE;

      //
      // Sub-leaf index 0 (ECX= 0 as input) provides enumeration parameters to extract
      // the SMT sub-field of x2APIC ID.
      //
      LevelType = (RegEcx >> 8) & 0xff;
      ASSERT (LevelType == CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_SMT);
      if ((RegEbx & 0xffff) > 1 ) {
        ThreadBits = RegEax & 0x1f;
      } else {
        //
        // HT is not supported
        //
        ThreadBits = 0;
      }

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

  Location->Thread = ApicId & ~((-1) << ThreadBits);
  Location->Core = (ApicId >> ThreadBits) & ~((-1) << CoreBits);
  Location->Package = (ApicId >> (ThreadBits+ CoreBits));
}

/**
  Gets processor information on the requested processor at the instant this call is made.

  @param[in]  This                 A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param[in]  ProcessorNumber      The handle number of processor.
  @param[out] ProcessorInfoBuffer  A pointer to the buffer where information for
                                   the requested processor is deposited.

  @retval EFI_SUCCESS             Processor information was returned.
  @retval EFI_INVALID_PARAMETER   ProcessorInfoBuffer is NULL.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is invalid.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist in the platform.

**/
EFI_STATUS
EFIAPI
SmmGetProcessorInfo (
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL *This,
  IN       UINTN                        ProcessorNumber,
  OUT      EFI_PROCESSOR_INFORMATION    *ProcessorInfoBuffer
  )
{
  //
  // Check parameter
  //
  if (ProcessorNumber >= mMaxNumberOfCpus || ProcessorInfoBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (gSmmCpuPrivate->ProcessorInfo[ProcessorNumber].ProcessorId == INVALID_APIC_ID) {
    return EFI_NOT_FOUND;
  }

  //
  // Fill in processor information
  //
  CopyMem (ProcessorInfoBuffer, &gSmmCpuPrivate->ProcessorInfo[ProcessorNumber], sizeof (EFI_PROCESSOR_INFORMATION));
  return EFI_SUCCESS;
}

/**
  This service switches the requested AP to be the BSP since the next SMI.

  @param[in] This             A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param[in] ProcessorNumber  The handle number of AP that is to become the new BSP.

  @retval EFI_SUCCESS             BSP will be switched in next SMI.
  @retval EFI_UNSUPPORTED         Switching the BSP or a processor to be hot-removed is not supported.
  @retval EFI_NOT_FOUND           The processor with the handle specified by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is invalid.
**/
EFI_STATUS
EFIAPI
SmmSwitchBsp (
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL *This,
  IN       UINTN                        ProcessorNumber
  )
{
  //
  // Check parameter
  //
  if (ProcessorNumber >= mMaxNumberOfCpus) {
    return EFI_INVALID_PARAMETER;
  }

  if (gSmmCpuPrivate->ProcessorInfo[ProcessorNumber].ProcessorId == INVALID_APIC_ID) {
    return EFI_NOT_FOUND;
  }

  if (gSmmCpuPrivate->Operation[ProcessorNumber] != SmmCpuNone ||
      gSmst->CurrentlyExecutingCpu == ProcessorNumber) {
    return EFI_UNSUPPORTED;
  }

  //
  // Setting of the BSP for next SMI is pending until all SMI handlers are finished
  //
  gSmmCpuPrivate->Operation[ProcessorNumber] = SmmCpuSwitchBsp;
  return EFI_SUCCESS;
}

/**
  Notify that a processor was hot-added.

  @param[in] This                A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param[in] ProcessorId         Local APIC ID of the hot-added processor.
  @param[out] ProcessorNumber    The handle number of the hot-added processor.

  @retval EFI_SUCCESS            The hot-addition of the specified processors was successfully notified.
  @retval EFI_UNSUPPORTED        Hot addition of processor is not supported.
  @retval EFI_NOT_FOUND          The processor with the handle specified by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER  ProcessorNumber is invalid.
  @retval EFI_ALREADY_STARTED    The processor is already online in the system.
**/
EFI_STATUS
EFIAPI
SmmAddProcessor (
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL  *This,
  IN       UINT64                        ProcessorId,
  OUT      UINTN                         *ProcessorNumber
  )
{
  UINTN  Index;

  if (!FeaturePcdGet (PcdCpuHotPlugSupport)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check parameter
  //
  if (ProcessorNumber == NULL || ProcessorId == INVALID_APIC_ID) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check if the processor already exists
  //

  for (Index = 0; Index < mMaxNumberOfCpus; Index++) {
    if (gSmmCpuPrivate->ProcessorInfo[Index].ProcessorId == ProcessorId) {
      return EFI_ALREADY_STARTED;
    }
  }

  //
  // Check CPU hot plug data. The CPU RAS handler should have created the mapping
  // of the APIC ID to SMBASE.
  //
  for (Index = 0; Index < mMaxNumberOfCpus; Index++) {
    if (mCpuHotPlugData.ApicId[Index] == ProcessorId &&
        gSmmCpuPrivate->ProcessorInfo[Index].ProcessorId == INVALID_APIC_ID) {
      gSmmCpuPrivate->ProcessorInfo[Index].ProcessorId = ProcessorId;
      gSmmCpuPrivate->ProcessorInfo[Index].StatusFlag = 0;
      SmmGetProcessorLocation ((UINT32)ProcessorId, &gSmmCpuPrivate->ProcessorInfo[Index].Location);

      *ProcessorNumber = Index;
      gSmmCpuPrivate->Operation[Index] = SmmCpuAdd;
      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Notify that a processor was hot-removed.

  @param[in] This                A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param[in] ProcessorNumber     The handle number of the hot-added processor.

  @retval EFI_SUCCESS            The hot-removal of the specified processors was successfully notified.
  @retval EFI_UNSUPPORTED        Hot removal of processor is not supported.
  @retval EFI_UNSUPPORTED        Hot removal of BSP is not supported.
  @retval EFI_UNSUPPORTED        Hot removal of a processor with pending hot-plug operation is not supported.
  @retval EFI_INVALID_PARAMETER  ProcessorNumber is invalid.
**/
EFI_STATUS
EFIAPI
SmmRemoveProcessor (
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL  *This,
  IN       UINTN                         ProcessorNumber
  )
{
  if (!FeaturePcdGet (PcdCpuHotPlugSupport)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check parameter
  //
  if (ProcessorNumber >= mMaxNumberOfCpus ||
      gSmmCpuPrivate->ProcessorInfo[ProcessorNumber].ProcessorId == INVALID_APIC_ID) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Can't remove BSP
  //
  if (ProcessorNumber == gSmmCpuPrivate->SmmCoreEntryContext.CurrentlyExecutingCpu) {
    return EFI_UNSUPPORTED;
  }

  if (gSmmCpuPrivate->Operation[ProcessorNumber] != SmmCpuNone) {
    return EFI_UNSUPPORTED;
  }

  gSmmCpuPrivate->ProcessorInfo[ProcessorNumber].ProcessorId = INVALID_APIC_ID;
  mCpuHotPlugData.ApicId[ProcessorNumber] = INVALID_APIC_ID;

  //
  // Removal of the processor from the CPU list is pending until all SMI handlers are finished
  //
  gSmmCpuPrivate->Operation[ProcessorNumber] = SmmCpuRemove;
  return EFI_SUCCESS;
}

/**
  This return the handle number for the calling processor.

  @param[in] This                 A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param[out] ProcessorNumber      The handle number of currently executing processor.

  @retval EFI_SUCCESS             The current processor handle number was returned
                                  in ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.

**/
EFI_STATUS
EFIAPI
SmmWhoAmI (
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL *This,
  OUT      UINTN                        *ProcessorNumber
  )
{
  UINTN  Index;
  UINT64 ApicId;

  //
  // Check parameter
  //
  if (ProcessorNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ApicId = GetApicId ();

  for (Index = 0; Index < mMaxNumberOfCpus; Index++) {
    if (gSmmCpuPrivate->ProcessorInfo[Index].ProcessorId == ApicId) {
      *ProcessorNumber = Index;
      return EFI_SUCCESS;
    }
  }
  //
  // This should not happen
  //
  ASSERT (FALSE);
  return EFI_NOT_FOUND;
}

/**
  Update the SMM CPU list per the pending operation.

  This function is called after return from SMI handlers.
**/
VOID
SmmCpuUpdate (
  VOID
  )
{
  UINTN   Index;

  //
  // Handle pending BSP switch operations
  //
  for (Index = 0; Index < mMaxNumberOfCpus; Index++) {
    if (gSmmCpuPrivate->Operation[Index] == SmmCpuSwitchBsp) {
      gSmmCpuPrivate->Operation[Index] = SmmCpuNone;
      mSmmMpSyncData->SwitchBsp = TRUE;
      mSmmMpSyncData->CandidateBsp[Index] = TRUE;
    }
  }

  //
  // Handle pending hot-add operations
  //
  for (Index = 0; Index < mMaxNumberOfCpus; Index++) {
    if (gSmmCpuPrivate->Operation[Index] == SmmCpuAdd) {
      gSmmCpuPrivate->Operation[Index] = SmmCpuNone;
      mNumberOfCpus++;
    }
  }

  //
  // Handle pending hot-remove operations
  //
  for (Index = 0; Index < mMaxNumberOfCpus; Index++) {
    if (gSmmCpuPrivate->Operation[Index] == SmmCpuRemove) {
      gSmmCpuPrivate->Operation[Index] = SmmCpuNone;
      mNumberOfCpus--;
    }
  }
}

/**
  Register exception handler.

  @param  This                  A pointer to the SMM_CPU_SERVICE_PROTOCOL instance.
  @param  ExceptionType         Defines which interrupt or exception to hook. Type EFI_EXCEPTION_TYPE and
                                the valid values for this parameter are defined in EFI_DEBUG_SUPPORT_PROTOCOL
                                of the UEFI 2.0 specification.
  @param  InterruptHandler      A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER
                                that is called when a processor interrupt occurs.
                                If this parameter is NULL, then the handler will be uninstalled.

  @retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for InterruptType was previously installed.
  @retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for InterruptType was not previously installed.
  @retval EFI_UNSUPPORTED       The interrupt specified by InterruptType is not supported.

**/
EFI_STATUS
EFIAPI
SmmRegisterExceptionHandler (
    IN EFI_SMM_CPU_SERVICE_PROTOCOL  *This,
    IN EFI_EXCEPTION_TYPE            ExceptionType,
    IN EFI_CPU_INTERRUPT_HANDLER     InterruptHandler
    )
{
  return RegisterCpuInterruptHandler (ExceptionType, InterruptHandler);
}

/**
  Initialize SMM CPU Services.

  It installs EFI SMM CPU Services Protocol.

  @param ImageHandle The firmware allocated handle for the EFI image.

  @retval EFI_SUCCESS    EFI SMM CPU Services Protocol was installed successfully.
**/
EFI_STATUS
InitializeSmmCpuServices (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS Status;

  Status = gSmst->SmmInstallProtocolInterface (
                    &Handle,
                    &gEfiSmmCpuServiceProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mSmmCpuService
                    );
  ASSERT_EFI_ERROR (Status);
  return Status;
}


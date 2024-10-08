/** @file
  MP initialize support functions for PEI phase.

  Copyright (c) 2016 - 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"
#include <Library/PeiServicesLib.h>
#include <Guid/S3SmmInitDone.h>
#include <Guid/EndOfS3Resume.h>
#include <Ppi/ShadowMicrocode.h>

STATIC UINT64  mSevEsPeiWakeupBuffer = BASE_1MB;

/**
  Enable Debug Agent to support source debugging on AP function.

**/
VOID
EnableDebugAgent (
  VOID
  )
{
}

/**
  Get pointer to CPU MP Data structure.
  For BSP, the pointer is retrieved from HOB.
  For AP, the structure is stored in the top of each AP's stack.

  @return  The pointer to CPU MP Data structure.
**/
CPU_MP_DATA *
GetCpuMpData (
  VOID
  )
{
  CPU_MP_DATA                  *CpuMpData;
  MSR_IA32_APIC_BASE_REGISTER  ApicBaseMsr;
  UINTN                        ApTopOfStack;
  AP_STACK_DATA                *ApStackData;

  ApicBaseMsr.Uint64 = AsmReadMsr64 (MSR_IA32_APIC_BASE);
  if (ApicBaseMsr.Bits.BSP == 1) {
    CpuMpData = GetCpuMpDataFromGuidedHob ();
    ASSERT (CpuMpData != NULL);
  } else {
    ApTopOfStack = ALIGN_VALUE ((UINTN)&ApTopOfStack, (UINTN)PcdGet32 (PcdCpuApStackSize));
    ApStackData  = (AP_STACK_DATA *)((UINTN)ApTopOfStack- sizeof (AP_STACK_DATA));
    CpuMpData    = (CPU_MP_DATA *)ApStackData->MpData;
  }

  return CpuMpData;
}

/**
  Save the pointer to CPU MP Data structure.

  @param[in] CpuMpData  The pointer to CPU MP Data structure will be saved.
**/
VOID
SaveCpuMpData (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  UINT32              MaxCpusPerHob;
  UINT32              CpusInHob;
  UINT64              Data64;
  UINT32              Index;
  UINT32              HobBase;
  CPU_INFO_IN_HOB     *CpuInfoInHob;
  MP_HAND_OFF         *MpHandOff;
  MP_HAND_OFF_CONFIG  MpHandOffConfig;
  UINTN               MpHandOffSize;

  MaxCpusPerHob = (0xFFF8 - sizeof (EFI_HOB_GUID_TYPE) - sizeof (MP_HAND_OFF)) / sizeof (PROCESSOR_HAND_OFF);

  //
  // When APs are in a state that can be waken up by a store operation to a memory address,
  // report the MP_HAND_OFF data for DXE to use.
  //
  CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuMpData->CpuInfoInHob;

  for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
    if (Index % MaxCpusPerHob == 0) {
      HobBase   = Index;
      CpusInHob = MIN (CpuMpData->CpuCount - HobBase, MaxCpusPerHob);

      MpHandOffSize = sizeof (MP_HAND_OFF) + sizeof (PROCESSOR_HAND_OFF) * CpusInHob;
      MpHandOff     = (MP_HAND_OFF *)BuildGuidHob (&mMpHandOffGuid, MpHandOffSize);
      ASSERT (MpHandOff != NULL);
      ZeroMem (MpHandOff, MpHandOffSize);

      MpHandOff->ProcessorIndex = HobBase;
      MpHandOff->CpuCount       = CpusInHob;
    }

    MpHandOff->Info[Index-HobBase].ApicId = CpuInfoInHob[Index].ApicId;
    MpHandOff->Info[Index-HobBase].Health = CpuInfoInHob[Index].Health;
    if (CpuMpData->ApLoopMode != ApInHltLoop) {
      MpHandOff->Info[Index-HobBase].StartupSignalAddress    = (UINT64)(UINTN)CpuMpData->CpuData[Index].StartupApSignal;
      MpHandOff->Info[Index-HobBase].StartupProcedureAddress = (UINT64)(UINTN)&CpuMpData->CpuData[Index].ApFunction;
    }
  }

  ZeroMem (&MpHandOffConfig, sizeof (MpHandOffConfig));
  if (CpuMpData->ApLoopMode != ApInHltLoop) {
    MpHandOffConfig.StartupSignalValue    = MP_HAND_OFF_SIGNAL;
    MpHandOffConfig.WaitLoopExecutionMode = sizeof (VOID *);
  }

  BuildGuidDataHob (
    &mMpHandOffConfigGuid,
    (VOID *)&MpHandOffConfig,
    sizeof (MpHandOffConfig)
    );

  //
  // Build location of CPU MP DATA buffer in HOB
  //
  Data64 = (UINT64)(UINTN)CpuMpData;
  BuildGuidDataHob (
    &mCpuInitMpLibHobGuid,
    (VOID *)&Data64,
    sizeof (UINT64)
    );
}

/**
  Check if AP wakeup buffer is overlapped with existing allocated buffer.

  @param[in]  WakeupBufferStart     AP wakeup buffer start address.
  @param[in]  WakeupBufferEnd       AP wakeup buffer end address.

  @retval  TRUE       There is overlap.
  @retval  FALSE      There is no overlap.
**/
BOOLEAN
CheckOverlapWithAllocatedBuffer (
  IN UINT64  WakeupBufferStart,
  IN UINT64  WakeupBufferEnd
  )
{
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryHob;
  BOOLEAN                    Overlapped;
  UINT64                     MemoryStart;
  UINT64                     MemoryEnd;

  Overlapped = FALSE;
  //
  // Get the HOB list for processing
  //
  Hob.Raw = GetHobList ();
  //
  // Collect memory ranges
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      MemoryHob   = Hob.MemoryAllocation;
      MemoryStart = MemoryHob->AllocDescriptor.MemoryBaseAddress;
      MemoryEnd   = MemoryHob->AllocDescriptor.MemoryBaseAddress + MemoryHob->AllocDescriptor.MemoryLength;
      if (!((WakeupBufferStart >= MemoryEnd) || (WakeupBufferEnd <= MemoryStart))) {
        Overlapped = TRUE;
        break;
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return Overlapped;
}

/**
  Get available system memory below 1MB by specified size.

  @param[in] WakeupBufferSize   Wakeup buffer size required

  @retval other   Return wakeup buffer address below 1MB.
  @retval -1      Cannot find free memory below 1MB.
**/
UINTN
GetWakeupBuffer (
  IN UINTN  WakeupBufferSize
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINT64                WakeupBufferStart;
  UINT64                WakeupBufferEnd;

  WakeupBufferSize = (WakeupBufferSize + SIZE_4KB - 1) & ~(SIZE_4KB - 1);

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
          )
      {
        //
        // Need memory under 1MB to be collected here
        //
        WakeupBufferEnd = Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength;
        if (ConfidentialComputingGuestHas (CCAttrAmdSevEs) &&
            (WakeupBufferEnd > mSevEsPeiWakeupBuffer))
        {
          //
          // SEV-ES Wakeup buffer should be under 1MB and under any previous one
          //
          WakeupBufferEnd = mSevEsPeiWakeupBuffer;
        } else if (WakeupBufferEnd > BASE_1MB) {
          //
          // Wakeup buffer should be under 1MB
          //
          WakeupBufferEnd = BASE_1MB;
        }

        while (WakeupBufferEnd > WakeupBufferSize) {
          //
          // Wakeup buffer should be aligned on 4KB
          //
          WakeupBufferStart = (WakeupBufferEnd - WakeupBufferSize) & ~(SIZE_4KB - 1);
          if (WakeupBufferStart < Hob.ResourceDescriptor->PhysicalStart) {
            break;
          }

          if (CheckOverlapWithAllocatedBuffer (WakeupBufferStart, WakeupBufferEnd)) {
            //
            // If this range is overlapped with existing allocated buffer, skip it
            // and find the next range
            //
            WakeupBufferEnd -= WakeupBufferSize;
            continue;
          }

          DEBUG ((
            DEBUG_INFO,
            "WakeupBufferStart = %x, WakeupBufferSize = %x\n",
            WakeupBufferStart,
            WakeupBufferSize
            ));

          if (ConfidentialComputingGuestHas (CCAttrAmdSevEs)) {
            //
            // Next SEV-ES wakeup buffer allocation must be below this
            // allocation
            //
            mSevEsPeiWakeupBuffer = WakeupBufferStart;
          }

          return (UINTN)WakeupBufferStart;
        }
      }
    }

    //
    // Find the next HOB
    //
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return (UINTN)-1;
}

/**
  Get available EfiBootServicesCode memory below 4GB by specified size.

  This buffer is required to safely transfer AP from real address mode to
  protected mode or long mode, due to the fact that the buffer returned by
  GetWakeupBuffer() may be marked as non-executable.

  @param[in] BufferSize   Wakeup transition buffer size.

  @retval other   Return wakeup transition buffer address below 4GB.
  @retval 0       Cannot find free memory below 4GB.
**/
UINTN
AllocateCodeBuffer (
  IN UINTN  BufferSize
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;

  Status = PeiServicesAllocatePages (EfiBootServicesCode, EFI_SIZE_TO_PAGES (BufferSize), &Address);
  if (EFI_ERROR (Status)) {
    Address = 0;
  }

  return (UINTN)Address;
}

/**
  Return the address of the SEV-ES AP jump table.

  This buffer is required in order for an SEV-ES guest to transition from
  UEFI into an OS.

  @return         Return SEV-ES AP jump table buffer
**/
UINTN
GetSevEsAPMemory (
  VOID
  )
{
  //
  // PEI phase doesn't need to do such transition. So simply return 0.
  //
  return 0;
}

/**
  Checks APs status and updates APs status if needed.

**/
VOID
CheckAndUpdateApsStatus (
  VOID
  )
{
}

/**
  Build the microcode patch HOB that contains the base address and size of the
  microcode patch stored in the memory.

  @param[in]  CpuMpData    Pointer to the CPU_MP_DATA structure.

**/
VOID
BuildMicrocodeCacheHob (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  EDKII_MICROCODE_PATCH_HOB  *MicrocodeHob;
  UINTN                      HobDataLength;
  UINT32                     Index;

  HobDataLength = sizeof (EDKII_MICROCODE_PATCH_HOB) +
                  sizeof (UINT64) * CpuMpData->CpuCount;

  MicrocodeHob = AllocatePool (HobDataLength);
  if (MicrocodeHob == NULL) {
    ASSERT (FALSE);
    return;
  }

  //
  // Store the information of the memory region that holds the microcode patches.
  //
  MicrocodeHob->MicrocodePatchAddress    = CpuMpData->MicrocodePatchAddress;
  MicrocodeHob->MicrocodePatchRegionSize = CpuMpData->MicrocodePatchRegionSize;

  //
  // Store the detected microcode patch for each processor as well.
  //
  MicrocodeHob->ProcessorCount = CpuMpData->CpuCount;
  for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
    if (CpuMpData->CpuData[Index].MicrocodeEntryAddr != 0) {
      MicrocodeHob->ProcessorSpecificPatchOffset[Index] =
        CpuMpData->CpuData[Index].MicrocodeEntryAddr - CpuMpData->MicrocodePatchAddress;
    } else {
      MicrocodeHob->ProcessorSpecificPatchOffset[Index] = MAX_UINT64;
    }
  }

  BuildGuidDataHob (
    &gEdkiiMicrocodePatchHobGuid,
    MicrocodeHob,
    HobDataLength
    );

  return;
}

/**
  S3 SMM Init Done notification function.

  @param  PeiServices      Indirect reference to the PEI Services Table.
  @param  NotifyDesc       Address of the notification descriptor data structure.
  @param  InvokePpi        Address of the PPI that was invoked.

  @retval EFI_SUCCESS      The function completes successfully.

**/
EFI_STATUS
EFIAPI
NotifyOnEndOfS3Resume (
  IN  EFI_PEI_SERVICES           **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN  VOID                       *InvokePpi
  )
{
  CPU_MP_DATA  *CpuMpData;

  CpuMpData       = GetCpuMpData ();
  mNumberToFinish = CpuMpData->CpuCount - 1;
  WakeUpAP (CpuMpData, TRUE, 0, RelocateApLoop, NULL, TRUE);
  while (mNumberToFinish > 0) {
    CpuPause ();
  }

  DEBUG ((DEBUG_INFO, "%a() done!\n", __func__));

  return EFI_SUCCESS;
}

//
// Global function
//
EFI_PEI_NOTIFY_DESCRIPTOR  mEndOfS3ResumeNotifyDesc = {
  EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEdkiiEndOfS3ResumeGuid,
  NotifyOnEndOfS3Resume
};

/**
  Initialize global data for MP support.

  @param[in] CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
InitMpGlobalData (
  IN CPU_MP_DATA  *CpuMpData
  )
{
  EFI_STATUS  Status;

  BuildMicrocodeCacheHob (CpuMpData);
  SaveCpuMpData (CpuMpData);
  PrepareApLoopCode (CpuMpData);

  ///
  /// Install Notify
  ///

  Status = PeiServicesNotifyPpi (&mEndOfS3ResumeNotifyDesc);
  ASSERT_EFI_ERROR (Status);
}

/**
  This service executes a caller provided function on all enabled APs.

  @param[in]  Procedure               A pointer to the function to be run on
                                      enabled APs of the system. See type
                                      EFI_AP_PROCEDURE.
  @param[in]  SingleThread            If TRUE, then all the enabled APs execute
                                      the function specified by Procedure one by
                                      one, in ascending order of processor handle
                                      number.  If FALSE, then all the enabled APs
                                      execute the function specified by Procedure
                                      simultaneously.
  @param[in]  WaitEvent               The event created by the caller with CreateEvent()
                                      service.  If it is NULL, then execute in
                                      blocking mode. BSP waits until all APs finish
                                      or TimeoutInMicroSeconds expires.  If it's
                                      not NULL, then execute in non-blocking mode.
                                      BSP requests the function specified by
                                      Procedure to be started on all the enabled
                                      APs, and go on executing immediately. If
                                      all return from Procedure, or TimeoutInMicroSeconds
                                      expires, this event is signaled. The BSP
                                      can use the CheckEvent() or WaitForEvent()
                                      services to check the state of event.  Type
                                      EFI_EVENT is defined in CreateEvent() in
                                      the Unified Extensible Firmware Interface
                                      Specification.
  @param[in]  TimeoutInMicroseconds   Indicates the time limit in microseconds for
                                      APs to return from Procedure, either for
                                      blocking or non-blocking mode. Zero means
                                      infinity.  If the timeout expires before
                                      all APs return from Procedure, then Procedure
                                      on the failed APs is terminated. All enabled
                                      APs are available for next function assigned
                                      by MpInitLibStartupAllAPs() or
                                      MPInitLibStartupThisAP().
                                      If the timeout expires in blocking mode,
                                      BSP returns EFI_TIMEOUT.  If the timeout
                                      expires in non-blocking mode, WaitEvent
                                      is signaled with SignalEvent().
  @param[in]  ProcedureArgument       The parameter passed into Procedure for
                                      all APs.
  @param[out] FailedCpuList           If NULL, this parameter is ignored. Otherwise,
                                      if all APs finish successfully, then its
                                      content is set to NULL. If not all APs
                                      finish before timeout expires, then its
                                      content is set to address of the buffer
                                      holding handle numbers of the failed APs.
                                      The buffer is allocated by MP Initialization
                                      library, and it's the caller's responsibility to
                                      free the buffer with FreePool() service.
                                      In blocking mode, it is ready for consumption
                                      when the call returns. In non-blocking mode,
                                      it is ready when WaitEvent is signaled.  The
                                      list of failed CPU is terminated by
                                      END_OF_CPU_LIST.

  @retval EFI_SUCCESS             In blocking mode, all APs have finished before
                                  the timeout expired.
  @retval EFI_SUCCESS             In non-blocking mode, function has been dispatched
                                  to all enabled APs.
  @retval EFI_UNSUPPORTED         A non-blocking mode request was made after the
                                  UEFI event EFI_EVENT_GROUP_READY_TO_BOOT was
                                  signaled.
  @retval EFI_UNSUPPORTED         WaitEvent is not NULL if non-blocking mode is not
                                  supported.
  @retval EFI_DEVICE_ERROR        Caller processor is AP.
  @retval EFI_NOT_STARTED         No enabled APs exist in the system.
  @retval EFI_NOT_READY           Any enabled APs are busy.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before
                                  all enabled APs have finished.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.

**/
EFI_STATUS
EFIAPI
MpInitLibStartupAllAPs (
  IN  EFI_AP_PROCEDURE  Procedure,
  IN  BOOLEAN           SingleThread,
  IN  EFI_EVENT         WaitEvent               OPTIONAL,
  IN  UINTN             TimeoutInMicroseconds,
  IN  VOID              *ProcedureArgument      OPTIONAL,
  OUT UINTN             **FailedCpuList         OPTIONAL
  )
{
  if (WaitEvent != NULL) {
    return EFI_UNSUPPORTED;
  }

  return StartupAllCPUsWorker (
           Procedure,
           SingleThread,
           TRUE,
           NULL,
           TimeoutInMicroseconds,
           ProcedureArgument,
           FailedCpuList
           );
}

/**
  This service lets the caller get one enabled AP to execute a caller-provided
  function.

  @param[in]  Procedure               A pointer to the function to be run on the
                                      designated AP of the system. See type
                                      EFI_AP_PROCEDURE.
  @param[in]  ProcessorNumber         The handle number of the AP. The range is
                                      from 0 to the total number of logical
                                      processors minus 1. The total number of
                                      logical processors can be retrieved by
                                      MpInitLibGetNumberOfProcessors().
  @param[in]  WaitEvent               The event created by the caller with CreateEvent()
                                      service.  If it is NULL, then execute in
                                      blocking mode. BSP waits until this AP finish
                                      or TimeoutInMicroSeconds expires.  If it's
                                      not NULL, then execute in non-blocking mode.
                                      BSP requests the function specified by
                                      Procedure to be started on this AP,
                                      and go on executing immediately. If this AP
                                      return from Procedure or TimeoutInMicroSeconds
                                      expires, this event is signaled. The BSP
                                      can use the CheckEvent() or WaitForEvent()
                                      services to check the state of event.  Type
                                      EFI_EVENT is defined in CreateEvent() in
                                      the Unified Extensible Firmware Interface
                                      Specification.
  @param[in]  TimeoutInMicroseconds   Indicates the time limit in microseconds for
                                      this AP to finish this Procedure, either for
                                      blocking or non-blocking mode. Zero means
                                      infinity.  If the timeout expires before
                                      this AP returns from Procedure, then Procedure
                                      on the AP is terminated. The
                                      AP is available for next function assigned
                                      by MpInitLibStartupAllAPs() or
                                      MpInitLibStartupThisAP().
                                      If the timeout expires in blocking mode,
                                      BSP returns EFI_TIMEOUT.  If the timeout
                                      expires in non-blocking mode, WaitEvent
                                      is signaled with SignalEvent().
  @param[in]  ProcedureArgument       The parameter passed into Procedure on the
                                      specified AP.
  @param[out] Finished                If NULL, this parameter is ignored.  In
                                      blocking mode, this parameter is ignored.
                                      In non-blocking mode, if AP returns from
                                      Procedure before the timeout expires, its
                                      content is set to TRUE. Otherwise, the
                                      value is set to FALSE. The caller can
                                      determine if the AP returned from Procedure
                                      by evaluating this value.

  @retval EFI_SUCCESS             In blocking mode, specified AP finished before
                                  the timeout expires.
  @retval EFI_SUCCESS             In non-blocking mode, the function has been
                                  dispatched to specified AP.
  @retval EFI_UNSUPPORTED         A non-blocking mode request was made after the
                                  UEFI event EFI_EVENT_GROUP_READY_TO_BOOT was
                                  signaled.
  @retval EFI_UNSUPPORTED         WaitEvent is not NULL if non-blocking mode is not
                                  supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before
                                  the specified AP has finished.
  @retval EFI_NOT_READY           The specified AP is busy.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.

**/
EFI_STATUS
EFIAPI
MpInitLibStartupThisAP (
  IN  EFI_AP_PROCEDURE  Procedure,
  IN  UINTN             ProcessorNumber,
  IN  EFI_EVENT         WaitEvent               OPTIONAL,
  IN  UINTN             TimeoutInMicroseconds,
  IN  VOID              *ProcedureArgument      OPTIONAL,
  OUT BOOLEAN           *Finished               OPTIONAL
  )
{
  if (WaitEvent != NULL) {
    return EFI_UNSUPPORTED;
  }

  return StartupThisAPWorker (
           Procedure,
           ProcessorNumber,
           NULL,
           TimeoutInMicroseconds,
           ProcedureArgument,
           Finished
           );
}

/**
  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes. This call can only be performed
  by the current BSP.

  @param[in] ProcessorNumber   The handle number of AP that is to become the new
                               BSP. The range is from 0 to the total number of
                               logical processors minus 1. The total number of
                               logical processors can be retrieved by
                               MpInitLibGetNumberOfProcessors().
  @param[in] EnableOldBSP      If TRUE, then the old BSP will be listed as an
                               enabled AP. Otherwise, it will be disabled.

  @retval EFI_SUCCESS             BSP successfully switched.
  @retval EFI_UNSUPPORTED         Switching the BSP cannot be completed prior to
                                  this service returning.
  @retval EFI_UNSUPPORTED         Switching the BSP is not supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the current BSP or
                                  a disabled AP.
  @retval EFI_NOT_READY           The specified AP is busy.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
EFIAPI
MpInitLibSwitchBSP (
  IN UINTN     ProcessorNumber,
  IN  BOOLEAN  EnableOldBSP
  )
{
  return SwitchBSPWorker (ProcessorNumber, EnableOldBSP);
}

/**
  This service lets the caller enable or disable an AP from this point onward.
  This service may only be called from the BSP.

  @param[in] ProcessorNumber   The handle number of AP.
                               The range is from 0 to the total number of
                               logical processors minus 1. The total number of
                               logical processors can be retrieved by
                               MpInitLibGetNumberOfProcessors().
  @param[in] EnableAP          Specifies the new state for the processor for
                               enabled, FALSE for disabled.
  @param[in] HealthFlag        If not NULL, a pointer to a value that specifies
                               the new health status of the AP. This flag
                               corresponds to StatusFlag defined in
                               EFI_MP_SERVICES_PROTOCOL.GetProcessorInfo(). Only
                               the PROCESSOR_HEALTH_STATUS_BIT is used. All other
                               bits are ignored.  If it is NULL, this parameter
                               is ignored.

  @retval EFI_SUCCESS             The specified AP was enabled or disabled successfully.
  @retval EFI_UNSUPPORTED         Enabling or disabling an AP cannot be completed
                                  prior to this service returning.
  @retval EFI_UNSUPPORTED         Enabling or disabling an AP is not supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_NOT_FOUND           Processor with the handle specified by ProcessorNumber
                                  does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
EFIAPI
MpInitLibEnableDisableAP (
  IN  UINTN    ProcessorNumber,
  IN  BOOLEAN  EnableAP,
  IN  UINT32   *HealthFlag OPTIONAL
  )
{
  return EnableDisableApWorker (ProcessorNumber, EnableAP, HealthFlag);
}

/**
  This funtion will try to invoke platform specific microcode shadow logic to
  relocate microcode update patches into memory.

  @param[in, out] CpuMpData  The pointer to CPU MP Data structure.

  @retval EFI_SUCCESS              Shadow microcode success.
  @retval EFI_OUT_OF_RESOURCES     No enough resource to complete the operation.
  @retval EFI_UNSUPPORTED          Can't find platform specific microcode shadow
                                   PPI/Protocol.
**/
EFI_STATUS
PlatformShadowMicrocode (
  IN OUT CPU_MP_DATA  *CpuMpData
  )
{
  EFI_STATUS                      Status;
  EDKII_PEI_SHADOW_MICROCODE_PPI  *ShadowMicrocodePpi;
  UINTN                           CpuCount;
  EDKII_PEI_MICROCODE_CPU_ID      *MicrocodeCpuId;
  UINTN                           Index;
  UINTN                           BufferSize;
  VOID                            *Buffer;

  Status = PeiServicesLocatePpi (
             &gEdkiiPeiShadowMicrocodePpiGuid,
             0,
             NULL,
             (VOID **)&ShadowMicrocodePpi
             );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  CpuCount       = CpuMpData->CpuCount;
  MicrocodeCpuId = (EDKII_PEI_MICROCODE_CPU_ID *)AllocateZeroPool (sizeof (EDKII_PEI_MICROCODE_CPU_ID) * CpuCount);
  if (MicrocodeCpuId == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
    MicrocodeCpuId[Index].ProcessorSignature = CpuMpData->CpuData[Index].ProcessorSignature;
    MicrocodeCpuId[Index].PlatformId         = CpuMpData->CpuData[Index].PlatformId;
  }

  Status = ShadowMicrocodePpi->ShadowMicrocode (
                                 ShadowMicrocodePpi,
                                 CpuCount,
                                 MicrocodeCpuId,
                                 &BufferSize,
                                 &Buffer
                                 );
  FreePool (MicrocodeCpuId);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  CpuMpData->MicrocodePatchAddress    = (UINTN)Buffer;
  CpuMpData->MicrocodePatchRegionSize = BufferSize;

  DEBUG ((
    DEBUG_INFO,
    "%a: Required microcode patches have been loaded at 0x%lx, with size 0x%lx.\n",
    __func__,
    CpuMpData->MicrocodePatchAddress,
    CpuMpData->MicrocodePatchRegionSize
    ));

  return EFI_SUCCESS;
}

/**
  Allocate buffer for ApLoopCode.

  @param[in]      Pages    Number of pages to allocate.
  @param[in, out] Address  Pointer to the allocated buffer.
**/
VOID
AllocateApLoopCodeBuffer (
  IN UINTN                     Pages,
  IN OUT EFI_PHYSICAL_ADDRESS  *Address
  )
{
  EFI_STATUS  Status;

  Status = PeiServicesAllocatePages (EfiACPIMemoryNVS, Pages, Address);
  if (EFI_ERROR (Status)) {
    *Address = 0;
  }
}

/**
  Remove Nx protection for the range specific by BaseAddress and Length.

  The PEI implementation uses CpuPageTableLib to change the attribute.
  The DXE implementation uses gDS to change the attribute.

  @param[in] BaseAddress  BaseAddress of the range.
  @param[in] Length       Length of the range.
**/
VOID
RemoveNxprotection (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN                 Length
  )
{
  EFI_STATUS                  Status;
  UINTN                       PageTable;
  EFI_PHYSICAL_ADDRESS        Buffer;
  UINTN                       BufferSize;
  IA32_MAP_ATTRIBUTE          MapAttribute;
  IA32_MAP_ATTRIBUTE          MapMask;
  PAGING_MODE                 PagingMode;
  IA32_CR4                    Cr4;
  BOOLEAN                     Page5LevelSupport;
  UINT32                      RegEax;
  BOOLEAN                     Page1GSupport;
  CPUID_EXTENDED_CPU_SIG_EDX  RegEdx;

  if (sizeof (UINTN) == sizeof (UINT64)) {
    //
    // Check Page5Level Support or not.
    //
    Cr4.UintN         = AsmReadCr4 ();
    Page5LevelSupport = (Cr4.Bits.LA57 ? TRUE : FALSE);

    //
    // Check Page1G Support or not.
    //
    Page1GSupport = FALSE;
    AsmCpuid (CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
    if (RegEax >= CPUID_EXTENDED_CPU_SIG) {
      AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, NULL, &RegEdx.Uint32);
      if (RegEdx.Bits.Page1GB != 0) {
        Page1GSupport = TRUE;
      }
    }

    //
    // Decide Paging Mode according Page5LevelSupport & Page1GSupport.
    //
    if (Page5LevelSupport) {
      PagingMode = Page1GSupport ? Paging5Level1GB : Paging5Level;
    } else {
      PagingMode = Page1GSupport ? Paging4Level1GB : Paging4Level;
    }
  } else {
    PagingMode = PagingPae;
  }

  MapAttribute.Uint64 = 0;
  MapMask.Uint64      = 0;
  MapMask.Bits.Nx     = 1;
  PageTable           = AsmReadCr3 () & PAGING_4K_ADDRESS_MASK_64;
  BufferSize          = 0;

  //
  // Get required buffer size for changing the pagetable.
  //
  Status = PageTableMap (&PageTable, PagingMode, 0, &BufferSize, BaseAddress, Length, &MapAttribute, &MapMask, NULL);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Allocate required Buffer.
    //
    Status = PeiServicesAllocatePages (
               EfiBootServicesData,
               EFI_SIZE_TO_PAGES (BufferSize),
               &Buffer
               );
    ASSERT_EFI_ERROR (Status);
    Status = PageTableMap (&PageTable, PagingMode, (VOID *)(UINTN)Buffer, &BufferSize, BaseAddress, Length, &MapAttribute, &MapMask, NULL);
  }

  ASSERT_EFI_ERROR (Status);
  AsmWriteCr3 (PageTable);
}

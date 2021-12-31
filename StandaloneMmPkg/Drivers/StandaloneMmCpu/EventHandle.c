/** @file

  Copyright (c) 2016 HP Development Company, L.P.
  Copyright (c) 2016 - 2021, Arm Limited. All rights reserved.
  Copyright (c) 2021, Linaro Limited

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Pi/PiMmCis.h>

#include <Library/ArmSvcLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>

#include <Protocol/DebugSupport.h> // for EFI_SYSTEM_CONTEXT

#include <Guid/ZeroGuid.h>
#include <Guid/MmramMemoryReserve.h>

#include <IndustryStandard/ArmFfaSvc.h>
#include <IndustryStandard/ArmStdSmc.h>

#include "StandaloneMmCpu.h"

EFI_STATUS
EFIAPI
MmFoundationEntryRegister (
  IN CONST EFI_MM_CONFIGURATION_PROTOCOL  *This,
  IN EFI_MM_ENTRY_POINT                   MmEntryPoint
  );

//
// On ARM platforms every event is expected to have a GUID associated with
// it. It will be used by the MM Entry point to find the handler for the
// event. It will either be populated in a EFI_MM_COMMUNICATE_HEADER by the
// caller of the event (e.g. MM_COMMUNICATE SMC) or by the CPU driver
// (e.g. during an asynchronous event). In either case, this context is
// maintained in an array which has an entry for each CPU. The pointer to this
// array is held in PerCpuGuidedEventContext. Memory is allocated once the
// number of CPUs in the system are made known through the
// MP_INFORMATION_HOB_DATA.
//
EFI_MM_COMMUNICATE_HEADER  **PerCpuGuidedEventContext = NULL;

// Descriptor with whereabouts of memory used for communication with the normal world
EFI_MMRAM_DESCRIPTOR  mNsCommBuffer;
EFI_MMRAM_DESCRIPTOR  mSCommBuffer;

MP_INFORMATION_HOB_DATA  *mMpInformationHobData;

EFI_MM_CONFIGURATION_PROTOCOL  mMmConfig = {
  0,
  MmFoundationEntryRegister
};

STATIC EFI_MM_ENTRY_POINT  mMmEntryPoint = NULL;

/**
  Perform bounds check on the common buffer.

  @param  [in] BufferAddr   Address of the common buffer.

  @retval   EFI_SUCCESS             Success.
  @retval   EFI_ACCESS_DENIED       Access not permitted.
**/
STATIC
EFI_STATUS
CheckBufferAddr (
  IN UINTN  BufferAddr
  )
{
  UINT64  NsCommBufferEnd;
  UINT64  SCommBufferEnd;
  UINT64  CommBufferEnd;

  NsCommBufferEnd = mNsCommBuffer.PhysicalStart + mNsCommBuffer.PhysicalSize;
  SCommBufferEnd  = mSCommBuffer.PhysicalStart + mSCommBuffer.PhysicalSize;

  if ((BufferAddr >= mNsCommBuffer.PhysicalStart) &&
      (BufferAddr < NsCommBufferEnd))
  {
    CommBufferEnd = NsCommBufferEnd;
  } else if ((BufferAddr >= mSCommBuffer.PhysicalStart) &&
             (BufferAddr < SCommBufferEnd))
  {
    CommBufferEnd = SCommBufferEnd;
  } else {
    return EFI_ACCESS_DENIED;
  }

  if ((CommBufferEnd - BufferAddr) < sizeof (EFI_MM_COMMUNICATE_HEADER)) {
    return EFI_ACCESS_DENIED;
  }

  // perform bounds check.
  if ((CommBufferEnd - BufferAddr - sizeof (EFI_MM_COMMUNICATE_HEADER)) <
      ((EFI_MM_COMMUNICATE_HEADER *)BufferAddr)->MessageLength)
  {
    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}

/**
  The PI Standalone MM entry point for the TF-A CPU driver.

  @param  [in] EventId            The event Id.
  @param  [in] CpuNumber          The CPU number.
  @param  [in] NsCommBufferAddr   Address of the NS common buffer.

  @retval   EFI_SUCCESS             Success.
  @retval   EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval   EFI_ACCESS_DENIED       Access not permitted.
  @retval   EFI_OUT_OF_RESOURCES    Out of resources.
  @retval   EFI_UNSUPPORTED         Operation not supported.
**/
EFI_STATUS
PiMmStandaloneArmTfCpuDriverEntry (
  IN UINTN  EventId,
  IN UINTN  CpuNumber,
  IN UINTN  NsCommBufferAddr
  )
{
  EFI_MM_COMMUNICATE_HEADER  *GuidedEventContext;
  EFI_MM_ENTRY_CONTEXT       MmEntryPointContext;
  EFI_STATUS                 Status;
  UINTN                      NsCommBufferSize;

  DEBUG ((DEBUG_INFO, "Received event - 0x%x on cpu %d\n", EventId, CpuNumber));

  Status = EFI_SUCCESS;
  //
  // ARM TF passes SMC FID of the MM_COMMUNICATE interface as the Event ID upon
  // receipt of a synchronous MM request. Use the Event ID to distinguish
  // between synchronous and asynchronous events.
  //
  if ((ARM_SMC_ID_MM_COMMUNICATE != EventId) &&
      (ARM_SVC_ID_FFA_MSG_SEND_DIRECT_REQ != EventId))
  {
    DEBUG ((DEBUG_ERROR, "UnRecognized Event - 0x%x\n", EventId));
    return EFI_INVALID_PARAMETER;
  }

  // Perform parameter validation of NsCommBufferAddr
  if (NsCommBufferAddr == (UINTN)NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = CheckBufferAddr (NsCommBufferAddr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Check Buffer failed: %r\n", Status));
    return Status;
  }

  // Find out the size of the buffer passed
  NsCommBufferSize = ((EFI_MM_COMMUNICATE_HEADER *)NsCommBufferAddr)->MessageLength +
                     sizeof (EFI_MM_COMMUNICATE_HEADER);

  GuidedEventContext = NULL;
  // Now that the secure world can see the normal world buffer, allocate
  // memory to copy the communication buffer to the secure world.
  Status = mMmst->MmAllocatePool (
                    EfiRuntimeServicesData,
                    NsCommBufferSize,
                    (VOID **)&GuidedEventContext
                    );

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Mem alloc failed - 0x%x\n", EventId));
    return EFI_OUT_OF_RESOURCES;
  }

  // X1 contains the VA of the normal world memory accessible from
  // S-EL0
  CopyMem (GuidedEventContext, (CONST VOID *)NsCommBufferAddr, NsCommBufferSize);

  // Stash the pointer to the allocated Event Context for this CPU
  PerCpuGuidedEventContext[CpuNumber] = GuidedEventContext;

  ZeroMem (&MmEntryPointContext, sizeof (EFI_MM_ENTRY_CONTEXT));

  MmEntryPointContext.CurrentlyExecutingCpu = CpuNumber;
  MmEntryPointContext.NumberOfCpus          = mMpInformationHobData->NumberOfProcessors;

  // Populate the MM system table with MP and state information
  mMmst->CurrentlyExecutingCpu = CpuNumber;
  mMmst->NumberOfCpus          = mMpInformationHobData->NumberOfProcessors;
  mMmst->CpuSaveStateSize      = 0;
  mMmst->CpuSaveState          = NULL;

  if (mMmEntryPoint == NULL) {
    DEBUG ((DEBUG_ERROR, "Mm Entry point Not Found\n"));
    return EFI_UNSUPPORTED;
  }

  mMmEntryPoint (&MmEntryPointContext);

  // Free the memory allocation done earlier and reset the per-cpu context
  ASSERT (GuidedEventContext);
  CopyMem ((VOID *)NsCommBufferAddr, (CONST VOID *)GuidedEventContext, NsCommBufferSize);

  Status = mMmst->MmFreePool ((VOID *)GuidedEventContext);
  if (Status != EFI_SUCCESS) {
    return EFI_OUT_OF_RESOURCES;
  }

  PerCpuGuidedEventContext[CpuNumber] = NULL;

  return Status;
}

/**
  Registers the MM foundation entry point.

  @param  [in] This               Pointer to the MM Configuration protocol.
  @param  [in] MmEntryPoint       Function pointer to the MM Entry point.

  @retval   EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
MmFoundationEntryRegister (
  IN CONST EFI_MM_CONFIGURATION_PROTOCOL  *This,
  IN EFI_MM_ENTRY_POINT                   MmEntryPoint
  )
{
  // store the entry point in a global
  mMmEntryPoint = MmEntryPoint;
  return EFI_SUCCESS;
}

/**
  This function is the main entry point for an MM handler dispatch
  or communicate-based callback.

  @param  DispatchHandle  The unique handle assigned to this handler by
                          MmiHandlerRegister().
  @param  Context         Points to an optional handler context which was
                          specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-MM environment into an
                          MM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
PiMmCpuTpFwRootMmiHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context         OPTIONAL,
  IN OUT VOID        *CommBuffer      OPTIONAL,
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       CpuNumber;

  ASSERT (Context == NULL);
  ASSERT (CommBuffer == NULL);
  ASSERT (CommBufferSize == NULL);

  CpuNumber = mMmst->CurrentlyExecutingCpu;
  if (PerCpuGuidedEventContext[CpuNumber] == NULL) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    DEBUG_INFO,
    "CommBuffer - 0x%x, CommBufferSize - 0x%x\n",
    PerCpuGuidedEventContext[CpuNumber],
    PerCpuGuidedEventContext[CpuNumber]->MessageLength
    ));

  Status = mMmst->MmiManage (
                    &PerCpuGuidedEventContext[CpuNumber]->HeaderGuid,
                    NULL,
                    PerCpuGuidedEventContext[CpuNumber]->Data,
                    &PerCpuGuidedEventContext[CpuNumber]->MessageLength
                    );

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_WARN, "Unable to manage Guided Event - %d\n", Status));
  }

  return Status;
}

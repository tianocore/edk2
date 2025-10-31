/** @file

  Copyright (c) 2016 HP Development Company, L.P.
  Copyright (c) 2016 - 2024, Arm Limited. All rights reserved.
  Copyright (c) 2021, Linaro Limited
  Copyright (c) 2023, Ventana Micro System Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Pi/PiMmCis.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>

#include <Protocol/DebugSupport.h> // for EFI_SYSTEM_CONTEXT

#include <Guid/ZeroGuid.h>
#include <Guid/MmramMemoryReserve.h>

#include <StandaloneMmCpu.h>

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
// maintained in single global variable because StandaloneMm is UP-migratable
// (which means it cannot run concurrently)
//
STATIC EFI_MM_COMMUNICATE_HEADER     *mGuidedEventContext = NULL;
STATIC CONST ARM_MM_HANDLER_CONTEXT  *mMmHandlerContext   = NULL;

EFI_MM_CONFIGURATION_PROTOCOL  mMmConfig = {
  0,
  MmFoundationEntryRegister
};

EDKII_PI_MM_CPU_DRIVER_EP_PROTOCOL  mPiMmCpuDriverEpProtocol = {
  PiMmStandaloneMmCpuDriverEntry
};

STATIC EFI_MM_ENTRY_POINT  mMmEntryPoint = NULL;

/**
  The PI Standalone MM entry point for the CPU driver.

  @param  [in] MmHandlerContext     Arm specific Mm handler context.
  @param  [in] CommBufferAddr       Address of the communication buffer.

  @retval   EFI_SUCCESS             Success.
  @retval   EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval   EFI_ACCESS_DENIED       Access not permitted.
  @retval   EFI_OUT_OF_RESOURCES    Out of resources.
  @retval   EFI_UNSUPPORTED         Operation not supported.
**/
EFI_STATUS
PiMmStandaloneMmCpuDriverEntry (
  IN CONST ARM_MM_HANDLER_CONTEXT  *MmHandlerContext,
  IN UINTN                         CommBufferAddr
  )
{
  EFI_MM_ENTRY_CONTEXT  MmEntryPointContext;
  EFI_STATUS            Status;
  UINTN                 CommBufferSize;

  Status = EFI_SUCCESS;

  // Perform parameter validation.
  if ((MmHandlerContext == NULL) || (CommBufferAddr == (UINTN)NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mMmEntryPoint == NULL) {
    DEBUG ((DEBUG_ERROR, "Mm Entry point Not Found\n"));
    return EFI_UNSUPPORTED;
  }

  // Find out the size of the buffer passed
  if (CompareGuid (
        &((EFI_MM_COMMUNICATE_HEADER *)CommBufferAddr)->HeaderGuid,
        &gEfiMmCommunicateHeaderV3Guid
        ))
  {
    // This is a v3 header
    CommBufferSize = ((EFI_MM_COMMUNICATE_HEADER_V3 *)CommBufferAddr)->BufferSize;
  } else {
    // This is a v1 header
    // Find out the size of the buffer passed
    CommBufferSize = ((EFI_MM_COMMUNICATE_HEADER *)CommBufferAddr)->MessageLength +
                     OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data);
  }

  // Now that the secure world can see the normal world buffer, allocate
  // memory to copy the communication buffer to the secure world.
  Status = mMmst->MmAllocatePool (
                    EfiRuntimeServicesData,
                    CommBufferSize,
                    (VOID **)&mGuidedEventContext
                    );

  if (EFI_ERROR (Status)) {
    mGuidedEventContext = NULL;
    DEBUG ((DEBUG_ERROR, "%a: Mem alloc failed\n", __func__));
    return Status;
  }

  CopyMem (mGuidedEventContext, (CONST VOID *)CommBufferAddr, CommBufferSize);
  mMmHandlerContext = MmHandlerContext;

  ZeroMem (&MmEntryPointContext, sizeof (EFI_MM_ENTRY_CONTEXT));

  // StandaloneMm UP-migratable which means it cannot run concurrently.
  // Therefore, set number of cpus as 1 and cpu number as 0.
  MmEntryPointContext.CurrentlyExecutingCpu = 0;
  MmEntryPointContext.NumberOfCpus          = 1;

  // Populate the MM system table with MP and state information
  mMmst->CurrentlyExecutingCpu = 0;
  mMmst->NumberOfCpus          = 1;
  mMmst->CpuSaveStateSize      = 0;
  mMmst->CpuSaveState          = NULL;

  mMmEntryPoint (&MmEntryPointContext);

  // Free the memory allocation done earlier and reset the per-cpu context
  CopyMem ((VOID *)CommBufferAddr, (CONST VOID *)mGuidedEventContext, CommBufferSize);

  Status = mMmst->MmFreePool ((VOID *)mGuidedEventContext);
  ASSERT_EFI_ERROR (Status);
  mGuidedEventContext = NULL;
  mMmHandlerContext   = NULL;

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
  EFI_STATUS                    Status;
  EFI_MM_COMMUNICATE_HEADER_V3  *GuidedEventContextV3;

  ASSERT (Context == NULL);
  ASSERT (CommBuffer == NULL);
  ASSERT (CommBufferSize == NULL);

  if (mGuidedEventContext == NULL) {
    return EFI_NOT_FOUND;
  }

  if (CompareGuid (
        &mGuidedEventContext->HeaderGuid,
        &gEfiMmCommunicateHeaderV3Guid
        ))
  {
    GuidedEventContextV3 = (EFI_MM_COMMUNICATE_HEADER_V3 *)mGuidedEventContext;
    DEBUG ((
      DEBUG_INFO,
      "CommBuffer - 0x%x, CommBufferSize - 0x%llx, MessageSize - 0x%llx\n",
      GuidedEventContextV3,
      GuidedEventContextV3->BufferSize,
      GuidedEventContextV3->MessageSize
      ));

    Status = mMmst->MmiManage (
                      &GuidedEventContextV3->MessageGuid,
                      mMmHandlerContext,
                      GuidedEventContextV3->MessageData,
                      (UINTN *)(&GuidedEventContextV3->MessageSize)
                      );
  } else {
    DEBUG ((
      DEBUG_INFO,
      "CommBuffer - 0x%x, CommBufferSize - 0x%x\n",
      mGuidedEventContext,
      mGuidedEventContext->MessageLength
      ));
    Status = mMmst->MmiManage (
                      &mGuidedEventContext->HeaderGuid,
                      mMmHandlerContext,
                      mGuidedEventContext->Data,
                      (UINTN *)(&mGuidedEventContext->MessageLength)
                      );
  }

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_WARN, "Unable to manage Guided Event - %d\n", Status));
  }

  return Status;
}

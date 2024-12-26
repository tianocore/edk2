/** @file

  Copyright (c) 2023, Ventana Micro System Inc. All rights reserved.
  Copyright (c) 2025, Rivos Inc. All rights reserved.

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

EFI_MMRAM_DESCRIPTOR  mSCommBuffer;

MP_INFORMATION_HOB_DATA  *mMpInformationHobData;

EFI_MM_CONFIGURATION_PROTOCOL  mMmConfig = {
  0,
  MmFoundationEntryRegister
};

EDKII_PI_MM_CPU_DRIVER_EP_PROTOCOL  mPiMmCpuDriverEpProtocol = {
  PiMmStandaloneMmCpuDriverEntry
};
STATIC EFI_MM_ENTRY_POINT           mMmEntryPoint = NULL;

/**
  The PI Standalone MM entry point for the CPU driver.

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
PiMmStandaloneMmCpuDriverEntry (
  IN UINTN  EventId,
  IN UINTN  CpuNumber,
  IN UINTN  NsCommBufferAddr
  )
{
  EFI_MM_ENTRY_CONTEXT  MmEntryPointContext;
  EFI_STATUS            Status;

  DEBUG ((DEBUG_VERBOSE, "Received event - 0x%x on cpu %d\n", EventId, CpuNumber));

  Status = EFI_SUCCESS;

  ZeroMem (&MmEntryPointContext, sizeof (EFI_MM_ENTRY_CONTEXT));

  MmEntryPointContext.CurrentlyExecutingCpu = CpuNumber;
  MmEntryPointContext.NumberOfCpus          = 1;

  // Populate the MM system table with MP and state information
  mMmst->CurrentlyExecutingCpu = CpuNumber;
  mMmst->NumberOfCpus          = 1;
  mMmst->CpuSaveStateSize      = 0;
  mMmst->CpuSaveState          = NULL;

  if (mMmEntryPoint == NULL) {
    DEBUG ((DEBUG_ERROR, "Mm Entry point Not Found\n"));
    return EFI_UNSUPPORTED;
  }

  mMmEntryPoint (&MmEntryPointContext);

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
  UINTN  CpuNumber;

  ASSERT (Context == NULL);
  ASSERT (CommBuffer == NULL);
  ASSERT (CommBufferSize == NULL);

  CpuNumber = 1;

  return EFI_SUCCESS;
}

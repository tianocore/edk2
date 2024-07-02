/** @file
  The instance of MM Unblock Page Library.
  This library provides an interface to request non-MMRAM pages to be mapped/unblocked
  from inside MM environment.
  For MM modules that need to access regions outside of MMRAMs, the agents that set up
  these regions are responsible for invoking this API in order for these memory areas
  to be accessed from inside MM.

  Copyright (c) Microsoft Corporation.
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <Guid/UnblockRegion.h>
#include <Ppi/MmCommunication.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Base.h>

/**
  This API provides a way to unblock certain data pages to be accessible inside MM environment.
  For the original design, when the CommonBufferX is allocated, Smi handler can sense directly, there is no need to
  register which is not allowed anymore.
  The driver, named DriverX, needs to sense a piece of memory to communicate with the mm environment. There are two ways:
      1. Use a fixed communication buffer, allocated by the PEI driver.
      2. Register a piece of memory through unblock memory request. Smi handlers and drivers can communicate through it.
       Once this memory is registered it calls the communication protocol which can transfer the buffer, named CommonBufferX to
       CommDxe, can copy to fixed communication buffer.
       Therefore, thus mmCore can shadow this buffer in preparation for communication.
  Here in this function, it implement the item 2 in above description.
  gMmUnblockRegionHobGuid will be consumed by ipl when call createMmHobs, and put all the information to the resource hob.
  @param  UnblockAddress              The address of buffer caller requests to unblock, the address
                                      has to be page aligned.
  @param  NumberOfPages               The number of pages requested to be unblocked from MM
                                      environment.
  @retval RETURN_SUCCESS              The request goes through successfully.
  @retval RETURN_NOT_AVAILABLE_YET    The requested functionality is not produced yet.
  @retval RETURN_UNSUPPORTED          The requested functionality is not supported on current platform.
  @retval RETURN_SECURITY_VIOLATION   The requested address failed to pass security check for
                                      unblocking.
  @retval RETURN_INVALID_PARAMETER    Input address either NULL pointer or not page aligned.
  @retval RETURN_ACCESS_DENIED        The request is rejected due to system has passed certain boot
                                      phase.
**/
EFI_STATUS
EFIAPI
MmUnblockMemoryRequest (
  IN EFI_PHYSICAL_ADDRESS  UnblockAddress,
  IN UINT64                NumberOfPages
  )
{
  EFI_STATUS                    Status;
  MM_UNBLOCK_REGION             *MmUnblockMemoryHob;
  EFI_PEI_MM_COMMUNICATION_PPI  *MmCommunicationPpi;

  //
  // IPL should make sure the gMmUnblockRegionHobGuid is created. At the point when we can ensure all the
  // consumers are already called this API via check is the gEfiPeiMmCommunicationPpiGuid failed or not.
  //
  Status = PeiServicesLocatePpi (&gEfiPeiMmCommunicationPpiGuid, 0, NULL, (VOID **)&MmCommunicationPpi);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to locate PEI MM Communication PPI Communicate PPI is installed, request too late: %r\n", __func__, Status));
    CpuDeadLoop ();
    goto Exit;
  }

  //
  // Build the GUID'd HOB for MmCore
  //
  MmUnblockMemoryHob = BuildGuidHob (&gMmUnblockRegionHobGuid, sizeof (MM_UNBLOCK_REGION));
  if (MmUnblockMemoryHob == NULL) {
    DEBUG ((DEBUG_ERROR, "%a Failed to allocate hob for unblocked data parameter!!\n", __FUNCTION__));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  ZeroMem (MmUnblockMemoryHob, sizeof (MM_UNBLOCK_REGION));

  //
  // Caller ID is filled in.
  //
  CopyMem (&MmUnblockMemoryHob->IdentifierGuid, &gEfiCallerIdGuid, sizeof (EFI_GUID));
  MmUnblockMemoryHob->MemoryDescriptor.PhysicalStart = UnblockAddress;
  MmUnblockMemoryHob->MemoryDescriptor.NumberOfPages = NumberOfPages;
  Status                                             = EFI_SUCCESS;

Exit:
  return Status;
}

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
#include <Guid/MmUnblockRegion.h>
#include <Ppi/MmCommunication.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Base.h>

/**
  This API provides a way to unblock certain data pages to be accessible inside MM environment.

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

  if (!IS_ALIGNED (UnblockAddress, SIZE_4KB)) {
    DEBUG ((DEBUG_ERROR, "Error: UnblockAddress is not 4KB aligned: %p\n", UnblockAddress));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Unblock requests are rejected when MmIpl finishes execution.
  //
  Status = PeiServicesLocatePpi (&gEfiPeiMmCommunicationPpiGuid, 0, NULL, (VOID **)&MmCommunicationPpi);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unblock requests are rejected since the MmIpl finishes execution\n"));
    return RETURN_ACCESS_DENIED;
  }

  //
  // Build the GUID'd HOB for MmCore
  //
  MmUnblockMemoryHob = BuildGuidHob (&gMmUnblockRegionHobGuid, sizeof (MM_UNBLOCK_REGION));
  if (MmUnblockMemoryHob == NULL) {
    DEBUG ((DEBUG_ERROR, "MmUnblockMemoryRequest: Failed to allocate hob for unblocked data parameter!!\n"));
    return Status;
  }

  ZeroMem (MmUnblockMemoryHob, sizeof (MM_UNBLOCK_REGION));

  //
  // Caller ID is filled in.
  //
  CopyGuid (&MmUnblockMemoryHob->IdentifierGuid, &gEfiCallerIdGuid);
  MmUnblockMemoryHob->PhysicalStart = UnblockAddress;
  MmUnblockMemoryHob->NumberOfPages = NumberOfPages;
  return EFI_SUCCESS;
}

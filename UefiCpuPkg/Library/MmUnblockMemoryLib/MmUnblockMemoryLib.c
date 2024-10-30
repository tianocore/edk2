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
#include <PiPei.h>

/**
  This function checks if the input buffer range [UnblockBase, UnblockEnd] is unblockable.
  The input range should be covered by the EfiRuntimeServicesData, EfiACPIMemoryNVS or
  EfiReservedMemoryType memory allocation HOB.

  @param[in]  HobStart                The starting HOB pointer to search from.
  @param[in]  UnblockAddress          Base address of the range to unblock.
  @param[in]  UnblockEnd              End address of the range to unblock.

  @retval RETURN_SUCCESS              The range is unblockable.
  @retval RETURN_INVALID_PARAMETER    The range to unblock contains invalid memory.
**/
EFI_STATUS
MmUnblockMemoryLibIsUnblockableRegion (
  IN VOID                  *HobStart,
  IN EFI_PHYSICAL_ADDRESS  UnblockBase,
  IN EFI_PHYSICAL_ADDRESS  UnblockEnd
  )
{
  EFI_PHYSICAL_ADDRESS       HobBase;
  EFI_PHYSICAL_ADDRESS       HobEnd;
  EFI_STATUS                 Status;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryAllocationHob;

  while ((MemoryAllocationHob = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, HobStart)) != NULL) {
    HobBase = MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress;
    HobEnd  = MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress +
              MemoryAllocationHob->AllocDescriptor.MemoryLength;
    if ((UnblockBase < HobEnd) && (UnblockEnd > HobBase)) {
      //
      // The overlapped memory allocation HOB type must be one of the three specific types.
      //
      if ((MemoryAllocationHob->AllocDescriptor.MemoryType != EfiRuntimeServicesData) &&
          (MemoryAllocationHob->AllocDescriptor.MemoryType != EfiACPIMemoryNVS) &&
          (MemoryAllocationHob->AllocDescriptor.MemoryType != EfiReservedMemoryType))
      {
        DEBUG ((DEBUG_ERROR, "Error: range [0x%lx, 0x%lx] to unblock contains invalid type memory\n", UnblockBase, UnblockEnd));
        return RETURN_INVALID_PARAMETER;
      }

      if (UnblockBase < HobBase) {
        //
        // Recursively call to check [UnblockBase, HobBase]
        //
        Status = MmUnblockMemoryLibIsUnblockableRegion (
                   GET_NEXT_HOB (MemoryAllocationHob),
                   UnblockBase,
                   HobBase
                   );
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }

      if (UnblockEnd > HobEnd) {
        //
        // Recursively call to check [HobEnd, UnblockEnd]
        //
        Status = MmUnblockMemoryLibIsUnblockableRegion (
                   GET_NEXT_HOB (MemoryAllocationHob),
                   HobEnd,
                   UnblockEnd
                   );
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }

      return EFI_SUCCESS;
    }

    HobStart = GET_NEXT_HOB (MemoryAllocationHob);
  }

  DEBUG ((DEBUG_ERROR, "Error: range [0x%lx, 0x%lx] to unblock doesn't belong to any memory allocation HOB\n", UnblockBase, UnblockEnd));
  return RETURN_INVALID_PARAMETER;
}

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
  @retval RETURN_INVALID_PARAMETER    Input range to unblock contains invalid types memory other than
                                      EfiRuntimeServicesData, EfiACPIMemoryNVS, and EfiReservedMemory.
  @retval RETURN_INVALID_PARAMETER    Input range to unblock contains memory that doesn't belong to
                                      any memory allocation HOB.
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
  VOID                          *HobList;

  if (!IS_ALIGNED (UnblockAddress, SIZE_4KB)) {
    DEBUG ((DEBUG_ERROR, "Error: UnblockAddress is not 4KB aligned: %p\n", UnblockAddress));
    return EFI_INVALID_PARAMETER;
  }

  HobList = GetHobList ();
  Status  = MmUnblockMemoryLibIsUnblockableRegion (HobList, UnblockAddress, UnblockAddress + EFI_PAGES_TO_SIZE (NumberOfPages));
  if (EFI_ERROR (Status)) {
    return Status;
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

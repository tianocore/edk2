/** @file
 Module produces EFI_FIRMWARE_VOLUME_INFO_PPI based on each FV HOB if no such PPI exist for the FV.

 UEFI PI Spec supersedes Intel's Framework Specs. 
 EFI_FIRMWARE_VOLUME_INFO_PPI defined in PI spec is required by PI PEI core to dispatch PEIMs or find DXE core
 in the FV.
 This module produces produces EFI_FIRMWARE_VOLUME_INFO_PPI based on each FV HOB if no such PPI exist for the FV.
 This module is used on platform when both of these two conditions are true:
 1) Framework platform module produces FV HOB but does not build EFI_FIRMWARE_VOLUME_INFO_PPI.
 2) The platform has PI PEI core that cosumes EFI_FIRMWARE_VOLUME_INFO_PPI to dispatch PEIMs or DXE core
     in FVs other than Boot Fimware Volume.

Copyright (c) 2008 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
Module Name:

**/

#include <PiPei.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>
#include <Library/PeiPiLib.h>


/**
  Check if there is a matching EFI_PEI_FIRMWARE_VOLUME_INFO_PPI for the FV described by FvHob.

  This function located all existing instance of EFI_PEI_FIRMWARE_VOLUME_INFO_PPI in the platform.
  Then it compare the base address of the FVs found with that of FvHob. If a matching base address is
  found, the function return TRUE. Otherwise, FALSE is returned.

  If a matching base address of FV is found but the length of FV does not match, then ASSERT ().

  @param  FvHob      The FV Hob.

  @retval   TRUE       A instance of EFI_PEI_FIRMWARE_VOLUME_INFO_PPI is already installed.

**/
BOOLEAN
FvInfoPpiInstalled (
  EFI_HOB_FIRMWARE_VOLUME * FvHob
  )
{
  EFI_STATUS                            Status;
  UINTN                                 Idx;
  EFI_PEI_FIRMWARE_VOLUME_INFO_PPI      *FvInfoPpi;

  for (Idx = 0; ; Idx++) {
    Status = PeiServicesLocatePpi (
                &gEfiPeiFirmwareVolumeInfoPpiGuid,
                Idx,
                NULL,
                (VOID **) &FvInfoPpi
               );
    if (!EFI_ERROR (Status)) {
      if (FvHob->BaseAddress == (EFI_PHYSICAL_ADDRESS) (UINTN) FvInfoPpi->FvInfo) {
        ASSERT (FvHob->Length == (UINT64) FvInfoPpi->FvInfoSize);
        return TRUE;
      }
    } else {
      break;
    }
  }

  return FALSE;
}

/**
  PEIM's standard entry point.
  
  @param FfsHeader    Image's header
  @param PeiServices  Pointer of EFI_PEI_SERVICES
  @return EFI_SUCESS This entry point always return successfully.
  
**/
EFI_STATUS
EFIAPI
FvHobtoFvInfoThunkEntry (
  IN EFI_PEI_FILE_HANDLE       FfsHeader,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  EFI_PEI_HOB_POINTERS        HobPointer;
  
  for (HobPointer.Raw = GetNextHob (EFI_HOB_TYPE_FV, GetHobList ());
        HobPointer.Raw != NULL;
        HobPointer.Raw = GetNextHob (EFI_HOB_TYPE_FV, GET_NEXT_HOB (HobPointer))) {
    if (!FvInfoPpiInstalled (HobPointer.FirmwareVolume)) {
      PiLibInstallFvInfoPpi (
        &(((EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) HobPointer.FirmwareVolume->BaseAddress)->FileSystemGuid),
        (VOID *) (UINTN) HobPointer.FirmwareVolume->BaseAddress,
        (UINT32) HobPointer.FirmwareVolume->Length,
        NULL,
        NULL
        );
    }
  }

  return EFI_SUCCESS;
}


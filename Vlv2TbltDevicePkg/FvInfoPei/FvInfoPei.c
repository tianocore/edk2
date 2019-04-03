/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:

    FvInfoPei.c

Abstract:

    EFI 2.0 PEIM to initialize the cache and program for unlock processor



--*/

#include <PiPei.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>

EFI_PEI_FIRMWARE_VOLUME_INFO_PPI mAddtionFVPpi = {
  EFI_FIRMWARE_FILE_SYSTEM2_GUID,
  (VOID*)(UINTN)FixedPcdGet32(PcdFlashFvRecovery2Base),
  FixedPcdGet32(PcdFlashFvRecovery2Size),
  NULL,
  NULL
};

EFI_PEI_PPI_DESCRIPTOR  mPpiList[] = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiFirmwareVolumeInfoPpiGuid,
  &mAddtionFVPpi
};


/**
  Add Recovery Fv Info to the Pei Core.

  @param  PeiServices  General purpose services available to every PEIM.

  @retval  Status

**/
EFI_STATUS
EFIAPI
PeimInitializeFvInfo (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )

//
// GC_TODO:    FfsHeader - add argument and description to function comment
//
{
  EFI_STATUS  Status;
  Status = (**PeiServices).InstallPpi (PeiServices, &mPpiList[0]);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((EFI_D_INFO, "\nFvInfo Add Fv Info\n"));

  return Status;
}

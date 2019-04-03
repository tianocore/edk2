/** @file
  This PEIM will parse the hoblist from fsp and report them into pei core.
  This file contains the main entrypoint of the PEIM.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <PiPei.h>
#include <Ppi/MasterBootMode.h>

static EFI_PEI_PPI_DESCRIPTOR       mPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiMasterBootModePpiGuid,
    NULL
  },
};

/**
  This is the entrypoint of PEIM

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
BootModePeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  (*PeiServices)->SetBootMode(PeiServices, BOOT_WITH_FULL_CONFIGURATION);

  (*PeiServices)->InstallPpi (PeiServices, &mPpiList[0]);

  return EFI_SUCCESS;
}

/** @file
  SEV Secret boot time HOB placement

  Copyright (C) 2020 James Bottomley, IBM Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Base.h>
#include <PiPei.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

EFI_STATUS
EFIAPI
InitializeSecretPei (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  BuildMemoryAllocationHob (
    PcdGet32 (PcdSevLaunchSecretBase),
    ALIGN_VALUE (PcdGet32 (PcdSevLaunchSecretSize), EFI_PAGE_SIZE),
    EfiReservedMemoryType
    );

  return EFI_SUCCESS;
}

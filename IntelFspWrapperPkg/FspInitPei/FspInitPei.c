/** @file
  This PEIM initialize FSP.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "FspInitPei.h"

/**
  This is the entrypoint of PEIM

  @param[in] FileHandle  Handle of the file being invoked.
  @param[in] PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
FspPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  FSP_INFO_HEADER      *FspHeader;
  UINT8                PcdFspApiVersion;

  DEBUG ((DEBUG_INFO, "FspPeiEntryPoint\n"));
  PcdFspApiVersion = 1;

  FspHeader = FspFindFspHeader (PcdGet32 (PcdFlashFvFspBase));
  DEBUG ((DEBUG_INFO, "FspHeader - 0x%x\n", FspHeader));
  if (FspHeader == NULL) {
    return EFI_DEVICE_ERROR;
  }

  ASSERT (FspHeader->TempRamInitEntryOffset != 0);
  ASSERT (FspHeader->FspInitEntryOffset != 0);
  ASSERT (FspHeader->NotifyPhaseEntryOffset != 0);

  if ((PcdGet8 (PcdFspApiVersion) >= 2) &&
      (FspHeader->HeaderRevision >= FSP_HEADER_REVISION_2) &&
      (FspHeader->ApiEntryNum >= 6) ) {
    ASSERT (FspHeader->FspMemoryInitEntryOffset != 0);
    ASSERT (FspHeader->TempRamExitEntryOffset != 0);
    ASSERT (FspHeader->FspSiliconInitEntryOffset != 0);
    PcdFspApiVersion = PcdGet8 (PcdFspApiVersion);
  }
  DEBUG ((DEBUG_INFO, "PcdFspApiVersion - 0x%x\n", PcdFspApiVersion));

  if (PcdFspApiVersion == 1) {
    PeiFspInitV1 (FspHeader);
  } else {
    PeiFspInitV2 (FspHeader);
  }

  return EFI_SUCCESS;
}

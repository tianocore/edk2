/** @file
  Dispatch Block to Aps in Pei phase for parallelhash algorithm.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CryptParallelHash.h"
#include <Library/PeiServicesTablePointerLib.h>
#include <PiPei.h>
#include <Ppi/MpServices.h>
#include <Library/PeiServicesLib.h>

/**
  Dispatch the block task to each AP in PEI phase.

**/
VOID
EFIAPI
DispatchBlockToAp (
  VOID
  )
{
  EFI_STATUS               Status;
  CONST EFI_PEI_SERVICES   **PeiServices;
  EFI_PEI_MP_SERVICES_PPI  *MpServicesPpi;

  PeiServices = GetPeiServicesTablePointer ();
  Status      = (*PeiServices)->LocatePpi (
                                  PeiServices,
                                  &gEfiPeiMpServicesPpiGuid,
                                  0,
                                  NULL,
                                  (VOID **)&MpServicesPpi
                                  );
  if (EFI_ERROR (Status)) {
    //
    // Failed to locate MpServices Ppi, do parallel hash by one core.
    //
    DEBUG ((DEBUG_ERROR, "[DispatchBlockToApPei] Failed to locate MpServices Ppi. Status = %r\n", Status));
    return;
  }

  Status = MpServicesPpi->StartupAllAPs (
                            (CONST EFI_PEI_SERVICES **)PeiServices,
                            MpServicesPpi,
                            ParallelHashApExecute,
                            FALSE,
                            0,
                            NULL
                            );
  return;
}

/** @file
  Dispatch Block to Aps in Dxe phase for parallelhash algorithm.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CryptParallelHash.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/MpService.h>

/**
  Dispatch the block task to each AP in PEI phase.

**/
VOID
EFIAPI
DispatchBlockToAp (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_MP_SERVICES_PROTOCOL  *MpServices;

  Status = gBS->LocateProtocol (
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  (VOID **)&MpServices
                  );
  if (EFI_ERROR (Status)) {
    //
    // Failed to locate MpServices Protocol, do parallel hash by one core.
    //
    DEBUG ((DEBUG_ERROR, "[DispatchBlockToApDxe] Failed to locate MpServices Protocol. Status = %r\n", Status));
    return;
  }

  Status = MpServices->StartupAllAPs (
                         MpServices,
                         ParallelHashApExecute,
                         FALSE,
                         NULL,
                         0,
                         NULL,
                         NULL
                         );
  return;
}

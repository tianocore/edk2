/**@file
  Install a callback to clear cache on all processors.
  This is for conformance with the TCG "Platform Reset Attack Mitigation
  Specification". Because clearing the CPU caches at boot doesn't impact
  performance significantly, do it unconditionally, for simplicity's
  sake.

  Copyright (C) 2018, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/MpServices.h>

#include "Platform.h"

/**
  Invalidate data & instruction caches.
  All APs execute this function in parallel. The BSP executes the function
  separately.

  @param[in,out] WorkSpace  Pointer to the input/output argument workspace
                            shared by all processors.
**/
STATIC
VOID
EFIAPI
ClearCache (
  IN OUT VOID *WorkSpace
  )
{
  WriteBackInvalidateDataCache ();
  InvalidateInstructionCache ();
}

/**
  Notification function called when EFI_PEI_MP_SERVICES_PPI becomes available.

  @param[in] PeiServices      Indirect reference to the PEI Services Table.
  @param[in] NotifyDescriptor Address of the notification descriptor data
                              structure.
  @param[in] Ppi              Address of the PPI that was installed.

  @return  Status of the notification. The status code returned from this
           function is ignored.
**/
STATIC
EFI_STATUS
EFIAPI
ClearCacheOnMpServicesAvailable (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_PEI_MP_SERVICES_PPI *MpServices;
  EFI_STATUS              Status;

  DEBUG ((DEBUG_INFO, "%a: %a\n", gEfiCallerBaseName, __FUNCTION__));

  //
  // Clear cache on all the APs in parallel.
  //
  MpServices = Ppi;
  Status = MpServices->StartupAllAPs (
                         (CONST EFI_PEI_SERVICES **)PeiServices,
                         MpServices,
                         ClearCache,          // Procedure
                         FALSE,               // SingleThread
                         0,                   // TimeoutInMicroSeconds: inf.
                         NULL                 // ProcedureArgument
                         );
  if (EFI_ERROR (Status) && Status != EFI_NOT_STARTED) {
    DEBUG ((DEBUG_ERROR, "%a: StartupAllAps(): %r\n", __FUNCTION__, Status));
    return Status;
  }

  //
  // Now clear cache on the BSP too.
  //
  ClearCache (NULL);
  return EFI_SUCCESS;
}

//
// Notification object for registering the callback, for when
// EFI_PEI_MP_SERVICES_PPI becomes available.
//
STATIC CONST EFI_PEI_NOTIFY_DESCRIPTOR mMpServicesNotify = {
  EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | // Flags
  EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEfiPeiMpServicesPpiGuid,               // Guid
  ClearCacheOnMpServicesAvailable          // Notify
};

VOID
InstallClearCacheCallback (
  VOID
  )
{
  EFI_STATUS           Status;

  Status = PeiServicesNotifyPpi (&mMpServicesNotify);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to set up MP Services callback: %r\n",
      __FUNCTION__, Status));
  }
}

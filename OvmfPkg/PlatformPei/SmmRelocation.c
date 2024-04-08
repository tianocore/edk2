/**@file
  Install a callback to do smm relocation.

  Copyright (c) 2024, Intel Corporation. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/SmmRelocationLib.h>
#include <Ppi/MpServices2.h>
#include "Platform.h"

/**
  Notification function called when EDKII_PEI_MP_SERVICES2_PPI becomes available.

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
OnMpServices2Available (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EDKII_PEI_MP_SERVICES2_PPI  *MpServices2;
  EFI_STATUS                  Status;

  DEBUG ((DEBUG_INFO, "%a: %a\n", gEfiCallerBaseName, __func__));

  MpServices2 = Ppi;

  //
  // Smm Relocation Initialize.
  //
  Status = SmmRelocationInit (MpServices2);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OnMpServices2Available: Not able to execute Smm Relocation Init.  Status: %r\n", Status));
  }

  return EFI_SUCCESS;
}

//
// Notification object for registering the callback, for when
// EDKII_PEI_MP_SERVICES2_PPI becomes available.
//
STATIC CONST EFI_PEI_NOTIFY_DESCRIPTOR  mMpServices2Notify = {
  EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK |   // Flags
  EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEdkiiPeiMpServices2PpiGuid,              // Guid
  OnMpServices2Available                     // Notify
};

VOID
RelocateSmBase (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = PeiServicesNotifyPpi (&mMpServices2Notify);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: failed to set up MP Services2 callback: %r\n",
      __func__,
      Status
      ));
  }
}

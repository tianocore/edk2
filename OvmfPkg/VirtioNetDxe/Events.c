/** @file

  Implements
  - the SNM.WaitForPacket EVT_NOTIFY_WAIT event,
  - the EVT_SIGNAL_EXIT_BOOT_SERVICES event
  for the virtio-net driver.

  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "VirtioNet.h"

/**
  Invoke a notification event

  @param  Event                 Event whose notification function is being
                                invoked.
  @param  Context               The pointer to the notification function's
                                context, which is implementation-dependent.

**/

VOID
EFIAPI
VirtioNetIsPacketAvailable (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
{
  //
  // This callback has been enqueued by an external application and is
  // running at TPL_CALLBACK already.
  //
  // The WaitForPacket logic is similar to that of WaitForKey. The former has
  // almost no documentation in either the UEFI-2.3.1+errC spec or the
  // DWG-2.3.1, but WaitForKey does have some.
  //
  VNET_DEV *Dev;
  UINT16   RxCurUsed;

  Dev = Context;
  if (Dev->Snm.State != EfiSimpleNetworkInitialized) {
    return;
  }

  //
  // virtio-0.9.5, 2.4.2 Receiving Used Buffers From the Device
  //
  MemoryFence ();
  RxCurUsed = *Dev->RxRing.Used.Idx;
  MemoryFence ();

  if (Dev->RxLastUsed != RxCurUsed) {
    gBS->SignalEvent (Dev->Snp.WaitForPacket);
  }
}

VOID
EFIAPI
VirtioNetExitBoot (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
{
  //
  // This callback has been enqueued by ExitBootServices() and is running at
  // TPL_CALLBACK already.
  //
  // Shut down pending transfers according to DWG-2.3.1, "25.5.1 Exit Boot
  // Services Event".
  //
  VNET_DEV *Dev;

  DEBUG ((DEBUG_VERBOSE, "%a: Context=0x%p\n", __FUNCTION__, Context));
  Dev = Context;
  if (Dev->Snm.State == EfiSimpleNetworkInitialized) {
    Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, 0);
  }
}

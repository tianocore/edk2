/** @file

  Helper functions used by at least two Simple Network Protocol methods.

  Copyright (C) 2013, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/MemoryAllocationLib.h>

#include "VirtioNet.h"

/**
  Release RX and TX resources on the boundary of the
  EfiSimpleNetworkInitialized state.

  These functions contribute to rolling back a partial, failed initialization
  of the virtio-net SNP driver instance, or to shutting down a fully
  initialized, running instance.

  They are only callable by the VirtioNetInitialize() and the
  VirtioNetShutdown() SNP methods. See the state diagram in "VirtioNet.h".

  @param[in,out] Dev  The VNET_DEV driver instance being shut down, or whose
                      partial, failed initialization is being rolled back.
*/

VOID
EFIAPI
VirtioNetShutdownRx (
  IN OUT VNET_DEV *Dev
  )
{
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Dev->RxBufMap);
  Dev->VirtIo->FreeSharedPages (
                 Dev->VirtIo,
                 Dev->RxBufNrPages,
                 Dev->RxBuf
                 );
}


VOID
EFIAPI
VirtioNetShutdownTx (
  IN OUT VNET_DEV *Dev
  )
{
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Dev->TxSharedReqMap);
  Dev->VirtIo->FreeSharedPages (
                 Dev->VirtIo,
                 EFI_SIZE_TO_PAGES (sizeof *(Dev->TxSharedReq)),
                 Dev->TxSharedReq
                 );

  FreePool (Dev->TxFreeStack);
}

/**
  Release TX and RX VRING resources.

  @param[in,out] Dev       The VNET_DEV driver instance which was using
                           the ring.
  @param[in,out] Ring      The virtio ring to clean up.
  @param[in]     RingMap   A token return from the VirtioRingMap()
*/
VOID
EFIAPI
VirtioNetUninitRing (
  IN OUT VNET_DEV *Dev,
  IN OUT VRING    *Ring,
  IN     VOID     *RingMap
  )
{
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, RingMap);
  VirtioRingUninit (Dev->VirtIo, Ring);
}

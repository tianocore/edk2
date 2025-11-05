/** @file

  Implementation of the SNP.GetStatus() function and its private helpers if
  any.

  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "VirtioNet.h"

/**
  Reads the current interrupt status and recycled transmit buffer status from
  a network interface.

  @param  This            The protocol instance pointer.
  @param  InterruptStatus A pointer to the bit mask of the currently active
                          interrupts If this is NULL, the interrupt status will
                          not be read from the device. If this is not NULL, the
                          interrupt status will be read from the device. When
                          the  interrupt status is read, it will also be
                          cleared. Clearing the transmit  interrupt does not
                          empty the recycled transmit buffer array.
  @param  TxBuf           Recycled transmit buffer address. The network
                          interface will not transmit if its internal recycled
                          transmit buffer array is full. Reading the transmit
                          buffer does not clear the transmit interrupt. If this
                          is NULL, then the transmit buffer status will not be
                          read. If there are no transmit buffers to recycle and
                          TxBuf is not NULL, * TxBuf will be set to NULL.

  @retval EFI_SUCCESS           The status of the network interface was
                                retrieved.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an
                                unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network
                                interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network
                                interface.

**/
EFI_STATUS
EFIAPI
VirtioNetGetStatus (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  OUT UINT32                      *InterruptStatus OPTIONAL,
  OUT VOID                        **TxBuf OPTIONAL
  )
{
  VNET_DEV              *Dev;
  EFI_TPL               OldTpl;
  EFI_STATUS            Status;
  UINT16                RxCurUsed;
  UINT16                TxCurUsed;
  EFI_PHYSICAL_ADDRESS  DeviceAddress;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev    = VIRTIO_NET_FROM_SNP (This);
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  switch (Dev->Snm.State) {
    case EfiSimpleNetworkStopped:
      Status = EFI_NOT_STARTED;
      goto Exit;
    case EfiSimpleNetworkStarted:
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    default:
      break;
  }

  //
  // update link status
  //
  if (Dev->Snm.MediaPresentSupported) {
    UINT16  LinkStatus;

    Status = VIRTIO_CFG_READ (Dev, LinkStatus, &LinkStatus);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Dev->Snm.MediaPresent =
      (BOOLEAN)((LinkStatus & VIRTIO_NET_S_LINK_UP) != 0);
  }

  //
  // virtio-0.9.5, 2.4.2 Receiving Used Buffers From the Device
  //
  MemoryFence ();
  RxCurUsed = *Dev->RxRing.Used.Idx;
  TxCurUsed = *Dev->TxRing.Used.Idx;
  MemoryFence ();

  if (InterruptStatus != NULL) {
    //
    // report the receive interrupt if there is data available for reception,
    // report the transmit interrupt if we have transmitted at least one buffer
    //
    *InterruptStatus = 0;
    if (Dev->RxLastUsed != RxCurUsed) {
      *InterruptStatus |= EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT;
    }

    if (Dev->TxLastUsed != TxCurUsed) {
      ASSERT (Dev->TxCurPending > 0);
      *InterruptStatus |= EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;
    }
  }

  if (TxBuf != NULL) {
    if (Dev->TxLastUsed == TxCurUsed) {
      *TxBuf = NULL;
    } else {
      UINT16  UsedElemIdx;
      UINT32  DescIdx;

      //
      // fetch the first descriptor among those that the hypervisor reports
      // completed
      //
      ASSERT (Dev->TxCurPending > 0);
      ASSERT (Dev->TxCurPending <= Dev->TxMaxPending);

      UsedElemIdx = Dev->TxLastUsed++ % Dev->TxRing.QueueSize;
      DescIdx     = Dev->TxRing.Used.UsedElem[UsedElemIdx].Id;
      ASSERT (DescIdx < (UINT32)(2 * Dev->TxMaxPending - 1));

      //
      // get the device address that has been enqueued for the caller's
      // transmit buffer
      //
      DeviceAddress = Dev->TxRing.Desc[DescIdx + 1].Addr;

      //
      // now this descriptor can be used again to enqueue a transmit buffer
      //
      Dev->TxFreeStack[--Dev->TxCurPending] = (UINT16)DescIdx;

      //
      // Unmap the device address and perform the reverse mapping to find the
      // caller buffer address.
      //
      Status = VirtioNetUnmapTxBuf (
                 Dev,
                 TxBuf,
                 DeviceAddress
                 );
      if (EFI_ERROR (Status)) {
        //
        // VirtioNetUnmapTxBuf should never fail, if we have reached here
        // that means our internal state has been corrupted
        //
        ASSERT (FALSE);
        Status = EFI_DEVICE_ERROR;
        goto Exit;
      }
    }
  }

  Status = EFI_SUCCESS;

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

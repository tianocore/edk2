/** @file

  Implementation of the SNP.Initialize() function and its private helpers if
  any.

  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, AMD Inc, All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "VirtioNet.h"

/**
  Initialize a virtio ring for a specific transfer direction of the virtio-net
  device.

  This function may only be called by VirtioNetInitialize().

  @param[in,out] Dev       The VNET_DEV driver instance about to enter the
                           EfiSimpleNetworkInitialized state.
  @param[in]     Selector  Identifies the transfer direction (virtio queue) of
                           the network device.
  @param[out]    Ring      The virtio-ring inside the VNET_DEV structure,
                           corresponding to Selector.
  @param[out]    Mapping   A resulting token to pass to VirtioNetUninitRing()

  @retval EFI_UNSUPPORTED  The queue size reported by the virtio-net device is
                           too small.
  @return                  Status codes from VIRTIO_CFG_WRITE(),
                           VIRTIO_CFG_READ(), VirtioRingInit() and
                           VirtioRingMap().
  @retval EFI_SUCCESS      Ring initialized.
*/
STATIC
EFI_STATUS
EFIAPI
VirtioNetInitRing (
  IN OUT VNET_DEV  *Dev,
  IN     UINT16    Selector,
  OUT    VRING     *Ring,
  OUT    VOID      **Mapping
  )
{
  EFI_STATUS  Status;
  UINT16      QueueSize;
  UINT64      RingBaseShift;
  VOID        *MapInfo;

  //
  // step 4b -- allocate selected queue
  //
  Status = Dev->VirtIo->SetQueueSel (Dev->VirtIo, Selector);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Dev->VirtIo->GetQueueNumMax (Dev->VirtIo, &QueueSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // For each packet (RX and TX alike), we need two descriptors:
  // one for the virtio-net request header, and another one for the data
  //
  if (QueueSize < 2) {
    return EFI_UNSUPPORTED;
  }

  Status = VirtioRingInit (Dev->VirtIo, QueueSize, Ring);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // If anything fails from here on, we must release the ring resources.
  //
  Status = VirtioRingMap (Dev->VirtIo, Ring, &RingBaseShift, &MapInfo);
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }

  //
  // Additional steps for MMIO: align the queue appropriately, and set the
  // size. If anything fails from here on, we must unmap the ring resources.
  //
  Status = Dev->VirtIo->SetQueueNum (Dev->VirtIo, QueueSize);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  Status = Dev->VirtIo->SetQueueAlign (Dev->VirtIo, EFI_PAGE_SIZE);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  //
  // step 4c -- report GPFN (guest-physical frame number) of queue
  //
  Status = Dev->VirtIo->SetQueueAddress (Dev->VirtIo, Ring, RingBaseShift);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  *Mapping = MapInfo;

  return EFI_SUCCESS;

UnmapQueue:
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, MapInfo);

ReleaseQueue:
  VirtioRingUninit (Dev->VirtIo, Ring);

  return Status;
}

/**
  Set up static scaffolding for the VirtioNetTransmit() and
  VirtioNetGetStatus() SNP methods.

  This function may only be called by VirtioNetInitialize().

  The structures laid out and resources configured include:
  - fully populate the TX queue with a static pattern of virtio descriptor
    chains,
  - tracking of heads of free descriptor chains from the above,
  - one common virtio-net request header (never modified by the host) for all
    pending TX packets,
  - select polling over TX interrupt.

  @param[in,out] Dev       The VNET_DEV driver instance about to enter the
                           EfiSimpleNetworkInitialized state.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the stack to track the heads
                                of free descriptor chains or failed to init
                                TxBufCollection.
  @return                       Status codes from VIRTIO_DEVICE_PROTOCOL.
                                AllocateSharedPages() or
                                VirtioMapAllBytesInSharedBuffer()
  @retval EFI_SUCCESS           TX setup successful.
*/
STATIC
EFI_STATUS
EFIAPI
VirtioNetInitTx (
  IN OUT VNET_DEV  *Dev
  )
{
  UINTN                 TxSharedReqSize;
  UINTN                 PktIdx;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  DeviceAddress;
  VOID                  *TxSharedReqBuffer;

  Dev->TxMaxPending = (UINT16)MIN (
                                Dev->TxRing.QueueSize / 2,
                                VNET_MAX_PENDING
                                );
  Dev->TxCurPending = 0;
  Dev->TxFreeStack  = AllocatePool (
                        Dev->TxMaxPending *
                        sizeof *Dev->TxFreeStack
                        );
  if (Dev->TxFreeStack == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Dev->TxBufCollection = OrderedCollectionInit (
                           VirtioNetTxBufMapInfoCompare,
                           VirtioNetTxBufDeviceAddressCompare
                           );
  if (Dev->TxBufCollection == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeTxFreeStack;
  }

  //
  // Allocate TxSharedReq header and map with BusMasterCommonBuffer so that it
  // can be accessed equally by both processor and device.
  //
  Status = Dev->VirtIo->AllocateSharedPages (
                          Dev->VirtIo,
                          EFI_SIZE_TO_PAGES (sizeof *Dev->TxSharedReq),
                          &TxSharedReqBuffer
                          );
  if (EFI_ERROR (Status)) {
    goto UninitTxBufCollection;
  }

  ZeroMem (TxSharedReqBuffer, sizeof *Dev->TxSharedReq);

  Status = VirtioMapAllBytesInSharedBuffer (
             Dev->VirtIo,
             VirtioOperationBusMasterCommonBuffer,
             TxSharedReqBuffer,
             sizeof *(Dev->TxSharedReq),
             &DeviceAddress,
             &Dev->TxSharedReqMap
             );
  if (EFI_ERROR (Status)) {
    goto FreeTxSharedReqBuffer;
  }

  Dev->TxSharedReq = TxSharedReqBuffer;

  //
  // In VirtIo 1.0, the NumBuffers field is mandatory. In 0.9.5, it depends on
  // VIRTIO_NET_F_MRG_RXBUF, which we never negotiate.
  //
  TxSharedReqSize = (Dev->VirtIo->Revision < VIRTIO_SPEC_REVISION (1, 0, 0)) ?
                    sizeof (Dev->TxSharedReq->V0_9_5) :
                    sizeof *Dev->TxSharedReq;

  for (PktIdx = 0; PktIdx < Dev->TxMaxPending; ++PktIdx) {
    UINT16  DescIdx;

    DescIdx                  = (UINT16)(2 * PktIdx);
    Dev->TxFreeStack[PktIdx] = DescIdx;

    //
    // For each possibly pending packet, lay out the descriptor for the common
    // (unmodified by the host) virtio-net request header.
    //
    Dev->TxRing.Desc[DescIdx].Addr  = DeviceAddress;
    Dev->TxRing.Desc[DescIdx].Len   = (UINT32)TxSharedReqSize;
    Dev->TxRing.Desc[DescIdx].Flags = VRING_DESC_F_NEXT;
    Dev->TxRing.Desc[DescIdx].Next  = (UINT16)(DescIdx + 1);

    //
    // The second descriptor of each pending TX packet is updated on the fly,
    // but it always terminates the descriptor chain of the packet.
    //
    Dev->TxRing.Desc[DescIdx + 1].Flags = 0;
  }

  //
  // virtio-0.9.5, Appendix C, Packet Transmission
  //
  Dev->TxSharedReq->V0_9_5.Flags   = 0;
  Dev->TxSharedReq->V0_9_5.GsoType = VIRTIO_NET_HDR_GSO_NONE;

  //
  // For VirtIo 1.0 only -- the field exists, but it is unused
  //
  Dev->TxSharedReq->NumBuffers = 0;

  //
  // virtio-0.9.5, 2.4.2 Receiving Used Buffers From the Device
  //
  MemoryFence ();
  Dev->TxLastUsed = *Dev->TxRing.Used.Idx;
  ASSERT (Dev->TxLastUsed == 0);

  //
  // want no interrupt when a transmit completes
  //
  *Dev->TxRing.Avail.Flags = (UINT16)VRING_AVAIL_F_NO_INTERRUPT;

  return EFI_SUCCESS;

FreeTxSharedReqBuffer:
  Dev->VirtIo->FreeSharedPages (
                 Dev->VirtIo,
                 EFI_SIZE_TO_PAGES (sizeof *(Dev->TxSharedReq)),
                 TxSharedReqBuffer
                 );

UninitTxBufCollection:
  OrderedCollectionUninit (Dev->TxBufCollection);

FreeTxFreeStack:
  FreePool (Dev->TxFreeStack);

  return Status;
}

/**
  Set up static scaffolding for the VirtioNetReceive() SNP method and enable
  live device operation.

  This function may only be called as VirtioNetInitialize()'s final step.

  The structures laid out and resources configured include:
  - destination area for the host to write virtio-net request headers and
    packet data into,
  - select polling over RX interrupt,
  - fully populate the RX queue with a static pattern of virtio descriptor
    chains.

  @param[in,out] Dev       The VNET_DEV driver instance about to enter the
                           EfiSimpleNetworkInitialized state.

  @return                       Status codes from VIRTIO_CFG_WRITE() or
                                VIRTIO_DEVICE_PROTOCOL.AllocateSharedPages or
                                VirtioMapAllBytesInSharedBuffer().
  @retval EFI_SUCCESS           RX setup successful. The device is live and may
                                already be writing to the receive area.
*/
STATIC
EFI_STATUS
EFIAPI
VirtioNetInitRx (
  IN OUT VNET_DEV  *Dev
  )
{
  EFI_STATUS            Status;
  UINTN                 VirtioNetReqSize;
  UINTN                 RxBufSize;
  UINT16                RxAlwaysPending;
  UINTN                 PktIdx;
  UINT16                DescIdx;
  UINTN                 NumBytes;
  EFI_PHYSICAL_ADDRESS  RxBufDeviceAddress;
  VOID                  *RxBuffer;

  //
  // In VirtIo 1.0, the NumBuffers field is mandatory. In 0.9.5, it depends on
  // VIRTIO_NET_F_MRG_RXBUF, which we never negotiate.
  //
  VirtioNetReqSize = (Dev->VirtIo->Revision < VIRTIO_SPEC_REVISION (1, 0, 0)) ?
                     sizeof (VIRTIO_NET_REQ) :
                     sizeof (VIRTIO_1_0_NET_REQ);

  //
  // For each incoming packet we must supply two descriptors:
  // - the recipient for the virtio-net request header, plus
  // - the recipient for the network data (which consists of Ethernet header
  //   and Ethernet payload).
  //
  RxBufSize = VirtioNetReqSize +
              (Dev->Snm.MediaHeaderSize + Dev->Snm.MaxPacketSize);

  //
  // Limit the number of pending RX packets if the queue is big. The division
  // by two is due to the above "two descriptors per packet" trait.
  //
  RxAlwaysPending = (UINT16)MIN (Dev->RxRing.QueueSize / 2, VNET_MAX_PENDING);

  //
  // The RxBuf is shared between guest and hypervisor, use
  // AllocateSharedPages() to allocate this memory region and map it with
  // BusMasterCommonBuffer so that it can be accessed by both guest and
  // hypervisor.
  //
  NumBytes          = RxAlwaysPending * RxBufSize;
  Dev->RxBufNrPages = EFI_SIZE_TO_PAGES (NumBytes);
  Status            = Dev->VirtIo->AllocateSharedPages (
                                     Dev->VirtIo,
                                     Dev->RxBufNrPages,
                                     &RxBuffer
                                     );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (RxBuffer, NumBytes);

  Status = VirtioMapAllBytesInSharedBuffer (
             Dev->VirtIo,
             VirtioOperationBusMasterCommonBuffer,
             RxBuffer,
             NumBytes,
             &Dev->RxBufDeviceBase,
             &Dev->RxBufMap
             );
  if (EFI_ERROR (Status)) {
    goto FreeSharedBuffer;
  }

  Dev->RxBuf = RxBuffer;

  //
  // virtio-0.9.5, 2.4.2 Receiving Used Buffers From the Device
  //
  MemoryFence ();
  Dev->RxLastUsed = *Dev->RxRing.Used.Idx;
  ASSERT (Dev->RxLastUsed == 0);

  //
  // virtio-0.9.5, 2.4.2 Receiving Used Buffers From the Device:
  // the host should not send interrupts, we'll poll in VirtioNetReceive()
  // and VirtioNetIsPacketAvailable().
  //
  *Dev->RxRing.Avail.Flags = (UINT16)VRING_AVAIL_F_NO_INTERRUPT;

  //
  // now set up a separate, two-part descriptor chain for each RX packet, and
  // link each chain into (from) the available ring as well
  //
  DescIdx            = 0;
  RxBufDeviceAddress = Dev->RxBufDeviceBase;
  for (PktIdx = 0; PktIdx < RxAlwaysPending; ++PktIdx) {
    //
    // virtio-0.9.5, 2.4.1.2 Updating the Available Ring
    // invisible to the host until we update the Index Field
    //
    Dev->RxRing.Avail.Ring[PktIdx] = DescIdx;

    //
    // virtio-0.9.5, 2.4.1.1 Placing Buffers into the Descriptor Table
    //
    Dev->RxRing.Desc[DescIdx].Addr  = RxBufDeviceAddress;
    Dev->RxRing.Desc[DescIdx].Len   = (UINT32)VirtioNetReqSize;
    Dev->RxRing.Desc[DescIdx].Flags = VRING_DESC_F_WRITE | VRING_DESC_F_NEXT;
    Dev->RxRing.Desc[DescIdx].Next  = (UINT16)(DescIdx + 1);
    RxBufDeviceAddress             += Dev->RxRing.Desc[DescIdx++].Len;

    Dev->RxRing.Desc[DescIdx].Addr  = RxBufDeviceAddress;
    Dev->RxRing.Desc[DescIdx].Len   = (UINT32)(RxBufSize - VirtioNetReqSize);
    Dev->RxRing.Desc[DescIdx].Flags = VRING_DESC_F_WRITE;
    RxBufDeviceAddress             += Dev->RxRing.Desc[DescIdx++].Len;
  }

  //
  // virtio-0.9.5, 2.4.1.3 Updating the Index Field
  //
  MemoryFence ();
  *Dev->RxRing.Avail.Idx = RxAlwaysPending;

  //
  // At this point reception may already be running. In order to make it sure,
  // kick the hypervisor. If we fail to kick it, we must first abort reception
  // before tearing down anything, because reception may have been already
  // running even without the kick.
  //
  // virtio-0.9.5, 2.4.1.4 Notifying the Device
  //
  MemoryFence ();
  Status = Dev->VirtIo->SetQueueNotify (Dev->VirtIo, VIRTIO_NET_Q_RX);
  if (EFI_ERROR (Status)) {
    Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, 0);
    goto UnmapSharedBuffer;
  }

  return Status;

UnmapSharedBuffer:
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Dev->RxBufMap);

FreeSharedBuffer:
  Dev->VirtIo->FreeSharedPages (
                 Dev->VirtIo,
                 Dev->RxBufNrPages,
                 RxBuffer
                 );
  return Status;
}

/**
  Resets a network adapter and allocates the transmit and receive buffers
  required by the network interface; optionally, also requests allocation  of
  additional transmit and receive buffers.

  @param  This              The protocol instance pointer.
  @param  ExtraRxBufferSize The size, in bytes, of the extra receive buffer
                            space that the driver should allocate for the
                            network interface. Some network interfaces will not
                            be able to use the extra buffer, and the caller
                            will not know if it is actually being used.
  @param  ExtraTxBufferSize The size, in bytes, of the extra transmit buffer
                            space that the driver should allocate for the
                            network interface. Some network interfaces will not
                            be able to use the extra buffer, and the caller
                            will not know if it is actually being used.

  @retval EFI_SUCCESS           The network interface was initialized.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory for the transmit
                                and receive buffers.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an
                                unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network
                                interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network
                                interface.

**/
EFI_STATUS
EFIAPI
VirtioNetInitialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN UINTN                        ExtraRxBufferSize  OPTIONAL,
  IN UINTN                        ExtraTxBufferSize  OPTIONAL
  )
{
  VNET_DEV    *Dev;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;
  UINT8       NextDevStat;
  UINT64      Features;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((ExtraRxBufferSize > 0) || (ExtraTxBufferSize > 0)) {
    return EFI_UNSUPPORTED;
  }

  Dev    = VIRTIO_NET_FROM_SNP (This);
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  if (Dev->Snm.State != EfiSimpleNetworkStarted) {
    Status = EFI_NOT_STARTED;
    goto InitFailed;
  }

  //
  // In the EfiSimpleNetworkStarted state the virtio-net device has status
  // value 0 (= reset) -- see the state diagram, the full call chain to
  // the end of VirtioNetGetFeatures() (considering we're here now),
  // the DeviceFailed label below, and VirtioNetShutdown().
  //
  // Accordingly, the below is a subsequence of the steps found in the
  // virtio-0.9.5 spec, 2.2.1 Device Initialization Sequence.
  //
  NextDevStat = VSTAT_ACK;    // step 2 -- acknowledge device presence
  Status      = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto InitFailed;
  }

  NextDevStat |= VSTAT_DRIVER; // step 3 -- we know how to drive it
  Status       = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto DeviceFailed;
  }

  //
  // Set Page Size - MMIO VirtIo Specific
  //
  Status = Dev->VirtIo->SetPageSize (Dev->VirtIo, EFI_PAGE_SIZE);
  if (EFI_ERROR (Status)) {
    goto DeviceFailed;
  }

  //
  // step 4a -- retrieve features. Note that we're past validating required
  // features in VirtioNetGetFeatures().
  //
  Status = Dev->VirtIo->GetDeviceFeatures (Dev->VirtIo, &Features);
  if (EFI_ERROR (Status)) {
    goto DeviceFailed;
  }

  ASSERT (Features & VIRTIO_NET_F_MAC);
  ASSERT (
    Dev->Snm.MediaPresentSupported ==
    !!(Features & VIRTIO_NET_F_STATUS)
    );

  Features &= VIRTIO_NET_F_MAC | VIRTIO_NET_F_STATUS | VIRTIO_F_VERSION_1 |
              VIRTIO_F_IOMMU_PLATFORM;

  //
  // In virtio-1.0, feature negotiation is expected to complete before queue
  // discovery, and the device can also reject the selected set of features.
  //
  if (Dev->VirtIo->Revision >= VIRTIO_SPEC_REVISION (1, 0, 0)) {
    Status = Virtio10WriteFeatures (Dev->VirtIo, Features, &NextDevStat);
    if (EFI_ERROR (Status)) {
      goto DeviceFailed;
    }
  }

  //
  // step 4b, 4c -- allocate and report virtqueues
  //
  Status = VirtioNetInitRing (
             Dev,
             VIRTIO_NET_Q_RX,
             &Dev->RxRing,
             &Dev->RxRingMap
             );
  if (EFI_ERROR (Status)) {
    goto DeviceFailed;
  }

  Status = VirtioNetInitRing (
             Dev,
             VIRTIO_NET_Q_TX,
             &Dev->TxRing,
             &Dev->TxRingMap
             );
  if (EFI_ERROR (Status)) {
    goto ReleaseRxRing;
  }

  //
  // step 5 -- keep only the features we want
  //
  if (Dev->VirtIo->Revision < VIRTIO_SPEC_REVISION (1, 0, 0)) {
    Features &= ~(UINT64)(VIRTIO_F_VERSION_1 | VIRTIO_F_IOMMU_PLATFORM);
    Status    = Dev->VirtIo->SetGuestFeatures (Dev->VirtIo, Features);
    if (EFI_ERROR (Status)) {
      goto ReleaseTxRing;
    }
  }

  //
  // step 6 -- virtio-net initialization complete
  //
  NextDevStat |= VSTAT_DRIVER_OK;
  Status       = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto ReleaseTxRing;
  }

  Status = VirtioNetInitTx (Dev);
  if (EFI_ERROR (Status)) {
    goto AbortDevice;
  }

  //
  // start receiving
  //
  Status = VirtioNetInitRx (Dev);
  if (EFI_ERROR (Status)) {
    goto ReleaseTxAux;
  }

  Dev->Snm.State = EfiSimpleNetworkInitialized;
  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;

ReleaseTxAux:
  VirtioNetShutdownTx (Dev);

AbortDevice:
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, 0);

ReleaseTxRing:
  VirtioNetUninitRing (Dev, &Dev->TxRing, Dev->TxRingMap);

ReleaseRxRing:
  VirtioNetUninitRing (Dev, &Dev->RxRing, Dev->RxRingMap);

DeviceFailed:
  //
  // restore device status invariant for the EfiSimpleNetworkStarted state
  //
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, 0);

InitFailed:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

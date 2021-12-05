/** @file

  Utility functions used by virtio device drivers.

  Copyright (C) 2012-2016, Red Hat, Inc.
  Portion of Copyright (C) 2013, ARM Ltd.
  Copyright (C) 2017, AMD Inc, All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/VirtioLib.h>

/**

  Configure a virtio ring.

  This function sets up internal storage (the guest-host communication area)
  and lays out several "navigation" (ie. no-ownership) pointers to parts of
  that storage.

  Relevant sections from the virtio-0.9.5 spec:
  - 1.1 Virtqueues,
  - 2.3 Virtqueue Configuration.

  @param[in]  VirtIo            The virtio device which will use the ring.

  @param[in]                    The number of descriptors to allocate for the
                                virtio ring, as requested by the host.

  @param[out] Ring              The virtio ring to set up.

  @return                       Status codes propagated from
                                VirtIo->AllocateSharedPages().

  @retval EFI_SUCCESS           Allocation and setup successful. Ring->Base
                                (and nothing else) is responsible for
                                deallocation.

**/
EFI_STATUS
EFIAPI
VirtioRingInit (
  IN  VIRTIO_DEVICE_PROTOCOL  *VirtIo,
  IN  UINT16                  QueueSize,
  OUT VRING                   *Ring
  )
{
  EFI_STATUS      Status;
  UINTN           RingSize;
  volatile UINT8  *RingPagesPtr;

  RingSize = ALIGN_VALUE (
               sizeof *Ring->Desc            * QueueSize +
               sizeof *Ring->Avail.Flags                 +
               sizeof *Ring->Avail.Idx                   +
               sizeof *Ring->Avail.Ring      * QueueSize +
               sizeof *Ring->Avail.UsedEvent,
               EFI_PAGE_SIZE
               );

  RingSize += ALIGN_VALUE (
                sizeof *Ring->Used.Flags                  +
                sizeof *Ring->Used.Idx                    +
                sizeof *Ring->Used.UsedElem   * QueueSize +
                sizeof *Ring->Used.AvailEvent,
                EFI_PAGE_SIZE
                );

  //
  // Allocate a shared ring buffer
  //
  Ring->NumPages = EFI_SIZE_TO_PAGES (RingSize);
  Status         = VirtIo->AllocateSharedPages (
                             VirtIo,
                             Ring->NumPages,
                             &Ring->Base
                             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SetMem (Ring->Base, RingSize, 0x00);
  RingPagesPtr = Ring->Base;

  Ring->Desc    = (volatile VOID *)RingPagesPtr;
  RingPagesPtr += sizeof *Ring->Desc * QueueSize;

  Ring->Avail.Flags = (volatile VOID *)RingPagesPtr;
  RingPagesPtr     += sizeof *Ring->Avail.Flags;

  Ring->Avail.Idx = (volatile VOID *)RingPagesPtr;
  RingPagesPtr   += sizeof *Ring->Avail.Idx;

  Ring->Avail.Ring = (volatile VOID *)RingPagesPtr;
  RingPagesPtr    += sizeof *Ring->Avail.Ring * QueueSize;

  Ring->Avail.UsedEvent = (volatile VOID *)RingPagesPtr;
  RingPagesPtr         += sizeof *Ring->Avail.UsedEvent;

  RingPagesPtr = (volatile UINT8 *)Ring->Base +
                 ALIGN_VALUE (
                   RingPagesPtr - (volatile UINT8 *)Ring->Base,
                   EFI_PAGE_SIZE
                   );

  Ring->Used.Flags = (volatile VOID *)RingPagesPtr;
  RingPagesPtr    += sizeof *Ring->Used.Flags;

  Ring->Used.Idx = (volatile VOID *)RingPagesPtr;
  RingPagesPtr  += sizeof *Ring->Used.Idx;

  Ring->Used.UsedElem = (volatile VOID *)RingPagesPtr;
  RingPagesPtr       += sizeof *Ring->Used.UsedElem * QueueSize;

  Ring->Used.AvailEvent = (volatile VOID *)RingPagesPtr;
  RingPagesPtr         += sizeof *Ring->Used.AvailEvent;

  Ring->QueueSize = QueueSize;
  return EFI_SUCCESS;
}

/**

  Tear down the internal resources of a configured virtio ring.

  The caller is responsible to stop the host from using this ring before
  invoking this function: the VSTAT_DRIVER_OK bit must be clear in
  VhdrDeviceStatus.

  @param[in]  VirtIo  The virtio device which was using the ring.

  @param[out] Ring    The virtio ring to clean up.

**/
VOID
EFIAPI
VirtioRingUninit (
  IN     VIRTIO_DEVICE_PROTOCOL  *VirtIo,
  IN OUT VRING                   *Ring
  )
{
  VirtIo->FreeSharedPages (VirtIo, Ring->NumPages, Ring->Base);
  SetMem (Ring, sizeof *Ring, 0x00);
}

/**

  Turn off interrupt notifications from the host, and prepare for appending
  multiple descriptors to the virtio ring.

  The calling driver must be in VSTAT_DRIVER_OK state.

  @param[in,out] Ring  The virtio ring we intend to append descriptors to.

  @param[out] Indices  The DESC_INDICES structure to initialize.

**/
VOID
EFIAPI
VirtioPrepare (
  IN OUT VRING         *Ring,
  OUT    DESC_INDICES  *Indices
  )
{
  //
  // Prepare for virtio-0.9.5, 2.4.2 Receiving Used Buffers From the Device.
  // We're going to poll the answer, the host should not send an interrupt.
  //
  *Ring->Avail.Flags = (UINT16)VRING_AVAIL_F_NO_INTERRUPT;

  //
  // Prepare for virtio-0.9.5, 2.4.1 Supplying Buffers to the Device.
  //
  // Since we support only one in-flight descriptor chain, we can always build
  // that chain starting at entry #0 of the descriptor table.
  //
  Indices->HeadDescIdx = 0;
  Indices->NextDescIdx = Indices->HeadDescIdx;
}

/**

  Append a contiguous buffer for transmission / reception via the virtio ring.

  This function implements the following section from virtio-0.9.5:
  - 2.4.1.1 Placing Buffers into the Descriptor Table

  Free space is taken as granted, since the individual drivers support only
  synchronous requests and host side status is processed in lock-step with
  request submission. It is the calling driver's responsibility to verify the
  ring size in advance.

  The caller is responsible for initializing *Indices with VirtioPrepare()
  first.

  @param[in,out] Ring               The virtio ring to append the buffer to,
                                    as a descriptor.

  @param[in] BufferDeviceAddress    (Bus master device) start address of the
                                    transmit / receive buffer.

  @param[in] BufferSize             Number of bytes to transmit or receive.

  @param[in] Flags                  A bitmask of VRING_DESC_F_* flags. The
                                    caller computes this mask dependent on
                                    further buffers to append and transfer
                                    direction. VRING_DESC_F_INDIRECT is
                                    unsupported. The VRING_DESC.Next field is
                                    always set, but the host only interprets
                                    it dependent on VRING_DESC_F_NEXT.

  @param[in,out] Indices            Indices->HeadDescIdx is not accessed.
                                    On input, Indices->NextDescIdx identifies
                                    the next descriptor to carry the buffer.
                                    On output, Indices->NextDescIdx is
                                    incremented by one, modulo 2^16.

**/
VOID
EFIAPI
VirtioAppendDesc (
  IN OUT VRING         *Ring,
  IN     UINT64        BufferDeviceAddress,
  IN     UINT32        BufferSize,
  IN     UINT16        Flags,
  IN OUT DESC_INDICES  *Indices
  )
{
  volatile VRING_DESC  *Desc;

  Desc        = &Ring->Desc[Indices->NextDescIdx++ % Ring->QueueSize];
  Desc->Addr  = BufferDeviceAddress;
  Desc->Len   = BufferSize;
  Desc->Flags = Flags;
  Desc->Next  = Indices->NextDescIdx % Ring->QueueSize;
}

/**

  Notify the host about the descriptor chain just built, and wait until the
  host processes it.

  @param[in] VirtIo       The target virtio device to notify.

  @param[in] VirtQueueId  Identifies the queue for the target device.

  @param[in,out] Ring     The virtio ring with descriptors to submit.

  @param[in] Indices      Indices->NextDescIdx is not accessed.
                          Indices->HeadDescIdx identifies the head descriptor
                          of the descriptor chain.

  @param[out] UsedLen     On success, the total number of bytes, consecutively
                          across the buffers linked by the descriptor chain,
                          that the host wrote. May be NULL if the caller
                          doesn't care, or can compute the same information
                          from device-specific request structures linked by the
                          descriptor chain.

  @return              Error code from VirtIo->SetQueueNotify() if it fails.

  @retval EFI_SUCCESS  Otherwise, the host processed all descriptors.

**/
EFI_STATUS
EFIAPI
VirtioFlush (
  IN     VIRTIO_DEVICE_PROTOCOL  *VirtIo,
  IN     UINT16                  VirtQueueId,
  IN OUT VRING                   *Ring,
  IN     DESC_INDICES            *Indices,
  OUT    UINT32                  *UsedLen    OPTIONAL
  )
{
  UINT16      NextAvailIdx;
  UINT16      LastUsedIdx;
  EFI_STATUS  Status;
  UINTN       PollPeriodUsecs;

  //
  // virtio-0.9.5, 2.4.1.2 Updating the Available Ring
  //
  // It is not exactly clear from the wording of the virtio-0.9.5
  // specification, but each entry in the Available Ring references only the
  // head descriptor of any given descriptor chain.
  //
  NextAvailIdx = *Ring->Avail.Idx;
  //
  // (Due to our lock-step progress, this is where the host will produce the
  // used element with the head descriptor's index in it.)
  //
  LastUsedIdx                                        = NextAvailIdx;
  Ring->Avail.Ring[NextAvailIdx++ % Ring->QueueSize] =
    Indices->HeadDescIdx % Ring->QueueSize;

  //
  // virtio-0.9.5, 2.4.1.3 Updating the Index Field
  //
  MemoryFence ();
  *Ring->Avail.Idx = NextAvailIdx;

  //
  // virtio-0.9.5, 2.4.1.4 Notifying the Device -- gratuitous notifications are
  // OK.
  //
  MemoryFence ();
  Status = VirtIo->SetQueueNotify (VirtIo, VirtQueueId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // virtio-0.9.5, 2.4.2 Receiving Used Buffers From the Device
  // Wait until the host processes and acknowledges our descriptor chain. The
  // condition we use for polling is greatly simplified and relies on the
  // synchronous, lock-step progress.
  //
  // Keep slowing down until we reach a poll period of slightly above 1 ms.
  //
  PollPeriodUsecs = 1;
  MemoryFence ();
  while (*Ring->Used.Idx != NextAvailIdx) {
    gBS->Stall (PollPeriodUsecs); // calls AcpiTimerLib::MicroSecondDelay

    if (PollPeriodUsecs < 1024) {
      PollPeriodUsecs *= 2;
    }

    MemoryFence ();
  }

  MemoryFence ();

  if (UsedLen != NULL) {
    volatile CONST VRING_USED_ELEM  *UsedElem;

    UsedElem = &Ring->Used.UsedElem[LastUsedIdx % Ring->QueueSize];
    ASSERT (UsedElem->Id == Indices->HeadDescIdx);
    *UsedLen = UsedElem->Len;
  }

  return EFI_SUCCESS;
}

/**

  Report the feature bits to the VirtIo 1.0 device that the VirtIo 1.0 driver
  understands.

  In VirtIo 1.0, a device can reject a self-inconsistent feature bitmap through
  the new VSTAT_FEATURES_OK status bit. (For example if the driver requests a
  higher level feature but clears a prerequisite feature.) This function is a
  small wrapper around VIRTIO_DEVICE_PROTOCOL.SetGuestFeatures() that also
  verifies if the VirtIo 1.0 device accepts the feature bitmap.

  @param[in]     VirtIo        Report feature bits to this device.

  @param[in]     Features      The set of feature bits that the driver wishes
                               to report. The caller is responsible to perform
                               any masking before calling this function; the
                               value is directly written with
                               VIRTIO_DEVICE_PROTOCOL.SetGuestFeatures().

  @param[in,out] DeviceStatus  On input, the status byte most recently written
                               to the device's status register. On output (even
                               on error), DeviceStatus will be updated so that
                               it is suitable for further status bit
                               manipulation and writing to the device's status
                               register.

  @retval  EFI_SUCCESS      The device accepted the configuration in Features.

  @return  EFI_UNSUPPORTED  The device rejected the configuration in Features.

  @retval  EFI_UNSUPPORTED  VirtIo->Revision is smaller than 1.0.0.

  @return                   Error codes from the SetGuestFeatures(),
                            SetDeviceStatus(), GetDeviceStatus() member
                            functions.

**/
EFI_STATUS
EFIAPI
Virtio10WriteFeatures (
  IN     VIRTIO_DEVICE_PROTOCOL  *VirtIo,
  IN     UINT64                  Features,
  IN OUT UINT8                   *DeviceStatus
  )
{
  EFI_STATUS  Status;

  if (VirtIo->Revision < VIRTIO_SPEC_REVISION (1, 0, 0)) {
    return EFI_UNSUPPORTED;
  }

  Status = VirtIo->SetGuestFeatures (VirtIo, Features);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *DeviceStatus |= VSTAT_FEATURES_OK;
  Status         = VirtIo->SetDeviceStatus (VirtIo, *DeviceStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = VirtIo->GetDeviceStatus (VirtIo, DeviceStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((*DeviceStatus & VSTAT_FEATURES_OK) == 0) {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/**
  Provides the virtio device address required to access system memory from a
  DMA bus master.

  The interface follows the same usage pattern as defined in UEFI spec 2.6
  (Section 13.2 PCI Root Bridge I/O Protocol)

  The VirtioMapAllBytesInSharedBuffer() is similar to VIRTIO_MAP_SHARED
  with exception that NumberOfBytes is IN-only parameter. The function
  maps all the bytes specified in NumberOfBytes param in one consecutive
  range.

  @param[in]     VirtIo           The virtio device for which the mapping is
                                  requested.

  @param[in]     Operation        Indicates if the bus master is going to
                                  read or write to system memory.

  @param[in]     HostAddress      The system memory address to map to shared
                                  buffer address.

  @param[in]     NumberOfBytes    Number of bytes to map.

  @param[out]    DeviceAddress    The resulting shared map address for the
                                  bus master to access the hosts HostAddress.

  @param[out]    Mapping          A resulting token to pass to
                                  VIRTIO_UNMAP_SHARED.


  @retval EFI_SUCCESS             The NumberOfBytes is successfully mapped.
  @retval EFI_UNSUPPORTED         The HostAddress cannot be mapped as a
                                  common buffer.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to
                                  a lack of resources. This includes the case
                                  when NumberOfBytes bytes cannot be mapped
                                  in one consecutive range.
  @retval EFI_DEVICE_ERROR        The system hardware could not map the
                                  requested address.
**/
EFI_STATUS
EFIAPI
VirtioMapAllBytesInSharedBuffer (
  IN  VIRTIO_DEVICE_PROTOCOL  *VirtIo,
  IN  VIRTIO_MAP_OPERATION    Operation,
  IN  VOID                    *HostAddress,
  IN  UINTN                   NumberOfBytes,
  OUT EFI_PHYSICAL_ADDRESS    *DeviceAddress,
  OUT VOID                    **Mapping
  )
{
  EFI_STATUS            Status;
  VOID                  *MapInfo;
  UINTN                 Size;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;

  Size   = NumberOfBytes;
  Status = VirtIo->MapSharedBuffer (
                     VirtIo,
                     Operation,
                     HostAddress,
                     &Size,
                     &PhysicalAddress,
                     &MapInfo
                     );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Size < NumberOfBytes) {
    goto Failed;
  }

  *Mapping       = MapInfo;
  *DeviceAddress = PhysicalAddress;

  return EFI_SUCCESS;

Failed:
  VirtIo->UnmapSharedBuffer (VirtIo, MapInfo);
  return EFI_OUT_OF_RESOURCES;
}

/**

  Map the ring buffer so that it can be accessed equally by both guest
  and hypervisor.

  @param[in]      VirtIo          The virtio device instance.

  @param[in]      Ring            The virtio ring to map.

  @param[out]     RingBaseShift   A resulting translation offset, to be
                                  passed to VirtIo->SetQueueAddress().

  @param[out]     Mapping         A resulting token to pass to
                                  VirtIo->UnmapSharedBuffer().

  @return         Status code from VirtIo->MapSharedBuffer()
**/
EFI_STATUS
EFIAPI
VirtioRingMap (
  IN  VIRTIO_DEVICE_PROTOCOL  *VirtIo,
  IN  VRING                   *Ring,
  OUT UINT64                  *RingBaseShift,
  OUT VOID                    **Mapping
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  DeviceAddress;

  Status = VirtioMapAllBytesInSharedBuffer (
             VirtIo,
             VirtioOperationBusMasterCommonBuffer,
             Ring->Base,
             EFI_PAGES_TO_SIZE (Ring->NumPages),
             &DeviceAddress,
             Mapping
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *RingBaseShift = DeviceAddress - (UINT64)(UINTN)Ring->Base;
  return EFI_SUCCESS;
}

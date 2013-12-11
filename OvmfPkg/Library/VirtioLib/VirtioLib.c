/** @file

  Utility functions used by virtio device drivers.

  Copyright (C) 2012, Red Hat, Inc.
  Portion of Copyright (C) 2013, ARM Ltd.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
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

  @param[in]                    The number of descriptors to allocate for the
                                virtio ring, as requested by the host.

  @param[out] Ring              The virtio ring to set up.

  @retval EFI_OUT_OF_RESOURCES  AllocatePages() failed to allocate contiguous
                                pages for the requested QueueSize. Fields of
                                Ring have indeterminate value.

  @retval EFI_SUCCESS           Allocation and setup successful. Ring->Base
                                (and nothing else) is responsible for
                                deallocation.

**/
EFI_STATUS
EFIAPI
VirtioRingInit (
  IN  UINT16 QueueSize,
  OUT VRING  *Ring
  )
{
  UINTN          RingSize;
  volatile UINT8 *RingPagesPtr;

  RingSize = ALIGN_VALUE (
               sizeof *Ring->Desc            * QueueSize +
               sizeof *Ring->Avail.Flags                 +
               sizeof *Ring->Avail.Idx                   +
               sizeof *Ring->Avail.Ring      * QueueSize +
               sizeof *Ring->Avail.UsedEvent,
               EFI_PAGE_SIZE);

  RingSize += ALIGN_VALUE (
                sizeof *Ring->Used.Flags                  +
                sizeof *Ring->Used.Idx                    +
                sizeof *Ring->Used.UsedElem   * QueueSize +
                sizeof *Ring->Used.AvailEvent,
                EFI_PAGE_SIZE);

  Ring->NumPages = EFI_SIZE_TO_PAGES (RingSize);
  Ring->Base = AllocatePages (Ring->NumPages);
  if (Ring->Base == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  SetMem (Ring->Base, RingSize, 0x00);
  RingPagesPtr = Ring->Base;

  Ring->Desc = (volatile VOID *) RingPagesPtr;
  RingPagesPtr += sizeof *Ring->Desc * QueueSize;

  Ring->Avail.Flags = (volatile VOID *) RingPagesPtr;
  RingPagesPtr += sizeof *Ring->Avail.Flags;

  Ring->Avail.Idx = (volatile VOID *) RingPagesPtr;
  RingPagesPtr += sizeof *Ring->Avail.Idx;

  Ring->Avail.Ring = (volatile VOID *) RingPagesPtr;
  RingPagesPtr += sizeof *Ring->Avail.Ring * QueueSize;

  Ring->Avail.UsedEvent = (volatile VOID *) RingPagesPtr;
  RingPagesPtr += sizeof *Ring->Avail.UsedEvent;

  RingPagesPtr = (volatile UINT8 *) Ring->Base +
                 ALIGN_VALUE (RingPagesPtr - (volatile UINT8 *) Ring->Base,
                   EFI_PAGE_SIZE);

  Ring->Used.Flags = (volatile VOID *) RingPagesPtr;
  RingPagesPtr += sizeof *Ring->Used.Flags;

  Ring->Used.Idx = (volatile VOID *) RingPagesPtr;
  RingPagesPtr += sizeof *Ring->Used.Idx;

  Ring->Used.UsedElem = (volatile VOID *) RingPagesPtr;
  RingPagesPtr += sizeof *Ring->Used.UsedElem * QueueSize;

  Ring->Used.AvailEvent = (volatile VOID *) RingPagesPtr;
  RingPagesPtr += sizeof *Ring->Used.AvailEvent;

  Ring->QueueSize = QueueSize;
  return EFI_SUCCESS;
}


/**

  Tear down the internal resources of a configured virtio ring.

  The caller is responsible to stop the host from using this ring before
  invoking this function: the VSTAT_DRIVER_OK bit must be clear in
  VhdrDeviceStatus.

  @param[out] Ring  The virtio ring to clean up.

**/
VOID
EFIAPI
VirtioRingUninit (
  IN OUT VRING *Ring
  )
{
  FreePages (Ring->Base, Ring->NumPages);
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
  IN OUT VRING        *Ring,
  OUT    DESC_INDICES *Indices
  )
{
  //
  // Prepare for virtio-0.9.5, 2.4.2 Receiving Used Buffers From the Device.
  // We're going to poll the answer, the host should not send an interrupt.
  //
  *Ring->Avail.Flags = (UINT16) VRING_AVAIL_F_NO_INTERRUPT;

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

  @param[in,out] Ring        The virtio ring to append the buffer to, as a
                             descriptor.

  @param[in] BufferPhysAddr  (Guest pseudo-physical) start address of the
                             transmit / receive buffer.

  @param[in] BufferSize      Number of bytes to transmit or receive.

  @param[in] Flags           A bitmask of VRING_DESC_F_* flags. The caller
                             computes this mask dependent on further buffers to
                             append and transfer direction.
                             VRING_DESC_F_INDIRECT is unsupported. The
                             VRING_DESC.Next field is always set, but the host
                             only interprets it dependent on VRING_DESC_F_NEXT.

  @param[in,out] Indices     Indices->HeadDescIdx is not accessed.
                             On input, Indices->NextDescIdx identifies the next
                             descriptor to carry the buffer. On output,
                             Indices->NextDescIdx is incremented by one, modulo
                             2^16.

**/
VOID
EFIAPI
VirtioAppendDesc (
  IN OUT VRING        *Ring,
  IN     UINTN        BufferPhysAddr,
  IN     UINT32       BufferSize,
  IN     UINT16       Flags,
  IN OUT DESC_INDICES *Indices
  )
{
  volatile VRING_DESC *Desc;

  Desc        = &Ring->Desc[Indices->NextDescIdx++ % Ring->QueueSize];
  Desc->Addr  = BufferPhysAddr;
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


  @return              Error code from VirtIo->SetQueueNotify() if it fails.

  @retval EFI_SUCCESS  Otherwise, the host processed all descriptors.

**/
EFI_STATUS
EFIAPI
VirtioFlush (
  IN     VIRTIO_DEVICE_PROTOCOL *VirtIo,
  IN     UINT16                 VirtQueueId,
  IN OUT VRING                  *Ring,
  IN     DESC_INDICES           *Indices
  )
{
  UINT16     NextAvailIdx;
  EFI_STATUS Status;
  UINTN      PollPeriodUsecs;

  //
  // virtio-0.9.5, 2.4.1.2 Updating the Available Ring
  //
  // It is not exactly clear from the wording of the virtio-0.9.5
  // specification, but each entry in the Available Ring references only the
  // head descriptor of any given descriptor chain.
  //
  NextAvailIdx = *Ring->Avail.Idx;
  Ring->Avail.Ring[NextAvailIdx++ % Ring->QueueSize] =
    Indices->HeadDescIdx % Ring->QueueSize;

  //
  // virtio-0.9.5, 2.4.1.3 Updating the Index Field
  //
  MemoryFence();
  *Ring->Avail.Idx = NextAvailIdx;

  //
  // virtio-0.9.5, 2.4.1.4 Notifying the Device -- gratuitous notifications are
  // OK.
  //
  MemoryFence();
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
  MemoryFence();
  while (*Ring->Used.Idx != NextAvailIdx) {
    gBS->Stall (PollPeriodUsecs); // calls AcpiTimerLib::MicroSecondDelay

    if (PollPeriodUsecs < 1024) {
      PollPeriodUsecs *= 2;
    }
    MemoryFence();
  }

  MemoryFence();
  return EFI_SUCCESS;
}

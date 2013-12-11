/** @file

  Declarations of utility functions used by virtio device drivers.

  Copyright (C) 2012, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VIRTIO_LIB_H_
#define _VIRTIO_LIB_H_

#include <Protocol/VirtioDevice.h>

#include <IndustryStandard/Virtio.h>


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
  );


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
  );


//
// Internal use structure for tracking the submission of a multi-descriptor
// request.
//
typedef struct {
  UINT16 HeadDescIdx;
  UINT16 NextDescIdx;
} DESC_INDICES;


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
  );


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
  );


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
  );

#endif // _VIRTIO_LIB_H_

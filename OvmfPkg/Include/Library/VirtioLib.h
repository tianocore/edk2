/** @file

  Declarations of utility functions used by virtio device drivers.

  Copyright (C) 2012-2016, Red Hat, Inc.
  Copyright (C) 2017, AMD Inc, All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

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
  IN  VIRTIO_DEVICE_PROTOCOL *VirtIo,
  IN  UINT16                 QueueSize,
  OUT VRING                  *Ring
  );


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
  IN  VIRTIO_DEVICE_PROTOCOL *VirtIo,
  IN  VRING                  *Ring,
  OUT UINT64                 *RingBaseShift,
  OUT VOID                   **Mapping
  );

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
  IN     VIRTIO_DEVICE_PROTOCOL *VirtIo,
  IN OUT VRING                  *Ring
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
  IN OUT VRING        *Ring,
  IN     UINT64       BufferDeviceAddress,
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
  IN     VIRTIO_DEVICE_PROTOCOL *VirtIo,
  IN     UINT16                 VirtQueueId,
  IN OUT VRING                  *Ring,
  IN     DESC_INDICES           *Indices,
  OUT    UINT32                 *UsedLen    OPTIONAL
  );


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
  IN     VIRTIO_DEVICE_PROTOCOL *VirtIo,
  IN     UINT64                 Features,
  IN OUT UINT8                  *DeviceStatus
  );

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


  @retval EFI_SUCCESS             The NumberOfBytes is succesfully mapped.
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
  );
#endif // _VIRTIO_LIB_H_

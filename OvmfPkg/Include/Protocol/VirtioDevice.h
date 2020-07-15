/** @file
  Virtio Device

  DISCLAIMER: the VIRTIO_DEVICE_PROTOCOL introduced here is a work in progress,
  and should not be used outside of the EDK II tree.

  Copyright (c) 2013, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2017, AMD Inc, All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VIRTIO_DEVICE_H__
#define __VIRTIO_DEVICE_H__

#include <IndustryStandard/Virtio.h>

//
// VirtIo Specification Revision: Major[31:24].Minor[23:16].Revision[15:0]
//
#define VIRTIO_SPEC_REVISION(major,minor,revision) \
  ((((major) & 0xFF) << 24) | (((minor) & 0xFF) << 16) | ((revision) & 0xFFFF))

#define VIRTIO_DEVICE_PROTOCOL_GUID { \
  0xfa920010, 0x6785, 0x4941, {0xb6, 0xec, 0x49, 0x8c, 0x57, 0x9f, 0x16, 0x0a }\
  }

typedef struct _VIRTIO_DEVICE_PROTOCOL  VIRTIO_DEVICE_PROTOCOL;

//
// VIRTIO Operation for VIRTIO_MAP_SHARED
//
typedef enum {
  //
  // A read operation from system memory by a bus master
  //
  VirtioOperationBusMasterRead,
  //
  // A write operation to system memory by a bus master
  //
  VirtioOperationBusMasterWrite,
  //
  // Provides both read and write access to system memory by both the
  // processor and a bus master
  //
  VirtioOperationBusMasterCommonBuffer,
} VIRTIO_MAP_OPERATION;

/**

  Read a word from the device-specific I/O region of the Virtio Header.

  @param[in] This         This instance of VIRTIO_DEVICE_PROTOCOL

  @param[in] FieldOffset  Source offset.

  @param[in] FieldSize    Source field size in bytes, must be in {1, 2, 4, 8}.

  @param[in] BufferSize   Number of bytes available in the target buffer. Must
                          equal FieldSize.

  @param[out] Buffer      Target buffer.

  @retval EFI_SUCCESS           The data was read successfully.
  @retval EFI_UNSUPPORTED       The underlying IO device doesn't support the
                                provided address offset and read size.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_DEVICE_READ) (
  IN  VIRTIO_DEVICE_PROTOCOL *This,
  IN  UINTN                  FieldOffset,
  IN  UINTN                  FieldSize,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  );

/**

  Write a word to the device-specific I/O region of the Virtio Header.

  @param[in] This         This instance of VIRTIO_DEVICE_PROTOCOL

  @param[in] FieldOffset  Destination offset.

  @param[in] FieldSize    Destination field size in bytes,
                          must be in {1, 2, 4, 8}.

  @param[out] Value       Value to write.

  @retval EFI_SUCCESS           The data was written successfully.
  @retval EFI_UNSUPPORTED       The underlying IO device doesn't support the
                                provided address offset and write size.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_DEVICE_WRITE) (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  IN UINTN                  FieldOffset,
  IN UINTN                  FieldSize,
  IN UINT64                 Value
  );

/**
  Read the device features field from the Virtio Header.

  @param[in] This                 This instance of VIRTIO_DEVICE_PROTOCOL

  @param[out] DeviceFeatures      The device features field.

  @retval EFI_SUCCESS             The data was read successfully.
  @retval EFI_UNSUPPORTED         The underlying IO device doesn't support the
                                  provided address offset and read size.
  @retval EFI_INVALID_PARAMETER   DeviceFeatures is NULL
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_GET_DEVICE_FEATURES) (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  OUT UINT64                *DeviceFeatures
  );

/**
  Write the guest features field in the Virtio Header.

  @param[in] This         This instance of VIRTIO_DEVICE_PROTOCOL

  @param[in] Features     The guest features field

**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_SET_GUEST_FEATURES) (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT64                   Features
  );

/**
  Write the queue address field(s) in the Virtio Header.

  @param[in] This             This instance of VIRTIO_DEVICE_PROTOCOL

  @param[in] Ring             The initialized VRING object to take the
                              addresses from. The caller is responsible for
                              ensuring that on input, all Ring->NumPages pages,
                              starting at Ring->Base, have been successfully
                              mapped with a single call to
                              This->MapSharedBuffer() for CommonBuffer bus
                              master operation.

  @param[in] RingBaseShift    Adding this value using UINT64 arithmetic to the
                              addresses found in Ring translates them from
                              system memory to bus addresses. The caller shall
                              calculate RingBaseShift as
                              (DeviceAddress - (UINT64)(UINTN)HostAddress),
                              where DeviceAddress and HostAddress (i.e.,
                              Ring->Base) were output and input parameters of
                              This->MapSharedBuffer(), respectively.

  @retval EFI_SUCCESS         The data was written successfully.
  @retval EFI_UNSUPPORTED     The underlying IO device doesn't support the
                              provided address offset and write size.
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_SET_QUEUE_ADDRESS) (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN VRING                   *Ring,
  IN UINT64                  RingBaseShift
  );

/**

  Write the queue select field in the Virtio Header.

  Writing to the queue select field sets the index of the queue to which
  operations such as SetQueueAlign and GetQueueNumMax apply.

  @param[in] This         This instance of VIRTIO_DEVICE_PROTOCOL

  @param[in] Index        The index of the queue to select

  @retval EFI_SUCCESS         The data was written successfully.
  @retval EFI_UNSUPPORTED     The underlying IO device doesn't support the
                              provided address offset and write size.
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_SET_QUEUE_SEL) (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT16                   Index
  );

/**

  Write the queue notify field in the Virtio Header.

  @param[in] This         This instance of VIRTIO_DEVICE_PROTOCOL

  @param[in] Address      The 32-bit Queue Notify field

  @retval EFI_SUCCESS         The data was written successfully.
  @retval EFI_UNSUPPORTED     The underlying IO device doesn't support the
                              provided address offset and write size.
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_SET_QUEUE_NOTIFY) (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT16                   Index
  );

/**
  Write the queue alignment field in the Virtio Header.

  The queue to which the alignment applies is selected by the Queue Select
  field.

  Note: This operation is not implemented by the VirtIo over PCI. The PCI
  implementation of this protocol returns EFI_SUCCESS.

  @param[in] This         This instance of VIRTIO_DEVICE_PROTOCOL

  @param[in] Alignment    The alignment boundary of the Used Ring in bytes.
                          Must be a power of 2.

  @retval EFI_SUCCESS         The data was written successfully.
  @retval EFI_UNSUPPORTED     The underlying IO device doesn't support the
                              provided address offset and write size.
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_SET_QUEUE_ALIGN) (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT32                   Alignment
  );

/**
  Write the guest page size.

  Note: This operation is not implemented by the VirtIo over PCI. The PCI
  implementation of this protocol returns EFI_SUCCESS.

  @param[in] This             This instance of VIRTIO_DEVICE_PROTOCOL

  @param[in] PageSize         Size of the Guest page in bytes.
                              Must be a power of 2.

  @retval EFI_SUCCESS         The data was written successfully.
  @retval EFI_UNSUPPORTED     The underlying IO device doesn't support the
                              provided address offset and write size.
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_SET_PAGE_SIZE) (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT32                   PageSize
  );

/**

  Get the size of the virtqueue selected by the queue select field.

  See Virtio spec Section 2.3

  @param[in] This                 This instance of VIRTIO_DEVICE_PROTOCOL

  @param[out] QueueNumMax         The size of the virtqueue in bytes.
                                  Always a power of 2.

  @retval EFI_SUCCESS             The data was read successfully.
  @retval EFI_UNSUPPORTED         The underlying IO device doesn't support the
                                  provided address offset and read size.
  @retval EFI_INVALID_PARAMETER   QueueNumMax is NULL
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_GET_QUEUE_NUM_MAX) (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT16                  *QueueNumMax
  );

/**

  Write to the QueueNum field in the Virtio Header.

  This function only applies to Virtio-MMIO and may be a stub for other
  implementations. See Virtio Spec appendix X.

  @param[in] This         This instance of VIRTIO_DEVICE_PROTOCOL

  @param[in] QueueSize    The number of elements in the queue.

  @retval EFI_SUCCESS         The data was written successfully.
  @retval EFI_UNSUPPORTED     The underlying IO device doesn't support the
                              provided address offset and write size.
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_SET_QUEUE_NUM) (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT16                   QueueSize
  );

/**

  Get the DeviceStatus field from the Virtio Header.

  @param[in] This                 This instance of VIRTIO_DEVICE_PROTOCOL

  @param[out] DeviceStatus        The 8-bit value for the Device status field

  @retval EFI_SUCCESS             The data was read successfully.
  @retval EFI_UNSUPPORTED         The underlying IO device doesn't support the
                                  provided address offset and read size.
  @retval EFI_INVALID_PARAMETER   DeviceStatus is NULL
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_GET_DEVICE_STATUS) (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT8                   *DeviceStatus
  );

/**

  Write the DeviceStatus field in the Virtio Header.

  @param[in] This         This instance of VIRTIO_DEVICE_PROTOCOL

  @param[in] DeviceStatus The 8-bit value for the Device status field

  @retval EFI_SUCCESS         The data was written successfully.
  @retval EFI_UNSUPPORTED     The underlying IO device doesn't support the
                              provided address offset and write size.
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_SET_DEVICE_STATUS) (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT8                   DeviceStatus
  );

/**

  Allocates pages that are suitable for an VirtioOperationBusMasterCommonBuffer
  mapping. This means that the buffer allocated by this function supports
  simultaneous access by both the processor and the bus master. The device
  address that the bus master uses to access the buffer must be retrieved with
  a call to VIRTIO_MAP_SHARED.

  @param[in]      This              The protocol instance pointer.

  @param[in]      Pages             The number of pages to allocate.

  @param[in,out]  HostAddress       A pointer to store the system memory base
                                    address of the allocated range.

  @retval EFI_SUCCESS               The requested memory pages were allocated.
  @retval EFI_OUT_OF_RESOURCES      The memory pages could not be allocated.

**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_ALLOCATE_SHARED)(
  IN     VIRTIO_DEVICE_PROTOCOL                   *This,
  IN     UINTN                                    Pages,
  IN OUT VOID                                     **HostAddress
  );

/**
  Frees memory that was allocated with VIRTIO_ALLOCATE_SHARED.

  @param[in]  This           The protocol instance pointer.

  @param[in]  Pages          The number of pages to free.

  @param[in]  HostAddress    The system memory base address of the allocated
                             range.

**/
typedef
VOID
(EFIAPI *VIRTIO_FREE_SHARED)(
  IN  VIRTIO_DEVICE_PROTOCOL                   *This,
  IN  UINTN                                    Pages,
  IN  VOID                                     *HostAddress
  );

/**
  Provides the virtio device address required to access system memory from a
  DMA bus master.

  The interface follows the same usage pattern as defined in UEFI spec 2.6
  (Section 13.2 PCI Root Bridge I/O Protocol)

  @param[in]     This             The protocol instance pointer.

  @param[in]     Operation        Indicates if the bus master is going to
                                  read or write to system memory.

  @param[in]     HostAddress      The system memory address to map to shared
                                  buffer address.

  @param[in,out] NumberOfBytes    On input the number of bytes to map.
                                  On output the number of bytes that were
                                  mapped.

  @param[out]    DeviceAddress    The resulting shared map address for the
                                  bus master to access the hosts HostAddress.

  @param[out]    Mapping          A resulting token to pass to
                                  VIRTIO_UNMAP_SHARED.

  @retval EFI_SUCCESS             The range was mapped for the returned
                                  NumberOfBytes.
  @retval EFI_UNSUPPORTED         The HostAddress cannot be mapped as a
                                  common buffer.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to
                                  a lack of resources.
  @retval EFI_DEVICE_ERROR        The system hardware could not map the
                                  requested address.
**/

typedef
EFI_STATUS
(EFIAPI *VIRTIO_MAP_SHARED) (
  IN     VIRTIO_DEVICE_PROTOCOL       *This,
  IN     VIRTIO_MAP_OPERATION         Operation,
  IN     VOID                         *HostAddress,
  IN OUT UINTN                        *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS         *DeviceAddress,
  OUT    VOID                         **Mapping
  );

/**
  Completes the VIRTIO_MAP_SHARED operation and releases any corresponding
  resources.

  @param[in]  This               The protocol instance pointer.

  @param[in]  Mapping            The mapping token returned from
                                 VIRTIO_MAP_SHARED.

  @retval EFI_SUCCESS            The range was unmapped.
  @retval EFI_INVALID_PARAMETER  Mapping is not a value that was returned by
                                 VIRTIO_MAP_SHARED. Passing an invalid Mapping
                                 token can cause undefined behavior.
  @retval EFI_DEVICE_ERROR       The data was not committed to the target
                                 system memory.
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_UNMAP_SHARED)(
  IN  VIRTIO_DEVICE_PROTOCOL    *This,
  IN  VOID                      *Mapping
  );

///
///  This protocol provides an abstraction over the VirtIo transport layer
///
///  DISCLAIMER: this protocol is a work in progress, and should not be used
///  outside of the EDK II tree.
///
struct _VIRTIO_DEVICE_PROTOCOL {
  //
  // VirtIo Specification Revision encoded with VIRTIO_SPEC_REVISION()
  //
  UINT32                      Revision;
  //
  // From the Virtio Spec
  //
  INT32                       SubSystemDeviceId;

  VIRTIO_GET_DEVICE_FEATURES  GetDeviceFeatures;
  VIRTIO_SET_GUEST_FEATURES   SetGuestFeatures;

  VIRTIO_SET_QUEUE_ADDRESS    SetQueueAddress;

  VIRTIO_SET_QUEUE_SEL        SetQueueSel;

  VIRTIO_SET_QUEUE_NOTIFY     SetQueueNotify;

  VIRTIO_SET_QUEUE_ALIGN      SetQueueAlign;
  VIRTIO_SET_PAGE_SIZE        SetPageSize;

  VIRTIO_GET_QUEUE_NUM_MAX    GetQueueNumMax;
  VIRTIO_SET_QUEUE_NUM        SetQueueNum;

  VIRTIO_GET_DEVICE_STATUS    GetDeviceStatus;
  VIRTIO_SET_DEVICE_STATUS    SetDeviceStatus;

  //
  // Functions to read/write Device Specific headers
  //
  VIRTIO_DEVICE_WRITE         WriteDevice;
  VIRTIO_DEVICE_READ          ReadDevice;

  //
  // Functions to allocate, free, map and unmap shared buffer
  //
  VIRTIO_ALLOCATE_SHARED      AllocateSharedPages;
  VIRTIO_FREE_SHARED          FreeSharedPages;
  VIRTIO_MAP_SHARED           MapSharedBuffer;
  VIRTIO_UNMAP_SHARED         UnmapSharedBuffer;
};

extern EFI_GUID gVirtioDeviceProtocolGuid;

#endif

/** @file
  Virtio Device

  DISCLAIMER: the VIRTIO_DEVICE_PROTOCOL introduced here is a work in progress,
  and should not be used outside of the EDK II tree.

  Copyright (c) 2013, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __VIRTIO_DEVICE_H__
#define __VIRTIO_DEVICE_H__

// VirtIo Specification Revision: Major[31:24].Minor[23:16].Revision[15:0
#define VIRTIO_SPEC_REVISION(major,minor,revision) \
  ((((major) & 0xFF) << 24) | (((minor) & 0xFF) << 16) | ((revision) & 0xFFFF))

#define VIRTIO_DEVICE_PROTOCOL_GUID { \
  0xfa920010, 0x6785, 0x4941, {0xb6, 0xec, 0x49, 0x8c, 0x57, 0x9f, 0x16, 0x0a }\
  }

typedef struct _VIRTIO_DEVICE_PROTOCOL  VIRTIO_DEVICE_PROTOCOL;

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

  @param[out] DeviceFeatures      The 32-bit device features field.

  @retval EFI_SUCCESS             The data was read successfully.
  @retval EFI_UNSUPPORTED         The underlying IO device doesn't support the
                                  provided address offset and read size.
  @retval EFI_INVALID_PARAMETER   DeviceFeatures is NULL
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_GET_DEVICE_FEATURES) (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  OUT UINT32                *DeviceFeatures
  );

/**
  Write the guest features field in the Virtio Header.

  @param[in] This         This instance of VIRTIO_DEVICE_PROTOCOL

  @param[in] Features     The 32-bit guest guest features field

**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_SET_GUEST_FEATURES) (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT32                   Features
  );

/**
  Read the queue address field from the Virtio Header.

  QueueAddress is the address of the virtqueue divided by 4096.

  @param[in] This                 This instance of VIRTIO_DEVICE_PROTOCOL

  @param[out] QueueAddress        The 32-bit queue address field.

  @retval EFI_SUCCESS             The data was read successfully.
  @retval EFI_UNSUPPORTED         The underlying IO device doesn't support the
                                  provided address offset and read size.
  @retval EFI_INVALID_PARAMETER   QueueAddress is NULL
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_GET_QUEUE_ADDRESS) (
  IN  VIRTIO_DEVICE_PROTOCOL *This,
  OUT UINT32                 *QueueAddress
  );

/**
  Write the queue address field in the Virtio Header.

  The parameter Address must be the base address of the virtqueue divided
  by 4096.

  @param[in] This             This instance of VIRTIO_DEVICE_PROTOCOL

  @param[in] Address          The 32-bit Queue Address field

  @retval EFI_SUCCESS         The data was written successfully.
  @retval EFI_UNSUPPORTED     The underlying IO device doesn't support the
                              provided address offset and write size.
**/
typedef
EFI_STATUS
(EFIAPI *VIRTIO_SET_QUEUE_ADDRESS) (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT32                   Address
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


///
///  This protocol provides an abstraction over the VirtIo transport layer
///
///  DISCLAIMER: this protocol is a work in progress, and should not be used
///  outside of the EDK II tree.
///
struct _VIRTIO_DEVICE_PROTOCOL {
  /// VirtIo Specification Revision encoded with VIRTIO_SPEC_REVISION()
  UINT32                      Revision;
  /// From the Virtio Spec
  INT32                       SubSystemDeviceId;

  VIRTIO_GET_DEVICE_FEATURES  GetDeviceFeatures;
  VIRTIO_SET_GUEST_FEATURES   SetGuestFeatures;

  VIRTIO_GET_QUEUE_ADDRESS    GetQueueAddress;
  VIRTIO_SET_QUEUE_ADDRESS    SetQueueAddress;

  VIRTIO_SET_QUEUE_SEL        SetQueueSel;

  VIRTIO_SET_QUEUE_NOTIFY     SetQueueNotify;

  VIRTIO_SET_QUEUE_ALIGN      SetQueueAlign;
  VIRTIO_SET_PAGE_SIZE        SetPageSize;

  VIRTIO_GET_QUEUE_NUM_MAX    GetQueueNumMax;
  VIRTIO_SET_QUEUE_NUM        SetQueueNum;

  VIRTIO_GET_DEVICE_STATUS    GetDeviceStatus;
  VIRTIO_SET_DEVICE_STATUS    SetDeviceStatus;

  // Functions to read/write Device Specific headers
  VIRTIO_DEVICE_WRITE         WriteDevice;
  VIRTIO_DEVICE_READ          ReadDevice;
};

extern EFI_GUID gVirtioDeviceProtocolGuid;

#endif

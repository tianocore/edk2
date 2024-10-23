/** @file
  Extension to the EFI UsbIo Protocol.

  This protocol extends the functionality of UsbIo protocol.

  - Report descriptors of USB interface's alternate settings without
    performing USB transfer.
  - Report class specific descriptors.

  Copyright (c) 2024, American Megatrends, LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __USB_IO_EXT_H__
#define __USB_IO_EXT_H__

#include <Protocol/UsbIo.h>

#define EDKII_USBIO_EXT_PROTOCOL_GUID \
  { \
    0xf4279fb1, 0xef1e, 0x4346, { 0xab, 0x32, 0x3f, 0xe3, 0x86, 0xee, 0xb4, 0x52 } \
  }

typedef struct _EDKII_USBIO_EXT_PROTOCOL EDKII_USBIO_EXT_PROTOCOL;

/**
  Retrieve the interface descriptor details from the interface setting.

  This is an extended version of UsbIo->GetInterfaceDescriptor. It returns the interface
  descriptor for an alternate setting of the interface without executing SET_INTERFACE
  transfer. It also returns the number of class specific interfaces.

  @param[in]  This              A pointer to the EDKII_USBIO_EXT_PROTOCOL instance.
  @param[in]  AltSetting        Interface alternate setting.
  @param[out]  Descriptor       The caller allocated buffer to return the contents of the Interface descriptor.
  @param[out]  CsInterfaceNumber  Number of class specific interfaces for this interface setting.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER Descriptor or CsInterfaceNumber is NULL.
  @retval EFI_UNSUPPORTED       AltSetting is greater than the number of alternate settings in this interface.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_USBIO_EXT_GET_INTERFACE_DESCRIPTOR)(
  IN  EDKII_USBIO_EXT_PROTOCOL      *This,
  IN  UINTN                         AltSetting,
  OUT EFI_USB_INTERFACE_DESCRIPTOR  *Descriptor,
  OUT UINTN                         *CsInterfacesNumber
  );

/**
  Retrieve the endpoint descriptor from the interface setting.

  This is an extended version of UsbIo->GetEndpointDescriptor. It returns the endpoint
  descriptor for an alternate setting of a given interface.
  Note: The total number of endpoints can be retrieved from the interface descriptor
  returned by EDKII_USBIO_EXT_GET_INTERFACE_DESCRIPTOR function.

  @param[in]  This              A pointer to the EDKII_USBIO_EXT_PROTOCOL instance.
  @param[in]  AltSetting        Interface alternate setting.
  @param[in]  Index             Index of the endpoint to retrieve. The valid range is 0..15.
  @param[out]  Descriptor       A pointer to the caller allocated USB Interface Descriptor.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER Descriptor is NULL.
  @retval EFI_UNSUPPORTED       AltSetting is greater than the number of alternate settings in this interface.
  @retval EFI_NOT_FOUND         Index is greater than the number of endpoints in this interface.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_USBIO_EXT_GET_ENDPOINT_DESCRIPTOR)(
  IN  EDKII_USBIO_EXT_PROTOCOL      *This,
  IN  UINTN                         AltSetting,
  IN  UINTN                         Index,
  OUT EFI_USB_ENDPOINT_DESCRIPTOR   *Descriptor
  );

/**
  Retrieve class specific interface descriptor.

  @param[in]  This              A pointer to the EDKII_USBIO_EXT_PROTOCOL instance.
  @param[in]  AltSetting        Interface alternate setting.
  @param[in]  Index             Zero-based index of the class specific interface.
  @param[in][out]  BufferSize   On input, the size in bytes of the return Descriptor buffer.
                                On output the size of data returned in Descriptor.
  @param[out]  Descriptor       The buffer to return the contents of the class specific interface descriptor. May
                                be NULL with a zero BufferSize in order to determine the size buffer needed.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER BufferSize is NULL.
                                Buffer is NULL and *BufferSize is not zero.
  @retval EFI_UNSUPPORTED       AltSetting is greater than the number of alternate settings in this interface.
  @retval EFI_NOT_FOUND         Index is greater than the number of class specific interfaces.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small for the result. BufferSize has been updated with the size
                                needed to complete the request.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_USBIO_EXT_GET_CS_INTERFACE_DESCRIPTOR)(
  IN  EDKII_USBIO_EXT_PROTOCOL  *This,
  IN  UINTN                     AltSetting,
  IN  UINTN                     Index,
  IN OUT UINTN                  *BufferSize,
  OUT VOID                      *Buffer
  );

/**
  Retrieve class specific endpoint descriptor.

  @param[in]  This              A pointer to the EDKII_USBIO_EXT_PROTOCOL instance.
  @param[in]  AltSetting        Interface alternate setting.
  @param[in]  Index             Zero-based index of the non-zero endpoint.
  @param[in][out]  BufferSize   On input, the size in bytes of the return Descriptor buffer.
                                On output the size of data returned in Descriptor.
  @param[out]  Descriptor       The buffer to return the contents of the class specific endpoint descriptor. May
                                be NULL with a zero BufferSize in order to determine the size buffer needed.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER BufferSize is NULL.
                                Buffer is NULL and *BufferSize is not zero.
  @retval EFI_UNSUPPORTED       AltSetting is greater than the number of alternate settings in this interface.
  @retval EFI_NOT_FOUND         Index is greater than the number of endpoints in this interface.
                                Endpoint does not have class specific endpoint descriptor.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small for the result. BufferSize has been updated with the size
                                needed to complete the request.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_USBIO_EXT_GET_CS_ENDPOINT_DESCRIPTOR)(
  IN  EDKII_USBIO_EXT_PROTOCOL  *This,
  IN  UINTN                     AltSettingsNumber,
  IN  UINTN                     Index,
  IN OUT UINTN                  *BufferSize,
  OUT VOID                      *Buffer
  );

struct _EDKII_USBIO_EXT_PROTOCOL {
  EDKII_USBIO_EXT_GET_INTERFACE_DESCRIPTOR       UsbIoExtGetInterfaceDescriptor;
  EDKII_USBIO_EXT_GET_ENDPOINT_DESCRIPTOR        UsbIoExtGetEndpointDescriptor;
  EDKII_USBIO_EXT_GET_CS_INTERFACE_DESCRIPTOR    UsbIoExtGetCsInterfaceDescriptor;
  EDKII_USBIO_EXT_GET_CS_ENDPOINT_DESCRIPTOR     UsbIoExtGetCsEndpointDescriptor;
};

extern EFI_GUID  gEdkIIUsbIoExtProtocolGuid;

#endif

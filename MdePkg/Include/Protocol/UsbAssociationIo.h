/** @file
  EFI Usb Interface Association protocol.

  This protocol is used by USB drivers, running in the EFI boot services
  environment to access USB devices that implement their configurations
  using interface association: USB cameras, USB audio devices, USB modems, etc.

  Copyright (c) 2024, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __USB_ASSOCIATION_IO_H__
#define __USB_ASSOCIATION_IO_H__

#include <IndustryStandard/Usb.h>
#include <Protocol/UsbIo.h>

typedef USB_INTERFACE_ASSOCIATION_DESCRIPTOR EFI_USB_INTERFACE_ASSOCIATION_DESCRIPTOR;

//
// Global ID for the USB IAD Protocol
//
#define EFI_USB_IA_PROTOCOL_GUID \
  { \
    0xf4279fb1, 0xef1e, 0x4346, { 0xab, 0x32, 0x3f, 0xe3, 0x86, 0xee, 0xb4, 0x52 } \
  }

typedef struct _EFI_USB_IA_PROTOCOL EFI_USB_IA_PROTOCOL;

/**
  Get the USB Interface Association Descriptor from the current USB configuration.

  @param[in]  This              A pointer to the EFI_USB_IA_PROTOCOL instance.
  @param[out]  Descriptor       A pointer to the caller allocated USB Interface Association Descriptor.

  @retval EFI_SUCCESS           The descriptor was retrieved successfully.
  @retval EFI_INVALID_PARAMETER Descriptor is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_IA_GET_ASSOCIATION_DESCRIPTOR)(
  IN  EFI_USB_IA_PROTOCOL                       *This,
  OUT EFI_USB_INTERFACE_ASSOCIATION_DESCRIPTOR  *Descriptor
  );

/**
  Retrieve the details of the requested interface that belongs to USB association.

  @param[in] This              A pointer to the EFI_USB_IA_PROTOCOL instance.
  @param[in] InterfaceNumber   Interface that belongs to this association.
  @param[out] UsbIo            A pointer to the caller allocated UsbIo protocol.
  @param[out] AltSettingsNum   Number of alternate settings of this interface.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER UsbIo or AltSettingsNum is NULL.
  @retval EFI_NOT_FOUND         InterfaceNumber does not belong to this interface association.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_IA_GET_ASSOCIATE_ALTSETTINGS)(
  IN  EFI_USB_IA_PROTOCOL   *This,
  IN  UINT8                 InterfaceNumber,
  OUT EFI_USB_IO_PROTOCOL   **UsbIo,
  OUT UINTN                 *AltSettingsNumber
  );

/**
  Retrieve the interface descriptor details from the interface setting.

  This is an extended version of UsbIo->GetInterfaceDescriptor. It can get the interface descriptor for any alternate
  setting of the interface. It also returns the number of class specific interfaces.

  @param[in]  This              A pointer to the EFI_USB_IA_PROTOCOL instance.
  @param[in]  InterfaceNumber   Interface that belongs to this association.
  @param[in]  AltSetting        Interface alternate setting.
  @param[out]  Descriptor       The caller allocated buffer to return the contents of the Interface descriptor.
  @param[out]  CsInterfaceNumber  Number of class specific interfaces for this interface setting.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER Descriptor or CsInterfaceNumber is NULL.
  @retval EFI_NOT_FOUND         Interface does not belong to this association.
                                AltSetting is greater than the number of alternate settings in this interface.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_IA_GET_INTERFACE_DESCRIPTOR)(
  IN  EFI_USB_IA_PROTOCOL           *This,
  IN  UINT8                         InterfaceNumber,
  IN  UINTN                         AltSetting,
  OUT EFI_USB_INTERFACE_DESCRIPTOR  *Descriptor,
  OUT UINTN                         *CsInterfacesNumber
  );

/**
  Retrieve the endpoint descriptor from the interface setting.

  This is an extended version of UsbIo->GetEndpointDescriptor. It can get the endpoint descriptor for any alternate
  setting of a given interface. Total number of endpoints for this setting can be retrieved from the interface
  descriptor returned by EFI_USB_IA_GET_INTERFACE_DESCRIPTOR function.

  @param[in]  This              A pointer to the EFI_USB_IA_PROTOCOL instance.
  @param[in]  InterfaceNumber   Interface that belongs to this association.
  @param[in]  AltSetting        Interface alternate setting.
  @param[in]  Index             Index of the endpoint to retrieve. The valid range is 0..15.
  @param[out]  Descriptor       A pointer to the caller allocated USB Interface Descriptor.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER Descriptor is NULL.
  @retval EFI_NOT_FOUND         Interface does not belong to this association.
                                AltSetting is greater than the number of alternate settings in this interface.
                                Index is greater than the number of endpoints in this interface.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_IA_GET_ENDPOINT_DESCRIPTOR)(
  IN  EFI_USB_IA_PROTOCOL           *This,
  IN  UINT8                         InterfaceNumber,
  IN  UINTN                         AltSetting,
  IN  UINTN                         Index,
  OUT EFI_USB_ENDPOINT_DESCRIPTOR   *Descriptor
  );

/**
  Retrieve class specific interface descriptor.

  @param[in]  This              A pointer to the EFI_USB_IA_PROTOCOL instance.
  @param[in]  InterfaceNumber   Interface that belongs to this association.
  @param[in]  AltSetting        Interface alternate setting.
  @param[in]  Index             Zero-based index of the class specific interface.
  @param[in][out]  BufferSize   On input, the size in bytes of the return Descriptor buffer.
                                On output the size of data returned in Descriptor.
  @param[out]  Descriptor       The buffer to return the contents of the class specific interface descriptor. May
                                be NULL with a zero BufferSize in order to determine the size buffer needed.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER BufferSize is NULL.
                                Buffer is NULL and *BufferSize is not zero.
  @retval EFI_NOT_FOUND         Interface does not belong to this association.
                                AltSetting is greater than the number of alternate settings in this interface.
                                Index is greater than the number of class specific interfaces.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small for the result. BufferSize has been updated with the size
                                needed to complete the request.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_IA_GET_CS_INTERFACE_DESCRIPTOR)(
  IN  EFI_USB_IA_PROTOCOL   *This,
  IN  UINT8                 InterfaceNumber,
  IN  UINTN                 AltSetting,
  IN  UINTN                 Index,
  IN OUT UINTN              *BufferSize,
  OUT VOID                  *Buffer
  );

/**
  Retrieve class specific endpoint descriptor.

  @param[in]  This              A pointer to the EFI_USB_IA_PROTOCOL instance.
  @param[in]  InterfaceNumber   Interface that belongs to this association.
  @param[in]  AltSetting        Interface alternate setting.
  @param[in]  Index             Index of the non-zero endpoint.
  @param[in][out]  BufferSize   On input, the size in bytes of the return Descriptor buffer.
                                On output the size of data returned in Descriptor.
  @param[out]  Descriptor       The buffer to return the contents of the class specific endpoint descriptor. May
                                be NULL with a zero BufferSize in order to determine the size buffer needed.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER BufferSize is NULL.
                                Buffer is NULL and *BufferSize is not zero.
  @retval EFI_NOT_FOUND         Interface does not belong to this association.
                                AltSetting is greater than the number of alternate settings in this interface.
                                Index is greater than the number of endpoints in this interface.
                                Endpoint does not have class specific endpoint descriptor.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small for the result. BufferSize has been updated with the size
                                needed to complete the request.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_IA_GET_CS_ENDPOINT_DESCRIPTOR)(
  IN  EFI_USB_IA_PROTOCOL   *This,
  IN  UINT8                 InterfaceNumber,
  IN  UINTN                 AltSettingsNumber,
  IN  UINTN                 Index,
  IN OUT UINTN              *BufferSize,
  OUT VOID                  *Buffer
  );

struct _EFI_USB_IA_PROTOCOL {
  EFI_USB_IA_GET_ASSOCIATION_DESCRIPTOR     UsbIaGetAssociationDescriptor;
  EFI_USB_IA_GET_ASSOCIATE_ALTSETTINGS      UsbIaGetAssociateSettings;
  EFI_USB_IA_GET_INTERFACE_DESCRIPTOR       UsbIaGetInterfaceDescriptor;
  EFI_USB_IA_GET_ENDPOINT_DESCRIPTOR        UsbIaGetEndpointDescriptor;
  EFI_USB_IA_GET_CS_INTERFACE_DESCRIPTOR    UsbIaGetCsInterfaceDescriptor;
  EFI_USB_IA_GET_CS_ENDPOINT_DESCRIPTOR     UsbIaGetCsEndpointDescriptor;
};

extern EFI_GUID  gEfiUsbIaProtocolGuid;

#endif

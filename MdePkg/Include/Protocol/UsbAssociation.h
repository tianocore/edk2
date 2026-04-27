/** @file
  EFI Usb Interface Association protocol.

  This protocol is used by USB drivers, running in the EFI boot services
  environment to access USB devices that implement their configurations
  using interface association: USB cameras, USB audio devices, USB modems, etc.

  Copyright (c) 2025, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef USB_ASSOCIATION_H__
#define USB_ASSOCIATION_H__

#include <IndustryStandard/Usb.h>
#include <Protocol/UsbIo.h>

typedef USB_INTERFACE_ASSOCIATION_DESCRIPTOR EFI_USB_INTERFACE_ASSOCIATION_DESCRIPTOR;

//
// Global ID for the USB IAD Protocol
//
#define EFI_USB_INTERFACE_ASSOCIATION_PROTOCOL_GUID \
  { \
    0xf4279fb1, 0xef1e, 0x4346, { 0xab, 0x32, 0x3f, 0xe3, 0x86, 0xee, 0xb4, 0x52 } \
  }

typedef struct _EFI_USB_INTERFACE_ASSOCIATION_PROTOCOL EFI_USB_INTERFACE_ASSOCIATION_PROTOCOL;

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
  IN  EFI_USB_INTERFACE_ASSOCIATION_PROTOCOL    *This,
  OUT EFI_USB_INTERFACE_ASSOCIATION_DESCRIPTOR  *Descriptor
  );

/**
  Retrieve the details of the requested interface that belongs to USB association.

  @param[in] This              A pointer to the EFI_USB_IA_PROTOCOL instance.
  @param[in] InterfaceNumber   Interface number.
  @param[out] UsbIo            A pointer to the caller allocated UsbIo protocol.
  @param[out] SettingsCount    A pointer to the caller allocated number of settings for this interface.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER UsbIo or SettuntsCount is NULL.
  @retval EFI_NOT_FOUND         Interface does not belong to this interface association.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_IA_GET_ASSOCIATE_SETTINGS)(
  IN  EFI_USB_INTERFACE_ASSOCIATION_PROTOCOL  *This,
  IN  UINT8                                   InterfaceNumber,
  OUT EFI_USB_IO_PROTOCOL                     **UsbIo,
  OUT UINTN                                   *SettingsCount
  );

/**
  USB interface association protocol functions.

**/
struct _EFI_USB_INTERFACE_ASSOCIATION_PROTOCOL {
  EFI_USB_IA_GET_ASSOCIATION_DESCRIPTOR    UsbIaGetAssociationDescriptor;
  EFI_USB_IA_GET_ASSOCIATE_SETTINGS        UsbIaGetAssociateSettings;
};

extern EFI_GUID  gEfiUsbInterfaceAssociationProtocolGuid;

#endif

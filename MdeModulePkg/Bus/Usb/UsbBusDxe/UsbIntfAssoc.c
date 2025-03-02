/** @file

  Usb Interface Association Protocol implementation.

Copyright (c) 2025, American Megatrends International, LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbBus.h"

/**
  Get the interface from interface association.

  @param[in]  UsbIa   A pointer to the interface association data.
  @param[in]  InterfaceNumber Interface to look for inside the association.

  @retval A pointer to the interface data
  @retval NULL  Interface is not found in the association.

**/
USB_INTERFACE_DESC *
UsbAssocFindInterface (
  IN USB_ASSOCIATION  *UsbIa,
  IN UINT8            InterfaceNumber
  )
{
  UINT8  Index;
  UINT8  Intrf;

  Intrf = UsbIa->IaDesc->Desc.FirstInterface;
  for (Index = 0; Index < UsbIa->IaDesc->Desc.InterfaceCount; Index++) {
    if (InterfaceNumber == Intrf) {
      return UsbIa->IaDesc->Interfaces[Index];
    }

    Intrf++;
  }

  DEBUG ((DEBUG_ERROR, "UsbAssocFindInterface: interface 0x%x does not belong to this association\n", InterfaceNumber));
  return NULL;
}

/**
  Get the USB Interface Association Descriptor from the current USB configuration.

  @param[in] This              A pointer to the EDKII_USB_INTERFACE_ASSOCIATION_PROTOCOL instance.
  @param[out] Descriptor       A pointer to the caller allocated USB Interface Association Descriptor.

  @retval EFI_SUCCESS           The descriptor was retrieved successfully.
  @retval EFI_INVALID_PARAMETER Descriptor is NULL.

**/
EFI_STATUS
EFIAPI
UsbIaGetAssociationDescriptor (
  IN  EDKII_USB_INTERFACE_ASSOCIATION_PROTOCOL  *This,
  OUT EFI_USB_INTERFACE_ASSOCIATION_DESCRIPTOR  *Descriptor
  )
{
  USB_ASSOCIATION  *UsbIa;
  EFI_TPL          OldTpl;
  EFI_STATUS       Status;

  OldTpl = gBS->RaiseTPL (USB_BUS_TPL);

  Status = EFI_SUCCESS;

  ASSERT (Descriptor != NULL);
  if (Descriptor == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  UsbIa = USB_ASSOCIATION_FROM_USBIA (This);
  CopyMem (Descriptor, &(UsbIa->IaDesc->Desc), sizeof (EFI_USB_INTERFACE_ASSOCIATION_DESCRIPTOR));

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Retrieve the details of the requested interface that belongs to USB association.

  @param[in] This              A pointer to the EDKII_USB_INTERFACE_ASSOCIATION_PROTOCOL instance.
  @param[in] InterfaceNumber   Interface number.
  @param[out] UsbIo            A pointer to the caller allocated UsbIo protocol.
  @param[out] SettingsCount    A pointer to the caller allocated number of settings for this interface.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER UsbIo or SettuntsCount is NULL.
  @retval EFI_NOT_FOUND         Interface does not belong to this interface association.

**/
EFI_STATUS
EFIAPI
UsbIaGetAssociateSettings (
  IN  EDKII_USB_INTERFACE_ASSOCIATION_PROTOCOL  *This,
  IN  UINT8                                     InterfaceNumber,
  OUT EFI_USB_IO_PROTOCOL                       **UsbIo,
  OUT UINTN                                     *SettingsCount
  )
{
  USB_ASSOCIATION     *UsbIa;
  EFI_TPL             OldTpl;
  UINT8               Index;
  EFI_STATUS          Status;
  USB_DEVICE          *Device;
  USB_INTERFACE_DESC  *IfDesc;

  OldTpl = gBS->RaiseTPL (USB_BUS_TPL);

  ASSERT ((UsbIo != NULL) && (SettingsCount != NULL));
  if ((UsbIo == NULL) || (SettingsCount == NULL)) {
    Status =  EFI_INVALID_PARAMETER;
    goto  ON_EXIT;
  }

  UsbIa = USB_ASSOCIATION_FROM_USBIA (This);

  IfDesc = UsbAssocFindInterface (UsbIa, InterfaceNumber);
  if (IfDesc == NULL) {
    Status = EFI_NOT_FOUND;
    goto ON_EXIT;
  }

  *SettingsCount = IfDesc->NumOfSetting - 1;

  //
  // Find UsbIo protocol for this interface
  //
  Device = UsbIa->Device;
  Status = EFI_NOT_FOUND;

  for (Index = 0; Index < Device->NumOfInterface; Index++) {
    if (Device->Interfaces[Index]->IfSetting->Desc.InterfaceNumber == InterfaceNumber) {
      *UsbIo = &Device->Interfaces[Index]->UsbIo;
      Status = EFI_SUCCESS;
      break;
    }
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

EDKII_USB_INTERFACE_ASSOCIATION_PROTOCOL  mUsbIaProtocol = {
  UsbIaGetAssociationDescriptor,
  UsbIaGetAssociateSettings
};

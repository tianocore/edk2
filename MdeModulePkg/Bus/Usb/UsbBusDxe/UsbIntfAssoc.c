/** @file

  Usb Interface Association Protocol implementation.

Copyright (c) 2024, American Megatrends International LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbIntfAssoc.h"

EFI_USB_IA_PROTOCOL  mUsbIaProtocol = {
  UsbIaGetAssociationDescriptor,
  UsbIaGetAssociateAltSettings,
  UsbIaGetInterfaceDescriptor,
  UsbIaGetEndpointDescriptor,
  UsbIaGetCsInterfaceDescriptor,
  UsbIaGetCsEndpointDescriptor
};

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

  @param[in] This              A pointer to the EFI_USB_IA_PROTOCOL instance.
  @param[out] Descriptor       A pointer to the caller allocated USB Interface Association Descriptor.

  @retval EFI_SUCCESS           The descriptor was retrieved successfully.
  @retval EFI_INVALID_PARAMETER Descriptor is NULL.

**/
EFI_STATUS
EFIAPI
UsbIaGetAssociationDescriptor (
  IN  EFI_USB_IA_PROTOCOL                       *This,
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

  @param[in]  This              A pointer to the EFI_USB_IA_PROTOCOL instance.
  @param[in]  InterfaceNumber   Interface that belongs to this association.
  @param[out]  UsbIo            A pointer to the caller allocated UsbIo protocol.
  @param[out]  AltSettingsNum   Number of alternate settings of this interface.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER UsbIo or AltSettingsNum is NULL.
  @retval EFI_NOT_FOUND         InterfaceNumber does not belong to this interface association.

**/
EFI_STATUS
EFIAPI
UsbIaGetAssociateAltSettings (
  IN  EFI_USB_IA_PROTOCOL  *This,
  IN  UINT8                InterfaceNumber,
  OUT EFI_USB_IO_PROTOCOL  **UsbIo,
  OUT UINTN                *AltSettingsNumber
  )
{
  USB_ASSOCIATION     *UsbIa;
  EFI_TPL             OldTpl;
  UINT8               Index;
  EFI_STATUS          Status;
  USB_DEVICE          *Device;
  USB_INTERFACE_DESC  *IfDesc;

  OldTpl = gBS->RaiseTPL (USB_BUS_TPL);

  ASSERT ((UsbIo != NULL) && (AltSettingsNumber != NULL));
  if ((UsbIo == NULL) || (AltSettingsNumber == NULL)) {
    Status =  EFI_INVALID_PARAMETER;
    goto  ON_EXIT;
  }

  UsbIa = USB_ASSOCIATION_FROM_USBIA (This);

  IfDesc = UsbAssocFindInterface (UsbIa, InterfaceNumber);
  if (IfDesc == NULL) {
    Status = EFI_NOT_FOUND;
    goto ON_EXIT;
  }

  *AltSettingsNumber = IfDesc->NumOfSetting - 1;

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
EFI_STATUS
EFIAPI
UsbIaGetInterfaceDescriptor (
  IN  EFI_USB_IA_PROTOCOL           *This,
  IN  UINT8                         InterfaceNumber,
  IN  UINTN                         AltSetting,
  OUT EFI_USB_INTERFACE_DESCRIPTOR  *Descriptor,
  OUT UINTN                         *CsInterfacesNumber
  )
{
  EFI_TPL             OldTpl;
  EFI_STATUS          Status;
  USB_ASSOCIATION     *UsbIa;
  USB_INTERFACE_DESC  *IfDesc;

  OldTpl = gBS->RaiseTPL (USB_BUS_TPL);

  Status = EFI_SUCCESS;

  ASSERT ((Descriptor != NULL) && (CsInterfacesNumber != NULL));
  if ((Descriptor == NULL) || (CsInterfacesNumber == NULL)) {
    Status =  EFI_INVALID_PARAMETER;
    goto  ON_EXIT;
  }

  UsbIa  = USB_ASSOCIATION_FROM_USBIA (This);
  IfDesc = UsbAssocFindInterface (UsbIa, InterfaceNumber);

  if ((IfDesc == NULL) || (AltSetting > (IfDesc->NumOfSetting - 1))) {
    Status = EFI_NOT_FOUND;
    goto ON_EXIT;
  }

  CopyMem (Descriptor, &(IfDesc->Settings[AltSetting]->Desc), sizeof (EFI_USB_INTERFACE_DESCRIPTOR));
  *CsInterfacesNumber = IfDesc->Settings[AltSetting]->NumOfCsDesc;

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

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
EFI_STATUS
EFIAPI
UsbIaGetEndpointDescriptor (
  IN  EFI_USB_IA_PROTOCOL          *This,
  IN  UINT8                        InterfaceNumber,
  IN  UINTN                        AltSetting,
  IN  UINTN                        Index,
  OUT EFI_USB_ENDPOINT_DESCRIPTOR  *Descriptor
  )
{
  EFI_TPL             OldTpl;
  EFI_STATUS          Status;
  USB_ASSOCIATION     *UsbIa;
  USB_INTERFACE_DESC  *IfDesc;

  OldTpl = gBS->RaiseTPL (USB_BUS_TPL);

  Status = EFI_SUCCESS;

  if (Descriptor == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  UsbIa  = USB_ASSOCIATION_FROM_USBIA (This);
  IfDesc = UsbAssocFindInterface (UsbIa, InterfaceNumber);

  if ((IfDesc == NULL) || (AltSetting > (IfDesc->NumOfSetting - 1))) {
    Status = EFI_NOT_FOUND;
    goto ON_EXIT;
  }

  CopyMem (Descriptor, &(IfDesc->Settings[AltSetting]->Endpoints[Index]->Desc), sizeof (EFI_USB_ENDPOINT_DESCRIPTOR));

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

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
EFI_STATUS
EFIAPI
UsbIaGetCsInterfaceDescriptor (
  IN  EFI_USB_IA_PROTOCOL  *This,
  IN  UINT8                InterfaceNumber,
  IN  UINTN                AltSetting,
  IN  UINTN                Index,
  IN OUT UINTN             *BufferSize,
  OUT VOID                 *Buffer
  )
{
  EFI_TPL             OldTpl;
  EFI_STATUS          Status;
  USB_ASSOCIATION     *UsbIa;
  USB_INTERFACE_DESC  *IfDesc;
  UINT8               *Desc;
  UINT8               DescLength;

  OldTpl = gBS->RaiseTPL (USB_BUS_TPL);
  Status = EFI_SUCCESS;

  if (  (BufferSize == NULL)
     || ((Buffer == NULL) && (*BufferSize != 0)))
  {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  UsbIa  = USB_ASSOCIATION_FROM_USBIA (This);
  IfDesc = UsbAssocFindInterface (UsbIa, InterfaceNumber);

  if (  (IfDesc == NULL)
     || (AltSetting > (IfDesc->NumOfSetting - 1))
     || ((Index + 1) > IfDesc->Settings[AltSetting]->NumOfCsDesc))
  {
    Status = EFI_NOT_FOUND;
    goto ON_EXIT;
  }

  Desc       = IfDesc->Settings[AltSetting]->CsDesc[Index];
  DescLength = Desc[0];

  *BufferSize = DescLength;
  if (  (Buffer == NULL)
     || (DescLength > *BufferSize))
  {
    Status = EFI_BUFFER_TOO_SMALL;
    goto ON_EXIT;
  }

  CopyMem (Buffer, Desc, DescLength);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

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
EFI_STATUS
EFIAPI
UsbIaGetCsEndpointDescriptor (
  IN  EFI_USB_IA_PROTOCOL  *This,
  IN  UINT8                InterfaceNumber,
  IN  UINTN                AltSetting,
  IN  UINTN                Index,
  IN OUT UINTN             *BufferSize,
  OUT VOID                 *Buffer
  )
{
  EFI_TPL             OldTpl;
  EFI_STATUS          Status;
  USB_ASSOCIATION     *UsbIa;
  USB_INTERFACE_DESC  *IfDesc;
  UINT8               *Desc;
  UINT8               DescLength;

  OldTpl = gBS->RaiseTPL (USB_BUS_TPL);
  Status = EFI_SUCCESS;

  if (  (BufferSize == NULL)
     || ((Buffer == NULL) && (*BufferSize != 0)))
  {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  UsbIa  = USB_ASSOCIATION_FROM_USBIA (This);
  IfDesc = UsbAssocFindInterface (UsbIa, InterfaceNumber);

  if (  (IfDesc == NULL)
     || (AltSetting > (IfDesc->NumOfSetting - 1))
     || ((Index + 1) > IfDesc->Settings[AltSetting]->Desc.NumEndpoints)
     || (IfDesc->Settings[AltSetting]->Endpoints[Index]->CsDesc == NULL))
  {
    Status = EFI_NOT_FOUND;
    goto ON_EXIT;
  }

  Desc       = IfDesc->Settings[AltSetting]->Endpoints[Index]->CsDesc;
  DescLength = Desc[0];

  *BufferSize = DescLength;
  if (  (Buffer == NULL)
     || (DescLength > *BufferSize))
  {
    Status = EFI_BUFFER_TOO_SMALL;
    goto ON_EXIT;
  }

  CopyMem (Buffer, Desc, DescLength);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

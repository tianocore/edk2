/** @file

  UsbIo Extension Protocol Protocol implementation.

Copyright (c) 2024, American Megatrends International LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbIoExtension.h"

EDKII_USBIO_EXT_PROTOCOL  mUsbIoExtProtocol = {
  UsbIoExtGetInterfaceDescriptor,
  UsbIoExtGetEndpointDescriptor,
  UsbIoExtGetCsInterfaceDescriptor,
  UsbIoExtGetCsEndpointDescriptor
};

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
EFI_STATUS
EFIAPI
UsbIoExtGetInterfaceDescriptor (
  IN  EDKII_USBIO_EXT_PROTOCOL      *This,
  IN  UINTN                         AltSetting,
  OUT EFI_USB_INTERFACE_DESCRIPTOR  *Descriptor,
  OUT UINTN                         *CsInterfacesNumber
  )
{
  EFI_TPL        OldTpl;
  EFI_STATUS     Status;
  USB_INTERFACE  *UsbIf;

  OldTpl = gBS->RaiseTPL (USB_BUS_TPL);

  Status = EFI_SUCCESS;

  ASSERT ((Descriptor != NULL) && (CsInterfacesNumber != NULL));
  if ((Descriptor == NULL) || (CsInterfacesNumber == NULL)) {
    Status =  EFI_INVALID_PARAMETER;
    goto  ON_EXIT;
  }

  UsbIf = USB_INTERFACE_FROM_USBIO_EXT (This);

  if (AltSetting > (UsbIf->IfDesc->NumOfSetting - 1)) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  CopyMem (
    Descriptor,
    &(UsbIf->IfDesc->Settings[AltSetting]->Desc),
    sizeof (EFI_USB_INTERFACE_DESCRIPTOR)
    );
  *CsInterfacesNumber = UsbIf->IfDesc->Settings[AltSetting]->NumOfCsDesc;

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

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
EFI_STATUS
EFIAPI
UsbIoExtGetEndpointDescriptor (
  IN  EDKII_USBIO_EXT_PROTOCOL     *This,
  IN  UINTN                        AltSetting,
  IN  UINTN                        Index,
  OUT EFI_USB_ENDPOINT_DESCRIPTOR  *Descriptor
  )
{
  EFI_TPL        OldTpl;
  EFI_STATUS     Status;
  USB_INTERFACE  *UsbIf;

  OldTpl = gBS->RaiseTPL (USB_BUS_TPL);

  Status = EFI_SUCCESS;

  if (Descriptor == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  UsbIf = USB_INTERFACE_FROM_USBIO_EXT (This);

  if (AltSetting > (UsbIf->IfDesc->NumOfSetting - 1)) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  if (Index >= UsbIf->IfDesc->Settings[AltSetting]->Desc.NumEndpoints) {
    gBS->RestoreTPL (OldTpl);
    return EFI_NOT_FOUND;
  }

  CopyMem (
    Descriptor,
    &(UsbIf->IfDesc->Settings[AltSetting]->Endpoints[Index]->Desc),
    sizeof (EFI_USB_ENDPOINT_DESCRIPTOR)
    );

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

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
EFI_STATUS
EFIAPI
UsbIoExtGetCsInterfaceDescriptor (
  IN  EDKII_USBIO_EXT_PROTOCOL  *This,
  IN  UINTN                     AltSetting,
  IN  UINTN                     Index,
  IN OUT UINTN                  *BufferSize,
  OUT VOID                      *Buffer
  )
{
  EFI_TPL        OldTpl;
  EFI_STATUS     Status;
  USB_INTERFACE  *UsbIf;
  UINT8          *Desc;
  UINT8          DescLength;

  OldTpl = gBS->RaiseTPL (USB_BUS_TPL);
  Status = EFI_SUCCESS;

  if (  (BufferSize == NULL)
     || ((Buffer == NULL) && (*BufferSize != 0)))
  {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  UsbIf = USB_INTERFACE_FROM_USBIO_EXT (This);

  if (AltSetting > (UsbIf->IfDesc->NumOfSetting - 1)) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  if (Index >= UsbIf->IfDesc->Settings[AltSetting]->NumOfCsDesc) {
    Status = EFI_NOT_FOUND;
    goto ON_EXIT;
  }

  Desc       = UsbIf->IfDesc->Settings[AltSetting]->CsDesc[Index];
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
EFI_STATUS
EFIAPI
UsbIoExtGetCsEndpointDescriptor (
  IN  EDKII_USBIO_EXT_PROTOCOL  *This,
  IN  UINTN                     AltSetting,
  IN  UINTN                     Index,
  IN OUT UINTN                  *BufferSize,
  OUT VOID                      *Buffer
  )
{
  EFI_TPL        OldTpl;
  EFI_STATUS     Status;
  USB_INTERFACE  *UsbIf;
  UINT8          *Desc;
  UINT8          DescLength;

  OldTpl = gBS->RaiseTPL (USB_BUS_TPL);
  Status = EFI_SUCCESS;

  if (  (BufferSize == NULL)
     || ((Buffer == NULL) && (*BufferSize != 0)))
  {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  UsbIf = USB_INTERFACE_FROM_USBIO_EXT (This);

  if (AltSetting > (UsbIf->IfDesc->NumOfSetting - 1)) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  if (  (Index >= UsbIf->IfDesc->Settings[AltSetting]->Desc.NumEndpoints)
     || (UsbIf->IfDesc->Settings[AltSetting]->Endpoints[Index]->CsDesc == NULL))
  {
    Status = EFI_NOT_FOUND;
    goto ON_EXIT;
  }

  Desc       = UsbIf->IfDesc->Settings[AltSetting]->Endpoints[Index]->CsDesc;
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

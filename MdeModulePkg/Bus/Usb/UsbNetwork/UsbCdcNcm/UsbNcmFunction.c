/** @file
  This file contains code for USB Ethernet descriptor
  and specific requests implement.

  Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UsbCdcNcm.h"

/**
  Load All of device descriptor.

  @param[in]  UsbIo                 A pointer to the EFI_USB_IO_PROTOCOL instance.
  @param[out] ConfigDesc            A pointer to the configuration descriptor.

  @retval EFI_SUCCESS           The request executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed because the
                                buffer specified by DescriptorLength and Descriptor
                                is not large enough to hold the result of the request.
  @retval EFI_TIMEOUT           A timeout occurred executing the request.
  @retval EFI_DEVICE_ERROR      The request failed due to a device error. The transfer
                                status is returned in Status.

**/
EFI_STATUS
LoadAllDescriptor (
  IN  EFI_USB_IO_PROTOCOL        *UsbIo,
  OUT EFI_USB_CONFIG_DESCRIPTOR  **ConfigDesc
  )
{
  EFI_STATUS                 Status;
  UINT32                     TransStatus;
  EFI_USB_CONFIG_DESCRIPTOR  Tmp;

  Status = UsbIo->UsbGetConfigDescriptor (UsbIo, &Tmp);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->AllocatePool (EfiBootServicesData, Tmp.TotalLength, (VOID **)ConfigDesc);
  ASSERT_EFI_ERROR (Status);

  Status = UsbGetDescriptor (
             UsbIo,
             USB_DESC_TYPE_CONFIG << 8 | (Tmp.ConfigurationValue - 1),                   // zero based
             0,
             Tmp.TotalLength,
             *ConfigDesc,
             &TransStatus
             );
  return Status;
}

/**
  Returns pointer to the next descriptor for the pack of USB descriptors
  located in continues memory segment

  @param[in]      Desc   A pointer to the CONFIG_DESCRIPTOR instance.
  @param[in, out] Offset A pointer to the sum of descriptor length.

  @retval TRUE   The request executed successfully.
  @retval FALSE  No next descriptor.

**/
BOOLEAN
NextDescriptor (
  IN EFI_USB_CONFIG_DESCRIPTOR  *Desc,
  IN OUT UINTN                  *Offset
  )
{
  if ((Desc == NULL) || (*Offset >= Desc->TotalLength)) {
    return FALSE;
  }

  if (((EFI_USB_CONFIG_DESCRIPTOR *)((char *)Desc+*Offset))->Length == 0) {
    return FALSE;
  }

  *Offset += ((EFI_USB_CONFIG_DESCRIPTOR *)((char *)Desc+*Offset))->Length;
  if ( *Offset >= Desc->TotalLength ) {
    return FALSE;
  }

  return TRUE;
}

/**
  Read Function descriptor

  @param[in]  Config             A pointer to all of configuration.
  @param[in]  FunDescriptorType  USB CDC class descriptor SubType.
  @param[out] DataBuffer         A pointer to the Data of corresponding to device capability.

  @retval EFI_SUCCESS        The device capability descriptor was retrieved
                             successfully.
  @retval EFI_UNSUPPORTED    No supported.
  @retval EFI_NOT_FOUND      The device capability descriptor was not found.

**/
EFI_STATUS
GetFunctionalDescriptor (
  IN  EFI_USB_CONFIG_DESCRIPTOR  *Config,
  IN  UINT8                      FunDescriptorType,
  OUT VOID                       *DataBuffer
  )
{
  EFI_STATUS                    Status;
  UINTN                         Offset;
  EFI_USB_INTERFACE_DESCRIPTOR  *Interface;

  Status = EFI_NOT_FOUND;

  for (Offset = 0; NextDescriptor (Config, &Offset);) {
    Interface = (EFI_USB_INTERFACE_DESCRIPTOR *)((UINT8 *)Config + Offset);
    if (Interface->DescriptorType == CS_INTERFACE) {
      if (((USB_HEADER_FUN_DESCRIPTOR *)Interface)->DescriptorSubtype == FunDescriptorType) {
        switch (FunDescriptorType) {
          case HEADER_FUN_DESCRIPTOR:
            CopyMem (
              DataBuffer,
              (USB_HEADER_FUN_DESCRIPTOR *)Interface,
              sizeof (USB_HEADER_FUN_DESCRIPTOR)
              );
            return EFI_SUCCESS;
          case UNION_FUN_DESCRIPTOR:
            CopyMem (
              DataBuffer,
              (USB_UNION_FUN_DESCRIPTOR *)Interface,
              ((USB_UNION_FUN_DESCRIPTOR *)Interface)->FunctionLength
              );
            return EFI_SUCCESS;
          case ETHERNET_FUN_DESCRIPTOR:
            CopyMem (
              DataBuffer,
              (USB_ETHERNET_FUN_DESCRIPTOR *)Interface,
              sizeof (USB_ETHERNET_FUN_DESCRIPTOR)
              );
            return EFI_SUCCESS;
          default:
            Status = EFI_UNSUPPORTED;
            break;
        }
      }
    }
  }

  return Status;
}

/**
  Get USB Ethernet IO endpoint and USB CDC data IO endpoint.

  @param[in]      UsbIo         A pointer to the EFI_USB_IO_PROTOCOL instance.
  @param[in, out] UsbEthDriver  A pointer to the USB_ETHERNET_DRIVER instance.

**/
VOID
GetEndpoint (
  IN      EFI_USB_IO_PROTOCOL  *UsbIo,
  IN OUT  USB_ETHERNET_DRIVER  *UsbEthDriver
  )
{
  EFI_STATUS                    Status;
  UINT8                         Index;
  UINT32                        Result;
  EFI_USB_INTERFACE_DESCRIPTOR  Interface;
  EFI_USB_ENDPOINT_DESCRIPTOR   Endpoint;

  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &Interface);
  ASSERT_EFI_ERROR (Status);

  if (Interface.NumEndpoints == 0) {
    Status = UsbSetInterface (UsbIo, Interface.InterfaceNumber, 1, &Result);
    ASSERT_EFI_ERROR (Status);

    Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &Interface);
    ASSERT_EFI_ERROR (Status);
  }

  for (Index = 0; Index < Interface.NumEndpoints; Index++) {
    Status = UsbIo->UsbGetEndpointDescriptor (UsbIo, Index, &Endpoint);
    ASSERT_EFI_ERROR (Status);

    switch ((Endpoint.Attributes & (BIT0 | BIT1))) {
      case USB_ENDPOINT_BULK:
        if (Endpoint.EndpointAddress & BIT7) {
          UsbEthDriver->BulkInEndpoint = Endpoint.EndpointAddress;
        } else {
          UsbEthDriver->BulkOutEndpoint = Endpoint.EndpointAddress;
        }

        break;
      case USB_ENDPOINT_INTERRUPT:
        UsbEthDriver->InterruptEndpoint = Endpoint.EndpointAddress;
        break;
    }
  }
}

/**
  This function is used to manage a USB device with the bulk transfer pipe. The endpoint is Bulk in.

  @param[in]      Cdb           A pointer to the command descriptor block.
  @param[in]      This          A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[in, out] Packet        A pointer to the buffer of data that will be transmitted to USB
                                device or received from USB device.
  @param[in, out] PacketLength  A pointer to the PacketLength.

  @retval EFI_SUCCESS           The bulk transfer has been successfully executed.
  @retval EFI_DEVICE_ERROR      The transfer failed. The transfer status is returned in status.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be submitted due to a lack of resources.
  @retval EFI_TIMEOUT           The control transfer fails due to timeout.

**/
EFI_STATUS
EFIAPI
UsbEthNcmReceive (
  IN     PXE_CDB                      *Cdb,
  IN     EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN OUT VOID                         *Packet,
  IN OUT UINTN                        *PacketLength
  )
{
  EFI_STATUS                   Status;
  USB_ETHERNET_DRIVER          *UsbEthDriver;
  EFI_USB_IO_PROTOCOL          *UsbIo;
  UINT32                       TransStatus;
  UINT8                        Index;
  UINTN                        BulkDataLength;
  UINTN                        TotalLength;
  USB_NCM_TRANSFER_HEADER_16   *Nth;
  USB_NCM_DATAGRAM_POINTER_16  *Ndp;
  USB_NCM_DATA_GRAM            *Datagram;

  UsbEthDriver = USB_ETHERNET_DEV_FROM_THIS (This);
  TotalLength  = 0;

  if (UsbEthDriver->TotalDatagram == UsbEthDriver->NowDatagram) {
    Status = gBS->HandleProtocol (
                    UsbEthDriver->UsbCdcDataHandle,
                    &gEfiUsbIoProtocolGuid,
                    (VOID **)&UsbIo
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (UsbEthDriver->BulkInEndpoint == 0) {
      GetEndpoint (UsbIo, UsbEthDriver);
    }

    BulkDataLength = USB_NCM_MAX_NTB_SIZE;
    SetMem (UsbEthDriver->BulkBuffer, BulkDataLength, 0);
    UsbEthDriver->NowDatagram   = 0;
    UsbEthDriver->TotalDatagram = 0;

    Status = UsbIo->UsbBulkTransfer (
                      UsbIo,
                      UsbEthDriver->BulkInEndpoint,
                      UsbEthDriver->BulkBuffer,
                      &BulkDataLength,
                      USB_ETHERNET_BULK_TIMEOUT,
                      &TransStatus
                      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Nth                         = (USB_NCM_TRANSFER_HEADER_16 *)UsbEthDriver->BulkBuffer;
    Ndp                         = (USB_NCM_DATAGRAM_POINTER_16 *)((UINT8 *)UsbEthDriver->BulkBuffer + Nth->NdpIndex);
    Datagram                    = (USB_NCM_DATA_GRAM *)((UINT8 *)Ndp + sizeof (USB_NCM_DATAGRAM_POINTER_16));
    UsbEthDriver->TotalDatagram = (UINT8)((Ndp->Length - 8) / 4 - 1);

    for (Index = 0; Index < UsbEthDriver->TotalDatagram; Index++) {
      TotalLength += Datagram->DatagramLength;
      Datagram     = (USB_NCM_DATA_GRAM *)((UINT8 *)Datagram + sizeof (USB_NCM_DATA_GRAM));
    }

    if (TotalLength < USB_ETHERNET_FRAME_SIZE) {
      Datagram = (USB_NCM_DATA_GRAM *)((UINT8 *)Ndp + sizeof (USB_NCM_DATAGRAM_POINTER_16));

      TotalLength = 0;
      for (Index = 0; Index < UsbEthDriver->TotalDatagram; Index++) {
        CopyMem ((UINT8 *)Packet + TotalLength, (UINT8 *)UsbEthDriver->BulkBuffer + Datagram->DatagramIndex, Datagram->DatagramLength);
        TotalLength += Datagram->DatagramLength;
        Datagram     = (USB_NCM_DATA_GRAM *)((UINT8 *)Datagram + sizeof (USB_NCM_DATA_GRAM));
      }

      *PacketLength             = TotalLength;
      UsbEthDriver->NowDatagram = UsbEthDriver->TotalDatagram;
    } else {
      UsbEthDriver->NowDatagram++;

      Datagram = (USB_NCM_DATA_GRAM *)((UINT8 *)Ndp + sizeof (USB_NCM_DATAGRAM_POINTER_16));
      CopyMem (Packet, (UINT8 *)UsbEthDriver->BulkBuffer + Datagram->DatagramIndex, Datagram->DatagramLength);
      *PacketLength = Datagram->DatagramLength;
    }

    return Status;
  } else {
    UsbEthDriver->NowDatagram++;

    Nth      = (USB_NCM_TRANSFER_HEADER_16 *)UsbEthDriver->BulkBuffer;
    Ndp      = (USB_NCM_DATAGRAM_POINTER_16 *)((UINT8 *)UsbEthDriver->BulkBuffer + Nth->NdpIndex);
    Datagram = (USB_NCM_DATA_GRAM *)((UINT8 *)Ndp + sizeof (USB_NCM_DATAGRAM_POINTER_16));
    Datagram = (USB_NCM_DATA_GRAM *)((UINT8 *)Datagram + sizeof (USB_NCM_DATA_GRAM) * (UsbEthDriver->NowDatagram - 1));

    CopyMem (Packet, (UINT8 *)UsbEthDriver->BulkBuffer + Datagram->DatagramIndex, Datagram->DatagramLength);
    *PacketLength = Datagram->DatagramLength;

    return EFI_SUCCESS;
  }
}

/**
  This function is used to manage a USB device with the bulk transfer pipe. The endpoint is Bulk out.

  @param[in]      Cdb           A pointer to the command descriptor block.
  @param[in]      This          A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[in]      Packet        A pointer to the buffer of data that will be transmitted to USB
                                device or received from USB device.
  @param[in, out] PacketLength  A pointer to the PacketLength.

  @retval EFI_SUCCESS           The bulk transfer has been successfully executed.
  @retval EFI_DEVICE_ERROR      The transfer failed. The transfer status is returned in status.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be submitted due to a lack of resources.
  @retval EFI_TIMEOUT           The control transfer fails due to timeout.

**/
EFI_STATUS
EFIAPI
UsbEthNcmTransmit (
  IN      PXE_CDB                      *Cdb,
  IN      EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN      VOID                         *Packet,
  IN OUT  UINTN                        *PacketLength
  )
{
  EFI_STATUS                   Status;
  USB_ETHERNET_DRIVER          *UsbEthDriver;
  EFI_USB_IO_PROTOCOL          *UsbIo;
  UINT32                       TransStatus;
  USB_NCM_TRANSFER_HEADER_16   *Nth;
  USB_NCM_DATAGRAM_POINTER_16  *Ndp;
  USB_NCM_DATA_GRAM            *Datagram;
  UINT8                        *TotalPacket;
  UINTN                        TotalLength;

  UsbEthDriver = USB_ETHERNET_DEV_FROM_THIS (This);

  Status = gBS->HandleProtocol (
                  UsbEthDriver->UsbCdcDataHandle,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (UsbEthDriver->BulkOutEndpoint == 0) {
    GetEndpoint (UsbIo, UsbEthDriver);
  }

  TotalLength = (UINTN)(USB_NCM_NTH_LENGTH + USB_NCM_NDP_LENGTH + (*PacketLength));

  Status = gBS->AllocatePool (EfiBootServicesData, TotalLength, (VOID **)&TotalPacket);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SetMem (TotalPacket, TotalLength, 0);

  Nth               = (USB_NCM_TRANSFER_HEADER_16 *)TotalPacket;
  Nth->Signature    = USB_NCM_NTH_SIGN_16;
  Nth->HeaderLength = USB_NCM_NTH_LENGTH;
  Nth->Sequence     = UsbEthDriver->BulkOutSequence++;
  Nth->BlockLength  = (UINT16)TotalLength;
  Nth->NdpIndex     = Nth->HeaderLength;

  Ndp               = (USB_NCM_DATAGRAM_POINTER_16 *)((UINT8 *)TotalPacket + Nth->NdpIndex);
  Ndp->Signature    = USB_NCM_NDP_SIGN_16;
  Ndp->Length       = USB_NCM_NDP_LENGTH;
  Ndp->NextNdpIndex = 0x00;

  Datagram                 = (USB_NCM_DATA_GRAM *)((UINT8 *)Ndp + sizeof (USB_NCM_DATAGRAM_POINTER_16));
  Datagram->DatagramIndex  = Nth->HeaderLength + Ndp->Length;
  Datagram->DatagramLength = (UINT16)*PacketLength;

  CopyMem (TotalPacket + Datagram->DatagramIndex, Packet, *PacketLength);

  *PacketLength = TotalLength;

  Status = UsbIo->UsbBulkTransfer (
                    UsbIo,
                    UsbEthDriver->BulkOutEndpoint,
                    TotalPacket,
                    PacketLength,
                    USB_ETHERNET_BULK_TIMEOUT,
                    &TransStatus
                    );
  FreePool (TotalPacket);
  return Status;
}

/**
  Async USB transfer callback routine.

  @param[in]  Data            Data received or sent via the USB Asynchronous Transfer, if the
                              transfer completed successfully.
  @param[in]  DataLength      The length of Data received or sent via the Asynchronous
                              Transfer, if transfer successfully completes.
  @param[in]  Context         Data passed from UsbAsyncInterruptTransfer() request.
  @param[in]  Status          Indicates the result of the asynchronous transfer.

  @retval EFI_SUCCESS           The asynchronous USB transfer request has been successfully executed.
  @retval EFI_DEVICE_ERROR      The asynchronous USB transfer request failed.

**/
EFI_STATUS
EFIAPI
InterruptCallback (
  IN  VOID    *Data,
  IN  UINTN   DataLength,
  IN  VOID    *Context,
  IN  UINT32  Status
  )
{
  if ((Data == NULL) || (Context == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (((EFI_USB_DEVICE_REQUEST *)Data)->Request == USB_CDC_NETWORK_CONNECTION) {
    CopyMem (
      (EFI_USB_DEVICE_REQUEST *)Context,
      (EFI_USB_DEVICE_REQUEST *)Data,
      sizeof (EFI_USB_DEVICE_REQUEST)
      );
  }

  return EFI_SUCCESS;
}

/**
  This function is used to manage a USB device with an interrupt transfer pipe.

  @param[in]  This              A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[in]  IsNewTransfer     If TRUE, a new transfer will be submitted to USB controller. If
                                FALSE, the interrupt transfer is deleted from the device's interrupt
                                transfer queue.
  @param[in]  PollingInterval   Indicates the periodic rate, in milliseconds, that the transfer is to be
                                executed.This parameter is required when IsNewTransfer is TRUE. The
                                value must be between 1 to 255, otherwise EFI_INVALID_PARAMETER is returned.
                                The units are in milliseconds.
  @param[in]  Request           A pointer to the EFI_USB_DEVICE_REQUEST data.

  @retval EFI_SUCCESS           The asynchronous USB transfer request transfer has been successfully executed.
  @retval EFI_DEVICE_ERROR      The asynchronous USB transfer request failed.

**/
EFI_STATUS
EFIAPI
UsbEthNcmInterrupt (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN BOOLEAN                      IsNewTransfer,
  IN UINTN                        PollingInterval,
  IN EFI_USB_DEVICE_REQUEST       *Request
  )
{
  EFI_STATUS           Status;
  USB_ETHERNET_DRIVER  *UsbEthDriver;
  UINTN                DataLength;

  UsbEthDriver = USB_ETHERNET_DEV_FROM_THIS (This);
  DataLength   = 0;

  if (IsNewTransfer) {
    DataLength = sizeof (EFI_USB_DEVICE_REQUEST) + sizeof (USB_CONNECT_SPEED_CHANGE);
    Status     = UsbEthDriver->UsbIo->UsbAsyncInterruptTransfer (
                                        UsbEthDriver->UsbIo,
                                        UsbEthDriver->InterruptEndpoint,
                                        IsNewTransfer,
                                        PollingInterval,
                                        DataLength,
                                        (EFI_ASYNC_USB_TRANSFER_CALLBACK)InterruptCallback,
                                        Request
                                        );
  } else {
    Status = UsbEthDriver->UsbIo->UsbAsyncInterruptTransfer (
                                    UsbEthDriver->UsbIo,
                                    UsbEthDriver->InterruptEndpoint,
                                    IsNewTransfer,
                                    0,
                                    0,
                                    NULL,
                                    NULL
                                    );
  }

  return Status;
}

/**
  Retrieves the USB Ethernet Mac Address.

  @param[in]  This          A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[out] MacAddress    A pointer to the caller allocated USB Ethernet Mac Address.

  @retval EFI_SUCCESS           The USB Header Functional descriptor was retrieved successfully.
  @retval EFI_INVALID_PARAMETER UsbHeaderFunDescriptor is NULL.
  @retval EFI_NOT_FOUND         The USB Header Functional descriptor was not found.

**/
EFI_STATUS
EFIAPI
GetUsbEthMacAddress (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT EFI_MAC_ADDRESS              *MacAddress
  )
{
  EFI_STATUS                   Status;
  USB_ETHERNET_DRIVER          *UsbEthDriver;
  USB_ETHERNET_FUN_DESCRIPTOR  UsbEthDescriptor;
  CHAR16                       *Data;
  CHAR16                       *DataPtr;
  CHAR16                       TmpStr[1];
  UINT8                        Index;
  UINT8                        Hi;
  UINT8                        Low;

  UsbEthDriver = USB_ETHERNET_DEV_FROM_THIS (This);

  Status = This->UsbEthFunDescriptor (This, &UsbEthDescriptor);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:UsbEthFunDescriptor status = %r\n", __func__, Status));
    return Status;
  }

  Status = UsbEthDriver->UsbIo->UsbGetStringDescriptor (
                                  UsbEthDriver->UsbIo,
                                  0x409,                        // English-US Language ID
                                  UsbEthDescriptor.MacAddress,
                                  &Data
                                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:UsbGetStringDescriptor status = %r\n", __func__, Status));
    return Status;
  }

  DataPtr = Data;
  for (Index = 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
    CopyMem (TmpStr, DataPtr, sizeof (CHAR16));
    DataPtr++;
    Hi = (UINT8)StrHexToUintn (TmpStr);
    CopyMem (TmpStr, DataPtr, sizeof (CHAR16));
    DataPtr++;
    Low                     = (UINT8)StrHexToUintn (TmpStr);
    MacAddress->Addr[Index] = (Hi << 4) | Low;
  }

  return Status;
}

/**
  Get the USB NCM max NTB size.

  @param[in]  This          A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[out] BulkSize      A pointer to the Bulk transfer data size.

  @retval EFI_SUCCESS           Get the USB NCM max NTB size successfully.

**/
EFI_STATUS
EFIAPI
UsbEthNcmBulkSize (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT UINTN                        *BulkSize
  )
{
  *BulkSize = USB_NCM_MAX_NTB_SIZE;
  return EFI_SUCCESS;
}

/**
  Retrieves the USB Header functional Descriptor.

  @param[in]  This                   A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[out] UsbHeaderFunDescriptor A pointer to the caller allocated USB Header Functional Descriptor.

  @retval EFI_SUCCESS           The USB Header Functional descriptor was retrieved successfully.
  @retval EFI_INVALID_PARAMETER UsbHeaderFunDescriptor is NULL.
  @retval EFI_NOT_FOUND         The USB Header Functional descriptor was not found.

**/
EFI_STATUS
EFIAPI
GetUsbHeaderFunDescriptor (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT USB_HEADER_FUN_DESCRIPTOR    *UsbHeaderFunDescriptor
  )
{
  EFI_STATUS           Status;
  USB_ETHERNET_DRIVER  *UsbEthDriver;

  UsbEthDriver = USB_ETHERNET_DEV_FROM_THIS (This);

  if (UsbHeaderFunDescriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetFunctionalDescriptor (UsbEthDriver->Config, HEADER_FUN_DESCRIPTOR, UsbHeaderFunDescriptor);
  return Status;
}

/**
  Retrieves the USB Union functional Descriptor.

  @param[in]  This                   A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[out] UsbUnionFunDescriptor  A pointer to the caller allocated USB Union Functional Descriptor.

  @retval EFI_SUCCESS           The USB Union Functional descriptor was retrieved successfully.
  @retval EFI_INVALID_PARAMETER UsbUnionFunDescriptor is NULL.
  @retval EFI_NOT_FOUND         The USB Union Functional descriptor was not found.

**/
EFI_STATUS
EFIAPI
GetUsbUnionFunDescriptor (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT USB_UNION_FUN_DESCRIPTOR     *UsbUnionFunDescriptor
  )
{
  EFI_STATUS           Status;
  USB_ETHERNET_DRIVER  *UsbEthDriver;

  UsbEthDriver = USB_ETHERNET_DEV_FROM_THIS (This);

  if (UsbUnionFunDescriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetFunctionalDescriptor (UsbEthDriver->Config, UNION_FUN_DESCRIPTOR, UsbUnionFunDescriptor);
  return Status;
}

/**
  Retrieves the USB Ethernet functional Descriptor.

  This function get the Mac Address, Ethernet statistics, maximum segment size,
  number of multicast filters, and number of pattern filters from Ethernet
  functional Descriptor.

  @param[in]  This                   A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[out] UsbEthFunDescriptor    A pointer to the caller allocated USB Ethernet Functional Descriptor.

  @retval EFI_SUCCESS           The USB Ethernet Functional descriptor was retrieved successfully.
  @retval EFI_INVALID_PARAMETER UsbEthFunDescriptor is NULL.
  @retval EFI_NOT_FOUND         The USB Ethernet Functional descriptor was not found.

**/
EFI_STATUS
EFIAPI
GetUsbEthFunDescriptor (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT USB_ETHERNET_FUN_DESCRIPTOR  *UsbEthFunDescriptor
  )
{
  EFI_STATUS           Status;
  USB_ETHERNET_DRIVER  *UsbEthDriver;

  UsbEthDriver = USB_ETHERNET_DEV_FROM_THIS (This);

  if (UsbEthFunDescriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetFunctionalDescriptor (UsbEthDriver->Config, ETHERNET_FUN_DESCRIPTOR, UsbEthFunDescriptor);
  return Status;
}

/**
  This request sets the Ethernet device multicast filters as specified in the
  sequential list of 48 bit Ethernet multicast addresses.

  @param[in]  This                   A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[in]  Value                  Number of filters.
  @param[in]  McastAddr              A pointer to the value of the multicast addresses.

  @retval EFI_SUCCESS           The request executed successfully.
  @retval EFI_TIMEOUT           A timeout occurred executing the request.
  @retval EFI_DEVICE_ERROR      The request failed due to a device error.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       Not supported.

**/
EFI_STATUS
EFIAPI
SetUsbEthMcastFilter (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value,
  IN VOID                         *McastAddr
  )
{
  EFI_STATUS                   Status;
  EFI_USB_DEVICE_REQUEST       Request;
  UINT32                       TransStatus;
  USB_ETHERNET_FUN_DESCRIPTOR  UsbEthFunDescriptor;
  USB_ETHERNET_DRIVER          *UsbEthDriver;

  UsbEthDriver = USB_ETHERNET_DEV_FROM_THIS (This);

  Status = This->UsbEthFunDescriptor (This, &UsbEthFunDescriptor);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((UsbEthFunDescriptor.NumberMcFilters & MAC_FILTERS_MASK) == 0) {
    return EFI_UNSUPPORTED;
  }

  Request.RequestType = USB_ETHERNET_SET_REQ_TYPE;
  Request.Request     = SET_ETH_MULTICAST_FILTERS_REQ;
  Request.Value       = Value;
  Request.Index       = UsbEthDriver->NumOfInterface;
  Request.Length      = Value * 6;

  return UsbEthDriver->UsbIo->UsbControlTransfer (
                                UsbEthDriver->UsbIo,
                                &Request,
                                EfiUsbDataOut,
                                USB_ETHERNET_TRANSFER_TIMEOUT,
                                McastAddr,
                                Request.Length,
                                &TransStatus
                                );
}

/**
  This request sets up the specified Ethernet power management pattern filter as
  described in the data structure.

  @param[in]  This                  A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[in]  Value                 Number of filters.
  @param[in]  Length                Size of the power management pattern filter data.
  @param[in]  PatternFilter         A pointer to the power management pattern filter structure.

  @retval EFI_SUCCESS           The request executed successfully.
  @retval EFI_TIMEOUT           A timeout occurred executing the request.
  @retval EFI_DEVICE_ERROR      The request failed due to a device error.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       Not supported.

**/
EFI_STATUS
EFIAPI
SetUsbEthPowerFilter (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value,
  IN UINT16                       Length,
  IN VOID                         *PatternFilter
  )
{
  EFI_USB_DEVICE_REQUEST  Request;
  UINT32                  TransStatus;
  USB_ETHERNET_DRIVER     *UsbEthDriver;

  UsbEthDriver = USB_ETHERNET_DEV_FROM_THIS (This);

  Request.RequestType = USB_ETHERNET_SET_REQ_TYPE;
  Request.Request     = SET_ETH_POWER_MANAGEMENT_PATTERN_FILTER_REQ;
  Request.Value       = Value;
  Request.Index       = UsbEthDriver->NumOfInterface;
  Request.Length      = Length;

  return UsbEthDriver->UsbIo->UsbControlTransfer (
                                UsbEthDriver->UsbIo,
                                &Request,
                                EfiUsbDataOut,
                                USB_ETHERNET_TRANSFER_TIMEOUT,
                                PatternFilter,
                                Length,
                                &TransStatus
                                );
}

/**
  This request retrieves the status of the specified Ethernet power management
  pattern filter from the device.

  @param[in]  This                   A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[in]  Value                  The filter number.
  @param[out] PatternActive          A pointer to the pattern active boolean.

  @retval EFI_SUCCESS           The request executed successfully.
  @retval EFI_TIMEOUT           A timeout occurred executing the request.
  @retval EFI_DEVICE_ERROR      The request failed due to a device error.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       Not supported.

**/
EFI_STATUS
EFIAPI
GetUsbEthPowerFilter (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN  UINT16                       Value,
  OUT BOOLEAN                      *PatternActive
  )
{
  EFI_USB_DEVICE_REQUEST  Request;
  UINT32                  TransStatus;
  USB_ETHERNET_DRIVER     *UsbEthDriver;

  UsbEthDriver = USB_ETHERNET_DEV_FROM_THIS (This);

  Request.RequestType = USB_ETHERNET_GET_REQ_TYPE;
  Request.Request     = GET_ETH_POWER_MANAGEMENT_PATTERN_FILTER_REQ;
  Request.Value       = Value;
  Request.Index       = UsbEthDriver->NumOfInterface;
  Request.Length      = USB_ETH_POWER_FILTER_LENGTH;

  return UsbEthDriver->UsbIo->UsbControlTransfer (
                                UsbEthDriver->UsbIo,
                                &Request,
                                EfiUsbDataIn,
                                USB_ETHERNET_TRANSFER_TIMEOUT,
                                PatternActive,
                                USB_ETH_POWER_FILTER_LENGTH,
                                &TransStatus
                                );
}

BIT_MAP  gTable[] = {
  { PXE_OPFLAGS_RECEIVE_FILTER_UNICAST,            USB_ETH_PACKET_TYPE_DIRECTED      },
  { PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST,          USB_ETH_PACKET_TYPE_BROADCAST     },
  { PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST, USB_ETH_PACKET_TYPE_MULTICAST     },
  { PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS,        USB_ETH_PACKET_TYPE_PROMISCUOUS   },
  { PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST,      USB_ETH_PACKET_TYPE_ALL_MULTICAST },
};

/**
  Convert value between PXE receive filter and USB ETH packet filter.

  @param[in]  Value      PXE filter data.
  @param[out] CdcFilter  A pointer to the Ethernet Packet Filter Bitmap value converted by PXE_OPFLAGS.

**/
VOID
ConvertFilter (
  IN  UINT16  Value,
  OUT UINT16  *CdcFilter
  )
{
  UINT32  Index;
  UINT32  Count;

  Count = sizeof (gTable)/sizeof (gTable[0]);

  for (Index = 0; (Index < Count) && (gTable[Index].Src != 0); Index++) {
    if (gTable[Index].Src & Value) {
      *CdcFilter |= gTable[Index].Dst;
    }
  }
}

/**
  This request is used to configure device Ethernet packet filter settings.

  @param[in]  This              A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[in]  Value             Packet Filter Bitmap.

  @retval EFI_SUCCESS           The request executed successfully.
  @retval EFI_TIMEOUT           A timeout occurred executing the request.
  @retval EFI_DEVICE_ERROR      The request failed due to a device error.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       Not supported.

**/
EFI_STATUS
EFIAPI
SetUsbEthPacketFilter (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value
  )
{
  EFI_USB_DEVICE_REQUEST  Request;
  UINT32                  TransStatus;
  USB_ETHERNET_DRIVER     *UsbEthDriver;
  UINT16                  CommandFilter;

  UsbEthDriver  = USB_ETHERNET_DEV_FROM_THIS (This);
  CommandFilter = 0;

  ConvertFilter (Value, &CommandFilter);

  Request.RequestType = USB_ETHERNET_SET_REQ_TYPE;
  Request.Request     = SET_ETH_PACKET_FILTER_REQ;
  Request.Value       = CommandFilter;
  Request.Index       = UsbEthDriver->NumOfInterface;
  Request.Length      = USB_ETH_PACKET_FILTER_LENGTH;

  return UsbEthDriver->UsbIo->UsbControlTransfer (
                                UsbEthDriver->UsbIo,
                                &Request,
                                EfiUsbNoData,
                                USB_ETHERNET_TRANSFER_TIMEOUT,
                                NULL,
                                USB_ETH_PACKET_FILTER_LENGTH,
                                &TransStatus
                                );
}

/**
  This request is used to retrieve a statistic based on the feature selector.

  @param[in]  This                  A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[in]  FeatureSelector       Value of the feature selector.
  @param[out] Statistic             A pointer to the 32 bit unsigned integer.

  @retval EFI_SUCCESS           The request executed successfully.
  @retval EFI_TIMEOUT           A timeout occurred executing the request.
  @retval EFI_DEVICE_ERROR      The request failed due to a device error.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       Not supported.

**/
EFI_STATUS
EFIAPI
GetUsbEthStatistic (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN  UINT16                       FeatureSelector,
  OUT VOID                         *Statistic
  )
{
  EFI_STATUS                   Status;
  EFI_USB_DEVICE_REQUEST       Request;
  UINT32                       TransStatus;
  USB_ETHERNET_FUN_DESCRIPTOR  UsbEthFunDescriptor;
  USB_ETHERNET_DRIVER          *UsbEthDriver;

  UsbEthDriver = USB_ETHERNET_DEV_FROM_THIS (This);

  Status = This->UsbEthFunDescriptor (This, &UsbEthFunDescriptor);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (UsbEthFunDescriptor.EthernetStatistics == 0) {
    return EFI_UNSUPPORTED;
  }

  Request.RequestType = USB_ETHERNET_GET_REQ_TYPE;
  Request.Request     = GET_ETH_STATISTIC_REQ;
  Request.Value       = FeatureSelector;
  Request.Index       = UsbEthDriver->NumOfInterface;
  Request.Length      = USB_ETH_STATISTIC;

  return UsbEthDriver->UsbIo->UsbControlTransfer (
                                UsbEthDriver->UsbIo,
                                &Request,
                                EfiUsbDataIn,
                                USB_ETHERNET_TRANSFER_TIMEOUT,
                                Statistic,
                                USB_ETH_STATISTIC,
                                &TransStatus
                                );
}

/** @file
  This file contains code for USB Ethernet descriptor
  and specific requests implement.

  Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UsbRndis.h"

UINT16  gStopBulkInCnt  = 0;
UINT16  gBlockBulkInCnt = 0;

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
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:UsbGetConfigDescriptor status = %r\n", __func__, Status));
    return Status;
  }

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  Tmp.TotalLength,
                  (VOID **)ConfigDesc
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AllocatePool status = %r\n", __func__, Status));
    return Status;
  }

  Status = UsbGetDescriptor (
             UsbIo,
             USB_DESC_TYPE_CONFIG << 8 | (Tmp.ConfigurationValue - 1),                 // zero based
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
  IN      EFI_USB_CONFIG_DESCRIPTOR  *Desc,
  IN OUT  UINTN                      *Offset
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

  @param[in]      UsbIo           A pointer to the EFI_USB_IO_PROTOCOL instance.
  @param[in, out] UsbRndisDevice  A pointer to the USB_RNDIS_DEVICE instance.

**/
VOID
GetEndpoint (
  IN      EFI_USB_IO_PROTOCOL  *UsbIo,
  IN OUT  USB_RNDIS_DEVICE     *UsbRndisDevice
  )
{
  EFI_STATUS                    Status;
  UINT8                         Index;
  UINT32                        Result;
  EFI_USB_INTERFACE_DESCRIPTOR  Interface;
  EFI_USB_ENDPOINT_DESCRIPTOR   Endpoint;

  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &Interface);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:UsbGetInterfaceDescriptor status = %r\n", __func__, Status));
    return;
  }

  if (Interface.NumEndpoints == 0 ) {
    Status = UsbSetInterface (UsbIo, 1, 0, &Result);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a:UsbSetInterface status = %r\n", __func__, Status));
      return;
    }

    Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &Interface);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a:UsbGetInterfaceDescriptor status = %r\n", __func__, Status));
      return;
    }
  }

  for (Index = 0; Index < Interface.NumEndpoints; Index++) {
    Status = UsbIo->UsbGetEndpointDescriptor (UsbIo, Index, &Endpoint);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a:UsbGetEndpointDescriptor status = %r\n", __func__, Status));
      return;
    }

    switch ((Endpoint.Attributes & (BIT0 | BIT1))) {
      case USB_ENDPOINT_BULK:
        if (Endpoint.EndpointAddress & BIT7) {
          UsbRndisDevice->BulkInEndpoint = Endpoint.EndpointAddress;
        } else {
          UsbRndisDevice->BulkOutEndpoint = Endpoint.EndpointAddress;
        }

        break;
      case USB_ENDPOINT_INTERRUPT:
        UsbRndisDevice->InterrupEndpoint = Endpoint.EndpointAddress;
        break;
    }
  }
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

  if (((EFI_USB_DEVICE_REQUEST *)Data)->Request == 0) {
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
  @param[in]  Requst            A pointer to the EFI_USB_DEVICE_REQUEST data.

  @retval EFI_SUCCESS           The asynchronous USB transfer request transfer has been successfully executed.
  @retval EFI_DEVICE_ERROR      The asynchronous USB transfer request failed.

**/
EFI_STATUS
EFIAPI
UsbRndisInterrupt (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN BOOLEAN                      IsNewTransfer,
  IN UINTN                        PollingInterval,
  IN EFI_USB_DEVICE_REQUEST       *Requst
  )
{
  EFI_STATUS        Status;
  USB_RNDIS_DEVICE  *UsbRndisDevice;
  UINTN             DataLength;

  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (This);
  DataLength     = 0;

  if (IsNewTransfer) {
    DataLength = sizeof (EFI_USB_DEVICE_REQUEST) + sizeof (USB_CONNECT_SPEED_CHANGE);
    Status     = UsbRndisDevice->UsbIo->UsbAsyncInterruptTransfer (
                                          UsbRndisDevice->UsbIo,
                                          UsbRndisDevice->InterrupEndpoint,
                                          IsNewTransfer,
                                          PollingInterval,
                                          DataLength,
                                          InterruptCallback,
                                          Requst
                                          );

    if (Status == EFI_INVALID_PARAMETER) {
      // Because of Stacked AsyncInterrupt request are not supported
      Status = UsbRndisDevice->UsbIo->UsbAsyncInterruptTransfer (
                                        UsbRndisDevice->UsbIo,
                                        UsbRndisDevice->InterrupEndpoint,
                                        0,
                                        0,
                                        0,
                                        NULL,
                                        NULL
                                        );
    }
  } else {
    Status = UsbRndisDevice->UsbIo->UsbAsyncInterruptTransfer (
                                      UsbRndisDevice->UsbIo,
                                      UsbRndisDevice->InterrupEndpoint,
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
  This function is used to read USB interrupt transfer before the response RNDIS message.

  @param[in]  UsbRndisDevice    A pointer to the USB_RNDIS_DEVICE instance.

  @retval EFI_SUCCESS           The USB interrupt transfer has been successfully executed.
  @retval EFI_DEVICE_ERROR      The USB interrupt transfer failed.

**/
EFI_STATUS
EFIAPI
ReadRndisResponseInterrupt (
  IN USB_RNDIS_DEVICE  *UsbRndisDevice
  )
{
  EFI_STATUS  Status;
  UINT32      Data[2];
  UINT32      UsbStatus;
  UINTN       DataLength;

  DataLength = 8;

  ZeroMem (Data, sizeof (Data));

  Status = UsbRndisDevice->UsbIo->UsbSyncInterruptTransfer (
                                    UsbRndisDevice->UsbIo,
                                    UsbRndisDevice->InterrupEndpoint,
                                    &Data,
                                    &DataLength,
                                    0x20,
                                    &UsbStatus
                                    );

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
  USB_RNDIS_DEVICE             *UsbRndisDevice;
  USB_ETHERNET_FUN_DESCRIPTOR  UsbEthDescriptor;
  CHAR16                       *Data;
  CHAR16                       *DataPtr;
  CHAR16                       TmpStr[1];
  UINT8                        Index;
  UINT8                        Hi;
  UINT8                        Low;

  REMOTE_NDIS_QUERY_MAC_MSG    RndisQueryMsg;
  REMOTE_NDIS_QUERY_MAC_CMPLT  RndisQueryMsgCmplt;

  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (This);

  ZeroMem (&RndisQueryMsg, sizeof (REMOTE_NDIS_QUERY_MAC_MSG));
  ZeroMem (&RndisQueryMsgCmplt, sizeof (REMOTE_NDIS_QUERY_MAC_CMPLT));

  RndisQueryMsg.QueryMsg.MessageType   = RNDIS_QUERY_MSG;
  RndisQueryMsg.QueryMsg.MessageLength = sizeof (REMOTE_NDIS_QUERY_MAC_MSG);
  RndisQueryMsg.QueryMsg.RequestID     = UsbRndisDevice->RequestId;
  RndisQueryMsg.QueryMsg.Oid           = OID_802_3_CURRENT_ADDRESS;

  RndisQueryMsgCmplt.QueryCmplt.MessageType   = RNDIS_QUERY_CMPLT;
  RndisQueryMsgCmplt.QueryCmplt.MessageLength = sizeof (REMOTE_NDIS_QUERY_MAC_CMPLT);

  Status = RndisControlMsg (
             UsbRndisDevice,
             (REMOTE_NDIS_MSG_HEADER *)&RndisQueryMsg,
             (REMOTE_NDIS_MSG_HEADER *)&RndisQueryMsgCmplt
             );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Success to get Mac address from RNDIS message.\n"));
    for (Index = 0; Index < PXE_HWADDR_LEN_ETHER; Index++) {
      MacAddress->Addr[Index] = RndisQueryMsgCmplt.Addr[Index];
    }

    UsbRndisDevice->RequestId++;
    return Status;
  }

  // If it is not support the OID_802_3_CURRENT_ADDRESS.
  // To check USB Ethernet functional Descriptor
  Status = This->UsbEthFunDescriptor (This, &UsbEthDescriptor);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:UsbEthFunDescriptor status = %r\n", __func__, Status));
    return Status;
  }

  Status = UsbRndisDevice->UsbIo->UsbGetStringDescriptor (
                                    UsbRndisDevice->UsbIo,
                                    0x409,                       // English-US Language ID
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
  Retrieves the USB Ethernet Bulk transfer data size.

  @param[in]  This          A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[out] BulkSize      A pointer to the Bulk transfer data size.

  @retval EFI_SUCCESS       The bulk transfer data size was retrieved successfully.
  @retval other             Failed to retrieve the bulk transfer data size.

**/
EFI_STATUS
EFIAPI
UsbEthBulkSize (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT UINTN                        *BulkSize
  )
{
  EFI_STATUS                   Status;
  USB_ETHERNET_FUN_DESCRIPTOR  UsbEthFunDescriptor;
  USB_RNDIS_DEVICE             *UsbRndisDevice;

  REMOTE_NDIS_QUERY_MAX_TOTAL_SIZE_MSG    RndisQueryMsg;
  REMOTE_NDIS_QUERY_MAX_TOTAL_SIZE_CMPLT  RndisQueryMsgCmplt;

  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (This);

  ZeroMem (&RndisQueryMsg, sizeof (REMOTE_NDIS_QUERY_MAX_TOTAL_SIZE_MSG));
  ZeroMem (&RndisQueryMsgCmplt, sizeof (REMOTE_NDIS_QUERY_MAX_TOTAL_SIZE_CMPLT));

  RndisQueryMsg.QueryMsg.MessageType   = RNDIS_QUERY_MSG;
  RndisQueryMsg.QueryMsg.MessageLength = sizeof (REMOTE_NDIS_QUERY_MAX_TOTAL_SIZE_MSG);
  RndisQueryMsg.QueryMsg.RequestID     = UsbRndisDevice->RequestId;
  RndisQueryMsg.QueryMsg.Oid           = OID_GEN_MAXIMUM_TOTAL_SIZE;

  RndisQueryMsgCmplt.QueryCmplt.MessageType   = RNDIS_QUERY_CMPLT;
  RndisQueryMsgCmplt.QueryCmplt.MessageLength = sizeof (REMOTE_NDIS_QUERY_MAX_TOTAL_SIZE_CMPLT);

  Status = RndisControlMsg (
             UsbRndisDevice,
             (REMOTE_NDIS_MSG_HEADER *)&RndisQueryMsg,
             (REMOTE_NDIS_MSG_HEADER *)&RndisQueryMsgCmplt
             );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Success to get Max Total size : %X  \n", RndisQueryMsgCmplt.MaxTotalSize));
    *BulkSize = RndisQueryMsgCmplt.MaxTotalSize;
    UsbRndisDevice->RequestId++;
    return Status;
  }

  Status = This->UsbEthFunDescriptor (This, &UsbEthFunDescriptor);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *BulkSize = (UINTN)UsbEthFunDescriptor.MaxSegmentSize;
  return Status;
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
  EFI_STATUS        Status;
  USB_RNDIS_DEVICE  *UsbRndisDevice;

  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (This);

  if (UsbHeaderFunDescriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetFunctionalDescriptor (
             UsbRndisDevice->Config,
             HEADER_FUN_DESCRIPTOR,
             UsbHeaderFunDescriptor
             );
  return Status;
}

/**
  Retrieves the USB Union functional Descriptor.

  @param[in]  This                   A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[out] UsbUnionFunDescriptor  A pointer to the caller allocated USB Union Functional Descriptor.

  @retval EFI_SUCCESS            The USB Union Functional descriptor was retrieved successfully.
  @retval EFI_INVALID_PARAMETER  UsbUnionFunDescriptor is NULL.
  @retval EFI_NOT_FOUND          The USB Union Functional descriptor was not found.

**/
EFI_STATUS
EFIAPI
GetUsbUnionFunDescriptor (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT USB_UNION_FUN_DESCRIPTOR     *UsbUnionFunDescriptor
  )
{
  EFI_STATUS        Status;
  USB_RNDIS_DEVICE  *UsbRndisDevice;

  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (This);

  if (UsbUnionFunDescriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetFunctionalDescriptor (
             UsbRndisDevice->Config,
             UNION_FUN_DESCRIPTOR,
             UsbUnionFunDescriptor
             );
  return Status;
}

/**
  Retrieves the USB Ethernet functional Descriptor.

  This function get the Mac Address, Ethernet statistics, maximum segment size,
  number of multicast filters, and number of pattern filters from Ethernet
  functional Descriptor.

  @param[in]  This                   A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[out] UsbEthFunDescriptor    A pointer to the caller allocated USB Ethernet Functional Descriptor.

  @retval EFI_SUCCESS            The USB Ethernet Functional descriptor was retrieved successfully.
  @retval EFI_INVALID_PARAMETER  UsbEthFunDescriptor is NULL.
  @retval EFI_NOT_FOUND          The USB Ethernet Functional descriptor was not found.

**/
EFI_STATUS
EFIAPI
GetUsbRndisFunDescriptor (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT USB_ETHERNET_FUN_DESCRIPTOR  *UsbEthFunDescriptor
  )
{
  EFI_STATUS        Status;
  USB_RNDIS_DEVICE  *UsbRndisDevice;

  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (This);

  if (UsbEthFunDescriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetFunctionalDescriptor (
             UsbRndisDevice->Config,
             ETHERNET_FUN_DESCRIPTOR,
             UsbEthFunDescriptor
             );
  return Status;
}

/**
  This request sets the Ethernet device multicast filters as specified in the
  sequential list of 48 bit Ethernet multicast addresses.

  @param[in]  This                   A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[in]  Value                  Number of filters.
  @param[in]  McastAddr              A pointer to the value of the multicast addresses.

  @retval EFI_SUCCESS            The request executed successfully.
  @retval EFI_TIMEOUT            A timeout occurred executing the request.
  @retval EFI_DEVICE_ERROR       The request failed due to a device error.
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED        Not supported.

**/
EFI_STATUS
EFIAPI
SetUsbRndisMcastFilter (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value,
  IN VOID                         *McastAddr
  )
{
  EFI_STATUS                   Status;
  EFI_USB_DEVICE_REQUEST       Request;
  UINT32                       TransStatus;
  USB_ETHERNET_FUN_DESCRIPTOR  UsbEthFunDescriptor;
  USB_RNDIS_DEVICE             *UsbRndisDevice;

  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (This);

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
  Request.Index       = UsbRndisDevice->NumOfInterface;
  Request.Length      = Value * 6;

  return UsbRndisDevice->UsbIo->UsbControlTransfer (
                                  UsbRndisDevice->UsbIo,
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

  @retval EFI_SUCCESS            The request executed successfully.
  @retval EFI_TIMEOUT            A timeout occurred executing the request.
  @retval EFI_DEVICE_ERROR       The request failed due to a device error.
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED        Not supported.

**/
EFI_STATUS
EFIAPI
SetUsbRndisPowerFilter (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value,
  IN UINT16                       Length,
  IN VOID                         *PatternFilter
  )
{
  EFI_USB_DEVICE_REQUEST  Request;
  UINT32                  TransStatus;
  USB_RNDIS_DEVICE        *UsbRndisDevice;

  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (This);

  Request.RequestType = USB_ETHERNET_SET_REQ_TYPE;
  Request.Request     = SET_ETH_POWER_MANAGEMENT_PATTERN_FILTER_REQ;
  Request.Value       = Value;
  Request.Index       = UsbRndisDevice->NumOfInterface;
  Request.Length      = Length;

  return UsbRndisDevice->UsbIo->UsbControlTransfer (
                                  UsbRndisDevice->UsbIo,
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

  @retval EFI_SUCCESS            The request executed successfully.
  @retval EFI_TIMEOUT            A timeout occurred executing the request.
  @retval EFI_DEVICE_ERROR       The request failed due to a device error.
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED        Not supported.

**/
EFI_STATUS
EFIAPI
GetUsbRndisPowerFilter (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN  UINT16                       Value,
  OUT BOOLEAN                      *PatternActive
  )
{
  EFI_USB_DEVICE_REQUEST  Request;
  UINT32                  TransStatus;
  USB_RNDIS_DEVICE        *UsbRndisDevice;

  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (This);

  Request.RequestType = USB_ETHERNET_GET_REQ_TYPE;
  Request.Request     = GET_ETH_POWER_MANAGEMENT_PATTERN_FILTER_REQ;
  Request.Value       = Value;
  Request.Index       = UsbRndisDevice->NumOfInterface;
  Request.Length      = USB_ETH_POWER_FILTER_LENGTH;

  return UsbRndisDevice->UsbIo->UsbControlTransfer (
                                  UsbRndisDevice->UsbIo,
                                  &Request,
                                  EfiUsbDataIn,
                                  USB_ETHERNET_TRANSFER_TIMEOUT,
                                  PatternActive,
                                  USB_ETH_POWER_FILTER_LENGTH,
                                  &TransStatus
                                  );
}

BIT_MAP  gTable[] = {
  { PXE_OPFLAGS_RECEIVE_FILTER_UNICAST,            NDIS_PACKET_TYPE_DIRECTED      },
  { PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST,          NDIS_PACKET_TYPE_BROADCAST     },
  { PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST, NDIS_PACKET_TYPE_MULTICAST     },
  { PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS,        NDIS_PACKET_TYPE_PROMISCUOUS   },
  { PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST,      NDIS_PACKET_TYPE_ALL_MULTICAST },
};

/**

  Converts PXE filter settings to RNDIS values

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

  Updates Filter settings on the device.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

  @retval EFI_STATUS

**/
EFI_STATUS
EFIAPI
RndisUndiReceiveFilter (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EFI_STATUS                   Status;
  UINT8                        *McastList;
  UINT8                        Count;
  UINT8                        Index1;
  UINT8                        Index2;
  UINT64                       CpbAddr;
  UINT32                       CpbSize;
  UINT16                       SetFilter;
  PXE_CPB_RECEIVE_FILTERS      *Cpb;
  USB_ETHERNET_FUN_DESCRIPTOR  UsbEthFunDescriptor;

  Count     = 0;
  CpbAddr   = Cdb->CPBaddr;
  CpbSize   = Cdb->CPBsize;
  SetFilter = (UINT16)(Cdb->OpFlags & 0x1F);
  Cpb       = (PXE_CPB_RECEIVE_FILTERS *)(UINTN)CpbAddr;

  // The Cpb could be NULL.(ref:PXE_CPBADDR_NOT_USED)
  Nic->RxFilter = (UINT8)SetFilter;

  if (((SetFilter & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) != 0) || (Cpb != NULL)) {
    if (Cpb != NULL) {
      Nic->McastCount = (UINT8)(CpbSize / PXE_MAC_LENGTH);
      CopyMem (&Nic->McastList, Cpb, Nic->McastCount);
    } else {
      Nic->McastCount = 0;
    }

    Nic->UsbEth->UsbEthFunDescriptor (Nic->UsbEth, &UsbEthFunDescriptor);
    if ((UsbEthFunDescriptor.NumberMcFilters & MAC_FILTERS_MASK) == 0) {
      Nic->RxFilter |= PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST;
      DEBUG ((DEBUG_INFO, "SetUsbEthPacketFilter Nic %lx Nic->UsbEth %lx ", Nic, Nic->UsbEth));
      Nic->UsbEth->SetUsbEthPacketFilter (Nic->UsbEth, Nic->RxFilter);
    } else {
      Status = gBS->AllocatePool (EfiBootServicesData, Nic->McastCount * 6, (VOID **)&McastList);
      if (EFI_ERROR (Status)) {
        return PXE_STATCODE_INVALID_PARAMETER;
      }

      if (Cpb != NULL) {
        for (Index1 = 0; Index1 < Nic->McastCount; Index1++) {
          for (Index2 = 0; Index2 < 6; Index2++) {
            McastList[Count++] = Cpb->MCastList[Index1][Index2];
          }
        }
      }

      Nic->RxFilter |= PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST;
      if (Cpb != NULL) {
        Nic->UsbEth->SetUsbEthMcastFilter (Nic->UsbEth, Nic->McastCount, McastList);
      }

      Nic->UsbEth->SetUsbEthPacketFilter (Nic->UsbEth, Nic->RxFilter);
      FreePool (McastList);
    }
  }

  return EFI_SUCCESS;
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
SetUsbRndisPacketFilter (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value
  )
{
  return EFI_SUCCESS;
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
GetRndisStatistic (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN  UINT16                       FeatureSelector,
  OUT VOID                         *Statistic
  )
{
  return EFI_SUCCESS;
}

/**
  This function is called when UndiStart is invoked.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

  @retval EFI_SUCCESS     The request executed successfully.

**/
EFI_STATUS
EFIAPI
RndisUndiStart (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "RndisUndiStart Nic %lx Cdb %lx Nic State %x\n", Nic, Cdb, Nic->State));

  // Issue Rndis Reset and bring the device to RNDIS_BUS_INITIALIZED state
  Status = RndisUndiReset (Cdb, Nic);
  if (EFI_ERROR (Status)) {
    RndisUndiReset (Cdb, Nic);
  }

  Status = RndisUndiInitialize (Cdb, Nic);
  if (EFI_ERROR (Status)) {
    RndisUndiInitialize (Cdb, Nic);
  }

  RndisUndiShutdown (Cdb, Nic);

  return EFI_SUCCESS;
}

/**
  This function is called when Undistop is invoked.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

  @retval EFI_SUCCESS     The request executed successfully.
**/
EFI_STATUS
EFIAPI
RndisUndiStop (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  DEBUG ((DEBUG_INFO, "RndisUndiStop State %x\n", Nic->State));
  return EFI_SUCCESS;
}

/**
  This function is called when UndiGetInitInfo is invoked.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

  @retval EFI_SUCCESS     The request executed successfully.

**/
EFI_STATUS
EFIAPI
RndisUndiGetInitInfo (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EDKII_USB_ETHERNET_PROTOCOL  *UsbEthDevice;
  USB_RNDIS_DEVICE             *UsbRndisDevice;
  PXE_DB_GET_INIT_INFO         *Db;

  DEBUG ((DEBUG_INFO, "RndisUndiGetInitInfo\n"));

  UsbEthDevice   = Nic->UsbEth;
  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (UsbEthDevice);

  Db = (PXE_DB_GET_INIT_INFO *)(UINTN)Cdb->DBaddr;

  Db->FrameDataLen = UsbRndisDevice->MaxTransferSize - sizeof (REMOTE_NDIS_PACKET_MSG) - PXE_MAC_HEADER_LEN_ETHER;
  // Limit Max MTU size to 1500 bytes as RNDIS spec.
  if (Db->FrameDataLen > PXE_MAX_TXRX_UNIT_ETHER) {
    Db->FrameDataLen = PXE_MAX_TXRX_UNIT_ETHER;
  }

  DEBUG ((DEBUG_INFO, "Db->FrameDataLen %x\n", Db->FrameDataLen));

  return EFI_SUCCESS;
}

/**
  This function is called when RndisUndiGetConfigInfo is invoked.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

  @retval EFI_SUCCESS     The request executed successfully.

**/
EFI_STATUS
EFIAPI
RndisUndiGetConfigInfo (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  DEBUG ((DEBUG_INFO, "RndisUndiGetConfigInfo\n"));
  return EFI_SUCCESS;
}

/**
  This function is called when UndiInitialize is invoked.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

  @retval EFI_SUCCESS           The request executed successfully.
  @retval EFI_UNSUPPORTED       Not supported.

**/
EFI_STATUS
EFIAPI
RndisUndiInitialize (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EDKII_USB_ETHERNET_PROTOCOL   *UsbEthDriver;
  USB_RNDIS_DEVICE              *UsbRndisDevice;
  REMOTE_NDIS_INITIALIZE_MSG    RndisInitMsg;
  REMOTE_NDIS_INITIALIZE_CMPLT  RndisInitMsgCmplt;
  EFI_STATUS                    Status;

  DEBUG ((DEBUG_INFO, "RndisUndiInitialize\n"));

  UsbEthDriver   = Nic->UsbEth;
  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (UsbEthDriver);

  ZeroMem (&RndisInitMsg, sizeof (REMOTE_NDIS_INITIALIZE_MSG));
  ZeroMem (&RndisInitMsgCmplt, sizeof (REMOTE_NDIS_INITIALIZE_CMPLT));

  RndisInitMsg.MessageType     = RNDIS_INITIALIZE_MSG;
  RndisInitMsg.MessageLength   = sizeof (REMOTE_NDIS_INITIALIZE_MSG);
  RndisInitMsg.RequestID       = UsbRndisDevice->RequestId;
  RndisInitMsg.MajorVersion    = RNDIS_MAJOR_VERSION;
  RndisInitMsg.MinorVersion    = RNDIS_MINOR_VERSION;
  RndisInitMsg.MaxTransferSize = RNDIS_MAX_TRANSFER_SIZE;

  RndisInitMsgCmplt.MessageType   = RNDIS_INITIALIZE_CMPLT;
  RndisInitMsgCmplt.MessageLength = sizeof (REMOTE_NDIS_INITIALIZE_CMPLT);

  Status = RndisControlMsg (UsbRndisDevice, (REMOTE_NDIS_MSG_HEADER *)&RndisInitMsg, (REMOTE_NDIS_MSG_HEADER *)&RndisInitMsgCmplt);

  UsbRndisDevice->RequestId++;

  if (EFI_ERROR (Status) || (RndisInitMsgCmplt.Status & 0x80000000)) {
    return Status;
  }

  // Only Wired Medium is supported
  if (RndisInitMsgCmplt.Medium) {
    return EFI_UNSUPPORTED;
  }

  UsbRndisDevice->Medium                = RndisInitMsgCmplt.Medium;
  UsbRndisDevice->MaxPacketsPerTransfer = RndisInitMsgCmplt.MaxPacketsPerTransfer;
  UsbRndisDevice->MaxTransferSize       = RndisInitMsgCmplt.MaxTransferSize;
  UsbRndisDevice->PacketAlignmentFactor = RndisInitMsgCmplt.PacketAlignmentFactor;

  DEBUG ((DEBUG_INFO, "Medium : %x \n", RndisInitMsgCmplt.Medium));
  DEBUG ((DEBUG_INFO, "MaxPacketsPerTransfer : %x \n", RndisInitMsgCmplt.MaxPacketsPerTransfer));
  DEBUG ((DEBUG_INFO, "MaxTransferSize : %x\n", RndisInitMsgCmplt.MaxTransferSize));
  DEBUG ((DEBUG_INFO, "PacketAlignmentFactor : %x\n", RndisInitMsgCmplt.PacketAlignmentFactor));

  return Status;
}

/**
  This function is called when UndiReset is invoked.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

  @retval EFI_SUCCESS           The request executed successfully.
  @retval EFI_DEVICE_ERROR      The request failed due to a device error.

**/
EFI_STATUS
EFIAPI
RndisUndiReset (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EDKII_USB_ETHERNET_PROTOCOL  *UsbEthDriver;
  USB_RNDIS_DEVICE             *UsbRndisDevice;
  REMOTE_NDIS_RESET_MSG        RndisResetMsg;
  REMOTE_NDIS_RESET_CMPLT      RndisResetCmplt;
  EFI_STATUS                   Status;

  DEBUG ((DEBUG_INFO, "RndisUndiReset\n"));

  UsbEthDriver   = Nic->UsbEth;
  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (UsbEthDriver);

  ZeroMem (&RndisResetMsg, sizeof (REMOTE_NDIS_RESET_MSG));
  ZeroMem (&RndisResetCmplt, sizeof (REMOTE_NDIS_RESET_CMPLT));

  RndisResetMsg.MessageType   = RNDIS_RESET_MSG;
  RndisResetMsg.MessageLength = sizeof (REMOTE_NDIS_RESET_MSG);

  RndisResetCmplt.MessageType   = RNDIS_RESET_CMPLT;
  RndisResetCmplt.MessageLength = sizeof (REMOTE_NDIS_RESET_CMPLT);

  Status = RndisControlMsg (UsbRndisDevice, (REMOTE_NDIS_MSG_HEADER *)&RndisResetMsg, (REMOTE_NDIS_MSG_HEADER *)&RndisResetCmplt);

  UsbRndisDevice->RequestId = 1;          // Let's start with 1

  if (EFI_ERROR (Status) || (RndisResetCmplt.Status & 0x80000000)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  This function is called when UndiShutdown is invoked.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

  @retval EFI_SUCCESS     The request executed successfully.

**/
EFI_STATUS
EFIAPI
RndisUndiShutdown (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  EDKII_USB_ETHERNET_PROTOCOL  *UsbEthDriver;
  USB_RNDIS_DEVICE             *UsbRndisDevice;
  REMOTE_NDIS_HALT_MSG         RndisHltMsg;
  EFI_STATUS                   Status;

  DEBUG ((DEBUG_INFO, "RndisUndiShutdown\n"));

  UsbEthDriver   = Nic->UsbEth;
  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (UsbEthDriver);

  ZeroMem (&RndisHltMsg, sizeof (REMOTE_NDIS_HALT_MSG));

  RndisHltMsg.MessageType   = RNDIS_HLT_MSG;
  RndisHltMsg.MessageLength = sizeof (REMOTE_NDIS_HALT_MSG);

  Status = RndisControlMsg (UsbRndisDevice, (REMOTE_NDIS_MSG_HEADER *)&RndisHltMsg, NULL);

  if (Status == EFI_DEVICE_ERROR) {
    Status = EFI_SUCCESS;
  }

  UsbRndisDevice->RequestId = 1;
  return Status;
}

/**
  Update the Media connection.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

  @retval EFI_SUCCESS     The request executed successfully.

**/
EFI_STATUS
EFIAPI
RndisUndiGetStatus (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  Cdb->StatFlags &= ~(PXE_STATFLAGS_GET_STATUS_NO_MEDIA);
  return EFI_SUCCESS;
}

/**
  Transmit the data after appending RNDIS header.

  @param[in]      Cdb           A pointer to the command descriptor block.
  @param[in]      This          A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[in]      BulkOutData   A pointer to the buffer of data that will be transmitted to USB
                                device or received from USB device.
  @param[in, out] DataLength    A pointer to the PacketLength.

  @retval EFI_SUCCESS     The request executed successfully.

**/
EFI_STATUS
EFIAPI
RndisUndiTransmit (
  IN      PXE_CDB                      *Cdb,
  IN      EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN      VOID                         *BulkOutData,
  IN OUT  UINTN                        *DataLength
  )
{
  EFI_STATUS              Status;
  USB_RNDIS_DEVICE        *UsbRndisDevice;
  REMOTE_NDIS_PACKET_MSG  *RndisPacketMsg;
  UINTN                   TransferLength;

  DEBUG ((DEBUG_INFO, "RndisUndiTransmit DataLength : %x\n", *DataLength));

  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (This);

  RndisPacketMsg = AllocateZeroPool (sizeof (REMOTE_NDIS_PACKET_MSG) + *DataLength);
  if (RndisPacketMsg == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  RndisPacketMsg->MessageType   = RNDIS_PACKET_MSG;
  RndisPacketMsg->MessageLength = sizeof (REMOTE_NDIS_PACKET_MSG) + (UINT32)*DataLength;
  RndisPacketMsg->DataOffset    = sizeof (REMOTE_NDIS_PACKET_MSG) - 8;
  RndisPacketMsg->DataLength    = (UINT32)*DataLength;

  CopyMem (
    ((UINT8 *)RndisPacketMsg) + sizeof (REMOTE_NDIS_PACKET_MSG),
    BulkOutData,
    *DataLength
    );

  TransferLength = RndisPacketMsg->MessageLength;

  Status = RndisTransmitDataMsg (
             UsbRndisDevice,
             (REMOTE_NDIS_MSG_HEADER *)RndisPacketMsg,
             &TransferLength
             );

  DEBUG ((DEBUG_INFO, "\nRndisUndiTransmit TransferLength %lx\n", TransferLength));

  FreePool (RndisPacketMsg);

  return Status;
}

/**
  Receives and removes RNDIS header and returns the raw data.

  @param[in]      Cdb           A pointer to the command descriptor block.
  @param[in]      This          A pointer to the EDKII_USB_ETHERNET_PROTOCOL instance.
  @param[in, out] BulkInData    A pointer to the buffer of data that will be transmitted to USB
                                device or received from USB device.
  @param[in, out] DataLength    A pointer to the PacketLength.

  @retval EFI_SUCCESS           The request executed successfully.
  @retval EFI_BUFFER_TOO_SMALL  The user provided buffer is too small
  @retval EFI_NOT_FOUND         No buffer was found in the list.

**/
EFI_STATUS
EFIAPI
RndisUndiReceive (
  IN     PXE_CDB                      *Cdb,
  IN     EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN OUT VOID                         *BulkInData,
  IN OUT UINTN                        *DataLength
  )
{
  EFI_STATUS              Status;
  USB_RNDIS_DEVICE        *UsbRndisDevice;
  REMOTE_NDIS_PACKET_MSG  *RndisPacketMsg;
  UINTN                   TransferLength;
  VOID                    *Buffer;
  PACKET_LIST             *HeadPacket;
  PACKET_LIST             *PacketList;

  // Check if there is any outstanding packet to receive
  // The buffer allocated has a linked List followed by the packet.

  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (This);
  Buffer         = NULL;
  HeadPacket     = NULL;

  while (1) {
    Buffer = AllocateZeroPool (sizeof (PACKET_LIST) + sizeof (REMOTE_NDIS_PACKET_MSG) + UsbRndisDevice->MaxTransferSize);
    if (Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    RndisPacketMsg                = (REMOTE_NDIS_PACKET_MSG *)(sizeof (PACKET_LIST) + (UINT8 *)Buffer);
    PacketList                    = (PACKET_LIST *)Buffer;
    PacketList->PacketStartBuffer = (UINT8 *)Buffer + sizeof (PACKET_LIST);
    // Save the original address for freeing it up
    PacketList->OrgBuffer = (UINT8 *)Buffer;
    TransferLength        = UsbRndisDevice->MaxTransferSize;

    Status = RndisReceiveDataMsg (
               UsbRndisDevice,
               (REMOTE_NDIS_MSG_HEADER *)RndisPacketMsg,
               &TransferLength
               );

    if (EFI_ERROR (Status) || (TransferLength == 0)) {
      FreePool (Buffer);
      break;
    }

    // Collect all the RNDIS packet in Linked list.
    if ((RndisPacketMsg->MessageType == RNDIS_PACKET_MSG) &&
        (RndisPacketMsg->DataOffset == sizeof (REMOTE_NDIS_PACKET_MSG) - RNDIS_RESERVED_BYTE_LENGTH) &&
        (TransferLength >= RndisPacketMsg->MessageLength))
    {
      // Insert Packet
      PacketList->RemainingLength = TransferLength;
      InsertTailList (&UsbRndisDevice->ReceivePacketList, Buffer);
    } else {
      FreePool (Buffer);
    }
  }

  while (!IsListEmpty (&UsbRndisDevice->ReceivePacketList)) {
    HeadPacket = (PACKET_LIST *)GetFirstNode (&UsbRndisDevice->ReceivePacketList);

    RndisPacketMsg = (REMOTE_NDIS_PACKET_MSG *)(UINT8 *)HeadPacket->PacketStartBuffer;

    PrintRndisMsg ((REMOTE_NDIS_MSG_HEADER *)RndisPacketMsg);

    // Check whether the packet is valid RNDIS packet.
    if ((HeadPacket->RemainingLength > sizeof (REMOTE_NDIS_PACKET_MSG)) && (RndisPacketMsg->MessageType == RNDIS_PACKET_MSG) &&
        (RndisPacketMsg->DataOffset == (sizeof (REMOTE_NDIS_PACKET_MSG) - RNDIS_RESERVED_BYTE_LENGTH)) &&
        (HeadPacket->RemainingLength >= RndisPacketMsg->MessageLength))
    {
      if (*DataLength >= RndisPacketMsg->DataLength) {
        CopyMem (
          BulkInData,
          (UINT8 *)RndisPacketMsg + (RndisPacketMsg->DataOffset + RNDIS_RESERVED_BYTE_LENGTH),
          RndisPacketMsg->DataLength
          );

        *DataLength =  RndisPacketMsg->DataLength;

        HeadPacket->RemainingLength   = HeadPacket->RemainingLength - RndisPacketMsg->MessageLength;
        HeadPacket->PacketStartBuffer = (UINT8 *)RndisPacketMsg + RndisPacketMsg->MessageLength;

        return EFI_SUCCESS;
      } else {
        *DataLength = RndisPacketMsg->DataLength;
        return EFI_BUFFER_TOO_SMALL;
      }
    }

    RemoveEntryList (&HeadPacket->PacketList);
    FreePool ((PACKET_LIST *)HeadPacket->OrgBuffer);
  }

  return EFI_NOT_FOUND;
}

/**
  This is a dummy function which just returns. Unimplemented EDKII_USB_ETHERNET_PROTOCOL functions
  point to this function.

  @param[in]  Cdb  A pointer to the command descriptor block.
  @param[in]  Nic  A pointer to the Network interface controller data.

  @retval EFI_SUCCESS     The request executed successfully.

**/
EFI_STATUS
EFIAPI
RndisDummyReturn (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  )
{
  DEBUG ((DEBUG_INFO, "RndisDummyReturn called\n"));
  return EFI_SUCCESS;
}

/**
  This function send the RNDIS command through the device's control endpoint

  @param[in]  UsbRndisDevice    A pointer to the USB_RNDIS_DEVICE instance.
  @param[in]  RndisMsg          A pointer to the REMOTE_NDIS_MSG_HEADER data.
  @param[out] RndisMsgResponse  A pointer to the REMOTE_NDIS_MSG_HEADER data for getting responses.

  @retval EFI_SUCCESS           The bulk transfer has been successfully executed.

**/
EFI_STATUS
RndisControlMsg (
  IN  USB_RNDIS_DEVICE        *UsbRndisDevice,
  IN  REMOTE_NDIS_MSG_HEADER  *RndisMsg,
  OUT REMOTE_NDIS_MSG_HEADER  *RndisMsgResponse
  )
{
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_DEVICE_REQUEST        DevReq;
  UINT32                        UsbStatus;
  EFI_STATUS                    Status;
  UINT32                        SaveResponseType;
  UINT32                        SaveResponseLength;
  UINT32                        Index;
  REMOTE_NDIS_INITIALIZE_CMPLT  *RndisInitCmplt;

  UsbIo              = UsbRndisDevice->UsbIo;
  SaveResponseType   = 0;
  SaveResponseLength = 0;
  RndisInitCmplt     = (REMOTE_NDIS_INITIALIZE_CMPLT *)RndisMsgResponse;

  if (RndisMsgResponse != NULL) {
    SaveResponseType   = RndisMsgResponse->MessageType;
    SaveResponseLength = RndisMsgResponse->MessageLength;
  }

  ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  DevReq.RequestType = USB_REQ_TYPE_CLASS | USB_TARGET_INTERFACE;
  DevReq.Request     = SEND_ENCAPSULATED_COMMAND;
  DevReq.Value       = 0;
  DevReq.Index       = 0;
  DevReq.Length      = (UINT16)RndisMsg->MessageLength;

  PrintRndisMsg (RndisMsg);

  Status = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &DevReq,
                    EfiUsbDataOut,
                    USB_ETHERNET_TRANSFER_TIMEOUT,
                    RndisMsg,
                    RndisMsg->MessageLength,
                    &UsbStatus
                    );

  DEBUG ((DEBUG_INFO, "RndisControlMsg: UsbStatus : %x Status : %r RndisMsgResponse : %lx\n", UsbStatus, Status, RndisMsgResponse));

  // Error or no response expected
  if ((EFI_ERROR (Status)) || (RndisMsgResponse == NULL)) {
    DEBUG ((DEBUG_INFO, "RndisControlMsg: UsbStatus : %x Status : %r\n", UsbStatus, Status));
    return Status;
  }

  for (Index = 0; Index < (RNDIS_CONTROL_TIMEOUT/100); Index++) {
    ReadRndisResponseInterrupt (UsbRndisDevice);
    ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

    DevReq.RequestType = USB_ENDPOINT_DIR_IN | USB_REQ_TYPE_CLASS | USB_TARGET_INTERFACE;
    DevReq.Request     = GET_ENCAPSULATED_RESPONSE;
    DevReq.Value       = 0;
    DevReq.Index       = 0;
    DevReq.Length      = (UINT16)RndisMsgResponse->MessageLength;

    Status = UsbIo->UsbControlTransfer (
                      UsbIo,
                      &DevReq,
                      EfiUsbDataIn,
                      USB_ETHERNET_TRANSFER_TIMEOUT,
                      RndisMsgResponse,
                      RndisMsgResponse->MessageLength,
                      &UsbStatus
                      );

    DEBUG ((DEBUG_INFO, "RndisControlMsg Response: UsbStatus : %x Status : %r \n", UsbStatus, Status));

    PrintRndisMsg (RndisMsgResponse);

    if (!EFI_ERROR (Status)) {
      if ((RndisInitCmplt->RequestID != ((REMOTE_NDIS_INITIALIZE_CMPLT *)RndisMsg)->RequestID) || (RndisInitCmplt->MessageType != SaveResponseType)) {
        DEBUG ((DEBUG_INFO, "Retry the response\n"));

        RndisMsgResponse->MessageType   = SaveResponseType;
        RndisMsgResponse->MessageLength = SaveResponseLength;
        continue;
      }
    }

    return Status;
  }

  DEBUG ((DEBUG_INFO, "RndisControlMsg: TimeOut\n"));

  return EFI_TIMEOUT;
}

/**
  This function send the RNDIS command through the device's Data endpoint

  @param[in]      UsbRndisDevice  A pointer to the USB_RNDIS_DEVICE instance.
  @param[in]      RndisMsg        A pointer to the REMOTE_NDIS_MSG_HEADER to send out.
  @param[in, out] TransferLength  The length of the RndisMsg data to transfer.

  @retval EFI_SUCCESS     The request executed successfully.

**/
EFI_STATUS
RndisTransmitDataMsg (
  IN      USB_RNDIS_DEVICE        *UsbRndisDevice,
  IN      REMOTE_NDIS_MSG_HEADER  *RndisMsg,
  IN OUT  UINTN                   *TransferLength
  )
{
  EFI_STATUS  Status;
  UINT32      UsbStatus;

  if (UsbRndisDevice->BulkInEndpoint == 0) {
    GetEndpoint (UsbRndisDevice->UsbIoCdcData, UsbRndisDevice);
  }

  PrintRndisMsg (RndisMsg);

  Status = UsbRndisDevice->UsbIoCdcData->UsbBulkTransfer (
                                           UsbRndisDevice->UsbIoCdcData,
                                           UsbRndisDevice->BulkOutEndpoint,
                                           RndisMsg,
                                           TransferLength,
                                           USB_TX_ETHERNET_BULK_TIMEOUT,
                                           &UsbStatus
                                           );

  if (Status == EFI_SUCCESS) {
    gStopBulkInCnt = MAXIMUM_STOPBULKIN_CNT;     // After sending cmd ,we will polling receive package for MAXIMUM_STOPBULKIN_CNT times
  }

  return Status;
}

/**
  This function send the RNDIS command through the device's Data endpoint

  @param[in]      UsbRndisDevice    A pointer to the USB_RNDIS_DEVICE instance.
  @param[in, out] RndisMsg          A pointer to the REMOTE_NDIS_MSG_HEADER to send out.
  @param[in, out] TransferLength    The length of the RndisMsg data to transfer.

  @retval EFI_SUCCESS     The request executed successfully.

**/
EFI_STATUS
RndisReceiveDataMsg (
  IN      USB_RNDIS_DEVICE        *UsbRndisDevice,
  IN OUT  REMOTE_NDIS_MSG_HEADER  *RndisMsg,
  IN OUT  UINTN                   *TransferLength
  )
{
  EFI_STATUS  Status;
  UINT32      UsbStatus;

  UsbStatus = 0;

  if (UsbRndisDevice->BulkInEndpoint == 0) {
    GetEndpoint (UsbRndisDevice->UsbIoCdcData, UsbRndisDevice);
  }

  // Use gStopBulkInCnt to stop BulkIn command
  if ((gStopBulkInCnt != 0) || LAN_BULKIN_CMD_CONTROL) {
    Status = UsbRndisDevice->UsbIoCdcData->UsbBulkTransfer (
                                             UsbRndisDevice->UsbIoCdcData,
                                             UsbRndisDevice->BulkInEndpoint,
                                             RndisMsg,
                                             TransferLength,
                                             USB_RX_ETHERNET_BULK_TIMEOUT,
                                             &UsbStatus
                                             );

    if (!EFI_ERROR (Status)) {
      gStopBulkInCnt = MINIMUM_STOPBULKIN_CNT;
    } else {
      gStopBulkInCnt--;
    }
  } else {
    Status          = EFI_TIMEOUT;
    *TransferLength = 0;
    gBlockBulkInCnt++;
  }

  if (gBlockBulkInCnt > BULKIN_CMD_POLLING_CNT) {
    gStopBulkInCnt  = MINIMUM_STOPBULKIN_CNT;
    gBlockBulkInCnt = 0;
  }

  PrintRndisMsg (RndisMsg);

  return Status;
}

/**
  Prints RNDIS Header and Data

  @param[in] RndisMsg    A pointer to the REMOTE_NDIS_MSG_HEADER data.

**/
VOID
PrintRndisMsg (
  IN  REMOTE_NDIS_MSG_HEADER  *RndisMsg
  )
{
  UINTN                    Length;
  REMOTE_NDIS_QUERY_CMPLT  *RndisQueryCmplt;

  Length = 0;

  switch (RndisMsg->MessageType) {
    case RNDIS_PACKET_MSG:
      DEBUG ((DEBUG_INFO, "RNDIS_PACKET_MSG:\n"));
      Length = sizeof (REMOTE_NDIS_PACKET_MSG) + 0x14;
      break;
    case RNDIS_INITIALIZE_MSG:
      DEBUG ((DEBUG_INFO, "RNDIS_INITIALIZE_MSG:\n"));
      Length = sizeof (REMOTE_NDIS_INITIALIZE_MSG);
      break;
    case RNDIS_INITIALIZE_CMPLT:
      DEBUG ((DEBUG_INFO, "RNDIS_INITIALIZE_CMPLT:\n"));
      Length = sizeof (REMOTE_NDIS_INITIALIZE_CMPLT);
      break;
    case RNDIS_HLT_MSG:
      DEBUG ((DEBUG_INFO, "RNDIS_HLT_MSG:\n"));
      Length = sizeof (REMOTE_NDIS_HALT_MSG);
      break;
    case RNDIS_QUERY_MSG:
      DEBUG ((DEBUG_INFO, "RNDIS_QUERY_MSG:\n"));
      Length = sizeof (REMOTE_NDIS_QUERY_MSG);
      break;
    case RNDIS_QUERY_CMPLT:
      DEBUG ((DEBUG_INFO, "RNDIS_QUERY_CMPLT:\n"));
      RndisQueryCmplt = (REMOTE_NDIS_QUERY_CMPLT *)RndisMsg;
      Length          = sizeof (REMOTE_NDIS_QUERY_CMPLT) + RndisQueryCmplt->InformationBufferLength;
      break;
    case RNDIS_SET_MSG:
      DEBUG ((DEBUG_INFO, "RNDIS_SET_MSG:\n"));
      Length = sizeof (REMOTE_NDIS_SET_MSG);
      break;
    case RNDIS_SET_CMPLT:
      DEBUG ((DEBUG_INFO, "RNDIS_SET_CMPLT:\n"));
      Length = sizeof (REMOTE_NDIS_SET_CMPLT);
      break;
    case RNDIS_RESET_MSG:
      DEBUG ((DEBUG_INFO, "RNDIS_RESET_MSG:\n"));
      Length = sizeof (REMOTE_NDIS_RESET_MSG);
      break;
    case RNDIS_RESET_CMPLT:
      DEBUG ((DEBUG_INFO, "RNDIS_RESET_CMPLT:\n"));
      Length = sizeof (REMOTE_NDIS_RESET_CMPLT);
      break;
    case RNDIS_INDICATE_STATUS_MSG:
      DEBUG ((DEBUG_INFO, "RNDIS_INDICATE_STATUS_MSG:\n"));
      Length = sizeof (REMOTE_NDIS_INDICATE_STATUS_MSG);
      break;
    case RNDIS_KEEPALIVE_MSG:
      DEBUG ((DEBUG_INFO, "RNDIS_KEEPALIVE_MSG:\n"));
      Length = sizeof (REMOTE_NDIS_KEEPALIVE_MSG);
      break;
    case RNDIS_KEEPALIVE_CMPLT:
      DEBUG ((DEBUG_INFO, "RNDIS_KEEPALIVE_CMPLT:\n"));
      Length = sizeof (REMOTE_NDIS_KEEPALIVE_CMPLT);
  }

  if (Length) {
    UINTN  Index;
    Index = 0;
    for ( ; Length; Length -= 4, Index++) {
      DEBUG ((DEBUG_INFO, "%8X\t", *((UINT32 *)RndisMsg + Index)));
      if (((Index % 4) == 3) && (Index != 0)) {
        DEBUG ((DEBUG_INFO, "\n"));
      }

      if ((Length < 8) && (Length > 4)) {
        UINT32  Data32;
        Index++;
        Data32 = *((UINT32 *)RndisMsg + Index);
        DEBUG ((DEBUG_INFO, "%8X\t", Data32));
        break;
      }
    }

    if ((Index % 4) != 0) {
      DEBUG ((DEBUG_INFO, "\n"));
    }
  }
}

/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:

    UsbIo.c

  Abstract:

    USB I/O Abstraction Driver

  Revision History

--*/

#include "usbbus.h"

//
// USB I/O Support Function Prototypes
//
STATIC
EFI_STATUS
EFIAPI
UsbControlTransfer (
  IN       EFI_USB_IO_PROTOCOL        *This,
  IN       EFI_USB_DEVICE_REQUEST     *Request,
  IN       EFI_USB_DATA_DIRECTION     Direction,
  IN       UINT32                     Timeout,
  IN OUT   VOID                       *Data, OPTIONAL
  IN       UINTN                      DataLength, OPTIONAL
  OUT      UINT32                     *Status
  );

STATIC
EFI_STATUS
EFIAPI
UsbBulkTransfer (
  IN       EFI_USB_IO_PROTOCOL     *This,
  IN       UINT8                   DeviceEndpoint,
  IN OUT   VOID                    *Data,
  IN OUT   UINTN                   *DataLength,
  IN       UINTN                   Timeout,
  OUT      UINT32                  *Status
  );

STATIC
EFI_STATUS
EFIAPI
UsbAsyncInterruptTransfer (
  IN EFI_USB_IO_PROTOCOL                 * This,
  IN UINT8                               DeviceEndpoint,
  IN BOOLEAN                             IsNewTransfer,
  IN UINTN                               PollingInterval, OPTIONAL
  IN UINTN                               DataLength, OPTIONAL
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK     InterruptCallBack, OPTIONAL
  IN VOID                                *Context OPTIONAL
  );

STATIC
EFI_STATUS
EFIAPI
UsbSyncInterruptTransfer (
  IN       EFI_USB_IO_PROTOCOL     *This,
  IN       UINT8                   DeviceEndpoint,
  IN OUT   VOID                    *Data,
  IN OUT   UINTN                   *DataLength,
  IN       UINTN                   Timeout,
  OUT      UINT32                  *Status
  );

STATIC
EFI_STATUS
EFIAPI
UsbIsochronousTransfer (
  IN       EFI_USB_IO_PROTOCOL     *This,
  IN       UINT8                   DeviceEndpoint,
  IN OUT   VOID                    *Data,
  IN       UINTN                   DataLength,
  OUT      UINT32                  *Status
  );

STATIC
EFI_STATUS
EFIAPI
UsbAsyncIsochronousTransfer (
  IN        EFI_USB_IO_PROTOCOL                 * This,
  IN        UINT8                               DeviceEndpoint,
  IN OUT    VOID                                *Data,
  IN        UINTN                               DataLength,
  IN        EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN        VOID                                *Context OPTIONAL
  );

STATIC
EFI_STATUS
EFIAPI
UsbGetDeviceDescriptor (
  IN  EFI_USB_IO_PROTOCOL           *This,
  OUT EFI_USB_DEVICE_DESCRIPTOR     *DeviceDescriptor
  );

STATIC
EFI_STATUS
EFIAPI
UsbGetActiveConfigDescriptor (
  IN  EFI_USB_IO_PROTOCOL           *This,
  OUT EFI_USB_CONFIG_DESCRIPTOR     *ConfigurationDescriptor
  );

STATIC
EFI_STATUS
EFIAPI
UsbGetInterfaceDescriptor (
  IN  EFI_USB_IO_PROTOCOL              *This,
  OUT EFI_USB_INTERFACE_DESCRIPTOR     *InterfaceDescriptor
  );

STATIC
EFI_STATUS
EFIAPI
UsbGetEndpointDescriptor (
  IN  EFI_USB_IO_PROTOCOL             *This,
  IN  UINT8                           EndpointIndex,
  OUT EFI_USB_ENDPOINT_DESCRIPTOR     *EndpointDescriptor
  );

STATIC
EFI_STATUS
EFIAPI
UsbGetStringDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *This,
  IN  UINT16                  LangID,
  IN  UINT8                   StringIndex,
  OUT CHAR16                  **String
  );

STATIC
EFI_STATUS
EFIAPI
UsbGetSupportedLanguages (
  IN  EFI_USB_IO_PROTOCOL      *This,
  OUT UINT16                   **LangIDTable,
  OUT UINT16                   *TableSize
  );

//
// USB I/O Interface structure
//
STATIC EFI_USB_IO_PROTOCOL  UsbIoInterface = {
  UsbControlTransfer,
  UsbBulkTransfer,
  UsbAsyncInterruptTransfer,
  UsbSyncInterruptTransfer,
  UsbIsochronousTransfer,
  UsbAsyncIsochronousTransfer,
  UsbGetDeviceDescriptor,
  UsbGetActiveConfigDescriptor,
  UsbGetInterfaceDescriptor,
  UsbGetEndpointDescriptor,
  UsbGetStringDescriptor,
  UsbGetSupportedLanguages,
  UsbPortReset
};

VOID
InitializeUsbIoInstance (
  IN USB_IO_CONTROLLER_DEVICE     *UsbIoController
  )
/*++

Routine Description:

  Initialize the instance of UsbIo controller

Arguments:

  UsbIoController - A pointer to controller structure of UsbIo

Returns:

--*/
{
  //
  // Copy EFI_USB_IO protocol instance
  //
  CopyMem (
    &UsbIoController->UsbIo,
    &UsbIoInterface,
    sizeof (EFI_USB_IO_PROTOCOL)
    );
}
//
// Implementation
//
STATIC
EFI_STATUS
EFIAPI
UsbControlTransfer (
  IN       EFI_USB_IO_PROTOCOL        *This,
  IN       EFI_USB_DEVICE_REQUEST     *Request,
  IN       EFI_USB_DATA_DIRECTION     Direction,
  IN       UINT32                     Timeout,
  IN OUT   VOID                       *Data, OPTIONAL
  IN       UINTN                      DataLength, OPTIONAL
  OUT      UINT32                     *Status
  )
/*++

  Routine Description:
    This function is used to manage a USB device with a control transfer pipe.

  Arguments:
    This        -   Indicates calling context.
    Request     -   A pointer to the USB device request that will be sent to
                    the USB device.
    Direction   -   Indicates the data direction.
    Data        -   A pointer to the buffer of data that will be transmitted
                    to USB device or received from USB device.
    Timeout     -   Indicates the transfer should be completed within this time
                    frame.
    DataLength  -   The size, in bytes, of the data buffer specified by Data.
    Status      -   A pointer to the result of the USB transfer.

  Returns:
    EFI_SUCCESS
    EFI_INVALID_PARAMETER
    EFI_OUT_OF_RESOURCES
    EFI_TIMEOUT
    EFI_DEVICE_ERROR

--*/
{
  USB_IO_CONTROLLER_DEVICE  *UsbIoController;

  EFI_STATUS                RetStatus;
  USB_IO_DEVICE             *UsbIoDev;
  UINT8                     MaxPacketLength;
  UINT32                    TransferResult;
  BOOLEAN                   Disconnected;
  //
  // Parameters Checking
  //
  if (Status == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // leave the HostController's ControlTransfer
  // to perform other parameters checking
  //
  UsbIoController = USB_IO_CONTROLLER_DEVICE_FROM_USB_IO_THIS (This);
  UsbIoDev        = UsbIoController->UsbDevice;

  MaxPacketLength = UsbIoDev->DeviceDescriptor.MaxPacketSize0;

 
  if (Request->Request     == USB_DEV_CLEAR_FEATURE && 
      Request->RequestType == 0x02                  && 
      Request->Value       == EfiUsbEndpointHalt) {
     //
     //Reduce the remove delay time for system response
     //
     IsDeviceDisconnected (UsbIoController, &Disconnected);
     if (!EFI_ERROR (Status) && Disconnected == TRUE) {
      DEBUG ((gUSBErrorLevel, "Device is disconnected when trying reset\n"));
      return EFI_DEVICE_ERROR;
    }
  }
  //
  // using HostController's ControlTransfer to complete the request
  //
  RetStatus = UsbVirtualHcControlTransfer (
                UsbIoDev->BusController,
                UsbIoDev->DeviceAddress,
                UsbIoDev->DeviceSpeed,
                MaxPacketLength,
                Request,
                Direction,
                Data,
                &DataLength,
                (UINTN) Timeout,
                UsbIoDev->Translator,
                &TransferResult
                );

  *Status = TransferResult;

  if (Request->Request     == USB_DEV_CLEAR_FEATURE && 
      Request->RequestType == 0x02                  && 
      Request->Value       == EfiUsbEndpointHalt) {
    //
    // This is a UsbClearEndpointHalt request
    // Need to clear data toggle
    // Request.Index == EndpointAddress
    //
    if (!EFI_ERROR (RetStatus) && TransferResult == EFI_USB_NOERROR) {
      SetDataToggleBit (
        This,
        (UINT8) Request->Index,
        0
        );
    }
  }
  return RetStatus;
}

STATIC
EFI_STATUS
EFIAPI
UsbBulkTransfer (
  IN       EFI_USB_IO_PROTOCOL     *This,
  IN       UINT8                   DeviceEndpoint,
  IN OUT   VOID                    *Data,
  IN OUT   UINTN                   *DataLength,
  IN       UINTN                   Timeout,
  OUT      UINT32                  *Status
  )
/*++

  Routine Description:
    This function is used to manage a USB device with the bulk transfer pipe.

  Arguments:
    This            - Indicates calling context.
    DeviceEndpoint  - The destination USB device endpoint to which the device
                      request is being sent.
    Data            - A pointer to the buffer of data that will be transmitted
                      to USB device or received from USB device.
    DataLength      - On input, the size, in bytes, of the data buffer
                      specified by Data.  On output, the number of bytes that
                      were actually transferred.
    Timeout         - Indicates the transfer should be completed within this
                      time frame.
    Status          - This parameter indicates the USB transfer status.

  Returns:
    EFI_SUCCESS
    EFI_INVALID_PARAMETER
    EFI_OUT_OF_RESOURCES
    EFI_TIMEOUT
    EFI_DEVICE_ERROR

--*/
{
  USB_IO_DEVICE               *UsbIoDev;
  UINTN                       MaxPacketLength;
  UINT8                       DataToggle;
  UINT8                       OldToggle;
  EFI_STATUS                  RetStatus;
  USB_IO_CONTROLLER_DEVICE    *UsbIoController;
  ENDPOINT_DESC_LIST_ENTRY    *EndPointListEntry;
  UINT8                       DataBuffersNumber;
  UINT32                      TransferResult;
  
  DataBuffersNumber = 1;
  UsbIoController   = USB_IO_CONTROLLER_DEVICE_FROM_USB_IO_THIS (This);
  UsbIoDev          = UsbIoController->UsbDevice;

  //
  // Parameters Checking
  //
  if ((DeviceEndpoint & 0x7F) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceEndpoint & 0x7F) > 15) {
    return EFI_INVALID_PARAMETER;
  }

  if (Status == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EndPointListEntry = FindEndPointListEntry (
                        This,
                        DeviceEndpoint
                        );

  if (EndPointListEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((EndPointListEntry->EndpointDescriptor.Attributes & 0x03) != 0x02) {
    return EFI_INVALID_PARAMETER;
  }
                        
  //
  // leave the HostController's BulkTransfer
  // to perform other parameters checking
  //
  GetDeviceEndPointMaxPacketLength (
    This,
    DeviceEndpoint,
    &MaxPacketLength
    );

  GetDataToggleBit (
    This,
    DeviceEndpoint,
    &DataToggle
    );

  OldToggle = DataToggle;

  //
  // using HostController's BulkTransfer to complete the request
  //
  RetStatus = UsbVirtualHcBulkTransfer (
                UsbIoDev->BusController,
                UsbIoDev->DeviceAddress,
                DeviceEndpoint,
                UsbIoDev->DeviceSpeed,
                MaxPacketLength,
                DataBuffersNumber,
                &Data,
                DataLength,
                &DataToggle,
                Timeout,
                UsbIoDev->Translator,
                &TransferResult
                );

  if (OldToggle != DataToggle) {
    //
    // Write the toggle back
    //
    SetDataToggleBit (
      This,
      DeviceEndpoint,
      DataToggle
      );
  }

  *Status = TransferResult;

  return RetStatus;
}

STATIC
EFI_STATUS
EFIAPI
UsbSyncInterruptTransfer (
  IN       EFI_USB_IO_PROTOCOL     *This,
  IN       UINT8                   DeviceEndpoint,
  IN OUT   VOID                    *Data,
  IN OUT   UINTN                   *DataLength,
  IN       UINTN                   Timeout,
  OUT      UINT32                  *Status
  )
/*++

  Routine Description:
    Usb Sync Interrupt Transfer

  Arguments:
    This            - Indicates calling context.
    DeviceEndpoint  - The destination USB device endpoint to which the device
                      request is being sent.
    Data            - A pointer to the buffer of data that will be transmitted
                      to USB device or received from USB device.
    DataLength      - On input, the size, in bytes, of the data buffer
                      specified by Data.  On output, the number of bytes that
                      were actually transferred.
    Timeout         - Indicates the transfer should be completed within this
                      time frame.
    Status          - This parameter indicates the USB transfer status.

  Returns:
    EFI_SUCCESS
    EFI_INVALID_PARAMETER
    EFI_OUT_OF_RESOURCES
    EFI_TIMEOUT
    EFI_DEVICE_ERROR

--*/
{
  USB_IO_DEVICE             *UsbIoDev;
  UINTN                     MaxPacketLength;
  UINT8                     DataToggle;
  UINT8                     OldToggle;
  EFI_STATUS                RetStatus;
  USB_IO_CONTROLLER_DEVICE  *UsbIoController;
  ENDPOINT_DESC_LIST_ENTRY  *EndPointListEntry;

  //
  // Parameters Checking
  //
  if ((DeviceEndpoint & 0x7F) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceEndpoint & 0x7F) > 15) {
    return EFI_INVALID_PARAMETER;
  }

  if (Status == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EndPointListEntry = FindEndPointListEntry (
                        This,
                        DeviceEndpoint
                        );

  if (EndPointListEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((EndPointListEntry->EndpointDescriptor.Attributes & 0x03) != 0x03) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // leave the HostController's SyncInterruptTransfer
  // to perform other parameters checking
  //
  UsbIoController = USB_IO_CONTROLLER_DEVICE_FROM_USB_IO_THIS (This);
  UsbIoDev        = UsbIoController->UsbDevice;
  GetDeviceEndPointMaxPacketLength (
    This,
    DeviceEndpoint,
    &MaxPacketLength
    );

  GetDataToggleBit (
    This,
    DeviceEndpoint,
    &DataToggle
    );

  OldToggle = DataToggle;
  //
  // using HostController's SyncInterruptTransfer to complete the request
  //
  RetStatus = UsbVirtualHcSyncInterruptTransfer (
                UsbIoDev->BusController,
                UsbIoDev->DeviceAddress,
                DeviceEndpoint,
                UsbIoDev->DeviceSpeed,
                MaxPacketLength,
                Data,
                DataLength,
                &DataToggle,
                Timeout,
                UsbIoDev->Translator,
                Status
                );

  if (OldToggle != DataToggle) {
    //
    // Write the toggle back
    //
    SetDataToggleBit (
      This,
      DeviceEndpoint,
      DataToggle
      );
  }

  return RetStatus;
}

STATIC
EFI_STATUS
EFIAPI
UsbAsyncInterruptTransfer (
  IN EFI_USB_IO_PROTOCOL                 *This,
  IN UINT8                               DeviceEndpoint,
  IN BOOLEAN                             IsNewTransfer,
  IN UINTN                               PollingInterval, OPTIONAL
  IN UINTN                               DataLength, OPTIONAL
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK     InterruptCallBack, OPTIONAL
  IN VOID                                *Context OPTIONAL
  )
/*++

  Routine Description:
    Usb Async Interrput Transfer

  Arguments:
    This              -   Indicates calling context.
    DeviceEndpoint    -   The destination USB device endpoint to which the
                          device request is being sent.
    IsNewTransfer     -   If TRUE, a new transfer will be submitted to USB
                          controller.  If FALSE,  the interrupt transfer is
                          deleted from the device's interrupt transfer queue.
    PollingInterval   -   Indicates the periodic rate, in milliseconds, that
                          the transfer is to be executed.
    DataLength        -   Specifies the length, in bytes, of the data to be
                          received from the USB device.
    InterruptCallBack -   The Callback function.  This function is called if
                          the asynchronous interrupt transfer is completed.
    Context           -   Passed to InterruptCallback 
  Returns:
    EFI_SUCCESS
    EFI_INVALID_PARAMETER
    EFI_OUT_OF_RESOURCES

--*/
{
  USB_IO_DEVICE             *UsbIoDev;
  UINTN                     MaxPacketLength;
  UINT8                     DataToggle;
  EFI_STATUS                RetStatus;
  USB_IO_CONTROLLER_DEVICE  *UsbIoController;
  ENDPOINT_DESC_LIST_ENTRY  *EndpointListEntry;

  //
  // Check endpoint
  //
  if ((DeviceEndpoint & 0x7F) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceEndpoint & 0x7F) > 15) {
    return EFI_INVALID_PARAMETER;
  }

  EndpointListEntry = FindEndPointListEntry (
                        This,
                        DeviceEndpoint
                        );

  if (EndpointListEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((EndpointListEntry->EndpointDescriptor.Attributes & 0x03) != 0x03) {
    return EFI_INVALID_PARAMETER;
  }

  UsbIoController = USB_IO_CONTROLLER_DEVICE_FROM_USB_IO_THIS (This);
  UsbIoDev        = UsbIoController->UsbDevice;

  if (!IsNewTransfer) {
    //
    // Delete this transfer
    //
    UsbVirtualHcAsyncInterruptTransfer (
      UsbIoDev->BusController,
      UsbIoDev->DeviceAddress,
      DeviceEndpoint,
      UsbIoDev->DeviceSpeed,
      0,
      FALSE,
      &DataToggle,
      PollingInterval,
      DataLength,
      UsbIoDev->Translator,
      NULL,
      NULL
      );

    //
    // We need to store the toggle value
    //
    SetDataToggleBit (
      This,
      DeviceEndpoint,
      DataToggle
      );

    return EFI_SUCCESS;
  }

  GetDeviceEndPointMaxPacketLength (
    This,
    DeviceEndpoint,
    &MaxPacketLength
    );

  GetDataToggleBit (
    This,
    DeviceEndpoint,
    &DataToggle
    );

  RetStatus = UsbVirtualHcAsyncInterruptTransfer (
                UsbIoDev->BusController,
                UsbIoDev->DeviceAddress,
                DeviceEndpoint,
                UsbIoDev->DeviceSpeed,
                MaxPacketLength,
                TRUE,
                &DataToggle,
                PollingInterval,
                DataLength,
                UsbIoDev->Translator,
                InterruptCallBack,
                Context
                );

  return RetStatus;
}

STATIC
EFI_STATUS
EFIAPI
UsbIsochronousTransfer (
  IN       EFI_USB_IO_PROTOCOL     *This,
  IN       UINT8                   DeviceEndpoint,
  IN OUT   VOID                    *Data,
  IN       UINTN                   DataLength,
  OUT      UINT32                  *Status
  )
/*++

  Routine Description:
    Usb Isochronous Transfer

  Arguments:
    This              -   Indicates calling context.
    DeviceEndpoint    -   The destination USB device endpoint to which the
                          device request is being sent.
    Data              -   A pointer to the buffer of data that will be
                          transmitted to USB device or received from USB device.
    DataLength        -   The size, in bytes, of the data buffer specified by
                          Data.
    Status            -   This parameter indicates the USB transfer status.

  Returns:
    EFI_SUCCESS
    EFI_INVALID_PARAMETER
    EFI_OUT_OF_RESOURCES
    EFI_TIMEOUT
    EFI_DEVICE_ERROR
    EFI_UNSUPPORTED
--*/
{
  //
  // Currently we don't support this transfer
  //
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
UsbAsyncIsochronousTransfer (
  IN        EFI_USB_IO_PROTOCOL                 *This,
  IN        UINT8                               DeviceEndpoint,
  IN OUT    VOID                                *Data,
  IN        UINTN                               DataLength,
  IN        EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN        VOID                                *Context OPTIONAL
  )
/*++

Routine Description:

  Usb Async Isochronous Transfer

Arguments:

  This                - EFI_USB_IO_PROTOCOL
  DeviceEndpoint      - DeviceEndpoint number
  Data                - Data to transfer
  DataLength          - DataLength
  IsochronousCallBack - Isochronous CallBack function
  Context             - Passed to IsochronousCallBack function
Returns:

  EFI_UNSUPPORTED     - Unsupported now

--*/
{
  //
  // Currently we don't support this transfer
  //
  return EFI_UNSUPPORTED;
}
//
// Here is new definitions
//
STATIC
EFI_STATUS
EFIAPI
UsbGetDeviceDescriptor (
  IN  EFI_USB_IO_PROTOCOL           *This,
  OUT EFI_USB_DEVICE_DESCRIPTOR     *DeviceDescriptor
  )
/*++

  Routine Description:
    Retrieves the USB Device Descriptor.

  Arguments:
    This              -   Indicates the calling context.
    DeviceDescriptor  -   A pointer to the caller allocated USB Device
                          Descriptor.

  Returns:
    EFI_SUCCESS
    EFI_INVALID_PARAMETER
    EFI_NOT_FOUND

--*/
{
  USB_IO_CONTROLLER_DEVICE  *UsbIoController;
  USB_IO_DEVICE             *UsbIoDev;

  //
  // This function just wrapps UsbGetDeviceDescriptor.
  //
  
  if (DeviceDescriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbIoController = USB_IO_CONTROLLER_DEVICE_FROM_USB_IO_THIS (This);
  UsbIoDev        = UsbIoController->UsbDevice;

  if (!UsbIoDev->IsConfigured) {
    return EFI_NOT_FOUND;
  }

  CopyMem (
    DeviceDescriptor,
    &UsbIoDev->DeviceDescriptor,
    sizeof (EFI_USB_DEVICE_DESCRIPTOR)
    );

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UsbGetActiveConfigDescriptor (
  IN  EFI_USB_IO_PROTOCOL           *This,
  OUT EFI_USB_CONFIG_DESCRIPTOR     *ConfigurationDescriptor
  )
/*++

  Routine Description:
    Retrieves the current USB configuration Descriptor.

  Arguments:
    This                     -   Indicates the calling context.
    ConfigurationDescriptor  -   A pointer to the caller allocated USB active
                                 Configuration Descriptor.

  Returns:
    EFI_SUCCESS
    EFI_INVALID_PARAMETER
    EFI_NOT_FOUND

--*/
{
  USB_IO_DEVICE             *UsbIoDev;
  USB_IO_CONTROLLER_DEVICE  *UsbIoController;

  //
  // This function just wrapps UsbGetActiveConfigDescriptor.
  //
  if (ConfigurationDescriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbIoController = USB_IO_CONTROLLER_DEVICE_FROM_USB_IO_THIS (This);
  UsbIoDev        = UsbIoController->UsbDevice;

  if (!UsbIoDev->IsConfigured) {
    return EFI_NOT_FOUND;
  }

  CopyMem (
    ConfigurationDescriptor,
    &(UsbIoDev->ActiveConfig->CongfigDescriptor),
    sizeof (EFI_USB_CONFIG_DESCRIPTOR)
    );

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UsbGetInterfaceDescriptor (
  IN  EFI_USB_IO_PROTOCOL              *This,
  OUT EFI_USB_INTERFACE_DESCRIPTOR     *InterfaceDescriptor
  )
/*++

  Routine Description:
    Retrieves the interface Descriptor for that controller.

  Arguments:
    This                  -   Indicates the calling context.
    InterfaceDescriptor   -   A pointer to the caller allocated USB interface
                              Descriptor.

  Returns:
    EFI_SUCCESS
    EFI_INVALID_PARAMETER
    EFI_NOT_FOUND

--*/
{
  INTERFACE_DESC_LIST_ENTRY *InterfaceListEntry;

  if (InterfaceDescriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InterfaceListEntry = FindInterfaceListEntry (This);

  if (InterfaceListEntry == NULL) {
    return EFI_NOT_FOUND;
  }

  CopyMem (
    InterfaceDescriptor,
    &(InterfaceListEntry->InterfaceDescriptor),
    sizeof (EFI_USB_INTERFACE_DESCRIPTOR)
    );

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UsbGetEndpointDescriptor (
  IN  EFI_USB_IO_PROTOCOL             *This,
  IN  UINT8                           EndpointIndex,
  OUT EFI_USB_ENDPOINT_DESCRIPTOR     *EndpointDescriptor
  )
/*++

  Routine Description:
    Retrieves the endpoint Descriptor for a given endpoint.

  Arguments:
    This                  -   Indicates the calling context.
    EndpointIndex         -   Indicates which endpoint descriptor to retrieve.
                              The valid range is 0..15.
    EndpointDescriptor    -   A pointer to the caller allocated USB Endpoint
                              Descriptor of a USB controller.

  Returns:
    EFI_SUCCESS           -   The endpoint descriptor was retrieved successfully.
    EFI_INVALID_PARAMETER -   EndpointIndex is not valid.
                          -   EndpointDescriptor is NULL.
    EFI_NOT_FOUND         -   The endpoint descriptor cannot be found.
                              The device may not be correctly configured.

--*/
{
  INTERFACE_DESC_LIST_ENTRY *InterfaceListEntry;
  LIST_ENTRY            *EndpointListHead;
  ENDPOINT_DESC_LIST_ENTRY  *EndpointListEntry;

  if (EndpointDescriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (EndpointIndex > 15) {
    return EFI_INVALID_PARAMETER;
  }

  InterfaceListEntry = FindInterfaceListEntry (This);

  if (InterfaceListEntry == NULL) {
    return EFI_NOT_FOUND;
  }

  EndpointListHead  = (LIST_ENTRY *) (&InterfaceListEntry->EndpointDescListHead);
  EndpointListEntry = (ENDPOINT_DESC_LIST_ENTRY *) (EndpointListHead->ForwardLink);

  if (EndpointIndex >= InterfaceListEntry->InterfaceDescriptor.NumEndpoints) {
    return EFI_NOT_FOUND;
  }
  //
  // Loop all endpoint descriptor to get match one.
  //
  while (EndpointIndex != 0) {
    EndpointListEntry = (ENDPOINT_DESC_LIST_ENTRY *) (EndpointListEntry->Link.ForwardLink);
    EndpointIndex--;
  }

  CopyMem (
    EndpointDescriptor,
    &EndpointListEntry->EndpointDescriptor,
    sizeof (EFI_USB_ENDPOINT_DESCRIPTOR)
    );

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UsbGetSupportedLanguages (
  IN  EFI_USB_IO_PROTOCOL     *This,
  OUT UINT16                  **LangIDTable,
  OUT UINT16                  *TableSize
  )
/*++

  Routine Description:
    Get all the languages that the USB device supports

  Arguments:
    This        -   Indicates the calling context.
    LangIDTable -   Language ID for the string the caller wants to get.
    TableSize   -   The size, in bytes, of the table LangIDTable.

  Returns:
    EFI_SUCCESS
    EFI_NOT_FOUND

--*/
{
  USB_IO_DEVICE             *UsbIoDev;
  USB_IO_CONTROLLER_DEVICE  *UsbIoController;
  UINTN                     Index;
  BOOLEAN                   Found;

  UsbIoController = USB_IO_CONTROLLER_DEVICE_FROM_USB_IO_THIS (This);
  UsbIoDev        = UsbIoController->UsbDevice;

  Found           = FALSE;
  Index           = 0;
  //
  // Loop language table
  //
  while (UsbIoDev->LangID[Index]) {
    Found = TRUE;
    Index++;
  }

  if (!Found) {
    return EFI_NOT_FOUND;
  }

  *LangIDTable  = UsbIoDev->LangID;
  *TableSize    = (UINT16) Index;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UsbGetStringDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *This,
  IN  UINT16                  LangID,
  IN  UINT8                   StringIndex,
  OUT CHAR16                  **String
  )
/*++

  Routine Description:
    Get a given string descriptor

  Arguments:
    This          -   Indicates the calling context.
    LangID        -   The Language ID for the string being retrieved.
    StringIndex  -   The ID of the string being retrieved.
    String        -   A pointer to a buffer allocated by this function
                      with AllocatePool() to store the string.  If this
                      function returns EFI_SUCCESS, it stores the string
                      the caller wants to get.  The caller should release
                      the string buffer with FreePool() after the string
                      is not used any more.
  Returns:
    EFI_SUCCESS
    EFI_NOT_FOUND
    EFI_OUT_OF_RESOURCES
    EFI_UNSUPPORTED

--*/
{
  UINT32                    Status;
  EFI_STATUS                Result;
  EFI_USB_STRING_DESCRIPTOR *StrDescriptor;
  UINT8                     *Buffer;
  CHAR16                    *UsbString;
  UINT16                    TempBuffer;
  USB_IO_DEVICE             *UsbIoDev;
  UINT8                     Index;
  BOOLEAN                   Found;
  USB_IO_CONTROLLER_DEVICE  *UsbIoController;

  if (StringIndex == 0) {
    return EFI_NOT_FOUND;
  }
  //
  // Search LanguageID, check if it is supported by this device
  //
  if (LangID == 0) {
    return EFI_NOT_FOUND;
  }

  UsbIoController = USB_IO_CONTROLLER_DEVICE_FROM_USB_IO_THIS (This);
  UsbIoDev        = UsbIoController->UsbDevice;

  Found           = FALSE;
  Index           = 0;
  while (UsbIoDev->LangID[Index]) {
    if (UsbIoDev->LangID[Index] == LangID) {
      Found = TRUE;
      break;
    }

    Index++;
  }

  if (!Found) {
    return EFI_NOT_FOUND;
  }
  //
  // Get String Length
  //
  Result = UsbGetString (
            This,
            LangID,
            StringIndex,
            &TempBuffer,
            2,
            &Status
            );
  if (EFI_ERROR (Result)) {
    return EFI_NOT_FOUND;
  }

  StrDescriptor = (EFI_USB_STRING_DESCRIPTOR *) &TempBuffer;

  if (StrDescriptor->Length == 0) {
    return EFI_UNSUPPORTED;
  }

  Buffer = AllocateZeroPool (StrDescriptor->Length);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Result = UsbGetString (
            This,
            LangID,
            StringIndex,
            Buffer,
            StrDescriptor->Length,
            &Status
            );

  if (EFI_ERROR (Result)) {
    gBS->FreePool (Buffer);
    return EFI_NOT_FOUND;
  }

  StrDescriptor = (EFI_USB_STRING_DESCRIPTOR *) Buffer;

  //
  // UsbString is a UNICODE string
  //
  UsbString = AllocateZeroPool (StrDescriptor->Length);
  if (UsbString == NULL) {
    gBS->FreePool (Buffer);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (
    (VOID *) UsbString,
    Buffer + 2,
    StrDescriptor->Length - 2
    );

  *String = UsbString;

  gBS->FreePool (Buffer);

  return EFI_SUCCESS;
}

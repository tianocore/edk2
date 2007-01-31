/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  cbi1.c

Abstract:
    cbi1 transportation protocol implementation files

--*/

#include "cbi.h"

//
// CBI Function prototypes
//
STATIC
EFI_STATUS
CBI1CommandPhase (
  IN  USB_CBI_DEVICE          *UsbCbiDev,
  IN  VOID                    *Command,
  IN  UINT8                   CommandSize,
  OUT UINT32                  *Result
  );

STATIC
EFI_STATUS
CBI1DataPhase (
  IN  USB_CBI_DEVICE          *UsbCbiDev,
  IN  UINT32                  DataSize,
  IN  OUT VOID                *DataBuffer,
  IN  EFI_USB_DATA_DIRECTION  Direction,
  IN  UINT16                  Timeout,
  OUT UINT32                  *Result
  );

//
// USB Atapi implementation
//
STATIC
EFI_STATUS
EFIAPI
CBI1AtapiCommand (
  IN  EFI_USB_ATAPI_PROTOCOL    *This,
  IN  VOID                      *Command,
  IN  UINT8                     CommandSize,
  IN  VOID                      *DataBuffer,
  IN  UINT32                    BufferLength,
  IN  EFI_USB_DATA_DIRECTION    Direction,
  IN  UINT16                    TimeOutInMilliSeconds
  );

STATIC
EFI_STATUS
EFIAPI
CBI1MassStorageReset (
  IN  EFI_USB_ATAPI_PROTOCOL      *This,
  IN  BOOLEAN                     ExtendedVerification
  );

//
// CBI1 Driver Binding Protocol
//
STATIC
EFI_STATUS
EFIAPI
CBI1DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

STATIC
EFI_STATUS
EFIAPI
CBI1DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

STATIC
EFI_STATUS
EFIAPI
CBI1DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     ControllerHandle,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );

STATIC
VOID
Cbi1ReportStatusCode (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_STATUS_CODE_TYPE      CodeType,
  IN EFI_STATUS_CODE_VALUE     Value
  );


EFI_DRIVER_BINDING_PROTOCOL   gUsbCbi1DriverBinding = {
  CBI1DriverBindingSupported,
  CBI1DriverBindingStart,
  CBI1DriverBindingStop,
  0xa,
  NULL,
  NULL
};

STATIC EFI_USB_ATAPI_PROTOCOL CBI1AtapiProtocol = {
  CBI1AtapiCommand,
  CBI1MassStorageReset,
  0
};

//
// CBI1 Driver Binding implementation
//
STATIC
EFI_STATUS
EFIAPI
CBI1DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

  Routine Description:
    Test to see if this driver supports ControllerHandle. Any ControllerHandle 
    than contains a BlockIo and DiskIo protocol can be supported.

  Arguments:
    This                - Protocol instance pointer.
    ControllerHandle    - Handle of device to test
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS         - This driver supports this device
    EFI_ALREADY_STARTED - This driver is already running on this device
    other               - This driver does not support this device

--*/
{
  EFI_STATUS                    Status;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;

  //
  // Check if the Controller supports USB IO protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Get the Controller interface descriptor
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    &InterfaceDescriptor
                    );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }
  //
  // Bug here: just let Vendor specific CBI protocol get supported
  //
  if (!((InterfaceDescriptor.InterfaceClass == 0xFF) &&
        (InterfaceDescriptor.InterfaceProtocol == 0))) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

Exit:
  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiUsbIoProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );
  return Status;

}

STATIC
EFI_STATUS
EFIAPI
CBI1DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

  Routine Description:
    Start this driver on ControllerHandle by opening a Block IO and Disk IO 
    protocol, reading Device Path, and creating a child handle with a 
    Disk IO and device path protocol.

  Arguments:
    This                - Protocol instance pointer.
    ControllerHandle    - Handle of device to bind driver to
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS         - This driver is added to DeviceHandle
    EFI_ALREADY_STARTED - This driver is already running on DeviceHandle
    other               - This driver does not support this device

--*/
{
  USB_CBI_DEVICE                *UsbCbiDev;
  UINT8                         Index;
  EFI_USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
  EFI_STATUS                    Status;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  BOOLEAN                       Found;

  Found = FALSE;
  //
  // Check if the Controller supports USB IO protocol
  //
  UsbCbiDev = NULL;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Get the controller interface descriptor
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    &InterfaceDescriptor
                    );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  CBI1AtapiProtocol.CommandProtocol = InterfaceDescriptor.InterfaceSubClass;

  UsbCbiDev                         = AllocateZeroPool (sizeof (USB_CBI_DEVICE));
  if (UsbCbiDev == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  UsbCbiDev->Signature            = USB_CBI_DEVICE_SIGNATURE;
  UsbCbiDev->UsbIo                = UsbIo;
  CopyMem (&UsbCbiDev->InterfaceDescriptor, &InterfaceDescriptor, sizeof (InterfaceDescriptor));
  CopyMem (&UsbCbiDev->UsbAtapiProtocol   , &CBI1AtapiProtocol,   sizeof (CBI1AtapiProtocol));

  //
  // Get the Device Path Protocol on Controller's handle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &UsbCbiDev->DevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  for (Index = 0; Index < InterfaceDescriptor.NumEndpoints; Index++) {
    UsbIo->UsbGetEndpointDescriptor (
            UsbIo,
            Index,
            &EndpointDescriptor
            );

    //
    // We parse bulk endpoint
    //
    if (EndpointDescriptor.Attributes == 0x02) {
      if (EndpointDescriptor.EndpointAddress & 0x80) {
        CopyMem (&UsbCbiDev->BulkInEndpointDescriptor, &EndpointDescriptor, sizeof (EndpointDescriptor));
	  } else {
        CopyMem (&UsbCbiDev->BulkOutEndpointDescriptor, &EndpointDescriptor, sizeof (EndpointDescriptor));
	  }

      Found = TRUE;
    }
    //
    // We parse interrupt endpoint
    //
    if (EndpointDescriptor.Attributes == 0x03) {
      CopyMem (&UsbCbiDev->InterruptEndpointDescriptor, &EndpointDescriptor, sizeof (EndpointDescriptor));
      Found = TRUE;
    }

  }
  //
  // Double check we have these
  //
  if (!Found) {
    goto ErrorExit;
  }
  //
  // After installing Usb-Atapi protocol onto this handle
  // it will be called by upper layer drivers such as Fat
  //
  Cbi1ReportStatusCode (
    UsbCbiDev->DevicePath,
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_ENABLE)
    );

  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &gEfiUsbAtapiProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &UsbCbiDev->UsbAtapiProtocol
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  UsbCbiDev->ControllerNameTable = NULL;
  AddUnicodeString (
    "eng",
    gUsbCbi1ComponentName.SupportedLanguages,
    &UsbCbiDev->ControllerNameTable,
    (CHAR16 *) L"Usb Cbi1 Mass Storage"
    );

  return EFI_SUCCESS;

ErrorExit:
  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiUsbIoProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );
  if (UsbCbiDev != NULL) {
    gBS->FreePool (UsbCbiDev);
  }

  return Status;

}

STATIC
EFI_STATUS
EFIAPI
CBI1DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     ControllerHandle,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  )
/*++

  Routine Description:
    Stop this driver on ControllerHandle. Support stoping any child handles 
    created by this driver.

  Arguments:
    This              - Protocol instance pointer.
    ControllerHandle  - Handle of device to stop driver on 
    NumberOfChildren  - Number of Children in the ChildHandleBuffer
    ChildHandleBuffer - List of handles for the children we need to stop.

  Returns:
    EFI_SUCCESS         - This driver is removed DeviceHandle
    EFI_UNSUPPORTED     - Can't open the gEfiUsbAtapiProtocolGuid protocol 
    other               - This driver was not removed from this device
   
--*/
{
  EFI_STATUS              Status;
  EFI_USB_ATAPI_PROTOCOL  *CBI1AtapiProtocol;
  USB_CBI_DEVICE          *UsbCbiDev;

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUsbAtapiProtocolGuid,
                  (VOID **) &CBI1AtapiProtocol,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  UsbCbiDev = USB_CBI_DEVICE_FROM_THIS (CBI1AtapiProtocol);

  Cbi1ReportStatusCode (
    UsbCbiDev->DevicePath,
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_DISABLE)
    );

  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEfiUsbAtapiProtocolGuid,
                  &UsbCbiDev->UsbAtapiProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->CloseProtocol (
                  ControllerHandle,
                  &gEfiUsbIoProtocolGuid,
                  This->DriverBindingHandle,
                  ControllerHandle
                  );
  gBS->FreePool (UsbCbiDev);

  return Status;

}
//
// CBI1 command
//
STATIC
EFI_STATUS
CBI1CommandPhase (
  IN  USB_CBI_DEVICE          *UsbCbiDev,
  IN  VOID                    *Command,
  IN  UINT8                   CommandSize,
  OUT UINT32                  *Result
  )
/*++

  Routine Description:
    In order to make consistence, CBI transportation protocol does only use
     the first 3 parameters. Other parameters are not used here.

  Arguments:
    UsbCbiDev    - USB_CBI_DEVICE
    Command      - Command to send 
    CommandSize  - Command Size
    Result       - Result to return

  Returns:
    EFI_SUCCESS         - This driver is removed DeviceHandle
    other               - This driver was not removed from this device 
--*/
{
  EFI_STATUS              Status;
  EFI_USB_IO_PROTOCOL     *UsbIo;
  EFI_USB_DEVICE_REQUEST  Request;
  UINT32                  TimeOutInMilliSeconds;

  UsbIo = UsbCbiDev->UsbIo;

  ZeroMem (&Request, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Device request see CBI specification
  //
  Request.RequestType   = 0x21;
  Request.Length        = CommandSize;

  TimeOutInMilliSeconds = 1000;

  Status = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbDataOut,
                    TimeOutInMilliSeconds,
                    Command,
                    CommandSize,
                    Result
                    );

  return Status;
}

STATIC
EFI_STATUS
CBI1DataPhase (
  IN  USB_CBI_DEVICE              *UsbCbiDev,
  IN  UINT32                      DataSize,
  IN  OUT VOID                    *DataBuffer,
  IN  EFI_USB_DATA_DIRECTION      Direction,
  IN  UINT16                      Timeout,
  OUT UINT32                      *Result
  )
/*++

Routine Description:

  CBI1 Data Phase

Arguments:

  UsbCbiDev   - USB_CBI_DEVICE
  DataSize    - Data Size
  DataBuffer  - Data Buffer
  Direction   - IN/OUT/NODATA
  Timeout     - Time out value in milliseconds
  Result      - Transfer result

Returns:

  EFI_SUCCESS - Success

--*/
{
  EFI_STATUS          Status;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               EndpointAddr;
  UINTN               Remain;
  UINTN               Increment;
  UINT32              MaxPacketLen;
  UINT8               *BufferPtr;

  UsbIo     = UsbCbiDev->UsbIo;

  Remain    = DataSize;
  BufferPtr = (UINT8 *) DataBuffer;

  //
  // retrieve the the max packet length of the given endpoint
  //
  if (Direction == EfiUsbDataIn) {
    MaxPacketLen  = (UsbCbiDev->BulkInEndpointDescriptor).MaxPacketSize;
    EndpointAddr  = (UsbCbiDev->BulkInEndpointDescriptor).EndpointAddress;
  } else {
    MaxPacketLen  = (UsbCbiDev->BulkOutEndpointDescriptor).MaxPacketSize;
    EndpointAddr  = (UsbCbiDev->BulkOutEndpointDescriptor).EndpointAddress;
  }

  while (Remain > 0) {
    //
    // Using 15 packets to aVOID Bitstuff error
    //
    if (Remain > 15 * MaxPacketLen) {
      Increment = 15 * MaxPacketLen;
    } else {
      Increment = Remain;
    }

    Status = UsbIo->UsbBulkTransfer (
                      UsbIo,
                      EndpointAddr,
                      BufferPtr,
                      &Increment,
                      Timeout,
                      Result
                      );

    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    BufferPtr += Increment;
    Remain -= Increment;
  }

  return EFI_SUCCESS;

ErrorExit:

  if (Direction == EfiUsbDataIn) {
    Cbi1ReportStatusCode (
      UsbCbiDev->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_EC_INPUT_ERROR)
      );
  } else {
    Cbi1ReportStatusCode (
      UsbCbiDev->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_EC_OUTPUT_ERROR)
      );
  }

  if (((*Result) & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
    //
    // just endpoint stall happens
    //
    UsbClearEndpointHalt (
      UsbIo,
      EndpointAddr,
      Result
      );
  }

  return Status;
}
//
// CBI1 USB ATAPI Protocol
//
STATIC
EFI_STATUS
EFIAPI
CBI1MassStorageReset (
  IN  EFI_USB_ATAPI_PROTOCOL      *This,
  IN  BOOLEAN                     ExtendedVerification
  )
/*++

  Routine Description:
    Reset CBI Devices
    
  Arguments:
    This                    - Protocol instance pointer.
    ExtendedVerification    - TRUE if we need to do strictly reset.

  Returns:
    EFI_SUCCESS         - Command succeeded.
    EFI_DEVICE_ERROR    - Command failed.

--*/
{
  UINT8               ResetCommand[12];
  EFI_USB_IO_PROTOCOL *UsbIo;
  USB_CBI_DEVICE      *UsbCbiDev;
  UINT8               EndpointAddr;
  UINT32              Result;

  UsbCbiDev = USB_CBI_DEVICE_FROM_THIS (This);
  UsbIo     = UsbCbiDev->UsbIo;

  Cbi1ReportStatusCode (
    UsbCbiDev->DevicePath,
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_RESET)
    );

  if (ExtendedVerification) {
    UsbIo->UsbPortReset (UsbIo);
  }
  //
  // CBI reset command protocol
  //
  SetMem (ResetCommand, sizeof (ResetCommand), 0xff);
  ResetCommand[0] = 0x1d;
  ResetCommand[1] = 0x04;

  CBI1CommandPhase (
    UsbCbiDev,
    ResetCommand,
    12,
    &Result
    );

  //
  // clear bulk in endpoint stall feature
  //
  EndpointAddr = UsbCbiDev->BulkInEndpointDescriptor.EndpointAddress;
  UsbClearEndpointHalt (
    UsbIo,
    EndpointAddr,
    &Result
    );

  //
  // clear bulk out endpoint stall feature
  //
  EndpointAddr = UsbCbiDev->BulkOutEndpointDescriptor.EndpointAddress;
  UsbClearEndpointHalt (
    UsbIo,
    EndpointAddr,
    &Result
    );

  return EFI_SUCCESS;

}

STATIC
EFI_STATUS
EFIAPI
CBI1AtapiCommand (
  IN  EFI_USB_ATAPI_PROTOCOL  *This,
  IN  VOID                    *Command,
  IN  UINT8                   CommandSize,
  IN  VOID                    *DataBuffer,
  IN  UINT32                  BufferLength,
  IN  EFI_USB_DATA_DIRECTION  Direction,
  IN  UINT16                  TimeOutInMilliSeconds
  )
/*++

  Routine Description:
    Send ATAPI command using CBI1 protocol.
    
  Arguments:
    This              - Protocol instance pointer.
    Command           - Command buffer 
    CommandSize       - Size of Command Buffer
    DataBuffer        - Data buffer
    BufferLength      - Length of Data buffer
    Direction         - Data direction of this command
    TimeOutInMilliSeconds - Timeout value in ms

  Returns:
    EFI_SUCCESS         - Command succeeded.
    EFI_DEVICE_ERROR    - Command failed.

--*/
{
  EFI_STATUS      Status;
  USB_CBI_DEVICE  *UsbCbiDev;
  UINT32          Result;
  UINT8           Index;
  UINT8           MaxRetryNum;

  UsbCbiDev   = USB_CBI_DEVICE_FROM_THIS (This);

  MaxRetryNum = 3;

  for (Index = 0; Index < MaxRetryNum; Index++) {
  
    //
    // First send ATAPI command through CBI1
    //
    Status = CBI1CommandPhase (
              UsbCbiDev,
              Command,
              CommandSize,
              &Result
              );
    if (EFI_ERROR (Status)) {

      switch (Result) {

      case EFI_USB_NOERROR:
      case EFI_USB_ERR_STALL:
      case EFI_USB_ERR_SYSTEM:
        return EFI_DEVICE_ERROR;

      default:
        continue;
        break;
      }
    } else {
      break;
    }
  }

  if (Index == MaxRetryNum) {
    return EFI_DEVICE_ERROR;
  }

  for (Index = 0; Index < MaxRetryNum; Index++) {
    //
    // Send/Get Data if there is a Data Stage
    //
    switch (Direction) {

    case EfiUsbDataIn:
    case EfiUsbDataOut:
      Status = CBI1DataPhase (
                UsbCbiDev,
                BufferLength,
                DataBuffer,
                Direction,
                TimeOutInMilliSeconds,
                &Result
                );

      if (EFI_ERROR (Status)) {
        switch (Result) {

        case EFI_USB_NOERROR:
        case EFI_USB_ERR_STALL:
        case EFI_USB_ERR_SYSTEM:
          return EFI_DEVICE_ERROR;

        default:
          continue;
          break;
        }

      } else {

        return EFI_SUCCESS;
      }
      break;

    case EfiUsbNoData:
      return EFI_SUCCESS;
    }
  }
  //
  // If goes here, means met error.
  //
  return EFI_DEVICE_ERROR;
}

STATIC
VOID
Cbi1ReportStatusCode (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_STATUS_CODE_TYPE      CodeType,
  IN EFI_STATUS_CODE_VALUE     Value
  )
/*++

  Routine Description:
    Report Status Code in Usb Cbi1 Driver

  Arguments:
    DevicePath  - Use this to get Device Path
    CodeType    - Status Code Type
    CodeValue   - Status Code Value

  Returns:
    None

--*/
{
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    CodeType,
    Value,
    DevicePath
    );

}

/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Cbi0.c

Abstract:

--*/

#include "cbi.h"

//
// Bot Driver Binding Protocol
//
STATIC
EFI_STATUS
EFIAPI
Cbi0DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

STATIC
EFI_STATUS
EFIAPI
Cbi0DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

STATIC
EFI_STATUS
EFIAPI
Cbi0DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     ControllerHandle,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );

STATIC
VOID
Cbi0ReportStatusCode (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_STATUS_CODE_TYPE      CodeType,
  IN EFI_STATUS_CODE_VALUE     Value
  );


EFI_DRIVER_BINDING_PROTOCOL         gUsbCbi0DriverBinding = {
  Cbi0DriverBindingSupported,
  Cbi0DriverBindingStart,
  Cbi0DriverBindingStop,
  0xa,
  NULL,
  NULL
};

STATIC
EFI_STATUS
Cbi0RecoveryReset (
  IN  USB_CBI_DEVICE   *UsbCbiDev
  );

STATIC
EFI_STATUS
Cbi0CommandPhase (
  IN  USB_CBI_DEVICE            *UsbCbiDev,
  IN  VOID                      *Command,
  IN  UINT8                     CommandSize,
  IN  UINT16                    Timeout
  );

STATIC
EFI_STATUS
Cbi0DataPhase (
  IN     USB_CBI_DEVICE            *UsbCbiDev,
  IN     UINT32                    *DataSize,
  IN OUT VOID                      *DataBuffer,
  IN     EFI_USB_DATA_DIRECTION    Direction,
  IN     UINT16                    Timeout
  );

STATIC
EFI_STATUS
Cbi0StatusPhase (
  IN  USB_CBI_DEVICE        *UsbCbiDev,
  OUT INTERRUPT_DATA_BLOCK  *InterruptDataBlock,
  IN  UINT16                Timeout
  );

//
// USB Atapi protocol prototype
//
STATIC
EFI_STATUS
EFIAPI
Cbi0AtapiCommand (
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
Cbi0MassStorageReset (
  IN  EFI_USB_ATAPI_PROTOCOL    *This,
  IN  BOOLEAN                   ExtendedVerification
  );

STATIC EFI_USB_ATAPI_PROTOCOL       Cbi0AtapiProtocol = {
  Cbi0AtapiCommand,
  Cbi0MassStorageReset,
  0
};

STATIC
EFI_STATUS
EFIAPI
Cbi0DriverBindingSupported (
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
  // Get the Default interface descriptor, now we only
  // suppose interface 1
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    &InterfaceDescriptor
                    );
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    return Status;
  }
  //
  // Check if it is a Cbi0 Type Mass Storage Device
  //
  if((InterfaceDescriptor.InterfaceClass != MASS_STORAGE_CLASS) ||
     (InterfaceDescriptor.InterfaceProtocol != CBI0_INTERFACE_PROTOCOL)) {
    Status = EFI_UNSUPPORTED;
  } else {
    Status = EFI_SUCCESS;
  }

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
Cbi0DriverBindingStart (
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
    EFI_OUT_OF_RESOURCES- Can't allocate memory 
    EFI_UNSUPPORTED     - Endpoint is not as expected
--*/
{
  USB_CBI_DEVICE                *UsbCbiDev;
  UINT8                         Index;
  EFI_USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
  EFI_STATUS                    Status;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  UINT8                         EndpointExistMask;

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
    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    return Status;
  }

  Cbi0AtapiProtocol.CommandProtocol = InterfaceDescriptor.InterfaceSubClass;

  UsbCbiDev                         = AllocateZeroPool (sizeof (USB_CBI_DEVICE));
  if (UsbCbiDev == NULL) {
    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    return EFI_OUT_OF_RESOURCES;
  }

  UsbCbiDev->Signature            = USB_CBI_DEVICE_SIGNATURE;
  UsbCbiDev->UsbIo                = UsbIo;
  CopyMem (&UsbCbiDev->InterfaceDescriptor, &InterfaceDescriptor, sizeof (InterfaceDescriptor));
  CopyMem (&UsbCbiDev->UsbAtapiProtocol, &Cbi0AtapiProtocol, sizeof (Cbi0AtapiProtocol));

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
  //
  // Mask used to see whether all three kinds of endpoints exist,
  // Mask value:
  //  bit0: bulk in endpoint;
  //  bit1: bulk out endpoint;
  //  bit2: interrupt in endpoint;
  //
  EndpointExistMask = 0;
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
	    EndpointExistMask |= bit (0);
      } else {
        CopyMem (&UsbCbiDev->BulkOutEndpointDescriptor, &EndpointDescriptor, sizeof (EndpointDescriptor));
	    EndpointExistMask |= bit (1);
      }
    }
    //
    // We parse interrupt endpoint
    //
    if (EndpointDescriptor.Attributes == 0x03) {
      CopyMem (&UsbCbiDev->InterruptEndpointDescriptor, &EndpointDescriptor, sizeof (EndpointDescriptor));
      EndpointExistMask |= bit (2);
    }

  }
  //
  // Double check we have all endpoints needed
  //
  if (EndpointExistMask != (bit (0) | bit (1) | bit (2))) {
    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    if (UsbCbiDev != NULL) {
      gBS->FreePool (UsbCbiDev);
    }

    return EFI_UNSUPPORTED;
  }
  //
  // After installing Usb-Atapi protocol onto this handle
  // it will be called by upper layer drivers such as Fat
  //
  Cbi0ReportStatusCode (
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

  UsbCbiDev->ControllerNameTable = NULL;
  AddUnicodeString (
    "eng",
    gUsbCbi0ComponentName.SupportedLanguages,
    &UsbCbiDev->ControllerNameTable,
    (CHAR16 *) L"Usb Cbi0 Mass Storage"
    );

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
Cbi0DriverBindingStop (
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
    ControllerHandle - Handle of device to stop driver on
    NumberOfChildren  - Number of Children in the ChildHandleBuffer
    ChildHandleBuffer - List of handles for the children we need to stop.

  Returns:
    EFI_SUCCESS         - This driver is removed DeviceHandle
    EFI_UNSUPPORTED     - This driver was not removed from this device

--*/
{
  EFI_STATUS              Status;
  EFI_USB_ATAPI_PROTOCOL  *Cbi0AtapiProtocol;
  USB_CBI_DEVICE          *UsbCbiDev;

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUsbAtapiProtocolGuid,
                  (VOID **) &Cbi0AtapiProtocol,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  UsbCbiDev = USB_CBI_DEVICE_FROM_THIS (Cbi0AtapiProtocol);

  Cbi0ReportStatusCode (
    UsbCbiDev->DevicePath,
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_DISABLE)
    );

  //
  // Uninstall protocol
  //
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
  //
  // Free all allocated resources
  //
  if (UsbCbiDev->ControllerNameTable) {
    FreeUnicodeStringTable (UsbCbiDev->ControllerNameTable);
  }

  gBS->FreePool (UsbCbiDev);

  return Status;
}


STATIC
EFI_STATUS
Cbi0RecoveryReset (
  IN  USB_CBI_DEVICE   *UsbCbiDev
  )
/*++

Routine Description:

  Cbi0 Recovery Reset routine

Arguments:

  UsbCbiDev - Cbi0RecoveryReset

Returns:

  EFI_SUCCESS - Success

--*/
{
  UINT8               ResetCommand[12];
  EFI_STATUS          Status;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               EndpointAddress;
  UINT32              Result;
  UINT16              Timeout;

  UsbIo = UsbCbiDev->UsbIo;

  Cbi0ReportStatusCode (
    UsbCbiDev->DevicePath,
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_RESET)
    );
  //
  // CBI reset command protocol
  //
  SetMem (ResetCommand, sizeof (ResetCommand), 0xff);
  ResetCommand[0] = 0x1d;
  ResetCommand[1] = 0x04;

  //
  // (in millisecond unit)
  //
  Timeout = STALL_1_SECOND;

  Status = Cbi0AtapiCommand (
            &UsbCbiDev->UsbAtapiProtocol,
            ResetCommand,
            12,
            NULL,
            0,
            EfiUsbNoData,
            Timeout
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->Stall (100 * 1000);
  //
  // clear bulk in endpoint stall feature
  //
  EndpointAddress = UsbCbiDev->BulkInEndpointDescriptor.EndpointAddress;
  Status = UsbClearEndpointHalt (
            UsbIo,
            EndpointAddress,
            &Result
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // clear bulk out endpoint stall feature
  //
  EndpointAddress = UsbCbiDev->BulkOutEndpointDescriptor.EndpointAddress;
  Status = UsbClearEndpointHalt (
            UsbIo,
            EndpointAddress,
            &Result
            );
  //
  // according to CBI spec, no need to clear interrupt endpoint feature.
  //
  return Status;
}

STATIC
EFI_STATUS
Cbi0CommandPhase (
  IN  USB_CBI_DEVICE          *UsbCbiDev,
  IN  VOID                    *Command,
  IN  UINT8                   CommandSize,
  IN  UINT16                  Timeout
  )
/*++

  Routine Description:
    Send ATAPI command through CBI0 interface.

  Arguments:
    UsbCbiDev   - USB_CBI_DEVICE
    Command     - Command to send
    CommandSize - Command size
    Timeout     - Time out value in milliseconds
  Returns:
    EFI_SUCCESS      - Success
    EFI_DEVICE_ERROR - Fail
    Others

--*/
{
  EFI_STATUS              Status;
  UINT32                  Result;
  EFI_USB_IO_PROTOCOL     *UsbIo;
  EFI_USB_DEVICE_REQUEST  Request;

  UsbIo = UsbCbiDev->UsbIo;

  ZeroMem (&Request, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Device request see CBI specification
  //
  Request.RequestType = 0x21;
  Request.Request     = 0x00;
  Request.Value       = 0;
  Request.Index       = 0;
  Request.Length      = CommandSize;

  Status = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbDataOut,
                    Timeout,
                    Command,
                    CommandSize,
                    &Result
                    );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
Cbi0DataPhase (
  IN      USB_CBI_DEVICE             *UsbCbiDev,
  IN      UINT32                     *DataSize,
  IN  OUT VOID                       *DataBuffer,
  IN      EFI_USB_DATA_DIRECTION     Direction,
  IN      UINT16                     Timeout
  )
/*++

  Routine Description:
    Get/Send Data through CBI0 interface

  Arguments:
    UsbCbiDev   - USB_CBI_DEVICE
    DataSize    - Data size
    DataBuffer  - Data buffer
    Direction   - IN/OUT/NODATA
    Timeout     - Time out value in milliseconds
  Returns:
    EFI_SUCCESS
    Others

--*/
{
  EFI_STATUS          Status;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               EndpointAddress;
  UINTN               Remain;
  UINTN               Increment;
  UINT32              MaxPacketLength;
  UINT8               *BufferPtr;
  UINT32              Result;
  UINTN               TransferredSize;

  UsbIo           = UsbCbiDev->UsbIo;

  Remain          = *DataSize;
  BufferPtr       = (UINT8 *) DataBuffer;
  TransferredSize = 0;
  //
  // retrieve the the max packet length of the given endpoint
  //
  if (Direction == EfiUsbDataIn) {
    MaxPacketLength = UsbCbiDev->BulkInEndpointDescriptor.MaxPacketSize;
    EndpointAddress = UsbCbiDev->BulkInEndpointDescriptor.EndpointAddress;
  } else {
    MaxPacketLength = UsbCbiDev->BulkOutEndpointDescriptor.MaxPacketSize;
    EndpointAddress = UsbCbiDev->BulkOutEndpointDescriptor.EndpointAddress;
  }

  while (Remain > 0) {

    if (Remain > 16 * MaxPacketLength) {
      Increment = 16 * MaxPacketLength;
    } else {
      Increment = Remain;
    }

    Status = UsbIo->UsbBulkTransfer (
                      UsbIo,
                      EndpointAddress,
                      BufferPtr,
                      &Increment,
                      Timeout,
                      &Result
                      );
    TransferredSize += Increment;

    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    BufferPtr += Increment;
    Remain -= Increment;
  }

  return EFI_SUCCESS;

ErrorExit:

  if (Direction == EfiUsbDataIn) {
    Cbi0ReportStatusCode (
      UsbCbiDev->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_EC_INPUT_ERROR)
      );
  } else {
    Cbi0ReportStatusCode (
      UsbCbiDev->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_EC_OUTPUT_ERROR)
      );
  }

  if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
    Status = Cbi0RecoveryReset (UsbCbiDev);
  }

  *DataSize = (UINT32) TransferredSize;
  return Status;
}

STATIC
EFI_STATUS
Cbi0StatusPhase (
  IN  USB_CBI_DEVICE        *UsbCbiDev,
  OUT INTERRUPT_DATA_BLOCK  *InterruptDataBlock,
  IN  UINT16                Timeout
  )
/*++

  Routine Description:
    Get transfer status through BOT interface

  Arguments:
    UsbCbiDev           -  USB_CBI_DEVICE
    InterruptDataBlock  -  Interrupt Data Block for interrupt transfer
    Timeout             -  Time out value in milliseconds  
  Returns:
    EFI_SUCCESS
    Others

--*/
{
  UINT8       EndpointAddress;
  UINTN       InterruptDataBlockLength;
  UINT32      Result;
  EFI_STATUS  Status;

  ZeroMem (InterruptDataBlock, sizeof (INTERRUPT_DATA_BLOCK));

  EndpointAddress           = UsbCbiDev->InterruptEndpointDescriptor.EndpointAddress;
  InterruptDataBlockLength  = sizeof (INTERRUPT_DATA_BLOCK);

  Status = UsbCbiDev->UsbIo->UsbSyncInterruptTransfer (
                              UsbCbiDev->UsbIo,
                              EndpointAddress,
                              InterruptDataBlock,
                              &InterruptDataBlockLength,
                              Timeout,
                              &Result
                              );
  if (EFI_ERROR (Status)) {
    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      //
      // just endpoint stall happens
      //
      UsbClearEndpointHalt (
        UsbCbiDev->UsbIo,
        EndpointAddress,
        &Result
        );
      gBS->Stall (100 * 1000);
    }

    return Status;
  }

  return EFI_SUCCESS;
}
//
// Cbi0 Atapi Protocol Implementation
//
STATIC
EFI_STATUS
EFIAPI
Cbi0MassStorageReset (
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
  EFI_STATUS          Status;
  USB_CBI_DEVICE      *UsbCbiDev;

  UsbCbiDev = USB_CBI_DEVICE_FROM_THIS (This);

  if (ExtendedVerification) {
    //
    //    UsbIo->UsbPortReset (UsbIo);
    //
  }

  Status = Cbi0RecoveryReset (UsbCbiDev);
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
Cbi0AtapiCommand (
  IN  EFI_USB_ATAPI_PROTOCOL      *This,
  IN  VOID                        *Command,
  IN  UINT8                       CommandSize,
  IN  VOID                        *DataBuffer,
  IN  UINT32                      BufferLength,
  IN  EFI_USB_DATA_DIRECTION      Direction,
  IN  UINT16                      TimeOutInMilliSeconds
  )
/*++

  Routine Description:
    Send ATAPI command using BOT protocol.

  Arguments:
    This                  - Protocol instance pointer.
    Command               - Command buffer
    CommandSize           - Size of Command Buffer
    DataBuffer            - Data buffer
    BufferLength          - Length of Data buffer
    Direction             - Data direction of this command
    TimeOutInMilliSeconds - Timeout value in ms

  Returns:
    EFI_SUCCESS           - Command succeeded.
    EFI_DEVICE_ERROR      - Command failed.
    EFI_INVALID_PARAMETER - Invalidate parameter 
--*/
{
  EFI_STATUS            Status;
  USB_CBI_DEVICE        *UsbCbiDev;
  UINT32                BufferSize;
  INTERRUPT_DATA_BLOCK  InterruptDataBlock;
  EFI_STATUS            DataPhaseStatus;

  if (Direction != EfiUsbNoData) {
    if (DataBuffer == NULL || BufferLength == 0) {
      return EFI_INVALID_PARAMETER;
    }
  }

  DataPhaseStatus = EFI_SUCCESS;
  //
  // Get the context
  //
  UsbCbiDev = USB_CBI_DEVICE_FROM_THIS (This);

  //
  // First send ATAPI command through Cbi
  //
  Status = Cbi0CommandPhase (
            UsbCbiDev,
            Command,
            CommandSize,
            TimeOutInMilliSeconds
            );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Send/Get Data if there is a Data Stage
  //
  switch (Direction) {

  case EfiUsbDataIn:
  case EfiUsbDataOut:
    BufferSize = BufferLength;

    DataPhaseStatus = Cbi0DataPhase (
                        UsbCbiDev,
                        &BufferSize,
                        DataBuffer,
                        Direction,
                        TimeOutInMilliSeconds
                        );
    break;

  case EfiUsbNoData:
    break;
  }

  if (EFI_ERROR (DataPhaseStatus)) {
    return EFI_DEVICE_ERROR;
  }
  
  //
  // Status Phase
  //
  Status = Cbi0StatusPhase (
            UsbCbiDev,
            &InterruptDataBlock,
            TimeOutInMilliSeconds
            );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (This->CommandProtocol != EFI_USB_SUBCLASS_UFI) {

    if (InterruptDataBlock.bType == 0) {
      //
      // indicates command completion
      //
      switch (InterruptDataBlock.bValue & 0x03) {

      case 0:
        Status = EFI_SUCCESS;
        break;

      case 1:
        Status = EFI_DEVICE_ERROR;
        break;

      case 2:
        Status = Cbi0RecoveryReset (UsbCbiDev);
        if (EFI_ERROR (Status)) {
          UsbCbiDev->UsbIo->UsbPortReset (UsbCbiDev->UsbIo);
        }

        Status = EFI_DEVICE_ERROR;
        break;

      case 3:
        Status = EFI_DEVICE_ERROR;
      }
    } else {
      Status = DataPhaseStatus;
    }

  } else {
    //
    // UFI device, InterruptDataBlock.bType: ASC (Additional Sense Code)
    //             InterruptDataBlock.bValue: ASCQ (Additional Snese Code Qualifier)
    //
    Status = DataPhaseStatus;
  }

  return Status;
}

STATIC
VOID
Cbi0ReportStatusCode (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_STATUS_CODE_TYPE      CodeType,
  IN EFI_STATUS_CODE_VALUE     Value
  )
/*++

  Routine Description:
    Report Status Code in Usb Cbi0 Driver

  Arguments:
    DevicePath   - Use this to get Device Path
    CodeType     - Status Code Type
    CodeValue    - Status Code Value

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

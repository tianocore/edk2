/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BOT.c

Abstract:

--*/

#include "bot.h"

//
// Function prototypes
//
EFI_STATUS
EFIAPI
UsbBotDriverEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

//
// Bot Driver Binding Protocol
//
EFI_STATUS
EFIAPI
BotDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
BotDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
BotDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     ControllerHandle,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );


EFI_DRIVER_BINDING_PROTOCOL   gUsbBotDriverBinding = {
  BotDriverBindingSupported,
  BotDriverBindingStart,
  BotDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// Bot Protocol
//
STATIC
EFI_STATUS
BotCommandPhase (
  IN  USB_BOT_DEVICE            *UsbBotDev,
  IN  VOID                      *Command,
  IN  UINT8                     CommandSize,
  IN  UINT32                    DataTransferLength,
  IN  EFI_USB_DATA_DIRECTION    Direction,
  IN  UINT16                    Timeout
  );

STATIC
EFI_STATUS
BotDataPhase (
  IN     USB_BOT_DEVICE            *UsbBotDev,
  IN     UINT32                    *DataSize,
  IN OUT VOID                      *DataBuffer,
  IN     EFI_USB_DATA_DIRECTION    Direction,
  IN     UINT16                    Timeout
  );

STATIC
EFI_STATUS
BotStatusPhase (
  IN  USB_BOT_DEVICE      *UsbBotDev,
  OUT UINT8               *TransferStatus,
  IN  UINT16              Timeout
  );

//
// USB Atapi protocol prototype
//
STATIC
EFI_STATUS
EFIAPI
BotAtapiCommand (
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
BotMassStorageReset (
  IN  EFI_USB_ATAPI_PROTOCOL    *This,
  IN  BOOLEAN                   ExtendedVerification
  );

VOID
BotReportStatusCode (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_STATUS_CODE_TYPE      CodeType,
  IN EFI_STATUS_CODE_VALUE     Value
  );

STATIC EFI_USB_ATAPI_PROTOCOL BotAtapiProtocol = {
  BotAtapiCommand,
  BotMassStorageReset,
  0
};

EFI_STATUS
EFIAPI
BotDriverBindingSupported (
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
    goto Exit;
  }
  //
  // Check if it is a BOT type Mass Storage Device
  //
  if ((InterfaceDescriptor.InterfaceClass    != 0x08) ||
      (InterfaceDescriptor.InterfaceProtocol != BOT)) {
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

EFI_STATUS
EFIAPI
BotDriverBindingStart (
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
    EFI_OUT_OF_RESOURCES- Can't allocate the memory resource
    other               - This driver does not support this device

--*/
{
  USB_BOT_DEVICE                *UsbBotDev;
  UINT8                         Index;
  EFI_USB_ENDPOINT_DESCRIPTOR   *EndpointDescriptor;
  EFI_USB_INTERFACE_DESCRIPTOR  *InterfaceDescriptor;
  EFI_STATUS                    Status;
  EFI_USB_IO_PROTOCOL           *UsbIo;

  //
  // Check if the Controller supports USB IO protocol
  //
  UsbBotDev = NULL;

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

  InterfaceDescriptor = AllocateZeroPool (sizeof (EFI_USB_INTERFACE_DESCRIPTOR));
  if (InterfaceDescriptor == NULL) {
    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Get the controller interface descriptor,
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    InterfaceDescriptor
                    );
  if (EFI_ERROR (Status)) {
    gBS->FreePool (InterfaceDescriptor);
    goto ErrorExit;
  }

  BotAtapiProtocol.CommandProtocol  = InterfaceDescriptor->InterfaceSubClass;

  UsbBotDev                         = AllocateZeroPool (sizeof (USB_BOT_DEVICE));
  if (UsbBotDev == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    gBS->FreePool (InterfaceDescriptor);
    goto ErrorExit;
  }

  UsbBotDev->Signature            = USB_BOT_DEVICE_SIGNATURE;
  UsbBotDev->UsbIo                = UsbIo;
  UsbBotDev->InterfaceDescriptor  = InterfaceDescriptor;
  CopyMem (&UsbBotDev->UsbAtapiProtocol, &BotAtapiProtocol, sizeof (BotAtapiProtocol));

  //
  // Get the Device Path Protocol on Controller's handle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &UsbBotDev->DevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  for (Index = 0; Index < InterfaceDescriptor->NumEndpoints; Index++) {
    EndpointDescriptor = AllocatePool (sizeof (EFI_USB_INTERFACE_DESCRIPTOR));
    if (EndpointDescriptor == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    UsbIo->UsbGetEndpointDescriptor (
            UsbIo,
            Index,
            EndpointDescriptor
            );

    //
    // We parse bulk endpoint
    //
    if ((EndpointDescriptor->Attributes & 0x03) == 0x02) {
      if ((EndpointDescriptor->EndpointAddress & 0x80) != 0) {
        UsbBotDev->BulkInEndpointDescriptor = EndpointDescriptor;
      } else {
        UsbBotDev->BulkOutEndpointDescriptor = EndpointDescriptor;
      }

      continue;
    }

    gBS->FreePool (EndpointDescriptor);
  }
  //
  // Double check we have these endpoint descriptors
  //
  if (!(UsbBotDev->BulkInEndpointDescriptor &&
        UsbBotDev->BulkOutEndpointDescriptor)) {
    Status = EFI_DEVICE_ERROR;
    goto ErrorExit;
  }
  //
  // After installing Usb-Atapi protocol onto this handle
  // it will be called by upper layer drivers such as Fat
  //
  BotReportStatusCode (
    UsbBotDev->DevicePath,
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_ENABLE)
    );

  //
  // Install Usb-Atapi Protocol onto the handle
  //
  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &gEfiUsbAtapiProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &UsbBotDev->UsbAtapiProtocol
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  UsbBotDev->ControllerNameTable = NULL;
  AddUnicodeString (
    "eng",
    gUsbBotComponentName.SupportedLanguages,
    &UsbBotDev->ControllerNameTable,
    (CHAR16 *) L"Usb Bot Mass Storage"
    );

  return EFI_SUCCESS;

ErrorExit:
  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiUsbIoProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );

  if (UsbBotDev != NULL) {
    if (UsbBotDev->InterfaceDescriptor != NULL) {
      gBS->FreePool (UsbBotDev->InterfaceDescriptor);
    }

    if (UsbBotDev->BulkInEndpointDescriptor != NULL) {
      gBS->FreePool (UsbBotDev->BulkInEndpointDescriptor);
    }

    if (UsbBotDev->BulkOutEndpointDescriptor != NULL) {
      gBS->FreePool (UsbBotDev->BulkOutEndpointDescriptor);
    }

    gBS->FreePool (UsbBotDev);
  }

  return Status;
}

EFI_STATUS
EFIAPI
BotDriverBindingStop (
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
    EFI_SUCCESS       - This driver is removed DeviceHandle
    EFI_UNSUPPORTED   - Can't open the gEfiUsbAtapiProtocolGuid protocl  
    other             - This driver was not removed from this device

--*/
{
  EFI_STATUS              Status;
  EFI_USB_ATAPI_PROTOCOL  *BotAtapiProtocol;
  USB_BOT_DEVICE          *UsbBotDev;

  EFI_USB_IO_PROTOCOL     *UsbIo;

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUsbAtapiProtocolGuid,
                  (VOID **) &BotAtapiProtocol,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  UsbBotDev = USB_BOT_DEVICE_FROM_THIS (BotAtapiProtocol);

  //
  // After installing Usb-Atapi protocol onto this handle
  // it will be called by upper layer drivers such as Fat
  //
  UsbIo = UsbBotDev->UsbIo;

  BotReportStatusCode (
    UsbBotDev->DevicePath,
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_DISABLE)
    );

  //
  // Uninstall protocol
  //
  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEfiUsbAtapiProtocolGuid,
                  &UsbBotDev->UsbAtapiProtocol
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
  if (UsbBotDev->InterfaceDescriptor != NULL) {
    gBS->FreePool (UsbBotDev->InterfaceDescriptor);
  }

  if (UsbBotDev->BulkInEndpointDescriptor != NULL) {
    gBS->FreePool (UsbBotDev->BulkInEndpointDescriptor);
  }

  if (UsbBotDev->BulkOutEndpointDescriptor != NULL) {
    gBS->FreePool (UsbBotDev->BulkOutEndpointDescriptor);
  }

  if (UsbBotDev->ControllerNameTable) {
    FreeUnicodeStringTable (UsbBotDev->ControllerNameTable);
  }

  gBS->FreePool (UsbBotDev);

  return Status;
}

STATIC
EFI_STATUS
BotRecoveryReset (
  IN  USB_BOT_DEVICE          *UsbBotDev
  )
/*++

Routine Description:

  Bot reset routine

Arguments:

  UsbBotDev - USB_BOT_DEVICE pointer

Returns:
  EFI_SUCCESS - Success the operation
  
--*/
{
  EFI_STATUS              Status;
  UINT32                  Result;
  EFI_USB_DEVICE_REQUEST  Request;
  EFI_USB_IO_PROTOCOL     *UsbIo;
  UINT8                   EndpointAddr;

  UsbIo = UsbBotDev->UsbIo;

  BotReportStatusCode (
    UsbBotDev->DevicePath,
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_RESET)
    );

  ZeroMem (&Request, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // See BOT specification
  //
  Request.RequestType = 0x21;
  Request.Request     = 0xFF;

  Status = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbNoData,
                    TIMEOUT_VALUE,
                    NULL,
                    0,
                    &Result
                    );

  gBS->Stall (100 * 1000);

  if (!EFI_ERROR (Status)) {
    //
    // clear bulk in endpoint stall feature
    //
    EndpointAddr = UsbBotDev->BulkInEndpointDescriptor->EndpointAddress;

    Status = UsbClearEndpointHalt (
               UsbIo,
               EndpointAddr,
               &Result
               );

    //
    // clear bulk out endpoint stall feature
    //
    EndpointAddr = UsbBotDev->BulkOutEndpointDescriptor->EndpointAddress;
    Status = UsbClearEndpointHalt (
               UsbIo,
               EndpointAddr,
               &Result
               );
  }

  return Status;
}
//
// Bot Protocol Implementation
//
STATIC
EFI_STATUS
BotCommandPhase (
  IN  USB_BOT_DEVICE          *UsbBotDev,
  IN  VOID                    *Command,
  IN  UINT8                   CommandSize,
  IN  UINT32                  DataTransferLength,
  IN  EFI_USB_DATA_DIRECTION  Direction,
  IN  UINT16                  Timeout
  )
/*++

  Routine Description:
    Send ATAPI command through BOT interface.

  Parameters:
    UsbBotDev           -   USB_BOT_DEVICE
    Command             -   command packet
    CommandSize         -   Command size
    DataTransferLength  -   Data Transfer Length
    Direction           -   Data IN/OUT/NODATA
    Timeout             -   Time out value in milliseconds
  Return Values:
    EFI_SUCCESS
    Others

--*/
{
  CBW                 cbw;
  EFI_STATUS          Status;
  UINT32              Result;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINTN               DataSize;

  UsbIo = UsbBotDev->UsbIo;

  ZeroMem (&cbw, sizeof (CBW));

  //
  // Fill the command block, detailed see BOT spec
  //
  cbw.dCBWSignature           = CBWSIG;
  cbw.dCBWTag                 = 0x01;
  cbw.dCBWDataTransferLength  = DataTransferLength;
  cbw.bmCBWFlags              = (UINT8) (Direction << 7);
  cbw.bCBWCBLength            = CommandSize;

  CopyMem (cbw.CBWCB, Command, CommandSize);

  DataSize = sizeof (CBW);

  Status = UsbIo->UsbBulkTransfer (
                    UsbIo,
                    (UsbBotDev->BulkOutEndpointDescriptor)->EndpointAddress,
                    &cbw,
                    &DataSize,
                    Timeout,
                    &Result
                    );
  if (EFI_ERROR (Status)) {
    //
    // Command phase fail, we need to recovery reset this device
    //
    BotRecoveryReset (UsbBotDev);
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
BotDataPhase (
  IN      USB_BOT_DEVICE            *UsbBotDev,
  IN      UINT32                    *DataSize,
  IN  OUT VOID                      *DataBuffer,
  IN      EFI_USB_DATA_DIRECTION    Direction,
  IN      UINT16                    Timeout
  )
/*++

  Routine Description:
    Get/Send Data through BOT interface

  Parameters:
    UsbBotDev    -  USB_BOT_DEVICE pointer
    DataSize     -  Data size
    DataBuffer   -  Data buffer pointer
    Direction    -  IN/OUT/NODATA
    Timeout      -  Time out value in milliseconds
  Return Value:
    EFI_SUCCESS
    Others

--*/
{
  EFI_STATUS          Status;
  UINT32              Result;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               EndpointAddr;
  UINTN               Remain;
  UINTN               Increment;
  UINT32              MaxPacketLen;
  UINT8               *BufferPtr;
  UINTN               TransferredSize;
  UINTN               RetryTimes;
  UINTN               MaxRetry;
  UINTN               BlockSize;
  UINTN               PackageNum;

  UsbIo           = UsbBotDev->UsbIo;
  Remain          = *DataSize;
  BufferPtr       = (UINT8 *) DataBuffer;
  TransferredSize = 0;
  MaxRetry        = 10;
  PackageNum      = 128;

  //
  // retrieve the the max packet length of the given endpoint
  //
  if (Direction == EfiUsbDataIn) {
    MaxPacketLen  = (UsbBotDev->BulkInEndpointDescriptor)->MaxPacketSize;
    EndpointAddr  = (UsbBotDev->BulkInEndpointDescriptor)->EndpointAddress;
  } else {
    MaxPacketLen  = (UsbBotDev->BulkOutEndpointDescriptor)->MaxPacketSize;
    EndpointAddr  = (UsbBotDev->BulkOutEndpointDescriptor)->EndpointAddress;
  }

  RetryTimes  = MaxRetry;
  BlockSize   = PackageNum * MaxPacketLen;
  while (Remain > 0) {
    //
    // Using 15 packets to aVOID Bitstuff error
    //
    if (Remain > PackageNum * MaxPacketLen) {
      Increment = BlockSize;
    } else {
      Increment = Remain;
    }

    Status = UsbIo->UsbBulkTransfer (
                      UsbIo,
                      EndpointAddr,
                      BufferPtr,
                      &Increment,
                      Timeout,
                      &Result
                      );

    TransferredSize += Increment;

    if (EFI_ERROR (Status)) {
      RetryTimes--;
      if ((RetryTimes == 0) || ((Result & EFI_USB_ERR_TIMEOUT) == 0)) {
        goto ErrorExit;
      }

      TransferredSize -= Increment;
      continue;
    } else {
      //
      // we try MaxTetry times for every bulk transfer
      //
      RetryTimes = MaxRetry;
    }

    BufferPtr += Increment;
    Remain -= Increment;
    if (Increment < BlockSize && TransferredSize <= *DataSize) {
      //
      // we get to the end of transter and transter size is
      // less than requriedsize
      //
      break;
    }
  }

  *DataSize = (UINT32) TransferredSize;

  return EFI_SUCCESS;

ErrorExit:
  if (Direction == EfiUsbDataIn) {
    BotReportStatusCode (
      UsbBotDev->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_EC_INPUT_ERROR)
      );
  } else {
    BotReportStatusCode (
      UsbBotDev->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_EC_OUTPUT_ERROR)
      );
  }

  if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
    //
    // just endpoint stall happens
    //
    UsbClearEndpointHalt (
      UsbIo,
      EndpointAddr,
      &Result
      );
  }

  *DataSize = (UINT32) TransferredSize;

  return Status;

}

STATIC
EFI_STATUS
BotStatusPhase (
  IN  USB_BOT_DEVICE        *UsbBotDev,
  OUT UINT8                 *TransferStatus,
  IN  UINT16                Timeout
  )
/*++

  Routine Description:
    Get transfer status through BOT interface

  Parameters:
    UsbBotDev       -  USB_BOT_DEVICE pointer
    TransferStatus  -  TransferStatus
    Timeout         -  Time out value in milliseconds
  Return Value:
    EFI_SUCCESS
    Others

--*/
{
  CSW                 csw;
  EFI_STATUS          Status;
  UINT32              Result;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               EndpointAddr;
  UINTN               DataSize;

  UsbIo = UsbBotDev->UsbIo;

  ZeroMem (&csw, sizeof (CSW));

  EndpointAddr  = (UsbBotDev->BulkInEndpointDescriptor)->EndpointAddress;

  DataSize      = sizeof (CSW);

  //
  // Get the status field from bulk transfer
  //
  Status = UsbIo->UsbBulkTransfer (
                    UsbIo,
                    EndpointAddr,
                    &csw,
                    &DataSize,
                    Timeout,
                    &Result
                    );
  if (EFI_ERROR (Status)) {
    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      //
      // just endpoint stall happens
      //
      UsbClearEndpointHalt (
        UsbIo,
        EndpointAddr,
        &Result
        );
    }

    ZeroMem (&csw, sizeof (CSW));

    EndpointAddr  = (UsbBotDev->BulkInEndpointDescriptor)->EndpointAddress;

    DataSize      = sizeof (CSW);
    Status = UsbIo->UsbBulkTransfer (
                      UsbIo,
                      EndpointAddr,
                      &csw,
                      &DataSize,
                      Timeout,
                      &Result
                      );
    if (EFI_ERROR (Status)) {
      if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
        UsbClearEndpointHalt (
          UsbIo,
          EndpointAddr,
          &Result
          );
      }

      return Status;
    }
  }

  if (csw.dCSWSignature == CSWSIG) {
    *TransferStatus = csw.bCSWStatus;
  } else {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
//
// Usb Atapi Protocol implementation
//
EFI_STATUS
EFIAPI
BotAtapiCommand (
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
    TimeoutInMilliSeconds - Timeout value in ms

  Returns:
    EFI_SUCCESS         - Command succeeded.
    EFI_DEVICE_ERROR    - Command failed.

--*/
{
  EFI_STATUS      Status;
  EFI_STATUS      BotDataStatus;
  UINT8           TransferStatus;
  USB_BOT_DEVICE  *UsbBotDev;
  UINT32          BufferSize;

  BotDataStatus = EFI_SUCCESS;
  TransferStatus = 0;

  //
  // Get the context
  //
  UsbBotDev = USB_BOT_DEVICE_FROM_THIS (This);

  //
  // First send ATAPI command through Bot
  //
  Status = BotCommandPhase (
            UsbBotDev,
            Command,
            CommandSize,
            BufferLength,
            Direction,
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

    BotDataStatus = BotDataPhase (
                      UsbBotDev,
                      &BufferSize,
                      DataBuffer,
                      Direction,
                      (UINT16) (TimeOutInMilliSeconds)
                      );

    break;

  case EfiUsbNoData:
    break;
  }
  
  //
  // Status Phase
  //
  Status = BotStatusPhase (
            UsbBotDev,
            &TransferStatus,
            TimeOutInMilliSeconds
            );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (TransferStatus == 0x02) {
    //
    // Phase error
    //
    BotRecoveryReset (UsbBotDev);
    return EFI_DEVICE_ERROR;
  }

  if (TransferStatus == 0x01) {
    return EFI_DEVICE_ERROR;
  }

  return BotDataStatus;
}

EFI_STATUS
EFIAPI
BotMassStorageReset (
  IN  EFI_USB_ATAPI_PROTOCOL      *This,
  IN  BOOLEAN                     ExtendedVerification
  )
/*++

  Routine Description:
    Reset Bot Devices

  Arguments:
    This                    - Protocol instance pointer.
    ExtendedVerification    - TRUE if we need to do strictly reset.

  Returns:
    EFI_SUCCESS         - Command succeeded.
    EFI_DEVICE_ERROR    - Command failed.

--*/
{
  EFI_STATUS          Status;
  USB_BOT_DEVICE      *UsbBotDev;
  EFI_USB_IO_PROTOCOL *UsbIo;

  UsbBotDev = USB_BOT_DEVICE_FROM_THIS (This);
  UsbIo     = UsbBotDev->UsbIo;

  if (ExtendedVerification) {
    //
    // If we need to do strictly reset, reset its parent hub port
    //
    Status = UsbIo->UsbPortReset (UsbIo);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = BotRecoveryReset (UsbBotDev);

  return Status;
}

VOID
BotReportStatusCode (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_STATUS_CODE_TYPE      CodeType,
  IN EFI_STATUS_CODE_VALUE     Value
  )
/*++

  Routine Description:
    Report Status Code in Usb Bot Driver

  Arguments:
    DevicePath - Use this to get Device Path
    CodeType   - Status Code Type
    CodeValue  - Status Code Value

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

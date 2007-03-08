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

GLOBAL_REMOVE_IF_UNREFERENCED UINT32     gBOTDebugLevel  = EFI_D_INFO;
GLOBAL_REMOVE_IF_UNREFERENCED UINT32     gBOTErrorLevel  = EFI_D_INFO;

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
  IN     UINTN                     *DataSize,
  IN OUT VOID                      *DataBuffer,
  IN     EFI_USB_DATA_DIRECTION    Direction,
  IN     UINT16                    Timeout
  );

STATIC
EFI_STATUS
BotStatusPhase (
  IN  USB_BOT_DEVICE          *UsbBotDev,
  OUT UINT32                  *DataResidue, 
  IN  UINT16                  Timeout
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

STATIC
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
  if ((InterfaceDescriptor.InterfaceClass    != MASS_STORAGE_CLASS) ||
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
ClearBulkInPipe (
  IN  USB_BOT_DEVICE          *UsbBotDev
  )
{
 UINT32   Result;

 return UsbClearEndpointHalt (
          UsbBotDev->UsbIo,
          UsbBotDev->BulkInEndpointDescriptor->EndpointAddress,
          &Result
          );
}


STATIC
EFI_STATUS
ClearBulkOutPipe  (
  IN  USB_BOT_DEVICE          *UsbBotDev
  )
{
 UINT32   Result;
 return UsbClearEndpointHalt (
          UsbBotDev->UsbIo,
          UsbBotDev->BulkOutEndpointDescriptor->EndpointAddress,
          &Result
          );
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
  EFI_USB_DEVICE_REQUEST  Request;
  UINT32                  Result;
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

  Status = UsbBotDev->UsbIo->UsbControlTransfer (
                               UsbBotDev->UsbIo,
                               &Request,
                               EfiUsbNoData,
                               TIMEOUT_VALUE,
                               NULL,
                               0,
                               &Result
                               );

  gBS->Stall (100 * 1000);

  ClearBulkInPipe (UsbBotDev);
  ClearBulkOutPipe (UsbBotDev);

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
  cbw.bmCBWFlags              = (UINT8) ((Direction == EfiUsbDataIn) ? 0x80 : 0);
  cbw.bCBWCBLength            = CommandSize;

  CopyMem (cbw.CBWCB, Command, CommandSize);

  DataSize = sizeof (CBW);

  Status = UsbIo->UsbBulkTransfer (
                    UsbIo,
                    UsbBotDev->BulkOutEndpointDescriptor->EndpointAddress,
                    &cbw,
                    &DataSize,
                    Timeout,
                    &Result
                    );
  return Status;
}

STATIC
EFI_STATUS
BotDataPhase (
  IN      USB_BOT_DEVICE            *UsbBotDev,
  IN      UINTN                     *DataSize,
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
  UINT8               *BufferPtr;

  UsbIo           = UsbBotDev->UsbIo;
  BufferPtr       = (UINT8 *) DataBuffer;

  //
  // retrieve the the max packet length of the given endpoint
  //
  if (Direction == EfiUsbDataIn) {
    EndpointAddr  = UsbBotDev->BulkInEndpointDescriptor->EndpointAddress;
  } else {
    EndpointAddr  = UsbBotDev->BulkOutEndpointDescriptor->EndpointAddress;
  }

    Status = UsbIo->UsbBulkTransfer (
                      UsbIo,
                      EndpointAddr,
                      BufferPtr,
                      DataSize,
                      (UINT16)(Timeout),
                      &Result
                      );

    if (EFI_ERROR (Status)) {
      if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
        if (Direction == EfiUsbDataIn) {
          DEBUG((gBOTErrorLevel, "BOT: Data IN Stall, ClearBulkInPipe\n"));
          ClearBulkInPipe (UsbBotDev);
        } else {
          DEBUG((gBOTErrorLevel, "BOT: Data OUT Stall, ClearBulkInPipe\n"));
          ClearBulkOutPipe (UsbBotDev);
        }
      }
     // BotRecoveryReset (UsbBotDev);
   }


  return Status;
}

STATIC
EFI_STATUS
BotStatusPhase (
  IN  USB_BOT_DEVICE          *UsbBotDev,
  OUT UINT32                  *DataResidue,
  IN  UINT16                  Timeout
  )
/*++

  Routine Description:
    Get transfer status through BOT interface

  Parameters:
    UsbBotDev       -  USB_BOT_DEVICE pointer
    Timeout         -  Time out value in milliseconds
  Return Value:
    EFI_SUCCESS
    Others

--*/
{
  CSW                 csw;
  EFI_STATUS          Status;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               EndpointAddr;
  UINTN               DataSize;
  UINT32              Result;
  UINT8               Index;

  UsbIo = UsbBotDev->UsbIo;
  EndpointAddr  = UsbBotDev->BulkInEndpointDescriptor->EndpointAddress;


  for (Index = 0; Index < 3; Index ++) {
    ZeroMem (&csw, sizeof (CSW));
    DataSize    = sizeof (CSW);
    Result      = 0;

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
        DEBUG((gBOTDebugLevel, "BOT: CSW Stall, ClearBulkInPipe\n"));
        ClearBulkInPipe (UsbBotDev);
        continue;
      }
    }

    if (csw.dCSWSignature == CSWSIG) {
      if (csw.bCSWStatus == 0 || csw.bCSWStatus  == 0x01) {
        if (DataResidue != NULL) {
          *DataResidue = csw.dCSWDataResidue;
        }
        if (csw.bCSWStatus  == 0x01) {
          return EFI_DEVICE_ERROR;
        }
        break;
      } else if (csw.bCSWStatus == 0x02) {
        DEBUG((gBOTErrorLevel, "BOT: Bot Phase error\n"));
        BotRecoveryReset (UsbBotDev);
      }

    }
  }

  if (Index == 3) {
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
  USB_BOT_DEVICE  *UsbBotDev;
  UINTN           BufferSize;
  UINT8           Index;
  UINT32          DataResidue;

  //
  // Get the context
  //
  UsbBotDev     = USB_BOT_DEVICE_FROM_THIS (This);
  BotDataStatus = EFI_SUCCESS;
  BufferSize    = 0;

  for (Index = 0; Index < 3; Index ++) {
    //
    // First send ATAPI command through Bot
    //
    Status = BotCommandPhase (
              UsbBotDev,
              Command,
              CommandSize,
              BufferLength,
              Direction,
              10 * 1000
              );

    if (EFI_ERROR (Status)) {
       DEBUG((gBOTErrorLevel, "BotCommandPhase Fail\n"));
       return Status;
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


      if (EFI_ERROR (BotDataStatus)) {
        DEBUG((gBOTErrorLevel, "BotDataPhase Fail\n"));
      }
      break;

    case EfiUsbNoData:
      break;
   }

    DataResidue = 0;
    //
    // Status Phase
    //
    Status = BotStatusPhase (
              UsbBotDev,
              &DataResidue,
              10 * 1000
              );

    if (EFI_ERROR (Status)) {
      DEBUG((gBOTErrorLevel, "BotStatusPhase Fail\n"));
      return Status;
    }

    if (!EFI_ERROR (BotDataStatus)) {
      break;
    }

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

STATIC
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

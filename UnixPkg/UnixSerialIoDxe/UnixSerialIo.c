/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UnixSerialIo.c

Abstract:

  Our DriverBinding member functions operate on the handles
  created by the NT Bus driver.

  Handle(1) - UnixIo - DevicePath(1)

  If a serial port is added to the system this driver creates a new handle.
  The new handle is required, since the serial device must add an UART device
  pathnode.

  Handle(2) - SerialIo - DevicePath(1)\UART

  The driver then adds a gEfiUnixSerialPortGuid as a protocol to Handle(1).
  The instance data for this protocol is the private data used to create
  Handle(2).

  Handle(1) - UnixIo - DevicePath(1) - UnixSerialPort

  If the driver is unloaded Handle(2) is removed from the system and
  gEfiUnixSerialPortGuid is removed from Handle(1).

  Note: Handle(1) is any handle created by the Win NT Bus driver that is passed
  into the DriverBinding member functions of this driver. This driver requires
  a Handle(1) to contain a UnixIo protocol, a DevicePath protocol, and
  the TypeGuid in the UnixIo must be gEfiUnixSerialPortGuid.

  If Handle(1) contains a gEfiUnixSerialPortGuid protocol then the driver is
  loaded on the device.

--*/

#include "UnixSerialIo.h"
#include <termio.h>

EFI_DRIVER_BINDING_PROTOCOL gUnixSerialIoDriverBinding = {
  UnixSerialIoDriverBindingSupported,
  UnixSerialIoDriverBindingStart,
  UnixSerialIoDriverBindingStop,
  0xa,
  NULL,
  NULL
};

UINTN
ConvertBaud2Unix (
  UINT64 BaudRate
  )
{
  switch (BaudRate) {
  case 0:
    return B0;
  case 50:
    return B50;
  case 75:
    return B75;
  case 110:
    return B110;
  case 134:
    return B134;
  case 150:
    return B150;
  case 200:
    return B200;
  case 300:
    return B300;
  case 600:
    return B600;
  case 1200:
    return B1200;
  case 1800:
    return B1800;
  case 2400:
    return B2400;
  case 4800:
    return B4800;
  case 9600:
    return B9600;
  case 19200:
    return B19200;
  case 38400:
    return B38400;
  case 57600:
    return B57600;
  case 115200:
    return B115200;
  case 230400:
    return B230400;
  case 460800:
    return B460800;
  case 500000:
    return B500000;
  case 576000:
    return B576000;
  case 921600:
    return B921600;
  case 1000000:
    return B1000000;
  case 1152000:
    return B1152000;
  case 1500000:
    return B1500000;
  case 2000000:
    return B2000000;
  case 2500000:
    return B2500000;
  case 3000000:
    return B3000000;
  case 3500000:
    return B3500000;
  case 4000000:
    return B4000000;
  case __MAX_BAUD:
  default:
    DEBUG ((EFI_D_ERROR, "Invalid Baud Rate Parameter!\r\n"));
  }
  return -1;
}

UINTN
ConvertByteSize2Unix (
  UINT8 DataBit
  )
{
  switch (DataBit) {
  case 5:
    return CS5;
  case 6:
    return CS6;
  case 7:
    return CS7;
  case 8:
    return CS8;
  default:
    DEBUG ((EFI_D_ERROR, "Invalid Data Size Parameter!\r\n"));
  }
  return -1;
}

VOID
ConvertParity2Unix (
  struct termios    *Options,
  EFI_PARITY_TYPE   Parity
  )
{
  switch (Parity) {
  case NoParity:
    Options->c_cflag &= ~PARENB;
    break;
  case EvenParity:
    Options->c_cflag |= PARENB;
    break;
  case OddParity:
    Options->c_cflag |= PARENB;
    Options->c_cflag |= PARODD;
    break;
  case MarkParity:
    Options->c_cflag = PARENB | CMSPAR | PARODD;
    break;
  case SpaceParity:
    Options->c_cflag |= PARENB | CMSPAR;
    Options->c_cflag &= ~PARODD;
    break;
  default:
    DEBUG ((EFI_D_ERROR, "Invalid Parity Parameter!\r\n"));
  }
}

VOID
ConvertStopBit2Unix (
  struct termios      *Options,
  EFI_STOP_BITS_TYPE  StopBits
  )
{
  switch (StopBits) {
  case TwoStopBits:
    Options->c_cflag |= CSTOPB;
    break;
  case OneStopBit:
  case OneFiveStopBits:
  case DefaultStopBits:
    Options->c_cflag &= ~CSTOPB;
  }
}

EFI_STATUS
EFIAPI
UnixSerialIoDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
  )
/*++

Routine Description:
  The implementation of EFI_DRIVER_BINDING_PROTOCOL.EFI_DRIVER_BINDING_SUPPORTED.

Arguments:
  
Returns:

  None

--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_UNIX_IO_PROTOCOL      *UnixIo;
  UART_DEVICE_PATH          *UartNode;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID**)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
        Handle,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        Handle
        );

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiUnixIoProtocolGuid,
                  (VOID**)&UnixIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure that the Unix Thunk Protocol is valid
  //
  if (UnixIo->UnixThunk->Signature != EFI_UNIX_THUNK_PROTOCOL_SIGNATURE) {
    Status = EFI_UNSUPPORTED;
    goto Error;
  }

  //
  // Check the GUID to see if this is a handle type the driver supports
  //
  if (!CompareGuid (UnixIo->TypeGuid, &gEfiUnixSerialPortGuid)) {
    Status = EFI_UNSUPPORTED;
    goto Error;
  }

  if (RemainingDevicePath != NULL) {
    Status    = EFI_UNSUPPORTED;
    UartNode  = (UART_DEVICE_PATH *) RemainingDevicePath;
    if (UartNode->Header.Type != MESSAGING_DEVICE_PATH ||
        UartNode->Header.SubType != MSG_UART_DP ||
        DevicePathNodeLength((EFI_DEVICE_PATH_PROTOCOL *)UartNode) != sizeof(UART_DEVICE_PATH)) {
      goto Error;
    }
    if (UartNode->BaudRate < 0 || UartNode->BaudRate > SERIAL_PORT_MAX_BAUD_RATE) {
      goto Error;
    }
    if (UartNode->Parity < NoParity || UartNode->Parity > SpaceParity) {
      goto Error;
    }
    if (UartNode->DataBits < 5 || UartNode->DataBits > 8) {
      goto Error;
    }
    if (UartNode->StopBits < OneStopBit || UartNode->StopBits > TwoStopBits) {
      goto Error;
    }
    if ((UartNode->DataBits == 5) && (UartNode->StopBits == TwoStopBits)) {
      goto Error;
    }
    if ((UartNode->DataBits >= 6) && (UartNode->DataBits <= 8) && (UartNode->StopBits == OneFiveStopBits)) {
      goto Error;
    }
    Status = EFI_SUCCESS;
  }

Error:
  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
        Handle,
        &gEfiUnixIoProtocolGuid,
        This->DriverBindingHandle,
        Handle
        );

  return Status;
}

EFI_STATUS
EFIAPI
UnixSerialIoDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  EFI_STATUS                          Status;
  EFI_UNIX_IO_PROTOCOL                *UnixIo;
  UNIX_SERIAL_IO_PRIVATE_DATA         *Private;
  UINTN                               UnixHandle;
  UART_DEVICE_PATH                    Node;
  EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenInfoBuffer;
  UINTN                               EntryCount;
  UINTN                               Index;
  EFI_SERIAL_IO_PROTOCOL              *SerialIo;
  CHAR8                               AsciiDevName[1024];

  DEBUG ((EFI_D_INFO, "SerialIo drive binding start!\r\n"));
  Private   = NULL;
  UnixHandle  = -1;

  //
  // Grab the protocols we need
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID**)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    return Status;
  }

  //
  // Grab the IO abstraction we need to get any work done
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiUnixIoProtocolGuid,
                  (VOID**)&UnixIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    gBS->CloseProtocol (
          Handle,
          &gEfiDevicePathProtocolGuid,
          This->DriverBindingHandle,
          Handle
          );
    return Status;
  }

  if (Status == EFI_ALREADY_STARTED) {

    if (RemainingDevicePath == NULL) {
      return EFI_SUCCESS;
    }

    //
    // Make sure a child handle does not already exist.  This driver can only
    // produce one child per serial port.
    //
    Status = gBS->OpenProtocolInformation (
                    Handle,
                    &gEfiUnixIoProtocolGuid,
                    &OpenInfoBuffer,
                    &EntryCount
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = EFI_ALREADY_STARTED;
    for (Index = 0; Index < EntryCount; Index++) {
      if (OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) {
        Status = gBS->OpenProtocol (
                        OpenInfoBuffer[Index].ControllerHandle,
                        &gEfiSerialIoProtocolGuid,
                        (VOID**)&SerialIo,
                        This->DriverBindingHandle,
                        Handle,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );
        if (!EFI_ERROR (Status)) {
          CopyMem (&Node, RemainingDevicePath, sizeof (UART_DEVICE_PATH));
          Status = SerialIo->SetAttributes (
                              SerialIo,
                              Node.BaudRate,
                              SerialIo->Mode->ReceiveFifoDepth,
                              SerialIo->Mode->Timeout,
                              Node.Parity,
                              Node.DataBits,
                              Node.StopBits
                              );
        }
        break;
      }
    }

    FreePool (OpenInfoBuffer);
    return Status;
  }

  //
  // Check to see if we can access the hardware device. If it's Open in Unix we
  // will not get access.
  //
  UnicodeStrToAsciiStr(UnixIo->EnvString, AsciiDevName);
  UnixHandle = UnixIo->UnixThunk->Open (AsciiDevName, O_RDWR | O_NOCTTY, 0);
  
  if (UnixHandle == -1) {
    DEBUG ((EFI_D_INFO, "Faile to open serial device, %s!\r\n", UnixIo->EnvString ));
    UnixIo->UnixThunk->Perror (AsciiDevName);
    Status = EFI_DEVICE_ERROR;
    goto Error;
  }
  DEBUG ((EFI_D_INFO, "Success to open serial device %s, Hanle = 0x%x \r\n", UnixIo->EnvString, UnixHandle));

  //
  // Construct Private data
  //
  Private = AllocatePool (sizeof (UNIX_SERIAL_IO_PRIVATE_DATA));
  if (Private == NULL) {
    goto Error;
  }

  //
  // This signature must be valid before any member function is called
  //
  Private->Signature              = UNIX_SERIAL_IO_PRIVATE_DATA_SIGNATURE;
  Private->UnixHandle             = UnixHandle;
  Private->ControllerHandle       = Handle;
  Private->Handle                 = NULL;
  Private->UnixThunk              = UnixIo->UnixThunk;
  Private->ParentDevicePath       = ParentDevicePath;
  Private->ControllerNameTable    = NULL;

  Private->SoftwareLoopbackEnable = FALSE;
  Private->HardwareLoopbackEnable = FALSE;
  Private->HardwareFlowControl    = FALSE;
  Private->Fifo.First             = 0;
  Private->Fifo.Last              = 0;
  Private->Fifo.Surplus           = SERIAL_MAX_BUFFER_SIZE;

  AddUnicodeString (
    "eng",
    gUnixSerialIoComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    UnixIo->EnvString
    );

  Private->SerialIo.Revision      = SERIAL_IO_INTERFACE_REVISION;
  Private->SerialIo.Reset         = UnixSerialIoReset;
  Private->SerialIo.SetAttributes = UnixSerialIoSetAttributes;
  Private->SerialIo.SetControl    = UnixSerialIoSetControl;
  Private->SerialIo.GetControl    = UnixSerialIoGetControl;
  Private->SerialIo.Write         = UnixSerialIoWrite;
  Private->SerialIo.Read          = UnixSerialIoRead;
  Private->SerialIo.Mode          = &Private->SerialIoMode;

  if (RemainingDevicePath != NULL) {
    //
    // Match the configuration of the RemainingDevicePath. IsHandleSupported()
    // already checked to make sure the RemainingDevicePath contains settings
    // that we can support.
    //
    CopyMem (&Private->UartDevicePath, RemainingDevicePath, sizeof (UART_DEVICE_PATH));
  } else {
    //
    // Build the device path by appending the UART node to the ParentDevicePath
    // from the UnixIo handle. The Uart setings are zero here, since
    // SetAttribute() will update them to match the default setings.
    //
    ZeroMem (&Private->UartDevicePath, sizeof (UART_DEVICE_PATH));
    Private->UartDevicePath.Header.Type     = MESSAGING_DEVICE_PATH;
    Private->UartDevicePath.Header.SubType  = MSG_UART_DP;
    SetDevicePathNodeLength ((EFI_DEVICE_PATH_PROTOCOL *) &Private->UartDevicePath, sizeof (UART_DEVICE_PATH));
  }

  //
  // Build the device path by appending the UART node to the ParentDevicePath
  // from the UnixIo handle. The Uart setings are zero here, since
  // SetAttribute() will update them to match the current setings.
  //
  Private->DevicePath = AppendDevicePathNode (
                          ParentDevicePath,
                          (EFI_DEVICE_PATH_PROTOCOL *) &Private->UartDevicePath
                          );
  if (Private->DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  //
  // Fill in Serial I/O Mode structure based on either the RemainingDevicePath or defaults.
  //
  Private->SerialIoMode.ControlMask       = SERIAL_CONTROL_MASK;
  Private->SerialIoMode.Timeout           = SERIAL_TIMEOUT_DEFAULT;
  Private->SerialIoMode.BaudRate          = Private->UartDevicePath.BaudRate;
  Private->SerialIoMode.ReceiveFifoDepth  = SERIAL_FIFO_DEFAULT;
  Private->SerialIoMode.DataBits          = Private->UartDevicePath.DataBits;
  Private->SerialIoMode.Parity            = Private->UartDevicePath.Parity;
  Private->SerialIoMode.StopBits          = Private->UartDevicePath.StopBits;

  //
  // Issue a reset to initialize the COM port
  //
  Status = Private->SerialIo.Reset (&Private->SerialIo);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Create new child handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->Handle,
                  &gEfiSerialIoProtocolGuid,
                  &Private->SerialIo,
                  &gEfiDevicePathProtocolGuid,
                  Private->DevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Open For Child Device
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiUnixIoProtocolGuid,
                  (VOID**)&UnixIo,
                  This->DriverBindingHandle,
                  Private->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  return EFI_SUCCESS;

Error:
  //
  // Use the Stop() function to free all resources allocated in Start()
  //
  if (Private != NULL) {
    if (Private->Handle != NULL) {
      This->Stop (This, Handle, 1, &Private->Handle);
    } else {
      if (UnixHandle != -1) {
        Private->UnixThunk->Close (UnixHandle);
      }

      if (Private->DevicePath != NULL) {
        FreePool (Private->DevicePath);
      }

      FreeUnicodeStringTable (Private->ControllerNameTable);

      FreePool (Private);
    }
  }

  This->Stop (This, Handle, 0, NULL);

  return Status;
}

EFI_STATUS
EFIAPI
UnixSerialIoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Handle,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This              - TODO: add argument description
  Handle            - TODO: add argument description
  NumberOfChildren  - TODO: add argument description
  ChildHandleBuffer - TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  BOOLEAN                       AllChildrenStopped;
  EFI_SERIAL_IO_PROTOCOL        *SerialIo;
  UNIX_SERIAL_IO_PRIVATE_DATA   *Private;
  EFI_UNIX_IO_PROTOCOL          *UnixIo;

  //
  // Complete all outstanding transactions to Controller.
  // Don't allow any new transaction to Controller to be started.
  //

  if (NumberOfChildren == 0) {
    //
    // Close the bus driver
    //
    Status = gBS->CloseProtocol (
                    Handle,
                    &gEfiUnixIoProtocolGuid,
                    This->DriverBindingHandle,
                    Handle
                    );
    Status = gBS->CloseProtocol (
                    Handle,
                    &gEfiDevicePathProtocolGuid,
                    This->DriverBindingHandle,
                    Handle
                    );
    return Status;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiSerialIoProtocolGuid,
                    (VOID**)&SerialIo,
                    This->DriverBindingHandle,
                    Handle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      Private = UNIX_SERIAL_IO_PRIVATE_DATA_FROM_THIS (SerialIo);

      ASSERT (Private->Handle == ChildHandleBuffer[Index]);

      Status = gBS->CloseProtocol (
                      Handle,
                      &gEfiUnixIoProtocolGuid,
                      This->DriverBindingHandle,
                      ChildHandleBuffer[Index]
                      );

      Status = gBS->UninstallMultipleProtocolInterfaces (
                      ChildHandleBuffer[Index],
                      &gEfiSerialIoProtocolGuid,
                      &Private->SerialIo,
                      &gEfiDevicePathProtocolGuid,
                      Private->DevicePath,
                      NULL
                      );

      if (EFI_ERROR (Status)) {
        gBS->OpenProtocol (
              Handle,
              &gEfiUnixIoProtocolGuid,
              (VOID **) &UnixIo,
              This->DriverBindingHandle,
              ChildHandleBuffer[Index],
              EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
              );
      } else {
        Private->UnixThunk->Close (Private->UnixHandle);

        FreePool (Private->DevicePath);

        FreeUnicodeStringTable (Private->ControllerNameTable);

        FreePool (Private);
      }
    }

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

//
// Serial IO Protocol member functions
//

EFI_STATUS
EFIAPI
UnixSerialIoReset (
  IN EFI_SERIAL_IO_PROTOCOL *This
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This  - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  UNIX_SERIAL_IO_PRIVATE_DATA *Private;
  EFI_TPL                      Tpl;
  UINTN                        UnixStatus;

  Tpl         = gBS->RaiseTPL (TPL_NOTIFY);

  Private     = UNIX_SERIAL_IO_PRIVATE_DATA_FROM_THIS (This);

  UnixStatus  = Private->UnixThunk->Tcflush (
                                    Private->UnixHandle, 
                                    TCIOFLUSH
                                    );
  switch (UnixStatus) {
  case EBADF:
    DEBUG ((EFI_D_ERROR, "Invalid handle of serial device!\r\n"));
    return EFI_DEVICE_ERROR;
  case EINVAL:
    DEBUG ((EFI_D_ERROR, "Invalid queue selector!\r\n"));
    return EFI_DEVICE_ERROR;
  case ENOTTY:
    DEBUG ((EFI_D_ERROR, "The file associated with serial's handle is not a terminal!\r\n"));
    return EFI_DEVICE_ERROR;
  default:
    DEBUG ((EFI_D_ERROR, "The serial IO device is reset successfully!\r\n"));
  }

  gBS->RestoreTPL (Tpl);

  return This->SetAttributes (
                This,
                This->Mode->BaudRate,
                This->Mode->ReceiveFifoDepth,
                This->Mode->Timeout,
                This->Mode->Parity,
                (UINT8) This->Mode->DataBits,
                This->Mode->StopBits
                );
}

EFI_STATUS
EFIAPI
UnixSerialIoSetAttributes (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  IN UINT64                 BaudRate,
  IN UINT32                 ReceiveFifoDepth,
  IN UINT32                 Timeout,
  IN EFI_PARITY_TYPE        Parity,
  IN UINT8                  DataBits,
  IN EFI_STOP_BITS_TYPE     StopBits
  )
/*++

Routine Description:

  This function is used to set the attributes.

Arguments:

  This              - A pointer to the EFI_SERIAL_IO_PROTOCOL structrue.
  BaudRate          - The Baud rate of the serial device.
  ReceiveFifoDepth  - The request depth of fifo on receive side.
  Timeout           - the request timeout for a single charact.
  Parity            - The type of parity used in serial device.
  DataBits          - Number of deata bits used in serial device.
  StopBits          - Number of stop bits used in serial device.

Returns:
  Status code

  None

--*/
{
  EFI_STATUS                    Status;
  UNIX_SERIAL_IO_PRIVATE_DATA   *Private;
  EFI_TPL                       Tpl;
  EFI_DEVICE_PATH_PROTOCOL      *NewDevicePath;

  Tpl     = gBS->RaiseTPL (TPL_NOTIFY);
  Private = UNIX_SERIAL_IO_PRIVATE_DATA_FROM_THIS (This);

  //
  //  Some of our arguments have defaults if a null value is passed in, and
  //   we must set the default values if a null argument is passed in.
  //
  if (BaudRate == 0) {
    BaudRate = SERIAL_BAUD_DEFAULT;
  }

  if (ReceiveFifoDepth == 0) {
    ReceiveFifoDepth = SERIAL_FIFO_DEFAULT;
  }

  if (Timeout == 0) {
    Timeout = SERIAL_TIMEOUT_DEFAULT;
  }

  if (Parity == DefaultParity) {
    Parity = NoParity;
  }

  if (DataBits == 0) {
    DataBits = SERIAL_DATABITS_DEFAULT;
  }

  if (StopBits == DefaultStopBits) {
    StopBits = OneStopBit;
  }

  //
  // See if the new attributes already match the current attributes
  //
  if (Private->UartDevicePath.BaudRate       == BaudRate         &&
      Private->UartDevicePath.DataBits       == DataBits         &&
      Private->UartDevicePath.Parity         == Parity           &&
      Private->UartDevicePath.StopBits       == StopBits         &&
      Private->SerialIoMode.ReceiveFifoDepth == ReceiveFifoDepth &&
      Private->SerialIoMode.Timeout          == Timeout             ) {
    gBS->RestoreTPL(Tpl);
    return EFI_SUCCESS;
  }

  //
  // Try to get options from serial device.
  // 
  if (Private->UnixThunk->Tcgetattr (Private->UnixHandle, &Private->UnixTermios) == -1) {
    Private->UnixThunk->Perror ("IoSetAttributes");
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  //
  // Setting Baud Rate
  // 
  Private->UnixThunk->Cfsetispeed (&Private->UnixTermios, ConvertBaud2Unix(BaudRate));
  Private->UnixThunk->Cfsetospeed (&Private->UnixTermios, ConvertBaud2Unix(BaudRate));
  //
  // Setting DataBits 
  // 
  Private->UnixTermios.c_cflag &= ~CSIZE;
  Private->UnixTermios.c_cflag |= ConvertByteSize2Unix (DataBits);
  //
  // Setting Parity
  // 
  ConvertParity2Unix (&Private->UnixTermios, Parity);
  //
  // Setting StopBits
  // 
  ConvertStopBit2Unix (&Private->UnixTermios, StopBits);
  //
  // Raw input
  // 
  Private->UnixTermios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  //
  // Raw output
  // 
  Private->UnixTermios.c_oflag &= ~OPOST;
  //
  // Support hardware flow control 
  // 
  Private->UnixTermios.c_cflag &= ~CRTSCTS;;
  //
  // Time out
  // 
  Private->UnixTermios.c_cc[VMIN]  = 0;
  Private->UnixTermios.c_cc[VTIME] = (Timeout/1000000) * 10;

  //
  // Set the options
  // 
  if (-1 == Private->UnixThunk->Tcsetattr (
                        Private->UnixHandle, 
                        TCSANOW, 
                        &Private->UnixTermios
                        )) {
    DEBUG ((EFI_D_INFO, "Fail to set options for serial device!\r\n"));
    return EFI_DEVICE_ERROR;
  }
  
  //
  //  Update mode
  //
  Private->SerialIoMode.BaudRate          = BaudRate;
  Private->SerialIoMode.ReceiveFifoDepth  = ReceiveFifoDepth;
  Private->SerialIoMode.Timeout           = Timeout;
  Private->SerialIoMode.Parity            = Parity;
  Private->SerialIoMode.DataBits          = DataBits;
  Private->SerialIoMode.StopBits          = StopBits;
  //
  // See if Device Path Node has actually changed
  //
  if (Private->UartDevicePath.BaudRate     == BaudRate &&
      Private->UartDevicePath.DataBits     == DataBits &&
      Private->UartDevicePath.Parity       == Parity   &&
      Private->UartDevicePath.StopBits     == StopBits    ) {
    gBS->RestoreTPL(Tpl);
    return EFI_SUCCESS;
  }

  //
  // Update the device path
  //
  Private->UartDevicePath.BaudRate  = BaudRate;
  Private->UartDevicePath.DataBits  = DataBits;
  Private->UartDevicePath.Parity    = (UINT8) Parity;
  Private->UartDevicePath.StopBits  = (UINT8) StopBits;

  NewDevicePath = AppendDevicePathNode (
                    Private->ParentDevicePath,
                    (EFI_DEVICE_PATH_PROTOCOL *) &Private->UartDevicePath
                    );
  if (NewDevicePath == NULL) {
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  if (Private->Handle != NULL) {
    Status = gBS->ReinstallProtocolInterface (
                    Private->Handle,
                    &gEfiDevicePathProtocolGuid,
                    Private->DevicePath,
                    NewDevicePath
                    );
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (Tpl);
      return Status;
    }
  }

  if (Private->DevicePath != NULL) {
    FreePool (Private->DevicePath);
  }

  Private->DevicePath = NewDevicePath;

  gBS->RestoreTPL (Tpl);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UnixSerialIoSetControl (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  IN UINT32                 Control
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Control - TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - TODO: Add description for return value
  EFI_DEVICE_ERROR - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  UNIX_SERIAL_IO_PRIVATE_DATA *Private;
  UINTN                       Result;
  UINTN                       Status;
  struct termios              Options;
  EFI_TPL                     Tpl;

  Tpl     = gBS->RaiseTPL (TPL_NOTIFY);

  Private = UNIX_SERIAL_IO_PRIVATE_DATA_FROM_THIS (This);

  Result  = Private->UnixThunk->IoCtl (Private->UnixHandle, TIOCMGET, &Status);

  if (Result == -1) {
    Private->UnixThunk->Perror ("SerialSetControl");
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  Private->HardwareFlowControl    = FALSE;
  Private->SoftwareLoopbackEnable = FALSE;
  Private->HardwareLoopbackEnable = FALSE;

  if (Control & EFI_SERIAL_REQUEST_TO_SEND) {
    Options.c_cflag |= TIOCM_RTS;
  }

  if (Control & EFI_SERIAL_DATA_TERMINAL_READY) {
    Options.c_cflag |= TIOCM_DTR;
  }

  if (Control & EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE) {
    Private->HardwareFlowControl = TRUE;
  }

  if (Control & EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE) {
    Private->SoftwareLoopbackEnable = TRUE;
  }

  if (Control & EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE) {
    Private->HardwareLoopbackEnable = TRUE;
  }

  Result  = Private->UnixThunk->IoCtl (Private->UnixHandle, TIOCMSET, &Status);

  if (Result == -1) {
    Private->UnixThunk->Perror ("SerialSetControl");
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  gBS->RestoreTPL (Tpl);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UnixSerialIoGetControl (
  IN  EFI_SERIAL_IO_PROTOCOL  *This,
  OUT UINT32                  *Control
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Control - TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - TODO: Add description for return value
  EFI_DEVICE_ERROR - TODO: Add description for return value
  EFI_DEVICE_ERROR - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  UNIX_SERIAL_IO_PRIVATE_DATA *Private;
  UINTN                       Result;
  UINTN                       Status;
  UINT32                      Bits;
  EFI_TPL                     Tpl;
  UINTN                       Bytes;

  Tpl     = gBS->RaiseTPL (TPL_NOTIFY);

  Private = UNIX_SERIAL_IO_PRIVATE_DATA_FROM_THIS (This);
  Result  = Private->UnixThunk->IoCtl (Private->UnixHandle, TIOCMGET, &Status);
  if (Result == -1) {
    Private->UnixThunk->Perror ("SerialGetControl");
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  if ((Status & TIOCM_CTS) == TIOCM_CTS) {
    Bits |= EFI_SERIAL_CLEAR_TO_SEND;
  }

  if ((Status & TIOCM_DSR) == TIOCM_DSR) {
    Bits |= EFI_SERIAL_DATA_SET_READY;
  }

  if ((Status & TIOCM_DTR) == TIOCM_DTR) {
    Bits |= EFI_SERIAL_DATA_TERMINAL_READY;
  }

  if ((Status & TIOCM_RTS) == TIOCM_RTS) {
    Bits |= EFI_SERIAL_REQUEST_TO_SEND;
  }

  if ((Status & TIOCM_RNG) == TIOCM_RNG) {
    Bits |= EFI_SERIAL_RING_INDICATE;
  }

  if ((Status & TIOCM_CAR) == TIOCM_CAR) {
    Bits |= EFI_SERIAL_CARRIER_DETECT;
  }

  if (Private->HardwareFlowControl) {
    Bits |= EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE;
  }

  if (Private->SoftwareLoopbackEnable) {
    Bits |= EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE;
  }

  if (Private->HardwareLoopbackEnable) {
    Bits |= EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE;
  }

  Result = Private->UnixThunk->IoCtl (Private->UnixHandle, FIONREAD, &Bytes);
  if (Result == -1) {
    Private->UnixThunk->Perror ("SerialGetControl");
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  if (Bytes == 0) {
    Bits |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
  }

  *Control = Bits;

  gBS->RestoreTPL (Tpl);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UnixSerialIoWrite (
  IN EFI_SERIAL_IO_PROTOCOL   *This,
  IN OUT UINTN                *BufferSize,
  IN VOID                     *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  UNIX_SERIAL_IO_PRIVATE_DATA   *Private;
  UINT8                         *ByteBuffer;
  UINT32                        TotalBytesWritten;
  UINT32                        BytesToGo;
  UINT32                        BytesWritten;
  UINT32                        Index;
  UINT32                        Control;
  EFI_TPL                       Tpl;

  Tpl               = gBS->RaiseTPL (TPL_NOTIFY);

  Private           = UNIX_SERIAL_IO_PRIVATE_DATA_FROM_THIS (This); 

  ByteBuffer        = (UINT8 *) Buffer;
  TotalBytesWritten = 0;

  if (Private->SoftwareLoopbackEnable || Private->HardwareLoopbackEnable) {
    for (Index = 0; Index < *BufferSize; Index++) {
      if (IsaSerialFifoAdd (&Private->Fifo, ByteBuffer[Index]) == EFI_SUCCESS) {
        TotalBytesWritten++;
      } else {
        break;
      }
    }
  } else {
    BytesToGo = (*BufferSize);

    do {
      if (Private->HardwareFlowControl) {
        //
        // Send RTS
        //
        UnixSerialIoGetControl (&Private->SerialIo, &Control);
        Control |= EFI_SERIAL_REQUEST_TO_SEND;
        UnixSerialIoSetControl (&Private->SerialIo, Control);
      }

      //
      //  Do the write
      //
      BytesWritten = Private->UnixThunk->Write ( 
                                           Private->UnixHandle,
                                           &ByteBuffer[TotalBytesWritten],
                                           BytesToGo
                                           );

      if (Private->HardwareFlowControl) {
        //
        // Assert RTS
        //
        UnixSerialIoGetControl (&Private->SerialIo, &Control);
        Control &= ~ (UINT32) EFI_SERIAL_REQUEST_TO_SEND;
        UnixSerialIoSetControl (&Private->SerialIo, Control);
      }

      TotalBytesWritten += BytesWritten;
      BytesToGo -= BytesWritten;
    } while (BytesToGo > 0);
  }

  *BufferSize = TotalBytesWritten;

  gBS->RestoreTPL (Tpl);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UnixSerialIoRead (
  IN  EFI_SERIAL_IO_PROTOCOL  *This,
  IN  OUT UINTN               *BufferSize,
  OUT VOID                    *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - TODO: Add description for return value

--*/
{
  UNIX_SERIAL_IO_PRIVATE_DATA   *Private;
  UINT32                        BytesRead;
  EFI_STATUS                    Status;
  UINT32                        Index;
  UINT8                         Data;
  UINT32                        Control;
  EFI_TPL                       Tpl;

  Tpl     = gBS->RaiseTPL (TPL_NOTIFY);

  Private = UNIX_SERIAL_IO_PRIVATE_DATA_FROM_THIS (This);

  //
  //  Do the read
  //
  if (Private->SoftwareLoopbackEnable || Private->HardwareLoopbackEnable) {
    for (Index = 0, BytesRead = 0; Index < *BufferSize; Index++) {
      if (IsaSerialFifoRemove (&Private->Fifo, &Data) == EFI_SUCCESS) {
        ((UINT8 *) Buffer)[Index] = Data;
        BytesRead++;
      } else {
        break;
      }
    }
  } else {
    if (Private->HardwareFlowControl) {
      UnixSerialIoGetControl (&Private->SerialIo, &Control);
      Control |= EFI_SERIAL_DATA_TERMINAL_READY;
      UnixSerialIoSetControl (&Private->SerialIo, Control);
    }

    BytesRead = Private->UnixThunk->Read (Private->UnixHandle, Buffer, *BufferSize);
    if (Private->HardwareFlowControl) {
      UnixSerialIoGetControl (&Private->SerialIo, &Control);
      Control &= ~ (UINT32) EFI_SERIAL_DATA_TERMINAL_READY;
      UnixSerialIoSetControl (&Private->SerialIo, Control);
    }

  }

  if (BytesRead != *BufferSize) {
    Status = EFI_TIMEOUT;
  } else {
    Status = EFI_SUCCESS;
  }

  *BufferSize = (UINTN) BytesRead;

  gBS->RestoreTPL (Tpl);

  return Status;
}

BOOLEAN
IsaSerialFifoFull (
  IN SERIAL_DEV_FIFO *Fifo
  )
/*++

  Routine Description:
  Detect whether specific FIFO is full or not

  Arguments:
  Fifo  SERIAL_DEV_FIFO *: A pointer to the Data Structure SERIAL_DEV_FIFO

  Returns:
  TRUE:  the FIFO is full
  FALSE: the FIFO is not full

--*/
{
  if (Fifo->Surplus == 0) {
    return TRUE;
  }

  return FALSE;
}

BOOLEAN
IsaSerialFifoEmpty (
  IN SERIAL_DEV_FIFO *Fifo
  )
/*++

  Routine Description:
  Detect whether specific FIFO is empty or not

  Arguments:
    Fifo  SERIAL_DEV_FIFO *: A pointer to the Data Structure SERIAL_DEV_FIFO

  Returns:
    TRUE:  the FIFO is empty
    FALSE: the FIFO is not empty

--*/
{
  if (Fifo->Surplus == SERIAL_MAX_BUFFER_SIZE) {
    return TRUE;
  }

  return FALSE;
}

EFI_STATUS
IsaSerialFifoAdd (
  IN SERIAL_DEV_FIFO *Fifo,
  IN UINT8           Data
  )
/*++

  Routine Description:
  Add data to specific FIFO

  Arguments:
    Fifo  SERIAL_DEV_FIFO *: A pointer to the Data Structure SERIAL_DEV_FIFO
    Data  UINT8: the data added to FIFO

  Returns:
    EFI_SUCCESS:  Add data to specific FIFO successfully
    EFI_OUT_RESOURCE: Failed to add data because FIFO is already full

--*/
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
{
  //
  // if FIFO full can not add data
  //
  if (IsaSerialFifoFull (Fifo)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // FIFO is not full can add data
  //
  Fifo->Data[Fifo->Last] = Data;
  Fifo->Surplus--;
  Fifo->Last++;
  if (Fifo->Last >= SERIAL_MAX_BUFFER_SIZE) {
    Fifo->Last = 0;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
IsaSerialFifoRemove (
  IN  SERIAL_DEV_FIFO *Fifo,
  OUT UINT8           *Data
  )
/*++

  Routine Description:
  Remove data from specific FIFO

  Arguments:
    Fifo  SERIAL_DEV_FIFO *: A pointer to the Data Structure SERIAL_DEV_FIFO
    Data  UINT8*: the data removed from FIFO

  Returns:
    EFI_SUCCESS:  Remove data from specific FIFO successfully
    EFI_OUT_RESOURCE: Failed to remove data because FIFO is empty

--*/
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
{
  //
  // if FIFO is empty, no data can remove
  //
  if (IsaSerialFifoEmpty (Fifo)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // FIFO is not empty, can remove data
  //
  *Data = Fifo->Data[Fifo->First];
  Fifo->Surplus++;
  Fifo->First++;
  if (Fifo->First >= SERIAL_MAX_BUFFER_SIZE) {
    Fifo->First = 0;
  }

  return EFI_SUCCESS;
}

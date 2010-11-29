/**@file

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  WinNtSerialIo.c

Abstract:

  Our DriverBinding member functions operate on the handles
  created by the NT Bus driver.

  Handle(1) - WinNtIo - DevicePath(1)

  If a serial port is added to the system this driver creates a new handle.
  The new handle is required, since the serial device must add an UART device
  pathnode.

  Handle(2) - SerialIo - DevicePath(1)\UART

  The driver then adds a gEfiWinNtSerialPortGuid as a protocol to Handle(1).
  The instance data for this protocol is the private data used to create
  Handle(2).

  Handle(1) - WinNtIo - DevicePath(1) - WinNtSerialPort

  If the driver is unloaded Handle(2) is removed from the system and
  gEfiWinNtSerialPortGuid is removed from Handle(1).

  Note: Handle(1) is any handle created by the Win NT Bus driver that is passed
  into the DriverBinding member functions of this driver. This driver requires
  a Handle(1) to contain a WinNtIo protocol, a DevicePath protocol, and
  the TypeGuid in the WinNtIo must be gEfiWinNtSerialPortGuid.

  If Handle(1) contains a gEfiWinNtSerialPortGuid protocol then the driver is
  loaded on the device.

**/

#include "WinNtSerialIo.h"

EFI_DRIVER_BINDING_PROTOCOL gWinNtSerialIoDriverBinding = {
  WinNtSerialIoDriverBindingSupported,
  WinNtSerialIoDriverBindingStart,
  WinNtSerialIoDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// List of supported baud rate
//
UINT64 mBaudRateCurrentSupport[] = {50, 75, 110, 134, 150, 300, 600, 1200, 1800, 2000, 2400, 3600, 4800, 7200, 9600, 19200, 38400, 57600, 115200, SERIAL_PORT_MAX_BAUD_RATE + 1};

/**
  Check the device path node whether it's the Flow Control node or not.

  @param[in] FlowControl    The device path node to be checked.
  
  @retval TRUE              It's the Flow Control node.
  @retval FALSE             It's not.

**/
BOOLEAN
IsUartFlowControlNode (
  IN UART_FLOW_CONTROL_DEVICE_PATH *FlowControl
  )
{
  return (BOOLEAN) (
           (DevicePathType (FlowControl) == MESSAGING_DEVICE_PATH) &&
           (DevicePathSubType (FlowControl) == MSG_VENDOR_DP) &&
           (CompareGuid (&FlowControl->Guid, &gEfiUartDevicePathGuid))
           );
}

/**
  Check the device path node whether it contains Flow Control node or not.

  @param[in] DevicePath     The device path to be checked.
  
  @retval TRUE              It contains the Flow Control node.
  @retval FALSE             It doesn't.

**/
BOOLEAN
ContainsFlowControl (
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  )
{
  while (!IsDevicePathEnd (DevicePath)) {
    if (IsUartFlowControlNode ((UART_FLOW_CONTROL_DEVICE_PATH *) DevicePath)) {
      return TRUE;
    }
    DevicePath = NextDevicePathNode (DevicePath);
  }

  return FALSE;
}

/**
  The user Entry Point for module WinNtSerialIo. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeWinNtSerialIo(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gWinNtSerialIoDriverBinding,
             ImageHandle,
             &gWinNtSerialIoComponentName,
             &gWinNtSerialIoComponentName2
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}

EFI_STATUS
EFIAPI
WinNtSerialIoDriverBindingSupported (
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
// TODO:    This - add argument and description to function comment
// TODO:    Handle - add argument and description to function comment
// TODO:    RemainingDevicePath - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath;
  EFI_WIN_NT_IO_PROTOCOL              *WinNtIo;
  UART_DEVICE_PATH                    *UartNode;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
  UART_FLOW_CONTROL_DEVICE_PATH       *FlowControlNode;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenInfoBuffer;
  UINTN                               EntryCount;
  UINTN                               Index;
  BOOLEAN                             RemainingDevicePathContainsFlowControl; 

  //
  // Check RemainingDevicePath validation
  //
  if (RemainingDevicePath != NULL) {
    //
    // Check if RemainingDevicePath is the End of Device Path Node, 
    // if yes, go on checking other conditions
    //
    if (!IsDevicePathEnd (RemainingDevicePath)) {
      //
      // If RemainingDevicePath isn't the End of Device Path Node,
      // check its validation
      //
      Status = EFI_UNSUPPORTED;

      UartNode  = (UART_DEVICE_PATH *) RemainingDevicePath;
      if (UartNode->Header.Type != MESSAGING_DEVICE_PATH ||
          UartNode->Header.SubType != MSG_UART_DP ||
          DevicePathNodeLength((EFI_DEVICE_PATH_PROTOCOL *)UartNode) != sizeof(UART_DEVICE_PATH)) {
        goto Error;
      }
      if ( UartNode->BaudRate > SERIAL_PORT_MAX_BAUD_RATE) {
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

      FlowControlNode = (UART_FLOW_CONTROL_DEVICE_PATH *) NextDevicePathNode (UartNode);
      if (IsUartFlowControlNode (FlowControlNode)) {
        //
        // If the second node is Flow Control Node,
        //   return error when it request other than hardware flow control.
        //
        if ((FlowControlNode->FlowControlMap & ~UART_FLOW_CONTROL_HARDWARE) != 0) {
          goto Error;
        }
      }
    }
  }

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiWinNtIoProtocolGuid,
                  (VOID **) &WinNtIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    if (RemainingDevicePath == NULL || IsDevicePathEnd (RemainingDevicePath)) {
      //
      // If RemainingDevicePath is NULL or is the End of Device Path Node
      //
      return EFI_SUCCESS;
    }
    //
    // When the driver has produced device path with flow control node but RemainingDevicePath only contains UART node,
    //   return unsupported, and vice versa.
    //
    Status = gBS->OpenProtocolInformation (
                    Handle,
                    &gEfiWinNtIoProtocolGuid,
                    &OpenInfoBuffer,
                    &EntryCount
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // See if RemainingDevicePath has a Flow Control device path node
    //
    RemainingDevicePathContainsFlowControl = ContainsFlowControl (RemainingDevicePath);
    
    for (Index = 0; Index < EntryCount; Index++) {
      if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
        Status = gBS->OpenProtocol (
                        OpenInfoBuffer[Index].ControllerHandle,
                        &gEfiDevicePathProtocolGuid,
                        (VOID **) &DevicePath,
                        This->DriverBindingHandle,
                        Handle,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );
        if (!EFI_ERROR (Status)) {
          if (RemainingDevicePathContainsFlowControl ^ ContainsFlowControl (DevicePath)) {
            Status = EFI_UNSUPPORTED;
          }
        }
        break;
      }
    }
    FreePool (OpenInfoBuffer);
    return Status;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
        Handle,
        &gEfiWinNtIoProtocolGuid,
        This->DriverBindingHandle,
        Handle
        );

  //
  // Open the EFI Device Path protocol needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
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
  // Close protocol, don't use device path protocol in the Support() function
  //
  gBS->CloseProtocol (
        Handle,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        Handle
        );

  //
  // Make sure that the WinNt Thunk Protocol is valid
  //
  if (WinNtIo->WinNtThunk->Signature != EFI_WIN_NT_THUNK_PROTOCOL_SIGNATURE) {
    Status = EFI_UNSUPPORTED;
    goto Error;
  }

  //
  // Check the GUID to see if this is a handle type the driver supports
  //
  if (!CompareGuid (WinNtIo->TypeGuid, &gEfiWinNtSerialPortGuid)) {
    Status = EFI_UNSUPPORTED;
    goto Error;
  }

  return EFI_SUCCESS;

Error:
  return Status;
}

EFI_STATUS
EFIAPI
WinNtSerialIoDriverBindingStart (
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
// TODO:    This - add argument and description to function comment
// TODO:    Handle - add argument and description to function comment
// TODO:    RemainingDevicePath - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS                          Status;
  EFI_WIN_NT_IO_PROTOCOL              *WinNtIo;
  WIN_NT_SERIAL_IO_PRIVATE_DATA       *Private;
  HANDLE                              NtHandle;
  UART_DEVICE_PATH                    UartNode;
  EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenInfoBuffer;
  UINTN                               EntryCount;
  UINTN                               Index;
  EFI_SERIAL_IO_PROTOCOL              *SerialIo;
  UART_DEVICE_PATH                    *Uart;
  UINT32                              FlowControlMap;
  UART_FLOW_CONTROL_DEVICE_PATH       *FlowControl;
  EFI_DEVICE_PATH_PROTOCOL            *TempDevicePath;
  UINT32                              Control;

  Private   = NULL;
  NtHandle  = INVALID_HANDLE_VALUE;

  //
  // Get the Parent Device Path
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
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
                  &gEfiWinNtIoProtocolGuid,
                  (VOID **) &WinNtIo,
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

    if (RemainingDevicePath == NULL || IsDevicePathEnd (RemainingDevicePath)) {
      //
      // If RemainingDevicePath is NULL or is the End of Device Path Node
      //
      return EFI_SUCCESS;
    }

    //
    // Make sure a child handle does not already exist.  This driver can only
    // produce one child per serial port.
    //
    Status = gBS->OpenProtocolInformation (
                    Handle,
                    &gEfiWinNtIoProtocolGuid,
                    &OpenInfoBuffer,
                    &EntryCount
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = EFI_ALREADY_STARTED;
    for (Index = 0; Index < EntryCount; Index++) {
      if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
        Status = gBS->OpenProtocol (
                        OpenInfoBuffer[Index].ControllerHandle,
                        &gEfiSerialIoProtocolGuid,
                        (VOID **) &SerialIo,
                        This->DriverBindingHandle,
                        Handle,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );
        if (!EFI_ERROR (Status)) {
          Uart   = (UART_DEVICE_PATH *) RemainingDevicePath;
          Status = SerialIo->SetAttributes (
                               SerialIo,
                               Uart->BaudRate,
                               SerialIo->Mode->ReceiveFifoDepth,
                               SerialIo->Mode->Timeout,
                               (EFI_PARITY_TYPE) Uart->Parity,
                               Uart->DataBits,
                               (EFI_STOP_BITS_TYPE) Uart->StopBits
                               );
          FlowControl = (UART_FLOW_CONTROL_DEVICE_PATH *) NextDevicePathNode (Uart);
          if (!EFI_ERROR (Status) && IsUartFlowControlNode (FlowControl)) {
            Status = SerialIo->GetControl (SerialIo, &Control);
            if (!EFI_ERROR (Status)) {
              if (FlowControl->FlowControlMap == UART_FLOW_CONTROL_HARDWARE) {
                Control |= EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE;
              } else {
                Control &= ~EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE;
              }
              //
              // Clear the bits that are not allowed to pass to SetControl
              //
              Control &= (EFI_SERIAL_REQUEST_TO_SEND | EFI_SERIAL_DATA_TERMINAL_READY |
                          EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE | EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE | 
                          EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE);
              Status = SerialIo->SetControl (SerialIo, Control);
            }
          }
        }
        break;
      }
    }

    FreePool (OpenInfoBuffer);
    return Status;
  }

  FlowControl    = NULL;
  FlowControlMap = 0;
  if (RemainingDevicePath == NULL) {
    //
    // Build the device path by appending the UART node to the ParentDevicePath
    // from the WinNtIo handle. The Uart setings are zero here, since
    // SetAttribute() will update them to match the default setings.
    //
    ZeroMem (&UartNode, sizeof (UART_DEVICE_PATH));
    UartNode.Header.Type     = MESSAGING_DEVICE_PATH;
    UartNode.Header.SubType  = MSG_UART_DP;
    SetDevicePathNodeLength ((EFI_DEVICE_PATH_PROTOCOL *) &UartNode, sizeof (UART_DEVICE_PATH));

  } else if (!IsDevicePathEnd (RemainingDevicePath)) {
    //
    // If RemainingDevicePath isn't the End of Device Path Node, 
    // only scan the specified device by RemainingDevicePath
    //
    //
    // Match the configuration of the RemainingDevicePath. IsHandleSupported()
    // already checked to make sure the RemainingDevicePath contains settings
    // that we can support.
    //
    CopyMem (&UartNode, RemainingDevicePath, sizeof (UART_DEVICE_PATH));
    FlowControl = (UART_FLOW_CONTROL_DEVICE_PATH *) NextDevicePathNode (RemainingDevicePath);
    if (IsUartFlowControlNode (FlowControl)) {
      FlowControlMap = FlowControl->FlowControlMap;
    } else {
      FlowControl    = NULL;
    }

  } else {
    //
    // If RemainingDevicePath is the End of Device Path Node,
    // skip enumerate any device and return EFI_SUCESSS
    // 
    return EFI_SUCCESS;
  }

  //
  // Check to see if we can access the hardware device. If it's Open in NT we
  // will not get access.
  //
  NtHandle = WinNtIo->WinNtThunk->CreateFile (
                                    WinNtIo->EnvString,
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL
                                    );
  if (NtHandle == INVALID_HANDLE_VALUE) {
    Status = EFI_DEVICE_ERROR;
    goto Error;
  }

  //
  // Construct Private data
  //
  Private = AllocatePool (sizeof (WIN_NT_SERIAL_IO_PRIVATE_DATA));
  if (Private == NULL) {
    goto Error;
  }

  //
  // This signature must be valid before any member function is called
  //
  Private->Signature              = WIN_NT_SERIAL_IO_PRIVATE_DATA_SIGNATURE;
  Private->NtHandle               = NtHandle;
  Private->ControllerHandle       = Handle;
  Private->Handle                 = NULL;
  Private->WinNtThunk             = WinNtIo->WinNtThunk;
  Private->ParentDevicePath       = ParentDevicePath;
  Private->ControllerNameTable    = NULL;

  Private->SoftwareLoopbackEnable = FALSE;
  Private->HardwareLoopbackEnable = FALSE;
  Private->HardwareFlowControl    = (BOOLEAN) (FlowControlMap == UART_FLOW_CONTROL_HARDWARE);
  Private->Fifo.First             = 0;
  Private->Fifo.Last              = 0;
  Private->Fifo.Surplus           = SERIAL_MAX_BUFFER_SIZE;

  CopyMem (&Private->UartDevicePath, &UartNode, sizeof (UART_DEVICE_PATH));

  AddUnicodeString2 (
    "eng",
    gWinNtSerialIoComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    WinNtIo->EnvString,
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gWinNtSerialIoComponentName2.SupportedLanguages,
    &Private->ControllerNameTable,
    WinNtIo->EnvString,
    FALSE
    );


  Private->SerialIo.Revision      = SERIAL_IO_INTERFACE_REVISION;
  Private->SerialIo.Reset         = WinNtSerialIoReset;
  Private->SerialIo.SetAttributes = WinNtSerialIoSetAttributes;
  Private->SerialIo.SetControl    = WinNtSerialIoSetControl;
  Private->SerialIo.GetControl    = WinNtSerialIoGetControl;
  Private->SerialIo.Write         = WinNtSerialIoWrite;
  Private->SerialIo.Read          = WinNtSerialIoRead;
  Private->SerialIo.Mode          = &Private->SerialIoMode;

  //
  // Build the device path by appending the UART node to the ParentDevicePath
  // from the WinNtIo handle. The Uart setings are zero here, since
  // SetAttribute() will update them to match the current setings.
  //
  Private->DevicePath = AppendDevicePathNode (
                          ParentDevicePath,
                          (EFI_DEVICE_PATH_PROTOCOL *) &Private->UartDevicePath
                          );
  //
  // Only produce the FlowControl node when remaining device path has it
  //
  if (FlowControl != NULL) {
    TempDevicePath = Private->DevicePath;
    if (TempDevicePath != NULL) {
      Private->DevicePath = AppendDevicePathNode (
                              TempDevicePath,
                              (EFI_DEVICE_PATH_PROTOCOL *) FlowControl
                              );
      FreePool (TempDevicePath);
    }
  }
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
                  &gEfiWinNtIoProtocolGuid,
                  (VOID **) &WinNtIo,
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
      if (NtHandle != INVALID_HANDLE_VALUE) {
        Private->WinNtThunk->CloseHandle (NtHandle);
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
WinNtSerialIoDriverBindingStop (
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
  WIN_NT_SERIAL_IO_PRIVATE_DATA *Private;
  EFI_WIN_NT_IO_PROTOCOL        *WinNtIo;

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
                    &gEfiWinNtIoProtocolGuid,
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
                    (VOID **) &SerialIo,
                    This->DriverBindingHandle,
                    Handle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      Private = WIN_NT_SERIAL_IO_PRIVATE_DATA_FROM_THIS (SerialIo);

      ASSERT (Private->Handle == ChildHandleBuffer[Index]);

      Status = gBS->CloseProtocol (
                      Handle,
                      &gEfiWinNtIoProtocolGuid,
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
              &gEfiWinNtIoProtocolGuid,
              (VOID **) &WinNtIo,
              This->DriverBindingHandle,
              ChildHandleBuffer[Index],
              EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
              );
      } else {
        Private->WinNtThunk->CloseHandle (Private->NtHandle);

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
WinNtSerialIoReset (
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
  WIN_NT_SERIAL_IO_PRIVATE_DATA *Private;
  EFI_TPL                       Tpl;

  Tpl     = gBS->RaiseTPL (TPL_NOTIFY);

  Private = WIN_NT_SERIAL_IO_PRIVATE_DATA_FROM_THIS (This);

  Private->WinNtThunk->PurgeComm (
                        Private->NtHandle,
                        PURGE_TXCLEAR | PURGE_RXCLEAR
                        );

  gBS->RestoreTPL (Tpl);

  return This->SetAttributes (
                This,
                This->Mode->BaudRate,
                This->Mode->ReceiveFifoDepth,
                This->Mode->Timeout,
                (EFI_PARITY_TYPE)This->Mode->Parity,
                (UINT8) This->Mode->DataBits,
                (EFI_STOP_BITS_TYPE)This->Mode->StopBits
                );
}

EFI_STATUS
EFIAPI
WinNtSerialIoSetAttributes (
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
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS                    Status;
  UINTN                         Index;  
  WIN_NT_SERIAL_IO_PRIVATE_DATA *Private;
  COMMTIMEOUTS                  PortTimeOuts;
  DWORD                         ConvertedTime;
  BOOL                          Result;
  UART_DEVICE_PATH              *Uart;
  EFI_TPL                       Tpl;

  Private = WIN_NT_SERIAL_IO_PRIVATE_DATA_FROM_THIS (This);

  //
  //  Some of our arguments have defaults if a null value is passed in, and
  //   we must set the default values if a null argument is passed in.
  //
  if (BaudRate == 0) {
    BaudRate = PcdGet64 (PcdUartDefaultBaudRate);
  }

  if (ReceiveFifoDepth == 0) {
    ReceiveFifoDepth = SERIAL_FIFO_DEFAULT;
  }

  if (Timeout == 0) {
    Timeout = SERIAL_TIMEOUT_DEFAULT;
  }

  if (Parity == DefaultParity) {
    Parity = (EFI_PARITY_TYPE) (PcdGet8 (PcdUartDefaultParity));
  }

  if (DataBits == 0) {
    DataBits = PcdGet8 (PcdUartDefaultDataBits);
  }

  if (StopBits == DefaultStopBits) {
    StopBits = (EFI_STOP_BITS_TYPE) PcdGet8 (PcdUartDefaultStopBits);
  }

  //
  // Make sure all parameters are valid
  //
  if ((BaudRate > SERIAL_PORT_MAX_BAUD_RATE) || (BaudRate < SERIAL_PORT_MIN_BAUD_RATE)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  //The lower baud rate supported by the serial device will be selected without exceeding the unsupported BaudRate parameter
  // 
  
  for (Index = 1; Index < (sizeof (mBaudRateCurrentSupport) / sizeof (mBaudRateCurrentSupport[0])); Index++) {
    if (BaudRate < mBaudRateCurrentSupport[Index]) {
      BaudRate = mBaudRateCurrentSupport[Index-1];
      break;
      }
  }

  if ((ReceiveFifoDepth < 1) || (ReceiveFifoDepth > SERIAL_PORT_MAX_RECEIVE_FIFO_DEPTH)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Timeout < SERIAL_PORT_MIN_TIMEOUT) || (Timeout > SERIAL_PORT_MAX_TIMEOUT)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Parity < NoParity) || (Parity > SpaceParity)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((StopBits < OneStopBit) || (StopBits > TwoStopBits)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Now we only support DataBits=7,8.
  //
  if ((DataBits < 7) || (DataBits > 8)) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Now we only support DataBits=7,8.
  // for DataBits = 6,7,8, StopBits can not set OneFiveStopBits.
  //
  if (StopBits == OneFiveStopBits) {
    return EFI_INVALID_PARAMETER;
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
    return EFI_SUCCESS;
  }

  Tpl     = gBS->RaiseTPL (TPL_NOTIFY);

  //
  //  Get current values from NT
  //
  ZeroMem (&Private->NtDCB, sizeof (DCB));
  Private->NtDCB.DCBlength = sizeof (DCB);

  if (!Private->WinNtThunk->GetCommState (Private->NtHandle, &Private->NtDCB)) {
    Private->NtError = Private->WinNtThunk->GetLastError ();
    DEBUG ((EFI_D_ERROR, "SerialSetAttributes: GetCommState %d\n", Private->NtError));
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  //
  // Map EFI com setting to NT
  //
  Private->NtDCB.BaudRate         = ConvertBaud2Nt (BaudRate);
  Private->NtDCB.ByteSize         = ConvertData2Nt (DataBits);
  Private->NtDCB.Parity           = ConvertParity2Nt (Parity);
  Private->NtDCB.StopBits         = ConvertStop2Nt (StopBits);

  Private->NtDCB.fBinary          = TRUE;
  Private->NtDCB.fParity          = Private->NtDCB.Parity == NOPARITY ? FALSE : TRUE;
  Private->NtDCB.fOutxCtsFlow     = FALSE;
  Private->NtDCB.fOutxDsrFlow     = FALSE;
  Private->NtDCB.fDtrControl      = DTR_CONTROL_ENABLE;
  Private->NtDCB.fDsrSensitivity  = FALSE;
  Private->NtDCB.fOutX            = FALSE;
  Private->NtDCB.fInX             = FALSE;
  Private->NtDCB.fRtsControl      = RTS_CONTROL_ENABLE;
  Private->NtDCB.fNull            = FALSE;

  //
  //  Set new values
  //
  Result = Private->WinNtThunk->SetCommState (Private->NtHandle, &Private->NtDCB);
  if (!Result) {
    Private->NtError = Private->WinNtThunk->GetLastError ();
    DEBUG ((EFI_D_ERROR, "SerialSetAttributes: SetCommState %d\n", Private->NtError));
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  //
  //  Set com port read/write timeout values
  //
  ConvertedTime = ConvertTime2Nt (Timeout);
  PortTimeOuts.ReadIntervalTimeout = MAXDWORD;
  PortTimeOuts.ReadTotalTimeoutMultiplier = 0;
  PortTimeOuts.ReadTotalTimeoutConstant = ConvertedTime;
  PortTimeOuts.WriteTotalTimeoutMultiplier = ConvertedTime == 0 ? 1 : ConvertedTime;
  PortTimeOuts.WriteTotalTimeoutConstant = 0;

  if (!Private->WinNtThunk->SetCommTimeouts (Private->NtHandle, &PortTimeOuts)) {
    Private->NtError = Private->WinNtThunk->GetLastError ();
    DEBUG ((EFI_D_ERROR, "SerialSetAttributes: SetCommTimeouts %d\n", Private->NtError));
    gBS->RestoreTPL (Tpl);
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

  Status = EFI_SUCCESS;
  if (Private->Handle != NULL) {
    Uart = (UART_DEVICE_PATH *) (
             (UINTN) Private->DevicePath
             + GetDevicePathSize (Private->ParentDevicePath)
             - END_DEVICE_PATH_LENGTH
             );
    CopyMem (Uart, &Private->UartDevicePath, sizeof (UART_DEVICE_PATH));
    Status = gBS->ReinstallProtocolInterface (
                    Private->Handle,
                    &gEfiDevicePathProtocolGuid,
                    Private->DevicePath,
                    Private->DevicePath
                    );
  }

  gBS->RestoreTPL (Tpl);

  return Status;
}

EFI_STATUS
EFIAPI
WinNtSerialIoSetControl (
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
  WIN_NT_SERIAL_IO_PRIVATE_DATA *Private;
  BOOL                          Result;
  DCB                           Dcb;
  EFI_TPL                       Tpl;
  UART_FLOW_CONTROL_DEVICE_PATH *FlowControl;
  EFI_STATUS                    Status;

  //
  // first determine the parameter is invalid
  //
  if (Control & (~(EFI_SERIAL_REQUEST_TO_SEND | EFI_SERIAL_DATA_TERMINAL_READY |
                   EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE | EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE | 
                   EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE))) {
    return EFI_UNSUPPORTED;
  }

  Tpl     = gBS->RaiseTPL (TPL_NOTIFY);

  Private = WIN_NT_SERIAL_IO_PRIVATE_DATA_FROM_THIS (This);

  Result  = Private->WinNtThunk->GetCommState (Private->NtHandle, &Dcb);

  if (!Result) {
    Private->NtError = Private->WinNtThunk->GetLastError ();
    DEBUG ((EFI_D_ERROR, "SerialSetControl: GetCommState %d\n", Private->NtError));
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  Dcb.fRtsControl                 = RTS_CONTROL_DISABLE;
  Dcb.fDtrControl                 = DTR_CONTROL_DISABLE;
  Private->HardwareFlowControl    = FALSE;
  Private->SoftwareLoopbackEnable = FALSE;
  Private->HardwareLoopbackEnable = FALSE;

  if (Control & EFI_SERIAL_REQUEST_TO_SEND) {
    Dcb.fRtsControl = RTS_CONTROL_ENABLE;
  }

  if (Control & EFI_SERIAL_DATA_TERMINAL_READY) {
    Dcb.fDtrControl = DTR_CONTROL_ENABLE;
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

  Result = Private->WinNtThunk->SetCommState (
                                  Private->NtHandle,
                                  &Dcb
                                  );

  if (!Result) {
    Private->NtError = Private->WinNtThunk->GetLastError ();
    DEBUG ((EFI_D_ERROR, "SerialSetControl: SetCommState %d\n", Private->NtError));
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  Status = EFI_SUCCESS;
  if (Private->Handle != NULL) {
    FlowControl = (UART_FLOW_CONTROL_DEVICE_PATH *) (
                    (UINTN) Private->DevicePath
                    + GetDevicePathSize (Private->ParentDevicePath)
                    - END_DEVICE_PATH_LENGTH
                    + sizeof (UART_DEVICE_PATH)
                    );
    if (IsUartFlowControlNode (FlowControl) &&
        ((FlowControl->FlowControlMap == UART_FLOW_CONTROL_HARDWARE) ^ Private->HardwareFlowControl)) {
      //
      // Flow Control setting is changed, need to reinstall device path protocol
      //
      FlowControl->FlowControlMap = Private->HardwareFlowControl ? UART_FLOW_CONTROL_HARDWARE : 0;
      Status = gBS->ReinstallProtocolInterface (
                      Private->Handle,
                      &gEfiDevicePathProtocolGuid,
                      Private->DevicePath,
                      Private->DevicePath
                      );
    }
  }

  gBS->RestoreTPL (Tpl);

  return Status;
}

EFI_STATUS
EFIAPI
WinNtSerialIoGetControl (
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
  WIN_NT_SERIAL_IO_PRIVATE_DATA *Private;
  DWORD                         ModemStatus;
  DWORD                         Errors;
  UINT32                        Bits;
  DCB                           Dcb;
  EFI_TPL                       Tpl;

  Tpl     = gBS->RaiseTPL (TPL_NOTIFY);

  Private = WIN_NT_SERIAL_IO_PRIVATE_DATA_FROM_THIS (This);

  //
  //  Get modem status
  //
  if (!Private->WinNtThunk->GetCommModemStatus (Private->NtHandle, &ModemStatus)) {
    Private->NtError = Private->WinNtThunk->GetLastError ();
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  Bits = 0;
  if (ModemStatus & MS_CTS_ON) {
    Bits |= EFI_SERIAL_CLEAR_TO_SEND;
  }

  if (ModemStatus & MS_DSR_ON) {
    Bits |= EFI_SERIAL_DATA_SET_READY;
  }

  if (ModemStatus & MS_RING_ON) {
    Bits |= EFI_SERIAL_RING_INDICATE;
  }

  if (ModemStatus & MS_RLSD_ON) {
    Bits |= EFI_SERIAL_CARRIER_DETECT;
  }

  //
  // Get ctrl status
  //
  if (!Private->WinNtThunk->GetCommState (Private->NtHandle, &Dcb)) {
    Private->NtError = Private->WinNtThunk->GetLastError ();
    DEBUG ((EFI_D_ERROR, "SerialGetControl: GetCommState %d\n", Private->NtError));
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  if (Dcb.fDtrControl == DTR_CONTROL_ENABLE) {
    Bits |= EFI_SERIAL_DATA_TERMINAL_READY;
  }

  if (Dcb.fRtsControl == RTS_CONTROL_ENABLE) {
    Bits |= EFI_SERIAL_REQUEST_TO_SEND;
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

  //
  //  Get input buffer status
  //
  if (!Private->WinNtThunk->ClearCommError (Private->NtHandle, &Errors, &Private->NtComStatus)) {
    Private->NtError = Private->WinNtThunk->GetLastError ();
    DEBUG ((EFI_D_ERROR, "SerialGetControl: ClearCommError %d\n", Private->NtError));
    gBS->RestoreTPL (Tpl);
    return EFI_DEVICE_ERROR;
  }

  if (Private->NtComStatus.cbInQue == 0) {
    Bits |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
  }

  *Control = Bits;

  gBS->RestoreTPL (Tpl);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtSerialIoWrite (
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
  WIN_NT_SERIAL_IO_PRIVATE_DATA *Private;
  UINT8                         *ByteBuffer;
  UINTN                         TotalBytesWritten;
  DWORD                         BytesToGo;
  DWORD                         BytesWritten;
  BOOL                          Result;
  UINT32                        Index;
  UINT32                        Control;
  EFI_TPL                       Tpl;

  Tpl               = gBS->RaiseTPL (TPL_NOTIFY);

  Private           = WIN_NT_SERIAL_IO_PRIVATE_DATA_FROM_THIS (This);

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
    BytesToGo = (DWORD) (*BufferSize);

    do {
      if (Private->HardwareFlowControl) {
        //
        // Send RTS
        //
        WinNtSerialIoGetControl (&Private->SerialIo, &Control);
        Control |= EFI_SERIAL_REQUEST_TO_SEND;
        WinNtSerialIoSetControl (&Private->SerialIo, Control);
      }

      //
      //  Do the write
      //
      Result = Private->WinNtThunk->WriteFile (
                                      Private->NtHandle,
                                      &ByteBuffer[TotalBytesWritten],
                                      BytesToGo,
                                      &BytesWritten,
                                      NULL
                                      );

      if (Private->HardwareFlowControl) {
        //
        // Assert RTS
        //
        WinNtSerialIoGetControl (&Private->SerialIo, &Control);
        Control &= ~ (UINT32) EFI_SERIAL_REQUEST_TO_SEND;
        WinNtSerialIoSetControl (&Private->SerialIo, Control);
      }

      TotalBytesWritten += BytesWritten;
      BytesToGo -= BytesWritten;
      if (!Result) {
        Private->NtError = Private->WinNtThunk->GetLastError ();
        DEBUG ((EFI_D_ERROR, "SerialWrite: FileWrite %d\n", Private->NtError));
        *BufferSize = TotalBytesWritten;
        gBS->RestoreTPL (Tpl);
        return EFI_DEVICE_ERROR;
      }
    } while (BytesToGo > 0);
  }

  *BufferSize = TotalBytesWritten;

  gBS->RestoreTPL (Tpl);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtSerialIoRead (
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
  WIN_NT_SERIAL_IO_PRIVATE_DATA *Private;
  BOOL                          Result;
  DWORD                         BytesRead;
  EFI_STATUS                    Status;
  UINT32                        Index;
  UINT8                         Data;
  UINT32                        Control;
  EFI_TPL                       Tpl;

  Tpl     = gBS->RaiseTPL (TPL_NOTIFY);

  Private = WIN_NT_SERIAL_IO_PRIVATE_DATA_FROM_THIS (This);

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
      WinNtSerialIoGetControl (&Private->SerialIo, &Control);
      Control |= EFI_SERIAL_DATA_TERMINAL_READY;
      WinNtSerialIoSetControl (&Private->SerialIo, Control);
    }

    Result = Private->WinNtThunk->ReadFile (
                                    Private->NtHandle,
                                    Buffer,
                                    (DWORD) *BufferSize,
                                    &BytesRead,
                                    NULL
                                    );

    if (Private->HardwareFlowControl) {
      WinNtSerialIoGetControl (&Private->SerialIo, &Control);
      Control &= ~ (UINT32) EFI_SERIAL_DATA_TERMINAL_READY;
      WinNtSerialIoSetControl (&Private->SerialIo, Control);
    }

    if (!Result) {
      Private->NtError = Private->WinNtThunk->GetLastError ();
      gBS->RestoreTPL (Tpl);
      return EFI_DEVICE_ERROR;
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

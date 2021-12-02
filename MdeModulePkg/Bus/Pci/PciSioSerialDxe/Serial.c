/** @file
  Serial driver for PCI or SIO UARTS.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Serial.h"

//
// ISA Serial Driver Global Variables
//

EFI_DRIVER_BINDING_PROTOCOL  gSerialControllerDriver = {
  SerialControllerDriverSupported,
  SerialControllerDriverStart,
  SerialControllerDriverStop,
  0xa,
  NULL,
  NULL
};

CONTROLLER_DEVICE_PATH  mControllerDevicePathTemplate = {
  {
    HARDWARE_DEVICE_PATH,
    HW_CONTROLLER_DP,
    {
      (UINT8)(sizeof (CONTROLLER_DEVICE_PATH)),
      (UINT8)((sizeof (CONTROLLER_DEVICE_PATH)) >> 8)
    }
  },
  0
};

SERIAL_DEV  gSerialDevTemplate = {
  SERIAL_DEV_SIGNATURE,
  NULL,
  {
    SERIAL_IO_INTERFACE_REVISION,
    SerialReset,
    SerialSetAttributes,
    SerialSetControl,
    SerialGetControl,
    SerialWrite,
    SerialRead,
    NULL
  },                                       // SerialIo
  {
    SERIAL_PORT_SUPPORT_CONTROL_MASK,
    SERIAL_PORT_DEFAULT_TIMEOUT,
    0,
    16,
    0,
    0,
    0
  },                                       // SerialMode
  NULL,                                    // DevicePath
  NULL,                                    // ParentDevicePath
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_UART_DP,
      {
        (UINT8)(sizeof (UART_DEVICE_PATH)),
        (UINT8)((sizeof (UART_DEVICE_PATH)) >> 8)
      }
    },
    0,                                     0,0, 0, 0
  },                                            // UartDevicePath
  0,                                            // BaseAddress
  FALSE,                                        // MmioAccess
  1,                                            // RegisterStride
  0,                                            // ClockRate
  16,                                           // ReceiveFifoDepth
  { 0,                                     0 }, // Receive;
  16,                                           // TransmitFifoDepth
  { 0,                                     0 }, // Transmit;
  FALSE,                                        // SoftwareLoopbackEnable;
  FALSE,                                        // HardwareFlowControl;
  NULL,                                         // *ControllerNameTable;
  FALSE,                                        // ContainsControllerNode;
  0,                                            // Instance;
  NULL                                          // *PciDeviceInfo;
};

/**
  Check the device path node whether it's the Flow Control node or not.

  @param[in] FlowControl    The device path node to be checked.

  @retval TRUE              It's the Flow Control node.
  @retval FALSE             It's not.

**/
BOOLEAN
IsUartFlowControlDevicePathNode (
  IN UART_FLOW_CONTROL_DEVICE_PATH  *FlowControl
  )
{
  return (BOOLEAN)(
                   (DevicePathType (FlowControl) == MESSAGING_DEVICE_PATH) &&
                   (DevicePathSubType (FlowControl) == MSG_VENDOR_DP) &&
                   (CompareGuid (&FlowControl->Guid, &gEfiUartDevicePathGuid))
                   );
}

/**
  The user Entry Point for module PciSioSerial. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializePciSioSerial (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gSerialControllerDriver,
             ImageHandle,
             &gPciSioSerialComponentName,
             &gPciSioSerialComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize UART default setting in gSerialDevTempate
  //
  gSerialDevTemplate.SerialMode.BaudRate     = PcdGet64 (PcdUartDefaultBaudRate);
  gSerialDevTemplate.SerialMode.DataBits     = PcdGet8 (PcdUartDefaultDataBits);
  gSerialDevTemplate.SerialMode.Parity       = PcdGet8 (PcdUartDefaultParity);
  gSerialDevTemplate.SerialMode.StopBits     = PcdGet8 (PcdUartDefaultStopBits);
  gSerialDevTemplate.UartDevicePath.BaudRate = PcdGet64 (PcdUartDefaultBaudRate);
  gSerialDevTemplate.UartDevicePath.DataBits = PcdGet8 (PcdUartDefaultDataBits);
  gSerialDevTemplate.UartDevicePath.Parity   = PcdGet8 (PcdUartDefaultParity);
  gSerialDevTemplate.UartDevicePath.StopBits = PcdGet8 (PcdUartDefaultStopBits);
  gSerialDevTemplate.ClockRate               = PcdGet32 (PcdSerialClockRate);

  return Status;
}

/**
  Return whether the controller is a SIO serial controller.

  @param  Controller   The controller handle.

  @retval EFI_SUCCESS  The controller is a SIO serial controller.
  @retval others       The controller is not a SIO serial controller.
**/
EFI_STATUS
IsSioSerialController (
  EFI_HANDLE  Controller
  )
{
  EFI_STATUS                Status;
  EFI_SIO_PROTOCOL          *Sio;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  ACPI_HID_DEVICE_PATH      *Acpi;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSioProtocolGuid,
                  (VOID **)&Sio,
                  gSerialControllerDriver.DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (!EFI_ERROR (Status)) {
    //
    // Close the I/O Abstraction(s) used to perform the supported test
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiSioProtocolGuid,
           gSerialControllerDriver.DriverBindingHandle,
           Controller
           );

    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath,
                    gSerialControllerDriver.DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );
    ASSERT (Status != EFI_ALREADY_STARTED);

    if (!EFI_ERROR (Status)) {
      do {
        Acpi       = (ACPI_HID_DEVICE_PATH *)DevicePath;
        DevicePath = NextDevicePathNode (DevicePath);
      } while (!IsDevicePathEnd (DevicePath));

      if ((DevicePathType (Acpi) != ACPI_DEVICE_PATH) ||
          ((DevicePathSubType (Acpi) != ACPI_DP) && (DevicePathSubType (Acpi) != ACPI_EXTENDED_DP)) ||
          (Acpi->HID != EISA_PNP_ID (0x501))
          )
      {
        Status = EFI_UNSUPPORTED;
      }
    }

    //
    // Close protocol, don't use device path protocol in the Support() function
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           gSerialControllerDriver.DriverBindingHandle,
           Controller
           );
  }

  return Status;
}

/**
  Return whether the controller is a PCI serial controller.

  @param  Controller   The controller handle.

  @retval EFI_SUCCESS  The controller is a PCI serial controller.
  @retval others       The controller is not a PCI serial controller.
**/
EFI_STATUS
IsPciSerialController (
  EFI_HANDLE  Controller
  )
{
  EFI_STATUS                Status;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  PCI_TYPE00                Pci;
  PCI_SERIAL_PARAMETER      *PciSerialParameter;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  gSerialControllerDriver.DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (!EFI_ERROR (Status)) {
    Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0, sizeof (Pci), &Pci);
    if (!EFI_ERROR (Status)) {
      if (!IS_PCI_16550_SERIAL (&Pci)) {
        for (PciSerialParameter = (PCI_SERIAL_PARAMETER *)PcdGetPtr (PcdPciSerialParameters)
             ; PciSerialParameter->VendorId != 0xFFFF
             ; PciSerialParameter++
             )
        {
          if ((Pci.Hdr.VendorId == PciSerialParameter->VendorId) &&
              (Pci.Hdr.DeviceId == PciSerialParameter->DeviceId)
              )
          {
            break;
          }
        }

        if (PciSerialParameter->VendorId == 0xFFFF) {
          Status = EFI_UNSUPPORTED;
        } else {
          Status = EFI_SUCCESS;
        }
      }
    }

    //
    // Close the I/O Abstraction(s) used to perform the supported test
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           gSerialControllerDriver.DriverBindingHandle,
           Controller
           );
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the EFI Device Path protocol needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  gSerialControllerDriver.DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  ASSERT (Status != EFI_ALREADY_STARTED);

  //
  // Close protocol, don't use device path protocol in the Support() function
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         gSerialControllerDriver.DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Check to see if this driver supports the given controller

  @param  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  Controller           The handle of the controller to test.
  @param  RemainingDevicePath  A pointer to the remaining portion of a device path.

  @return EFI_SUCCESS          This driver can support the given controller

**/
EFI_STATUS
EFIAPI
SerialControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )

{
  EFI_STATUS                     Status;
  UART_DEVICE_PATH               *Uart;
  UART_FLOW_CONTROL_DEVICE_PATH  *FlowControl;

  //
  // Test RemainingDevicePath
  //
  if ((RemainingDevicePath != NULL) && !IsDevicePathEnd (RemainingDevicePath)) {
    Status = EFI_UNSUPPORTED;

    Uart = SkipControllerDevicePathNode (RemainingDevicePath, NULL, NULL);
    if ((DevicePathType (Uart) != MESSAGING_DEVICE_PATH) ||
        (DevicePathSubType (Uart) != MSG_UART_DP) ||
        (DevicePathNodeLength (Uart) != sizeof (UART_DEVICE_PATH))
        )
    {
      return EFI_UNSUPPORTED;
    }

    //
    // Do a rough check because Clock Rate is unknown until DriverBindingStart()
    //
    if (!VerifyUartParameters (0, Uart->BaudRate, Uart->DataBits, Uart->Parity, Uart->StopBits, NULL, NULL)) {
      return EFI_UNSUPPORTED;
    }

    FlowControl = (UART_FLOW_CONTROL_DEVICE_PATH *)NextDevicePathNode (Uart);
    if (IsUartFlowControlDevicePathNode (FlowControl)) {
      //
      // If the second node is Flow Control Node,
      //   return error when it request other than hardware flow control.
      //
      if ((ReadUnaligned32 (&FlowControl->FlowControlMap) & ~UART_FLOW_CONTROL_HARDWARE) != 0) {
        return EFI_UNSUPPORTED;
      }
    }
  }

  Status = IsSioSerialController (Controller);
  if (EFI_ERROR (Status)) {
    Status = IsPciSerialController (Controller);
  }

  return Status;
}

/**
  Create the child serial device instance.

  @param Controller           The parent controller handle.
  @param Uart                 Pointer to the UART device path node in RemainingDevicePath,
                              or NULL if RemainingDevicePath is NULL.
  @param ParentDevicePath     Pointer to the parent device path.
  @param CreateControllerNode TRUE to create the controller node.
  @param Instance             Instance number of the serial device.
                              The value will be set to the controller node
                              if CreateControllerNode is TRUE.
  @param ParentIo             A union type pointer to either Sio or PciIo.
  @param PciSerialParameter   The PCI serial parameter to be used by current serial device.
                              NULL for SIO serial device.
  @param PciDeviceInfo        The PCI device info for the current serial device.
                              NULL for SIO serial device.

  @retval EFI_SUCCESS         The serial device was created successfully.
  @retval others              The serial device wasn't created.
**/
EFI_STATUS
CreateSerialDevice (
  IN EFI_HANDLE                Controller,
  IN UART_DEVICE_PATH          *Uart,
  IN EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath,
  IN BOOLEAN                   CreateControllerNode,
  IN UINT32                    Instance,
  IN PARENT_IO_PROTOCOL_PTR    ParentIo,
  IN PCI_SERIAL_PARAMETER      *PciSerialParameter  OPTIONAL,
  IN PCI_DEVICE_INFO           *PciDeviceInfo       OPTIONAL
  )
{
  EFI_STATUS                                  Status;
  SERIAL_DEV                                  *SerialDevice;
  UINT8                                       BarIndex;
  UINT64                                      Offset;
  UART_FLOW_CONTROL_DEVICE_PATH               *FlowControl;
  UINT32                                      FlowControlMap;
  ACPI_RESOURCE_HEADER_PTR                    Resources;
  EFI_ACPI_IO_PORT_DESCRIPTOR                 *Io;
  EFI_ACPI_FIXED_LOCATION_IO_PORT_DESCRIPTOR  *FixedIo;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR           *AddressSpace;
  EFI_DEVICE_PATH_PROTOCOL                    *TempDevicePath;

  BarIndex       = 0;
  Offset         = 0;
  FlowControl    = NULL;
  FlowControlMap = 0;

  //
  // Initialize the serial device instance
  //
  SerialDevice = AllocateCopyPool (sizeof (SERIAL_DEV), &gSerialDevTemplate);
  ASSERT (SerialDevice != NULL);

  SerialDevice->SerialIo.Mode    = &(SerialDevice->SerialMode);
  SerialDevice->ParentDevicePath = ParentDevicePath;
  SerialDevice->PciDeviceInfo    = PciDeviceInfo;
  SerialDevice->Instance         = Instance;

  if (Uart != NULL) {
    CopyMem (&SerialDevice->UartDevicePath, Uart, sizeof (UART_DEVICE_PATH));
    FlowControl = (UART_FLOW_CONTROL_DEVICE_PATH *)NextDevicePathNode (Uart);
    if (IsUartFlowControlDevicePathNode (FlowControl)) {
      FlowControlMap = ReadUnaligned32 (&FlowControl->FlowControlMap);
    } else {
      FlowControl = NULL;
    }
  }

  //
  // For PCI serial device, use the information from PCD
  //
  if (PciSerialParameter != NULL) {
    BarIndex = (PciSerialParameter->BarIndex == MAX_UINT8) ? 0 : PciSerialParameter->BarIndex;
    Offset   = PciSerialParameter->Offset;
    if (PciSerialParameter->RegisterStride != 0) {
      SerialDevice->RegisterStride = PciSerialParameter->RegisterStride;
    }

    if (PciSerialParameter->ClockRate != 0) {
      SerialDevice->ClockRate = PciSerialParameter->ClockRate;
    }

    if (PciSerialParameter->ReceiveFifoDepth != 0) {
      SerialDevice->ReceiveFifoDepth = PciSerialParameter->ReceiveFifoDepth;
    }

    if (PciSerialParameter->TransmitFifoDepth != 0) {
      SerialDevice->TransmitFifoDepth = PciSerialParameter->TransmitFifoDepth;
    }
  }

  //
  // Pass NULL ActualBaudRate to VerifyUartParameters to disallow baudrate degrade.
  // DriverBindingStart() shouldn't create a handle with different UART device path.
  //
  if (!VerifyUartParameters (
         SerialDevice->ClockRate,
         SerialDevice->UartDevicePath.BaudRate,
         SerialDevice->UartDevicePath.DataBits,
         SerialDevice->UartDevicePath.Parity,
         SerialDevice->UartDevicePath.StopBits,
         NULL,
         NULL
         ))
  {
    Status = EFI_INVALID_PARAMETER;
    goto CreateError;
  }

  if (PciSerialParameter == NULL) {
    Status = ParentIo.Sio->GetResources (ParentIo.Sio, &Resources);
  } else {
    Status = ParentIo.PciIo->GetBarAttributes (ParentIo.PciIo, BarIndex, NULL, (VOID **)&Resources);
  }

  if (!EFI_ERROR (Status)) {
    //
    // Get the base address information from ACPI resource descriptor.
    // ACPI_IO_PORT_DESCRIPTOR and ACPI_FIXED_LOCATION_IO_PORT_DESCRIPTOR are returned from Sio;
    // ACPI_ADDRESS_SPACE_DESCRIPTOR is returned from PciIo.
    //
    while ((Resources.SmallHeader->Byte != ACPI_END_TAG_DESCRIPTOR) && (SerialDevice->BaseAddress == 0)) {
      switch (Resources.SmallHeader->Byte) {
        case ACPI_IO_PORT_DESCRIPTOR:
          Io = (EFI_ACPI_IO_PORT_DESCRIPTOR *)Resources.SmallHeader;
          if (Io->Length != 0) {
            SerialDevice->BaseAddress = Io->BaseAddressMin;
          }

          break;

        case ACPI_FIXED_LOCATION_IO_PORT_DESCRIPTOR:
          FixedIo = (EFI_ACPI_FIXED_LOCATION_IO_PORT_DESCRIPTOR *)Resources.SmallHeader;
          if (FixedIo->Length != 0) {
            SerialDevice->BaseAddress = FixedIo->BaseAddress;
          }

          break;

        case ACPI_ADDRESS_SPACE_DESCRIPTOR:
          AddressSpace = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Resources.SmallHeader;
          if (AddressSpace->AddrLen != 0) {
            if (AddressSpace->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
              SerialDevice->MmioAccess = TRUE;
            }

            SerialDevice->BaseAddress = AddressSpace->AddrRangeMin + Offset;
          }

          break;
      }

      if (Resources.SmallHeader->Bits.Type == 0) {
        Resources.SmallHeader = (ACPI_SMALL_RESOURCE_HEADER *)((UINT8 *)Resources.SmallHeader
                                                               + Resources.SmallHeader->Bits.Length
                                                               + sizeof (*Resources.SmallHeader));
      } else {
        Resources.LargeHeader = (ACPI_LARGE_RESOURCE_HEADER *)((UINT8 *)Resources.LargeHeader
                                                               + Resources.LargeHeader->Length
                                                               + sizeof (*Resources.LargeHeader));
      }
    }
  }

  if (SerialDevice->BaseAddress == 0) {
    Status = EFI_INVALID_PARAMETER;
    goto CreateError;
  }

  SerialDevice->HardwareFlowControl = (BOOLEAN)(FlowControlMap == UART_FLOW_CONTROL_HARDWARE);

  //
  // Report status code the serial present
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_P_PC_PRESENCE_DETECT | EFI_PERIPHERAL_SERIAL_PORT,
    SerialDevice->ParentDevicePath
    );

  if (!SerialPresent (SerialDevice)) {
    Status = EFI_DEVICE_ERROR;
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE,
      EFI_P_EC_NOT_DETECTED | EFI_PERIPHERAL_SERIAL_PORT,
      SerialDevice->ParentDevicePath
      );
    goto CreateError;
  }

  //
  // 1. Append Controller device path node.
  //
  if (CreateControllerNode) {
    mControllerDevicePathTemplate.ControllerNumber = SerialDevice->Instance;
    SerialDevice->DevicePath                       = AppendDevicePathNode (
                                                       SerialDevice->ParentDevicePath,
                                                       (EFI_DEVICE_PATH_PROTOCOL *)&mControllerDevicePathTemplate
                                                       );
    SerialDevice->ContainsControllerNode = TRUE;
  }

  //
  // 2. Append UART device path node.
  //    The Uart setings are zero here.
  //    SetAttribute() will update them to match the default setings.
  //
  TempDevicePath = SerialDevice->DevicePath;
  if (TempDevicePath != NULL) {
    SerialDevice->DevicePath = AppendDevicePathNode (
                                 TempDevicePath,
                                 (EFI_DEVICE_PATH_PROTOCOL *)&SerialDevice->UartDevicePath
                                 );
    FreePool (TempDevicePath);
  } else {
    SerialDevice->DevicePath = AppendDevicePathNode (
                                 SerialDevice->ParentDevicePath,
                                 (EFI_DEVICE_PATH_PROTOCOL *)&SerialDevice->UartDevicePath
                                 );
  }

  //
  // 3. Append the Flow Control device path node.
  //    Only produce the Flow Control node when remaining device path has it
  //
  if (FlowControl != NULL) {
    TempDevicePath = SerialDevice->DevicePath;
    if (TempDevicePath != NULL) {
      SerialDevice->DevicePath = AppendDevicePathNode (
                                   TempDevicePath,
                                   (EFI_DEVICE_PATH_PROTOCOL *)FlowControl
                                   );
      FreePool (TempDevicePath);
    }
  }

  ASSERT (SerialDevice->DevicePath != NULL);

  //
  // Fill in Serial I/O Mode structure based on either the RemainingDevicePath or defaults.
  //
  SerialDevice->SerialMode.BaudRate = SerialDevice->UartDevicePath.BaudRate;
  SerialDevice->SerialMode.DataBits = SerialDevice->UartDevicePath.DataBits;
  SerialDevice->SerialMode.Parity   = SerialDevice->UartDevicePath.Parity;
  SerialDevice->SerialMode.StopBits = SerialDevice->UartDevicePath.StopBits;

  //
  // Issue a reset to initialize the COM port
  //
  Status = SerialDevice->SerialIo.Reset (&SerialDevice->SerialIo);
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE,
      EFI_P_EC_CONTROLLER_ERROR | EFI_PERIPHERAL_SERIAL_PORT,
      SerialDevice->DevicePath
      );
    goto CreateError;
  }

  AddName (SerialDevice, Instance);
  //
  // Install protocol interfaces for the serial device.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &SerialDevice->Handle,
                  &gEfiDevicePathProtocolGuid,
                  SerialDevice->DevicePath,
                  &gEfiSerialIoProtocolGuid,
                  &SerialDevice->SerialIo,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto CreateError;
  }

  //
  // Open For Child Device
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  PciSerialParameter != NULL ? &gEfiPciIoProtocolGuid : &gEfiSioProtocolGuid,
                  (VOID **)&ParentIo,
                  gSerialControllerDriver.DriverBindingHandle,
                  SerialDevice->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );

  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           SerialDevice->Handle,
           &gEfiDevicePathProtocolGuid,
           SerialDevice->DevicePath,
           &gEfiSerialIoProtocolGuid,
           &SerialDevice->SerialIo,
           NULL
           );
  }

CreateError:
  if (EFI_ERROR (Status)) {
    if (SerialDevice->DevicePath != NULL) {
      FreePool (SerialDevice->DevicePath);
    }

    if (SerialDevice->ControllerNameTable != NULL) {
      FreeUnicodeStringTable (SerialDevice->ControllerNameTable);
    }

    FreePool (SerialDevice);
  }

  return Status;
}

/**
  Returns an array of pointers containing all the child serial device pointers.

  @param Controller      The parent controller handle.
  @param IoProtocolGuid  The protocol GUID, either equals to gEfiSioProtocolGuid
                         or equals to gEfiPciIoProtocolGuid.
  @param Count           Count of the serial devices.

  @return  An array of pointers containing all the child serial device pointers.
**/
SERIAL_DEV **
GetChildSerialDevices (
  IN EFI_HANDLE  Controller,
  IN EFI_GUID    *IoProtocolGuid,
  OUT UINTN      *Count
  )
{
  EFI_STATUS                           Status;
  UINTN                                Index;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfoBuffer;
  UINTN                                EntryCount;
  SERIAL_DEV                           **SerialDevices;
  EFI_SERIAL_IO_PROTOCOL               *SerialIo;
  BOOLEAN                              OpenByDriver;

  *Count = 0;
  //
  // If the SerialIo instance specified by RemainingDevicePath is already created,
  // update the attributes/control.
  //
  Status = gBS->OpenProtocolInformation (
                  Controller,
                  IoProtocolGuid,
                  &OpenInfoBuffer,
                  &EntryCount
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  SerialDevices = AllocatePool (EntryCount * sizeof (SERIAL_DEV *));
  ASSERT (SerialDevices != NULL);

  *Count       = 0;
  OpenByDriver = FALSE;
  for (Index = 0; Index < EntryCount; Index++) {
    if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
      Status = gBS->OpenProtocol (
                      OpenInfoBuffer[Index].ControllerHandle,
                      &gEfiSerialIoProtocolGuid,
                      (VOID **)&SerialIo,
                      gSerialControllerDriver.DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (!EFI_ERROR (Status)) {
        SerialDevices[(*Count)++] = SERIAL_DEV_FROM_THIS (SerialIo);
      }
    }

    if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
      ASSERT (OpenInfoBuffer[Index].AgentHandle == gSerialControllerDriver.DriverBindingHandle);
      OpenByDriver = TRUE;
    }
  }

  if (OpenInfoBuffer != NULL) {
    FreePool (OpenInfoBuffer);
  }

  ASSERT ((*Count == 0) || (OpenByDriver));

  return SerialDevices;
}

/**
  Start to management the controller passed in

  @param  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  Controller           The handle of the controller to test.
  @param  RemainingDevicePath  A pointer to the remaining portion of a device path.

  @return EFI_SUCCESS   Driver is started successfully
**/
EFI_STATUS
EFIAPI
SerialControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                     Status;
  UINTN                          Index;
  EFI_DEVICE_PATH_PROTOCOL       *ParentDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       *Node;
  EFI_SERIAL_IO_PROTOCOL         *SerialIo;
  UINT32                         ControllerNumber;
  UART_DEVICE_PATH               *Uart;
  UART_FLOW_CONTROL_DEVICE_PATH  *FlowControl;
  UINT32                         Control;
  PARENT_IO_PROTOCOL_PTR         ParentIo;
  ACPI_HID_DEVICE_PATH           *Acpi;
  EFI_GUID                       *IoProtocolGuid;
  PCI_SERIAL_PARAMETER           *PciSerialParameter;
  PCI_SERIAL_PARAMETER           DefaultPciSerialParameter;
  PCI_TYPE00                     Pci;
  UINT32                         PciSerialCount;
  SERIAL_DEV                     **SerialDevices;
  UINTN                          SerialDeviceCount;
  PCI_DEVICE_INFO                *PciDeviceInfo;
  UINT64                         Supports;
  BOOLEAN                        ContainsControllerNode;

  //
  // Get the Parent Device Path
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  //
  // Report status code enable the serial
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_P_PC_ENABLE | EFI_PERIPHERAL_SERIAL_PORT,
    ParentDevicePath
    );

  //
  // Grab the IO abstraction we need to get any work done
  //
  IoProtocolGuid = &gEfiSioProtocolGuid;
  Status         = gBS->OpenProtocol (
                          Controller,
                          IoProtocolGuid,
                          (VOID **)&ParentIo,
                          This->DriverBindingHandle,
                          Controller,
                          EFI_OPEN_PROTOCOL_BY_DRIVER
                          );
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    IoProtocolGuid = &gEfiPciIoProtocolGuid;
    Status         = gBS->OpenProtocol (
                            Controller,
                            IoProtocolGuid,
                            (VOID **)&ParentIo,
                            This->DriverBindingHandle,
                            Controller,
                            EFI_OPEN_PROTOCOL_BY_DRIVER
                            );
  }

  ASSERT (!EFI_ERROR (Status) || Status == EFI_ALREADY_STARTED);

  //
  // Do nothing for END device path node
  //
  if ((RemainingDevicePath != NULL) && IsDevicePathEnd (RemainingDevicePath)) {
    return EFI_SUCCESS;
  }

  ControllerNumber       = 0;
  ContainsControllerNode = FALSE;
  SerialDevices          = GetChildSerialDevices (Controller, IoProtocolGuid, &SerialDeviceCount);

  if (SerialDeviceCount != 0) {
    if (RemainingDevicePath == NULL) {
      //
      // If the SerialIo instance is already created, NULL as RemainingDevicePath is treated
      // as to create the same SerialIo instance.
      //
      return EFI_SUCCESS;
    } else {
      //
      // Update the attributes/control of the SerialIo instance specified by RemainingDevicePath.
      //
      Uart = (UART_DEVICE_PATH *)SkipControllerDevicePathNode (RemainingDevicePath, &ContainsControllerNode, &ControllerNumber);
      for (Index = 0; Index < SerialDeviceCount; Index++) {
        ASSERT ((SerialDevices != NULL) && (SerialDevices[Index] != NULL));
        if ((!SerialDevices[Index]->ContainsControllerNode && !ContainsControllerNode) ||
            (SerialDevices[Index]->ContainsControllerNode && ContainsControllerNode && (SerialDevices[Index]->Instance == ControllerNumber))
            )
        {
          SerialIo = &SerialDevices[Index]->SerialIo;
          Status   = EFI_INVALID_PARAMETER;
          //
          // Pass NULL ActualBaudRate to VerifyUartParameters to disallow baudrate degrade.
          // DriverBindingStart() shouldn't create a handle with different UART device path.
          //
          if (VerifyUartParameters (
                SerialDevices[Index]->ClockRate,
                Uart->BaudRate,
                Uart->DataBits,
                (EFI_PARITY_TYPE)Uart->Parity,
                (EFI_STOP_BITS_TYPE)Uart->StopBits,
                NULL,
                NULL
                ))
          {
            Status = SerialIo->SetAttributes (
                                 SerialIo,
                                 Uart->BaudRate,
                                 SerialIo->Mode->ReceiveFifoDepth,
                                 SerialIo->Mode->Timeout,
                                 (EFI_PARITY_TYPE)Uart->Parity,
                                 Uart->DataBits,
                                 (EFI_STOP_BITS_TYPE)Uart->StopBits
                                 );
          }

          FlowControl = (UART_FLOW_CONTROL_DEVICE_PATH *)NextDevicePathNode (Uart);
          if (!EFI_ERROR (Status) && IsUartFlowControlDevicePathNode (FlowControl)) {
            Status = SerialIo->GetControl (SerialIo, &Control);
            if (!EFI_ERROR (Status)) {
              if (ReadUnaligned32 (&FlowControl->FlowControlMap) == UART_FLOW_CONTROL_HARDWARE) {
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

          break;
        }
      }

      if (Index != SerialDeviceCount) {
        //
        // Directly return if the SerialIo instance specified by RemainingDevicePath is found and updated.
        // Otherwise continue to create the instance specified by RemainingDevicePath.
        //
        if (SerialDevices != NULL) {
          FreePool (SerialDevices);
        }

        return Status;
      }
    }
  }

  if (RemainingDevicePath != NULL) {
    Uart = (UART_DEVICE_PATH *)SkipControllerDevicePathNode (RemainingDevicePath, &ContainsControllerNode, &ControllerNumber);
  } else {
    Uart = NULL;
  }

  PciDeviceInfo = NULL;
  if (IoProtocolGuid == &gEfiSioProtocolGuid) {
    Status = EFI_NOT_FOUND;
    if ((RemainingDevicePath == NULL) || !ContainsControllerNode) {
      Node = ParentDevicePath;
      do {
        Acpi = (ACPI_HID_DEVICE_PATH *)Node;
        Node = NextDevicePathNode (Node);
      } while (!IsDevicePathEnd (Node));

      Status = CreateSerialDevice (Controller, Uart, ParentDevicePath, FALSE, Acpi->UID, ParentIo, NULL, NULL);
      DEBUG ((DEBUG_INFO, "PciSioSerial: Create SIO child serial device - %r\n", Status));
    }
  } else {
    Status = ParentIo.PciIo->Pci.Read (ParentIo.PciIo, EfiPciIoWidthUint8, 0, sizeof (Pci), &Pci);
    if (!EFI_ERROR (Status)) {
      //
      // PcdPciSerialParameters takes the higher priority.
      //
      PciSerialCount = 0;
      for (PciSerialParameter = PcdGetPtr (PcdPciSerialParameters); PciSerialParameter->VendorId != 0xFFFF; PciSerialParameter++) {
        if ((PciSerialParameter->VendorId == Pci.Hdr.VendorId) &&
            (PciSerialParameter->DeviceId == Pci.Hdr.DeviceId)
            )
        {
          PciSerialCount++;
        }
      }

      if (SerialDeviceCount == 0) {
        //
        // Enable the IO & MEM decoding when creating the first child.
        // Restore the PCI attributes when all children is destroyed (PciDeviceInfo->ChildCount == 0).
        //
        PciDeviceInfo = AllocatePool (sizeof (PCI_DEVICE_INFO));
        ASSERT (PciDeviceInfo != NULL);
        PciDeviceInfo->ChildCount = 0;
        PciDeviceInfo->PciIo      = ParentIo.PciIo;
        Status                    = ParentIo.PciIo->Attributes (
                                                      ParentIo.PciIo,
                                                      EfiPciIoAttributeOperationGet,
                                                      0,
                                                      &PciDeviceInfo->PciAttributes
                                                      );

        if (!EFI_ERROR (Status)) {
          Status = ParentIo.PciIo->Attributes (
                                     ParentIo.PciIo,
                                     EfiPciIoAttributeOperationSupported,
                                     0,
                                     &Supports
                                     );
          if (!EFI_ERROR (Status)) {
            Supports &= (UINT64)(EFI_PCI_IO_ATTRIBUTE_IO | EFI_PCI_IO_ATTRIBUTE_MEMORY);
            Status    = ParentIo.PciIo->Attributes (
                                          ParentIo.PciIo,
                                          EfiPciIoAttributeOperationEnable,
                                          Supports,
                                          NULL
                                          );
          }
        }
      } else {
        //
        // Re-use the PciDeviceInfo stored in existing children.
        //
        ASSERT ((SerialDevices != NULL) && (SerialDevices[0] != NULL));
        PciDeviceInfo = SerialDevices[0]->PciDeviceInfo;
        ASSERT (PciDeviceInfo != NULL);
      }

      Status = EFI_NOT_FOUND;
      if (PciSerialCount <= 1) {
        //
        // PCI serial device contains only one UART
        //
        if ((RemainingDevicePath == NULL) || !ContainsControllerNode) {
          //
          // This PCI serial device is matched by class code in Supported()
          //
          if (PciSerialCount == 0) {
            DefaultPciSerialParameter.VendorId       = Pci.Hdr.VendorId;
            DefaultPciSerialParameter.DeviceId       = Pci.Hdr.DeviceId;
            DefaultPciSerialParameter.BarIndex       = 0;
            DefaultPciSerialParameter.Offset         = 0;
            DefaultPciSerialParameter.RegisterStride = 0;
            DefaultPciSerialParameter.ClockRate      = 0;
            PciSerialParameter                       = &DefaultPciSerialParameter;
          } else if (PciSerialCount == 1) {
            PciSerialParameter = PcdGetPtr (PcdPciSerialParameters);
          }

          Status = CreateSerialDevice (Controller, Uart, ParentDevicePath, FALSE, 0, ParentIo, PciSerialParameter, PciDeviceInfo);
          DEBUG ((DEBUG_INFO, "PciSioSerial: Create PCI child serial device (single) - %r\n", Status));
          if (!EFI_ERROR (Status)) {
            PciDeviceInfo->ChildCount++;
          }
        }
      } else {
        //
        // PCI serial device contains multiple UARTs
        //
        if ((RemainingDevicePath == NULL) || ContainsControllerNode) {
          PciSerialCount = 0;
          for (PciSerialParameter = PcdGetPtr (PcdPciSerialParameters); PciSerialParameter->VendorId != 0xFFFF; PciSerialParameter++) {
            if ((PciSerialParameter->VendorId == Pci.Hdr.VendorId) &&
                (PciSerialParameter->DeviceId == Pci.Hdr.DeviceId) &&
                ((RemainingDevicePath == NULL) || (ControllerNumber == PciSerialCount))
                )
            {
              //
              // Create controller node when PCI serial device contains multiple UARTs
              //
              Status = CreateSerialDevice (Controller, Uart, ParentDevicePath, TRUE, PciSerialCount, ParentIo, PciSerialParameter, PciDeviceInfo);
              PciSerialCount++;
              DEBUG ((DEBUG_INFO, "PciSioSerial: Create PCI child serial device (multiple) - %r\n", Status));
              if (!EFI_ERROR (Status)) {
                PciDeviceInfo->ChildCount++;
              }
            }
          }
        }
      }
    }
  }

  if (SerialDevices != NULL) {
    FreePool (SerialDevices);
  }

  //
  // For multiple PCI serial devices, set Status to SUCCESS if one child is created successfully
  //
  if ((PciDeviceInfo != NULL) && (PciDeviceInfo->ChildCount != 0)) {
    Status = EFI_SUCCESS;
  }

  if (EFI_ERROR (Status) && (SerialDeviceCount == 0)) {
    if (PciDeviceInfo != NULL) {
      Status = ParentIo.PciIo->Attributes (
                                 ParentIo.PciIo,
                                 EfiPciIoAttributeOperationSet,
                                 PciDeviceInfo->PciAttributes,
                                 NULL
                                 );
      ASSERT_EFI_ERROR (Status);
      FreePool (PciDeviceInfo);
    }

    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    gBS->CloseProtocol (
           Controller,
           IoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }

  return Status;
}

/**
  Disconnect this driver with the controller, uninstall related protocol instance

  @param  This                  A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  Controller            The handle of the controller to test.
  @param  NumberOfChildren      Number of child device.
  @param  ChildHandleBuffer     A pointer to the remaining portion of a device path.

  @retval EFI_SUCCESS           Operation successfully
  @retval EFI_DEVICE_ERROR      Cannot stop the driver successfully

**/
EFI_STATUS
EFIAPI
SerialControllerDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )

{
  EFI_STATUS                Status;
  UINTN                     Index;
  BOOLEAN                   AllChildrenStopped;
  EFI_SERIAL_IO_PROTOCOL    *SerialIo;
  SERIAL_DEV                *SerialDevice;
  VOID                      *IoProtocol;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  PCI_DEVICE_INFO           *PciDeviceInfo;

  PciDeviceInfo = NULL;

  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath
                  );

  //
  // Report the status code disable the serial
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_P_PC_DISABLE | EFI_PERIPHERAL_SERIAL_PORT,
    DevicePath
    );

  if (NumberOfChildren == 0) {
    //
    // Close the bus driver
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiPciIoProtocolGuid,
                    &IoProtocol,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    gBS->CloseProtocol (
           Controller,
           !EFI_ERROR (Status) ? &gEfiPciIoProtocolGuid : &gEfiSioProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiSerialIoProtocolGuid,
                    (VOID **)&SerialIo,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      SerialDevice = SERIAL_DEV_FROM_THIS (SerialIo);
      ASSERT ((PciDeviceInfo == NULL) || (PciDeviceInfo == SerialDevice->PciDeviceInfo));
      PciDeviceInfo = SerialDevice->PciDeviceInfo;

      Status = gBS->CloseProtocol (
                      Controller,
                      PciDeviceInfo != NULL ? &gEfiPciIoProtocolGuid : &gEfiSioProtocolGuid,
                      This->DriverBindingHandle,
                      ChildHandleBuffer[Index]
                      );

      Status = gBS->UninstallMultipleProtocolInterfaces (
                      ChildHandleBuffer[Index],
                      &gEfiDevicePathProtocolGuid,
                      SerialDevice->DevicePath,
                      &gEfiSerialIoProtocolGuid,
                      &SerialDevice->SerialIo,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        gBS->OpenProtocol (
               Controller,
               PciDeviceInfo != NULL ? &gEfiPciIoProtocolGuid : &gEfiSioProtocolGuid,
               &IoProtocol,
               This->DriverBindingHandle,
               ChildHandleBuffer[Index],
               EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
               );
      } else {
        FreePool (SerialDevice->DevicePath);
        FreeUnicodeStringTable (SerialDevice->ControllerNameTable);
        FreePool (SerialDevice);

        if (PciDeviceInfo != NULL) {
          ASSERT (PciDeviceInfo->ChildCount != 0);
          PciDeviceInfo->ChildCount--;
        }
      }
    }

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  } else {
    //
    // If all children are destroyed, restore the PCI attributes.
    //
    if ((PciDeviceInfo != NULL) && (PciDeviceInfo->ChildCount == 0)) {
      ASSERT (PciDeviceInfo->PciIo != NULL);
      Status = PciDeviceInfo->PciIo->Attributes (
                                       PciDeviceInfo->PciIo,
                                       EfiPciIoAttributeOperationSet,
                                       PciDeviceInfo->PciAttributes,
                                       NULL
                                       );
      ASSERT_EFI_ERROR (Status);
      FreePool (PciDeviceInfo);
    }

    return EFI_SUCCESS;
  }
}

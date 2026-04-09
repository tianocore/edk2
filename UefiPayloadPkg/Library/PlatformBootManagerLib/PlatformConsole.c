/** @file
This file include all platform action which can be customized by IBV/OEM.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformBootManager.h"
#include "PlatformConsole.h"
#include <Guid/SerialPortLibVendor.h>

#define PCI_DEVICE_PATH_NODE(Func, Dev) \
  { \
    { \
      HARDWARE_DEVICE_PATH, \
      HW_PCI_DP, \
      { \
        (UINT8) (sizeof (PCI_DEVICE_PATH)), \
        (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8) \
      } \
    }, \
    (Func), \
    (Dev) \
  }

#define PNPID_DEVICE_PATH_NODE(PnpId) \
  { \
    { \
      ACPI_DEVICE_PATH, \
      ACPI_DP, \
      { \
        (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), \
        (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8) \
      }, \
    }, \
    EISA_PNP_ID((PnpId)), \
    0 \
  }

#define gPnp16550ComPort \
  PNPID_DEVICE_PATH_NODE(0x0501)

#define gPnpPs2Keyboard \
  PNPID_DEVICE_PATH_NODE(0x0303)

#define gPcAnsiTerminal \
  { \
    { \
      MESSAGING_DEVICE_PATH, \
      MSG_VENDOR_DP, \
      { \
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)), \
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8) \
      } \
    }, \
    DEVICE_PATH_MESSAGING_PC_ANSI \
  }

ACPI_HID_DEVICE_PATH  gPnpPs2KeyboardDeviceNode  = gPnpPs2Keyboard;
ACPI_HID_DEVICE_PATH  gPnp16550ComPortDeviceNode = gPnp16550ComPort;
VENDOR_DEVICE_PATH    gTerminalTypeDeviceNode    = gPcAnsiTerminal;

BOOLEAN  mDetectDisplayOnly;

/**
  Add IsaKeyboard to ConIn.

  @param[in] DeviceHandle  Handle of the LPC Bridge device.

  @retval EFI_SUCCESS  IsaKeyboard on the LPC bridge have been added to ConIn.
  @return              Error codes, due to EFI_DEVICE_PATH_PROTOCOL missing
                       from DeviceHandle.
**/
EFI_STATUS
PrepareLpcBridgeDevicePath (
  IN EFI_HANDLE  DeviceHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath = NULL;
  Status     = gBS->HandleProtocol (
                      DeviceHandle,
                      &gEfiDevicePathProtocolGuid,
                      (VOID *)&DevicePath
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Register Keyboard
  //
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gPnpPs2KeyboardDeviceNode);
  EfiBootManagerUpdateConsoleVariable (ConIn, DevicePath, NULL);
  return EFI_SUCCESS;
}

/**
  Return the GOP device path in the platform.

  @param[in]   PciDevicePath - Device path for the PCI graphics device.
  @param[out]  GopDevicePath - Return the device path with GOP installed.

  @retval EFI_SUCCESS  - PCI VGA is added to ConOut.
  @retval EFI_INVALID_PARAMETER   - The device path parameter is invalid.
  @retval EFI_STATUS   - No GOP device found.
**/
EFI_STATUS
GetGopDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *PciDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL  **GopDevicePath
  )
{
  UINTN                     Index;
  EFI_STATUS                Status;
  EFI_HANDLE                PciDeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempPciDevicePath;
  UINTN                     GopHandleCount;
  EFI_HANDLE                *GopHandleBuffer;

  if ((PciDevicePath == NULL) || (GopDevicePath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Initialize the GopDevicePath to be PciDevicePath
  //
  *GopDevicePath    = PciDevicePath;
  TempPciDevicePath = PciDevicePath;

  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &TempPciDevicePath,
                  &PciDeviceHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->ConnectController (PciDeviceHandle, NULL, NULL, FALSE);

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &GopHandleCount,
                  &GopHandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Add all the child handles as possible Console Device
    //
    for (Index = 0; Index < GopHandleCount; Index++) {
      Status = gBS->HandleProtocol (GopHandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID *)&TempDevicePath);
      if (EFI_ERROR (Status)) {
        continue;
      }

      if (CompareMem (
            PciDevicePath,
            TempDevicePath,
            GetDevicePathSize (PciDevicePath) - END_DEVICE_PATH_LENGTH
            ) == 0)
      {
        //
        // In current implementation, we only enable one of the child handles
        // as console device, i.e. sotre one of the child handle's device
        // path to variable "ConOut"
        // In future, we could select all child handles to be console device
        //
        *GopDevicePath = TempDevicePath;

        //
        // Delete the PCI device's path that added by GetPlugInPciVgaDevicePath()
        // Add the integrity GOP device path.
        //
        EfiBootManagerUpdateConsoleVariable (ConOut, NULL, PciDevicePath);
        EfiBootManagerUpdateConsoleVariable (ConOut, TempDevicePath, NULL);
      }
    }

    gBS->FreePool (GopHandleBuffer);
  }

  return EFI_SUCCESS;
}

/**
  Add PCI VGA to ConOut, ConIn, ErrOut.

  @param[in]  DeviceHandle - Handle of PciIo protocol.

  @retval EFI_SUCCESS  - PCI VGA is added to ConOut.
  @retval EFI_STATUS   - No PCI VGA device is added.

**/
EFI_STATUS
PreparePciVgaDevicePath (
  IN EFI_HANDLE  DeviceHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *GopDevicePath;

  DevicePath = NULL;
  Status     = gBS->HandleProtocol (
                      DeviceHandle,
                      &gEfiDevicePathProtocolGuid,
                      (VOID *)&DevicePath
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  GetGopDevicePath (DevicePath, &GopDevicePath);
  DevicePath = GopDevicePath;

  EfiBootManagerUpdateConsoleVariable (ConOut, DevicePath, NULL);

  return EFI_SUCCESS;
}

/**
  For every PCI instance execute a callback function.

  @param[in]  Id                 - The protocol GUID for callback
  @param[in]  CallBackFunction   - The callback function

  @retval EFI_STATUS - Callback function failed.

**/
EFI_STATUS
EFIAPI
VisitAllInstancesOfProtocol (
  IN EFI_GUID                           *Id,
  IN SIMPLE_PROTOCOL_INSTANCE_CALLBACK  CallBackFunction
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;
  VOID        *Instance;

  //
  // Start to check all the PciIo to find all possible device
  //
  HandleCount  = 0;
  HandleBuffer = NULL;
  Status       = gBS->LocateHandleBuffer (
                        ByProtocol,
                        Id,
                        NULL,
                        &HandleCount,
                        &HandleBuffer
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], Id, &Instance);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = (*CallBackFunction)(
  HandleBuffer[Index],
  Instance
  );
  }

  gBS->FreePool (HandleBuffer);

  return EFI_SUCCESS;
}

/**
  Do platform specific PCI Device check and add them to
  ConOut, ConIn, ErrOut.

  @param[in]  Handle    - Handle of PCI device instance
  @param[in]  Instance  - The instance of PCI device

  @retval EFI_SUCCESS - PCI Device check and Console variable update successfully.
  @retval EFI_STATUS - PCI Device check or Console variable update fail.

**/
EFI_STATUS
EFIAPI
DetectAndPreparePlatformPciDevicePath (
  IN EFI_HANDLE  Handle,
  IN VOID        *Instance
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           Pci;

  PciIo = (EFI_PCI_IO_PROTOCOL *)Instance;

  //
  // Check for all PCI device
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (Pci) / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationEnable,
                    EFI_PCI_DEVICE_ENABLE,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);

  if (!mDetectDisplayOnly) {
    //
    // Here we decide whether it is LPC Bridge
    //
    if ((IS_PCI_LPC (&Pci)) ||
        ((IS_PCI_ISA_PDECODE (&Pci)) &&
         (Pci.Hdr.VendorId == 0x8086)
        )
        )
    {
      //
      // Add IsaKeyboard to ConIn,
      // add IsaSerial to ConOut, ConIn, ErrOut
      //
      DEBUG ((DEBUG_INFO, "Found LPC Bridge device\n"));
      PrepareLpcBridgeDevicePath (Handle);
      return EFI_SUCCESS;
    }
  }

  //
  // Enable all display devices
  //
  if (IS_PCI_DISPLAY (&Pci)) {
    //
    // Add them to ConOut.
    //
    DEBUG ((DEBUG_INFO, "Found PCI Display device\n"));
    EfiBootManagerConnectVideoController (Handle);
    return EFI_SUCCESS;
  }

  return Status;
}

/**
  For every Serial Io instance, add it to ConOut, ConIn, ErrOut.

  @param[in]  Handle     - The Serial Io device handle
  @param[in]  Instance   - The instance of the SerialIo protocol

  @retval EFI_STATUS - Callback function failed.

**/
EFI_STATUS
EFIAPI
AddDevicePathForOneSerialIoInstance (
  IN EFI_HANDLE  Handle,
  IN VOID        *Instance
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath = NULL;
  Status     = gBS->HandleProtocol (
                      Handle,
                      &gEfiDevicePathProtocolGuid,
                      (VOID *)&DevicePath
                      );
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gTerminalTypeDeviceNode);

  EfiBootManagerUpdateConsoleVariable (ConOut, DevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ConIn, DevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ErrOut, DevicePath, NULL);
  return Status;
}

/**
  Do platform specific PCI Device check and add them to ConOut, ConIn, ErrOut

  @param[in]  DetectDisplayOnly - Only detect display device if it's TRUE.

  @retval EFI_SUCCESS - PCI Device check and Console variable update successfully.
  @retval EFI_STATUS - PCI Device check or Console variable update fail.

**/
EFI_STATUS
DetectAndPreparePlatformPciDevicePaths (
  BOOLEAN  DetectDisplayOnly
  )
{
  EFI_STATUS  Status;

  mDetectDisplayOnly = DetectDisplayOnly;

  EfiBootManagerUpdateConsoleVariable (
    ConIn,
    (EFI_DEVICE_PATH_PROTOCOL *)&gUsbClassKeyboardDevicePath,
    NULL
    );

  VisitAllInstancesOfProtocol (
    &gEfiSerialIoProtocolGuid,
    AddDevicePathForOneSerialIoInstance
    );

  Status = VisitAllInstancesOfProtocol (
             &gEfiPciIoProtocolGuid,
             DetectAndPreparePlatformPciDevicePath
             );
  return Status;
}

/**
  The function will connect one root bridge

  @param[in]  Handle     - The root bridge handle
  @param[in]  Instance   - The instance of the root bridge

  @return EFI_SUCCESS      Connect RootBridge successfully.

**/
EFI_STATUS
EFIAPI
ConnectOneRootBridge (
  IN EFI_HANDLE  Handle,
  IN VOID        *Instance
  )
{
  EFI_STATUS  Status;

  Status = gBS->ConnectController (Handle, NULL, NULL, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Platform console init. Include the platform firmware vendor, revision
  and so crc check.

**/
VOID
EFIAPI
PlatformConsoleInit (
  VOID
  )
{
  VisitAllInstancesOfProtocol (
    &gEfiPciRootBridgeIoProtocolGuid,
    ConnectOneRootBridge
    );

  //
  // Do platform specific PCI Device check and add them to ConOut, ConIn, ErrOut
  //
  DetectAndPreparePlatformPciDevicePaths (FALSE);
}

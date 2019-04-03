/** @file
  ISA ACPI Protocol Implementation

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "PcatIsaAcpi.h"

//
// Platform specific data for the ISA devices that are present.in the platform
//

//
// COM 1 UART Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE mPcatIsaAcpiCom1DeviceResources[] = {
  {EfiIsaAcpiResourceIo,        0, 0x3f8, 0x3ff},
  {EfiIsaAcpiResourceInterrupt, 0, 4,     0},
  {EfiIsaAcpiResourceEndOfList, 0, 0,     0}
};

//
// COM 2 UART Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE mPcatIsaAcpiCom2DeviceResources[] = {
  {EfiIsaAcpiResourceIo,        0, 0x2f8, 0x2ff},
  {EfiIsaAcpiResourceInterrupt, 0, 3,     0},
  {EfiIsaAcpiResourceEndOfList, 0, 0,     0}
};

//
// PS/2 Keyboard Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE  mPcatIsaAcpiPs2KeyboardDeviceResources[] = {
  {EfiIsaAcpiResourceIo,        0, 0x60, 0x64},
  {EfiIsaAcpiResourceInterrupt, 0, 1,     0},
  {EfiIsaAcpiResourceEndOfList, 0, 0,     0}
};

//
// PS/2 Mouse Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE  mPcatIsaAcpiPs2MouseDeviceResources[] = {
  {EfiIsaAcpiResourceIo,        0, 0x60, 0x64},
  {EfiIsaAcpiResourceInterrupt, 0, 12,     0},
  {EfiIsaAcpiResourceEndOfList, 0, 0,     0}
};

//
// Floppy Disk Controller
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE mPcatIsaAcpiFloppyResources[] = {
  {EfiIsaAcpiResourceIo,        0, 0x3f0, 0x3f7},
  {EfiIsaAcpiResourceInterrupt, 0, 6,     0},
  {EfiIsaAcpiResourceDma,       EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_COMPATIBLE | EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_WIDTH_8 | EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SINGLE_MODE, 2,     0},
  {EfiIsaAcpiResourceEndOfList, 0, 0,     0}
};

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE_LIST mPcatIsaAcpiCom1Device = {
  {EISA_PNP_ID(0x501), 0}, mPcatIsaAcpiCom1DeviceResources
}; // COM 1 UART Controller

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE_LIST mPcatIsaAcpiCom2Device = {
  {EISA_PNP_ID(0x501), 1}, mPcatIsaAcpiCom2DeviceResources
}; // COM 2 UART Controller

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE_LIST mPcatIsaAcpiPs2KeyboardDevice = {
  {EISA_PNP_ID(0x303), 0}, mPcatIsaAcpiPs2KeyboardDeviceResources
}; // PS/2 Keyboard Controller

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE_LIST mPcatIsaAcpiPs2MouseDevice = {
  {EISA_PNP_ID(0x303), 1}, mPcatIsaAcpiPs2MouseDeviceResources
}; // PS/2 Mouse Controller

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE_LIST mPcatIsaAcpiFloppyADevice = {
  {EISA_PNP_ID(0x604), 0}, mPcatIsaAcpiFloppyResources
}; // Floppy Disk Controller A:

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_ISA_ACPI_RESOURCE_LIST mPcatIsaAcpiFloppyBDevice = {
  {EISA_PNP_ID(0x604), 1}, mPcatIsaAcpiFloppyResources
}; // Floppy Disk Controller B:

//
// Table of ISA Controllers
//
EFI_ISA_ACPI_RESOURCE_LIST gPcatIsaAcpiDeviceList[7] = {{{0, 0}, NULL}};

/**
  Initialize gPcatIsaAcpiDeviceList.
**/
VOID
InitializePcatIsaAcpiDeviceList (
  VOID
  )
{
  UINTN  Index;

  Index = 0;
  if (PcdGetBool (PcdIsaAcpiCom1Enable)) {
    CopyMem (&gPcatIsaAcpiDeviceList[Index], &mPcatIsaAcpiCom1Device, sizeof(mPcatIsaAcpiCom1Device));
    Index++;
  }
  if (PcdGetBool (PcdIsaAcpiCom2Enable)) {
    CopyMem (&gPcatIsaAcpiDeviceList[Index], &mPcatIsaAcpiCom2Device, sizeof(mPcatIsaAcpiCom2Device));
    Index++;
  }
  if (PcdGetBool (PcdIsaAcpiPs2KeyboardEnable)) {
    CopyMem (&gPcatIsaAcpiDeviceList[Index], &mPcatIsaAcpiPs2KeyboardDevice, sizeof(mPcatIsaAcpiPs2KeyboardDevice));
    Index++;
  }
  if (PcdGetBool (PcdIsaAcpiPs2MouseEnable)) {
    CopyMem (&gPcatIsaAcpiDeviceList[Index], &mPcatIsaAcpiPs2MouseDevice, sizeof(mPcatIsaAcpiPs2MouseDevice));
    Index++;
  }
  if (PcdGetBool (PcdIsaAcpiFloppyAEnable)) {
    CopyMem (&gPcatIsaAcpiDeviceList[Index], &mPcatIsaAcpiFloppyADevice, sizeof(mPcatIsaAcpiFloppyADevice));
    Index++;
  }
  if (PcdGetBool (PcdIsaAcpiFloppyBEnable)) {
    CopyMem (&gPcatIsaAcpiDeviceList[Index], &mPcatIsaAcpiFloppyBDevice, sizeof(mPcatIsaAcpiFloppyBDevice));
    Index++;
  }
}

//
// ISA ACPI Protocol Functions
//
/**
  Enumerate the ISA devices on the ISA bus.

  @param Device             Point to device ID instance
  @param IsaAcpiDevice      On return, point to resource data for Isa device
  @param NextIsaAcpiDevice  On return, point to resource data for next Isa device
**/
VOID
IsaDeviceLookup (
  IN  EFI_ISA_ACPI_DEVICE_ID      *Device,
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **IsaAcpiDevice,
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **NextIsaAcpiDevice
  )
{
  UINTN  Index;

  *IsaAcpiDevice = NULL;
  if (NextIsaAcpiDevice != NULL) {
    *NextIsaAcpiDevice = NULL;
  }
  if (Device == NULL) {
    Index = 0;
  } else {
    for(Index = 0; gPcatIsaAcpiDeviceList[Index].ResourceItem != NULL; Index++) {
      if (Device->HID == gPcatIsaAcpiDeviceList[Index].Device.HID &&
          Device->UID == gPcatIsaAcpiDeviceList[Index].Device.UID    ) {
        break;
      }
    }
    if (gPcatIsaAcpiDeviceList[Index].ResourceItem == NULL) {
      return;
    }
    *IsaAcpiDevice = &(gPcatIsaAcpiDeviceList[Index]);
    Index++;
  }
  if (gPcatIsaAcpiDeviceList[Index].ResourceItem != NULL && NextIsaAcpiDevice != NULL) {
    *NextIsaAcpiDevice = &(gPcatIsaAcpiDeviceList[Index]);
  }
}

/**
  Enumerate the ISA devices on the ISA bus


  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance

  @retval EFI_NOT_FOUND  Can not found the next Isa device.
  @retval EFI_SUCCESS    Success retrieve the next Isa device for enumration.

**/
EFI_STATUS
EFIAPI
IsaDeviceEnumerate (
  IN  EFI_ISA_ACPI_PROTOCOL   *This,
  OUT EFI_ISA_ACPI_DEVICE_ID  **Device
  )
{
  EFI_ISA_ACPI_RESOURCE_LIST  *IsaAcpiDevice;
  EFI_ISA_ACPI_RESOURCE_LIST  *NextIsaAcpiDevice;

  IsaDeviceLookup (*Device, &IsaAcpiDevice, &NextIsaAcpiDevice);
  if (NextIsaAcpiDevice == NULL) {
    return EFI_NOT_FOUND;
  }
  *Device = &(NextIsaAcpiDevice->Device);
  return EFI_SUCCESS;
}

/**
  Set ISA device power


  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance
  @param OnOff           TRUE for setting isa device power on,
                         FALSE for setting isa device power off

  @return EFI_SUCCESS    Success to change power status for isa device.
**/
EFI_STATUS
EFIAPI
IsaDeviceSetPower (
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device,
  IN BOOLEAN                 OnOff
  )
{
  return EFI_SUCCESS;
}

/**
  Get current resource for the specific ISA device.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance
  @param ResourceList    On return, point to resources instances for given isa device

  @retval EFI_NOT_FOUND Can not found the resource instance for given isa device
  @retval EFI_SUCCESS   Success to get resource instance for given isa device.
**/
EFI_STATUS
EFIAPI
IsaGetCurrentResource (
  IN  EFI_ISA_ACPI_PROTOCOL       *This,
  IN  EFI_ISA_ACPI_DEVICE_ID      *Device,
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **ResourceList
  )
{
  IsaDeviceLookup (Device, ResourceList, NULL);
  if (*ResourceList == NULL) {
    return EFI_NOT_FOUND;
  }
  return EFI_SUCCESS;
}

/**
  Get possible resource for the specific ISA device.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance
  @param ResourceList    On return, point to resources instances for given isa device

  @retval EFI_SUCCESS   Success to get resource instance for given isa device.
**/
EFI_STATUS
EFIAPI
IsaGetPossibleResource (
  IN  EFI_ISA_ACPI_PROTOCOL       *This,
  IN  EFI_ISA_ACPI_DEVICE_ID      *Device,
  OUT EFI_ISA_ACPI_RESOURCE_LIST  **ResourceList
  )
{
  return EFI_SUCCESS;
}

/**
  Set resource for the specific ISA device.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance
  @param ResourceList    Point to resources instances for given isa device

  @return EFI_SUCCESS  Success to set resource.

**/
EFI_STATUS
EFIAPI
IsaSetResource (
  IN EFI_ISA_ACPI_PROTOCOL       *This,
  IN EFI_ISA_ACPI_DEVICE_ID      *Device,
  IN EFI_ISA_ACPI_RESOURCE_LIST  *ResourceList
  )
{
  return EFI_SUCCESS;
}

/**
  Enable/Disable the specific ISA device.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance
  @param Enable          Enable/Disable

  @return EFI_SUCCESS  Success to enable/disable.

**/
EFI_STATUS
EFIAPI
IsaEnableDevice (
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device,
  IN BOOLEAN                 Enable
  )
{
  return EFI_SUCCESS;
}

/**
  Initialize the specific ISA device.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL
  @param Device          Point to device ID instance

  @return EFI_SUCCESS  Success to initialize.

**/
EFI_STATUS
EFIAPI
IsaInitDevice (
  IN EFI_ISA_ACPI_PROTOCOL   *This,
  IN EFI_ISA_ACPI_DEVICE_ID  *Device
  )
{
  return EFI_SUCCESS;
}


/**
  Initialize the ISA interface.

  @param This            Point to instance of EFI_ISA_ACPI_PROTOCOL

  @return EFI_SUCCESS  Success to initialize ISA interface.

**/
EFI_STATUS
EFIAPI
IsaInterfaceInit (
  IN EFI_ISA_ACPI_PROTOCOL  *This
)
{
  return EFI_SUCCESS;
}

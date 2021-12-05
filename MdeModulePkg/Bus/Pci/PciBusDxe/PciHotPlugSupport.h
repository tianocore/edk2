/** @file
  PCI Hot Plug support functions declaration for PCI Bus module.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_PCI_HOT_PLUG_SUPPORT_H_
#define _EFI_PCI_HOT_PLUG_SUPPORT_H_

//
// stall 1 second, its unit is 100ns
//
#define STALL_1_SECOND  1000000

//
// PCI Hot Plug controller private data
//
typedef struct {
  EFI_EVENT    Event;
  BOOLEAN      Found;
  BOOLEAN      Initialized;
  VOID         *Padding;
} ROOT_HPC_DATA;

//
// Reference of some global variables
//
extern EFI_PCI_HOT_PLUG_INIT_PROTOCOL  *gPciHotPlugInit;
extern EFI_HPC_LOCATION                *gPciRootHpcPool;
extern ROOT_HPC_DATA                   *gPciRootHpcData;

/**
  Event notification function to set Hot Plug controller status.

  @param  Event                    The event that invoke this function.
  @param  Context                  The calling context, pointer to ROOT_HPC_DATA.

**/
VOID
EFIAPI
PciHPCInitialized (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Compare two device paths to check if they are exactly same.

  @param DevicePath1    A pointer to the first device path data structure.
  @param DevicePath2    A pointer to the second device path data structure.

  @retval TRUE    They are same.
  @retval FALSE   They are not same.

**/
BOOLEAN
EfiCompareDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath1,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath2
  );

/**
  Check hot plug support and initialize root hot plug private data.

  If Hot Plug is supported by the platform, call PCI Hot Plug Init protocol
  to get PCI Hot Plug controller's information and constructor the root hot plug
  private data structure.

  @retval EFI_SUCCESS           They are same.
  @retval EFI_UNSUPPORTED       No PCI Hot Plug controller on the platform.
  @retval EFI_OUT_OF_RESOURCES  No memory to constructor root hot plug private
                                data structure.

**/
EFI_STATUS
InitializeHotPlugSupport (
  VOID
  );

/**
  Test whether PCI device is hot plug bus.

  @param PciIoDevice  PCI device instance.

  @retval TRUE    PCI device is a hot plug bus.
  @retval FALSE   PCI device is not a hot plug bus.

**/
BOOLEAN
IsPciHotPlugBus (
  PCI_IO_DEVICE  *PciIoDevice
  );

/**
  Test whether device path is for root pci hot plug bus.

  @param HpbDevicePath  A pointer to device path data structure to be tested.
  @param HpIndex        If HpIndex is not NULL, return the index of root hot
                        plug in global array when TRUE is returned.

  @retval TRUE          The device path is for root pci hot plug bus.
  @retval FALSE         The device path is not for root pci hot plug bus.

**/
BOOLEAN
IsRootPciHotPlugBus (
  IN  EFI_DEVICE_PATH_PROTOCOL  *HpbDevicePath,
  OUT UINTN                     *HpIndex    OPTIONAL
  );

/**
  Test whether device path is for root pci hot plug controller.

  @param HpcDevicePath  A pointer to device path data structure to be tested.
  @param HpIndex        If HpIndex is not NULL, return the index of root hot
                        plug in global array when TRUE is returned.

  @retval TRUE          The device path is for root pci hot plug controller.
  @retval FALSE         The device path is not for root pci hot plug controller.

**/
BOOLEAN
IsRootPciHotPlugController (
  IN EFI_DEVICE_PATH_PROTOCOL  *HpcDevicePath,
  OUT UINTN                    *HpIndex
  );

/**
  Creating event object for PCI Hot Plug controller.

  @param  HpIndex   Index of hot plug device in global array.
  @param  Event     The returned event that invoke this function.

  @return Status of create event.

**/
EFI_STATUS
CreateEventForHpc (
  IN  UINTN      HpIndex,
  OUT EFI_EVENT  *Event
  );

/**
  Wait for all root PCI Hot Plug controller finished initializing.

  @param TimeoutInMicroSeconds  Microseconds to wait for all root HPCs' initialization.

  @retval EFI_SUCCESS           All HPCs initialization finished.
  @retval EFI_TIMEOUT           Not ALL HPCs initialization finished in Microseconds.

**/
EFI_STATUS
AllRootHPCInitialized (
  IN  UINTN  TimeoutInMicroSeconds
  );

/**
  Check whether PCI-PCI bridge has PCI Hot Plug capability register block.

  @param PciIoDevice    A Pointer to the PCI-PCI bridge.

  @retval TRUE    PCI device is HPC.
  @retval FALSE   PCI device is not HPC.

**/
BOOLEAN
IsSHPC (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

/**
  Check whether PciIoDevice supports PCIe hotplug.

  This is equivalent to the following condition:
  - the device is either a PCIe switch downstream port or a root port,
  - and the device has the SlotImplemented bit set in its PCIe capability
    register,
  - and the device has the HotPlugCapable bit set in its slot capabilities
    register.

  @param[in] PciIoDevice  The device being checked.

  @retval TRUE   PciIoDevice is a PCIe port that accepts a hot-plugged device.
  @retval FALSE  Otherwise.

**/
BOOLEAN
SupportsPcieHotplug (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

/**
  Get resource padding if the specified PCI bridge is a hot plug bus.

  @param PciIoDevice    PCI bridge instance.

**/
VOID
GetResourcePaddingForHpb (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

#endif

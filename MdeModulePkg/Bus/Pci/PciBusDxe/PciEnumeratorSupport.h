/** @file
  PCI enumeration support functions declaration for PCI Bus module.

Copyright (c) 2006 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_PCI_ENUMERATOR_SUPPORT_H_
#define _EFI_PCI_ENUMERATOR_SUPPORT_H_

/**
  This routine is used to check whether the pci device is present.

  @param PciRootBridgeIo   Pointer to instance of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param Pci               Output buffer for PCI device configuration space.
  @param Bus               PCI bus NO.
  @param Device            PCI device NO.
  @param Func              PCI Func NO.

  @retval EFI_NOT_FOUND    PCI device not present.
  @retval EFI_SUCCESS      PCI device is found.

**/
EFI_STATUS
PciDevicePresent (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  OUT PCI_TYPE00                       *Pci,
  IN  UINT8                            Bus,
  IN  UINT8                            Device,
  IN  UINT8                            Func
  );

/**
  Collect all the resource information under this root bridge.

  A database that records all the information about pci device subject to this
  root bridge will then be created.

  @param Bridge         Parent bridge instance.
  @param StartBusNumber Bus number of beginning.

  @retval EFI_SUCCESS   PCI device is found.
  @retval other         Some error occurred when reading PCI bridge information.

**/
EFI_STATUS
PciPciDeviceInfoCollector (
  IN PCI_IO_DEVICE  *Bridge,
  IN UINT8          StartBusNumber
  );

/**
  Search required device and create PCI device instance.

  @param Bridge     Parent bridge instance.
  @param Pci        Input PCI device information block.
  @param Bus        PCI bus NO.
  @param Device     PCI device NO.
  @param Func       PCI func  NO.
  @param PciDevice  Output of searched PCI device instance.

  @retval EFI_SUCCESS           Successfully created PCI device instance.
  @retval EFI_OUT_OF_RESOURCES  Cannot get PCI device information.

**/
EFI_STATUS
PciSearchDevice (
  IN  PCI_IO_DEVICE  *Bridge,
  IN  PCI_TYPE00     *Pci,
  IN  UINT8          Bus,
  IN  UINT8          Device,
  IN  UINT8          Func,
  OUT PCI_IO_DEVICE  **PciDevice
  );

/**
  Create PCI device instance for PCI device.

  @param Bridge   Parent bridge instance.
  @param Pci      Input PCI device information block.
  @param Bus      PCI device Bus NO.
  @param Device   PCI device Device NO.
  @param Func     PCI device's func NO.

  @return  Created PCI device instance.

**/
PCI_IO_DEVICE *
GatherDeviceInfo (
  IN PCI_IO_DEVICE  *Bridge,
  IN PCI_TYPE00     *Pci,
  IN UINT8          Bus,
  IN UINT8          Device,
  IN UINT8          Func
  );

/**
  Create PCI device instance for PCI-PCI bridge.

  @param Bridge   Parent bridge instance.
  @param Pci      Input PCI device information block.
  @param Bus      PCI device Bus NO.
  @param Device   PCI device Device NO.
  @param Func     PCI device's func NO.

  @return  Created PCI device instance.

**/
PCI_IO_DEVICE *
GatherPpbInfo (
  IN PCI_IO_DEVICE  *Bridge,
  IN PCI_TYPE00     *Pci,
  IN UINT8          Bus,
  IN UINT8          Device,
  IN UINT8          Func
  );

/**
  Create PCI device instance for PCI Card bridge device.

  @param Bridge   Parent bridge instance.
  @param Pci      Input PCI device information block.
  @param Bus      PCI device Bus NO.
  @param Device   PCI device Device NO.
  @param Func     PCI device's func NO.

  @return  Created PCI device instance.

**/
PCI_IO_DEVICE *
GatherP2CInfo (
  IN PCI_IO_DEVICE  *Bridge,
  IN PCI_TYPE00     *Pci,
  IN UINT8          Bus,
  IN UINT8          Device,
  IN UINT8          Func
  );

/**
  Create device path for pci device.

  @param ParentDevicePath  Parent bridge's path.
  @param PciIoDevice       Pci device instance.

  @return Device path protocol instance for specific pci device.

**/
EFI_DEVICE_PATH_PROTOCOL *
CreatePciDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath,
  IN  PCI_IO_DEVICE             *PciIoDevice
  );

/**
  Check whether the PCI IOV VF bar is existed or not.

  @param PciIoDevice       A pointer to the PCI_IO_DEVICE.
  @param Offset            The offset.
  @param BarLengthValue    The bar length value returned.
  @param OriginalBarValue  The original bar value returned.

  @retval EFI_NOT_FOUND    The bar doesn't exist.
  @retval EFI_SUCCESS      The bar exist.

**/
EFI_STATUS
VfBarExisted (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINTN          Offset,
  OUT UINT32        *BarLengthValue,
  OUT UINT32        *OriginalBarValue
  );

/**
  Check whether the bar is existed or not.

  @param PciIoDevice       A pointer to the PCI_IO_DEVICE.
  @param Offset            The offset.
  @param BarLengthValue    The bar length value returned.
  @param OriginalBarValue  The original bar value returned.

  @retval EFI_NOT_FOUND    The bar doesn't exist.
  @retval EFI_SUCCESS      The bar exist.

**/
EFI_STATUS
BarExisted (
  IN  PCI_IO_DEVICE  *PciIoDevice,
  IN  UINTN          Offset,
  OUT UINT32         *BarLengthValue,
  OUT UINT32         *OriginalBarValue
  );

/**
  Test whether the device can support given attributes.

  @param PciIoDevice      Pci device instance.
  @param Command          Input command register value, and
                          returned supported register value.
  @param BridgeControl    Input bridge control value for PPB or P2C, and
                          returned supported bridge control value.
  @param OldCommand       Returned and stored old command register offset.
  @param OldBridgeControl Returned and stored old Bridge control value for PPB or P2C.

**/
VOID
PciTestSupportedAttribute (
  IN     PCI_IO_DEVICE  *PciIoDevice,
  IN OUT UINT16         *Command,
  IN OUT UINT16         *BridgeControl,
  OUT UINT16            *OldCommand,
  OUT UINT16            *OldBridgeControl
  );

/**
  Set the supported or current attributes of a PCI device.

  @param PciIoDevice    Structure pointer for PCI device.
  @param Command        Command register value.
  @param BridgeControl  Bridge control value for PPB or P2C.
  @param Option         Make a choice of EFI_SET_SUPPORTS or EFI_SET_ATTRIBUTES.

**/
VOID
PciSetDeviceAttribute (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINT16         Command,
  IN UINT16         BridgeControl,
  IN UINTN          Option
  );

/**
  Determine if the device can support Fast Back to Back attribute.

  @param PciIoDevice  Pci device instance.
  @param StatusIndex  Status register value.

  @retval EFI_SUCCESS       This device support Fast Back to Back attribute.
  @retval EFI_UNSUPPORTED   This device doesn't support Fast Back to Back attribute.

**/
EFI_STATUS
GetFastBackToBackSupport (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINT8          StatusIndex
  );

/**
  Determine the related attributes of all devices under a Root Bridge.

  @param PciIoDevice   PCI device instance.

**/
EFI_STATUS
DetermineDeviceAttribute (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

/**
  This routine is used to update the bar information for those incompatible PCI device.

  @param PciIoDevice      Input Pci device instance. Output Pci device instance with updated
                          Bar information.

  @retval EFI_SUCCESS     Successfully updated bar information.
  @retval EFI_UNSUPPORTED Given PCI device doesn't belong to incompatible PCI device list.

**/
EFI_STATUS
UpdatePciInfo (
  IN OUT PCI_IO_DEVICE  *PciIoDevice
  );

/**
  This routine will update the alignment with the new alignment.

  @param Alignment    Input Old alignment. Output updated alignment.
  @param NewAlignment New alignment.

**/
VOID
SetNewAlign (
  IN OUT UINT64  *Alignment,
  IN     UINT64  NewAlignment
  );

/**
  Parse PCI bar information and fill them into PCI device instance.

  @param PciIoDevice  Pci device instance.
  @param Offset       Bar offset.
  @param BarIndex     Bar index.

  @return Next bar offset.

**/
UINTN
PciParseBar (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINTN          Offset,
  IN UINTN          BarIndex
  );

/**
  Parse PCI IOV VF bar information and fill them into PCI device instance.

  @param PciIoDevice  Pci device instance.
  @param Offset       Bar offset.
  @param BarIndex     Bar index.

  @return Next bar offset.

**/
UINTN
PciIovParseVfBar (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINTN          Offset,
  IN UINTN          BarIndex
  );

/**
  This routine is used to initialize the bar of a PCI device.

  @param PciIoDevice Pci device instance.

  @note It can be called typically when a device is going to be rejected.

**/
VOID
InitializePciDevice (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

/**
  This routine is used to initialize the bar of a PCI-PCI Bridge device.

  @param  PciIoDevice PCI-PCI bridge device instance.

**/
VOID
InitializePpb (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

/**
  This routine is used to initialize the bar of a PCI Card Bridge device.

  @param PciIoDevice  PCI Card bridge device.

**/
VOID
InitializeP2C (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

/**
  Create and initialize general PCI I/O device instance for
  PCI device/bridge device/hotplug bridge device.

  @param Bridge            Parent bridge instance.
  @param Pci               Input Pci information block.
  @param Bus               Device Bus NO.
  @param Device            Device device NO.
  @param Func              Device func NO.

  @return Instance of PCI device. NULL means no instance created.

**/
PCI_IO_DEVICE *
CreatePciIoDevice (
  IN PCI_IO_DEVICE  *Bridge,
  IN PCI_TYPE00     *Pci,
  IN UINT8          Bus,
  IN UINT8          Device,
  IN UINT8          Func
  );

/**
  This routine is used to enumerate entire pci bus system
  in a given platform.

  It is only called on the second start on the same Root Bridge.

  @param  Controller     Parent bridge handler.

  @retval EFI_SUCCESS    PCI enumeration finished successfully.
  @retval other          Some error occurred when enumerating the pci bus system.

**/
EFI_STATUS
PciEnumeratorLight (
  IN EFI_HANDLE  Controller
  );

/**
  Get bus range from PCI resource descriptor list.

  @param Descriptors  A pointer to the address space descriptor.
  @param MinBus       The min bus returned.
  @param MaxBus       The max bus returned.
  @param BusRange     The bus range returned.

  @retval EFI_SUCCESS    Successfully got bus range.
  @retval EFI_NOT_FOUND  Can not find the specific bus.

**/
EFI_STATUS
PciGetBusRange (
  IN     EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  **Descriptors,
  OUT    UINT16                             *MinBus,
  OUT    UINT16                             *MaxBus,
  OUT    UINT16                             *BusRange
  );

/**
  This routine can be used to start the root bridge.

  @param RootBridgeDev     Pci device instance.

  @retval EFI_SUCCESS      This device started.
  @retval other            Failed to get PCI Root Bridge I/O protocol.

**/
EFI_STATUS
StartManagingRootBridge (
  IN PCI_IO_DEVICE  *RootBridgeDev
  );

/**
  This routine can be used to check whether a PCI device should be rejected when light enumeration.

  @param PciIoDevice  Pci device instance.

  @retval TRUE      This device should be rejected.
  @retval FALSE     This device shouldn't be rejected.

**/
BOOLEAN
IsPciDeviceRejected (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

/**
  Reset all bus number from specific bridge.

  @param Bridge           Parent specific bridge.
  @param StartBusNumber   Start bus number.

**/
VOID
ResetAllPpbBusNumber (
  IN PCI_IO_DEVICE  *Bridge,
  IN UINT8          StartBusNumber
  );

/**
  Dump the PPB padding resource information.

  @param PciIoDevice     PCI IO instance.
  @param ResourceType    The desired resource type to dump.
                         PciBarTypeUnknown means to dump all types of resources.
**/
VOID
DumpPpbPaddingResource (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN PCI_BAR_TYPE   ResourceType
  );

/**
  Dump the PCI BAR information.

  @param PciIoDevice     PCI IO instance.
**/
VOID
DumpPciBars (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

#endif

/**@file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#ifndef _EFI_PCI_ENUMERATOR_SUPPORT_H
#define _EFI_PCI_ENUMERATOR_SUPPORT_H

/**
  This routine is used to check whether the pci device is present.
  
  @param PciRootBridgeIo   Pointer to instance of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
  @param Pci               Output buffer for PCI device structure
  @param Bus               PCI bus NO
  @param Device            PCI device NO
  @param Func              PCI Func NO
  
  @retval EFI_NOT_FOUND device not present
  @retval EFI_SUCCESS   device is found.
**/
EFI_STATUS
PciDevicePresent (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  PCI_TYPE00                          *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  );

/**
  Collect all the resource information under this root bridge
  A database that records all the information about pci device subject to this
  root bridge will then be created.
    
  @param Bridge         Parent bridge instance
  @param StartBusNumer  Bus number of begining 
**/
EFI_STATUS
PciPciDeviceInfoCollector (
  IN PCI_IO_DEVICE                      *Bridge,
  UINT8                                 StartBusNumber
  );

/**
  Seach required device and get PCI device info block
  
  @param Bridge     Parent bridge instance
  @param Pci        Output of PCI device info block
  @param Bus        PCI bus NO.
  @param Device     PCI device NO.
  @param Func       PCI func  NO.
  @param PciDevice  output of searched PCI device instance
**/
EFI_STATUS
PciSearchDevice (
  IN PCI_IO_DEVICE                      *Bridge,
  PCI_TYPE00                            *Pci,
  UINT8                                 Bus,
  UINT8                                 Device,
  UINT8                                 Func,
  PCI_IO_DEVICE                         **PciDevice
  );

/**
  Create PCI private data for PCI device
  
  @param Bridge Parent bridge instance
  @param Pci    PCI bar block
  @param Bus    PCI device Bus NO.
  @param Device PCI device DeviceNO.
  @param Func   PCI device's func NO.
  
  @return new PCI device's private date structure.
**/
PCI_IO_DEVICE             *
GatherDeviceInfo (
  IN PCI_IO_DEVICE                    *Bridge,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  );

/**
  Create private data for bridge device's PPB.
  
  @param Bridge     Parent bridge 
  @param Pci        Pci device block
  @param Bus        Bridge device's bus NO.
  @param Device     Bridge device's device NO.
  @param Func       Bridge device's func NO.
  
  @return bridge device instance
**/
PCI_IO_DEVICE             *
GatherPpbInfo (
  IN PCI_IO_DEVICE                    *Bridge,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  );

/**
  Create private data for hotplug bridge device
  
  @param Bridge Parent bridge instance
  @param Pci    PCI bar block
  @param Bus    hotplug bridge device's bus NO.
  @param Device hotplug bridge device's device NO.
  @param Func   hotplug bridge device's Func NO.
  
  @return hotplug bridge device instance
**/
PCI_IO_DEVICE             *
GatherP2CInfo (
  IN PCI_IO_DEVICE                    *Bridge,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  );

/**
  Create device path for pci deivce
  
  @param ParentDevicePath  Parent bridge's path
  @param PciIoDevice       Pci device instance
  
  @return device path protocol instance for specific pci device.
**/
EFI_DEVICE_PATH_PROTOCOL  *
CreatePciDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL *ParentDevicePath,
  IN  PCI_IO_DEVICE            *PciIoDevice
  );

/**
  Check the bar is existed or not.

  @param PciIoDevice       - A pointer to the PCI_IO_DEVICE.
  @param Offset            - The offset.
  @param BarLengthValue    - The bar length value.
  @param OriginalBarValue  - The original bar value.

  @retval EFI_NOT_FOUND     - The bar don't exist.
  @retval EFI_SUCCESS       - The bar exist.

**/
EFI_STATUS
BarExisted (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINTN         Offset,
  OUT UINT32       *BarLengthValue,
  OUT UINT32       *OriginalBarValue
  );

/**
  Test whether the device can support attributes 
  
  @param PciIoDevice   Pci device instance
  @param Command       Command register value.
  @param BridgeControl Bridge control value for PPB or P2C.
  @param OldCommand    Old command register offset
  @param OldBridgeControl Old Bridge control value for PPB or P2C.
  
  @return EFI_SUCCESS
**/
EFI_STATUS
PciTestSupportedAttribute (
  IN PCI_IO_DEVICE                      *PciIoDevice,
  IN UINT16                             *Command,
  IN UINT16                             *BridgeControl,
  IN UINT16                             *OldCommand,
  IN UINT16                             *OldBridgeControl
  );

/**
  Set the supported or current attributes of a PCI device
  
  @param PciIoDevice   - Structure pointer for PCI device.
  @param Command       - Command register value.
  @param BridgeControl - Bridge control value for PPB or P2C.
  @param Option        - Make a choice of EFI_SET_SUPPORTS or EFI_SET_ATTRIBUTES.
  
**/
EFI_STATUS
PciSetDeviceAttribute (
  IN PCI_IO_DEVICE                      *PciIoDevice,
  IN UINT16                             Command,
  IN UINT16                             BridgeControl,
  IN UINTN                              Option
  );

/**
  Determine if the device can support Fast Back to Back attribute
  
  @param PciIoDevice  Pci device instance
  @param StatusIndex  Status register value
**/
EFI_STATUS
GetFastBackToBackSupport (
  IN PCI_IO_DEVICE                      *PciIoDevice,
  IN UINT8                              StatusIndex
  );

/**
 Determine the related attributes of all devices under a Root Bridge
 
 @param PciIoDevice   PCI device instance
 
**/
EFI_STATUS
DetermineDeviceAttribute (
  IN PCI_IO_DEVICE                      *PciIoDevice
  );

/**
  This routine is used to update the bar information for those incompatible PCI device
  
  @param PciIoDevice      Pci device instance
  @return EFI_UNSUPPORTED failed to update Pci Info
**/
EFI_STATUS
UpdatePciInfo (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

/**
  This routine will update the alignment with the new alignment
  
  @param Alignment old alignment
  @param NewAlignment new alignment
  
**/
VOID
SetNewAlign (
  IN UINT64 *Alignment,
  IN UINT64 NewAlignment
  );

/**
  Parse PCI bar bit.
  
  @param PciIoDevice  Pci device instance
  @param Offset       bar offset
  @param BarIndex     bar index
  
  @return next bar offset.
**/
UINTN
PciParseBar (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINTN          Offset,
  IN UINTN          BarIndex
  );

/**
  This routine is used to initialize the bar of a PCI device
  It can be called typically when a device is going to be rejected

  @param PciIoDevice Pci device instance
**/
EFI_STATUS
InitializePciDevice (
  IN PCI_IO_DEVICE *PciIoDevice
  );

/**
  Init PPB for bridge device
  
  @param  PciIoDevice Pci device instance
**/
EFI_STATUS
InitializePpb (
  IN PCI_IO_DEVICE *PciIoDevice
  );

/**
  Init private data for Hotplug bridge device
  
  @param PciIoDevice hotplug bridge device
**/
EFI_STATUS
InitializeP2C (
  IN PCI_IO_DEVICE *PciIoDevice
  );

/**
  Create and initiliaze general PCI I/O device instance for
  PCI device/bridge device/hotplug bridge device.
  
  @param PciRootBridgeIo   Pointer to instance of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
  @param Pci               Pci bar block
  @param Bus               device Bus NO.
  @param Device            device device NO.
  @param Func              device func NO.
  
  @return instance of PCI device
**/
PCI_IO_DEVICE             *
CreatePciIoDevice (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  );

/**
  This routine is used to enumerate entire pci bus system
  in a given platform
  It is only called on the second start on the same Root Bridge.

  @param Controller  Parent bridge handler
  
  @return status of operation.
**/
EFI_STATUS
PciEnumeratorLight (
  IN EFI_HANDLE                    Controller
  );

/**
  Get bus range.
  
  @param Descriptors  A pointer to the address space descriptor.
  @param MinBus       The min bus.
  @param MaxBus       The max bus.
  @param BusRange     The bus range.
  
  @retval EFI_SUCCESS Success operation.
  @retval EFI_NOT_FOUND  can not find the specific bus.
**/
EFI_STATUS
PciGetBusRange (
  IN     EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  **Descriptors,
  OUT    UINT16                             *MinBus,
  OUT    UINT16                             *MaxBus,
  OUT    UINT16                             *BusRange
  );

EFI_STATUS
StartManagingRootBridge (
  IN PCI_IO_DEVICE *RootBridgeDev
  );

/**
  This routine can be used to check whether a PCI device should be rejected when light enumeration

  @param PciIoDevice  Pci device instance

  @retval TRUE      This device should be rejected
  @retval FALSE     This device shouldn't be rejected
  
**/
BOOLEAN
IsPciDeviceRejected (
  IN PCI_IO_DEVICE *PciIoDevice
  );

#endif

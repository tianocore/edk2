/** @file

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PciLib.h

Abstract:

  PCI Bus Driver Lib header file.
  Please use PCD feature flag PcdPciBusHotplugDeviceSupport to enable
  support hot plug.

Revision History

**/

#ifndef _EFI_PCI_LIB_H
#define _EFI_PCI_LIB_H

//
// Mask definistions for PCD PcdPciIncompatibleDeviceSupportMask
//
#define PCI_INCOMPATIBLE_ACPI_RESOURCE_SUPPORT         0x01
#define PCI_INCOMPATIBLE_READ_SUPPORT                  0x02
#define PCI_INCOMPATIBLE_WRITE_SUPPORT                 0x04
#define PCI_INCOMPATIBLE_REGISTER_UPDATE_SUPPORT       0x08
#define PCI_INCOMPATIBLE_ACCESS_WIDTH_SUPPORT          0x0a

typedef struct {
  EFI_HANDLE            Handle;
} EFI_DEVICE_HANDLE_EXTENDED_DATA_PAYLOAD;

typedef struct {
  UINT32                             Bar;
  UINT16                             DevicePathSize;
  UINT16                             ReqResSize;
  UINT16                             AllocResSize;
  UINT8                              *DevicePath;
  UINT8                              *ReqRes;
  UINT8                              *AllocRes;
} EFI_RESOURCE_ALLOC_FAILURE_ERROR_DATA_PAYLOAD;

/**
  Install protocol gEfiPciHotPlugRequestProtocolGuid
  @param Status    return status of protocol installation.
**/
void
InstallHotPlugRequestProtocol (
  IN  EFI_STATUS                    *Status
  );

/**
  Install protocol gEfiPciHotplugDeviceGuid into hotplug device
  instance
  
  @param PciIoDevice  hotplug device instance
  
**/
VOID
InstallPciHotplugGuid (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  );

/**
  UnInstall protocol gEfiPciHotplugDeviceGuid into hotplug device
  instance
  
  @param PciIoDevice  hotplug device instance
  
**/
VOID
UninstallPciHotplugGuid (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  );

/**
  Retrieve the BAR information via PciIo interface
  
  @param PciIoDevice Pci device instance
**/
VOID
GetBackPcCardBar (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  );

/**
  Remove rejected pci device from specific root bridge
  handle.
  
  @param RootBridgeHandle  specific parent root bridge handle
  @param Bridge            Bridge device instance
  
  @retval EFI_SUCCESS  Success operation.
**/
EFI_STATUS
RemoveRejectedPciDevices (
  EFI_HANDLE        RootBridgeHandle,
  IN PCI_IO_DEVICE  *Bridge
  );

/**
  Wrapper function for allocating resource for pci host bridge.
  
  @param PciResAlloc Point to protocol instance EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
  
**/
EFI_STATUS
PciHostBridgeResourceAllocator (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  );

EFI_STATUS
PciHostBridgeResourceAllocator_WithoutHotPlugDeviceSupport (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  );

EFI_STATUS
PciHostBridgeResourceAllocator_WithHotPlugDeviceSupport (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  );

/**
  Wapper function of scanning pci bus and assign bus number to the given PCI bus system
  Feature flag PcdPciBusHotplugDeviceSupport determine whether need support hotplug  
  
  @param  Bridge          Bridge device instance
  @param  StartBusNumber  start point
  @param  SubBusNumber    Point to sub bus number
  @param  PaddedBusRange  Customized bus number
  
  @retval EFI_SUCCESS     Success
  @retval EFI_DEVICE_ERROR Fail to scan bus
**/
EFI_STATUS
PciScanBus (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber,
  OUT UINT8                             *PaddedBusRange
  );

EFI_STATUS
PciScanBus_WithHotPlugDeviceSupport (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber,
  OUT UINT8                             *PaddedBusRange
  );

EFI_STATUS
PciScanBus_WithoutHotPlugDeviceSupport (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber,
  OUT UINT8                             *PaddedBusRange
  );

/**
  Process Option Rom on this host bridge
  
  @param Bridge  Pci bridge device instance
  
  @retval EFI_SUCCESS Success
**/

EFI_STATUS
PciRootBridgeP2CProcess (
  IN PCI_IO_DEVICE *Bridge
  );

/**
  Process Option Rom on this host bridge
  
  @param PciResAlloc Pointer to instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
  
  @retval EFI_NOT_FOUND Can not find the root bridge instance
  @retval EFI_SUCCESS   Success process
**/
EFI_STATUS
PciHostBridgeP2CProcess (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  );

/**
  This function is used to enumerate the entire host bridge
  in a given platform

  @param PciResAlloc   A pointer to the resource allocate protocol.

  @retval EFI_OUT_OF_RESOURCES no enough resource
  @retval EFI_SUCCESS Success

**/

EFI_STATUS
PciHostBridgeEnumerator (
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *PciResAlloc
  );

/**
  Read PCI configuration space through EFI_PCI_IO_PROTOCOL.

  @param  PciIo               A pointer to the EFI_PCI_O_PROTOCOL.
  @param  Width               Signifies the width of the memory operations.
  @Param  Address             The address within the PCI configuration space for the PCI controller.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

  @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
PciIoRead (
  IN       EFI_PCI_IO_PROTOCOL                    *PciIo,
  IN       EFI_PCI_IO_PROTOCOL_WIDTH              Width,
  IN       UINT32                                 Address,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
  );

/**
  Write PCI configuration space through EFI_PCI_IO_PROTOCOL.

  @param  PciIo               A pointer to the EFI_PCI_O_PROTOCOL.
  @param  Width               Signifies the width of the memory operations.
  @Param  Address             The address within the PCI configuration space for the PCI controller.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

  @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
PciIoWrite (
  IN       EFI_PCI_IO_PROTOCOL                    *PciIo,
  IN       EFI_PCI_IO_PROTOCOL_WIDTH              Width,
  IN       UINT32                                 Address,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
  );

/**
  Write PCI configuration space through EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.

  @param  PciRootBridgeIo     A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Pci                 A pointer to PCI_TYPE00.
  @param  Width               Signifies the width of the memory operations.
  @Param  Address             The address within the PCI configuration space for the PCI controller.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

  @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
PciRootBridgeIoWrite (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *PciRootBridgeIo,
  IN       PCI_TYPE00                             *Pci,
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN       UINT64                                 Address,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
  );

/**
  Read PCI configuration space through EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.

  @param  PciRootBridgeIo     A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Pci                 A pointer to PCI_TYPE00.
  @param  Width               Signifies the width of the memory operations.
  @Param  Address             The address within the PCI configuration space for the PCI controller.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

  @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
PciRootBridgeIoRead (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *PciRootBridgeIo,
  IN       PCI_TYPE00                             *Pci,
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN       UINT64                                 Address,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
  );
#endif

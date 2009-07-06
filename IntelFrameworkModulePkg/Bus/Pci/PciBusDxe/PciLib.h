/** @file
  Internal library declaration for PCI Bus module.

Copyright (c) 2006 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_PCI_LIB_H_
#define _EFI_PCI_LIB_H_

//
// Mask definistions for PCD PcdPciIncompatibleDeviceSupportMask
//
#define PCI_INCOMPATIBLE_ACPI_RESOURCE_SUPPORT         0x01
#define PCI_INCOMPATIBLE_READ_SUPPORT                  0x02
#define PCI_INCOMPATIBLE_WRITE_SUPPORT                 0x04
#define PCI_INCOMPATIBLE_REGISTER_UPDATE_SUPPORT       0x08
#define PCI_INCOMPATIBLE_ACCESS_WIDTH_SUPPORT          0x10

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
  Retrieve the PCI Card device BAR information via PciIo interface.

  @param PciIoDevice        PCI Card device instance.

**/
VOID
GetBackPcCardBar (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  );

/**
  Remove rejected pci device from specific root bridge
  handle.

  @param RootBridgeHandle  Specific parent root bridge handle.
  @param Bridge            Bridge device instance.

**/
VOID
RemoveRejectedPciDevices (
  IN EFI_HANDLE        RootBridgeHandle,
  IN PCI_IO_DEVICE     *Bridge
  );

/**
  Submits the I/O and memory resource requirements for the specified PCI Host Bridge.

  @param PciResAlloc  Point to protocol instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.

  @retval EFI_SUCCESS           Successfully finished resource allocation.
  @retval EFI_NOT_FOUND         Cannot get root bridge instance.
  @retval EFI_OUT_OF_RESOURCES  Platform failed to program the resources if no hot plug supported.
  @retval other                 Some error occurred when allocating resources for the PCI Host Bridge.

  @note   Feature flag PcdPciBusHotplugDeviceSupport determine whether need support hotplug.

**/
EFI_STATUS
PciHostBridgeResourceAllocator (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  );

/**
  Scan pci bus and assign bus number to the given PCI bus system.

  @param  Bridge           Bridge device instance.
  @param  StartBusNumber   start point.
  @param  SubBusNumber     Point to sub bus number.
  @param  PaddedBusRange   Customized bus number.

  @retval EFI_SUCCESS      Successfully scanned and assigned bus number.
  @retval other            Some error occurred when scanning pci bus.

  @note   Feature flag PcdPciBusHotplugDeviceSupport determine whether need support hotplug.

**/
EFI_STATUS
PciScanBus (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber,
  OUT UINT8                             *PaddedBusRange
  );

/**
  Process Option Rom on the specified root bridge.

  @param Bridge  Pci root bridge device instance.

  @retval EFI_SUCCESS   Success process.
  @retval other         Some error occurred when processing Option Rom on the root bridge.

**/
EFI_STATUS
PciRootBridgeP2CProcess (
  IN PCI_IO_DEVICE *Bridge
  );

/**
  Process Option Rom on the specified host bridge.

  @param PciResAlloc    Pointer to instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.

  @retval EFI_SUCCESS   Success process.
  @retval EFI_NOT_FOUND Can not find the root bridge instance.
  @retval other         Some error occurred when processing Option Rom on the host bridge.

**/
EFI_STATUS
PciHostBridgeP2CProcess (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  );

/**
  This function is used to enumerate the entire host bridge
  in a given platform.

  @param PciResAlloc   A pointer to the PCI Host Resource Allocation protocol.

  @retval EFI_SUCCESS            Successfully enumerated the host bridge.
  @retval EFI_OUT_OF_RESOURCES   No enough memory available.
  @retval other                  Some error occurred when enumerating the host bridge.

**/
EFI_STATUS
PciHostBridgeEnumerator (
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *PciResAlloc
  );

/**
  Read PCI configuration space through EFI_PCI_IO_PROTOCOL.

  @param  PciIo               A pointer to the EFI_PCI_O_PROTOCOL.
  @param  Width               Signifies the width of the memory operations.
  @param  Offset              The offset within the PCI configuration space for the PCI controller.
  @param  Count               The number of unit to be read.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI configuration header of the PCI controller.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER Buffer is NULL or Width is invalid.

**/
EFI_STATUS
PciIoRead (
  IN       EFI_PCI_IO_PROTOCOL                    *PciIo,
  IN       EFI_PCI_IO_PROTOCOL_WIDTH              Width,
  IN       UINT32                                 Offset,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
  );

/**
  Write PCI configuration space through EFI_PCI_IO_PROTOCOL.

  If PCI incompatibility check is enabled, do incompatibility check.

  @param  PciIo                 A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory operations.
  @param  Offset                The offset within the PCI configuration space for the PCI controller.
  @param  Count                 The number of PCI configuration operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI configuration header of the PCI controller.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER Buffer is NULL or Width is invalid.

**/
EFI_STATUS
PciIoWrite (
  IN       EFI_PCI_IO_PROTOCOL                    *PciIo,
  IN       EFI_PCI_IO_PROTOCOL_WIDTH              Width,
  IN       UINT32                                 Offset,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
  );

/**
  Write PCI configuration space through EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.

  @param  PciRootBridgeIo     A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Pci                 A pointer to PCI_TYPE00.
  @param  Width               Signifies the width of the memory operations.
  @param  Offset              The offset within the PCI configuration space for the PCI controller.
  @param  Count               The number of unit to be read.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI root bridge.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
PciRootBridgeIoWrite (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *PciRootBridgeIo,
  IN       PCI_TYPE00                             *Pci,
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN       UINT64                                 Offset,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
  );

/**
  Read PCI configuration space through EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.

  @param  PciRootBridgeIo     A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Pci                 A pointer to PCI_TYPE00.
  @param  Width               Signifies the width of the memory operations.
  @param  Offset              The offset within the PCI configuration space for the PCI controller.
  @param  Count               The number of unit to be read.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI root bridge.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
PciRootBridgeIoRead (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *PciRootBridgeIo,
  IN       PCI_TYPE00                             *Pci,            OPTIONAL
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN       UINT64                                 Offset,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
  );

#endif

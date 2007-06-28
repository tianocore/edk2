/*++

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

--*/

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

VOID
InstallHotPlugRequestProtocol (
  IN  EFI_STATUS                    *Status
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Status  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
InstallPciHotplugGuid (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
UninstallPciHotplugGuid (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
GetBackPcCardBar (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
RemoveRejectedPciDevices (
  EFI_HANDLE        RootBridgeHandle,
  IN PCI_IO_DEVICE  *Bridge
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridgeHandle  - TODO: add argument description
  Bridge            - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciHostBridgeResourceAllocator (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciResAlloc - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciHostBridgeResourceAllocator_WithoutHotPlugDeviceSupport (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  )
;

EFI_STATUS
PciHostBridgeResourceAllocator_WithHotPlugDeviceSupport (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  )
;

EFI_STATUS
PciScanBus (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber,
  OUT UINT8                             *PaddedBusRange
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge          - TODO: add argument description
  StartBusNumber  - TODO: add argument description
  SubBusNumber    - TODO: add argument description
  PaddedBusRange  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciScanBus_WithHotPlugDeviceSupport (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber,
  OUT UINT8                             *PaddedBusRange
  )
;

EFI_STATUS
PciScanBus_WithoutHotPlugDeviceSupport (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber,
  OUT UINT8                             *PaddedBusRange
  )
;

EFI_STATUS
PciRootBridgeP2CProcess (
  IN PCI_IO_DEVICE *Bridge
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciHostBridgeP2CProcess (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciResAlloc - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciHostBridgeEnumerator (
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *PciResAlloc
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciResAlloc - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

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

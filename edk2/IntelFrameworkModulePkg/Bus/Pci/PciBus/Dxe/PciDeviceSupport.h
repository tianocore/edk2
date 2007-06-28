/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciDeviceSupport.h
  
Abstract:

  

Revision History

--*/

#ifndef _EFI_PCI_DEVICE_SUPPORT_H
#define _EFI_PCI_DEVICE_SUPPORT_H

EFI_STATUS
InitializePciDevicePool (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
InsertRootBridge (
  PCI_IO_DEVICE *RootBridge
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridge  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
InsertPciDevice (
  PCI_IO_DEVICE *Bridge,
  PCI_IO_DEVICE *PciDeviceNode
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge        - TODO: add argument description
  PciDeviceNode - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DestroyRootBridge (
  IN PCI_IO_DEVICE *RootBridge
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridge  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DestroyPciDeviceTree (
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
DestroyRootBridgeByHandle (
  EFI_HANDLE Controller
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Controller  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
RegisterPciDevice (
  IN  EFI_HANDLE                     Controller,
  IN  PCI_IO_DEVICE                  *PciIoDevice,
  OUT EFI_HANDLE                     *Handle OPTIONAL
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Controller  - TODO: add argument description
  PciIoDevice - TODO: add argument description
  Handle      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
RemoveAllPciDeviceOnBridge (
  EFI_HANDLE               RootBridgeHandle,
  PCI_IO_DEVICE            *Bridge
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
DeRegisterPciDevice (
  IN  EFI_HANDLE                     Controller,
  IN  EFI_HANDLE                     Handle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Controller  - TODO: add argument description
  Handle      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
StartPciDevicesOnBridge (
  IN EFI_HANDLE                          Controller,
  IN PCI_IO_DEVICE                       *RootBridge,
  IN EFI_DEVICE_PATH_PROTOCOL            *RemainingDevicePath,
  IN OUT UINT8                           *NumberOfChildren,
  IN OUT EFI_HANDLE                      *ChildHandleBuffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Controller          - TODO: add argument description
  RootBridge          - TODO: add argument description
  RemainingDevicePath - TODO: add argument description
  NumberOfChildren    - TODO: add argument description
  ChildHandleBuffer   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
StartPciDevices (
  IN EFI_HANDLE                         Controller,
  IN EFI_DEVICE_PATH_PROTOCOL           *RemainingDevicePath
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Controller          - TODO: add argument description
  RemainingDevicePath - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_IO_DEVICE *
CreateRootBridge (
  IN EFI_HANDLE RootBridgeHandle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridgeHandle  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_IO_DEVICE *
GetRootBridgeByHandle (
  EFI_HANDLE RootBridgeHandle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridgeHandle  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
RootBridgeExisted (
  IN EFI_HANDLE RootBridgeHandle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridgeHandle  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
PciDeviceExisted (
  IN PCI_IO_DEVICE    *Bridge,
  IN PCI_IO_DEVICE    *PciIoDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge      - TODO: add argument description
  PciIoDevice - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_IO_DEVICE *
ActiveVGADeviceOnTheSameSegment (
  IN PCI_IO_DEVICE        *VgaDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  VgaDevice - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_IO_DEVICE *
ActiveVGADeviceOnTheRootBridge (
  IN PCI_IO_DEVICE        *RootBridge
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridge  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
GetHpcPciAddress (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN  EFI_DEVICE_PATH_PROTOCOL         *HpcDevicePath,
  OUT UINT64                           *PciAddress
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciRootBridgeIo - TODO: add argument description
  HpcDevicePath   - TODO: add argument description
  PciAddress      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
GetHpcPciAddressFromRootBridge (
  IN  PCI_IO_DEVICE                    *RootBridge,
  IN  EFI_DEVICE_PATH_PROTOCOL         *RemainingDevicePath,
  OUT UINT64                           *PciAddress
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridge          - TODO: add argument description
  RemainingDevicePath - TODO: add argument description
  PciAddress          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
FreePciDevice (
  IN PCI_IO_DEVICE *PciIoDevice
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

#endif

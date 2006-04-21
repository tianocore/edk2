/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciEnumeratorSupport.h
  
Abstract:

  PCI Bus Driver

Revision History

--*/

#ifndef _EFI_PCI_ENUMERATOR_SUPPORT_H
#define _EFI_PCI_ENUMERATOR_SUPPORT_H

EFI_STATUS
PciDevicePresent (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  PCI_TYPE00                          *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciRootBridgeIo - TODO: add argument description
  Pci             - TODO: add argument description
  Bus             - TODO: add argument description
  Device          - TODO: add argument description
  Func            - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciPciDeviceInfoCollector (
  IN PCI_IO_DEVICE                      *Bridge,
  UINT8                                 StartBusNumber
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge          - TODO: add argument description
  StartBusNumber  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciSearchDevice (
  IN PCI_IO_DEVICE                      *Bridge,
  PCI_TYPE00                            *Pci,
  UINT8                                 Bus,
  UINT8                                 Device,
  UINT8                                 Func,
  PCI_IO_DEVICE                         **PciDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge    - TODO: add argument description
  Pci       - TODO: add argument description
  Bus       - TODO: add argument description
  Device    - TODO: add argument description
  Func      - TODO: add argument description
  PciDevice - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_IO_DEVICE             *
GatherDeviceInfo (
  IN PCI_IO_DEVICE                    *Bridge,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge  - TODO: add argument description
  Pci     - TODO: add argument description
  Bus     - TODO: add argument description
  Device  - TODO: add argument description
  Func    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_IO_DEVICE             *
GatherPpbInfo (
  IN PCI_IO_DEVICE                    *Bridge,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge  - TODO: add argument description
  Pci     - TODO: add argument description
  Bus     - TODO: add argument description
  Device  - TODO: add argument description
  Func    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_IO_DEVICE             *
GatherP2CInfo (
  IN PCI_IO_DEVICE                    *Bridge,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge  - TODO: add argument description
  Pci     - TODO: add argument description
  Bus     - TODO: add argument description
  Device  - TODO: add argument description
  Func    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_DEVICE_PATH_PROTOCOL  *
CreatePciDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL *ParentDevicePath,
  IN  PCI_IO_DEVICE            *PciIoDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ParentDevicePath  - TODO: add argument description
  PciIoDevice       - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
BarExisted (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINTN         Offset,
  OUT UINT32       *BarLengthValue,
  OUT UINT32       *OriginalBarValue
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice       - TODO: add argument description
  Offset            - TODO: add argument description
  BarLengthValue    - TODO: add argument description
  OriginalBarValue  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciTestSupportedAttribute (
  IN PCI_IO_DEVICE                      *PciIoDevice,
  IN UINT16                             *Command,
  IN UINT16                             *BridgeControl,
  IN UINT16                             *OldCommand,
  IN UINT16                             *OldBridgeControl
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice       - TODO: add argument description
  Command           - TODO: add argument description
  BridgeControl     - TODO: add argument description
  OldCommand        - TODO: add argument description
  OldBridgeControl  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciSetDeviceAttribute (
  IN PCI_IO_DEVICE                      *PciIoDevice,
  IN UINT16                             Command,
  IN UINT16                             BridgeControl,
  IN UINTN                              Option
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice   - TODO: add argument description
  Command       - TODO: add argument description
  BridgeControl - TODO: add argument description
  Option        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
GetFastBackToBackSupport (
  IN PCI_IO_DEVICE                      *PciIoDevice,
  IN UINT8                              StatusIndex
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description
  StatusIndex - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DetermineDeviceAttribute (
  IN PCI_IO_DEVICE                      *PciIoDevice
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
UpdatePciInfo (
  IN PCI_IO_DEVICE  *PciIoDevice
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
SetNewAlign (
  IN UINT64 *Alignment,
  IN UINT64 NewAlignment
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Alignment     - TODO: add argument description
  NewAlignment  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

UINTN
PciParseBar (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINTN          Offset,
  IN UINTN          BarIndex
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description
  Offset      - TODO: add argument description
  BarIndex    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
InitializePciDevice (
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

EFI_STATUS
InitializePpb (
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

EFI_STATUS
InitializeP2C (
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

PCI_IO_DEVICE             *
CreatePciIoDevice (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciRootBridgeIo - TODO: add argument description
  Pci             - TODO: add argument description
  Bus             - TODO: add argument description
  Device          - TODO: add argument description
  Func            - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciEnumeratorLight (
  IN EFI_HANDLE                    Controller
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
PciGetBusRange (
  IN     EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  **Descriptors,
  OUT    UINT16                             *MinBus,
  OUT    UINT16                             *MaxBus,
  OUT    UINT16                             *BusRange
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Descriptors - TODO: add argument description
  MinBus      - TODO: add argument description
  MaxBus      - TODO: add argument description
  BusRange    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
StartManagingRootBridge (
  IN PCI_IO_DEVICE *RootBridgeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridgeDev - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
IsPciDeviceRejected (
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

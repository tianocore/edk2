/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciEnumerator.h
  
Abstract:

  PCI Bus Driver

Revision History

--*/

#ifndef _EFI_PCI_ENUMERATOR_H
#define _EFI_PCI_ENUMERATOR_H

#include "PciResourceSupport.h"

EFI_STATUS
PciEnumerator (
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
PciRootBridgeEnumerator (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *PciResAlloc,
  IN PCI_IO_DEVICE                                     *RootBridgeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciResAlloc   - TODO: add argument description
  RootBridgeDev - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ProcessOptionRom (
  IN PCI_IO_DEVICE *Bridge,
  IN UINT64        RomBase,
  IN UINT64        MaxLength
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge    - TODO: add argument description
  RomBase   - TODO: add argument description
  MaxLength - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciAssignBusNumber (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge          - TODO: add argument description
  StartBusNumber  - TODO: add argument description
  SubBusNumber    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DetermineRootBridgeAttributes (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc,
  IN PCI_IO_DEVICE                                    *RootBridgeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciResAlloc   - TODO: add argument description
  RootBridgeDev - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

UINT64
GetMaxOptionRomSize (
  IN PCI_IO_DEVICE   *Bridge
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
PciHostBridgeDeviceAttribute (
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
GetResourceAllocationStatus (
  VOID        *AcpiConfig,
  OUT UINT64  *IoResStatus,
  OUT UINT64  *Mem32ResStatus,
  OUT UINT64  *PMem32ResStatus,
  OUT UINT64  *Mem64ResStatus,
  OUT UINT64  *PMem64ResStatus
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  AcpiConfig      - TODO: add argument description
  IoResStatus     - TODO: add argument description
  Mem32ResStatus  - TODO: add argument description
  PMem32ResStatus - TODO: add argument description
  Mem64ResStatus  - TODO: add argument description
  PMem64ResStatus - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
RejectPciDevice (
  IN PCI_IO_DEVICE       *PciDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciDevice - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
IsRejectiveDevice (
  IN  PCI_RESOURCE_NODE   *PciResNode
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciResNode  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_RESOURCE_NODE *
GetLargerConsumerDevice (
  IN  PCI_RESOURCE_NODE   *PciResNode1,
  IN  PCI_RESOURCE_NODE   *PciResNode2
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciResNode1 - TODO: add argument description
  PciResNode2 - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_RESOURCE_NODE *
GetMaxResourceConsumerDevice (
  IN  PCI_RESOURCE_NODE   *ResPool
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ResPool - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciHostBridgeAdjustAllocation (
  IN  PCI_RESOURCE_NODE   *IoPool,
  IN  PCI_RESOURCE_NODE   *Mem32Pool,
  IN  PCI_RESOURCE_NODE   *PMem32Pool,
  IN  PCI_RESOURCE_NODE   *Mem64Pool,
  IN  PCI_RESOURCE_NODE   *PMem64Pool,
  IN  UINT64              IoResStatus,
  IN  UINT64              Mem32ResStatus,
  IN  UINT64              PMem32ResStatus,
  IN  UINT64              Mem64ResStatus,
  IN  UINT64              PMem64ResStatus
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IoPool          - TODO: add argument description
  Mem32Pool       - TODO: add argument description
  PMem32Pool      - TODO: add argument description
  Mem64Pool       - TODO: add argument description
  PMem64Pool      - TODO: add argument description
  IoResStatus     - TODO: add argument description
  Mem32ResStatus  - TODO: add argument description
  PMem32ResStatus - TODO: add argument description
  Mem64ResStatus  - TODO: add argument description
  PMem64ResStatus - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ConstructAcpiResourceRequestor (
  IN PCI_IO_DEVICE      *Bridge,
  IN PCI_RESOURCE_NODE  *IoNode,
  IN PCI_RESOURCE_NODE  *Mem32Node,
  IN PCI_RESOURCE_NODE  *PMem32Node,
  IN PCI_RESOURCE_NODE  *Mem64Node,
  IN PCI_RESOURCE_NODE  *PMem64Node,
  OUT VOID              **pConfig
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge      - TODO: add argument description
  IoNode      - TODO: add argument description
  Mem32Node   - TODO: add argument description
  PMem32Node  - TODO: add argument description
  Mem64Node   - TODO: add argument description
  PMem64Node  - TODO: add argument description
  pConfig     - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
GetResourceBase (
  IN VOID     *pConfig,
  OUT UINT64  *IoBase,
  OUT UINT64  *Mem32Base,
  OUT UINT64  *PMem32Base,
  OUT UINT64  *Mem64Base,
  OUT UINT64  *PMem64Base
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  pConfig     - TODO: add argument description
  IoBase      - TODO: add argument description
  Mem32Base   - TODO: add argument description
  PMem32Base  - TODO: add argument description
  Mem64Base   - TODO: add argument description
  PMem64Base  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciBridgeEnumerator (
  IN PCI_IO_DEVICE                                     *BridgeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  BridgeDev - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PciBridgeResourceAllocator (
  IN PCI_IO_DEVICE  *Bridge
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
GetResourceBaseFromBridge (
  IN  PCI_IO_DEVICE *Bridge,
  OUT UINT64        *IoBase,
  OUT UINT64        *Mem32Base,
  OUT UINT64        *PMem32Base,
  OUT UINT64        *Mem64Base,
  OUT UINT64        *PMem64Base
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge      - TODO: add argument description
  IoBase      - TODO: add argument description
  Mem32Base   - TODO: add argument description
  PMem32Base  - TODO: add argument description
  Mem64Base   - TODO: add argument description
  PMem64Base  - TODO: add argument description

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
NotifyPhase (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc,
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE       Phase
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciResAlloc - TODO: add argument description
  Phase       - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PreprocessController (
  IN PCI_IO_DEVICE                                  *Bridge,
  IN UINT8                                          Bus,
  IN UINT8                                          Device,
  IN UINT8                                          Func,
  IN EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE   Phase
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge  - TODO: add argument description
  Bus     - TODO: add argument description
  Device  - TODO: add argument description
  Func    - TODO: add argument description
  Phase   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
PciHotPlugRequestNotify (
  IN EFI_PCI_HOTPLUG_REQUEST_PROTOCOL * This,
  IN EFI_PCI_HOTPLUG_OPERATION        Operation,
  IN EFI_HANDLE                       Controller,
  IN EFI_DEVICE_PATH_PROTOCOL         * RemainingDevicePath OPTIONAL,
  IN OUT UINT8                        *NumberOfChildren,
  IN OUT EFI_HANDLE                   * ChildHandleBuffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This                - TODO: add argument description
  Operation           - TODO: add argument description
  Controller          - TODO: add argument description
  RemainingDevicePath - TODO: add argument description
  NumberOfChildren    - TODO: add argument description
  ChildHandleBuffer   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
SearchHostBridgeHandle (
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

EFI_STATUS
AddHostBridgeEnumerator (
  IN EFI_HANDLE HostBridgeHandle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  HostBridgeHandle  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif

/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciResourceSupport.h
  
Abstract:

  

Revision History

--*/

#ifndef _EFI_PCI_RESOURCE_SUPPORT_H
#define _EFI_PCI_RESOURCE_SUPPORT_H

#define RESERVED_RESOURCE_SIGNATURE EFI_SIGNATURE_32 ('r', 's', 'v', 'd')

typedef struct {
  UINT64        Base;
  UINT64        Length;
  PCI_BAR_TYPE  ResType;
} PCI_RESERVED_RESOURCE_NODE;

typedef struct {
  UINT32                      Signature;
  LIST_ENTRY                  Link;
  PCI_RESERVED_RESOURCE_NODE  Node;
} PCI_RESERVED_RESOURCE_LIST;

#define RESOURCED_LIST_FROM_NODE(a) \
  CR (a, PCI_RESERVED_RESOURCE_LIST, Node, RESERVED_RESOURCE_SIGNATURE)

#define RESOURCED_LIST_FROM_LINK(a) \
  CR (a, PCI_RESERVED_RESOURCE_LIST, Link, RESERVED_RESOURCE_SIGNATURE)

typedef enum {
  PciResUsageTypical            = 0,
  PciResUsagePadding,
  PciResUsageOptionRomProcessing
} PCI_RESOURCE_USAGE;

#define PCI_RESOURCE_SIGNATURE  EFI_SIGNATURE_32 ('p', 'c', 'r', 'c')

typedef struct {
  UINT32              Signature;
  LIST_ENTRY          Link;
  LIST_ENTRY          ChildList;
  PCI_IO_DEVICE       *PciDev;
  UINT64              Alignment;
  UINT64              Offset;
  UINT8               Bar;
  PCI_BAR_TYPE        ResType;
  UINT64              Length;
  BOOLEAN             Reserved;
  PCI_RESOURCE_USAGE  ResourceUsage;
} PCI_RESOURCE_NODE;

#define RESOURCE_NODE_FROM_LINK(a) \
  CR (a, PCI_RESOURCE_NODE, Link, PCI_RESOURCE_SIGNATURE)

EFI_STATUS
SkipVGAAperture (
  OUT UINT64   *Start,
  IN  UINT64   Length
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Start   - TODO: add argument description
  Length  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SkipIsaAliasAperture (
  OUT UINT64   *Start,
  IN  UINT64   Length
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Start   - TODO: add argument description
  Length  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
InsertResourceNode (
  PCI_RESOURCE_NODE *Bridge,
  PCI_RESOURCE_NODE *ResNode
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge  - TODO: add argument description
  ResNode - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
MergeResourceTree (
  PCI_RESOURCE_NODE *Dst,
  PCI_RESOURCE_NODE *Res,
  BOOLEAN           TypeMerge
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Dst       - TODO: add argument description
  Res       - TODO: add argument description
  TypeMerge - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
CalculateApertureIo16 (
  IN PCI_RESOURCE_NODE *Bridge
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
CalculateResourceAperture (
  IN PCI_RESOURCE_NODE *Bridge
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
GetResourceFromDevice (
  PCI_IO_DEVICE     *PciDev,
  PCI_RESOURCE_NODE *IoNode,
  PCI_RESOURCE_NODE *Mem32Node,
  PCI_RESOURCE_NODE *PMem32Node,
  PCI_RESOURCE_NODE *Mem64Node,
  PCI_RESOURCE_NODE *PMem64Node
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciDev      - TODO: add argument description
  IoNode      - TODO: add argument description
  Mem32Node   - TODO: add argument description
  PMem32Node  - TODO: add argument description
  Mem64Node   - TODO: add argument description
  PMem64Node  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

PCI_RESOURCE_NODE *
CreateResourceNode (
  IN PCI_IO_DEVICE         *PciDev,
  IN UINT64                Length,
  IN UINT64                Alignment,
  IN UINT8                 Bar,
  IN PCI_BAR_TYPE          ResType,
  IN PCI_RESOURCE_USAGE    ResUsage
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciDev    - TODO: add argument description
  Length    - TODO: add argument description
  Alignment - TODO: add argument description
  Bar       - TODO: add argument description
  ResType   - TODO: add argument description
  ResUsage  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
CreateResourceMap (
  IN PCI_IO_DEVICE     *Bridge,
  IN PCI_RESOURCE_NODE *IoNode,
  IN PCI_RESOURCE_NODE *Mem32Node,
  IN PCI_RESOURCE_NODE *PMem32Node,
  IN PCI_RESOURCE_NODE *Mem64Node,
  IN PCI_RESOURCE_NODE *PMem64Node
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

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ResourcePaddingPolicy (
  PCI_IO_DEVICE     *PciDev,
  PCI_RESOURCE_NODE *IoNode,
  PCI_RESOURCE_NODE *Mem32Node,
  PCI_RESOURCE_NODE *PMem32Node,
  PCI_RESOURCE_NODE *Mem64Node,
  PCI_RESOURCE_NODE *PMem64Node
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciDev      - TODO: add argument description
  IoNode      - TODO: add argument description
  Mem32Node   - TODO: add argument description
  PMem32Node  - TODO: add argument description
  Mem64Node   - TODO: add argument description
  PMem64Node  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DegradeResource (
  IN PCI_IO_DEVICE     *Bridge,
  IN PCI_RESOURCE_NODE *Mem32Node,
  IN PCI_RESOURCE_NODE *PMem32Node,
  IN PCI_RESOURCE_NODE *Mem64Node,
  IN PCI_RESOURCE_NODE *PMem64Node
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge      - TODO: add argument description
  Mem32Node   - TODO: add argument description
  PMem32Node  - TODO: add argument description
  Mem64Node   - TODO: add argument description
  PMem64Node  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
BridgeSupportResourceDecode (
  IN PCI_IO_DEVICE *Bridge,
  IN UINT32        Decode
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Bridge  - TODO: add argument description
  Decode  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ProgramResource (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Bridge
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Base    - TODO: add argument description
  Bridge  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ProgramBar (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Node
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Base  - TODO: add argument description
  Node  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ProgramPpbApperture (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Node
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Base  - TODO: add argument description
  Node  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ProgrameUpstreamBridgeForRom (
  IN PCI_IO_DEVICE   *PciDevice,
  IN UINT32          OptionRomBase,
  IN BOOLEAN         Enable
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciDevice     - TODO: add argument description
  OptionRomBase - TODO: add argument description
  Enable        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
ResourceRequestExisted (
  IN PCI_RESOURCE_NODE *Bridge
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
InitializeResourcePool (
  PCI_RESOURCE_NODE   *ResourcePool,
  PCI_BAR_TYPE        ResourceType
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ResourcePool  - TODO: add argument description
  ResourceType  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
GetResourceMap (
  PCI_IO_DEVICE      *PciDev,
  PCI_RESOURCE_NODE  **IoBridge,
  PCI_RESOURCE_NODE  **Mem32Bridge,
  PCI_RESOURCE_NODE  **PMem32Bridge,
  PCI_RESOURCE_NODE  **Mem64Bridge,
  PCI_RESOURCE_NODE  **PMem64Bridge,
  PCI_RESOURCE_NODE  *IoPool,
  PCI_RESOURCE_NODE  *Mem32Pool,
  PCI_RESOURCE_NODE  *PMem32Pool,
  PCI_RESOURCE_NODE  *Mem64Pool,
  PCI_RESOURCE_NODE  *PMem64Pool
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciDev        - TODO: add argument description
  IoBridge      - TODO: add argument description
  Mem32Bridge   - TODO: add argument description
  PMem32Bridge  - TODO: add argument description
  Mem64Bridge   - TODO: add argument description
  PMem64Bridge  - TODO: add argument description
  IoPool        - TODO: add argument description
  Mem32Pool     - TODO: add argument description
  PMem32Pool    - TODO: add argument description
  Mem64Pool     - TODO: add argument description
  PMem64Pool    - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DestroyResourceTree (
  IN PCI_RESOURCE_NODE *Bridge
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
RecordReservedResource (
  IN UINT64         Base,
  IN UINT64         Length,
  IN PCI_BAR_TYPE   ResType,
  IN PCI_IO_DEVICE  *Bridge
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Base    - TODO: add argument description
  Length  - TODO: add argument description
  ResType - TODO: add argument description
  Bridge  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ResourcePaddingForCardBusBridge (
  PCI_IO_DEVICE     *PciDev,
  PCI_RESOURCE_NODE *IoNode,
  PCI_RESOURCE_NODE *Mem32Node,
  PCI_RESOURCE_NODE *PMem32Node,
  PCI_RESOURCE_NODE *Mem64Node,
  PCI_RESOURCE_NODE *PMem64Node
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciDev      - TODO: add argument description
  IoNode      - TODO: add argument description
  Mem32Node   - TODO: add argument description
  PMem32Node  - TODO: add argument description
  Mem64Node   - TODO: add argument description
  PMem64Node  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ProgramP2C (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Node
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Base  - TODO: add argument description
  Node  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ApplyResourcePadding (
  PCI_IO_DEVICE     *PciDev,
  PCI_RESOURCE_NODE *IoNode,
  PCI_RESOURCE_NODE *Mem32Node,
  PCI_RESOURCE_NODE *PMem32Node,
  PCI_RESOURCE_NODE *Mem64Node,
  PCI_RESOURCE_NODE *PMem64Node
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciDev      - TODO: add argument description
  IoNode      - TODO: add argument description
  Mem32Node   - TODO: add argument description
  PMem32Node  - TODO: add argument description
  Mem64Node   - TODO: add argument description
  PMem64Node  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
GetResourcePaddingPpb (
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
ResetAllPpbBusReg (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber
  )
/*++

Routine Description:

  Reset bus register

Arguments:

  Bridge          - a pointer to the PCI_IO_DEVICE
  StartBusNumber  - the number of bus

Returns:

  None

--*/
;

#endif

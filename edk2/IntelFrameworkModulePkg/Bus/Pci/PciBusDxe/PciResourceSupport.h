/** @file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _EFI_PCI_RESOURCE_SUPPORT_H_
#define _EFI_PCI_RESOURCE_SUPPORT_H_

#define RESERVED_RESOURCE_SIGNATURE SIGNATURE_32 ('r', 's', 'v', 'd')

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

#define PCI_RESOURCE_SIGNATURE  SIGNATURE_32 ('p', 'c', 'r', 'c')

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

/**
  The function is used to skip VGA range
  
  @param Start    address including VGA range
  @param Length   length of VGA range.
  
  @retval EFI_SUCCESS success.
**/
EFI_STATUS
SkipVGAAperture (
  OUT UINT64   *Start,
  IN  UINT64   Length
  );

/**
  This function is used to skip ISA aliasing aperture.
  
  @param Start    address including ISA aliasing aperture.
  @param Length   length of ISA aliasing aperture.
  
  @retval EFI_SUCCESS success.
**/
EFI_STATUS
SkipIsaAliasAperture (
  OUT UINT64   *Start,
  IN  UINT64   Length
  );

/**
  This function inserts a resource node into the resource list.
  The resource list is sorted in descend order.

  @param Bridge  PCI resource node for bridge.
  @param ResNode Resource node want to be inserted.
  
  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
InsertResourceNode (
  PCI_RESOURCE_NODE *Bridge,
  PCI_RESOURCE_NODE *ResNode
  );

/**

Routine Description:

  This routine is used to merge two different resource tree in need of
  resoure degradation. For example, if a upstream PPB doesn't support,
  prefetchable memory decoding, the PCI bus driver will choose to call this function
  to merge prefectchable memory resource list into normal memory list.

  If the TypeMerge is TRUE, Res resource type is changed to the type of destination resource
  type.

  @param Dst        Point to destination resource tree.
  @param Res        Point to source resource tree.
  @param TypeMerge  If the TypeMerge is TRUE, Res resource type is changed to the type of 
                    destination resource type.
                    
                    
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
MergeResourceTree (
  PCI_RESOURCE_NODE *Dst,
  PCI_RESOURCE_NODE *Res,
  BOOLEAN           TypeMerge
  );

/**
  This function is used to calculate the IO16 aperture
  for a bridge.

  @param Bridge PCI resource node for bridge.
  
  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
CalculateApertureIo16 (
  IN PCI_RESOURCE_NODE *Bridge
  );

/**
  This function is used to calculate the resource aperture
  for a given bridge device.

  @param Bridge Give bridge device.
  
  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
CalculateResourceAperture (
  IN PCI_RESOURCE_NODE *Bridge
  );

/**
  Get IO/Memory resource infor for given PCI device.
  
  @param PciDev     Pci device instance.
  @param IoNode     Resource info node for IO .
  @param Mem32Node  Resource info node for 32-bit memory.
  @param PMem32Node Resource info node for 32-bit PMemory.
  @param Mem64Node  Resource info node for 64-bit memory.
  @param PMem64Node Resource info node for 64-bit PMemory.
  
  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
GetResourceFromDevice (
  PCI_IO_DEVICE     *PciDev,
  PCI_RESOURCE_NODE *IoNode,
  PCI_RESOURCE_NODE *Mem32Node,
  PCI_RESOURCE_NODE *PMem32Node,
  PCI_RESOURCE_NODE *Mem64Node,
  PCI_RESOURCE_NODE *PMem64Node
  );

/**
  This function is used to create a resource node.

  @param PciDev       Pci device instance.
  @param Length       Length of Io/Memory resource.
  @param Alignment    Alignment of resource.
  @param Bar          Bar index.
  @param ResType      Type of resource: IO/Memory.
  @param ResUsage     Resource usage.
**/
PCI_RESOURCE_NODE *
CreateResourceNode (
  IN PCI_IO_DEVICE         *PciDev,
  IN UINT64                Length,
  IN UINT64                Alignment,
  IN UINT8                 Bar,
  IN PCI_BAR_TYPE          ResType,
  IN PCI_RESOURCE_USAGE    ResUsage
  );

/**
  This routine is used to extract resource request from
  device node list.

  @param Bridge     Pci device instance.
  @param IoNode     Resource info node for IO.
  @param Mem32Node  Resource info node for 32-bit memory.
  @param PMem32Node Resource info node for 32-bit PMemory.
  @param Mem64Node  Resource info node for 64-bit memory.
  @param PMem64Node Resource info node for 64-bit PMemory.

  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
CreateResourceMap (
  IN PCI_IO_DEVICE     *Bridge,
  IN PCI_RESOURCE_NODE *IoNode,
  IN PCI_RESOURCE_NODE *Mem32Node,
  IN PCI_RESOURCE_NODE *PMem32Node,
  IN PCI_RESOURCE_NODE *Mem64Node,
  IN PCI_RESOURCE_NODE *PMem64Node
  );

/**
  This function is used to do the resource padding for a specific platform.

  @param PciDev     Pci device instance.
  @param IoNode     Resource info node for IO. 
  @param Mem32Node  Resource info node for 32-bit memory.
  @param PMem32Node Resource info node for 32-bit PMemory.
  @param Mem64Node  Resource info node for 64-bit memory.
  @param PMem64Node Resource info node for 64-bit PMemory.

  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
ResourcePaddingPolicy (
  PCI_IO_DEVICE     *PciDev,
  PCI_RESOURCE_NODE *IoNode,
  PCI_RESOURCE_NODE *Mem32Node,
  PCI_RESOURCE_NODE *PMem32Node,
  PCI_RESOURCE_NODE *Mem64Node,
  PCI_RESOURCE_NODE *PMem64Node
  );

/**
  This function is used to degrade resource if the upstream bridge 
  doesn't support certain resource. Degradation path is 
  PMEM64 -> MEM64  -> MEM32
  PMEM64 -> PMEM32 -> MEM32
  IO32   -> IO16

  @param Bridge     Pci device instance.
  @param Mem32Node  Resource info node for 32-bit memory.
  @param PMem32Node Resource info node for 32-bit PMemory.
  @param Mem64Node  Resource info node for 64-bit memory.
  @param PMem64Node Resource info node for 64-bit PMemory.

  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
DegradeResource (
  IN PCI_IO_DEVICE     *Bridge,
  IN PCI_RESOURCE_NODE *Mem32Node,
  IN PCI_RESOURCE_NODE *PMem32Node,
  IN PCI_RESOURCE_NODE *Mem64Node,
  IN PCI_RESOURCE_NODE *PMem64Node
  );

/**
  Test whether bridge device support decode resource.
  
  @param Bridge    Bridge device instance.
  @param Decode    Decode type according to resource type.
  
  @return whether bridge device support decode resource.
  
**/
BOOLEAN
BridgeSupportResourceDecode (
  IN PCI_IO_DEVICE *Bridge,
  IN UINT32        Decode
  );

/**
  This function is used to program the resource allocated 
  for each resource node.

  
  @param Base     Base address of resource.
  @param Bridge   Bridge device instance.
  
  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
ProgramResource (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Bridge
  );

/**
  Program Bar register.
  
  @param Base  Base address for resource.
  @param Node  Point to resoure node structure.
  
  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
ProgramBar (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Node
  );

/**
  Program PPB apperture.
  
  @param Base  Base address for resource.
  @param Node  Point to resoure node structure.
  
  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
ProgramPpbApperture (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Node
  );

/**
  Program parent bridge for oprom.
  
  @param PciDevice      Pci deivce instance.
  @param OptionRomBase  Base address for oprom.
  @param Enable         Enable/Disable.
  
  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
ProgrameUpstreamBridgeForRom (
  IN PCI_IO_DEVICE   *PciDevice,
  IN UINT32          OptionRomBase,
  IN BOOLEAN         Enable
  );

/**
  Test whether resource exists for a bridge.
  
  @param Bridge  Point to resource node for a bridge.
  
  @return whether resource exists.
**/
BOOLEAN
ResourceRequestExisted (
  IN PCI_RESOURCE_NODE *Bridge
  );

/**
  Initialize resource pool structure.
  
  @param ResourcePool Point to resource pool structure.
  @param ResourceType Type of resource.
**/
EFI_STATUS
InitializeResourcePool (
  PCI_RESOURCE_NODE   *ResourcePool,
  PCI_BAR_TYPE        ResourceType
  );

/**
  Get all resource information for given Pci device.
  
  @param PciDev         Pci device instance.
  @param IoBridge       Io resource node.
  @param Mem32Bridge    32-bit memory node.
  @param PMem32Bridge   32-bit Pmemory node.
  @param Mem64Bridge    64-bit memory node.
  @param PMem64Bridge   64-bit PMemory node.
  @param IoPool         Link list header for Io resource.
  @param Mem32Pool      Link list header for 32-bit memory.
  @param PMem32Pool     Link list header for 32-bit Pmemory.
  @param Mem64Pool      Link list header for 64-bit memory.
  @param PMem64Pool     Link list header for 64-bit Pmemory.
  
  @retval EFI_SUCCESS Success.
**/
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
  );

/**
  Destory given resource tree.
  
  @param Bridge  root node of resource tree.
  
  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
DestroyResourceTree (
  IN PCI_RESOURCE_NODE *Bridge
  );

/**
  Record the reserved resource and insert to reserved list.
  
  @param Base     Base address of reserved resourse.
  @param Length   Length of reserved resource.
  @param ResType  Resource type.
  @param Bridge   Pci device instance.
**/
EFI_STATUS
RecordReservedResource (
  IN UINT64         Base,
  IN UINT64         Length,
  IN PCI_BAR_TYPE   ResType,
  IN PCI_IO_DEVICE  *Bridge
  );

/**
  Insert resource padding for P2C.
  
  @param PciDev     Pci device instance.
  @param IoNode     Resource info node for IO. 
  @param Mem32Node  Resource info node for 32-bit memory.
  @param PMem32Node Resource info node for 32-bit PMemory.
  @param Mem64Node  Resource info node for 64-bit memory.
  @param PMem64Node Resource info node for 64-bit PMemory.
  
  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
ResourcePaddingForCardBusBridge (
  PCI_IO_DEVICE     *PciDev,
  PCI_RESOURCE_NODE *IoNode,
  PCI_RESOURCE_NODE *Mem32Node,
  PCI_RESOURCE_NODE *PMem32Node,
  PCI_RESOURCE_NODE *Mem64Node,
  PCI_RESOURCE_NODE *PMem64Node
  );

/**
  Program P2C register for given resource node.
  
  @param Base    Base address of P2C device.
  @param Node    Given resource node.
  
  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
ProgramP2C (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Node
  );

/**
  Create padding resource node.
  
  @param PciDev     Pci device instance.
  @param IoNode     Resource info node for IO. 
  @param Mem32Node  Resource info node for 32-bit memory.
  @param PMem32Node Resource info node for 32-bit PMemory.
  @param Mem64Node  Resource info node for 64-bit memory.
  @param PMem64Node Resource info node for 64-bit PMemory.
  
  @retval EFI_SUCCESS Success

**/
EFI_STATUS
ApplyResourcePadding (
  PCI_IO_DEVICE     *PciDev,
  PCI_RESOURCE_NODE *IoNode,
  PCI_RESOURCE_NODE *Mem32Node,
  PCI_RESOURCE_NODE *PMem32Node,
  PCI_RESOURCE_NODE *Mem64Node,
  PCI_RESOURCE_NODE *PMem64Node
  );

/**
  Get padding resource for PPB
  Light PCI bus driver woundn't support hotplug root device
  So no need to pad resource for them.

  @param   PciIoDevice Pci device instance.
**/
VOID
GetResourcePaddingPpb (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  );

/**
  Reset and all bus number from specific bridge.
  
  @param Bridge           Parent specific bridge.
  @param StartBusNumber   start bus number.
**/
EFI_STATUS
ResetAllPpbBusNumber (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber
  );

#endif

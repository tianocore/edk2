/** @file
  PCI resouces support functions declaration for PCI Bus module.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_PCI_RESOURCE_SUPPORT_H_
#define _EFI_PCI_RESOURCE_SUPPORT_H_

typedef enum {
  PciResUsageTypical,
  PciResUsagePadding
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
  BOOLEAN             Virtual;
} PCI_RESOURCE_NODE;

#define RESOURCE_NODE_FROM_LINK(a) \
  CR (a, PCI_RESOURCE_NODE, Link, PCI_RESOURCE_SIGNATURE)

/**
  The function is used to skip VGA range.

  @param Start    Returned start address including VGA range.
  @param Length   The length of VGA range.

**/
VOID
SkipVGAAperture (
  OUT UINT64   *Start,
  IN  UINT64   Length
  );

/**
  This function is used to skip ISA aliasing aperture.

  @param Start    Returned start address including ISA aliasing aperture.
  @param Length   The length of ISA aliasing aperture.

**/
VOID
SkipIsaAliasAperture (
  OUT UINT64   *Start,
  IN  UINT64   Length
  );

/**
  This function inserts a resource node into the resource list.
  The resource list is sorted in descend order.

  @param Bridge  PCI resource node for bridge.
  @param ResNode Resource node want to be inserted.

**/
VOID
InsertResourceNode (
  IN OUT PCI_RESOURCE_NODE   *Bridge,
  IN     PCI_RESOURCE_NODE   *ResNode
  );

/**
  This routine is used to merge two different resource trees in need of
  resoure degradation.

  For example, if an upstream PPB doesn't support,
  prefetchable memory decoding, the PCI bus driver will choose to call this function
  to merge prefectchable memory resource list into normal memory list.

  If the TypeMerge is TRUE, Res resource type is changed to the type of destination resource
  type.
  If Dst is NULL or Res is NULL, ASSERT ().

  @param Dst        Point to destination resource tree.
  @param Res        Point to source resource tree.
  @param TypeMerge  If the TypeMerge is TRUE, Res resource type is changed to the type of
                    destination resource type.

**/
VOID
MergeResourceTree (
  IN PCI_RESOURCE_NODE   *Dst,
  IN PCI_RESOURCE_NODE   *Res,
  IN BOOLEAN             TypeMerge
  );

/**
  This function is used to calculate the IO16 aperture
  for a bridge.

  @param Bridge    PCI resource node for bridge.

**/
VOID
CalculateApertureIo16 (
  IN PCI_RESOURCE_NODE    *Bridge
  );

/**
  This function is used to calculate the resource aperture
  for a given bridge device.

  @param Bridge      PCI resouce node for given bridge device.

**/
VOID
CalculateResourceAperture (
  IN PCI_RESOURCE_NODE    *Bridge
  );

/**
  Get IO/Memory resource infor for given PCI device.

  @param PciDev     Pci device instance.
  @param IoNode     Resource info node for IO .
  @param Mem32Node  Resource info node for 32-bit memory.
  @param PMem32Node Resource info node for 32-bit Prefetchable Memory.
  @param Mem64Node  Resource info node for 64-bit memory.
  @param PMem64Node Resource info node for 64-bit Prefetchable Memory.

**/
VOID
GetResourceFromDevice (
  IN     PCI_IO_DEVICE     *PciDev,
  IN OUT PCI_RESOURCE_NODE *IoNode,
  IN OUT PCI_RESOURCE_NODE *Mem32Node,
  IN OUT PCI_RESOURCE_NODE *PMem32Node,
  IN OUT PCI_RESOURCE_NODE *Mem64Node,
  IN OUT PCI_RESOURCE_NODE *PMem64Node
  );

/**
  This function is used to create a resource node.

  @param PciDev       Pci device instance.
  @param Length       Length of Io/Memory resource.
  @param Alignment    Alignment of resource.
  @param Bar          Bar index.
  @param ResType      Type of resource: IO/Memory.
  @param ResUsage     Resource usage.

  @return PCI resource node created for given PCI device.
          NULL means PCI resource node is not created.

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
  This function is used to extract resource request from
  IOV VF device node list.

  @param PciDev       Pci device instance.
  @param Length       Length of Io/Memory resource.
  @param Alignment    Alignment of resource.
  @param Bar          Bar index.
  @param ResType      Type of resource: IO/Memory.
  @param ResUsage     Resource usage.

  @return PCI resource node created for given PCI device.
          NULL means PCI resource node is not created.

**/
PCI_RESOURCE_NODE *
CreateVfResourceNode (
  IN PCI_IO_DEVICE         *PciDev,
  IN UINT64                Length,
  IN UINT64                Alignment,
  IN UINT8                 Bar,
  IN PCI_BAR_TYPE          ResType,
  IN PCI_RESOURCE_USAGE    ResUsage
  );

/**
  This function is used to extract resource request from
  device node list.

  @param Bridge     Pci device instance.
  @param IoNode     Resource info node for IO.
  @param Mem32Node  Resource info node for 32-bit memory.
  @param PMem32Node Resource info node for 32-bit Prefetchable Memory.
  @param Mem64Node  Resource info node for 64-bit memory.
  @param PMem64Node Resource info node for 64-bit Prefetchable Memory.

**/
VOID
CreateResourceMap (
  IN     PCI_IO_DEVICE     *Bridge,
  IN OUT PCI_RESOURCE_NODE *IoNode,
  IN OUT PCI_RESOURCE_NODE *Mem32Node,
  IN OUT PCI_RESOURCE_NODE *PMem32Node,
  IN OUT PCI_RESOURCE_NODE *Mem64Node,
  IN OUT PCI_RESOURCE_NODE *PMem64Node
  );

/**
  This function is used to do the resource padding for a specific platform.

  @param PciDev     Pci device instance.
  @param IoNode     Resource info node for IO.
  @param Mem32Node  Resource info node for 32-bit memory.
  @param PMem32Node Resource info node for 32-bit Prefetchable Memory.
  @param Mem64Node  Resource info node for 64-bit memory.
  @param PMem64Node Resource info node for 64-bit Prefetchable Memory.

**/
VOID
ResourcePaddingPolicy (
  IN PCI_IO_DEVICE     *PciDev,
  IN PCI_RESOURCE_NODE *IoNode,
  IN PCI_RESOURCE_NODE *Mem32Node,
  IN PCI_RESOURCE_NODE *PMem32Node,
  IN PCI_RESOURCE_NODE *Mem64Node,
  IN PCI_RESOURCE_NODE *PMem64Node
  );

/**
  This function is used to degrade resource if the upstream bridge
  doesn't support certain resource. Degradation path is
  PMEM64 -> MEM64  -> MEM32
  PMEM64 -> PMEM32 -> MEM32
  IO32   -> IO16.

  @param Bridge     Pci device instance.
  @param Mem32Node  Resource info node for 32-bit memory.
  @param PMem32Node Resource info node for 32-bit Prefetchable Memory.
  @param Mem64Node  Resource info node for 64-bit memory.
  @param PMem64Node Resource info node for 64-bit Prefetchable Memory.

**/
VOID
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

  @return TRUE     The bridge device support decode resource.
  @return FALSE    The bridge device don't support decode resource.

**/
BOOLEAN
BridgeSupportResourceDecode (
  IN PCI_IO_DEVICE *Bridge,
  IN UINT32        Decode
  );

/**
  This function is used to program the resource allocated
  for each resource node under specified bridge.

  @param Base     Base address of resource to be progammed.
  @param Bridge   PCI resource node for the bridge device.

  @retval EFI_SUCCESS            Successfully to program all resouces
                                 on given PCI bridge device.
  @retval EFI_OUT_OF_RESOURCES   Base is all one.

**/
EFI_STATUS
ProgramResource (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Bridge
  );

/**
  Program Bar register for PCI device.

  @param Base  Base address for PCI device resource to be progammed.
  @param Node  Point to resoure node structure.

**/
VOID
ProgramBar (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Node
  );

/**
  Program IOV VF Bar register for PCI device.

  @param Base  Base address for PCI device resource to be progammed.
  @param Node  Point to resoure node structure.

**/
EFI_STATUS
ProgramVfBar (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Node
  );

/**
  Program PCI-PCI bridge apperture.

  @param Base  Base address for resource.
  @param Node  Point to resoure node structure.

**/
VOID
ProgramPpbApperture (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Node
  );

/**
  Program parent bridge for Option Rom.

  @param PciDevice      Pci deivce instance.
  @param OptionRomBase  Base address for Optiona Rom.
  @param Enable         Enable or disable PCI memory.

**/
VOID
ProgrameUpstreamBridgeForRom (
  IN PCI_IO_DEVICE   *PciDevice,
  IN UINT32          OptionRomBase,
  IN BOOLEAN         Enable
  );

/**
  Test whether resource exists for a bridge.

  @param Bridge  Point to resource node for a bridge.

  @retval TRUE   There is resource on the given bridge.
  @retval FALSE  There isn't resource on the given bridge.

**/
BOOLEAN
ResourceRequestExisted (
  IN PCI_RESOURCE_NODE    *Bridge
  );

/**
  Initialize resource pool structure.

  @param ResourcePool Point to resource pool structure. This pool
                      is reset to all zero when returned.
  @param ResourceType Type of resource.

**/
VOID
InitializeResourcePool (
  IN OUT PCI_RESOURCE_NODE   *ResourcePool,
  IN     PCI_BAR_TYPE        ResourceType
  );

/**
  Destory given resource tree.

  @param Bridge  PCI resource root node of resource tree.

**/
VOID
DestroyResourceTree (
  IN PCI_RESOURCE_NODE *Bridge
  );

/**
  Insert resource padding for P2C.

  @param PciDev     Pci device instance.
  @param IoNode     Resource info node for IO.
  @param Mem32Node  Resource info node for 32-bit memory.
  @param PMem32Node Resource info node for 32-bit Prefetchable Memory.
  @param Mem64Node  Resource info node for 64-bit memory.
  @param PMem64Node Resource info node for 64-bit Prefetchable Memory.

**/
VOID
ResourcePaddingForCardBusBridge (
  IN PCI_IO_DEVICE        *PciDev,
  IN PCI_RESOURCE_NODE    *IoNode,
  IN PCI_RESOURCE_NODE    *Mem32Node,
  IN PCI_RESOURCE_NODE    *PMem32Node,
  IN PCI_RESOURCE_NODE    *Mem64Node,
  IN PCI_RESOURCE_NODE    *PMem64Node
  );

/**
  Program PCI Card device register for given resource node.

  @param Base    Base address of PCI Card device to be programmed.
  @param Node    Given resource node.

**/
VOID
ProgramP2C (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Node
  );

/**
  Create padding resource node.

  @param PciDev     Pci device instance.
  @param IoNode     Resource info node for IO.
  @param Mem32Node  Resource info node for 32-bit memory.
  @param PMem32Node Resource info node for 32-bit Prefetchable Memory.
  @param Mem64Node  Resource info node for 64-bit memory.
  @param PMem64Node Resource info node for 64-bit Prefetchable Memory.

**/
VOID
ApplyResourcePadding (
  IN PCI_IO_DEVICE         *PciDev,
  IN PCI_RESOURCE_NODE     *IoNode,
  IN PCI_RESOURCE_NODE     *Mem32Node,
  IN PCI_RESOURCE_NODE     *PMem32Node,
  IN PCI_RESOURCE_NODE     *Mem64Node,
  IN PCI_RESOURCE_NODE     *PMem64Node
  );

/**
  Get padding resource for PCI-PCI bridge.

  @param  PciIoDevice     PCI-PCI bridge device instance.

  @note   Feature flag PcdPciBusHotplugDeviceSupport determines
          whether need to pad resource for them.
**/
VOID
GetResourcePaddingPpb (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  );

#endif

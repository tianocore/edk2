/**@file

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PciBus.h"
#include "PciResourceSupport.h"
#include "PciCommand.h"

/**
  The function is used to skip VGA range
  
  @param Start    address including VGA range
  @param Length   length of VGA range.
  
  @retval EFI_SUCCESS success
**/
EFI_STATUS
SkipVGAAperture (
  OUT UINT64   *Start,
  IN  UINT64   Length
  )
{
  UINT64  Original;
  UINT64  Mask;
  UINT64  StartOffset;
  UINT64  LimitOffset;

  //
  // For legacy VGA, bit 10 to bit 15 is not decoded
  //
  Mask        = 0x3FF;

  Original    = *Start;
  StartOffset = Original & Mask;
  LimitOffset = ((*Start) + Length - 1) & Mask;
  if (LimitOffset >= VGABASE1) {
    *Start = *Start - StartOffset + VGALIMIT2 + 1;
  }

  return EFI_SUCCESS;
}

/**
  This function is used to skip ISA aliasing aperture
  
  @param Start    address including ISA aliasing aperture
  @param Length   length of ISA aliasing aperture
  
  @retval EFI_SUCCESS success
**/
EFI_STATUS
SkipIsaAliasAperture (
  OUT UINT64   *Start,
  IN  UINT64   Length
  )
{

  UINT64  Original;
  UINT64  Mask;
  UINT64  StartOffset;
  UINT64  LimitOffset;

  //
  // For legacy ISA, bit 10 to bit 15 is not decoded
  //
  Mask        = 0x3FF;

  Original    = *Start;
  StartOffset = Original & Mask;
  LimitOffset = ((*Start) + Length - 1) & Mask;

  if (LimitOffset >= ISABASE) {
    *Start = *Start - StartOffset + ISALIMIT + 1;
  }

  return EFI_SUCCESS;
}

/**
  This function inserts a resource node into the resource list.
  The resource list is sorted in descend order.

  @param Bridge  PCI resource node for bridge
  @param ResNode Resource node want to be inserted
  
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
InsertResourceNode (
  PCI_RESOURCE_NODE *Bridge,
  PCI_RESOURCE_NODE *ResNode
  )
{
  LIST_ENTRY        *CurrentLink;
  PCI_RESOURCE_NODE *Temp;
  UINT64            ResNodeAlignRest;
  UINT64            TempAlignRest;

  InsertHeadList (&Bridge->ChildList, &ResNode->Link);

  CurrentLink = Bridge->ChildList.ForwardLink->ForwardLink;
  while (CurrentLink != &Bridge->ChildList) {
    Temp = RESOURCE_NODE_FROM_LINK (CurrentLink);

    if (ResNode->Alignment > Temp->Alignment) {
      break;
    } else if (ResNode->Alignment == Temp->Alignment) {
      ResNodeAlignRest  = ResNode->Length & ResNode->Alignment;
      TempAlignRest     = Temp->Length & Temp->Alignment;
      if ((ResNodeAlignRest == 0) || (ResNodeAlignRest >= TempAlignRest)) {
        break;
      }
    }

    SwapListEntries (&ResNode->Link, CurrentLink);

    CurrentLink = ResNode->Link.ForwardLink;
  }

  return EFI_SUCCESS;
}

/**

Routine Description:

  This routine is used to merge two different resource tree in need of
  resoure degradation. For example, if a upstream PPB doesn't support,
  prefetchable memory decoding, the PCI bus driver will choose to call this function
  to merge prefectchable memory resource list into normal memory list.

  If the TypeMerge is TRUE, Res resource type is changed to the type of destination resource
  type.

  @param Dst        Point to destination resource tree
  @param Res        Point to source resource tree
  @param TypeMerge  If the TypeMerge is TRUE, Res resource type is changed to the type of 
                    destination resource type.
                    
                    
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
MergeResourceTree (
  PCI_RESOURCE_NODE *Dst,
  PCI_RESOURCE_NODE *Res,
  BOOLEAN           TypeMerge
  )
{

  LIST_ENTRY        *CurrentLink;
  PCI_RESOURCE_NODE *Temp;

  while (!IsListEmpty (&Res->ChildList)) {
    CurrentLink = Res->ChildList.ForwardLink;

    Temp        = RESOURCE_NODE_FROM_LINK (CurrentLink);

    if (TypeMerge) {
      Temp->ResType = Dst->ResType;
    }

    RemoveEntryList (CurrentLink);
    InsertResourceNode (Dst, Temp);

  }

  return EFI_SUCCESS;
}

/**
  This function is used to calculate the IO16 aperture
  for a bridge.

  @param Bridge PCI resource node for bridge.
  
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
CalculateApertureIo16 (
  IN PCI_RESOURCE_NODE *Bridge
  )
{

  UINT64            Aperture;
  LIST_ENTRY        *CurrentLink;
  PCI_RESOURCE_NODE *Node;
  UINT64            offset;
  BOOLEAN           IsaEnable;
  BOOLEAN           VGAEnable;

  //
  // Always assume there is ISA device and VGA device on the platform
  // will be customized later
  //
  IsaEnable = FALSE;
  VGAEnable = FALSE;

  if (FeaturePcdGet (PcdPciIsaEnable)){
    IsaEnable = TRUE;
  }

  if (FeaturePcdGet (PcdPciVgaEnable)){
    VGAEnable = TRUE;
  }

  Aperture = 0;

  if (!Bridge) {
    return EFI_SUCCESS;
  }

  CurrentLink = Bridge->ChildList.ForwardLink;

  //
  // Assume the bridge is aligned
  //
  while (CurrentLink != &Bridge->ChildList) {

    Node = RESOURCE_NODE_FROM_LINK (CurrentLink);

    //
    // Consider the aperture alignment
    //
    offset = Aperture & (Node->Alignment);

    if (offset) {

      Aperture = Aperture + (Node->Alignment + 1) - offset;

    }

    //
    // IsaEnable and VGAEnable can not be implemented now.
    // If both of them are enabled, then the IO resource would
    // become too limited to meet the requirement of most of devices.
    //

    if (IsaEnable || VGAEnable) {
      if (!IS_PCI_BRIDGE (&(Node->PciDev->Pci)) && !IS_CARDBUS_BRIDGE (&(Node->PciDev->Pci))) {
        //
        // Check if there is need to support ISA/VGA decoding
        // If so, we need to avoid isa/vga aliasing range
        //
        if (IsaEnable) {
          SkipIsaAliasAperture (
            &Aperture,
            Node->Length               
            );
          offset = Aperture & (Node->Alignment);
          if (offset) {
            Aperture = Aperture + (Node->Alignment + 1) - offset;
          }
        } else if (VGAEnable) {
          SkipVGAAperture (
            &Aperture,
            Node->Length
            );
          offset = Aperture & (Node->Alignment);
          if (offset) {
            Aperture = Aperture + (Node->Alignment + 1) - offset;
          }
        }
      }
    }

    Node->Offset = Aperture;

    //
    // Increment aperture by the length of node
    //
    Aperture += Node->Length;

    CurrentLink = CurrentLink->ForwardLink;
  }

  //
  // At last, adjust the aperture with the bridge's
  // alignment
  //
  offset = Aperture & (Bridge->Alignment);

  if (offset) {
    Aperture = Aperture + (Bridge->Alignment + 1) - offset;
  }

  Bridge->Length = Aperture;
  //
  // At last, adjust the bridge's alignment to the first child's alignment
  // if the bridge has at least one child
  //
  CurrentLink = Bridge->ChildList.ForwardLink;
  if (CurrentLink != &Bridge->ChildList) {
    Node = RESOURCE_NODE_FROM_LINK (CurrentLink);
    if (Node->Alignment > Bridge->Alignment) {
      Bridge->Alignment = Node->Alignment;
    }
  }

  return EFI_SUCCESS;
}

/**
  This function is used to calculate the resource aperture
  for a given bridge device

  @param Bridge Give bridge device
  
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
CalculateResourceAperture (
  IN PCI_RESOURCE_NODE *Bridge
  )
{
  UINT64            Aperture;
  LIST_ENTRY        *CurrentLink;
  PCI_RESOURCE_NODE *Node;

  UINT64            offset;

  Aperture = 0;

  if (!Bridge) {
    return EFI_SUCCESS;
  }

  if (Bridge->ResType == PciBarTypeIo16) {
    return CalculateApertureIo16 (Bridge);
  }

  CurrentLink = Bridge->ChildList.ForwardLink;

  //
  // Assume the bridge is aligned
  //
  while (CurrentLink != &Bridge->ChildList) {

    Node = RESOURCE_NODE_FROM_LINK (CurrentLink);

    //
    // Apply padding resource if available
    //
        
    offset = Aperture & (Node->Alignment);

    if (offset) {

      Aperture = Aperture + (Node->Alignment + 1) - offset;

    }

    //
    // Recode current aperture as a offset
    // this offset will be used in future real allocation
    //
    Node->Offset = Aperture;

    //
    // Increment aperture by the length of node
    //
    Aperture += Node->Length;

    //
    // Consider the aperture alignment
    //
    
    CurrentLink = CurrentLink->ForwardLink;
  }

  //
  // At last, adjust the aperture with the bridge's
  // alignment
  //
  offset = Aperture & (Bridge->Alignment);
  if (offset) {
    Aperture = Aperture + (Bridge->Alignment + 1) - offset;
  }

  //
  // If the bridge has already padded the resource and the
  // amount of padded resource is larger, then keep the
  // padded resource
  //
  if (Bridge->Length < Aperture) {
    Bridge->Length = Aperture;
  }
  
  //
  // At last, adjust the bridge's alignment to the first child's alignment
  // if the bridge has at least one child
  //
  CurrentLink = Bridge->ChildList.ForwardLink;
  if (CurrentLink != &Bridge->ChildList) {
    Node = RESOURCE_NODE_FROM_LINK (CurrentLink);
    if (Node->Alignment > Bridge->Alignment) {
      Bridge->Alignment = Node->Alignment;
    }
  }

  return EFI_SUCCESS;
}

/**
  Get IO/Memory resource infor for given PCI device
  
  @param PciDev     Pci device instance
  @param IoNode     Resource info node for IO 
  @param Mem32Node  Resource info node for 32-bit memory
  @param PMem32Node Resource info node for 32-bit PMemory
  @param Mem64Node  Resource info node for 64-bit memory
  @param PMem64Node Resource info node for 64-bit PMemory
  
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
GetResourceFromDevice (
  PCI_IO_DEVICE     *PciDev,
  PCI_RESOURCE_NODE *IoNode,
  PCI_RESOURCE_NODE *Mem32Node,
  PCI_RESOURCE_NODE *PMem32Node,
  PCI_RESOURCE_NODE *Mem64Node,
  PCI_RESOURCE_NODE *PMem64Node
  )
{

  UINT8             Index;
  PCI_RESOURCE_NODE *Node;
  BOOLEAN           ResourceRequested;

  Node              = NULL;
  ResourceRequested = FALSE;

  for (Index = 0; Index < PCI_MAX_BAR; Index++) {

    switch ((PciDev->PciBar)[Index].BarType) {

    case PciBarTypeMem32:

      Node = CreateResourceNode (
              PciDev,
              (PciDev->PciBar)[Index].Length,
              (PciDev->PciBar)[Index].Alignment,
              Index,
              PciBarTypeMem32,
              PciResUsageTypical
              );

      InsertResourceNode (
        Mem32Node,
        Node
        );

      ResourceRequested = TRUE;
      break;

    case PciBarTypeMem64:

      Node = CreateResourceNode (
              PciDev,
              (PciDev->PciBar)[Index].Length,
              (PciDev->PciBar)[Index].Alignment,
              Index,
              PciBarTypeMem64,
              PciResUsageTypical
              );

      InsertResourceNode (
        Mem64Node,
        Node
        );

      ResourceRequested = TRUE;
      break;

    case PciBarTypePMem64:

      Node = CreateResourceNode (
              PciDev,
              (PciDev->PciBar)[Index].Length,
              (PciDev->PciBar)[Index].Alignment,
              Index,
              PciBarTypePMem64,
              PciResUsageTypical
              );

      InsertResourceNode (
        PMem64Node,
        Node
        );

      ResourceRequested = TRUE;
      break;

    case PciBarTypePMem32:

      Node = CreateResourceNode (
              PciDev,
              (PciDev->PciBar)[Index].Length,
              (PciDev->PciBar)[Index].Alignment,
              Index,
              PciBarTypePMem32,
              PciResUsageTypical
              );

      InsertResourceNode (
        PMem32Node,
        Node
        );
      ResourceRequested = TRUE;
      break;

    case PciBarTypeIo16:
    case PciBarTypeIo32:

      Node = CreateResourceNode (
              PciDev,
              (PciDev->PciBar)[Index].Length,
              (PciDev->PciBar)[Index].Alignment,
              Index,
              PciBarTypeIo16,
              PciResUsageTypical
              );

      InsertResourceNode (
        IoNode,
        Node
        );
      ResourceRequested = TRUE;
      break;

    case PciBarTypeUnknown:
      break;

    default:
      break;
    }
  }

  //
  // If there is no resource requested from this device,
  // then we indicate this device has been allocated naturally.
  //
  if (!ResourceRequested) {
    PciDev->Allocated = TRUE;
  }

  return EFI_SUCCESS;
}

/**
  This function is used to create a resource node

  @param PciDev       Pci device instance
  @param Length       Length of Io/Memory resource
  @param Alignment    Alignment of resource
  @param Bar          Bar index 
  @param ResType      Type of resource: IO/Memory
  @param ResUage      Resource usage
**/
PCI_RESOURCE_NODE *
CreateResourceNode (
  IN PCI_IO_DEVICE         *PciDev,
  IN UINT64                Length,
  IN UINT64                Alignment,
  IN UINT8                 Bar,
  IN PCI_BAR_TYPE          ResType,
  IN PCI_RESOURCE_USAGE    ResUsage
  )
{
  PCI_RESOURCE_NODE *Node;

  Node    = NULL;

  Node    = AllocatePool (sizeof (PCI_RESOURCE_NODE));
  ASSERT (Node != NULL);
  if (Node == NULL) {
    return NULL;
  }

  ZeroMem (Node, sizeof (PCI_RESOURCE_NODE));

  Node->Signature     = PCI_RESOURCE_SIGNATURE;
  Node->PciDev        = PciDev;
  Node->Length        = Length;
  Node->Alignment     = Alignment;
  Node->Bar           = Bar;
  Node->ResType       = ResType;
  Node->Reserved      = FALSE;
  Node->ResourceUsage = ResUsage;
  InitializeListHead (&Node->ChildList);
  return Node;
}

/**
  This routine is used to extract resource request from
  device node list.

  @param Bridge     Pci device instance
  @param IoNode     Resource info node for IO 
  @param Mem32Node  Resource info node for 32-bit memory
  @param PMem32Node Resource info node for 32-bit PMemory
  @param Mem64Node  Resource info node for 64-bit memory
  @param PMem64Node Resource info node for 64-bit PMemory

  @retval EFI_SUCCESS Success
**/
EFI_STATUS
CreateResourceMap (
  IN PCI_IO_DEVICE     *Bridge,
  IN PCI_RESOURCE_NODE *IoNode,
  IN PCI_RESOURCE_NODE *Mem32Node,
  IN PCI_RESOURCE_NODE *PMem32Node,
  IN PCI_RESOURCE_NODE *Mem64Node,
  IN PCI_RESOURCE_NODE *PMem64Node
  )
{
  PCI_IO_DEVICE     *Temp;
  PCI_RESOURCE_NODE *IoBridge;
  PCI_RESOURCE_NODE *Mem32Bridge;
  PCI_RESOURCE_NODE *PMem32Bridge;
  PCI_RESOURCE_NODE *Mem64Bridge;
  PCI_RESOURCE_NODE *PMem64Bridge;
  LIST_ENTRY        *CurrentLink;

  CurrentLink = Bridge->ChildList.ForwardLink;

  while (CurrentLink && CurrentLink != &Bridge->ChildList) {

    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);

    //
    // Create resource nodes for this device by scanning the
    // Bar array in the device private data
    // If the upstream bridge doesn't support this device,
    // no any resource node will be created for this device
    //
    GetResourceFromDevice (
      Temp,
      IoNode,
      Mem32Node,
      PMem32Node,
      Mem64Node,
      PMem64Node
      );

    if (IS_PCI_BRIDGE (&Temp->Pci)) {

      //
      // If the device has children, create a bridge resource node for this PPB
      // Note: For PPB, memory aperture is aligned with 1MB and IO aperture
      // is aligned with 4KB
      // This device is typically a bridge device like PPB and P2C
      //
      IoBridge = CreateResourceNode (
                  Temp,
                  0,
                  0xFFF,
                  PPB_IO_RANGE,
                  PciBarTypeIo16,
                  PciResUsageTypical
                  ); //0x1000 aligned
      
      Mem32Bridge = CreateResourceNode (
                      Temp,
                      0,
                      0xFFFFF,
                      PPB_MEM32_RANGE,
                      PciBarTypeMem32,
                      PciResUsageTypical
                      );

      PMem32Bridge = CreateResourceNode (
                      Temp,
                      0,
                      0xFFFFF,
                      PPB_PMEM32_RANGE,
                      PciBarTypePMem32,
                      PciResUsageTypical
                      );

      Mem64Bridge = CreateResourceNode (
                      Temp,
                      0,
                      0xFFFFF,
                      PPB_MEM64_RANGE,
                      PciBarTypeMem64,
                      PciResUsageTypical
                      );

      PMem64Bridge = CreateResourceNode (
                      Temp,
                      0,
                      0xFFFFF,
                      PPB_PMEM64_RANGE,
                      PciBarTypePMem64,
                      PciResUsageTypical
                      );

      //
      // Recursively create resouce map on this bridge
      //
      CreateResourceMap (
        Temp,
        IoBridge,
        Mem32Bridge,
        PMem32Bridge,
        Mem64Bridge,
        PMem64Bridge
        );

      if (ResourceRequestExisted (IoBridge)) {
        InsertResourceNode (
          IoNode,
          IoBridge
          );
      } else {
        gBS->FreePool (IoBridge);
        IoBridge = NULL;
      }

      //
      // If there is node under this resource bridge,
      // then calculate bridge's aperture of this type
      // and insert it into the respective resource tree.
      // If no, delete this resource bridge
      //
      if (ResourceRequestExisted (Mem32Bridge)) {
        InsertResourceNode (
          Mem32Node,
          Mem32Bridge
          );
      } else {
        gBS->FreePool (Mem32Bridge);
        Mem32Bridge = NULL;
      }

      //
      // If there is node under this resource bridge,
      // then calculate bridge's aperture of this type
      // and insert it into the respective resource tree.
      // If no, delete this resource bridge
      //
      if (ResourceRequestExisted (PMem32Bridge)) {
        InsertResourceNode (
          PMem32Node,
          PMem32Bridge
          );
      } else {
        gBS->FreePool (PMem32Bridge);
        PMem32Bridge = NULL;
      }

      //
      // If there is node under this resource bridge,
      // then calculate bridge's aperture of this type
      // and insert it into the respective resource tree.
      // If no, delete this resource bridge
      //
      if (ResourceRequestExisted (Mem64Bridge)) {
        InsertResourceNode (
          Mem64Node,
          Mem64Bridge
          );
      } else {
        gBS->FreePool (Mem64Bridge);
        Mem64Bridge = NULL;
      }

      //
      // If there is node under this resource bridge,
      // then calculate bridge's aperture of this type
      // and insert it into the respective resource tree.
      // If no, delete this resource bridge
      //
      if (ResourceRequestExisted (PMem64Bridge)) {
        InsertResourceNode (
          PMem64Node,
          PMem64Bridge
          );
      } else {
        gBS->FreePool (PMem64Bridge);
        PMem64Bridge = NULL;
      }

    }

    //
    // If it is P2C, apply hard coded resource padding
    //
    //
    if (IS_CARDBUS_BRIDGE (&Temp->Pci)) {
      ResourcePaddingForCardBusBridge (
        Temp,
        IoNode,
        Mem32Node,
        PMem32Node,
        Mem64Node,
        PMem64Node
        );
    }

    CurrentLink = CurrentLink->ForwardLink;
  }
  //
  //
  // To do some platform specific resource padding ...
  //
  ResourcePaddingPolicy (
    Bridge,
    IoNode,
    Mem32Node,
    PMem32Node,
    Mem64Node,
    PMem64Node
    );

  //
  // Degrade resource if necessary
  //
  DegradeResource (
    Bridge,
    Mem32Node,
    PMem32Node,
    Mem64Node,
    PMem64Node
    );

  //
  // Calculate resource aperture for this bridge device
  //
  CalculateResourceAperture (Mem32Node);
  CalculateResourceAperture (PMem32Node);
  CalculateResourceAperture (Mem64Node);
  CalculateResourceAperture (PMem64Node);
  CalculateResourceAperture (IoNode);

  return EFI_SUCCESS;

}

/**
  This function is used to do the resource padding for a specific platform

  @param Bridge     Pci device instance
  @param IoNode     Resource info node for IO 
  @param Mem32Node  Resource info node for 32-bit memory
  @param PMem32Node Resource info node for 32-bit PMemory
  @param Mem64Node  Resource info node for 64-bit memory
  @param PMem64Node Resource info node for 64-bit PMemory

  @retval EFI_SUCCESS Success
**/
EFI_STATUS
ResourcePaddingPolicy (
  PCI_IO_DEVICE     *PciDev,
  PCI_RESOURCE_NODE *IoNode,
  PCI_RESOURCE_NODE *Mem32Node,
  PCI_RESOURCE_NODE *PMem32Node,
  PCI_RESOURCE_NODE *Mem64Node,
  PCI_RESOURCE_NODE *PMem64Node
  )
{
  //
  // Create padding resource node
  //
  if (PciDev->ResourcePaddingDescriptors != NULL) {
    ApplyResourcePadding (
      PciDev,
      IoNode,
      Mem32Node,
      PMem32Node,
      Mem64Node,
      PMem64Node
      );
  }

  return EFI_SUCCESS;

}

/**
  This function is used to degrade resource if the upstream bridge 
  doesn't support certain resource. Degradation path is 
  PMEM64 -> MEM64  -> MEM32
  PMEM64 -> PMEM32 -> MEM32
  IO32   -> IO16

  @param Bridge     Pci device instance
  @param IoNode     Resource info node for IO 
  @param Mem32Node  Resource info node for 32-bit memory
  @param PMem32Node Resource info node for 32-bit PMemory
  @param Mem64Node  Resource info node for 64-bit memory
  @param PMem64Node Resource info node for 64-bit PMemory

  @retval EFI_SUCCESS Success
**/
EFI_STATUS
DegradeResource (
  IN PCI_IO_DEVICE     *Bridge,
  IN PCI_RESOURCE_NODE *Mem32Node,
  IN PCI_RESOURCE_NODE *PMem32Node,
  IN PCI_RESOURCE_NODE *Mem64Node,
  IN PCI_RESOURCE_NODE *PMem64Node
  )
{

  //
  // If bridge doesn't support Prefetchable
  // memory64, degrade it to Prefetchable memory32
  //
  if (!BridgeSupportResourceDecode (Bridge, EFI_BRIDGE_PMEM64_DECODE_SUPPORTED)) {
    MergeResourceTree (
      PMem32Node,
      PMem64Node,
      TRUE
      );
  } else {
    //
    // if no PMem32 request, still keep PMem64. Otherwise degrade to PMem32
    //
    if (PMem32Node != NULL && PMem32Node->Length != 0 && Bridge->Parent != NULL ) { 
      //
      // Fixed the issue that there is no resource for 64-bit (above 4G)
      //
      MergeResourceTree (
        PMem32Node,
        PMem64Node,
        TRUE
        );
    }
  }


  //
  // If bridge doesn't support Mem64
  // degrade it to mem32
  //
  if (!BridgeSupportResourceDecode (Bridge, EFI_BRIDGE_MEM64_DECODE_SUPPORTED)) {
    MergeResourceTree (
      Mem32Node,
      Mem64Node,
      TRUE
      );
  }

  //
  // If bridge doesn't support Pmem32
  // degrade it to mem32
  //
  if (!BridgeSupportResourceDecode (Bridge, EFI_BRIDGE_PMEM32_DECODE_SUPPORTED)) {
    MergeResourceTree (
      Mem32Node,
      PMem32Node,
      TRUE
      );
  }

  //
  // if bridge supports combined Pmem Mem decoding
  // merge these two type of resource
  //
  if (BridgeSupportResourceDecode (Bridge, EFI_BRIDGE_PMEM_MEM_COMBINE_SUPPORTED)) {
    MergeResourceTree (
      Mem32Node,
      PMem32Node,
      FALSE
      );

    MergeResourceTree (
      Mem64Node,
      PMem64Node,
      FALSE
      );
  }

  return EFI_SUCCESS;
}

/**
  Test whether bridge device support decode resource
  
  @param Bridge    Bridge device instance
  @param Decode    Decode type according to resource type
  
  @return whether bridge device support decode resource
  
**/
BOOLEAN
BridgeSupportResourceDecode (
  IN PCI_IO_DEVICE *Bridge,
  IN UINT32        Decode
  )
{

  if ((Bridge->Decodes) & Decode) {
    return TRUE;
  }

  return FALSE;
}

/**
  This function is used to program the resource allocated 
  for each resource node

  
  @param Base     Base address of resource
  @param Bridge   Bridge device instance
  
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
ProgramResource (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Bridge
  )
{
  LIST_ENTRY        *CurrentLink;
  PCI_RESOURCE_NODE *Node;
  EFI_STATUS        Status;

  if (Base == gAllOne) {
    return EFI_OUT_OF_RESOURCES;
  }

  CurrentLink = Bridge->ChildList.ForwardLink;

  while (CurrentLink != &Bridge->ChildList) {

    Node = RESOURCE_NODE_FROM_LINK (CurrentLink);

    if (!IS_PCI_BRIDGE (&(Node->PciDev->Pci))) {

      if (IS_CARDBUS_BRIDGE (&(Node->PciDev->Pci))) {
        ProgramP2C (Base, Node);
      } else {
        ProgramBar (Base, Node);
      }
    } else {
      Status = ProgramResource (Base + Node->Offset, Node);

      if (EFI_ERROR (Status)) {
        return Status;
      }

      ProgramPpbApperture (Base, Node);
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return EFI_SUCCESS;
}

/**
  Program Bar register.
  
  @param Base  Base address for resource
  @param Node  Point to resoure node structure
  
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
ProgramBar (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Node
  )
{
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT64              Address;
  UINT32              Address32;

  Address = 0;
  PciIo   = &(Node->PciDev->PciIo);

  Address = Base + Node->Offset;

  //
  // Indicate pci bus driver has allocated
  // resource for this device
  // It might be a temporary solution here since
  // pci device could have multiple bar
  //
  Node->PciDev->Allocated = TRUE;

  switch ((Node->PciDev->PciBar[Node->Bar]).BarType) {

  case PciBarTypeIo16:
  case PciBarTypeIo32:
  case PciBarTypeMem32:
  case PciBarTypePMem32:

    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                (Node->PciDev->PciBar[Node->Bar]).Offset,
                1,
                &Address
                );

    Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;

    break;

  case PciBarTypeMem64:
  case PciBarTypePMem64:

    Address32 = (UINT32) (Address & 0x00000000FFFFFFFF);

    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                (Node->PciDev->PciBar[Node->Bar]).Offset,
                1,
                &Address32
                );

    Address32 = (UINT32) RShiftU64 (Address, 32);

    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                (UINT8) ((Node->PciDev->PciBar[Node->Bar]).Offset + 4),
                1,
                &Address32
                );

    Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;

    break;

  default:
    break;
  }

  return EFI_SUCCESS;
}

/**
  Program PPB apperture
  
  @param Base  Base address for resource
  @param Node  Point to resoure node structure
  
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
ProgramPpbApperture (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Node
  )
{
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT64              Address;
  UINT32              Address32;

  Address = 0;
  //
  // if no device south of this PPB, return anyway
  // Apperture is set default in the initialization code
  //
  if (Node->Length == 0 || Node->ResourceUsage == PciResUsagePadding) {
    //
    // For padding resource node, just ignore when programming
    //
    return EFI_SUCCESS;
  }

  PciIo   = &(Node->PciDev->PciIo);
  Address = Base + Node->Offset;

  //
  // Indicate the PPB resource has been allocated
  //
  Node->PciDev->Allocated = TRUE;

  switch (Node->Bar) {

  case PPB_BAR_0:
  case PPB_BAR_1:
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                (Node->PciDev->PciBar[Node->Bar]).Offset,
                1,
                &Address
                );

    Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
    Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;

    break;

  case PPB_IO_RANGE:

    Address32 = ((UINT32) (Address)) >> 8;
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint8,
                0x1C,
                1,
                &Address32
                );

    Address32 >>= 8;
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint16,
                0x30,
                1,
                &Address32
                );

    Address32 = (UINT32) (Address + Node->Length - 1);
    Address32 = ((UINT32) (Address32)) >> 8;
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint8,
                0x1D,
                1,
                &Address32
                );

    Address32 >>= 8;
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint16,
                0x32,
                1,
                &Address32
                );

    Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
    Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;
    break;

  case PPB_MEM32_RANGE:

    Address32 = ((UINT32) (Address)) >> 16;
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint16,
                0x20,
                1,
                &Address32
                );

    Address32 = (UINT32) (Address + Node->Length - 1);
    Address32 = ((UINT32) (Address32)) >> 16;
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint16,
                0x22,
                1,
                &Address32
                );

    Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
    Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;
    break;

  case PPB_PMEM32_RANGE:
  case PPB_PMEM64_RANGE:

    Address32 = ((UINT32) (Address)) >> 16;
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint16,
                0x24,
                1,
                &Address32
                );

    Address32 = (UINT32) (Address + Node->Length - 1);
    Address32 = ((UINT32) (Address32)) >> 16;
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint16,
                0x26,
                1,
                &Address32
                );

    Address32 = (UINT32) RShiftU64 (Address, 32);
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                0x28,
                1,
                &Address32
                );

    Address32 = (UINT32) RShiftU64 ((Address + Node->Length - 1), 32);
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                0x2C,
                1,
                &Address32
                );

    Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
    Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;
    break;

  default:
    break;
  }

  return EFI_SUCCESS;
}

/**
  Program parent bridge for oprom
  
  @param PciDevice      Pci deivce instance
  @param OptionRomBase  Base address for oprom
  @param Enable         Enable/Disable
  
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
ProgrameUpstreamBridgeForRom (
  IN PCI_IO_DEVICE   *PciDevice,
  IN UINT32          OptionRomBase,
  IN BOOLEAN         Enable
  )
{
  PCI_IO_DEVICE     *Parent;
  PCI_RESOURCE_NODE Node;
  //
  // For root bridge, just return.
  //
  Parent = PciDevice->Parent;
  ZeroMem (&Node, sizeof (Node));
  while (Parent) {
    if (!IS_PCI_BRIDGE (&Parent->Pci)) {
      break;
    }

    Node.PciDev     = Parent;
    Node.Length     = PciDevice->RomSize;
    Node.Alignment  = 0;
    Node.Bar        = PPB_MEM32_RANGE;
    Node.ResType    = PciBarTypeMem32;
    Node.Offset     = 0;

    //
    // Program PPB to only open a single <= 16<MB apperture
    //
    if (Enable) {
      ProgramPpbApperture (OptionRomBase, &Node);
      PciEnableCommandRegister (Parent, EFI_PCI_COMMAND_MEMORY_SPACE);
    } else {
      InitializePpb (Parent);
      PciDisableCommandRegister (Parent, EFI_PCI_COMMAND_MEMORY_SPACE);
    }

    Parent = Parent->Parent;
  }

  return EFI_SUCCESS;
}

/**
  Test whether resource exists for a bridge
  
  @param Bridge  Point to resource node for a bridge 
  
  @return whether resource exists
**/
BOOLEAN
ResourceRequestExisted (
  IN PCI_RESOURCE_NODE *Bridge
  )
{
  if (Bridge != NULL) {
    if (!IsListEmpty (&Bridge->ChildList) || Bridge->Length != 0) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Initialize resource pool structure.
  
  @param ResourcePool Point to resource pool structure
  @param ResourceType Type of resource
**/
EFI_STATUS
InitializeResourcePool (
  PCI_RESOURCE_NODE   *ResourcePool,
  PCI_BAR_TYPE        ResourceType
  )
{

  ZeroMem (ResourcePool, sizeof (PCI_RESOURCE_NODE));
  ResourcePool->ResType   = ResourceType;
  ResourcePool->Signature = PCI_RESOURCE_SIGNATURE;
  InitializeListHead (&ResourcePool->ChildList);

  return EFI_SUCCESS;
}

/**
  Get all resource information for given Pci device
  
  @param PciDev         Pci device instance
  @param IoBridge       Io resource node
  @param Mem32Bridge    32-bit memory node
  @param PMem32Bridge   32-bit Pmemory node
  @param Mem64Bridge    64-bit memory node
  @param PMem64Bridge   64-bit PMemory node
  @param IoPool         Link list header for Io resource
  @param Mem32Pool      Link list header for 32-bit memory
  @param PMem32Pool     Link list header for 32-bit Pmemory
  @param Mem64Pool      Link list header for 64-bit memory
  @param PMem64Pool     Link list header for 64-bit Pmemory
  
  @retval EFI_SUCCESS Success
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
  )
{

  PCI_RESOURCE_NODE *Temp;
  LIST_ENTRY        *CurrentLink;

  CurrentLink = IoPool->ChildList.ForwardLink;

  //
  // Get Io resource map
  //
  while (CurrentLink != &IoPool->ChildList) {

    Temp = RESOURCE_NODE_FROM_LINK (CurrentLink);

    if (Temp->PciDev == PciDev) {
      *IoBridge = Temp;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  //
  // Get Mem32 resource map
  //
  CurrentLink = Mem32Pool->ChildList.ForwardLink;

  while (CurrentLink != &Mem32Pool->ChildList) {

    Temp = RESOURCE_NODE_FROM_LINK (CurrentLink);

    if (Temp->PciDev == PciDev) {
      *Mem32Bridge = Temp;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  //
  // Get Pmem32 resource map
  //
  CurrentLink = PMem32Pool->ChildList.ForwardLink;

  while (CurrentLink != &PMem32Pool->ChildList) {

    Temp = RESOURCE_NODE_FROM_LINK (CurrentLink);

    if (Temp->PciDev == PciDev) {
      *PMem32Bridge = Temp;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  //
  // Get Mem64 resource map
  //
  CurrentLink = Mem64Pool->ChildList.ForwardLink;

  while (CurrentLink != &Mem64Pool->ChildList) {

    Temp = RESOURCE_NODE_FROM_LINK (CurrentLink);

    if (Temp->PciDev == PciDev) {
      *Mem64Bridge = Temp;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  //
  // Get Pmem64 resource map
  //
  CurrentLink = PMem64Pool->ChildList.ForwardLink;

  while (CurrentLink != &PMem64Pool->ChildList) {

    Temp = RESOURCE_NODE_FROM_LINK (CurrentLink);

    if (Temp->PciDev == PciDev) {
      *PMem64Bridge = Temp;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return EFI_SUCCESS;
}

/**
  Destory given resource tree
  
  @param Bridge  root node of resource tree
  
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
DestroyResourceTree (
  IN PCI_RESOURCE_NODE *Bridge
  )
{
  PCI_RESOURCE_NODE *Temp;
  LIST_ENTRY        *CurrentLink;

  while (!IsListEmpty (&Bridge->ChildList)) {

    CurrentLink = Bridge->ChildList.ForwardLink;

    Temp        = RESOURCE_NODE_FROM_LINK (CurrentLink);

    RemoveEntryList (CurrentLink);

    if (IS_PCI_BRIDGE (&(Temp->PciDev->Pci))) {
      DestroyResourceTree (Temp);
    }

    gBS->FreePool (Temp);
  }

  return EFI_SUCCESS;
}

/**
  Record the reserved resource and insert to reserved list.
  
  @param Base     Base address of reserved resourse
  @param Length   Length of reserved resource 
  @param ResType  Resource type
  @param Bridge   Pci device instance
**/
EFI_STATUS
RecordReservedResource (
  IN UINT64         Base,
  IN UINT64         Length,
  IN PCI_BAR_TYPE   ResType,
  IN PCI_IO_DEVICE  *Bridge
  )
{
  PCI_RESERVED_RESOURCE_LIST  *ReservedNode;

  ReservedNode = AllocatePool (sizeof (PCI_RESERVED_RESOURCE_LIST));
  if (ReservedNode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ReservedNode->Signature     = RESERVED_RESOURCE_SIGNATURE;
  ReservedNode->Node.Base     = Base;
  ReservedNode->Node.Length   = Length;
  ReservedNode->Node.ResType  = ResType;

  InsertTailList (&Bridge->ReservedResourceList, &(ReservedNode->Link));

  return EFI_SUCCESS;
}

/**
  Insert resource padding for P2C
  
  @param PciDev     Pci device instance
  @param IoNode     Resource info node for IO 
  @param Mem32Node  Resource info node for 32-bit memory
  @param PMem32Node Resource info node for 32-bit PMemory
  @param Mem64Node  Resource info node for 64-bit memory
  @param PMem64Node Resource info node for 64-bit PMemory
  
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
ResourcePaddingForCardBusBridge (
  PCI_IO_DEVICE     *PciDev,
  PCI_RESOURCE_NODE *IoNode,
  PCI_RESOURCE_NODE *Mem32Node,
  PCI_RESOURCE_NODE *PMem32Node,
  PCI_RESOURCE_NODE *Mem64Node,
  PCI_RESOURCE_NODE *PMem64Node
  )
{
  PCI_RESOURCE_NODE *Node;

  Node = NULL;

  //
  // Memory Base/Limit Register 0
  // Bar 1 denodes memory range 0
  //
  Node = CreateResourceNode (
          PciDev,
          0x2000000,
          0x1ffffff,
          1,
          PciBarTypeMem32,
          PciResUsagePadding
          );

  InsertResourceNode (
    Mem32Node,
    Node
    );

  //
  // Memory Base/Limit Register 1
  // Bar 2 denodes memory range1
  //
  Node = CreateResourceNode (
          PciDev,
          0x2000000,
          0x1ffffff,
          2,
          PciBarTypePMem32,
          PciResUsagePadding
          );

  InsertResourceNode (
    PMem32Node,
    Node
    );

  //
  // Io Base/Limit
  // Bar 3 denodes io range 0
  //
  Node = CreateResourceNode (
          PciDev,
          0x100,
          0xff,
          3,
          PciBarTypeIo16,
          PciResUsagePadding
          );

  InsertResourceNode (
    IoNode,
    Node
    );

  //
  // Io Base/Limit
  // Bar 4 denodes io range 0
  //
  Node = CreateResourceNode (
          PciDev,
          0x100,
          0xff,
          4,
          PciBarTypeIo16,
          PciResUsagePadding
          );

  InsertResourceNode (
    IoNode,
    Node
    );

  return EFI_SUCCESS;
}

/**
  Program P2C register for given resource node
  
  @param Base    Base address of P2C device
  @param Node    Given resource node.
  
  @retval EFI_SUCCESS Success
**/
EFI_STATUS
ProgramP2C (
  IN UINT64            Base,
  IN PCI_RESOURCE_NODE *Node
  )
{
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT64              Address;
  UINT64              TempAddress;
  UINT16              BridgeControl;

  Address = 0;
  PciIo   = &(Node->PciDev->PciIo);

  Address = Base + Node->Offset;

  //
  // Indicate pci bus driver has allocated
  // resource for this device
  // It might be a temporary solution here since
  // pci device could have multiple bar
  //
  Node->PciDev->Allocated = TRUE;

  switch (Node->Bar) {

  case P2C_BAR_0:
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                (Node->PciDev->PciBar[Node->Bar]).Offset,
                1,
                &Address
                );

    Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
    Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;
    break;

  case P2C_MEM_1:
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                0x1c,
                1,
                &Address
                );

    TempAddress = Address + Node->Length - 1;
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                0x20,
                1,
                &TempAddress
                );

    if (Node->ResType == PciBarTypeMem32) {

      //
      // Set non-prefetchable bit
      //
      PciIoRead (
                  PciIo,
                  EfiPciIoWidthUint16,
                  0x3e,
                  1,
                  &BridgeControl
                  );

      BridgeControl &= 0xfeff;
      PciIoWrite (
                  PciIo,
                  EfiPciIoWidthUint16,
                  0x3e,
                  1,
                  &BridgeControl
                  );

    } else {

      //
      // Set pre-fetchable bit
      //
      PciIoRead (
                  PciIo,
                  EfiPciIoWidthUint16,
                  0x3e,
                  1,
                  &BridgeControl
                  );

      BridgeControl |= 0x0100;
      PciIoWrite (
                  PciIo,
                  EfiPciIoWidthUint16,
                  0x3e,
                  1,
                  &BridgeControl
                  );
    }

    Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
    Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;
    Node->PciDev->PciBar[Node->Bar].BarType     = Node->ResType;

    break;

  case P2C_MEM_2:
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                0x24,
                1,
                &Address
                );

    TempAddress = Address + Node->Length - 1;

    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                0x28,
                1,
                &TempAddress
                );

    if (Node->ResType == PciBarTypeMem32) {

      //
      // Set non-prefetchable bit
      //
      PciIoRead (
                  PciIo,
                  EfiPciIoWidthUint16,
                  0x3e,
                  1,
                  &BridgeControl
                  );

      BridgeControl &= 0xfdff;
      PciIoWrite (
                  PciIo,
                  EfiPciIoWidthUint16,
                  0x3e,
                  1,
                  &BridgeControl
                  );
    } else {

      //
      // Set pre-fetchable bit
      //
      PciIoRead (
                  PciIo,
                  EfiPciIoWidthUint16,
                  0x3e,
                  1,
                  &BridgeControl
                  );

      BridgeControl |= 0x0200;
      PciIoWrite (
                  PciIo,
                  EfiPciIoWidthUint16,
                  0x3e,
                  1,
                  &BridgeControl
                  );
    }

    Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
    Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;
    Node->PciDev->PciBar[Node->Bar].BarType     = Node->ResType;
    break;

  case P2C_IO_1:
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                0x2c,
                1,
                &Address
                );
    TempAddress = Address + Node->Length - 1;
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                0x30,
                1,
                &TempAddress
                );

    Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
    Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;
    Node->PciDev->PciBar[Node->Bar].BarType     = Node->ResType;

    break;

  case P2C_IO_2:
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                0x34,
                1,
                &Address
                );

    TempAddress = Address + Node->Length - 1;
    PciIoWrite (
                PciIo,
                EfiPciIoWidthUint32,
                0x38,
                1,
                &TempAddress
                );

    Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
    Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;
    Node->PciDev->PciBar[Node->Bar].BarType     = Node->ResType;
    break;

  default:
    break;
  }

  return EFI_SUCCESS;
}

/**
  Create padding resource node.
  
  @param PciDev     Pci device instance
  @param IoNode     Resource info node for IO 
  @param Mem32Node  Resource info node for 32-bit memory
  @param PMem32Node Resource info node for 32-bit PMemory
  @param Mem64Node  Resource info node for 64-bit memory
  @param PMem64Node Resource info node for 64-bit PMemory
  
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
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Ptr;
  PCI_RESOURCE_NODE                 *Node;
  UINT8                             DummyBarIndex;

  DummyBarIndex = 0;
  Ptr           = PciDev->ResourcePaddingDescriptors;

  while (((EFI_ACPI_END_TAG_DESCRIPTOR *) Ptr)->Desc != ACPI_END_TAG_DESCRIPTOR) {

    if (Ptr->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR && Ptr->ResType == ACPI_ADDRESS_SPACE_TYPE_IO) {
      if (Ptr->AddrLen != 0) {

        Node = CreateResourceNode (
                PciDev,
                Ptr->AddrLen,
                Ptr->AddrRangeMax,
                DummyBarIndex,
                PciBarTypeIo16,
                PciResUsagePadding
                );
        InsertResourceNode (
          IoNode,
          Node
          );
      }

      Ptr++;
      continue;
    }

    if (Ptr->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR && Ptr->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {

      if (Ptr->AddrSpaceGranularity == 32) {

        //
        // prefechable
        //
        if (Ptr->SpecificFlag == 0x6) {
          if (Ptr->AddrLen) {
            Node = CreateResourceNode (
                    PciDev,
                    Ptr->AddrLen,
                    Ptr->AddrRangeMax,
                    DummyBarIndex,
                    PciBarTypePMem32,
                    PciResUsagePadding
                    );
            InsertResourceNode (
              PMem32Node,
              Node
              );
          }

          Ptr++;
          continue;
        }

        //
        // Non-prefechable
        //
        if (Ptr->SpecificFlag == 0) {
          if (Ptr->AddrLen) {
            Node = CreateResourceNode (
                    PciDev,
                    Ptr->AddrLen,
                    Ptr->AddrRangeMax,
                    DummyBarIndex,
                    PciBarTypeMem32,
                    PciResUsagePadding
                    );
            InsertResourceNode (
              Mem32Node,
              Node
              );
          }

          Ptr++;
          continue;
        }
      }

      if (Ptr->AddrSpaceGranularity == 64) {

        //
        // prefechable
        //
        if (Ptr->SpecificFlag == 0x6) {
          if (Ptr->AddrLen) {
            Node = CreateResourceNode (
                    PciDev,
                    Ptr->AddrLen,
                    Ptr->AddrRangeMax,
                    DummyBarIndex,
                    PciBarTypePMem64,
                    PciResUsagePadding
                    );
            InsertResourceNode (
              PMem64Node,
              Node
              );
          }

          Ptr++;
          continue;
        }

        //
        // Non-prefechable
        //
        if (Ptr->SpecificFlag == 0) {
          if (Ptr->AddrLen) {
            Node = CreateResourceNode (
                    PciDev,
                    Ptr->AddrLen,
                    Ptr->AddrRangeMax,
                    DummyBarIndex,
                    PciBarTypeMem64,
                    PciResUsagePadding
                    );
            InsertResourceNode (
              Mem64Node,
              Node
              );
          }

          Ptr++;
          continue;
        }
      }
    }

    Ptr++;
  }

  return EFI_SUCCESS;
}

/**
  Get padding resource for PPB
  Light PCI bus driver woundn't support hotplug root device
  So no need to pad resource for them

  @param   PciIoDevice Pci device instance
**/
VOID
GetResourcePaddingPpb (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  )
{
  if (gPciHotPlugInit) {
    if (PciIoDevice->ResourcePaddingDescriptors == NULL) {
      GetResourcePaddingForHpb (PciIoDevice);
    }
  }
}


/** @file
  PCI resources support functions implementation for PCI Bus module.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PciBus.h"

//
// The default policy for the PCI bus driver is NOT to reserve I/O ranges for both ISA aliases and VGA aliases.
//
BOOLEAN  mReserveIsaAliases = FALSE;
BOOLEAN  mReserveVgaAliases = FALSE;
BOOLEAN  mPolicyDetermined  = FALSE;

/**
  The function is used to skip VGA range.

  @param Start    Returned start address including VGA range.
  @param Length   The length of VGA range.

**/
VOID
SkipVGAAperture (
  OUT UINT64  *Start,
  IN  UINT64  Length
  )
{
  UINT64  Original;
  UINT64  Mask;
  UINT64  StartOffset;
  UINT64  LimitOffset;

  ASSERT (Start != NULL);
  //
  // For legacy VGA, bit 10 to bit 15 is not decoded
  //
  Mask = 0x3FF;

  Original    = *Start;
  StartOffset = Original & Mask;
  LimitOffset = ((*Start) + Length - 1) & Mask;
  if (LimitOffset >= VGABASE1) {
    *Start = *Start - StartOffset + VGALIMIT2 + 1;
  }
}

/**
  This function is used to skip ISA aliasing aperture.

  @param Start    Returned start address including ISA aliasing aperture.
  @param Length   The length of ISA aliasing aperture.

**/
VOID
SkipIsaAliasAperture (
  OUT UINT64  *Start,
  IN  UINT64  Length
  )
{
  UINT64  Original;
  UINT64  Mask;
  UINT64  StartOffset;
  UINT64  LimitOffset;

  ASSERT (Start != NULL);

  //
  // For legacy ISA, bit 10 to bit 15 is not decoded
  //
  Mask = 0x3FF;

  Original    = *Start;
  StartOffset = Original & Mask;
  LimitOffset = ((*Start) + Length - 1) & Mask;

  if (LimitOffset >= ISABASE) {
    *Start = *Start - StartOffset + ISALIMIT + 1;
  }
}

/**
  This function inserts a resource node into the resource list.
  The resource list is sorted in descend order.

  @param Bridge  PCI resource node for bridge.
  @param ResNode Resource node want to be inserted.

**/
VOID
InsertResourceNode (
  IN OUT PCI_RESOURCE_NODE  *Bridge,
  IN     PCI_RESOURCE_NODE  *ResNode
  )
{
  LIST_ENTRY         *CurrentLink;
  PCI_RESOURCE_NODE  *Temp;
  UINT64             ResNodeAlignRest;
  UINT64             TempAlignRest;

  ASSERT (Bridge  != NULL);
  ASSERT (ResNode != NULL);

  InsertHeadList (&Bridge->ChildList, &ResNode->Link);

  CurrentLink = Bridge->ChildList.ForwardLink->ForwardLink;
  while (CurrentLink != &Bridge->ChildList) {
    Temp = RESOURCE_NODE_FROM_LINK (CurrentLink);

    if (ResNode->Alignment > Temp->Alignment) {
      break;
    } else if (ResNode->Alignment == Temp->Alignment) {
      ResNodeAlignRest = ResNode->Length & ResNode->Alignment;
      TempAlignRest    = Temp->Length & Temp->Alignment;
      if ((ResNodeAlignRest == 0) || (ResNodeAlignRest >= TempAlignRest)) {
        break;
      }
    }

    SwapListEntries (&ResNode->Link, CurrentLink);

    CurrentLink = ResNode->Link.ForwardLink;
  }
}

/**
  This routine is used to merge two different resource trees in need of
  resource degradation.

  For example, if an upstream PPB doesn't support,
  prefetchable memory decoding, the PCI bus driver will choose to call this function
  to merge prefetchable memory resource list into normal memory list.

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
  IN PCI_RESOURCE_NODE  *Dst,
  IN PCI_RESOURCE_NODE  *Res,
  IN BOOLEAN            TypeMerge
  )
{
  LIST_ENTRY         *CurrentLink;
  PCI_RESOURCE_NODE  *Temp;

  ASSERT (Dst != NULL);
  ASSERT (Res != NULL);

  while (!IsListEmpty (&Res->ChildList)) {
    CurrentLink = Res->ChildList.ForwardLink;

    Temp = RESOURCE_NODE_FROM_LINK (CurrentLink);

    if (TypeMerge) {
      Temp->ResType = Dst->ResType;
    }

    RemoveEntryList (CurrentLink);
    InsertResourceNode (Dst, Temp);
  }
}

/**
  This function is used to calculate the IO16 aperture
  for a bridge.

  @param Bridge    PCI resource node for bridge.

**/
VOID
CalculateApertureIo16 (
  IN PCI_RESOURCE_NODE  *Bridge
  )
{
  EFI_STATUS               Status;
  UINT64                   Aperture;
  LIST_ENTRY               *CurrentLink;
  PCI_RESOURCE_NODE        *Node;
  UINT64                   Offset;
  EFI_PCI_PLATFORM_POLICY  PciPolicy;
  UINT64                   PaddingAperture;

  if (!mPolicyDetermined) {
    //
    // Check PciPlatform policy
    //
    Status    = EFI_NOT_FOUND;
    PciPolicy = 0;
    if (gPciPlatformProtocol != NULL) {
      Status = gPciPlatformProtocol->GetPlatformPolicy (
                                       gPciPlatformProtocol,
                                       &PciPolicy
                                       );
    }

    if (EFI_ERROR (Status) && (gPciOverrideProtocol != NULL)) {
      Status = gPciOverrideProtocol->GetPlatformPolicy (
                                       gPciOverrideProtocol,
                                       &PciPolicy
                                       );
    }

    if (!EFI_ERROR (Status)) {
      if ((PciPolicy & EFI_RESERVE_ISA_IO_ALIAS) != 0) {
        mReserveIsaAliases = TRUE;
      }

      if ((PciPolicy & EFI_RESERVE_VGA_IO_ALIAS) != 0) {
        mReserveVgaAliases = TRUE;
      }
    }

    mPolicyDetermined = TRUE;
  }

  Aperture        = 0;
  PaddingAperture = 0;

  if (Bridge == NULL) {
    return;
  }

  //
  // Assume the bridge is aligned
  //
  for ( CurrentLink = GetFirstNode (&Bridge->ChildList)
        ; !IsNull (&Bridge->ChildList, CurrentLink)
        ; CurrentLink = GetNextNode (&Bridge->ChildList, CurrentLink)
        )
  {
    Node = RESOURCE_NODE_FROM_LINK (CurrentLink);
    if (Node->ResourceUsage == PciResUsagePadding) {
      ASSERT (PaddingAperture == 0);
      PaddingAperture = Node->Length;
      continue;
    }

    //
    // Consider the aperture alignment
    //
    Offset = Aperture & (Node->Alignment);

    if (Offset != 0) {
      Aperture = Aperture + (Node->Alignment + 1) - Offset;
    }

    //
    // IsaEnable and VGAEnable can not be implemented now.
    // If both of them are enabled, then the IO resource would
    // become too limited to meet the requirement of most of devices.
    //
    if (mReserveIsaAliases || mReserveVgaAliases) {
      if (!IS_PCI_BRIDGE (&(Node->PciDev->Pci)) && !IS_CARDBUS_BRIDGE (&(Node->PciDev->Pci))) {
        //
        // Check if there is need to support ISA/VGA decoding
        // If so, we need to avoid isa/vga aliasing range
        //
        if (mReserveIsaAliases) {
          SkipIsaAliasAperture (
            &Aperture,
            Node->Length
            );
          Offset = Aperture & (Node->Alignment);
          if (Offset != 0) {
            Aperture = Aperture + (Node->Alignment + 1) - Offset;
          }
        } else if (mReserveVgaAliases) {
          SkipVGAAperture (
            &Aperture,
            Node->Length
            );
          Offset = Aperture & (Node->Alignment);
          if (Offset != 0) {
            Aperture = Aperture + (Node->Alignment + 1) - Offset;
          }
        }
      }
    }

    Node->Offset = Aperture;

    //
    // Increment aperture by the length of node
    //
    Aperture += Node->Length;
  }

  //
  // Adjust the aperture with the bridge's alignment
  //
  Offset = Aperture & (Bridge->Alignment);

  if (Offset != 0) {
    Aperture = Aperture + (Bridge->Alignment + 1) - Offset;
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

  //
  // Hotplug controller needs padding resources.
  // Use the larger one between the padding resource and actual occupied resource.
  //
  Bridge->Length = MAX (Bridge->Length, PaddingAperture);
}

/**
  This function is used to calculate the resource aperture
  for a given bridge device.

  @param Bridge      PCI resource node for given bridge device.

**/
VOID
CalculateResourceAperture (
  IN PCI_RESOURCE_NODE  *Bridge
  )
{
  UINT64             Aperture[2];
  LIST_ENTRY         *CurrentLink;
  PCI_RESOURCE_NODE  *Node;

  if (Bridge == NULL) {
    return;
  }

  if (Bridge->ResType == PciBarTypeIo16) {
    CalculateApertureIo16 (Bridge);
    return;
  }

  Aperture[PciResUsageTypical] = 0;
  Aperture[PciResUsagePadding] = 0;
  //
  // Assume the bridge is aligned
  //
  for ( CurrentLink = GetFirstNode (&Bridge->ChildList)
        ; !IsNull (&Bridge->ChildList, CurrentLink)
        ; CurrentLink = GetNextNode (&Bridge->ChildList, CurrentLink)
        )
  {
    Node = RESOURCE_NODE_FROM_LINK (CurrentLink);

    //
    // It's possible for a bridge to contain multiple padding resource
    // nodes due to DegradeResource().
    //
    ASSERT (
      (Node->ResourceUsage == PciResUsageTypical) ||
      (Node->ResourceUsage == PciResUsagePadding)
      );
    ASSERT (Node->ResourceUsage < ARRAY_SIZE (Aperture));
    //
    // Recode current aperture as a offset
    // Apply padding resource to meet alignment requirement
    // Node offset will be used in future real allocation
    //
    Node->Offset = ALIGN_VALUE (Aperture[Node->ResourceUsage], Node->Alignment + 1);

    //
    // Record the total aperture.
    //
    Aperture[Node->ResourceUsage] = Node->Offset + Node->Length;
  }

  //
  // Adjust the aperture with the bridge's alignment
  //
  Aperture[PciResUsageTypical] = ALIGN_VALUE (Aperture[PciResUsageTypical], Bridge->Alignment + 1);
  Aperture[PciResUsagePadding] = ALIGN_VALUE (Aperture[PciResUsagePadding], Bridge->Alignment + 1);

  //
  // Hotplug controller needs padding resources.
  // Use the larger one between the padding resource and actual occupied resource.
  //
  Bridge->Length = MAX (Aperture[PciResUsageTypical], Aperture[PciResUsagePadding]);

  //
  // Adjust the bridge's alignment to the MAX (first) alignment of all children.
  //
  CurrentLink = Bridge->ChildList.ForwardLink;
  if (CurrentLink != &Bridge->ChildList) {
    Node = RESOURCE_NODE_FROM_LINK (CurrentLink);
    if (Node->Alignment > Bridge->Alignment) {
      Bridge->Alignment = Node->Alignment;
    }
  }
}

/**
  Get IO/Memory resource info for given PCI device.

  @param PciDev     Pci device instance.
  @param IoNode     Resource info node for IO .
  @param Mem32Node  Resource info node for 32-bit memory.
  @param PMem32Node Resource info node for 32-bit Prefetchable Memory.
  @param Mem64Node  Resource info node for 64-bit memory.
  @param PMem64Node Resource info node for 64-bit Prefetchable Memory.

**/
VOID
GetResourceFromDevice (
  IN     PCI_IO_DEVICE      *PciDev,
  IN OUT PCI_RESOURCE_NODE  *IoNode,
  IN OUT PCI_RESOURCE_NODE  *Mem32Node,
  IN OUT PCI_RESOURCE_NODE  *PMem32Node,
  IN OUT PCI_RESOURCE_NODE  *Mem64Node,
  IN OUT PCI_RESOURCE_NODE  *PMem64Node
  )
{
  UINT8              Index;
  PCI_RESOURCE_NODE  *Node;
  BOOLEAN            ResourceRequested;

  Node              = NULL;
  ResourceRequested = FALSE;

  for (Index = 0; Index < PCI_MAX_BAR; Index++) {
    switch ((PciDev->PciBar)[Index].BarType) {
      case PciBarTypeMem32:
      case PciBarTypeOpRom:

        Node = CreateResourceNode (
                 PciDev,
                 (PciDev->PciBar)[Index].Length,
                 (PciDev->PciBar)[Index].Alignment,
                 Index,
                 (PciDev->PciBar)[Index].BarType,
                 PciResUsageTypical
                 );
        if (Node != NULL) {
          InsertResourceNode (
            Mem32Node,
            Node
            );

          ResourceRequested = TRUE;
        }

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
        if (Node != NULL) {
          InsertResourceNode (
            Mem64Node,
            Node
            );

          ResourceRequested = TRUE;
        }

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
        if (Node != NULL) {
          InsertResourceNode (
            PMem64Node,
            Node
            );

          ResourceRequested = TRUE;
        }

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
        if (Node != NULL) {
          InsertResourceNode (
            PMem32Node,
            Node
            );
          ResourceRequested = TRUE;
        }

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
        if (Node != NULL) {
          InsertResourceNode (
            IoNode,
            Node
            );
          ResourceRequested = TRUE;
        }

        break;

      case PciBarTypeUnknown:
        break;

      default:
        break;
    }
  }

  //
  // Add VF resource
  //
  for (Index = 0; Index < PCI_MAX_BAR; Index++) {
    switch ((PciDev->VfPciBar)[Index].BarType) {
      case PciBarTypeMem32:

        Node = CreateVfResourceNode (
                 PciDev,
                 (PciDev->VfPciBar)[Index].Length,
                 (PciDev->VfPciBar)[Index].Alignment,
                 Index,
                 PciBarTypeMem32,
                 PciResUsageTypical
                 );
        if (Node != NULL) {
          InsertResourceNode (
            Mem32Node,
            Node
            );
        }

        break;

      case PciBarTypeMem64:

        Node = CreateVfResourceNode (
                 PciDev,
                 (PciDev->VfPciBar)[Index].Length,
                 (PciDev->VfPciBar)[Index].Alignment,
                 Index,
                 PciBarTypeMem64,
                 PciResUsageTypical
                 );
        if (Node != NULL) {
          InsertResourceNode (
            Mem64Node,
            Node
            );
        }

        break;

      case PciBarTypePMem64:

        Node = CreateVfResourceNode (
                 PciDev,
                 (PciDev->VfPciBar)[Index].Length,
                 (PciDev->VfPciBar)[Index].Alignment,
                 Index,
                 PciBarTypePMem64,
                 PciResUsageTypical
                 );
        if (Node != NULL) {
          InsertResourceNode (
            PMem64Node,
            Node
            );
        }

        break;

      case PciBarTypePMem32:

        Node = CreateVfResourceNode (
                 PciDev,
                 (PciDev->VfPciBar)[Index].Length,
                 (PciDev->VfPciBar)[Index].Alignment,
                 Index,
                 PciBarTypePMem32,
                 PciResUsageTypical
                 );
        if (Node != NULL) {
          InsertResourceNode (
            PMem32Node,
            Node
            );
        }

        break;

      case PciBarTypeIo16:
      case PciBarTypeIo32:
        break;

      case PciBarTypeUnknown:
        break;

      default:
        break;
    }
  }

  // If there is no resource requested from this device,
  // then we indicate this device has been allocated naturally.
  //
  if (!ResourceRequested) {
    PciDev->Allocated = TRUE;
  }
}

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
  IN PCI_IO_DEVICE       *PciDev,
  IN UINT64              Length,
  IN UINT64              Alignment,
  IN UINT8               Bar,
  IN PCI_BAR_TYPE        ResType,
  IN PCI_RESOURCE_USAGE  ResUsage
  )
{
  PCI_RESOURCE_NODE  *Node;

  Node = NULL;

  Node = AllocateZeroPool (sizeof (PCI_RESOURCE_NODE));
  ASSERT (Node != NULL);
  if (Node == NULL) {
    return NULL;
  }

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
  This function is used to create a IOV VF resource node.

  @param PciDev       Pci device instance.
  @param Length       Length of Io/Memory resource.
  @param Alignment    Alignment of resource.
  @param Bar          Bar index.
  @param ResType      Type of resource: IO/Memory.
  @param ResUsage     Resource usage.

  @return PCI resource node created for given VF PCI device.
          NULL means PCI resource node is not created.

**/
PCI_RESOURCE_NODE *
CreateVfResourceNode (
  IN PCI_IO_DEVICE       *PciDev,
  IN UINT64              Length,
  IN UINT64              Alignment,
  IN UINT8               Bar,
  IN PCI_BAR_TYPE        ResType,
  IN PCI_RESOURCE_USAGE  ResUsage
  )
{
  PCI_RESOURCE_NODE  *Node;

  Node = CreateResourceNode (PciDev, Length, Alignment, Bar, ResType, ResUsage);
  if (Node == NULL) {
    return Node;
  }

  Node->Virtual = TRUE;

  return Node;
}

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
  IN     PCI_IO_DEVICE      *Bridge,
  IN OUT PCI_RESOURCE_NODE  *IoNode,
  IN OUT PCI_RESOURCE_NODE  *Mem32Node,
  IN OUT PCI_RESOURCE_NODE  *PMem32Node,
  IN OUT PCI_RESOURCE_NODE  *Mem64Node,
  IN OUT PCI_RESOURCE_NODE  *PMem64Node
  )
{
  PCI_IO_DEVICE      *Temp;
  PCI_RESOURCE_NODE  *IoBridge;
  PCI_RESOURCE_NODE  *Mem32Bridge;
  PCI_RESOURCE_NODE  *PMem32Bridge;
  PCI_RESOURCE_NODE  *Mem64Bridge;
  PCI_RESOURCE_NODE  *PMem64Bridge;
  LIST_ENTRY         *CurrentLink;

  CurrentLink = Bridge->ChildList.ForwardLink;

  while (CurrentLink != NULL && CurrentLink != &Bridge->ChildList) {
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
      // is aligned with 4KB (smaller alignments may be supported).
      //
      IoBridge = CreateResourceNode (
                   Temp,
                   0,
                   Temp->BridgeIoAlignment,
                   PPB_IO_RANGE,
                   PciBarTypeIo16,
                   PciResUsageTypical
                   );

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

      if ((IoBridge == NULL) || (Mem32Bridge == NULL) || (PMem32Bridge == NULL) ||
          (Mem64Bridge == NULL) || (PMem64Bridge == NULL))
      {
        if (IoBridge != NULL) {
          FreePool (IoBridge);
        }

        if (Mem32Bridge != NULL) {
          FreePool (Mem32Bridge);
        }

        if (PMem32Bridge != NULL) {
          FreePool (PMem32Bridge);
        }

        if (Mem64Bridge != NULL) {
          FreePool (Mem64Bridge);
        }

        if (PMem64Bridge != NULL) {
          FreePool (PMem64Bridge);
        }

        return;
      }

      //
      // Recursively create resource map on this bridge
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
        FreePool (IoBridge);
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
        FreePool (Mem32Bridge);
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
        FreePool (PMem32Bridge);
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
        FreePool (Mem64Bridge);
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
        FreePool (PMem64Bridge);
        PMem64Bridge = NULL;
      }
    }

    //
    // If it is P2C, apply hard coded resource padding
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
}

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
  IN PCI_IO_DEVICE      *PciDev,
  IN PCI_RESOURCE_NODE  *IoNode,
  IN PCI_RESOURCE_NODE  *Mem32Node,
  IN PCI_RESOURCE_NODE  *PMem32Node,
  IN PCI_RESOURCE_NODE  *Mem64Node,
  IN PCI_RESOURCE_NODE  *PMem64Node
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
}

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
  IN PCI_IO_DEVICE      *Bridge,
  IN PCI_RESOURCE_NODE  *Mem32Node,
  IN PCI_RESOURCE_NODE  *PMem32Node,
  IN PCI_RESOURCE_NODE  *Mem64Node,
  IN PCI_RESOURCE_NODE  *PMem64Node
  )
{
  PCI_IO_DEVICE      *PciIoDevice;
  LIST_ENTRY         *ChildDeviceLink;
  LIST_ENTRY         *ChildNodeLink;
  LIST_ENTRY         *NextChildNodeLink;
  PCI_RESOURCE_NODE  *ResourceNode;

  if (FeaturePcdGet (PcdPciDegradeResourceForOptionRom)) {
    //
    // If any child device has both option ROM and 64-bit BAR, degrade its PMEM64/MEM64
    // requests in case that if a legacy option ROM image can not access 64-bit resources.
    //
    ChildDeviceLink = Bridge->ChildList.ForwardLink;
    while (ChildDeviceLink != NULL && ChildDeviceLink != &Bridge->ChildList) {
      PciIoDevice = PCI_IO_DEVICE_FROM_LINK (ChildDeviceLink);
      if (PciIoDevice->RomSize != 0) {
        if (!IsListEmpty (&Mem64Node->ChildList)) {
          ChildNodeLink = Mem64Node->ChildList.ForwardLink;
          while (ChildNodeLink != &Mem64Node->ChildList) {
            ResourceNode      = RESOURCE_NODE_FROM_LINK (ChildNodeLink);
            NextChildNodeLink = ChildNodeLink->ForwardLink;

            if ((ResourceNode->PciDev == PciIoDevice) &&
                (ResourceNode->Virtual || !PciIoDevice->PciBar[ResourceNode->Bar].BarTypeFixed)
                )
            {
              RemoveEntryList (ChildNodeLink);
              InsertResourceNode (Mem32Node, ResourceNode);
            }

            ChildNodeLink = NextChildNodeLink;
          }
        }

        if (!IsListEmpty (&PMem64Node->ChildList)) {
          ChildNodeLink = PMem64Node->ChildList.ForwardLink;
          while (ChildNodeLink != &PMem64Node->ChildList) {
            ResourceNode      = RESOURCE_NODE_FROM_LINK (ChildNodeLink);
            NextChildNodeLink = ChildNodeLink->ForwardLink;

            if ((ResourceNode->PciDev == PciIoDevice) &&
                (ResourceNode->Virtual || !PciIoDevice->PciBar[ResourceNode->Bar].BarTypeFixed)
                )
            {
              RemoveEntryList (ChildNodeLink);
              InsertResourceNode (PMem32Node, ResourceNode);
            }

            ChildNodeLink = NextChildNodeLink;
          }
        }
      }

      ChildDeviceLink = ChildDeviceLink->ForwardLink;
    }
  }

  //
  // If firmware is in 32-bit mode,
  // then degrade PMEM64/MEM64 requests
  //
  if (sizeof (UINTN) <= 4) {
    MergeResourceTree (
      Mem32Node,
      Mem64Node,
      TRUE
      );

    MergeResourceTree (
      PMem32Node,
      PMem64Node,
      TRUE
      );
  } else {
    //
    // if the bridge does not support MEM64, degrade MEM64 to MEM32
    //
    if (!BridgeSupportResourceDecode (Bridge, EFI_BRIDGE_MEM64_DECODE_SUPPORTED)) {
      MergeResourceTree (
        Mem32Node,
        Mem64Node,
        TRUE
        );
    }

    //
    // if the bridge does not support PMEM64, degrade PMEM64 to PMEM32
    //
    if (!BridgeSupportResourceDecode (Bridge, EFI_BRIDGE_PMEM64_DECODE_SUPPORTED)) {
      MergeResourceTree (
        PMem32Node,
        PMem64Node,
        TRUE
        );
    }

    //
    // if both PMEM64 and PMEM32 requests from child devices, which can not be satisfied
    // by a P2P bridge simultaneously, keep PMEM64 and degrade PMEM32 to MEM32.
    //
    if (!IsListEmpty (&PMem64Node->ChildList) && (Bridge->Parent != NULL)) {
      MergeResourceTree (
        Mem32Node,
        PMem32Node,
        TRUE
        );
    }
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
  // if root bridge supports combined Pmem Mem decoding
  // merge these two type of resource
  //
  if (BridgeSupportResourceDecode (Bridge, EFI_BRIDGE_PMEM_MEM_COMBINE_SUPPORTED)) {
    MergeResourceTree (
      Mem32Node,
      PMem32Node,
      FALSE
      );

    //
    // No need to check if to degrade MEM64 after merge, because
    // if there are PMEM64 still here, 64-bit decode should be supported
    // by the root bride.
    //
    MergeResourceTree (
      Mem64Node,
      PMem64Node,
      FALSE
      );
  }
}

/**
  Test whether bridge device support decode resource.

  @param Bridge    Bridge device instance.
  @param Decode    Decode type according to resource type.

  @return TRUE     The bridge device support decode resource.
  @return FALSE    The bridge device don't support decode resource.

**/
BOOLEAN
BridgeSupportResourceDecode (
  IN PCI_IO_DEVICE  *Bridge,
  IN UINT32         Decode
  )
{
  if (((Bridge->Decodes) & Decode) != 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  This function is used to program the resource allocated
  for each resource node under specified bridge.

  @param Base     Base address of resource to be programmed.
  @param Bridge   PCI resource node for the bridge device.

  @retval EFI_SUCCESS            Successfully to program all resources
                                 on given PCI bridge device.
  @retval EFI_OUT_OF_RESOURCES   Base is all one.

**/
EFI_STATUS
ProgramResource (
  IN UINT64             Base,
  IN PCI_RESOURCE_NODE  *Bridge
  )
{
  LIST_ENTRY         *CurrentLink;
  PCI_RESOURCE_NODE  *Node;
  EFI_STATUS         Status;

  if (Base == gAllOne) {
    return EFI_OUT_OF_RESOURCES;
  }

  CurrentLink = Bridge->ChildList.ForwardLink;

  while (CurrentLink != &Bridge->ChildList) {
    Node = RESOURCE_NODE_FROM_LINK (CurrentLink);

    if (!IS_PCI_BRIDGE (&(Node->PciDev->Pci))) {
      if (IS_CARDBUS_BRIDGE (&(Node->PciDev->Pci))) {
        //
        // Program the PCI Card Bus device
        //
        ProgramP2C (Base, Node);
      } else {
        //
        // Program the PCI device BAR
        //
        ProgramBar (Base, Node);
      }
    } else {
      //
      // Program the PCI devices under this bridge
      //
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
  Program Bar register for PCI device.

  @param Base  Base address for PCI device resource to be programmed.
  @param Node  Point to resource node structure.

**/
VOID
ProgramBar (
  IN UINT64             Base,
  IN PCI_RESOURCE_NODE  *Node
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT64               Address;
  UINT32               Address32;

  ASSERT (Node->Bar < PCI_MAX_BAR);

  //
  // Check VF BAR
  //
  if (Node->Virtual) {
    ProgramVfBar (Base, Node);
    return;
  }

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

      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   (Node->PciDev->PciBar[Node->Bar]).Offset,
                   1,
                   &Address
                   );
    //
    // Continue to the case PciBarTypeOpRom to set the BaseAddress.
    // PciBarTypeOpRom is a virtual BAR only in root bridge, to capture
    // the MEM32 resource requirement for Option ROM shadow.
    //

    case PciBarTypeOpRom:
      Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;

      break;

    case PciBarTypeMem64:
    case PciBarTypePMem64:

      Address32 = (UINT32)(Address & 0x00000000FFFFFFFF);

      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   (Node->PciDev->PciBar[Node->Bar]).Offset,
                   1,
                   &Address32
                   );

      Address32 = (UINT32)RShiftU64 (Address, 32);

      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   (UINT8)((Node->PciDev->PciBar[Node->Bar]).Offset + 4),
                   1,
                   &Address32
                   );

      Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;

      break;

    default:
      break;
  }
}

/**
  Program IOV VF Bar register for PCI device.

  @param Base  Base address for PCI device resource to be programmed.
  @param Node  Point to resource node structure.

**/
EFI_STATUS
ProgramVfBar (
  IN UINT64             Base,
  IN PCI_RESOURCE_NODE  *Node
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT64               Address;
  UINT32               Address32;

  ASSERT (Node->Bar < PCI_MAX_BAR);
  ASSERT (Node->Virtual);

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

  switch ((Node->PciDev->VfPciBar[Node->Bar]).BarType) {
    case PciBarTypeMem32:
    case PciBarTypePMem32:

      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   (Node->PciDev->VfPciBar[Node->Bar]).Offset,
                   1,
                   &Address
                   );

      Node->PciDev->VfPciBar[Node->Bar].BaseAddress = Address;
      break;

    case PciBarTypeMem64:
    case PciBarTypePMem64:

      Address32 = (UINT32)(Address & 0x00000000FFFFFFFF);

      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   (Node->PciDev->VfPciBar[Node->Bar]).Offset,
                   1,
                   &Address32
                   );

      Address32 = (UINT32)RShiftU64 (Address, 32);

      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   ((Node->PciDev->VfPciBar[Node->Bar]).Offset + 4),
                   1,
                   &Address32
                   );

      Node->PciDev->VfPciBar[Node->Bar].BaseAddress = Address;
      break;

    case PciBarTypeIo16:
    case PciBarTypeIo32:
      break;

    default:
      break;
  }

  return EFI_SUCCESS;
}

/**
  Program PCI-PCI bridge aperture.

  @param Base  Base address for resource.
  @param Node  Point to resource node structure.

**/
VOID
ProgramPpbApperture (
  IN UINT64             Base,
  IN PCI_RESOURCE_NODE  *Node
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT64               Address;
  UINT32               Address32;

  Address = 0;
  //
  // If no device resource of this PPB, return anyway
  // Aperture is set default in the initialization code
  //
  if ((Node->Length == 0) || (Node->ResourceUsage == PciResUsagePadding)) {
    //
    // For padding resource node, just ignore when programming
    //
    return;
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
      switch ((Node->PciDev->PciBar[Node->Bar]).BarType) {
        case PciBarTypeIo16:
        case PciBarTypeIo32:
        case PciBarTypeMem32:
        case PciBarTypePMem32:

          PciIo->Pci.Write (
                       PciIo,
                       EfiPciIoWidthUint32,
                       (Node->PciDev->PciBar[Node->Bar]).Offset,
                       1,
                       &Address
                       );

          Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
          Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;
          break;

        case PciBarTypeMem64:
        case PciBarTypePMem64:

          Address32 = (UINT32)(Address & 0x00000000FFFFFFFF);

          PciIo->Pci.Write (
                       PciIo,
                       EfiPciIoWidthUint32,
                       (Node->PciDev->PciBar[Node->Bar]).Offset,
                       1,
                       &Address32
                       );

          Address32 = (UINT32)RShiftU64 (Address, 32);

          PciIo->Pci.Write (
                       PciIo,
                       EfiPciIoWidthUint32,
                       (UINT8)((Node->PciDev->PciBar[Node->Bar]).Offset + 4),
                       1,
                       &Address32
                       );

          Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
          Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;
          break;

        default:
          break;
      }

      break;

    case PPB_IO_RANGE:

      Address32 = ((UINT32)(Address)) >> 8;
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint8,
                   0x1C,
                   1,
                   &Address32
                   );

      Address32 >>= 8;
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint16,
                   0x30,
                   1,
                   &Address32
                   );

      Address32 = (UINT32)(Address + Node->Length - 1);
      Address32 = ((UINT32)(Address32)) >> 8;
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint8,
                   0x1D,
                   1,
                   &Address32
                   );

      Address32 >>= 8;
      PciIo->Pci.Write (
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

      Address32 = ((UINT32)(Address)) >> 16;
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint16,
                   0x20,
                   1,
                   &Address32
                   );

      Address32 = (UINT32)(Address + Node->Length - 1);
      Address32 = ((UINT32)(Address32)) >> 16;
      PciIo->Pci.Write (
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

      Address32 = ((UINT32)(Address)) >> 16;
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint16,
                   0x24,
                   1,
                   &Address32
                   );

      Address32 = (UINT32)(Address + Node->Length - 1);
      Address32 = ((UINT32)(Address32)) >> 16;
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint16,
                   0x26,
                   1,
                   &Address32
                   );

      Address32 = (UINT32)RShiftU64 (Address, 32);
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   0x28,
                   1,
                   &Address32
                   );

      Address32 = (UINT32)RShiftU64 ((Address + Node->Length - 1), 32);
      PciIo->Pci.Write (
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
}

/**
  Program parent bridge for Option Rom.

  @param PciDevice      Pci device instance.
  @param OptionRomBase  Base address for Option Rom.
  @param Enable         Enable or disable PCI memory.

**/
VOID
ProgramUpstreamBridgeForRom (
  IN PCI_IO_DEVICE  *PciDevice,
  IN UINT32         OptionRomBase,
  IN BOOLEAN        Enable
  )
{
  PCI_IO_DEVICE        *Parent;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT16               Base;
  UINT16               Limit;

  //
  // For root bridge, just return.
  //
  Parent = PciDevice->Parent;
  while (Parent != NULL) {
    if (!IS_PCI_BRIDGE (&Parent->Pci)) {
      break;
    }

    PciIo = &Parent->PciIo;

    //
    // Program PPB to only open a single <= 16MB aperture
    //
    if (Enable) {
      //
      // Only cover MMIO for Option ROM.
      //
      Base  = (UINT16)(OptionRomBase >> 16);
      Limit = (UINT16)((OptionRomBase + PciDevice->RomSize - 1) >> 16);
      PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, OFFSET_OF (PCI_TYPE01, Bridge.MemoryBase), 1, &Base);
      PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, OFFSET_OF (PCI_TYPE01, Bridge.MemoryLimit), 1, &Limit);

      PCI_ENABLE_COMMAND_REGISTER (Parent, EFI_PCI_COMMAND_MEMORY_SPACE);
    } else {
      //
      // Cover 32bit MMIO for devices below the bridge.
      //
      if (Parent->PciBar[PPB_MEM32_RANGE].Length == 0) {
        //
        // When devices under the bridge contains Option ROM and doesn't require 32bit MMIO.
        //
        Base  = (UINT16)gAllOne;
        Limit = (UINT16)gAllZero;
      } else {
        Base  = (UINT16)((UINT32)Parent->PciBar[PPB_MEM32_RANGE].BaseAddress >> 16);
        Limit = (UINT16)((UINT32)(Parent->PciBar[PPB_MEM32_RANGE].BaseAddress
                                  + Parent->PciBar[PPB_MEM32_RANGE].Length - 1) >> 16);
      }

      PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, OFFSET_OF (PCI_TYPE01, Bridge.MemoryBase), 1, &Base);
      PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, OFFSET_OF (PCI_TYPE01, Bridge.MemoryLimit), 1, &Limit);

      PCI_DISABLE_COMMAND_REGISTER (Parent, EFI_PCI_COMMAND_MEMORY_SPACE);
    }

    Parent = Parent->Parent;
  }
}

/**
  Test whether resource exists for a bridge.

  @param Bridge  Point to resource node for a bridge.

  @retval TRUE   There is resource on the given bridge.
  @retval FALSE  There isn't resource on the given bridge.

**/
BOOLEAN
ResourceRequestExisted (
  IN PCI_RESOURCE_NODE  *Bridge
  )
{
  if (Bridge != NULL) {
    if (!IsListEmpty (&Bridge->ChildList) || (Bridge->Length != 0)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Initialize resource pool structure.

  @param ResourcePool Point to resource pool structure. This pool
                      is reset to all zero when returned.
  @param ResourceType Type of resource.

**/
VOID
InitializeResourcePool (
  IN OUT PCI_RESOURCE_NODE  *ResourcePool,
  IN     PCI_BAR_TYPE       ResourceType
  )
{
  ZeroMem (ResourcePool, sizeof (PCI_RESOURCE_NODE));
  ResourcePool->ResType   = ResourceType;
  ResourcePool->Signature = PCI_RESOURCE_SIGNATURE;
  InitializeListHead (&ResourcePool->ChildList);
}

/**
  Destroy given resource tree.

  @param Bridge  PCI resource root node of resource tree.

**/
VOID
DestroyResourceTree (
  IN PCI_RESOURCE_NODE  *Bridge
  )
{
  PCI_RESOURCE_NODE  *Temp;
  LIST_ENTRY         *CurrentLink;

  while (!IsListEmpty (&Bridge->ChildList)) {
    CurrentLink = Bridge->ChildList.ForwardLink;

    Temp = RESOURCE_NODE_FROM_LINK (CurrentLink);
    ASSERT (Temp);

    RemoveEntryList (CurrentLink);

    if (IS_PCI_BRIDGE (&(Temp->PciDev->Pci))) {
      DestroyResourceTree (Temp);
    }

    FreePool (Temp);
  }
}

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
  IN PCI_IO_DEVICE      *PciDev,
  IN PCI_RESOURCE_NODE  *IoNode,
  IN PCI_RESOURCE_NODE  *Mem32Node,
  IN PCI_RESOURCE_NODE  *PMem32Node,
  IN PCI_RESOURCE_NODE  *Mem64Node,
  IN PCI_RESOURCE_NODE  *PMem64Node
  )
{
  PCI_RESOURCE_NODE  *Node;

  Node = NULL;

  //
  // Memory Base/Limit Register 0
  // Bar 1 decodes memory range 0
  //
  Node = CreateResourceNode (
           PciDev,
           0x2000000,
           0x1ffffff,
           1,
           PciBarTypeMem32,
           PciResUsagePadding
           );
  ASSERT (Node != NULL);
  if (Node != NULL) {
    InsertResourceNode (
      Mem32Node,
      Node
      );
  }

  //
  // Memory Base/Limit Register 1
  // Bar 2 decodes memory range1
  //
  Node = CreateResourceNode (
           PciDev,
           0x2000000,
           0x1ffffff,
           2,
           PciBarTypePMem32,
           PciResUsagePadding
           );

  ASSERT (Node != NULL);
  if (Node != NULL) {
    InsertResourceNode (
      PMem32Node,
      Node
      );
  }

  //
  // Io Base/Limit
  // Bar 3 decodes io range 0
  //
  Node = CreateResourceNode (
           PciDev,
           0x100,
           0xff,
           3,
           PciBarTypeIo16,
           PciResUsagePadding
           );

  ASSERT (Node != NULL);
  if (Node != NULL) {
    InsertResourceNode (
      IoNode,
      Node
      );
  }

  //
  // Io Base/Limit
  // Bar 4 decodes io range 0
  //
  Node = CreateResourceNode (
           PciDev,
           0x100,
           0xff,
           4,
           PciBarTypeIo16,
           PciResUsagePadding
           );

  ASSERT (Node != NULL);
  if (Node != NULL) {
    InsertResourceNode (
      IoNode,
      Node
      );
  }
}

/**
  Program PCI Card device register for given resource node.

  @param Base    Base address of PCI Card device to be programmed.
  @param Node    Given resource node.

**/
VOID
ProgramP2C (
  IN UINT64             Base,
  IN PCI_RESOURCE_NODE  *Node
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT64               Address;
  UINT64               TempAddress;
  UINT16               BridgeControl;

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
      PciIo->Pci.Write (
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
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   PCI_CARD_MEMORY_BASE_0,
                   1,
                   &Address
                   );

      TempAddress = Address + Node->Length - 1;
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   PCI_CARD_MEMORY_LIMIT_0,
                   1,
                   &TempAddress
                   );

      if (Node->ResType == PciBarTypeMem32) {
        //
        // Set non-prefetchable bit
        //
        PciIo->Pci.Read (
                     PciIo,
                     EfiPciIoWidthUint16,
                     PCI_CARD_BRIDGE_CONTROL,
                     1,
                     &BridgeControl
                     );

        BridgeControl &= (UINT16) ~PCI_CARD_PREFETCHABLE_MEMORY_0_ENABLE;
        PciIo->Pci.Write (
                     PciIo,
                     EfiPciIoWidthUint16,
                     PCI_CARD_BRIDGE_CONTROL,
                     1,
                     &BridgeControl
                     );
      } else {
        //
        // Set prefetchable bit
        //
        PciIo->Pci.Read (
                     PciIo,
                     EfiPciIoWidthUint16,
                     PCI_CARD_BRIDGE_CONTROL,
                     1,
                     &BridgeControl
                     );

        BridgeControl |= PCI_CARD_PREFETCHABLE_MEMORY_0_ENABLE;
        PciIo->Pci.Write (
                     PciIo,
                     EfiPciIoWidthUint16,
                     PCI_CARD_BRIDGE_CONTROL,
                     1,
                     &BridgeControl
                     );
      }

      Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
      Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;
      Node->PciDev->PciBar[Node->Bar].BarType     = Node->ResType;

      break;

    case P2C_MEM_2:
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   PCI_CARD_MEMORY_BASE_1,
                   1,
                   &Address
                   );

      TempAddress = Address + Node->Length - 1;

      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   PCI_CARD_MEMORY_LIMIT_1,
                   1,
                   &TempAddress
                   );

      if (Node->ResType == PciBarTypeMem32) {
        //
        // Set non-prefetchable bit
        //
        PciIo->Pci.Read (
                     PciIo,
                     EfiPciIoWidthUint16,
                     PCI_CARD_BRIDGE_CONTROL,
                     1,
                     &BridgeControl
                     );

        BridgeControl &= (UINT16) ~(PCI_CARD_PREFETCHABLE_MEMORY_1_ENABLE);
        PciIo->Pci.Write (
                     PciIo,
                     EfiPciIoWidthUint16,
                     PCI_CARD_BRIDGE_CONTROL,
                     1,
                     &BridgeControl
                     );
      } else {
        //
        // Set prefetchable bit
        //
        PciIo->Pci.Read (
                     PciIo,
                     EfiPciIoWidthUint16,
                     PCI_CARD_BRIDGE_CONTROL,
                     1,
                     &BridgeControl
                     );

        BridgeControl |= PCI_CARD_PREFETCHABLE_MEMORY_1_ENABLE;
        PciIo->Pci.Write (
                     PciIo,
                     EfiPciIoWidthUint16,
                     PCI_CARD_BRIDGE_CONTROL,
                     1,
                     &BridgeControl
                     );
      }

      Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
      Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;
      Node->PciDev->PciBar[Node->Bar].BarType     = Node->ResType;
      break;

    case P2C_IO_1:
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   PCI_CARD_IO_BASE_0_LOWER,
                   1,
                   &Address
                   );

      TempAddress = Address + Node->Length - 1;
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   PCI_CARD_IO_LIMIT_0_LOWER,
                   1,
                   &TempAddress
                   );

      Node->PciDev->PciBar[Node->Bar].BaseAddress = Address;
      Node->PciDev->PciBar[Node->Bar].Length      = Node->Length;
      Node->PciDev->PciBar[Node->Bar].BarType     = Node->ResType;

      break;

    case P2C_IO_2:
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   PCI_CARD_IO_BASE_1_LOWER,
                   1,
                   &Address
                   );

      TempAddress = Address + Node->Length - 1;
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   PCI_CARD_IO_LIMIT_1_LOWER,
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
}

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
  IN PCI_IO_DEVICE      *PciDev,
  IN PCI_RESOURCE_NODE  *IoNode,
  IN PCI_RESOURCE_NODE  *Mem32Node,
  IN PCI_RESOURCE_NODE  *PMem32Node,
  IN PCI_RESOURCE_NODE  *Mem64Node,
  IN PCI_RESOURCE_NODE  *PMem64Node
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Ptr;
  PCI_RESOURCE_NODE                  *Node;
  UINT8                              DummyBarIndex;

  DummyBarIndex = 0;
  Ptr           = PciDev->ResourcePaddingDescriptors;

  while (((EFI_ACPI_END_TAG_DESCRIPTOR *)Ptr)->Desc != ACPI_END_TAG_DESCRIPTOR) {
    if ((Ptr->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) && (Ptr->ResType == ACPI_ADDRESS_SPACE_TYPE_IO)) {
      if (Ptr->AddrLen != 0) {
        Node = CreateResourceNode (
                 PciDev,
                 Ptr->AddrLen,
                 Ptr->AddrRangeMax,
                 DummyBarIndex,
                 PciBarTypeIo16,
                 PciResUsagePadding
                 );

        if (Node != NULL) {
          InsertResourceNode (
            IoNode,
            Node
            );
        }
      }

      Ptr++;
      continue;
    }

    if ((Ptr->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) && (Ptr->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM)) {
      if (Ptr->AddrSpaceGranularity == 32) {
        //
        // prefetchable
        //
        if (Ptr->SpecificFlag == 0x6) {
          if (Ptr->AddrLen != 0) {
            Node = CreateResourceNode (
                     PciDev,
                     Ptr->AddrLen,
                     Ptr->AddrRangeMax,
                     DummyBarIndex,
                     PciBarTypePMem32,
                     PciResUsagePadding
                     );
            if (Node != NULL) {
              InsertResourceNode (
                PMem32Node,
                Node
                );
            }
          }

          Ptr++;
          continue;
        }

        //
        // Non-prefetchable
        //
        if (Ptr->SpecificFlag == 0) {
          if (Ptr->AddrLen != 0) {
            Node = CreateResourceNode (
                     PciDev,
                     Ptr->AddrLen,
                     Ptr->AddrRangeMax,
                     DummyBarIndex,
                     PciBarTypeMem32,
                     PciResUsagePadding
                     );
            if (Node != NULL) {
              InsertResourceNode (
                Mem32Node,
                Node
                );
            }
          }

          Ptr++;
          continue;
        }
      }

      if (Ptr->AddrSpaceGranularity == 64) {
        //
        // prefetchable
        //
        if (Ptr->SpecificFlag == 0x6) {
          if (Ptr->AddrLen != 0) {
            Node = CreateResourceNode (
                     PciDev,
                     Ptr->AddrLen,
                     Ptr->AddrRangeMax,
                     DummyBarIndex,
                     PciBarTypePMem64,
                     PciResUsagePadding
                     );
            if (Node != NULL) {
              InsertResourceNode (
                PMem64Node,
                Node
                );
            }
          }

          Ptr++;
          continue;
        }

        //
        // Non-prefetchable
        //
        if (Ptr->SpecificFlag == 0) {
          if (Ptr->AddrLen != 0) {
            Node = CreateResourceNode (
                     PciDev,
                     Ptr->AddrLen,
                     Ptr->AddrRangeMax,
                     DummyBarIndex,
                     PciBarTypeMem64,
                     PciResUsagePadding
                     );
            if (Node != NULL) {
              InsertResourceNode (
                Mem64Node,
                Node
                );
            }
          }

          Ptr++;
          continue;
        }
      }
    }

    Ptr++;
  }
}

/**
  Get padding resource for PCI-PCI bridge.

  @param  PciIoDevice     PCI-PCI bridge device instance.

  @note   Feature flag PcdPciBusHotplugDeviceSupport determines
          whether need to pad resource for them.
**/
VOID
GetResourcePaddingPpb (
  IN  PCI_IO_DEVICE  *PciIoDevice
  )
{
  if ((gPciHotPlugInit != NULL) && FeaturePcdGet (PcdPciBusHotplugDeviceSupport)) {
    if (PciIoDevice->ResourcePaddingDescriptors == NULL) {
      GetResourcePaddingForHpb (PciIoDevice);
    }
  }
}

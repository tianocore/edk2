/** @file

  Provides the basic interfaces to abstract a PCI Host Bridge Resource Allocation.

Copyright (c) 1999 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PciHostBridge.h"
#include "PciRootBridge.h"
#include "PciHostResource.h"


EFI_METRONOME_ARCH_PROTOCOL *mMetronome;
EFI_CPU_IO2_PROTOCOL        *mCpuIo;

GLOBAL_REMOVE_IF_UNREFERENCED CHAR16 *mAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};
GLOBAL_REMOVE_IF_UNREFERENCED CHAR16 *mPciResourceTypeStr[] = {
  L"I/O", L"Mem", L"PMem", L"Mem64", L"PMem64", L"Bus"
};

EDKII_IOMMU_PROTOCOL        *mIoMmuProtocol;
EFI_EVENT                   mIoMmuEvent;
VOID                        *mIoMmuRegistration;

/**
  This routine gets translation offset from a root bridge instance by resource type.

  @param RootBridge The Root Bridge Instance for the resources.
  @param ResourceType The Resource Type of the translation offset.

  @retval The Translation Offset of the specified resource.
**/
UINT64
GetTranslationByResourceType (
  IN  PCI_ROOT_BRIDGE_INSTANCE     *RootBridge,
  IN  PCI_RESOURCE_TYPE            ResourceType
  )
{
  switch (ResourceType) {
    case TypeIo:
      return RootBridge->Io.Translation;
    case TypeMem32:
      return RootBridge->Mem.Translation;
    case TypePMem32:
      return RootBridge->PMem.Translation;
    case TypeMem64:
      return RootBridge->MemAbove4G.Translation;
    case TypePMem64:
      return RootBridge->PMemAbove4G.Translation;
    case TypeBus:
      return RootBridge->Bus.Translation;
    default:
      ASSERT (FALSE);
      return 0;
  }
}

/**
  Ensure the compatibility of an IO space descriptor with the IO aperture.

  The IO space descriptor can come from the GCD IO space map, or it can
  represent a gap between two neighboring IO space descriptors. In the latter
  case, the GcdIoType field is expected to be EfiGcdIoTypeNonExistent.

  If the IO space descriptor already has type EfiGcdIoTypeIo, then no action is
  taken -- it is by definition compatible with the aperture.

  Otherwise, the intersection of the IO space descriptor is calculated with the
  aperture. If the intersection is the empty set (no overlap), no action is
  taken; the IO space descriptor is compatible with the aperture.

  Otherwise, the type of the descriptor is investigated again. If the type is
  EfiGcdIoTypeNonExistent (representing a gap, or a genuine descriptor with
  such a type), then an attempt is made to add the intersection as IO space to
  the GCD IO space map. This ensures continuity for the aperture, and the
  descriptor is deemed compatible with the aperture.

  Otherwise, the IO space descriptor is incompatible with the IO aperture.

  @param[in] Base        Base address of the aperture.
  @param[in] Length      Length of the aperture.
  @param[in] Descriptor  The descriptor to ensure compatibility with the
                         aperture for.

  @retval EFI_SUCCESS            The descriptor is compatible. The GCD IO space
                                 map may have been updated, for continuity
                                 within the aperture.
  @retval EFI_INVALID_PARAMETER  The descriptor is incompatible.
  @return                        Error codes from gDS->AddIoSpace().
**/
EFI_STATUS
IntersectIoDescriptor (
  IN  UINT64                            Base,
  IN  UINT64                            Length,
  IN  CONST EFI_GCD_IO_SPACE_DESCRIPTOR *Descriptor
  )
{
  UINT64                                IntersectionBase;
  UINT64                                IntersectionEnd;
  EFI_STATUS                            Status;

  if (Descriptor->GcdIoType == EfiGcdIoTypeIo) {
    return EFI_SUCCESS;
  }

  IntersectionBase = MAX (Base, Descriptor->BaseAddress);
  IntersectionEnd = MIN (Base + Length,
                      Descriptor->BaseAddress + Descriptor->Length);
  if (IntersectionBase >= IntersectionEnd) {
    //
    // The descriptor and the aperture don't overlap.
    //
    return EFI_SUCCESS;
  }

  if (Descriptor->GcdIoType == EfiGcdIoTypeNonExistent) {
    Status = gDS->AddIoSpace (EfiGcdIoTypeIo, IntersectionBase,
                    IntersectionEnd - IntersectionBase);

    DEBUG ((EFI_ERROR (Status) ? EFI_D_ERROR : EFI_D_VERBOSE,
      "%a: %a: add [%Lx, %Lx): %r\n", gEfiCallerBaseName, __FUNCTION__,
      IntersectionBase, IntersectionEnd, Status));
    return Status;
  }

  DEBUG ((EFI_D_ERROR, "%a: %a: desc [%Lx, %Lx) type %u conflicts with "
    "aperture [%Lx, %Lx)\n", gEfiCallerBaseName, __FUNCTION__,
    Descriptor->BaseAddress, Descriptor->BaseAddress + Descriptor->Length,
    (UINT32)Descriptor->GcdIoType, Base, Base + Length));
  return EFI_INVALID_PARAMETER;
}

/**
  Add IO space to GCD.
  The routine checks the GCD database and only adds those which are
  not added in the specified range to GCD.

  @param Base   Base address of the IO space.
  @param Length Length of the IO space.

  @retval EFI_SUCCES The IO space was added successfully.
**/
EFI_STATUS
AddIoSpace (
  IN  UINT64                        Base,
  IN  UINT64                        Length
  )
{
  EFI_STATUS                        Status;
  UINTN                             Index;
  UINTN                             NumberOfDescriptors;
  EFI_GCD_IO_SPACE_DESCRIPTOR       *IoSpaceMap;

  Status = gDS->GetIoSpaceMap (&NumberOfDescriptors, &IoSpaceMap);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: %a: GetIoSpaceMap(): %r\n",
      gEfiCallerBaseName, __FUNCTION__, Status));
    return Status;
  }

  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    Status = IntersectIoDescriptor (Base, Length, &IoSpaceMap[Index]);
    if (EFI_ERROR (Status)) {
      goto FreeIoSpaceMap;
    }
  }

  DEBUG_CODE (
    //
    // Make sure there are adjacent descriptors covering [Base, Base + Length).
    // It is possible that they have not been merged; merging can be prevented
    // by allocation.
    //
    UINT64                      CheckBase;
    EFI_STATUS                  CheckStatus;
    EFI_GCD_IO_SPACE_DESCRIPTOR Descriptor;

    for (CheckBase = Base;
         CheckBase < Base + Length;
         CheckBase = Descriptor.BaseAddress + Descriptor.Length) {
      CheckStatus = gDS->GetIoSpaceDescriptor (CheckBase, &Descriptor);
      ASSERT_EFI_ERROR (CheckStatus);
      ASSERT (Descriptor.GcdIoType == EfiGcdIoTypeIo);
    }
    );

FreeIoSpaceMap:
  FreePool (IoSpaceMap);

  return Status;
}

/**
  Ensure the compatibility of a memory space descriptor with the MMIO aperture.

  The memory space descriptor can come from the GCD memory space map, or it can
  represent a gap between two neighboring memory space descriptors. In the
  latter case, the GcdMemoryType field is expected to be
  EfiGcdMemoryTypeNonExistent.

  If the memory space descriptor already has type
  EfiGcdMemoryTypeMemoryMappedIo, and its capabilities are a superset of the
  required capabilities, then no action is taken -- it is by definition
  compatible with the aperture.

  Otherwise, the intersection of the memory space descriptor is calculated with
  the aperture. If the intersection is the empty set (no overlap), no action is
  taken; the memory space descriptor is compatible with the aperture.

  Otherwise, the type of the descriptor is investigated again. If the type is
  EfiGcdMemoryTypeNonExistent (representing a gap, or a genuine descriptor with
  such a type), then an attempt is made to add the intersection as MMIO space
  to the GCD memory space map, with the specified capabilities. This ensures
  continuity for the aperture, and the descriptor is deemed compatible with the
  aperture.

  Otherwise, the memory space descriptor is incompatible with the MMIO
  aperture.

  @param[in] Base         Base address of the aperture.
  @param[in] Length       Length of the aperture.
  @param[in] Capabilities Capabilities required by the aperture.
  @param[in] Descriptor   The descriptor to ensure compatibility with the
                          aperture for.

  @retval EFI_SUCCESS            The descriptor is compatible. The GCD memory
                                 space map may have been updated, for
                                 continuity within the aperture.
  @retval EFI_INVALID_PARAMETER  The descriptor is incompatible.
  @return                        Error codes from gDS->AddMemorySpace().
**/
EFI_STATUS
IntersectMemoryDescriptor (
  IN  UINT64                                Base,
  IN  UINT64                                Length,
  IN  UINT64                                Capabilities,
  IN  CONST EFI_GCD_MEMORY_SPACE_DESCRIPTOR *Descriptor
  )
{
  UINT64                                    IntersectionBase;
  UINT64                                    IntersectionEnd;
  EFI_STATUS                                Status;

  if (Descriptor->GcdMemoryType == EfiGcdMemoryTypeMemoryMappedIo &&
      (Descriptor->Capabilities & Capabilities) == Capabilities) {
    return EFI_SUCCESS;
  }

  IntersectionBase = MAX (Base, Descriptor->BaseAddress);
  IntersectionEnd = MIN (Base + Length,
                      Descriptor->BaseAddress + Descriptor->Length);
  if (IntersectionBase >= IntersectionEnd) {
    //
    // The descriptor and the aperture don't overlap.
    //
    return EFI_SUCCESS;
  }

  if (Descriptor->GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
    Status = gDS->AddMemorySpace (EfiGcdMemoryTypeMemoryMappedIo,
                    IntersectionBase, IntersectionEnd - IntersectionBase,
                    Capabilities);

    DEBUG ((EFI_ERROR (Status) ? EFI_D_ERROR : EFI_D_VERBOSE,
      "%a: %a: add [%Lx, %Lx): %r\n", gEfiCallerBaseName, __FUNCTION__,
      IntersectionBase, IntersectionEnd, Status));
    return Status;
  }

  DEBUG ((EFI_D_ERROR, "%a: %a: desc [%Lx, %Lx) type %u cap %Lx conflicts "
    "with aperture [%Lx, %Lx) cap %Lx\n", gEfiCallerBaseName, __FUNCTION__,
    Descriptor->BaseAddress, Descriptor->BaseAddress + Descriptor->Length,
    (UINT32)Descriptor->GcdMemoryType, Descriptor->Capabilities,
    Base, Base + Length, Capabilities));
  return EFI_INVALID_PARAMETER;
}

/**
  Add MMIO space to GCD.
  The routine checks the GCD database and only adds those which are
  not added in the specified range to GCD.

  @param Base         Base address of the MMIO space.
  @param Length       Length of the MMIO space.
  @param Capabilities Capabilities of the MMIO space.

  @retval EFI_SUCCES The MMIO space was added successfully.
**/
EFI_STATUS
AddMemoryMappedIoSpace (
  IN  UINT64                            Base,
  IN  UINT64                            Length,
  IN  UINT64                            Capabilities
  )
{
  EFI_STATUS                            Status;
  UINTN                                 Index;
  UINTN                                 NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR       *MemorySpaceMap;

  Status = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: %a: GetMemorySpaceMap(): %r\n",
      gEfiCallerBaseName, __FUNCTION__, Status));
    return Status;
  }

  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    Status = IntersectMemoryDescriptor (Base, Length, Capabilities,
               &MemorySpaceMap[Index]);
    if (EFI_ERROR (Status)) {
      goto FreeMemorySpaceMap;
    }
  }

  DEBUG_CODE (
    //
    // Make sure there are adjacent descriptors covering [Base, Base + Length).
    // It is possible that they have not been merged; merging can be prevented
    // by allocation and different capabilities.
    //
    UINT64                          CheckBase;
    EFI_STATUS                      CheckStatus;
    EFI_GCD_MEMORY_SPACE_DESCRIPTOR Descriptor;

    for (CheckBase = Base;
         CheckBase < Base + Length;
         CheckBase = Descriptor.BaseAddress + Descriptor.Length) {
      CheckStatus = gDS->GetMemorySpaceDescriptor (CheckBase, &Descriptor);
      ASSERT_EFI_ERROR (CheckStatus);
      ASSERT (Descriptor.GcdMemoryType == EfiGcdMemoryTypeMemoryMappedIo);
      ASSERT ((Descriptor.Capabilities & Capabilities) == Capabilities);
    }
    );

FreeMemorySpaceMap:
  FreePool (MemorySpaceMap);

  return Status;
}

/**
  Event notification that is fired when IOMMU protocol is installed.

  @param  Event                 The Event that is being processed.
  @param  Context               Event Context.

**/
VOID
EFIAPI
IoMmuProtocolCallback (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS   Status;

  Status = gBS->LocateProtocol (&gEdkiiIoMmuProtocolGuid, NULL, (VOID **)&mIoMmuProtocol);
  if (!EFI_ERROR(Status)) {
    gBS->CloseEvent (mIoMmuEvent);
  }
}

/**

  Entry point of this driver.

  @param ImageHandle  Image handle of this driver.
  @param SystemTable  Pointer to standard EFI system table.

  @retval EFI_SUCCESS       Succeed.
  @retval EFI_DEVICE_ERROR  Fail to install PCI_ROOT_BRIDGE_IO protocol.

**/
EFI_STATUS
EFIAPI
InitializePciHostBridge (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                  Status;
  PCI_HOST_BRIDGE_INSTANCE    *HostBridge;
  PCI_ROOT_BRIDGE_INSTANCE    *RootBridge;
  PCI_ROOT_BRIDGE             *RootBridges;
  UINTN                       RootBridgeCount;
  UINTN                       Index;
  PCI_ROOT_BRIDGE_APERTURE    *MemApertures[4];
  UINTN                       MemApertureIndex;
  BOOLEAN                     ResourceAssigned;
  LIST_ENTRY                  *Link;
  UINT64                      HostAddress;

  RootBridges = PciHostBridgeGetRootBridges (&RootBridgeCount);
  if ((RootBridges == NULL) || (RootBridgeCount == 0)) {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->LocateProtocol (&gEfiMetronomeArchProtocolGuid, NULL, (VOID **) &mMetronome);
  ASSERT_EFI_ERROR (Status);
  Status = gBS->LocateProtocol (&gEfiCpuIo2ProtocolGuid, NULL, (VOID **) &mCpuIo);
  ASSERT_EFI_ERROR (Status);

  //
  // Most systems in the world including complex servers have only one Host Bridge.
  //
  HostBridge = AllocateZeroPool (sizeof (PCI_HOST_BRIDGE_INSTANCE));
  ASSERT (HostBridge != NULL);

  HostBridge->Signature        = PCI_HOST_BRIDGE_SIGNATURE;
  HostBridge->CanRestarted     = TRUE;
  InitializeListHead (&HostBridge->RootBridges);
  ResourceAssigned             = FALSE;

  //
  // Create Root Bridge Device Handle in this Host Bridge
  //
  for (Index = 0; Index < RootBridgeCount; Index++) {
    //
    // Create Root Bridge Handle Instance
    //
    RootBridge = CreateRootBridge (&RootBridges[Index]);
    ASSERT (RootBridge != NULL);
    if (RootBridge == NULL) {
      continue;
    }

    //
    // Make sure all root bridges share the same ResourceAssigned value.
    //
    if (Index == 0) {
      ResourceAssigned = RootBridges[Index].ResourceAssigned;
    } else {
      ASSERT (ResourceAssigned == RootBridges[Index].ResourceAssigned);
    }

    if (RootBridges[Index].Io.Base <= RootBridges[Index].Io.Limit) {
      //
      // Base and Limit in PCI_ROOT_BRIDGE_APERTURE are device address.
      // For GCD resource manipulation, we need to use host address.
      //
      HostAddress = TO_HOST_ADDRESS (RootBridges[Index].Io.Base,
        RootBridges[Index].Io.Translation);

      Status = AddIoSpace (
                 HostAddress,
                 RootBridges[Index].Io.Limit - RootBridges[Index].Io.Base + 1
                 );
      ASSERT_EFI_ERROR (Status);
      if (ResourceAssigned) {
        Status = gDS->AllocateIoSpace (
                        EfiGcdAllocateAddress,
                        EfiGcdIoTypeIo,
                        0,
                        RootBridges[Index].Io.Limit - RootBridges[Index].Io.Base + 1,
                        &HostAddress,
                        gImageHandle,
                        NULL
                        );
        ASSERT_EFI_ERROR (Status);
      }
    }

    //
    // Add all the Mem/PMem aperture to GCD
    // Mem/PMem shouldn't overlap with each other
    // Root bridge which needs to combine MEM and PMEM should only report
    // the MEM aperture in Mem
    //
    MemApertures[0] = &RootBridges[Index].Mem;
    MemApertures[1] = &RootBridges[Index].MemAbove4G;
    MemApertures[2] = &RootBridges[Index].PMem;
    MemApertures[3] = &RootBridges[Index].PMemAbove4G;

    for (MemApertureIndex = 0; MemApertureIndex < ARRAY_SIZE (MemApertures); MemApertureIndex++) {
      if (MemApertures[MemApertureIndex]->Base <= MemApertures[MemApertureIndex]->Limit) {
        //
        // Base and Limit in PCI_ROOT_BRIDGE_APERTURE are device address.
        // For GCD resource manipulation, we need to use host address.
        //
        HostAddress = TO_HOST_ADDRESS (MemApertures[MemApertureIndex]->Base,
          MemApertures[MemApertureIndex]->Translation);
        Status = AddMemoryMappedIoSpace (
                   HostAddress,
                   MemApertures[MemApertureIndex]->Limit - MemApertures[MemApertureIndex]->Base + 1,
                   EFI_MEMORY_UC
                   );
        ASSERT_EFI_ERROR (Status);
        Status = gDS->SetMemorySpaceAttributes (
                        HostAddress,
                        MemApertures[MemApertureIndex]->Limit - MemApertures[MemApertureIndex]->Base + 1,
                        EFI_MEMORY_UC
                        );
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_WARN, "PciHostBridge driver failed to set EFI_MEMORY_UC to MMIO aperture - %r.\n", Status));
        }
        if (ResourceAssigned) {
          Status = gDS->AllocateMemorySpace (
                          EfiGcdAllocateAddress,
                          EfiGcdMemoryTypeMemoryMappedIo,
                          0,
                          MemApertures[MemApertureIndex]->Limit - MemApertures[MemApertureIndex]->Base + 1,
                          &HostAddress,
                          gImageHandle,
                          NULL
                          );
          ASSERT_EFI_ERROR (Status);
        }
      }
    }
    //
    // Insert Root Bridge Handle Instance
    //
    InsertTailList (&HostBridge->RootBridges, &RootBridge->Link);
  }

  //
  // When resources were assigned, it's not needed to expose
  // PciHostBridgeResourceAllocation protocol.
  //
  if (!ResourceAssigned) {
    HostBridge->ResAlloc.NotifyPhase = NotifyPhase;
    HostBridge->ResAlloc.GetNextRootBridge = GetNextRootBridge;
    HostBridge->ResAlloc.GetAllocAttributes = GetAttributes;
    HostBridge->ResAlloc.StartBusEnumeration = StartBusEnumeration;
    HostBridge->ResAlloc.SetBusNumbers = SetBusNumbers;
    HostBridge->ResAlloc.SubmitResources = SubmitResources;
    HostBridge->ResAlloc.GetProposedResources = GetProposedResources;
    HostBridge->ResAlloc.PreprocessController = PreprocessController;

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &HostBridge->Handle,
                    &gEfiPciHostBridgeResourceAllocationProtocolGuid, &HostBridge->ResAlloc,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  for (Link = GetFirstNode (&HostBridge->RootBridges)
       ; !IsNull (&HostBridge->RootBridges, Link)
       ; Link = GetNextNode (&HostBridge->RootBridges, Link)
       ) {
    RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
    RootBridge->RootBridgeIo.ParentHandle = HostBridge->Handle;

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &RootBridge->Handle,
                    &gEfiDevicePathProtocolGuid, RootBridge->DevicePath,
                    &gEfiPciRootBridgeIoProtocolGuid, &RootBridge->RootBridgeIo,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }
  PciHostBridgeFreeRootBridges (RootBridges, RootBridgeCount);

  if (!EFI_ERROR (Status)) {
    mIoMmuEvent = EfiCreateProtocolNotifyEvent (
                    &gEdkiiIoMmuProtocolGuid,
                    TPL_CALLBACK,
                    IoMmuProtocolCallback,
                    NULL,
                    &mIoMmuRegistration
                    );
  }

  return Status;
}

/**
  This routine constructs the resource descriptors for all root bridges and call PciHostBridgeResourceConflict().

  @param HostBridge The Host Bridge Instance where the resource adjustment happens.
**/
VOID
ResourceConflict (
  IN  PCI_HOST_BRIDGE_INSTANCE *HostBridge
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Resources;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  EFI_ACPI_END_TAG_DESCRIPTOR       *End;
  PCI_ROOT_BRIDGE_INSTANCE          *RootBridge;
  LIST_ENTRY                        *Link;
  UINTN                             RootBridgeCount;
  PCI_RESOURCE_TYPE                 Index;
  PCI_RES_NODE                      *ResAllocNode;

  RootBridgeCount = 0;
  for (Link = GetFirstNode (&HostBridge->RootBridges)
       ; !IsNull (&HostBridge->RootBridges, Link)
       ; Link = GetNextNode (&HostBridge->RootBridges, Link)
       ) {
    RootBridgeCount++;
  }

  Resources = AllocatePool (
                RootBridgeCount * (TypeMax * sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR)) +
                sizeof (EFI_ACPI_END_TAG_DESCRIPTOR)
                );
  ASSERT (Resources != NULL);

  for (Link = GetFirstNode (&HostBridge->RootBridges), Descriptor = Resources
       ; !IsNull (&HostBridge->RootBridges, Link)
       ; Link = GetNextNode (&HostBridge->RootBridges, Link)
       ) {
    RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
    for (Index = TypeIo; Index < TypeMax; Index++) {
      ResAllocNode = &RootBridge->ResAllocNode[Index];

      Descriptor->Desc = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Descriptor->Len = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
      Descriptor->AddrRangeMin = ResAllocNode->Base;
      Descriptor->AddrRangeMax = ResAllocNode->Alignment;
      Descriptor->AddrLen      = ResAllocNode->Length;
      Descriptor->SpecificFlag = 0;
      switch (ResAllocNode->Type) {

      case TypeIo:
        Descriptor->ResType = ACPI_ADDRESS_SPACE_TYPE_IO;
        break;

      case TypePMem32:
        Descriptor->SpecificFlag = EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE;
      case TypeMem32:
        Descriptor->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
        Descriptor->AddrSpaceGranularity = 32;
        break;

      case TypePMem64:
        Descriptor->SpecificFlag = EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE;
      case TypeMem64:
        Descriptor->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
        Descriptor->AddrSpaceGranularity = 64;
        break;

      case TypeBus:
        Descriptor->ResType = ACPI_ADDRESS_SPACE_TYPE_BUS;
        break;

      default:
        break;
      }

      Descriptor++;
    }
    //
    // Terminate the root bridge resources.
    //
    End = (EFI_ACPI_END_TAG_DESCRIPTOR *) Descriptor;
    End->Desc = ACPI_END_TAG_DESCRIPTOR;
    End->Checksum = 0x0;

    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) (End + 1);
  }
  //
  // Terminate the host bridge resources.
  //
  End = (EFI_ACPI_END_TAG_DESCRIPTOR *) Descriptor;
  End->Desc = ACPI_END_TAG_DESCRIPTOR;
  End->Checksum = 0x0;

  DEBUG ((DEBUG_ERROR, "Call PciHostBridgeResourceConflict().\n"));
  PciHostBridgeResourceConflict (HostBridge->Handle, Resources);
  FreePool (Resources);
}

/**
  Allocate Length of MMIO or IO resource with alignment BitsOfAlignment
  from GCD range [BaseAddress, Limit).

  @param Mmio            TRUE for MMIO and FALSE for IO.
  @param Length          Length of the resource to allocate.
  @param BitsOfAlignment Alignment of the resource to allocate.
  @param BaseAddress     The starting address the allocation is from.
  @param Limit           The ending address the allocation is to.

  @retval  The base address of the allocated resource or MAX_UINT64 if allocation
           fails.
**/
UINT64
AllocateResource (
  BOOLEAN Mmio,
  UINT64  Length,
  UINTN   BitsOfAlignment,
  UINT64  BaseAddress,
  UINT64  Limit
  )
{
  EFI_STATUS Status;

  if (BaseAddress < Limit) {
    //
    // Have to make sure Aligment is handled since we are doing direct address allocation
    // Strictly speaking, alignment requirement should be applied to device
    // address instead of host address which is used in GCD manipulation below,
    // but as we restrict the alignment of Translation to be larger than any BAR
    // alignment in the root bridge, we can simplify the situation and consider
    // the same alignment requirement is also applied to host address.
    //
    BaseAddress = ALIGN_VALUE (BaseAddress, LShiftU64 (1, BitsOfAlignment));

    while (BaseAddress + Length <= Limit + 1) {
      if (Mmio) {
        Status = gDS->AllocateMemorySpace (
                        EfiGcdAllocateAddress,
                        EfiGcdMemoryTypeMemoryMappedIo,
                        BitsOfAlignment,
                        Length,
                        &BaseAddress,
                        gImageHandle,
                        NULL
                        );
      } else {
        Status = gDS->AllocateIoSpace (
                        EfiGcdAllocateAddress,
                        EfiGcdIoTypeIo,
                        BitsOfAlignment,
                        Length,
                        &BaseAddress,
                        gImageHandle,
                        NULL
                        );
      }

      if (!EFI_ERROR (Status)) {
        return BaseAddress;
      }
      BaseAddress += LShiftU64 (1, BitsOfAlignment);
    }
  }
  return MAX_UINT64;
}

/**

  Enter a certain phase of the PCI enumeration process.

  @param This   The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL instance.
  @param Phase  The phase during enumeration.

  @retval EFI_SUCCESS            Succeed.
  @retval EFI_INVALID_PARAMETER  Wrong phase parameter passed in.
  @retval EFI_NOT_READY          Resources have not been submitted yet.

**/
EFI_STATUS
EFIAPI
NotifyPhase (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE    Phase
  )
{
  PCI_HOST_BRIDGE_INSTANCE              *HostBridge;
  PCI_ROOT_BRIDGE_INSTANCE              *RootBridge;
  LIST_ENTRY                            *Link;
  EFI_PHYSICAL_ADDRESS                  BaseAddress;
  UINTN                                 BitsOfAlignment;
  UINT64                                Alignment;
  EFI_STATUS                            Status;
  EFI_STATUS                            ReturnStatus;
  PCI_RESOURCE_TYPE                     Index;
  PCI_RESOURCE_TYPE                     Index1;
  PCI_RESOURCE_TYPE                     Index2;
  BOOLEAN                               ResNodeHandled[TypeMax];
  UINT64                                MaxAlignment;
  UINT64                                Translation;

  HostBridge = PCI_HOST_BRIDGE_FROM_THIS (This);

  switch (Phase) {
  case EfiPciHostBridgeBeginEnumeration:
    if (!HostBridge->CanRestarted) {
      return EFI_NOT_READY;
    }
    //
    // Reset Root Bridge
    //
    for (Link = GetFirstNode (&HostBridge->RootBridges)
          ; !IsNull (&HostBridge->RootBridges, Link)
          ; Link = GetNextNode (&HostBridge->RootBridges, Link)
          ) {
      RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
      for (Index = TypeIo; Index < TypeMax; Index++) {
        RootBridge->ResAllocNode[Index].Type   = Index;
        RootBridge->ResAllocNode[Index].Base   = 0;
        RootBridge->ResAllocNode[Index].Length = 0;
        RootBridge->ResAllocNode[Index].Status = ResNone;

        RootBridge->ResourceSubmitted = FALSE;
      }
    }

    HostBridge->CanRestarted = TRUE;
    break;

  case EfiPciHostBridgeBeginBusAllocation:
    //
    // No specific action is required here, can perform any chipset specific programing
    //
    HostBridge->CanRestarted = FALSE;
    break;

  case EfiPciHostBridgeEndBusAllocation:
    //
    // No specific action is required here, can perform any chipset specific programing
    //
    break;

  case EfiPciHostBridgeBeginResourceAllocation:
    //
    // No specific action is required here, can perform any chipset specific programing
    //
    break;

  case EfiPciHostBridgeAllocateResources:
    ReturnStatus = EFI_SUCCESS;

    //
    // Make sure the resource for all root bridges has been submitted.
    //
    for (Link = GetFirstNode (&HostBridge->RootBridges)
         ; !IsNull (&HostBridge->RootBridges, Link)
         ; Link = GetNextNode (&HostBridge->RootBridges, Link)
         ) {
      RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
      if (!RootBridge->ResourceSubmitted) {
        return EFI_NOT_READY;
      }
    }

    DEBUG ((EFI_D_INFO, "PciHostBridge: NotifyPhase (AllocateResources)\n"));
    for (Link = GetFirstNode (&HostBridge->RootBridges)
         ; !IsNull (&HostBridge->RootBridges, Link)
         ; Link = GetNextNode (&HostBridge->RootBridges, Link)
         ) {
      for (Index = TypeIo; Index < TypeBus; Index++) {
        ResNodeHandled[Index] = FALSE;
      }

      RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
      DEBUG ((EFI_D_INFO, " RootBridge: %s\n", RootBridge->DevicePathStr));

      for (Index1 = TypeIo; Index1 < TypeBus; Index1++) {
        if (RootBridge->ResAllocNode[Index1].Status == ResNone) {
          ResNodeHandled[Index1] = TRUE;
        } else {
          //
          // Allocate the resource node with max alignment at first
          //
          MaxAlignment = 0;
          Index = TypeMax;
          for (Index2 = TypeIo; Index2 < TypeBus; Index2++) {
            if (ResNodeHandled[Index2]) {
              continue;
            }
            if (MaxAlignment <= RootBridge->ResAllocNode[Index2].Alignment) {
              MaxAlignment = RootBridge->ResAllocNode[Index2].Alignment;
              Index = Index2;
            }
          }

          ASSERT (Index < TypeMax);
          ResNodeHandled[Index] = TRUE;
          Alignment = RootBridge->ResAllocNode[Index].Alignment;
          BitsOfAlignment = LowBitSet64 (Alignment + 1);
          BaseAddress = MAX_UINT64;

          //
          // RESTRICTION: To simplify the situation, we require the alignment of
          // Translation must be larger than any BAR alignment in the same root
          // bridge, so that resource allocation alignment can be applied to
          // both device address and host address.
          //
          Translation = GetTranslationByResourceType (RootBridge, Index);
          if ((Translation & Alignment) != 0) {
            DEBUG ((DEBUG_ERROR, "[%a:%d] Translation %lx is not aligned to %lx!\n",
              __FUNCTION__, __LINE__, Translation, Alignment
              ));
            ASSERT ((Translation & Alignment) == 0);
            //
            // This may be caused by too large alignment or too small
            // Translation; pick the 1st possibility and return out of resource,
            // which can also go thru the same process for out of resource
            // outside the loop.
            //
            ReturnStatus = EFI_OUT_OF_RESOURCES;
            continue;
          }

          switch (Index) {
          case TypeIo:
            //
            // Base and Limit in PCI_ROOT_BRIDGE_APERTURE are device address.
            // For AllocateResource is manipulating GCD resource, we need to use
            // host address here.
            //
            BaseAddress = AllocateResource (
                            FALSE,
                            RootBridge->ResAllocNode[Index].Length,
                            MIN (15, BitsOfAlignment),
                            TO_HOST_ADDRESS (ALIGN_VALUE (RootBridge->Io.Base, Alignment + 1),
                              RootBridge->Io.Translation),
                            TO_HOST_ADDRESS (RootBridge->Io.Limit,
                              RootBridge->Io.Translation)
                            );
            break;

          case TypeMem64:
            BaseAddress = AllocateResource (
                            TRUE,
                            RootBridge->ResAllocNode[Index].Length,
                            MIN (63, BitsOfAlignment),
                            TO_HOST_ADDRESS (ALIGN_VALUE (RootBridge->MemAbove4G.Base, Alignment + 1),
                              RootBridge->MemAbove4G.Translation),
                            TO_HOST_ADDRESS (RootBridge->MemAbove4G.Limit,
                              RootBridge->MemAbove4G.Translation)
                            );
            if (BaseAddress != MAX_UINT64) {
              break;
            }
            //
            // If memory above 4GB is not available, try memory below 4GB
            //

          case TypeMem32:
            BaseAddress = AllocateResource (
                            TRUE,
                            RootBridge->ResAllocNode[Index].Length,
                            MIN (31, BitsOfAlignment),
                            TO_HOST_ADDRESS (ALIGN_VALUE (RootBridge->Mem.Base, Alignment + 1),
                              RootBridge->Mem.Translation),
                            TO_HOST_ADDRESS (RootBridge->Mem.Limit,
                              RootBridge->Mem.Translation)
                            );
            break;

          case TypePMem64:
            BaseAddress = AllocateResource (
                            TRUE,
                            RootBridge->ResAllocNode[Index].Length,
                            MIN (63, BitsOfAlignment),
                            TO_HOST_ADDRESS (ALIGN_VALUE (RootBridge->PMemAbove4G.Base, Alignment + 1),
                              RootBridge->PMemAbove4G.Translation),
                            TO_HOST_ADDRESS (RootBridge->PMemAbove4G.Limit,
                              RootBridge->PMemAbove4G.Translation)
                            );
            if (BaseAddress != MAX_UINT64) {
              break;
            }
            //
            // If memory above 4GB is not available, try memory below 4GB
            //
          case TypePMem32:
            BaseAddress = AllocateResource (
                            TRUE,
                            RootBridge->ResAllocNode[Index].Length,
                            MIN (31, BitsOfAlignment),
                            TO_HOST_ADDRESS (ALIGN_VALUE (RootBridge->PMem.Base, Alignment + 1),
                              RootBridge->PMem.Translation),
                            TO_HOST_ADDRESS (RootBridge->PMem.Limit,
                              RootBridge->PMem.Translation)
                            );
            break;

          default:
            ASSERT (FALSE);
            break;
          }

          DEBUG ((DEBUG_INFO, "  %s: Base/Length/Alignment = %lx/%lx/%lx - ",
                  mPciResourceTypeStr[Index], BaseAddress, RootBridge->ResAllocNode[Index].Length, Alignment));
          if (BaseAddress != MAX_UINT64) {
            RootBridge->ResAllocNode[Index].Base = BaseAddress;
            RootBridge->ResAllocNode[Index].Status = ResAllocated;
            DEBUG ((DEBUG_INFO, "Success\n"));
          } else {
            ReturnStatus = EFI_OUT_OF_RESOURCES;
            DEBUG ((DEBUG_ERROR, "Out Of Resource!\n"));
          }
        }
      }
    }

    if (ReturnStatus == EFI_OUT_OF_RESOURCES) {
      ResourceConflict (HostBridge);
    }

    //
    // Set resource to zero for nodes where allocation fails
    //
    for (Link = GetFirstNode (&HostBridge->RootBridges)
          ; !IsNull (&HostBridge->RootBridges, Link)
          ; Link = GetNextNode (&HostBridge->RootBridges, Link)
          ) {
      RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
      for (Index = TypeIo; Index < TypeBus; Index++) {
        if (RootBridge->ResAllocNode[Index].Status != ResAllocated) {
          RootBridge->ResAllocNode[Index].Length = 0;
        }
      }
    }
    return ReturnStatus;

  case EfiPciHostBridgeSetResources:
    //
    // HostBridgeInstance->CanRestarted = FALSE;
    //
    break;

  case EfiPciHostBridgeFreeResources:
    //
    // HostBridgeInstance->CanRestarted = FALSE;
    //
    ReturnStatus = EFI_SUCCESS;
    for (Link = GetFirstNode (&HostBridge->RootBridges)
         ; !IsNull (&HostBridge->RootBridges, Link)
         ; Link = GetNextNode (&HostBridge->RootBridges, Link)
         ) {
      RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
      for (Index = TypeIo; Index < TypeBus; Index++) {
        if (RootBridge->ResAllocNode[Index].Status == ResAllocated) {
          switch (Index) {
          case TypeIo:
            Status = gDS->FreeIoSpace (RootBridge->ResAllocNode[Index].Base, RootBridge->ResAllocNode[Index].Length);
            if (EFI_ERROR (Status)) {
              ReturnStatus = Status;
            }
            break;

          case TypeMem32:
          case TypePMem32:
          case TypeMem64:
          case TypePMem64:
            Status = gDS->FreeMemorySpace (RootBridge->ResAllocNode[Index].Base, RootBridge->ResAllocNode[Index].Length);
            if (EFI_ERROR (Status)) {
              ReturnStatus = Status;
            }
            break;

          default:
            ASSERT (FALSE);
            break;
          }

          RootBridge->ResAllocNode[Index].Type = Index;
          RootBridge->ResAllocNode[Index].Base = 0;
          RootBridge->ResAllocNode[Index].Length = 0;
          RootBridge->ResAllocNode[Index].Status = ResNone;
        }
      }

      RootBridge->ResourceSubmitted = FALSE;
    }

    HostBridge->CanRestarted = TRUE;
    return ReturnStatus;

  case EfiPciHostBridgeEndResourceAllocation:
    //
    // The resource allocation phase is completed.  No specific action is required
    // here. This notification can be used to perform any chipset specific programming.
    //
    break;

  case EfiPciHostBridgeEndEnumeration:
    //
    // The Host Bridge Enumeration is completed. No specific action is required here.
    // This notification can be used to perform any chipset specific programming.
    //
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**

  Return the device handle of the next PCI root bridge that is associated with
  this Host Bridge.

  @param This              The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  @param RootBridgeHandle  Returns the device handle of the next PCI Root Bridge.
                           On input, it holds the RootBridgeHandle returned by the most
                           recent call to GetNextRootBridge().The handle for the first
                           PCI Root Bridge is returned if RootBridgeHandle is NULL on input.

  @retval EFI_SUCCESS            Succeed.
  @retval EFI_NOT_FOUND          Next PCI root bridge not found.
  @retval EFI_INVALID_PARAMETER  Wrong parameter passed in.

**/
EFI_STATUS
EFIAPI
GetNextRootBridge (
  IN     EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN OUT EFI_HANDLE                                       *RootBridgeHandle
  )
{
  BOOLEAN                   ReturnNext;
  LIST_ENTRY                *Link;
  PCI_HOST_BRIDGE_INSTANCE  *HostBridge;
  PCI_ROOT_BRIDGE_INSTANCE  *RootBridge;

  if (RootBridgeHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HostBridge = PCI_HOST_BRIDGE_FROM_THIS (This);
  ReturnNext = (BOOLEAN) (*RootBridgeHandle == NULL);

  for (Link = GetFirstNode (&HostBridge->RootBridges)
      ; !IsNull (&HostBridge->RootBridges, Link)
      ; Link = GetNextNode (&HostBridge->RootBridges, Link)
      ) {
    RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
    if (ReturnNext) {
      *RootBridgeHandle = RootBridge->Handle;
      return EFI_SUCCESS;
    }

    ReturnNext = (BOOLEAN) (*RootBridgeHandle == RootBridge->Handle);
  }

  if (ReturnNext) {
    ASSERT (IsNull (&HostBridge->RootBridges, Link));
    return EFI_NOT_FOUND;
  } else {
    return EFI_INVALID_PARAMETER;
  }
}

/**

  Returns the attributes of a PCI Root Bridge.

  @param This              The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  @param RootBridgeHandle  The device handle of the PCI Root Bridge
                           that the caller is interested in.
  @param Attributes        The pointer to attributes of the PCI Root Bridge.

  @retval EFI_SUCCESS            Succeed.
  @retval EFI_INVALID_PARAMETER  Attributes parameter passed in is NULL or
                                 RootBridgeHandle is not an EFI_HANDLE
                                 that was returned on a previous call to
                                 GetNextRootBridge().

**/
EFI_STATUS
EFIAPI
GetAttributes (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT UINT64                                           *Attributes
  )
{
  LIST_ENTRY                *Link;
  PCI_HOST_BRIDGE_INSTANCE  *HostBridge;
  PCI_ROOT_BRIDGE_INSTANCE  *RootBridge;

  if (Attributes == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HostBridge = PCI_HOST_BRIDGE_FROM_THIS (This);
  for (Link = GetFirstNode (&HostBridge->RootBridges)
      ; !IsNull (&HostBridge->RootBridges, Link)
      ; Link = GetNextNode (&HostBridge->RootBridges, Link)
      ) {
    RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
    if (RootBridgeHandle == RootBridge->Handle) {
      *Attributes = RootBridge->AllocationAttributes;
      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_PARAMETER;
}

/**

  This is the request from the PCI enumerator to set up
  the specified PCI Root Bridge for bus enumeration process.

  @param This              The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  @param RootBridgeHandle  The PCI Root Bridge to be set up.
  @param Configuration     Pointer to the pointer to the PCI bus resource descriptor.

  @retval EFI_SUCCESS            Succeed.
  @retval EFI_OUT_OF_RESOURCES   Not enough pool to be allocated.
  @retval EFI_INVALID_PARAMETER  RootBridgeHandle is not a valid handle.

**/
EFI_STATUS
EFIAPI
StartBusEnumeration (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT VOID                                             **Configuration
  )
{
  LIST_ENTRY                *Link;
  PCI_HOST_BRIDGE_INSTANCE  *HostBridge;
  PCI_ROOT_BRIDGE_INSTANCE  *RootBridge;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  EFI_ACPI_END_TAG_DESCRIPTOR       *End;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HostBridge = PCI_HOST_BRIDGE_FROM_THIS (This);
  for (Link = GetFirstNode (&HostBridge->RootBridges)
       ; !IsNull (&HostBridge->RootBridges, Link)
       ; Link = GetNextNode (&HostBridge->RootBridges, Link)
       ) {
    RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
    if (RootBridgeHandle == RootBridge->Handle) {
      *Configuration = AllocatePool (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
      if (*Configuration == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) *Configuration;
      Descriptor->Desc                  = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Descriptor->Len                   = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
      Descriptor->ResType               = ACPI_ADDRESS_SPACE_TYPE_BUS;
      Descriptor->GenFlag               = 0;
      Descriptor->SpecificFlag          = 0;
      Descriptor->AddrSpaceGranularity  = 0;
      Descriptor->AddrRangeMin          = RootBridge->Bus.Base;
      Descriptor->AddrRangeMax          = 0;
      Descriptor->AddrTranslationOffset = 0;
      Descriptor->AddrLen               = RootBridge->Bus.Limit - RootBridge->Bus.Base + 1;

      End = (EFI_ACPI_END_TAG_DESCRIPTOR *) (Descriptor + 1);
      End->Desc = ACPI_END_TAG_DESCRIPTOR;
      End->Checksum = 0x0;

      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_PARAMETER;
}

/**

  This function programs the PCI Root Bridge hardware so that
  it decodes the specified PCI bus range.

  @param This              The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  @param RootBridgeHandle  The PCI Root Bridge whose bus range is to be programmed.
  @param Configuration     The pointer to the PCI bus resource descriptor.

  @retval EFI_SUCCESS            Succeed.
  @retval EFI_INVALID_PARAMETER  Wrong parameters passed in.

**/
EFI_STATUS
EFIAPI
SetBusNumbers (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_HANDLE                                       RootBridgeHandle,
  IN VOID                                             *Configuration
  )
{
  LIST_ENTRY                *Link;
  PCI_HOST_BRIDGE_INSTANCE  *HostBridge;
  PCI_ROOT_BRIDGE_INSTANCE  *RootBridge;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  EFI_ACPI_END_TAG_DESCRIPTOR       *End;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;
  End = (EFI_ACPI_END_TAG_DESCRIPTOR *) (Descriptor + 1);

  //
  // Check the Configuration is valid
  //
  if ((Descriptor->Desc != ACPI_ADDRESS_SPACE_DESCRIPTOR) ||
      (Descriptor->ResType != ACPI_ADDRESS_SPACE_TYPE_BUS) ||
      (End->Desc != ACPI_END_TAG_DESCRIPTOR)
     ) {
    return EFI_INVALID_PARAMETER;
  }

  HostBridge = PCI_HOST_BRIDGE_FROM_THIS (This);
  for (Link = GetFirstNode (&HostBridge->RootBridges)
       ; !IsNull (&HostBridge->RootBridges, Link)
       ; Link = GetNextNode (&HostBridge->RootBridges, Link)
       ) {
    RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
    if (RootBridgeHandle == RootBridge->Handle) {

      if (Descriptor->AddrLen == 0) {
        return EFI_INVALID_PARAMETER;
      }

      if ((Descriptor->AddrRangeMin < RootBridge->Bus.Base) ||
          (Descriptor->AddrRangeMin + Descriptor->AddrLen - 1 > RootBridge->Bus.Limit)
         ) {
        return EFI_INVALID_PARAMETER;
      }
      //
      // Update the Bus Range
      //
      RootBridge->ResAllocNode[TypeBus].Base    = Descriptor->AddrRangeMin;
      RootBridge->ResAllocNode[TypeBus].Length  = Descriptor->AddrLen;
      RootBridge->ResAllocNode[TypeBus].Status  = ResAllocated;
      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_PARAMETER;
}

/**

  Submits the I/O and memory resource requirements for the specified PCI Root Bridge.

  @param This              The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  @param RootBridgeHandle  The PCI Root Bridge whose I/O and memory resource requirements.
                           are being submitted.
  @param Configuration     The pointer to the PCI I/O and PCI memory resource descriptor.

  @retval EFI_SUCCESS            Succeed.
  @retval EFI_INVALID_PARAMETER  Wrong parameters passed in.
**/
EFI_STATUS
EFIAPI
SubmitResources (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_HANDLE                                       RootBridgeHandle,
  IN VOID                                             *Configuration
  )
{
  LIST_ENTRY                        *Link;
  PCI_HOST_BRIDGE_INSTANCE          *HostBridge;
  PCI_ROOT_BRIDGE_INSTANCE          *RootBridge;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  PCI_RESOURCE_TYPE                 Type;

  //
  // Check the input parameter: Configuration
  //
  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HostBridge = PCI_HOST_BRIDGE_FROM_THIS (This);
  for (Link = GetFirstNode (&HostBridge->RootBridges)
       ; !IsNull (&HostBridge->RootBridges, Link)
       ; Link = GetNextNode (&HostBridge->RootBridges, Link)
       ) {
    RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
    if (RootBridgeHandle == RootBridge->Handle) {
      DEBUG ((EFI_D_INFO, "PciHostBridge: SubmitResources for %s\n", RootBridge->DevicePathStr));
      //
      // Check the resource descriptors.
      // If the Configuration includes one or more invalid resource descriptors, all the resource
      // descriptors are ignored and the function returns EFI_INVALID_PARAMETER.
      //
      for (Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
        if (Descriptor->ResType > ACPI_ADDRESS_SPACE_TYPE_BUS) {
          return EFI_INVALID_PARAMETER;
        }

        DEBUG ((EFI_D_INFO, " %s: Granularity/SpecificFlag = %ld / %02x%s\n",
                mAcpiAddressSpaceTypeStr[Descriptor->ResType], Descriptor->AddrSpaceGranularity, Descriptor->SpecificFlag,
                (Descriptor->SpecificFlag & EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE) != 0 ? L" (Prefetchable)" : L""
                ));
        DEBUG ((EFI_D_INFO, "      Length/Alignment = 0x%lx / 0x%lx\n", Descriptor->AddrLen, Descriptor->AddrRangeMax));
        switch (Descriptor->ResType) {
        case ACPI_ADDRESS_SPACE_TYPE_MEM:
          if (Descriptor->AddrSpaceGranularity != 32 && Descriptor->AddrSpaceGranularity != 64) {
            return EFI_INVALID_PARAMETER;
          }
          if (Descriptor->AddrSpaceGranularity == 32 && Descriptor->AddrLen >= SIZE_4GB) {
            return EFI_INVALID_PARAMETER;
          }
          //
          // If the PCI root bridge does not support separate windows for nonprefetchable and
          // prefetchable memory, then the PCI bus driver needs to include requests for
          // prefetchable memory in the nonprefetchable memory pool.
          //
          if (((RootBridge->AllocationAttributes & EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM) != 0) &&
              ((Descriptor->SpecificFlag & EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE) != 0)
             ) {
            return EFI_INVALID_PARAMETER;
          }
        case ACPI_ADDRESS_SPACE_TYPE_IO:
          //
          // Check aligment, it should be of the form 2^n-1
          //
          if (GetPowerOfTwo64 (Descriptor->AddrRangeMax + 1) != (Descriptor->AddrRangeMax + 1)) {
            return EFI_INVALID_PARAMETER;
          }
          break;
        default:
          ASSERT (FALSE);
          break;
        }
      }
      if (Descriptor->Desc != ACPI_END_TAG_DESCRIPTOR) {
        return EFI_INVALID_PARAMETER;
      }

      for (Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
        if (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
          if (Descriptor->AddrSpaceGranularity == 32) {
            if ((Descriptor->SpecificFlag & EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE) != 0) {
              Type = TypePMem32;
            } else {
              Type = TypeMem32;
            }
          } else {
            ASSERT (Descriptor->AddrSpaceGranularity == 64);
            if ((Descriptor->SpecificFlag & EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE) != 0) {
              Type = TypePMem64;
            } else {
              Type = TypeMem64;
            }
          }
        } else {
          ASSERT (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_IO);
          Type = TypeIo;
        }
        RootBridge->ResAllocNode[Type].Length    = Descriptor->AddrLen;
        RootBridge->ResAllocNode[Type].Alignment = Descriptor->AddrRangeMax;
        RootBridge->ResAllocNode[Type].Status    = ResSubmitted;
      }
      RootBridge->ResourceSubmitted = TRUE;
      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_PARAMETER;
}

/**

  This function returns the proposed resource settings for the specified
  PCI Root Bridge.

  @param This              The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  @param RootBridgeHandle  The PCI Root Bridge handle.
  @param Configuration     The pointer to the pointer to the PCI I/O
                           and memory resource descriptor.

  @retval EFI_SUCCESS            Succeed.
  @retval EFI_OUT_OF_RESOURCES   Not enough pool to be allocated.
  @retval EFI_INVALID_PARAMETER  RootBridgeHandle is not a valid handle.

**/
EFI_STATUS
EFIAPI
GetProposedResources (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT VOID                                             **Configuration
  )
{
  LIST_ENTRY                        *Link;
  PCI_HOST_BRIDGE_INSTANCE          *HostBridge;
  PCI_ROOT_BRIDGE_INSTANCE          *RootBridge;
  UINTN                             Index;
  UINTN                             Number;
  VOID                              *Buffer;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  EFI_ACPI_END_TAG_DESCRIPTOR       *End;
  UINT64                            ResStatus;

  HostBridge = PCI_HOST_BRIDGE_FROM_THIS (This);
  for (Link = GetFirstNode (&HostBridge->RootBridges)
      ; !IsNull (&HostBridge->RootBridges, Link)
      ; Link = GetNextNode (&HostBridge->RootBridges, Link)
      ) {
    RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
    if (RootBridgeHandle == RootBridge->Handle) {
      for (Index = 0, Number = 0; Index < TypeBus; Index++) {
        if (RootBridge->ResAllocNode[Index].Status != ResNone) {
          Number++;
        }
      }

      Buffer = AllocateZeroPool (Number * sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
      if (Buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Buffer;
      for (Index = 0; Index < TypeBus; Index++) {
        ResStatus = RootBridge->ResAllocNode[Index].Status;
        if (ResStatus != ResNone) {
          Descriptor->Desc                  = ACPI_ADDRESS_SPACE_DESCRIPTOR;
          Descriptor->Len                   = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;;
          Descriptor->GenFlag               = 0;
          //
          // AddrRangeMin in Resource Descriptor here should be device address
          // instead of host address, or else PCI bus driver cannot set correct
          // address into PCI BAR registers.
          // Base in ResAllocNode is a host address, so conversion is needed.
          //
          Descriptor->AddrRangeMin          = TO_DEVICE_ADDRESS (RootBridge->ResAllocNode[Index].Base,
            GetTranslationByResourceType (RootBridge, Index));
          Descriptor->AddrRangeMax          = 0;
          Descriptor->AddrTranslationOffset = (ResStatus == ResAllocated) ? EFI_RESOURCE_SATISFIED : PCI_RESOURCE_LESS;
          Descriptor->AddrLen               = RootBridge->ResAllocNode[Index].Length;

          switch (Index) {

          case TypeIo:
            Descriptor->ResType              = ACPI_ADDRESS_SPACE_TYPE_IO;
            break;

          case TypePMem32:
            Descriptor->SpecificFlag = EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE;
          case TypeMem32:
            Descriptor->ResType              = ACPI_ADDRESS_SPACE_TYPE_MEM;
            Descriptor->AddrSpaceGranularity = 32;
            break;

          case TypePMem64:
            Descriptor->SpecificFlag = EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE;
          case TypeMem64:
            Descriptor->ResType              = ACPI_ADDRESS_SPACE_TYPE_MEM;
            Descriptor->AddrSpaceGranularity = 64;
            break;
          }

          Descriptor++;
        }
      }
      End = (EFI_ACPI_END_TAG_DESCRIPTOR *) Descriptor;
      End->Desc      = ACPI_END_TAG_DESCRIPTOR;
      End->Checksum  = 0;

      *Configuration = Buffer;

      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_PARAMETER;
}

/**

  This function is called for all the PCI controllers that the PCI
  bus driver finds. Can be used to Preprogram the controller.

  @param This              The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  @param RootBridgeHandle  The PCI Root Bridge handle.
  @param PciAddress        Address of the controller on the PCI bus.
  @param Phase             The Phase during resource allocation.

  @retval EFI_SUCCESS            Succeed.
  @retval EFI_INVALID_PARAMETER  RootBridgeHandle is not a valid handle.

**/
EFI_STATUS
EFIAPI
PreprocessController (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL          *This,
  IN  EFI_HANDLE                                                RootBridgeHandle,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS               PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE              Phase
  )
{
  LIST_ENTRY                *Link;
  PCI_HOST_BRIDGE_INSTANCE  *HostBridge;
  PCI_ROOT_BRIDGE_INSTANCE  *RootBridge;

  if ((UINT32) Phase > EfiPciBeforeResourceCollection) {
    return EFI_INVALID_PARAMETER;
  }

  HostBridge = PCI_HOST_BRIDGE_FROM_THIS (This);
  for (Link = GetFirstNode (&HostBridge->RootBridges)
       ; !IsNull (&HostBridge->RootBridges, Link)
       ; Link = GetNextNode (&HostBridge->RootBridges, Link)
       ) {
    RootBridge = ROOT_BRIDGE_FROM_LINK (Link);
    if (RootBridgeHandle == RootBridge->Handle) {
      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_PARAMETER;
}

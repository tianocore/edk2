/** @file

  Provides the basic interfaces to abstract a PCI Host Bridge Resource Allocation.

Copyright (c) 1999 - 2016, Intel Corporation. All rights reserved.<BR>
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

  HostBridge->ResAlloc.NotifyPhase          = NotifyPhase;
  HostBridge->ResAlloc.GetNextRootBridge    = GetNextRootBridge;
  HostBridge->ResAlloc.GetAllocAttributes   = GetAttributes;
  HostBridge->ResAlloc.StartBusEnumeration  = StartBusEnumeration;
  HostBridge->ResAlloc.SetBusNumbers        = SetBusNumbers;
  HostBridge->ResAlloc.SubmitResources      = SubmitResources;
  HostBridge->ResAlloc.GetProposedResources = GetProposedResources;
  HostBridge->ResAlloc.PreprocessController = PreprocessController;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &HostBridge->Handle,
                  &gEfiPciHostBridgeResourceAllocationProtocolGuid, &HostBridge->ResAlloc,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (HostBridge);
    PciHostBridgeFreeRootBridges (RootBridges, RootBridgeCount);
    return Status;
  }

  //
  // Create Root Bridge Device Handle in this Host Bridge
  //
  for (Index = 0; Index < RootBridgeCount; Index++) {
    //
    // Create Root Bridge Handle Instance
    //
    RootBridge = CreateRootBridge (&RootBridges[Index], HostBridge->Handle);
    ASSERT (RootBridge != NULL);
    if (RootBridge == NULL) {
      continue;
    }

    if (RootBridges[Index].Io.Limit > RootBridges[Index].Io.Base) {
      Status = gDS->AddIoSpace (
                      EfiGcdIoTypeIo,
                      RootBridges[Index].Io.Base,
                      RootBridges[Index].Io.Limit - RootBridges[Index].Io.Base + 1
                      );
      ASSERT_EFI_ERROR (Status);
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

    for (MemApertureIndex = 0; MemApertureIndex < sizeof (MemApertures) / sizeof (MemApertures[0]); MemApertureIndex++) {
      if (MemApertures[MemApertureIndex]->Limit > MemApertures[MemApertureIndex]->Base) {
        Status = gDS->AddMemorySpace (
                        EfiGcdMemoryTypeMemoryMappedIo,
                        MemApertures[MemApertureIndex]->Base,
                        MemApertures[MemApertureIndex]->Limit - MemApertures[MemApertureIndex]->Base + 1,
                        EFI_MEMORY_UC
                        );
        ASSERT_EFI_ERROR (Status);
        Status = gDS->SetMemorySpaceAttributes (
                        MemApertures[MemApertureIndex]->Base,
                        MemApertures[MemApertureIndex]->Limit - MemApertures[MemApertureIndex]->Base + 1,
                        EFI_MEMORY_UC
                        );
        ASSERT_EFI_ERROR (Status);
      }
    }
    //
    // Insert Root Bridge Handle Instance
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &RootBridge->Handle,
                    &gEfiDevicePathProtocolGuid, RootBridge->DevicePath,
                    &gEfiPciRootBridgeIoProtocolGuid, &RootBridge->RootBridgeIo,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
    InsertTailList (&HostBridge->RootBridges, &RootBridge->Link);
  }
  PciHostBridgeFreeRootBridges (RootBridges, RootBridgeCount);
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
      Descriptor->AddrLen = ResAllocNode->Length;
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
  UINT64                                AddrLen;
  UINTN                                 BitsOfAlignment;
  UINT64                                Alignment;
  EFI_STATUS                            Status;
  EFI_STATUS                            ReturnStatus;
  PCI_RESOURCE_TYPE                     Index;
  PCI_RESOURCE_TYPE                     Index1;
  PCI_RESOURCE_TYPE                     Index2;
  BOOLEAN                               ResNodeHandled[TypeMax];
  UINT64                                MaxAlignment;

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
          AddrLen = RootBridge->ResAllocNode[Index].Length;
          Alignment = RootBridge->ResAllocNode[Index].Alignment;
          BitsOfAlignment = LowBitSet64 (Alignment + 1);
          BaseAddress = MAX_UINT64;

          switch (Index) {
          case TypeIo:
            BaseAddress = AllocateResource (
                            FALSE,
                            RootBridge->ResAllocNode[Index].Length,
                            MIN (15, BitsOfAlignment),
                            ALIGN_VALUE (RootBridge->Io.Base, Alignment + 1),
                            RootBridge->Io.Limit
                            );
            break;

          case TypeMem64:
            BaseAddress = AllocateResource (
                            TRUE,
                            RootBridge->ResAllocNode[Index].Length,
                            MIN (63, BitsOfAlignment),
                            ALIGN_VALUE (RootBridge->MemAbove4G.Base, Alignment + 1),
                            RootBridge->MemAbove4G.Limit
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
                            ALIGN_VALUE (RootBridge->Mem.Base, Alignment + 1),
                            RootBridge->Mem.Limit
                            );
            break;

          case TypePMem64:
            BaseAddress = AllocateResource (
                            TRUE,
                            RootBridge->ResAllocNode[Index].Length,
                            MIN (63, BitsOfAlignment),
                            ALIGN_VALUE (RootBridge->PMemAbove4G.Base, Alignment + 1),
                            RootBridge->PMemAbove4G.Limit
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
                            ALIGN_VALUE (RootBridge->PMem.Base, Alignment + 1),
                            RootBridge->PMem.Limit
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
  UINTN                     BusStart;
  UINTN                     BusEnd;
  UINTN                     BusLen;

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
      BusStart  = (UINTN) Descriptor->AddrRangeMin;
      BusLen    = (UINTN) Descriptor->AddrLen;
      BusEnd    = BusStart + BusLen - 1;

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
          Descriptor->AddrRangeMin          = RootBridge->ResAllocNode[Index].Base;
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

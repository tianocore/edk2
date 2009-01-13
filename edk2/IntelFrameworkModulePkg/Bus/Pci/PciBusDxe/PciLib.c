/** @file

  PCI Bus Driver Lib file
  It abstracts some functions that can be different
  between light PCI bus driver and full PCI bus driver

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PciBus.h"

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PCI_HOTPLUG_REQUEST_PROTOCOL gPciHotPlugRequest = {
  PciHotPlugRequestNotify
};

/**
  Install protocol gEfiPciHotPlugRequestProtocolGuid
  @param Status    return status of protocol installation.
**/
VOID
InstallHotPlugRequestProtocol (
  IN EFI_STATUS *Status
  )
{
  EFI_HANDLE  Handle;

  if (!FeaturePcdGet (PcdPciBusHotplugDeviceSupport)) {
    return;
  }

  Handle = NULL;
  *Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiPciHotPlugRequestProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gPciHotPlugRequest
                  );
}

/**
  Install protocol gEfiPciHotplugDeviceGuid into hotplug device
  instance.
  
  @param PciIoDevice  hotplug device instance.
  
**/
VOID
InstallPciHotplugGuid (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  )
{
  EFI_STATUS  Status;

  if (!FeaturePcdGet (PcdPciBusHotplugDeviceSupport)) {
    return;
  }

  if (IS_CARDBUS_BRIDGE (&PciIoDevice->Parent->Pci)) {

    Status = gBS->InstallProtocolInterface (
                    &PciIoDevice->Handle,
                    &gEfiPciHotplugDeviceGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  UnInstall protocol gEfiPciHotplugDeviceGuid into hotplug device
  instance.
  
  @param PciIoDevice  hotplug device instance.
  
**/
VOID
UninstallPciHotplugGuid (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  )
{
  EFI_STATUS  Status;

  if (!FeaturePcdGet (PcdPciBusHotplugDeviceSupport)) {
    return;
  }

  Status = gBS->OpenProtocol (
                  PciIoDevice->Handle,
                  &gEfiPciHotplugDeviceGuid,
                  NULL,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if (Status == EFI_SUCCESS) {
    //
    // This may triger CardBus driver to stop for
    // Pccard devices opened the GUID via BY_DRIVER
    //
    Status = gBS->UninstallProtocolInterface (
                    PciIoDevice->Handle,
                    &gEfiPciHotplugDeviceGuid,
                    NULL
                    );
  }
}

/**
  Retrieve the BAR information via PciIo interface.
  
  @param PciIoDevice Pci device instance.
**/
VOID
GetBackPcCardBar (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  )
{
  UINT32  Address;

  if (!FeaturePcdGet (PcdPciBusHotplugDeviceSupport)) {
    return;
  }

  //
  // Read PciBar information from the bar register
  //
  if (!gFullEnumeration) {

    Address = 0;
    PciIoRead (
                            &(PciIoDevice->PciIo),
                            EfiPciIoWidthUint32,
                            0x1c,
                            1,
                            &Address
                            );

    (PciIoDevice->PciBar)[P2C_MEM_1].BaseAddress  = (UINT64) (Address);
    (PciIoDevice->PciBar)[P2C_MEM_1].Length       = 0x2000000;
    (PciIoDevice->PciBar)[P2C_MEM_1].BarType      = PciBarTypeMem32;

    Address = 0;
    PciIoRead (
                            &(PciIoDevice->PciIo),
                            EfiPciIoWidthUint32,
                            0x20,
                            1,
                            &Address
                            );
    (PciIoDevice->PciBar)[P2C_MEM_2].BaseAddress  = (UINT64) (Address);
    (PciIoDevice->PciBar)[P2C_MEM_2].Length       = 0x2000000;
    (PciIoDevice->PciBar)[P2C_MEM_2].BarType      = PciBarTypePMem32;

    Address = 0;
    PciIoRead (
                            &(PciIoDevice->PciIo),
                            EfiPciIoWidthUint32,
                            0x2c,
                            1,
                            &Address
                            );
    (PciIoDevice->PciBar)[P2C_IO_1].BaseAddress = (UINT64) (Address);
    (PciIoDevice->PciBar)[P2C_IO_1].Length      = 0x100;
    (PciIoDevice->PciBar)[P2C_IO_1].BarType     = PciBarTypeIo16;

    Address = 0;
    PciIoRead (
                            &(PciIoDevice->PciIo),
                            EfiPciIoWidthUint32,
                            0x34,
                            1,
                            &Address
                            );
    (PciIoDevice->PciBar)[P2C_IO_2].BaseAddress = (UINT64) (Address);
    (PciIoDevice->PciBar)[P2C_IO_2].Length      = 0x100;
    (PciIoDevice->PciBar)[P2C_IO_2].BarType     = PciBarTypeIo16;

  }

  if (gPciHotPlugInit != NULL) {
    GetResourcePaddingForHpb (PciIoDevice);
  }
}

/**
  Remove rejected pci device from specific root bridge
  handle.
  
  @param RootBridgeHandle  specific parent root bridge handle.
  @param Bridge            Bridge device instance.
  
  @retval EFI_SUCCESS  Success operation.
**/
EFI_STATUS
RemoveRejectedPciDevices (
  EFI_HANDLE        RootBridgeHandle,
  IN PCI_IO_DEVICE  *Bridge
  )
{
  PCI_IO_DEVICE   *Temp;
  LIST_ENTRY      *CurrentLink;
  LIST_ENTRY      *LastLink;

  if (!FeaturePcdGet (PcdPciBusHotplugDeviceSupport)) {
    return EFI_SUCCESS;
  }

  CurrentLink = Bridge->ChildList.ForwardLink;

  while (CurrentLink != NULL && CurrentLink != &Bridge->ChildList) {

    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);

    if (IS_PCI_BRIDGE (&Temp->Pci)) {
      //
      // Remove rejected devices recusively
      //
      RemoveRejectedPciDevices (RootBridgeHandle, Temp);
    } else {
      //
      // Skip rejection for all PPBs, while detect rejection for others
      //
      if (IsPciDeviceRejected (Temp)) {

        //
        // For P2C, remove all devices on it
        //

        if (!IsListEmpty (&Temp->ChildList)) {
          RemoveAllPciDeviceOnBridge (RootBridgeHandle, Temp);
        }

        //
        // Finally remove itself
        //

        LastLink = CurrentLink->BackLink;
        RemoveEntryList (CurrentLink);
        FreePciDevice (Temp);

        CurrentLink = LastLink;
      }
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return EFI_SUCCESS;
}

/**
  Wrapper function for allocating resource for pci host bridge.
  
  @param PciResAlloc Point to protocol instance EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.
  
**/
EFI_STATUS
PciHostBridgeResourceAllocator (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  )
{
  if (FeaturePcdGet (PcdPciBusHotplugDeviceSupport)) {
    return PciHostBridgeResourceAllocator_WithHotPlugDeviceSupport (
             PciResAlloc
             );
  } else {
    return PciHostBridgeResourceAllocator_WithoutHotPlugDeviceSupport (
             PciResAlloc
             );
  }
}

/**
  Submits the I/O and memory resource requirements for the specified PCI Root Bridge.

  @param PciResAlloc  Point to protocol instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.

  @retval EFI_SUCCESS           Success.
**/
EFI_STATUS
PciHostBridgeResourceAllocator_WithoutHotPlugDeviceSupport (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  )
{
  PCI_IO_DEVICE                   *RootBridgeDev;
  EFI_HANDLE                      RootBridgeHandle;
  VOID                            *AcpiConfig;
  EFI_STATUS                      Status;
  UINT64                          IoBase;
  UINT64                          Mem32Base;
  UINT64                          PMem32Base;
  UINT64                          Mem64Base;
  UINT64                          PMem64Base;
  UINT64                          MaxOptionRomSize;
  PCI_RESOURCE_NODE               *IoBridge;
  PCI_RESOURCE_NODE               *Mem32Bridge;
  PCI_RESOURCE_NODE               *PMem32Bridge;
  PCI_RESOURCE_NODE               *Mem64Bridge;
  PCI_RESOURCE_NODE               *PMem64Bridge;
  PCI_RESOURCE_NODE               IoPool;
  PCI_RESOURCE_NODE               Mem32Pool;
  PCI_RESOURCE_NODE               PMem32Pool;
  PCI_RESOURCE_NODE               Mem64Pool;
  PCI_RESOURCE_NODE               PMem64Pool;
  EFI_DEVICE_HANDLE_EXTENDED_DATA_PAYLOAD  ExtendedData;

  //
  // Initialize resource pool
  //

  InitializeResourcePool (&IoPool, PciBarTypeIo16);
  InitializeResourcePool (&Mem32Pool, PciBarTypeMem32);
  InitializeResourcePool (&PMem32Pool, PciBarTypePMem32);
  InitializeResourcePool (&Mem64Pool, PciBarTypeMem64);
  InitializeResourcePool (&PMem64Pool, PciBarTypePMem64);

  RootBridgeDev     = NULL;
  RootBridgeHandle  = 0;

  while (PciResAlloc->GetNextRootBridge (PciResAlloc, &RootBridgeHandle) == EFI_SUCCESS) {
    //
    // Get RootBridg Device by handle
    //
    RootBridgeDev = GetRootBridgeByHandle (RootBridgeHandle);

    if (RootBridgeDev == NULL) {
      return EFI_NOT_FOUND;
    }

    //
    // Get host bridge handle for status report
    //
    ExtendedData.Handle = RootBridgeDev->PciRootBridgeIo->ParentHandle;

    //
    // Create the entire system resource map from the information collected by
    // enumerator. Several resource tree was created
    //

    IoBridge = CreateResourceNode (
                RootBridgeDev,
                0,
                0xFFF,
                0,
                PciBarTypeIo16,
                PciResUsageTypical
                );

    Mem32Bridge = CreateResourceNode (
                    RootBridgeDev,
                    0,
                    0xFFFFF,
                    0,
                    PciBarTypeMem32,
                    PciResUsageTypical
                    );

    PMem32Bridge = CreateResourceNode (
                    RootBridgeDev,
                    0,
                    0xFFFFF,
                    0,
                    PciBarTypePMem32,
                    PciResUsageTypical
                    );

    Mem64Bridge = CreateResourceNode (
                    RootBridgeDev,
                    0,
                    0xFFFFF,
                    0,
                    PciBarTypeMem64,
                    PciResUsageTypical
                    );

    PMem64Bridge = CreateResourceNode (
                    RootBridgeDev,
                    0,
                    0xFFFFF,
                    0,
                    PciBarTypePMem64,
                    PciResUsageTypical
                    );

    //
    // Create resourcemap by going through all the devices subject to this root bridge
    //
    Status = CreateResourceMap (
              RootBridgeDev,
              IoBridge,
              Mem32Bridge,
              PMem32Bridge,
              Mem64Bridge,
              PMem64Bridge
              );

    //
    // Get the max ROM size that the root bridge can process
    //
    RootBridgeDev->RomSize = Mem32Bridge->Length;

    //
    // Get Max Option Rom size for current root bridge
    //
    MaxOptionRomSize = GetMaxOptionRomSize (RootBridgeDev);

    //
    // Enlarger the mem32 resource to accomdate the option rom
    // if the mem32 resource is not enough to hold the rom
    //
    if (MaxOptionRomSize > Mem32Bridge->Length) {

      Mem32Bridge->Length     = MaxOptionRomSize;
      RootBridgeDev->RomSize  = MaxOptionRomSize;

      //
      // Alignment should be adjusted as well
      //
      if (Mem32Bridge->Alignment < MaxOptionRomSize - 1) {
        Mem32Bridge->Alignment = MaxOptionRomSize - 1;
      }
    }

    //
    // Based on the all the resource tree, contruct ACPI resource node to
    // submit the resource aperture to pci host bridge protocol
    //
    Status = ConstructAcpiResourceRequestor (
              RootBridgeDev,
              IoBridge,
              Mem32Bridge,
              PMem32Bridge,
              Mem64Bridge,
              PMem64Bridge,
              &AcpiConfig
              );

    //
    // Insert these resource nodes into the database
    //
    InsertResourceNode (&IoPool, IoBridge);
    InsertResourceNode (&Mem32Pool, Mem32Bridge);
    InsertResourceNode (&PMem32Pool, PMem32Bridge);
    InsertResourceNode (&Mem64Pool, Mem64Bridge);
    InsertResourceNode (&PMem64Pool, PMem64Bridge);

    if (Status == EFI_SUCCESS) {
      //
      // Submit the resource requirement
      //
      Status = PciResAlloc->SubmitResources (
                              PciResAlloc,
                              RootBridgeDev->Handle,
                              AcpiConfig
                              );
    }
    //
    // Free acpi resource node
    //
    if (AcpiConfig != NULL) {
      FreePool (AcpiConfig);
    }

    if (EFI_ERROR (Status)) {
      //
      // Destroy all the resource tree
      //
      DestroyResourceTree (&IoPool);
      DestroyResourceTree (&Mem32Pool);
      DestroyResourceTree (&PMem32Pool);
      DestroyResourceTree (&Mem64Pool);
      DestroyResourceTree (&PMem64Pool);
      return Status;
    }
  }
  //
  // End while
  //

  //
  // Notify pci bus driver starts to program the resource
  //
  Status = NotifyPhase (PciResAlloc, EfiPciHostBridgeAllocateResources);

  if (EFI_ERROR (Status)) {
    //
    // Allocation failed, then return
    //
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Raise the EFI_IOB_PCI_RES_ALLOC status code
  //
  REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
        EFI_PROGRESS_CODE,
        EFI_IO_BUS_PCI | EFI_IOB_PCI_PC_RES_ALLOC,
        (VOID *) &ExtendedData,
        sizeof (ExtendedData)
        );

  //
  // Notify pci bus driver starts to program the resource
  //
  NotifyPhase (PciResAlloc, EfiPciHostBridgeSetResources);

  RootBridgeDev     = NULL;

  RootBridgeHandle  = 0;

  while (PciResAlloc->GetNextRootBridge (PciResAlloc, &RootBridgeHandle) == EFI_SUCCESS) {
    //
    // Get RootBridg Device by handle
    //
    RootBridgeDev = GetRootBridgeByHandle (RootBridgeHandle);

    if (RootBridgeDev == NULL) {
      return EFI_NOT_FOUND;
    }

    //
    // Get acpi resource node for all the resource types
    //
    AcpiConfig = NULL;
    Status = PciResAlloc->GetProposedResources (
                            PciResAlloc,
                            RootBridgeDev->Handle,
                            &AcpiConfig
                            );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Get the resource base by interpreting acpi resource node
    //
    //
    GetResourceBase (
      AcpiConfig,
      &IoBase,
      &Mem32Base,
      &PMem32Base,
      &Mem64Base,
      &PMem64Base
      );

    //
    // Process option rom for this root bridge
    //
    Status = ProcessOptionRom (RootBridgeDev, Mem32Base, RootBridgeDev->RomSize);

    //
    // Create the entire system resource map from the information collected by
    // enumerator. Several resource tree was created
    //
    Status = GetResourceMap (
              RootBridgeDev,
              &IoBridge,
              &Mem32Bridge,
              &PMem32Bridge,
              &Mem64Bridge,
              &PMem64Bridge,
              &IoPool,
              &Mem32Pool,
              &PMem32Pool,
              &Mem64Pool,
              &PMem64Pool
              );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Program IO resources
    //
    ProgramResource (
      IoBase,
      IoBridge
      );

    //
    // Program Mem32 resources
    //
    ProgramResource (
      Mem32Base,
      Mem32Bridge
      );

    //
    // Program PMem32 resources
    //
    ProgramResource (
      PMem32Base,
      PMem32Bridge
      );

    //
    // Program Mem64 resources
    //
    ProgramResource (
      Mem64Base,
      Mem64Bridge
      );

    //
    // Program PMem64 resources
    //
    ProgramResource (
      PMem64Base,
      PMem64Bridge
      );

    if (AcpiConfig != NULL) {
      FreePool (AcpiConfig);
    }
  }

  //
  // Destroy all the resource tree
  //
  DestroyResourceTree (&IoPool);
  DestroyResourceTree (&Mem32Pool);
  DestroyResourceTree (&PMem32Pool);
  DestroyResourceTree (&Mem64Pool);
  DestroyResourceTree (&PMem64Pool);

  //
  // Notify the resource allocation phase is to end
  //
  NotifyPhase (PciResAlloc, EfiPciHostBridgeEndResourceAllocation);

  return EFI_SUCCESS;
}

/**
  Submits the I/O and memory resource requirements for the specified PCI Root Bridge.

  @param PciResAlloc  Point to protocol instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.

  @retval EFI_SUCCESS           Success.
**/
EFI_STATUS
PciHostBridgeResourceAllocator_WithHotPlugDeviceSupport (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  )
{
  PCI_IO_DEVICE                         *RootBridgeDev;
  EFI_HANDLE                            RootBridgeHandle;
  VOID                                  *AcpiConfig;
  EFI_STATUS                            Status;
  UINT64                                IoBase;
  UINT64                                Mem32Base;
  UINT64                                PMem32Base;
  UINT64                                Mem64Base;
  UINT64                                PMem64Base;
  UINT64                                IoResStatus;
  UINT64                                Mem32ResStatus;
  UINT64                                PMem32ResStatus;
  UINT64                                Mem64ResStatus;
  UINT64                                PMem64ResStatus;
  UINT64                                MaxOptionRomSize;
  PCI_RESOURCE_NODE                     *IoBridge;
  PCI_RESOURCE_NODE                     *Mem32Bridge;
  PCI_RESOURCE_NODE                     *PMem32Bridge;
  PCI_RESOURCE_NODE                     *Mem64Bridge;
  PCI_RESOURCE_NODE                     *PMem64Bridge;
  PCI_RESOURCE_NODE                     IoPool;
  PCI_RESOURCE_NODE                     Mem32Pool;
  PCI_RESOURCE_NODE                     PMem32Pool;
  PCI_RESOURCE_NODE                     Mem64Pool;
  PCI_RESOURCE_NODE                     PMem64Pool;
  BOOLEAN                               ReAllocate;
  EFI_DEVICE_HANDLE_EXTENDED_DATA_PAYLOAD        HandleExtendedData;
  EFI_RESOURCE_ALLOC_FAILURE_ERROR_DATA_PAYLOAD  AllocFailExtendedData;

  //
  // Reallocate flag
  //
  ReAllocate = FALSE;

  //
  // It will try several times if the resource allocation fails
  //
  while (TRUE) {

    //
    // Initialize resource pool
    //
    InitializeResourcePool (&IoPool, PciBarTypeIo16);
    InitializeResourcePool (&Mem32Pool, PciBarTypeMem32);
    InitializeResourcePool (&PMem32Pool, PciBarTypePMem32);
    InitializeResourcePool (&Mem64Pool, PciBarTypeMem64);
    InitializeResourcePool (&PMem64Pool, PciBarTypePMem64);

    RootBridgeDev     = NULL;
    RootBridgeHandle  = 0;

    while (PciResAlloc->GetNextRootBridge (PciResAlloc, &RootBridgeHandle) == EFI_SUCCESS) {

      //
      // Get RootBridg Device by handle
      //
      RootBridgeDev = GetRootBridgeByHandle (RootBridgeHandle);

      if (RootBridgeDev == NULL) {
        return EFI_NOT_FOUND;
      }

      //
      // Create the entire system resource map from the information collected by
      // enumerator. Several resource tree was created
      //

      IoBridge = CreateResourceNode (
                  RootBridgeDev,
                  0,
                  0xFFF,
                  0,
                  PciBarTypeIo16,
                  PciResUsageTypical
                  );

      Mem32Bridge = CreateResourceNode (
                      RootBridgeDev,
                      0,
                      0xFFFFF,
                      0,
                      PciBarTypeMem32,
                      PciResUsageTypical
                      );

      PMem32Bridge = CreateResourceNode (
                      RootBridgeDev,
                      0,
                      0xFFFFF,
                      0,
                      PciBarTypePMem32,
                      PciResUsageTypical
                      );

      Mem64Bridge = CreateResourceNode (
                      RootBridgeDev,
                      0,
                      0xFFFFF,
                      0,
                      PciBarTypeMem64,
                      PciResUsageTypical
                      );

      PMem64Bridge = CreateResourceNode (
                      RootBridgeDev,
                      0,
                      0xFFFFF,
                      0,
                      PciBarTypePMem64,
                      PciResUsageTypical
                      );

      //
      // Create resourcemap by going through all the devices subject to this root bridge
      //
      Status = CreateResourceMap (
                RootBridgeDev,
                IoBridge,
                Mem32Bridge,
                PMem32Bridge,
                Mem64Bridge,
                PMem64Bridge
                );

      //
      // Get the max ROM size that the root bridge can process
      //
      RootBridgeDev->RomSize = Mem32Bridge->Length;

      //
      // Skip to enlarge the resource request during realloction
      //
      if (!ReAllocate) {
        //
        // Get Max Option Rom size for current root bridge
        //
        MaxOptionRomSize = GetMaxOptionRomSize (RootBridgeDev);

        //
        // Enlarger the mem32 resource to accomdate the option rom
        // if the mem32 resource is not enough to hold the rom
        //
        if (MaxOptionRomSize > Mem32Bridge->Length) {

          Mem32Bridge->Length     = MaxOptionRomSize;
          RootBridgeDev->RomSize  = MaxOptionRomSize;

          //
          // Alignment should be adjusted as well
          //
          if (Mem32Bridge->Alignment < MaxOptionRomSize - 1) {
            Mem32Bridge->Alignment = MaxOptionRomSize - 1;
          }
        }
      }

      //
      // Based on the all the resource tree, contruct ACPI resource node to
      // submit the resource aperture to pci host bridge protocol
      //
      Status = ConstructAcpiResourceRequestor (
                RootBridgeDev,
                IoBridge,
                Mem32Bridge,
                PMem32Bridge,
                Mem64Bridge,
                PMem64Bridge,
                &AcpiConfig
                );

      //
      // Insert these resource nodes into the database
      //
      InsertResourceNode (&IoPool, IoBridge);
      InsertResourceNode (&Mem32Pool, Mem32Bridge);
      InsertResourceNode (&PMem32Pool, PMem32Bridge);
      InsertResourceNode (&Mem64Pool, Mem64Bridge);
      InsertResourceNode (&PMem64Pool, PMem64Bridge);

      if (Status == EFI_SUCCESS) {
        //
        // Submit the resource requirement
        //
        Status = PciResAlloc->SubmitResources (
                                PciResAlloc,
                                RootBridgeDev->Handle,
                                AcpiConfig
                                );
      }

      //
      // Free acpi resource node
      //
      if (AcpiConfig != NULL) {
        FreePool (AcpiConfig);
      }

      if (EFI_ERROR (Status)) {
        //
        // Destroy all the resource tree
        //
        DestroyResourceTree (&IoPool);
        DestroyResourceTree (&Mem32Pool);
        DestroyResourceTree (&PMem32Pool);
        DestroyResourceTree (&Mem64Pool);
        DestroyResourceTree (&PMem64Pool);
        return Status;
      }
    }

    //
    // Notify pci bus driver starts to program the resource
    //

    Status = NotifyPhase (PciResAlloc, EfiPciHostBridgeAllocateResources);

    if (!EFI_ERROR (Status)) {
      //
      // Allocation succeed, then continue the following
      //
      break;
    }

    //
    // If the resource allocation is unsuccessful, free resources on bridge
    //

    RootBridgeDev     = NULL;
    RootBridgeHandle  = 0;

    IoResStatus       = EFI_RESOURCE_SATISFIED;
    Mem32ResStatus    = EFI_RESOURCE_SATISFIED;
    PMem32ResStatus   = EFI_RESOURCE_SATISFIED;
    Mem64ResStatus    = EFI_RESOURCE_SATISFIED;
    PMem64ResStatus   = EFI_RESOURCE_SATISFIED;

    while (PciResAlloc->GetNextRootBridge (PciResAlloc, &RootBridgeHandle) == EFI_SUCCESS) {
      //
      // Get RootBridg Device by handle
      //
      RootBridgeDev = GetRootBridgeByHandle (RootBridgeHandle);
      if (RootBridgeDev == NULL) {
        return EFI_NOT_FOUND;
      }

      //
      // Get host bridge handle for status report
      //
      HandleExtendedData.Handle = RootBridgeDev->PciRootBridgeIo->ParentHandle;

      //
      // Get acpi resource node for all the resource types
      //
      AcpiConfig = NULL;

      Status = PciResAlloc->GetProposedResources (
                              PciResAlloc,
                              RootBridgeDev->Handle,
                              &AcpiConfig
                              );

      if (EFI_ERROR (Status)) {
        return Status;
      }

      if (AcpiConfig != NULL) {
        //
        // Adjust resource allocation policy for each RB
        //
        GetResourceAllocationStatus (
          AcpiConfig,
          &IoResStatus,
          &Mem32ResStatus,
          &PMem32ResStatus,
          &Mem64ResStatus,
          &PMem64ResStatus
          );
        FreePool (AcpiConfig);
      }
    }
    //
    // End while
    //

    //
    // Raise the EFI_IOB_EC_RESOURCE_CONFLICT status code
    //
    //
    // It is very difficult to follow the spec here
    // Device path , Bar index can not be get here
    //
    ZeroMem (&AllocFailExtendedData, sizeof (AllocFailExtendedData));

    REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
          EFI_PROGRESS_CODE,
          EFI_IO_BUS_PCI | EFI_IOB_EC_RESOURCE_CONFLICT,
          (VOID *) &AllocFailExtendedData,
          sizeof (AllocFailExtendedData)
          );

    Status = PciHostBridgeAdjustAllocation (
              &IoPool,
              &Mem32Pool,
              &PMem32Pool,
              &Mem64Pool,
              &PMem64Pool,
              IoResStatus,
              Mem32ResStatus,
              PMem32ResStatus,
              Mem64ResStatus,
              PMem64ResStatus
              );

    //
    // Destroy all the resource tree
    //
    DestroyResourceTree (&IoPool);
    DestroyResourceTree (&Mem32Pool);
    DestroyResourceTree (&PMem32Pool);
    DestroyResourceTree (&Mem64Pool);
    DestroyResourceTree (&PMem64Pool);

    NotifyPhase (PciResAlloc, EfiPciHostBridgeFreeResources);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    ReAllocate = TRUE;

  }
  //
  // End main while
  //

  //
  // Raise the EFI_IOB_PCI_RES_ALLOC status code
  //
  REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
        EFI_PROGRESS_CODE,
        EFI_IO_BUS_PCI | EFI_IOB_PCI_PC_RES_ALLOC,
        (VOID *) &HandleExtendedData,
        sizeof (HandleExtendedData)
        );

  //
  // Notify pci bus driver starts to program the resource
  //
  NotifyPhase (PciResAlloc, EfiPciHostBridgeSetResources);

  RootBridgeDev     = NULL;

  RootBridgeHandle  = 0;

  while (PciResAlloc->GetNextRootBridge (PciResAlloc, &RootBridgeHandle) == EFI_SUCCESS) {

    //
    // Get RootBridg Device by handle
    //
    RootBridgeDev = GetRootBridgeByHandle (RootBridgeHandle);

    if (RootBridgeDev == NULL) {
      return EFI_NOT_FOUND;
    }

    //
    // Get acpi resource node for all the resource types
    //
    AcpiConfig = NULL;
    Status = PciResAlloc->GetProposedResources (
                            PciResAlloc,
                            RootBridgeDev->Handle,
                            &AcpiConfig
                            );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Get the resource base by interpreting acpi resource node
    //
    //
    GetResourceBase (
      AcpiConfig,
      &IoBase,
      &Mem32Base,
      &PMem32Base,
      &Mem64Base,
      &PMem64Base
      );

    //
    // Process option rom for this root bridge
    //
    Status = ProcessOptionRom (RootBridgeDev, Mem32Base, RootBridgeDev->RomSize);

    //
    // Create the entire system resource map from the information collected by
    // enumerator. Several resource tree was created
    //
    Status = GetResourceMap (
              RootBridgeDev,
              &IoBridge,
              &Mem32Bridge,
              &PMem32Bridge,
              &Mem64Bridge,
              &PMem64Bridge,
              &IoPool,
              &Mem32Pool,
              &PMem32Pool,
              &Mem64Pool,
              &PMem64Pool
              );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Program IO resources
    //
    ProgramResource (
      IoBase,
      IoBridge
      );

    //
    // Program Mem32 resources
    //
    ProgramResource (
      Mem32Base,
      Mem32Bridge
      );

    //
    // Program PMem32 resources
    //
    ProgramResource (
      PMem32Base,
      PMem32Bridge
      );

    //
    // Program Mem64 resources
    //
    ProgramResource (
      Mem64Base,
      Mem64Bridge
      );

    //
    // Program PMem64 resources
    //
    ProgramResource (
      PMem64Base,
      PMem64Bridge
      );

    if (AcpiConfig != NULL) {
      gBS->FreePool (AcpiConfig);
    }
  }

  //
  // Destroy all the resource tree
  //
  DestroyResourceTree (&IoPool);
  DestroyResourceTree (&Mem32Pool);
  DestroyResourceTree (&PMem32Pool);
  DestroyResourceTree (&Mem64Pool);
  DestroyResourceTree (&PMem64Pool);

  //
  // Notify the resource allocation phase is to end
  //
  NotifyPhase (PciResAlloc, EfiPciHostBridgeEndResourceAllocation);

  return EFI_SUCCESS;
}

/**
  Wapper function of scanning pci bus and assign bus number to the given PCI bus system
  Feature flag PcdPciBusHotplugDeviceSupport determine whether need support hotplug.
  
  @param  Bridge          Bridge device instance.
  @param  StartBusNumber  start point.
  @param  SubBusNumber    Point to sub bus number.
  @param  PaddedBusRange  Customized bus number.
  
  @retval EFI_SUCCESS     Success.
  @retval EFI_DEVICE_ERROR Fail to scan bus.
**/
EFI_STATUS
PciScanBus (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber,
  OUT UINT8                             *PaddedBusRange
  )
{
  if (FeaturePcdGet (PcdPciBusHotplugDeviceSupport)) {
    return PciScanBus_WithHotPlugDeviceSupport (
      Bridge,
      StartBusNumber,
      SubBusNumber,
      PaddedBusRange
      );
  } else {
    return PciScanBus_WithoutHotPlugDeviceSupport (
      Bridge,
      StartBusNumber,
      SubBusNumber,
      PaddedBusRange
      );
  }
}

/**
  Wapper function of scanning pci bus and assign bus number to the given PCI bus system
  Feature flag PcdPciBusHotplugDeviceSupport determine whether need support hotplug.  
  
  @param  Bridge          Bridge device instance.
  @param  StartBusNumber  start point.
  @param  SubBusNumber    Point to sub bus number.
  @param  PaddedBusRange  Customized bus number.
  
  @retval EFI_SUCCESS     Success.
  @retval EFI_DEVICE_ERROR Fail to scan bus.
**/
EFI_STATUS
PciScanBus_WithoutHotPlugDeviceSupport (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber,
  OUT UINT8                             *PaddedBusRange
  )
{
  EFI_STATUS                      Status;
  PCI_TYPE00                      Pci;
  UINT8                           Device;
  UINT8                           Func;
  UINT64                          Address;
  UINTN                           SecondBus;
  UINT16                          Register;
  PCI_IO_DEVICE                   *PciDevice;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;

  PciRootBridgeIo = Bridge->PciRootBridgeIo;
  SecondBus       = 0;
  Register        = 0;

  for (Device = 0; Device <= PCI_MAX_DEVICE; Device++) {
    for (Func = 0; Func <= PCI_MAX_FUNC; Func++) {

      //
      // Check to see whether a pci device is present
      //
      Status = PciDevicePresent (
                PciRootBridgeIo,
                &Pci,
                StartBusNumber,
                Device,
                Func
                );

      if (!EFI_ERROR (Status)) {
        DEBUG((EFI_D_ERROR, "Found DEV(%02d,%02d,%02d)\n", StartBusNumber, Device, Func));
        
        if (IS_PCI_BRIDGE (&Pci) ||
          IS_CARDBUS_BRIDGE (&Pci)) {

          //
          // Get the bridge information
          //
          Status = PciSearchDevice (
                    Bridge,
                    &Pci,
                    StartBusNumber,
                    Device,
                    Func,
                    &PciDevice
                    );
        
          if (EFI_ERROR (Status)) {
            return Status;
          }
        
          //
          // Add feature to support customized secondary bus number
          //
          if (*SubBusNumber == 0) {
            *SubBusNumber   = *PaddedBusRange;
            *PaddedBusRange = 0;
          }
        
          (*SubBusNumber)++;
        
          SecondBus = (*SubBusNumber);
        
          Register  = (UINT16) ((SecondBus << 8) | (UINT16) StartBusNumber);
        
          Address   = EFI_PCI_ADDRESS (StartBusNumber, Device, Func, 0x18);
        
          Status = PciRootBridgeIoWrite (
                                          PciRootBridgeIo,
                                          &Pci,
                                          EfiPciWidthUint16,
                                          Address,
                                          1,
                                          &Register
                                          );
        
          //
          // Initialize SubBusNumber to SecondBus
          //
          Address = EFI_PCI_ADDRESS (StartBusNumber, Device, Func, 0x1A);
          Status = PciRootBridgeIoWrite (
                                          PciRootBridgeIo,
                                          &Pci,
                                          EfiPciWidthUint8,
                                          Address,
                                          1,
                                          SubBusNumber
                                          );
          //
          // If it is PPB, resursively search down this bridge
          //
          if (IS_PCI_BRIDGE (&Pci)) {
            //
            // Temporarily initialize SubBusNumber to maximum bus number to ensure the
            // PCI configuration transaction to go through any PPB
            //
            Address   = EFI_PCI_ADDRESS (StartBusNumber, Device, Func, 0x1A);
            Register  = 0xFF;
            Status = PciRootBridgeIoWrite (
                                            PciRootBridgeIo,
                                            &Pci,
                                            EfiPciWidthUint8,
                                            Address,
                                            1,
                                            &Register
                                            );
        
            PreprocessController (
              PciDevice,
              PciDevice->BusNumber,
              PciDevice->DeviceNumber,
              PciDevice->FunctionNumber,
              EfiPciBeforeChildBusEnumeration
              );
        
            DEBUG((EFI_D_ERROR, "Scan  PPB(%02d,%02d,%02d)\n", PciDevice->BusNumber, PciDevice->DeviceNumber,PciDevice->FunctionNumber ));
            Status = PciScanBus (
                      PciDevice,
                      (UINT8) (SecondBus),
                      SubBusNumber,
                      PaddedBusRange
                      );
        
            if (EFI_ERROR (Status)) {
              return EFI_DEVICE_ERROR;
            }
          }
        
          //
          // Set the current maximum bus number under the PPB
          //
        
          Address = EFI_PCI_ADDRESS (StartBusNumber, Device, Func, 0x1A);
        
          Status = PciRootBridgeIoWrite (
                                          PciRootBridgeIo,
                                          &Pci,
                                          EfiPciWidthUint8,
                                          Address,
                                          1,
                                          SubBusNumber
                                          );
        
        }
      }
      if (Func == 0 && !IS_PCI_MULTI_FUNC (&Pci)) {

        //
        // Skip sub functions, this is not a multi function device
        //

        Func = PCI_MAX_FUNC;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Wapper function of scanning pci bus and assign bus number to the given PCI bus system
  Feature flag PcdPciBusHotplugDeviceSupport determine whether need support hotplug.  
  
  @param  Bridge          Bridge device instance.
  @param  StartBusNumber  start point.
  @param  SubBusNumber    Point to sub bus number.
  @param  PaddedBusRange  Customized bus number.
  
  @retval EFI_SUCCESS     Success.
  @retval EFI_DEVICE_ERROR Fail to scan bus.
**/
EFI_STATUS
PciScanBus_WithHotPlugDeviceSupport (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber,
  OUT UINT8                             *PaddedBusRange
  )
{
  EFI_STATUS                        Status;
  PCI_TYPE00                        Pci;
  UINT8                             Device;
  UINT8                             Func;
  UINT64                            Address;
  UINTN                             SecondBus;
  UINT16                            Register;
  UINTN                             HpIndex;
  PCI_IO_DEVICE                     *PciDevice;
  EFI_EVENT                         Event;
  EFI_HPC_STATE                     State;
  UINT64                            PciAddress;
  EFI_HPC_PADDING_ATTRIBUTES        Attributes;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptors;
  UINT16                            BusRange;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL   *PciRootBridgeIo;
  BOOLEAN                           BusPadding;

  PciRootBridgeIo = Bridge->PciRootBridgeIo;
  SecondBus       = 0;
  Register        = 0;
  State           = 0;
  Attributes      = (EFI_HPC_PADDING_ATTRIBUTES) 0;
  BusRange        = 0;

  for (Device = 0; Device <= PCI_MAX_DEVICE; Device++) {
    for (Func = 0; Func <= PCI_MAX_FUNC; Func++) {

      //
      // Check to see whether a pci device is present
      //
      Status = PciDevicePresent (
                PciRootBridgeIo,
                &Pci,
                StartBusNumber,
                Device,
                Func
                );

      if (EFI_ERROR (Status)) {
        if (Func == 0) {
          //
          // Skip sub functions, this is not a multi function device
          //
          Func = PCI_MAX_FUNC;
        }

        continue;
      }

      DEBUG((EFI_D_ERROR, "Found DEV(%02d,%02d,%02d)\n", StartBusNumber, Device, Func ));

      //
      // Get the PCI device information
      //
      Status = PciSearchDevice (
                Bridge,
                &Pci,
                StartBusNumber,
                Device,
                Func,
                &PciDevice
                );

      ASSERT (!EFI_ERROR (Status));

      PciAddress = EFI_PCI_ADDRESS (StartBusNumber, Device, Func, 0);

      if (!IS_PCI_BRIDGE (&Pci)) {
        //
        // PCI bridges will be called later
        // Here just need for PCI device or PCI to cardbus controller
        // EfiPciBeforeChildBusEnumeration for PCI Device Node
        //
        PreprocessController (
            PciDevice,
            PciDevice->BusNumber,
            PciDevice->DeviceNumber,
            PciDevice->FunctionNumber,
            EfiPciBeforeChildBusEnumeration
            );
      }

      //
      // For Pci Hotplug controller devcie only
      //
      if (gPciHotPlugInit != NULL) {
        //
        // Check if it is a Hotplug PCI controller
        //
        if (IsRootPciHotPlugController (PciDevice->DevicePath, &HpIndex)) {

          if (!gPciRootHpcData[HpIndex].Initialized) {

            Status = CreateEventForHpc (HpIndex, &Event);

            ASSERT (!EFI_ERROR (Status));

            Status = gPciHotPlugInit->InitializeRootHpc (
                                        gPciHotPlugInit,
                                        gPciRootHpcPool[HpIndex].HpcDevicePath,
                                        PciAddress,
                                        Event,
                                        &State
                                        );

            PreprocessController (
              PciDevice,
              PciDevice->BusNumber,
              PciDevice->DeviceNumber,
              PciDevice->FunctionNumber,
              EfiPciBeforeChildBusEnumeration
            );
          }
        }
      }

      if (IS_PCI_BRIDGE (&Pci) || IS_CARDBUS_BRIDGE (&Pci)) {
        //
        // For PPB
        // Get the bridge information
        //
        BusPadding = FALSE;
        if (gPciHotPlugInit != NULL) {

          if (IsRootPciHotPlugBus (PciDevice->DevicePath, &HpIndex)) {

            //
            // If it is initialized, get the padded bus range
            //
            Status = gPciHotPlugInit->GetResourcePadding (
                                        gPciHotPlugInit,
                                        gPciRootHpcPool[HpIndex].HpbDevicePath,
                                        PciAddress,
                                        &State,
                                        (VOID **) &Descriptors,
                                        &Attributes
                                        );

            if (EFI_ERROR (Status)) {
              return Status;
            }

            BusRange = 0;
            Status = PciGetBusRange (
                      &Descriptors,
                      NULL,
                      NULL,
                      &BusRange
                      );

            gBS->FreePool (Descriptors);

            if (EFI_ERROR (Status)) {
              return Status;
            }

            BusPadding = TRUE;
          }
        }

        //
        // Add feature to support customized secondary bus number
        //
        if (*SubBusNumber == 0) {
          *SubBusNumber   = *PaddedBusRange;
          *PaddedBusRange = 0;
        }

        (*SubBusNumber)++;
        SecondBus = *SubBusNumber;

        Register  = (UINT16) ((SecondBus << 8) | (UINT16) StartBusNumber);
        Address   = EFI_PCI_ADDRESS (StartBusNumber, Device, Func, 0x18);

        Status = PciRootBridgeIoWrite (
                                        PciRootBridgeIo,
                                        &Pci,
                                        EfiPciWidthUint16,
                                        Address,
                                        1,
                                        &Register
                                        );


        //
        // If it is PPB, resursively search down this bridge
        //
        if (IS_PCI_BRIDGE (&Pci)) {

          //
          // Initialize SubBusNumber to Maximum bus number
          //
          Register  = 0xFF;
          Address   = EFI_PCI_ADDRESS (StartBusNumber, Device, Func, 0x1A);
          Status = PciRootBridgeIoWrite (
                                          PciRootBridgeIo,
                                          &Pci,
                                          EfiPciWidthUint8,
                                          Address,
                                          1,
                                          &Register
                                          );

          //
          // Nofify EfiPciBeforeChildBusEnumeration for PCI Brige
          //
          PreprocessController (
            PciDevice,
            PciDevice->BusNumber,
            PciDevice->DeviceNumber,
            PciDevice->FunctionNumber,
            EfiPciBeforeChildBusEnumeration
            );

          DEBUG((EFI_D_ERROR, "Scan  PPB(%02d,%02d,%02d)\n", PciDevice->BusNumber, PciDevice->DeviceNumber,PciDevice->FunctionNumber ));
          Status = PciScanBus (
                    PciDevice,
                    (UINT8) (SecondBus),
                    SubBusNumber,
                    PaddedBusRange
                    );

          if (EFI_ERROR (Status)) {
            return EFI_DEVICE_ERROR;
          }
        }

        if (BusPadding) {
          //
          // Ensure the device is enabled and initialized
          //
          if ((Attributes == EfiPaddingPciRootBridge) &&
              (State & EFI_HPC_STATE_ENABLED) != 0    &&
              (State & EFI_HPC_STATE_INITIALIZED) != 0) {
            *PaddedBusRange = (UINT8) ((UINT8) (BusRange) +*PaddedBusRange);
          } else {
            *SubBusNumber = (UINT8) ((UINT8) (BusRange) +*SubBusNumber);
          }
        }

        //
        // Set the current maximum bus number under the PPB
        //
        Address = EFI_PCI_ADDRESS (StartBusNumber, Device, Func, 0x1A);

        Status = PciRootBridgeIoWrite (
                                        PciRootBridgeIo,
                                        &Pci,
                                        EfiPciWidthUint8,
                                        Address,
                                        1,
                                        SubBusNumber
                                        );
      }

      if (Func == 0 && !IS_PCI_MULTI_FUNC (&Pci)) {

        //
        // Skip sub functions, this is not a multi function device
        //
        Func = PCI_MAX_FUNC;
      }

    }
  }

  return EFI_SUCCESS;
}

/**
  Process Option Rom on this host bridge.
  
  @param Bridge  Pci bridge device instance.
  
  @retval EFI_SUCCESS Success.
**/
EFI_STATUS
PciRootBridgeP2CProcess (
  IN PCI_IO_DEVICE *Bridge
  )
{
  LIST_ENTRY      *CurrentLink;
  PCI_IO_DEVICE   *Temp;
  EFI_HPC_STATE   State;
  UINT64          PciAddress;
  EFI_STATUS      Status;

  CurrentLink = Bridge->ChildList.ForwardLink;

  while (CurrentLink != NULL && CurrentLink != &Bridge->ChildList) {

    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);

    if (IS_CARDBUS_BRIDGE (&Temp->Pci)) {

      if (gPciHotPlugInit != NULL && Temp->Allocated) {

        //
        // Raise the EFI_IOB_PCI_HPC_INIT status code
        //
        REPORT_STATUS_CODE_WITH_DEVICE_PATH (
          EFI_PROGRESS_CODE,
          EFI_IO_BUS_PCI | EFI_IOB_PCI_PC_HPC_INIT,
          Temp->DevicePath
          );

        PciAddress = EFI_PCI_ADDRESS (Temp->BusNumber, Temp->DeviceNumber, Temp->FunctionNumber, 0);
        Status = gPciHotPlugInit->InitializeRootHpc (
                                    gPciHotPlugInit,
                                    Temp->DevicePath,
                                    PciAddress,
                                    NULL,
                                    &State
                                    );

        if (!EFI_ERROR (Status)) {
          Status = PciBridgeEnumerator (Temp);

          if (EFI_ERROR (Status)) {
            return Status;
          }
        }

        CurrentLink = CurrentLink->ForwardLink;
        continue;

      }
    }

    if (!IsListEmpty (&Temp->ChildList)) {
      Status = PciRootBridgeP2CProcess (Temp);
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return EFI_SUCCESS;
}

/**
  Process Option Rom on this host bridge.
  
  @param PciResAlloc Pointer to instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.
  
  @retval EFI_NOT_FOUND Can not find the root bridge instance.
  @retval EFI_SUCCESS   Success process.
**/
EFI_STATUS
PciHostBridgeP2CProcess (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  )
{
  EFI_HANDLE    RootBridgeHandle;
  PCI_IO_DEVICE *RootBridgeDev;
  EFI_STATUS    Status;

  if (!FeaturePcdGet (PcdPciBusHotplugDeviceSupport)) {
    return EFI_SUCCESS;
  }

  RootBridgeHandle = NULL;

  while (PciResAlloc->GetNextRootBridge (PciResAlloc, &RootBridgeHandle) == EFI_SUCCESS) {

    //
    // Get RootBridg Device by handle
    //
    RootBridgeDev = GetRootBridgeByHandle (RootBridgeHandle);

    if (RootBridgeDev == NULL) {
      return EFI_NOT_FOUND;
    }

    Status = PciRootBridgeP2CProcess (RootBridgeDev);

    if (EFI_ERROR (Status)) {
      return Status;
    }

  }

  return EFI_SUCCESS;
}

/**
  This function is used to enumerate the entire host bridge
  in a given platform.

  @param PciResAlloc   A pointer to the resource allocate protocol.

  @retval EFI_OUT_OF_RESOURCES no enough resource.
  @retval EFI_SUCCESS Success.

**/
EFI_STATUS
PciHostBridgeEnumerator (
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *PciResAlloc
  )
{
  EFI_HANDLE                        RootBridgeHandle;
  PCI_IO_DEVICE                     *RootBridgeDev;
  EFI_STATUS                        Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL   *PciRootBridgeIo;
  UINT16                            MinBus;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptors;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Configuration;
  UINT8                             StartBusNumber;
  LIST_ENTRY                        RootBridgeList;
  LIST_ENTRY                        *Link;

  InitializeHotPlugSupport ();

  InitializeListHead (&RootBridgeList);

  //
  // Notify the bus allocation phase is about to start
  //
  NotifyPhase (PciResAlloc, EfiPciHostBridgeBeginBusAllocation);

  DEBUG((EFI_D_ERROR, "PCI Bus First Scanning\n"));
  RootBridgeHandle = NULL;
  while (PciResAlloc->GetNextRootBridge (PciResAlloc, &RootBridgeHandle) == EFI_SUCCESS) {

    //
    // if a root bridge instance is found, create root bridge device for it
    //

    RootBridgeDev = CreateRootBridge (RootBridgeHandle);

    if (RootBridgeDev == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Enumerate all the buses under this root bridge
    //

    Status = PciRootBridgeEnumerator (
              PciResAlloc,
              RootBridgeDev
              );

    if (gPciHotPlugInit != NULL) {
      InsertTailList (&RootBridgeList, &(RootBridgeDev->Link));
    } else {
      DestroyRootBridge (RootBridgeDev);
    }
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Notify the bus allocation phase is finished for the first time
  //
  NotifyPhase (PciResAlloc, EfiPciHostBridgeEndBusAllocation);

  if (FeaturePcdGet (PcdPciBusHotplugDeviceSupport)) {

    if (gPciHotPlugInit != NULL) {
      //
      // Reset all assigned PCI bus number in all PPB
      //
      RootBridgeHandle = NULL;
      Link = GetFirstNode (&RootBridgeList);
      while ((PciResAlloc->GetNextRootBridge (PciResAlloc, &RootBridgeHandle) == EFI_SUCCESS) &&
        (!IsNull (&RootBridgeList, Link))) {
        RootBridgeDev = PCI_IO_DEVICE_FROM_LINK (Link);
        //
        // Get the Bus information
        //
        Status = PciResAlloc->StartBusEnumeration (
                                PciResAlloc,
                                RootBridgeHandle,
                                (VOID **) &Configuration
                                );
        if (EFI_ERROR (Status)) {
          return Status;
        }

        //
        // Get the bus number to start with
        //
        StartBusNumber  = (UINT8) (Configuration->AddrRangeMin);

        ResetAllPpbBusNumber (
          RootBridgeDev,
          StartBusNumber
        );

        gBS->FreePool (Configuration);
        Link = GetNextNode (&RootBridgeList, Link);
        DestroyRootBridge (RootBridgeDev);
      }

      //
      // Wait for all HPC initialized
     //
      Status = AllRootHPCInitialized (STALL_1_SECOND * 15);

      if (EFI_ERROR (Status)) {
        return Status;
      }

      //
      // Notify the bus allocation phase is about to start for the 2nd time
      //
      NotifyPhase (PciResAlloc, EfiPciHostBridgeBeginBusAllocation);

      DEBUG((EFI_D_ERROR, "PCI Bus Second Scanning\n"));
      RootBridgeHandle = NULL;
      while (PciResAlloc->GetNextRootBridge (PciResAlloc, &RootBridgeHandle) == EFI_SUCCESS) {

        //
        // if a root bridge instance is found, create root bridge device for it
        //

        RootBridgeDev = CreateRootBridge (RootBridgeHandle);

        if (RootBridgeDev == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        //
        // Enumerate all the buses under this root bridge
        //

        Status = PciRootBridgeEnumerator (
                  PciResAlloc,
                  RootBridgeDev
                  );

        DestroyRootBridge (RootBridgeDev);
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }

      //
      // Notify the bus allocation phase is to end for the 2nd time
      //
      NotifyPhase (PciResAlloc, EfiPciHostBridgeEndBusAllocation);
    }
  }

  //
  // Notify the resource allocation phase is to start
  //
  NotifyPhase (PciResAlloc, EfiPciHostBridgeBeginResourceAllocation);

  RootBridgeHandle = NULL;
  while (PciResAlloc->GetNextRootBridge (PciResAlloc, &RootBridgeHandle) == EFI_SUCCESS) {

    //
    // if a root bridge instance is found, create root bridge device for it
    //

    RootBridgeDev = CreateRootBridge (RootBridgeHandle);

    if (RootBridgeDev == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = StartManagingRootBridge (RootBridgeDev);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    PciRootBridgeIo = RootBridgeDev->PciRootBridgeIo;
    Status          = PciRootBridgeIo->Configuration (PciRootBridgeIo, (VOID **) &Descriptors);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = PciGetBusRange (&Descriptors, &MinBus, NULL, NULL);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Determine root bridge attribute by calling interface of Pcihostbridge
    // protocol
    //
    DetermineRootBridgeAttributes (
      PciResAlloc,
      RootBridgeDev
      );

    //
    // Collect all the resource information under this root bridge
    // A database that records all the information about pci device subject to this
    // root bridge will then be created
    //
    Status = PciPciDeviceInfoCollector (
              RootBridgeDev,
              (UINT8) MinBus
              );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    InsertRootBridge (RootBridgeDev);

    //
    // Record the hostbridge handle
    //
    AddHostBridgeEnumerator (RootBridgeDev->PciRootBridgeIo->ParentHandle);
  }

  return EFI_SUCCESS;
}

/**
  Read PCI device configuration register by specified address.

  This function check the incompatiblilites on PCI device. Return the register
  value.

  @param  PciRootBridgeIo     A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  PciIo               A pointer to EFI_PCI_PROTOCOL.
  @param  PciDeviceInfo       A pointer to EFI_PCI_DEVICE_INFO.
  @param  Width               Signifies the width of the memory operations.
  @param  Address             The address within the PCI configuration space for the PCI controller.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

   @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
ReadConfigData (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *PciRootBridgeIo,  OPTIONAL
  IN       EFI_PCI_IO_PROTOCOL                    *PciIo,            OPTIONAL
  IN       EFI_PCI_DEVICE_INFO                    *PciDeviceInfo,
  IN       UINT64                                 Width,
  IN       UINT64                                 Address,
  IN OUT   VOID                                   *Buffer
  )
{
  EFI_STATUS                    Status;
  UINT64                        AccessWidth;
  EFI_PCI_REGISTER_ACCESS_DATA  *PciRegisterAccessData;
  UINT64                        AccessAddress;
  UINTN                         Stride;
  UINT64                        TempBuffer;
  UINT8                         *Pointer;

  ASSERT ((PciRootBridgeIo == NULL) ^ (PciIo == NULL));

  if (PcdGet8 (PcdPciIncompatibleDeviceSupportMask) & PCI_INCOMPATIBLE_ACCESS_WIDTH_SUPPORT) {
    //
    // check access compatibility at first time
    //
    Status = PciRegisterAccessCheck (PciDeviceInfo, PCI_REGISTER_READ, Address & 0xff, Width, &PciRegisterAccessData);

    if (Status == EFI_SUCCESS) {
      //
      // there exist incompatibility on this operation
      //
      AccessWidth = Width;

      if (PciRegisterAccessData->Width != VALUE_NOCARE) {
        AccessWidth = PciRegisterAccessData->Width;
      }

      AccessAddress = Address & ~((1 << AccessWidth) - 1);

      TempBuffer    = 0;
      Stride        = 0;
      Pointer       = (UINT8 *) &TempBuffer;

      while (1) {

        if (PciRootBridgeIo != NULL) {
          Status = PciRootBridgeIo->Pci.Read (
                         PciRootBridgeIo,
                         (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) AccessWidth,
                         AccessAddress,
                         1,
                         Pointer
                         );
        } else if (PciIo != NULL) {
          Status = PciIo->Pci.Read (
                         PciIo,
                         (EFI_PCI_IO_PROTOCOL_WIDTH) AccessWidth,
                         (UINT32) AccessAddress,
                         1,
                         Pointer
                         );
        }

        if (Status != EFI_SUCCESS) {
          return Status;
        }

       Stride = (UINTN)1 << AccessWidth;
        AccessAddress += Stride;
        if (AccessAddress >= (Address + LShiftU64 (1ULL, (UINTN)Width))) {
          //
          // if all datas have been read, exist
          //
          break;
        }

        Pointer += Stride;

        if ((AccessAddress & 0xff) < PciRegisterAccessData->EndOffset) {
          //
          // if current offset doesn't reach the end
          //
          continue;
        }

        FreePool (PciRegisterAccessData);

        //
        // continue checking access incompatibility
        //
        Status = PciRegisterAccessCheck (PciDeviceInfo, PCI_REGISTER_READ, AccessAddress & 0xff, AccessWidth, &PciRegisterAccessData);
        if (Status == EFI_SUCCESS) {
          if (PciRegisterAccessData->Width != VALUE_NOCARE) {
            AccessWidth = PciRegisterAccessData->Width;
          }
        }
      }

      FreePool (PciRegisterAccessData);

      switch (Width) {
      case EfiPciWidthUint8:
        * (UINT8 *) Buffer = (UINT8) TempBuffer;
        break;
      case EfiPciWidthUint16:
        * (UINT16 *) Buffer = (UINT16) TempBuffer;
        break;
      case EfiPciWidthUint32:
        * (UINT32 *) Buffer = (UINT32) TempBuffer;
        break;
      default:
        return EFI_UNSUPPORTED;
      }

      return Status;
    }
  }
  //
  // AccessWidth incompatible check not supportted
  // or, there doesn't exist incompatibility on this operation
  //
  if (PciRootBridgeIo != NULL) {
    Status = PciRootBridgeIo->Pci.Read (
                     PciRootBridgeIo,
                     (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                     Address,
                     1,
                     Buffer
                     );

  } else {
    Status = PciIo->Pci.Read (
                     PciIo,
                     (EFI_PCI_IO_PROTOCOL_WIDTH) Width,
                     (UINT32) Address,
                     1,
                     Buffer
                     );
  }

  return Status;
}

/**
  Update register value by checking PCI device incompatibility.

  This function check register value incompatibilites on PCI device. Return the register
  value.

  @param  PciDeviceInfo       A pointer to EFI_PCI_DEVICE_INFO.
  @param  AccessType          Access type, READ or WRITE.
  @param  Width               Signifies the width of the memory operations.
  @param  Address             The address within the PCI configuration space.
  @param  Buffer              Store the register data.

  @retval EFI_SUCCESS         The data has been updated.

**/
EFI_STATUS
UpdateConfigData (
  IN       EFI_PCI_DEVICE_INFO                    *PciDeviceInfo,
  IN       UINT64                                 AccessType,
  IN       UINT64                                 Width,
  IN       UINT64                                 Address,
  IN OUT   VOID                                   *Buffer
)
{
  EFI_STATUS                    Status;
  EFI_PCI_REGISTER_VALUE_DATA   *PciRegisterData;
  UINT32                        AndValue;
  UINT32                        OrValue;
  UINT32                        TempValue;

  //
  // check register value incompatibility
  //
  Status = PciRegisterUpdateCheck (PciDeviceInfo, AccessType, Address & 0xff, &PciRegisterData);

  if (Status == EFI_SUCCESS) {

    AndValue = ((UINT32) PciRegisterData->AndValue) >> (((UINT8) Address & 0x3) * 8);
    OrValue  = ((UINT32) PciRegisterData->OrValue)  >> (((UINT8) Address & 0x3) * 8);

    TempValue = * (UINT32 *) Buffer;
    if (PciRegisterData->AndValue != VALUE_NOCARE) {
      TempValue &= AndValue;
    }
    if (PciRegisterData->OrValue != VALUE_NOCARE) {
      TempValue |= OrValue;
    }

    switch (Width) {
    case EfiPciWidthUint8:
      *(UINT8 *)Buffer = (UINT8) TempValue;
      break;

    case EfiPciWidthUint16:
      *(UINT16 *)Buffer = (UINT16) TempValue;
      break;
    case EfiPciWidthUint32:
      *(UINT32 *)Buffer = TempValue;
      break;

    default:
      return EFI_UNSUPPORTED;
    }

    FreePool (PciRegisterData);
  }

  return Status;
}

/**
  Write PCI device configuration register by specified address.

  This function check the incompatiblilites on PCI device, and write date
  into register.

  @param  PciRootBridgeIo     A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  PciIo               A pointer to EFI_PCI_PROTOCOL.
  @param  PciDeviceInfo       A pointer to EFI_PCI_DEVICE_INFO.
  @param  Width               Signifies the width of the memory operations.
  @param  Address             The address within the PCI configuration space for the PCI controller.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

   @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
WriteConfigData (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *PciRootBridgeIo,  OPTIONAL
  IN       EFI_PCI_IO_PROTOCOL                    *PciIo,            OPTIONAL
  IN       EFI_PCI_DEVICE_INFO                    *PciDeviceInfo,
  IN       UINT64                                 Width,
  IN       UINT64                                 Address,
  IN       VOID                                   *Buffer
  )
{
  EFI_STATUS                    Status;
  UINT64                        AccessWidth;
  EFI_PCI_REGISTER_ACCESS_DATA  *PciRegisterAccessData;
  UINT64                        AccessAddress;
  UINTN                         Stride;
  UINT8                         *Pointer;
  UINT64                        Data;
  UINTN                         Shift;

  ASSERT ((PciRootBridgeIo == NULL) ^ (PciIo == NULL));

  if (PcdGet8 (PcdPciIncompatibleDeviceSupportMask) & PCI_INCOMPATIBLE_ACCESS_WIDTH_SUPPORT) {
    //
    // check access compatibility at first time
    //
    Status = PciRegisterAccessCheck (PciDeviceInfo, PCI_REGISTER_WRITE, Address & 0xff, Width, &PciRegisterAccessData);

    if (Status == EFI_SUCCESS) {
      //
      // there exist incompatibility on this operation
      //
      AccessWidth = Width;

      if (PciRegisterAccessData->Width != VALUE_NOCARE) {
        AccessWidth = PciRegisterAccessData->Width;
      }

      AccessAddress = Address & ~((1 << AccessWidth) - 1);

      Stride        = 0;
      Pointer       = (UINT8 *) &Buffer;
      Data          = * (UINT64 *) Buffer;

      while (1) {

        if (AccessWidth > Width) {
          //
          // if actual access width is larger than orignal one, additional data need to be read back firstly
          //
          Status = ReadConfigData (PciRootBridgeIo, PciIo, PciDeviceInfo, AccessWidth, AccessAddress, &Data);
          if (Status != EFI_SUCCESS) {
            return Status;
          }

          //
          // check data read incompatibility
          //
          UpdateConfigData (PciDeviceInfo, PCI_REGISTER_READ, AccessWidth, AccessAddress & 0xff, &Data);

          Shift = (UINTN)(Address - AccessAddress) * 8;
          switch (Width) {
          case EfiPciWidthUint8:
            Data = (* (UINT8 *) Buffer) << Shift | (Data & ~(0xff << Shift));
            break;

          case EfiPciWidthUint16:
            Data = (* (UINT16 *) Buffer) << Shift | (Data & ~(0xffff << Shift));
            break;
          }

          //
          // check data write incompatibility
          //
          UpdateConfigData (PciDeviceInfo, PCI_REGISTER_WRITE, AccessWidth, MultU64x32 (AccessAddress, 0xff), &Data);
        }

        if (PciRootBridgeIo != NULL) {
          Status = PciRootBridgeIo->Pci.Write (
                         PciRootBridgeIo,
                         (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) AccessWidth,
                         AccessAddress,
                         1,
                         &Data
                         );
        } else {
          Status = PciIo->Pci.Write (
                         PciIo,
                         (EFI_PCI_IO_PROTOCOL_WIDTH) AccessWidth,
                         (UINT32) AccessAddress,
                         1,
                         &Data
                         );
        }

        if (Status != EFI_SUCCESS) {
          return Status;
        }

        Data = RShiftU64 (Data, ((1 << AccessWidth) * 8));

        Stride = (UINTN)1 << AccessWidth;
        AccessAddress += Stride;
        if (AccessAddress >= (Address + LShiftU64 (1ULL, (UINTN)Width))) {
          //
          // if all datas have been written, exist
          //
          break;
        }

        Pointer += Stride;

        if ((AccessAddress & 0xff) < PciRegisterAccessData->EndOffset) {
          //
          // if current offset doesn't reach the end
          //
          continue;
        }

        FreePool (PciRegisterAccessData);

        //
        // continue checking access incompatibility
        //
        Status = PciRegisterAccessCheck (PciDeviceInfo, PCI_REGISTER_WRITE, AccessAddress & 0xff, AccessWidth, &PciRegisterAccessData);
        if (Status == EFI_SUCCESS) {
          if (PciRegisterAccessData->Width != VALUE_NOCARE) {
            AccessWidth = PciRegisterAccessData->Width;
          }
        }
      };

      FreePool (PciRegisterAccessData);

      return Status;
    }

  }
  //
  // AccessWidth incompatible check not supportted
  // or, there doesn't exist incompatibility on this operation
  //
  if (PciRootBridgeIo != NULL) {
    Status = PciRootBridgeIo->Pci.Write (
                     PciRootBridgeIo,
                     (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                     Address,
                     1,
                     Buffer
                     );
  } else {
    Status = PciIo->Pci.Write (
                   PciIo,
                   (EFI_PCI_IO_PROTOCOL_WIDTH) Width,
                   (UINT32) Address,
                   1,
                   Buffer
                   );
  }

  return Status;
}

/**
  Abstract PCI device device information.

  @param  PciRootBridgeIo     A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  PciIo               A pointer to EFI_PCI_PROTOCOL.
  @param  Pci                 A pointer to PCI_TYPE00.
  @param  Address             The address within the PCI configuration space for the PCI controller.
  @param  PciDeviceInfo       A pointer to EFI_PCI_DEVICE_INFO.

  @retval EFI_SUCCESS         Pci device device information has been abstracted.

**/
EFI_STATUS
GetPciDeviceDeviceInfo (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *PciRootBridgeIo,  OPTIONAL
  IN       EFI_PCI_IO_PROTOCOL                    *PciIo,            OPTIONAL
  IN       PCI_TYPE00                             *Pci,              OPTIONAL
  IN       UINT64                                 Address,           OPTIONAL
  OUT      EFI_PCI_DEVICE_INFO                    *PciDeviceInfo
)
{
  EFI_STATUS                    Status;
  UINT64                        PciAddress;
  UINT32                        PciConfigData;
  PCI_IO_DEVICE                 *PciIoDevice;

  ASSERT ((PciRootBridgeIo == NULL) ^ (PciIo == NULL));

  if (PciIo != NULL) {
    PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (PciIo);

    //
    // get pointer to PCI_TYPE00 from PciIoDevice
    //
    Pci = &PciIoDevice->Pci;
  }

  if (Pci == NULL) {
    //
    // while PCI_TYPE00 hasn't been gotten, read PCI device device information directly
    //
    PciAddress = Address & 0xffffffffffffff00ULL;
    Status = PciRootBridgeIo->Pci.Read (
                                    PciRootBridgeIo,
                                    EfiPciWidthUint32,
                                    PciAddress,
                                    1,
                                    &PciConfigData
                                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((PciConfigData & 0xffff) == 0xffff) {
      return EFI_NOT_FOUND;
    }

    PciDeviceInfo->VendorID = PciConfigData & 0xffff;
    PciDeviceInfo->DeviceID = PciConfigData >> 16;

    Status = PciRootBridgeIo->Pci.Read (
                                    PciRootBridgeIo,
                                    EfiPciWidthUint32,
                                    PciAddress + 8,
                                    1,
                                    &PciConfigData
                                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    PciDeviceInfo->RevisionID = PciConfigData & 0xf;

    Status = PciRootBridgeIo->Pci.Read (
                                    PciRootBridgeIo,
                                    EfiPciWidthUint32,
                                    PciAddress + 0x2c,
                                    1,
                                    &PciConfigData
                                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    PciDeviceInfo->SubsystemVendorID = PciConfigData & 0xffff;
    PciDeviceInfo->SubsystemID = PciConfigData >> 16;

  } else {
    PciDeviceInfo->VendorID          = Pci->Hdr.VendorId;
    PciDeviceInfo->DeviceID          = Pci->Hdr.DeviceId;
    PciDeviceInfo->RevisionID        = Pci->Hdr.RevisionID;
    PciDeviceInfo->SubsystemVendorID = Pci->Device.SubsystemVendorID;
    PciDeviceInfo->SubsystemID       = Pci->Device.SubsystemID;
  }

  return EFI_SUCCESS;
}

/**
  Read PCI configuration space with incompatibility check.

  @param  PciRootBridgeIo     A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  PciIo               A pointer to the EFI_PCI_IO_PROTOCOL.
  @param  Pci                 A pointer to PCI_TYPE00.
  @param  Width               Signifies the width of the memory operations.
  @param  Address             The address within the PCI configuration space for the PCI controller.
  @param  Count               The number of unit to be read.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

  @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
PciIncompatibilityCheckRead (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *PciRootBridgeIo,   OPTIONAL
  IN       EFI_PCI_IO_PROTOCOL                    *PciIo,             OPTIONAL
  IN       PCI_TYPE00                             *Pci,               OPTIONAL
  IN       UINTN                                  Width,
  IN       UINT64                                 Address,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
)
{
  EFI_STATUS                    Status;
  EFI_PCI_DEVICE_INFO           PciDeviceInfo;
  UINT32                        Stride;

  ASSERT ((PciRootBridgeIo == NULL) ^ (PciIo == NULL));

  //
  // get PCI device device information
  //
  Status = GetPciDeviceDeviceInfo (PciRootBridgeIo, PciIo, Pci, Address, &PciDeviceInfo);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  Stride = 1 << Width;

  for (; Count > 0; Count--, Address += Stride, Buffer = (UINT8 *)Buffer + Stride) {

    //
    // read configuration register
    //
    Status = ReadConfigData (PciRootBridgeIo, PciIo, &PciDeviceInfo, (UINT64) Width, Address, Buffer);

    if (Status != EFI_SUCCESS) {
      return Status;
    }

    //
    // update the data read from configuration register
    //
    if (PcdGet8 (PcdPciIncompatibleDeviceSupportMask) & PCI_INCOMPATIBLE_REGISTER_UPDATE_SUPPORT) {
      UpdateConfigData (&PciDeviceInfo, PCI_REGISTER_READ, Width, Address & 0xff, Buffer);
    }
  }

  return EFI_SUCCESS;
}

/**
  Write PCI configuration space with incompatibility check.

  @param  PciRootBridgeIo     A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  PciIo               A pointer to the EFI_PCI_IO_PROTOCOL.
  @param  Pci                 A pointer to PCI_TYPE00.
  @param  Width               Signifies the width of the memory operations.
  @param  Address             The address within the PCI configuration space for the PCI controller.
  @param  Count               The number of unit to be write.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

  @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
PciIncompatibilityCheckWrite (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *PciRootBridgeIo,   OPTIONAL
  IN       EFI_PCI_IO_PROTOCOL                    *PciIo,             OPTIONAL
  IN       PCI_TYPE00                             *Pci,               OPTIONAL
  IN       UINTN                                  Width,
  IN       UINT64                                 Address,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
)
{
  EFI_STATUS                    Status;
  EFI_PCI_DEVICE_INFO           PciDeviceInfo;
  UINT32                        Stride;
  UINT64                        Data;

  ASSERT ((PciRootBridgeIo == NULL) ^ (PciIo == NULL));

  //
  // get PCI device device information
  //
  Status = GetPciDeviceDeviceInfo (PciRootBridgeIo, PciIo, Pci, Address, &PciDeviceInfo);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  Stride = 1 << Width;

  for (; Count > 0; Count--, Address += Stride, Buffer = (UINT8 *) Buffer + Stride) {

    Data = 0;

    switch (Width) {
    case EfiPciWidthUint8:
      Data = * (UINT8 *) Buffer;
      break;
    case EfiPciWidthUint16:
      Data = * (UINT16 *) Buffer;
      break;

    case EfiPciWidthUint32:
      Data = * (UINT32 *) Buffer;
      break;

    default:
      return EFI_UNSUPPORTED;
    }

    //
    // update the data writen into configuration register
    //
    if (PcdGet8 (PcdPciIncompatibleDeviceSupportMask) & PCI_INCOMPATIBLE_REGISTER_UPDATE_SUPPORT) {
      UpdateConfigData (&PciDeviceInfo, PCI_REGISTER_WRITE, Width, Address & 0xff, &Data);
    }

    //
    // write configuration register
    //
    Status = WriteConfigData (PciRootBridgeIo, PciIo, &PciDeviceInfo, Width, Address, &Data);

    if (Status != EFI_SUCCESS) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Read PCI configuration space through EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.

  @param  PciRootBridgeIo     A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Pci                 A pointer to PCI_TYPE00.
  @param  Width               Signifies the width of the memory operations.
  @param  Address             The address within the PCI configuration space for the PCI controller.
  @param  Count               The number of unit to be read.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

  @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
PciRootBridgeIoRead (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *PciRootBridgeIo,
  IN       PCI_TYPE00                             *Pci,            OPTIONAL
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN       UINT64                                 Address,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
  )
{
  if (PcdGet8 (PcdPciIncompatibleDeviceSupportMask) & PCI_INCOMPATIBLE_READ_SUPPORT) {
    //
    // if PCI incompatibility check enabled
    //
    return PciIncompatibilityCheckRead (
                   PciRootBridgeIo,
                   NULL,
                   Pci,
                   (UINTN) Width,
                   Address,
                   Count,
                   Buffer
                   );
  } else {
    return PciRootBridgeIo->Pci.Read (
                   PciRootBridgeIo,
                   Width,
                   Address,
                   Count,
                   Buffer
                   );
  }
}

/**
  Write PCI configuration space through EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.

  @param  PciRootBridgeIo     A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Pci                 A pointer to PCI_TYPE00.
  @param  Width               Signifies the width of the memory operations.
  @param  Address             The address within the PCI configuration space for the PCI controller.
  @param  Count               The number of unit to be read.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

  @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
PciRootBridgeIoWrite (
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *PciRootBridgeIo,
  IN       PCI_TYPE00                             *Pci,
  IN       EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN       UINT64                                 Address,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
  )
{
  if (PcdGet8 (PcdPciIncompatibleDeviceSupportMask) & PCI_INCOMPATIBLE_WRITE_SUPPORT) {
    //
    // if PCI incompatibility check enabled
    //
    return  PciIncompatibilityCheckWrite (
                   PciRootBridgeIo,
                   NULL,
                   Pci,
                   Width,
                   Address,
                   Count,
                   Buffer
                   );

  } else {
    return  PciRootBridgeIo->Pci.Write (
                   PciRootBridgeIo,
                   Width,
                   Address,
                   Count,
                   Buffer
                   );
  }
}

/**
  Read PCI configuration space through EFI_PCI_IO_PROTOCOL.

  @param  PciIo               A pointer to the EFI_PCI_O_PROTOCOL.
  @param  Width               Signifies the width of the memory operations.
  @param  Address             The address within the PCI configuration space for the PCI controller.
  @param  Count               The number of unit to be read.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

  @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
PciIoRead (
  IN       EFI_PCI_IO_PROTOCOL                    *PciIo,
  IN       EFI_PCI_IO_PROTOCOL_WIDTH              Width,
  IN       UINT32                                 Address,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
  )
{
  if (PcdGet8 (PcdPciIncompatibleDeviceSupportMask) & PCI_INCOMPATIBLE_READ_SUPPORT) {
    //
    // if PCI incompatibility check enabled
    //
    return PciIncompatibilityCheckRead (
                   NULL,
                   PciIo,
                   NULL,
                   (UINTN) Width,
                   Address,
                   Count,
                   Buffer
                   );
  } else {
    return PciIo->Pci.Read (
                   PciIo,
                   Width,
                   Address,
                   Count,
                   Buffer
                   );
  }
}

/**
  Write PCI configuration space through EFI_PCI_IO_PROTOCOL.

  @param  PciIo               A pointer to the EFI_PCI_O_PROTOCOL.
  @param  Width               Signifies the width of the memory operations.
  @param  Address             The address within the PCI configuration space for the PCI controller.
  @param  Count               The number of unit to be read.
  @param  Buffer              For read operations, the destination buffer to store the results. For
                              write operations, the source buffer to write data from.

  @retval EFI_SUCCESS            The data was read from or written to the PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
PciIoWrite (
  IN       EFI_PCI_IO_PROTOCOL                    *PciIo,
  IN       EFI_PCI_IO_PROTOCOL_WIDTH              Width,
  IN       UINT32                                 Address,
  IN       UINTN                                  Count,
  IN OUT   VOID                                   *Buffer
  )
{
  if (PcdGet8 (PcdPciIncompatibleDeviceSupportMask) & PCI_INCOMPATIBLE_WRITE_SUPPORT) {

    //
    // if PCI incompatibility check enabled
    //
    return  PciIncompatibilityCheckWrite (
                   NULL,
                   PciIo,
                   NULL,
                   Width,
                   Address,
                   Count,
                   Buffer
                   );

  } else {
    return PciIo->Pci.Write (
                   PciIo,
                   Width,
                   Address,
                   Count,
                   Buffer
                   );
  }
}


/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  LightPciLib.c
  
Abstract:

  Light PCI Bus Driver Lib file
  It abstracts some functions that can be different 
  between light PCI bus driver and full PCI bus driver

Revision History

--*/

#include "pcibus.h"

//
// Light PCI bus driver woundn't support hotplug device
// So just return
//
VOID
InstallHotPlugRequestProtocol (
  IN EFI_STATUS *Status
  )
/*++

Routine Description:


Arguments:

Returns:

  None

--*/
// TODO:    Status - add argument and description to function comment
{
  return ;
}

//
// Light PCI bus driver woundn't support hotplug device
// So just skip install this GUID
//
VOID
InstallPciHotplugGuid (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  )
/*++

Routine Description:


Arguments:

Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
{
  return ;
}

//
// Light PCI bus driver woundn't support hotplug device
// So just skip uninstall the GUID
//
VOID
UninstallPciHotplugGuid (
  IN  PCI_IO_DEVICE                  *PciIoDevice
  )
/*++

Routine Description:


Arguments:

Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
{
  return ;
}

//
// Light PCI bus driver woundn't support PCCard
// So it needn't get the bar of CardBus
//
VOID
GetBackPcCardBar (
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
{
  return ;
}

//
// Light PCI bus driver woundn't support resource reallocation
// So just return
//
EFI_STATUS
RemoveRejectedPciDevices (
  EFI_HANDLE        RootBridgeHandle,
  IN PCI_IO_DEVICE  *Bridge
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  RootBridgeHandle  - TODO: add argument description
  Bridge            - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  return EFI_SUCCESS;
}

//
// Light PCI bus driver woundn't support resource reallocation
// Simplified the code
//
EFI_STATUS
PciHostBridgeResourceAllocator (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    PciResAlloc - add argument and description to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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
  REPORT_STATUS_CODE_LIBRARY_DEVICE_HANDLE_EXTENDED_DATA  ExtendedData;

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
    if (AcpiConfig) {
      gBS->FreePool (AcpiConfig);
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

EFI_STATUS
PciScanBus (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber,
  OUT UINT8                             *PaddedBusRange
  )
/*++

Routine Description:

  This routine is used to assign bus number to the given PCI bus system

Arguments:

Returns:

  None

--*/
// TODO:    Bridge - add argument and description to function comment
// TODO:    StartBusNumber - add argument and description to function comment
// TODO:    SubBusNumber - add argument and description to function comment
// TODO:    PaddedBusRange - add argument and description to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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

  ResetAllPpbBusReg (Bridge, StartBusNumber);

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

      if (!EFI_ERROR (Status)   && 
          (IS_PCI_BRIDGE (&Pci) ||
          IS_CARDBUS_BRIDGE (&Pci))) {

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

        (*SubBusNumber)++;

        SecondBus = (*SubBusNumber);

        Register  = (UINT16) ((SecondBus << 8) | (UINT16) StartBusNumber);

        Address   = EFI_PCI_ADDRESS (StartBusNumber, Device, Func, 0x18);

        Status = PciRootBridgeIo->Pci.Write (
                                        PciRootBridgeIo,
                                        EfiPciWidthUint16,
                                        Address,
                                        1,
                                        &Register
                                        );

        //
        // Initialize SubBusNumber to SecondBus
        //
        Address = EFI_PCI_ADDRESS (StartBusNumber, Device, Func, 0x1A);
        Status = PciRootBridgeIo->Pci.Write (
                                        PciRootBridgeIo,
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
          Status = PciRootBridgeIo->Pci.Write (
                                          PciRootBridgeIo,
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

        Status = PciRootBridgeIo->Pci.Write (
                                        PciRootBridgeIo,
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

//
// Light PCI bus driver woundn't support P2C
// Return instead
//
EFI_STATUS
PciHostBridgeP2CProcess (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  )
/*++

Routine Description:
  
Arguments:

Returns:

  None

--*/
// TODO:    PciResAlloc - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  return EFI_SUCCESS;
}

//
// Light PCI bus driver woundn't support hotplug device
// Simplified the code
//
EFI_STATUS
PciHostBridgeEnumerator (
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *PciResAlloc
  )
/*++

Routine Description:

  This function is used to enumerate the entire host bridge 
  in a given platform

Arguments:

  PciResAlloc              A pointer to the protocol to allocate resource.

Returns:

  None

--*/
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_HANDLE                        RootBridgeHandle;
  PCI_IO_DEVICE                     *RootBridgeDev;
  EFI_STATUS                        Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL   *PciRootBridgeIo;
  UINT16                            MinBus;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptors;

  InitializeHotPlugSupport ();

  //
  // Notify the bus allocation phase is about to start
  //
  NotifyPhase (PciResAlloc, EfiPciHostBridgeBeginBusAllocation);

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

    if (EFI_ERROR (Status)) {
      return Status;
    }

    DestroyRootBridge (RootBridgeDev);

    //
    // Error proccess here
    //
  }
      
  //
  // Notify the bus allocation phase is to end
  //
  NotifyPhase (PciResAlloc, EfiPciHostBridgeEndBusAllocation);

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

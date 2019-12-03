/** @file
  PCI eunmeration implementation on entire PCI bus system for PCI Bus module.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PciBus.h"

/**
  This routine is used to enumerate entire pci bus system
  in a given platform.

  @param Controller          Parent controller handle.
  @param HostBridgeHandle    Host bridge handle.

  @retval EFI_SUCCESS    PCI enumeration finished successfully.
  @retval other          Some error occurred when enumerating the pci bus system.

**/
EFI_STATUS
PciEnumerator (
  IN EFI_HANDLE                    Controller,
  IN EFI_HANDLE                    HostBridgeHandle
  )
{
  EFI_STATUS                                        Status;
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *PciResAlloc;

  //
  // Get the pci host bridge resource allocation protocol
  //
  Status = gBS->OpenProtocol (
                  HostBridgeHandle,
                  &gEfiPciHostBridgeResourceAllocationProtocolGuid,
                  (VOID **) &PciResAlloc,
                  gPciBusDriverBinding.DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Notify the pci bus enumeration is about to begin
  //
  Status = NotifyPhase (PciResAlloc, EfiPciHostBridgeBeginEnumeration);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Start the bus allocation phase
  //
  Status = PciHostBridgeEnumerator (PciResAlloc);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Submit the resource request
  //
  Status = PciHostBridgeResourceAllocator (PciResAlloc);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Notify the pci bus enumeration is about to complete
  //
  Status = NotifyPhase (PciResAlloc, EfiPciHostBridgeEndEnumeration);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Process P2C
  //
  Status = PciHostBridgeP2CProcess (PciResAlloc);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Process attributes for devices on this host bridge
  //
  Status = PciHostBridgeDeviceAttribute (PciResAlloc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Enumerate PCI root bridge.

  @param PciResAlloc   Pointer to protocol instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.
  @param RootBridgeDev Instance of root bridge device.

  @retval EFI_SUCCESS  Successfully enumerated root bridge.
  @retval other        Failed to enumerate root bridge.

**/
EFI_STATUS
PciRootBridgeEnumerator (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *PciResAlloc,
  IN PCI_IO_DEVICE                                     *RootBridgeDev
  )
{
  EFI_STATUS                        Status;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Configuration;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Configuration1;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Configuration2;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Configuration3;
  UINT8                             SubBusNumber;
  UINT8                             StartBusNumber;
  UINT8                             PaddedBusRange;
  EFI_HANDLE                        RootBridgeHandle;
  UINT8                             Desc;
  UINT64                            AddrLen;
  UINT64                            AddrRangeMin;

  SubBusNumber    = 0;
  StartBusNumber  = 0;
  PaddedBusRange  = 0;

  //
  // Get the root bridge handle
  //
  RootBridgeHandle = RootBridgeDev->Handle;

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_IO_BUS_PCI | EFI_IOB_PCI_BUS_ENUM,
    RootBridgeDev->DevicePath
    );

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

  if (Configuration == NULL || Configuration->Desc == ACPI_END_TAG_DESCRIPTOR) {
    return EFI_INVALID_PARAMETER;
  }
  RootBridgeDev->BusNumberRanges = Configuration;

  //
  // Sort the descriptors in ascending order
  //
  for (Configuration1 = Configuration; Configuration1->Desc != ACPI_END_TAG_DESCRIPTOR; Configuration1++) {
    Configuration2 = Configuration1;
    for (Configuration3 = Configuration1 + 1; Configuration3->Desc != ACPI_END_TAG_DESCRIPTOR; Configuration3++) {
      if (Configuration2->AddrRangeMin > Configuration3->AddrRangeMin) {
        Configuration2 = Configuration3;
      }
    }
    //
    // All other fields other than AddrRangeMin and AddrLen are ignored in a descriptor,
    // so only need to swap these two fields.
    //
    if (Configuration2 != Configuration1) {
      AddrRangeMin = Configuration1->AddrRangeMin;
      Configuration1->AddrRangeMin = Configuration2->AddrRangeMin;
      Configuration2->AddrRangeMin = AddrRangeMin;

      AddrLen = Configuration1->AddrLen;
      Configuration1->AddrLen = Configuration2->AddrLen;
      Configuration2->AddrLen = AddrLen;
    }
  }

  //
  // Get the bus number to start with
  //
  StartBusNumber = (UINT8) (Configuration->AddrRangeMin);

  //
  // Initialize the subordinate bus number
  //
  SubBusNumber = StartBusNumber;

  //
  // Reset all assigned PCI bus number
  //
  ResetAllPpbBusNumber (
    RootBridgeDev,
    StartBusNumber
  );

  //
  // Assign bus number
  //
  Status = PciScanBus (
            RootBridgeDev,
            StartBusNumber,
            &SubBusNumber,
            &PaddedBusRange
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }


  //
  // Assign max bus number scanned
  //

  Status = PciAllocateBusNumber (RootBridgeDev, SubBusNumber, PaddedBusRange, &SubBusNumber);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find the bus range which contains the higest bus number, then returns the number of buses
  // that should be decoded.
  //
  while (Configuration->AddrRangeMin + Configuration->AddrLen - 1 < SubBusNumber) {
    Configuration++;
  }
  AddrLen = Configuration->AddrLen;
  Configuration->AddrLen = SubBusNumber - Configuration->AddrRangeMin + 1;

  //
  // Save the Desc field of the next descriptor. Mark the next descriptor as an END descriptor.
  //
  Configuration++;
  Desc = Configuration->Desc;
  Configuration->Desc = ACPI_END_TAG_DESCRIPTOR;

  //
  // Set bus number
  //
  Status = PciResAlloc->SetBusNumbers (
                          PciResAlloc,
                          RootBridgeHandle,
                          RootBridgeDev->BusNumberRanges
                          );

  //
  // Restore changed fields
  //
  Configuration->Desc = Desc;
  (Configuration - 1)->AddrLen = AddrLen;

  return Status;
}

/**
  This routine is used to process all PCI devices' Option Rom
  on a certain root bridge.

  @param Bridge     Given parent's root bridge.
  @param RomBase    Base address of ROM driver loaded from.
  @param MaxLength  Maximum rom size.

**/
VOID
ProcessOptionRom (
  IN PCI_IO_DEVICE *Bridge,
  IN UINT64        RomBase,
  IN UINT64        MaxLength
  )
{
  LIST_ENTRY      *CurrentLink;
  PCI_IO_DEVICE   *Temp;

  //
  // Go through bridges to reach all devices
  //
  CurrentLink = Bridge->ChildList.ForwardLink;
  while (CurrentLink != NULL && CurrentLink != &Bridge->ChildList) {
    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);
    if (!IsListEmpty (&Temp->ChildList)) {

      //
      // Go further to process the option rom under this bridge
      //
      ProcessOptionRom (Temp, RomBase, MaxLength);
    }

    if (Temp->RomSize != 0 && Temp->RomSize <= MaxLength) {

      //
      // Load and process the option rom
      //
      LoadOpRomImage (Temp, RomBase);
    }

    CurrentLink = CurrentLink->ForwardLink;
  }
}

/**
  This routine is used to assign bus number to the given PCI bus system

  @param Bridge             Parent root bridge instance.
  @param StartBusNumber     Number of beginning.
  @param SubBusNumber       The number of sub bus.

  @retval EFI_SUCCESS       Successfully assigned bus number.
  @retval EFI_DEVICE_ERROR  Failed to assign bus number.

**/
EFI_STATUS
PciAssignBusNumber (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber
  )
{
  EFI_STATUS                      Status;
  PCI_TYPE00                      Pci;
  UINT8                           Device;
  UINT8                           Func;
  UINT64                          Address;
  UINTN                           SecondBus;
  UINT16                          Register;
  UINT8                           Register8;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;

  PciRootBridgeIo = Bridge->PciRootBridgeIo;

  SecondBus       = 0;
  Register        = 0;

  *SubBusNumber = StartBusNumber;

  //
  // First check to see whether the parent is ppb
  //
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

      if (EFI_ERROR (Status) && Func == 0) {
        //
        // go to next device if there is no Function 0
        //
        break;
      }

      if (!EFI_ERROR (Status)   &&
          (IS_PCI_BRIDGE (&Pci) || IS_CARDBUS_BRIDGE (&Pci))) {

        //
        // Reserved one bus for cardbus bridge
        //
        Status = PciAllocateBusNumber (Bridge, *SubBusNumber, 1, SubBusNumber);
        if (EFI_ERROR (Status)) {
          return Status;
        }
        SecondBus = *SubBusNumber;

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

          Register8 = 0xFF;
          Status = PciRootBridgeIo->Pci.Write (
                                          PciRootBridgeIo,
                                          EfiPciWidthUint8,
                                          Address,
                                          1,
                                          &Register8
                                          );

          Status = PciAssignBusNumber (
                    Bridge,
                    (UINT8) (SecondBus),
                    SubBusNumber
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

/**
  This routine is used to determine the root bridge attribute by interfacing
  the host bridge resource allocation protocol.

  @param PciResAlloc    Protocol instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
  @param RootBridgeDev  Root bridge instance

  @retval EFI_SUCCESS  Successfully got root bridge's attribute.
  @retval other        Failed to get attribute.

**/
EFI_STATUS
DetermineRootBridgeAttributes (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc,
  IN PCI_IO_DEVICE                                    *RootBridgeDev
  )
{
  UINT64      Attributes;
  EFI_STATUS  Status;
  EFI_HANDLE  RootBridgeHandle;

  Attributes        = 0;
  RootBridgeHandle  = RootBridgeDev->Handle;

  //
  // Get root bridge attribute by calling into pci host bridge resource allocation protocol
  //
  Status = PciResAlloc->GetAllocAttributes (
                          PciResAlloc,
                          RootBridgeHandle,
                          &Attributes
                          );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Here is the point where PCI bus driver calls HOST bridge allocation protocol
  // Currently we hardcoded for ea815
  //
  if ((Attributes & EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM) != 0) {
    RootBridgeDev->Decodes |= EFI_BRIDGE_PMEM_MEM_COMBINE_SUPPORTED;
  }

  if ((Attributes & EFI_PCI_HOST_BRIDGE_MEM64_DECODE) != 0) {
    RootBridgeDev->Decodes |= EFI_BRIDGE_MEM64_DECODE_SUPPORTED;
    RootBridgeDev->Decodes |= EFI_BRIDGE_PMEM64_DECODE_SUPPORTED;
  }

  RootBridgeDev->Decodes |= EFI_BRIDGE_MEM32_DECODE_SUPPORTED;
  RootBridgeDev->Decodes |= EFI_BRIDGE_PMEM32_DECODE_SUPPORTED;
  RootBridgeDev->Decodes |= EFI_BRIDGE_IO16_DECODE_SUPPORTED;

  return EFI_SUCCESS;
}

/**
  Get Max Option Rom size on specified bridge.

  @param Bridge    Given bridge device instance.

  @return Max size of option rom needed.

**/
UINT32
GetMaxOptionRomSize (
  IN PCI_IO_DEVICE   *Bridge
  )
{
  LIST_ENTRY      *CurrentLink;
  PCI_IO_DEVICE   *Temp;
  UINT32          MaxOptionRomSize;
  UINT32          TempOptionRomSize;

  MaxOptionRomSize = 0;

  //
  // Go through bridges to reach all devices
  //
  CurrentLink = Bridge->ChildList.ForwardLink;
  while (CurrentLink != NULL && CurrentLink != &Bridge->ChildList) {
    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);
    if (!IsListEmpty (&Temp->ChildList)) {

      //
      // Get max option rom size under this bridge
      //
      TempOptionRomSize = GetMaxOptionRomSize (Temp);

      //
      // Compare with the option rom size of the bridge
      // Get the larger one
      //
      if (Temp->RomSize > TempOptionRomSize) {
        TempOptionRomSize = Temp->RomSize;
      }

    } else {

      //
      // For devices get the rom size directly
      //
      TempOptionRomSize = Temp->RomSize;
    }

    //
    // Get the largest rom size on this bridge
    //
    if (TempOptionRomSize > MaxOptionRomSize) {
      MaxOptionRomSize = TempOptionRomSize;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return MaxOptionRomSize;
}

/**
  Process attributes of devices on this host bridge

  @param PciResAlloc Protocol instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.

  @retval EFI_SUCCESS   Successfully process attribute.
  @retval EFI_NOT_FOUND Can not find the specific root bridge device.
  @retval other         Failed to determine the root bridge device's attribute.

**/
EFI_STATUS
PciHostBridgeDeviceAttribute (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  )
{
  EFI_HANDLE    RootBridgeHandle;
  PCI_IO_DEVICE *RootBridgeDev;
  EFI_STATUS    Status;

  RootBridgeHandle = NULL;

  while (PciResAlloc->GetNextRootBridge (PciResAlloc, &RootBridgeHandle) == EFI_SUCCESS) {

    //
    // Get RootBridg Device by handle
    //
    RootBridgeDev = GetRootBridgeByHandle (RootBridgeHandle);

    if (RootBridgeDev == NULL) {
      return EFI_NOT_FOUND;
    }

    //
    // Set the attributes for devcies behind the Root Bridge
    //
    Status = DetermineDeviceAttribute (RootBridgeDev);
    if (EFI_ERROR (Status)) {
      return Status;
    }

  }

  return EFI_SUCCESS;
}

/**
  Get resource allocation status from the ACPI resource descriptor.

  @param AcpiConfig       Point to Acpi configuration table.
  @param IoResStatus      Return the status of I/O resource.
  @param Mem32ResStatus   Return the status of 32-bit Memory resource.
  @param PMem32ResStatus  Return the status of 32-bit Prefetchable Memory resource.
  @param Mem64ResStatus   Return the status of 64-bit Memory resource.
  @param PMem64ResStatus  Return the status of 64-bit Prefetchable Memory resource.

**/
VOID
GetResourceAllocationStatus (
  VOID        *AcpiConfig,
  OUT UINT64  *IoResStatus,
  OUT UINT64  *Mem32ResStatus,
  OUT UINT64  *PMem32ResStatus,
  OUT UINT64  *Mem64ResStatus,
  OUT UINT64  *PMem64ResStatus
  )
{
  UINT8                             *Temp;
  UINT64                            ResStatus;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *ACPIAddressDesc;

  Temp = (UINT8 *) AcpiConfig;

  while (*Temp == ACPI_ADDRESS_SPACE_DESCRIPTOR) {

    ACPIAddressDesc       = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp;
    ResStatus = ACPIAddressDesc->AddrTranslationOffset;

    switch (ACPIAddressDesc->ResType) {
    case 0:
      if (ACPIAddressDesc->AddrSpaceGranularity == 32) {
        if (ACPIAddressDesc->SpecificFlag == 0x06) {
          //
          // Pmem32
          //
          *PMem32ResStatus = ResStatus;
        } else {
          //
          // Mem32
          //
          *Mem32ResStatus = ResStatus;
        }
      }

      if (ACPIAddressDesc->AddrSpaceGranularity == 64) {
        if (ACPIAddressDesc->SpecificFlag == 0x06) {
          //
          // PMem64
          //
          *PMem64ResStatus = ResStatus;
        } else {
          //
          // Mem64
          //
          *Mem64ResStatus = ResStatus;
        }
      }

      break;

    case 1:
      //
      // Io
      //
      *IoResStatus = ResStatus;
      break;

    default:
      break;
    }

    Temp += sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
  }
}

/**
  Remove a PCI device from device pool and mark its bar.

  @param PciDevice Instance of Pci device.

  @retval EFI_SUCCESS Successfully remove the PCI device.
  @retval EFI_ABORTED Pci device is a root bridge or a PCI-PCI bridge.

**/
EFI_STATUS
RejectPciDevice (
  IN PCI_IO_DEVICE       *PciDevice
  )
{
  PCI_IO_DEVICE   *Bridge;
  PCI_IO_DEVICE   *Temp;
  LIST_ENTRY      *CurrentLink;

  //
  // Remove the padding resource from a bridge
  //
  if ( IS_PCI_BRIDGE(&PciDevice->Pci) &&
       PciDevice->ResourcePaddingDescriptors != NULL ) {
    FreePool (PciDevice->ResourcePaddingDescriptors);
    PciDevice->ResourcePaddingDescriptors = NULL;
    return EFI_SUCCESS;
  }

  //
  // Skip RB and PPB
  //
  if (IS_PCI_BRIDGE (&PciDevice->Pci) || (PciDevice->Parent == NULL)) {
    return EFI_ABORTED;
  }

  if (IS_CARDBUS_BRIDGE (&PciDevice->Pci)) {
    //
    // Get the root bridge device
    //
    Bridge = PciDevice;
    while (Bridge->Parent != NULL) {
      Bridge = Bridge->Parent;
    }

    RemoveAllPciDeviceOnBridge (Bridge->Handle, PciDevice);

    //
    // Mark its bar
    //
    InitializeP2C (PciDevice);
  }

  //
  // Remove the device
  //
  Bridge      = PciDevice->Parent;
  CurrentLink = Bridge->ChildList.ForwardLink;
  while (CurrentLink != NULL && CurrentLink != &Bridge->ChildList) {
    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);
    if (Temp == PciDevice) {
      InitializePciDevice (Temp);
      RemoveEntryList (CurrentLink);
      return EFI_SUCCESS;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return EFI_ABORTED;
}

/**
  Determine whethter a PCI device can be rejected.

  @param  PciResNode Pointer to Pci resource node instance.

  @retval TRUE  The PCI device can be rejected.
  @retval TRUE  The PCI device cannot be rejected.

**/
BOOLEAN
IsRejectiveDevice (
  IN  PCI_RESOURCE_NODE   *PciResNode
  )
{
  PCI_IO_DEVICE *Temp;

  Temp = PciResNode->PciDev;

  //
  // Ensure the device is present
  //
  if (Temp == NULL) {
    return FALSE;
  }

  //
  // PPB and RB should go ahead
  //
  if (IS_PCI_BRIDGE (&Temp->Pci) || (Temp->Parent == NULL)) {
    return TRUE;
  }

  //
  // Skip device on Bus0
  //
  if ((Temp->Parent != NULL) && (Temp->BusNumber == 0)) {
    return FALSE;
  }

  //
  // Skip VGA
  //
  if (IS_PCI_VGA (&Temp->Pci)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Compare two resource nodes and get the larger resource consumer.

  @param PciResNode1  resource node 1 want to be compared
  @param PciResNode2  resource node 2 want to be compared

  @return Larger resource node.

**/
PCI_RESOURCE_NODE *
GetLargerConsumerDevice (
  IN  PCI_RESOURCE_NODE   *PciResNode1,
  IN  PCI_RESOURCE_NODE   *PciResNode2
  )
{
  if (PciResNode2 == NULL) {
    return PciResNode1;
  }

  if ((IS_PCI_BRIDGE(&(PciResNode2->PciDev->Pci)) || (PciResNode2->PciDev->Parent == NULL)) \
       && (PciResNode2->ResourceUsage != PciResUsagePadding) )
  {
    return PciResNode1;
  }

  if (PciResNode1 == NULL) {
    return PciResNode2;
  }

  if ((PciResNode1->Length) > (PciResNode2->Length)) {
    return PciResNode1;
  }

  return PciResNode2;
}


/**
  Get the max resource consumer in the host resource pool.

  @param ResPool  Pointer to resource pool node.

  @return The max resource consumer in the host resource pool.

**/
PCI_RESOURCE_NODE *
GetMaxResourceConsumerDevice (
  IN  PCI_RESOURCE_NODE   *ResPool
  )
{
  PCI_RESOURCE_NODE *Temp;
  LIST_ENTRY        *CurrentLink;
  PCI_RESOURCE_NODE *PciResNode;
  PCI_RESOURCE_NODE *PPBResNode;

  PciResNode  = NULL;

  CurrentLink = ResPool->ChildList.ForwardLink;
  while (CurrentLink != NULL && CurrentLink != &ResPool->ChildList) {

    Temp = RESOURCE_NODE_FROM_LINK (CurrentLink);

    if (!IsRejectiveDevice (Temp)) {
      CurrentLink = CurrentLink->ForwardLink;
      continue;
    }

    if ((IS_PCI_BRIDGE (&(Temp->PciDev->Pci)) || (Temp->PciDev->Parent == NULL)) \
          && (Temp->ResourceUsage != PciResUsagePadding))
    {
      PPBResNode  = GetMaxResourceConsumerDevice (Temp);
      PciResNode  = GetLargerConsumerDevice (PciResNode, PPBResNode);
    } else {
      PciResNode = GetLargerConsumerDevice (PciResNode, Temp);
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return PciResNode;
}

/**
  Adjust host bridge allocation so as to reduce resource requirement

  @param IoPool           Pointer to instance of I/O resource Node.
  @param Mem32Pool        Pointer to instance of 32-bit memory resource Node.
  @param PMem32Pool       Pointer to instance of 32-bit Prefetchable memory resource node.
  @param Mem64Pool        Pointer to instance of 64-bit memory resource node.
  @param PMem64Pool       Pointer to instance of 64-bit Prefetchable memory resource node.
  @param IoResStatus      Status of I/O resource Node.
  @param Mem32ResStatus   Status of 32-bit memory resource Node.
  @param PMem32ResStatus  Status of 32-bit Prefetchable memory resource node.
  @param Mem64ResStatus   Status of 64-bit memory resource node.
  @param PMem64ResStatus  Status of 64-bit Prefetchable memory resource node.

  @retval EFI_SUCCESS     Successfully adjusted resource on host bridge.
  @retval EFI_ABORTED     Host bridge hasn't this resource type or no resource be adjusted.

**/
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
{
  BOOLEAN                               AllocationAjusted;
  PCI_RESOURCE_NODE                     *PciResNode;
  PCI_RESOURCE_NODE                     *ResPool[5];
  PCI_IO_DEVICE                         *RemovedPciDev[5];
  UINT64                                ResStatus[5];
  UINTN                                 RemovedPciDevNum;
  UINTN                                 DevIndex;
  UINTN                                 ResType;
  EFI_STATUS                            Status;
  EFI_RESOURCE_ALLOC_FAILURE_ERROR_DATA_PAYLOAD AllocFailExtendedData;

  PciResNode = NULL;
  ZeroMem (RemovedPciDev, 5 * sizeof (PCI_IO_DEVICE *));
  RemovedPciDevNum  = 0;

  ResPool[0]        = IoPool;
  ResPool[1]        = Mem32Pool;
  ResPool[2]        = PMem32Pool;
  ResPool[3]        = Mem64Pool;
  ResPool[4]        = PMem64Pool;

  ResStatus[0]      = IoResStatus;
  ResStatus[1]      = Mem32ResStatus;
  ResStatus[2]      = PMem32ResStatus;
  ResStatus[3]      = Mem64ResStatus;
  ResStatus[4]      = PMem64ResStatus;

  AllocationAjusted = FALSE;

  for (ResType = 0; ResType < 5; ResType++) {

    if (ResStatus[ResType] == EFI_RESOURCE_SATISFIED) {
      continue;
    }

    if (ResStatus[ResType] == EFI_RESOURCE_NOT_SATISFIED) {
      //
      // Host bridge hasn't this resource type
      //
      return EFI_ABORTED;
    }

    //
    // Hostbridge hasn't enough resource
    //
    PciResNode = GetMaxResourceConsumerDevice (ResPool[ResType]);
    if (PciResNode == NULL) {
      continue;
    }

    //
    // Check if the device has been removed before
    //
    for (DevIndex = 0; DevIndex < RemovedPciDevNum; DevIndex++) {
      if (PciResNode->PciDev == RemovedPciDev[DevIndex]) {
        break;
      }
    }

    if (DevIndex != RemovedPciDevNum) {
      continue;
    }

    //
    // Remove the device if it isn't in the array
    //
    Status = RejectPciDevice (PciResNode->PciDev);
    if (Status == EFI_SUCCESS) {
      DEBUG ((
        EFI_D_ERROR,
        "PciBus: [%02x|%02x|%02x] was rejected due to resource confliction.\n",
        PciResNode->PciDev->BusNumber, PciResNode->PciDev->DeviceNumber, PciResNode->PciDev->FunctionNumber
        ));

      //
      // Raise the EFI_IOB_EC_RESOURCE_CONFLICT status code
      //
      //
      // Have no way to get ReqRes, AllocRes & Bar here
      //
      ZeroMem (&AllocFailExtendedData, sizeof (AllocFailExtendedData));
      AllocFailExtendedData.DevicePathSize = (UINT16) sizeof (EFI_DEVICE_PATH_PROTOCOL);
      AllocFailExtendedData.DevicePath     = (UINT8 *) PciResNode->PciDev->DevicePath;
      AllocFailExtendedData.Bar            = PciResNode->Bar;

      REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
            EFI_PROGRESS_CODE,
            EFI_IO_BUS_PCI | EFI_IOB_EC_RESOURCE_CONFLICT,
            (VOID *) &AllocFailExtendedData,
            sizeof (AllocFailExtendedData)
            );

      //
      // Add it to the array and indicate at least a device has been rejected
      //
      RemovedPciDev[RemovedPciDevNum++] = PciResNode->PciDev;
      AllocationAjusted                 = TRUE;
    }
  }
  //
  // End for
  //

  if (AllocationAjusted) {
    return EFI_SUCCESS;
  } else {
    return EFI_ABORTED;
  }
}

/**
  Summary requests for all resource type, and construct ACPI resource
  requestor instance.

  @param Bridge           detecting bridge
  @param IoNode           Pointer to instance of I/O resource Node
  @param Mem32Node        Pointer to instance of 32-bit memory resource Node
  @param PMem32Node       Pointer to instance of 32-bit Pmemory resource node
  @param Mem64Node        Pointer to instance of 64-bit memory resource node
  @param PMem64Node       Pointer to instance of 64-bit Pmemory resource node
  @param Config           Output buffer holding new constructed APCI resource requestor

  @retval EFI_SUCCESS           Successfully constructed ACPI resource.
  @retval EFI_OUT_OF_RESOURCES  No memory available.

**/
EFI_STATUS
ConstructAcpiResourceRequestor (
  IN PCI_IO_DEVICE      *Bridge,
  IN PCI_RESOURCE_NODE  *IoNode,
  IN PCI_RESOURCE_NODE  *Mem32Node,
  IN PCI_RESOURCE_NODE  *PMem32Node,
  IN PCI_RESOURCE_NODE  *Mem64Node,
  IN PCI_RESOURCE_NODE  *PMem64Node,
  OUT VOID              **Config
  )
{
  UINT8                             NumConfig;
  UINT8                             Aperture;
  UINT8                             *Configuration;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Ptr;
  EFI_ACPI_END_TAG_DESCRIPTOR       *PtrEnd;

  NumConfig = 0;
  Aperture  = 0;

  *Config  = NULL;

  //
  // if there is io request, add to the io aperture
  //
  if (ResourceRequestExisted (IoNode)) {
    NumConfig++;
    Aperture |= 0x01;
  }

  //
  // if there is mem32 request, add to the mem32 aperture
  //
  if (ResourceRequestExisted (Mem32Node)) {
    NumConfig++;
    Aperture |= 0x02;
  }

  //
  // if there is pmem32 request, add to the pmem32 aperture
  //
  if (ResourceRequestExisted (PMem32Node)) {
    NumConfig++;
    Aperture |= 0x04;
  }

  //
  // if there is mem64 request, add to the mem64 aperture
  //
  if (ResourceRequestExisted (Mem64Node)) {
    NumConfig++;
    Aperture |= 0x08;
  }

  //
  // if there is pmem64 request, add to the pmem64 aperture
  //
  if (ResourceRequestExisted (PMem64Node)) {
    NumConfig++;
    Aperture |= 0x10;
  }

  if (NumConfig != 0) {

    //
    // If there is at least one type of resource request,
    // allocate a acpi resource node
    //
    Configuration = AllocateZeroPool (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) * NumConfig + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
    if (Configuration == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Ptr = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;

    //
    // Deal with io aperture
    //
    if ((Aperture & 0x01) != 0) {
      Ptr->Desc     = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Ptr->Len      = (UINT16) (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3);
      //
      // Io
      //
      Ptr->ResType  = ACPI_ADDRESS_SPACE_TYPE_IO;
      //
      // non ISA range
      //
      Ptr->SpecificFlag = 1;
      Ptr->AddrLen      = IoNode->Length;
      Ptr->AddrRangeMax = IoNode->Alignment;

      Ptr++;
    }
    //
    // Deal with mem32 aperture
    //
    if ((Aperture & 0x02) != 0) {
      Ptr->Desc     = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Ptr->Len      = (UINT16) (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3);
      //
      // Mem
      //
      Ptr->ResType  = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // Nonprefechable
      //
      Ptr->SpecificFlag = 0;
      //
      // 32 bit
      //
      Ptr->AddrSpaceGranularity = 32;
      Ptr->AddrLen      = Mem32Node->Length;
      Ptr->AddrRangeMax = Mem32Node->Alignment;

      Ptr++;
    }

    //
    // Deal with Pmem32 aperture
    //
    if ((Aperture & 0x04) != 0) {
      Ptr->Desc     = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Ptr->Len      = (UINT16) (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3);
      //
      // Mem
      //
      Ptr->ResType  = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // prefechable
      //
      Ptr->SpecificFlag = 0x6;
      //
      // 32 bit
      //
      Ptr->AddrSpaceGranularity = 32;
      Ptr->AddrLen      = PMem32Node->Length;
      Ptr->AddrRangeMax = PMem32Node->Alignment;

      Ptr++;
    }
    //
    // Deal with mem64 aperture
    //
    if ((Aperture & 0x08) != 0) {
      Ptr->Desc     = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Ptr->Len      = (UINT16) (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3);
      //
      // Mem
      //
      Ptr->ResType  = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // nonprefechable
      //
      Ptr->SpecificFlag = 0;
      //
      // 64 bit
      //
      Ptr->AddrSpaceGranularity = 64;
      Ptr->AddrLen      = Mem64Node->Length;
      Ptr->AddrRangeMax = Mem64Node->Alignment;

      Ptr++;
    }
    //
    // Deal with Pmem64 aperture
    //
    if ((Aperture & 0x10) != 0) {
      Ptr->Desc     = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Ptr->Len      = (UINT16) (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3);
      //
      // Mem
      //
      Ptr->ResType  = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // prefechable
      //
      Ptr->SpecificFlag = 0x06;
      //
      // 64 bit
      //
      Ptr->AddrSpaceGranularity = 64;
      Ptr->AddrLen      = PMem64Node->Length;
      Ptr->AddrRangeMax = PMem64Node->Alignment;

      Ptr++;
    }

    //
    // put the checksum
    //
    PtrEnd            = (EFI_ACPI_END_TAG_DESCRIPTOR *) Ptr;

    PtrEnd->Desc      = ACPI_END_TAG_DESCRIPTOR;
    PtrEnd->Checksum  = 0;

  } else {

    //
    // If there is no resource request
    //
    Configuration = AllocateZeroPool (sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
    if (Configuration == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    PtrEnd            = (EFI_ACPI_END_TAG_DESCRIPTOR *) (Configuration);
    PtrEnd->Desc      = ACPI_END_TAG_DESCRIPTOR;
    PtrEnd->Checksum  = 0;
  }

  *Config = Configuration;

  return EFI_SUCCESS;
}

/**
  Get resource base from an acpi configuration descriptor.

  @param Config       An acpi configuration descriptor.
  @param IoBase       Output of I/O resource base address.
  @param Mem32Base    Output of 32-bit memory base address.
  @param PMem32Base   Output of 32-bit prefetchable memory base address.
  @param Mem64Base    Output of 64-bit memory base address.
  @param PMem64Base   Output of 64-bit prefetchable memory base address.

**/
VOID
GetResourceBase (
  IN VOID     *Config,
  OUT UINT64  *IoBase,
  OUT UINT64  *Mem32Base,
  OUT UINT64  *PMem32Base,
  OUT UINT64  *Mem64Base,
  OUT UINT64  *PMem64Base
  )
{
  UINT8                             *Temp;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Ptr;
  UINT64                            ResStatus;

  ASSERT (Config != NULL);

  *IoBase     = 0xFFFFFFFFFFFFFFFFULL;
  *Mem32Base  = 0xFFFFFFFFFFFFFFFFULL;
  *PMem32Base = 0xFFFFFFFFFFFFFFFFULL;
  *Mem64Base  = 0xFFFFFFFFFFFFFFFFULL;
  *PMem64Base = 0xFFFFFFFFFFFFFFFFULL;

  Temp        = (UINT8 *) Config;

  while (*Temp == ACPI_ADDRESS_SPACE_DESCRIPTOR) {

    Ptr       = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp;
    ResStatus = Ptr->AddrTranslationOffset;

    if (ResStatus == EFI_RESOURCE_SATISFIED) {

      switch (Ptr->ResType) {

      //
      // Memory type aperture
      //
      case 0:

        //
        // Check to see the granularity
        //
        if (Ptr->AddrSpaceGranularity == 32) {
          if ((Ptr->SpecificFlag & 0x06) != 0) {
            *PMem32Base = Ptr->AddrRangeMin;
          } else {
            *Mem32Base = Ptr->AddrRangeMin;
          }
        }

        if (Ptr->AddrSpaceGranularity == 64) {
          if ((Ptr->SpecificFlag & 0x06) != 0) {
            *PMem64Base = Ptr->AddrRangeMin;
          } else {
            *Mem64Base = Ptr->AddrRangeMin;
          }
        }
        break;

      case 1:

        //
        // Io type aperture
        //
        *IoBase = Ptr->AddrRangeMin;
        break;

      default:
        break;

      }
      //
      // End switch
      //
    }
    //
    // End for
    //
    Temp += sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
  }
}

/**
  Enumerate pci bridge, allocate resource and determine attribute
  for devices on this bridge.

  @param BridgeDev    Pointer to instance of bridge device.

  @retval EFI_SUCCESS Successfully enumerated PCI bridge.
  @retval other       Failed to enumerate.

**/
EFI_STATUS
PciBridgeEnumerator (
  IN PCI_IO_DEVICE                                     *BridgeDev
  )
{
  UINT8               SubBusNumber;
  UINT8               StartBusNumber;
  EFI_PCI_IO_PROTOCOL *PciIo;
  EFI_STATUS          Status;

  SubBusNumber    = 0;
  StartBusNumber  = 0;
  PciIo           = &(BridgeDev->PciIo);
  Status          = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x19, 1, &StartBusNumber);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciAssignBusNumber (
            BridgeDev,
            StartBusNumber,
            &SubBusNumber
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciPciDeviceInfoCollector (BridgeDev, StartBusNumber);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciBridgeResourceAllocator (BridgeDev);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DetermineDeviceAttribute (BridgeDev);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;

}

/**
  Allocate all kinds of resource for PCI bridge.

  @param  Bridge      Pointer to bridge instance.

  @retval EFI_SUCCESS Successfully allocated resource for PCI bridge.
  @retval other       Failed to allocate resource for bridge.

**/
EFI_STATUS
PciBridgeResourceAllocator (
  IN PCI_IO_DEVICE  *Bridge
  )
{
  PCI_RESOURCE_NODE *IoBridge;
  PCI_RESOURCE_NODE *Mem32Bridge;
  PCI_RESOURCE_NODE *PMem32Bridge;
  PCI_RESOURCE_NODE *Mem64Bridge;
  PCI_RESOURCE_NODE *PMem64Bridge;
  UINT64            IoBase;
  UINT64            Mem32Base;
  UINT64            PMem32Base;
  UINT64            Mem64Base;
  UINT64            PMem64Base;
  EFI_STATUS        Status;

  IoBridge = CreateResourceNode (
               Bridge,
               0,
               Bridge->BridgeIoAlignment,
               0,
               PciBarTypeIo16,
               PciResUsageTypical
               );

  Mem32Bridge = CreateResourceNode (
                  Bridge,
                  0,
                  0xFFFFF,
                  0,
                  PciBarTypeMem32,
                  PciResUsageTypical
                  );

  PMem32Bridge = CreateResourceNode (
                   Bridge,
                   0,
                   0xFFFFF,
                   0,
                   PciBarTypePMem32,
                   PciResUsageTypical
                   );

  Mem64Bridge = CreateResourceNode (
                  Bridge,
                  0,
                  0xFFFFF,
                  0,
                  PciBarTypeMem64,
                  PciResUsageTypical
                  );

  PMem64Bridge = CreateResourceNode (
                   Bridge,
                   0,
                   0xFFFFF,
                   0,
                   PciBarTypePMem64,
                   PciResUsageTypical
                   );

  //
  // Create resourcemap by going through all the devices subject to this root bridge
  //
  CreateResourceMap (
    Bridge,
    IoBridge,
    Mem32Bridge,
    PMem32Bridge,
    Mem64Bridge,
    PMem64Bridge
    );

  Status = GetResourceBaseFromBridge (
             Bridge,
             &IoBase,
             &Mem32Base,
             &PMem32Base,
             &Mem64Base,
             &PMem64Base
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

  DestroyResourceTree (IoBridge);
  DestroyResourceTree (Mem32Bridge);
  DestroyResourceTree (PMem32Bridge);
  DestroyResourceTree (PMem64Bridge);
  DestroyResourceTree (Mem64Bridge);

  gBS->FreePool (IoBridge);
  gBS->FreePool (Mem32Bridge);
  gBS->FreePool (PMem32Bridge);
  gBS->FreePool (PMem64Bridge);
  gBS->FreePool (Mem64Bridge);

  return EFI_SUCCESS;
}

/**
  Get resource base address for a pci bridge device.

  @param Bridge     Given Pci driver instance.
  @param IoBase     Output for base address of I/O type resource.
  @param Mem32Base  Output for base address of 32-bit memory type resource.
  @param PMem32Base Ooutput for base address of 32-bit Pmemory type resource.
  @param Mem64Base  Output for base address of 64-bit memory type resource.
  @param PMem64Base Output for base address of 64-bit Pmemory type resource.

  @retval EFI_SUCCESS           Successfully got resource base address.
  @retval EFI_OUT_OF_RESOURCES  PCI bridge is not available.

**/
EFI_STATUS
GetResourceBaseFromBridge (
  IN  PCI_IO_DEVICE *Bridge,
  OUT UINT64        *IoBase,
  OUT UINT64        *Mem32Base,
  OUT UINT64        *PMem32Base,
  OUT UINT64        *Mem64Base,
  OUT UINT64        *PMem64Base
  )
{
  if (!Bridge->Allocated) {
    return EFI_OUT_OF_RESOURCES;
  }

  *IoBase     = gAllOne;
  *Mem32Base  = gAllOne;
  *PMem32Base = gAllOne;
  *Mem64Base  = gAllOne;
  *PMem64Base = gAllOne;

  if (IS_PCI_BRIDGE (&Bridge->Pci)) {

    if (Bridge->PciBar[PPB_IO_RANGE].Length > 0) {
      *IoBase = Bridge->PciBar[PPB_IO_RANGE].BaseAddress;
    }

    if (Bridge->PciBar[PPB_MEM32_RANGE].Length > 0) {
      *Mem32Base = Bridge->PciBar[PPB_MEM32_RANGE].BaseAddress;
    }

    if (Bridge->PciBar[PPB_PMEM32_RANGE].Length > 0) {
      *PMem32Base = Bridge->PciBar[PPB_PMEM32_RANGE].BaseAddress;
    }

    if (Bridge->PciBar[PPB_PMEM64_RANGE].Length > 0) {
      *PMem64Base = Bridge->PciBar[PPB_PMEM64_RANGE].BaseAddress;
    } else {
      *PMem64Base = gAllOne;
    }

  }

  if (IS_CARDBUS_BRIDGE (&Bridge->Pci)) {
    if (Bridge->PciBar[P2C_IO_1].Length > 0) {
      *IoBase = Bridge->PciBar[P2C_IO_1].BaseAddress;
    } else {
      if (Bridge->PciBar[P2C_IO_2].Length > 0) {
        *IoBase = Bridge->PciBar[P2C_IO_2].BaseAddress;
      }
    }

    if (Bridge->PciBar[P2C_MEM_1].Length > 0) {
      if (Bridge->PciBar[P2C_MEM_1].BarType == PciBarTypePMem32) {
        *PMem32Base = Bridge->PciBar[P2C_MEM_1].BaseAddress;
      }

      if (Bridge->PciBar[P2C_MEM_1].BarType == PciBarTypeMem32) {
        *Mem32Base = Bridge->PciBar[P2C_MEM_1].BaseAddress;
      }
    }

    if (Bridge->PciBar[P2C_MEM_2].Length > 0) {
      if (Bridge->PciBar[P2C_MEM_2].BarType == PciBarTypePMem32) {
        *PMem32Base = Bridge->PciBar[P2C_MEM_2].BaseAddress;
      }

      if (Bridge->PciBar[P2C_MEM_2].BarType == PciBarTypeMem32) {
        *Mem32Base = Bridge->PciBar[P2C_MEM_2].BaseAddress;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
   These are the notifications from the PCI bus driver that it is about to enter a certain
   phase of the PCI enumeration process.

   This member function can be used to notify the host bridge driver to perform specific actions,
   including any chipset-specific initialization, so that the chipset is ready to enter the next phase.
   Eight notification points are defined at this time. See belows:
   EfiPciHostBridgeBeginEnumeration       Resets the host bridge PCI apertures and internal data
                                          structures. The PCI enumerator should issue this notification
                                          before starting a fresh enumeration process. Enumeration cannot
                                          be restarted after sending any other notification such as
                                          EfiPciHostBridgeBeginBusAllocation.
   EfiPciHostBridgeBeginBusAllocation     The bus allocation phase is about to begin. No specific action is
                                          required here. This notification can be used to perform any
                                          chipset-specific programming.
   EfiPciHostBridgeEndBusAllocation       The bus allocation and bus programming phase is complete. No
                                          specific action is required here. This notification can be used to
                                          perform any chipset-specific programming.
   EfiPciHostBridgeBeginResourceAllocation
                                          The resource allocation phase is about to begin. No specific
                                          action is required here. This notification can be used to perform
                                          any chipset-specific programming.
   EfiPciHostBridgeAllocateResources      Allocates resources per previously submitted requests for all the PCI
                                          root bridges. These resource settings are returned on the next call to
                                          GetProposedResources(). Before calling NotifyPhase() with a Phase of
                                          EfiPciHostBridgeAllocateResource, the PCI bus enumerator is responsible
                                          for gathering I/O and memory requests for
                                          all the PCI root bridges and submitting these requests using
                                          SubmitResources(). This function pads the resource amount
                                          to suit the root bridge hardware, takes care of dependencies between
                                          the PCI root bridges, and calls the Global Coherency Domain (GCD)
                                          with the allocation request. In the case of padding, the allocated range
                                          could be bigger than what was requested.
   EfiPciHostBridgeSetResources           Programs the host bridge hardware to decode previously allocated
                                          resources (proposed resources) for all the PCI root bridges. After the
                                          hardware is programmed, reassigning resources will not be supported.
                                          The bus settings are not affected.
   EfiPciHostBridgeFreeResources          Deallocates resources that were previously allocated for all the PCI
                                          root bridges and resets the I/O and memory apertures to their initial
                                          state. The bus settings are not affected. If the request to allocate
                                          resources fails, the PCI enumerator can use this notification to
                                          deallocate previous resources, adjust the requests, and retry
                                          allocation.
   EfiPciHostBridgeEndResourceAllocation  The resource allocation phase is completed. No specific action is
                                          required here. This notification can be used to perform any chipsetspecific
                                          programming.

   @param[in] PciResAlloc         The instance pointer of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
   @param[in] Phase               The phase during enumeration

   @retval EFI_NOT_READY          This phase cannot be entered at this time. For example, this error
                                  is valid for a Phase of EfiPciHostBridgeAllocateResources if
                                  SubmitResources() has not been called for one or more
                                  PCI root bridges before this call
   @retval EFI_DEVICE_ERROR       Programming failed due to a hardware error. This error is valid
                                  for a Phase of EfiPciHostBridgeSetResources.
   @retval EFI_INVALID_PARAMETER  Invalid phase parameter
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
                                  This error is valid for a Phase of EfiPciHostBridgeAllocateResources if the
                                  previously submitted resource requests cannot be fulfilled or
                                  were only partially fulfilled.
   @retval EFI_SUCCESS            The notification was accepted without any errors.

**/
EFI_STATUS
NotifyPhase (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc,
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE       Phase
  )
{
  EFI_HANDLE                      HostBridgeHandle;
  EFI_HANDLE                      RootBridgeHandle;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  EFI_STATUS                      Status;

  HostBridgeHandle  = NULL;
  RootBridgeHandle  = NULL;
  if (gPciPlatformProtocol != NULL) {
    //
    // Get Host Bridge Handle.
    //
    PciResAlloc->GetNextRootBridge (PciResAlloc, &RootBridgeHandle);

    //
    // Get the rootbridge Io protocol to find the host bridge handle
    //
    Status = gBS->HandleProtocol (
                    RootBridgeHandle,
                    &gEfiPciRootBridgeIoProtocolGuid,
                    (VOID **) &PciRootBridgeIo
                    );

    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }

    HostBridgeHandle = PciRootBridgeIo->ParentHandle;

    //
    // Call PlatformPci::PlatformNotify() if the protocol is present.
    //
    gPciPlatformProtocol->PlatformNotify (
                            gPciPlatformProtocol,
                            HostBridgeHandle,
                            Phase,
                            ChipsetEntry
                            );
  } else if (gPciOverrideProtocol != NULL){
    //
    // Get Host Bridge Handle.
    //
    PciResAlloc->GetNextRootBridge (PciResAlloc, &RootBridgeHandle);

    //
    // Get the rootbridge Io protocol to find the host bridge handle
    //
    Status = gBS->HandleProtocol (
                    RootBridgeHandle,
                    &gEfiPciRootBridgeIoProtocolGuid,
                    (VOID **) &PciRootBridgeIo
                    );

    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }

    HostBridgeHandle = PciRootBridgeIo->ParentHandle;

    //
    // Call PlatformPci::PhaseNotify() if the protocol is present.
    //
    gPciOverrideProtocol->PlatformNotify (
                            gPciOverrideProtocol,
                            HostBridgeHandle,
                            Phase,
                            ChipsetEntry
                            );
  }

  Status = PciResAlloc->NotifyPhase (
                          PciResAlloc,
                          Phase
                          );

  if (gPciPlatformProtocol != NULL) {
    //
    // Call PlatformPci::PlatformNotify() if the protocol is present.
    //
    gPciPlatformProtocol->PlatformNotify (
                            gPciPlatformProtocol,
                            HostBridgeHandle,
                            Phase,
                            ChipsetExit
                            );

  } else if (gPciOverrideProtocol != NULL) {
    //
    // Call PlatformPci::PhaseNotify() if the protocol is present.
    //
    gPciOverrideProtocol->PlatformNotify (
                            gPciOverrideProtocol,
                            HostBridgeHandle,
                            Phase,
                            ChipsetExit
                            );
  }

  return Status;
}

/**
  Provides the hooks from the PCI bus driver to every PCI controller (device/function) at various
  stages of the PCI enumeration process that allow the host bridge driver to preinitialize individual
  PCI controllers before enumeration.

  This function is called during the PCI enumeration process. No specific action is expected from this
  member function. It allows the host bridge driver to preinitialize individual PCI controllers before
  enumeration.

  @param Bridge            Pointer to the EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL instance.
  @param Bus               The bus number of the pci device.
  @param Device            The device number of the pci device.
  @param Func              The function number of the pci device.
  @param Phase             The phase of the PCI device enumeration.

  @retval EFI_SUCCESS              The requested parameters were returned.
  @retval EFI_INVALID_PARAMETER    RootBridgeHandle is not a valid root bridge handle.
  @retval EFI_INVALID_PARAMETER    Phase is not a valid phase that is defined in
                                   EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE.
  @retval EFI_DEVICE_ERROR         Programming failed due to a hardware error. The PCI enumerator should
                                   not enumerate this device, including its child devices if it is a PCI-to-PCI
                                   bridge.

**/
EFI_STATUS
PreprocessController (
  IN PCI_IO_DEVICE                                    *Bridge,
  IN UINT8                                            Bus,
  IN UINT8                                            Device,
  IN UINT8                                            Func,
  IN EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE     Phase
  )
{
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS       RootBridgePciAddress;
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *PciResAlloc;
  EFI_HANDLE                                        RootBridgeHandle;
  EFI_HANDLE                                        HostBridgeHandle;
  EFI_STATUS                                        Status;

  //
  // Get the host bridge handle
  //
  HostBridgeHandle = Bridge->PciRootBridgeIo->ParentHandle;

  //
  // Get the pci host bridge resource allocation protocol
  //
  Status = gBS->OpenProtocol (
                  HostBridgeHandle,
                  &gEfiPciHostBridgeResourceAllocationProtocolGuid,
                  (VOID **) &PciResAlloc,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Get Root Brige Handle
  //
  while (Bridge->Parent != NULL) {
    Bridge = Bridge->Parent;
  }

  RootBridgeHandle                      = Bridge->Handle;

  RootBridgePciAddress.Register         = 0;
  RootBridgePciAddress.Function         = Func;
  RootBridgePciAddress.Device           = Device;
  RootBridgePciAddress.Bus              = Bus;
  RootBridgePciAddress.ExtendedRegister = 0;

  if (gPciPlatformProtocol != NULL) {
    //
    // Call PlatformPci::PrepController() if the protocol is present.
    //
    gPciPlatformProtocol->PlatformPrepController (
                            gPciPlatformProtocol,
                            HostBridgeHandle,
                            RootBridgeHandle,
                            RootBridgePciAddress,
                            Phase,
                            ChipsetEntry
                            );
  } else if (gPciOverrideProtocol != NULL) {
    //
    // Call PlatformPci::PrepController() if the protocol is present.
    //
    gPciOverrideProtocol->PlatformPrepController (
                            gPciOverrideProtocol,
                            HostBridgeHandle,
                            RootBridgeHandle,
                            RootBridgePciAddress,
                            Phase,
                            ChipsetEntry
                            );
  }

  Status = PciResAlloc->PreprocessController (
                          PciResAlloc,
                          RootBridgeHandle,
                          RootBridgePciAddress,
                          Phase
                          );

  if (gPciPlatformProtocol != NULL) {
    //
    // Call PlatformPci::PrepController() if the protocol is present.
    //
    gPciPlatformProtocol->PlatformPrepController (
                            gPciPlatformProtocol,
                            HostBridgeHandle,
                            RootBridgeHandle,
                            RootBridgePciAddress,
                            Phase,
                            ChipsetExit
                            );
  } else if (gPciOverrideProtocol != NULL) {
    //
    // Call PlatformPci::PrepController() if the protocol is present.
    //
    gPciOverrideProtocol->PlatformPrepController (
                            gPciOverrideProtocol,
                            HostBridgeHandle,
                            RootBridgeHandle,
                            RootBridgePciAddress,
                            Phase,
                            ChipsetExit
                            );
  }

  return EFI_SUCCESS;
}

/**
  This function allows the PCI bus driver to be notified to act as requested when a hot-plug event has
  happened on the hot-plug controller. Currently, the operations include add operation and remove operation..

  @param This                 A pointer to the hot plug request protocol.
  @param Operation            The operation the PCI bus driver is requested to make.
  @param Controller           The handle of the hot-plug controller.
  @param RemainingDevicePath  The remaining device path for the PCI-like hot-plug device.
  @param NumberOfChildren     The number of child handles.
                              For a add operation, it is an output parameter.
                              For a remove operation, it's an input parameter.
  @param ChildHandleBuffer    The buffer which contains the child handles.

  @retval EFI_INVALID_PARAMETER  Operation is not a legal value.
                                 Controller is NULL or not a valid handle.
                                 NumberOfChildren is NULL.
                                 ChildHandleBuffer is NULL while Operation is add.
  @retval EFI_OUT_OF_RESOURCES   There are no enough resources to start the devices.
  @retval EFI_NOT_FOUND          Can not find bridge according to controller handle.
  @retval EFI_SUCCESS            The handles for the specified device have been created or destroyed
                                 as requested, and for an add operation, the new handles are
                                 returned in ChildHandleBuffer.
**/
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
{
  PCI_IO_DEVICE       *Bridge;
  PCI_IO_DEVICE       *Temp;
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINTN               Index;
  EFI_HANDLE          RootBridgeHandle;
  EFI_STATUS          Status;

  //
  // Check input parameter validity
  //
  if ((Controller == NULL) || (NumberOfChildren == NULL)){
    return EFI_INVALID_PARAMETER;
  }

  if ((Operation != EfiPciHotPlugRequestAdd) && (Operation != EfiPciHotplugRequestRemove)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Operation == EfiPciHotPlugRequestAdd){
    if (ChildHandleBuffer == NULL) {
      return EFI_INVALID_PARAMETER;
    }
  } else if ((Operation == EfiPciHotplugRequestRemove) && (*NumberOfChildren != 0)) {
    if (ChildHandleBuffer == NULL) {
      return EFI_INVALID_PARAMETER;
    }
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  gPciBusDriverBinding.DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Bridge = PCI_IO_DEVICE_FROM_PCI_IO_THIS (PciIo);

  //
  // Get root bridge handle
  //
  Temp = Bridge;
  while (Temp->Parent != NULL) {
    Temp = Temp->Parent;
  }

  RootBridgeHandle = Temp->Handle;

  if (Operation == EfiPciHotPlugRequestAdd) {
    //
    // Report Status Code to indicate hot plug happens
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_PROGRESS_CODE,
      (EFI_IO_BUS_PCI | EFI_IOB_PC_HOTPLUG),
      Temp->DevicePath
      );

    if (NumberOfChildren != NULL) {
      *NumberOfChildren = 0;
    }

    if (IsListEmpty (&Bridge->ChildList)) {

      Status = PciBridgeEnumerator (Bridge);

      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    Status = StartPciDevicesOnBridge (
              RootBridgeHandle,
              Bridge,
              RemainingDevicePath,
              NumberOfChildren,
              ChildHandleBuffer
              );

    return Status;
  }

  if (Operation == EfiPciHotplugRequestRemove) {

    if (*NumberOfChildren == 0) {
      //
      // Remove all devices on the bridge
      //
      RemoveAllPciDeviceOnBridge (RootBridgeHandle, Bridge);
      return EFI_SUCCESS;

    }

    for (Index = 0; Index < *NumberOfChildren; Index++) {
      //
      // De register all the pci device
      //
      Status = DeRegisterPciDevice (RootBridgeHandle, ChildHandleBuffer[Index]);

      if (EFI_ERROR (Status)) {
        return Status;
      }

    }
    //
    // End for
    //
    return EFI_SUCCESS;
  }

  return EFI_SUCCESS;
}

/**
  Search hostbridge according to given handle

  @param RootBridgeHandle  Host bridge handle.

  @retval TRUE             Found host bridge handle.
  @retval FALSE            Not found hot bridge handle.

**/
BOOLEAN
SearchHostBridgeHandle (
  IN EFI_HANDLE RootBridgeHandle
  )
{
  EFI_HANDLE                      HostBridgeHandle;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  UINTN                           Index;
  EFI_STATUS                      Status;

  //
  // Get the rootbridge Io protocol to find the host bridge handle
  //
  Status = gBS->OpenProtocol (
                  RootBridgeHandle,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  (VOID **) &PciRootBridgeIo,
                  gPciBusDriverBinding.DriverBindingHandle,
                  RootBridgeHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  HostBridgeHandle = PciRootBridgeIo->ParentHandle;
  for (Index = 0; Index < gPciHostBridgeNumber; Index++) {
    if (HostBridgeHandle == gPciHostBrigeHandles[Index]) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Add host bridge handle to global variable for enumerating.

  @param HostBridgeHandle   Host bridge handle.

  @retval EFI_SUCCESS       Successfully added host bridge.
  @retval EFI_ABORTED       Host bridge is NULL, or given host bridge
                            has been in host bridge list.

**/
EFI_STATUS
AddHostBridgeEnumerator (
  IN EFI_HANDLE HostBridgeHandle
  )
{
  UINTN Index;

  if (HostBridgeHandle == NULL) {
    return EFI_ABORTED;
  }

  for (Index = 0; Index < gPciHostBridgeNumber; Index++) {
    if (HostBridgeHandle == gPciHostBrigeHandles[Index]) {
      return EFI_ABORTED;
    }
  }

  if (Index < PCI_MAX_HOST_BRIDGE_NUM) {
    gPciHostBrigeHandles[Index] = HostBridgeHandle;
    gPciHostBridgeNumber++;
  }

  return EFI_SUCCESS;
}


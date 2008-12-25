/** @file

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#include "PciBus.h"
#include "PciEnumerator.h"
#include "PciResourceSupport.h"
#include "PciOptionRomSupport.h"

/**
  This routine is used to enumerate entire pci bus system
  in a given platform.

  @param Controller  Parent controller handle.
  
  @return Status of enumerating.
**/
EFI_STATUS
PciEnumerator (
  IN EFI_HANDLE                    Controller
  )
{

  EFI_HANDLE                                        HostBridgeHandle;
  EFI_STATUS                                        Status;
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *PciResAlloc;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL                   *PciRootBridgeIo;

  //
  // If PCI bus has already done the full enumeration, never do it again
  //
  if (!gFullEnumeration) {
    return PciEnumeratorLight (Controller);
  }

  //
  // Get the rootbridge Io protocol to find the host bridge handle
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  (VOID **) &PciRootBridgeIo,
                  gPciBusDriverBinding.DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the host bridge handle
  //
  HostBridgeHandle = PciRootBridgeIo->ParentHandle;

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
  NotifyPhase (PciResAlloc, EfiPciHostBridgeBeginEnumeration);

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

  gFullEnumeration = FALSE;

  return EFI_SUCCESS;
}

/**
  Enumerate PCI root bridge
  
  @param PciResAlloc   Pointer to protocol instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.
  @param RootBridgeDev Instance of root bridge device.
  
  @retval EFI_SUCCESS  Success to enumerate root bridge.
  @retval Others       Fail to enumerate root bridge.
  
**/
EFI_STATUS
PciRootBridgeEnumerator (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *PciResAlloc,
  IN PCI_IO_DEVICE                                     *RootBridgeDev
  )
{
  EFI_STATUS                        Status;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *pConfiguration;
  UINT8                             SubBusNumber;
  UINT8                             StartBusNumber;
  UINT8                             PaddedBusRange;
  EFI_HANDLE                        RootBridgeHandle;

  SubBusNumber    = 0;
  StartBusNumber  = 0;
  PaddedBusRange  = 0;

  //
  // Get the root bridge handle
  //
  RootBridgeHandle = RootBridgeDev->Handle;

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_IO_BUS_PCI | EFI_IOB_PCI_PC_BUS_ENUM,
    RootBridgeDev->DevicePath
    );

  //
  // Get the Bus information
  //
  Status = PciResAlloc->StartBusEnumeration (
                          PciResAlloc,
                          RootBridgeHandle,
                          (VOID **) &pConfiguration
                          );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the bus number to start with
  //
  StartBusNumber = (UINT8) (pConfiguration->AddrRangeMin);
  PaddedBusRange  = (UINT8) (pConfiguration->AddrRangeMax);

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
            (UINT8) (pConfiguration->AddrRangeMin),
            &SubBusNumber,
            &PaddedBusRange
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }


  //
  // Assign max bus number scanned
  //
  pConfiguration->AddrLen = SubBusNumber - StartBusNumber + 1 + PaddedBusRange;

  //
  // Set bus number
  //
  Status = PciResAlloc->SetBusNumbers (
                          PciResAlloc,
                          RootBridgeHandle,
                          pConfiguration
                          );
  
  gBS->FreePool (pConfiguration);
  
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  This routine is used to process option rom on a certain root bridge
  
  @param Bridge     Given parent's root bridge
  @param RomBase    Base address of ROM driver loaded from
  @param MaxLength  Max rom size
  
  @retval EFI_SUCCESS Success to process option rom image.
**/
EFI_STATUS
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

  return EFI_SUCCESS;
}

/**
  This routine is used to assign bus number to the given PCI bus system
  
  @param Bridge           Parent root bridge instance.
  @param StartBusNumber   Number of beginning.
  @param SubBusNumber     the number of sub bus.
  
  @retval EFI_SUCCESS  Success to assign bus number.
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

      if (!EFI_ERROR (Status)   &&
          (IS_PCI_BRIDGE (&Pci) || IS_CARDBUS_BRIDGE (&Pci))) {

        //
        // Reserved one bus for cardbus bridge
        //
        SecondBus = ++(*SubBusNumber);

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

          Register8 = 0xFF;
          Status = PciRootBridgeIoWrite (
                                          PciRootBridgeIo,
                                          &Pci,
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
  This routine is used to determine the root bridge attribute by interfacing
  the host bridge resource allocation protocol.

  @param PciResAlloc    Protocol instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
  @param RootBridgeDev  Root bridge instance
  
  @retval EFI_SUCCESS  Success to get root bridge's attribute
  @retval Others       Fail to get attribute
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
    RootBridgeDev->Decodes |= EFI_BRIDGE_PMEM64_DECODE_SUPPORTED;
  }

  RootBridgeDev->Decodes |= EFI_BRIDGE_MEM32_DECODE_SUPPORTED;
  RootBridgeDev->Decodes |= EFI_BRIDGE_PMEM32_DECODE_SUPPORTED;
  RootBridgeDev->Decodes |= EFI_BRIDGE_IO16_DECODE_SUPPORTED;

  return EFI_SUCCESS;
}

/**
  Get Max Option Rom size on this bridge
  
  @param Bridge  Bridge device instance.
  @return Max size of option rom.
**/
UINT64
GetMaxOptionRomSize (
  IN PCI_IO_DEVICE   *Bridge
  )
{
  LIST_ENTRY      *CurrentLink;
  PCI_IO_DEVICE   *Temp;
  UINT64          MaxOptionRomSize;
  UINT64          TempOptionRomSize;

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
  
  @retval EFI_NOT_FOUND Can not find the specific root bridge device.
  @retval EFI_SUCCESS   Success Process attribute.
  @retval Others        Can not determine the root bridge device's attribute.
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
  Get resource allocation status from the ACPI pointer

  @param AcpiConfig       Point to Acpi configuration table
  @param IoResStatus      Return the status of I/O resource
  @param Mem32ResStatus   Return the status of 32-bit Memory resource
  @param PMem32ResStatus  Return the status of 32-bit PMemory resource
  @param Mem64ResStatus   Return the status of 64-bit Memory resource
  @param PMem64ResStatus  Return the status of 64-bit PMemory resource
  
  @retval EFI_SUCCESS Success to get resource allocation status from ACPI configuration table.
**/
EFI_STATUS
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
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *ptr;

  Temp = (UINT8 *) AcpiConfig;

  while (*Temp == ACPI_ADDRESS_SPACE_DESCRIPTOR) {

    ptr       = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp;
    ResStatus = ptr->AddrTranslationOffset;

    switch (ptr->ResType) {
    case 0:
      if (ptr->AddrSpaceGranularity == 32) {
        if (ptr->SpecificFlag == 0x06) {
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

      if (ptr->AddrSpaceGranularity == 64) {
        if (ptr->SpecificFlag == 0x06) {
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

  return EFI_SUCCESS;
}

/**
  Remove a PCI device from device pool and mark its bar
  
  @param PciDevice Instance of Pci device.
  
  @retval EFI_SUCCESS Success Operation.
  @retval EFI_ABORTED Pci device is a root bridge.
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
  if ( IS_PCI_BRIDGE(&PciDevice->Pci) && \
       PciDevice->ResourcePaddingDescriptors != NULL ) {
    gBS->FreePool (PciDevice->ResourcePaddingDescriptors);
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
      FreePciDevice (Temp);
      return EFI_SUCCESS;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return EFI_ABORTED;
}

/**
  Determine whethter a PCI device can be rejected.
  
  @param PciResNode Pointer to Pci resource node instance.
  
  @return whethter a PCI device can be rejected.
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
  Compare two resource node and get the larger resource consumer
  
  @param PciResNode1  resource node 1 want to be compared
  @param PciResNode2  resource node 2 want to be compared
  
  @return Larger resource consumer.
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
  
  @return the max resource consumer in the host resource pool.
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
  @param PMem32Pool       Pointer to instance of 32-bit Pmemory resource node.
  @param Mem64Pool        Pointer to instance of 64-bit memory resource node.
  @param PMem64Pool       Pointer to instance of 64-bit Pmemory resource node.
  @param IoResStatus      Status of I/O resource Node.
  @param Mem32ResStatus   Status of 32-bit memory resource Node.
  @param PMem32ResStatus  Status of 32-bit Pmemory resource node.
  @param Mem64ResStatus   Status of 64-bit memory resource node.
  @param PMem64ResStatus  Status of 64-bit Pmemory resource node.
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

    if (ResStatus[ResType] == EFI_RESOURCE_NONEXISTENT) {
      //
      // Hostbridge hasn't this resource type
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

      //
      // Raise the EFI_IOB_EC_RESOURCE_CONFLICT status code
      //
      //
      // Have no way to get ReqRes, AllocRes & Bar here
      //
      ZeroMem (&AllocFailExtendedData, sizeof (AllocFailExtendedData));
      AllocFailExtendedData.DevicePathSize = sizeof (EFI_DEVICE_PATH_PROTOCOL);
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
  Summary requests for all resource type, and contruct ACPI resource
  requestor instance.
  
  @param Bridge           detecting bridge
  @param IoNode           Pointer to instance of I/O resource Node
  @param Mem32Node        Pointer to instance of 32-bit memory resource Node
  @param PMem32Node       Pointer to instance of 32-bit Pmemory resource node
  @param Mem64Node        Pointer to instance of 64-bit memory resource node
  @param PMem64Node       Pointer to instance of 64-bit Pmemory resource node
  @param pConfig          outof buffer holding new constructed APCI resource requestor
**/
EFI_STATUS
ConstructAcpiResourceRequestor (
  IN PCI_IO_DEVICE      *Bridge,
  IN PCI_RESOURCE_NODE  *IoNode,
  IN PCI_RESOURCE_NODE  *Mem32Node,
  IN PCI_RESOURCE_NODE  *PMem32Node,
  IN PCI_RESOURCE_NODE  *Mem64Node,
  IN PCI_RESOURCE_NODE  *PMem64Node,
  OUT VOID              **pConfig
  )
{
  UINT8                             NumConfig;
  UINT8                             Aperture;
  UINT8                             *Configuration;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Ptr;
  EFI_ACPI_END_TAG_DESCRIPTOR       *PtrEnd;

  NumConfig = 0;
  Aperture  = 0;

  *pConfig  = NULL;

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
    Configuration = AllocatePool (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) * NumConfig + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
    if (Configuration == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (
      Configuration,
      sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) * NumConfig + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR)
      );

    Ptr = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;

    //
    // Deal with io aperture
    //
    if ((Aperture & 0x01) != 0) {
      Ptr->Desc     = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Ptr->Len      = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
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

      Ptr               = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) ((UINT8 *) Ptr + sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR));
    }
    //
    // Deal with mem32 aperture
    //
    if ((Aperture & 0x02) != 0) {
      Ptr->Desc     = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Ptr->Len      = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
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

      Ptr               = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) ((UINT8 *) Ptr + sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR));
    }

    //
    // Deal with Pmem32 aperture
    //
    if ((Aperture & 0x04) != 0) {
      Ptr->Desc     = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Ptr->Len      = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
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

      Ptr               = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) ((UINT8 *) Ptr + sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR));
    }
    //
    // Deal with mem64 aperture
    //
    if ((Aperture & 0x08) != 0) {
      Ptr->Desc     = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Ptr->Len      = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
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

      Ptr               = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) ((UINT8 *) Ptr + sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR));
    }
    //
    // Deal with Pmem64 aperture
    //
    if ((Aperture & 0x10) != 0) {
      Ptr->Desc     = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Ptr->Len      = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
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

      Ptr               = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) ((UINT8 *) Ptr + sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR));
    }

    //
    // put the checksum
    //
    PtrEnd            = (EFI_ACPI_END_TAG_DESCRIPTOR *) ((UINT8 *) Ptr);

    PtrEnd->Desc      = ACPI_END_TAG_DESCRIPTOR;
    PtrEnd->Checksum  = 0;

  } else {

    //
    // If there is no resource request
    //
    Configuration = AllocatePool (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
    if (Configuration == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (Configuration, sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));

    Ptr               = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) (Configuration);
    Ptr->Desc         = ACPI_ADDRESS_SPACE_DESCRIPTOR;

    PtrEnd            = (EFI_ACPI_END_TAG_DESCRIPTOR *) (Configuration + sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR));
    PtrEnd->Desc      = ACPI_END_TAG_DESCRIPTOR;
    PtrEnd->Checksum  = 0;
  }

  *pConfig = Configuration;

  return EFI_SUCCESS;
}

/**
  Get resource base from a acpi configuration descriptor.
  
  @param pConfig      an acpi configuration descriptor.
  @param IoBase       output of I/O resource base address.
  @param Mem32Base    output of 32-bit memory base address.
  @param PMem32Base   output of 32-bit pmemory base address.
  @param Mem64Base    output of 64-bit memory base address.
  @param PMem64Base   output of 64-bit pmemory base address.
  
  @return EFI_SUCCESS  Success operation.
**/
EFI_STATUS
GetResourceBase (
  IN VOID     *pConfig,
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

  *IoBase     = 0xFFFFFFFFFFFFFFFFULL;
  *Mem32Base  = 0xFFFFFFFFFFFFFFFFULL;
  *PMem32Base = 0xFFFFFFFFFFFFFFFFULL;
  *Mem64Base  = 0xFFFFFFFFFFFFFFFFULL;
  *PMem64Base = 0xFFFFFFFFFFFFFFFFULL;

  Temp        = (UINT8 *) pConfig;

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
          if (Ptr->SpecificFlag & 0x06) {
            *PMem32Base = Ptr->AddrRangeMin;
          } else {
            *Mem32Base = Ptr->AddrRangeMin;
          }
        }

        if (Ptr->AddrSpaceGranularity == 64) {
          if (Ptr->SpecificFlag & 0x06) {
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

  return EFI_SUCCESS;
}

/**
  Enumerate pci bridge, allocate resource and determine attribute
  for devices on this bridge
  
  @param BridgeDev Pointer to instance of bridge device.
  
  @retval EFI_SUCCESS Success operation.
  @retval Others      Fail to enumerate.
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
  Status          = PciIoRead (PciIo, EfiPciIoWidthUint8, 0x19, 1, &StartBusNumber);

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
  Allocate all kinds of resource for bridge
  
  @param Bridge      Pointer to bridge instance.
  
  @retval EFI_SUCCESS Success operation.
  @retval Others      Fail to allocate resource for bridge.
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
              0xFFF,
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
  Status = CreateResourceMap (
            Bridge,
            IoBridge,
            Mem32Bridge,
            PMem32Bridge,
            Mem64Bridge,
            PMem64Bridge
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }

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
  Get resource base address for a pci bridge device
  
  @param Bridge  Given Pci driver instance.
  @param IoBase  output for base address of I/O type resource.
  @param Mem32Base  output for base address of 32-bit memory type resource.
  @param PMem32Base  output for base address of 32-bit Pmemory type resource.
  @param Mem64Base  output for base address of 64-bit memory type resource.
  @param PMem64Base  output for base address of 64-bit Pmemory type resource.
  
  @retval EFI_SUCCESS Succes to get resource base address.
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
   EfiPciHostBridgeBeginEnumeration     - Resets the host bridge PCI apertures and internal data
                                          structures. The PCI enumerator should issue this notification
                                          before starting a fresh enumeration process. Enumeration cannot
                                          be restarted after sending any other notification such as
                                          EfiPciHostBridgeBeginBusAllocation.
   EfiPciHostBridgeBeginBusAllocation   - The bus allocation phase is about to begin. No specific action is
                                          required here. This notification can be used to perform any
                                          chipset-specific programming.
   EfiPciHostBridgeEndBusAllocation     - The bus allocation and bus programming phase is complete. No
                                          specific action is required here. This notification can be used to
                                          perform any chipset-specific programming.
   EfiPciHostBridgeBeginResourceAllocation -  The resource allocation phase is about to begin. No specific
                                              action is required here. This notification can be used to perform
                                              any chipset-specific programming.
   EfiPciHostBridgeAllocateResources    - Allocates resources per previously submitted requests for all the PCI
                                          root bridges. These resource settings are returned on the next call to
                                          GetProposedResources(). Before calling NotifyPhase() with a Phase of
                                          EfiPciHostBridgeAllocateResource, the PCI bus enumerator is responsible for gathering I/O and memory requests for
                                          all the PCI root bridges and submitting these requests using
                                          SubmitResources(). This function pads the resource amount
                                          to suit the root bridge hardware, takes care of dependencies between
                                          the PCI root bridges, and calls the Global Coherency Domain (GCD)
                                          with the allocation request. In the case of padding, the allocated range
                                          could be bigger than what was requested.
   EfiPciHostBridgeSetResources         - Programs the host bridge hardware to decode previously allocated
                                          resources (proposed resources) for all the PCI root bridges. After the
                                          hardware is programmed, reassigning resources will not be supported.
                                          The bus settings are not affected.
   EfiPciHostBridgeFreeResources        - Deallocates resources that were previously allocated for all the PCI
                                          root bridges and resets the I/O and memory apertures to their initial
                                          state. The bus settings are not affected. If the request to allocate
                                          resources fails, the PCI enumerator can use this notification to
                                          deallocate previous resources, adjust the requests, and retry
                                          allocation.
   EfiPciHostBridgeEndResourceAllocation- The resource allocation phase is completed. No specific action is
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
    // Call PlatformPci::PhaseNotify() if the protocol is present.
    //
    gPciPlatformProtocol->PhaseNotify (
                            gPciPlatformProtocol,
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
    // Call PlatformPci::PhaseNotify() if the protocol is present.
    //
    gPciPlatformProtocol->PhaseNotify (
                            gPciPlatformProtocol,
                            HostBridgeHandle,
                            Phase,
                            ChipsetExit
                            );

  }

  return EFI_SUCCESS;
}

/**
   Provides the hooks from the PCI bus driver to every PCI controller (device/function) at various
   stages of the PCI enumeration process that allow the host bridge driver to preinitialize individual
   PCI controllers before enumeration.

   This function is called during the PCI enumeration process. No specific action is expected from this
   member function. It allows the host bridge driver to preinitialize individual PCI controllers before
   enumeration.

   @param This              Pointer to the EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL instance.
   @param RootBridgeHandle  The associated PCI root bridge handle. Type EFI_HANDLE is defined in
                            InstallProtocolInterface() in the UEFI 2.0 Specification.
   @param PciAddress        The address of the PCI device on the PCI bus. This address can be passed to the
                            EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL member functions to access the PCI
                            configuration space of the device. See Table 12-1 in the UEFI 2.0 Specification for
                            the definition of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS.
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
  }

  return EFI_SUCCESS;
}

/**
  Hot plug request notify.
  
  @param This                 - A pointer to the hot plug request protocol.
  @param Operation            - The operation.
  @param Controller           - A pointer to the controller.
  @param RemainningDevicePath - A pointer to the device path.
  @param NumberOfChildren     - A the number of child handle in the ChildHandleBuffer.
  @param ChildHandleBuffer    - A pointer to the array contain the child handle.
  
  @retval EFI_NOT_FOUND Can not find bridge according to controller handle.
  @retval EFI_SUCCESS   Success operating.
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

    return EFI_SUCCESS;
  }

  if (Operation == EfiPciHotplugRequestRemove) {

    if (*NumberOfChildren == 0) {
      //
      // Remove all devices on the bridge
      //
      Status = RemoveAllPciDeviceOnBridge (RootBridgeHandle, Bridge);
      return Status;

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
  
  @return whether found
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
  Add host bridge handle to global variable for enumating.
  
  @param HostBridgeHandle host bridge handle.
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


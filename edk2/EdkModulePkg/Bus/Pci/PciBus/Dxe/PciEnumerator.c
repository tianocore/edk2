/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PciEnumerator.c

Abstract:

  PCI Bus Driver

Revision History

--*/

#include "pcibus.h"
#include "PciEnumerator.h"
#include "PciResourceSupport.h"
#include "PciOptionRomSupport.h"

EFI_STATUS
PciEnumerator (
  IN EFI_HANDLE                    Controller
  )
/*++

Routine Description:

  This routine is used to enumerate entire pci bus system
  in a given platform

Arguments:

Returns:

  None

--*/
// TODO:    Controller - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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
  // If this host bridge has been already enumerated, then return successfully
  //
  if (RootBridgeExisted (Controller)) {
    return EFI_SUCCESS;
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

EFI_STATUS
PciRootBridgeEnumerator (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *PciResAlloc,
  IN PCI_IO_DEVICE                                     *RootBridgeDev
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    PciResAlloc - add argument and description to function comment
// TODO:    RootBridgeDev - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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

  //
  // Initialize the subordinate bus number
  //
  SubBusNumber = StartBusNumber;

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

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
ProcessOptionRom (
  IN PCI_IO_DEVICE *Bridge,
  IN UINT64        RomBase,
  IN UINT64        MaxLength
  )
/*++

Routine Description:

  This routine is used to process option rom on a certain root bridge

Arguments:

Returns:

  None

--*/
// TODO:    Bridge - add argument and description to function comment
// TODO:    RomBase - add argument and description to function comment
// TODO:    MaxLength - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  LIST_ENTRY      *CurrentLink;
  PCI_IO_DEVICE   *Temp;
  EFI_STATUS      Status;

  //
  // Go through bridges to reach all devices
  //
  CurrentLink = Bridge->ChildList.ForwardLink;
  while (CurrentLink && CurrentLink != &Bridge->ChildList) {
    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);
    if (!IsListEmpty (&Temp->ChildList)) {

      //
      // Go further to process the option rom under this bridge
      //
      Status = ProcessOptionRom (Temp, RomBase, MaxLength);
    }

    if (Temp->RomSize != 0 && Temp->RomSize <= MaxLength) {

      //
      // Load and process the option rom
      //
      Status = LoadOpRomImage (Temp, RomBase);
      if (Status == EFI_SUCCESS) {
        Status = ProcessOpRomImage (Temp);
      }
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PciAssignBusNumber (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber
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

EFI_STATUS
DetermineRootBridgeAttributes (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc,
  IN PCI_IO_DEVICE                                    *RootBridgeDev
  )
/*++

Routine Description:

  This routine is used to determine the root bridge attribute by interfacing
  the host bridge resource allocation protocol.

Arguments:

Returns:

  None

--*/
// TODO:    PciResAlloc - add argument and description to function comment
// TODO:    RootBridgeDev - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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

  if (Attributes & EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM) {
    RootBridgeDev->Decodes |= EFI_BRIDGE_PMEM_MEM_COMBINE_SUPPORTED;
  }

  if (Attributes & EFI_PCI_HOST_BRIDGE_MEM64_DECODE) {
    RootBridgeDev->Decodes |= EFI_BRIDGE_PMEM64_DECODE_SUPPORTED;
  }

  RootBridgeDev->Decodes |= EFI_BRIDGE_MEM32_DECODE_SUPPORTED;
  RootBridgeDev->Decodes |= EFI_BRIDGE_PMEM32_DECODE_SUPPORTED;
  RootBridgeDev->Decodes |= EFI_BRIDGE_IO16_DECODE_SUPPORTED;

  return EFI_SUCCESS;
}

UINT64
GetMaxOptionRomSize (
  IN PCI_IO_DEVICE   *Bridge
  )
/*++

Routine Description:

    Get Max Option Rom size on this bridge

Arguments:

Returns:

  None

--*/
// TODO:    Bridge - add argument and description to function comment
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
  while (CurrentLink && CurrentLink != &Bridge->ChildList) {
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

EFI_STATUS
PciHostBridgeDeviceAttribute (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  )
/*++

Routine Description:

    Process attributes of devices on this host bridge

Arguments:

Returns:

  None

--*/
// TODO:    PciResAlloc - add argument and description to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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

EFI_STATUS
GetResourceAllocationStatus (
  VOID        *AcpiConfig,
  OUT UINT64  *IoResStatus,
  OUT UINT64  *Mem32ResStatus,
  OUT UINT64  *PMem32ResStatus,
  OUT UINT64  *Mem64ResStatus,
  OUT UINT64  *PMem64ResStatus
  )
/*++

Routine Description:

    Get resource allocation status from the ACPI pointer

Arguments:

Returns:

  None

--*/
// TODO:    AcpiConfig - add argument and description to function comment
// TODO:    IoResStatus - add argument and description to function comment
// TODO:    Mem32ResStatus - add argument and description to function comment
// TODO:    PMem32ResStatus - add argument and description to function comment
// TODO:    Mem64ResStatus - add argument and description to function comment
// TODO:    PMem64ResStatus - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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

EFI_STATUS
RejectPciDevice (
  IN PCI_IO_DEVICE       *PciDevice
  )
/*++

Routine Description:

    Remove a PCI device from device pool and mark its bar

Arguments:

Returns:

  None

--*/
// TODO:    PciDevice - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_ABORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_ABORTED - add return value to function comment
{
  PCI_IO_DEVICE   *Bridge;
  PCI_IO_DEVICE   *Temp;
  LIST_ENTRY      *CurrentLink;

  //
  // Remove the padding resource from a bridge
  //
  if ( IS_PCI_BRIDGE(&PciDevice->Pci) && \
       PciDevice->ResourcePaddingDescriptors ) {
    gBS->FreePool (PciDevice->ResourcePaddingDescriptors);
    PciDevice->ResourcePaddingDescriptors = NULL;
    return EFI_SUCCESS;
  }

  //
  // Skip RB and PPB
  //
  if (IS_PCI_BRIDGE (&PciDevice->Pci) || (!PciDevice->Parent)) {
    return EFI_ABORTED;
  }

  if (IS_CARDBUS_BRIDGE (&PciDevice->Pci)) {
    //
    // Get the root bridge device
    //
    Bridge = PciDevice;
    while (Bridge->Parent) {
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
  while (CurrentLink && CurrentLink != &Bridge->ChildList) {
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

BOOLEAN
IsRejectiveDevice (
  IN  PCI_RESOURCE_NODE   *PciResNode
  )
/*++

Routine Description:

    Determine whethter a PCI device can be rejected

Arguments:

Returns:

  None

--*/
// TODO:    PciResNode - add argument and description to function comment
{
  PCI_IO_DEVICE *Temp;

  Temp = PciResNode->PciDev;

  //
  // Ensure the device is present
  //
  if (!Temp) {
    return FALSE;
  }

  //
  // PPB and RB should go ahead
  //
  if (IS_PCI_BRIDGE (&Temp->Pci) || (!Temp->Parent)) {
    return TRUE;
  }

  //
  // Skip device on Bus0
  //
  if ((Temp->Parent) && (Temp->BusNumber == 0)) {
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

PCI_RESOURCE_NODE *
GetLargerConsumerDevice (
  IN  PCI_RESOURCE_NODE   *PciResNode1,
  IN  PCI_RESOURCE_NODE   *PciResNode2
  )
/*++

Routine Description:

    Get the larger resource consumer

Arguments:

Returns:

  None

--*/
// TODO:    PciResNode1 - add argument and description to function comment
// TODO:    PciResNode2 - add argument and description to function comment
{
  if (!PciResNode2) {
    return PciResNode1;
  }

  if ((IS_PCI_BRIDGE(&(PciResNode2->PciDev->Pci)) || !(PciResNode2->PciDev->Parent)) \
       && (PciResNode2->ResourceUsage != PciResUsagePadding) )
  {
    return PciResNode1;
  }

  if (!PciResNode1) {
    return PciResNode2;
  }

  if ((PciResNode1->Length) > (PciResNode2->Length)) {
    return PciResNode1;
  }

  return PciResNode2;

}

PCI_RESOURCE_NODE *
GetMaxResourceConsumerDevice (
  IN  PCI_RESOURCE_NODE   *ResPool
  )
/*++

Routine Description:

    Get the max resource consumer in the host resource pool

Arguments:

Returns:

  None

--*/
// TODO:    ResPool - add argument and description to function comment
{
  PCI_RESOURCE_NODE *Temp;
  LIST_ENTRY        *CurrentLink;
  PCI_RESOURCE_NODE *PciResNode;
  PCI_RESOURCE_NODE *PPBResNode;

  PciResNode  = NULL;

  CurrentLink = ResPool->ChildList.ForwardLink;
  while (CurrentLink && CurrentLink != &ResPool->ChildList) {

    Temp = RESOURCE_NODE_FROM_LINK (CurrentLink);

    if (!IsRejectiveDevice (Temp)) {
      CurrentLink = CurrentLink->ForwardLink;
      continue;
    }

    if ((IS_PCI_BRIDGE (&(Temp->PciDev->Pci)) || (!Temp->PciDev->Parent)) \
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
/*++

Routine Description:

    Adjust host bridge allocation so as to reduce resource requirement

Arguments:

Returns:

  None

--*/
// TODO:    IoPool - add argument and description to function comment
// TODO:    Mem32Pool - add argument and description to function comment
// TODO:    PMem32Pool - add argument and description to function comment
// TODO:    Mem64Pool - add argument and description to function comment
// TODO:    PMem64Pool - add argument and description to function comment
// TODO:    IoResStatus - add argument and description to function comment
// TODO:    Mem32ResStatus - add argument and description to function comment
// TODO:    PMem32ResStatus - add argument and description to function comment
// TODO:    Mem64ResStatus - add argument and description to function comment
// TODO:    PMem64ResStatus - add argument and description to function comment
// TODO:    EFI_ABORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_ABORTED - add return value to function comment
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
  REPORT_STATUS_CODE_LIBRARY_RESOURCE_ALLOC_FAILURE_ERROR_DATA AllocFailExtendedData;

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
    if (!PciResNode) {
      continue;
    }

    //
    // Check if the device has been removed before
    //
    for (DevIndex = 0; DevIndex < RemovedPciDevNum; DevIndex++) {
      if (PciResNode->PciDev == RemovedPciDev[DevIndex]) {
        continue;
      }
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
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    Bridge - add argument and description to function comment
// TODO:    IoNode - add argument and description to function comment
// TODO:    Mem32Node - add argument and description to function comment
// TODO:    PMem32Node - add argument and description to function comment
// TODO:    Mem64Node - add argument and description to function comment
// TODO:    PMem64Node - add argument and description to function comment
// TODO:    pConfig - add argument and description to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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
    if (Aperture & 0x01) {
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
    if (Aperture & 0x02) {
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
    if (Aperture & 0x04) {
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
    if (Aperture & 0x08) {
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
    if (Aperture & 0x10) {
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

      Ptr               = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) (Configuration + sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR));
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

EFI_STATUS
GetResourceBase (
  IN VOID     *pConfig,
  OUT UINT64  *IoBase,
  OUT UINT64  *Mem32Base,
  OUT UINT64  *PMem32Base,
  OUT UINT64  *Mem64Base,
  OUT UINT64  *PMem64Base
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    pConfig - add argument and description to function comment
// TODO:    IoBase - add argument and description to function comment
// TODO:    Mem32Base - add argument and description to function comment
// TODO:    PMem32Base - add argument and description to function comment
// TODO:    Mem64Base - add argument and description to function comment
// TODO:    PMem64Base - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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

EFI_STATUS
PciBridgeEnumerator (
  IN PCI_IO_DEVICE                                     *BridgeDev
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    BridgeDev - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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

EFI_STATUS
PciBridgeResourceAllocator (
  IN PCI_IO_DEVICE  *Bridge
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    Bridge - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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

EFI_STATUS
GetResourceBaseFromBridge (
  IN  PCI_IO_DEVICE *Bridge,
  OUT UINT64        *IoBase,
  OUT UINT64        *Mem32Base,
  OUT UINT64        *PMem32Base,
  OUT UINT64        *Mem64Base,
  OUT UINT64        *PMem64Base
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    Bridge - add argument and description to function comment
// TODO:    IoBase - add argument and description to function comment
// TODO:    Mem32Base - add argument and description to function comment
// TODO:    PMem32Base - add argument and description to function comment
// TODO:    Mem64Base - add argument and description to function comment
// TODO:    PMem64Base - add argument and description to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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

    if (Bridge->PciBar[PPB_IO_RANGE].Length) {
      *IoBase = Bridge->PciBar[PPB_IO_RANGE].BaseAddress;
    }

    if (Bridge->PciBar[PPB_MEM32_RANGE].Length) {
      *Mem32Base = Bridge->PciBar[PPB_MEM32_RANGE].BaseAddress;
    }

    if (Bridge->PciBar[PPB_PMEM32_RANGE].Length) {
      *PMem32Base = Bridge->PciBar[PPB_PMEM32_RANGE].BaseAddress;
    }

    if (Bridge->PciBar[PPB_PMEM64_RANGE].Length) {
      *PMem64Base = Bridge->PciBar[PPB_PMEM64_RANGE].BaseAddress;
    } else {
      *PMem64Base = gAllOne;
    }

  }

  if (IS_CARDBUS_BRIDGE (&Bridge->Pci)) {
    if (Bridge->PciBar[P2C_IO_1].Length) {
      *IoBase = Bridge->PciBar[P2C_IO_1].BaseAddress;
    } else {
      if (Bridge->PciBar[P2C_IO_2].Length) {
        *IoBase = Bridge->PciBar[P2C_IO_2].BaseAddress;
      }
    }

    if (Bridge->PciBar[P2C_MEM_1].Length) {
      if (Bridge->PciBar[P2C_MEM_1].BarType == PciBarTypePMem32) {
        *PMem32Base = Bridge->PciBar[P2C_MEM_1].BaseAddress;
      }

      if (Bridge->PciBar[P2C_MEM_1].BarType == PciBarTypeMem32) {
        *Mem32Base = Bridge->PciBar[P2C_MEM_1].BaseAddress;
      }
    }

    if (Bridge->PciBar[P2C_MEM_2].Length) {
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

EFI_STATUS
NotifyPhase (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc,
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE       Phase
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    PciResAlloc - add argument and description to function comment
// TODO:    Phase - add argument and description to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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

EFI_STATUS
PreprocessController (
  IN PCI_IO_DEVICE                                    *Bridge,
  IN UINT8                                            Bus,
  IN UINT8                                            Device,
  IN UINT8                                            Func,
  IN EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE     Phase
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    Bridge - add argument and description to function comment
// TODO:    Bus - add argument and description to function comment
// TODO:    Device - add argument and description to function comment
// TODO:    Func - add argument and description to function comment
// TODO:    Phase - add argument and description to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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
  while (Bridge->Parent) {
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
/*++

Routine Description:

  Hot plug request notify.

Arguments:

  This                 - A pointer to the hot plug request protocol.
  Operation            - The operation.
  Controller           - A pointer to the controller.
  RemainningDevicePath - A pointer to the device path.
  NumberOfChildren     - A the number of child handle in the ChildHandleBuffer.
  ChildHandleBuffer    - A pointer to the array contain the child handle.

Returns:

  Status code.

--*/
// TODO:    RemainingDevicePath - add argument and description to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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
  while (Temp->Parent) {
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

BOOLEAN
SearchHostBridgeHandle (
  IN EFI_HANDLE RootBridgeHandle
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    RootBridgeHandle - add argument and description to function comment
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

EFI_STATUS
AddHostBridgeEnumerator (
  IN EFI_HANDLE HostBridgeHandle
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    HostBridgeHandle - add argument and description to function comment
// TODO:    EFI_ABORTED - add return value to function comment
// TODO:    EFI_ABORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  UINTN Index;

  if (!HostBridgeHandle) {
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

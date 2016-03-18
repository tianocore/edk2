/** @file
  PCI Hot Plug support functions implementation for PCI Bus module..

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PciBus.h"

EFI_PCI_HOT_PLUG_INIT_PROTOCOL  *gPciHotPlugInit = NULL;
EFI_HPC_LOCATION                *gPciRootHpcPool = NULL;
UINTN                           gPciRootHpcCount = 0;
ROOT_HPC_DATA                   *gPciRootHpcData = NULL;


/**
  Event notification function to set Hot Plug controller status.

  @param  Event                    The event that invoke this function.
  @param  Context                  The calling context, pointer to ROOT_HPC_DATA.

**/
VOID
EFIAPI
PciHPCInitialized (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  ROOT_HPC_DATA   *HpcData;

  HpcData               = (ROOT_HPC_DATA *) Context;
  HpcData->Initialized  = TRUE;
}

/**
  Compare two device pathes to check if they are exactly same.

  @param DevicePath1    A pointer to the first device path data structure.
  @param DevicePath2    A pointer to the second device path data structure.

  @retval TRUE    They are same.
  @retval FALSE   They are not same.

**/
BOOLEAN
EfiCompareDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL *DevicePath1,
  IN EFI_DEVICE_PATH_PROTOCOL *DevicePath2
  )
{
  UINTN Size1;
  UINTN Size2;

  Size1 = GetDevicePathSize (DevicePath1);
  Size2 = GetDevicePathSize (DevicePath2);

  if (Size1 != Size2) {
    return FALSE;
  }

  if (CompareMem (DevicePath1, DevicePath2, Size1) != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Check hot plug support and initialize root hot plug private data.

  If Hot Plug is supported by the platform, call PCI Hot Plug Init protocol
  to get PCI Hot Plug controller's information and constructor the root hot plug
  private data structure.

  @retval EFI_SUCCESS           They are same.
  @retval EFI_UNSUPPORTED       No PCI Hot Plug controler on the platform.
  @retval EFI_OUT_OF_RESOURCES  No memory to constructor root hot plug private
                                data structure.

**/
EFI_STATUS
InitializeHotPlugSupport (
  VOID
  )
{
  EFI_STATUS        Status;
  EFI_HPC_LOCATION  *HpcList;
  UINTN             HpcCount;

  //
  // Locate the PciHotPlugInit Protocol
  // If it doesn't exist, that means there is no
  // hot plug controller supported on the platform
  // the PCI Bus driver is running on. HotPlug Support
  // is an optional feature, so absence of the protocol
  // won't incur the penalty.
  //
  Status = gBS->LocateProtocol (
                  &gEfiPciHotPlugInitProtocolGuid,
                  NULL,
                  (VOID **) &gPciHotPlugInit
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = gPciHotPlugInit->GetRootHpcList (
                              gPciHotPlugInit,
                              &HpcCount,
                              &HpcList
                              );

  if (!EFI_ERROR (Status)) {

    gPciRootHpcPool   = HpcList;
    gPciRootHpcCount  = HpcCount;
    gPciRootHpcData   = AllocateZeroPool (sizeof (ROOT_HPC_DATA) * gPciRootHpcCount);
    if (gPciRootHpcData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  return EFI_SUCCESS;
}

/**
  Test whether device path is for root pci hot plug bus.

  @param HpbDevicePath  A pointer to device path data structure to be tested.
  @param HpIndex        If HpIndex is not NULL, return the index of root hot
                        plug in global array when TRUE is retuned.

  @retval TRUE          The device path is for root pci hot plug bus.
  @retval FALSE         The device path is not for root pci hot plug bus.

**/
BOOLEAN
IsRootPciHotPlugBus (
  IN  EFI_DEVICE_PATH_PROTOCOL        *HpbDevicePath,
  OUT UINTN                           *HpIndex    OPTIONAL
  )
{
  UINTN Index;

  for (Index = 0; Index < gPciRootHpcCount; Index++) {

    if (EfiCompareDevicePath (gPciRootHpcPool[Index].HpbDevicePath, HpbDevicePath)) {

      if (HpIndex != NULL) {
        *HpIndex = Index;
      }

      return TRUE;
    }
  }

  return FALSE;
}

/**
  Test whether device path is for root pci hot plug controller.

  @param HpcDevicePath  A pointer to device path data structure to be tested.
  @param HpIndex        If HpIndex is not NULL, return the index of root hot
                        plug in global array when TRUE is retuned.

  @retval TRUE          The device path is for root pci hot plug controller.
  @retval FALSE         The device path is not for root pci hot plug controller.

**/
BOOLEAN
IsRootPciHotPlugController (
  IN EFI_DEVICE_PATH_PROTOCOL         *HpcDevicePath,
  OUT UINTN                           *HpIndex
  )
{
  UINTN Index;

  for (Index = 0; Index < gPciRootHpcCount; Index++) {

    if (EfiCompareDevicePath (gPciRootHpcPool[Index].HpcDevicePath, HpcDevicePath)) {

      if (HpIndex != NULL) {
        *HpIndex = Index;
      }

      return TRUE;
    }
  }

  return FALSE;
}

/**
  Creating event object for PCI Hot Plug controller.

  @param  HpIndex   Index of hot plug device in global array.
  @param  Event     The retuned event that invoke this function.

  @return Status of create event invoken.

**/
EFI_STATUS
CreateEventForHpc (
  IN  UINTN      HpIndex,
  OUT EFI_EVENT  *Event
  )
{
  EFI_STATUS  Status;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  PciHPCInitialized,
                  gPciRootHpcData + HpIndex,
                  &((gPciRootHpcData + HpIndex)->Event)
                  );

  if (!EFI_ERROR (Status)) {
    *Event = (gPciRootHpcData + HpIndex)->Event;
  }

  return Status;
}

/**
  Wait for all root PCI Hot Plug controller finished initializing.

  @param TimeoutInMicroSeconds  Microseconds to wait for all root HPCs' initialization.

  @retval EFI_SUCCESS           All HPCs initialization finished.
  @retval EFI_TIMEOUT           Not ALL HPCs initialization finished in Microseconds.

**/
EFI_STATUS
AllRootHPCInitialized (
  IN  UINTN           TimeoutInMicroSeconds
  )
{
  UINT32  Delay;
  UINTN   Index;

  Delay = (UINT32) ((TimeoutInMicroSeconds / 30) + 1);

  do {
    for (Index = 0; Index < gPciRootHpcCount; Index++) {

      if (gPciRootHpcData[Index].Found && !gPciRootHpcData[Index].Initialized) {
        break;
      }
    }

    if (Index == gPciRootHpcCount) {
      return EFI_SUCCESS;
    }

    //
    // Stall for 30 microseconds..
    //
    gBS->Stall (30);

    Delay--;

  } while (Delay > 0);

  return EFI_TIMEOUT;
}

/**
  Check whether PCI-PCI bridge has PCI Hot Plug capability register block.

  @param PciIoDevice    A Pointer to the PCI-PCI bridge.

  @retval TRUE    PCI device is HPC.
  @retval FALSE   PCI device is not HPC.

**/
BOOLEAN
IsSHPC (
  IN PCI_IO_DEVICE                      *PciIoDevice
  )
{

  EFI_STATUS  Status;
  UINT8       Offset;

  if (PciIoDevice == NULL) {
    return FALSE;
  }

  Offset = 0;
  Status = LocateCapabilityRegBlock (
            PciIoDevice,
            EFI_PCI_CAPABILITY_ID_HOTPLUG,
            &Offset,
            NULL
            );

  //
  // If the PCI-PCI bridge has the hot plug controller build-in,
  // then return TRUE;
  //
  if (!EFI_ERROR (Status)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Get resource padding if the specified PCI bridge is a hot plug bus.

  @param PciIoDevice    PCI bridge instance.

**/
VOID
GetResourcePaddingForHpb (
  IN PCI_IO_DEVICE      *PciIoDevice
  )
{
  EFI_STATUS                        Status;
  EFI_HPC_STATE                     State;
  UINT64                            PciAddress;
  EFI_HPC_PADDING_ATTRIBUTES        Attributes;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptors;

  if (IsPciHotPlugBus (PciIoDevice)) {
    //
    // If PCI-PCI bridge device is PCI Hot Plug bus.
    //
    PciAddress = EFI_PCI_ADDRESS (PciIoDevice->BusNumber, PciIoDevice->DeviceNumber, PciIoDevice->FunctionNumber, 0);
    Status = gPciHotPlugInit->GetResourcePadding (
                                gPciHotPlugInit,
                                PciIoDevice->DevicePath,
                                PciAddress,
                                &State,
                                (VOID **) &Descriptors,
                                &Attributes
                                );

    if (EFI_ERROR (Status)) {
      return;
    }

    if ((State & EFI_HPC_STATE_ENABLED) != 0 && (State & EFI_HPC_STATE_INITIALIZED) != 0) {
      PciIoDevice->ResourcePaddingDescriptors = Descriptors;
      PciIoDevice->PaddingAttributes          = Attributes;
    }

    return;
  }
}

/**
  Test whether PCI device is hot plug bus.

  @param PciIoDevice  PCI device instance.

  @retval TRUE    PCI device is a hot plug bus.
  @retval FALSE   PCI device is not a hot plug bus.

**/
BOOLEAN
IsPciHotPlugBus (
  PCI_IO_DEVICE                       *PciIoDevice
  )
{
  if (IsSHPC (PciIoDevice)) {
    //
    // If the PPB has the hot plug controller build-in,
    // then return TRUE;
    //
    return TRUE;
  }

  //
  // Otherwise, see if it is a Root HPC
  //
  if(IsRootPciHotPlugBus (PciIoDevice->DevicePath, NULL)) {
    return TRUE;
  }

  return FALSE;
}


/** @file
  This module provide support function for hot plug device.
  
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#include "PciBus.h"
#include "PciHotPlugSupport.h"

EFI_PCI_HOT_PLUG_INIT_PROTOCOL  *gPciHotPlugInit;
EFI_HPC_LOCATION                *gPciRootHpcPool;
UINTN                           gPciRootHpcCount;
ROOT_HPC_DATA                   *gPciRootHpcData;

/**
  Init HPC private data.
  
  @param  Event     event object
  @param  Context   HPC private data.
**/
VOID
EFIAPI
PciHPCInitialized (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  ROOT_HPC_DATA *HpcData;

  HpcData               = (ROOT_HPC_DATA *) Context;
  HpcData->Initialized  = TRUE;

}

/**
  Compare two device path
  
  @param DevicePath1    the first device path want to be compared.
  @param DevicePath2    the first device path want to be compared.
  
  @retval TRUE    equal.
  @retval FALSE   different.
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
  Init hot plug support and root hot plug private data.
  
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
  // won't incur the penalty
  //
  gPciHotPlugInit   = NULL;
  gPciRootHpcPool   = NULL;
  gPciRootHpcCount  = 0;
  gPciRootHpcData   = NULL;

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
  Test whether device path is for root pci hot plug bus
  
  @param HpbDevicePath  tested device path.
  @param HpIndex        Return the index of root hot plug in global array.
  
  @retval TRUE  device path is for root pci hot plug.
  @retval FALSE device path is not for root pci hot plug.
**/
BOOLEAN
IsRootPciHotPlugBus (
  IN EFI_DEVICE_PATH_PROTOCOL         *HpbDevicePath,
  OUT UINTN                           *HpIndex
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
  Test whether device path is for root pci hot plug controller
  
  @param HpcDevicePath  tested device path.
  @param HpIndex        Return the index of root hot plug in global array.
  
  @retval TRUE  device path is for root pci hot plug controller.
  @retval FALSE device path is not for root pci hot plug controller.
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
  Wrapper for creating event object for HPC 
  
  @param  HpIndex   index of hot plug device in global array.
  @param  Event     event object.
  
  @return status of create event invoken.
**/
EFI_STATUS
CreateEventForHpc (
  IN UINTN       HpIndex,
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
  Wait for all root HPC initialized.
  
  @param TimeoutInMicroSeconds  microseconds to wait for all root hpc's initialization.
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

      if (!gPciRootHpcData[Index].Initialized) {
        break;
      }
    }

    if (Index == gPciRootHpcCount) {
      return EFI_SUCCESS;
    }

    //
    // Stall for 30 us
    //
    gBS->Stall (30);

    Delay--;

  } while (Delay > 0);

  return EFI_TIMEOUT;
}

/**
  Check HPC capability register block
  
  @param PciIoDevice PCI device instance.
  
  @retval EFI_SUCCESS   PCI device is HPC.
  @retval EFI_NOT_FOUND PCI device is not HPC.
**/
EFI_STATUS
IsSHPC (
  PCI_IO_DEVICE                       *PciIoDevice
  )
{

  EFI_STATUS  Status;
  UINT8       Offset;

  if (PciIoDevice == NULL) {
    return EFI_NOT_FOUND;
  }

  Offset = 0;
  Status = LocateCapabilityRegBlock (
            PciIoDevice,
            EFI_PCI_CAPABILITY_ID_HOTPLUG,
            &Offset,
            NULL
            );

  //
  // If the PPB has the hot plug controller build-in,
  // then return TRUE;
  //
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Get resource padding for hot plug bus
  
  @param PciIoDevice PCI device instance
  
  @retval EFI_SUCCESS   success get padding and set it into PCI device instance
  @retval EFI_NOT_FOUND PCI device is not a hot plug bus.
**/
EFI_STATUS
GetResourcePaddingForHpb (
  IN PCI_IO_DEVICE *PciIoDevice
  )
/**

Routine Description:

Arguments:

Returns:

  None

**/
// TODO:    PciIoDevice - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
{
  EFI_STATUS                        Status;
  EFI_HPC_STATE                     State;
  UINT64                            PciAddress;
  EFI_HPC_PADDING_ATTRIBUTES        Attributes;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptors;

  Status = IsPciHotPlugBus (PciIoDevice);

  if (!EFI_ERROR (Status)) {
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
      return Status;
    }

    if ((State & EFI_HPC_STATE_ENABLED) != 0 && (State & EFI_HPC_STATE_INITIALIZED) != 0) {
      PciIoDevice->ResourcePaddingDescriptors = Descriptors;
      PciIoDevice->PaddingAttributes          = Attributes;
    }

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Test whether PCI device is hot plug bus.
  
  @param PciIoDevice  PCI device instance.
  
  @retval EFI_SUCCESS   PCI device is hot plug bus.
  @retval EFI_NOT_FOUND PCI device is not hot plug bus.
**/
EFI_STATUS
IsPciHotPlugBus (
  PCI_IO_DEVICE                       *PciIoDevice
  )
{
  BOOLEAN     Result;
  EFI_STATUS  Status;

  Status = IsSHPC (PciIoDevice);

  //
  // If the PPB has the hot plug controller build-in,
  // then return TRUE;
  //
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  //
  // Otherwise, see if it is a Root HPC
  //
  Result = IsRootPciHotPlugBus (PciIoDevice->DevicePath, NULL);

  if (Result) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}


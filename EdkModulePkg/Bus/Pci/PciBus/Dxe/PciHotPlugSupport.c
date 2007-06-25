/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PciHotPlugSupport.c

Abstract:



Revision History

--*/

#include "Pcibus.h"
#include "PciHotPlugSupport.h"

EFI_PCI_HOT_PLUG_INIT_PROTOCOL  *gPciHotPlugInit;
EFI_HPC_LOCATION                *gPciRootHpcPool;
UINTN                           gPciRootHpcCount;
ROOT_HPC_DATA                   *gPciRootHpcData;

VOID
EFIAPI
PciHPCInitialized (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    Event - add argument and description to function comment
// TODO:    Context - add argument and description to function comment
{
  ROOT_HPC_DATA *HpcData;

  HpcData               = (ROOT_HPC_DATA *) Context;
  HpcData->Initialized  = TRUE;

}

BOOLEAN
EfiCompareDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL *DevicePath1,
  IN EFI_DEVICE_PATH_PROTOCOL *DevicePath2
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    DevicePath1 - add argument and description to function comment
// TODO:    DevicePath2 - add argument and description to function comment
{
  UINTN Size1;
  UINTN Size2;

  Size1 = GetDevicePathSize (DevicePath1);
  Size2 = GetDevicePathSize (DevicePath2);

  if (Size1 != Size2) {
    return FALSE;
  }

  if (CompareMem (DevicePath1, DevicePath2, Size1)) {
    return FALSE;
  }

  return TRUE;
}

EFI_STATUS
InitializeHotPlugSupport (
  VOID
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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

BOOLEAN
IsRootPciHotPlugBus (
  IN EFI_DEVICE_PATH_PROTOCOL         *HpbDevicePath,
  OUT UINTN                           *HpIndex
  )
/*++

Routine Description:

Arguments:

  HpcDevicePath       - A pointer to the EFI_DEVICE_PATH_PROTOCOL.
  HpIndex             - A pointer to the Index.

Returns:

  None

--*/
// TODO:    HpbDevicePath - add argument and description to function comment
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

BOOLEAN
IsRootPciHotPlugController (
  IN EFI_DEVICE_PATH_PROTOCOL         *HpcDevicePath,
  OUT UINTN                           *HpIndex
  )
/*++

Routine Description:

Arguments:

  HpcDevicePath       - A pointer to the EFI_DEVICE_PATH_PROTOCOL.
  HpIndex             - A pointer to the Index.

Returns:

  None

--*/
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

EFI_STATUS
CreateEventForHpc (
  IN UINTN       HpIndex,
  OUT EFI_EVENT  *Event
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    HpIndex - add argument and description to function comment
// TODO:    Event - add argument and description to function comment
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

EFI_STATUS
AllRootHPCInitialized (
  IN  UINTN           TimeoutInMicroSeconds
  )
/*++

Routine Description:

Arguments:
  TimeoutInMicroSeconds - microseconds to wait for all root hpc's initialization

Returns:
  EFI_SUCCESS - All root hpc's initialization is finished before the timeout
  EFI_TIMEOUT - Time out

--*/
// TODO:    TimeoutInMilliSeconds - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_TIMEOUT - add return value to function comment
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

  } while (Delay);

  return EFI_TIMEOUT;
}

EFI_STATUS
IsSHPC (
  PCI_IO_DEVICE                       *PciIoDevice
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
{

  EFI_STATUS  Status;
  UINT8       Offset;

  if (!PciIoDevice) {
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

EFI_STATUS
GetResourcePaddingForHpb (
  IN PCI_IO_DEVICE *PciIoDevice
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
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

    if ((State & EFI_HPC_STATE_ENABLED) && (State & EFI_HPC_STATE_INITIALIZED)) {
      PciIoDevice->ResourcePaddingDescriptors = Descriptors;
      PciIoDevice->PaddingAttributes          = Attributes;
    }

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
IsPciHotPlugBus (
  PCI_IO_DEVICE                       *PciIoDevice
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
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

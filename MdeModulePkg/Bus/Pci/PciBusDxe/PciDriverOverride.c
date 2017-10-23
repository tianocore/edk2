/** @file
  Functions implementation for Bus Specific Driver Override protoocl.

Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PciBus.h"

/**
  Initializes a PCI Driver Override Instance.

  @param  PciIoDevice   PCI Device instance.

**/
VOID
InitializePciDriverOverrideInstance (
  IN OUT PCI_IO_DEVICE          *PciIoDevice
  )
{
  PciIoDevice->PciDriverOverride.GetDriver = GetDriver;
}

/**
  Find the image handle whose path equals to ImagePath.

  @param ImagePath   Image path.

  @return Image handle.
**/
EFI_HANDLE
LocateImageHandle (
  IN EFI_DEVICE_PATH_PROTOCOL   *ImagePath
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *Handles;
  UINTN                         Index;
  UINTN                         HandleNum;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  UINTN                         ImagePathSize;
  EFI_HANDLE                    ImageHandle;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiLoadedImageDevicePathProtocolGuid,
                  NULL,
                  &HandleNum,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  ImageHandle   = NULL;
  ImagePathSize = GetDevicePathSize (ImagePath);

  for (Index = 0; Index < HandleNum; Index++) {
    Status = gBS->HandleProtocol (Handles[Index], &gEfiLoadedImageDevicePathProtocolGuid, (VOID **) &DevicePath);
    if (EFI_ERROR (Status)) {
      continue;
    }
    if ((ImagePathSize == GetDevicePathSize (DevicePath)) &&
        (CompareMem (ImagePath, DevicePath, ImagePathSize) == 0)
        ) {
      ImageHandle = Handles[Index];
      break;
    }
  }

  FreePool (Handles);
  return ImageHandle;
}

/**
  Uses a bus specific algorithm to retrieve a driver image handle for a controller.

  @param  This                  A pointer to the EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL instance.
  @param  DriverImageHandle     On input, a pointer to the previous driver image handle returned
                                by GetDriver(). On output, a pointer to the next driver
                                image handle. Passing in a NULL, will return the first driver
                                image handle.

  @retval EFI_SUCCESS           A bus specific override driver is returned in DriverImageHandle.
  @retval EFI_NOT_FOUND         The end of the list of override drivers was reached.
                                A bus specific override driver is not returned in DriverImageHandle.
  @retval EFI_INVALID_PARAMETER DriverImageHandle is not a handle that was returned on a
                                previous call to GetDriver().

**/
EFI_STATUS
EFIAPI
GetDriver (
  IN EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL              *This,
  IN OUT EFI_HANDLE                                         *DriverImageHandle
  )
{
  PCI_IO_DEVICE             *PciIoDevice;
  LIST_ENTRY                *Link;
  PCI_DRIVER_OVERRIDE_LIST  *Override;
  BOOLEAN                   ReturnNext;

  Override    = NULL;
  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_DRIVER_OVERRIDE_THIS (This);
  ReturnNext  = (BOOLEAN) (*DriverImageHandle == NULL);
  for ( Link = GetFirstNode (&PciIoDevice->OptionRomDriverList)
      ; !IsNull (&PciIoDevice->OptionRomDriverList, Link)
      ; Link = GetNextNode (&PciIoDevice->OptionRomDriverList, Link)
      ) {

    Override = DRIVER_OVERRIDE_FROM_LINK (Link);

    if (ReturnNext) {
      if (Override->DriverImageHandle == NULL) {
        Override->DriverImageHandle = LocateImageHandle (Override->DriverImagePath);
      }

      if (Override->DriverImageHandle == NULL) {
        //
        // The Option ROM identified by Override->DriverImagePath is not loaded.
        //
        continue;
      } else {
        *DriverImageHandle = Override->DriverImageHandle;
        return EFI_SUCCESS;
      }
    }

    if (*DriverImageHandle == Override->DriverImageHandle) {
      ReturnNext = TRUE;
    }
  }

  ASSERT (IsNull (&PciIoDevice->OptionRomDriverList, Link));
  //
  // ReturnNext indicates a handle match happens.
  // If all nodes are checked without handle match happening,
  // the DriverImageHandle should be a invalid handle.
  //
  if (ReturnNext) {
    return EFI_NOT_FOUND;
  } else {
    return EFI_INVALID_PARAMETER;
  }
}

/**
  Add an overriding driver image.

  @param PciIoDevice        Instance of PciIo device.
  @param DriverImageHandle  Image handle of newly added driver image.
  @param DriverImagePath    Device path of newly added driver image.

  @retval EFI_SUCCESS          Successfully added driver.
  @retval EFI_OUT_OF_RESOURCES No memory resource for new driver instance.
  @retval other                Some error occurred when locating gEfiLoadedImageProtocolGuid.

**/
EFI_STATUS
AddDriver (
  IN PCI_IO_DEVICE            *PciIoDevice,
  IN EFI_HANDLE               DriverImageHandle,
  IN EFI_DEVICE_PATH_PROTOCOL *DriverImagePath
  )
{
  PCI_DRIVER_OVERRIDE_LIST      *Node;

  //
  // Caller should pass in either Image Handle or Image Path, but not both.
  //
  ASSERT ((DriverImageHandle == NULL) || (DriverImagePath == NULL));

  Node = AllocateZeroPool (sizeof (PCI_DRIVER_OVERRIDE_LIST));
  if (Node == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Node->Signature         = DRIVER_OVERRIDE_SIGNATURE;
  Node->DriverImageHandle = DriverImageHandle;
  Node->DriverImagePath   = DuplicateDevicePath (DriverImagePath);

  InsertTailList (&PciIoDevice->OptionRomDriverList, &Node->Link);

  PciIoDevice->BusOverride  = TRUE;
  return EFI_SUCCESS;
}


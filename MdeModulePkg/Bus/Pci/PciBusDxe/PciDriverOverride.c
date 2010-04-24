/** @file
  Functions implementation for Bus Specific Driver Override protoocl.

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
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
  LIST_ENTRY                *CurrentLink;
  PCI_DRIVER_OVERRIDE_LIST  *Node;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_DRIVER_OVERRIDE_THIS (This);

  CurrentLink = PciIoDevice->OptionRomDriverList.ForwardLink;

  while (CurrentLink != NULL && CurrentLink != &PciIoDevice->OptionRomDriverList) {

    Node = DRIVER_OVERRIDE_FROM_LINK (CurrentLink);

    if (*DriverImageHandle == NULL) {

      *DriverImageHandle = Node->DriverImageHandle;
      return EFI_SUCCESS;
    }

    if (*DriverImageHandle == Node->DriverImageHandle) {

      if (CurrentLink->ForwardLink == &PciIoDevice->OptionRomDriverList ||
          CurrentLink->ForwardLink == NULL) {
        return EFI_NOT_FOUND;
      }

      //
      // Get next node
      //
      Node                = DRIVER_OVERRIDE_FROM_LINK (CurrentLink->ForwardLink);
      *DriverImageHandle  = Node->DriverImageHandle;
      return EFI_SUCCESS;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Add an overriding driver image.

  @param PciIoDevice        Instance of PciIo device.
  @param DriverImageHandle  new added driver image.

  @retval EFI_SUCCESS          Successfully added driver.
  @retval EFI_OUT_OF_RESOURCES No memory resource for new driver instance.
  @retval other                Some error occurred when locating gEfiLoadedImageProtocolGuid.

**/
EFI_STATUS
AddDriver (
  IN PCI_IO_DEVICE     *PciIoDevice,
  IN EFI_HANDLE        DriverImageHandle
  )
{
  EFI_STATUS                    Status;
  EFI_LOADED_IMAGE_PROTOCOL     *LoadedImage;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  PCI_DRIVER_OVERRIDE_LIST      *Node;

  Status = gBS->HandleProtocol (DriverImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &LoadedImage);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Node = AllocatePool (sizeof (PCI_DRIVER_OVERRIDE_LIST));
  if (Node == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Node->Signature         = DRIVER_OVERRIDE_SIGNATURE;
  Node->DriverImageHandle = DriverImageHandle;

  InsertTailList (&PciIoDevice->OptionRomDriverList, &(Node->Link));

  PciIoDevice->BusOverride  = TRUE;

  ImageContext.Handle    = LoadedImage->ImageBase;
  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

  //
  // Get information about the image
  //
  PeCoffLoaderGetImageInfo (&ImageContext);

  return EFI_SUCCESS;
}


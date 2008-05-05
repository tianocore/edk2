/*++

Copyright (c) 2005 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciDriverOverride.c
  
Abstract:

  PCI Bus Driver

Revision History

--*/

#include "pcibus.h"

EFI_STATUS
EFIAPI
GetDriver(
  IN     struct _EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL  *This,
  IN OUT EFI_HANDLE                                         *DriverImageHandle
  );



EFI_STATUS
InitializePciDriverOverrideInstance (
  PCI_IO_DEVICE  *PciIoDevice
  )
/*++

Routine Description:

  Initializes a PCI Driver Override Instance

Arguments:
  
Returns:

  None

--*/

{
  PciIoDevice->PciDriverOverride.GetDriver = GetDriver;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetDriver (
  IN EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL              *This,
  IN OUT EFI_HANDLE                                         *DriverImageHandle
  )
/*++

Routine Description:

  Get a overriding driver image

Arguments:
  
Returns:

  None

--*/
{
  PCI_IO_DEVICE             *PciIoDevice;
  LIST_ENTRY            *CurrentLink;
  PCI_DRIVER_OVERRIDE_LIST  *Node;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_DRIVER_OVERRIDE_THIS (This);

  CurrentLink = PciIoDevice->OptionRomDriverList.ForwardLink;

  while (CurrentLink && CurrentLink != &PciIoDevice->OptionRomDriverList) {

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

EFI_STATUS
AddDriver (
  IN PCI_IO_DEVICE     *PciIoDevice,
  IN EFI_HANDLE        DriverImageHandle
  )
/*++

Routine Description:

  Add a overriding driver image

Arguments:
  
Returns:

  None

--*/

{
  EFI_STATUS                    Status;
  EFI_IMAGE_DOS_HEADER          *DosHdr;
  EFI_IMAGE_NT_HEADERS          *PeHdr;
  EFI_LOADED_IMAGE_PROTOCOL     *LoadedImage;
  PCI_DRIVER_OVERRIDE_LIST      *Node;
#if (EFI_SPECIFICATION_VERSION < 0x00020000)
  EFI_DRIVER_OS_HANDOFF_HEADER  *DriverOsHandoffHeader;
  EFI_DRIVER_OS_HANDOFF_HEADER  *NewDriverOsHandoffHeader;
  EFI_DRIVER_OS_HANDOFF         *DriverOsHandoff;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_HANDLE                    DeviceHandle;
  UINTN                         NumberOfEntries;
  UINTN                         Size;
  UINTN                         Index;
#endif

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

  DosHdr                    = (EFI_IMAGE_DOS_HEADER *) LoadedImage->ImageBase;
  if (DosHdr->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    return EFI_SUCCESS;
  }

  PeHdr = (EFI_IMAGE_NT_HEADERS *) ((UINTN) LoadedImage->ImageBase + DosHdr->e_lfanew);

  if (PeHdr->FileHeader.Machine != EFI_IMAGE_MACHINE_EBC) {
    return EFI_SUCCESS;
  }

#if (EFI_SPECIFICATION_VERSION < 0x00020000)
  DriverOsHandoffHeader = NULL;
  Status                = EfiLibGetSystemConfigurationTable (&gEfiUgaIoProtocolGuid, (VOID **) &DriverOsHandoffHeader);
  if (!EFI_ERROR (Status) && DriverOsHandoffHeader != NULL) {
    for (Index = 0; Index < DriverOsHandoffHeader->NumberOfEntries; Index++) {
      DriverOsHandoff = (EFI_DRIVER_OS_HANDOFF *)((UINTN)(DriverOsHandoffHeader)    + 
                                                  DriverOsHandoffHeader->HeaderSize + 
                                                  Index * DriverOsHandoffHeader->SizeOfEntries);
      DevicePath  = DriverOsHandoff->DevicePath;
      Status      = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &DevicePath, &DeviceHandle);
      if (!EFI_ERROR (Status) && DeviceHandle != NULL && IsDevicePathEnd (DevicePath)) {
        if (DeviceHandle == PciIoDevice->Handle) {
          return EFI_SUCCESS;
        }
      }
    }

    NumberOfEntries = DriverOsHandoffHeader->NumberOfEntries + 1;
  } else {
    NumberOfEntries = 1;
  }

  Status = gBS->AllocatePool (
                  EfiRuntimeServicesData,
                  sizeof (EFI_DRIVER_OS_HANDOFF_HEADER) + NumberOfEntries * sizeof (EFI_DRIVER_OS_HANDOFF),
                  (VOID **) &NewDriverOsHandoffHeader
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DriverOsHandoffHeader == NULL) {
    NewDriverOsHandoffHeader->Version         = 0;
    NewDriverOsHandoffHeader->HeaderSize      = sizeof (EFI_DRIVER_OS_HANDOFF_HEADER);
    NewDriverOsHandoffHeader->SizeOfEntries   = sizeof (EFI_DRIVER_OS_HANDOFF);
    NewDriverOsHandoffHeader->NumberOfEntries = (UINT32) NumberOfEntries;
  } else {
    gBS->CopyMem (
          NewDriverOsHandoffHeader,
          DriverOsHandoffHeader,
          DriverOsHandoffHeader->HeaderSize + (NumberOfEntries - 1) * DriverOsHandoffHeader->SizeOfEntries
          );
    NewDriverOsHandoffHeader->NumberOfEntries = (UINT32) NumberOfEntries;
  }

  DriverOsHandoff = (EFI_DRIVER_OS_HANDOFF *)((UINTN)NewDriverOsHandoffHeader      + 
                                              NewDriverOsHandoffHeader->HeaderSize + 
                                              (NumberOfEntries - 1) * NewDriverOsHandoffHeader->SizeOfEntries);

  //
  // Fill in the EFI_DRIVER_OS_HANDOFF structure
  //
  DriverOsHandoff->Type = EfiUgaDriverFromPciRom;

  //
  // Compute the size of the device path
  //
  Size = EfiDevicePathSize (PciIoDevice->DevicePath);
  if (Size == 0) {
    DriverOsHandoff->DevicePath = NULL;
  } else {

    //
    // Allocate space for duplicate device path
    //
    Status = gBS->AllocatePool (
                    EfiRuntimeServicesData,
                    Size,
                    (VOID **) &DriverOsHandoff->DevicePath
                    );
    if (EFI_ERROR (Status)) {
      gBS->FreePool (NewDriverOsHandoffHeader);
      return Status;
    }

    //
    // Make copy of device path
    //
    CopyMem (DriverOsHandoff->DevicePath, PciIoDevice->DevicePath, Size);
  }

  DriverOsHandoff->PciRomSize = (UINT64) PciIoDevice->PciIo.RomSize;
  Status = gBS->AllocatePool (
                  EfiRuntimeServicesData,
                  (UINTN) DriverOsHandoff->PciRomSize,
                  (VOID **) &DriverOsHandoff->PciRomImage
                  );
  if (EFI_ERROR (Status)) {
    gBS->FreePool (NewDriverOsHandoffHeader);
    return Status;
  }

  gBS->CopyMem (
        DriverOsHandoff->PciRomImage,
        PciIoDevice->PciIo.RomImage,
        (UINTN) DriverOsHandoff->PciRomSize
        );

  Status = gBS->InstallConfigurationTable (&gEfiUgaIoProtocolGuid, NewDriverOsHandoffHeader);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DriverOsHandoffHeader != NULL) {
    gBS->FreePool (DriverOsHandoffHeader);
  }
#endif

  return EFI_SUCCESS;
}

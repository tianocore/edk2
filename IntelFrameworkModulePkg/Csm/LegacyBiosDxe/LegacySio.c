/** @file
  Collect Sio information from Native EFI Drivers.
  Sio is floppy, parallel, serial, ... hardware

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LegacyBiosInterface.h"


/**
  Collect EFI Info about legacy devices.

  @param  Private      Legacy BIOS Instance data

  @retval EFI_SUCCESS  It should always work.

**/
EFI_STATUS
LegacyBiosBuildSioData (
  IN  LEGACY_BIOS_INSTANCE      *Private
  )
{
  EFI_STATUS                          Status;
  DEVICE_PRODUCER_DATA_HEADER         *SioPtr;
  DEVICE_PRODUCER_SERIAL              *Sio1Ptr;
  DEVICE_PRODUCER_PARALLEL            *Sio2Ptr;
  DEVICE_PRODUCER_FLOPPY              *Sio3Ptr;
  EFI_HANDLE                          IsaBusController;
  UINTN                               HandleCount;
  EFI_HANDLE                          *HandleBuffer;
  UINTN                               Index;
  UINTN                               ResourceIndex;
  UINTN                               ChildIndex;
  EFI_ISA_IO_PROTOCOL                 *IsaIo;
  EFI_ISA_ACPI_RESOURCE_LIST          *ResourceList;
  EFI_ISA_ACPI_RESOURCE               *IoResource;
  EFI_ISA_ACPI_RESOURCE               *DmaResource;
  EFI_ISA_ACPI_RESOURCE               *InterruptResource;
  UINTN                               EntryCount;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenInfoBuffer;
  EFI_BLOCK_IO_PROTOCOL               *BlockIo;

  //
  // Get the pointer to the SIO data structure
  //
  SioPtr = &Private->IntThunk->EfiToLegacy16BootTable.SioData;

  //
  // Zero the data in the SIO data structure
  //
  gBS->SetMem (SioPtr, sizeof (DEVICE_PRODUCER_DATA_HEADER), 0);

  //
  // Find the ISA Bus Controller used for legacy
  //
  Status = Private->LegacyBiosPlatform->GetPlatformHandle (
                                          Private->LegacyBiosPlatform,
                                          EfiGetPlatformIsaBusHandle,
                                          0,
                                          &HandleBuffer,
                                          &HandleCount,
                                          NULL
                                          );
  IsaBusController = HandleBuffer[0];
  if (!EFI_ERROR (Status)) {
    //
    // Force ISA Bus Controller to produce all ISA devices
    //
    gBS->ConnectController (IsaBusController, NULL, NULL, TRUE);
  }
  //
  // Get the list of ISA controllers in the system
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiIsaIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }
  //
  // Collect legacy information from each of the ISA controllers in the system
  //
  for (Index = 0; Index < HandleCount; Index++) {

    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiIsaIoProtocolGuid, (VOID **) &IsaIo);
    if (EFI_ERROR (Status)) {
      continue;
    }

    ResourceList = IsaIo->ResourceList;

    if (ResourceList == NULL) {
      continue;
    }
    //
    // Collect the resource types neededto fill in the SIO data structure
    //
    IoResource        = NULL;
    DmaResource       = NULL;
    InterruptResource = NULL;
    for (ResourceIndex = 0;
         ResourceList->ResourceItem[ResourceIndex].Type != EfiIsaAcpiResourceEndOfList;
         ResourceIndex++
        ) {
      switch (ResourceList->ResourceItem[ResourceIndex].Type) {
      case EfiIsaAcpiResourceIo:
        IoResource = &ResourceList->ResourceItem[ResourceIndex];
        break;

      case EfiIsaAcpiResourceMemory:
        break;

      case EfiIsaAcpiResourceDma:
        DmaResource = &ResourceList->ResourceItem[ResourceIndex];
        break;

      case EfiIsaAcpiResourceInterrupt:
        InterruptResource = &ResourceList->ResourceItem[ResourceIndex];
        break;

      default:
        break;
      }
    }
    //
    // See if this is an ISA serial port
    //
    // Ignore DMA resource since it is always returned NULL
    //
    if (ResourceList->Device.HID == EISA_PNP_ID (0x500) || ResourceList->Device.HID == EISA_PNP_ID (0x501)) {

      if (ResourceList->Device.UID <= 3 &&
          IoResource != NULL &&
          InterruptResource != NULL
          ) {
        //
        // Get the handle of the child device that has opened the ISA I/O Protocol
        //
        Status = gBS->OpenProtocolInformation (
                        HandleBuffer[Index],
                        &gEfiIsaIoProtocolGuid,
                        &OpenInfoBuffer,
                        &EntryCount
                        );
        if (EFI_ERROR (Status)) {
          continue;
        }
        //
        // We want resource for legacy even if no 32-bit driver installed
        //
        for (ChildIndex = 0; ChildIndex < EntryCount; ChildIndex++) {
          Sio1Ptr           = &SioPtr->Serial[ResourceList->Device.UID];
          Sio1Ptr->Address  = (UINT16) IoResource->StartRange;
          Sio1Ptr->Irq      = (UINT8) InterruptResource->StartRange;
          Sio1Ptr->Mode     = DEVICE_SERIAL_MODE_NORMAL | DEVICE_SERIAL_MODE_DUPLEX_HALF;
        }

        FreePool (OpenInfoBuffer);
      }
    }
    //
    // See if this is an ISA parallel port
    //
    // Ignore DMA resource since it is always returned NULL, port
    // only used in output mode.
    //
    if (ResourceList->Device.HID == EISA_PNP_ID (0x400) || ResourceList->Device.HID == EISA_PNP_ID (0x401)) {
      if (ResourceList->Device.UID <= 2 &&
          IoResource != NULL &&
          InterruptResource != NULL &&
          DmaResource != NULL
          ) {
        Sio2Ptr           = &SioPtr->Parallel[ResourceList->Device.UID];
        Sio2Ptr->Address  = (UINT16) IoResource->StartRange;
        Sio2Ptr->Irq      = (UINT8) InterruptResource->StartRange;
        Sio2Ptr->Dma      = (UINT8) DmaResource->StartRange;
        Sio2Ptr->Mode     = DEVICE_PARALLEL_MODE_MODE_OUTPUT_ONLY;
      }
    }
    //
    // See if this is an ISA floppy controller
    //
    if (ResourceList->Device.HID == EISA_PNP_ID (0x604)) {
      if (IoResource != NULL && InterruptResource != NULL && DmaResource != NULL) {
        Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiBlockIoProtocolGuid, (VOID **) &BlockIo);
        if (!EFI_ERROR (Status)) {
          Sio3Ptr           = &SioPtr->Floppy;
          Sio3Ptr->Address  = (UINT16) IoResource->StartRange;
          Sio3Ptr->Irq      = (UINT8) InterruptResource->StartRange;
          Sio3Ptr->Dma      = (UINT8) DmaResource->StartRange;
          Sio3Ptr->NumberOfFloppy++;
        }
      }
    }
    //
    // See if this is a mouse
    // Always set mouse found so USB hot plug will work
    //
    // Ignore lower byte of HID. Pnp0fxx is any type of mouse.
    //
    //    Hid = ResourceList->Device.HID & 0xff00ffff;
    //    PnpId = EISA_PNP_ID(0x0f00);
    //    if (Hid == PnpId) {
    //      if (ResourceList->Device.UID == 1) {
    //        Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiSimplePointerProtocolGuid, &SimplePointer);
    //      if (!EFI_ERROR (Status)) {
    //
    SioPtr->MousePresent = 0x01;
    //
    //        }
    //      }
    //    }
    //
  }

  FreePool (HandleBuffer);

  return EFI_SUCCESS;
}

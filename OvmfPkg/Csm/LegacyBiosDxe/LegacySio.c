/** @file
  Collect Sio information from Native EFI Drivers.
  Sio is floppy, parallel, serial, ... hardware

Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LegacyBiosInterface.h"

/**
  Collect EFI Info about legacy devices through Super IO interface.

  @param  SioPtr       Pointer to SIO data.

  @retval EFI_SUCCESS   When SIO data is got successfully.
  @retval EFI_NOT_FOUND When ISA IO interface is absent.

**/
EFI_STATUS
LegacyBiosBuildSioDataFromSio (
  IN DEVICE_PRODUCER_DATA_HEADER             *SioPtr
  )
{
  EFI_STATUS                                 Status;
  DEVICE_PRODUCER_SERIAL                     *SioSerial;
  DEVICE_PRODUCER_PARALLEL                   *SioParallel;
  DEVICE_PRODUCER_FLOPPY                     *SioFloppy;
  UINTN                                      HandleCount;
  EFI_HANDLE                                 *HandleBuffer;
  UINTN                                      Index;
  UINTN                                      ChildIndex;
  EFI_SIO_PROTOCOL                           *Sio;
  ACPI_RESOURCE_HEADER_PTR                   Resources;
  EFI_ACPI_IO_PORT_DESCRIPTOR                *IoResource;
  EFI_ACPI_FIXED_LOCATION_IO_PORT_DESCRIPTOR *FixedIoResource;
  EFI_ACPI_DMA_DESCRIPTOR                    *DmaResource;
  EFI_ACPI_IRQ_NOFLAG_DESCRIPTOR             *IrqResource;
  UINT16                                     Address;
  UINT8                                      Dma;
  UINT8                                      Irq;
  UINTN                                      EntryCount;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY        *OpenInfoBuffer;
  EFI_BLOCK_IO_PROTOCOL                      *BlockIo;
  EFI_SERIAL_IO_PROTOCOL                     *SerialIo;
  EFI_DEVICE_PATH_PROTOCOL                   *DevicePath;
  ACPI_HID_DEVICE_PATH                       *Acpi;

  //
  // Get the list of ISA controllers in the system
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSioProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  //
  // Collect legacy information from each of the ISA controllers in the system
  //
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiSioProtocolGuid, (VOID **) &Sio);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Address = MAX_UINT16;
    Dma     = MAX_UINT8;
    Irq     = MAX_UINT8;
    Status = Sio->GetResources (Sio, &Resources);
    if (!EFI_ERROR (Status)) {
      //
      // Get the base address information from ACPI resource descriptor.
      //
      while (Resources.SmallHeader->Byte != ACPI_END_TAG_DESCRIPTOR) {
        switch (Resources.SmallHeader->Byte) {
        case ACPI_IO_PORT_DESCRIPTOR:
          IoResource = (EFI_ACPI_IO_PORT_DESCRIPTOR *) Resources.SmallHeader;
          Address = IoResource->BaseAddressMin;
          break;

        case ACPI_FIXED_LOCATION_IO_PORT_DESCRIPTOR:
          FixedIoResource = (EFI_ACPI_FIXED_LOCATION_IO_PORT_DESCRIPTOR *) Resources.SmallHeader;
          Address = FixedIoResource->BaseAddress;
          break;

        case ACPI_DMA_DESCRIPTOR:
          DmaResource = (EFI_ACPI_DMA_DESCRIPTOR *) Resources.SmallHeader;
          Dma = (UINT8) LowBitSet32 (DmaResource->ChannelMask);
          break;

        case ACPI_IRQ_DESCRIPTOR:
        case ACPI_IRQ_NOFLAG_DESCRIPTOR:
          IrqResource = (EFI_ACPI_IRQ_NOFLAG_DESCRIPTOR *) Resources.SmallHeader;
          Irq = (UINT8) LowBitSet32 (IrqResource->Mask);
          break;

        default:
          break;
        }

        if (Resources.SmallHeader->Bits.Type == 0) {
          Resources.SmallHeader = (ACPI_SMALL_RESOURCE_HEADER *) ((UINT8 *) Resources.SmallHeader
                                                                  + Resources.SmallHeader->Bits.Length
                                                                  + sizeof (*Resources.SmallHeader));
        } else {
          Resources.LargeHeader = (ACPI_LARGE_RESOURCE_HEADER *) ((UINT8 *) Resources.LargeHeader
                                                                  + Resources.LargeHeader->Length
                                                                  + sizeof (*Resources.LargeHeader));
        }
      }
    }

    DEBUG ((EFI_D_INFO, "LegacySio: Address/Dma/Irq = %x/%d/%d\n", Address, Dma, Irq));

    DevicePath = DevicePathFromHandle (HandleBuffer[Index]);
    if (DevicePath == NULL) {
      continue;
    }

    Acpi = NULL;
    while (!IsDevicePathEnd (DevicePath)) {
      Acpi = (ACPI_HID_DEVICE_PATH *) DevicePath;
      DevicePath = NextDevicePathNode (DevicePath);
    }

    if ((Acpi == NULL) || (DevicePathType (Acpi) != ACPI_DEVICE_PATH) ||
        ((DevicePathSubType (Acpi) != ACPI_DP) && (DevicePathSubType (Acpi) != ACPI_EXTENDED_DP))
        ) {
      continue;
    }

    //
    // See if this is an ISA serial port
    //
    // Ignore DMA resource since it is always returned NULL
    //
    if (Acpi->HID == EISA_PNP_ID (0x500) || Acpi->HID == EISA_PNP_ID (0x501)) {

      if (Acpi->UID < 4 && Address != MAX_UINT16 && Irq != MAX_UINT8) {
        //
        // Get the handle of the child device that has opened the Super I/O Protocol
        //
        Status = gBS->OpenProtocolInformation (
                        HandleBuffer[Index],
                        &gEfiSioProtocolGuid,
                        &OpenInfoBuffer,
                        &EntryCount
                        );
        if (EFI_ERROR (Status)) {
          continue;
        }
        for (ChildIndex = 0; ChildIndex < EntryCount; ChildIndex++) {
          if ((OpenInfoBuffer[ChildIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
            Status = gBS->HandleProtocol (OpenInfoBuffer[ChildIndex].ControllerHandle, &gEfiSerialIoProtocolGuid, (VOID **) &SerialIo);
            if (!EFI_ERROR (Status)) {
              SioSerial           = &SioPtr->Serial[Acpi->UID];
              SioSerial->Address  = Address;
              SioSerial->Irq      = Irq;
              SioSerial->Mode     = DEVICE_SERIAL_MODE_NORMAL | DEVICE_SERIAL_MODE_DUPLEX_HALF;
              break;
            }
          }
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
    if (Acpi->HID == EISA_PNP_ID (0x400) || Acpi->HID == EISA_PNP_ID (0x401)) {
      if (Acpi->UID < 3 && Address != MAX_UINT16 && Irq != MAX_UINT8 && Dma != MAX_UINT8) {
        SioParallel           = &SioPtr->Parallel[Acpi->UID];
        SioParallel->Address  = Address;
        SioParallel->Irq      = Irq;
        SioParallel->Dma      = Dma;
        SioParallel->Mode     = DEVICE_PARALLEL_MODE_MODE_OUTPUT_ONLY;
      }
    }
    //
    // See if this is an ISA floppy controller
    //
    if (Acpi->HID == EISA_PNP_ID (0x604)) {
      if (Address != MAX_UINT16 && Irq != MAX_UINT8 && Dma != MAX_UINT8) {
        Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiBlockIoProtocolGuid, (VOID **) &BlockIo);
        if (!EFI_ERROR (Status)) {
          SioFloppy           = &SioPtr->Floppy;
          SioFloppy->Address  = Address;
          SioFloppy->Irq      = Irq;
          SioFloppy->Dma      = Dma;
          SioFloppy->NumberOfFloppy++;
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

/**
  Collect EFI Info about legacy devices through ISA IO interface.

  @param  SioPtr       Pointer to SIO data.

  @retval EFI_SUCCESS   When SIO data is got successfully.
  @retval EFI_NOT_FOUND When ISA IO interface is absent.

**/
EFI_STATUS
LegacyBiosBuildSioDataFromIsaIo (
  IN DEVICE_PRODUCER_DATA_HEADER      *SioPtr
  )
{
  EFI_STATUS                          Status;
  DEVICE_PRODUCER_SERIAL              *SioSerial;
  DEVICE_PRODUCER_PARALLEL            *SioParallel;
  DEVICE_PRODUCER_FLOPPY              *SioFloppy;
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
  EFI_SERIAL_IO_PROTOCOL              *SerialIo;

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
    return EFI_NOT_FOUND;
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
          if ((OpenInfoBuffer[ChildIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
            Status = gBS->HandleProtocol (OpenInfoBuffer[ChildIndex].ControllerHandle, &gEfiSerialIoProtocolGuid, (VOID **) &SerialIo);
            if (!EFI_ERROR (Status)) {
              SioSerial           = &SioPtr->Serial[ResourceList->Device.UID];
              SioSerial->Address  = (UINT16) IoResource->StartRange;
              SioSerial->Irq      = (UINT8) InterruptResource->StartRange;
              SioSerial->Mode     = DEVICE_SERIAL_MODE_NORMAL | DEVICE_SERIAL_MODE_DUPLEX_HALF;
              break;
            }
          }
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
        SioParallel           = &SioPtr->Parallel[ResourceList->Device.UID];
        SioParallel->Address  = (UINT16) IoResource->StartRange;
        SioParallel->Irq      = (UINT8) InterruptResource->StartRange;
        SioParallel->Dma      = (UINT8) DmaResource->StartRange;
        SioParallel->Mode     = DEVICE_PARALLEL_MODE_MODE_OUTPUT_ONLY;
      }
    }
    //
    // See if this is an ISA floppy controller
    //
    if (ResourceList->Device.HID == EISA_PNP_ID (0x604)) {
      if (IoResource != NULL && InterruptResource != NULL && DmaResource != NULL) {
        Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiBlockIoProtocolGuid, (VOID **) &BlockIo);
        if (!EFI_ERROR (Status)) {
          SioFloppy           = &SioPtr->Floppy;
          SioFloppy->Address  = (UINT16) IoResource->StartRange;
          SioFloppy->Irq      = (UINT8) InterruptResource->StartRange;
          SioFloppy->Dma      = (UINT8) DmaResource->StartRange;
          SioFloppy->NumberOfFloppy++;
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
  EFI_HANDLE                          IsaBusController;
  UINTN                               HandleCount;
  EFI_HANDLE                          *HandleBuffer;

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

  Status = LegacyBiosBuildSioDataFromIsaIo (SioPtr);
  if (EFI_ERROR (Status)) {
    LegacyBiosBuildSioDataFromSio (SioPtr);
  }

  return EFI_SUCCESS;
}

/** @file
  The implementation for EFI_ISA_IO_PROTOCOL.

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "IsaIo.h"

//
// Module Variables
//
EFI_ISA_IO_PROTOCOL mIsaIoInterface = {
  {
    IsaIoMemRead,
    IsaIoMemWrite
  },
  {
    IsaIoIoRead,
    IsaIoIoWrite
  },
  IsaIoCopyMem,
  IsaIoMap,
  IsaIoUnmap,
  IsaIoAllocateBuffer,
  IsaIoFreeBuffer,
  IsaIoFlush,
  NULL,
  0,
  NULL
};

EFI_ISA_DMA_REGISTERS  mDmaRegisters[8] = {
  {
    0x00,
    0x87,
    0x01
  },
  {
    0x02,
    0x83,
    0x03
  },
  {
    0x04,
    0x81,
    0x05
  },
  {
    0x06,
    0x82,
    0x07
  },
  {
    0x00,
    0x00,
    0x00
  },  // Channel 4 is invalid
  {
    0xC4,
    0x8B,
    0xC6
  },
  {
    0xC8,
    0x89,
    0xCA
  },
  {
    0xCC,
    0x8A,
    0xCE
  },
};

/**
  Verifies access to an ISA device

  @param[in] IsaIoDevice         The ISA device to be verified.
  @param[in] Type                The Access type. The input must be either IsaAccessTypeMem or IsaAccessTypeIo.
  @param[in] Width               The width of the memory operation.
  @param[in] Count               The number of memory operations to perform.
  @param[in] Offset              The offset in ISA memory space to start the memory operation.

  @retval EFI_SUCCESS            Verify success.
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED        The device ont support the access type.
**/
EFI_STATUS
IsaIoVerifyAccess (
  IN ISA_IO_DEVICE              *IsaIoDevice,
  IN ISA_ACCESS_TYPE            Type,
  IN EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN UINTN                      Count,
  IN UINT32                     Offset
  )
{
  EFI_ISA_ACPI_RESOURCE *Item;
  EFI_STATUS            Status;

  if ((UINT32)Width >= EfiIsaIoWidthMaximum ||
      Width == EfiIsaIoWidthReserved ||
      Width == EfiIsaIoWidthFifoReserved ||
      Width == EfiIsaIoWidthFillReserved
      ) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If Width is EfiIsaIoWidthFifoUintX then convert to EfiIsaIoWidthUintX
  // If Width is EfiIsaIoWidthFillUintX then convert to EfiIsaIoWidthUintX
  //
  if (Width >= EfiIsaIoWidthFifoUint8 && Width < EfiIsaIoWidthFifoReserved) {
    Count = 1;
  }

  Width = (EFI_ISA_IO_PROTOCOL_WIDTH) (Width & 0x03);

  Status  = EFI_UNSUPPORTED;
  Item    = IsaIoDevice->IsaIo.ResourceList->ResourceItem;
  while (Item->Type != EfiIsaAcpiResourceEndOfList) {
    if ((Type == IsaAccessTypeMem && Item->Type == EfiIsaAcpiResourceMemory) ||
        (Type == IsaAccessTypeIo && Item->Type == EfiIsaAcpiResourceIo)) {
      if (Offset >= Item->StartRange && (Offset + Count * (UINT32)(1 << Width)) - 1 <= Item->EndRange) {
        return EFI_SUCCESS;
      }

      if (Offset >= Item->StartRange && Offset <= Item->EndRange) {
        Status = EFI_INVALID_PARAMETER;
      }
    }

    Item++;
  }

  return Status;
}

/**
  Convert the IO Information in ACPI descriptor to IO ISA Attribute.

  @param[in] Information   The IO Information in ACPI descriptor

  @return UINT32           The IO ISA Attribute
**/
UINT32
IsaIoAttribute (
  IN UINT8                            Information
  )
{
  UINT32             Attribute;

  Attribute = 0;

  switch (Information & EFI_ACPI_IO_DECODE_MASK) {
    case EFI_ACPI_IO_DECODE_16_BIT:
      Attribute |= EFI_ISA_ACPI_IO_DECODE_16_BITS;
      break;

    case EFI_ACPI_IO_DECODE_10_BIT:
      Attribute |= EFI_ISA_ACPI_IO_DECODE_10_BITS;
      break;
  }

  return Attribute;
}

/**
  Convert the IRQ Information in ACPI descriptor to IRQ ISA Attribute.

  @param[in] Information   The IRQ Information in ACPI descriptor

  @return UINT32           The IRQ ISA Attribute
**/
UINT32
IsaIrqAttribute (
  IN UINT8                            Information
  )
{
  UINT32             Attribute;

  Attribute = 0;

  if ((Information & EFI_ACPI_IRQ_POLARITY_MASK) == EFI_ACPI_IRQ_HIGH_TRUE) {
    if ((Information & EFI_ACPI_IRQ_MODE) == EFI_ACPI_IRQ_LEVEL_TRIGGERED) {
      Attribute = EFI_ISA_ACPI_IRQ_TYPE_HIGH_TRUE_LEVEL_SENSITIVE;
    } else {
      Attribute = EFI_ISA_ACPI_IRQ_TYPE_HIGH_TRUE_EDGE_SENSITIVE;
    }
  } else {
    if ((Information & EFI_ACPI_IRQ_MODE) == EFI_ACPI_IRQ_LEVEL_TRIGGERED) {
      Attribute = EFI_ISA_ACPI_IRQ_TYPE_LOW_TRUE_LEVEL_SENSITIVE;
    } else {
      Attribute = EFI_ISA_ACPI_IRQ_TYPE_LOW_TRUE_EDGE_SENSITIVE;
    }
  }
  return Attribute;
}

/**
  Convert the Memory Information in ACPI descriptor to Memory ISA Attribute.

  @param[in] Information   The Memory Information in ACPI descriptor

  @return UINT32           The Memory ISA Attribute
**/
UINT32
IsaMemoryAttribute (
  IN UINT8                            Information
  )
{
  UINT32             Attribute;

  Attribute = 0;

  switch (Information & EFI_ACPI_MEMORY_WRITE_STATUS_MASK) {
    case EFI_ACPI_MEMORY_WRITABLE:
      Attribute |= EFI_ISA_ACPI_MEMORY_WRITEABLE;
      break;
  }

  return Attribute;
}

/**
  Convert the DMA Information in ACPI descriptor to DMA ISA Attribute.

  @param[in] Information   The DMA Information in ACPI descriptor

  @return UINT32           The DMA ISA Attribute
**/
UINT32
IsaDmaAttribute (
  IN UINT8                            Information
  )
{
  UINT32             Attribute;

  Attribute = EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SINGLE_MODE;

  switch (Information & EFI_ACPI_DMA_SPEED_TYPE_MASK) {
    case EFI_ACPI_DMA_SPEED_TYPE_COMPATIBILITY:
      Attribute |= EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_COMPATIBLE;
      break;
    case EFI_ACPI_DMA_SPEED_TYPE_A:
      Attribute |= EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_A;
      break;
    case EFI_ACPI_DMA_SPEED_TYPE_B:
      Attribute |= EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_B;
      break;
    case EFI_ACPI_DMA_SPEED_TYPE_F:
      Attribute |= EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_C;
      break;
  }

  switch (Information & EFI_ACPI_DMA_TRANSFER_TYPE_MASK) {
    case EFI_ACPI_DMA_TRANSFER_TYPE_8_BIT:
      Attribute |= EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_WIDTH_8;
      break;
    case EFI_ACPI_DMA_TRANSFER_TYPE_16_BIT:
      Attribute |= EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_WIDTH_16;
      break;
  }

  return Attribute;
}

/**
  Convert the ACPI resource descriptor to ISA resource descriptor.

  @param[in]  AcpiResource          Pointer to the ACPI resource descriptor
  @param[out] IsaResource           The optional pointer to the buffer to
                                    store the converted ISA resource descriptor

  @return     UINTN                 Number of ISA resource descriptor needed
**/
UINTN
AcpiResourceToIsaResource (
  IN  ACPI_RESOURCE_HEADER_PTR        AcpiResource,
  OUT EFI_ISA_ACPI_RESOURCE           *IsaResource   OPTIONAL
  )
{
  UINT32                                        Index;
  UINTN                                         Count;
  UINT32                                        LastIndex;
  EFI_ACPI_IO_PORT_DESCRIPTOR                   *Io;
  EFI_ACPI_FIXED_LOCATION_IO_PORT_DESCRIPTOR    *FixedIo;
  EFI_ACPI_IRQ_DESCRIPTOR                       *Irq;
  EFI_ACPI_DMA_DESCRIPTOR                       *Dma;
  EFI_ACPI_32_BIT_MEMORY_RANGE_DESCRIPTOR       *Memory;
  EFI_ACPI_32_BIT_FIXED_MEMORY_RANGE_DESCRIPTOR *FixedMemory;

  Count     = 0;
  LastIndex = 0;

  switch (AcpiResource.SmallHeader->Byte) {
    case ACPI_DMA_DESCRIPTOR:
      Dma = (EFI_ACPI_DMA_DESCRIPTOR *) AcpiResource.SmallHeader;
      for (Index = 0; Index < sizeof (Dma->ChannelMask) * 8; Index++) {
        if (Dma->ChannelMask & (1 << Index)) {
          if ((Count > 0) && (LastIndex + 1 == Index)) {
            if (IsaResource != NULL) {
              IsaResource[Count - 1].EndRange ++;
            }
          } else {
            if (IsaResource != NULL) {
              IsaResource[Count].Type       = EfiIsaAcpiResourceDma;
              IsaResource[Count].Attribute  = IsaDmaAttribute (Dma->Information);
              IsaResource[Count].StartRange = Index;
              IsaResource[Count].EndRange   = Index;
            }
            Count ++;
          }

          LastIndex = Index;
        }
      }
      break;

    case ACPI_IO_PORT_DESCRIPTOR:
      Io = (EFI_ACPI_IO_PORT_DESCRIPTOR *) AcpiResource.SmallHeader;
      if (Io->Length != 0) {
        if (IsaResource != NULL) {
          IsaResource[Count].Type       = EfiIsaAcpiResourceIo;
          IsaResource[Count].Attribute  = IsaIoAttribute (Io->Information);
          IsaResource[Count].StartRange = Io->BaseAddressMin;
          IsaResource[Count].EndRange   = Io->BaseAddressMin + Io->Length - 1;
        }
        Count ++;
      }
      break;

    case ACPI_FIXED_LOCATION_IO_PORT_DESCRIPTOR:
      FixedIo = (EFI_ACPI_FIXED_LOCATION_IO_PORT_DESCRIPTOR *) AcpiResource.SmallHeader;
      if (FixedIo->Length != 0) {
        if (IsaResource != NULL) {
          IsaResource[Count].Type       = EfiIsaAcpiResourceIo;
          IsaResource[Count].Attribute  = EFI_ISA_ACPI_IO_DECODE_10_BITS;
          IsaResource[Count].StartRange = FixedIo->BaseAddress;
          IsaResource[Count].EndRange   = FixedIo->BaseAddress + FixedIo->Length - 1;
        }
        Count ++;
      }
      break;

    case ACPI_IRQ_DESCRIPTOR:
    case ACPI_IRQ_NOFLAG_DESCRIPTOR:
      Irq = (EFI_ACPI_IRQ_DESCRIPTOR *) AcpiResource.SmallHeader;
      for (Index = 0; Index < sizeof (Irq->Mask) * 8; Index++) {
        if (Irq->Mask & (1 << Index)) {
          if ((Count > 0) && (LastIndex + 1 == Index)) {
            if (IsaResource != NULL) {
              IsaResource[Count - 1].EndRange ++;
            }
          } else {
            if (IsaResource != NULL) {
              IsaResource[Count].Type       = EfiIsaAcpiResourceInterrupt;
              if (AcpiResource.SmallHeader->Byte == ACPI_IRQ_DESCRIPTOR) {
                IsaResource[Count].Attribute = IsaIrqAttribute (Irq->Information);
              } else {
                IsaResource[Count].Attribute  = EFI_ISA_ACPI_IRQ_TYPE_HIGH_TRUE_EDGE_SENSITIVE;
              }
              IsaResource[Count].StartRange = Index;
              IsaResource[Count].EndRange   = Index;
            }
            Count++;
          }

          LastIndex = Index;
        }
      }
      break;

    case ACPI_32_BIT_MEMORY_RANGE_DESCRIPTOR:
      Memory = (EFI_ACPI_32_BIT_MEMORY_RANGE_DESCRIPTOR *) AcpiResource.LargeHeader;
      if (Memory->Length != 0) {
        if (IsaResource != NULL) {
          IsaResource[Count].Type       = EfiIsaAcpiResourceMemory;
          IsaResource[Count].Attribute  = IsaMemoryAttribute (Memory->Information);
          IsaResource[Count].StartRange = Memory->BaseAddressMin;
          IsaResource[Count].EndRange   = Memory->BaseAddressMin + Memory->Length - 1;
        }
        Count ++;
      }
      break;

    case ACPI_32_BIT_FIXED_MEMORY_RANGE_DESCRIPTOR:
      FixedMemory = (EFI_ACPI_32_BIT_FIXED_MEMORY_RANGE_DESCRIPTOR *) AcpiResource.LargeHeader;
      if (FixedMemory->Length != 0) {
        if (IsaResource != NULL) {
          IsaResource[Count].Type       = EfiIsaAcpiResourceMemory;
          IsaResource[Count].Attribute  = IsaMemoryAttribute (FixedMemory->Information);
          IsaResource[Count].StartRange = FixedMemory->BaseAddress;
          IsaResource[Count].EndRange   = FixedMemory->BaseAddress + FixedMemory->Length - 1;
        }
        Count ++;
      }
      break;

    case ACPI_END_TAG_DESCRIPTOR:
      if (IsaResource != NULL) {
        IsaResource[Count].Type       = EfiIsaAcpiResourceEndOfList;
        IsaResource[Count].Attribute  = 0;
        IsaResource[Count].StartRange = 0;
        IsaResource[Count].EndRange   = 0;
      }
      Count ++;
      break;
  }

  return Count;
}

/**
  Initializes an ISA I/O Instance

  @param[in] IsaIoDevice            The isa device to be initialized.
  @param[in] DevicePath             The device path of the isa device.
  @param[in] Resources              The ACPI resource list.

**/
VOID
InitializeIsaIoInstance (
  IN ISA_IO_DEVICE               *IsaIoDevice,
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN ACPI_RESOURCE_HEADER_PTR    Resources
  )
{
  UINTN                                       Index;
  ACPI_HID_DEVICE_PATH                        *AcpiNode;
  ACPI_RESOURCE_HEADER_PTR                    ResourcePtr;

  //
  // Use the ISA IO Protocol structure template to initialize the ISA IO instance
  //
  CopyMem (
    &IsaIoDevice->IsaIo,
    &mIsaIoInterface,
    sizeof (EFI_ISA_IO_PROTOCOL)
    );

  //
  // Count the resources including the ACPI End Tag
  //
  ResourcePtr = Resources;
  Index       = 0;
  while (ResourcePtr.SmallHeader->Byte != ACPI_END_TAG_DESCRIPTOR) {

    Index += AcpiResourceToIsaResource (ResourcePtr, NULL);

    if (ResourcePtr.SmallHeader->Bits.Type == 0) {
      ResourcePtr.SmallHeader = (ACPI_SMALL_RESOURCE_HEADER *) ((UINT8 *) ResourcePtr.SmallHeader
                              + ResourcePtr.SmallHeader->Bits.Length
                              + sizeof (*ResourcePtr.SmallHeader));
    } else {
      ResourcePtr.LargeHeader = (ACPI_LARGE_RESOURCE_HEADER *) ((UINT8 *) ResourcePtr.LargeHeader
                              + ResourcePtr.LargeHeader->Length
                              + sizeof (*ResourcePtr.LargeHeader));
    }

  }
  //
  // Get the Isa Resource count for ACPI End Tag
  //
  Index += AcpiResourceToIsaResource (ResourcePtr, NULL);

  //
  // Initialize the ResourceList
  //
  IsaIoDevice->IsaIo.ResourceList = AllocatePool (sizeof (EFI_ISA_ACPI_RESOURCE_LIST) + Index * sizeof (EFI_ISA_ACPI_RESOURCE));
  ASSERT (IsaIoDevice->IsaIo.ResourceList != NULL);
  IsaIoDevice->IsaIo.ResourceList->ResourceItem = (EFI_ISA_ACPI_RESOURCE *) (IsaIoDevice->IsaIo.ResourceList + 1);

  AcpiNode = (ACPI_HID_DEVICE_PATH *) ((UINT8 *) DevicePath + GetDevicePathSize (DevicePath) - END_DEVICE_PATH_LENGTH - sizeof (ACPI_HID_DEVICE_PATH));
  IsaIoDevice->IsaIo.ResourceList->Device.HID = AcpiNode->HID;
  IsaIoDevice->IsaIo.ResourceList->Device.UID = AcpiNode->UID;

  ResourcePtr = Resources;
  Index       = 0;
  while (ResourcePtr.SmallHeader->Byte != ACPI_END_TAG_DESCRIPTOR) {

    Index += AcpiResourceToIsaResource (ResourcePtr, &IsaIoDevice->IsaIo.ResourceList->ResourceItem[Index]);

    if (ResourcePtr.SmallHeader->Bits.Type == 0) {
      ResourcePtr.SmallHeader = (ACPI_SMALL_RESOURCE_HEADER *) ((UINT8 *) ResourcePtr.SmallHeader
                              + ResourcePtr.SmallHeader->Bits.Length
                              + sizeof (*ResourcePtr.SmallHeader));
    } else {
      ResourcePtr.LargeHeader = (ACPI_LARGE_RESOURCE_HEADER *) ((UINT8 *) ResourcePtr.LargeHeader
                              + ResourcePtr.LargeHeader->Length
                              + sizeof (*ResourcePtr.LargeHeader));
    }
  }

  //
  // Convert the ACPI End Tag
  //
  AcpiResourceToIsaResource (ResourcePtr, &IsaIoDevice->IsaIo.ResourceList->ResourceItem[Index]);
}

/**
  Performs an ISA I/O Read Cycle

  @param[in]  This              A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in]  Width             Specifies the width of the I/O operation.
  @param[in]  Offset            The offset in ISA I/O space to start the I/O operation.
  @param[in]  Count             The number of I/O operations to perform.
  @param[out] Buffer            The destination buffer to store the results

  @retval EFI_SUCCESS           The data was read from the device sucessfully.
  @retval EFI_UNSUPPORTED       The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
IsaIoIoRead (
  IN  EFI_ISA_IO_PROTOCOL        *This,
  IN  EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN  UINT32                     Offset,
  IN  UINTN                      Count,
  OUT VOID                       *Buffer
  )
{
  EFI_STATUS    Status;
  ISA_IO_DEVICE *IsaIoDevice;

  IsaIoDevice = ISA_IO_DEVICE_FROM_ISA_IO_THIS (This);

  //
  // Verify Isa IO Access
  //
  Status = IsaIoVerifyAccess (
             IsaIoDevice,
             IsaAccessTypeIo,
             Width,
             Count,
             Offset
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IsaIoDevice->PciIo->Io.Read (
                                    IsaIoDevice->PciIo,
                                    (EFI_PCI_IO_PROTOCOL_WIDTH) Width,
                                    EFI_PCI_IO_PASS_THROUGH_BAR,
                                    Offset,
                                    Count,
                                    Buffer
                                    );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_LPC | EFI_IOB_EC_CONTROLLER_ERROR
      );
  }

  return Status;
}

/**
  Performs an ISA I/O Write Cycle

  @param[in] This                A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Width               Specifies the width of the I/O operation.
  @param[in] Offset              The offset in ISA I/O space to start the I/O operation.
  @param[in] Count               The number of I/O operations to perform.
  @param[in] Buffer              The source buffer to write data from

  @retval EFI_SUCCESS            The data was writen to the device sucessfully.
  @retval EFI_UNSUPPORTED        The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
IsaIoIoWrite (
  IN EFI_ISA_IO_PROTOCOL        *This,
  IN EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN UINT32                     Offset,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  )
{
  EFI_STATUS    Status;
  ISA_IO_DEVICE *IsaIoDevice;

  IsaIoDevice = ISA_IO_DEVICE_FROM_ISA_IO_THIS (This);

  //
  // Verify Isa IO Access
  //
  Status = IsaIoVerifyAccess (
             IsaIoDevice,
             IsaAccessTypeIo,
             Width,
             Count,
             Offset
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IsaIoDevice->PciIo->Io.Write (
                                    IsaIoDevice->PciIo,
                                    (EFI_PCI_IO_PROTOCOL_WIDTH) Width,
                                    EFI_PCI_IO_PASS_THROUGH_BAR,
                                    Offset,
                                    Count,
                                    Buffer
                                    );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_LPC | EFI_IOB_EC_CONTROLLER_ERROR
      );
  }

  return Status;
}

/**
  Writes an 8-bit I/O Port

  @param[in] This                A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Offset              The offset in ISA IO space to start the IO operation.
  @param[in] Value               The data to write port.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Parameter is invalid.
  @retval EFI_UNSUPPORTED        The address range specified by Offset is not valid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
WritePort (
  IN EFI_ISA_IO_PROTOCOL  *This,
  IN UINT32               Offset,
  IN UINT8                Value
  )
{
  EFI_STATUS    Status;
  ISA_IO_DEVICE *IsaIoDevice;

  IsaIoDevice = ISA_IO_DEVICE_FROM_ISA_IO_THIS (This);

  Status = IsaIoDevice->PciIo->Io.Write (
                                    IsaIoDevice->PciIo,
                                    EfiPciIoWidthUint8,
                                    EFI_PCI_IO_PASS_THROUGH_BAR,
                                    Offset,
                                    1,
                                    &Value
                                    );
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_LPC | EFI_IOB_EC_CONTROLLER_ERROR
      );
    return Status;
  }

  //
  // Wait for 50 microseconds to take affect.
  //
  gBS->Stall (50);

  return EFI_SUCCESS;
}

/**
  Writes I/O operation base address and count number to a 8 bit I/O Port.

  @param[in] This                A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] AddrOffset          The address' offset.
  @param[in] PageOffset          The page's offest.
  @param[in] CountOffset         The count's offset.
  @param[in] BaseAddress         The base address.
  @param[in] Count               The number of I/O operations to perform.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Parameter is invalid.
  @retval EFI_UNSUPPORTED        The address range specified by these Offsets and Count is not valid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
WriteDmaPort (
  IN EFI_ISA_IO_PROTOCOL  *This,
  IN UINT32               AddrOffset,
  IN UINT32               PageOffset,
  IN UINT32               CountOffset,
  IN UINT32               BaseAddress,
  IN UINT16               Count
  )
{
  EFI_STATUS  Status;

  Status = WritePort (This, AddrOffset, (UINT8) (BaseAddress & 0xff));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WritePort (This, AddrOffset, (UINT8) ((BaseAddress >> 8) & 0xff));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WritePort (This, PageOffset, (UINT8) ((BaseAddress >> 16) & 0xff));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WritePort (This, CountOffset, (UINT8) (Count & 0xff));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WritePort (This, CountOffset, (UINT8) ((Count >> 8) & 0xff));
  return Status;
}

/**
  Unmaps a memory region for DMA

  @param[in] This           A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Mapping        The mapping value returned from EFI_ISA_IO.Map().

  @retval EFI_SUCCESS       The range was unmapped.
  @retval EFI_DEVICE_ERROR  The data was not committed to the target system memory.
**/
EFI_STATUS
EFIAPI
IsaIoUnmap (
  IN EFI_ISA_IO_PROTOCOL  *This,
  IN VOID                 *Mapping
  )
{
  ISA_MAP_INFO  *IsaMapInfo;

  //
  // Check if DMA is supported.
  //
  if ((PcdGet8 (PcdIsaBusSupportedFeatures) & PCD_ISA_BUS_SUPPORT_DMA) == 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // See if the Map() operation associated with this Unmap() required a mapping
  // buffer.If a mapping buffer was not required, then this function simply
  // returns EFI_SUCCESS.
  //
  if (Mapping != NULL) {
    //
    // Get the MAP_INFO structure from Mapping
    //
    IsaMapInfo = (ISA_MAP_INFO *) Mapping;

    //
    // If this is a write operation from the Agent's point of view,
    // then copy the contents of the mapped buffer into the real buffer
    // so the processor can read the contents of the real buffer.
    //
    if (IsaMapInfo->Operation == EfiIsaIoOperationBusMasterWrite) {
      CopyMem (
        (VOID *) (UINTN) IsaMapInfo->HostAddress,
        (VOID *) (UINTN) IsaMapInfo->MappedHostAddress,
        IsaMapInfo->NumberOfBytes
        );
    }
    //
    // Free the mapped buffer and the MAP_INFO structure.
    //
    gBS->FreePages (IsaMapInfo->MappedHostAddress, IsaMapInfo->NumberOfPages);
    FreePool (IsaMapInfo);
  }

  return EFI_SUCCESS;
}

/**
  Flushes any posted write data to the system memory.

  @param[in] This             A pointer to the EFI_ISA_IO_PROTOCOL instance.

  @retval  EFI_SUCCESS        The buffers were flushed.
  @retval  EFI_DEVICE_ERROR   The buffers were not flushed due to a hardware error.
**/
EFI_STATUS
EFIAPI
IsaIoFlush (
  IN EFI_ISA_IO_PROTOCOL  *This
  )
{
  EFI_STATUS    Status;
  ISA_IO_DEVICE *IsaIoDevice;

  IsaIoDevice = ISA_IO_DEVICE_FROM_ISA_IO_THIS (This);

  Status = IsaIoDevice->PciIo->Flush (IsaIoDevice->PciIo);

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_LPC | EFI_IOB_EC_CONTROLLER_ERROR
      );
  }

  return Status;
}

/**
  Performs an ISA Memory Read Cycle

  @param[in]  This               A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in]  Width              Specifies the width of the memory operation.
  @param[in]  Offset             The offset in ISA memory space to start the memory operation.
  @param[in]  Count              The number of memory operations to perform.
  @param[out] Buffer             The destination buffer to store the results

  @retval EFI_SUCCESS            The data was read from the device successfully.
  @retval EFI_UNSUPPORTED        The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
IsaIoMemRead (
  IN  EFI_ISA_IO_PROTOCOL        *This,
  IN  EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN  UINT32                     Offset,
  IN  UINTN                      Count,
  OUT VOID                       *Buffer
  )
{
  EFI_STATUS    Status;
  ISA_IO_DEVICE *IsaIoDevice;

  //
  // Check if ISA memory is supported.
  //
  if ((PcdGet8 (PcdIsaBusSupportedFeatures) & PCD_ISA_BUS_SUPPORT_ISA_MEMORY) == 0) {
    return EFI_UNSUPPORTED;
  }

  IsaIoDevice = ISA_IO_DEVICE_FROM_ISA_IO_THIS (This);

  //
  // Verify the Isa Io Access
  //
  Status = IsaIoVerifyAccess (
             IsaIoDevice,
             IsaAccessTypeMem,
             Width,
             Count,
             Offset
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IsaIoDevice->PciIo->Mem.Read (
                                     IsaIoDevice->PciIo,
                                     (EFI_PCI_IO_PROTOCOL_WIDTH) Width,
                                     EFI_PCI_IO_PASS_THROUGH_BAR,
                                     Offset,
                                     Count,
                                     Buffer
                                     );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_LPC | EFI_IOB_EC_CONTROLLER_ERROR
      );
  }

  return Status;
}

/**
  Performs an ISA Memory Write Cycle

  @param[in] This                A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Width               Specifies the width of the memory operation.
  @param[in] Offset              The offset in ISA memory space to start the memory operation.
  @param[in] Count               The number of memory operations to perform.
  @param[in] Buffer              The source buffer to write data from

  @retval EFI_SUCCESS            The data was written to the device sucessfully.
  @retval EFI_UNSUPPORTED        The Offset is not valid for this device.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
IsaIoMemWrite (
  IN EFI_ISA_IO_PROTOCOL        *This,
  IN EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN UINT32                     Offset,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  )
{
  EFI_STATUS    Status;
  ISA_IO_DEVICE *IsaIoDevice;

  //
  // Check if ISA memory is supported.
  //
  if ((PcdGet8 (PcdIsaBusSupportedFeatures) & PCD_ISA_BUS_SUPPORT_ISA_MEMORY) == 0) {
    return EFI_UNSUPPORTED;
  }

  IsaIoDevice = ISA_IO_DEVICE_FROM_ISA_IO_THIS (This);

  //
  // Verify Isa IO Access
  //
  Status = IsaIoVerifyAccess (
             IsaIoDevice,
             IsaAccessTypeMem,
             Width,
             Count,
             Offset
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IsaIoDevice->PciIo->Mem.Write (
                                     IsaIoDevice->PciIo,
                                     (EFI_PCI_IO_PROTOCOL_WIDTH) Width,
                                     EFI_PCI_IO_PASS_THROUGH_BAR,
                                     Offset,
                                     Count,
                                     Buffer
                                     );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_LPC | EFI_IOB_EC_CONTROLLER_ERROR
      );
  }

  return Status;
}

/**
  Copy one region of ISA memory space to another region of ISA memory space on the ISA controller.

  @param[in]  This               A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in]  Width              Specifies the width of the memory copy operation.
  @param[in]  DestOffset         The offset of the destination
  @param[in]  SrcOffset          The offset of the source
  @param[in]  Count              The number of memory copy  operations to perform

  @retval EFI_SUCCESS            The data was copied sucessfully.
  @retval EFI_UNSUPPORTED        The DestOffset or SrcOffset is not valid for this device.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
IsaIoCopyMem (
  IN EFI_ISA_IO_PROTOCOL        *This,
  IN EFI_ISA_IO_PROTOCOL_WIDTH  Width,
  IN UINT32                     DestOffset,
  IN UINT32                     SrcOffset,
  IN UINTN                      Count
  )
{
  EFI_STATUS    Status;
  ISA_IO_DEVICE *IsaIoDevice;

  //
  // Check if ISA memory is supported.
  //
  if ((PcdGet8 (PcdIsaBusSupportedFeatures) & PCD_ISA_BUS_SUPPORT_ISA_MEMORY) == 0) {
    return EFI_UNSUPPORTED;
  }

  IsaIoDevice = ISA_IO_DEVICE_FROM_ISA_IO_THIS (This);

  //
  // Verify Isa IO Access for destination and source
  //
  Status = IsaIoVerifyAccess (
             IsaIoDevice,
             IsaAccessTypeMem,
             Width,
             Count,
             DestOffset
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IsaIoVerifyAccess (
             IsaIoDevice,
             IsaAccessTypeMem,
             Width,
             Count,
             SrcOffset
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IsaIoDevice->PciIo->CopyMem (
                                 IsaIoDevice->PciIo,
                                 (EFI_PCI_IO_PROTOCOL_WIDTH) Width,
                                 EFI_PCI_IO_PASS_THROUGH_BAR,
                                 DestOffset,
                                 EFI_PCI_IO_PASS_THROUGH_BAR,
                                 SrcOffset,
                                 Count
                                 );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_LPC | EFI_IOB_EC_CONTROLLER_ERROR
      );
  }

  return Status;
}

/**
  Maps a memory region for DMA, note this implementation
  only supports slave read/write operation to save code size.

  @param This                    A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param Operation               Indicates the type of DMA (slave or bus master), and if
                                 the DMA operation is going to read or write to system memory.
  @param ChannelNumber           The slave channel number to use for this DMA operation.
                                 If Operation and ChannelAttributes shows that this device
                                 performs bus mastering DMA, then this field is ignored.
                                 The legal range for this field is 0..7.
  @param ChannelAttributes       The attributes of the DMA channel to use for this DMA operation
  @param HostAddress             The system memory address to map to the device.
  @param NumberOfBytes           On input the number of bytes to map.  On output the number
                                 of bytes that were mapped.
  @param DeviceAddress           The resulting map address for the bus master device to use
                                 to access the hosts HostAddress.
  @param Mapping                 A resulting value to pass to EFI_ISA_IO.Unmap().

  @retval EFI_SUCCESS            The range was mapped for the returned NumberOfBytes.
  @retval EFI_INVALID_PARAMETER  The Operation or HostAddress is undefined.
  @retval EFI_UNSUPPORTED        The HostAddress can not be mapped as a common buffer.
  @retval EFI_DEVICE_ERROR       The system hardware could not map the requested address.
  @retval EFI_OUT_OF_RESOURCES   The memory pages could not be allocated.
**/
EFI_STATUS
IsaIoMapOnlySupportSlaveReadWrite (
  IN     EFI_ISA_IO_PROTOCOL            *This,
  IN     EFI_ISA_IO_PROTOCOL_OPERATION  Operation,
  IN     UINT8                          ChannelNumber  OPTIONAL,
  IN     UINT32                         ChannelAttributes,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;
  ISA_MAP_INFO          *IsaMapInfo;
  UINT8                 DmaMode;
  UINTN                 MaxNumberOfBytes;
  UINT32                BaseAddress;
  UINT16                Count;
  UINT8                 DmaMask;
  UINT8                 DmaClear;
  UINT8                 DmaChannelMode;

  if ((NULL == This) ||
      (NULL == HostAddress) ||
      (NULL == NumberOfBytes) ||
      (NULL == DeviceAddress) ||
      (NULL == Mapping)
      ) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Initialize the return values to their defaults
  //
  *Mapping = NULL;

  //
  // Make sure the Operation parameter is valid.
  // Light IsaIo only supports two operations.
  //
  if (!(Operation == EfiIsaIoOperationSlaveRead ||
        Operation == EfiIsaIoOperationSlaveWrite)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ChannelNumber >= 4) {
    //
    // The Light IsaIo doesn't support channelNumber larger than 4.
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Map the HostAddress to a DeviceAddress.
  //
  PhysicalAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) HostAddress;
  if ((PhysicalAddress + *NumberOfBytes) > BASE_16MB) {
    //
    // Common Buffer operations can not be remapped.  If the common buffer
    // is above 16MB, then it is not possible to generate a mapping, so return
    // an error.
    //
    if (Operation == EfiIsaIoOperationBusMasterCommonBuffer) {
      return EFI_UNSUPPORTED;
    }
    //
    // Allocate an ISA_MAP_INFO structure to remember the mapping when Unmap()
    // is called later.
    //
    IsaMapInfo = AllocatePool (sizeof (ISA_MAP_INFO));
    if (IsaMapInfo == NULL) {
      *NumberOfBytes = 0;
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Return a pointer to the MAP_INFO structure in Mapping
    //
    *Mapping = IsaMapInfo;

    //
    // Initialize the MAP_INFO structure
    //
    IsaMapInfo->Operation         = Operation;
    IsaMapInfo->NumberOfBytes     = *NumberOfBytes;
    IsaMapInfo->NumberOfPages     = EFI_SIZE_TO_PAGES (*NumberOfBytes);
    IsaMapInfo->HostAddress       = PhysicalAddress;
    IsaMapInfo->MappedHostAddress = BASE_16MB - 1;

    //
    // Allocate a buffer below 16MB to map the transfer to.
    //
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiBootServicesData,
                    IsaMapInfo->NumberOfPages,
                    &IsaMapInfo->MappedHostAddress
                    );
    if (EFI_ERROR (Status)) {
      FreePool (IsaMapInfo);
      *NumberOfBytes  = 0;
      *Mapping        = NULL;
      return Status;
    }
    //
    // If this is a read operation from the DMA agents's point of view,
    // then copy the contents of the real buffer into the mapped buffer
    // so the DMA agent can read the contents of the real buffer.
    //
    if (Operation == EfiIsaIoOperationSlaveRead) {
      CopyMem (
        (VOID *) (UINTN) IsaMapInfo->MappedHostAddress,
        (VOID *) (UINTN) IsaMapInfo->HostAddress,
        IsaMapInfo->NumberOfBytes
        );
    }
    //
    // The DeviceAddress is the address of the maped buffer below 16 MB
    //
    *DeviceAddress = IsaMapInfo->MappedHostAddress;
  } else {
    //
    // The transfer is below 16 MB, so the DeviceAddress is simply the
    // HostAddress
    //
    *DeviceAddress = PhysicalAddress;
  }

  //
  // Figure out what to program into the DMA Channel Mode Register
  //
  DmaMode = (UINT8) (B_8237_DMA_CHMODE_INCREMENT | (ChannelNumber & 0x03));
  if (Operation == EfiIsaIoOperationSlaveRead) {
    DmaMode |= V_8237_DMA_CHMODE_MEM2IO;
  } else {
    DmaMode |= V_8237_DMA_CHMODE_IO2MEM;
  }
  //
  // We only support EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SINGLE_MODE in simplified IsaIo
  //
  DmaMode |= V_8237_DMA_CHMODE_SINGLE;

  //
  // A Slave DMA transfer can not cross a 64K boundary.
  // Compute *NumberOfBytes based on this restriction.
  //
  MaxNumberOfBytes = 0x10000 - ((UINT32) (*DeviceAddress) & 0xffff);
  if (*NumberOfBytes > MaxNumberOfBytes) {
    *NumberOfBytes = MaxNumberOfBytes;
  }
  //
  // Compute the values to program into the BaseAddress and Count registers
  // of the Slave DMA controller
  //
  BaseAddress = (UINT32) (*DeviceAddress);
  Count       = (UINT16) (*NumberOfBytes - 1);
  //
  // Program the DMA Write Single Mask Register for ChannelNumber
  // Clear the DMA Byte Pointer Register
  //
  DmaMask         = R_8237_DMA_WRSMSK_CH0_3;
  DmaClear        = R_8237_DMA_CBPR_CH0_3;
  DmaChannelMode  = R_8237_DMA_CHMODE_CH0_3;

  Status = WritePort (
             This,
             DmaMask,
             (UINT8) (B_8237_DMA_WRSMSK_CMS | (ChannelNumber & 0x03))
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WritePort (
             This,
             DmaClear,
             (UINT8) (B_8237_DMA_WRSMSK_CMS | (ChannelNumber & 0x03))
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WritePort (This, DmaChannelMode, DmaMode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WriteDmaPort (
             This,
             mDmaRegisters[ChannelNumber].Address,
             mDmaRegisters[ChannelNumber].Page,
             mDmaRegisters[ChannelNumber].Count,
             BaseAddress,
             Count
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WritePort (
             This,
             DmaMask,
             (UINT8) (ChannelNumber & 0x03)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Maps a memory region for DMA. This implementation implement the
  the full mapping support.

  @param This                    A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param Operation               Indicates the type of DMA (slave or bus master), and if
                                 the DMA operation is going to read or write to system memory.
  @param ChannelNumber           The slave channel number to use for this DMA operation.
                                 If Operation and ChannelAttributes shows that this device
                                 performs bus mastering DMA, then this field is ignored.
                                 The legal range for this field is 0..7.
  @param ChannelAttributes       The attributes of the DMA channel to use for this DMA operation
  @param HostAddress             The system memory address to map to the device.
  @param NumberOfBytes           On input the number of bytes to map.  On output the number
                                 of bytes that were mapped.
  @param DeviceAddress           The resulting map address for the bus master device to use
                                 to access the hosts HostAddress.
  @param Mapping                 A resulting value to pass to EFI_ISA_IO.Unmap().

  @retval EFI_SUCCESS           - The range was mapped for the returned NumberOfBytes.
  @retval EFI_INVALID_PARAMETER - The Operation or HostAddress is undefined.
  @retval EFI_UNSUPPORTED       - The HostAddress can not be mapped as a common buffer.
  @retval EFI_DEVICE_ERROR      - The system hardware could not map the requested address.
  @retval EFI_OUT_OF_RESOURCES  - The memory pages could not be allocated.
**/
EFI_STATUS
IsaIoMapFullSupport (
  IN     EFI_ISA_IO_PROTOCOL                                  *This,
  IN     EFI_ISA_IO_PROTOCOL_OPERATION                        Operation,
  IN     UINT8                                                ChannelNumber         OPTIONAL,
  IN     UINT32                                               ChannelAttributes,
  IN     VOID                                                 *HostAddress,
  IN OUT UINTN                                                *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                                 *DeviceAddress,
  OUT    VOID                                                 **Mapping
  )
{
  EFI_STATUS            Status;
  BOOLEAN               Master;
  BOOLEAN               Read;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;
  ISA_MAP_INFO          *IsaMapInfo;
  UINT8                 DmaMode;
  UINTN                 MaxNumberOfBytes;
  UINT32                BaseAddress;
  UINT16                Count;
  UINT8                 DmaMask;
  UINT8                 DmaClear;
  UINT8                 DmaChannelMode;

  if ((NULL == This) ||
      (NULL == HostAddress) ||
      (NULL == NumberOfBytes) ||
      (NULL == DeviceAddress) ||
      (NULL == Mapping)
      ) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Initialize the return values to their defaults
  //
  *Mapping = NULL;

  //
  // Make sure the Operation parameter is valid
  //
  if ((UINT32)Operation >= EfiIsaIoOperationMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (ChannelNumber >= 8) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // See if this is a Slave DMA Operation
  //
  Master  = TRUE;
  Read    = FALSE;
  if (Operation == EfiIsaIoOperationSlaveRead) {
    Operation = EfiIsaIoOperationBusMasterRead;
    Master    = FALSE;
    Read      = TRUE;
  }

  if (Operation == EfiIsaIoOperationSlaveWrite) {
    Operation = EfiIsaIoOperationBusMasterWrite;
    Master    = FALSE;
    Read      = FALSE;
  }

  if (!Master) {
    //
    // Make sure that ChannelNumber is a valid channel number
    // Channel 4 is used to cascade, so it is illegal.
    //
    if (ChannelNumber == 4 || ChannelNumber > 7) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // This implementation only support COMPATIBLE DMA Transfers
    //
    if ((ChannelAttributes & EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_COMPATIBLE) == 0) {
      return EFI_INVALID_PARAMETER;
    }

    if ((ChannelAttributes &
         (EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_A |
          EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_B |
          EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SPEED_C)) != 0) {
      return EFI_INVALID_PARAMETER;
    }

    if (ChannelNumber < 4) {
      //
      // If this is Channel 0..3, then the width must be 8 bit
      //
      if (((ChannelAttributes & EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_WIDTH_8) == 0) ||
          ((ChannelAttributes & EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_WIDTH_16) != 0)
          ) {
        return EFI_INVALID_PARAMETER;
      }
    } else {
      //
      // If this is Channel 4..7, then the width must be 16 bit
      //
      if (((ChannelAttributes & EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_WIDTH_8) != 0) ||
          ((ChannelAttributes & EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_WIDTH_16) == 0)) {
        return EFI_INVALID_PARAMETER;
      }
    }
    //
    // Either Demand Mode or Single Mode must be selected, but not both
    //
    if ((ChannelAttributes & EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SINGLE_MODE) != 0) {
      if ((ChannelAttributes & EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_DEMAND_MODE) != 0) {
        return EFI_INVALID_PARAMETER;
      }
    } else {
      if ((ChannelAttributes & EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_DEMAND_MODE) == 0) {
        return EFI_INVALID_PARAMETER;
      }
    }
  }
  //
  // Map the HostAddress to a DeviceAddress.
  //
  PhysicalAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) HostAddress;
  if ((PhysicalAddress +*NumberOfBytes) > BASE_16MB) {
    //
    // Common Buffer operations can not be remapped.  If the common buffer
    // is above 16MB, then it is not possible to generate a mapping, so return
    // an error.
    //
    if (Operation == EfiIsaIoOperationBusMasterCommonBuffer) {
      return EFI_UNSUPPORTED;
    }
    //
    // Allocate an ISA_MAP_INFO structure to remember the mapping when Unmap()
    // is called later.
    //
    IsaMapInfo = AllocatePool (sizeof (ISA_MAP_INFO));
    if (IsaMapInfo == NULL) {
      *NumberOfBytes = 0;
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Return a pointer to the MAP_INFO structure in Mapping
    //
    *Mapping = IsaMapInfo;

    //
    // Initialize the MAP_INFO structure
    //
    IsaMapInfo->Operation         = Operation;
    IsaMapInfo->NumberOfBytes     = *NumberOfBytes;
    IsaMapInfo->NumberOfPages     = EFI_SIZE_TO_PAGES (*NumberOfBytes);
    IsaMapInfo->HostAddress       = PhysicalAddress;
    IsaMapInfo->MappedHostAddress = BASE_16MB - 1;

    //
    // Allocate a buffer below 16MB to map the transfer to.
    //
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiBootServicesData,
                    IsaMapInfo->NumberOfPages,
                    &IsaMapInfo->MappedHostAddress
                    );
    if (EFI_ERROR (Status)) {
      FreePool (IsaMapInfo);
      *NumberOfBytes  = 0;
      *Mapping        = NULL;
      return Status;
    }
    //
    // If this is a read operation from the DMA agents's point of view,
    // then copy the contents of the real buffer into the mapped buffer
    // so the DMA agent can read the contents of the real buffer.
    //
    if (Operation == EfiIsaIoOperationBusMasterRead) {
      CopyMem (
        (VOID *) (UINTN) IsaMapInfo->MappedHostAddress,
        (VOID *) (UINTN) IsaMapInfo->HostAddress,
        IsaMapInfo->NumberOfBytes
        );
    }
    //
    // The DeviceAddress is the address of the maped buffer below 16 MB
    //
    *DeviceAddress = IsaMapInfo->MappedHostAddress;
  } else {
    //
    // The transfer is below 16 MB, so the DeviceAddress is simply the
    // HostAddress
    //
    *DeviceAddress = PhysicalAddress;
  }
  //
  // If this is a Bus Master operation then return
  //
  if (Master) {
    return EFI_SUCCESS;
  }
  //
  // Figure out what to program into the DMA Channel Mode Register
  //
  DmaMode = (UINT8) (B_8237_DMA_CHMODE_INCREMENT | (ChannelNumber & 0x03));
  if (Read) {
    DmaMode |= V_8237_DMA_CHMODE_MEM2IO;
  } else {
    DmaMode |= V_8237_DMA_CHMODE_IO2MEM;
  }

  if ((ChannelAttributes & EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_AUTO_INITIALIZE) != 0) {
    DmaMode |= B_8237_DMA_CHMODE_AE;
  }

  if ((ChannelAttributes & EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_DEMAND_MODE) != 0) {
    DmaMode |= V_8237_DMA_CHMODE_DEMAND;
  }

  if ((ChannelAttributes & EFI_ISA_IO_SLAVE_DMA_ATTRIBUTE_SINGLE_MODE) != 0) {
    DmaMode |= V_8237_DMA_CHMODE_SINGLE;
  }
  //
  // A Slave DMA transfer can not cross a 64K boundary.
  // Compute *NumberOfBytes based on this restriction.
  //
  MaxNumberOfBytes = 0x10000 - ((UINT32) (*DeviceAddress) & 0xffff);
  if (*NumberOfBytes > MaxNumberOfBytes) {
    *NumberOfBytes = MaxNumberOfBytes;
  }
  //
  // Compute the values to program into the BaseAddress and Count registers
  // of the Slave DMA controller
  //
  if (ChannelNumber < 4) {
    BaseAddress = (UINT32) (*DeviceAddress);
    Count       = (UINT16) (*NumberOfBytes - 1);
  } else {
    BaseAddress = (UINT32) (((UINT32) (*DeviceAddress) & 0xff0000) | (((UINT32) (*DeviceAddress) & 0xffff) >> 1));
    Count       = (UINT16) ((*NumberOfBytes - 1) >> 1);
  }
  //
  // Program the DMA Write Single Mask Register for ChannelNumber
  // Clear the DMA Byte Pointer Register
  //
  if (ChannelNumber < 4) {
    DmaMask         = R_8237_DMA_WRSMSK_CH0_3;
    DmaClear        = R_8237_DMA_CBPR_CH0_3;
    DmaChannelMode  = R_8237_DMA_CHMODE_CH0_3;
  } else {
    DmaMask         = R_8237_DMA_WRSMSK_CH4_7;
    DmaClear        = R_8237_DMA_CBPR_CH4_7;
    DmaChannelMode  = R_8237_DMA_CHMODE_CH4_7;
  }

  Status = WritePort (
             This,
             DmaMask,
             (UINT8) (B_8237_DMA_WRSMSK_CMS | (ChannelNumber & 0x03))
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WritePort (
             This,
             DmaClear,
             (UINT8) (B_8237_DMA_WRSMSK_CMS | (ChannelNumber & 0x03))
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WritePort (This, DmaChannelMode, DmaMode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WriteDmaPort (
             This,
             mDmaRegisters[ChannelNumber].Address,
             mDmaRegisters[ChannelNumber].Page,
             mDmaRegisters[ChannelNumber].Count,
             BaseAddress,
             Count
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WritePort (
             This,
             DmaMask,
             (UINT8) (ChannelNumber & 0x03)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Maps a memory region for DMA

  @param This                    A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param Operation               Indicates the type of DMA (slave or bus master), and if
                                 the DMA operation is going to read or write to system memory.
  @param ChannelNumber           The slave channel number to use for this DMA operation.
                                 If Operation and ChannelAttributes shows that this device
                                 performs bus mastering DMA, then this field is ignored.
                                 The legal range for this field is 0..7.
  @param ChannelAttributes       The attributes of the DMA channel to use for this DMA operation
  @param HostAddress             The system memory address to map to the device.
  @param NumberOfBytes           On input the number of bytes to map.  On output the number
                                 of bytes that were mapped.
  @param DeviceAddress           The resulting map address for the bus master device to use
                                 to access the hosts HostAddress.
  @param Mapping                 A resulting value to pass to EFI_ISA_IO.Unmap().

  @retval EFI_SUCCESS            The range was mapped for the returned NumberOfBytes.
  @retval EFI_INVALID_PARAMETER  The Operation or HostAddress is undefined.
  @retval EFI_UNSUPPORTED        The HostAddress can not be mapped as a common buffer.
  @retval EFI_DEVICE_ERROR       The system hardware could not map the requested address.
  @retval EFI_OUT_OF_RESOURCES   The memory pages could not be allocated.
**/
EFI_STATUS
EFIAPI
IsaIoMap (
  IN     EFI_ISA_IO_PROTOCOL            *This,
  IN     EFI_ISA_IO_PROTOCOL_OPERATION  Operation,
  IN     UINT8                          ChannelNumber  OPTIONAL,
  IN     UINT32                         ChannelAttributes,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
  )
{
  //
  // Check if DMA is supported.
  //
  if ((PcdGet8 (PcdIsaBusSupportedFeatures) & PCD_ISA_BUS_SUPPORT_DMA) == 0) {
    return EFI_UNSUPPORTED;
  }
  //
  // Set Feature Flag PcdIsaBusSupportBusMaster to FALSE to disable support for
  // ISA Bus Master.
  //
  // So we just return EFI_UNSUPPORTED for these functions.
  //
  if ((PcdGet8 (PcdIsaBusSupportedFeatures) & PCD_ISA_BUS_ONLY_SUPPORT_SLAVE_DMA) != 0) {
    return IsaIoMapOnlySupportSlaveReadWrite (
             This,
             Operation,
             ChannelNumber,
             ChannelAttributes,
             HostAddress,
             NumberOfBytes,
             DeviceAddress,
             Mapping
             );

  } else {
    return IsaIoMapFullSupport (
             This,
             Operation,
             ChannelNumber,
             ChannelAttributes,
             HostAddress,
             NumberOfBytes,
             DeviceAddress,
             Mapping
             );
  }
}

/**
  Allocates pages that are suitable for an EfiIsaIoOperationBusMasterCommonBuffer mapping.

  @param[in]  This               A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in]  Type               The type allocation to perform.
  @param[in]  MemoryType         The type of memory to allocate.
  @param[in]  Pages              The number of pages to allocate.
  @param[out] HostAddress        A pointer to store the base address of the allocated range.
  @param[in]  Attributes         The requested bit mask of attributes for the allocated range.

  @retval EFI_SUCCESS            The requested memory pages were allocated.
  @retval EFI_INVALID_PARAMETER  Type is invalid or MemoryType is invalid or HostAddress is NULL
  @retval EFI_UNSUPPORTED        Attributes is unsupported or the memory range specified
                                 by HostAddress, Pages, and Type is not available for common buffer use.
  @retval EFI_OUT_OF_RESOURCES   The memory pages could not be allocated.
**/
EFI_STATUS
EFIAPI
IsaIoAllocateBuffer (
  IN  EFI_ISA_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE    Type,
  IN  EFI_MEMORY_TYPE      MemoryType,
  IN  UINTN                Pages,
  OUT VOID                 **HostAddress,
  IN  UINT64               Attributes
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;

  //
  // Set Feature Flag PcdIsaBusOnlySupportSlaveDma to FALSE to disable support for
  // ISA Bus Master.
  // Or unset Feature Flag PcdIsaBusSupportDma to disable support for ISA DMA.
  //
  if (((PcdGet8 (PcdIsaBusSupportedFeatures) & PCD_ISA_BUS_SUPPORT_DMA) == 0) ||
      ((PcdGet8 (PcdIsaBusSupportedFeatures) & PCD_ISA_BUS_ONLY_SUPPORT_SLAVE_DMA) != 0)) {
    return EFI_UNSUPPORTED;
  }

  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINT32)Type >= MaxAllocateType) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // The only valid memory types are EfiBootServicesData and EfiRuntimeServicesData
  //
  if (MemoryType != EfiBootServicesData && MemoryType != EfiRuntimeServicesData) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Attributes & ~(EFI_ISA_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE | EFI_ISA_IO_ATTRIBUTE_MEMORY_CACHED)) != 0) {
    return EFI_UNSUPPORTED;
  }

  PhysicalAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) (BASE_16MB - 1);
  if (Type == AllocateAddress) {
    if ((UINTN) (*HostAddress) >= BASE_16MB) {
      return EFI_UNSUPPORTED;
    } else {
      PhysicalAddress = (UINTN) (*HostAddress);
    }
  }

  if (Type == AllocateAnyPages) {
    Type = AllocateMaxAddress;
  }

  Status = gBS->AllocatePages (Type, MemoryType, Pages, &PhysicalAddress);
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_LPC | EFI_IOB_EC_CONTROLLER_ERROR
      );
    return Status;
  }

  *HostAddress = (VOID *) (UINTN) PhysicalAddress;
  return Status;
}

/**
  Frees memory that was allocated with EFI_ISA_IO.AllocateBuffer().

  @param[in] This                A pointer to the EFI_ISA_IO_PROTOCOL instance.
  @param[in] Pages               The number of pages to free.
  @param[in] HostAddress         The base address of the allocated range.

  @retval EFI_SUCCESS            The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER  The memory was not allocated with EFI_ISA_IO.AllocateBufer().
**/
EFI_STATUS
EFIAPI
IsaIoFreeBuffer (
  IN EFI_ISA_IO_PROTOCOL  *This,
  IN UINTN                Pages,
  IN VOID                 *HostAddress
  )
{
  EFI_STATUS  Status;

  //
  // Set Feature Flag PcdIsaBusOnlySupportSlaveDma to FALSE to disable support for
  // ISA Bus Master.
  // Or unset Feature Flag PcdIsaBusSupportDma to disable support for ISA DMA.
  //
  if (((PcdGet8 (PcdIsaBusSupportedFeatures) & PCD_ISA_BUS_SUPPORT_DMA) == 0) ||
      ((PcdGet8 (PcdIsaBusSupportedFeatures) & PCD_ISA_BUS_ONLY_SUPPORT_SLAVE_DMA) != 0)) {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->FreePages (
                  (EFI_PHYSICAL_ADDRESS) (UINTN) HostAddress,
                  Pages
                  );
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_LPC | EFI_IOB_EC_CONTROLLER_ERROR
      );
  }

  return Status;
}


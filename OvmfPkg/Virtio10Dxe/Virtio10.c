/** @file
  A non-transitional driver for VirtIo 1.0 PCI devices.

  Copyright (C) 2016, Red Hat, Inc.
  Copyright (C) 2017, AMD Inc, All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Virtio.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/VirtioDevice.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciCapLib.h>
#include <Library/PciCapPciIoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "Virtio10.h"


//
// Utility functions
//

/**
  Transfer data between the caller and a register in a virtio-1.0 register
  block.

  @param[in]     PciIo        The EFI_PCI_IO_PROTOCOL instance that represents
                              the device.

  @param[in]     Config       The "fat pointer" structure that identifies the
                              register block to access.

  @param[in]     Write        TRUE if the register should be written, FALSE if
                              the register should be read.

  @param[in]     FieldOffset  The offset of the register within the register
                              block.

  @param[in]     FieldSize    The size of the register within the register
                              block. Can be one of 1, 2, 4 and 8. Accesses to
                              8-byte registers are broken up into two 4-byte
                              accesses.

  @param[in,out] Buffer       When Write is TRUE, the register is written with
                              data from Buffer. When Write is FALSE, the caller
                              receives the register value into Buffer.

  @retval  EFI_SUCCESS            Register access successful.

  @retval  EFI_INVALID_PARAMETER  The register block pointed-to by Config
                                  doesn't exist; or FieldOffset and FieldSize
                                  would overflow the register block; or
                                  FieldSize is invalid.

  @return                         Error codes from
                                  EFI_PCI_IO_PROTOCOL.(Io|Mem).(Read|Write)
                                  member functions.
**/
STATIC
EFI_STATUS
Virtio10Transfer (
  IN     EFI_PCI_IO_PROTOCOL *PciIo,
  IN     VIRTIO_1_0_CONFIG   *Config,
  IN     BOOLEAN             Write,
  IN     UINTN               FieldOffset,
  IN     UINTN               FieldSize,
  IN OUT VOID                *Buffer
  )
{
  UINTN                      Count;
  EFI_PCI_IO_PROTOCOL_WIDTH  Width;
  EFI_PCI_IO_PROTOCOL_ACCESS *BarType;
  EFI_PCI_IO_PROTOCOL_IO_MEM Access;

  if (!Config->Exists ||
      FieldSize > Config->Length ||
      FieldOffset > Config->Length - FieldSize) {
    return EFI_INVALID_PARAMETER;
  }

  Count = 1;
  switch (FieldSize) {
    case 1:
      Width = EfiPciIoWidthUint8;
      break;

    case 2:
      Width = EfiPciIoWidthUint16;
      break;

    case 8:
      Count = 2;
      //
      // fall through
      //

    case 4:
      Width = EfiPciIoWidthUint32;
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  BarType = (Config->BarType == Virtio10BarTypeMem) ? &PciIo->Mem : &PciIo->Io;
  Access = Write ? BarType->Write : BarType->Read;

  return Access (PciIo, Width, Config->Bar, Config->Offset + FieldOffset,
           Count, Buffer);
}


/**
  Determine if a PCI BAR is IO or MMIO.

  @param[in]  PciIo     The EFI_PCI_IO_PROTOCOL instance that represents the
                        device.

  @param[in]  BarIndex  The number of the BAR whose type the caller is
                        interested in.

  @param[out] BarType   On output, a VIRTIO_1_0_BAR_TYPE value that gives the
                        type of the BAR.

  @retval EFI_SUCCESS      The BAR type has been recognized and stored in
                           BarType.

  @retval EFI_UNSUPPORTED  The BAR type couldn't be identified.

  @return                  Error codes from
                           EFI_PCI_IO_PROTOCOL.GetBarAttributes().
**/
STATIC
EFI_STATUS
GetBarType (
  IN  EFI_PCI_IO_PROTOCOL *PciIo,
  IN  UINT8               BarIndex,
  OUT VIRTIO_1_0_BAR_TYPE *BarType
  )
{
  EFI_STATUS Status;
  VOID       *Resources;

  Status = PciIo->GetBarAttributes (PciIo, BarIndex, NULL, &Resources);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EFI_UNSUPPORTED;

  if (*(UINT8 *)Resources == ACPI_QWORD_ADDRESS_SPACE_DESCRIPTOR) {
    EFI_ACPI_QWORD_ADDRESS_SPACE_DESCRIPTOR *Descriptor;

    Descriptor = Resources;
    switch (Descriptor->ResType) {
    case ACPI_ADDRESS_SPACE_TYPE_MEM:
      *BarType = Virtio10BarTypeMem;
      Status = EFI_SUCCESS;
      break;

    case ACPI_ADDRESS_SPACE_TYPE_IO:
      *BarType = Virtio10BarTypeIo;
      Status = EFI_SUCCESS;
      break;

    default:
      break;
    }
  }

  FreePool (Resources);
  return Status;
}


/*
  Traverse the PCI capabilities list of a virtio-1.0 device, and capture the
  locations of the interesting virtio-1.0 register blocks.

  @param[in,out] Device         The VIRTIO_1_0_DEV structure that identifies
                                the device. On input, the caller is responsible
                                that the Device->PciIo member be live, and that
                                the CommonConfig, NotifyConfig,
                                NotifyOffsetMultiplier and SpecificConfig
                                members be zeroed. On output,  said members
                                will have been updated from the PCI
                                capabilities found.

  @retval EFI_SUCCESS  Traversal successful.

  @return              Error codes from PciCapPciIoLib, PciCapLib, and the
                       GetBarType() helper function.
*/
STATIC
EFI_STATUS
ParseCapabilities (
  IN OUT VIRTIO_1_0_DEV *Device
  )
{
  EFI_STATUS   Status;
  PCI_CAP_DEV  *PciDevice;
  PCI_CAP_LIST *CapList;
  UINT16       VendorInstance;
  PCI_CAP      *VendorCap;

  Status = PciCapPciIoDeviceInit (Device->PciIo, &PciDevice);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = PciCapListInit (PciDevice, &CapList);
  if (EFI_ERROR (Status)) {
    goto UninitPciDevice;
  }

  for (VendorInstance = 0;
       !EFI_ERROR (PciCapListFindCap (CapList, PciCapNormal,
                     EFI_PCI_CAPABILITY_ID_VENDOR, VendorInstance,
                     &VendorCap));
       VendorInstance++) {
    UINT8             CapLen;
    VIRTIO_PCI_CAP    VirtIoCap;
    VIRTIO_1_0_CONFIG *ParsedConfig;

    //
    // Big enough to accommodate a VIRTIO_PCI_CAP structure?
    //
    Status = PciCapRead (PciDevice, VendorCap,
               OFFSET_OF (EFI_PCI_CAPABILITY_VENDOR_HDR, Length), &CapLen,
               sizeof CapLen);
    if (EFI_ERROR (Status)) {
      goto UninitCapList;
    }
    if (CapLen < sizeof VirtIoCap) {
      //
      // Too small, move to next.
      //
      continue;
    }

    //
    // Read interesting part of capability.
    //
    Status = PciCapRead (PciDevice, VendorCap, 0, &VirtIoCap, sizeof VirtIoCap);
    if (EFI_ERROR (Status)) {
      goto UninitCapList;
    }

    switch (VirtIoCap.ConfigType) {
    case VIRTIO_PCI_CAP_COMMON_CFG:
      ParsedConfig = &Device->CommonConfig;
      break;
    case VIRTIO_PCI_CAP_NOTIFY_CFG:
      ParsedConfig = &Device->NotifyConfig;
      break;
    case VIRTIO_PCI_CAP_DEVICE_CFG:
      ParsedConfig = &Device->SpecificConfig;
      break;
    default:
      //
      // Capability is not interesting.
      //
      continue;
    }

    //
    // Save the location of the register block into ParsedConfig.
    //
    Status = GetBarType (Device->PciIo, VirtIoCap.Bar, &ParsedConfig->BarType);
    if (EFI_ERROR (Status)) {
      goto UninitCapList;
    }
    ParsedConfig->Bar    = VirtIoCap.Bar;
    ParsedConfig->Offset = VirtIoCap.Offset;
    ParsedConfig->Length = VirtIoCap.Length;

    if (VirtIoCap.ConfigType == VIRTIO_PCI_CAP_NOTIFY_CFG) {
      //
      // This capability has an additional field called NotifyOffsetMultiplier;
      // parse it too.
      //
      if (CapLen < sizeof VirtIoCap + sizeof Device->NotifyOffsetMultiplier) {
        //
        // Too small, move to next.
        //
        continue;
      }

      Status = PciCapRead (PciDevice, VendorCap, sizeof VirtIoCap,
                 &Device->NotifyOffsetMultiplier,
                 sizeof Device->NotifyOffsetMultiplier);
      if (EFI_ERROR (Status)) {
        goto UninitCapList;
      }
    }

    //
    // Capability parsed successfully.
    //
    ParsedConfig->Exists = TRUE;
  }

  ASSERT_EFI_ERROR (Status);

UninitCapList:
  PciCapListUninit (CapList);

UninitPciDevice:
  PciCapPciIoDeviceUninit (PciDevice);

  return Status;
}


/**
  Accumulate the BAR type of a virtio-1.0 register block into a UINT64
  attribute map, such that the latter is suitable for enabling IO / MMIO
  decoding with EFI_PCI_IO_PROTOCOL.Attributes().

  @param[in]     Config      The "fat pointer" structure that identifies the
                             register block. It is allowed for the register
                             block not to exist.

  @param[in,out] Attributes  On output, if the register block exists,
                             EFI_PCI_IO_ATTRIBUTE_MEMORY or
                             EFI_PCI_IO_ATTRIBUTE_IO is OR-ed into Attributes,
                             according to the register block's BAR type.
**/
STATIC
VOID
UpdateAttributes (
  IN     VIRTIO_1_0_CONFIG *Config,
  IN OUT UINT64            *Attributes
  )
{
  if (Config->Exists) {
    *Attributes |= (Config->BarType == Virtio10BarTypeMem) ?
                     EFI_PCI_IO_ATTRIBUTE_MEMORY:
                     EFI_PCI_IO_ATTRIBUTE_IO;
  }
}


//
// VIRTIO_DEVICE_PROTOCOL member functions
//

STATIC
EFI_STATUS
EFIAPI
Virtio10GetDeviceFeatures (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  OUT UINT64                *DeviceFeatures
  )
{
  VIRTIO_1_0_DEV *Dev;
  UINT32         Selector;
  UINT32         Features32[2];

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  for (Selector = 0; Selector < 2; ++Selector) {
    EFI_STATUS Status;

    //
    // Select the low or high half of the features.
    //
    Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, TRUE,
               OFFSET_OF (VIRTIO_PCI_COMMON_CFG, DeviceFeatureSelect),
               sizeof Selector, &Selector);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Fetch that half.
    //
    Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, FALSE,
               OFFSET_OF (VIRTIO_PCI_COMMON_CFG, DeviceFeature),
               sizeof Features32[Selector], &Features32[Selector]);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  *DeviceFeatures = LShiftU64 (Features32[1], 32) | Features32[0];
  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10SetGuestFeatures (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT64                   Features
  )
{
  VIRTIO_1_0_DEV *Dev;
  UINT32         Selector;
  UINT32         Features32[2];

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  Features32[0] = (UINT32)Features;
  Features32[1] = (UINT32)RShiftU64 (Features, 32);

  for (Selector = 0; Selector < 2; ++Selector) {
    EFI_STATUS Status;

    //
    // Select the low or high half of the features.
    //
    Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, TRUE,
               OFFSET_OF (VIRTIO_PCI_COMMON_CFG, DriverFeatureSelect),
               sizeof Selector, &Selector);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Write that half.
    //
    Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, TRUE,
               OFFSET_OF (VIRTIO_PCI_COMMON_CFG, DriverFeature),
               sizeof Features32[Selector], &Features32[Selector]);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10SetQueueAddress (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN VRING                   *Ring,
  IN UINT64                  RingBaseShift
  )
{
  VIRTIO_1_0_DEV *Dev;
  EFI_STATUS     Status;
  UINT64         Address;
  UINT16         Enable;

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  Address = (UINTN)Ring->Desc;
  Address += RingBaseShift;
  Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, TRUE,
             OFFSET_OF (VIRTIO_PCI_COMMON_CFG, QueueDesc),
             sizeof Address, &Address);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Address = (UINTN)Ring->Avail.Flags;
  Address += RingBaseShift;
  Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, TRUE,
             OFFSET_OF (VIRTIO_PCI_COMMON_CFG, QueueAvail),
             sizeof Address, &Address);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Address = (UINTN)Ring->Used.Flags;
  Address += RingBaseShift;
  Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, TRUE,
             OFFSET_OF (VIRTIO_PCI_COMMON_CFG, QueueUsed),
             sizeof Address, &Address);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Enable = 1;
  Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, TRUE,
             OFFSET_OF (VIRTIO_PCI_COMMON_CFG, QueueEnable),
             sizeof Enable, &Enable);
  return Status;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10SetQueueSel (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT16                   Index
  )
{
  VIRTIO_1_0_DEV *Dev;
  EFI_STATUS     Status;

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, TRUE,
             OFFSET_OF (VIRTIO_PCI_COMMON_CFG, QueueSelect),
             sizeof Index, &Index);
  return Status;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10SetQueueNotify (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT16                   Index
  )
{
  VIRTIO_1_0_DEV *Dev;
  EFI_STATUS     Status;
  UINT16         SavedQueueSelect;
  UINT16         NotifyOffset;

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  //
  // Read NotifyOffset first. NotifyOffset is queue specific, so we have
  // to stash & restore the current queue selector around it.
  //
  // So, start with saving the current queue selector.
  //
  Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, FALSE,
             OFFSET_OF (VIRTIO_PCI_COMMON_CFG, QueueSelect),
             sizeof SavedQueueSelect, &SavedQueueSelect);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Select the requested queue.
  //
  Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, TRUE,
             OFFSET_OF (VIRTIO_PCI_COMMON_CFG, QueueSelect),
             sizeof Index, &Index);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Read the QueueNotifyOff field.
  //
  Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, FALSE,
             OFFSET_OF (VIRTIO_PCI_COMMON_CFG, QueueNotifyOff),
             sizeof NotifyOffset, &NotifyOffset);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Re-select the original queue.
  //
  Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, TRUE,
             OFFSET_OF (VIRTIO_PCI_COMMON_CFG, QueueSelect),
             sizeof SavedQueueSelect, &SavedQueueSelect);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // We can now kick the queue.
  //
  Status = Virtio10Transfer (Dev->PciIo, &Dev->NotifyConfig, TRUE,
             NotifyOffset * Dev->NotifyOffsetMultiplier,
             sizeof Index, &Index);
  return Status;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10SetQueueAlign (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT32                   Alignment
  )
{
  return (Alignment == EFI_PAGE_SIZE) ? EFI_SUCCESS : EFI_UNSUPPORTED;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10SetPageSize (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT32                   PageSize
  )
{
  return (PageSize == EFI_PAGE_SIZE) ? EFI_SUCCESS : EFI_UNSUPPORTED;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10GetQueueNumMax (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT16                  *QueueNumMax
  )
{
  VIRTIO_1_0_DEV *Dev;
  EFI_STATUS     Status;

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, FALSE,
             OFFSET_OF (VIRTIO_PCI_COMMON_CFG, QueueSize),
             sizeof *QueueNumMax, QueueNumMax);
  return Status;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10SetQueueNum (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT16                   QueueSize
  )
{
  EFI_STATUS     Status;
  UINT16         CurrentSize;

  //
  // This member function is required for VirtIo MMIO, and a no-op in
  // VirtIo PCI 0.9.5. In VirtIo 1.0, drivers can theoretically use this
  // member to reduce memory consumption, but none of our drivers do. So
  // just check that they set the size that is already in effect.
  //
  Status = Virtio10GetQueueNumMax (This, &CurrentSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  return (CurrentSize == QueueSize) ? EFI_SUCCESS : EFI_UNSUPPORTED;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10GetDeviceStatus (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT8                   *DeviceStatus
  )
{
  VIRTIO_1_0_DEV *Dev;
  EFI_STATUS     Status;

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, FALSE,
             OFFSET_OF (VIRTIO_PCI_COMMON_CFG, DeviceStatus),
             sizeof *DeviceStatus, DeviceStatus);
  return Status;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10SetDeviceStatus (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT8                   DeviceStatus
  )
{
  VIRTIO_1_0_DEV *Dev;
  EFI_STATUS     Status;

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  Status = Virtio10Transfer (Dev->PciIo, &Dev->CommonConfig, TRUE,
             OFFSET_OF (VIRTIO_PCI_COMMON_CFG, DeviceStatus),
             sizeof DeviceStatus, &DeviceStatus);
  return Status;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10WriteDevice (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  IN UINTN                  FieldOffset,
  IN UINTN                  FieldSize,
  IN UINT64                 Value
  )
{
  VIRTIO_1_0_DEV *Dev;
  EFI_STATUS     Status;

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  Status = Virtio10Transfer (Dev->PciIo, &Dev->SpecificConfig, TRUE,
             FieldOffset, FieldSize, &Value);
  return Status;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10ReadDevice (
  IN  VIRTIO_DEVICE_PROTOCOL *This,
  IN  UINTN                  FieldOffset,
  IN  UINTN                  FieldSize,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
{
  VIRTIO_1_0_DEV *Dev;
  EFI_STATUS     Status;

  if (FieldSize != BufferSize) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  Status = Virtio10Transfer (Dev->PciIo, &Dev->SpecificConfig, FALSE,
             FieldOffset, FieldSize, Buffer);
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
Virtio10AllocateSharedPages (
  IN     VIRTIO_DEVICE_PROTOCOL  *This,
  IN     UINTN                   Pages,
  IN OUT VOID                    **HostAddress
  )
{
  VIRTIO_1_0_DEV *Dev;
  EFI_STATUS     Status;

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  Status = Dev->PciIo->AllocateBuffer (
                         Dev->PciIo,
                         AllocateAnyPages,
                         EfiBootServicesData,
                         Pages,
                         HostAddress,
                         EFI_PCI_ATTRIBUTE_MEMORY_CACHED
                         );
  return Status;
}

STATIC
VOID
EFIAPI
Virtio10FreeSharedPages (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  UINTN                   Pages,
  IN  VOID                    *HostAddress
  )
{
  VIRTIO_1_0_DEV *Dev;

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  Dev->PciIo->FreeBuffer (
                Dev->PciIo,
                Pages,
                HostAddress
                );
}

STATIC
EFI_STATUS
EFIAPI
Virtio10MapSharedBuffer (
  IN     VIRTIO_DEVICE_PROTOCOL  *This,
  IN     VIRTIO_MAP_OPERATION    Operation,
  IN     VOID                    *HostAddress,
  IN OUT UINTN                   *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS    *DeviceAddress,
  OUT    VOID                    **Mapping
  )
{
  EFI_STATUS                    Status;
  VIRTIO_1_0_DEV                *Dev;
  EFI_PCI_IO_PROTOCOL_OPERATION PciIoOperation;

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  //
  // Map VIRTIO_MAP_OPERATION to EFI_PCI_IO_PROTOCOL_OPERATION
  //
  switch (Operation) {
  case VirtioOperationBusMasterRead:
    PciIoOperation = EfiPciIoOperationBusMasterRead;
    break;
  case VirtioOperationBusMasterWrite:
    PciIoOperation = EfiPciIoOperationBusMasterWrite;
    break;
  case VirtioOperationBusMasterCommonBuffer:
    PciIoOperation = EfiPciIoOperationBusMasterCommonBuffer;
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  Status = Dev->PciIo->Map (
                         Dev->PciIo,
                         PciIoOperation,
                         HostAddress,
                         NumberOfBytes,
                         DeviceAddress,
                         Mapping
                         );
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
Virtio10UnmapSharedBuffer (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  VOID                    *Mapping
  )
{
  EFI_STATUS      Status;
  VIRTIO_1_0_DEV  *Dev;

  Dev = VIRTIO_1_0_FROM_VIRTIO_DEVICE (This);

  Status = Dev->PciIo->Unmap (
                         Dev->PciIo,
                         Mapping
                         );

  return Status;
}

STATIC CONST VIRTIO_DEVICE_PROTOCOL mVirtIoTemplate = {
  VIRTIO_SPEC_REVISION (1, 0, 0),
  0,                              // SubSystemDeviceId, filled in dynamically
  Virtio10GetDeviceFeatures,
  Virtio10SetGuestFeatures,
  Virtio10SetQueueAddress,
  Virtio10SetQueueSel,
  Virtio10SetQueueNotify,
  Virtio10SetQueueAlign,
  Virtio10SetPageSize,
  Virtio10GetQueueNumMax,
  Virtio10SetQueueNum,
  Virtio10GetDeviceStatus,
  Virtio10SetDeviceStatus,
  Virtio10WriteDevice,
  Virtio10ReadDevice,
  Virtio10AllocateSharedPages,
  Virtio10FreeSharedPages,
  Virtio10MapSharedBuffer,
  Virtio10UnmapSharedBuffer
};


//
// EFI_DRIVER_BINDING_PROTOCOL member functions
//

STATIC
EFI_STATUS
EFIAPI
Virtio10BindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00          Pci;

  Status = gBS->OpenProtocol (DeviceHandle, &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo, This->DriverBindingHandle,
                  DeviceHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0,
                        sizeof Pci / sizeof (UINT32), &Pci);
  if (EFI_ERROR (Status)) {
    goto CloseProtocol;
  }

  Status = EFI_UNSUPPORTED;
  //
  // Recognize non-transitional modern devices. Also, we'll have to parse the
  // PCI capability list, so make sure the CapabilityPtr field will be valid.
  //
  if (Pci.Hdr.VendorId == VIRTIO_VENDOR_ID &&
      Pci.Hdr.DeviceId >= 0x1040 &&
      Pci.Hdr.DeviceId <= 0x107F &&
      Pci.Hdr.RevisionID >= 0x01 &&
      Pci.Device.SubsystemID >= 0x40 &&
      (Pci.Hdr.Status & EFI_PCI_STATUS_CAPABILITY) != 0) {
    //
    // The virtio-vga device is special. It can be driven both as a VGA device
    // with a linear framebuffer, and through its underlying, modern,
    // virtio-gpu-pci device, which has no linear framebuffer itself. For
    // compatibility with guest OSes that insist on inheriting a linear
    // framebuffer from the firmware, we should leave virtio-vga to
    // QemuVideoDxe, and support only virtio-gpu-pci here.
    //
    // Both virtio-vga and virtio-gpu-pci have DeviceId 0x1050, but only the
    // former has device class PCI_CLASS_DISPLAY_VGA.
    //
    if (Pci.Hdr.DeviceId != 0x1050 || !IS_PCI_VGA (&Pci)) {
      Status = EFI_SUCCESS;
    }
  }

CloseProtocol:
  gBS->CloseProtocol (DeviceHandle, &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle, DeviceHandle);

  return Status;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10BindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  VIRTIO_1_0_DEV *Device;
  EFI_STATUS     Status;
  PCI_TYPE00     Pci;
  UINT64         SetAttributes;

  Device = AllocateZeroPool (sizeof *Device);
  if (Device == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Device->Signature = VIRTIO_1_0_SIGNATURE;
  CopyMem (&Device->VirtIo, &mVirtIoTemplate, sizeof mVirtIoTemplate);

  Status = gBS->OpenProtocol (DeviceHandle, &gEfiPciIoProtocolGuid,
                  (VOID **)&Device->PciIo, This->DriverBindingHandle,
                  DeviceHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (EFI_ERROR (Status)) {
    goto FreeDevice;
  }

  Status = Device->PciIo->Pci.Read (Device->PciIo, EfiPciIoWidthUint32, 0,
                                sizeof Pci / sizeof (UINT32), &Pci);
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  Device->VirtIo.SubSystemDeviceId = Pci.Hdr.DeviceId - 0x1040;

  Status = ParseCapabilities (Device);
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  Status = Device->PciIo->Attributes (Device->PciIo,
                            EfiPciIoAttributeOperationGet, 0,
                            &Device->OriginalPciAttributes);
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  SetAttributes = (EFI_PCI_IO_ATTRIBUTE_BUS_MASTER |
                   EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE);
  UpdateAttributes (&Device->CommonConfig, &SetAttributes);
  UpdateAttributes (&Device->NotifyConfig, &SetAttributes);
  UpdateAttributes (&Device->SpecificConfig, &SetAttributes);
  Status = Device->PciIo->Attributes (Device->PciIo,
                            EfiPciIoAttributeOperationEnable, SetAttributes,
                            NULL);
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  Status = gBS->InstallProtocolInterface (&DeviceHandle,
                  &gVirtioDeviceProtocolGuid, EFI_NATIVE_INTERFACE,
                  &Device->VirtIo);
  if (EFI_ERROR (Status)) {
    goto RestorePciAttributes;
  }

  return EFI_SUCCESS;

RestorePciAttributes:
  Device->PciIo->Attributes (Device->PciIo, EfiPciIoAttributeOperationSet,
                   Device->OriginalPciAttributes, NULL);

ClosePciIo:
  gBS->CloseProtocol (DeviceHandle, &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle, DeviceHandle);

FreeDevice:
  FreePool (Device);

  return Status;
}


STATIC
EFI_STATUS
EFIAPI
Virtio10BindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  )
{
  EFI_STATUS             Status;
  VIRTIO_DEVICE_PROTOCOL *VirtIo;
  VIRTIO_1_0_DEV         *Device;

  Status = gBS->OpenProtocol (DeviceHandle, &gVirtioDeviceProtocolGuid,
                  (VOID **)&VirtIo, This->DriverBindingHandle,
                  DeviceHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Device = VIRTIO_1_0_FROM_VIRTIO_DEVICE (VirtIo);

  Status = gBS->UninstallProtocolInterface (DeviceHandle,
                  &gVirtioDeviceProtocolGuid, &Device->VirtIo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Device->PciIo->Attributes (Device->PciIo, EfiPciIoAttributeOperationSet,
                   Device->OriginalPciAttributes, NULL);
  gBS->CloseProtocol (DeviceHandle, &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle, DeviceHandle);
  FreePool (Device);

  return EFI_SUCCESS;
}


STATIC EFI_DRIVER_BINDING_PROTOCOL mDriverBinding = {
  &Virtio10BindingSupported,
  &Virtio10BindingStart,
  &Virtio10BindingStop,
  0x10, // Version
  NULL, // ImageHandle, to be overwritten
  NULL  // DriverBindingHandle, to be overwritten
};


//
// EFI_COMPONENT_NAME_PROTOCOL and EFI_COMPONENT_NAME2_PROTOCOL
// implementations
//

STATIC
EFI_UNICODE_STRING_TABLE mDriverNameTable[] = {
  { "eng;en", L"Virtio 1.0 PCI Driver" },
  { NULL,     NULL                     }
};

STATIC
EFI_COMPONENT_NAME_PROTOCOL mComponentName;

STATIC
EFI_STATUS
EFIAPI
Virtio10GetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &mComponentName) // Iso639Language
           );
}

STATIC
EFI_STATUS
EFIAPI
Virtio10GetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  EFI_HANDLE                  DeviceHandle,
  IN  EFI_HANDLE                  ChildHandle,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_COMPONENT_NAME_PROTOCOL mComponentName = {
  &Virtio10GetDriverName,
  &Virtio10GetDeviceName,
  "eng"
};

STATIC
EFI_COMPONENT_NAME2_PROTOCOL mComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)     &Virtio10GetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) &Virtio10GetDeviceName,
  "en"
};


//
// Entry point of this driver
//

EFI_STATUS
EFIAPI
Virtio10EntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &mDriverBinding,
           ImageHandle,
           &mComponentName,
           &mComponentName2
           );
}

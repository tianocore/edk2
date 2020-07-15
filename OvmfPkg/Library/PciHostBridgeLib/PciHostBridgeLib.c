/** @file
  OVMF's instance of the PCI Host Bridge Library.

  Copyright (C) 2016, Red Hat, Inc.
  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Q35MchIch9.h>

#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/PciRootBridgeIo.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/PciLib.h>
#include <Library/QemuFwCfgLib.h>
#include "PciHostBridge.h"


#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH     AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} OVMF_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()


GLOBAL_REMOVE_IF_UNREFERENCED
CHAR16 *mPciHostBridgeLibAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};


STATIC
CONST
OVMF_PCI_ROOT_BRIDGE_DEVICE_PATH mRootBridgeDevicePathTemplate = {
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8) (sizeof(ACPI_HID_DEVICE_PATH)),
        (UINT8) ((sizeof(ACPI_HID_DEVICE_PATH)) >> 8)
      }
    },
    EISA_PNP_ID(0x0A03), // HID
    0                    // UID
  },

  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

STATIC PCI_ROOT_BRIDGE_APERTURE mNonExistAperture = { MAX_UINT64, 0 };

/**
  Initialize a PCI_ROOT_BRIDGE structure.

  @param[in]  Supports         Supported attributes.

  @param[in]  Attributes       Initial attributes.

  @param[in]  AllocAttributes  Allocation attributes.

  @param[in]  RootBusNumber    The bus number to store in RootBus.

  @param[in]  MaxSubBusNumber  The inclusive maximum bus number that can be
                               assigned to any subordinate bus found behind any
                               PCI bridge hanging off this root bus.

                               The caller is repsonsible for ensuring that
                               RootBusNumber <= MaxSubBusNumber. If
                               RootBusNumber equals MaxSubBusNumber, then the
                               root bus has no room for subordinate buses.

  @param[in]  Io               IO aperture.

  @param[in]  Mem              MMIO aperture.

  @param[in]  MemAbove4G       MMIO aperture above 4G.

  @param[in]  PMem             Prefetchable MMIO aperture.

  @param[in]  PMemAbove4G      Prefetchable MMIO aperture above 4G.

  @param[out] RootBus          The PCI_ROOT_BRIDGE structure (allocated by the
                               caller) that should be filled in by this
                               function.

  @retval EFI_SUCCESS           Initialization successful. A device path
                                consisting of an ACPI device path node, with
                                UID = RootBusNumber, has been allocated and
                                linked into RootBus.

  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
EFI_STATUS
InitRootBridge (
  IN  UINT64                   Supports,
  IN  UINT64                   Attributes,
  IN  UINT64                   AllocAttributes,
  IN  UINT8                    RootBusNumber,
  IN  UINT8                    MaxSubBusNumber,
  IN  PCI_ROOT_BRIDGE_APERTURE *Io,
  IN  PCI_ROOT_BRIDGE_APERTURE *Mem,
  IN  PCI_ROOT_BRIDGE_APERTURE *MemAbove4G,
  IN  PCI_ROOT_BRIDGE_APERTURE *PMem,
  IN  PCI_ROOT_BRIDGE_APERTURE *PMemAbove4G,
  OUT PCI_ROOT_BRIDGE          *RootBus
  )
{
  OVMF_PCI_ROOT_BRIDGE_DEVICE_PATH *DevicePath;

  //
  // Be safe if other fields are added to PCI_ROOT_BRIDGE later.
  //
  ZeroMem (RootBus, sizeof *RootBus);

  RootBus->Segment = 0;

  RootBus->Supports   = Supports;
  RootBus->Attributes = Attributes;

  RootBus->DmaAbove4G = FALSE;

  RootBus->AllocationAttributes = AllocAttributes;
  RootBus->Bus.Base  = RootBusNumber;
  RootBus->Bus.Limit = MaxSubBusNumber;
  CopyMem (&RootBus->Io, Io, sizeof (*Io));
  CopyMem (&RootBus->Mem, Mem, sizeof (*Mem));
  CopyMem (&RootBus->MemAbove4G, MemAbove4G, sizeof (*MemAbove4G));
  CopyMem (&RootBus->PMem, PMem, sizeof (*PMem));
  CopyMem (&RootBus->PMemAbove4G, PMemAbove4G, sizeof (*PMemAbove4G));

  RootBus->NoExtendedConfigSpace = (PcdGet16 (PcdOvmfHostBridgePciDevId) !=
                                    INTEL_Q35_MCH_DEVICE_ID);

  DevicePath = AllocateCopyPool (sizeof mRootBridgeDevicePathTemplate,
                 &mRootBridgeDevicePathTemplate);
  if (DevicePath == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __FUNCTION__, EFI_OUT_OF_RESOURCES));
    return EFI_OUT_OF_RESOURCES;
  }
  DevicePath->AcpiDevicePath.UID = RootBusNumber;
  RootBus->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)DevicePath;

  DEBUG ((DEBUG_INFO,
    "%a: populated root bus %d, with room for %d subordinate bus(es)\n",
    __FUNCTION__, RootBusNumber, MaxSubBusNumber - RootBusNumber));
  return EFI_SUCCESS;
}


/**
  Uninitialize a PCI_ROOT_BRIDGE structure set up with InitRootBridge().

  param[in] RootBus  The PCI_ROOT_BRIDGE structure, allocated by the caller and
                     initialized with InitRootBridge(), that should be
                     uninitialized. This function doesn't free RootBus.
**/
STATIC
VOID
UninitRootBridge (
  IN PCI_ROOT_BRIDGE *RootBus
  )
{
  FreePool (RootBus->DevicePath);
}


/**
  Return all the root bridge instances in an array.

  @param Count  Return the count of root bridge instances.

  @return All the root bridge instances in an array.
          The array should be passed into PciHostBridgeFreeRootBridges()
          when it's not used.
**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  UINTN *Count
  )
{
  EFI_STATUS           Status;
  FIRMWARE_CONFIG_ITEM FwCfgItem;
  UINTN                FwCfgSize;
  UINT64               ExtraRootBridges;
  PCI_ROOT_BRIDGE      *Bridges;
  UINTN                Initialized;
  UINTN                LastRootBridgeNumber;
  UINTN                RootBridgeNumber;
  UINT64               Attributes;
  UINT64               AllocationAttributes;
  PCI_ROOT_BRIDGE_APERTURE Io;
  PCI_ROOT_BRIDGE_APERTURE Mem;
  PCI_ROOT_BRIDGE_APERTURE MemAbove4G;

  if (PcdGetBool (PcdPciDisableBusEnumeration)) {
    return ScanForRootBridges (Count);
  }

  ZeroMem (&Io, sizeof (Io));
  ZeroMem (&Mem, sizeof (Mem));
  ZeroMem (&MemAbove4G, sizeof (MemAbove4G));

  Attributes = EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO |
    EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO |
    EFI_PCI_ATTRIBUTE_ISA_IO_16 |
    EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO |
    EFI_PCI_ATTRIBUTE_VGA_MEMORY |
    EFI_PCI_ATTRIBUTE_VGA_IO_16 |
    EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16;

  AllocationAttributes = EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM;
  if (PcdGet64 (PcdPciMmio64Size) > 0) {
    AllocationAttributes |= EFI_PCI_HOST_BRIDGE_MEM64_DECODE;
    MemAbove4G.Base = PcdGet64 (PcdPciMmio64Base);
    MemAbove4G.Limit = PcdGet64 (PcdPciMmio64Base) +
                       PcdGet64 (PcdPciMmio64Size) - 1;
  } else {
    CopyMem (&MemAbove4G, &mNonExistAperture, sizeof (mNonExistAperture));
  }

  Io.Base = PcdGet64 (PcdPciIoBase);
  Io.Limit = PcdGet64 (PcdPciIoBase) + (PcdGet64 (PcdPciIoSize) - 1);
  Mem.Base = PcdGet64 (PcdPciMmio32Base);
  Mem.Limit = PcdGet64 (PcdPciMmio32Base) + (PcdGet64 (PcdPciMmio32Size) - 1);

  *Count = 0;

  //
  // QEMU provides the number of extra root buses, shortening the exhaustive
  // search below. If there is no hint, the feature is missing.
  //
  Status = QemuFwCfgFindFile ("etc/extra-pci-roots", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status) || FwCfgSize != sizeof ExtraRootBridges) {
    ExtraRootBridges = 0;
  } else {
    QemuFwCfgSelectItem (FwCfgItem);
    QemuFwCfgReadBytes (FwCfgSize, &ExtraRootBridges);

    if (ExtraRootBridges > PCI_MAX_BUS) {
      DEBUG ((DEBUG_ERROR, "%a: invalid count of extra root buses (%Lu) "
        "reported by QEMU\n", __FUNCTION__, ExtraRootBridges));
      return NULL;
    }
    DEBUG ((DEBUG_INFO, "%a: %Lu extra root buses reported by QEMU\n",
      __FUNCTION__, ExtraRootBridges));
  }

  //
  // Allocate the "main" root bridge, and any extra root bridges.
  //
  Bridges = AllocatePool ((1 + (UINTN)ExtraRootBridges) * sizeof *Bridges);
  if (Bridges == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __FUNCTION__, EFI_OUT_OF_RESOURCES));
    return NULL;
  }
  Initialized = 0;

  //
  // The "main" root bus is always there.
  //
  LastRootBridgeNumber = 0;

  //
  // Scan all other root buses. If function 0 of any device on a bus returns a
  // VendorId register value different from all-bits-one, then that bus is
  // alive.
  //
  for (RootBridgeNumber = 1;
       RootBridgeNumber <= PCI_MAX_BUS && Initialized < ExtraRootBridges;
       ++RootBridgeNumber) {
    UINTN Device;

    for (Device = 0; Device <= PCI_MAX_DEVICE; ++Device) {
      if (PciRead16 (PCI_LIB_ADDRESS (RootBridgeNumber, Device, 0,
                       PCI_VENDOR_ID_OFFSET)) != MAX_UINT16) {
        break;
      }
    }
    if (Device <= PCI_MAX_DEVICE) {
      //
      // Found the next root bus. We can now install the *previous* one,
      // because now we know how big a bus number range *that* one has, for any
      // subordinate buses that might exist behind PCI bridges hanging off it.
      //
      Status = InitRootBridge (
        Attributes,
        Attributes,
        AllocationAttributes,
        (UINT8) LastRootBridgeNumber,
        (UINT8) (RootBridgeNumber - 1),
        &Io,
        &Mem,
        &MemAbove4G,
        &mNonExistAperture,
        &mNonExistAperture,
        &Bridges[Initialized]
        );
      if (EFI_ERROR (Status)) {
        goto FreeBridges;
      }
      ++Initialized;
      LastRootBridgeNumber = RootBridgeNumber;
    }
  }

  //
  // Install the last root bus (which might be the only, ie. main, root bus, if
  // we've found no extra root buses).
  //
  Status = InitRootBridge (
    Attributes,
    Attributes,
    AllocationAttributes,
    (UINT8) LastRootBridgeNumber,
    PCI_MAX_BUS,
    &Io,
    &Mem,
    &MemAbove4G,
    &mNonExistAperture,
    &mNonExistAperture,
    &Bridges[Initialized]
    );
  if (EFI_ERROR (Status)) {
    goto FreeBridges;
  }
  ++Initialized;

  *Count = Initialized;
  return Bridges;

FreeBridges:
  while (Initialized > 0) {
    --Initialized;
    UninitRootBridge (&Bridges[Initialized]);
  }

  FreePool (Bridges);
  return NULL;
}


/**
  Free the root bridge instances array returned from
  PciHostBridgeGetRootBridges().

  @param  The root bridge instances array.
  @param  The count of the array.
**/
VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  PCI_ROOT_BRIDGE *Bridges,
  UINTN           Count
  )
{
  if (Bridges == NULL && Count == 0) {
    return;
  }
  ASSERT (Bridges != NULL && Count > 0);

  do {
    --Count;
    UninitRootBridge (&Bridges[Count]);
  } while (Count > 0);

  FreePool (Bridges);
}


/**
  Inform the platform that the resource conflict happens.

  @param HostBridgeHandle Handle of the Host Bridge.
  @param Configuration    Pointer to PCI I/O and PCI memory resource
                          descriptors. The Configuration contains the resources
                          for all the root bridges. The resource for each root
                          bridge is terminated with END descriptor and an
                          additional END is appended indicating the end of the
                          entire resources. The resource descriptor field
                          values follow the description in
                          EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                          .SubmitResources().
**/
VOID
EFIAPI
PciHostBridgeResourceConflict (
  EFI_HANDLE                        HostBridgeHandle,
  VOID                              *Configuration
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  UINTN                             RootBridgeIndex;
  DEBUG ((DEBUG_ERROR, "PciHostBridge: Resource conflict happens!\n"));

  RootBridgeIndex = 0;
  Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;
  while (Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    DEBUG ((DEBUG_ERROR, "RootBridge[%d]:\n", RootBridgeIndex++));
    for (; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
      ASSERT (Descriptor->ResType <
              ARRAY_SIZE (mPciHostBridgeLibAcpiAddressSpaceTypeStr)
              );
      DEBUG ((DEBUG_ERROR, " %s: Length/Alignment = 0x%lx / 0x%lx\n",
              mPciHostBridgeLibAcpiAddressSpaceTypeStr[Descriptor->ResType],
              Descriptor->AddrLen, Descriptor->AddrRangeMax
              ));
      if (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
        DEBUG ((DEBUG_ERROR, "     Granularity/SpecificFlag = %ld / %02x%s\n",
                Descriptor->AddrSpaceGranularity, Descriptor->SpecificFlag,
                ((Descriptor->SpecificFlag &
                  EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE
                  ) != 0) ? L" (Prefetchable)" : L""
                ));
      }
    }
    //
    // Skip the END descriptor for root bridge
    //
    ASSERT (Descriptor->Desc == ACPI_END_TAG_DESCRIPTOR);
    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)(
                   (EFI_ACPI_END_TAG_DESCRIPTOR *)Descriptor + 1
                   );
  }
}

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
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/PciHostBridgeUtilityLib.h>
#include <Library/PciLib.h>
#include <Library/QemuFwCfgLib.h>
#include "PciHostBridge.h"


STATIC PCI_ROOT_BRIDGE_APERTURE mNonExistAperture = { MAX_UINT64, 0 };


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
      Status = PciHostBridgeUtilityInitRootBridge (
        Attributes,
        Attributes,
        AllocationAttributes,
        FALSE,
        PcdGet16 (PcdOvmfHostBridgePciDevId) != INTEL_Q35_MCH_DEVICE_ID,
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
  Status = PciHostBridgeUtilityInitRootBridge (
    Attributes,
    Attributes,
    AllocationAttributes,
    FALSE,
    PcdGet16 (PcdOvmfHostBridgePciDevId) != INTEL_Q35_MCH_DEVICE_ID,
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
    PciHostBridgeUtilityUninitRootBridge (&Bridges[Initialized]);
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
    PciHostBridgeUtilityUninitRootBridge (&Bridges[Count]);
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
  PciHostBridgeUtilityResourceConflict (Configuration);
}

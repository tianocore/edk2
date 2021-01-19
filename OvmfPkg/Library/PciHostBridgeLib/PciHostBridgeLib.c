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

  return PciHostBridgeUtilityGetRootBridges (
    Count,
    Attributes,
    AllocationAttributes,
    FALSE,
    PcdGet16 (PcdOvmfHostBridgePciDevId) != INTEL_Q35_MCH_DEVICE_ID,
    0,
    PCI_MAX_BUS,
    &Io,
    &Mem,
    &MemAbove4G,
    &mNonExistAperture,
    &mNonExistAperture
    );
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
  PciHostBridgeUtilityFreeRootBridges (Bridges, Count);
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

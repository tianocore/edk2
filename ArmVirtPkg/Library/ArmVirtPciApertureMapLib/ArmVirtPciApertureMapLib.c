/** @file
  An Arm CCA implementation of PCI aperture mapping library.

  Provides functions to premap the PCI apertures. This is required for
  Arm CCA so that the memory attributes for the PCI mmio regions is set
  as Uncached memory.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiSpec.h>
#include <Library/ArmCcaLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/PciApertureMapLib.h>

/**
  Pre-map PCI root bridge MMIO apertures for Arm CCA.

  In a Realm, PCI MMIO ranges must be mapped with the memory attributes required
  before PCI BARs are allocated or accessed.

  @param[in] Io           I/O aperture.
  @param[in] Mem          MMIO aperture below 4 GB.
  @param[in] MemAbove4G   MMIO aperture above 4 GB.
  @param[in] PMem         Prefetchable MMIO aperture below 4 GB.
  @param[in] PMemAbove4G  Prefetchable MMIO aperture above 4 GB.

  @retval EFI_SUCCESS            The apertures were mapped, or
                                 no mapping was required.
  @retval EFI_INVALID_PARAMETER  A required aperture parameter is NULL or
                                 invalid.
  @retval Others                 PCI aperture mapping failed.
**/
EFI_STATUS
EFIAPI
MapPciApertures (
  IN  PCI_ROOT_BRIDGE_APERTURE  *Io,
  IN  PCI_ROOT_BRIDGE_APERTURE  *Mem,
  IN  PCI_ROOT_BRIDGE_APERTURE  *MemAbove4G,
  IN  PCI_ROOT_BRIDGE_APERTURE  *PMem,
  IN  PCI_ROOT_BRIDGE_APERTURE  *PMemAbove4G
  )
{
  EFI_STATUS  Status;

  if ((Io == NULL)          ||
      (Mem == NULL)         ||
      (MemAbove4G == NULL)  ||
      (PMem == NULL)        ||
      (PMemAbove4G == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (!ArmCcaIsRealm ()) {
    // Nothing to do.
    return EFI_SUCCESS;
  }

  // We expect that there exists at least one valid aperture, so set the
  // default status to INVALID.
  Status = EFI_INVALID_PARAMETER;

  // Map the 32 bit memory region.
  if (Mem->Base <= Mem->Limit) {
    Status = MapMmioMemory (
               Mem->Base,
               (Mem->Limit - Mem->Base + 1),
               EFI_MEMORY_UC | EFI_MEMORY_XP
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  // Map the 64 bit memory region.
  if (MemAbove4G->Base <= MemAbove4G->Limit) {
    Status = MapMmioMemory (
               MemAbove4G->Base,
               (MemAbove4G->Limit - MemAbove4G->Base + 1),
               EFI_MEMORY_UC | EFI_MEMORY_XP
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  return Status;
}

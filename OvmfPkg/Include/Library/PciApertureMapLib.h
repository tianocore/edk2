/** @file
  Library class for mapping PCI apertures.

  Provides functions to premap the PCI apertures. This is mostly required
  for Arm CCA so that the memory attributes for the PCI mmio regions is set
  as Uncached memory.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi/UefiBaseType.h>
#include <Library/PciHostBridgeLib.h>

/**
  Map the PCI root bridge apertures before PCI enumeration.

  Platforms can use this hook to add the host bridge aperture ranges to the
  GCD memory map and apply the memory attributes required before PCI BARs are
  allocated or accessed.

  @param[in] Io           I/O aperture.
  @param[in] Mem          MMIO aperture below 4 GB.
  @param[in] MemAbove4G   MMIO aperture above 4 GB.
  @param[in] PMem         Prefetchable MMIO aperture below 4 GB.
  @param[in] PMemAbove4G  Prefetchable MMIO aperture above 4 GB.

  @retval EFI_SUCCESS            The apertures were mapped, or no mapping was
                                  required.
  @retval EFI_INVALID_PARAMETER  A required aperture parameter is NULL or
                                  contains an invalid range.
  @retval Others                 The aperture mapping failed.
**/
EFI_STATUS
EFIAPI
MapPciApertures (
  IN  PCI_ROOT_BRIDGE_APERTURE  *Io,
  IN  PCI_ROOT_BRIDGE_APERTURE  *Mem,
  IN  PCI_ROOT_BRIDGE_APERTURE  *MemAbove4G,
  IN  PCI_ROOT_BRIDGE_APERTURE  *PMem,
  IN  PCI_ROOT_BRIDGE_APERTURE  *PMemAbove4G
  );

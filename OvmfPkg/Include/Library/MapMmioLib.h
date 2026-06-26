/** @file
  Helper library to map memory regions.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Base.h>

/**
  Map a range as MMIO in the GCD memory map.

  The requested range is expanded to page boundaries before it is processed.
  Missing GCD memory space descriptors are added as
  EfiGcdMemoryTypeMemoryMappedIo. Existing MMIO descriptors are accepted only
  when their capabilities contain the requested attributes. Existing descriptors
  of any other type are treated as conflicts.

  After the range is backed by compatible MMIO descriptors, the requested GCD
  memory space attributes are applied to the normalized full range.

  If this function fails after adding new GCD MMIO descriptors, the descriptors
  are not rolled back. Callers are expected to treat failures from this function
  as fatal to the current boot path.

  @param[in] Base        The base address of the requested MMIO range.
  @param[in] Length      The size of the requested MMIO range, in bytes.
  @param[in] Attributes  The GCD memory space attributes to apply to the MMIO
                          range.

  @retval EFI_SUCCESS            The full range was mapped as MMIO and
                                  configured with the requested attributes.
  @retval EFI_INVALID_PARAMETER  Length is zero, or the normalized range
                                  overflows the physical address space.
  @retval EFI_UNSUPPORTED        The range overlaps an existing non-MMIO
                                  descriptor, or an MMIO descriptor without the
                                  requested attributes.
  @retval EFI_ABORTED            An existing GCD descriptor is malformed.
  @retval Others                 The GCD memory services returned an error.
**/
EFI_STATUS
EFIAPI
MapMmioMemory (
  IN EFI_PHYSICAL_ADDRESS  Base,
  IN UINT64                Length,
  IN UINT64                Attributes
  );

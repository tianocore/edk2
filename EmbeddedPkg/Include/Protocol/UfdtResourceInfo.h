/** @file
  Prepared resources for FDT device consumers.

  Each device-specific resource protocol carries UFDT_RESOURCE_PROTOCOL.
  Its Info points to the corresponding snapshot below. Legacy PCD consumers
  use the first prepared device; setup or publication failure stops selection.
  Prepared, unselected candidates remain in their driver's list. Fixed clocks
  publish directly during configuration. Consumers call the optional MapMmio
  callback before accessing the selected device.

  These native-endian snapshots are producer-owned and valid until
  ExitBootServices(). Runtime drivers must copy required values into their own
  runtime storage. Resource data is immutable; only the producer may update
  the Common mapping state. These interfaces are not runtime protocols.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi/UefiBaseType.h>
#include <Protocol/UfdtResource.h>

/** State shared by all device-specific resource snapshots.

  Each UFDT_*_INFO structure starts with this header. The producer initializes
  MmioMapped to FALSE. Only its MapMmio callback may update this field, after
  all required mappings succeed. Resources without a callback leave it FALSE.
*/
typedef struct {
  /// TRUE after the device's MapMmio callback has completed successfully.
  BOOLEAN    MmioMapped;
} UFDT_COMMON;

// UFDT_RESOURCE_PROTOCOL wraps each device-specific snapshot below.

/** CPU physical MMIO region.

  Length is in bytes, not an inclusive limit.
*/
typedef struct {
  /// Base after translation through ancestor bus ranges.
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  /// Byte count; a zero length denotes an unpopulated optional region.
  UINT64                  Length;
} UFDT_MEMORY_REGION;

/// Prepared resource interfaces; their presence also satisfies consumer DEPEX.

/** Absence markers for bindings with no selected device after discovery.

  Each protocol has a NULL interface and is installed on its binding handle
  after all probing and resource selection finish. It means that no resource
  was published for that binding, including after configuration or publication
  failure. Another binding may have selected a device of the same generic
  class. These markers are valid only during boot services.
**/

/** @file

  Definitions for the Platform Runtime Mechanism (PRM) MMIO elements.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_MMIO_H_
#define PRM_MMIO_H_

#include <Uefi.h>

#pragma pack(push, 1)

///
/// Describes a memory range that needs to be made accessible at OS runtime.
///
/// The memory range with the given base address and length will be marked as EFI_MEMORY_RUNTIME.
///
typedef struct {
  EFI_PHYSICAL_ADDRESS    PhysicalBaseAddress;
  EFI_PHYSICAL_ADDRESS    VirtualBaseAddress;
  UINT32                  Length;
} PRM_RUNTIME_MMIO_RANGE;

///
/// Describes a buffer with an array of PRM module
/// config runtime memory ranges.
///
typedef struct {
  ///
  /// The number of runtime memory range elements in this buffer.
  ///
  UINT64                    Count;
  ///
  /// The beginning of the runtime memory range data.
  ///
  PRM_RUNTIME_MMIO_RANGE    Range[1];
} PRM_RUNTIME_MMIO_RANGES;

#pragma pack(pop)

#endif

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

/** Architectural timer dispatcher struct.

  Struct associated with gEdkiiUfdtTimerInfoProtocolGuid

  Architectural INTIDs, not raw DT PPI numbers.
  The first three interrupts are mandatory; absent optional interrupts are 0.
  The architectural timer uses system registers and has no MMIO region.
*/
typedef struct {
  /// Common Ufdt information
  UFDT_COMMON    Common;
  /// Secure physical timer INTID.
  UINT32         SecurePhysicalInterrupt;
  /// Non-secure physical timer INTID.
  UINT32         NonSecurePhysicalInterrupt;
  /// Virtual timer INTID.
  UINT32         VirtualInterrupt;
  /// Hypervisor physical timer INTID; zero when absent.
  UINT32         HypervisorPhysicalInterrupt;
  /// Hypervisor virtual timer INTID; zero when absent.
  UINT32         HypervisorVirtualInterrupt;
} UFDT_TIMER_INFO;

/** GICv2 dispatcher struct.

  Struct associated with gEdkiiUfdtGicV2InfoProtocolGuid

  Only the distributor and CPU interface consumed by ArmGicDxe are listed.
*/
typedef struct {
  /// Common Ufdt information
  UFDT_COMMON           Common;
  /// GIC distributor register region.
  UFDT_MEMORY_REGION    Distributor;
  /// GICv2 CPU interface register region.
  UFDT_MEMORY_REGION    CpuInterface;
} UFDT_GIC_V2_INFO;

/** GICv3 dispatcher struct.

  Struct associated with gEdkiiUfdtGicV3InfoProtocolGuid

  The CPU interface uses system registers and needs no MMIO region.
*/
typedef struct {
  /// Common Ufdt information
  UFDT_COMMON           Common;
  /// GIC distributor register region.
  UFDT_MEMORY_REGION    Distributor;
  /// Contiguous GICv3 redistributor register region.
  UFDT_MEMORY_REGION    Redistributors;
} UFDT_GIC_V3_INFO;

/// Prepared resource interfaces; their presence also satisfies consumer DEPEX.
extern EFI_GUID  gEdkiiUfdtGicV2InfoProtocolGuid;
extern EFI_GUID  gEdkiiUfdtGicV3InfoProtocolGuid;
extern EFI_GUID  gEdkiiUfdtTimerInfoProtocolGuid;
extern EFI_GUID  gEdkiiUfdtClockInfoProtocolGuid;

/** Absence markers for bindings with no selected device after discovery.

  Each protocol has a NULL interface and is installed on its binding handle
  after all probing and resource selection finish. It means that no resource
  was published for that binding, including after configuration or publication
  failure. Another binding may have selected a device of the same generic
  class. These markers are valid only during boot services.
**/
extern EFI_GUID  gEdkiiUfdtGicV2AbsentProtocolGuid;
extern EFI_GUID  gEdkiiUfdtGicV3AbsentProtocolGuid;
extern EFI_GUID  gEdkiiUfdtTimerAbsentProtocolGuid;
extern EFI_GUID  gEdkiiUfdtClockAbsentProtocolGuid;

/** Fixed-clock dispatcher struct.

  Struct associated with gEdkiiUfdtClockInfoProtocolGuid
*/
typedef struct {
  /// Common Ufdt information
  UFDT_COMMON    Common;
  /// Fixed, nonzero output frequency in hertz.
  UINT64         Frequency;
} UFDT_CLOCK_INFO;

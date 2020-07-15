/** @file
  Simple wrapper functions and utility functions that access QEMU's modern CPU
  hotplug register block.

  These functions manipulate some of the registers described in
  "docs/specs/acpi_cpu_hotplug.txt" in the QEMU source. IO Ports are accessed
  via EFI_MM_CPU_IO_PROTOCOL. If a protocol call fails, these functions don't
  return.

  Copyright (c) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef QEMU_CPUHP_H_
#define QEMU_CPUHP_H_

#include <Protocol/MmCpuIo.h>  // EFI_MM_CPU_IO_PROTOCOL
#include <Uefi/UefiBaseType.h> // EFI_STATUS

#include "ApicId.h"            // APIC_ID

UINT32
QemuCpuhpReadCommandData2 (
  IN CONST EFI_MM_CPU_IO_PROTOCOL *MmCpuIo
  );

UINT8
QemuCpuhpReadCpuStatus (
  IN CONST EFI_MM_CPU_IO_PROTOCOL *MmCpuIo
  );

UINT32
QemuCpuhpReadCommandData (
  IN CONST EFI_MM_CPU_IO_PROTOCOL *MmCpuIo
  );

VOID
QemuCpuhpWriteCpuSelector (
  IN CONST EFI_MM_CPU_IO_PROTOCOL *MmCpuIo,
  IN UINT32                       Selector
  );

VOID
QemuCpuhpWriteCommand (
  IN CONST EFI_MM_CPU_IO_PROTOCOL *MmCpuIo,
  IN UINT8                        Command
  );

EFI_STATUS
QemuCpuhpCollectApicIds (
  IN  CONST EFI_MM_CPU_IO_PROTOCOL *MmCpuIo,
  IN  UINT32                       PossibleCpuCount,
  IN  UINT32                       ApicIdCount,
  OUT APIC_ID                      *PluggedApicIds,
  OUT UINT32                       *PluggedCount,
  OUT APIC_ID                      *ToUnplugApicIds,
  OUT UINT32                       *ToUnplugCount
  );

#endif // QEMU_CPUHP_H_

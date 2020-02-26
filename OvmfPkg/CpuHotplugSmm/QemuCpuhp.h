/** @file
  Simple wrapper functions that access QEMU's modern CPU hotplug register
  block.

  These functions thinly wrap some of the registers described in
  "docs/specs/acpi_cpu_hotplug.txt" in the QEMU source. IO Ports are accessed
  via EFI_MM_CPU_IO_PROTOCOL. If a protocol call fails, these functions don't
  return.

  Copyright (c) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef QEMU_CPUHP_H_
#define QEMU_CPUHP_H_

#include <Protocol/MmCpuIo.h>  // EFI_MM_CPU_IO_PROTOCOL

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

#endif // QEMU_CPUHP_H_

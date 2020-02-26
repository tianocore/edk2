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

#include <IndustryStandard/Q35MchIch9.h>     // ICH9_CPU_HOTPLUG_BASE
#include <IndustryStandard/QemuCpuHotplug.h> // QEMU_CPUHP_R_CMD_DATA2
#include <Library/BaseLib.h>                 // CpuDeadLoop()
#include <Library/DebugLib.h>                // DEBUG()

#include "QemuCpuhp.h"

UINT32
QemuCpuhpReadCommandData2 (
  IN CONST EFI_MM_CPU_IO_PROTOCOL *MmCpuIo
  )
{
  UINT32     CommandData2;
  EFI_STATUS Status;

  CommandData2 = 0;
  Status = MmCpuIo->Io.Read (
                         MmCpuIo,
                         MM_IO_UINT32,
                         ICH9_CPU_HOTPLUG_BASE + QEMU_CPUHP_R_CMD_DATA2,
                         1,
                         &CommandData2
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __FUNCTION__, Status));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
  return CommandData2;
}

UINT8
QemuCpuhpReadCpuStatus (
  IN CONST EFI_MM_CPU_IO_PROTOCOL *MmCpuIo
  )
{
  UINT8      CpuStatus;
  EFI_STATUS Status;

  CpuStatus = 0;
  Status = MmCpuIo->Io.Read (
                         MmCpuIo,
                         MM_IO_UINT8,
                         ICH9_CPU_HOTPLUG_BASE + QEMU_CPUHP_R_CPU_STAT,
                         1,
                         &CpuStatus
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __FUNCTION__, Status));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
  return CpuStatus;
}

UINT32
QemuCpuhpReadCommandData (
  IN CONST EFI_MM_CPU_IO_PROTOCOL *MmCpuIo
  )
{
  UINT32     CommandData;
  EFI_STATUS Status;

  CommandData = 0;
  Status = MmCpuIo->Io.Read (
                         MmCpuIo,
                         MM_IO_UINT32,
                         ICH9_CPU_HOTPLUG_BASE + QEMU_CPUHP_RW_CMD_DATA,
                         1,
                         &CommandData
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __FUNCTION__, Status));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
  return CommandData;
}

VOID
QemuCpuhpWriteCpuSelector (
  IN CONST EFI_MM_CPU_IO_PROTOCOL *MmCpuIo,
  IN UINT32                       Selector
  )
{
  EFI_STATUS Status;

  Status = MmCpuIo->Io.Write (
                         MmCpuIo,
                         MM_IO_UINT32,
                         ICH9_CPU_HOTPLUG_BASE + QEMU_CPUHP_W_CPU_SEL,
                         1,
                         &Selector
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __FUNCTION__, Status));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
}

VOID
QemuCpuhpWriteCommand (
  IN CONST EFI_MM_CPU_IO_PROTOCOL *MmCpuIo,
  IN UINT8                        Command
  )
{
  EFI_STATUS Status;

  Status = MmCpuIo->Io.Write (
                         MmCpuIo,
                         MM_IO_UINT8,
                         ICH9_CPU_HOTPLUG_BASE + QEMU_CPUHP_W_CMD,
                         1,
                         &Command
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __FUNCTION__, Status));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
}

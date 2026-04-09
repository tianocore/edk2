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

#include <IndustryStandard/Q35MchIch9.h>     // ICH9_CPU_HOTPLUG_BASE
#include <IndustryStandard/QemuCpuHotplug.h> // QEMU_CPUHP_R_CMD_DATA2
#include <Library/BaseLib.h>                 // CpuDeadLoop()
#include <Library/DebugLib.h>                // DEBUG()

#include "QemuCpuhp.h"

UINT32
QemuCpuhpReadCommandData2 (
  IN CONST EFI_MM_CPU_IO_PROTOCOL  *MmCpuIo
  )
{
  UINT32      CommandData2;
  EFI_STATUS  Status;

  CommandData2 = 0;
  Status       = MmCpuIo->Io.Read (
                               MmCpuIo,
                               MM_IO_UINT32,
                               ICH9_CPU_HOTPLUG_BASE + QEMU_CPUHP_R_CMD_DATA2,
                               1,
                               &CommandData2
                               );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, Status));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  return CommandData2;
}

UINT8
QemuCpuhpReadCpuStatus (
  IN CONST EFI_MM_CPU_IO_PROTOCOL  *MmCpuIo
  )
{
  UINT8       CpuStatus;
  EFI_STATUS  Status;

  CpuStatus = 0;
  Status    = MmCpuIo->Io.Read (
                            MmCpuIo,
                            MM_IO_UINT8,
                            ICH9_CPU_HOTPLUG_BASE + QEMU_CPUHP_R_CPU_STAT,
                            1,
                            &CpuStatus
                            );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, Status));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  return CpuStatus;
}

UINT32
QemuCpuhpReadCommandData (
  IN CONST EFI_MM_CPU_IO_PROTOCOL  *MmCpuIo
  )
{
  UINT32      CommandData;
  EFI_STATUS  Status;

  CommandData = 0;
  Status      = MmCpuIo->Io.Read (
                              MmCpuIo,
                              MM_IO_UINT32,
                              ICH9_CPU_HOTPLUG_BASE + QEMU_CPUHP_RW_CMD_DATA,
                              1,
                              &CommandData
                              );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, Status));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  return CommandData;
}

VOID
QemuCpuhpWriteCpuSelector (
  IN CONST EFI_MM_CPU_IO_PROTOCOL  *MmCpuIo,
  IN UINT32                        Selector
  )
{
  EFI_STATUS  Status;

  Status = MmCpuIo->Io.Write (
                         MmCpuIo,
                         MM_IO_UINT32,
                         ICH9_CPU_HOTPLUG_BASE + QEMU_CPUHP_W_CPU_SEL,
                         1,
                         &Selector
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, Status));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
}

VOID
QemuCpuhpWriteCpuStatus (
  IN CONST EFI_MM_CPU_IO_PROTOCOL  *MmCpuIo,
  IN UINT8                         CpuStatus
  )
{
  EFI_STATUS  Status;

  Status = MmCpuIo->Io.Write (
                         MmCpuIo,
                         MM_IO_UINT8,
                         ICH9_CPU_HOTPLUG_BASE + QEMU_CPUHP_R_CPU_STAT,
                         1,
                         &CpuStatus
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, Status));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
}

VOID
QemuCpuhpWriteCommand (
  IN CONST EFI_MM_CPU_IO_PROTOCOL  *MmCpuIo,
  IN UINT8                         Command
  )
{
  EFI_STATUS  Status;

  Status = MmCpuIo->Io.Write (
                         MmCpuIo,
                         MM_IO_UINT8,
                         ICH9_CPU_HOTPLUG_BASE + QEMU_CPUHP_W_CMD,
                         1,
                         &Command
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, Status));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
}

/**
  Collect the APIC IDs of
  - the CPUs that have been hot-plugged,
  - the CPUs that are about to be hot-unplugged.

  This function only scans for events -- it does not modify them -- in the
  hotplug registers.

  On error, the contents of the output parameters are undefined.

  @param[in] MmCpuIo             The EFI_MM_CPU_IO_PROTOCOL instance for
                                 accessing IO Ports.

  @param[in] PossibleCpuCount    The number of possible CPUs in the system. Must
                                 be positive.

  @param[in] ApicIdCount         The number of elements each one of the
                                 PluggedApicIds and ToUnplugApicIds arrays can
                                 accommodate. Must be positive.

  @param[out] PluggedApicIds     The APIC IDs of the CPUs that have been
                                 hot-plugged.

  @param[out] PluggedCount       The number of filled-in APIC IDs in
                                 PluggedApicIds.

  @param[out] ToUnplugApicIds    The APIC IDs of the CPUs that are about to be
                                 hot-unplugged.

  @param[out] ToUnplugSelectors  The QEMU Selectors of the CPUs that are about
                                 to be hot-unplugged.

  @param[out] ToUnplugCount      The number of filled-in APIC IDs in
                                 ToUnplugApicIds.

  @retval EFI_INVALID_PARAMETER  PossibleCpuCount is zero, or ApicIdCount is
                                 zero.

  @retval EFI_PROTOCOL_ERROR     Invalid bitmap detected in the
                                 QEMU_CPUHP_R_CPU_STAT register.

  @retval EFI_BUFFER_TOO_SMALL   There was an attempt to place more than
                                 ApicIdCount APIC IDs into one of the
                                 PluggedApicIds and ToUnplugApicIds arrays.

  @retval EFI_SUCCESS            Output parameters have been set successfully.
**/
EFI_STATUS
QemuCpuhpCollectApicIds (
  IN  CONST EFI_MM_CPU_IO_PROTOCOL  *MmCpuIo,
  IN  UINT32                        PossibleCpuCount,
  IN  UINT32                        ApicIdCount,
  OUT APIC_ID                       *PluggedApicIds,
  OUT UINT32                        *PluggedCount,
  OUT APIC_ID                       *ToUnplugApicIds,
  OUT UINT32                        *ToUnplugSelectors,
  OUT UINT32                        *ToUnplugCount
  )
{
  UINT32  CurrentSelector;

  if ((PossibleCpuCount == 0) || (ApicIdCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  *PluggedCount  = 0;
  *ToUnplugCount = 0;

  CurrentSelector = 0;
  do {
    UINT32   PendingSelector;
    UINT8    CpuStatus;
    APIC_ID  *ExtendIds;
    UINT32   *ExtendSels;
    UINT32   *ExtendCount;
    APIC_ID  NewApicId;

    //
    // Write CurrentSelector (which is valid) to the CPU selector register.
    // Consequences:
    //
    // - Other register accesses will be permitted.
    //
    // - The QEMU_CPUHP_CMD_GET_PENDING command will start scanning for a CPU
    //   with pending events at CurrentSelector (inclusive).
    //
    QemuCpuhpWriteCpuSelector (MmCpuIo, CurrentSelector);
    //
    // Write the QEMU_CPUHP_CMD_GET_PENDING command. Consequences
    // (independently of each other):
    //
    // - If there is a CPU with pending events, starting at CurrentSelector
    //   (inclusive), the CPU selector will be updated to that CPU. Note that
    //   the scanning in QEMU may wrap around, because we must never clear the
    //   event bits.
    //
    // - The QEMU_CPUHP_RW_CMD_DATA register will return the (possibly updated)
    //   CPU selector value.
    //
    QemuCpuhpWriteCommand (MmCpuIo, QEMU_CPUHP_CMD_GET_PENDING);
    PendingSelector = QemuCpuhpReadCommandData (MmCpuIo);
    if (PendingSelector < CurrentSelector) {
      DEBUG ((
        DEBUG_VERBOSE,
        "%a: CurrentSelector=%u PendingSelector=%u: "
        "wrap-around\n",
        __func__,
        CurrentSelector,
        PendingSelector
        ));
      break;
    }

    CurrentSelector = PendingSelector;

    //
    // Check the known status / event bits for the currently selected CPU.
    //
    CpuStatus = QemuCpuhpReadCpuStatus (MmCpuIo);
    if ((CpuStatus & QEMU_CPUHP_STAT_INSERT) != 0) {
      //
      // The "insert" event guarantees the "enabled" status; plus it excludes
      // the "fw_remove" event.
      //
      if (((CpuStatus & QEMU_CPUHP_STAT_ENABLED) == 0) ||
          ((CpuStatus & QEMU_CPUHP_STAT_FW_REMOVE) != 0))
      {
        DEBUG ((
          DEBUG_ERROR,
          "%a: CurrentSelector=%u CpuStatus=0x%x: "
          "inconsistent CPU status\n",
          __func__,
          CurrentSelector,
          CpuStatus
          ));
        return EFI_PROTOCOL_ERROR;
      }

      DEBUG ((
        DEBUG_VERBOSE,
        "%a: CurrentSelector=%u: insert\n",
        __func__,
        CurrentSelector
        ));

      ExtendIds   = PluggedApicIds;
      ExtendSels  = NULL;
      ExtendCount = PluggedCount;
    } else if ((CpuStatus & QEMU_CPUHP_STAT_FW_REMOVE) != 0) {
      //
      // "fw_remove" event guarantees "enabled".
      //
      if ((CpuStatus & QEMU_CPUHP_STAT_ENABLED) == 0) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: CurrentSelector=%u CpuStatus=0x%x: "
          "inconsistent CPU status\n",
          __func__,
          CurrentSelector,
          CpuStatus
          ));
        return EFI_PROTOCOL_ERROR;
      }

      DEBUG ((
        DEBUG_VERBOSE,
        "%a: CurrentSelector=%u: fw_remove\n",
        __func__,
        CurrentSelector
        ));

      ExtendIds   = ToUnplugApicIds;
      ExtendSels  = ToUnplugSelectors;
      ExtendCount = ToUnplugCount;
    } else if ((CpuStatus & QEMU_CPUHP_STAT_REMOVE) != 0) {
      //
      // Let the OSPM deal with the "remove" event.
      //
      DEBUG ((
        DEBUG_VERBOSE,
        "%a: CurrentSelector=%u: remove (ignored)\n",
        __func__,
        CurrentSelector
        ));

      ExtendIds   = NULL;
      ExtendSels  = NULL;
      ExtendCount = NULL;
    } else {
      DEBUG ((
        DEBUG_VERBOSE,
        "%a: CurrentSelector=%u: no event\n",
        __func__,
        CurrentSelector
        ));
      break;
    }

    ASSERT ((ExtendIds == NULL) == (ExtendCount == NULL));
    ASSERT ((ExtendSels == NULL) || (ExtendIds != NULL));

    if (ExtendIds != NULL) {
      //
      // Save the APIC ID of the CPU with the pending event, to the
      // corresponding APIC ID array.
      // For unplug events, also save the CurrentSelector.
      //
      if (*ExtendCount == ApicIdCount) {
        DEBUG ((DEBUG_ERROR, "%a: APIC ID array too small\n", __func__));
        return EFI_BUFFER_TOO_SMALL;
      }

      QemuCpuhpWriteCommand (MmCpuIo, QEMU_CPUHP_CMD_GET_ARCH_ID);
      NewApicId = QemuCpuhpReadCommandData (MmCpuIo);
      DEBUG ((
        DEBUG_VERBOSE,
        "%a: ApicId=" FMT_APIC_ID "\n",
        __func__,
        NewApicId
        ));
      if (ExtendSels != NULL) {
        ExtendSels[(*ExtendCount)] = CurrentSelector;
      }

      ExtendIds[(*ExtendCount)++] = NewApicId;
    }

    //
    // We've processed the CPU with (known) pending events, but we must never
    // clear events. Therefore we need to advance past this CPU manually;
    // otherwise, QEMU_CPUHP_CMD_GET_PENDING would stick to the currently
    // selected CPU.
    //
    CurrentSelector++;
  } while (CurrentSelector < PossibleCpuCount);

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: PluggedCount=%u ToUnplugCount=%u\n",
    __func__,
    *PluggedCount,
    *ToUnplugCount
    ));
  return EFI_SUCCESS;
}

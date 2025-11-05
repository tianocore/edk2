/** @file
  Various defines for qemu microvm

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MICROVM_H__
#define __MICROVM_H__

#define MICROVM_PSEUDO_DEVICE_ID  0xfff1

/* generic event device */
#define MICROVM_GED_MMIO_BASE           0xfea00000
#define MICROVM_GED_MMIO_BASE_REGS      (MICROVM_GED_MMIO_BASE + 0x200)
#define MICROVM_ACPI_GED_REG_SLEEP_CTL  0x00
#define MICROVM_ACPI_GED_REG_RESET      0x02
#define MICROVM_ACPI_GED_RESET_VALUE    0x42

#endif // __MICROVM_H__

/** @file
  Macros for accessing QEMU's CPU hotplug register block.

  Copyright (C) 2019, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:

  - "docs/specs/acpi_cpu_hotplug.txt" in the QEMU source tree.

    The original (now "legacy") CPU hotplug interface appeared in QEMU v1.5.0.
    The new ("modern") hotplug interface appeared in QEMU v2.7.0.

    The macros in this header file map to the minimal subset of the modern
    interface that OVMF needs.
**/

#ifndef QEMU_CPU_HOTPLUG_H_
#define QEMU_CPU_HOTPLUG_H_

#include <Base.h>

//
// Each register offset is:
// - relative to the board-dependent IO base address of the register block,
// - named QEMU_CPUHP_(R|W|RW)_*, according to the possible access modes of the
//   register,
// - followed by distinguished bitmasks or values in the register.
//
#define QEMU_CPUHP_R_CMD_DATA2               0x0

#define QEMU_CPUHP_R_CPU_STAT                0x4
#define QEMU_CPUHP_STAT_ENABLED                BIT0
#define QEMU_CPUHP_STAT_INSERT                 BIT1
#define QEMU_CPUHP_STAT_REMOVE                 BIT2

#define QEMU_CPUHP_RW_CMD_DATA               0x8

#define QEMU_CPUHP_W_CPU_SEL                 0x0

#define QEMU_CPUHP_W_CMD                     0x5
#define QEMU_CPUHP_CMD_GET_PENDING             0x0
#define QEMU_CPUHP_CMD_GET_ARCH_ID             0x3

#endif // QEMU_CPU_HOTPLUG_H_

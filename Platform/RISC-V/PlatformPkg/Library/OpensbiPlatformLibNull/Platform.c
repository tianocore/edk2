/*
 *
 * Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra <atish.patra@wdc.com>
 */

#include <sbi/sbi_const.h>
#include <sbi/sbi_platform.h>

const struct sbi_platform_operations platform_ops = {
    .pmp_region_count   = NULL,
    .pmp_region_info    = NULL,
    .final_init         = NULL,
    .console_putc       = NULL,
    .console_getc       = NULL,
    .console_init       = NULL,
    .irqchip_init       = NULL,
    .ipi_send           = NULL,
    .ipi_clear          = NULL,
    .ipi_init           = NULL,
    .timer_value        = NULL,
    .timer_event_stop   = NULL,
    .timer_event_start  = NULL,
    .timer_init         = NULL,
    .system_reboot      = NULL,
    .system_shutdown    = NULL
};

const struct sbi_platform platform = {
    .opensbi_version    = OPENSBI_VERSION,                      // The OpenSBI version this platform table is built bassed on.
    .platform_version   = SBI_PLATFORM_VERSION(0x0000, 0x0000), // SBI Platform version 1.0
    .name               = "NULL platform",
    .features           = 0,
    .hart_count         = 0,
    .hart_stack_size    = 0,
    .disabled_hart_mask = 0,
    .platform_ops_addr  = 0
};

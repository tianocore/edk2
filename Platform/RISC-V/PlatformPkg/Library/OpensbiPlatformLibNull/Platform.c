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
    .early_init         = NULL,
    .final_init         = NULL,
    .early_exit         = NULL,
    .final_exit         = NULL,
    .domains_root_regions = NULL,
    .domains_init       = NULL,
    .console_putc       = NULL,
    .console_getc       = NULL,
    .console_init       = NULL,
    .irqchip_init       = NULL,
    .irqchip_exit       = NULL,
    .ipi_send           = NULL,
    .ipi_clear          = NULL,
    .ipi_init           = NULL,
    .ipi_exit           = NULL,
    .get_tlbr_flush_limit = NULL,
    .timer_value        = NULL,
    .timer_event_stop   = NULL,
    .timer_event_start  = NULL,
    .timer_init         = NULL,
    .timer_exit         = NULL,
    .system_reset_check = NULL,
    .system_reset       = NULL,
};

struct sbi_platform platform = {
    .opensbi_version    = OPENSBI_VERSION,
    .platform_version   = SBI_PLATFORM_VERSION(0x0, 0x01),
    .name               = "NULL Platform",
    .features           = 0,
    .hart_count         = 0,
    .hart_index2id      = 0,
    .hart_stack_size    = 0,
    .platform_ops_addr  = 0,
};

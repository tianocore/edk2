/*
  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

 */

#include <Library/DebugAgentLib.h>
#include <Library/DebugLib.h>

#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_math.h>
#include <sbi_utils/fdt/fdt_domain.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/irqchip/fdt_irqchip.h>
#include <sbi_utils/serial/fdt_serial.h>
#include <sbi_utils/timer/fdt_timer.h>
#include <sbi_utils/ipi/fdt_ipi.h>
#include <sbi_utils/reset/fdt_reset.h>

#include "SecMain.h"

extern struct sbi_platform_operations platform_ops;

int Edk2OpensbiPlatformEarlyInit (
    BOOLEAN ColdBoot
    )
{
    int ReturnCode;

    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.early_init) {
        ReturnCode = platform_ops.early_init (ColdBoot);
        if (ReturnCode) {
            return ReturnCode;
        }
    }
    if (ColdBoot == TRUE) {
        return SecPostOpenSbiPlatformEarlylInit(ColdBoot);
    }
    return 0;
}

int Edk2OpensbiPlatformFinalInit (
    BOOLEAN ColdBoot
    )
{
    int ReturnCode;

    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.final_init) {
        ReturnCode = platform_ops.final_init (ColdBoot);
        if (ReturnCode) {
            return ReturnCode;
        }
    }
    if (ColdBoot == TRUE) {
        return SecPostOpenSbiPlatformFinalInit(ColdBoot);
    }
    return 0;
}

VOID Edk2OpensbiPlatformEarlyExit (
    VOID
    )
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.early_exit) {
        return platform_ops.early_exit ();
    }
}

/** Platform final exit */
VOID Edk2OpensbiPlatformFinalExit (
    VOID
    )
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.early_exit) {
        return platform_ops.early_exit ();
    }
}

/**
  For platforms that do not implement misa, non-standard
  methods are needed to determine cpu extension.
**/
int Edk2OpensbiPlatforMMISACheckExtension (
    CHAR8 Extension
    )
{
    if (platform_ops.misa_check_extension) {
        return platform_ops.misa_check_extension (Extension);
    }
    return 0;
}

/**
  For platforms that do not implement misa, non-standard
  methods are needed to get MXL field of misa.
**/
int Edk2OpensbiPlatforMMISAGetXLEN (VOID)
{
    if (platform_ops.misa_get_xlen) {
        return platform_ops.misa_get_xlen ();
    }
    return 0;
}

/** Initialize (or populate) domains for the platform */
int Edk2OpensbiPlatformDomainsInit (VOID)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.domains_init) {
        return platform_ops.domains_init ();
    }
    return 0;
}

/** Initialize the platform console */
int Edk2OpensbiPlatformSerialInit (VOID)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.console_init) {
        return platform_ops.console_init ();
    }
    return 0;
}

/** Initialize the platform interrupt controller for current HART */
int Edk2OpensbiPlatformIrqchipInit (
    BOOLEAN ColdBoot
    )
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.irqchip_init) {
        return platform_ops.irqchip_init (ColdBoot);
    }
    return 0;
}

/** Exit the platform interrupt controller for current HART */
VOID Edk2OpensbiPlatformIrqchipExit (VOID)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.irqchip_exit) {
        return platform_ops.irqchip_exit ();
    }
}

/** Initialize IPI for current HART */
int Edk2OpensbiPlatformIpiInit (
    BOOLEAN ColdBoot
    )
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.ipi_init) {
        return platform_ops.ipi_init (ColdBoot);
    }
    return 0;
}

/** Exit IPI for current HART */
VOID Edk2OpensbiPlatformIpiExit (VOID)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.ipi_exit) {
        return platform_ops.ipi_exit ();
    }
}

/** Get tlb flush limit value **/
UINT64 Edk2OpensbiPlatformTlbrFlushLimit (VOID)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.get_tlbr_flush_limit) {
        return platform_ops.get_tlbr_flush_limit ();
    }
    return 0;
}

/** Initialize platform timer for current HART */
int Edk2OpensbiPlatformTimerInit (
    BOOLEAN ColdBoot
    )
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.timer_init) {
        return platform_ops.timer_init (ColdBoot);
    }
    return 0;
}

/** Exit platform timer for current HART */
VOID Edk2OpensbiPlatformTimerExit (VOID)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.timer_exit) {
        return platform_ops.timer_exit ();
    }
}

/** platform specific SBI extension implementation probe function */
int Edk2OpensbiPlatformVendorExtCheck (
    long ExtId
    )
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.vendor_ext_check) {
        return platform_ops.vendor_ext_check (ExtId);
    }
    return 0;
}


/** platform specific SBI extension implementation provider */
int Edk2OpensbiPlatformVendorExtProvider (
    long ExtId,
    long FuncId,
    const struct sbi_trap_regs *Regs,
    unsigned long *OutValue,
    struct sbi_trap_info *OutTrap
    )
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.vendor_ext_provider) {
        return platform_ops.vendor_ext_provider (
                                ExtId,
                                FuncId,
                                Regs,
                                OutValue,
                                OutTrap
                                );
    }
    return 0;
}

const struct sbi_platform_operations Edk2OpensbiPlatformOps = {
    .early_init             = Edk2OpensbiPlatformEarlyInit,
    .final_init             = Edk2OpensbiPlatformFinalInit,
    .early_exit             = Edk2OpensbiPlatformEarlyExit,
    .final_exit             = Edk2OpensbiPlatformFinalExit,
    .misa_check_extension   = Edk2OpensbiPlatforMMISACheckExtension,
    .misa_get_xlen          = Edk2OpensbiPlatforMMISAGetXLEN,
    .domains_init           = Edk2OpensbiPlatformDomainsInit,
    .console_init           = Edk2OpensbiPlatformSerialInit,
    .irqchip_init           = Edk2OpensbiPlatformIrqchipInit,
    .irqchip_exit           = Edk2OpensbiPlatformIrqchipExit,
    .ipi_init               = Edk2OpensbiPlatformIpiInit,
    .ipi_exit               = Edk2OpensbiPlatformIpiExit,
    .get_tlbr_flush_limit   = Edk2OpensbiPlatformTlbrFlushLimit,
    .timer_init             = Edk2OpensbiPlatformTimerInit,
    .timer_exit             = Edk2OpensbiPlatformTimerExit,
    .vendor_ext_check       = Edk2OpensbiPlatformVendorExtCheck,
    .vendor_ext_provider    = Edk2OpensbiPlatformVendorExtProvider,
};

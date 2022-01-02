/** @file
  EDK2 OpenSBI generic platform wrapper library

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <Library/DebugAgentLib.h>
#include <Library/DebugLib.h>
#include <IndustryStandard/RiscVOpensbi.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_math.h>

extern struct sbi_platform_operations platform_ops;
extern atomic_t BootHartDone;

/**
  Add firmware memory domain.

  @retval  OpenSBI error code.

**/
INT32
SecSetEdk2FwMemoryRegions (
  VOID
  )
{
  INT32 Ret;
  struct sbi_domain_memregion fw_memregs;

  Ret = 0;

  //
  // EDK2 PEI domain memory region
  //
  fw_memregs.order = log2roundup(FixedPcdGet32(PcdFirmwareDomainSize));
  fw_memregs.base = FixedPcdGet32(PcdFirmwareDomainBaseAddress);
  fw_memregs.flags = SBI_DOMAIN_MEMREGION_EXECUTABLE | SBI_DOMAIN_MEMREGION_READABLE;
  Ret = sbi_domain_root_add_memregion ((CONST struct sbi_domain_memregion *)&fw_memregs);
  if (Ret != 0) {
    DEBUG ((DEBUG_ERROR, "%a: Add firmware regiosn of FW Domain fail\n", __FUNCTION__));
  }

  //
  // EDK2 EFI Variable domain memory region
  //
  fw_memregs.order = log2roundup(FixedPcdGet32(PcdVariableFirmwareRegionSize));
  fw_memregs.base = FixedPcdGet32(PcdVariableFirmwareRegionBaseAddress);
  fw_memregs.flags = SBI_DOMAIN_MEMREGION_READABLE | SBI_DOMAIN_MEMREGION_WRITEABLE;
  Ret = sbi_domain_root_add_memregion ((CONST struct sbi_domain_memregion *)&fw_memregs);
  if (Ret != 0) {
    DEBUG ((DEBUG_ERROR, "%a: Add firmware regiosn of variable FW Domain fail\n", __FUNCTION__));
  }
  return Ret;
}
/**
  OpenSBI platform early init hook.

  @param[in]   ColdBoot  Is cold boot path or warm boot path.
  @retval      OpenSBI error code.

**/
INT32
SecPostOpenSbiPlatformEarlylInit(
  IN BOOLEAN ColdBoot
  )
{
  UINT32 HartId;

  if (!ColdBoot) {
    HartId = current_hartid();
    DEBUG ((DEBUG_INFO, "%a: Non boot hart %d.\n", __FUNCTION__, HartId));
    return 0;
  }
  //
  // Setup firmware memory region.
  //
  if (SecSetEdk2FwMemoryRegions () != 0) {
    ASSERT (FALSE);
  }

  //
  // Boot HART is already in the process of OpenSBI initialization.
  // We can let other HART to keep booting.
  //
  DEBUG ((DEBUG_INFO, "%a: Set boot hart done.\n", __FUNCTION__));
  atomic_write (&BootHartDone, (UINT64)TRUE);
  return 0;
}

/**
  OpenSBI platform final init hook.
  We restore the next_arg1 to the pointer of EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT.

  @param[in]   ColdBoot  Is cold boot path or warm boot path.
  @retval      OpenSBI error code.

**/
INT32
SecPostOpenSbiPlatformFinalInit (
  IN BOOLEAN ColdBoot
  )
{
  UINT32 HartId;
  struct sbi_scratch *SbiScratch;
  struct sbi_scratch *ScratchSpace;
  struct sbi_platform *SbiPlatform;
  EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT *FirmwareContext;

  if (!ColdBoot) {
    HartId = current_hartid();
    DEBUG ((DEBUG_INFO, "%a: Non boot hart %d.\n", __FUNCTION__, HartId));
    return 0;
  }

  DEBUG((DEBUG_INFO, "%a: Entry, preparing to jump to PEI Core\n\n", __FUNCTION__));

  SbiScratch = sbi_scratch_thishart_ptr();
  SbiPlatform = (struct sbi_platform *)sbi_platform_ptr(SbiScratch);
  FirmwareContext = (EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT *)SbiPlatform->firmware_context;

  //
  // Print out scratch address of each hart
  //
  DEBUG ((DEBUG_INFO, "%a: OpenSBI scratch address for each hart:\n", __FUNCTION__));
  for (HartId = 0; HartId < SBI_HARTMASK_MAX_BITS; HartId ++) {
    if (sbi_platform_hart_invalid(SbiPlatform, HartId)) {
      continue;
    }
    ScratchSpace = sbi_hartid_to_scratch (HartId);
    if(ScratchSpace != NULL) {
      DEBUG((DEBUG_INFO, "          Hart %d: 0x%x\n", HartId, ScratchSpace));
    } else {
      DEBUG((DEBUG_INFO, "          Hart %d not initialized yet\n", HartId));
    }
  }

  //
  // Set firmware context Hart-specific pointer
  //
  for (HartId = 0; HartId < SBI_HARTMASK_MAX_BITS; HartId ++) {
    if (sbi_platform_hart_invalid(SbiPlatform, HartId)) {
      continue;
    }
    ScratchSpace = sbi_hartid_to_scratch (HartId);
    if (ScratchSpace != NULL) {
      FirmwareContext->HartSpecific[HartId] =
        (EFI_RISCV_FIRMWARE_CONTEXT_HART_SPECIFIC *)((UINT8 *)ScratchSpace - FIRMWARE_CONTEXT_HART_SPECIFIC_SIZE);
        DEBUG ((DEBUG_INFO, "%a: OpenSBI Hart %d Firmware Context Hart-specific at address: 0x%x\n",
                __FUNCTION__,
                 HartId,
                 FirmwareContext->HartSpecific [HartId]
                 ));
    }
  }

  DEBUG((DEBUG_INFO, "%a: Will jump to PEI Core in OpenSBI with \n", __FUNCTION__));
  DEBUG((DEBUG_INFO, "  sbi_scratch = %x\n", SbiScratch));
  DEBUG((DEBUG_INFO, "  sbi_platform = %x\n", SbiPlatform));
  DEBUG((DEBUG_INFO, "  FirmwareContext = %x\n", FirmwareContext));
  SbiScratch->next_arg1 = (unsigned long)FirmwareContext;

  return 0;
}
/**
  OpenSBI platform early init hook.

  @param[in]   ColdBoot  Is cold boot path or warm boot path.
  @retval      OpenSBI error code.

**/
INT32
Edk2OpensbiPlatformEarlyInit (
  IN BOOLEAN ColdBoot
  )
{
    INT32 ReturnCode;

    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.early_init) {
        ReturnCode = platform_ops.early_init (ColdBoot);
        if (ReturnCode) {
            return ReturnCode;
        }
    }
    if (ColdBoot) {
        return SecPostOpenSbiPlatformEarlylInit(ColdBoot);
    }
    return 0;
}
/**
  OpenSBI platform final init hook.

  @param[in]   ColdBoot  Is cold boot path or warm boot path.
  @retval      OpenSBI error code.

**/
INT32
Edk2OpensbiPlatformFinalInit (
  IN BOOLEAN ColdBoot
  )
{
    INT32 ReturnCode;

    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.final_init) {
        ReturnCode = platform_ops.final_init (ColdBoot);
        if (ReturnCode) {
            return ReturnCode;
        }
    }
    if (ColdBoot) {
        return SecPostOpenSbiPlatformFinalInit(ColdBoot);
    }
    return 0;
}
/**
  OpenSBI platform early exit hook.

**/
VOID
Edk2OpensbiPlatformEarlyExit (
  VOID
)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.early_exit) {
        return platform_ops.early_exit ();
    }
}

/**
 Platform final exit hook

 **/
VOID
Edk2OpensbiPlatformFinalExit (
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

  @param[in]   Extension  Check ISA extension.
  @retval      OpenSBI error code.

**/
INT32
Edk2OpensbiPlatforMMISACheckExtension (
  IN CHAR8 Extension
  )
{
    if (platform_ops.misa_check_extension) {
        return platform_ops.misa_check_extension (Extension);
    }
    return 0;
}

/**
  Get the XLEN.

  @retval Return the XLEN

**/
INT32
Edk2OpensbiPlatforMMISAGetXLEN (
  VOID
)
{
    if (platform_ops.misa_get_xlen) {
        return platform_ops.misa_get_xlen ();
    }
    return 0;
}

/**
  Initialize (or populate) domains for the platform*

  @retval  OpenSBI error code.

**/
INT32
Edk2OpensbiPlatformDomainsInit (
  VOID
)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.domains_init) {
        return platform_ops.domains_init ();
    }
    return 0;
}

/**
 Initialize the platform console

 @retval  OpenSBI error code.

**/
INT32
Edk2OpensbiPlatformSerialInit (
  VOID
)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.console_init) {
        return platform_ops.console_init ();
    }
    return 0;
}

/**
  Initialize the platform interrupt controller for current HART

  @param[in]   ColdBoot  Is cold boot path or warm boot path.
  @retval  OpenSBI error code.

**/
INT32
Edk2OpensbiPlatformIrqchipInit (
  IN BOOLEAN ColdBoot
)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.irqchip_init) {
        return platform_ops.irqchip_init (ColdBoot);
    }
    return 0;
}

/**
 Exit the platform interrupt controller for current HART

**/
VOID
Edk2OpensbiPlatformIrqchipExit (
  VOID
)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.irqchip_exit) {
        return platform_ops.irqchip_exit ();
    }
}

/**
  Initialize IPI for current HART

  @param[in]   ColdBoot  Is cold boot path or warm boot path.
  @retval      OpenSBI error code.

**/
INT32
Edk2OpensbiPlatformIpiInit (
  IN  BOOLEAN ColdBoot
)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.ipi_init) {
        return platform_ops.ipi_init (ColdBoot);
    }
    return 0;
}

/**
 Exit IPI for current HART

**/
VOID
Edk2OpensbiPlatformIpiExit (
  VOID
)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.ipi_exit) {
        return platform_ops.ipi_exit ();
    }
}

/**
  Get tlb flush limit value

  @retval  Cache flush limit value.

**/
UINT64
Edk2OpensbiPlatformTlbrFlushLimit (
  VOID
)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.get_tlbr_flush_limit) {
        return platform_ops.get_tlbr_flush_limit ();
    }
    return 0;
}

/**
  Initialize platform timer for current HART

  @param[in]   ColdBoot  Is cold boot path or warm boot path.
  @retval      OpenSBI error code.

**/
INT32
Edk2OpensbiPlatformTimerInit (
  IN BOOLEAN ColdBoot
)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.timer_init) {
        return platform_ops.timer_init (ColdBoot);
    }
    return 0;
}

/**
 Exit platform timer for current HART

**/
VOID
Edk2OpensbiPlatformTimerExit (
  VOID
)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.timer_exit) {
        return platform_ops.timer_exit ();
    }
}

/**
  Check platform vendor SBI extension.

  @param[in]  ExtId  Extension ID.
  @retval     OpenSBI error code.

 **/
INT32
Edk2OpensbiPlatformVendorExtCheck (
  IN long ExtId
)
{
    DEBUG((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

    if (platform_ops.vendor_ext_check) {
        return platform_ops.vendor_ext_check (ExtId);
    }
    return 0;
}

/**
  Platform specific SBI extension implementation provider

  @param[in]   ExtId    SBI extension ID.
  @param[in]   FuncId   Function ID.
  @param[in]   Regs     The trap register.
  @param[in]   OutValue Value returned from SBI.
  @param[in]   OutTrap  The trap infomation after calling to SBI.

  @retval  OpenSBI error code.

**/
INT32
Edk2OpensbiPlatformVendorExtProvider (
  IN long ExtId,
  IN long FuncId,
  IN CONST struct sbi_trap_regs *Regs,
  IN unsigned long *OutValue,
  IN struct sbi_trap_info *OutTrap
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

CONST struct sbi_platform_operations Edk2OpensbiPlatformOps = {
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

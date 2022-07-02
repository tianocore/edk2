//
//  Copyright (c) 2012-2013, ARM Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

#include <AsmMacroIoLibV8.h>

  .macro mov_i, reg:req, imm:req
  movz   \reg, :abs_g3:\imm
  movk   \reg, :abs_g2_nc:\imm
  movk   \reg, :abs_g1_nc:\imm
  movk   \reg, :abs_g0_nc:\imm
  .endm

 .set    MAIR_DEV_nGnRE,      0x04
 .set    MAIR_MEM_WBWA,       0xff
 .set    mairval, MAIR_DEV_nGnRE | (MAIR_MEM_WBWA << 8)

 .set    TCR_TG0_4KB,         0x0 << 14
 .set    TCR_TG1_4KB,         0x2 << 30
 .set    TCR_IPS_SHIFT,       32
 .set    TCR_EPD1,            0x1 << 23
 .set    TCR_SH_INNER,        0x3 << 12
 .set    TCR_RGN_OWB,         0x1 << 10
 .set    TCR_RGN_IWB,         0x1 << 8
 .set    tcrval, TCR_TG0_4KB | TCR_TG1_4KB | TCR_EPD1 | TCR_RGN_OWB
 .set    tcrval, tcrval | TCR_RGN_IWB | TCR_SH_INNER | (64 - 39) // TCR_T0SZ

 .set    SCTLR_ELx_I,         0x1 << 12
 .set    SCTLR_ELx_SA,        0x1 << 3
 .set    SCTLR_ELx_C,         0x1 << 2
 .set    SCTLR_ELx_M,         0x1 << 0
 .set    SCTLR_EL1_SPAN,      0x1 << 23
 .set    SCTLR_EL1_WXN,       0x1 << 19
 .set    SCTLR_EL1_SED,       0x1 << 8
 .set    SCTLR_EL1_ITD,       0x1 << 7
 .set    SCTLR_EL1_RES1,      (0x1 << 11) | (0x1 << 20) | (0x1 << 22) | (0x1 << 28) | (0x1 << 29)
 .set    sctlrval, SCTLR_ELx_M | SCTLR_ELx_C | SCTLR_ELx_SA | SCTLR_EL1_ITD | SCTLR_EL1_SED
 .set    sctlrval, sctlrval | SCTLR_ELx_I | SCTLR_EL1_SPAN | SCTLR_EL1_RES1


ASM_FUNC(ArmPlatformPeiBootAction)
  mov_i  x0, mairval
  mov_i  x1, tcrval
  adrp   x2, idmap
  mov_i  x3, sctlrval

  mrs    x6, id_aa64mmfr0_el1    // check the supported PA range
  bfi    x1, x6, #32, #4         // Copy PArange into TCR_EL1.IPS

  msr    mair_el1, x0            // set up the 1:1 mapping
  msr    tcr_el1, x1
  msr    ttbr0_el1, x2
  isb

  tlbi   vmalle1                 // invalidate any cached translations
  ic     iallu                   // invalidate the I-cache
  dsb    nsh
  isb

  msr    sctlr_el1, x3           // enable MMU and caches
  isb
  ret

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
// With this function: CorePos = (ClusterId * 4) + CoreId
ASM_FUNC(ArmPlatformGetCorePosition)
  mov   x0, xzr
  ret

//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ASM_FUNC(ArmPlatformGetPrimaryCoreMpId)
  MOV32  (w0, FixedPcdGet32 (PcdArmPrimaryCore))
  ret

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ASM_FUNC(ArmPlatformIsPrimaryCore)
  mov   x0, #1
  ret

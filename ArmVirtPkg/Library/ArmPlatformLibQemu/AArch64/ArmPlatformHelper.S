//
//  Copyright (c) 2022, Google LLC. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

#include <AsmMacroLib.h>

  .macro mov_i, reg:req, imm:req
  movz   \reg, :abs_g1:\imm
  movk   \reg, :abs_g0_nc:\imm
  .endm

 .set    MAIR_DEV_nGnRnE,     0x00
 .set    MAIR_MEM_NC,         0x44
 .set    MAIR_MEM_WT,         0xbb
 .set    MAIR_MEM_WBWA,       0xff
 .set    mairval, MAIR_DEV_nGnRnE | (MAIR_MEM_NC << 8) | (MAIR_MEM_WT << 16) | (MAIR_MEM_WBWA << 24)

 .set    TCR_TG0_4KB,         0x0 << 14
 .set    TCR_TG1_4KB,         0x2 << 30
 .set    TCR_IPS_SHIFT,       32
 .set    TCR_EPD1,            0x1 << 23
 .set    TCR_SH_INNER,        0x3 << 12
 .set    TCR_RGN_OWB,         0x1 << 10
 .set    TCR_RGN_IWB,         0x1 << 8
 .set    tcrval, TCR_TG0_4KB | TCR_TG1_4KB | TCR_EPD1 | TCR_RGN_OWB
 .set    tcrval, tcrval | TCR_RGN_IWB | TCR_SH_INNER

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

 .set    ID_AA64MMFR1_VH_MASK,  0xf << 8
 .set    HCR_EL2_E2H,           0x1 << 34


ASM_FUNC(ArmPlatformPeiBootAction)
#ifdef CAVIUM_ERRATUM_27456
  /*
   * On Cavium ThunderX, using non-global mappings that are executable at EL1
   * results in I-cache corruption. So just avoid the early ID mapping there.
   *
   * MIDR implementor   0x43
   * MIDR part numbers  0xA1 0xA2 (but not 0xAF)
   */
  mrs    x0, midr_el1            // read the MIDR into X0
  ubfx   x1, x0, #24, #8         // grab implementor id
  ubfx   x0, x0, #7, #9          // grab part number bits [11:3]
  cmp    x1, #0x43               // compare implementor id
  ccmp   x0, #0xA0 >> 3, #0, eq  // compare part# bits [11:3]
  b.eq   0f
#endif
  mrs    x0, CurrentEL           // check current exception level
  tbz    x0, #3, .Learly_idmap   // use early ID map if at EL1
  mrs    x0, id_aa64mmfr1_el1    // otherwise, check for VHE support
  tst    x0, #ID_AA64MMFR1_VH_MASK
  b.eq   0f

  mrs    x0, hcr_el2             // Enable VHE support
  orr    x0, x0, #HCR_EL2_E2H
  msr    hcr_el2, x0
  isb

.Learly_idmap:
  mov_i  x0, mairval
  mov_i  x1, tcrval
  adrp   x2, idmap
  orr    x2, x2, #0xff << 48     // set non-zero ASID
  mov_i  x3, sctlrval

  mrs    x6, id_aa64mmfr0_el1    // get the supported PA range
  and    x6, x6, #0xf            // isolate PArange bits
  cmp    x6, #6                  // 0b0110 == 52 bits
  sub    x6, x6, #1              // subtract 1
  cinc   x6, x6, ne              // add back 1 unless PArange == 52 bits
  bfi    x1, x6, #32, #3         // copy updated PArange into TCR_EL1.IPS

  cmp    x6, #3                  // 0b0011 == 42 bits
  sub    x6, x6, #1              // subtract 1
  cinc   x6, x6, lt              // add back 1 unless VA range >= 42

  mov    x7, #32
  sub    x6, x7, x6, lsl #2      // T0SZ for PArange != 42
  mov    x7, #64 - 42            // T0SZ for PArange == 42
  csel   x6, x6, x7, ne
  orr    x1, x1, x6              // set T0SZ field in TCR

  cmp    x6, #64 - 40            // VA size < 40 bits?
  add    x4, x2, #0x1000         // advance to level 1 descriptor
  csel   x2, x4, x2, gt

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

0:b      ArmEnableVFP            // enable SIMD before entering C code

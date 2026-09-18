// SPDX-License-Identifier: BSD-2-Clause-Patent
// Copyright 2022 Google LLC
// Author: Ard Biesheuvel <ardb@google.com>

  .set      TT_TYPE_BLOCK, 0x1
  .set      TT_TYPE_PAGE,  0x3
  .set      TT_TYPE_TABLE, 0x3

  .set      TT_AF, 0x1 << 10
  .set      TT_NG, 0x1 << 11
  .set      TT_RO, 0x2 << 6
  .set      TT_XN, 0x3 << 53

  .set      TT_MT_DEV, 0x0 << 2                 // MAIR #0
  .set      TT_MT_MEM, (0x3 << 2) | (0x3 << 8)  // MAIR #3
  .set      TT_MT_MEM_LPA2, (0x3 << 2)          // MAIR #3

  .set      PAGE_XIP,  TT_TYPE_PAGE  | TT_AF | TT_RO | TT_NG
  .set      PAGE_DEV,  TT_TYPE_PAGE  | TT_AF | TT_XN | TT_NG
  .set      BLOCK_XIP, TT_TYPE_BLOCK | TT_AF | TT_RO | TT_NG
  .set      BLOCK_DEV, TT_TYPE_BLOCK | TT_AF | TT_XN | TT_NG
  .set      BLOCK_MEM, TT_TYPE_BLOCK | TT_AF | TT_XN | TT_NG

  .set      GRANULE_L2, 21                // 2MB
  .set      GRANULE_L3, 12                // 4KB

  /* Identity-map range of addresses [from, to) */
  .macro    id_pte  from, to, flags, granule=GRANULE_L2
  .set      idx, (\from) >> (\granule)
  .rept     ((\to) - (\from)) >> (\granule)
  .quad     (\flags) | (idx << (\granule))
  .set      idx, idx + 1
  .endr
  .endm

  // Keep the root/L1/L2 hierarchy read-only so it can be used directly from
  // NOR flash before RAM, the MMU and caches are fully enabled.
  .section  ".rodata.idmap", "a"
  .align    12

   .macro   make_idmap, name, uart_pte_name, root_levels, mt
  .globl    \name
\name:
  .rept     \root_levels
  .quad     1f + TT_TYPE_TABLE
  .fill     511, 8, 0x0
1:
  .endr

  .quad     20f + TT_TYPE_TABLE                 // 1 GB of flash and device mappings
  .quad     21f + TT_TYPE_TABLE                 // up to 1 GB of DRAM
  .fill     510, 8, 0x0                         // 510 GB of remaining VA space

20:         /* level 2 */
  .quad     300f + TT_TYPE_TABLE                // up to 2 MB of flash
  .quad     BLOCK_XIP | \mt | (0x1  << 21)      // another 2 MB of flash
  .fill     62, 8, 0x0                          // 124 MB of unused flash
                                                  // 896 MB of RW- device mappings
  id_pte    0x08000000, 0x09000000, BLOCK_DEV | TT_MT_DEV
  /*
   * The 2 MB device window contains the early PL011 UART. Use a
   * writable L3 table here so Realm boot can patch only the UART
   * page while the rest of the ID-map hierarchy remains read-only.
   */
  .quad     \uart_pte_name + TT_TYPE_TABLE
  id_pte    0x09200000, 0x40000000, BLOCK_DEV | TT_MT_DEV

21:         /* level 2 */
                                                  // 128 MB of RW- memory mappings
  id_pte    0x40000000, 0x48000000, BLOCK_MEM | \mt
  .fill     448, 8, 0x0

300:        /* level 3 */
  .quad     0x0                                 // omit first 4k page
                                                  // 2044 KiB of R-X flash mappings
  id_pte    0x1000, 0x200000, PAGE_XIP | \mt, GRANULE_L3

  .endm

  make_idmap  idmap,      uart_pte_name=uart_pte,      root_levels=1, mt=TT_MT_MEM
  make_idmap  idmap_lpa2, uart_pte_name=uart_pte,      root_levels=2, mt=TT_MT_MEM_LPA2

// CLANGPDB does not support %progbits and
// expects comdat type such as 'discard' or
// 'largest' after protection bits.
// Therefore, for CLANGPDB configure the
// section as data + writable.
#ifdef __ELF__
  .section  ".data.idmap_uart", "aw", %progbits
#else
  .section  ".data.idmap_uart", "dw"
#endif
                                                  // Only the UART L3 table is
                                                  // writable for Realm remapping.
  .align    12

  .macro    make_uart_pte, name
  .globl    \name
\name:
  /*
   * This L3 table maps the whole 2 MB device window using 4 KB descriptors.
   * Entry 0 maps the fixed early PL011 UART register page at 0x09000000.
   * In a Realm, ArmPlatformHelper.S patches that entry to set the IPA MSB
   * and reach the emulated UART through the unprotected IPA range.
   */
  id_pte    0x09000000, 0x09200000, PAGE_DEV | TT_MT_DEV, GRANULE_L3
  .endm

  make_uart_pte uart_pte

AARCH64_BTI_NOTE()

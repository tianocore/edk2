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

  .set      PAGE_XIP,  TT_TYPE_PAGE  | TT_MT_MEM | TT_AF | TT_RO | TT_NG
  .set      PAGE_DEV,  TT_TYPE_PAGE  | TT_MT_DEV | TT_AF | TT_XN | TT_NG
  .set      BLOCK_XIP, TT_TYPE_BLOCK | TT_MT_MEM | TT_AF | TT_RO | TT_NG
  .set      BLOCK_DEV, TT_TYPE_BLOCK | TT_MT_DEV | TT_AF | TT_XN | TT_NG
  .set      BLOCK_MEM, TT_TYPE_BLOCK | TT_MT_MEM | TT_AF | TT_XN | TT_NG

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

  .globl    idmap
  .globl    uart_pte
  .section  ".data.idmap", "aw", %progbits
  .align    12

idmap:      /* level 0 */
  .quad     1f + TT_TYPE_TABLE
  .fill     511, 8, 0x0

1:          /* level 1 */
  .quad     20f + TT_TYPE_TABLE           // 1 GB of flash and device mappings
  .quad     21f + TT_TYPE_TABLE           // up to 1 GB of DRAM
  .fill     510, 8, 0x0                   // 510 GB of remaining VA space

20:         /* level 2 */
  .quad     300f + TT_TYPE_TABLE          // up to 2 MB of flash
  .quad     BLOCK_XIP | (0x1  << 21)      // another 2 MB of flash
  .fill     62, 8, 0x0                    // 124 MB of unused flash
            // 896 MB of RW- device mappings
  id_pte    0x08000000, 0x09000000, BLOCK_DEV
  .quad     301f + TT_TYPE_TABLE
  id_pte    0x09200000, 0x40000000, BLOCK_DEV

21:         /* level 2 */
            // 128 MB of RW- memory mappings
  id_pte    0x40000000, 0x48000000, BLOCK_MEM
  .fill     448, 8, 0x0

300:        /* level 3 */
  .quad     0x0                           // omit first 4k page
            // 2044 KiB of R-X flash mappings
  id_pte    0x1000, 0x200000, PAGE_XIP, GRANULE_L3

301:        /* level 3 */
  /*
   * In a Realm, we may need to update the UART mapping 0x09000000 - 0x0900ffff
   * to access the emulated device via unprotected IPA (MSB set).
   */
uart_pte:
  id_pte    0x09000000, 0x09200000, PAGE_DEV, GRANULE_L3

AARCH64_BTI_NOTE()

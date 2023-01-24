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
  .set      BLOCK_XIP, TT_TYPE_BLOCK | TT_MT_MEM | TT_AF | TT_RO | TT_NG
  .set      BLOCK_DEV, TT_TYPE_BLOCK | TT_MT_DEV | TT_AF | TT_XN | TT_NG
  .set      BLOCK_MEM, TT_TYPE_BLOCK | TT_MT_MEM | TT_AF | TT_XN | TT_NG

  .globl    idmap
  .section  ".rodata.idmap", "a", %progbits
  .align    12

idmap:      /* level 0 */
  .quad     1f + TT_TYPE_TABLE
  .fill     511, 8, 0x0

1:          /* level 1 */
  .quad     20f + TT_TYPE_TABLE           // 1 GB of flash and device mappings
  .quad     21f + TT_TYPE_TABLE           // up to 1 GB of DRAM
  .fill     510, 8, 0x0                   // 510 GB of remaining VA space

20:         /* level 2 */
  .quad     3f + TT_TYPE_TABLE            // up to 2 MB of flash
  .quad     BLOCK_XIP | (0x1  << 21)      // another 2 MB of flash
  .fill     62, 8, 0x0                    // 124 MB of unused flash
  .set      idx, 64
  .rept     448
  .quad     BLOCK_DEV | (idx << 21)       // 896 MB of RW- device mappings
  .set      idx, idx + 1
  .endr

21:         /* level 2 */
  .set      idx, 0x40000000 >> 21
  .rept     64
  .quad     BLOCK_MEM | (idx << 21)       // 128 MB of RW- memory mappings
  .set      idx, idx + 1
  .endr
  .fill     448, 8, 0x0

3:          /* level 3 */
  .quad     0x0                           // omit first 4k page
  .set      idx, 1
  .rept     511
  .quad     PAGE_XIP | (idx << 12)        // 2044 KiB of R-X flash mappings
  .set      idx, idx + 1
  .endr

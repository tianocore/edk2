/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef L2CACHELIB_H_
#define L2CACHELIB_H_

#define L2X0_CACHEID            0x000
#define L2X0_CTRL               0x100
#define L2X0_AUXCTRL            0x104
#define L230_TAG_LATENCY        0x108
#define L230_DATA_LATENCY       0x10C
#define L2X0_INTCLEAR           0x220
#define L2X0_CACHE_SYNC         0x730
#define L2X0_INVWAY             0x77C
#define L2X0_CLEAN_WAY          0x7BC
#define L2X0_PFCTRL             0xF60
#define L2X0_PWRCTRL            0xF80

#define L2X0_CACHEID_IMPLEMENTER_ARM        0x41
#define L2X0_CACHEID_PARTNUM_PL310          0x03

#define L2X0_CTRL_ENABLED                   0x1
#define L2X0_CTRL_DISABLED                  0x0

#define L2X0_AUXCTRL_EXCLUSIVE              (1 << 12)
#define L2X0_AUXCTRL_ASSOCIATIVITY          (1 << 16)
#define L2X0_AUXCTRL_WAYSIZE_MASK           (3 << 17)
#define L2X0_AUXCTRL_WAYSIZE_16KB           (1 << 17)
#define L2X0_AUXCTRL_WAYSIZE_32KB           (2 << 17)
#define L2X0_AUXCTRL_WAYSIZE_64KB           (3 << 17)
#define L2X0_AUXCTRL_WAYSIZE_128KB          (4 << 17)
#define L2X0_AUXCTRL_WAYSIZE_256KB          (5 << 17)
#define L2X0_AUXCTRL_WAYSIZE_512KB          (6 << 17)
#define L2X0_AUXCTRL_EM                     (1 << 20)
#define L2X0_AUXCTRL_SHARED_OVERRIDE        (1 << 22)
#define L2x0_AUXCTRL_AW_AWCACHE             (0 << 23)
#define L2x0_AUXCTRL_AW_NOALLOC             (1 << 23)
#define L2x0_AUXCTRL_AW_OVERRIDE            (2 << 23)
#define L2X0_AUXCTRL_SBO                    (1 << 25)
#define L2X0_AUXCTRL_NSAC                   (1 << 27)
#define L2x0_AUXCTRL_DPREFETCH              (1 << 28)
#define L2x0_AUXCTRL_IPREFETCH              (1 << 29)
#define L2x0_AUXCTRL_EARLY_BRESP            (1 << 30)

#define L2x0_LATENCY_1_CYCLE                 0
#define L2x0_LATENCY_2_CYCLES                1
#define L2x0_LATENCY_3_CYCLES                2
#define L2x0_LATENCY_4_CYCLES                3
#define L2x0_LATENCY_5_CYCLES                4
#define L2x0_LATENCY_6_CYCLES                5
#define L2x0_LATENCY_7_CYCLES                6
#define L2x0_LATENCY_8_CYCLES                7

#define PL310_LATENCIES(Write,Read,Setup)      (((Write) << 8) | ((Read) << 4) | (Setup))
#define PL310_TAG_LATENCIES(Write,Read,Setup)  PL310_LATENCIES(Write,Read,Setup)
#define PL310_DATA_LATENCIES(Write,Read,Setup) PL310_LATENCIES(Write,Read,Setup)

VOID
L2x0CacheInit (
  IN  UINTN   L2x0Base,
  IN  UINT32  L2x0TagLatencies,
  IN  UINT32  L2x0DataLatencies,
  IN  UINT32  L2x0AuxValue,
  IN  UINT32  L2x0AuxMask,
  IN  BOOLEAN CacheEnabled
  );

#endif /* L2CACHELIB_H_ */

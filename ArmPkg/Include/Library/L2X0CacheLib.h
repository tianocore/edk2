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

#define L2_LATENCY                          7

#define L2_TAG_ACCESS_LATENCY               L2_LATENCY
#define L2_TAG_SETUP_LATENCY                L2_LATENCY
#define L2_DATA_ACCESS_LATENCY              L2_LATENCY
#define L2_DATA_SETUP_LATENCY               L2_LATENCY


#define L2X0_CACHEID            0x000
#define L2X0_CTRL               0x100
#define L2X0_AUXCTRL            0x104
#define L230_TAG_LATENCY        0x108
#define L230_DATA_LATENCY       0x10C
#define L2X0_INTCLEAR           0x220
#define L2X0_CACHE_SYNC			0x730
#define L2X0_INVWAY             0x77C
#define L2X0_CLEAN_WAY          0x7BC
#define L2X0_PFCTRL             0xF60
#define L2X0_PWRCTRL            0xF80

#define L2X0_CACHEID_IMPLEMENTER_ARM        0x41
#define L2X0_CACHEID_PARTNUM_PL310          0x03

#define L2X0_CTRL_ENABLED                   0x1
#define L2X0_CTRL_DISABLED                  0x0

#define L2X0_AUXCTRL_EXCLUSIVE              (1<<12)
#define L2X0_AUXCTRL_WAYSIZE_16KB           (0x001 << 17)
#define L2X0_AUXCTRL_WAYSIZE_32KB           (0x010 << 17)
#define L2X0_AUXCTRL_WAYSIZE_64KB           (0x011 << 17)
#define L2X0_AUXCTRL_WAYSIZE_128KB          (0x100 << 17)
#define L2X0_AUXCTRL_WAYSIZE_256KB          (0x101 << 17)
#define L2X0_AUXCTRL_WAYSIZE_512KB          (0x110 << 17)
#define L2X0_AUXCTRL_EM                     (1 << 20)
#define L2x0_AUXCTRL_AW_AWCACHE             (0x00 << 23)
#define L2x0_AUXCTRL_AW_NOALLOC             (0x01 << 23)
#define L2x0_AUXCTRL_AW_OVERRIDE            (0x10 << 23)
#define L2X0_AUXCTRL_SBO                    (1 << 25)
#define L2X0_AUXCTRL_NSAC                   (1 << 27)
#define L2x0_AUXCTRL_DPREFETCH              (1 << 28)
#define L2x0_AUXCTRL_IPREFETCH              (1 << 29)

VOID L2x0CacheInit(UINTN L2x0Base, BOOLEAN CacheEnabled);

#endif /* L2CACHELIB_H_ */

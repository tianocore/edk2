/** @file

  Copyright (c) 2008-2009 Apple Inc. All rights reserved.<BR>

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __OMAP3530SDIO_H__
#define __OMAP3530SDIO_H__

//MMC/SD/SDIO1 register definitions.
#define MMCHS1BASE        0x4809C000
#define MMC_REFERENCE_CLK (96000000)

#define MMCHS_SYSCONFIG   (MMCHS1BASE + 0x10)
#define SOFTRESET         (0x1UL << 1)
#define ENAWAKEUP         (0x1UL << 2)

#define MMCHS_SYSSTATUS   (MMCHS1BASE + 0x14)
#define RESETDONE_MASK    (0x1UL << 0)
#define RESETDONE         (0x1UL << 0)

#define MMCHS_CSRE        (MMCHS1BASE + 0x24)
#define MMCHS_SYSTEST     (MMCHS1BASE + 0x28)

#define MMCHS_CON         (MMCHS1BASE + 0x2C)
#define OD                (0x1UL << 0)
#define NOINIT            (0x0UL << 1)
#define INIT              (0x1UL << 1)
#define HR                (0x1UL << 2) 
#define STR               (0x1UL << 3)
#define MODE              (0x1UL << 4)
#define DW8_1_4_BIT       (0x0UL << 5)
#define DW8_8_BIT         (0x1UL << 5)
#define MIT               (0x1UL << 6)
#define CDP               (0x1UL << 7)
#define WPP               (0x1UL << 8) 
#define CTPL              (0x1UL << 11) 
#define CEATA_OFF         (0x0UL << 12)
#define CEATA_ON          (0x1UL << 12)

#define MMCHS_PWCNT       (MMCHS1BASE + 0x30)

#define MMCHS_BLK         (MMCHS1BASE + 0x104)
#define BLEN_512BYTES     (0x200UL << 0)

#define MMCHS_ARG         (MMCHS1BASE + 0x108)

#define MMCHS_CMD         (MMCHS1BASE + 0x10C)
#define DE_ENABLE         (0x1UL << 0)
#define BCE_ENABLE        (0x1UL << 1)
#define ACEN_ENABLE       (0x1UL << 2)
#define DDIR_READ         (0x1UL << 4)
#define DDIR_WRITE        (0x0UL << 4)
#define MSBS_SGLEBLK      (0x0UL << 5)
#define MSBS_MULTBLK      (0x1UL << 5)
#define RSP_TYPE_MASK     (0x3UL << 16)
#define RSP_TYPE_136BITS  (0x1UL << 16)
#define RSP_TYPE_48BITS   (0x2UL << 16)
#define CCCE_ENABLE       (0x1UL << 19)
#define CICE_ENABLE       (0x1UL << 20)
#define DP_ENABLE         (0x1UL << 21) 
#define INDX(CMD_INDX)    ((CMD_INDX & 0x3F) << 24)

#define MMCHS_RSP10       (MMCHS1BASE + 0x110)
#define MMCHS_RSP32       (MMCHS1BASE + 0x114)
#define MMCHS_RSP54       (MMCHS1BASE + 0x118)
#define MMCHS_RSP76       (MMCHS1BASE + 0x11C)
#define MMCHS_DATA        (MMCHS1BASE + 0x120)

#define MMCHS_PSTATE      (MMCHS1BASE + 0x124)
#define CMDI_MASK         (0x1UL << 0)
#define CMDI_ALLOWED      (0x0UL << 0)
#define CMDI_NOT_ALLOWED  (0x1UL << 0)
#define DATI_MASK         (0x1UL << 1)
#define DATI_ALLOWED      (0x0UL << 1)
#define DATI_NOT_ALLOWED  (0x1UL << 1)

#define MMCHS_HCTL        (MMCHS1BASE + 0x128)
#define DTW_1_BIT         (0x0UL << 1)
#define DTW_4_BIT         (0x1UL << 1)
#define SDBP_MASK         (0x1UL << 8)
#define SDBP_OFF          (0x0UL << 8)
#define SDBP_ON           (0x1UL << 8)
#define SDVS_1_8_V        (0x5UL << 9)
#define SDVS_3_0_V        (0x6UL << 9)
#define IWE               (0x1UL << 24)

#define MMCHS_SYSCTL      (MMCHS1BASE + 0x12C)
#define ICE               (0x1UL << 0)
#define ICS_MASK          (0x1UL << 1)
#define ICS               (0x1UL << 1)
#define CEN               (0x1UL << 2)
#define CLKD_MASK         (0x3FFUL << 6)
#define CLKD_80KHZ        (0x258UL) //(96*1000/80)/2
#define CLKD_400KHZ       (0xF0UL)
#define DTO_MASK          (0xFUL << 16)
#define DTO_VAL           (0xEUL << 16)
#define SRA               (0x1UL << 24)
#define SRC_MASK          (0x1UL << 25)
#define SRC               (0x1UL << 25)
#define SRD               (0x1UL << 26)

#define MMCHS_STAT        (MMCHS1BASE + 0x130)
#define CC                (0x1UL << 0)
#define TC                (0x1UL << 1)
#define BWR               (0x1UL << 4)
#define BRR               (0x1UL << 5)
#define ERRI              (0x1UL << 15)
#define CTO               (0x1UL << 16)
#define DTO               (0x1UL << 20)
#define DCRC              (0x1UL << 21)
#define DEB               (0x1UL << 22)

#define MMCHS_IE          (MMCHS1BASE + 0x134)
#define CC_EN             (0x1UL << 0)
#define TC_EN             (0x1UL << 1)
#define BWR_EN            (0x1UL << 4)
#define BRR_EN            (0x1UL << 5)
#define CTO_EN            (0x1UL << 16)
#define CCRC_EN           (0x1UL << 17)
#define CEB_EN            (0x1UL << 18)
#define CIE_EN            (0x1UL << 19)
#define DTO_EN            (0x1UL << 20)
#define DCRC_EN           (0x1UL << 21)
#define DEB_EN            (0x1UL << 22)
#define CERR_EN           (0x1UL << 28)
#define BADA_EN           (0x1UL << 29)

#define MMCHS_ISE         (MMCHS1BASE + 0x138)
#define CC_SIGEN          (0x1UL << 0)
#define TC_SIGEN          (0x1UL << 1)
#define BWR_SIGEN         (0x1UL << 4)
#define BRR_SIGEN         (0x1UL << 5)
#define CTO_SIGEN         (0x1UL << 16)
#define CCRC_SIGEN        (0x1UL << 17)
#define CEB_SIGEN         (0x1UL << 18)
#define CIE_SIGEN         (0x1UL << 19)
#define DTO_SIGEN         (0x1UL << 20)
#define DCRC_SIGEN        (0x1UL << 21)
#define DEB_SIGEN         (0x1UL << 22)
#define CERR_SIGEN        (0x1UL << 28)
#define BADA_SIGEN        (0x1UL << 29)

#define MMCHS_AC12        (MMCHS1BASE + 0x13C)

#define MMCHS_CAPA        (MMCHS1BASE + 0x140)
#define VS30              (0x1UL << 25)
#define VS18              (0x1UL << 26)

#define MMCHS_CUR_CAPA    (MMCHS1BASE + 0x148)
#define MMCHS_REV         (MMCHS1BASE + 0x1FC)

#define CMD0              INDX(0)
#define CMD0_INT_EN       (CC_EN | CEB_EN)

#define CMD1              (INDX(1) | RSP_TYPE_48BITS)
#define CMD1_INT_EN       (CC_EN | CEB_EN | CTO_EN)

#define CMD2              (INDX(2) | CCCE_ENABLE | RSP_TYPE_136BITS)
#define CMD2_INT_EN       (CERR_EN | CIE_EN | CCRC_EN | CC_EN | CEB_EN | CTO_EN)

#define CMD3              (INDX(3) | CICE_ENABLE | CCCE_ENABLE | RSP_TYPE_48BITS)
#define CMD3_INT_EN       (CERR_EN | CIE_EN | CCRC_EN | CC_EN | CEB_EN | CTO_EN)

#define CMD5              (INDX(5) | RSP_TYPE_48BITS)
#define CMD5_INT_EN       (CC_EN | CEB_EN | CTO_EN)

#define CMD7              (INDX(7) | CICE_ENABLE | CCCE_ENABLE | RSP_TYPE_48BITS)
#define CMD7_INT_EN       (CERR_EN | CIE_EN | CCRC_EN | CC_EN | CEB_EN | CTO_EN)

#define CMD8              (INDX(8) | CICE_ENABLE | CCCE_ENABLE | RSP_TYPE_48BITS)
#define CMD8_INT_EN       (CERR_EN | CIE_EN | CCRC_EN | CC_EN | CEB_EN | CTO_EN)
//Reserved(0)[12:31], Supply voltage(1)[11:8], check pattern(0xCE)[7:0] = 0x1CE
#define CMD8_ARG          (0x0UL << 12 | 0x1UL << 8 | 0xCEUL << 0)

#define CMD9              (INDX(9) | CCCE_ENABLE | RSP_TYPE_136BITS)
#define CMD9_INT_EN       (CCRC_EN | CC_EN | CEB_EN | CTO_EN)

#define CMD16             (INDX(16) | CICE_ENABLE | CCCE_ENABLE | RSP_TYPE_48BITS)
#define CMD16_INT_EN      (CERR_EN | CIE_EN | CCRC_EN | CC_EN | CEB_EN | CTO_EN)

#define CMD17             (INDX(17) | DP_ENABLE | CICE_ENABLE | CCCE_ENABLE | RSP_TYPE_48BITS | DDIR_READ)
#define CMD17_INT_EN      (CERR_EN | CIE_EN | CCRC_EN | CC_EN | TC_EN | BRR_EN | CTO_EN | DTO_EN | DCRC_EN | DEB_EN | CEB_EN)

#define CMD18             (INDX(18) | DP_ENABLE | CICE_ENABLE | CCCE_ENABLE | RSP_TYPE_48BITS | MSBS_MULTBLK | DDIR_READ | BCE_ENABLE | DE_ENABLE)
#define CMD18_INT_EN      (CERR_EN | CIE_EN | CCRC_EN | CC_EN | TC_EN | BRR_EN | CTO_EN | DTO_EN | DCRC_EN | DEB_EN | CEB_EN)

#define CMD23             (INDX(23) | CICE_ENABLE | CCCE_ENABLE | RSP_TYPE_48BITS)
#define CMD23_INT_EN      (CERR_EN | CIE_EN | CCRC_EN | CC_EN | CEB_EN | CTO_EN)

#define CMD24             (INDX(24) | DP_ENABLE | CICE_ENABLE | CCCE_ENABLE | RSP_TYPE_48BITS | DDIR_WRITE)
#define CMD24_INT_EN      (CERR_EN | CIE_EN | CCRC_EN | CC_EN | TC_EN | BWR_EN | CTO_EN | DTO_EN | DCRC_EN | DEB_EN | CEB_EN)

#define CMD55             (INDX(55) | CICE_ENABLE | CCCE_ENABLE | RSP_TYPE_48BITS)
#define CMD55_INT_EN      (CERR_EN | CIE_EN | CCRC_EN | CC_EN | CEB_EN | CTO_EN)

#define ACMD41            (INDX(41) | RSP_TYPE_48BITS)
#define ACMD41_INT_EN     (CERR_EN | CIE_EN | CCRC_EN | CC_EN | CEB_EN | CTO_EN)

#endif //__OMAP3530SDIO_H__

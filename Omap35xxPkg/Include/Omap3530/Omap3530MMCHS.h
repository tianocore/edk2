/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __OMAP3530SDIO_H__
#define __OMAP3530SDIO_H__

//MMC/SD/SDIO1 register definitions.
#define MMCHS1BASE        0x4809C000
#define MMC_REFERENCE_CLK (96000000)

#define MMCHS_SYSCONFIG   (MMCHS1BASE + 0x10)
#define SOFTRESET         BIT1
#define ENAWAKEUP         BIT2

#define MMCHS_SYSSTATUS   (MMCHS1BASE + 0x14)
#define RESETDONE_MASK    BIT0
#define RESETDONE         BIT0

#define MMCHS_CSRE        (MMCHS1BASE + 0x24)
#define MMCHS_SYSTEST     (MMCHS1BASE + 0x28)

#define MMCHS_CON         (MMCHS1BASE + 0x2C)
#define OD                BIT0
#define NOINIT            (0x0UL << 1)
#define INIT              BIT1
#define HR                BIT2
#define STR               BIT3
#define MODE              BIT4
#define DW8_1_4_BIT       (0x0UL << 5)
#define DW8_8_BIT         BIT5
#define MIT               BIT6
#define CDP               BIT7
#define WPP               BIT8
#define CTPL              BIT11
#define CEATA_OFF         (0x0UL << 12)
#define CEATA_ON          BIT12

#define MMCHS_PWCNT       (MMCHS1BASE + 0x30)

#define MMCHS_BLK         (MMCHS1BASE + 0x104)
#define BLEN_512BYTES     (0x200UL << 0)

#define MMCHS_ARG         (MMCHS1BASE + 0x108)

#define MMCHS_CMD         (MMCHS1BASE + 0x10C)
#define DE_ENABLE         BIT0
#define BCE_ENABLE        BIT1
#define ACEN_ENABLE       BIT2
#define DDIR_READ         BIT4
#define DDIR_WRITE        (0x0UL << 4)
#define MSBS_SGLEBLK      (0x0UL << 5)
#define MSBS_MULTBLK      BIT5
#define RSP_TYPE_MASK     (0x3UL << 16)
#define RSP_TYPE_136BITS  BIT16
#define RSP_TYPE_48BITS   (0x2UL << 16)
#define CCCE_ENABLE       BIT19
#define CICE_ENABLE       BIT20
#define DP_ENABLE         BIT21
#define INDX(CMD_INDX)    ((CMD_INDX & 0x3F) << 24)

#define MMCHS_RSP10       (MMCHS1BASE + 0x110)
#define MMCHS_RSP32       (MMCHS1BASE + 0x114)
#define MMCHS_RSP54       (MMCHS1BASE + 0x118)
#define MMCHS_RSP76       (MMCHS1BASE + 0x11C)
#define MMCHS_DATA        (MMCHS1BASE + 0x120)

#define MMCHS_PSTATE      (MMCHS1BASE + 0x124)
#define CMDI_MASK         BIT0
#define CMDI_ALLOWED      (0x0UL << 0)
#define CMDI_NOT_ALLOWED  BIT0
#define DATI_MASK         BIT1
#define DATI_ALLOWED      (0x0UL << 1)
#define DATI_NOT_ALLOWED  BIT1

#define MMCHS_HCTL        (MMCHS1BASE + 0x128)
#define DTW_1_BIT         (0x0UL << 1)
#define DTW_4_BIT         BIT1
#define SDBP_MASK         BIT8
#define SDBP_OFF          (0x0UL << 8)
#define SDBP_ON           BIT8
#define SDVS_1_8_V        (0x5UL << 9)
#define SDVS_3_0_V        (0x6UL << 9)
#define IWE               BIT24

#define MMCHS_SYSCTL      (MMCHS1BASE + 0x12C)
#define ICE               BIT0
#define ICS_MASK          BIT1
#define ICS               BIT1
#define CEN               BIT2
#define CLKD_MASK         (0x3FFUL << 6)
#define CLKD_80KHZ        (0x258UL) //(96*1000/80)/2
#define CLKD_400KHZ       (0xF0UL)
#define DTO_MASK          (0xFUL << 16)
#define DTO_VAL           (0xEUL << 16)
#define SRA               BIT24
#define SRC_MASK          BIT25
#define SRC               BIT25
#define SRD               BIT26

#define MMCHS_STAT        (MMCHS1BASE + 0x130)
#define CC                BIT0
#define TC                BIT1
#define BWR               BIT4
#define BRR               BIT5
#define ERRI              BIT15
#define CTO               BIT16
#define DTO               BIT20
#define DCRC              BIT21
#define DEB               BIT22

#define MMCHS_IE          (MMCHS1BASE + 0x134)
#define CC_EN             BIT0
#define TC_EN             BIT1
#define BWR_EN            BIT4
#define BRR_EN            BIT5
#define CTO_EN            BIT16
#define CCRC_EN           BIT17
#define CEB_EN            BIT18
#define CIE_EN            BIT19
#define DTO_EN            BIT20
#define DCRC_EN           BIT21
#define DEB_EN            BIT22
#define CERR_EN           BIT28
#define BADA_EN           BIT29

#define MMCHS_ISE         (MMCHS1BASE + 0x138)
#define CC_SIGEN          BIT0
#define TC_SIGEN          BIT1
#define BWR_SIGEN         BIT4
#define BRR_SIGEN         BIT5
#define CTO_SIGEN         BIT16
#define CCRC_SIGEN        BIT17
#define CEB_SIGEN         BIT18
#define CIE_SIGEN         BIT19
#define DTO_SIGEN         BIT20
#define DCRC_SIGEN        BIT21
#define DEB_SIGEN         BIT22
#define CERR_SIGEN        BIT28
#define BADA_SIGEN        BIT29

#define MMCHS_AC12        (MMCHS1BASE + 0x13C)

#define MMCHS_CAPA        (MMCHS1BASE + 0x140)
#define VS30              BIT25
#define VS18              BIT26

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
#define CMD8_ARG          (0x0UL << 12 | BIT8 | 0xCEUL << 0)

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

#define CMD25             (INDX(25) | DP_ENABLE | CICE_ENABLE | CCCE_ENABLE | RSP_TYPE_48BITS | MSBS_MULTBLK | DDIR_READ | BCE_ENABLE | DE_ENABLE)
#define CMD25_INT_EN      (CERR_EN | CIE_EN | CCRC_EN | CC_EN | TC_EN | BRR_EN | CTO_EN | DTO_EN | DCRC_EN | DEB_EN | CEB_EN)

#define CMD55             (INDX(55) | CICE_ENABLE | CCCE_ENABLE | RSP_TYPE_48BITS)
#define CMD55_INT_EN      (CERR_EN | CIE_EN | CCRC_EN | CC_EN | CEB_EN | CTO_EN)

#define ACMD41            (INDX(41) | RSP_TYPE_48BITS)
#define ACMD41_INT_EN     (CERR_EN | CIE_EN | CCRC_EN | CC_EN | CEB_EN | CTO_EN)

#define ACMD6             (INDX(6) | RSP_TYPE_48BITS)
#define ACMD6_INT_EN      (CERR_EN | CIE_EN | CCRC_EN | CC_EN | CEB_EN | CTO_EN)

#endif //__OMAP3530SDIO_H__

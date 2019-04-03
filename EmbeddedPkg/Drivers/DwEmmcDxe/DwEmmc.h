/** @file
*
*  Copyright (c) 2014-2017, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/


#ifndef __DWEMMC_H__
#define __DWEMMC_H__

#include <Protocol/EmbeddedGpio.h>

// DW MMC Registers
#define DWEMMC_CTRL             ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x000)
#define DWEMMC_PWREN            ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x004)
#define DWEMMC_CLKDIV           ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x008)
#define DWEMMC_CLKSRC           ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x00c)
#define DWEMMC_CLKENA           ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x010)
#define DWEMMC_TMOUT            ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x014)
#define DWEMMC_CTYPE            ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x018)
#define DWEMMC_BLKSIZ           ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x01c)
#define DWEMMC_BYTCNT           ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x020)
#define DWEMMC_INTMASK          ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x024)
#define DWEMMC_CMDARG           ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x028)
#define DWEMMC_CMD              ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x02c)
#define DWEMMC_RESP0            ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x030)
#define DWEMMC_RESP1            ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x034)
#define DWEMMC_RESP2            ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x038)
#define DWEMMC_RESP3            ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x03c)
#define DWEMMC_RINTSTS          ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x044)
#define DWEMMC_STATUS           ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x048)
#define DWEMMC_FIFOTH           ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x04c)
#define DWEMMC_TCBCNT           ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x05c)
#define DWEMMC_TBBCNT           ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x060)
#define DWEMMC_DEBNCE           ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x064)
#define DWEMMC_HCON             ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x070)
#define DWEMMC_UHSREG           ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x074)
#define DWEMMC_BMOD             ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x080)
#define DWEMMC_DBADDR           ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x088)
#define DWEMMC_IDSTS            ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x08c)
#define DWEMMC_IDINTEN          ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x090)
#define DWEMMC_DSCADDR          ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x094)
#define DWEMMC_BUFADDR          ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0x098)
#define DWEMMC_CARDTHRCTL       ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0X100)
#define DWEMMC_DATA             ((UINT32)PcdGet32 (PcdDwEmmcDxeBaseAddress) + 0X200)

#define CMD_UPDATE_CLK                          0x80202000
#define CMD_START_BIT                           (1 << 31)

#define MMC_8BIT_MODE                           (1 << 16)

#define BIT_CMD_RESPONSE_EXPECT                 (1 << 6)
#define BIT_CMD_LONG_RESPONSE                   (1 << 7)
#define BIT_CMD_CHECK_RESPONSE_CRC              (1 << 8)
#define BIT_CMD_DATA_EXPECTED                   (1 << 9)
#define BIT_CMD_READ                            (0 << 10)
#define BIT_CMD_WRITE                           (1 << 10)
#define BIT_CMD_BLOCK_TRANSFER                  (0 << 11)
#define BIT_CMD_STREAM_TRANSFER                 (1 << 11)
#define BIT_CMD_SEND_AUTO_STOP                  (1 << 12)
#define BIT_CMD_WAIT_PRVDATA_COMPLETE           (1 << 13)
#define BIT_CMD_STOP_ABORT_CMD                  (1 << 14)
#define BIT_CMD_SEND_INIT                       (1 << 15)
#define BIT_CMD_UPDATE_CLOCK_ONLY               (1 << 21)
#define BIT_CMD_READ_CEATA_DEVICE               (1 << 22)
#define BIT_CMD_CCS_EXPECTED                    (1 << 23)
#define BIT_CMD_ENABLE_BOOT                     (1 << 24)
#define BIT_CMD_EXPECT_BOOT_ACK                 (1 << 25)
#define BIT_CMD_DISABLE_BOOT                    (1 << 26)
#define BIT_CMD_MANDATORY_BOOT                  (0 << 27)
#define BIT_CMD_ALTERNATE_BOOT                  (1 << 27)
#define BIT_CMD_VOLT_SWITCH                     (1 << 28)
#define BIT_CMD_USE_HOLD_REG                    (1 << 29)
#define BIT_CMD_START                           (1 << 31)

#define DWEMMC_INT_EBE                          (1 << 15)       /* End-bit Err */
#define DWEMMC_INT_SBE                          (1 << 13)       /* Start-bit  Err */
#define DWEMMC_INT_HLE                          (1 << 12)       /* Hardware-lock Err */
#define DWEMMC_INT_FRUN                         (1 << 11)       /* FIFO UN/OV RUN */
#define DWEMMC_INT_DRT                          (1 << 9)        /* Data timeout */
#define DWEMMC_INT_RTO                          (1 << 8)        /* Response timeout */
#define DWEMMC_INT_DCRC                         (1 << 7)        /* Data CRC err */
#define DWEMMC_INT_RCRC                         (1 << 6)        /* Response CRC err */
#define DWEMMC_INT_RXDR                         (1 << 5)
#define DWEMMC_INT_TXDR                         (1 << 4)
#define DWEMMC_INT_DTO                          (1 << 3)        /* Data trans over */
#define DWEMMC_INT_CMD_DONE                     (1 << 2)
#define DWEMMC_INT_RE                           (1 << 1)

#define DWEMMC_IDMAC_DES0_DIC                   (1 << 1)
#define DWEMMC_IDMAC_DES0_LD                    (1 << 2)
#define DWEMMC_IDMAC_DES0_FS                    (1 << 3)
#define DWEMMC_IDMAC_DES0_CH                    (1 << 4)
#define DWEMMC_IDMAC_DES0_ER                    (1 << 5)
#define DWEMMC_IDMAC_DES0_CES                   (1 << 30)
#define DWEMMC_IDMAC_DES0_OWN                   (1 << 31)
#define DWEMMC_IDMAC_DES1_BS1(x)                ((x) & 0x1fff)
#define DWEMMC_IDMAC_DES2_BS2(x)                (((x) & 0x1fff) << 13)
#define DWEMMC_IDMAC_SWRESET                    (1 << 0)
#define DWEMMC_IDMAC_FB                         (1 << 1)
#define DWEMMC_IDMAC_ENABLE                     (1 << 7)

#define EMMC_FIX_RCA                            6

/* bits in MMC0_CTRL */
#define DWEMMC_CTRL_RESET                       (1 << 0)
#define DWEMMC_CTRL_FIFO_RESET                  (1 << 1)
#define DWEMMC_CTRL_DMA_RESET                   (1 << 2)
#define DWEMMC_CTRL_INT_EN                      (1 << 4)
#define DWEMMC_CTRL_DMA_EN                      (1 << 5)
#define DWEMMC_CTRL_IDMAC_EN                    (1 << 25)
#define DWEMMC_CTRL_RESET_ALL                   (DWEMMC_CTRL_RESET | DWEMMC_CTRL_FIFO_RESET | DWEMMC_CTRL_DMA_RESET)

#define DWEMMC_STS_DATA_BUSY                    (1 << 9)

#define DWEMMC_FIFO_TWMARK(x)                   (x & 0xfff)
#define DWEMMC_FIFO_RWMARK(x)                   ((x & 0x1ff) << 16)
#define DWEMMC_DMA_BURST_SIZE(x)                ((x & 0x7) << 28)

#define DWEMMC_CARD_RD_THR(x)                   ((x & 0xfff) << 16)
#define DWEMMC_CARD_RD_THR_EN                   (1 << 0)

#define DWEMMC_GET_HDATA_WIDTH(x)               (((x) >> 7) & 0x7)

#endif  // __DWEMMC_H__

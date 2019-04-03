/** @file
Include file for I2C DXE Driver register definitions (PCIe config. space and memory space).

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _I2C_REGS_H_
#define _I2C_REGS_H_


//----------------------------------------------------------------------------
/// I2C Device Address
//----------------------------------------------------------------------------
typedef struct {
  ///
  /// The I2C hardware address to which the I2C device is preassigned or allocated.
  ///
  UINTN I2CDeviceAddress : 10;
} EFI_I2C_DEVICE_ADDRESS;

//----------------------------------------------------------------------------
/// I2C Addressing Mode (7-bit or 10 bit)
//----------------------------------------------------------------------------
typedef enum _EFI_I2C_ADDR_MODE {
  EfiI2CSevenBitAddrMode,
  EfiI2CTenBitAddrMode,
} EFI_I2C_ADDR_MODE;


//----------------------------------------------------------------------------
// I2C Controller B:D:F
//----------------------------------------------------------------------------
#define I2C_Bus     0x00
#define I2C_Device  0x15
#define I2C_Func    0x02

//----------------------------------------------------------------------------
// Memory Mapped Registers
//----------------------------------------------------------------------------
#define I2C_REG_CON                        0x00          // Control Register
#define   B_I2C_REG_CON_SPEED                (BIT2+BIT1)   // standard mode (01) or fast mode (10)
#define   B_I2C_REG_CON_10BITADD_MASTER      (BIT4)        // 7-bit addressing (0) or 10-bit addressing (1)
#define I2C_REG_TAR                        0x04          // Master Target Address Register
#define   B_I2C_REG_TAR                      (BIT9+BIT8+BIT7+BIT6+BIT5+BIT4+BIT3+BIT2+BIT1+BIT0) // Master Target Address bits
#define I2C_REG_DATA_CMD                   0x10          // Data Buffer and Command Register
#define   B_I2C_REG_DATA_CMD_RW              (BIT8)      // Data Buffer and Command Register Read/Write bit
#define   B_I2C_REG_DATA_CMD_STOP            (BIT9)      // Data Buffer and Command Register STOP bit
#define   B_I2C_REG_DATA_CMD_RESTART         (BIT10)     // Data Buffer and Command Register RESTART bit
#define I2C_REG_SS_SCL_HCNT                0x14          // Standard Speed Clock SCL High Count Register
#define I2C_REG_SS_SCL_LCNT                0x18          // Standard Speed Clock SCL Low Count Register
#define I2C_REG_FS_SCL_HCNT                0x1C          // Fast Speed Clock SCL High Count Register
#define I2C_REG_FS_SCL_LCNT                0x20          // Fast Speed Clock SCL Low Count Register
#define I2C_REG_INTR_STAT                  0x2C          // Interrupt Status Register
#define   B_I2C_REG_INTR_STAT_STOP_DET       (BIT9)        // Interrupt Status Register STOP_DET signal status
#define I2C_REG_INTR_MASK                  0x30          // Interrupt Status Mask Register
#define I2C_REG_RAW_INTR_STAT              0x34          // Raw Interrupt Status Register
#define   I2C_REG_RAW_INTR_STAT_STOP_DET    (BIT9)         // Raw Interrupt Status Register STOP_DET signal status.
#define   I2C_REG_RAW_INTR_STAT_TX_ABRT     (BIT6)         // Raw Interrupt Status Register TX Abort status.
#define   I2C_REG_RAW_INTR_STAT_TX_OVER     (BIT3)         // Raw Interrupt Status Register TX Overflow signal status.
#define   I2C_REG_RAW_INTR_STAT_RX_OVER     (BIT1)         // Raw Interrupt Status Register RX Overflow signal status.
#define   I2C_REG_RAW_INTR_STAT_RX_UNDER    (BIT0)         // Raw Interrupt Status Register RX Underflow signal status.
#define I2C_REG_RX_TL                      0x38          // Receive FIFO Threshold Level Register
#define I2C_REG_TX_TL                      0x3C          // Transmit FIFO Threshold Level Register
#define I2C_REG_CLR_INT                    0x40          // Clear Combined and Individual Interrupt Register
#define I2C_REG_CLR_RX_UNDER               0x44          // Clear RX Under Interrupt Register
#define I2C_REG_CLR_RX_OVER                0x48          // Clear RX Over Interrupt Register
#define I2C_REG_CLR_TX_OVER                0x4C          // Clear TX Over Interrupt Register
#define I2C_REG_CLR_RD_REQ                 0x50          // Clear RD REQ Interrupt Register
#define I2C_REG_CLR_TX_ABRT                0x54          // Clear TX ABRT Interrupt Register
#define I2C_REG_CLR_ACTIVITY               0x5C          // Clear Activity Interrupt Register
#define I2C_REG_CLR_STOP_DET               0x60          // Clear STOP DET Interrupt Register
#define   B_I2C_REG_CLR_STOP_DET             (BIT0)        // Clear STOP DET Interrupt Register
#define I2C_REG_CLR_START_DET              0x64          // Clear START DET Interrupt Register
#define   B_I2C_REG_CLR_START_DET          (BIT0)          // Clear START DET Interrupt Register
#define I2C_REG_ENABLE                     0x6C          // Enable Register
#define   B_I2C_REG_ENABLE                   (BIT0)        // Enable (1) or disable (0) I2C Controller
#define I2C_REG_STATUS                     0x70          // Status Register
#define I2C_REG_TXFLR                      0x74          // Transmit FIFO Level Register
#define   B_I2C_REG_TXFLR                   (BIT3+BIT2+BIT1+BIT0)  // Transmit FIFO Level Register bits
#define I2C_REG_RXFLR                      0x78          // Receive FIFO Level Register
#define   B_I2C_REG_RXFLR                   (BIT3+BIT2+BIT1+BIT0)  // Receive FIFO Level Register bits
#define I2C_REG_SDA_HOLD                   0x7C          // SDA HOLD Register
#define I2C_REG_TX_ABRT_SOURCE             0x80          // Transmit Abort Source Register
#define I2C_REG_ENABLE_STATUS              0x9C          // Enable Status Register
#define I2C_REG_FS_SPKLEN                  0xA0          // SS and FS Spike Suppression Limit Register

//
// Features.
//
#define I2C_FIFO_SIZE                      16

#endif

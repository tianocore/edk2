/** @file
  Register Definitions for I2C Driver/PEIM.
  
  Copyright (c) 2004  - 2015, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                               
--*/

#ifndef I2C_REGS_H
#define I2C_REGS_H

//
// FIFO write delay value.
//
#define FIFO_WRITE_DELAY    2

//
// MMIO Register Definitions.
//
#define    R_IC_CON                          ( 0x00) // I2C Control  
#define     B_IC_RESTART_EN                  BIT5
#define     B_IC_SLAVE_DISABLE               BIT6
#define     V_SPEED_STANDARD                 0x02
#define     V_SPEED_FAST                     0x04
#define     V_SPEED_HIGH                     0x06
#define     B_MASTER_MODE                    BIT0

#define    R_IC_TAR                          ( 0x04) // I2C Target Address
#define     IC_TAR_10BITADDR_MASTER           BIT12

#define    R_IC_SAR                          ( 0x08) // I2C Slave Address
#define    R_IC_HS_MADDR                     ( 0x0C) // I2C HS MasterMode Code Address
#define    R_IC_DATA_CMD                     ( 0x10) // I2C Rx/Tx Data Buffer and Command

#define    B_READ_CMD                         BIT8    // 1 = read, 0 = write
#define    B_CMD_STOP                         BIT9    // 1 = STOP
#define    B_CMD_RESTART                      BIT10   // 1 = IC_RESTART_EN

#define    V_WRITE_CMD_MASK                  ( 0xFF)

#define    R_IC_SS_SCL_HCNT                  ( 0x14) // Standard Speed I2C Clock SCL High Count
#define    R_IC_SS_SCL_LCNT                  ( 0x18) // Standard Speed I2C Clock SCL Low Count
#define    R_IC_FS_SCL_HCNT                  ( 0x1C) // Full Speed I2C Clock SCL High Count
#define    R_IC_FS_SCL_LCNT                  ( 0x20) // Full Speed I2C Clock SCL Low Count
#define    R_IC_HS_SCL_HCNT                  ( 0x24) // High Speed I2C Clock SCL High Count
#define    R_IC_HS_SCL_LCNT                  ( 0x28) // High Speed I2C Clock SCL Low Count
#define    R_IC_INTR_STAT                    ( 0x2C) // I2C Inetrrupt Status
#define    R_IC_INTR_MASK                    ( 0x30) // I2C Interrupt Mask
#define     I2C_INTR_GEN_CALL                 BIT11  // General call received
#define     I2C_INTR_START_DET                BIT10
#define     I2C_INTR_STOP_DET                 BIT9
#define     I2C_INTR_ACTIVITY                 BIT8
#define     I2C_INTR_TX_ABRT                  BIT6   // Set on NACK
#define     I2C_INTR_TX_EMPTY                 BIT4
#define     I2C_INTR_TX_OVER                  BIT3
#define     I2C_INTR_RX_FULL                  BIT2   // Data bytes in RX FIFO over threshold
#define     I2C_INTR_RX_OVER                  BIT1
#define     I2C_INTR_RX_UNDER                 BIT0
#define    R_IC_RawIntrStat                ( 0x34) // I2C Raw Interrupt Status
#define    R_IC_RX_TL                        ( 0x38) // I2C Receive FIFO Threshold
#define    R_IC_TX_TL                        ( 0x3C) // I2C Transmit FIFO Threshold
#define    R_IC_CLR_INTR                     ( 0x40) // Clear Combined and Individual Interrupts
#define    R_IC_CLR_RX_UNDER                 ( 0x44) // Clear RX_UNDER Interrupt
#define    R_IC_CLR_RX_OVER                  ( 0x48) // Clear RX_OVERinterrupt
#define    R_IC_CLR_TX_OVER                  ( 0x4C) // Clear TX_OVER interrupt
#define    R_IC_CLR_RD_REQ                   ( 0x50) // Clear RD_REQ interrupt
#define    R_IC_CLR_TX_ABRT                  ( 0x54) // Clear TX_ABRT interrupt
#define    R_IC_CLR_RX_DONE                  ( 0x58) // Clear RX_DONE interrupt
#define    R_IC_CLR_ACTIVITY                 ( 0x5C) // Clear ACTIVITY interrupt
#define    R_IC_CLR_STOP_DET                 ( 0x60) // Clear STOP_DET interrupt
#define    R_IC_CLR_START_DET                ( 0x64) // Clear START_DET interrupt
#define    R_IC_CLR_GEN_CALL                 ( 0x68) // Clear GEN_CALL interrupt
#define    R_IC_ENABLE                       ( 0x6C) // I2C Enable
#define    R_IC_STATUS                       ( 0x70) // I2C Status

#define    R_IC_SDA_HOLD                     ( 0x7C) // I2C IC_DEFAULT_SDA_HOLD//16bits

#define     STAT_MST_ACTIVITY                 BIT5   // Master FSM Activity Status.
#define     STAT_RFF                          BIT4   // RX FIFO is completely full
#define     STAT_RFNE                         BIT3   // RX FIFO is not empty
#define     STAT_TFE                          BIT2   // TX FIFO is completely empty
#define     STAT_TFNF                         BIT1   // TX FIFO is not full

#define    R_IC_TXFLR                        ( 0x74) // Transmit FIFO Level Register
#define    R_IC_RXFLR                        ( 0x78) // Receive FIFO Level Register
#define    R_IC_TX_ABRT_SOURCE               ( 0x80) // I2C Transmit Abort Status Register
#define    R_IC_SLV_DATA_NACK_ONLY           ( 0x84) // Generate SLV_DATA_NACK Register
#define    R_IC_DMA_CR                       ( 0x88) // DMA Control Register
#define    R_IC_DMA_TDLR                     ( 0x8C) // DMA Transmit Data Level
#define    R_IC_DMA_RDLR                     ( 0x90) // DMA Receive Data Level
#define    R_IC_SDA_SETUP                    ( 0x94) // I2C SDA Setup Register
#define    R_IC_ACK_GENERAL_CALL             ( 0x98) // I2C ACK General Call Register
#define    R_IC_ENABLE_STATUS                ( 0x9C) // I2C Enable Status Register
#define    R_IC_COMP_PARAM                   ( 0xF4) // Component Parameter Register
#define    R_IC_COMP_VERSION                 ( 0xF8) // Component Version ID
#define    R_IC_COMP_TYPE                    ( 0xFC) // Component Type

#define    I2C_SS_SCL_HCNT_VALUE_100M        0x1DD
#define    I2C_SS_SCL_LCNT_VALUE_100M        0x1E4
#define    I2C_FS_SCL_HCNT_VALUE_100M        0x54
#define    I2C_FS_SCL_LCNT_VALUE_100M        0x9a
#define    I2C_HS_SCL_HCNT_VALUE_100M        0x7
#define    I2C_HS_SCL_LCNT_VALUE_100M        0xE

#define     IC_TAR_10BITADDR_MASTER           BIT12
#define     FIFO_SIZE                         32
#define     R_IC_INTR_STAT                    ( 0x2C) // I2c Inetrrupt Status
#define     R_IC_INTR_MASK                    ( 0x30) // I2c Interrupt Mask
#define     I2C_INTR_GEN_CALL                 BIT11  // General call received
#define     I2C_INTR_START_DET                BIT10
#define     I2C_INTR_STOP_DET                 BIT9
#define     I2C_INTR_ACTIVITY                 BIT8
#define     I2C_INTR_TX_ABRT                  BIT6   // Set on NACK
#define     I2C_INTR_TX_EMPTY                 BIT4
#define     I2C_INTR_TX_OVER                  BIT3
#define     I2C_INTR_RX_FULL                  BIT2   // Data bytes in RX FIFO over threshold
#define     I2C_INTR_RX_OVER                  BIT1
#define     I2C_INTR_RX_UNDER                 BIT0

#define R_PCH_LPIO_I2C_MEM_RESETS                 0x804 // Software Reset
#define B_PCH_LPIO_I2C_MEM_RESETS_FUNC            BIT1  // Function Clock Domain Reset
#define B_PCH_LPIO_I2C_MEM_RESETS_APB             BIT0  // APB Domain Reset
#define R_PCH_LPSS_I2C_MEM_PCP                    0x800 // Private Clock Parameters

#endif
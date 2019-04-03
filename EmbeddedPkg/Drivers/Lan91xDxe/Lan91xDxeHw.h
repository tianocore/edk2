/** @file
*  SMSC LAN91x series Network Controller Driver.
*
*  Copyright (c) 2013-2017 Linaro.org
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef __LAN91XDXEHW_H__
#define __LAN91XDXEHW_H__

#include <Base.h>

#define MakeRegister(Bank, Offset)  (((Bank) << 8) | (Offset))
#define RegisterToBank(Register)    (((Register) >> 8) & 0x07)
#define RegisterToOffset(Register)  ((Register) & 0x0f)

/*---------------------------------------------------------------------------------------------------------------------

        SMSC LAN91x Registers

---------------------------------------------------------------------------------------------------------------------*/
#define LAN91X_BANK_OFFSET      0xe                     // Bank Select Register (all banks)

#define LAN91X_TCR      MakeRegister (0, 0x0)           // Transmit Control Register
#define LAN91X_EPHSR    MakeRegister (0, 0x2)           // EPH Status Register
#define LAN91X_RCR      MakeRegister (0, 0x4)           // Receive Control Register
#define LAN91X_ECR      MakeRegister (0, 0x6)           // Counter Register
#define LAN91X_MIR      MakeRegister (0, 0x8)           // Memory Information Register
#define LAN91X_RPCR     MakeRegister (0, 0xa)           // Receive/Phy Control Register

#define LAN91X_CR       MakeRegister (1, 0x0)           // Configuration Register
#define LAN91X_BAR      MakeRegister (1, 0x2)           // Base Address Register
#define LAN91X_IAR0     MakeRegister (1, 0x4)           // Individual Address Register 0
#define LAN91X_IAR1     MakeRegister (1, 0x5)           // Individual Address Register 1
#define LAN91X_IAR2     MakeRegister (1, 0x6)           // Individual Address Register 2
#define LAN91X_IAR3     MakeRegister (1, 0x7)           // Individual Address Register 3
#define LAN91X_IAR4     MakeRegister (1, 0x8)           // Individual Address Register 4
#define LAN91X_IAR5     MakeRegister (1, 0x9)           // Individual Address Register 5
#define LAN91X_GPR      MakeRegister (1, 0xa)           // General Purpose Register
#define LAN91X_CTR      MakeRegister (1, 0xc)           // Control Register

#define LAN91X_MMUCR    MakeRegister (2, 0x0)           // MMU Command Register
#define LAN91X_PNR      MakeRegister (2, 0x2)           // Packet Number Register
#define LAN91X_ARR      MakeRegister (2, 0x3)           // Allocation Result Register
#define LAN91X_FIFO     MakeRegister (2, 0x4)           // FIFO Ports Register
#define LAN91X_PTR      MakeRegister (2, 0x6)           // Pointer Register
#define LAN91X_DATA0    MakeRegister (2, 0x8)           // Data Register 0
#define LAN91X_DATA1    MakeRegister (2, 0x9)           // Data Register 1
#define LAN91X_DATA2    MakeRegister (2, 0xa)           // Data Register 2
#define LAN91X_DATA3    MakeRegister (2, 0xb)           // Data Register 3
#define LAN91X_IST      MakeRegister (2, 0xc)           // Interrupt Status Register
#define LAN91X_MSK      MakeRegister (2, 0xd)           // Interrupt Mask Register

#define LAN91X_MT0      MakeRegister (3, 0x0)           // Multicast Table Register 0
#define LAN91X_MT1      MakeRegister (3, 0x1)           // Multicast Table Register 1
#define LAN91X_MT2      MakeRegister (3, 0x2)           // Multicast Table Register 2
#define LAN91X_MT3      MakeRegister (3, 0x3)           // Multicast Table Register 3
#define LAN91X_MT4      MakeRegister (3, 0x4)           // Multicast Table Register 4
#define LAN91X_MT5      MakeRegister (3, 0x5)           // Multicast Table Register 5
#define LAN91X_MT6      MakeRegister (3, 0x6)           // Multicast Table Register 6
#define LAN91X_MT7      MakeRegister (3, 0x7)           // Multicast Table Register 7
#define LAN91X_MGMT     MakeRegister (3, 0x8)           // Management Interface Register
#define LAN91X_REV      MakeRegister (3, 0xa)           // Revision Register
#define LAN91X_RCV      MakeRegister (3, 0xc)           // RCV Register

// Transmit Control Register Bits
#define TCR_TXENA       BIT0
#define TCR_LOOP        BIT1
#define TCR_FORCOL      BIT2
#define TCR_PAD_EN      BIT7
#define TCR_NOCRC       BIT8
#define TCR_MON_CSN     BIT10
#define TCR_FDUPLX      BIT11
#define TCR_STP_SQET    BIT12
#define TCR_EPH_LOOP    BIT13
#define TCR_SWFDUP      BIT15

#define TCR_DEFAULT     (TCR_TXENA | TCR_PAD_EN)
#define TCR_CLEAR       0x0

// EPH Status Register Bits
#define EPHSR_TX_SUC    BIT0
#define EPHSR_SNGLCOL   BIT1
#define EPHSR_MULCOL    BIT2
#define EPHSR_LTX_MULT  BIT3
#define EPHSR_16COL     BIT4
#define EPHSR_SQET      BIT5
#define EPHSR_LTX_BRD   BIT6
#define EPHSR_TX_DEFR   BIT7
#define EPHSR_LATCOL    BIT9
#define EPHSR_LOST_CARR BIT10
#define EPHSR_EXC_DEF   BIT11
#define EPHSR_CTR_ROL   BIT12
#define EPHSR_LINK_OK   BIT14

// Receive Control Register Bits
#define RCR_RX_ABORT    BIT0
#define RCR_PRMS        BIT1
#define RCR_ALMUL       BIT2
#define RCR_RXEN        BIT8
#define RCR_STRIP_CRC   BIT9
#define RCR_ABORT_ENB   BIT13
#define RCR_FILT_CAR    BIT14
#define RCR_SOFT_RST    BIT15

#define RCR_DEFAULT     (RCR_STRIP_CRC | RCR_RXEN)
#define RCR_CLEAR       0x0

// Receive/Phy Control Register Bits
#define RPCR_LS0B       BIT2
#define RPCR_LS1B       BIT3
#define RPCR_LS2B       BIT4
#define RPCR_LS0A       BIT5
#define RPCR_LS1A       BIT6
#define RPCR_LS2A       BIT7
#define RPCR_ANEG       BIT11
#define RPCR_DPLX       BIT12
#define RPCR_SPEED      BIT13

// Configuration Register Bits
#define CR_EXT_PHY      BIT9
#define CR_GPCNTRL      BIT10
#define CR_NO_WAIT      BIT12
#define CR_EPH_POWER_EN BIT15

#define CR_DEFAULT      (CR_EPH_POWER_EN | CR_NO_WAIT)

// Control Register Bits
#define CTR_STORE       BIT0
#define CTR_RELOAD      BIT1
#define CTR_EEPROM_SEL  BIT2
#define CTR_TE_ENABLE   BIT5
#define CTR_CR_ENABLE   BIT6
#define CTR_LE_ENABLE   BIT7
#define CTR_AUTO_REL    BIT11
#define CTR_RCV_BAD     BIT14

#define CTR_RESERVED    (BIT12 | BIT9 | BIT4)
#define CTR_DEFAULT     (CTR_RESERVED | CTR_AUTO_REL)

// MMU Command Register Bits
#define MMUCR_BUSY      BIT0

// MMU Command Register Operaction Codes
#define MMUCR_OP_NOOP           (0 << 5)        // No operation
#define MMUCR_OP_TX_ALLOC       (1 << 5)        // Allocate memory for TX
#define MMUCR_OP_RESET_MMU      (2 << 5)        // Reset MMU to initial state
#define MMUCR_OP_RX_POP         (3 << 5)        // Remove frame from top of RX FIFO
#define MMUCR_OP_RX_POP_REL     (4 << 5)        // Remove and release frame from top of RX FIFO
#define MMUCR_OP_RX_REL         (5 << 5)        // Release specific RX frame
#define MMUCR_OP_TX_PUSH        (6 << 5)        // Enqueue packet number into TX FIFO
#define MMUCR_OP_TX_RESET       (7 << 5)        // Reset TX FIFOs

// Packet Number Register Bits
#define PNR_PACKET      (0x3f)

// Allocation Result Register Bits
#define ARR_PACKET      (0x3f)
#define ARR_FAILED      BIT7

// FIFO Ports Register Bits
#define FIFO_TX_PACKET  (0x003f)
#define FIFO_TEMPTY     BIT7
#define FIFO_RX_PACKET  (0x3f00)
#define FIFO_REMPTY     BIT15

// Pointer Register Bits
#define PTR_POINTER     (0x07ff)
#define PTR_NOT_EMPTY   BIT11
#define PTR_READ        BIT13
#define PTR_AUTO_INCR   BIT14
#define PTR_RCV         BIT15

// Interupt Status and Mask Register Bits
#define IST_RCV         BIT0
#define IST_TX          BIT1
#define IST_TX_EMPTY    BIT2
#define IST_ALLOC       BIT3
#define IST_RX_OVRN     BIT4
#define IST_EPH         BIT5
#define IST_MD          BIT7

// Management Interface
#define MGMT_MDO        BIT0
#define MGMT_MDI        BIT1
#define MGMT_MCLK       BIT2
#define MGMT_MDOE       BIT3
#define MGMT_MSK_CRS100 BIT14

// RCV Register
#define RCV_MBO         (0x1f)
#define RCV_RCV_DISCRD  BIT7

// Packet RX Status word bits
#define RX_MULTICAST    BIT0
#define RX_HASH         (0x7e)
#define RX_TOO_SHORT    BIT10
#define RX_TOO_LONG     BIT11
#define RX_ODD_FRAME    BIT12
#define RX_BAD_CRC      BIT13
#define RX_BROADCAST    BIT14
#define RX_ALGN_ERR     BIT15

// Packet Byte Count word bits
#define BCW_COUNT       (0x7fe)

// Packet Control Word bits
#define PCW_ODD_BYTE    (0x00ff)
#define PCW_CRC         BIT12
#define PCW_ODD         BIT13

/*---------------------------------------------------------------------------------------------------------------------

        SMSC PHY Registers

        Most of these should be common, as there is
        documented STANDARD for PHY registers!

---------------------------------------------------------------------------------------------------------------------*/
//
// PHY Register Numbers
//
#define PHY_INDEX_BASIC_CTRL              0
#define PHY_INDEX_BASIC_STATUS            1
#define PHY_INDEX_ID1                     2
#define PHY_INDEX_ID2                     3
#define PHY_INDEX_AUTO_NEG_ADVERT         4
#define PHY_INDEX_AUTO_NEG_LINK_ABILITY   5

#define PHY_INDEX_CONFIG1                 16
#define PHY_INDEX_CONFIG2                 17
#define PHY_INDEX_STATUS_OUTPUT           18
#define PHY_INDEX_MASK                    19


// PHY control register bits
#define PHYCR_COLL_TEST                       BIT7                  // Collision test enable
#define PHYCR_DUPLEX_MODE                     BIT8                  // Set Duplex Mode
#define PHYCR_RST_AUTO                        BIT9                  // Restart Auto-Negotiation of Link abilities
#define PHYCR_PD                              BIT11                 // Power-Down switch
#define PHYCR_AUTO_EN                         BIT12                 // Auto-Negotiation Enable
#define PHYCR_SPEED_SEL                       BIT13                 // Link Speed Selection
#define PHYCR_LOOPBK                          BIT14                 // Set loopback mode
#define PHYCR_RESET                           BIT15                 // Do a PHY reset

// PHY status register bits
#define PHYSTS_EXT_CAP                        BIT0                  // Extended Capabilities Register capability
#define PHYSTS_JABBER                         BIT1                  // Jabber condition detected
#define PHYSTS_LINK_STS                       BIT2                  // Link Status
#define PHYSTS_AUTO_CAP                       BIT3                  // Auto-Negotiation Capability
#define PHYSTS_REMOTE_FAULT                   BIT4                  // Remote fault detected
#define PHYSTS_AUTO_COMP                      BIT5                  // Auto-Negotiation Completed
#define PHYSTS_10BASET_HDPLX                  BIT11                 // 10Mbps Half-Duplex ability
#define PHYSTS_10BASET_FDPLX                  BIT12                 // 10Mbps Full-Duplex ability
#define PHYSTS_100BASETX_HDPLX                BIT13                 // 100Mbps Half-Duplex ability
#define PHYSTS_100BASETX_FDPLX                BIT14                 // 100Mbps Full-Duplex ability
#define PHYSTS_100BASE_T4                     BIT15                 // Base T4 ability

// PHY Auto-Negotiation advertisement
#define PHYANA_SEL_MASK                       ((UINT32)0x1F)        // Link type selector
#define PHYANA_CSMA                           BIT0                  // Advertise CSMA capability
#define PHYANA_10BASET                        BIT5                  // Advertise 10BASET capability
#define PHYANA_10BASETFD                      BIT6                  // Advertise 10BASET Full duplex capability
#define PHYANA_100BASETX                      BIT7                  // Advertise 100BASETX capability
#define PHYANA_100BASETXFD                    BIT8                  // Advertise 100 BASETX Full duplex capability
#define PHYANA_100BASET4                      BIT9                  // Advertise 100 BASETX Full duplex capability
#define PHYANA_PAUSE_OP_MASK                  (3 << 10)             // Advertise PAUSE frame capability
#define PHYANA_REMOTE_FAULT                   BIT13                 // Remote fault detected

#endif /* __LAN91XDXEHW_H__ */

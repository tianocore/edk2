/** @file
*
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef __LAN9118_DXE_HW_H__
#define __LAN9118_DXE_HW_H__

/*------------------------------------------------------------------------------
  LAN9118 SMCS Registers
------------------------------------------------------------------------------*/

// Base address as on the VE board
#define LAN9118_BA                            ((UINT32) PcdGet32(PcdLan9118DxeBaseAddress))

/* ------------- Tx and Rx Data and Status Memory Locations ------------------*/
#define LAN9118_RX_DATA                       (0x00000000 + LAN9118_BA)
#define LAN9118_RX_STATUS                     (0x00000040 + LAN9118_BA)
#define LAN9118_RX_STATUS_PEEK                (0x00000044 + LAN9118_BA)
#define LAN9118_TX_DATA                       (0x00000020 + LAN9118_BA)
#define LAN9118_TX_STATUS                     (0x00000048 + LAN9118_BA)
#define LAN9118_TX_STATUS_PEEK                (0x0000004C + LAN9118_BA)

/* ------------- System Control and Status Registers -------------------------*/
#define LAN9118_ID_REV                        (0x00000050 + LAN9118_BA)    // Chip ID and Revision
#define LAN9118_IRQ_CFG                       (0x00000054 + LAN9118_BA)    // Interrupt Configuration
#define LAN9118_INT_STS                       (0x00000058 + LAN9118_BA)    // Interrupt Status
#define LAN9118_INT_EN                        (0x0000005C + LAN9118_BA)    // Interrupt Enable
//#define LAN9118_RESERVED                    (0x00000060)
#define LAN9118_BYTE_TEST                     (0x00000064 + LAN9118_BA)    // Byte Order Test
#define LAN9118_FIFO_INT                      (0x00000068 + LAN9118_BA)    // FIFO Level Interrupts
#define LAN9118_RX_CFG                        (0x0000006C + LAN9118_BA)    // Receive Configuration
#define LAN9118_TX_CFG                        (0x00000070 + LAN9118_BA)    // Transmit Configuration
#define LAN9118_HW_CFG                        (0x00000074 + LAN9118_BA)    // Hardware Configuration
#define LAN9118_RX_DP_CTL                     (0x00000078 + LAN9118_BA)    // Receive Data-Path Configuration
#define LAN9118_RX_FIFO_INF                   (0x0000007C + LAN9118_BA)    // Receive FIFO Information
#define LAN9118_TX_FIFO_INF                   (0x00000080 + LAN9118_BA)    // Transmit FIFO Information
#define LAN9118_PMT_CTRL                      (0x00000084 + LAN9118_BA)    // Power Management Control
#define LAN9118_GPIO_CFG                      (0x00000088 + LAN9118_BA)    // General Purpose IO Configuration
#define LAN9118_GPT_CFG                       (0x0000008C + LAN9118_BA)    // General Purpose Timer Configuration
#define LAN9118_GPT_CNT                       (0x00000090 + LAN9118_BA)    // General Purpose Timer Current Count
#define LAN9118_WORD_SWAP                     (0x00000098 + LAN9118_BA)    // Word Swap Control
#define LAN9118_FREE_RUN                      (0x0000009C + LAN9118_BA)    // Free-Run 25MHz Counter
#define LAN9118_RX_DROP                       (0x000000A0 + LAN9118_BA)    // Receiver Dropped Frames Counter
#define LAN9118_MAC_CSR_CMD                   (0x000000A4 + LAN9118_BA)    // MAC CSR Synchronizer Command
#define LAN9118_MAC_CSR_DATA                  (0x000000A8 + LAN9118_BA)    // MAC CSR Synchronizer Data
#define LAN9118_AFC_CFG                       (0x000000AC + LAN9118_BA)    // Automatic Flow Control Configuration
#define LAN9118_E2P_CMD                       (0x000000B0 + LAN9118_BA)    // EEPROM Command
#define LAN9118_E2P_DATA                      (0x000000B4 + LAN9118_BA)    // EEPROM Data

/*
 * Required delays following write cycles (number of BYTE_TEST reads)
 * Taken from Table 6.1 in Revision 1.5 (07-11-08) of the LAN9118 datasheet.
 * Where no delay listed, 0 has been assumed.
 */
#define LAN9118_RX_DATA_WR_DELAY              0
#define LAN9118_RX_STATUS_WR_DELAY            0
#define LAN9118_RX_STATUS_PEEK_WR_DELAY       0
#define LAN9118_TX_DATA_WR_DELAY              0
#define LAN9118_TX_STATUS_WR_DELAY            0
#define LAN9118_TX_STATUS_PEEK_WR_DELAY       0
#define LAN9118_ID_REV_WR_DELAY               0
#define LAN9118_IRQ_CFG_WR_DELAY              3
#define LAN9118_INT_STS_WR_DELAY              2
#define LAN9118_INT_EN_WR_DELAY               1
#define LAN9118_BYTE_TEST_WR_DELAY            0
#define LAN9118_FIFO_INT_WR_DELAY             1
#define LAN9118_RX_CFG_WR_DELAY               1
#define LAN9118_TX_CFG_WR_DELAY               1
#define LAN9118_HW_CFG_WR_DELAY               1
#define LAN9118_RX_DP_CTL_WR_DELAY            1
#define LAN9118_RX_FIFO_INF_WR_DELAY          0
#define LAN9118_TX_FIFO_INF_WR_DELAY          3
#define LAN9118_PMT_CTRL_WR_DELAY             7
#define LAN9118_GPIO_CFG_WR_DELAY             1
#define LAN9118_GPT_CFG_WR_DELAY              1
#define LAN9118_GPT_CNT_WR_DELAY              3
#define LAN9118_WORD_SWAP_WR_DELAY            1
#define LAN9118_FREE_RUN_WR_DELAY             4
#define LAN9118_RX_DROP_WR_DELAY              0
#define LAN9118_MAC_CSR_CMD_WR_DELAY          1
#define LAN9118_MAC_CSR_DATA_WR_DELAY         1
#define LAN9118_AFC_CFG_WR_DELAY              1
#define LAN9118_E2P_CMD_WR_DELAY              1
#define LAN9118_E2P_DATA_WR_DELAY             1

/*
 * Required delays following read cycles (number of BYTE_TEST reads)
 * Taken from Table 6.2 in Revision 1.5 (07-11-08) of the LAN9118 datasheet.
 * Where no delay listed, 0 has been assumed.
 */
#define LAN9118_RX_DATA_RD_DELAY              3
#define LAN9118_RX_STATUS_RD_DELAY            3
#define LAN9118_RX_STATUS_PEEK_RD_DELAY       0
#define LAN9118_TX_DATA_RD_DELAY              0
#define LAN9118_TX_STATUS_RD_DELAY            3
#define LAN9118_TX_STATUS_PEEK_RD_DELAY       0
#define LAN9118_ID_REV_RD_DELAY               0
#define LAN9118_IRQ_CFG_RD_DELAY              0
#define LAN9118_INT_STS_RD_DELAY              0
#define LAN9118_INT_EN_RD_DELAY               0
#define LAN9118_BYTE_TEST_RD_DELAY            0
#define LAN9118_FIFO_INT_RD_DELAY             0
#define LAN9118_RX_CFG_RD_DELAY               0
#define LAN9118_TX_CFG_RD_DELAY               0
#define LAN9118_HW_CFG_RD_DELAY               0
#define LAN9118_RX_DP_CTL_RD_DELAY            0
#define LAN9118_RX_FIFO_INF_RD_DELAY          0
#define LAN9118_TX_FIFO_INF_RD_DELAY          0
#define LAN9118_PMT_CTRL_RD_DELAY             0
#define LAN9118_GPIO_CFG_RD_DELAY             0
#define LAN9118_GPT_CFG_RD_DELAY              0
#define LAN9118_GPT_CNT_RD_DELAY              0
#define LAN9118_WORD_SWAP_RD_DELAY            0
#define LAN9118_FREE_RUN_RD_DELAY             0
#define LAN9118_RX_DROP_RD_DELAY              4
#define LAN9118_MAC_CSR_CMD_RD_DELAY          0
#define LAN9118_MAC_CSR_DATA_RD_DELAY         0
#define LAN9118_AFC_CFG_RD_DELAY              0
#define LAN9118_E2P_CMD_RD_DELAY              0
#define LAN9118_E2P_DATA_RD_DELAY             0

// Receiver Status bits
#define RXSTATUS_CRC_ERROR                    BIT1                      // Cyclic Redundancy Check Error
#define RXSTATUS_DB                           BIT2                      // Dribbling bit: Frame had non-integer multiple of 8bits
#define RXSTATUS_MII_ERROR                    BIT3                      // Receive error during interception
#define RXSTATUS_RXW_TO                       BIT4                      // Incomming frame larger than 2kb
#define RXSTATUS_FT                           BIT5                      // 1: Ether type / 0: 802.3 type frame
#define RXSTATUS_LCOLL                        BIT6                      // Late collision detected
#define RXSTATUS_FTL                          BIT7                      // Frame longer than Ether type
#define RXSTATUS_MCF                          BIT10                     // Frame has Multicast Address
#define RXSTATUS_RUNT                         BIT11                     // Bad frame
#define RXSTATUS_LE                           BIT12                     // Actual length of frame different than it claims
#define RXSTATUS_BCF                          BIT13                     // Frame has Broadcast Address
#define RXSTATUS_ES                           BIT15                     // Reports any error from bits 1,6,7 and 11
#define RXSTATUS_PL_MASK                      (0x3FFF0000)              // Packet length bit mask
#define GET_RXSTATUS_PACKET_LENGTH(RxStatus)  (((RxStatus) >> 16) & 0x3FFF) // Packet length bit mask
#define RXSTATUS_FILT_FAIL                    BIT30                     // The frame failed filtering test

// Transmitter Status bits
#define TXSTATUS_DEF                          BIT0                      // Packet tx was deferred
#define TXSTATUS_EDEF                         BIT2                      // Tx ended because of excessive deferral (> 24288 bit times)
#define TXSTATUS_CC_MASK                      (0x00000078)              // Collision Count (before Tx) bit mask
#define TXSTATUS_ECOLL                        BIT8                      // Tx ended because of Excessive Collisions (makes CC_MASK invalid after 16 collisions)
#define TXSTATUS_LCOLL                        BIT9                      // Packet Tx aborted after coll window of 64 bytes
#define TXSTATUS_NO_CA                        BIT10                     // Carrier signal not present during Tx (bad?)
#define TXSTATUS_LOST_CA                      BIT11                     // Lost carrier during Tx
#define TXSTATUS_ES                           BIT15                     // Reports any errors from bits 1,2,8,9,10 and 11
#define TXSTATUS_PTAG_MASK                    (0xFFFF0000)              // Mask for Unique ID of packets (So we know who the packets are for)

// ID_REV register bits
#define IDREV_ID                              ((Lan9118MmioRead32(LAN9118_ID_REV) & 0xFFFF0000) >> 16)
#define IDREV_REV                             (Lan9118MmioRead32(LAN9118_ID_REV) & 0x0000FFFF)

// Interrupt Config Register bits
#define IRQCFG_IRQ_TYPE                       BIT0                    // IRQ Buffer type
#define IRQCFG_IRQ_POL                        BIT4                    // IRQ Polarity
#define IRQCFG_IRQ_EN                         BIT8                    // Enable external interrupt
#define IRQCFG_IRQ_INT                        BIT12                   // State of internal interrupts line
#define IRQCFG_INT_DEAS_STS                   BIT13                   // State of deassertion interval
#define IRQCFG_INT_DEAS_CLR                   BIT14                   // Clear the deassertion counter
#define IRQCFG_INT_DEAS_MASK                  (0xFF000000)            // Interrupt deassertion interval value mask

// Interrupt Status Register bits
#define INSTS_GPIO_MASK                       (0x7)                   // GPIO interrupts mask
#define INSTS_RSFL                            (0x8)                   // Rx Status FIFO Level reached
#define INSTS_RSFF                            BIT4                    // Rx Status FIFO full
#define INSTS_RXDF_INT                        BIT6                    // Rx Frame dropped
#define INSTS_TSFL                            BIT7                    // Tx Status FIFO Level reached
#define INSTS_TSFF                            BIT8                    // Tx Status FIFO full
#define INSTS_TDFA                            BIT9                    // Tx Data FIFO Level exceeded
#define INSTS_TDFO                            BIT10                   // Tx Data FIFO full
#define INSTS_TXE                             BIT13                   // Transmitter Error
#define INSTS_RXE                             BIT14                   // Receiver Error
#define INSTS_RWT                             BIT15                   // Packet > 2048 bytes received
#define INSTS_TXSO                            BIT16                   // Tx Status FIFO Overflow
#define INSTS_PME_INT                         BIT17                   // PME Signal detected
#define INSTS_PHY_INT                         BIT18                   // Indicates PHY Interrupt
#define INSTS_GPT_INT                         BIT19                   // GP Timer wrapped past 0xFFFF
#define INSTS_RXD_INT                         BIT20                   // Indicates that amount of data written to RX_CFG was cleared
#define INSTS_TX_IOC                          BIT21                   // Finished loading IOC flagged buffer to Tx FIFO
#define INSTS_RXDFH_INT                       BIT23                   // Rx Dropped frames went past 0x7FFFFFFF
#define INSTS_RXSTOP_INT                      BIT24                   // Rx was stopped
#define INSTS_TXSTOP_INT                      BIT25                   // Tx was stopped
#define INSTS_SW_INT                          BIT31                   // Software Interrupt occurred

// Interrupt Enable Register bits


// Hardware Config Register bits
#define HWCFG_SRST                            BIT0                       // Software Reset bit         (SC)
#define HWCFG_SRST_TO                         BIT1                       // Software Reset Timeout bit (RO)
#define HWCFG_BMODE                           BIT2                       // 32/16 bit Mode bit         (RO)
#define HWCFG_TX_FIFO_SIZE_MASK               (~ (UINT32)0xF0000)        // Mask to Clear FIFO Size
#define HWCFG_MBO                             BIT20                      // Must Be One bit

// Power Management Control Register
#define MPTCTRL_READY                         BIT0                // Device ready indicator
#define MPTCTRL_PME_EN                        BIT1                // Enable external PME signals
#define MPTCTRL_PME_POL                       BIT2                // Set polarity of PME signals
#define MPTCTRL_PME_IND                       BIT3                // Signal type of PME (refer to Spec)
#define MPTCTRL_WUPS_MASK                     (0x18)              // Wake up status indicator mask
#define MPTCTRL_PME_TYPE                      BIT6                // PME Buffer type (Open Drain or Push-Pull)
#define MPTCTRL_ED_EN                         BIT8                // Energy-detect enable
#define MPTCTRL_WOL_EN                        BIT9                // Enable wake-on-lan
#define MPTCTRL_PHY_RST                       BIT10               // Reset the PHY
#define MPTCTRL_PM_MODE_MASK                  (BIT12 | BIT13)     // Set the power mode

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
#define PHYANA_10BASET                        BIT5                  // Advertise 10BASET capability
#define PHYANA_10BASETFD                      BIT6                  // Advertise 10BASET Full duplex capability
#define PHYANA_100BASETX                      BIT7                  // Advertise 100BASETX capability
#define PHYANA_100BASETXFD                    BIT8                  // Advertise 100 BASETX Full duplex capability
#define PHYANA_PAUSE_OP_MASK                  (3 << 10)             // Advertise PAUSE frame capability
#define PHYANA_REMOTE_FAULT                   BIT13                 // Remote fault detected


// PHY Auto-Negotiation Link Partner Ability

// PHY Auto-Negotiation Expansion

// PHY Mode control/status

// PHY Special Modes

// PHY Special control/status

// PHY Interrupt Source Flags

// PHY Interrupt Mask

// PHY Super Special control/status
#define PHYSSCS_HCDSPEED_MASK                 (7 << 2)              // Speed indication
#define PHYSSCS_AUTODONE                      BIT12                 // Auto-Negotiation Done


// MAC control register bits
#define MACCR_RX_EN                       BIT2                     // Enable Receiver bit
#define MACCR_TX_EN                       BIT3                     // Enable Transmitter bit
#define MACCR_DFCHK                       BIT5                     // Deferral Check bit
#define MACCR_PADSTR                      BIT8                     // Automatic Pad Stripping bit
#define MACCR_BOLMT_MASK                  (0xC0)                   // Back-Off limit mask
#define MACCR_DISRTY                      BIT10                    // Disable Transmit Retry bit
#define MACCR_BCAST                       BIT11                    // Disable Broadcast Frames bit
#define MACCR_LCOLL                       BIT12                    // Late Collision Control bit
#define MACCR_HPFILT                      BIT13                    // Hash/Perfect Filtering Mode bit
#define MACCR_HO                          BIT15                    // Hash Only Filtering Mode
#define MACCR_PASSBAD                     BIT16                    // Receive all frames that passed filter bit
#define MACCR_INVFILT                     BIT17                    // Enable Inverse Filtering bit
#define MACCR_PRMS                        BIT18                    // Promiscuous Mode bit
#define MACCR_MCPAS                       BIT19                    // Pass all Multicast packets bit
#define MACCR_FDPX                        BIT20                    // Full Duplex Mode bit
#define MACCR_LOOPBK                      BIT21                    // Loopback operation mode bit
#define MACCR_RCVOWN                      BIT23                    // Disable Receive Own frames bit
#define MACCR_RX_ALL                      BIT31                    // Receive all Packets and route to Filter

// Wake-Up Control and Status Register
#define WUCSR_MPEN                        BIT1                     // Magic Packet enable (allow wake from Magic P)
#define WUCSR_WUEN                        BIT2                     // Allow remote wake up using Wake-Up Frames
#define WUCSR_MPR_MASK                    (0x10)                   // Received Magic Packet
#define WUCSR_WUFR_MASK                   (0x20)                   // Received Wake-Up Frame
#define WUCSR_GUE                         BIT9                     // Enable wake on global unicast frames

// RX Configuration Register bits
#define RXCFG_RXDOFF_MASK                 (0x1F00)                 // Rx Data Offset in Bytes
#define RXCFG_RX_DUMP                     BIT15                    // Clear Rx data and status FIFOs
#define RXCFG_RX_DMA_CNT_MASK             (0x0FFF0000)             // Amount of data to be read from Rx FIFO
#define RXCFG_RX_DMA_CNT(cnt)             (((cnt) & 0xFFF) << 16)  // Amount of data to be read from Rx FIFO
#define RXCFG_RX_END_ALIGN_MASK           (0xC0000000)             // Alignment to preserve

// TX Configuration Register bits
#define TXCFG_STOP_TX                     BIT0                     // Stop the transmitter
#define TXCFG_TX_ON                       BIT1                     // Start the transmitter
#define TXCFG_TXSAO                       BIT2                     // Tx Status FIFO full
#define TXCFG_TXD_DUMP                    BIT14                    // Clear Tx Data FIFO
#define TXCFG_TXS_DUMP                    BIT15                    // Clear Tx Status FIFO

// Rx FIFO Information Register bits
#define RXFIFOINF_RXDUSED_MASK            (0xFFFF)                 // Rx Data FIFO Used Space
#define RXFIFOINF_RXSUSED_MASK            (0xFF0000)               // Rx Status FIFO Used Space

// Tx FIFO Information Register bits
#define TXFIFOINF_TDFREE_MASK             (0xFFFF)                 // Tx Data FIFO Free Space
#define TXFIFOINF_TXSUSED_MASK            (0xFF0000)               // Tx Status FIFO Used Space

// E2P Register
#define E2P_EPC_BUSY                BIT31
#define E2P_EPC_CMD_READ            (0)
#define E2P_EPC_TIMEOUT             BIT9
#define E2P_EPC_MAC_ADDRESS_LOADED  BIT8
#define E2P_EPC_ADDRESS(address)    ((address) & 0xFFFF)

// GPIO Configuration register
#define GPIO_GPIO0_PUSH_PULL        BIT16
#define GPIO_GPIO1_PUSH_PULL        BIT17
#define GPIO_GPIO2_PUSH_PULL        BIT18
#define GPIO_LED1_ENABLE            BIT28
#define GPIO_LED2_ENABLE            BIT29
#define GPIO_LED3_ENABLE            BIT30

// MII_ACC bits
#define MII_ACC_MII_BUSY        BIT0
#define MII_ACC_MII_WRITE       BIT1
#define MII_ACC_MII_READ        0

#define MII_ACC_PHY_VALUE             BIT11
#define MII_ACC_MII_REG_INDEX(index)  (((index) & 0x1F) << 6)

//
// PHY Control Indexes
//
#define PHY_INDEX_BASIC_CTRL              0
#define PHY_INDEX_BASIC_STATUS            1
#define PHY_INDEX_ID1                     2
#define PHY_INDEX_ID2                     3
#define PHY_INDEX_AUTO_NEG_ADVERT         4
#define PHY_INDEX_AUTO_NEG_LINK_ABILITY   5
#define PHY_INDEX_AUTO_NEG_EXP            6
#define PHY_INDEX_MODE                    17
#define PHY_INDEX_SPECIAL_MODES           18
#define PHY_INDEX_SPECIAL_CTLR            27
#define PHY_INDEX_INT_SRC                 29
#define PHY_INDEX_INT_MASK                30
#define PHY_INDEX_SPECIAL_PHY_CTLR        31

// Indirect MAC Indexes
#define INDIRECT_MAC_INDEX_CR         1
#define INDIRECT_MAC_INDEX_ADDRH      2
#define INDIRECT_MAC_INDEX_ADDRL      3
#define INDIRECT_MAC_INDEX_HASHH      4
#define INDIRECT_MAC_INDEX_HASHL      5
#define INDIRECT_MAC_INDEX_MII_ACC    6
#define INDIRECT_MAC_INDEX_MII_DATA   7

//
// MAC CSR Synchronizer Command register
//
#define MAC_CSR_BUSY            BIT31
#define MAC_CSR_READ            BIT30
#define MAC_CSR_WRITE           0
#define MAC_CSR_ADDR(Addr)      ((Addr) & 0xFF)

//
// TX Packet Format
//
#define TX_CMD_A_COMPLETION_INT             BIT31
#define TX_CMD_A_FIRST_SEGMENT              BIT13
#define TX_CMD_A_LAST_SEGMENT               BIT12
#define TX_CMD_A_BUFF_SIZE(size)            ((size) & 0x000003FF)
#define TX_CMD_A_DATA_START_OFFSET(offset)  (((offset) & 0x1F) << 16)
#define TX_CMD_B_PACKET_LENGTH(size)        ((size) & 0x000003FF)
#define TX_CMD_B_PACKET_TAG(tag)            (((tag) & 0x3FF) << 16)

// Hardware Configuration Register
#define HW_CFG_TX_FIFO_SIZE_MASK        (0xF << 16)
#define HW_CFG_TX_FIFO_SIZE(size)       (((size) & 0xF) << 16)

// EEPROM Definition
#define EEPROM_EXTERNAL_SERIAL_EEPROM   0xA5

//
// Conditional compilation flags
//
//#define EVAL_PERFORMANCE


#endif /* __LAN9118_DXE_HDR_H__ */

/** @file
  Definitions for network adapter card.

Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _E100B_H_
#define _E100B_H_

// pci config offsets:

#define RX_BUFFER_COUNT 32
#define TX_BUFFER_COUNT 32

#define PCI_VENDOR_ID_INTEL 0x8086
#define PCI_DEVICE_ID_INTEL_82557 0x1229
#define D100_VENDOR_ID   0x8086
#define D100_DEVICE_ID   0x1229
#define D102_DEVICE_ID   0x2449

#define ICH3_DEVICE_ID_1   0x1031
#define ICH3_DEVICE_ID_2   0x1032
#define ICH3_DEVICE_ID_3   0x1033
#define ICH3_DEVICE_ID_4   0x1034
#define ICH3_DEVICE_ID_5   0x1035
#define ICH3_DEVICE_ID_6   0x1036
#define ICH3_DEVICE_ID_7   0x1037
#define ICH3_DEVICE_ID_8   0x1038

#define SPEEDO_DEVICE_ID   0x1227
#define SPLASH1_DEVICE_ID   0x1226


// bit fields for the command
#define PCI_COMMAND_MASTER  0x04  // bit 2
#define PCI_COMMAND_IO    0x01  // bit 0
#define PCI_COMMAND  0x04
#define PCI_LATENCY_TIMER  0x0D

#define ETHER_MAC_ADDR_LEN 6
#ifdef AVL_XXX
#define ETHER_HEADER_LEN 14
// media interface type
// #define INTERFACE_TYPE "

// Hardware type values
#define HW_ETHER_TYPE    1
#define HW_EXPERIMENTAL_ETHER_TYPE 2
#define HW_IEEE_TYPE    6
#define HW_ARCNET_TYPE     7

#endif  // AVL_XXX

#define MAX_ETHERNET_PKT_SIZE 1514  // including eth header
#define RX_BUFFER_SIZE 1536  // including crc and padding
#define TX_BUFFER_SIZE 64
#define ETH_MTU 1500  // does not include ethernet header length

#define SPEEDO3_TOTAL_SIZE 0x20

#pragma pack(1)

typedef struct eth {
  UINT8 dest_addr[PXE_HWADDR_LEN_ETHER];
  UINT8 src_addr[PXE_HWADDR_LEN_ETHER];
  UINT16 type;
} EtherHeader;

#pragma pack(1)
typedef struct CONFIG_HEADER {
  UINT16 VendorID;
  UINT16 DeviceID;
  UINT16 Command;
  UINT16 Status;
  UINT16 RevID;
  UINT16 ClassID;
  UINT8  CacheLineSize;
  UINT8  LatencyTimer;
  UINT8  HeaderType;    // must be zero to impose this structure...
  UINT8  BIST;  // built-in self test
  UINT32 BaseAddressReg_0;  // memory mapped address
  UINT32 BaseAddressReg_1;  //io mapped address, Base IO address
  UINT32 BaseAddressReg_2;  // option rom address
  UINT32 BaseAddressReg_3;
  UINT32 BaseAddressReg_4;
  UINT32 BaseAddressReg_5;
  UINT32 CardBusCISPtr;
  UINT16 SubVendorID;
  UINT16 SubSystemID;
  UINT32 ExpansionROMBaseAddr;
  UINT8 CapabilitiesPtr;
  UINT8 reserved1;
  UINT16 Reserved2;
  UINT32 Reserved3;
  UINT8 int_line;
  UINT8 int_pin;
  UINT8 Min_gnt;
  UINT8 Max_lat;
} PCI_CONFIG_HEADER;
#pragma pack()

//-------------------------------------------------------------------------
// Offsets to the various registers.
//   All accesses need not be longword aligned.
//-------------------------------------------------------------------------
enum speedo_offsets {
  SCBStatus = 0, SCBCmd = 2,     // Rx/Command Unit command and status.
  SCBPointer = 4,                // General purpose pointer.
  SCBPort = 8,                   // Misc. commands and operands.
  SCBflash = 12, SCBeeprom = 14, // EEPROM and flash memory control.
  SCBCtrlMDI = 16,               // MDI interface control.
  SCBEarlyRx = 20,               // Early receive byte count.
  SCBEarlyRxInt = 24, SCBFlowCtrlReg = 25, SCBPmdr = 27,
  // offsets for general control registers (GCRs)
  SCBGenCtrl = 28, SCBGenStatus = 29, SCBGenCtrl2 = 30, SCBRsvd = 31
};

#define GCR2_EEPROM_ACCESS_SEMAPHORE 0x80 // bit offset into the gcr2

//-------------------------------------------------------------------------
// Action commands - Commands that can be put in a command list entry.
//-------------------------------------------------------------------------
enum commands {
  CmdNOp = 0, CmdIASetup = 1, CmdConfigure = 2, CmdMulticastList = 3,
  CmdTx = 4, CmdTDR = 5, CmdDump = 6, CmdDiagnose = 7,
  CmdSuspend = 0x4000,    /* Suspend after completion. */
  CmdIntr = 0x2000,      /* Interrupt after completion. */
  CmdTxFlex = 0x0008      /* Use "Flexible mode" for CmdTx command. */
};

//-------------------------------------------------------------------------
// port commands
//-------------------------------------------------------------------------
#define PORT_RESET 0
#define PORT_SELF_TEST 1
#define POR_SELECTIVE_RESET 2
#define PORT_DUMP_POINTER 2

//-------------------------------------------------------------------------
// SCB Command Word bit definitions
//-------------------------------------------------------------------------
//- CUC fields
#define   CU_START    0x0010
#define   CU_RESUME    0x0020
#define   CU_STATSADDR  0x0040
#define   CU_SHOWSTATS  0x0050  /* Dump statistics counters. */
#define   CU_CMD_BASE  0x0060  /* Base address to add to add CU commands. */
#define   CU_DUMPSTATS  0x0070  /* Dump then reset stats counters. */

//- RUC fields
#define   RX_START  0x0001
#define   RX_RESUME  0x0002
#define   RX_ABORT  0x0004
#define   RX_ADDR_LOAD  0x0006  /* load ru_base_reg */
#define   RX_RESUMENR  0x0007

// Interrupt fields (assuming byte addressing)
#define INT_MASK  0x0100
#define DRVR_INT  0x0200    /* Driver generated interrupt. */

//- CB Status Word
#define CMD_STATUS_COMPLETE 0x8000
#define RX_STATUS_COMPLETE 0x8000
#define CMD_STATUS_MASK 0xF000

//-------------------------------------------------------------------------
//- SCB Status bits:
// Interrupts are ACKed by writing to the upper 6 interrupt bits
//-------------------------------------------------------------------------
#define SCB_STATUS_MASK        0xFC00 // bits 2-7 - STATUS/ACK Mask
#define SCB_STATUS_CX_TNO      0x8000 // BIT_15  - CX or TNO Interrupt
#define SCB_STATUS_FR          0x4000 // BIT_14 - FR Interrupt
#define SCB_STATUS_CNA         0x2000 // BIT_13 - CNA Interrupt
#define SCB_STATUS_RNR         0x1000 // BIT_12  - RNR Interrupt
#define SCB_STATUS_MDI         0x0800 // BIT_11  - MDI R/W Done Interrupt
#define SCB_STATUS_SWI         0x0400 // BIT_10  - SWI Interrupt

// CU STATUS: bits 6 & 7
#define SCB_STATUS_CU_MASK     0x00C0 // bits 6 & 7
#define SCB_STATUS_CU_IDLE     0x0000 // 00
#define SCB_STATUS_CU_SUSPEND  0x0040 // 01
#define SCB_STATUS_CU_ACTIVE   0x0080 // 10

// RU STATUS: bits 2-5
#define SCB_RUS_IDLE         0x0000
#define SCB_RUS_SUSPENDED    0x0004  // bit 2
#define SCB_RUS_NO_RESOURCES   0x0008 // bit 3
#define SCB_RUS_READY       0x0010 // bit 4

//-------------------------------------------------------------------------
// Bit Mask definitions
//-------------------------------------------------------------------------
#define BIT_0       0x0001
#define BIT_1       0x0002
#define BIT_2       0x0004
#define BIT_3       0x0008
#define BIT_4       0x0010
#define BIT_5       0x0020
#define BIT_6       0x0040
#define BIT_7       0x0080
#define BIT_8       0x0100
#define BIT_9       0x0200
#define BIT_10      0x0400
#define BIT_11      0x0800
#define BIT_12      0x1000
#define BIT_13      0x2000
#define BIT_14      0x4000
#define BIT_15      0x8000
#define BIT_24      0x01000000
#define BIT_28      0x10000000


//-------------------------------------------------------------------------
// MDI Control register bit definitions
//-------------------------------------------------------------------------
#define MDI_DATA_MASK           BIT_0_15        // MDI Data port
#define MDI_REG_ADDR            BIT_16_20       // which MDI register to read/write
#define MDI_PHY_ADDR            BIT_21_25       // which PHY to read/write
#define MDI_PHY_OPCODE          BIT_26_27       // which PHY to read/write
#define MDI_PHY_READY           BIT_28          // PHY is ready for another MDI cycle
#define MDI_PHY_INT_ENABLE      BIT_29          // Assert INT at MDI cycle completion

#define BIT_0_2     0x0007
#define BIT_0_3     0x000F
#define BIT_0_4     0x001F
#define BIT_0_5     0x003F
#define BIT_0_6     0x007F
#define BIT_0_7     0x00FF
#define BIT_0_8     0x01FF
#define BIT_0_13    0x3FFF
#define BIT_0_15    0xFFFF
#define BIT_1_2     0x0006
#define BIT_1_3     0x000E
#define BIT_2_5     0x003C
#define BIT_3_4     0x0018
#define BIT_4_5     0x0030
#define BIT_4_6     0x0070
#define BIT_4_7     0x00F0
#define BIT_5_7     0x00E0
#define BIT_5_9     0x03E0
#define BIT_5_12    0x1FE0
#define BIT_5_15    0xFFE0
#define BIT_6_7     0x00c0
#define BIT_7_11    0x0F80
#define BIT_8_10    0x0700
#define BIT_9_13    0x3E00
#define BIT_12_15   0xF000

#define BIT_16_20   0x001F0000
#define BIT_21_25   0x03E00000
#define BIT_26_27   0x0C000000

//-------------------------------------------------------------------------
// MDI Control register opcode definitions
//-------------------------------------------------------------------------
#define MDI_WRITE               1               // Phy Write
#define MDI_READ                2               // Phy read

//-------------------------------------------------------------------------
// PHY 100 MDI Register/Bit Definitions
//-------------------------------------------------------------------------
// MDI register set
#define MDI_CONTROL_REG             0x00        // MDI control register
#define MDI_STATUS_REG              0x01        // MDI Status regiser
#define PHY_ID_REG_1                0x02        // Phy indentification reg (word 1)
#define PHY_ID_REG_2                0x03        // Phy indentification reg (word 2)
#define AUTO_NEG_ADVERTISE_REG      0x04        // Auto-negotiation advertisement
#define AUTO_NEG_LINK_PARTNER_REG   0x05        // Auto-negotiation link partner ability
#define AUTO_NEG_EXPANSION_REG      0x06        // Auto-negotiation expansion
#define AUTO_NEG_NEXT_PAGE_REG      0x07        // Auto-negotiation next page transmit
#define EXTENDED_REG_0              0x10        // Extended reg 0 (Phy 100 modes)
#define EXTENDED_REG_1              0x14        // Extended reg 1 (Phy 100 error indications)
#define NSC_CONG_CONTROL_REG        0x17        // National (TX) congestion control
#define NSC_SPEED_IND_REG           0x19        // National (TX) speed indication

// MDI Control register bit definitions
#define MDI_CR_COLL_TEST_ENABLE     BIT_7       // Collision test enable
#define MDI_CR_FULL_HALF            BIT_8       // FDX =1, half duplex =0
#define MDI_CR_RESTART_AUTO_NEG     BIT_9       // Restart auto negotiation
#define MDI_CR_ISOLATE              BIT_10      // Isolate PHY from MII
#define MDI_CR_POWER_DOWN           BIT_11      // Power down
#define MDI_CR_AUTO_SELECT          BIT_12      // Auto speed select enable
#define MDI_CR_10_100               BIT_13      // 0 = 10Mbs, 1 = 100Mbs
#define MDI_CR_LOOPBACK             BIT_14      // 0 = normal, 1 = loopback
#define MDI_CR_RESET                BIT_15      // 0 = normal, 1 = PHY reset

// MDI Status register bit definitions
#define MDI_SR_EXT_REG_CAPABLE      BIT_0       // Extended register capabilities
#define MDI_SR_JABBER_DETECT        BIT_1       // Jabber detected
#define MDI_SR_LINK_STATUS          BIT_2       // Link Status -- 1 = link
#define MDI_SR_AUTO_SELECT_CAPABLE  BIT_3       // Auto speed select capable
#define MDI_SR_REMOTE_FAULT_DETECT  BIT_4       // Remote fault detect
#define MDI_SR_AUTO_NEG_COMPLETE    BIT_5       // Auto negotiation complete
#define MDI_SR_10T_HALF_DPX         BIT_11      // 10BaseT Half Duplex capable
#define MDI_SR_10T_FULL_DPX         BIT_12      // 10BaseT full duplex capable
#define MDI_SR_TX_HALF_DPX          BIT_13      // TX Half Duplex capable
#define MDI_SR_TX_FULL_DPX          BIT_14      // TX full duplex capable
#define MDI_SR_T4_CAPABLE           BIT_15      // T4 capable

// Auto-Negotiation advertisement register bit definitions
#define NWAY_AD_SELCTOR_FIELD       BIT_0_4     // identifies supported protocol
#define NWAY_AD_ABILITY             BIT_5_12    // technologies that are supported
#define NWAY_AD_10T_HALF_DPX        BIT_5       // 10BaseT Half Duplex capable
#define NWAY_AD_10T_FULL_DPX        BIT_6       // 10BaseT full duplex capable
#define NWAY_AD_TX_HALF_DPX         BIT_7       // TX Half Duplex capable
#define NWAY_AD_TX_FULL_DPX         BIT_8       // TX full duplex capable
#define NWAY_AD_T4_CAPABLE          BIT_9       // T4 capable
#define NWAY_AD_REMOTE_FAULT        BIT_13      // indicates local remote fault
#define NWAY_AD_RESERVED            BIT_14      // reserved
#define NWAY_AD_NEXT_PAGE           BIT_15      // Next page (not supported)

// Auto-Negotiation link partner ability register bit definitions
#define NWAY_LP_SELCTOR_FIELD       BIT_0_4     // identifies supported protocol
#define NWAY_LP_ABILITY             BIT_5_9     // technologies that are supported
#define NWAY_LP_REMOTE_FAULT        BIT_13      // indicates partner remote fault
#define NWAY_LP_ACKNOWLEDGE         BIT_14      // acknowledge
#define NWAY_LP_NEXT_PAGE           BIT_15      // Next page (not supported)

// Auto-Negotiation expansion register bit definitions
#define NWAY_EX_LP_NWAY             BIT_0       // link partner is NWAY
#define NWAY_EX_PAGE_RECEIVED       BIT_1       // link code word received
#define NWAY_EX_NEXT_PAGE_ABLE      BIT_2       // local is next page able
#define NWAY_EX_LP_NEXT_PAGE_ABLE   BIT_3       // partner is next page able
#define NWAY_EX_PARALLEL_DET_FLT    BIT_4       // parallel detection fault
#define NWAY_EX_RESERVED            BIT_5_15    // reserved


// PHY 100 Extended Register 0 bit definitions
#define PHY_100_ER0_FDX_INDIC       BIT_0       // 1 = FDX, 0 = half duplex
#define PHY_100_ER0_SPEED_INDIC     BIT_1       // 1 = 100mbs, 0= 10mbs
#define PHY_100_ER0_WAKE_UP         BIT_2       // Wake up DAC
#define PHY_100_ER0_RESERVED        BIT_3_4     // Reserved
#define PHY_100_ER0_REV_CNTRL       BIT_5_7     // Revsion control (A step = 000)
#define PHY_100_ER0_FORCE_FAIL      BIT_8       // Force Fail is enabled
#define PHY_100_ER0_TEST            BIT_9_13    // Revsion control (A step = 000)
#define PHY_100_ER0_LINKDIS         BIT_14      // Link integrity test is disabled
#define PHY_100_ER0_JABDIS          BIT_15      // Jabber function is disabled


// PHY 100 Extended Register 1 bit definitions
#define PHY_100_ER1_RESERVED        BIT_0_8     // Reserved
#define PHY_100_ER1_CH2_DET_ERR     BIT_9       // Channel 2 EOF detection error
#define PHY_100_ER1_MANCH_CODE_ERR  BIT_10      // Manchester code error
#define PHY_100_ER1_EOP_ERR         BIT_11      // EOP error
#define PHY_100_ER1_BAD_CODE_ERR    BIT_12      // bad code error
#define PHY_100_ER1_INV_CODE_ERR    BIT_13      // invalid code error
#define PHY_100_ER1_DC_BAL_ERR      BIT_14      // DC balance error
#define PHY_100_ER1_PAIR_SKEW_ERR   BIT_15      // Pair skew error

// National Semiconductor TX phy congestion control register bit definitions
#define NSC_TX_CONG_TXREADY         BIT_10      // Makes TxReady an input
#define NSC_TX_CONG_ENABLE          BIT_8       // Enables congestion control
#define NSC_TX_CONG_F_CONNECT       BIT_5       // Enables congestion control

// National Semiconductor TX phy speed indication register bit definitions
#define NSC_TX_SPD_INDC_SPEED       BIT_6       // 0 = 100mb, 1=10mb

//-------------------------------------------------------------------------
// Phy related constants
//-------------------------------------------------------------------------
#define PHY_503                 0
#define PHY_100_A               0x000003E0
#define PHY_100_C               0x035002A8
#define PHY_TX_ID               0x015002A8
#define PHY_NSC_TX              0x5c002000
#define PHY_OTHER               0xFFFF

#define PHY_MODEL_REV_ID_MASK   0xFFF0FFFF
#define PARALLEL_DETECT         0
#define N_WAY                   1

#define RENEGOTIATE_TIME        35 // (3.5 Seconds)

#define CONNECTOR_AUTO          0
#define CONNECTOR_TPE           1
#define CONNECTOR_MII           2

//-------------------------------------------------------------------------

/* The Speedo3 Rx and Tx frame/buffer descriptors. */
#pragma pack(1)
struct CB_Header {      /* A generic descriptor. */
  UINT16 status;    /* Offset 0. */
  UINT16 command;    /* Offset 2. */
  UINT32 link;          /* struct descriptor *  */
};

/* transmit command block structure */
#pragma pack(1)
typedef struct s_TxCB {
  struct CB_Header cb_header;
  UINT32 PhysTBDArrayAddres;  /* address of an array that contains
                physical TBD pointers */
  UINT16 ByteCount;  /* immediate data count = 0 always */
  UINT8 Threshold;
  UINT8 TBDCount;
  UINT8 ImmediateData[TX_BUFFER_SIZE];
  /* following fields are not seen by the 82557 */
  struct TBD {
    UINT32 phys_buf_addr;
    UINT32 buf_len;
    } TBDArray[MAX_XMIT_FRAGMENTS];
  UINT32 PhysArrayAddr;  /* in case the one in the header is lost */
  UINT32 PhysTCBAddress;    /* for this TCB */
  struct s_TxCB *NextTCBVirtualLinkPtr;
  struct s_TxCB *PrevTCBVirtualLinkPtr;
  UINT64 free_data_ptr;  // to be given to the upper layer when this xmit completes1
}TxCB;

/* The Speedo3 Rx and Tx buffer descriptors. */
#pragma pack(1)
typedef struct s_RxFD {          /* Receive frame descriptor. */
  struct CB_Header cb_header;
  UINT32 rx_buf_addr;      /* VOID * */
  UINT16 ActualCount;
  UINT16 RFDSize;
  UINT8 RFDBuffer[RX_BUFFER_SIZE];
  UINT8 forwarded;
  UINT8 junk[3];
}RxFD;

/* Elements of the RxFD.status word. */
#define RX_COMPLETE 0x8000
#define RX_FRAME_OK 0x2000

/* Elements of the dump_statistics block. This block must be lword aligned. */
#pragma pack(1)
struct speedo_stats {
  UINT32 tx_good_frames;
  UINT32 tx_coll16_errs;
  UINT32 tx_late_colls;
  UINT32 tx_underruns;
  UINT32 tx_lost_carrier;
  UINT32 tx_deferred;
  UINT32 tx_one_colls;
  UINT32 tx_multi_colls;
  UINT32 tx_total_colls;
  UINT32 rx_good_frames;
  UINT32 rx_crc_errs;
  UINT32 rx_align_errs;
  UINT32 rx_resource_errs;
  UINT32 rx_overrun_errs;
  UINT32 rx_colls_errs;
  UINT32 rx_runt_errs;
  UINT32 done_marker;
};
#pragma pack()


struct Krn_Mem{
  RxFD rx_ring[RX_BUFFER_COUNT];
  TxCB tx_ring[TX_BUFFER_COUNT];
  struct speedo_stats statistics;
};
#define MEMORY_NEEDED  sizeof(struct Krn_Mem)

/* The parameters for a CmdConfigure operation.
   There are so many options that it would be difficult to document each bit.
   We mostly use the default or recommended settings.
*/

/*
 *--------------------------------------------------------------------------
 * Configuration CB Parameter Bit Definitions
 *--------------------------------------------------------------------------
 */
// - Byte 0  (Default Value = 16h)
#define CFIG_BYTE_COUNT    0x16       // 22 Configuration Bytes

//- Byte 1  (Default Value = 88h)
#define CFIG_TXRX_FIFO_LIMIT  0x88

//- Byte 2  (Default Value = 0)
#define CFIG_ADAPTIVE_IFS    0

//- Byte 3  (Default Value = 0, ALWAYS. This byte is RESERVED)
#define CFIG_RESERVED        0

//- Byte 4  (Default Value = 0. Default implies that Rx DMA cannot be
//-          preempted).
#define CFIG_RXDMA_BYTE_COUNT      0

//- Byte 5  (Default Value = 80h. Default implies that Tx DMA cannot be
//-          preempted. However, setting these counters is enabled.)
#define CFIG_DMBC_ENABLE            0x80

//- Byte 6  (Default Value = 33h. Late SCB enabled, No TNO interrupts,
//-          CNA interrupts and do not save bad frames.)
#define CFIG_LATE_SCB               1  // BIT 0
#define CFIG_TNO_INTERRUPT          0x4  // BIT 2
#define CFIG_CI_INTERRUPT           0x8  // BIT 3
#define CFIG_SAVE_BAD_FRAMES        0x80  // BIT_7

//- Byte 7  (Default Value = 7h. Discard short frames automatically and
//-          attempt upto 3 retries on transmit.)
#define CFIG_DISCARD_SHORTRX         0x00001
#define CFIG_URUN_RETRY              BIT_1 OR BIT_2

//- Byte 8  (Default Value = 1. Enable MII mode.)
#define CFIG_503_MII              BIT_0

//- Byte 9  (Default Value = 0, ALWAYS)

//- Byte 10 (Default Value = 2Eh)
#define CFIG_NSAI                   BIT_3
#define CFIG_PREAMBLE_LENGTH         BIT_5      ;- Bit 5-4  = 1-0
#define CFIG_NO_LOOPBACK             0
#define CFIG_INTERNAL_LOOPBACK       BIT_6
#define CFIG_EXT_LOOPBACK            BIT_7
#define CFIG_EXT_PIN_LOOPBACK        BIT_6 OR BIT_7

//- Byte 11 (Default Value = 0)
#define CFIG_LINEAR_PRIORITY         0

//- Byte 12 (Default Value = 60h)
#define CFIG_LPRIORITY_MODE          0
#define CFIG_IFS                     6          ;- 6 * 16 = 96

//- Byte 13 (Default Value = 0, ALWAYS)

//- Byte 14 (Default Value = 0F2h, ALWAYS)

//- Byte 15 (Default Value = E8h)
#define CFIG_PROMISCUOUS_MODE        BIT_0
#define CFIG_BROADCAST_DISABLE       BIT_1
#define CFIG_CRS_CDT                 BIT_7

//- Byte 16 (Default Value = 0, ALWAYS)

//- Byte 17 (Default Value = 40h, ALWAYS)

//- Byte 18 (Default Value = F2h)
#define CFIG_STRIPPING               BIT_0
#define CFIG_PADDING                 BIT_1
#define CFIG_RX_CRC_TRANSFER         BIT_2

//- Byte 19 (Default Value = 80h)
#define CFIG_FORCE_FDX               BIT_6
#define CFIG_FDX_PIN_ENABLE          BIT_7

//- Byte 20 (Default Value = 3Fh)
#define CFIG_MULTI_IA                BIT_6

//- Byte 21 (Default Value = 05)
#define CFIG_MC_ALL                  BIT_3

/*-----------------------------------------------------------------------*/
#define D102_REVID 0x0b

#define HALF_DUPLEX 1
#define FULL_DUPLEX 2

typedef struct s_data_instance {

  UINT16 State;  // stopped, started or initialized
  UINT16 Bus;
  UINT8 Device;
  UINT8 Function;
  UINT16 VendorID;
  UINT16 DeviceID;
  UINT16 RevID;
  UINT16 SubVendorID;
  UINT16 SubSystemID;

  UINT8 PermNodeAddress[PXE_MAC_LENGTH];
  UINT8 CurrentNodeAddress[PXE_MAC_LENGTH];
  UINT8 BroadcastNodeAddress[PXE_MAC_LENGTH];
  UINT32 Config[MAX_PCI_CONFIG_LEN];
  UINT32 NVData[MAX_EEPROM_LEN];

  UINT32 ioaddr;
  UINT32 flash_addr;

  UINT16 LinkSpeed;     // actual link speed setting
  UINT16 LinkSpeedReq;  // requested (forced) link speed
  UINT8  DuplexReq;     // requested duplex
  UINT8  Duplex;        // Duplex set
  UINT8  CableDetect;   // 1 to detect and 0 not to detect the cable
  UINT8  LoopBack;

  UINT16 TxBufCnt;
  UINT16 TxBufSize;
  UINT16 RxBufCnt;
  UINT16 RxBufSize;
  UINT32 RxTotals;
  UINT32 TxTotals;

  UINT16 int_mask;
  UINT16 Int_Status;
  UINT16 PhyRecord[2];  // primary and secondary PHY record registers from eeprom
  UINT8  PhyAddress;
  UINT8  int_num;
  UINT16 NVData_Len;
  UINT32 MemoryLength;

  RxFD *rx_ring;  // array of rx buffers
  TxCB *tx_ring;  // array of tx buffers
  struct speedo_stats *statistics;
  TxCB *FreeTxHeadPtr;
  TxCB *FreeTxTailPtr;
  RxFD *RFDTailPtr;

  UINT64 rx_phy_addr;  // physical addresses
  UINT64 tx_phy_addr;
  UINT64 stat_phy_addr;
  UINT64 MemoryPtr;
  UINT64 Mapped_MemoryPtr;

  UINT64 xmit_done[TX_BUFFER_COUNT << 1]; // circular buffer
  UINT16 xmit_done_head;  // index into the xmit_done array
  UINT16 xmit_done_tail;  // where are we filling now (index into xmit_done)
  UINT16 cur_rx_ind;  // current RX Q head index
  UINT16 FreeCBCount;

  BOOLEAN in_interrupt;
  BOOLEAN in_transmit;
  BOOLEAN Receive_Started;
  UINT8 Rx_Filter;
  UINT8 VersionFlag;  // UNDI30 or UNDI31??
  UINT8 rsvd[3];

  struct mc{
    UINT16 reserved [3]; // padding for this structure to make it 8 byte aligned
    UINT16 list_len;
    UINT8 mc_list[MAX_MCAST_ADDRESS_CNT][PXE_MAC_LENGTH]; // 8*32 is the size
  } mcast_list;

  UINT64 Unique_ID;

  EFI_PCI_IO_PROTOCOL   *Io_Function;
  //
  // Original PCI attributes
  //
  UINT64                OriginalPciAttributes;

  VOID (*Delay_30)(UINTN);  // call back routine
  VOID (*Virt2Phys_30)(UINT64 virtual_addr, UINT64 physical_ptr);  // call back routine
  VOID (*Block_30)(UINT32 enable);  // call back routine
  VOID (*Mem_Io_30)(UINT8 read_write, UINT8 len, UINT64 port, UINT64 buf_addr);
  VOID (*Delay)(UINT64, UINTN);  // call back routine
  VOID (*Virt2Phys)(UINT64 unq_id, UINT64 virtual_addr, UINT64 physical_ptr);  // call back routine
  VOID (*Block)(UINT64 unq_id, UINT32 enable);  // call back routine
  VOID (*Mem_Io)(UINT64 unq_id, UINT8 read_write, UINT8 len, UINT64 port,
          UINT64 buf_addr);
  VOID (*Map_Mem)(UINT64 unq_id, UINT64 virtual_addr, UINT32 size,
                   UINT32 Direction, UINT64 mapped_addr);
  VOID (*UnMap_Mem)(UINT64 unq_id, UINT64 virtual_addr, UINT32 size,
            UINT32 Direction, UINT64 mapped_addr);
  VOID (*Sync_Mem)(UINT64 unq_id, UINT64 virtual_addr,
            UINT32 size, UINT32 Direction, UINT64 mapped_addr);
} NIC_DATA_INSTANCE;

#pragma pack(1)
struct MC_CB_STRUCT{
  UINT16 count;
  UINT8 m_list[MAX_MCAST_ADDRESS_CNT][ETHER_MAC_ADDR_LEN];
};
#pragma pack()

#define FOUR_GIGABYTE (UINT64)0x100000000ULL

#endif


/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


--*/


/*++

Module Name:

  MMC.h

Abstract:

  Header file for Industry MMC 4.2 spec.

--*/

#ifndef _MMC_H
#define _MMC_H

#pragma pack(1)
//
//Command definition
//

#define  CMD0              0
#define  CMD1              1
#define  CMD2              2
#define  CMD3              3
#define  CMD4              4
#define  CMD6              6
#define  CMD7              7
#define  CMD8              8
#define  CMD9              9
#define  CMD10             10
#define  CMD11             11
#define  CMD12             12
#define  CMD13             13
#define  CMD14             14
#define  CMD15             15
#define  CMD16             16
#define  CMD17             17
#define  CMD18             18
#define  CMD19             19
#define  CMD20             20
#define  CMD23             23
#define  CMD24             24
#define  CMD25             25
#define  CMD26             26
#define  CMD27             27
#define  CMD28             28
#define  CMD29             29
#define  CMD30             30
#define  CMD35             35
#define  CMD36             36
#define  CMD38             38
#define  CMD39             39
#define  CMD40             40
#define  CMD42             42
#define  CMD55             55
#define  CMD56             56



#define  GO_IDLE_STATE           CMD0
#define  SEND_OP_COND            CMD1
#define  ALL_SEND_CID            CMD2
#define  SET_RELATIVE_ADDR       CMD3
#define  SET_DSR                 CMD4
#define  SWITCH                  CMD6
#define  SELECT_DESELECT_CARD    CMD7
#define  SEND_EXT_CSD            CMD8
#define  SEND_CSD                CMD9
#define  SEND_CID                CMD10
#define  READ_DAT_UNTIL_STOP     CMD11
#define  STOP_TRANSMISSION       CMD12
#define  SEND_STATUS             CMD13
#define  BUSTEST_R               CMD14
#define  GO_INACTIVE_STATE       CMD15
#define  SET_BLOCKLEN            CMD16
#define  READ_SINGLE_BLOCK       CMD17
#define  READ_MULTIPLE_BLOCK     CMD18
#define  BUSTEST_W               CMD19
#define  WRITE_DAT_UNTIL_STOP    CMD20
#define  SET_BLOCK_COUNT         CMD23
#define  WRITE_BLOCK             CMD24
#define  WRITE_MULTIPLE_BLOCK    CMD25
#define  PROGRAM_CID             CMD26
#define  PROGRAM_CSD             CMD27
#define  SET_WRITE_PROT          CMD28
#define  CLR_WRITE_PROT          CMD29
#define  SEND_WRITE_PROT         CMD30
#define  ERASE_GROUP_START       CMD35
#define  ERASE_GROUP_END         CMD36
#define  ERASE                   CMD38
#define  FAST_IO                 CMD39
#define  GO_IRQ_STATE            CMD40
#define  LOCK_UNLOCK             CMD42
#define  APP_CMD                 CMD55
#define  GEN_CMD                 CMD56

#define B_PERM_WP_DIS            0x10
#define B_PWR_WP_EN              0x01
#define US_PERM_WP_DIS           0x10
#define US_PWR_WP_EN             0x01

#define FREQUENCY_OD            (400 * 1000)
#define FREQUENCY_MMC_PP        (26 * 1000 * 1000)
#define FREQUENCY_MMC_PP_HIGH   (52 * 1000 * 1000)

#define DEFAULT_DSR_VALUE        0x404

//
//Registers definition
//

typedef struct {
  UINT32  Reserved0:   7;  // 0
  UINT32  V170_V195:   1;  // 1.70V - 1.95V
  UINT32  V200_V260:   7;  // 2.00V - 2.60V
  UINT32  V270_V360:   9;  // 2.70V - 3.60V
  UINT32  Reserved1:   5;  // 0
  UINT32  AccessMode:  2;  // 00b (byte mode), 10b (sector mode)
  UINT32  Busy:        1;  // This bit is set to LOW if the card has not finished the power up routine
} OCR;


typedef struct {
  UINT8   NotUsed:     1; //  1
  UINT8   CRC:         7; //  CRC7 checksum
  UINT8   MDT;            //  Manufacturing date
  UINT32  PSN;            //  Product serial number
  UINT8   PRV;            //  Product revision
  UINT8   PNM[6];         //  Product name
  UINT16  OID;            //  OEM/Application ID
  UINT8   MID;            //  Manufacturer ID
} CID;


typedef struct {
  UINT8   NotUsed:            1; //  1 [0:0]
  UINT8   CRC:                7; //  CRC [7:1]
  UINT8   ECC:                2; //  ECC code [9:8]
  UINT8   FILE_FORMAT:        2; //  File format [11:10]
  UINT8   TMP_WRITE_PROTECT:  1; //  Temporary write protection [12:12]
  UINT8   PERM_WRITE_PROTECT: 1; //  Permanent write protection [13:13]
  UINT8   COPY:               1; //  Copy flag (OTP) [14:14]
  UINT8   FILE_FORMAT_GRP:    1; //  File format group [15:15]
  UINT16  CONTENT_PROT_APP:   1; //  Content protection application [16:16]
  UINT16  Reserved0:          4; //  0 [20:17]
  UINT16  WRITE_BL_PARTIAL:   1; //  Partial blocks for write allowed [21:21]
  UINT16  WRITE_BL_LEN:       4; //  Max. write data block length [25:22]
  UINT16  R2W_FACTOR:         3; //  Write speed factor [28:26]
  UINT16  DEFAULT_ECC:        2; //  Manufacturer default ECC [30:29]
  UINT16  WP_GRP_ENABLE:      1; //  Write protect group enable [31:31]
  UINT32  WP_GRP_SIZE:        5; //  Write protect group size [36:32]
  UINT32  ERASE_GRP_MULT:     5; //  Erase group size multiplier [41:37]
  UINT32  ERASE_GRP_SIZE:     5; //  Erase group size [46:42]
  UINT32  C_SIZE_MULT:        3; //  Device size multiplier [49:47]
  UINT32  VDD_W_CURR_MAX:     3; //  Max. write current @ VDD max [52:50]
  UINT32  VDD_W_CURR_MIN:     3; //  Max. write current @ VDD min [55:53]
  UINT32  VDD_R_CURR_MAX:     3; //  Max. read current @ VDD max [58:56]
  UINT32  VDD_R_CURR_MIN:     3; //  Max. read current @ VDD min [61:59]
  UINT32  C_SIZELow2:         2;//  Device size [73:62]
  UINT32  C_SIZEHigh10:       10;//  Device size [73:62]
  UINT32  Reserved1:          2; //  0 [75:74]
  UINT32  DSR_IMP:            1; //  DSR implemented [76:76]
  UINT32  READ_BLK_MISALIGN:  1; //  Read block misalignment [77:77]
  UINT32  WRITE_BLK_MISALIGN: 1; //  Write block misalignment [78:78]
  UINT32  READ_BL_PARTIAL:    1; //  Partial blocks for read allowed [79:79]
  UINT32  READ_BL_LEN:        4; //  Max. read data block length [83:80]
  UINT32  CCC:                12;//  Card command classes [95:84]
  UINT8   TRAN_SPEED          ; //  Max. bus clock frequency [103:96]
  UINT8   NSAC                ; //  Data read access-time 2 in CLK cycles (NSAC*100) [111:104]
  UINT8   TAAC                ; //  Data read access-time 1 [119:112]
  UINT8   Reserved2:          2; //  0 [121:120]
  UINT8   SPEC_VERS:          4; //  System specification version [125:122]
  UINT8   CSD_STRUCTURE:      2; //  CSD structure [127:126]
} CSD;

typedef struct {
  UINT8  Reserved133_0[134];     // [133:0] 0
  UINT8  SEC_BAD_BLOCK_MGMNT;    // [134] Bad Block Management mode
  UINT8  Reserved135;            // [135] 0
  UINT8  ENH_START_ADDR[4];      // [139:136] Enhanced User Data Start Address
  UINT8  ENH_SIZE_MULT[3];       // [142:140] Enhanced User Data Start Size
  UINT8  GP_SIZE_MULT_1[3];      // [145:143] GPP1 Size
  UINT8  GP_SIZE_MULT_2[3];      // [148:146] GPP2 Size
  UINT8  GP_SIZE_MULT_3[3];      // [151:149] GPP3 Size
  UINT8  GP_SIZE_MULT_4[3];      // [154:152] GPP4 Size
  UINT8  PARTITION_SETTING_COMPLETED; // [155] Partitioning Setting
  UINT8  PARTITIONS_ATTRIBUTES;  // [156] Partitions attributes
  UINT8  MAX_ENH_SIZE_MULT[3];   // [159:157] GPP4 Start Size
  UINT8  PARTITIONING_SUPPORT;   // [160] Partitioning Support
  UINT8  HPI_MGMT;               // [161] HPI management
  UINT8  RST_n_FUNCTION;         // [162] H/W reset function
  UINT8  BKOPS_EN;               // [163] Enable background operations handshake
  UINT8  BKOPS_START;            // [164] Manually start background operations
  UINT8  Reserved165;            // [165] 0
  UINT8  WR_REL_PARAM;           // [166] Write reliability parameter register
  UINT8  WR_REL_SET;             // [167] Write reliability setting register
  UINT8  RPMB_SIZE_MULT;         // [168] RPMB Size
  UINT8  FW_CONFIG;              // [169] FW configuration
  UINT8  Reserved170;            // [170] 0
  UINT8  USER_WP;                // [171] User area write protection
  UINT8  Reserved172;            // [172] 0
  UINT8  BOOT_WP;                // [173] Boot area write protection
  UINT8  Reserved174;            // [174] 0
  UINT8  ERASE_GROUP_DEF;        // [175] High density erase group definition
  UINT8  Reserved176;            // [176] 0
  UINT8  BOOT_BUS_WIDTH;         // [177] Boot bus width
  UINT8  BOOT_CONFIG_PROT;       // [178] Boot config protection
  UINT8  PARTITION_CONFIG;       // [179] Partition config
  UINT8  Reserved180;            // [180] 0
  UINT8  ERASED_MEM_CONT;        // [181] Erased Memory Content
  UINT8  Reserved182;            // [182] 0
  UINT8  BUS_WIDTH;              // [183] Bus Width Mode
  UINT8  Reserved184;            // [184] 0
  UINT8  HS_TIMING;              // [185] High Speed Interface Timing
  UINT8  Reserved186;            // [186] 0
  UINT8  POWER_CLASS;            // [187] Power Class
  UINT8  Reserved188;            // [188] 0
  UINT8  CMD_SET_REV;            // [189] Command Set Revision
  UINT8  Reserved190;            // [190] 0
  UINT8  CMD_SET;                // [191] Command Set
  UINT8  EXT_CSD_REV;            // [192] Extended CSD Revision
  UINT8  Reserved193;            // [193] 0
  UINT8  CSD_STRUCTURE;          // [194] CSD Structure Version
  UINT8  Reserved195;            // [195] 0
  UINT8  CARD_TYPE;              // [196] Card Type
  UINT8  Reserved197;            // [197] 0
  UINT8  OUT_OF_INTERRUPT_TIME;  // [198] Out-of-interrupt busy timing
  UINT8  PARTITION_SWITCH_TIME;  // [199] Partition switching timing
  UINT8  PWR_CL_52_195;          // [200] Power Class for 52MHz @ 1.95V
  UINT8  PWR_CL_26_195;          // [201] Power Class for 26MHz @ 1.95V
  UINT8  PWR_CL_52_360;          // [202] Power Class for 52MHz @ 3.6V
  UINT8  PWR_CL_26_360;          // [203] Power Class for 26MHz @ 3.6V
  UINT8  Reserved204;            // [204] 0
  UINT8  MIN_PERF_R_4_26;        // [205] Minimum Read Performance for 4bit @26MHz
  UINT8  MIN_PERF_W_4_26;        // [206] Minimum Write Performance for 4bit @26MHz
  UINT8  MIN_PERF_R_8_26_4_52;   // [207] Minimum Read Performance for 8bit @26MHz/4bit @52MHz
  UINT8  MIN_PERF_W_8_26_4_52;   // [208] Minimum Write Performance for 8bit @26MHz/4bit @52MHz
  UINT8  MIN_PERF_R_8_52;        // [209] Minimum Read Performance for 8bit @52MHz
  UINT8  MIN_PERF_W_8_52;        // [210] Minimum Write Performance for 8bit @52MHz
  UINT8  Reserved211;            // [211] 0
  UINT8  SEC_COUNT[4];           // [215:212] Sector Count
  UINT8  Reserved216;            // [216] 0
  UINT8  S_A_TIMEOUT;            // [217] Sleep/awake timeout
  UINT8  Reserved218;            // [218] 0
  UINT8  S_C_VCCQ;               // [219] Sleep current (VCCQ)
  UINT8  S_C_VCC;                // [220] Sleep current (VCC)
  UINT8  HC_WP_GRP_SIZE;         // [221] High-capacity write protect group size
  UINT8  REL_WR_SEC_C;           // [222] Reliable write sector count
  UINT8  ERASE_TIMEOUT_MULT;     // [223] High-capacity erase timeout
  UINT8  HC_ERASE_GRP_SIZE;      // [224] High-capacity erase unit size
  UINT8  ACC_SIZE;               // [225] Access size
  UINT8  BOOT_SIZE_MULTI;        // [226] Boot partition size
  UINT8  Reserved227;            // [227] 0
  UINT8  BOOT_INFO;              // [228] Boot information
  UINT8  SEC_TRIM_MULT;          // [229] Secure TRIM Multiplier
  UINT8  SEC_ERASE_MULT;         // [230] Secure Erase Multiplier
  UINT8  SEC_FEATURE_SUPPORT;    // [231] Secure Feature support
  UINT8  TRIM_MULT;              // [232] TRIM Multiplier
  UINT8  Reserved233;            // [233] 0
  UINT8  MIN_PERF_DDR_R_8_52;    // [234] Min Read Performance for 8-bit @ 52MHz
  UINT8  MIN_PERF_DDR_W_8_52;    // [235] Min Write Performance for 8-bit @ 52MHz
  UINT8  Reserved237_236[2];     // [237:236] 0
  UINT8  PWR_CL_DDR_52_195;      // [238] Power class for 52MHz, DDR at 1.95V
  UINT8  PWR_CL_DDR_52_360;      // [239] Power class for 52MHz, DDR at 3.6V
  UINT8  Reserved240;            // [240] 0
  UINT8  INI_TIMEOUT_AP;         // [241] 1st initialization time after partitioning
  UINT8  CORRECTLY_PRG_SECTORS_NUM[4]; // [245:242] Number of correctly programmed sectors
  UINT8  BKOPS_STATUS;           // [246] Background operations status
  UINT8  Reserved501_247[255];   // [501:247] 0
  UINT8  BKOPS_SUPPORT;          // [502] Background operations support
  UINT8  HPI_FEATURES;           // [503] HPI features
  UINT8  S_CMD_SET;              // [504] Sector Count
  UINT8  Reserved511_505[7];     // [511:505] Sector Count
} EXT_CSD;


//
//Card Status definition
//
typedef struct {
  UINT32  Reserved0:           2; //Reserved for Manufacturer Test Mode
  UINT32  Reserved1:           2; //Reserved for Application Specific commands
  UINT32  Reserved2:           1; //
  UINT32  SAPP_CMD:            1; //
  UINT32  Reserved3:           1; //Reserved
  UINT32  SWITCH_ERROR:        1; //
  UINT32  READY_FOR_DATA:      1; //
  UINT32  CURRENT_STATE:       4; //
  UINT32  ERASE_RESET:         1; //
  UINT32  Reserved4:           1; //Reserved
  UINT32  WP_ERASE_SKIP:       1; //
  UINT32  CID_CSD_OVERWRITE:   1; //
  UINT32  OVERRUN:             1; //
  UINT32  UNDERRUN:            1; //
  UINT32  ERROR:               1; //
  UINT32  CC_ERROR:            1; //
  UINT32  CARD_ECC_FAILED:     1; //
  UINT32  ILLEGAL_COMMAND:     1; //
  UINT32  COM_CRC_ERROR:       1; //
  UINT32  LOCK_UNLOCK_FAILED:  1; //
  UINT32  CARD_IS_LOCKED:      1; //
  UINT32  WP_VIOLATION:        1; //
  UINT32  ERASE_PARAM:         1; //
  UINT32  ERASE_SEQ_ERROR:     1; //
  UINT32  BLOCK_LEN_ERROR:     1; //
  UINT32  ADDRESS_MISALIGN:    1; //
  UINT32  ADDRESS_OUT_OF_RANGE:1; //
} CARD_STATUS;

typedef struct {
  UINT32  CmdSet:              3;
  UINT32  Reserved0:           5;
  UINT32  Value:               8;
  UINT32  Index:               8;
  UINT32  Access:              2;
  UINT32  Reserved1:           6;
} SWITCH_ARGUMENT;

#define CommandSet_Mode          0
#define SetBits_Mode             1
#define ClearBits_Mode           2
#define WriteByte_Mode           3


#define  Idle_STATE              0
#define  Ready_STATE             1
#define  Ident_STATE             2
#define  Stby_STATE              3
#define  Tran_STATE              4
#define  Data_STATE              5
#define  Rcv_STATE               6
#define  Prg_STATE               7
#define  Dis_STATE               8
#define  Btst_STATE              9



#pragma pack()
#endif

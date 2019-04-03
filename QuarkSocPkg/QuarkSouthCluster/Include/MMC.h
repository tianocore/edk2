/** @file

Header file for Industry MMC 4.2 spec.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

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


#define CMD_INDEX_MASK           0x3F
#define AUTO_CMD12_ENABLE        BIT6
#define AUTO_CMD23_ENABLE        BIT7

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
}OCR;


typedef struct {
  UINT8   NotUsed:     1; //  1
  UINT8   CRC:         7; //  CRC7 checksum
  UINT8   MDT;            //  Manufacturing date
  UINT32  PSN;            //  Product serial number
  UINT8   PRV;            //  Product revision
  UINT8   PNM[6];         //  Product name
  UINT16  OID;            //  OEM/Application ID
  UINT8   MID;            //  Manufacturer ID
}CID;


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
}CSD;

typedef struct {
  UINT8  Reserved0[181];         //  0 [0:180]
  UINT8  ERASED_MEM_CONT;        //  Erased Memory Content [181]
  UINT8  Reserved2;              //  Erased Memory Content [182]
  UINT8  BUS_WIDTH;              //  Bus Width Mode [183]
  UINT8  Reserved3;              //  0 [184]
  UINT8  HS_TIMING;              //  High Speed Interface Timing [185]
  UINT8  Reserved4;              //  0 [186]
  UINT8  POWER_CLASS;            //  Power Class [187]
  UINT8  Reserved5;              //  0 [188]
  UINT8  CMD_SET_REV;            //  Command Set Revision [189]
  UINT8  Reserved6;              //  0 [190]
  UINT8  CMD_SET;                //  Command Set [191]
  UINT8  EXT_CSD_REV;            //  Extended CSD Revision [192]
  UINT8  Reserved7;              //  0 [193]
  UINT8  CSD_STRUCTURE;          //  CSD Structure Version [194]
  UINT8  Reserved8;              //  0 [195]
  UINT8  CARD_TYPE;              //  Card Type [196]
  UINT8  Reserved9[3];           //  0 [199:197]
  UINT8  PWR_CL_52_195;          //  Power Class for 52MHz @ 1.95V [200]
  UINT8  PWR_CL_26_195;          //  Power Class for 26MHz @ 1.95V [201]
  UINT8  PWR_CL_52_360;          //  Power Class for 52MHz @ 3.6V [202]
  UINT8  PWR_CL_26_360;          //  Power Class for 26MHz @ 3.6V [203]
  UINT8  Reserved10;             //  0 [204]
  UINT8  MIN_PERF_R_4_26;        //  Minimum Read Performance for 4bit @26MHz [205]
  UINT8  MIN_PERF_W_4_26;        //  Minimum Write Performance for 4bit @26MHz [206]
  UINT8  MIN_PERF_R_8_26_4_52;   //  Minimum Read Performance for 8bit @26MHz/4bit @52MHz [207]
  UINT8  MIN_PERF_W_8_26_4_52;   //  Minimum Write Performance for 8bit @26MHz/4bit @52MHz [208]
  UINT8  MIN_PERF_R_8_52;        //  Minimum Read Performance for 8bit @52MHz [209]
  UINT8  MIN_PERF_W_8_52;        //  Minimum Write Performance for 8bit @52MHz [210]
  UINT8  Reserved11;             //  0 [211]
  UINT8  SEC_COUNT[4];           //  Sector Count [215:212]
  UINT8  Reserved12[288];        //  0 [503:216]
  UINT8  S_CMD_SET;              //  Sector Count [504]
  UINT8  Reserved13[7];          //  Sector Count [511:505]
}EXT_CSD;


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
}CARD_STATUS;

typedef struct {
  UINT32  CmdSet:              3;
  UINT32  Reserved0:           5;
  UINT32  Value:               8;
  UINT32  Index:               8;
  UINT32  Access:              2;
  UINT32  Reserved1:           6;
}SWITCH_ARGUMENT;

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

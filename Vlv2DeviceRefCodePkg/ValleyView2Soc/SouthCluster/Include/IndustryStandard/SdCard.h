/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent


--*/


/*++
Module Name:

  SDCard.h

Abstract:

  Header file for Industry SD Card 2.0 spec.

--*/

#ifndef _SD_CARD_H
#define _SD_CARD_H

#include "Mmc.h"

#pragma pack(1)

#define CHECK_PATTERN     0xAA

#define ACMD6             6
#define ACMD13            13
#define ACMD23            23
#define ACMD41            41
#define ACMD42            42
#define ACMD51            51


#define SWITCH_FUNC              CMD6
#define SEND_IF_COND             CMD8


#define SET_BUS_WIDTH            ACMD6
#define SD_STATUS                ACMD13
#define SET_WR_BLK_ERASE_COUNT   ACMD23
#define SD_SEND_OP_COND          ACMD41
#define SET_CLR_CARD_DETECT      ACMD42
#define SEND_SCR                 ACMD51



#define SD_BUS_WIDTH_1              0
#define SD_BUS_WIDTH_4              2



#define FREQUENCY_SD_PP        (25 * 1000 * 1000)
#define FREQUENCY_SD_PP_HIGH   (50 * 1000 * 1000)


#define SD_SPEC_10                  0
#define SD_SPEC_11                  1
#define SD_SPEC_20                  2


#define VOLTAGE_27_36               0x1

typedef struct {
  UINT8   NotUsed:            1; //  1 [0:0]
  UINT8   CRC:                7; //  CRC [7:1]
  UINT8   ECC:                2; //  ECC code [9:8]
  UINT8   FILE_FORMAT:        2; //  File format [11:10]
  UINT8   TMP_WRITE_PROTECT:  1; //  Temporary write protection [12:12]
  UINT8   PERM_WRITE_PROTECT: 1; //  Permanent write protection [13:13]
  UINT8   COPY:               1; //  Copy flag (OTP) [14:14]
  UINT8   FILE_FORMAT_GRP:    1; //  File format group [15:15]
  UINT16  Reserved0:          5; //  0 [20:16]
  UINT16  WRITE_BL_PARTIAL:   1; //  Partial blocks for write allowed [21:21]
  UINT16  WRITE_BL_LEN:       4; //  Max. write data block length [25:22]
  UINT16  R2W_FACTOR:         3; //  Write speed factor [28:26]
  UINT16  DEFAULT_ECC:        2; //  Manufacturer default ECC [30:29]
  UINT16  WP_GRP_ENABLE:      1; //  Write protect group enable [31:31]
  UINT16  WP_GRP_SIZE:        7; //  Write protect group size [38:32]
  UINT16  SECTOR_SIZE:        7; //  Erase sector size [45:39]
  UINT16  ERASE_BLK_EN:       1; //  Erase single block enable [46:46]
  UINT16  Reserved1:          1; //  0 [47:47]

  UINT32  C_SIZE:             22; //  Device size [69:48]
  UINT32  Reserved2:          6;  //  0 [75:70]
  UINT32  DSR_IMP:            1;  //  DSR implemented [76:76]
  UINT32  READ_BLK_MISALIGN:  1;  //  Read block misalignment [77:77]
  UINT32  WRITE_BLK_MISALIGN: 1;  //  Write block misalignment [78:78]
  UINT32  READ_BL_PARTIAL:    1;  //  Partial blocks for read allowed [79:79]

  UINT16  READ_BL_LEN:        4;  //  Max. read data block length [83:80]
  UINT16  CCC:                12; //  Card command classes [95:84]
  UINT8   TRAN_SPEED          ;   //  Max. bus clock frequency [103:96]
  UINT8   NSAC                ;   //  Data read access-time 2 in CLK cycles (NSAC*100) [111:104]
  UINT8   TAAC                ;   //  Data read access-time 1 [119:112]
  UINT8   Reserved3:          6;  //  0 [125:120]
  UINT8   CSD_STRUCTURE:      2;  //  CSD structure [127:126]
} CSD_SDV2;

typedef struct {
  UINT32  Reserved0;
  UINT32  Reserved1:               16;
  UINT32  SD_BUS_WIDTH:            4;
  UINT32  SD_SECURITY:             3;
  UINT32  DATA_STAT_AFTER_ERASE:   1;
  UINT32  SD_SPEC:                 4;
  UINT32  SCR_STRUCT:              4;
} SCR;


typedef struct {
  UINT8   Reserved0[50];
  UINT8   ERASE_OFFSET:               2;
  UINT8   ERASE_TIMEOUT:              6;
  UINT16  ERASE_SIZE;
  UINT8   Reserved1:                  4;
  UINT8   AU_SIZE:                    4;
  UINT8   PERFORMANCE_MOVE;
  UINT8   SPEED_CLASS;
  UINT32  SIZE_OF_PROTECTED_AREA;
  UINT32  SD_CARD_TYPE:              16;
  UINT32  Reserved2:                 13;
  UINT32  SECURED_MODE:               1;
  UINT32  DAT_BUS_WIDTH:              2;
} SD_STATUS_REG;



typedef struct {
  UINT8   Reserved0[34];
  UINT16  Group1BusyStatus;
  UINT16  Group2BusyStatus;
  UINT16  Group3BusyStatus;
  UINT16  Group4BusyStatus;
  UINT16  Group5BusyStatus;
  UINT16  Group6BusyStatus;
  UINT8   DataStructureVersion;
  UINT8   Group21Status;
  UINT8   Group43Status;
  UINT8   Group65Status;
  UINT16  Group1Function;
  UINT16  Group2Function;
  UINT16  Group3Function;
  UINT16  Group4Function;
  UINT16  Group5Function;
  UINT16  Group6Function;
  UINT16  MaxCurrent;
} SWITCH_STATUS;


#pragma pack()
#endif


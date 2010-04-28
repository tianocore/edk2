/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    Atapi.h
    
Abstract: 
    

Revision History
--*/

#ifndef _ATAPI_H
#define _ATAPI_H

#include "Tiano.h"

#pragma pack(1)

typedef struct {
  UINT16  config;                     /* General Configuration */
  UINT16  cylinders;                  /* Number of Cylinders */
  UINT16  reserved_2;
  UINT16  heads;                      /* Number of logical heads */
  UINT16  vendor_data1;
  UINT16  vendoe_data2;
  UINT16  sectors_per_track;
  UINT16  vendor_specific_7_9[3];
  CHAR8   SerialNo[20];               /* ASCII */
  UINT16  vendor_specific_20_21[2];
  UINT16  ecc_bytes_available;
  CHAR8   FirmwareVer[8];             /* ASCII */
  CHAR8   ModelName[40];              /* ASCII */
  UINT16  multi_sector_cmd_max_sct_cnt;
  UINT16  reserved_48;
  UINT16  capabilities;
  UINT16  reserved_50;
  UINT16  pio_cycle_timing;
  UINT16  reserved_52;
  UINT16  field_validity;
  UINT16  current_cylinders;
  UINT16  current_heads;
  UINT16  current_sectors;
  UINT16  CurrentCapacityLsb;
  UINT16  CurrentCapacityMsb;
  UINT16  reserved_59;
  UINT16  user_addressable_sectors_lo;
  UINT16  user_addressable_sectors_hi;
  UINT16  reserved_62;
  UINT16  multi_word_dma_mode;
  UINT16  advanced_pio_modes;
  UINT16  min_multi_word_dma_cycle_time;
  UINT16  rec_multi_word_dma_cycle_time;
  UINT16  min_pio_cycle_time_without_flow_control;
  UINT16  min_pio_cycle_time_with_flow_control;
  UINT16  reserved_69_79[11];
  UINT16  major_version_no;
  UINT16  minor_version_no;
  UINT16  reserved_82_127[46];
  UINT16  security_status;
  UINT16  vendor_data_129_159[31];
  UINT16  reserved_160_255[96];
} IDENTIFY;

typedef struct {
  UINT8 peripheral_type;
  UINT8 RMB;
  UINT8 version;
  UINT8 response_data_format;
  UINT8 addnl_length;
  UINT8 reserved_5;
  UINT8 reserved_6;
  UINT8 reserved_7;
  UINT8 vendor_info[8];
  UINT8 product_id[12];
  UINT8 eeprom_product_code[4];
  UINT8 firmware_rev_level[4];
  UINT8 firmware_sub_rev_level[1];
  UINT8 reserved_37;
  UINT8 reserved_38;
  UINT8 reserved_39;
  UINT8 max_capacity_hi;
  UINT8 max_capacity_mid;
  UINT8 max_capacity_lo;
  UINT8 reserved_43_95[95 - 43 + 1];
  UINT8 vendor_id[20];
  UINT8 eeprom_drive_sno[12];
} INQUIRY_DATA;

typedef struct {
  UINT8 error_code : 7;
  UINT8 valid : 1;
  UINT8 reserved_1;
  UINT8 sense_key : 4;
  UINT8 reserved_21 : 1;
  UINT8 ILI : 1;
  UINT8 reserved_22 : 2;
  UINT8 vendor_specific_3;
  UINT8 vendor_specific_4;
  UINT8 vendor_specific_5;
  UINT8 vendor_specific_6;
  UINT8 addnl_sense_length;           // n - 7
  UINT8 vendor_specific_8;
  UINT8 vendor_specific_9;
  UINT8 vendor_specific_10;
  UINT8 vendor_specific_11;
  UINT8 addnl_sense_code;             // mandatory
  UINT8 addnl_sense_code_qualifier;   // mandatory
  UINT8 field_replaceable_unit_code;  // optional
  UINT8 reserved_15;
  UINT8 reserved_16;
  UINT8 reserved_17;
  //
  // Followed by additional sense bytes     : FIXME
  //
} REQUEST_SENSE_DATA;

typedef struct {
  UINT8 LastLba3;
  UINT8 LastLba2;
  UINT8 LastLba1;
  UINT8 LastLba0;
  UINT8 BlockSize3;
  UINT8 BlockSize2;
  UINT8 BlockSize1;
  UINT8 BlockSize0;
} READ_CAPACITY_DATA;

typedef struct {
  UINT8 reserved_0;
  UINT8 reserved_1;
  UINT8 reserved_2;
  UINT8 Capacity_Length;
  UINT8 LastLba3;
  UINT8 LastLba2;
  UINT8 LastLba1;
  UINT8 LastLba0;
  UINT8 DesCode : 2;
  UINT8 reserved_9 : 6;
  UINT8 BlockSize2;
  UINT8 BlockSize1;
  UINT8 BlockSize0;
} READ_FORMAT_CAPACITY_DATA;

#pragma pack()
//
// ATAPI Command
//
#define ATAPI_SOFT_RESET_CMD      0x08
#define PACKET_CMD                0xA0
#define ATAPI_IDENTIFY_DEVICE_CMD 0xA1
#define ATAPI_SERVICE_CMD         0xA2

//
// ATAPI Packet Command
//
#pragma pack(1)

typedef struct {
  UINT8 opcode;
  UINT8 reserved_1;
  UINT8 reserved_2;
  UINT8 reserved_3;
  UINT8 reserved_4;
  UINT8 reserved_5;
  UINT8 reserved_6;
  UINT8 reserved_7;
  UINT8 reserved_8;
  UINT8 reserved_9;
  UINT8 reserved_10;
  UINT8 reserved_11;
} TEST_UNIT_READY_CMD;

typedef struct {
  UINT8 opcode;
  UINT8 reserved_1 : 4;
  UINT8 lun : 4;
  UINT8 page_code;
  UINT8 reserved_3;
  UINT8 allocation_length;
  UINT8 reserved_5;
  UINT8 reserved_6;
  UINT8 reserved_7;
  UINT8 reserved_8;
  UINT8 reserved_9;
  UINT8 reserved_10;
  UINT8 reserved_11;
} INQUIRY_CMD;

typedef struct {
  UINT8 opcode;
  UINT8 reserved_1 : 4;
  UINT8 lun : 4;
  UINT8 reserved_2;
  UINT8 reserved_3;
  UINT8 allocation_length;
  UINT8 reserved_5;
  UINT8 reserved_6;
  UINT8 reserved_7;
  UINT8 reserved_8;
  UINT8 reserved_9;
  UINT8 reserved_10;
  UINT8 reserved_11;
} REQUEST_SENSE_CMD;

typedef struct {
  UINT8 opcode;
  UINT8 reserved_1 : 5;
  UINT8 lun : 3;
  UINT8 Lba0;
  UINT8 Lba1;
  UINT8 Lba2;
  UINT8 Lba3;
  UINT8 reserved_6;
  UINT8 TranLen0;
  UINT8 TranLen1;
  UINT8 reserved_9;
  UINT8 reserved_10;
  UINT8 reserved_11;
} READ10_CMD;

typedef struct {
  UINT8 opcode;
  UINT8 reserved_1;
  UINT8 reserved_2;
  UINT8 reserved_3;
  UINT8 reserved_4;
  UINT8 reserved_5;
  UINT8 reserved_6;
  UINT8 allocation_length_hi;
  UINT8 allocation_length_lo;
  UINT8 reserved_9;
  UINT8 reserved_10;
  UINT8 reserved_11;
} READ_FORMAT_CAP_CMD;

typedef struct {
  UINT8 peripheral_type;
  UINT8 RMB;
  UINT8 version;
  UINT8 response_data_format;
  UINT8 addnl_length;
  UINT8 reserved_5;
  UINT8 reserved_6;
  UINT8 reserved_7;
  UINT8 vendor_info[8];
  UINT8 product_id[12];
  UINT8 eeprom_product_code[4];
  UINT8 firmware_rev_level[4];
} USB_INQUIRY_DATA;

typedef union {
  UINT16              Data16[6];
  TEST_UNIT_READY_CMD TestUnitReady;
  READ10_CMD          Read10;
  REQUEST_SENSE_CMD   RequestSence;
  INQUIRY_CMD         Inquiry;
  READ_FORMAT_CAP_CMD ReadFormatCapacity;
} ATAPI_PACKET_COMMAND;

#pragma pack()
//
// Packet Command Code
//
#define TEST_UNIT_READY       0x00
#define REQUEST_SENSE         0x03
#define INQUIRY               0x12
#define READ_FORMAT_CAPACITY  0x23
#define READ_CAPACITY         0x25
#define READ_10               0x28

#define DEFAULT_CTL           (0x0a)  // default content of device control register, disable INT
#define DEFAULT_CMD           (0xa0)

#define MAX_ATAPI_BYTE_COUNT  (0xfffe)

//
// Sense Key
//
#define REQUEST_SENSE_ERROR (0x70)
#define SK_NO_SENSE         (0x0)
#define SK_RECOVERY_ERROR   (0x1)
#define SK_NOT_READY        (0x2)
#define SK_MEDIUM_ERROR     (0x3)
#define SK_HARDWARE_ERROR   (0x4)
#define SK_ILLEGAL_REQUEST  (0x5)
#define SK_UNIT_ATTENTION   (0x6)
#define SK_DATA_PROTECT     (0x7)
#define SK_BLANK_CHECK      (0x8)
#define SK_VENDOR_SPECIFIC  (0x9)
#define SK_RESERVED_A       (0xA)
#define SK_ABORT            (0xB)
#define SK_RESERVED_C       (0xC)
#define SK_OVERFLOW         (0xD)
#define SK_MISCOMPARE       (0xE)
#define SK_RESERVED_F       (0xF)

//
// Additional Sense Codes
//
#define ASC_NOT_READY                   (0x04)
#define ASC_MEDIA_ERR1                  (0x10)
#define ASC_MEDIA_ERR2                  (0x11)
#define ASC_MEDIA_ERR3                  (0x14)
#define ASC_MEDIA_ERR4                  (0x30)
#define ASC_MEDIA_UPSIDE_DOWN           (0x06)
#define ASC_INVALID_CMD                 (0x20)
#define ASC_LBA_OUT_OF_RANGE            (0x21)
#define ASC_INVALID_FIELD               (0x24)
#define ASC_WRITE_PROTECTED             (0x27)
#define ASC_MEDIA_CHANGE                (0x28)
#define ASC_RESET                       (0x29)  /* Power On Reset or Bus Reset occurred */
#define ASC_ILLEGAL_FIELD               (0x26)
#define ASC_NO_MEDIA                    (0x3A)
#define ASC_ILLEGAL_MODE_FOR_THIS_TRACK (0x64)

//
// Additional Sense Code Qualifier
//
#define ASCQ_IN_PROGRESS  (0x01)

#endif

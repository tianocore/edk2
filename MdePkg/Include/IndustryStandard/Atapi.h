/** @file
  This file contains just some basic definitions that are needed by drivers
  that dealing with ATA/ATAPI interface.

Copyright (c) 2007 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ATAPI_H_
#define _ATAPI_H_

#pragma pack(1)

///
/// ATAPI_IDENTIFY_DATA is defined in ATA-6
///
typedef struct {
    UINT16  config;             ///< General Configuration
    UINT16  obsolete_1;
    UINT16  specific_config;
    UINT16  obsolete_3;
    UINT16  retired_4_5[2];
    UINT16  obsolete_6;
    UINT16  cfa_reserved_7_8[2];
    UINT16  retired_9;
    CHAR8   SerialNo[20];       ///< ASCII
    UINT16  retired_20_21[2];
    UINT16  obsolete_22;
    CHAR8   FirmwareVer[8];     ///< ASCII
    CHAR8   ModelName[40];      ///< ASCII
    UINT16  multi_sector_cmd_max_sct_cnt;
    UINT16  reserved_48;
    UINT16  capabilities_49;
    UINT16  capabilities_50;
    UINT16  obsolete_51_52[2];
    UINT16  field_validity;
    UINT16  obsolete_54_58[5];
    UINT16  mutil_sector_setting;
    UINT16  user_addressable_sectors_lo;
    UINT16  user_addressable_sectors_hi;
    UINT16  obsolete_62;
    UINT16  multi_word_dma_mode;
    UINT16  advanced_pio_modes;
    UINT16  min_multi_word_dma_cycle_time;
    UINT16  rec_multi_word_dma_cycle_time;
    UINT16  min_pio_cycle_time_without_flow_control;
    UINT16  min_pio_cycle_time_with_flow_control;
    UINT16  reserved_69_74[6];
    UINT16  queue_depth;
    UINT16  reserved_76_79[4];
    UINT16  major_version_no;
    UINT16  minor_version_no;
    UINT16  cmd_set_support_82;
    UINT16  cmd_set_support_83;
    UINT16  cmd_feature_support;
    UINT16  cmd_feature_enable_85;
    UINT16  cmd_feature_enable_86;
    UINT16  cmd_feature_default;
    UINT16  ultra_dma_select;
    UINT16  time_required_for_sec_erase;
    UINT16  time_required_for_enhanced_sec_erase;
    UINT16  current_advanced_power_mgmt_value;
    UINT16  master_pwd_revison_code;
    UINT16  hardware_reset_result;
    UINT16  current_auto_acoustic_mgmt_value;
    UINT16  reserved_95_99[5];
    UINT16  max_user_lba_for_48bit_addr[4];
    UINT16  reserved_104_126[23];
    UINT16  removable_media_status_notification_support;
    UINT16  security_status;
    UINT16  vendor_data_129_159[31];
    UINT16  cfa_power_mode;
    UINT16  cfa_reserved_161_175[15];
    UINT16  current_media_serial_no[30];
    UINT16  reserved_206_254[49];
    UINT16  integrity_word;
} ATAPI_IDENTIFY_DATA;

///
/// Standard Quiry Data format, defined in SFF-8070i(ATAPI Removable Rewritable Specification)
///
typedef struct {
  UINT8 peripheral_type;
  UINT8 RMB;
  UINT8 version;
  UINT8 response_data_format;
  UINT8 addnl_length;     ///< n - 4, Numbers of bytes following this one
  UINT8 reserved_5;
  UINT8 reserved_6;
  UINT8 reserved_7;
  UINT8 vendor_info[8];
  UINT8 product_id[16];
  UINT8 product_revision_level[4];
  UINT8 vendor_specific_36_55[55 - 36 + 1];
  UINT8 reserved_56_95[95 - 56 + 1];
  ///
  /// Vendor specific parameters fields, the sizeof (ATAPI_INQUIRY_DATA) is 254
  /// since allocation_length is one byte in ATAPI_INQUIRY_CMD.
  ///
  UINT8 vendor_specific_96_253[253 - 96 + 1];
} ATAPI_INQUIRY_DATA;

///
/// Request Sense Standard Data, defined in SFF-8070i(ATAPI Removable Rewritable Specification)
///
typedef struct {
  UINT8 error_code : 7;
  UINT8 valid : 1;
  UINT8 reserved_1;
  UINT8 sense_key : 4;
  UINT8 reserved_2 : 1;
  UINT8 Vendor_specifc_1 : 3;
  UINT8 vendor_specific_3;
  UINT8 vendor_specific_4;
  UINT8 vendor_specific_5;
  UINT8 vendor_specific_6;
  UINT8 addnl_sense_length;           ///< n - 7
  UINT8 vendor_specific_8;
  UINT8 vendor_specific_9;
  UINT8 vendor_specific_10;
  UINT8 vendor_specific_11;
  UINT8 addnl_sense_code;             ///< mandatory
  UINT8 addnl_sense_code_qualifier;   ///< mandatory
  UINT8 field_replaceable_unit_code;  ///< optional
  UINT8 sense_key_specific_15 : 7;
  UINT8 SKSV : 1;
  UINT8 sense_key_specific_16;
  UINT8 sense_key_specific_17;
} ATAPI_REQUEST_SENSE_DATA;

//
// The followings are defined in SFF-8070i(ATAPI Removable Rewritable Specification)
//

///
/// READ CAPACITY Data 
///
typedef struct {
  UINT8 LastLba3;
  UINT8 LastLba2;
  UINT8 LastLba1;
  UINT8 LastLba0;
  UINT8 BlockSize3;
  UINT8 BlockSize2;
  UINT8 BlockSize1;
  UINT8 BlockSize0;
} ATAPI_READ_CAPACITY_DATA;

///
/// Capacity List Header + Current/Maximum Capacity Descriptor,
///
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
} ATAPI_READ_FORMAT_CAPACITY_DATA;

///
/// Test Unit Ready Command
///
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
} ATAPI_TEST_UNIT_READY_CMD;

///
/// INQUIRY Command
///
typedef struct {
  UINT8 opcode;
  UINT8 reserved_1 : 5;
  UINT8 lun : 3;
  UINT8 page_code;        ///< defined in SFF8090i, V6
  UINT8 reserved_3;
  UINT8 allocation_length;
  UINT8 reserved_5;
  UINT8 reserved_6;
  UINT8 reserved_7;
  UINT8 reserved_8;
  UINT8 reserved_9;
  UINT8 reserved_10;
  UINT8 reserved_11;
} ATAPI_INQUIRY_CMD;

///
/// REQUEST SENSE Command
///
typedef struct {
  UINT8 opcode;
  UINT8 reserved_1 : 5;
  UINT8 lun : 3;
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
} ATAPI_REQUEST_SENSE_CMD;

///
/// READ (10) Command
///
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
} ATAPI_READ10_CMD;

///
/// READ Format Capacity Command
///
typedef struct {
  UINT8 opcode;
  UINT8 reserved_1 : 5;
  UINT8 lun : 3;
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
} ATAPI_READ_FORMAT_CAP_CMD;

///
/// MODE SENSE Command
///
typedef struct {
  UINT8 opcode;
  UINT8 reserved_1 : 5;
  UINT8 lun : 3;
  UINT8 page_code : 6;
  UINT8 page_control : 2;
  UINT8 reserved_3;
  UINT8 reserved_4;
  UINT8 reserved_5;
  UINT8 reserved_6;
  UINT8 parameter_list_length_hi;
  UINT8 parameter_list_length_lo;
  UINT8 reserved_9;
  UINT8 reserved_10;
  UINT8 reserved_11;
} ATAPI_MODE_SENSE_CMD;

///
/// ATAPI_PACKET_COMMAND is not defined in ATA specification.
/// We add it here for the convenience for ATA/ATAPI module writer. 
///
typedef union {
  UINT16                    Data16[6];
  ATAPI_TEST_UNIT_READY_CMD TestUnitReady;
  ATAPI_READ10_CMD          Read10;
  ATAPI_REQUEST_SENSE_CMD   RequestSence;
  ATAPI_INQUIRY_CMD         Inquiry;
  ATAPI_MODE_SENSE_CMD      ModeSense;
  ATAPI_READ_FORMAT_CAP_CMD ReadFormatCapacity;
} ATAPI_PACKET_COMMAND;

#pragma pack()


#define ATAPI_MAX_DMA_EXT_CMD_SECTORS 0x10000
#define ATAPI_MAX_DMA_CMD_SECTORS     0x100

//
// ATA Packet Command Code
//
#define ATA_CMD_SOFT_RESET                  0x08   ///< defined in ATA-6
#define ATA_CMD_PACKET                      0xA0   ///< defined in ATA-6
#define ATA_CMD_IDENTIFY_DEVICE             0xA1   ///< defined in ATA-6
#define ATA_CMD_SERVICE                     0xA2   ///< defined in ATA-6
#define ATA_CMD_TEST_UNIT_READY             0x00   ///< defined in ATA-6
#define ATA_CMD_REQUEST_SENSE               0x03   ///< defined in ATA-6
#define ATA_CMD_INQUIRY                     0x12   ///< defined in ATAPI Removable Rewritable Media Devcies
#define ATA_CMD_READ_FORMAT_CAPACITY        0x23   ///< defined in ATAPI Removable Rewritable Media Devcies
#define ATA_CMD_READ_CAPACITY               0x25   ///< defined in ATAPI Removable Rewritable Media Devcies
#define ATA_CMD_READ_10                     0x28   ///< defined in ATAPI Removable Rewritable Media Devcies
#define ATA_CMD_WRITE_10                    0x2A   ///< defined in ATAPI Removable Rewritable Media Devcies

//
// ATA Commands Code
//

//
// Class 1: PIO Data-In Commands
//
#define ATA_CMD_IDENTIFY_DRIVE          0xec
#define ATA_CMD_READ_BUFFER             0xe4
#define ATA_CMD_READ_SECTORS            0x20   ///< defined in ATA-5     
#define ATA_CMD_READ_SECTORS_WITH_RETRY 0x21   ///< defined in ATA-5
#define ATA_CMD_READ_LONG               0x22   ///< defined in ATA-5
#define ATA_CMD_READ_LONG_WITH_RETRY    0x23   ///< defined in ATA-5
#define ATA_CMD_READ_SECTORS_EXT        0x24   ///< defined in ATA-6

//
// Class 2: PIO Data-Out Commands
//
#define ATA_CMD_FORMAT_TRACK              0x50  ///< defined in ATA-3
#define ATA_CMD_WRITE_BUFFER              0xe8  ///< defined in ATA-6  
#define ATA_CMD_WRITE_SECTORS             0x30  ///< defined in ATA-6
#define ATA_CMD_WRITE_SECTORS_WITH_RETRY  0x31  ///< defined in ATA-4
#define ATA_CMD_WRITE_LONG                0x32  ///< defined in ATA-3
#define ATA_CMD_WRITE_LONG_WITH_RETRY     0x33  ///< defined in ATA-3
#define ATA_CMD_WRITE_VERIFY              0x3c  ///< defined in ATA-3
#define ATA_CMD_WRITE_SECTORS_EXT         0x34  ///< defined in ATA-6

//
// Class 3 No Data Command
//
#define ATA_CMD_ACK_MEDIA_CHANGE        0xdb  ///< defined in ATA-2
#define ATA_CMD_BOOT_POST_BOOT          0xdc  ///< defined in ATA-2
#define ATA_CMD_BOOT_PRE_BOOT           0xdd  ///< defined in ATA-2
#define ATA_CMD_CHECK_POWER_MODE        0x98  ///< defined in ATA-3
#define ATA_CMD_CHECK_POWER_MODE_ALIAS  0xe5  ///< defined in ATA-6
#define ATA_CMD_DOOR_LOCK               0xde  ///< defined in ATA-6
#define ATA_CMD_DOOR_UNLOCK             0xdf  ///< defined in ATA-6
#define ATA_CMD_EXEC_DRIVE_DIAG         0x90  ///< defined in ATA-6
#define ATA_CMD_IDLE_ALIAS              0x97  ///< defined in ATA-3
#define ATA_CMD_IDLE                    0xe3  ///< defined in ATA-6
#define ATA_CMD_IDLE_IMMEDIATE          0x95  ///< defined in ATA-3
#define ATA_CMD_IDLE_IMMEDIATE_ALIAS    0xe1  ///< defined in ATA-6
#define ATA_CMD_INIT_DRIVE_PARAM        0x91  ///< defined in ATA-5
#define ATA_CMD_RECALIBRATE             0x10  ///< defined in ATA-3
#define ATA_CMD_READ_DRIVE_STATE        0xe9  ///< defined in ATA-2
#define ATA_CMD_SET_MULTIPLE_MODE       0xC6  ///< defined in ATA-6
#define ATA_CMD_READ_VERIFY             0x40  ///< defined in ATA-6
#define ATA_CMD_READ_VERIFY_WITH_RETRY  0x41  ///< defined in ATA-4
#define ATA_CMD_SEEK                    0x70  ///< defined in ATA-6
#define ATA_CMD_SET_FEATURES            0xef  ///< defined in ATA-6
#define ATA_CMD_STANDBY                 0x96  ///< defined in ATA-3
#define ATA_CMD_STANDBY_ALIAS           0xe2  ///< defined in ATA-6
#define ATA_CMD_STANDBY_IMMEDIATE       0x94  ///< defined in ATA-3
#define ATA_CMD_STANDBY_IMMEDIATE_ALIAS 0xe0  ///< defined in ATA-6
//
// S.M.A.R.T
//
#define ATA_CMD_SMART               0xb0
#define ATA_CONSTANT_C2             0xc2
#define ATA_CONSTANT_4F             0x4f
#define ATA_SMART_ENABLE_OPERATION  0xd8
#define ATA_SMART_RETURN_STATUS     0xda

//
// Class 4: DMA Command
//
#define ATA_CMD_READ_DMA              0xc8   ///< defined in ATA-6
#define ATA_CMD_READ_DMA_WITH_RETRY   0xc9   ///< defined in ATA-4
#define ATA_CMD_READ_DMA_EXT          0x25   ///< defined in ATA-6
#define ATA_CMD_WRITE_DMA             0xca   ///< defined in ATA-6
#define ATA_CMD_WRITE_DMA_WITH_RETRY  0xcb   ///< defined in ATA-4
#define ATA_CMD_WRITE_DMA_EXT         0x35   ///< defined in ATA-6
        
///
/// default content of device control register, disable INT,
/// Bit3 is set to 1 according ATA-1
///
#define ATA_DEFAULT_CTL           (0x0a)  
///
/// default context of Device/Head Register,
/// Bit7 and Bit5 are set to 1 for back-compatibilities
///
#define ATA_DEFAULT_CMD           (0xa0)

#define ATAPI_MAX_BYTE_COUNT  (0xfffe)

#define ATA_REQUEST_SENSE_ERROR (0x70) ///< defined in SFF-8070i

//
// Sense Key, Additional Sense Codes and Additional Sense Code Qualifier
// defined in MultiMedia Commands (MMC, MMC-2) 
//
// Sense Key 
//
#define ATA_SK_NO_SENSE         (0x0)
#define ATA_SK_RECOVERY_ERROR   (0x1)
#define ATA_SK_NOT_READY        (0x2)
#define ATA_SK_MEDIUM_ERROR     (0x3)
#define ATA_SK_HARDWARE_ERROR   (0x4)
#define ATA_SK_ILLEGAL_REQUEST  (0x5)
#define ATA_SK_UNIT_ATTENTION   (0x6)
#define ATA_SK_DATA_PROTECT     (0x7)
#define ATA_SK_BLANK_CHECK      (0x8)
#define ATA_SK_VENDOR_SPECIFIC  (0x9)
#define ATA_SK_RESERVED_A       (0xA)
#define ATA_SK_ABORT            (0xB)
#define ATA_SK_RESERVED_C       (0xC)
#define ATA_SK_OVERFLOW         (0xD)
#define ATA_SK_MISCOMPARE       (0xE)
#define ATA_SK_RESERVED_F       (0xF)

//
// Additional Sense Codes
//
#define ATA_ASC_NOT_READY                   (0x04)
#define ATA_ASC_MEDIA_ERR1                  (0x10)
#define ATA_ASC_MEDIA_ERR2                  (0x11)
#define ATA_ASC_MEDIA_ERR3                  (0x14)
#define ATA_ASC_MEDIA_ERR4                  (0x30)
#define ATA_ASC_MEDIA_UPSIDE_DOWN           (0x06)
#define ATA_ASC_INVALID_CMD                 (0x20)
#define ATA_ASC_LBA_OUT_OF_RANGE            (0x21)
#define ATA_ASC_INVALID_FIELD               (0x24)
#define ATA_ASC_WRITE_PROTECTED             (0x27)
#define ATA_ASC_MEDIA_CHANGE                (0x28)
#define ATA_ASC_RESET                       (0x29)  ///< Power On Reset or Bus Reset occurred
#define ATA_ASC_ILLEGAL_FIELD               (0x26)
#define ATA_ASC_NO_MEDIA                    (0x3A)
#define ATA_ASC_ILLEGAL_MODE_FOR_THIS_TRACK (0x64)

//
// Additional Sense Code Qualifier
//
#define ATA_ASCQ_IN_PROGRESS  (0x01)

//
// Error Register
//
#define ATA_ERRREG_BBK   BIT7  ///< Bad block detected      defined in ATA-1
#define ATA_ERRREG_UNC   BIT6  ///< Uncorrectable Data      defined in ATA-3
#define ATA_ERRREG_MC    BIT5  ///< Media Change            defined in ATA-3
#define ATA_ERRREG_IDNF  BIT4  ///< ID Not Found            defined in ATA-3
#define ATA_ERRREG_MCR   BIT3  ///< Media Change Requested  defined in ATA-3
#define ATA_ERRREG_ABRT  BIT2  ///< Aborted Command         defined in ATA-6
#define ATA_ERRREG_TK0NF BIT1  ///< Track 0 Not Found       defined in ATA-3
#define ATA_ERRREG_AMNF  BIT0  ///< Address Mark Not Found  defined in ATA-3

//
// Status Register
//
#define ATA_STSREG_BSY   BIT7  ///< Controller Busy         defined in ATA-6
#define ATA_STSREG_DRDY  BIT6  ///< Drive Ready             defined in ATA-6
#define ATA_STSREG_DWF   BIT5  ///< Drive Write Fault       defined in ATA-6
#define ATA_STSREG_DSC   BIT4  ///< Disk Seek Complete      defined in ATA-3
#define ATA_STSREG_DRQ   BIT3  ///< Data Request            defined in ATA-6
#define ATA_STSREG_CORR  BIT2  ///< Corrected Data          defined in ATA-3
#define ATA_STSREG_IDX   BIT1  ///< Index                   defined in ATA-3
#define ATA_STSREG_ERR   BIT0  ///< Error                   defined in ATA-6

//
// Device Control Register
//
#define ATA_CTLREG_SRST  BIT2  ///< Software Reset
#define ATA_CTLREG_IEN_L BIT1  ///< Interrupt Enable #

#endif


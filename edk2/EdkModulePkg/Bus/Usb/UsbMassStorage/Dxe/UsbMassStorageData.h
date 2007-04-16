/*++
Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    UsbMassStorageData.h

Abstract:

    Header file for USB Mass Storage Device related Data Structures

Revision History
--*/

#ifndef _USB_FLP_DATA_H
#define _USB_FLP_DATA_H

//
// bit definition
//
#define bit(a)  (1 << (a))

//
// timeout unit is in millisecond.
//

#define STALL_1_MILLI_SECOND  1000
#define USBFLPTIMEOUT         STALL_1_MILLI_SECOND
#define USBDATATIMEOUT        2 * STALL_1_MILLI_SECOND
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
  UINT8 reserved_1 : 4;
  UINT8 lun : 4;
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
} MODE_SENSE_CMD_UFI;

typedef struct {
  UINT8 opcode;
  UINT8 reserved_1 : 3;
  UINT8 dbd : 1;
  UINT8 reserved_2 : 1;
  UINT8 lun : 3;
  UINT8 page_code : 6;
  UINT8 page_control : 2;
  UINT8 reserved_3;
  UINT8 allocation_length;
  UINT8 control;
} MODE_SENSE_CMD_SCSI;

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

typedef union {
  UINT16              Data16[6];
  TEST_UNIT_READY_CMD TestUnitReady;
  READ10_CMD          Read10;
  REQUEST_SENSE_CMD   RequestSense;
  INQUIRY_CMD         Inquiry;
  MODE_SENSE_CMD_UFI  ModeSenseUFI;
  READ_FORMAT_CAP_CMD ReadFormatCapacity;
  MODE_SENSE_CMD_SCSI ModeSenseSCSI;
} ATAPI_PACKET_COMMAND;

#pragma pack()
//
// Packet Command Code
//
#define TEST_UNIT_READY             0x00
#define REZERO                      0x01
#define REQUEST_SENSE               0x03
#define FORMAT_UNIT                 0x04
#define REASSIGN_BLOCKS             0x07
#define INQUIRY                     0x12
#define START_STOP_UNIT             0x1B
#define PREVENT_ALLOW_MEDIA_REMOVAL 0x1E
#define READ_FORMAT_CAPACITY        0x23
#define OLD_FORMAT_UNIT             0x24
#define READ_CAPACITY               0x25
#define READ_10                     0x28
#define WRITE_10                    0x2A
#define SEEK                        0x2B
#define SEND_DIAGNOSTICS            0x3D
#define WRITE_VERIFY                0x2E
#define VERIFY                      0x2F
#define READ_DEFECT_DATA            0x37
#define WRITE_BUFFER                0x38
#define READ_BUFFER                 0x3C
#define READ_LONG                   0x3E
#define WRITE_LONG                  0x3F
#define MODE_SELECT                 0x55
#define UFI_MODE_SENSE5A            0x5A
#define SCSI_MODE_SENSE1A           0x1A
#define READ_12                     0xA8
#define WRITE_12                    0xAA
#define MAX_ATAPI_BYTE_COUNT        (0xfffe)

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
#define ASC_LOGICAL_UNIT_STATUS         (0x08)

//
// Additional Sense Code Qualifier
//
#define ASCQ_IN_PROGRESS          (0x01)
#define ASCQ_DEVICE_BUSY          (0xff)
#define ASCQ_LOGICAL_UNIT_FAILURE (0x00)
#define ASCQ_LOGICAL_UNIT_TIMEOUT (0x01)
#define ASCQ_LOGICAL_UNIT_OVERRUN (0x80)

#define SETFEATURE                TRUE
#define CLEARFEATURE              FALSE

//
//  ATAPI Data structure
//
#pragma pack(1)

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

typedef struct {
  UINT8 mode_data_len_hi;
  UINT8 mode_data_len_lo;
  UINT8 media_type_code;
  UINT8 reserved_3_0 : 4;
  UINT8 dpofua : 1;
  UINT8 reserved_3_1 : 2;
  UINT8 write_protected : 1;
  UINT8 reserved_4;
  UINT8 reserved_5;
  UINT8 reserved_6;
  UINT8 reserved_7;
} UFI_MODE_PARAMETER_HEADER;

typedef struct {
  UINT8 mode_data_len;
  UINT8 media_type_code;
  UINT8 speed : 4;
  UINT8 buffered_mode : 3;
  UINT8 write_protected : 1;
  UINT8 block_descritptor_length;
} SCSI_MODE_PARAMETER_HEADER6;

typedef struct {
  UINT8 page_code : 6;
  UINT8 reserved_0 : 1;
  UINT8 parameter_savable : 1;
  UINT8 page_length;
  UINT8 transfer_rate_msb;
  UINT8 transfer_rate_lsb;
  UINT8 number_of_heads;
  UINT8 sectors_per_track;
  UINT8 databytes_per_sector_msb;
  UINT8 databytes_per_sector_lsb;
  UINT8 number_of_cylinders_msb;
  UINT8 number_of_cylinders_lsb;
  UINT8 reserved_10_18[9];
  UINT8 motor_on_delay;
  UINT8 motor_off_delay;
  UINT8 reserved_21_27[7];
  UINT8 medium_rotation_rate_msb;
  UINT8 medium_rotation_rate_lsb;
  UINT8 reserved_30_31[2];
} FLEXIBLE_DISK_PAGE;

typedef struct {
  UFI_MODE_PARAMETER_HEADER mode_param_header;
  FLEXIBLE_DISK_PAGE        flex_disk_page;
} UFI_MODE_PARAMETER_PAGE_5;

typedef struct {
  UINT8 page_code : 6;
  UINT8 reserved_0 : 1;
  UINT8 parameter_savable : 1;
  UINT8 page_length;
  UINT8 reserved_2;
  UINT8 inactive_time_multplier : 4;
  UINT8 reserved_3 : 4;
  UINT8 software_write_protect : 1;
  UINT8 disable_media_access : 1;
  UINT8 reserved_4 : 6;
  UINT8 reserved_5;
  UINT8 reserved_6;
  UINT8 reserved_7;
} TIMER_AND_PROTECT_PAGE;

typedef struct {
  UFI_MODE_PARAMETER_HEADER mode_param_header;
  TIMER_AND_PROTECT_PAGE    time_and_protect_page;
} UFI_MODE_PARAMETER_PAGE_1C;

#pragma pack()

#endif

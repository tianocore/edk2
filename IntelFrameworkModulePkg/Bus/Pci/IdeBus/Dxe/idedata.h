/** @file
  Header file for IDE Bus Driver's Data Structures

  Copyright (c) 2006 - 2007 Intel Corporation. <BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _IDE_DATA_H
#define _IDE_DATA_H

//
// bit definition
//
#define bit0  (1 << 0)
#define bit1  (1 << 1)
#define bit2  (1 << 2)
#define bit3  (1 << 3)
#define bit4  (1 << 4)
#define bit5  (1 << 5)
#define bit6  (1 << 6)
#define bit7  (1 << 7)
#define bit8  (1 << 8)
#define bit9  (1 << 9)
#define bit10 (1 << 10)
#define bit11 (1 << 11)
#define bit12 (1 << 12)
#define bit13 (1 << 13)
#define bit14 (1 << 14)
#define bit15 (1 << 15)
#define bit16 (1 << 16)
#define bit17 (1 << 17)
#define bit18 (1 << 18)
#define bit19 (1 << 19)
#define bit20 (1 << 20)
#define bit21 (1 << 21)
#define bit22 (1 << 22)
#define bit23 (1 << 23)
#define bit24 (1 << 24)
#define bit25 (1 << 25)
#define bit26 (1 << 26)
#define bit27 (1 << 27)
#define bit28 (1 << 28)
#define bit29 (1 << 29)
#define bit30 (1 << 30)
#define bit31 (1 << 31)

//
// common constants
//
#define STALL_1_MILLI_SECOND  1000    // stall 1 ms
#define STALL_1_SECOND        1000000 // stall 1 second
typedef enum {
  IdePrimary    = 0,
  IdeSecondary  = 1,
  IdeMaxChannel = 2
} EFI_IDE_CHANNEL;

typedef enum {
  IdeMaster     = 0,
  IdeSlave      = 1,
  IdeMaxDevice  = 2
} EFI_IDE_DEVICE;

typedef enum {
  IdeMagnetic,                        /* ZIP Drive or LS120 Floppy Drive */
  IdeCdRom,                           /* ATAPI CDROM */
  IdeHardDisk,                        /* Hard Disk */
  Ide48bitAddressingHardDisk,         /* Hard Disk larger than 120GB */
  IdeUnknown
} IDE_DEVICE_TYPE;

typedef enum {
  SenseNoSenseKey,
  SenseDeviceNotReadyNoRetry,
  SenseDeviceNotReadyNeedRetry,
  SenseNoMedia,
  SenseMediaChange,
  SenseMediaError,
  SenseOtherSense
} SENSE_RESULT;

typedef enum {
  AtaUdmaReadOp,
  AtaUdmaReadExtOp,
  AtaUdmaWriteOp,
  AtaUdmaWriteExtOp
} ATA_UDMA_OPERATION;

//
// IDE Registers
//
typedef union {
  UINT16  Command;        /* when write */
  UINT16  Status;         /* when read */
} IDE_CMD_OR_STATUS;

typedef union {
  UINT16  Error;          /* when read */
  UINT16  Feature;        /* when write */
} IDE_ERROR_OR_FEATURE;

typedef union {
  UINT16  AltStatus;      /* when read */
  UINT16  DeviceControl;  /* when write */
} IDE_AltStatus_OR_DeviceControl;

//
// IDE registers set
//
typedef struct {
  UINT16                          Data;
  IDE_ERROR_OR_FEATURE            Reg1;
  UINT16                          SectorCount;
  UINT16                          SectorNumber;
  UINT16                          CylinderLsb;
  UINT16                          CylinderMsb;
  UINT16                          Head;
  IDE_CMD_OR_STATUS               Reg;

  IDE_AltStatus_OR_DeviceControl  Alt;
  UINT16                          DriveAddress;

  UINT16                          MasterSlave;
  UINT16                          BusMasterBaseAddr;
} IDE_BASE_REGISTERS;

//
// IDE registers' base addresses
//
typedef struct {
  UINT16  CommandBlockBaseAddr;
  UINT16  ControlBlockBaseAddr;
  UINT16  BusMasterBaseAddr;
} IDE_REGISTERS_BASE_ADDR;

//
// Bit definitions in Programming Interface byte of the Class Code field
// in PCI IDE controller's Configuration Space
//
#define IDE_PRIMARY_OPERATING_MODE            bit0
#define IDE_PRIMARY_PROGRAMMABLE_INDICATOR    bit1
#define IDE_SECONDARY_OPERATING_MODE          bit2
#define IDE_SECONDARY_PROGRAMMABLE_INDICATOR  bit3

//
// IDE registers bit definitions
//

//
// Err Reg
//
#define BBK_ERR   bit7  /* Bad block detected */
#define UNC_ERR   bit6  /* Uncorrectable Data */
#define MC_ERR    bit5  /* Media Change */
#define IDNF_ERR  bit4  /* ID Not Found */
#define MCR_ERR   bit3  /* Media Change Requested */
#define ABRT_ERR  bit2  /* Aborted Command */
#define TK0NF_ERR bit1  /* Track 0 Not Found */
#define AMNF_ERR  bit0  /* Address Mark Not Found */

//
// Device/Head Reg
//
#define LBA_MODE  bit6
#define DEV       bit4
#define HS3       bit3
#define HS2       bit2
#define HS1       bit1
#define HS0       bit0
#define CHS_MODE  (0)
#define DRV0      (0)
#define DRV1      (1)
#define MST_DRV   DRV0
#define SLV_DRV   DRV1

//
// Status Reg
//
#define BSY   bit7  /* Controller Busy */
#define DRDY  bit6  /* Drive Ready */
#define DWF   bit5  /* Drive Write Fault */
#define DSC   bit4  /* Disk Seek Complete */
#define DRQ   bit3  /* Data Request */
#define CORR  bit2  /* Corrected Data */
#define IDX   bit1  /* Index */
#define ERR   bit0  /* Error */

//
// Device Control Reg
//
#define SRST  bit2  /* Software Reset */
#define IEN_L bit1  /* Interrupt Enable #*/

//
// Bus Master Reg
//
#define BMIC_nREAD      bit3
#define BMIC_START      bit0
#define BMIS_INTERRUPT  bit2
#define BMIS_ERROR      bit1

#define BMICP_OFFSET    0x00
#define BMISP_OFFSET    0x02
#define BMIDP_OFFSET    0x04
#define BMICS_OFFSET    0x08
#define BMISS_OFFSET    0x0A
#define BMIDS_OFFSET    0x0C

//
// Time Out Value For IDE Device Polling
//

//
// ATATIMEOUT is used for waiting time out for ATA device
//

//
// 1 second
//
#define ATATIMEOUT  1000  

//
// ATAPITIMEOUT is used for waiting operation
// except read and write time out for ATAPI device
//

//
// 1 second
//
#define ATAPITIMEOUT  1000 

//
// ATAPILONGTIMEOUT is used for waiting read and
// write operation timeout for ATAPI device
//

//
// 2 seconds
//
#define CDROMLONGTIMEOUT  2000  

//
// 5 seconds
//
#define ATAPILONGTIMEOUT  5000  

//
// 10 seconds
//
#define ATASMARTTIMEOUT   10000

//
// ATA Commands Code
//
#define ATA_INITIALIZE_DEVICE 0x91

//
// Class 1
//
#define IDENTIFY_DRIVE_CMD          0xec
#define READ_BUFFER_CMD             0xe4
#define READ_SECTORS_CMD            0x20
#define READ_SECTORS_WITH_RETRY_CMD 0x21
#define READ_LONG_CMD               0x22
#define READ_LONG_WITH_RETRY_CMD    0x23
//
// Class 1 - Atapi6 enhanced commands
//
#define READ_SECTORS_EXT_CMD  0x24

//
// Class 2
//
#define FORMAT_TRACK_CMD              0x50
#define WRITE_BUFFER_CMD              0xe8
#define WRITE_SECTORS_CMD             0x30
#define WRITE_SECTORS_WITH_RETRY_CMD  0x31
#define WRITE_LONG_CMD                0x32
#define WRITE_LONG_WITH_RETRY_CMD     0x33
#define WRITE_VERIFY_CMD              0x3c
//
// Class 2 - Atapi6 enhanced commands
//
#define WRITE_SECTORS_EXT_CMD 0x34

//
// Class 3
//
#define ACK_MEDIA_CHANGE_CMD        0xdb
#define BOOT_POST_BOOT_CMD          0xdc
#define BOOT_PRE_BOOT_CMD           0xdd
#define CHECK_POWER_MODE_CMD        0x98
#define CHECK_POWER_MODE_CMD_ALIAS  0xe5
#define DOOR_LOCK_CMD               0xde
#define DOOR_UNLOCK_CMD             0xdf
#define EXEC_DRIVE_DIAG_CMD         0x90
#define IDLE_CMD_ALIAS              0x97
#define IDLE_CMD                    0xe3
#define IDLE_IMMEDIATE_CMD          0x95
#define IDLE_IMMEDIATE_CMD_ALIAS    0xe1
#define INIT_DRIVE_PARAM_CMD        0x91
#define RECALIBRATE_CMD             0x10  /* aliased to 1x */
#define READ_DRIVE_STATE_CMD        0xe9
#define SET_MULTIPLE_MODE_CMD       0xC6
#define READ_DRIVE_STATE_CMD        0xe9
#define READ_VERIFY_CMD             0x40
#define READ_VERIFY_WITH_RETRY_CMD  0x41
#define SEEK_CMD                    0x70  /* aliased to 7x */
#define SET_FEATURES_CMD            0xef
#define STANDBY_CMD                 0x96
#define STANDBY_CMD_ALIAS           0xe2
#define STANDBY_IMMEDIATE_CMD       0x94
#define STANDBY_IMMEDIATE_CMD_ALIAS 0xe0

//
// Class 4
//
#define READ_DMA_CMD              0xc8
#define READ_DMA_WITH_RETRY_CMD   0xc9
#define READ_DMA_EXT_CMD          0x25
#define WRITE_DMA_CMD             0xca
#define WRITE_DMA_WITH_RETRY_CMD  0xcb
#define WRITE_DMA_EXT_CMD         0x35

//
// Class 5
//
#define READ_MULTIPLE_CMD         0xc4
#define REST_CMD                  0xe7
#define RESTORE_DRIVE_STATE_CMD   0xea
#define SET_SLEEP_MODE_CMD        0x99
#define SET_SLEEP_MODE_CMD_ALIAS  0xe6
#define WRITE_MULTIPLE_CMD        0xc5
#define WRITE_SAME_CMD            0xe9

//
// Class 6 - Host protected area access feature set
//
#define READ_NATIVE_MAX_ADDRESS_CMD 0xf8
#define SET_MAX_ADDRESS_CMD         0xf9

//
// Class 6 - ATA/ATAPI-6 enhanced commands
//
#define READ_NATIVE_MAX_ADDRESS_EXT_CMD 0x27
#define SET_MAX_ADDRESS_CMD_EXT         0x37

//
// Class 6 - SET_MAX related sub command (in feature register)
//
#define PARTIES_SET_MAX_ADDRESS_SUB_CMD 0x00
#define PARTIES_SET_PASSWORD_SUB_CMD    0x01
#define PARTIES_LOCK_SUB_CMD            0x02
#define PARTIES_UNLOCK_SUB_CMD          0x03
#define PARTIES_FREEZE_SUB_CMD          0x04

//
// S.M.A.R.T
//
#define ATA_SMART_CMD               0xb0
#define ATA_CONSTANT_C2             0xc2
#define ATA_CONSTANT_4F             0x4f
#define ATA_SMART_ENABLE_OPERATION  0xd8
#define ATA_SMART_RETURN_STATUS     0xda

//
// Error codes for Exec Drive Diag
//
#define DRIV_DIAG_NO_ERROR          (0x01)
#define DRIV_DIAG_FORMATTER_ERROR   (0x02)
#define DRIV_DIAG_DATA_BUFFER_ERROR (0x03)
#define DRIV_DIAG_ECC_CKT_ERRROR    (0x04)
#define DRIV_DIAG_UP_ERROR          (0x05)
#define DRIV_DIAG_SLAVE_DRV_ERROR   (0x80)  /* aliased to 0x8x */

//
// Codes for Format Track
//
#define FORMAT_GOOD_SECTOR            (0x00)
#define FORMAT_SUSPEND_ALLOC          (0x01)
#define FORMAT_REALLOC_SECTOR         (0x02)
#define FORMAT_MARK_SECTOR_DEFECTIVE  (0x03)

//
// IDE_IDENTIFY bits
// config bits :
//
#define ID_CONFIG_RESERVED0                             bit0
#define ID_CONFIG_HARD_SECTORED_DRIVE                   bit1
#define ID_CONFIG_SOFT_SECTORED_DRIVE                   bit2
#define ID_CONFIG_NON_MFM                               bit3
#define ID_CONFIG_15uS_HEAD_SWITCHING                   bit4
#define ID_CONFIG_SPINDLE_MOTOR_CONTROL                 bit5
#define ID_CONFIG_HARD_DRIVE                            bit6
#define ID_CONFIG_CHANGEABLE_MEDIUM                     bit7
#define ID_CONFIG_DATA_RATE_TO_5MHZ                     bit8
#define ID_CONFIG_DATA_RATE_5_TO_10MHZ                  bit9
#define ID_CONFIG_DATA_RATE_ABOVE_10MHZ                 bit10
#define ID_CONFIG_MOTOR_SPEED_TOLERANCE_ABOVE_0_5_PERC  bit11
#define ID_CONFIG_DATA_CLK_OFFSET_AVAIL                 bit12
#define ID_CONFIG_TRACK_OFFSET_AVAIL                    bit13
#define ID_CONFIG_SPEED_TOLERANCE_GAP_NECESSARY         bit14
#define ID_CONFIG_RESERVED1                             bit15

#define ID_DOUBLE_WORD_IO_POSSIBLE                      bit01
#define ID_LBA_SUPPORTED                                bit9
#define ID_DMA_SUPPORTED                                bit8

#define SET_FEATURE_ENABLE_8BIT_TRANSFER                (0x01)
#define SET_FEATURE_ENABLE_WRITE_CACHE                  (0x02)
#define SET_FEATURE_TRANSFER_MODE                       (0x03)
#define SET_FEATURE_WRITE_SAME_WRITE_SPECIFIC_AREA      (0x22)
#define SET_FEATURE_DISABLE_RETRIES                     (0x33)
//
// for Read & Write Longs
//
#define SET_FEATURE_VENDOR_SPEC_ECC_LENGTH                          (0x44)
#define SET_FEATURE_PLACE_NO_OF_CACHE_SEGMENTS_IN_SECTOR_NO_REG     (0x54)
#define SET_FEATURE_DISABLE_READ_AHEAD                              (0x55)
#define SET_FEATURE_MAINTAIN_PARAM_AFTER_RESET                      (0x66)
#define SET_FEATURE_DISABLE_ECC                                     (0x77)
#define SET_FEATURE_DISABLE_8BIT_TRANSFER                           (0x81)
#define SET_FEATURE_DISABLE_WRITE_CACHE                             (0x82)
#define SET_FEATURE_ENABLE_ECC                                      (0x88)
#define SET_FEATURE_ENABLE_RETRIES                                  (0x99)
#define SET_FEATURE_ENABLE_READ_AHEAD                               (0xaa)
#define SET_FEATURE_SET_SECTOR_CNT_REG_AS_NO_OF_READ_AHEAD_SECTORS  (0xab)
#define SET_FEATURE_ALLOW_REST_MODE                                 (0xac)
//
// for Read & Write Longs
//
#define SET_FEATURE_4BYTE_ECC                           (0xbb)
#define SET_FEATURE_DEFALUT_FEATURES_ON_SOFTWARE_RESET  (0xcc)
#define SET_FEATURE_WRITE_SAME_TO_WRITE_ENTIRE_MEDIUM   (0xdd)

#define BLOCK_TRANSFER_MODE                             (0x00)
#define SINGLE_WORD_DMA_TRANSFER_MODE                   (0x10)
#define MULTI_WORD_DMA_TRANSFER_MODE                    (0x20)
#define TRANSFER_MODE_MASK                              (0x07)  // 3 LSBs

//
// Drive 0 - Head 0
//
#define DEFAULT_DRIVE (0x00)
#define DEFAULT_CMD   (0xa0)
//
// default content of device control register, disable INT
//
#define DEFAULT_CTL                 (0x0a)
#define DEFAULT_IDE_BM_IO_BASE_ADR  (0xffa0)

//
// ATAPI6 related data structure definition
//

//
// The maximum sectors count in 28 bit addressing mode
//
#define MAX_28BIT_ADDRESSING_CAPACITY 0xfffffff

//
// Move the IDENTIFY section to DXE\Protocol\IdeControllerInit
//

//
// ATAPI Command
//
#define ATAPI_SOFT_RESET_CMD      0x08
#define ATAPI_PACKET_CMD          0xA0
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
  UINT8 reserved_1 : 4;
  UINT8 lun : 4;
  UINT8 page_code : 4;
  UINT8 page_control : 4;
  UINT8 reserved_3;
  UINT8 reserved_4;
  UINT8 reserved_5;
  UINT8 reserved_6;
  UINT8 parameter_list_length_hi;
  UINT8 parameter_list_length_lo;
  UINT8 reserved_9;
  UINT8 reserved_10;
  UINT8 reserved_11;
} MODE_SENSE_CMD;

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
  REQUEST_SENSE_CMD   RequestSence;
  INQUIRY_CMD         Inquiry;
  MODE_SENSE_CMD      ModeSense;
  READ_FORMAT_CAP_CMD ReadFormatCapacity;
} ATAPI_PACKET_COMMAND;

typedef struct {
  UINT32  RegionBaseAddr;
  UINT16  ByteCount;
  UINT16  EndOfTable;
} IDE_DMA_PRD;

#define MAX_DMA_EXT_COMMAND_SECTORS 0x10000
#define MAX_DMA_COMMAND_SECTORS     0x100

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
#define MODE_SENSE                  0x5A
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

//
// Additional Sense Code Qualifier
//
#define ASCQ_IN_PROGRESS  (0x01)

#define SETFEATURE        TRUE
#define CLEARFEATURE      FALSE

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
  UINT8 firmware_sub_rev_level[1];
  UINT8 reserved_37;
  UINT8 reserved_38;
  UINT8 reserved_39;
  UINT8 max_capacity_hi;
  UINT8 max_capacity_mid;
  UINT8 max_capacity_lo;
  UINT8 reserved_43_95[95 - 43 + 1];
} INQUIRY_DATA;

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
  UINT8 product_id[16];
  UINT8 product_revision_level[4];
  UINT8 vendor_specific[20];
  UINT8 reserved_56_95[40];
} CDROM_INQUIRY_DATA;

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
// PIO mode definition
//
typedef enum {
  ATA_PIO_MODE_BELOW_2,
  ATA_PIO_MODE_2,
  ATA_PIO_MODE_3,
  ATA_PIO_MODE_4
} ATA_PIO_MODE;

//
// Multi word DMA definition
//
typedef enum {
  ATA_MDMA_MODE_0,
  ATA_MDMA_MODE_1,
  ATA_MDMA_MODE_2
} ATA_MDMA_MODE;

//
// UDMA mode definition
//
typedef enum {
  ATA_UDMA_MODE_0,
  ATA_UDMA_MODE_1,
  ATA_UDMA_MODE_2,
  ATA_UDMA_MODE_3,
  ATA_UDMA_MODE_4,
  ATA_UDMA_MODE_5
} ATA_UDMA_MODE;

#define ATA_MODE_CATEGORY_DEFAULT_PIO 0x00
#define ATA_MODE_CATEGORY_FLOW_PIO    0x01
#define ATA_MODE_CATEGORY_MDMA        0x04
#define ATA_MODE_CATEGORY_UDMA        0x08

#pragma pack(1)

typedef struct {
  UINT8 ModeNumber : 3;
  UINT8 ModeCategory : 5;
} ATA_TRANSFER_MODE;

typedef struct {
  UINT8 Sector;
  UINT8 Heads;
  UINT8 MultipleSector;
} ATA_DRIVE_PARMS;

#pragma pack()
//
// IORDY Sample Point field value
//
#define ISP_5_CLK 0
#define ISP_4_CLK 1
#define ISP_3_CLK 2
#define ISP_2_CLK 3

//
// Recovery Time field value
//
#define RECVY_4_CLK 0
#define RECVY_3_CLK 1
#define RECVY_2_CLK 2
#define RECVY_1_CLK 3

//
// Slave IDE Timing Register Enable
//
#define SITRE bit14

//
// DMA Timing Enable Only Select 1
//
#define DTE1  bit7

//
// Pre-fetch and Posting Enable Select 1
//
#define PPE1  bit6

//
// IORDY Sample Point Enable Select 1
//
#define IE1 bit5

//
// Fast Timing Bank Drive Select 1
//
#define TIME1 bit4

//
// DMA Timing Enable Only Select 0
//
#define DTE0  bit3

//
// Pre-fetch and Posting Enable Select 0
//
#define PPE0  bit2

//
// IOREY Sample Point Enable Select 0
//
#define IE0 bit1

//
// Fast Timing Bank Drive Select 0
//
#define TIME0 bit0

#endif

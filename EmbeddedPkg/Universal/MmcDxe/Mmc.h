/** @file
  Main Header file for the MMC DXE driver

  Copyright (c) 2011-2015, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MMC_H
#define __MMC_H

#include <Uefi.h>

#include <Protocol/DiskIo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/MmcHost.h>

#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define MMC_TRACE(txt)  DEBUG((DEBUG_BLKIO, "MMC: " txt "\n"))

#define MMC_IOBLOCKS_READ   0
#define MMC_IOBLOCKS_WRITE  1

#define MMC_OCR_POWERUP  0x80000000

#define MMC_OCR_ACCESS_MASK    0x3          /* bit[30-29] */
#define MMC_OCR_ACCESS_BYTE    0x1          /* bit[29] */
#define MMC_OCR_ACCESS_SECTOR  0x2          /* bit[30] */

#define MMC_CSD_GET_CCC(Response)            (Response[2] >> 20)
#define MMC_CSD_GET_TRANSPEED(Response)      (Response[3] & 0xFF)
#define MMC_CSD_GET_READBLLEN(Response)      ((Response[2] >> 16) & 0xF)
#define MMC_CSD_GET_WRITEBLLEN(Response)     ((Response[0] >> 22) & 0xF)
#define MMC_CSD_GET_FILEFORMAT(Response)     ((Response[0] >> 10) & 0x3)
#define MMC_CSD_GET_FILEFORMATGRP(Response)  ((Response[0] >> 15) & 0x1)
#define MMC_CSD_GET_DEVICESIZE(csd)          (((Response[1] >> 30) & 0x3) | ((Response[2] & 0x3FF) << 2))
#define HC_MMC_CSD_GET_DEVICESIZE(Response)  ((Response[1] >> 16) | ((Response[2] & 0x40) << 16));
#define MMC_CSD_GET_DEVICESIZEMULT(csd)      ((Response[1] >> 15) & 0x7)

#define MMC_R0_READY_FOR_DATA  (1 << 8)

#define MMC_R0_CURRENTSTATE(Response)  ((Response[0] >> 9) & 0xF)

#define MMC_R0_STATE_IDLE   0
#define MMC_R0_STATE_READY  1
#define MMC_R0_STATE_IDENT  2
#define MMC_R0_STATE_STDBY  3
#define MMC_R0_STATE_TRAN   4
#define MMC_R0_STATE_DATA   5

#define EMMC_CMD6_ARG_ACCESS(x)   (((x) & 0x3) << 24)
#define EMMC_CMD6_ARG_INDEX(x)    (((x) & 0xFF) << 16)
#define EMMC_CMD6_ARG_VALUE(x)    (((x) & 0xFF) << 8)
#define EMMC_CMD6_ARG_CMD_SET(x)  (((x) & 0x7) << 0)

#define SWITCH_CMD_DATA_LENGTH   64
#define SD_HIGH_SPEED_SUPPORTED  0x20000
#define SD_DEFAULT_SPEED         25000000
#define SD_HIGH_SPEED            50000000
#define SWITCH_CMD_SUCCESS_MASK  0x0f000000

#define SD_CARD_CAPACITY  0x00000002

#define BUSWIDTH_4  4

typedef enum {
  UNKNOWN_CARD,
  MMC_CARD,              // MMC card
  MMC_CARD_HIGH,         // MMC Card with High capacity
  EMMC_CARD,             // eMMC 4.41 card
  SD_CARD,               // SD 1.1 card
  SD_CARD_2,             // SD 2.0 or above standard card
  SD_CARD_2_HIGH         // SD 2.0 or above high capacity card
} CARD_TYPE;

typedef struct {
  UINT32    Reserved0  :   7;   // 0
  UINT32    V170_V195  :   1;   // 1.70V - 1.95V
  UINT32    V200_V260  :   7;   // 2.00V - 2.60V
  UINT32    V270_V360  :   9;   // 2.70V - 3.60V
  UINT32    RESERVED_1 :  5;    // Reserved
  UINT32    AccessMode :  2;    // 00b (byte mode), 10b (sector mode)
  UINT32    PowerUp    :     1; // This bit is set to LOW if the card has not finished the power up routine
} OCR;

typedef struct {
  UINT8     SD_SPEC               :               4; // SD Memory Card - Spec. Version [59:56]
  UINT8     SCR_STRUCTURE         :         4;       // SCR Structure [63:60]
  UINT8     SD_BUS_WIDTHS         :         4;       // DAT Bus widths supported [51:48]
  UINT8     DATA_STAT_AFTER_ERASE : 1;               // Data Status after erases [55]
  UINT8     SD_SECURITY           :           3;     // CPRM Security Support [54:52]
  UINT8     EX_SECURITY_1         :         1;       // Extended Security Support [43]
  UINT8     SD_SPEC4              :              1;  // Spec. Version 4.00 or higher [42]
  UINT8     RESERVED_1            :            2;    // Reserved [41:40]
  UINT8     SD_SPEC3              :              1;  // Spec. Version 3.00 or higher [47]
  UINT8     EX_SECURITY_2         :         3;       // Extended Security Support [46:44]
  UINT8     CMD_SUPPORT           :           4;     // Command Support bits [35:32]
  UINT8     RESERVED_2            :            4;    // Reserved [39:36]
  UINT32    RESERVED_3;                              // Manufacturer Usage [31:0]
} SCR;

typedef struct {
  UINT32    NOT_USED;   // 1 [0:0]
  UINT32    CRC;        // CRC7 checksum [7:1]
  UINT32    MDT;        // Manufacturing date [19:8]
  UINT32    RESERVED_1; // Reserved [23:20]
  UINT32    PSN;        // Product serial number [55:24]
  UINT8     PRV;        // Product revision [63:56]
  UINT8     PNM[5];     // Product name [64:103]
  UINT16    OID;        // OEM/Application ID [119:104]
  UINT8     MID;        // Manufacturer ID [127:120]
} CID;

typedef struct {
  UINT8     NOT_USED           :           1;      // Not used, always 1 [0:0]
  UINT8     CRC                :                7; // CRC [7:1]

  UINT8     RESERVED_1         :         2;       // Reserved [9:8]
  UINT8     FILE_FORMAT        :        2;        // File format [11:10]
  UINT8     TMP_WRITE_PROTECT  :  1;              // Temporary write protection [12:12]
  UINT8     PERM_WRITE_PROTECT : 1;               // Permanent write protection [13:13]
  UINT8     COPY               :               1; // Copy flag (OTP) [14:14]
  UINT8     FILE_FORMAT_GRP    :    1;            // File format group [15:15]

  UINT16    RESERVED_2         :         5; // Reserved [20:16]
  UINT16    WRITE_BL_PARTIAL   :   1;       // Partial blocks for write allowed [21:21]
  UINT16    WRITE_BL_LEN       :       4;   // Max. write data block length [25:22]
  UINT16    R2W_FACTOR         :         3; // Write speed factor [28:26]
  UINT16    RESERVED_3         :         2; // Reserved [30:29]
  UINT16    WP_GRP_ENABLE      :      1;    // Write protect group enable [31:31]

  UINT32    WP_GRP_SIZE        :        7;  // Write protect group size [38:32]
  UINT32    SECTOR_SIZE        :        7;  // Erase sector size [45:39]
  UINT32    ERASE_BLK_EN       :       1;   // Erase single block enable [46:46]
  UINT32    C_SIZE_MULT        :        3;  // Device size multiplier [49:47]
  UINT32    VDD_W_CURR_MAX     :     3;     // Max. write current @ VDD max [52:50]
  UINT32    VDD_W_CURR_MIN     :     3;     // Max. write current @ VDD min [55:53]
  UINT32    VDD_R_CURR_MAX     :     3;     // Max. read current @ VDD max [58:56]
  UINT32    VDD_R_CURR_MIN     :     3;     // Max. read current @ VDD min [61:59]
  UINT32    C_SIZELow2         :         2; // Device size [63:62]

  UINT32    C_SIZEHigh10       :       10;          // Device size [73:64]
  UINT32    RESERVED_4         :         2;         // Reserved [75:74]
  UINT32    DSR_IMP            :            1;      // DSR implemented [76:76]
  UINT32    READ_BLK_MISALIGN  :  1;                // Read block misalignment [77:77]
  UINT32    WRITE_BLK_MISALIGN : 1;                 // Write block misalignment [78:78]
  UINT32    READ_BL_PARTIAL    :    1;              // Partial blocks for read allowed [79:79]
  UINT32    READ_BL_LEN        :        4;          // Max. read data block length [83:80]
  UINT32    CCC                :                12; // Card command classes [95:84]

  UINT8     TRAN_SPEED;          // Max. bus clock frequency [103:96]
  UINT8     NSAC;                // Data read access-time 2 in CLK cycles (NSAC*100) [111:104]
  UINT8     TAAC;                // Data read access-time 1 [119:112]

  UINT8     RESERVED_5         :         2;  // Reserved [121:120]
  UINT8     SPEC_VERS          :          4; // System specification version [125:122]
  UINT8     CSD_STRUCTURE      :      2;     // CSD structure [127:126]
} CSD;

typedef struct {
  UINT8     RESERVED_1[16];                               // Reserved [15:0]
  UINT8     SECURE_REMOVAL_TYPE;                          // Secure Removal Type [16:16]
  UINT8     PRODUCT_STATE_AWARENESS_ENABLEMENT;           // Product state awareness enablement [17:17]
  UINT8     MAX_PRE_LOADING_DATA_SIZE[4];                 // MAX pre loading data size [21:18]
  UINT8     PRE_LOADING_DATA_SIZE[4];                     // Pre loading data size [25:22]
  UINT8     FFU_STATUS;                                   // FFU Status [26:26]
  UINT8     RESERVED_2[2];                                // Reserved [28:27]
  UINT8     MODE_OPERATION_CODES;                         // Mode operation codes [29:29]
  UINT8     MODE_CONFIG;                                  // Mode config [30:30]
  UINT8     RESERVED_3;                                   // Reserved [31:31]
  UINT8     FLUSH_CACHE;                                  // Flushing of the cache [32:32]
  UINT8     CACHE_CTRL;                                   // Control to turn the cache ON/OFF [33:33]
  UINT8     POWER_OFF_NOTIFICATION;                       // Power Off Notification [34:34]
  UINT8     PACKED_FAILURE_INDEX;                         // Packed command failure index [35:35]
  UINT8     PACKED_COMMAND_STATUS;                        // Packed command status [36:36]
  UINT8     CONTEXT_CONF[15];                             // Context configuration [51:37]
  UINT8     EXT_PARTITIONS_ATTRIBUTE[2];                  // Extended partitions attribute [53:52]
  UINT8     EXCEPTION_EVENTS_STATUS[2];                   // Exception events status [55:54]
  UINT8     EXCEPTION_EVENTS_CTRL[2];                     // Exception events control [57:56]
  UINT8     DYNCAP_NEEDED;                                // Number of addressed group to be released [58:58]
  UINT8     CLASS_6_CTRL;                                 // Class 6 commands control [59:59]
  UINT8     INI_TIMEOUT_EMU;                              // 1st initialization after disabling sector size emulation [60:60]
  UINT8     DATA_SECTOR_SIZE;                             // Sector size [61:61]
  UINT8     USE_NATIVE_SECTOR;                            // Sector size emulation [62:62]
  UINT8     NATIVE_SECTOR_SIZE;                           // Native sector size [63:63]
  UINT8     VENDOR_SPECIFIC_FIELD[64];                    // Vendor specific fields [127:64]
  UINT8     RESERVED_4[2];                                // Reserved [129:128]
  UINT8     PROGRAM_CID_CSD_DDR_SUPPORT;                  // Program CID/CSD in DDR mode support [130:130]
  UINT8     PERIODIC_WAKEUP;                              // Periodic wake-up [131:131]
  UINT8     TCASE_SUPPORT;                                // Package case temperature is controlled [132:132]
  UINT8     PRODUCTION_STATE_AWARENESS;                   // Production state awareness [133:133]
  UINT8     SECTOR_BAD_BLK_MGMNT;                         // Bad block management mode [134:134]
  UINT8     RESERVED_5;                                   // Reserved [135:135]
  UINT8     ENH_START_ADDR[4];                            // Enhanced user data start address [139:136]
  UINT8     ENH_SIZE_MULT[3];                             // Enhanced user data area size [142:140]
  UINT8     GP_SIZE_MULT[12];                             // General purpose partition size [154:143]
  UINT8     PARTITION_SETTING_COMPLETED;                  // Partitioning setting [155:155]
  UINT8     PARTITIONS_ATTRIBUTE;                         // Partitions attribute [156:156]
  UINT8     MAX_ENH_SIZE_MULT[3];                         // Max enhanced area size [159:157]
  UINT8     PARTITIONING_SUPPORT;                         // Partitioning [160:160]
  UINT8     HPI_MGMT;                                     // HPI management [161:161]
  UINT8     RST_N_FUNCTION;                               // H/W reset function [162:162]
  UINT8     BKOPS_EN;                                     // Enable background operations handshake [163:163]
  UINT8     BKOPS_START;                                  // Manually start background operations [164:164]
  UINT8     SANITIZE_START;                               // Start sanitize operation [165:165]
  UINT8     WR_REL_PARAM;                                 // Write reliability parameter register [166:166]
  UINT8     WR_REL_SET;                                   // Write reliability setting register [167:167]
  UINT8     RPMB_SIZE_MULT;                               // RPMB size [168:168]
  UINT8     FW_CONFIG;                                    // FW configuration [169:169]
  UINT8     RESERVED_6;                                   // Reserved [170:170]
  UINT8     USER_WP;                                      // User area write protection register [171:171]
  UINT8     RESERVED_7;                                   // Reserved [172:172]
  UINT8     BOOT_WP;                                      // Boot area write protection register [173:173]
  UINT8     BOOT_WP_STATUS;                               // Boot write protection register [174:174]
  UINT8     ERASE_GROUP_DEF;                              // High-density erase group definition [175:175]
  UINT8     RESERVED_8;                                   // Reserved [176:176]
  UINT8     BOOT_BUS_CONDITIONS;                          // Boot bus conditions [177:177]
  UINT8     BOOT_CONFIG_PROT;                             // Boot config protection [178:178]
  UINT8     PARTITION_CONFIG;                             // Partition config [179:179]
  UINT8     RESERVED_9;                                   // Reserved [180:180]
  UINT8     ERASED_MEM_CONT;                              // Erased memory content [181:181]
  UINT8     RESERVED_10;                                  // Reserved [182:182]
  UINT8     BUS_WIDTH;                                    // Bus width mode [183:183]
  UINT8     RESERVED_11;                                  // Reserved [184:184]
  UINT8     HS_TIMING;                                    // High-speed interface timing [185:185]
  UINT8     RESERVED_12;                                  // Reserved [186:186]
  UINT8     POWER_CLASS;                                  // Power class [187:187]
  UINT8     RESERVED_13;                                  // Reserved [188:188]
  UINT8     CMD_SET_REV;                                  // Command set revision [189:189]
  UINT8     RESERVED_14;                                  // Reserved [190:190]
  UINT8     CMD_SET;                                      // Command set [191:191]
  UINT8     EXT_CSD_REV;                                  // Extended CSD revision [192:192]
  UINT8     RESERVED_15;                                  // Reserved [193:193]
  UINT8     CSD_STRUCTURE;                                // CSD Structure [194:194]
  UINT8     RESERVED_16;                                  // Reserved [195:195]
  UINT8     DEVICE_TYPE;                                  // Device type [196:196]
  UINT8     DRIVER_STRENGTH;                              // I/O Driver strength [197:197]
  UINT8     OUT_OF_INTERRUPT_TIME;                        // Out-of-interrupt busy timing [198:198]
  UINT8     PARTITION_SWITCH_TIME;                        // Partition switching timing [199:199]
  UINT8     PWR_CL_52_195;                                // Power class for 52MHz at 1.95V 1 R [200:200]
  UINT8     PWR_CL_26_195;                                // Power class for 26MHz at 1.95V 1 R [201:201]
  UINT8     PWR_CL_52_360;                                // Power class for 52MHz at 3.6V 1 R [202:202]
  UINT8     PWR_CL_26_360;                                // Power class for 26MHz at 3.6V 1 R [203:203]
  UINT8     RESERVED_17;                                  // Reserved [204:204]
  UINT8     MIN_PERF_R_4_26;                              // Minimum read performance for 4bit at 26MHz [205:205]
  UINT8     MIN_PERF_W_4_26;                              // Minimum write performance for 4bit at 26MHz [206:206]
  UINT8     MIN_PERF_R_8_26_4_52;                         // Minimum read performance for 8bit at 26MHz, for 4bit at 52MHz [207:207]
  UINT8     MIN_PERF_W_8_26_4_52;                         // Minimum write performance for 8bit at 26MHz, for 4bit at 52MHz [208:208]
  UINT8     MIN_PERF_R_8_52;                              // Minimum read performance for 8bit at 52MHz [209:209]
  UINT8     MIN_PERF_W_8_52;                              // Minimum write performance for 8bit at 52MHz [210:210]
  UINT8     RESERVED_18;                                  // Reserved [211:211]
  UINT32    SECTOR_COUNT;                                 // Sector count [215:212]
  UINT8     SLEEP_NOTIFICATION_TIME;                      // Sleep notification timeout [216:216]
  UINT8     S_A_TIMEOUT;                                  // Sleep/awake timeout [217:217]
  UINT8     PRODUCTION_STATE_AWARENESS_TIMEOUT;           // Production state awareness timeout [218:218]
  UINT8     S_C_VCCQ;                                     // Sleep current (VCCQ) [219:219]
  UINT8     S_C_VCC;                                      // Sleep current (VCC) [220:220]
  UINT8     HC_WP_GRP_SIZE;                               // High-capacity write protect group size [221:221]
  UINT8     REL_WR_SECTOR_C;                              // Reliable write sector count [222:222]
  UINT8     ERASE_TIMEOUT_MULT;                           // High-capacity erase timeout [223:223]
  UINT8     HC_ERASE_GRP_SIZE;                            // High-capacity erase unit size [224:224]
  UINT8     ACC_SIZE;                                     // Access size [225:225]
  UINT8     BOOT_SIZE_MULTI;                              // Boot partition size [226:226]
  UINT8     RESERVED_19;                                  // Reserved [227:227]
  UINT8     BOOT_INFO;                                    // Boot information [228:228]
  UINT8     SECURE_TRIM_MULT;                             // Secure TRIM Multiplier [229:229]
  UINT8     SECURE_ERASE_MULT;                            // Secure Erase Multiplier [230:230]
  UINT8     SECURE_FEATURE_SUPPORT;                       // Secure Feature Support [231:231]
  UINT8     TRIM_MULT;                                    // TRIM Multiplier [232:232]
  UINT8     RESERVED_20;                                  // Reserved [233:233]
  UINT8     MIN_PREF_DDR_R_8_52;                          // Minimum read performance for 8bit at 52MHz in DDR mode [234:234]
  UINT8     MIN_PREF_DDR_W_8_52;                          // Minimum write performance for 8bit at 52MHz in DDR mode [235:235]
  UINT8     PWR_CL_200_130;                               // Power class for 200MHz at VCCQ=1.3V, VCC=3.6V [236:236]
  UINT8     PWR_CL_200_195;                               // Power class for 200MHz at VCCQ=1.95V, VCC=3.6V [237:237]
  UINT8     PWR_CL_DDR_52_195;                            // Power class for 52MHz, DDR at 1.95V [238:238]
  UINT8     PWR_CL_DDR_52_360;                            // Power class for 52Mhz, DDR at 3.6V [239:239]
  UINT8     RESERVED_21;                                  // Reserved [240:240]
  UINT8     INI_TIMEOUT_AP;                               // 1st initialization time after partitioning [241:241]
  UINT8     CORRECTLY_PRG_SECTORS_NUM[4];                 // Number of correctly programmed sectors [245:242]
  UINT8     BKOPS_STATUS;                                 // Background operations status [246:246]
  UINT8     POWER_OFF_LONG_TIME;                          // Power off notification (long) timeout [247:247]
  UINT8     GENERIC_CMD6_TIME;                            // Generic CMD6 timeout [248:248]
  UINT8     CACHE_SIZE[4];                                // Cache size [252:249]
  UINT8     PWR_CL_DDR_200_360;                           // Power class for 200MHz, DDR at VCC=3.6V [253:253]
  UINT8     FIRMWARE_VERSION[8];                          // Firmware version [261:254]
  UINT8     DEVICE_VERSION[2];                            // Device version [263:262]
  UINT8     OPTIMAL_TRIM_UNIT_SIZE;                       // Optimal trim unit size [264:264]
  UINT8     OPTIMAL_WRITE_SIZE;                           // Optimal write size [265:265]
  UINT8     OPTIMAL_READ_SIZE;                            // Optimal read size [266:266]
  UINT8     PRE_EOL_INFO;                                 // Pre EOL information [267:267]
  UINT8     DEVICE_LIFE_TIME_EST_TYP_A;                   // Device life time estimation type A [268:268]
  UINT8     DEVICE_LIFE_TIME_EST_TYP_B;                   // Device life time estimation type B [269:269]
  UINT8     VENDOR_PROPRIETARY_HEALTH_REPORT[32];         // Vendor proprietary health report [301:270]
  UINT8     NUMBER_OF_FW_SECTORS_CORRECTLY_PROGRAMMED[4]; // Number of FW sectors correctly programmed [305:302]
  UINT8     RESERVED_22[181];                             // Reserved [486:306]
  UINT8     FFU_ARG[4];                                   // FFU argument [490:487]
  UINT8     OPERATION_CODE_TIMEOUT;                       // Operation codes timeout [491:491]
  UINT8     FFU_FEATURES;                                 // FFU features [492:492]
  UINT8     SUPPORTED_MODES;                              // Supported modes [493:493]
  UINT8     EXT_SUPPORT;                                  // Extended partitions attribute support [494:494]
  UINT8     LARGE_UNIT_SIZE_M1;                           // Large unit size [495:495]
  UINT8     CONTEXT_CAPABILITIES;                         // Context management capabilities [496:496]
  UINT8     TAG_RES_SIZE;                                 // Tag resource size [497:497]
  UINT8     TAG_UNIT_SIZE;                                // Tag unit size [498:498]
  UINT8     DATA_TAG_SUPPORT;                             // Data tag support [499:499]
  UINT8     MAX_PACKED_WRITES;                            // Max packed write commands [500:500]
  UINT8     MAX_PACKED_READS;                             // Max packed read commands [501:501]
  UINT8     BKOPS_SUPPORT;                                // Background operations support [502:502]
  UINT8     HPI_FEATURES;                                 // HPI features [503:503]
  UINT8     S_CMD_SET;                                    // Supported command sets [504:504]
  UINT8     EXT_SECURITY_ERR;                             // Extended security commands error [505:505]
  UINT8     RESERVED_23[6];                               // Reserved [511:506]
} ECSD;

typedef struct  {
  UINT16       RCA;
  CARD_TYPE    CardType;
  OCR          OCRData;
  CID          CIDData;
  CSD          CSDData;
  ECSD         *ECSDData;                      // MMC V4 extended card specific
} CARD_INFO;

typedef struct _MMC_HOST_INSTANCE {
  UINTN                       Signature;
  LIST_ENTRY                  Link;
  EFI_HANDLE                  MmcHandle;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;

  MMC_STATE                   State;
  EFI_BLOCK_IO_PROTOCOL       BlockIo;
  CARD_INFO                   CardInfo;
  EFI_MMC_HOST_PROTOCOL       *MmcHost;

  BOOLEAN                     Initialized;
} MMC_HOST_INSTANCE;

#define MMC_HOST_INSTANCE_SIGNATURE  SIGNATURE_32('m', 'm', 'c', 'h')
#define MMC_HOST_INSTANCE_FROM_BLOCK_IO_THIS(a)  CR (a, MMC_HOST_INSTANCE, BlockIo, MMC_HOST_INSTANCE_SIGNATURE)
#define MMC_HOST_INSTANCE_FROM_LINK(a)           CR (a, MMC_HOST_INSTANCE, Link, MMC_HOST_INSTANCE_SIGNATURE)

EFI_STATUS
EFIAPI
MmcGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
MmcGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

extern EFI_COMPONENT_NAME_PROTOCOL   gMmcComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gMmcComponentName2;

extern EFI_DRIVER_DIAGNOSTICS2_PROTOCOL  gMmcDriverDiagnostics2;

extern LIST_ENTRY  mMmcHostPool;

/**
  Reset the block device.

  This function implements EFI_BLOCK_IO_PROTOCOL.Reset().
  It resets the block device hardware.
  ExtendedVerification is ignored in this implementation.

  @param  This                   Indicates a pointer to the calling context.
  @param  ExtendedVerification   Indicates that the driver may perform a more exhaustive
                                 verification operation of the device during reset.

  @retval EFI_SUCCESS            The block device was reset.
  @retval EFI_DEVICE_ERROR       The block device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
MmcReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  );

/**
  Reads the requested number of blocks from the device.

  This function implements EFI_BLOCK_IO_PROTOCOL.ReadBlocks().
  It reads the requested number of blocks from the device.
  All the blocks are read, or an error is returned.

  @param  This                   Indicates a pointer to the calling context.
  @param  MediaId                The media ID that the read request is for.
  @param  Lba                    The starting logical block address to read from on the device.
  @param  BufferSize             The size of the Buffer in bytes.
                                 This must be a multiple of the intrinsic block size of the device.
  @param  Buffer                 A pointer to the destination buffer for the data. The caller is
                                 responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS            The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to perform the read operation.
  @retval EFI_NO_MEDIA           There is no media in the device.
  @retval EFI_MEDIA_CHANGED      The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE    The BufferSize parameter is not a multiple of the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER  The read request contains LBAs that are not valid,
                                 or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
MmcReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  OUT VOID                  *Buffer
  );

/**
  Writes a specified number of blocks to the device.

  This function implements EFI_BLOCK_IO_PROTOCOL.WriteBlocks().
  It writes a specified number of blocks to the device.
  All blocks are written, or an error is returned.

  @param  This                   Indicates a pointer to the calling context.
  @param  MediaId                The media ID that the write request is for.
  @param  Lba                    The starting logical block address to be written.
  @param  BufferSize             The size of the Buffer in bytes.
                                 This must be a multiple of the intrinsic block size of the device.
  @param  Buffer                 Pointer to the source buffer for the data.

  @retval EFI_SUCCESS            The data were written correctly to the device.
  @retval EFI_WRITE_PROTECTED    The device cannot be written to.
  @retval EFI_NO_MEDIA           There is no media in the device.
  @retval EFI_MEDIA_CHANGED      The MediaId is not for the current media.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to perform the write operation.
  @retval EFI_BAD_BUFFER_SIZE    The BufferSize parameter is not a multiple of the intrinsic
                                 block size of the device.
  @retval EFI_INVALID_PARAMETER  The write request contains LBAs that are not valid,
                                 or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
MmcWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  );

/**
  Flushes all modified data to a physical block device.

  @param  This                   Indicates a pointer to the calling context.

  @retval EFI_SUCCESS            All outstanding data were written correctly to the device.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to write data.
  @retval EFI_NO_MEDIA           There is no media in the device.

**/
EFI_STATUS
EFIAPI
MmcFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  );

EFI_STATUS
MmcNotifyState (
  IN MMC_HOST_INSTANCE  *MmcHostInstance,
  IN MMC_STATE          State
  );

EFI_STATUS
InitializeMmcDevice (
  IN  MMC_HOST_INSTANCE  *MmcHost
  );

VOID
EFIAPI
CheckCardsCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

VOID
PrintCSD (
  IN UINT32  *Csd
  );

VOID
PrintRCA (
  IN UINT32  Rca
  );

VOID
PrintOCR (
  IN UINT32  Ocr
  );

VOID
PrintResponseR1 (
  IN  UINT32  Response
  );

VOID
PrintCID (
  IN UINT32  *Cid
  );

#endif

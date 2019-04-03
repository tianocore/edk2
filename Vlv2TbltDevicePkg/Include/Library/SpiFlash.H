/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   
**/

#ifndef _SPIFlash_H_
#define _SPIFlash_H_

#include <Protocol/Spi.h>

//EFI_STATUS SpiFlashLock(BOOLEAN Lock);
//EFI_STATUS SpiFlashInit(void);

typedef enum {
  EnumSpiFlashW25Q64,
  EnumSpiFlashAT25DF321A,
  EnumSpiFlashAT26DF321,
  EnumSpiFlashAT25DF641,
  EnumSpiFlashW25Q16,
  EnumSpiFlashW25Q32,
  EnumSpiFlashW25X32,
  EnumSpiFlashW25X64,
  EnumSpiFlashW25Q128,
  EnumSpiFlashMX25L16,
  EnumSpiFlashMX25L32,
  EnumSpiFlashMX25L64,
  EnumSpiFlashMX25L128,
  EnumSpiFlashMX25U6435F,
  EnumSpiFlashSST25VF016B,
  EnumSpiFlashSST25VF064C,
  EnumSpiFlashN25Q064,
  EnumSpiFlashM25PX16,
  EnumSpiFlashN25Q032,
  EnumSpiFlashM25PX32,
  EnumSpiFlashM25PX64,
  EnumSpiFlashN25Q128,
  EnumSpiFlashEN25Q16,
  EnumSpiFlashEN25Q32,
  EnumSpiFlashEN25Q64,
  EnumSpiFlashEN25Q128,
  EnumSpiFlashA25L016,
  EnumSpiFlashMax
} SPI_FLASH_TYPES_SUPPORTED;

//
// Serial Flash VendorId and DeviceId
//
#define SF_VENDOR_ID_ATMEL          0x1F
#define SF_DEVICE_ID0_AT26DF321     0x47
#define SF_DEVICE_ID1_AT26DF321     0x00
#define SF_DEVICE_ID0_AT25DF321A    0x47
#define SF_DEVICE_ID1_AT25DF321A    0x01
#define SF_DEVICE_ID0_AT25DF641     0x48
#define SF_DEVICE_ID1_AT25DF641     0x00

#define SF_VENDOR_ID_WINBOND        0xEF
#define SF_DEVICE_ID0_W25XXX        0x30
#define SF_DEVICE_ID1_W25X32        0x16
#define SF_DEVICE_ID1_W25X64        0x17
#define SF_DEVICE_ID0_W25QXX        0x40
#define SF_DEVICE_ID1_W25Q16        0x15
#define SF_DEVICE_ID1_W25Q32        0x16
#define SF_DEVICE_ID1_W25Q64        0x17
#define SF_DEVICE_ID1_W25Q128       0x18

#define	SF_VENDOR_ID_MACRONIX       0xC2
#define	SF_DEVICE_ID0_MX25LXX       0x20
#define	SF_DEVICE_ID1_MX25L16       0x15
#define	SF_DEVICE_ID1_MX25L32       0x16
#define	SF_DEVICE_ID1_MX25L64       0x17
#define	SF_DEVICE_ID1_MX25L128      0x18
#define SF_DEVICE_ID0_MX25UXX       0x25
#define SF_DEVICE_ID1_MX25U6435F    0x37

#define	SF_VENDOR_ID_NUMONYX        0x20
#define SF_DEVICE_ID0_N25Q064       0xBB
#define SF_DEVICE_ID1_N25Q064       0x17
#define	SF_DEVICE_ID0_M25PXXX       0x71
#define	SF_DEVICE_ID0_N25QXXX       0xBA
#define	SF_DEVICE_ID1_M25PX16       0x15
#define	SF_DEVICE_ID1_N25Q032       0x16
#define	SF_DEVICE_ID1_M25PX32       0x16
#define	SF_DEVICE_ID1_M25PX64       0x17
#define	SF_DEVICE_ID1_N25Q128       0x18

#define SF_VENDOR_ID_SST            0xBF
#define SF_DEVICE_ID0_SST25VF0XXX   0x25
#define SF_DEVICE_ID1_SST25VF016B   0x41
#define SF_DEVICE_ID1_SST25VF064C   0x4B

#define SF_VENDOR_ID_EON            0x1C
#define SF_DEVICE_ID0_EN25QXX       0x30
#define SF_DEVICE_ID1_EN25Q16       0x15
#define SF_DEVICE_ID1_EN25Q32       0x16
#define SF_DEVICE_ID1_EN25Q64       0x17
#define SF_DEVICE_ID1_EN25Q128      0x18

#define SF_VENDOR_ID_AMIC           0x37
#define SF_DEVICE_ID0_A25L016       0x30
#define SF_DEVICE_ID1_A25L016       0x15

#define ATMEL_AT26DF321_SIZE        0x00400000
#define ATMEL_AT25DF321A_SIZE       0x00400000
#define ATMEL_AT25DF641_SIZE        0x00800000
#define WINBOND_W25X32_SIZE         0x00400000
#define WINBOND_W25X64_SIZE         0x00800000
#define WINBOND_W25Q16_SIZE         0x00200000
#define WINBOND_W25Q32_SIZE         0x00400000
#define WINBOND_W25Q64_SIZE         0x00800000
#define WINBOND_W25Q128_SIZE        0x01000000
#define SST_SST25VF016B_SIZE        0x00200000
#define SST_SST25VF064C_SIZE        0x00800000
#define MACRONIX_MX25L16_SIZE       0x00200000
#define MACRONIX_MX25L32_SIZE       0x00400000
#define MACRONIX_MX25L64_SIZE       0x00800000
#define MACRONIX_MX25U64_SIZE       0x00800000
#define MACRONIX_MX25L128_SIZE      0x01000000
#define NUMONYX_M25PX16_SIZE        0x00400000
#define NUMONYX_N25Q032_SIZE        0x00400000
#define NUMONYX_M25PX32_SIZE        0x00400000
#define NUMONYX_M25PX64_SIZE        0x00800000
#define NUMONYX_N25Q064_SIZE        0x00800000
#define NUMONYX_N25Q128_SIZE        0x01000000
#define EON_EN25Q16_SIZE            0x00200000
#define EON_EN25Q32_SIZE            0x00400000
#define EON_EN25Q64_SIZE            0x00800000
#define EON_EN25Q128_SIZE           0x01000000
#define AMIC_A25L16_SIZE            0x00200000

#define SF_VENDOR_ID_SST           0xBF
#define SF_DEVICE_ID0_25LF080A     0x25
#define SF_DEVICE_ID1_25LF080A     0x8E
#define SF_DEVICE_ID0_25VF016B     0x25
#define SF_DEVICE_ID1_25VF016B     0x41

#define SF_VENDOR_ID_ATMEL         0x1F
#define SF_DEVICE_ID0_AT26DF321    0x47
#define SF_DEVICE_ID1_AT26DF321    0x00

#define SF_VENDOR_ID_STM           0x20
#define SF_DEVICE_ID0_M25P32       0x20
#define SF_DEVICE_ID1_M25P32       0x16

#define SF_VENDOR_ID_WINBOND       0xEF
#define SF_DEVICE_ID0_W25XXX       0x30

#define SF_DEVICE_ID1_W25X80       0x14
#define SF_DEVICE_ID1_W25X16       0x15
#define SF_DEVICE_ID1_W25X32       0x16
#define SF_DEVICE_ID1_W25X64       0x17

#define	SF_VENDOR_ID_MX				0xC2
#define	SF_DEVICE_ID0_25L1605A		0x20
#define	SF_DEVICE_ID1_25L1605A		0x15

#define SF_VENDOR_ID_NUMONYX        0x20
#define SF_DEVICE_ID0_M25PX16       0x71
#define SF_DEVICE_ID1_M25PX16       0x15

#define SST_25LF080A_SIZE          0x00100000
#define SST_25LF016B_SIZE          0x00200000
#define ATMEL_AT26DF321_SIZE       0x00400000
#define STM_M25P32_SIZE            0x00400000
#define WINBOND_W25X80_SIZE        0x00100000
#define WINBOND_W25X16_SIZE        0x00200000
#define WINBOND_W25X32_SIZE        0x00400000
#define WINBOND_W25X64_SIZE        0x00800000
#define MX_25L1605A_SIZE           0x00200000

//
// Physical Sector Size on the Serial Flash device
//
#define SF_SECTOR_SIZE    0x1000
#define SF_BLOCK_SIZE     0x8000

//
// Serial Flash Status Register definitions
//
#define SF_SR_BUSY        0x01      // Indicates if internal write operation is in progress
#define SF_SR_WEL         0x02      // Indicates if device is memory write enabled
#define SF_SR_BP0         0x04      // Block protection bit 0
#define SF_SR_BP1         0x08      // Block protection bit 1
#define SF_SR_BP2         0x10      // Block protection bit 2
#define SF_SR_BP3         0x20      // Block protection bit 3
#define SF_SR_WPE         0x3C      // Enable write protection on all blocks
#define SF_SR_AAI         0x40      // Auto Address Increment Programming status
#define SF_SR_BPL         0x80      // Block protection lock-down

//
// Operation Instruction definitions for  the Serial Flash Device
//
#define SF_INST_WRSR            0x01     // Write Status Register
#define SF_INST_PROG            0x02     // Byte Program
#define SF_INST_READ            0x03     // Read
#define SF_INST_WRDI            0x04     // Write Disable
#define SF_INST_RDSR            0x05     // Read Status Register
#define SF_INST_WREN            0x06     // Write Enable
#define SF_INST_HS_READ         0x0B     // High-speed Read
#define SF_INST_SERASE          0x20     // Sector Erase (4KB)
#define SF_INST_BERASE          0x52     // Block Erase (32KB)
#define SF_INST_64KB_ERASE      0xD8     // Block Erase (64KB)
#define SF_INST_EWSR            0x50     // Enable Write Status Register
#define SF_INST_READ_ID         0xAB     // Read ID
#define SF_INST_JEDEC_READ_ID   0x9F     // JEDEC Read ID
#define SF_INST_DOFR            0x3B     // Dual Output Fast Read
#define SF_INST_SFDP            0x5A     // Serial Flash Discovery Parameters

#define SECTOR_SIZE_4KB 	0x1000       // Common 4kBytes sector size
#define SECTOR_SIZE_64KB	0x10000      // Common 64kBytes sector size
#define BLOCK_SIZE_64KB 	0x00010000   // Common 64kBytes block size
#define MAX_FWH_SIZE    	0x00100000   // 8Mbit (Note that this can also be used for the 4Mbit )

//
// Prefix Opcode Index on the host SPI controller
//
typedef enum {
  SPI_WREN,             // Prefix Opcode 0: Write Enable
  SPI_EWSR,             // Prefix Opcode 1: Enable Write Status Register
} PREFIX_OPCODE_INDEX;

//
// Opcode Menu Index on the host SPI controller
//
typedef enum {
  SPI_READ_ID,        // Opcode 0: READ ID, Read cycle with address
  SPI_READ,           // Opcode 1: READ, Read cycle with address
  SPI_RDSR,           // Opcode 2: Read Status Register, No address
  SPI_WRDI_SFDP,      // Opcode 3: Write Disable or Discovery Parameters, No address
  SPI_SERASE,         // Opcode 4: Sector Erase (4KB), Write cycle with address
  SPI_BERASE,         // Opcode 5: Block Erase (32KB), Write cycle with address
  SPI_PROG,           // Opcode 6: Byte Program, Write cycle with address
  SPI_WRSR,           // Opcode 7: Write Status Register, No address
} SPI_OPCODE_INDEX;

#endif

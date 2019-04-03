/** @file

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _SPI_FLASH_DEVICE_H_
#define _SPI_FLASH_DEVICE_H_

#include <PiDxe.h>
#include <Protocol/Spi.h>
#include <Protocol/FirmwareVolumeBlock.h>

//
// Supported SPI Flash Devices
//
typedef enum {
  EnumSpiFlash25L3205D,   // Macronix 32Mbit part
  EnumSpiFlashW25Q32,     // Winbond 32Mbit part
  EnumSpiFlashW25X32,     // Winbond 32Mbit part
  EnumSpiFlashAT25DF321,  // Atmel 32Mbit part
  EnumSpiFlashQH25F320,   // Intel 32Mbit part
  EnumSpiFlash25VF064C,   // SST 64Mbit part
  EnumSpiFlashM25PX64,    // NUMONYX 64Mbit part
  EnumSpiFlashAT25DF641,  // Atmel 64Mbit part
  EnumSpiFlashS25FL064K,  // Spansion 64Mbit part
  EnumSpiFlash25L6405D,   // Macronix 64Mbit part
  EnumSpiFlashW25Q64,     // Winbond 64Mbit part
  EnumSpiFlashW25X64,     // Winbond 64Mbit part
  EnumSpiFlashQH25F640,   // Intel 64Mbit part
  EnumSpiFlashMax
} SPI_FLASH_TYPES_SUPPORTED;

//
// Flash Device commands
//
// If a supported device uses a command different from the list below, a device specific command
// will be defined just below it's JEDEC id section.
//
#define SPI_COMMAND_WRITE                 0x02
#define SPI_COMMAND_WRITE_AAI             0xAD
#define SPI_COMMAND_READ                  0x03
#define SPI_COMMAND_ERASE                 0x20
#define SPI_COMMAND_WRITE_DISABLE         0x04
#define SPI_COMMAND_READ_S                0x05
#define SPI_COMMAND_WRITE_ENABLE          0x06
#define SPI_COMMAND_READ_ID               0xAB
#define SPI_COMMAND_JEDEC_ID              0x9F
#define SPI_COMMAND_WRITE_S_EN            0x50
#define SPI_COMMAND_WRITE_S               0x01
#define SPI_COMMAND_CHIP_ERASE            0xC7
#define SPI_COMMAND_BLOCK_ERASE           0xD8

//
// Flash JEDEC device ids
//
// SST 8Mbit part
//
#define SPI_SST25VF080B_ID1               0xBF
#define SPI_SST25VF080B_ID2               0x25
#define SPI_SST25VF080B_ID3               0x8E
//
// SST 16Mbit part
//
#define SPI_SST25VF016B_ID1               0xBF
#define SPI_SST25VF016B_ID2               0x25
#define SPI_SST25V016BF_ID3               0x41
//
// Macronix 32Mbit part
//
// MX25 part does not support WRITE_AAI comand (0xAD)
//
#define SPI_MX25L3205_ID1                 0xC2
#define SPI_MX25L3205_ID2                 0x20
#define SPI_MX25L3205_ID3                 0x16
//
// Intel 32Mbit part bottom boot
//
#define SPI_QH25F320_ID1                  0x89
#define SPI_QH25F320_ID2                  0x89
#define SPI_QH25F320_ID3                  0x12  // 32Mbit bottom boot
//
// Intel 64Mbit part bottom boot
//
#define SPI_QH25F640_ID1                  0x89
#define SPI_QH25F640_ID2                  0x89
#define SPI_QH25F640_ID3                  0x13  // 64Mbit bottom boot
//
// QH part does not support command 0x20 for erase; only 0xD8 (sector erase)
// QH part has 0x40 command for erase of parameter block (8 x 8K blocks at bottom of part)
// 0x40 command ignored if address outside of parameter block range
//
#define SPI_QH25F320_COMMAND_PBLOCK_ERASE 0x40
//
// Winbond 32Mbit part
//
#define SPI_W25X32_ID1                    0xEF
#define SPI_W25X32_ID2                    0x30  // Memory Type
#define SPI_W25X32_ID3                    0x16  // Capacity
#define SF_DEVICE_ID1_W25Q32              0x16

//
// Winbond 64Mbit part
//
#define SPI_W25X64_ID1                    0xEF
#define SPI_W25X64_ID2                    0x30  // Memory Type
#define SPI_W25X64_ID3                    0x17  // Capacity
#define SF_DEVICE_ID0_W25QXX              0x40
#define SF_DEVICE_ID1_W25Q64              0x17
//
// Winbond 128Mbit part
//
#define SF_DEVICE_ID0_W25Q128             0x40
#define SF_DEVICE_ID1_W25Q128             0x18

//
// Atmel 32Mbit part
//
#define SPI_AT26DF321_ID1                 0x1F
#define SPI_AT26DF321_ID2                 0x47  // [7:5]=Family, [4:0]=Density
#define SPI_AT26DF321_ID3                 0x00

#define SF_VENDOR_ID_ATMEL                0x1F
#define SF_DEVICE_ID0_AT25DF641           0x48
#define SF_DEVICE_ID1_AT25DF641           0x00

//
// SST 8Mbit part
//
#define SPI_SST25VF080B_ID1               0xBF
#define SPI_SST25VF080B_ID2               0x25
#define SPI_SST25VF080B_ID3               0x8E
#define SF_DEVICE_ID0_25VF064C            0x25
#define SF_DEVICE_ID1_25VF064C            0x4B

//
// SST 16Mbit part
//
#define SPI_SST25VF016B_ID1               0xBF
#define SPI_SST25VF016B_ID2               0x25
#define SPI_SST25V016BF_ID3               0x41

//
// Winbond 32Mbit part
//
#define SPI_W25X32_ID1                    0xEF
#define SPI_W25X32_ID2                    0x30  // Memory Type
#define SPI_W25X32_ID3                    0x16  // Capacity

#define  SF_VENDOR_ID_MX             0xC2
#define  SF_DEVICE_ID0_25L6405D      0x20
#define  SF_DEVICE_ID1_25L6405D      0x17

#define  SF_VENDOR_ID_NUMONYX        0x20
#define  SF_DEVICE_ID0_M25PX64       0x71
#define  SF_DEVICE_ID1_M25PX64       0x17

//
// Spansion 64Mbit part
//
#define SF_VENDOR_ID_SPANSION             0xEF
#define SF_DEVICE_ID0_S25FL064K           0x40
#define SF_DEVICE_ID1_S25FL064K           0x00

//
// index for prefix opcodes
//
#define SPI_WREN_INDEX                    0   // Prefix Opcode 0: SPI_COMMAND_WRITE_ENABLE
#define SPI_EWSR_INDEX                    1   // Prefix Opcode 1: SPI_COMMAND_WRITE_S_EN
#define BIOS_CTRL                         0xDC

#define PFAB_CARD_DEVICE_ID               0x5150
#define PFAB_CARD_VENDOR_ID               0x8086
#define PFAB_CARD_SETUP_REGISTER          0x40
#define PFAB_CARD_SETUP_BYTE              0x0d


#endif

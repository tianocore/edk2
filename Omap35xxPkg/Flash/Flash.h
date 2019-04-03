/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FLASH_H
#define FLASH_H

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/Cpu.h>
#include <Omap3530/Omap3530.h>

#define PAGE_SIZE(x)             ((x) & 0x01)
#define PAGE_SIZE_2K_VAL         (0x01UL)

#define SPARE_AREA_SIZE(x)       (((x) >> 2) & 0x01)
#define SPARE_AREA_SIZE_64B_VAL  (0x1UL)

#define BLOCK_SIZE(x)            (((x) >> 4) & 0x01)
#define BLOCK_SIZE_128K_VAL      (0x01UL)

#define ORGANIZATION(x)          (((x) >> 6) & 0x01)
#define ORGANIZATION_X8          (0x0UL)
#define ORGANIZATION_X16         (0x1UL)

#define PAGE_SIZE_512B           (512)
#define PAGE_SIZE_2K             (2048)
#define PAGE_SIZE_4K             (4096)
#define SPARE_AREA_SIZE_16B      (16)
#define SPARE_AREA_SIZE_64B      (64)

#define BLOCK_SIZE_16K           (16*1024)
#define BLOCK_SIZE_128K          (128*1024)

#define BLOCK_COUNT              (2048)
#define LAST_BLOCK               (BLOCK_COUNT - 1)

#define ECC_POSITION             2

//List of commands.
#define RESET_CMD                0xFF
#define READ_ID_CMD              0x90

#define READ_STATUS_CMD          0x70

#define PAGE_READ_CMD            0x00
#define PAGE_READ_CONFIRM_CMD    0x30

#define BLOCK_ERASE_CMD          0x60
#define BLOCK_ERASE_CONFIRM_CMD  0xD0

#define PROGRAM_PAGE_CMD         0x80
#define PROGRAM_PAGE_CONFIRM_CMD 0x10

//Nand status register bit definition
#define NAND_SUCCESS             (0x0UL << 0)
#define NAND_FAILURE             BIT0

#define NAND_BUSY                (0x0UL << 6)
#define NAND_READY               BIT6

#define NAND_RESET_STATUS        (0x60UL << 0)

#define MAX_RETRY_COUNT          1500


typedef struct {
  UINT8 ManufactureId;
  UINT8 DeviceId;
  UINT8 BlockAddressStart; //Start of the Block address in actual NAND
  UINT8 PageAddressStart;  //Start of the Page address in actual NAND
} NAND_PART_INFO_TABLE;

typedef struct {
  UINT8     ManufactureId;
  UINT8     DeviceId;
  UINT8     Organization;      //x8 or x16
  UINT32    PageSize;
  UINT32    SparePageSize;
  UINT32    BlockSize;
  UINT32    NumPagesPerBlock;
  UINT8     BlockAddressStart; //Start of the Block address in actual NAND
  UINT8     PageAddressStart;  //Start of the Page address in actual NAND
} NAND_FLASH_INFO;

#endif //FLASH_H

/** @file
*
*  Copyright (c) 2017 Marvell International Ltd.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef __NOR_FLASH_ID_LIB_H__
#define __NOR_FLASH_ID_LIB_H__

#include <Uefi/UefiBaseType.h>

#define NOR_FLASH_MAX_ID_LEN  6

typedef struct {
  /* Device name */
  UINT16    *Name;

  /*
   * JEDEC ID
   */
  UINT8     Id[NOR_FLASH_MAX_ID_LEN];
  UINT8     IdLen;

  UINT16    PageSize;

  /*
   * Below parameters can be referred as BlockSize
   * and BlockCount, when treating the NorFlash as
   * block device.
   */
  UINT32    SectorSize;
  UINT32    SectorCount;

  UINT16    Flags;
  #define NOR_FLASH_ERASE_4K   (1 << 0)  /* Use 4 KB erase blocks and CMD_ERASE_4K */
  #define NOR_FLASH_ERASE_32K  (1 << 1)  /* Use 32 KB erase blocks and CMD_ERASE_32K */
  #define NOR_FLASH_WRITE_FSR  (1 << 2)  /* Use flag status register for write */
  #define NOR_FLASH_4B_ADDR    (1 << 3)  /* Use 4B addressing */
} NOR_FLASH_INFO;

/* Vendor IDs */
#define NOR_FLASH_ID_ATMEL       0x1f
#define NOR_FLASH_ID_EON         0x1c
#define NOR_FLASH_ID_GIGADEVICE  0xc8
#define NOR_FLASH_ID_ISSI        0x9d
#define NOR_FLASH_ID_MACRONIX    0xc2
#define NOR_FLASH_ID_SPANSION    0x01
#define NOR_FLASH_ID_STMICRO     0x20
#define NOR_FLASH_ID_SST         0xbf
#define NOR_FLASH_ID_WINDBOND    0xef

/**
  Return an allocated copy pool of the NOR flash information structure.

  @param[in]       Id                 Pointer to an array with JEDEC ID obtained
                                      from the NOR flash with READ_ID command
                                      (0x9f)
  @param[in out]   FlashInfo          Pointer to NOR flash information structure
  @param[in]       AllocateForRuntime A flag specifying a type of a copy pool
                                      allocation (TRUE for runtime, FALSE for
                                      normal)

  @retval       EFI_SUCCESS           Operation completed successfully
  @retval       EFI_NOT_FOUND         No matching entry in NOR ID table found
  @retval       EFI_OUT_OF_RESOURCES  No pool memory available

**/
EFI_STATUS
EFIAPI
NorFlashGetInfo (
  IN UINT8               *Id,
  IN OUT NOR_FLASH_INFO  **FlashInfo,
  IN BOOLEAN             AllocateForRuntime
  );

/**
  Print NOR flash information basing on data stored in
  the NOR_FLASH_INFO structure.

  @param[in]       FlashInfo          Pointer to NOR flash information structure

**/
VOID
EFIAPI
NorFlashPrintInfo (
  IN     NOR_FLASH_INFO  *Info
  );

#endif

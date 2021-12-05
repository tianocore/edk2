/** @file
*
*  Copyright (c) 2017 Marvell International Ltd.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NorFlashInfoLib.h>

STATIC CONST NOR_FLASH_INFO  NorFlashIds[] = {
  /* ATMEL */
  { L"at45db011d",     { 0x1f, 0x22, 0x00 }, 3, 256, 64 * 1024,  4,    NOR_FLASH_ERASE_4K                       },
  { L"at45db021d",     { 0x1f, 0x23, 0x00 }, 3, 256, 64 * 1024,  8,    NOR_FLASH_ERASE_4K                       },
  { L"at45db041d",     { 0x1f, 0x24, 0x00 }, 3, 256, 64 * 1024,  8,    NOR_FLASH_ERASE_4K                       },
  { L"at45db081d",     { 0x1f, 0x25, 0x00 }, 3, 256, 64 * 1024,  16,   NOR_FLASH_ERASE_4K                       },
  { L"at45db161d",     { 0x1f, 0x26, 0x00 }, 3, 256, 64 * 1024,  32,   NOR_FLASH_ERASE_4K                       },
  { L"at45db321d",     { 0x1f, 0x27, 0x00 }, 3, 256, 64 * 1024,  64,   NOR_FLASH_ERASE_4K                       },
  { L"at45db641d",     { 0x1f, 0x28, 0x00 }, 3, 256, 64 * 1024,  128,  NOR_FLASH_ERASE_4K                       },
  { L"at25df321a",     { 0x1f, 0x47, 0x01 }, 3, 256, 64 * 1024,  64,   NOR_FLASH_ERASE_4K                       },
  { L"at25df321",      { 0x1f, 0x47, 0x00 }, 3, 256, 64 * 1024,  64,   NOR_FLASH_ERASE_4K                       },
  { L"at26df081a",     { 0x1f, 0x45, 0x01 }, 3, 256, 64 * 1024,  16,   NOR_FLASH_ERASE_4K                       },
  /* EON */
  { L"en25q32b",       { 0x1c, 0x30, 0x16 }, 3, 256, 64 * 1024,  64,   0                                        },
  { L"en25q64",        { 0x1c, 0x30, 0x17 }, 3, 256, 64 * 1024,  128,  NOR_FLASH_ERASE_4K                       },
  { L"en25q128b",      { 0x1c, 0x30, 0x18 }, 3, 256, 64 * 1024,  256,  0                                        },
  { L"en25s64",        { 0x1c, 0x38, 0x17 }, 3, 256, 64 * 1024,  128,  0                                        },
  /* GIGADEVICE */
  { L"gd25q64b",       { 0xc8, 0x40, 0x17 }, 3, 256, 64 * 1024,  128,  NOR_FLASH_ERASE_4K                       },
  { L"gd25lq32",       { 0xc8, 0x60, 0x16 }, 3, 256, 64 * 1024,  64,   NOR_FLASH_ERASE_4K                       },
  /* ISSI */
  { L"is25lp032",      { 0x9d, 0x60, 0x16 }, 3, 256, 64 * 1024,  64,   0                                        },
  { L"is25lp064",      { 0x9d, 0x60, 0x17 }, 3, 256, 64 * 1024,  128,  0                                        },
  { L"is25lp128",      { 0x9d, 0x60, 0x18 }, 3, 256, 64 * 1024,  256,  0                                        },
  /* MACRONIX */
  { L"mx25l2006e",     { 0xc2, 0x20, 0x12 }, 3, 256, 64 * 1024,  4,    0                                        },
  { L"mx25l4005",      { 0xc2, 0x20, 0x13 }, 3, 256, 64 * 1024,  8,    0                                        },
  { L"mx25l8005",      { 0xc2, 0x20, 0x14 }, 3, 256, 64 * 1024,  16,   0                                        },
  { L"mx25l1605d",     { 0xc2, 0x20, 0x15 }, 3, 256, 64 * 1024,  32,   0                                        },
  { L"mx25l3205d",     { 0xc2, 0x20, 0x16 }, 3, 256, 64 * 1024,  64,   0                                        },
  { L"mx25l6405d",     { 0xc2, 0x20, 0x17 }, 3, 256, 64 * 1024,  128,  0                                        },
  { L"mx25l12805",     { 0xc2, 0x20, 0x18 }, 3, 256, 64 * 1024,  256,  0                                        },
  { L"mx25l25635f",    { 0xc2, 0x20, 0x19 }, 3, 256, 64 * 1024,  512,  0                                        },
  { L"mx25l51235f",    { 0xc2, 0x20, 0x1a }, 3, 256, 64 * 1024,  1024, 0                                        },
  { L"mx25l12855e",    { 0xc2, 0x26, 0x18 }, 3, 256, 64 * 1024,  256,  0                                        },
  { L"mx66u51235f",    { 0xc2, 0x25, 0x3a }, 3, 256, 64 * 1024,  1024, 0                                        },
  { L"mx66u1g45g",     { 0xc2, 0x25, 0x3b }, 3, 256, 64 * 1024,  2048, 0                                        },
  { L"mx66l1g45g",     { 0xc2, 0x20, 0x1b }, 3, 256, 64 * 1024,  2048, 0                                        },
  /* SPANSION */
  { L"s25fl008a",      { 0x01, 0x02, 0x13 }, 3, 256, 64 * 1024,  16,   0                                        },
  { L"s25fl016a",      { 0x01, 0x02, 0x14 }, 3, 256, 64 * 1024,  32,   0                                        },
  { L"s25fl032a",      { 0x01, 0x02, 0x15 }, 3, 256, 64 * 1024,  64,   0                                        },
  { L"s25fl064a",      { 0x01, 0x02, 0x16 }, 3, 256, 64 * 1024,  128,  0                                        },
  { L"s25fl116k",      { 0x01, 0x40, 0x15 }, 3, 256, 64 * 1024,  128,  0                                        },
  { L"s25fl164k",      { 0x01, 0x40, 0x17, 0x01, 0x40}, 5, 256, 64 * 1024,  128,  0                                        },
  { L"s25fl128p_256k", { 0x01, 0x20, 0x18, 0x03, 0x00}, 5, 256, 256 * 1024, 64,   0                                        },
  { L"s25fl128p_64k",  { 0x01, 0x20, 0x18, 0x03, 0x01}, 5, 256, 64 * 1024,  256,  0                                        },
  { L"s25fl032p",      { 0x01, 0x02, 0x15, 0x4d, 0x00}, 5, 256, 64 * 1024,  64,   0                                        },
  { L"s25fl064p",      { 0x01, 0x02, 0x16, 0x4d, 0x00}, 5, 256, 64 * 1024,  128,  0                                        },
  { L"s25fl128s_256k", { 0x01, 0x20, 0x18, 0x4d, 0x00}, 5, 256, 256 * 1024, 64,   0                                        },
  { L"s25fl128s_64k",  { 0x01, 0x20, 0x18, 0x4d, 0x01}, 5, 256, 64 * 1024,  256,  0                                        },
  { L"s25fl256s_256k", { 0x01, 0x02, 0x19, 0x4d, 0x00}, 5, 256, 256 * 1024, 128,  0                                        },
  { L"s25fl256s_64k",  { 0x01, 0x02, 0x19, 0x4d, 0x01}, 5, 256, 64 * 1024,  512,  0                                        },
  { L"s25fl512s_256k", { 0x01, 0x02, 0x20, 0x4d, 0x00}, 5, 256, 256 * 1024, 256,  0                                        },
  { L"s25fl512s_64k",  { 0x01, 0x02, 0x20, 0x4d, 0x01}, 5, 256, 64 * 1024,  1024, 0                                        },
  { L"s25fl512s_512k", { 0x01, 0x02, 0x20, 0x4f, 0x00}, 5, 256, 256 * 1024, 256,  0                                        },
  /* STMICRO */
  { L"m25p10",         { 0x20, 0x20, 0x11 }, 3, 256, 32 * 1024,  4,    0                                        },
  { L"m25p20",         { 0x20, 0x20, 0x12 }, 3, 256, 64 * 1024,  4,    0                                        },
  { L"m25p40",         { 0x20, 0x20, 0x13 }, 3, 256, 64 * 1024,  8,    0                                        },
  { L"m25p80",         { 0x20, 0x20, 0x14 }, 3, 256, 64 * 1024,  16,   0                                        },
  { L"m25p16",         { 0x20, 0x20, 0x15 }, 3, 256, 64 * 1024,  32,   0                                        },
  { L"m25pE16",        { 0x20, 0x80, 0x15, 0x10, 0x00}, 5, 256, 64 * 1024,  32,   0                                        },
  { L"m25pX16",        { 0x20, 0x71, 0x15, 0x10, 0x00}, 5, 256, 64 * 1024,  32,   0                                        },
  { L"m25p32",         { 0x20, 0x20, 0x16 }, 3, 256, 64 * 1024,  64,   0                                        },
  { L"m25p64",         { 0x20, 0x20, 0x17 }, 3, 256, 64 * 1024,  128,  0                                        },
  { L"m25p128",        { 0x20, 0x20, 0x18 }, 3, 256, 256 * 1024, 64,   0                                        },
  { L"m25pX64",        { 0x20, 0x71, 0x17 }, 3, 256, 64 * 1024,  128,  NOR_FLASH_ERASE_4K                       },
  { L"n25q016a",       { 0x20, 0xbb, 0x15 }, 3, 256, 64 * 1024,  32,   NOR_FLASH_ERASE_4K                       },
  { L"n25q32",         { 0x20, 0xba, 0x16 }, 3, 256, 64 * 1024,  64,   NOR_FLASH_ERASE_4K                       },
  { L"n25q32a",        { 0x20, 0xbb, 0x16 }, 3, 256, 64 * 1024,  64,   NOR_FLASH_ERASE_4K                       },
  { L"n25q64",         { 0x20, 0xba, 0x17 }, 3, 256, 64 * 1024,  128,  NOR_FLASH_ERASE_4K                       },
  { L"n25q64a",        { 0x20, 0xbb, 0x17 }, 3, 256, 64 * 1024,  128,  NOR_FLASH_ERASE_4K                       },
  { L"n25q128",        { 0x20, 0xba, 0x18 }, 3, 256, 64 * 1024,  256,  0                                        },
  { L"n25q128a",       { 0x20, 0xbb, 0x18 }, 3, 256, 64 * 1024,  256,  0                                        },
  { L"n25q256",        { 0x20, 0xba, 0x19 }, 3, 256, 64 * 1024,  512,  NOR_FLASH_ERASE_4K                       },
  { L"n25q256a",       { 0x20, 0xbb, 0x19 }, 3, 256, 64 * 1024,  512,  NOR_FLASH_ERASE_4K                       },
  { L"n25q512",        { 0x20, 0xba, 0x20 }, 3, 256, 64 * 1024,  1024, NOR_FLASH_WRITE_FSR | NOR_FLASH_ERASE_4K },
  { L"n25q512a",       { 0x20, 0xbb, 0x20 }, 3, 256, 64 * 1024,  1024, NOR_FLASH_WRITE_FSR | NOR_FLASH_ERASE_4K },
  { L"n25q1024",       { 0x20, 0xba, 0x21 }, 3, 256, 64 * 1024,  2048, NOR_FLASH_WRITE_FSR | NOR_FLASH_ERASE_4K },
  { L"n25q1024a",      { 0x20, 0xbb, 0x21 }, 3, 256, 64 * 1024,  2048, NOR_FLASH_WRITE_FSR | NOR_FLASH_ERASE_4K },
  { L"mt25qu02g",      { 0x20, 0xbb, 0x22 }, 3, 256, 64 * 1024,  4096, NOR_FLASH_WRITE_FSR | NOR_FLASH_ERASE_4K },
  { L"mt25ql02g",      { 0x20, 0xba, 0x22 }, 3, 256, 64 * 1024,  4096, NOR_FLASH_WRITE_FSR | NOR_FLASH_ERASE_4K },
  /* SST */
  { L"sst25vf040b",    { 0xbf, 0x25, 0x8d }, 3, 256, 64 * 1024,  8,    NOR_FLASH_ERASE_4K                       },
  { L"sst25vf080b",    { 0xbf, 0x25, 0x8e }, 3, 256, 64 * 1024,  16,   NOR_FLASH_ERASE_4K                       },
  { L"sst25vf016b",    { 0xbf, 0x25, 0x41 }, 3, 256, 64 * 1024,  32,   NOR_FLASH_ERASE_4K                       },
  { L"sst25vf032b",    { 0xbf, 0x25, 0x4a }, 3, 256, 64 * 1024,  64,   NOR_FLASH_ERASE_4K                       },
  { L"sst25vf064c",    { 0xbf, 0x25, 0x4b }, 3, 256, 64 * 1024,  128,  NOR_FLASH_ERASE_4K                       },
  { L"sst25wf512",     { 0xbf, 0x25, 0x01 }, 3, 256, 64 * 1024,  1,    NOR_FLASH_ERASE_4K                       },
  { L"sst25wf010",     { 0xbf, 0x25, 0x02 }, 3, 256, 64 * 1024,  2,    NOR_FLASH_ERASE_4K                       },
  { L"sst25wf020",     { 0xbf, 0x25, 0x03 }, 3, 256, 64 * 1024,  4,    NOR_FLASH_ERASE_4K                       },
  { L"sst25wf040",     { 0xbf, 0x25, 0x04 }, 3, 256, 64 * 1024,  8,    NOR_FLASH_ERASE_4K                       },
  { L"sst25wf040b",    { 0x62, 0x16, 0x13 }, 3, 256, 64 * 1024,  8,    NOR_FLASH_ERASE_4K                       },
  { L"sst25wf080",     { 0xbf, 0x25, 0x05 }, 3, 256, 64 * 1024,  16,   NOR_FLASH_ERASE_4K                       },
  /* WINBOND */
  { L"w25p80",         { 0xef, 0x20, 0x14 }, 3, 256, 64 * 1024,  16,   0                                        },
  { L"w25p16",         { 0xef, 0x20, 0x15 }, 3, 256, 64 * 1024,  32,   0                                        },
  { L"w25p32",         { 0xef, 0x20, 0x16 }, 3, 256, 64 * 1024,  64,   0                                        },
  { L"w25x40",         { 0xef, 0x30, 0x13 }, 3, 256, 64 * 1024,  8,    NOR_FLASH_ERASE_4K                       },
  { L"w25x16",         { 0xef, 0x30, 0x15 }, 3, 256, 64 * 1024,  32,   NOR_FLASH_ERASE_4K                       },
  { L"w25x32",         { 0xef, 0x30, 0x16 }, 3, 256, 64 * 1024,  64,   NOR_FLASH_ERASE_4K                       },
  { L"w25x64",         { 0xef, 0x30, 0x17 }, 3, 256, 64 * 1024,  128,  NOR_FLASH_ERASE_4K                       },
  { L"w25q80bl",       { 0xef, 0x40, 0x14 }, 3, 256, 64 * 1024,  16,   NOR_FLASH_ERASE_4K                       },
  { L"w25q16cl",       { 0xef, 0x40, 0x15 }, 3, 256, 64 * 1024,  32,   NOR_FLASH_ERASE_4K                       },
  { L"w25q32bv",       { 0xef, 0x40, 0x16 }, 3, 256, 64 * 1024,  64,   NOR_FLASH_ERASE_4K                       },
  { L"w25q64cv",       { 0xef, 0x40, 0x17 }, 3, 256, 64 * 1024,  128,  NOR_FLASH_ERASE_4K                       },
  { L"w25q128bv",      { 0xef, 0x40, 0x18 }, 3, 256, 64 * 1024,  256,  NOR_FLASH_ERASE_4K                       },
  { L"w25q256",        { 0xef, 0x40, 0x19 }, 3, 256, 64 * 1024,  512,  NOR_FLASH_ERASE_4K                       },
  { L"w25q80bw",       { 0xef, 0x50, 0x14 }, 3, 256, 64 * 1024,  16,   NOR_FLASH_ERASE_4K                       },
  { L"w25q16dw",       { 0xef, 0x60, 0x15 }, 3, 256, 64 * 1024,  32,   NOR_FLASH_ERASE_4K                       },
  { L"w25q32dw",       { 0xef, 0x60, 0x16 }, 3, 256, 64 * 1024,  64,   NOR_FLASH_ERASE_4K                       },
  { L"w25q64dw",       { 0xef, 0x60, 0x17 }, 3, 256, 64 * 1024,  128,  NOR_FLASH_ERASE_4K                       },
  { L"w25q128fw",      { 0xef, 0x60, 0x18 }, 3, 256, 64 * 1024,  256,  NOR_FLASH_ERASE_4K                       },
  { },                 /* Empty entry to terminate the list */
};

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
  )
{
  CONST NOR_FLASH_INFO  *TmpInfo;

  /*
   * Iterate over NorFlashIds table, in order to find matching entry.
   */
  TmpInfo = NorFlashIds;
  for ( ; TmpInfo->Name != NULL; TmpInfo++) {
    if (CompareMem (TmpInfo->Id, Id, TmpInfo->IdLen) == 0) {
      break;
    }
  }

  /*
   * Matching entry was not found.
   */
  if (TmpInfo->Name == NULL) {
    return EFI_NOT_FOUND;
  }

  /*
   * Allocate and copy NOR flash information structure.
   */
  if (AllocateForRuntime) {
    *FlashInfo = AllocateRuntimeCopyPool (sizeof (NOR_FLASH_INFO), TmpInfo);
  } else {
    *FlashInfo = AllocateCopyPool (sizeof (NOR_FLASH_INFO), TmpInfo);
  }

  if (FlashInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Print NOR flash information basing on data stored in
  the NOR_FLASH_INFO structure.

  @param[in]       FlashInfo          Pointer to NOR flash information structure

**/
VOID
EFIAPI
NorFlashPrintInfo (
  IN     NOR_FLASH_INFO  *Info
  )
{
  UINTN  EraseSize;

  if (Info->Flags & NOR_FLASH_ERASE_4K) {
    EraseSize = SIZE_4KB;
  } else {
    EraseSize = Info->SectorSize;
  }

  DEBUG ((
    DEBUG_ERROR,
    "Detected %s SPI NOR flash with page size %d B, erase size %d KB, total %d MB\n",
    Info->Name,
    Info->PageSize,
    EraseSize / 1024,
    (Info->SectorSize * Info->SectorCount) / 1024 / 1024
    ));
}

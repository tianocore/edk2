/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   



Module Name:

  Fd.h

Abstract:

  EFI Intel82802AB/82802AC Firmware Hub.


--*/


//
// Supported SPI devices
//

//
// MFG and Device code
//
#define SST_25LF040A        0x0044BF
#define SST_25LF040         0x0040BF
#define SST_25LF080A        0x0080BF
#define SST_25VF080B        0x008EBF
#define SST_25VF016B        0x0041BF
#define SST_25VF032B        0x004ABF

#define PMC_25LV040         0x007E9D

#define ATMEL_26DF041       0x00441F
#define Atmel_AT26F004      0x00041F
#define Atmel_AT26DF081A    0x01451F
#define Atmel_AT25DF161     0x02461F
#define Atmel_AT26DF161     0x00461F
#define Atmel_AT25DF641     0x00481F
#define Atmel_AT26DF321     0x00471F

#define Macronix_MX25L8005  0x1420C2
#define Macronix_MX25L1605A 0x1520C2
#define Macronix_MX25L3205D 0x1620C2

#define STMicro_M25PE80     0x148020

#define Winbond_W25X40      0x1330EF
#define Winbond_W25X80      0x1430EF
#define Winbond_W25Q80      0x1440EF

#define Winbond_W25X16      0x1540EF    // W25Q16
#define Winbond_W25X32      0x1630EF

//
//  NOTE: Assuming that 8Mbit flash will only contain a 4Mbit binary.
//  Treating 4Mbit and 8Mbit devices the same.
//

//
// BIOS Base Address
//
#define BIOS_BASE_ADDRESS_4M  0xFFF80000
#define BIOS_BASE_ADDRESS_8M  0xFFF00000
#define BIOS_BASE_ADDRESS_16M 0xFFE00000

//
// block and sector sizes
//
#define SECTOR_SIZE_256BYTE 0x100       // 256byte page size
#define SECTOR_SIZE_4KB     0x1000      // 4kBytes sector size
#define BLOCK_SIZE_32KB     0x00008000  // 32Kbytes block size
#define MAX_FLASH_SIZE      0x00400000  // 32Mbit (Note that this can also be used for the 4Mbit & 8Mbit)

//
// Flash commands
//
#define SPI_SST25LF_COMMAND_WRITE         0x02
#define SPI_SST25LF_COMMAND_READ          0x03
#define SPI_SST25LF_COMMAND_ERASE         0x20
#define SPI_SST25LF_COMMAND_WRITE_DISABLE 0x04
#define SPI_SST25LF_COMMAND_READ_STATUS   0x05
#define SPI_SST25LF_COMMAND_WRITE_ENABLE  0x06
#define SPI_SST25LF_COMMAND_READ_ID       0xAB
#define SPI_SST25LF_COMMAND_WRITE_S_EN    0x50
#define SPI_SST25LF_COMMAND_WRITE_S       0x01

#define SPI_PMC25LV_COMMAND_WRITE         0x02
#define SPI_PMC25LV_COMMAND_READ          0x03
#define SPI_PMC25LV_COMMAND_ERASE         0xD7
#define SPI_PMC25LV_COMMAND_WRITE_DISABLE 0x04
#define SPI_PMC25LV_COMMAND_READ_STATUS   0x05
#define SPI_PMC25LV_COMMAND_WRITE_ENABLE  0x06
#define SPI_PMC25LV_COMMAND_READ_ID       0xAB
#define SPI_PMC25LV_COMMAND_WRITE_S_EN    0x06
#define SPI_PMC25LV_COMMAND_WRITE_S       0x01

#define SPI_AT26DF_COMMAND_WRITE         0x02
#define SPI_AT26DF_COMMAND_READ          0x03
#define SPI_AT26DF_COMMAND_ERASE         0x20
#define SPI_AT26DF_COMMAND_WRITE_DISABLE 0x00
#define SPI_AT26DF_COMMAND_READ_STATUS   0x05
#define SPI_AT26DF_COMMAND_WRITE_ENABLE  0x00
#define SPI_AT26DF_COMMAND_READ_ID       0x9F
#define SPI_AT26DF_COMMAND_WRITE_S_EN    0x00
#define SPI_AT26DF_COMMAND_WRITE_S       0x00

#define SPI_AT26F_COMMAND_WRITE          0x02
#define SPI_AT26F_COMMAND_READ           0x03
#define SPI_AT26F_COMMAND_ERASE          0x20
#define SPI_AT26F_COMMAND_WRITE_DISABLE  0x04
#define SPI_AT26F_COMMAND_READ_STATUS    0x05
#define SPI_AT26F_COMMAND_WRITE_ENABLE   0x06
#define SPI_AT26F_COMMAND_JEDEC_ID       0x9F
#define SPI_AT26F_COMMAND_WRITE_S_EN     0x00
#define SPI_AT26F_COMMAND_WRITE_S        0x01
#define SPI_AT26F_COMMAND_WRITE_UNPROTECT 0x39

#define SPI_SST25VF_COMMAND_WRITE         0x02
#define SPI_SST25VF_COMMAND_READ          0x03
#define SPI_SST25VF_COMMAND_ERASE         0x20
#define SPI_SST25VF_COMMAND_WRITE_DISABLE 0x04
#define SPI_SST25VF_COMMAND_READ_STATUS   0x05
#define SPI_SST25VF_COMMAND_WRITE_ENABLE  0x06
#define SPI_SST25VF_COMMAND_READ_ID       0xAB
#define SPI_SST25VF_COMMAND_JEDEC_ID      0x9F
#define SPI_SST25VF_COMMAND_WRITE_S_EN    0x50
#define SPI_SST25VF_COMMAND_WRITE_S       0x01

#define SPI_STM25PE_COMMAND_WRITE         0x02
#define SPI_STM25PE_COMMAND_READ          0x03
#define SPI_STM25PE_COMMAND_ERASE         0xDB
#define SPI_STM25PE_COMMAND_WRITE_DISABLE 0x04
#define SPI_STM25PE_COMMAND_READ_STATUS   0x05
#define SPI_STM25PE_COMMAND_WRITE_ENABLE  0x06
#define SPI_STM25PE_COMMAND_JEDEC_ID      0x9F

#define SPI_WinbondW25X_COMMAND_WRITE_S      0x01
#define SPI_WinbondW25X_COMMAND_WRITE        0x02
#define SPI_WinbondW25X_COMMAND_READ         0x03
#define SPI_WinbondW25X_COMMAND_READ_STATUS  0x05
#define SPI_WinbondW25X_COMMAND_ERASE_S      0x20
#define SPI_WinbondW25X_COMMAND_WRITE_ENABLE 0x06
#define SPI_WinbondW25X_COMMAND_JEDEC_ID     0x9F

//
// SPI default opcode slots
//
#define SPI_OPCODE_WRITE_INDEX           0
#define SPI_OPCODE_READ_INDEX            1
#define SPI_OPCODE_ERASE_INDEX           2
#define SPI_OPCODE_READ_S_INDEX          3
#define SPI_OPCODE_READ_ID_INDEX         4
#define SPI_OPCODE_WRITE_S_INDEX         6
#define SPI_OPCODE_WRITE_UNPROTECT_INDEX 7

#define SPI_PREFIX_WRITE_S_EN 1
#define SPI_PREFIX_WRITE_EN   0

//
// Atmel AT26F00x
//
#define B_AT26F_STS_REG_SPRL  0x80
#define B_AT26F_STS_REG_SWP   0x0C

//
// Block lock bit definitions:
//
#define READ_LOCK   0x04
#define LOCK_DOWN   0x02
#define WRITE_LOCK  0x01
#define FULL_ACCESS 0x00

//
// Function Prototypes
//
EFI_STATUS
FlashGetNextBlock (
  IN UINTN*                               Key,
  OUT EFI_PHYSICAL_ADDRESS*               BlockAddress,
  OUT UINTN*                              BlockSize
  );

EFI_STATUS
FlashGetSize (
  OUT UINTN* Size
  );

EFI_STATUS
FlashGetUniformBlockSize (
  OUT UINTN* Size
  );

EFI_STATUS
FlashEraseWithNoTopSwapping (
  IN  UINT8 *BaseAddress,
  IN  UINTN NumBytes
  );

EFI_STATUS
FlashErase (
  IN  UINT8                 *BaseAddress,
  IN  UINTN                 NumBytes
  );

EFI_STATUS
FlashWriteWithNoTopSwapping (
  IN UINT8*                 DstBufferPtr,
  IN UINT8*                 SrcBufferPtr,
  IN UINTN                  NumBytes
  );

EFI_STATUS
FlashWrite (
  IN  UINT8                 *DstBufferPtr,
  IN  UINT8                 *SrcBufferPtr,
  IN  UINTN                 NumBytes
  );

EFI_STATUS
FlashReadWithNoTopSwapping (
  IN  UINT8                 *BaseAddress,
  IN  UINT8                 *DstBufferPtr,
  IN  UINTN                 NumBytes
  );

EFI_STATUS
FlashRead (
  IN  UINT8                 *BaseAddress,
  IN  UINT8                 *DstBufferPtr,
  IN  UINTN                 NumBytes
  );

EFI_STATUS
FlashLockWithNoTopSwapping (
  IN UINT8*                BaseAddress,
  IN UINTN                 NumBytes,
  IN UINT8                 LockState
  );

EFI_STATUS
FlashLock(
  IN  UINT8                 *BaseAddress,
  IN  UINTN                 NumBytes,
  IN  UINT8                 LockState
  );

EFI_STATUS
CheckIfErased(
  IN  UINT8       *DstBufferPtr,
  IN  UINTN       NumBytes
  );

EFI_STATUS
CheckIfFlashIsReadyForWrite (
  IN  UINT8       *DstBufferPtr,
  IN  UINT8       *SrcBufferPtr,
  IN  UINTN       NumBytes
  );

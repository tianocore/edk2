/** @file  VirtNorFlashDeviceLib.h

  Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef VIRT_NOR_FLASH_DEVICE_LIB_H
#define VIRT_NOR_FLASH_DEVICE_LIB_H

// Each command must be sent simultaneously to both chips,
// i.e. at the lower 16 bits AND at the higher 16 bits
#define CREATE_NOR_ADDRESS(BaseAddr, OffsetAddr)       ((BaseAddr) + ((OffsetAddr) << 2))
#define CREATE_DUAL_CMD(Cmd)                           ( ( Cmd << 16) | ( Cmd & LOW_16_BITS) )
#define SEND_NOR_COMMAND(BaseAddr, Offset, Cmd)        MmioWrite32 (CREATE_NOR_ADDRESS(BaseAddr,Offset), CREATE_DUAL_CMD(Cmd))
#define GET_NOR_BLOCK_ADDRESS(BaseAddr, Lba, LbaSize)  ( BaseAddr + (UINTN)((Lba) * LbaSize) )

#define NOR_FLASH_ERASE_RETRY  10

// Device access macros
// These are necessary because we use 2 x 16bit parts to make up 32bit data

#define HIGH_16_BITS  0xFFFF0000
#define LOW_16_BITS   0x0000FFFF
#define LOW_8_BITS    0x000000FF

#define FOLD_32BIT_INTO_16BIT(value)  ( ( value >> 16 ) | ( value & LOW_16_BITS ) )

#define GET_LOW_BYTE(value)   ( value & LOW_8_BITS )
#define GET_HIGH_BYTE(value)  ( GET_LOW_BYTE( value >> 16 ) )

// Status Register Bits
#define P30_SR_BIT_WRITE            (BIT7 << 16 | BIT7)
#define P30_SR_BIT_ERASE_SUSPEND    (BIT6 << 16 | BIT6)
#define P30_SR_BIT_ERASE            (BIT5 << 16 | BIT5)
#define P30_SR_BIT_PROGRAM          (BIT4 << 16 | BIT4)
#define P30_SR_BIT_VPP              (BIT3 << 16 | BIT3)
#define P30_SR_BIT_PROGRAM_SUSPEND  (BIT2 << 16 | BIT2)
#define P30_SR_BIT_BLOCK_LOCKED     (BIT1 << 16 | BIT1)
#define P30_SR_BIT_BEFP             (BIT0 << 16 | BIT0)

// Device Commands for Intel StrataFlash(R) Embedded Memory (P30) Family

// On chip buffer size for buffered programming operations
// There are 2 chips, each chip can buffer up to 32 (16-bit)words, and each word is 2 bytes.
// Therefore the total size of the buffer is 2 x 32 x 2 = 128 bytes
#define P30_MAX_BUFFER_SIZE_IN_BYTES  ((UINTN)128)
#define P30_MAX_BUFFER_SIZE_IN_WORDS  (P30_MAX_BUFFER_SIZE_IN_BYTES/((UINTN)4))
#define MAX_BUFFERED_PROG_ITERATIONS  10000000
#define BOUNDARY_OF_32_WORDS          ((UINTN)0x7F)

// CFI Addresses
#define P30_CFI_ADDR_QUERY_UNIQUE_QRY  0x10
#define P30_CFI_ADDR_VENDOR_ID         0x13

// CFI Data
#define CFI_QRY  0x00595251

// READ Commands
#define P30_CMD_READ_DEVICE_ID         0x0090
#define P30_CMD_READ_STATUS_REGISTER   0x0070
#define P30_CMD_CLEAR_STATUS_REGISTER  0x0050
#define P30_CMD_READ_ARRAY             0x00FF
#define P30_CMD_READ_CFI_QUERY         0x0098

// WRITE Commands
#define P30_CMD_WORD_PROGRAM_SETUP            0x0040
#define P30_CMD_ALTERNATE_WORD_PROGRAM_SETUP  0x0010
#define P30_CMD_BUFFERED_PROGRAM_SETUP        0x00E8
#define P30_CMD_BUFFERED_PROGRAM_CONFIRM      0x00D0
#define P30_CMD_BEFP_SETUP                    0x0080
#define P30_CMD_BEFP_CONFIRM                  0x00D0

// ERASE Commands
#define P30_CMD_BLOCK_ERASE_SETUP    0x0020
#define P30_CMD_BLOCK_ERASE_CONFIRM  0x00D0

// SUSPEND Commands
#define P30_CMD_PROGRAM_OR_ERASE_SUSPEND  0x00B0
#define P30_CMD_SUSPEND_RESUME            0x00D0

// BLOCK LOCKING / UNLOCKING Commands
#define P30_CMD_LOCK_BLOCK_SETUP  0x0060
#define P30_CMD_LOCK_BLOCK        0x0001
#define P30_CMD_UNLOCK_BLOCK      0x00D0
#define P30_CMD_LOCK_DOWN_BLOCK   0x002F

// PROTECTION Commands
#define P30_CMD_PROGRAM_PROTECTION_REGISTER_SETUP  0x00C0

// CONFIGURATION Commands
#define P30_CMD_READ_CONFIGURATION_REGISTER_SETUP  0x0060
#define P30_CMD_READ_CONFIGURATION_REGISTER        0x0003

EFI_STATUS
EFIAPI
NorFlashWriteBuffer (
  IN  UINTN   DeviceBaseAddress,
  IN  UINTN   TargetAddress,
  IN  UINTN   BufferSizeInBytes,
  IN  UINT32  *Buffer
  );

EFI_STATUS
EFIAPI
NorFlashWriteSingleBlock (
  IN  UINTN      DeviceBaseAddress,
  IN  UINTN      RegionBaseAddress,
  IN  EFI_LBA    Lba,
  IN  UINT32     LastBlock,
  IN  UINT32     BlockSize,
  IN  UINTN      Size,
  IN  UINTN      Offset,
  IN OUT  UINTN  *NumBytes,
  IN  UINT8      *Buffer,
  IN  VOID       *ShadowBuffer
  );

EFI_STATUS
EFIAPI
NorFlashWriteBlocks (
  IN  UINTN    DeviceBaseAddress,
  IN  UINTN    RegionBaseAddress,
  IN  EFI_LBA  Lba,
  IN  EFI_LBA  LastBlock,
  IN  UINT32   BlockSize,
  IN  UINTN    BufferSizeInBytes,
  IN  VOID     *Buffer
  );

EFI_STATUS
EFIAPI
NorFlashReadBlocks (
  IN  UINTN    DeviceBaseAddress,
  IN  UINTN    RegionBaseAddress,
  IN  EFI_LBA  Lba,
  IN  EFI_LBA  LastBlock,
  IN  UINT32   BlockSize,
  IN  UINTN    BufferSizeInBytes,
  OUT  VOID    *Buffer
  );

EFI_STATUS
EFIAPI
NorFlashRead (
  IN  UINTN    DeviceBaseAddress,
  IN  UINTN    RegionBaseAddress,
  IN  EFI_LBA  Lba,
  IN  UINT32   BlockSize,
  IN  UINTN    Size,
  IN  UINTN    Offset,
  IN  UINTN    BufferSizeInBytes,
  OUT  VOID    *Buffer
  );

EFI_STATUS
EFIAPI
NorFlashEraseSingleBlock (
  IN  UINTN  DeviceBaseAddress,
  IN  UINTN  BlockAddress
  );

EFI_STATUS
EFIAPI
NorFlashUnlockSingleBlockIfNecessary (
  IN  UINTN  DeviceBaseAddress,
  IN  UINTN  BlockAddress
  );

EFI_STATUS
EFIAPI
NorFlashWriteSingleWord (
  IN  UINTN   DeviceBaseAddress,
  IN  UINTN   WordAddress,
  IN  UINT32  WriteData
  );

EFI_STATUS
EFIAPI
NorFlashWriteFullBlock (
  IN  UINTN    DeviceBaseAddress,
  IN  UINTN    RegionBaseAddress,
  IN  EFI_LBA  Lba,
  IN  UINT32   *DataBuffer,
  IN  UINT32   BlockSizeInWords
  );

EFI_STATUS
EFIAPI
NorFlashUnlockAndEraseSingleBlock (
  IN  UINTN  DeviceBaseAddress,
  IN  UINTN  BlockAddress
  );

EFI_STATUS
EFIAPI
NorFlashReset (
  IN  UINTN  DeviceBaseAddress
  );

#endif /* VIRT_NOR_FLASH_DEVICE_LIB_H */

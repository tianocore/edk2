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

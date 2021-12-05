/** @file
  General purpose supporting routines for FAT recovery PEIM

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FatLitePeim.h"

#define CHAR_FAT_VALID  0x01

/**
  Converts a union code character to upper case.
  This functions converts a unicode character to upper case.
  If the input Letter is not a lower-cased letter,
  the original value is returned.

  @param  Letter            The input unicode character.

  @return The upper cased letter.

**/
CHAR16
ToUpper (
  IN CHAR16  Letter
  )
{
  if (('a' <= Letter) && (Letter <= 'z')) {
    Letter = (CHAR16)(Letter - 0x20);
  }

  return Letter;
}

/**
  Reads a block of data from the block device by calling
  underlying Block I/O service.

  @param  PrivateData       Global memory map for accessing global variables
  @param  BlockDeviceNo     The index for the block device number.
  @param  Lba               The logic block address to read data from.
  @param  BufferSize        The size of data in byte to read.
  @param  Buffer            The buffer of the

  @retval EFI_DEVICE_ERROR  The specified block device number exceeds the maximum
                            device number.
  @retval EFI_DEVICE_ERROR  The maximum address has exceeded the maximum address
                            of the block device.

**/
EFI_STATUS
FatReadBlock (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  UINTN                 BlockDeviceNo,
  IN  EFI_PEI_LBA           Lba,
  IN  UINTN                 BufferSize,
  OUT VOID                  *Buffer
  )
{
  EFI_STATUS            Status;
  PEI_FAT_BLOCK_DEVICE  *BlockDev;

  if (BlockDeviceNo > PEI_FAT_MAX_BLOCK_DEVICE - 1) {
    return EFI_DEVICE_ERROR;
  }

  Status   = EFI_SUCCESS;
  BlockDev = &(PrivateData->BlockDevice[BlockDeviceNo]);

  if (BufferSize > MultU64x32 (BlockDev->LastBlock - Lba + 1, BlockDev->BlockSize)) {
    return EFI_DEVICE_ERROR;
  }

  if (!BlockDev->Logical) {
    //
    // Status = BlockDev->ReadFunc
    //  (PrivateData->PeiServices, BlockDev->PhysicalDevNo, Lba, BufferSize, Buffer);
    //
    if (BlockDev->BlockIo2 != NULL) {
      Status = BlockDev->BlockIo2->ReadBlocks (
                                     (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                                     BlockDev->BlockIo2,
                                     BlockDev->PhysicalDevNo,
                                     Lba,
                                     BufferSize,
                                     Buffer
                                     );
    } else {
      Status = BlockDev->BlockIo->ReadBlocks (
                                    (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                                    BlockDev->BlockIo,
                                    BlockDev->PhysicalDevNo,
                                    Lba,
                                    BufferSize,
                                    Buffer
                                    );
    }
  } else {
    Status = FatReadDisk (
               PrivateData,
               BlockDev->ParentDevNo,
               BlockDev->StartingPos + MultU64x32 (Lba, BlockDev->BlockSize),
               BufferSize,
               Buffer
               );
  }

  return Status;
}

/**
  Find a cache block designated to specific Block device and Lba.
  If not found, invalidate an oldest one and use it. (LRU cache)

  @param  PrivateData       the global memory map.
  @param  BlockDeviceNo     the Block device.
  @param  Lba               the Logical Block Address
  @param  CachePtr          Ptr to the starting address of the memory holding the
                            data;

  @retval EFI_SUCCESS       The function completed successfully.
  @retval EFI_DEVICE_ERROR  Something error while accessing media.

**/
EFI_STATUS
FatGetCacheBlock (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  UINTN                 BlockDeviceNo,
  IN  UINT64                Lba,
  OUT CHAR8                 **CachePtr
  )
{
  EFI_STATUS            Status;
  PEI_FAT_CACHE_BUFFER  *CacheBuffer;
  INTN                  Index;
  STATIC UINT8          Seed;

  Status      = EFI_SUCCESS;
  CacheBuffer = NULL;

  //
  // go through existing cache buffers
  //
  for (Index = 0; Index < PEI_FAT_CACHE_SIZE; Index++) {
    CacheBuffer = &(PrivateData->CacheBuffer[Index]);
    if (CacheBuffer->Valid && (CacheBuffer->BlockDeviceNo == BlockDeviceNo) && (CacheBuffer->Lba == Lba)) {
      break;
    }
  }

  if (Index < PEI_FAT_CACHE_SIZE) {
    *CachePtr = (CHAR8 *)CacheBuffer->Buffer;
    return EFI_SUCCESS;
  }

  //
  // We have to find an invalid cache buffer
  //
  for (Index = 0; Index < PEI_FAT_CACHE_SIZE; Index++) {
    if (!PrivateData->CacheBuffer[Index].Valid) {
      break;
    }
  }

  //
  // Use the cache buffer
  //
  if (Index == PEI_FAT_CACHE_SIZE) {
    Index = (Seed++) % PEI_FAT_CACHE_SIZE;
  }

  //
  // Current device ID should be less than maximum device ID.
  //
  if (BlockDeviceNo >= PEI_FAT_MAX_BLOCK_DEVICE) {
    return EFI_DEVICE_ERROR;
  }

  CacheBuffer = &(PrivateData->CacheBuffer[Index]);

  CacheBuffer->BlockDeviceNo = BlockDeviceNo;
  CacheBuffer->Lba           = Lba;
  CacheBuffer->Size          = PrivateData->BlockDevice[BlockDeviceNo].BlockSize;

  //
  // Read in the data
  //
  Status = FatReadBlock (
             PrivateData,
             BlockDeviceNo,
             Lba,
             CacheBuffer->Size,
             CacheBuffer->Buffer
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  CacheBuffer->Valid = TRUE;
  *CachePtr          = (CHAR8 *)CacheBuffer->Buffer;

  return Status;
}

/**
  Disk reading.

  @param  PrivateData       the global memory map;
  @param  BlockDeviceNo     the block device to read;
  @param  StartingAddress   the starting address.
  @param  Size              the amount of data to read.
  @param  Buffer            the buffer holding the data

  @retval EFI_SUCCESS       The function completed successfully.
  @retval EFI_DEVICE_ERROR  Something error.

**/
EFI_STATUS
FatReadDisk (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  UINTN                 BlockDeviceNo,
  IN  UINT64                StartingAddress,
  IN  UINTN                 Size,
  OUT VOID                  *Buffer
  )
{
  EFI_STATUS  Status;
  UINT32      BlockSize;
  CHAR8       *BufferPtr;
  CHAR8       *CachePtr;
  UINT32      Offset;
  UINT64      Lba;
  UINT64      OverRunLba;
  UINTN       Amount;

  Status    = EFI_SUCCESS;
  BufferPtr = Buffer;
  BlockSize = PrivateData->BlockDevice[BlockDeviceNo].BlockSize;

  //
  // Read underrun
  //
  Lba    = DivU64x32Remainder (StartingAddress, BlockSize, &Offset);
  Status = FatGetCacheBlock (PrivateData, BlockDeviceNo, Lba, &CachePtr);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Amount = Size < (BlockSize - Offset) ? Size : (BlockSize - Offset);
  CopyMem (BufferPtr, CachePtr + Offset, Amount);

  if (Size == Amount) {
    return EFI_SUCCESS;
  }

  Size            -= Amount;
  BufferPtr       += Amount;
  StartingAddress += Amount;
  Lba             += 1;

  //
  // Read aligned parts
  //
  OverRunLba = Lba + DivU64x32Remainder (Size, BlockSize, &Offset);

  Size  -= Offset;
  Status = FatReadBlock (PrivateData, BlockDeviceNo, Lba, Size, BufferPtr);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  BufferPtr += Size;

  //
  // Read overrun
  //
  if (Offset != 0) {
    Status = FatGetCacheBlock (PrivateData, BlockDeviceNo, OverRunLba, &CachePtr);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    CopyMem (BufferPtr, CachePtr, Offset);
  }

  return Status;
}

/**
  This version is different from the version in Unicode collation
  protocol in that this version strips off trailing blanks.
  Converts an 8.3 FAT file name using an OEM character set
  to a Null-terminated Unicode string.
  Here does not expand DBCS FAT chars.

  @param  FatSize           The size of the string Fat in bytes.
  @param  Fat               A pointer to a Null-terminated string that contains
                            an 8.3 file name using an OEM character set.
  @param  Str               A pointer to a Null-terminated Unicode string. The
                            string must be allocated in advance to hold FatSize
                            Unicode characters

**/
VOID
EngFatToStr (
  IN UINTN    FatSize,
  IN CHAR8    *Fat,
  OUT CHAR16  *Str
  )
{
  CHAR16  *String;

  String = Str;
  //
  // No DBCS issues, just expand and add null terminate to end of string
  //
  while (*Fat != 0 && FatSize != 0) {
    if (*Fat == ' ') {
      break;
    }

    *String  = *Fat;
    String  += 1;
    Fat     += 1;
    FatSize -= 1;
  }

  *String = 0;
}

/**
  Performs a case-insensitive comparison of two Null-terminated Unicode strings.

  @param  PrivateData       Global memory map for accessing global variables
  @param  Str1              First string to perform case insensitive comparison.
  @param  Str2              Second string to perform case insensitive comparison.

**/
BOOLEAN
EngStriColl (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN CHAR16                 *Str1,
  IN CHAR16                 *Str2
  )
{
  CHAR16  UpperS1;
  CHAR16  UpperS2;

  UpperS1 = ToUpper (*Str1);
  UpperS2 = ToUpper (*Str2);
  while (*Str1 != 0) {
    if (UpperS1 != UpperS2) {
      return FALSE;
    }

    Str1++;
    Str2++;
    UpperS1 = ToUpper (*Str1);
    UpperS2 = ToUpper (*Str2);
  }

  return (BOOLEAN)((*Str2 != 0) ? FALSE : TRUE);
}

/** @file
  Produce EFI_BLOCK_IO_PROTOCOL on a RAM disk device.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "RamDiskImpl.h"

//
// The EFI_BLOCK_IO_PROTOCOL instances that is installed onto the handle
// for newly registered RAM disks
//
EFI_BLOCK_IO_PROTOCOL  mRamDiskBlockIoTemplate = {
  EFI_BLOCK_IO_PROTOCOL_REVISION,
  (EFI_BLOCK_IO_MEDIA *) 0,
  RamDiskBlkIoReset,
  RamDiskBlkIoReadBlocks,
  RamDiskBlkIoWriteBlocks,
  RamDiskBlkIoFlushBlocks
};

//
// The EFI_BLOCK_IO_PROTOCOL2 instances that is installed onto the handle
// for newly registered RAM disks
//
EFI_BLOCK_IO2_PROTOCOL  mRamDiskBlockIo2Template = {
  (EFI_BLOCK_IO_MEDIA *) 0,
  RamDiskBlkIo2Reset,
  RamDiskBlkIo2ReadBlocksEx,
  RamDiskBlkIo2WriteBlocksEx,
  RamDiskBlkIo2FlushBlocksEx
};


/**
  Initialize the BlockIO & BlockIO2 protocol of a RAM disk device.

  @param[in] PrivateData     Points to RAM disk private data.

**/
VOID
RamDiskInitBlockIo (
  IN     RAM_DISK_PRIVATE_DATA    *PrivateData
  )
{
  EFI_BLOCK_IO_PROTOCOL           *BlockIo;
  EFI_BLOCK_IO2_PROTOCOL          *BlockIo2;
  EFI_BLOCK_IO_MEDIA              *Media;

  BlockIo  = &PrivateData->BlockIo;
  BlockIo2 = &PrivateData->BlockIo2;
  Media    = &PrivateData->Media;

  CopyMem (BlockIo, &mRamDiskBlockIoTemplate, sizeof (EFI_BLOCK_IO_PROTOCOL));
  CopyMem (BlockIo2, &mRamDiskBlockIo2Template, sizeof (EFI_BLOCK_IO2_PROTOCOL));

  BlockIo->Media          = Media;
  BlockIo2->Media         = Media;
  Media->RemovableMedia   = FALSE;
  Media->MediaPresent     = TRUE;
  Media->LogicalPartition = FALSE;
  Media->ReadOnly         = FALSE;
  Media->WriteCaching     = FALSE;
  Media->BlockSize        = RAM_DISK_BLOCK_SIZE;
  Media->LastBlock        = DivU64x32 (
                              PrivateData->Size + RAM_DISK_BLOCK_SIZE - 1,
                              RAM_DISK_BLOCK_SIZE
                              ) - 1;
}


/**
  Reset the Block Device.

  @param  This                 Indicates a pointer to the calling context.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIoReset (
  IN EFI_BLOCK_IO_PROTOCOL        *This,
  IN BOOLEAN                      ExtendedVerification
  )
{
  return EFI_SUCCESS;
}


/**
  Read BufferSize bytes from Lba into Buffer.

  @param[in]  This           Indicates a pointer to the calling context.
  @param[in]  MediaId        Id of the media, changes every time the media is
                             replaced.
  @param[in]  Lba            The starting Logical Block Address to read from.
  @param[in]  BufferSize     Size of Buffer, must be a multiple of device block
                             size.
  @param[out] Buffer         A pointer to the destination buffer for the data.
                             The caller is responsible for either having
                             implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while performing
                                  the read.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId does not matched the current
                                  device.
  @retval EFI_BAD_BUFFER_SIZE     The Buffer was not a multiple of the block
                                  size of the device.
  @retval EFI_INVALID_PARAMETER   The read request contains LBAs that are not
                                  valid, or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIoReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL        *This,
  IN UINT32                       MediaId,
  IN EFI_LBA                      Lba,
  IN UINTN                        BufferSize,
  OUT VOID                        *Buffer
  )
{
  RAM_DISK_PRIVATE_DATA           *PrivateData;
  UINTN                           NumberOfBlocks;

  PrivateData = RAM_DISK_PRIVATE_FROM_BLKIO (This);

  if (MediaId != PrivateData->Media.MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  if ((BufferSize % PrivateData->Media.BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Lba > PrivateData->Media.LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  NumberOfBlocks = BufferSize / PrivateData->Media.BlockSize;
  if ((Lba + NumberOfBlocks - 1) > PrivateData->Media.LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (
    Buffer,
    (VOID *)(UINTN)(PrivateData->StartingAddr + MultU64x32 (Lba, PrivateData->Media.BlockSize)),
    BufferSize
    );

  return EFI_SUCCESS;
}


/**
  Write BufferSize bytes from Lba into Buffer.

  @param[in] This            Indicates a pointer to the calling context.
  @param[in] MediaId         The media ID that the write request is for.
  @param[in] Lba             The starting logical block address to be written.
                             The caller is responsible for writing to only
                             legitimate locations.
  @param[in] BufferSize      Size of Buffer, must be a multiple of device block
                             size.
  @param[in] Buffer          A pointer to the source buffer for the data.

  @retval EFI_SUCCESS             The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED     The device can not be written to.
  @retval EFI_DEVICE_ERROR        The device reported an error while performing
                                  the write.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHNAGED       The MediaId does not matched the current
                                  device.
  @retval EFI_BAD_BUFFER_SIZE     The Buffer was not a multiple of the block
                                  size of the device.
  @retval EFI_INVALID_PARAMETER   The write request contains LBAs that are not
                                  valid, or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIoWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL        *This,
  IN UINT32                       MediaId,
  IN EFI_LBA                      Lba,
  IN UINTN                        BufferSize,
  IN VOID                         *Buffer
  )
{
  RAM_DISK_PRIVATE_DATA           *PrivateData;
  UINTN                           NumberOfBlocks;

  PrivateData = RAM_DISK_PRIVATE_FROM_BLKIO (This);

  if (MediaId != PrivateData->Media.MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (TRUE == PrivateData->Media.ReadOnly) {
    return EFI_WRITE_PROTECTED;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  if ((BufferSize % PrivateData->Media.BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Lba > PrivateData->Media.LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  NumberOfBlocks = BufferSize / PrivateData->Media.BlockSize;
  if ((Lba + NumberOfBlocks - 1) > PrivateData->Media.LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (
    (VOID *)(UINTN)(PrivateData->StartingAddr + MultU64x32 (Lba, PrivateData->Media.BlockSize)),
    Buffer,
    BufferSize
    );

  return EFI_SUCCESS;
}


/**
  Flush the Block Device.

  @param[in] This            Indicates a pointer to the calling context.

  @retval EFI_SUCCESS             All outstanding data was written to the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while writting
                                  back the data
  @retval EFI_NO_MEDIA            There is no media in the device.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL        *This
  )
{
  return EFI_SUCCESS;
}


/**
  Resets the block device hardware.

  @param[in] This                 The pointer of EFI_BLOCK_IO2_PROTOCOL.
  @param[in] ExtendedVerification The flag about if extend verificate.

  @retval EFI_SUCCESS             The device was reset.
  @retval EFI_DEVICE_ERROR        The block device is not functioning correctly
                                  and could not be reset.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIo2Reset (
  IN EFI_BLOCK_IO2_PROTOCOL       *This,
  IN BOOLEAN                      ExtendedVerification
  )
{
  return EFI_SUCCESS;
}


/**
  Reads the requested number of blocks from the device.

  @param[in]      This            Indicates a pointer to the calling context.
  @param[in]      MediaId         The media ID that the read request is for.
  @param[in]      Lba             The starting logical block address to read
                                  from on the device.
  @param[in, out] Token           A pointer to the token associated with the
                                  transaction.
  @param[in]      BufferSize      The size of the Buffer in bytes. This must be
                                  a multiple of the intrinsic block size of the
                                  device.
  @param[out]     Buffer          A pointer to the destination buffer for the
                                  data. The caller is responsible for either
                                  having implicit or explicit ownership of the
                                  buffer.

  @retval EFI_SUCCESS             The read request was queued if Token->Event
                                  is not NULL. The data was read correctly from
                                  the device if the Token->Event is NULL.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the read operation.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE     The BufferSize parameter is not a multiple of
                                  the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER   The read request contains LBAs that are not
                                  valid, or the buffer is not on proper
                                  alignment.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a
                                  lack of resources.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIo2ReadBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL   *This,
  IN     UINT32                   MediaId,
  IN     EFI_LBA                  Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN      *Token,
  IN     UINTN                    BufferSize,
     OUT VOID                     *Buffer
  )
{
  RAM_DISK_PRIVATE_DATA           *PrivateData;
  EFI_STATUS                      Status;

  PrivateData = RAM_DISK_PRIVATE_FROM_BLKIO2 (This);

  Status = RamDiskBlkIoReadBlocks (
              &PrivateData->BlockIo,
              MediaId,
              Lba,
              BufferSize,
              Buffer
              );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // If caller's event is given, signal it after the memory read completes.
  //
  if ((Token != NULL) && (Token->Event != NULL)) {
    Token->TransactionStatus = EFI_SUCCESS;
    gBS->SignalEvent (Token->Event);
  }

  return EFI_SUCCESS;
}


/**
  Writes a specified number of blocks to the device.

  @param[in]      This            Indicates a pointer to the calling context.
  @param[in]      MediaId         The media ID that the write request is for.
  @param[in]      Lba             The starting logical block address to be
                                  written. The caller is responsible for
                                  writing to only legitimate locations.
  @param[in, out] Token           A pointer to the token associated with the
                                  transaction.
  @param[in]      BufferSize      The size in bytes of Buffer. This must be a
                                  multiple of the intrinsic block size of the
                                  device.
  @param[in]      Buffer          A pointer to the source buffer for the data.

  @retval EFI_SUCCESS             The write request was queued if Event is not
                                  NULL. The data was written correctly to the
                                  device if the Event is NULL.
  @retval EFI_WRITE_PROTECTED     The device cannot be written to.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the write operation.
  @retval EFI_BAD_BUFFER_SIZE     The BufferSize parameter is not a multiple of
                                  the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER   The write request contains LBAs that are not
                                  valid, or the buffer is not on proper
                                  alignment.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a
                                  lack of resources.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIo2WriteBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL   *This,
  IN     UINT32                   MediaId,
  IN     EFI_LBA                  Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN      *Token,
  IN     UINTN                    BufferSize,
  IN     VOID                     *Buffer
  )
{
  RAM_DISK_PRIVATE_DATA           *PrivateData;
  EFI_STATUS                      Status;

  PrivateData = RAM_DISK_PRIVATE_FROM_BLKIO2 (This);

  Status = RamDiskBlkIoWriteBlocks (
              &PrivateData->BlockIo,
              MediaId,
              Lba,
              BufferSize,
              Buffer
              );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // If caller's event is given, signal it after the memory write completes.
  //
  if ((Token != NULL) && (Token->Event != NULL)) {
    Token->TransactionStatus = EFI_SUCCESS;
    gBS->SignalEvent (Token->Event);
  }

  return EFI_SUCCESS;
}


/**
  Flushes all modified data to a physical block device.

  @param[in]      This            Indicates a pointer to the calling context.
  @param[in, out] Token           A pointer to the token associated with the
                                  transaction.

  @retval EFI_SUCCESS             The flush request was queued if Event is not
                                  NULL. All outstanding data was written
                                  correctly to the device if the Event is NULL.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to write data.
  @retval EFI_WRITE_PROTECTED     The device cannot be written to.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a
                                  lack of resources.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIo2FlushBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL   *This,
  IN OUT EFI_BLOCK_IO2_TOKEN      *Token
  )
{
  RAM_DISK_PRIVATE_DATA           *PrivateData;

  PrivateData = RAM_DISK_PRIVATE_FROM_BLKIO2 (This);

  if (TRUE == PrivateData->Media.ReadOnly) {
    return EFI_WRITE_PROTECTED;
  }

  //
  // If caller's event is given, signal it directly.
  //
  if ((Token != NULL) && (Token->Event != NULL)) {
    Token->TransactionStatus = EFI_SUCCESS;
    gBS->SignalEvent (Token->Event);
  }

  return EFI_SUCCESS;
}

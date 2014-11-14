/** @file
  BlockIo implementation for Xen PV Block driver.

  This file is implementing the interface between the actual driver in
  BlockFront.c to the BlockIo protocol.

  Copyright (C) 2014, Citrix Ltd.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "XenPvBlkDxe.h"

#include "BlockFront.h"

///
/// Block I/O Media structure
///
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_BLOCK_IO_MEDIA  gXenPvBlkDxeBlockIoMedia = {
  0,      // MediaId
  FALSE,  // RemovableMedia
  FALSE,  // MediaPresent
  FALSE,  // LogicalPartition
  TRUE,   // ReadOnly
  FALSE,  // WriteCaching
  512,    // BlockSize
  512,    // IoAlign, BlockFront does not support less than 512 bits-aligned.
  0,      // LastBlock
  0,      // LowestAlignedLba
  0,      // LogicalBlocksPerPhysicalBlock
  0       // OptimalTransferLengthGranularity
};

///
/// Block I/O Protocol instance
///
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_BLOCK_IO_PROTOCOL  gXenPvBlkDxeBlockIo = {
  EFI_BLOCK_IO_PROTOCOL_REVISION3,          // Revision
  &gXenPvBlkDxeBlockIoMedia,                // Media
  XenPvBlkDxeBlockIoReset,                  // Reset
  XenPvBlkDxeBlockIoReadBlocks,             // ReadBlocks
  XenPvBlkDxeBlockIoWriteBlocks,            // WriteBlocks
  XenPvBlkDxeBlockIoFlushBlocks             // FlushBlocks
};




/**
  Read/Write BufferSize bytes from Lba into Buffer.

  This function is commun to XenPvBlkDxeBlockIoReadBlocks and
  XenPvBlkDxeBlockIoWriteBlocks.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from/write to.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination/source buffer for the data.
  @param  IsWrite    Indicate if the operation is write or read.

  @return See description of XenPvBlkDxeBlockIoReadBlocks and
          XenPvBlkDxeBlockIoWriteBlocks.
**/
STATIC
EFI_STATUS
XenPvBlkDxeBlockIoReadWriteBlocks (
  IN     EFI_BLOCK_IO_PROTOCOL  *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN     UINTN                  BufferSize,
  IN OUT VOID                   *Buffer,
  IN     BOOLEAN                IsWrite
  )
{
  XEN_BLOCK_FRONT_IO IoData;
  EFI_BLOCK_IO_MEDIA *Media = This->Media;
  UINTN Sector;
  EFI_STATUS Status;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  if (BufferSize % Media->BlockSize != 0) {
    DEBUG ((EFI_D_ERROR, "XenPvBlkDxe: Bad buffer size: 0x%X\n", BufferSize));
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Lba > Media->LastBlock ||
      (BufferSize / Media->BlockSize) - 1 > Media->LastBlock - Lba) {
    DEBUG ((EFI_D_ERROR, "XenPvBlkDxe: %a with invalid LBA: 0x%LX, size: 0x%x\n",
            IsWrite ? "Write" : "Read", Lba, BufferSize));
    return EFI_INVALID_PARAMETER;
  }

  if (IsWrite && Media->ReadOnly) {
    return EFI_WRITE_PROTECTED;
  }

  if ((Media->IoAlign > 1) && (UINTN)Buffer & (Media->IoAlign - 1)) {
    //
    // Grub2 does not appear to respect IoAlign of 512, so reallocate the
    // buffer here.
    //
    VOID *NewBuffer;

    //
    // Try again with a properly aligned buffer.
    //
    NewBuffer = AllocateAlignedPages((BufferSize + EFI_PAGE_SIZE) / EFI_PAGE_SIZE,
                                     Media->IoAlign);
    if (!IsWrite) {
      Status = XenPvBlkDxeBlockIoReadBlocks (This, MediaId,
                                             Lba, BufferSize, NewBuffer);
      CopyMem (Buffer, NewBuffer, BufferSize);
    } else {
      CopyMem (NewBuffer, Buffer, BufferSize);
      Status = XenPvBlkDxeBlockIoWriteBlocks (This, MediaId,
                                              Lba, BufferSize, NewBuffer);
    }
    FreeAlignedPages (NewBuffer, (BufferSize + EFI_PAGE_SIZE) / EFI_PAGE_SIZE);
    return Status;
  }

  IoData.Dev = XEN_BLOCK_FRONT_FROM_BLOCK_IO (This);
  Sector = (UINTN)MultU64x32 (Lba, Media->BlockSize / 512);

  while (BufferSize > 0) {
    if (((UINTN)Buffer & EFI_PAGE_MASK) == 0) {
      IoData.Size = MIN (BLKIF_MAX_SEGMENTS_PER_REQUEST * EFI_PAGE_SIZE,
                         BufferSize);
    } else {
      IoData.Size = MIN ((BLKIF_MAX_SEGMENTS_PER_REQUEST - 1) * EFI_PAGE_SIZE,
                         BufferSize);
    }

    IoData.Buffer = Buffer;
    IoData.Sector = Sector;
    BufferSize -= IoData.Size;
    Buffer = (VOID*) ((UINTN) Buffer + IoData.Size);
    Sector += IoData.Size / 512;
    Status = XenPvBlockIo (&IoData, IsWrite);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "XenPvBlkDxe: Error durring %a operation.\n",
              IsWrite ? "write" : "read"));
      return Status;
    }
  }
  return EFI_SUCCESS;
}


/**
  Read BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
XenPvBlkDxeBlockIoReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL         *This,
  IN  UINT32                        MediaId,
  IN  EFI_LBA                       Lba,
  IN  UINTN                         BufferSize,
  OUT VOID                          *Buffer
  )
{
  return XenPvBlkDxeBlockIoReadWriteBlocks (This,
      MediaId, Lba, BufferSize, Buffer, FALSE);
}

/**
  Write BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
                     responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
XenPvBlkDxeBlockIoWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL          *This,
  IN UINT32                         MediaId,
  IN EFI_LBA                        Lba,
  IN UINTN                          BufferSize,
  IN VOID                           *Buffer
  )
{
  return XenPvBlkDxeBlockIoReadWriteBlocks (This,
      MediaId, Lba, BufferSize, Buffer, TRUE);
}

/**
  Flush the Block Device.

  @param  This              Indicates a pointer to the calling context.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writting back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
XenPvBlkDxeBlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  XenPvBlockSync (XEN_BLOCK_FRONT_FROM_BLOCK_IO (This));
  return EFI_SUCCESS;
}

/**
  Reset the block device hardware.

  @param[in]  This                 Indicates a pointer to the calling context.
  @param[in]  ExtendedVerification Not used.

  @retval EFI_SUCCESS          The device was reset.

**/
EFI_STATUS
EFIAPI
XenPvBlkDxeBlockIoReset (
  IN EFI_BLOCK_IO_PROTOCOL   *This,
  IN BOOLEAN                 ExtendedVerification
  )
{
  //
  // Since the initialization of the devices is done, then the device is
  // working correctly.
  //
  return EFI_SUCCESS;
}

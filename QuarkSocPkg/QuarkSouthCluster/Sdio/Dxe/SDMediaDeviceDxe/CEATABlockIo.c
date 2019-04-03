/** @file

Block I/O protocol for CE-ATA device

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SDMediaDevice.h"

/**
  Implements EFI_BLOCK_IO_PROTOCOL.Reset() function.

  @param  This                   The EFI_BLOCK_IO_PROTOCOL instance.
  @param  ExtendedVerification   Indicates that the driver may perform a more exhaustive.
                                 verification operation of the device during reset.
                                 (This parameter is ingored in this driver.)

  @retval EFI_SUCCESS                Success
**/
EFI_STATUS
EFIAPI
CEATABlockReset (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  BOOLEAN                 ExtendedVerification
  )
{
  EFI_STATUS                 Status;
  CARD_DATA                  *CardData;
  EFI_SD_HOST_IO_PROTOCOL    *SDHostIo;

  CardData  = CARD_DATA_FROM_THIS(This);
  SDHostIo = CardData->SDHostIo;

  if (!ExtendedVerification) {
    Status = SoftwareReset (CardData);
  } else {
    Status = SDHostIo->ResetSDHost (SDHostIo, Reset_DAT_CMD);
    if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "CEATABlockReset: Fail to ResetSDHost\n" ));
      return Status;
    }
    Status = MMCSDCardInit (CardData);
  }


  return Status;

 }

/**
  Implements EFI_BLOCK_IO_PROTOCOL.ReadBlocks() function.

  @param  This                   The EFI_BLOCK_IO_PROTOCOL instance.
  @param  MediaId                The media id that the write request is for.
  @param  LBA                    The starting logical block address to read from on the device.
                                 The caller is responsible for writing to only legitimate locations.
  @param  BufferSize             The size of the Buffer in bytes. This must be a multiple of the
                                 intrinsic block size of the device.
  @param  Buffer                 A pointer to the destination buffer for the data. The caller
                                 is responsible for either having implicit or explicit ownership
                                 of the buffer.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad
**/
EFI_STATUS
EFIAPI
CEATABlockReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  )
{
  EFI_STATUS                  Status;
  CARD_DATA                   *CardData;
  UINT32                      TransferSize;
  UINT8                       *pBuf;
  UINT32                      Index;
  UINT64                      Address;
  UINT32                      Remainder;
  UINT64                      CEATALBA;
  UINT32                      BoundarySize;

  Status       = EFI_SUCCESS;
  CardData     = CARD_DATA_FROM_THIS(This);
  pBuf         = Buffer;
  Index        = 0;
  Address      = MultU64x32(LBA, CardData->BlockIoMedia.BlockSize);
  BoundarySize = CardData->SDHostIo->HostCapability.BoundarySize;

  if (!Buffer) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG((EFI_D_ERROR, "CEATABlockReadBlocks:Invalid parameter\n" ));
    goto Exit;
  }

  if (MediaId != CardData->BlockIoMedia.MediaId) {
    Status = EFI_MEDIA_CHANGED;
  DEBUG((EFI_D_ERROR, "CEATABlockReadBlocks:Media changed\n" ));
    goto Exit;
  }

  if ((BufferSize % CardData->BlockIoMedia.BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
  DEBUG((EFI_D_ERROR, "CEATABlockReadBlocks:Bad buffer size\n" ));
    goto Exit;
  }

  if (BufferSize == 0) {
    Status = EFI_SUCCESS;
    goto Exit;
  }

  if ((Address + BufferSize) > MultU64x32 (CardData->BlockIoMedia.LastBlock + 1, CardData->BlockIoMedia.BlockSize)) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG((EFI_D_ERROR, "CEATABlockReadBlocks:Invalid parameter\n" ));
    goto Exit;
  }


  do {
    if (BufferSize < BoundarySize) {
      TransferSize = (UINT32)BufferSize;
    } else {
      TransferSize = BoundarySize;
    }

    Address += Index * TransferSize;
    CEATALBA = DivU64x32Remainder (Address, DATA_UNIT_SIZE, &Remainder);
    ASSERT(Remainder == 0);

    Status = ReadDMAExt (
               CardData,
               CEATALBA,
               pBuf,
               (UINT16)(TransferSize / DATA_UNIT_SIZE)
               );
    if (EFI_ERROR (Status)) {
     DEBUG((EFI_D_ERROR, "Read Failed at 0x%x, Index %d, Size 0x%x\n", Address, Index, TransferSize));
     This->Reset (This, TRUE);
     goto Exit;
    }
    BufferSize -= TransferSize;
    pBuf       += TransferSize;
    Index ++;
  } while (BufferSize != 0);


Exit:
  return Status;
}

/**
  Implements EFI_BLOCK_IO_PROTOCOL.WriteBlocks() function.

  @param  This                   The EFI_BLOCK_IO_PROTOCOL instance.
  @param  MediaId                The media id that the write request is for.
  @param  LBA                    The starting logical block address to read from on the device.
                                 The caller is responsible for writing to only legitimate locations.
  @param  BufferSize             The size of the Buffer in bytes. This must be a multiple of the
                                 intrinsic block size of the device.
  @param  Buffer                 A pointer to the destination buffer for the data. The caller
                                 is responsible for either having implicit or explicit ownership
                                 of the buffer.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad
**/
EFI_STATUS
EFIAPI
CEATABlockWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  )
{
  EFI_STATUS                  Status;
  CARD_DATA                   *CardData;
  UINT32                      TransferSize;
  UINT8                       *pBuf;
  UINT32                      Index;
  UINT64                      Address;
  UINT32                      Remainder;
  UINT64                      CEATALBA;
  UINT32                      BoundarySize;


  Status       = EFI_SUCCESS;
  CardData     = CARD_DATA_FROM_THIS(This);
  pBuf         = Buffer;
  Index        = 0;
  Address      = MultU64x32(LBA, CardData->BlockIoMedia.BlockSize);
  BoundarySize = CardData->SDHostIo->HostCapability.BoundarySize;


  if (!Buffer) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (MediaId != CardData->BlockIoMedia.MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto Exit;
  }

  if ((BufferSize % CardData->BlockIoMedia.BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Exit;
  }

  if (BufferSize == 0) {
    Status = EFI_SUCCESS;
    goto Exit;
  }

  if (CardData->BlockIoMedia.ReadOnly) {
    Status = EFI_WRITE_PROTECTED;
    goto Exit;
  }

  if ((Address + BufferSize) > MultU64x32 (CardData->BlockIoMedia.LastBlock + 1, CardData->BlockIoMedia.BlockSize)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  CardData->NeedFlush = TRUE;

  do {
    if (BufferSize < BoundarySize) {
      TransferSize = (UINT32)BufferSize;
    } else {
      TransferSize = BoundarySize;
    }

    Address += Index * TransferSize;
    CEATALBA = DivU64x32Remainder (Address, DATA_UNIT_SIZE, &Remainder);
    ASSERT(Remainder == 0);

    Status = WriteDMAExt (
               CardData,
               CEATALBA,
               pBuf,
               (UINT16)(TransferSize / DATA_UNIT_SIZE)
               );
    if (EFI_ERROR (Status)) {
     DEBUG((EFI_D_ERROR, "Write Failed at 0x%x, Index %d, Size 0x%x\n", Address, Index, TransferSize));
     This->Reset (This, TRUE);
     goto Exit;
    }
    BufferSize -= TransferSize;
    pBuf       += TransferSize;
    Index ++;
  } while (BufferSize != 0);


Exit:
  return Status;
}

/**
  Implements EFI_BLOCK_IO_PROTOCOL.FlushBlocks() function.
    (In this driver, this function just returns EFI_SUCCESS.)

  @param  This                   The EFI_BLOCK_IO_PROTOCOL instance.

  @retval EFI_SUCCESS
  @retval Others
**/
EFI_STATUS
EFIAPI
CEATABlockFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  )
{

  CARD_DATA                   *CardData;

  CardData  = CARD_DATA_FROM_THIS(This);

  if (CardData->NeedFlush) {
    CardData->NeedFlush = FALSE;
    FlushCache (CardData);
  }

  return EFI_SUCCESS;
}


/**
  CEATA card BlockIo init function.

  @param  CardData               Pointer to CARD_DATA.

  @retval EFI_SUCCESS
  @retval Others
**/
EFI_STATUS
CEATABlockIoInit (
  IN  CARD_DATA    *CardData
  )
/*++

  Routine Description:
    CEATA card BlockIo init function

  Arguments:
    CardData  -   Pointer to CARD_DATA

  Returns:
    EFI_SUCCESS - Success
--*/
{
  EFI_STATUS   Status;
  UINT64       MaxSize;
  UINT32       Remainder;
  //
  //BlockIO protocol
  //
  CardData->BlockIo.Revision    = EFI_BLOCK_IO_PROTOCOL_REVISION;
  CardData->BlockIo.Media       = &(CardData->BlockIoMedia);
  CardData->BlockIo.Reset       = CEATABlockReset;
  CardData->BlockIo.ReadBlocks  = CEATABlockReadBlocks ;
  CardData->BlockIo.WriteBlocks = CEATABlockWriteBlocks;
  CardData->BlockIo.FlushBlocks = CEATABlockFlushBlocks;

  CardData->BlockIoMedia.MediaId          = 0;
  CardData->BlockIoMedia.RemovableMedia   = FALSE;
  CardData->BlockIoMedia.MediaPresent     = TRUE;
  CardData->BlockIoMedia.LogicalPartition = FALSE;

  if (CardData->CSDRegister.PERM_WRITE_PROTECT | CardData->CSDRegister.TMP_WRITE_PROTECT) {
    CardData->BlockIoMedia.ReadOnly       = TRUE;
  } else {
    CardData->BlockIoMedia.ReadOnly       = FALSE;
  }


  CardData->BlockIoMedia.WriteCaching     = FALSE;
  CardData->BlockIoMedia.IoAlign          = 1;

  Status = IndentifyDevice (CardData);
  if (EFI_ERROR (Status)) {
   goto Exit;
  }

  //
  //Some device does not support this feature
  //

  if (CardData->IndentifyDeviceData.MaxWritesPerAddress == 0) {
    CardData->BlockIoMedia.ReadOnly       = TRUE;
  }

  CardData->BlockIoMedia.BlockSize        = (1 << CardData->IndentifyDeviceData.Sectorsize);
  ASSERT(CardData->BlockIoMedia.BlockSize >= 12);


  MaxSize = *(UINT64*)(CardData->IndentifyDeviceData.MaximumLBA);
  MaxSize = MultU64x32 (MaxSize, 512);

  Remainder = 0;
  CardData->BlockNumber = DivU64x32Remainder (MaxSize, CardData->BlockIoMedia.BlockSize, &Remainder);
  ASSERT(Remainder == 0);

  CardData->BlockIoMedia.LastBlock        = (EFI_LBA)(CardData->BlockNumber - 1);


Exit:
  return Status;

}




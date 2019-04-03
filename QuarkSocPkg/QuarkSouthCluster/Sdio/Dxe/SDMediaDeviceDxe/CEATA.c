/** @file

CEATA specific functions implementation

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SDMediaDevice.h"

/**
  Send RW_MULTIPLE_REGISTER command

  @param  CardData             Pointer to CARD_DATA.
  @param  Address              Register address.
  @param  ByteCount            Buffer size.
  @param  Write                TRUE means write, FALSE means read.
  @param  Buffer               Buffer pointer.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
ReadWriteMultipleRegister (
  IN  CARD_DATA   *CardData,
  IN  UINT16      Address,
  IN  UINT8       ByteCount,
  IN  BOOLEAN     Write,
  IN  UINT8       *Buffer
  )
{
  EFI_STATUS                 Status;
  UINT32                     Argument;

  Status   = EFI_SUCCESS;

  if ((Address % 4 != 0) || (ByteCount % 4 != 0)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  Argument = (Address << 16) | ByteCount;
  if (Write) {
    Argument |= BIT31;
  }


  if (Write) {
    CopyMem (CardData->AlignedBuffer, Buffer, ByteCount);

    Status = SendCommand (
               CardData,
               RW_MULTIPLE_REGISTER,
               Argument,
               OutData,
               CardData->AlignedBuffer,
               ByteCount,
               ResponseR1b,
               TIMEOUT_DATA,
               (UINT32*)&(CardData->CardStatus)
               );
  } else {
    Status = SendCommand (
               CardData,
               RW_MULTIPLE_REGISTER,
               Argument,
               InData,
               CardData->AlignedBuffer,
               ByteCount,
               ResponseR1,
               TIMEOUT_DATA,
               (UINT32*)&(CardData->CardStatus)
               );
    if (!EFI_ERROR (Status)) {
      CopyMem (Buffer, CardData->AlignedBuffer, ByteCount);
    }

  }
Exit:
  return Status;
}

/**
  Send ReadWriteMultipleBlock command with RW_MULTIPLE_REGISTER command

  @param  CardData             Pointer to CARD_DATA.
  @param  DataUnitCount        Buffer size in 512 bytes unit.
  @param  Write                TRUE means write, FALSE means read.
  @param  Buffer               Buffer pointer.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
ReadWriteMultipleBlock (
  IN  CARD_DATA   *CardData,
  IN  UINT16      DataUnitCount,
  IN  BOOLEAN     Write,
  IN  UINT8       *Buffer
  )
{
  EFI_STATUS                 Status;
  EFI_SD_HOST_IO_PROTOCOL    *SDHostIo;
  UINT32                     TransferLength;

  Status   = EFI_SUCCESS;
  SDHostIo = CardData->SDHostIo;

  TransferLength = DataUnitCount * DATA_UNIT_SIZE;
  if (TransferLength > SDHostIo->HostCapability.BoundarySize) {
    return EFI_INVALID_PARAMETER;
  }

  if (Write) {
    CopyMem (CardData->AlignedBuffer, Buffer, TransferLength);

    Status = SendCommand (
               CardData,
               RW_MULTIPLE_BLOCK,
               (DataUnitCount | BIT31),
               OutData,
               CardData->AlignedBuffer,
               TransferLength,
               ResponseR1b,
               TIMEOUT_DATA,
               (UINT32*)&(CardData->CardStatus)
               );
   } else {
      Status = SendCommand (
                 CardData,
                 RW_MULTIPLE_BLOCK,
                 DataUnitCount,
                 InData,
                 CardData->AlignedBuffer,
                 TransferLength,
                 ResponseR1,
                 TIMEOUT_DATA,
                 (UINT32*)&(CardData->CardStatus)
                 );
      if (!EFI_ERROR (Status)) {
        CopyMem (Buffer, CardData->AlignedBuffer, TransferLength);
      }
  }

  return Status;
}

/**
  Send software reset

  @param  CardData             Pointer to CARD_DATA.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
SoftwareReset (
  IN  CARD_DATA    *CardData
  )
{
  EFI_STATUS    Status;
  UINT8         Data;
  UINT32        TimeOut;

  Data = BIT2;

  Status = FastIO (CardData, Reg_Control, &Data, TRUE);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  TimeOut = 5 * 1000;

  do {
    gBS->Stall (1 * 1000);
    Status = FastIO (CardData, Reg_Control, &Data, FALSE);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
    if ((Data & BIT2) == BIT2) {
      break;
    }

    TimeOut--;
  } while (TimeOut > 0);

  if (TimeOut == 0) {
   Status = EFI_TIMEOUT;
   goto Exit;
  }

  Data &= ~BIT2;
  Status = FastIO (CardData, Reg_Control, &Data, TRUE);

  TimeOut = 5 * 1000;

  do {
    gBS->Stall (1 * 1000);
    Status = FastIO (CardData, Reg_Control, &Data, FALSE);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
    if ((Data & BIT2) != BIT2) {
      break;
    }

    TimeOut--;
  } while (TimeOut > 0);


  if (TimeOut == 0) {
   Status = EFI_TIMEOUT;
   goto Exit;
  }


Exit:
  return Status;
}


/**
  SendATACommand specificed in Taskfile

  @param  CardData             Pointer to CARD_DATA.
  @param  TaskFile             Pointer to TASK_FILE.
  @param  Write                TRUE means write, FALSE means read.
  @param  Buffer               If NULL, means no data transfer, neither read nor write.
  @param  SectorCount          Buffer size in 512 bytes unit.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
SendATACommand (
  IN  CARD_DATA   *CardData,
  IN  TASK_FILE   *TaskFile,
  IN  BOOLEAN     Write,
  IN  UINT8       *Buffer,
  IN  UINT16      SectorCount
  )
{
  EFI_STATUS                 Status;
  UINT8                      Data;
  UINT32                     TimeOut;

  //
  //Write register
  //
  Status = ReadWriteMultipleRegister (
             CardData,
             0,
             sizeof (TASK_FILE),
             TRUE,
             (UINT8*)TaskFile
             );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "ReadWriteMultipleRegister 0x%x\n", Status));
    goto Exit;
  }

  TimeOut = 5000;
  do {
    gBS->Stall (1 * 1000);
    Data = 0;
    Status = FastIO (
               CardData,
               Reg_Command_Status,
               &Data,
               FALSE
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (((Data & BIT7) == 0) && ((Data & BIT6) == BIT6)) {
      break;
    }

    TimeOut --;
  } while (TimeOut > 0);

  if (TimeOut == 0) {
    DEBUG((EFI_D_ERROR, "ReadWriteMultipleRegister FastIO EFI_TIMEOUT 0x%x\n", Data));
    Status = EFI_TIMEOUT;
    goto Exit;
  }


  if (Buffer != NULL) {
    Status = ReadWriteMultipleBlock (
               CardData,
               SectorCount,
               Write,
               (UINT8*)Buffer
               );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "ReadWriteMultipleBlock EFI_TIMEOUT 0x%x\n", Status));
      goto Exit;
    }

    TimeOut = 5 * 1000;
    do {
      gBS->Stall (1 * 1000);

      Data = 0;
      Status = FastIO (
                 CardData,
                 Reg_Command_Status,
                 &Data,
                 FALSE
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      if (((Data & BIT7) == 0) && ((Data & BIT3) == 0)) {
        break;
      }

      TimeOut --;
    } while (TimeOut > 0);
    if (TimeOut == 0) {
      DEBUG((EFI_D_ERROR, "ReadWriteMultipleBlock FastIO EFI_TIMEOUT 0x%x\n", Data));
      Status = EFI_TIMEOUT;
      goto Exit;
    }


    if (((Data & BIT6) == BIT6) && (Data & BIT0) == 0) {
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_DEVICE_ERROR;
    }
  }

Exit:
  if (EFI_ERROR (Status)) {
    SoftwareReset (CardData);
  }

  return Status;
}

/**
  IDENTIFY_DEVICE command

  @param  CardData             Pointer to CARD_DATA.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
IndentifyDevice (
  IN  CARD_DATA    *CardData
  )
{
  EFI_STATUS                 Status;

  ZeroMem (&CardData->TaskFile, sizeof (TASK_FILE));

  //
  //The host only supports nIEN = 0
  //
  CardData->TaskFile.Command_Status = IDENTIFY_DEVICE;


  Status = SendATACommand (
             CardData,
             &CardData->TaskFile,
             FALSE,
             (UINT8*)&(CardData->IndentifyDeviceData),
             1
             );


  return Status;
}

/**
  FLUSH_CACHE_EXT command

  @param  CardData             Pointer to CARD_DATA.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
FlushCache (
  IN  CARD_DATA    *CardData
  )
{

  //
  //Hitachi CE-ATA will always make the busy high after
  //receving this command
  //
/*
  EFI_STATUS  Status;
  ZeroMem (&CardData->TaskFile, sizeof (TASK_FILE));
  //
  //The host only supports nIEN = 0
  //
  CardData->TaskFile.Command_Status = FLUSH_CACHE_EXT;

  Status = SendATACommand (
             CardData,
             &CardData->TaskFile,
             FALSE,
             NULL,
             0
             );
*/
  return EFI_SUCCESS;
}

/**
  STANDBY_IMMEDIATE command

  @param  CardData             Pointer to CARD_DATA.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
StandByImmediate (
  IN  CARD_DATA    *CardData
  )
{
  EFI_STATUS  Status;

  ZeroMem (&CardData->TaskFile, sizeof (TASK_FILE));
  //
  //The host only supports nIEN = 0
  //
  CardData->TaskFile.Command_Status = STANDBY_IMMEDIATE;


  Status = SendATACommand (
             CardData,
             &CardData->TaskFile,
             FALSE,
             NULL,
             0
             );
  return Status;
}

/**
  READ_DMA_EXT command

  @param  CardData             Pointer to CARD_DATA.
  @param  LBA                  The starting logical block address to read from on the device.
  @param  Buffer               A pointer to the destination buffer for the data. The caller
                               is responsible for either having implicit or explicit ownership
                               of the buffer.
  @param  SectorCount          Size in 512 bytes unit.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
ReadDMAExt (
  IN  CARD_DATA   *CardData,
  IN  EFI_LBA     LBA,
  IN  UINT8       *Buffer,
  IN  UINT16      SectorCount
  )
{

  EFI_STATUS  Status;

  ZeroMem (&CardData->TaskFile, sizeof (TASK_FILE));
  //
  //The host only supports nIEN = 0
  //
  CardData->TaskFile.Command_Status = READ_DMA_EXT;

  CardData->TaskFile.SectorCount     = (UINT8)SectorCount;
  CardData->TaskFile.SectorCount_Exp = (UINT8)(SectorCount >> 8);

  CardData->TaskFile.LBALow          = (UINT8)LBA;
  CardData->TaskFile.LBAMid          = (UINT8)RShiftU64(LBA, 8);
  CardData->TaskFile.LBAHigh         = (UINT8)RShiftU64(LBA, 16);

  CardData->TaskFile.LBALow_Exp      = (UINT8)RShiftU64(LBA, 24);
  CardData->TaskFile.LBAMid_Exp      = (UINT8)RShiftU64(LBA, 32);
  CardData->TaskFile.LBAHigh_Exp     = (UINT8)RShiftU64(LBA, 40);

  Status = SendATACommand (
             CardData,
             &CardData->TaskFile,
             FALSE,
             Buffer,
             SectorCount
             );
  return Status;

}

/**
  WRITE_DMA_EXT command

  @param  CardData             Pointer to CARD_DATA.
  @param  LBA                  The starting logical block address to read from on the device.
  @param  Buffer               A pointer to the destination buffer for the data. The caller
                               is responsible for either having implicit or explicit ownership
                               of the buffer.
  @param  SectorCount          Size in 512 bytes unit.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
WriteDMAExt (
  IN  CARD_DATA   *CardData,
  IN  EFI_LBA     LBA,
  IN  UINT8       *Buffer,
  IN  UINT16      SectorCount
  )
{

  EFI_STATUS  Status;

  ZeroMem (&CardData->TaskFile, sizeof (TASK_FILE));
  //
  //The host only supports nIEN = 0
  //
  CardData->TaskFile.Command_Status = WRITE_DMA_EXT;

  CardData->TaskFile.SectorCount     = (UINT8)SectorCount;
  CardData->TaskFile.SectorCount_Exp = (UINT8)(SectorCount >> 8);

  CardData->TaskFile.LBALow          = (UINT8)LBA;
  CardData->TaskFile.LBAMid          = (UINT8)RShiftU64(LBA, 8);
  CardData->TaskFile.LBAHigh         = (UINT8)RShiftU64(LBA, 16);

  CardData->TaskFile.LBALow_Exp      = (UINT8)RShiftU64(LBA, 24);
  CardData->TaskFile.LBAMid_Exp      = (UINT8)RShiftU64(LBA, 32);
  CardData->TaskFile.LBAHigh_Exp     = (UINT8)RShiftU64(LBA, 40);

  Status = SendATACommand (
             CardData,
             &CardData->TaskFile,
             TRUE,
             Buffer,
             SectorCount
             );
  return Status;

}


/**
  Judge whether it is CE-ATA device or not.

  @param  CardData             Pointer to CARD_DATA.

  @retval TRUE
  @retval FALSE

**/
BOOLEAN
IsCEATADevice (
  IN  CARD_DATA    *CardData
  )
{
  EFI_STATUS                 Status;

  Status = ReadWriteMultipleRegister (
             CardData,
             0,
             sizeof (TASK_FILE),
             FALSE,
             (UINT8*)&CardData->TaskFile
             );
  if (EFI_ERROR (Status)) {
    //
    //To bring back the normal MMC card to work
    //
    CardData->SDHostIo->ResetSDHost (CardData->SDHostIo, Reset_DAT_CMD);
    return FALSE;
  }

  if (CardData->TaskFile.LBAMid == CE_ATA_SIG_CE &&
      CardData->TaskFile.LBAHigh == CE_ATA_SIG_AA
    ) {
    //
    //Disable Auto CMD for CE-ATA
    //
    CardData->SDHostIo->EnableAutoStopCmd (CardData->SDHostIo, FALSE);

    return TRUE;
  }

  return FALSE;
}




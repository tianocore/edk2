/** @file

MMC/SD transfer specific functions

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SDMediaDevice.h"

/**
  Check card status, print the debug info and check the error

  @param  Status                Status got from card status register.

  @retval EFI_SUCCESS
  @retval EFI_DEVICE_ERROR

**/
EFI_STATUS
CheckCardStatus (
  IN  UINT32    Status
  )
{
  CARD_STATUS    *CardStatus;
  CardStatus = (CARD_STATUS*)(&Status);

  if (CardStatus->ADDRESS_OUT_OF_RANGE) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ADDRESS_OUT_OF_RANGE\n"));
  }

  if (CardStatus->ADDRESS_MISALIGN) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ADDRESS_MISALIGN\n"));
  }

  if (CardStatus->BLOCK_LEN_ERROR) {
    DEBUG ((EFI_D_ERROR, "CardStatus: BLOCK_LEN_ERROR\n"));
  }

  if (CardStatus->ERASE_SEQ_ERROR) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ERASE_SEQ_ERROR\n"));
  }

  if (CardStatus->ERASE_PARAM) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ERASE_PARAM\n"));
  }

  if (CardStatus->WP_VIOLATION) {
    DEBUG ((EFI_D_ERROR, "CardStatus: WP_VIOLATION\n"));
  }

  if (CardStatus->CARD_IS_LOCKED) {
    DEBUG ((EFI_D_ERROR, "CardStatus: CARD_IS_LOCKED\n"));
  }

  if (CardStatus->LOCK_UNLOCK_FAILED) {
    DEBUG ((EFI_D_ERROR, "CardStatus: LOCK_UNLOCK_FAILED\n"));
  }

  if (CardStatus->COM_CRC_ERROR) {
    DEBUG ((EFI_D_ERROR, "CardStatus: COM_CRC_ERROR\n"));
  }

  if (CardStatus->ILLEGAL_COMMAND) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ILLEGAL_COMMAND\n"));
  }

  if (CardStatus->CARD_ECC_FAILED) {
    DEBUG ((EFI_D_ERROR, "CardStatus: CARD_ECC_FAILED\n"));
  }

  if (CardStatus->CC_ERROR) {
    DEBUG ((EFI_D_ERROR, "CardStatus: CC_ERROR\n"));
  }

  if (CardStatus->ERROR) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ERROR\n"));
  }

  if (CardStatus->UNDERRUN) {
    DEBUG ((EFI_D_ERROR, "CardStatus: UNDERRUN\n"));
  }

  if (CardStatus->OVERRUN) {
    DEBUG ((EFI_D_ERROR, "CardStatus: OVERRUN\n"));
  }

  if (CardStatus->CID_CSD_OVERWRITE) {
    DEBUG ((EFI_D_ERROR, "CardStatus: CID_CSD_OVERWRITE\n"));
  }

  if (CardStatus->WP_ERASE_SKIP) {
    DEBUG ((EFI_D_ERROR, "CardStatus: WP_ERASE_SKIP\n"));
  }

  if (CardStatus->ERASE_RESET) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ERASE_RESET\n"));
  }

  if (CardStatus->SWITCH_ERROR) {
    DEBUG ((EFI_D_ERROR, "CardStatus: SWITCH_ERROR\n"));
  }

  if ((Status & 0xFCFFA080) != 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Send command by using Host IO protocol

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  CommandIndex          The command index to set the command index field of command register.
  @param  Argument              Command argument to set the argument field of command register.
  @param  DataType              TRANSFER_TYPE, indicates no data, data in or data out.
  @param  Buffer                Contains the data read from / write to the device.
  @param  BufferSize            The size of the buffer.
  @param  ResponseType          RESPONSE_TYPE.
  @param  TimeOut               Time out value in 1 ms unit.
  @param  ResponseData          Depending on the ResponseType, such as CSD or card status.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_UNSUPPORTED
  @retval EFI_DEVICE_ERROR

**/
EFI_STATUS
SendCommand (
  IN   CARD_DATA                  *CardData,
  IN   UINT16                     CommandIndex,
  IN   UINT32                     Argument,
  IN   TRANSFER_TYPE              DataType,
  IN   UINT8                      *Buffer, OPTIONAL
  IN   UINT32                     BufferSize,
  IN   RESPONSE_TYPE              ResponseType,
  IN   UINT32                     TimeOut,
  OUT  UINT32                     *ResponseData
  )
{

  EFI_STATUS    Status;
  EFI_SD_HOST_IO_PROTOCOL    *SDHostIo;
  SDHostIo = CardData->SDHostIo;
  if (CardData->CardType != MMCCard && CardData->CardType != MMCCardHighCap) {
    CommandIndex |= AUTO_CMD12_ENABLE;
  }

  Status = SDHostIo->SendCommand (
                   SDHostIo,
                   CommandIndex,
                   Argument,
                   DataType,
                   Buffer,
                   BufferSize,
                   ResponseType,
                   TimeOut,
                   ResponseData
                   );
  if (!EFI_ERROR (Status)) {
    if (ResponseType == ResponseR1 || ResponseType == ResponseR1b) {
      ASSERT(ResponseData != NULL);
      Status = CheckCardStatus (*ResponseData);
    }
  } else {
    SDHostIo->ResetSDHost (SDHostIo, Reset_DAT_CMD);
  }

  return Status;
}

/**
  Send the card APP_CMD command with the following command indicated by CommandIndex

  @param  CardData              Pointer to CARD_DATA.
  @param  CommandIndex          The command index to set the command index field of command register.
  @param  Argument              Command argument to set the argument field of command register.
  @param  DataType              TRANSFER_TYPE, indicates no data, data in or data out.
  @param  Buffer                Contains the data read from / write to the device.
  @param  BufferSize            The size of the buffer.
  @param  ResponseType          RESPONSE_TYPE.
  @param  TimeOut               Time out value in 1 ms unit.
  @param  ResponseData          Depending on the ResponseType, such as CSD or card status.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_UNSUPPORTED
  @retval EFI_DEVICE_ERROR

**/
EFI_STATUS
SendAppCommand (
  IN   CARD_DATA                  *CardData,
  IN   UINT16                     CommandIndex,
  IN   UINT32                     Argument,
  IN   TRANSFER_TYPE              DataType,
  IN   UINT8                      *Buffer, OPTIONAL
  IN   UINT32                     BufferSize,
  IN   RESPONSE_TYPE              ResponseType,
  IN   UINT32                     TimeOut,
  OUT  UINT32                     *ResponseData
  )
{

  EFI_STATUS                 Status;
  EFI_SD_HOST_IO_PROTOCOL    *SDHostIo;
  UINT8                      Index;

  SDHostIo = CardData->SDHostIo;
  Status = EFI_SUCCESS;

  for (Index = 0; Index < 2; Index++) {
    Status = SDHostIo->SendCommand (
                         SDHostIo,
                         APP_CMD,
                         (CardData->Address << 16),
                         NoData,
                         NULL,
                         0,
                         ResponseR1,
                         TIMEOUT_COMMAND,
                         (UINT32*)&(CardData->CardStatus)
                         );
    if (!EFI_ERROR (Status)) {
        Status = CheckCardStatus (*(UINT32*)&(CardData->CardStatus));
        if (CardData->CardStatus.SAPP_CMD != 1) {
          Status = EFI_DEVICE_ERROR;
        }
        if (!EFI_ERROR (Status)) {
           break;
        }
    } else {
       SDHostIo->ResetSDHost (SDHostIo, Reset_Auto);
    }
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (CardData->CardType != MMCCard && CardData->CardType != MMCCardHighCap) {
    CommandIndex |= AUTO_CMD12_ENABLE;
  }

  Status = SDHostIo->SendCommand (
                       SDHostIo,
                       CommandIndex,
                       Argument,
                       DataType,
                       Buffer,
                       BufferSize,
                       ResponseType,
                       TimeOut,
                       ResponseData
                       );
  if (!EFI_ERROR (Status)) {
    if (ResponseType == ResponseR1 || ResponseType == ResponseR1b) {
      ASSERT(ResponseData != NULL);
      Status = CheckCardStatus (*ResponseData);
    }
  } else {
    SDHostIo->ResetSDHost (SDHostIo, Reset_Auto);
  }

  return Status;
}


/**
  Send the card FAST_IO command

  @param  CardData               Pointer to CARD_DATA.
  @param  RegisterAddress        Register Address.
  @param  RegisterData           Pointer to register Data.
  @param  Write                  TRUE for write, FALSE for read.

  @retval EFI_SUCCESS
  @retval EFI_UNSUPPORTED
  @retval EFI_INVALID_PARAMETER
  @retval EFI_DEVICE_ERROR

**/
EFI_STATUS
FastIO (
  IN      CARD_DATA   *CardData,
  IN      UINT8       RegisterAddress,
  IN  OUT UINT8       *RegisterData,
  IN      BOOLEAN     Write
  )
{
  EFI_STATUS                 Status;
  UINT32                     Argument;
  UINT32                     Data;

  Status   = EFI_SUCCESS;

  if (RegisterData == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  Argument = (CardData->Address << 16) | (RegisterAddress << 8);
  if (Write) {
    Argument |= BIT15 | (*RegisterData);
  }

  Status = SendCommand (
             CardData,
             FAST_IO,
             Argument,
             NoData,
             NULL,
             0,
             ResponseR4,
             TIMEOUT_COMMAND,
             &Data
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if ((Data & BIT15) == 0) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  if (!Write) {
   *RegisterData = (UINT8)Data;
  }

Exit:
  return Status;
}

/**
  Send the card GO_INACTIVE_STATE command.

  @param  CardData             Pointer to CARD_DATA.

  @return EFI_SUCCESS
  @return others

**/
EFI_STATUS
PutCardInactive (
  IN  CARD_DATA   *CardData
  )
{
  EFI_STATUS                 Status;


  Status = SendCommand (
             CardData,
             GO_INACTIVE_STATE,
             (CardData->Address << 16),
             NoData,
             NULL,
             0,
             ResponseNo,
             TIMEOUT_COMMAND,
             NULL
             );

  return Status;

}

/**
  Get card interested information for CSD rergister

  @param  CardData               Pointer to CARD_DATA.

  @retval EFI_SUCCESS
  @retval EFI_UNSUPPORTED
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
CaculateCardParameter (
  IN  CARD_DATA    *CardData
  )
{
  EFI_STATUS     Status;
  UINT32         Frequency;
  UINT32         Multiple;
  UINT32         CSize;
  CSD_SDV2       *CsdSDV2;

  Status = EFI_SUCCESS;

  switch (CardData->CSDRegister.TRAN_SPEED & 0x7) {
    case 0:
      Frequency = 100 * 1000;
      break;

    case 1:
      Frequency = 1 * 1000 * 1000;
      break;

    case 2:
      Frequency = 10 * 1000 * 1000;
      break;

    case 3:
      Frequency = 100 * 1000 * 1000;
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
  }

  switch ((CardData->CSDRegister.TRAN_SPEED >> 3) & 0xF) {
    case 1:
      Multiple = 10;
      break;

    case 2:
      Multiple = 12;
      break;

    case 3:
      Multiple = 13;
      break;

    case 4:
      Multiple = 15;
      break;

    case 5:
      Multiple = 20;
      break;

    case 6:
      if (CardData->CardType == MMCCard  || CardData->CardType == MMCCardHighCap) {
        Multiple = 26;
      } else {
        Multiple = 25;
      }
      break;

    case 7:
      Multiple = 30;
      break;

    case 8:
      Multiple = 35;
      break;

    case 9:
      Multiple = 40;
      break;

    case 10:
      Multiple = 45;
      break;

    case 11:
      if (CardData->CardType == MMCCard  || CardData->CardType == MMCCardHighCap) {
        Multiple = 52;
      } else {
        Multiple = 50;
      }
      break;

    case 12:
      Multiple = 55;
      break;

    case 13:
      Multiple = 60;
      break;

    case 14:
      Multiple = 70;
      break;

    case 15:
      Multiple = 80;
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
  }

  Frequency = Frequency * Multiple / 10;
  CardData->MaxFrequency = Frequency;

  CardData->BlockLen = 1 << CardData->CSDRegister.READ_BL_LEN;

  if (CardData->CardType == SDMemoryCard2High) {
    ASSERT(CardData->CSDRegister.CSD_STRUCTURE == 1);
    CsdSDV2 = (CSD_SDV2*)&CardData->CSDRegister;
    //
    // The SD Spec 2.0 says (CSize + 1) * 512K is the total size, so block numbber is (CSize + 1) * 1K
    // the K here means 1024 not 1000
    //
    CardData->BlockNumber = DivU64x32 (MultU64x32 (CsdSDV2->C_SIZE + 1, 512 * 1024) , CardData->BlockLen);
  } else {
    //
    // For MMC card > 2G, the block number will be recaculate later
    //
    CSize = CardData->CSDRegister.C_SIZELow2 | (CardData->CSDRegister.C_SIZEHigh10 << 2);
    CardData->BlockNumber = MultU64x32 (LShiftU64 (1, CardData->CSDRegister.C_SIZE_MULT + 2), CSize + 1);
  }

  //
  //For >= 2G card, BlockLen may be 1024, but the transfer size is still 512 bytes
  //
  if (CardData->BlockLen > 512) {
    CardData->BlockNumber = DivU64x32 (MultU64x32 (CardData->BlockNumber, CardData->BlockLen), 512);
    CardData->BlockLen    = 512;
  }

  DEBUG((
    EFI_D_INFO,
          "CalculateCardParameter: Card Size: 0x%lx\n", MultU64x32 (CardData->BlockNumber, CardData->BlockLen)
    ));

Exit:
  return Status;
}

/**
  Test the bus width setting for MMC card.It is used only for verification purpose.

  @param  CardData               Pointer to CARD_DATA.
  @param  Width                  1, 4, 8 bits.

  @retval EFI_SUCCESS
  @retval EFI_UNSUPPORTED
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
MMCCardBusWidthTest (
  IN  CARD_DATA             *CardData,
  IN  UINT32                Width
  )
{
  EFI_STATUS                 Status;
  UINT64                     Data;
  UINT64                     Value;

  ASSERT(CardData != NULL);


  Value = 0;

  switch (Width) {
    case 1:
      Data = 0x80;
      break;

    case 4:
      Data = 0x5A;
      break;

    case 8:
      Data = 0xAA55;
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
  }

  CopyMem (CardData->AlignedBuffer, &Data, Width);
  Status  = SendCommand (
              CardData,
              BUSTEST_W,
              0,
              OutData,
              CardData->AlignedBuffer,
              Width,
              ResponseR1,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CardStatus)
              );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "MMCCardBusWidthTest:SendCommand BUSTEST_W 0x%x\n", *(UINT32*)&(CardData->CardStatus)));
    goto Exit;
  }

  gBS->Stall (10 * 1000);

  Data = 0;

  Status  = SendCommand (
              CardData,
              BUSTEST_R,
              0,
              InData,
              CardData->AlignedBuffer,
              Width,
              ResponseR1,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CardStatus)
              );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "MMCCardBusWidthTest:SendCommand BUSTEST_R 0x%x\n", *(UINT32*)&(CardData->CardStatus)));
    goto Exit;
  }
  CopyMem (&Data, CardData->AlignedBuffer, Width);

  switch (Width) {
    case 1:
      Value = (~(Data ^ 0x80)) & 0xC0;
      break;
    case 4:
      Value = (~(Data ^ 0x5A)) & 0xFF;
      break;
    case 8:
      Value = (~(Data ^ 0xAA55)) & 0xFFFF;
      break;
  }

  if (Value == 0) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_UNSUPPORTED;
  }


Exit:
  return Status;
}

/**
  This function can detect these card types:
    1. MMC card
    2. SD 1.1 card
    3. SD 2.0 standard card
    3. SD 2.0 high capacity card

  @param  CardData             Pointer to CARD_DATA.

  @return EFI_SUCCESS
  @return others

**/
EFI_STATUS
GetCardType (
  IN  CARD_DATA              *CardData
  )
{
  EFI_STATUS                 Status;
  EFI_SD_HOST_IO_PROTOCOL    *SDHostIo;
  UINT32                     Argument;
  UINT32                     ResponseData;
  UINT32                     Count;
  BOOLEAN                    SDCommand8Support;


  SDHostIo = CardData->SDHostIo;

  //
  // Reset the card
  //
  Status  = SendCommand (
              CardData,
              GO_IDLE_STATE,
              0,
              NoData,
              NULL,
              0,
              ResponseNo,
              TIMEOUT_COMMAND,
              NULL
              );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "GO_IDLE_STATE Fail Status = 0x%x\n", Status));
    goto Exit;
  }

  //
  //No spec requirment, can be adjusted
  //
  gBS->Stall (10 * 1000);


  //
  // Only 2.7V - 3.6V is supported for SD2.0, only SD 2.0 card can pass
  // MMC and SD1.1 card will fail this command
  //
  Argument          = (VOLTAGE_27_36 << 8) | CHECK_PATTERN;
  ResponseData      = 0;
  SDCommand8Support = FALSE;

  Status  = SendCommand (
              CardData,
              SEND_IF_COND,
              Argument,
              NoData,
              NULL,
              0,
              ResponseR7,
              TIMEOUT_COMMAND,
              &ResponseData
              );

  if (EFI_ERROR (Status)) {
    if (Status != EFI_TIMEOUT) {
       DEBUG((EFI_D_ERROR, "SEND_IF_COND Fail, none time out error\n"));
       goto Exit;
    }
  } else {
     if (ResponseData != Argument) {
       DEBUG((EFI_D_ERROR, "SEND_IF_COND Fail, respond data does not match send data\n"));
       Status = EFI_DEVICE_ERROR;
       goto Exit;
    }
    SDCommand8Support = TRUE;
  }


  Argument = 0;
  if (SDHostIo->HostCapability.V30Support == TRUE) {
    Argument |= BIT17 | BIT18;
  } else if (SDHostIo->HostCapability.V33Support == TRUE) {
    Argument |= BIT20 | BIT21;
  }

  if (SDCommand8Support) {
    //
    //If command SD_SEND_OP_COND sucessed, it should be set.
    // SD 1.1 card will ignore it
    // SD 2.0 standard card will repsond with CCS 0, SD high capacity card will respond with CCS 1
    // CCS is BIT30 of OCR
    Argument |= BIT30;
  }


  Count        = 20;
  //
  //Only SD card will respond to this command, and spec says the card only checks condition at first ACMD41 command
  //
  do {
    Status  = SendAppCommand (
                CardData,
                SD_SEND_OP_COND,
                Argument,
                NoData,
                NULL,
                0,
                ResponseR3,
                TIMEOUT_COMMAND,
                (UINT32*)&(CardData->OCRRegister)
                );
    if (EFI_ERROR (Status)) {
      if ((Status == EFI_TIMEOUT) && (!SDCommand8Support)) {
        CardData->CardType = MMCCard;
        Status = EFI_SUCCESS;
        DEBUG((EFI_D_INFO, "SD_SEND_OP_COND, MMC card was identified\n"));
      } else {
        //
        // Not as expected, MMC card should has no response, which means timeout.
        // SD card should pass this command
        //
        DEBUG((EFI_D_ERROR, "SD_SEND_OP_COND Fail, check whether it is neither a MMC card nor a SD card\n"));
      }
      goto Exit;
    }
    //
    //Avoid waiting if sucess. Busy bit 0 means not ready
    //
    if (CardData->OCRRegister.Busy == 1) {
      break;
    }

    gBS->Stall (50 * 1000);
    Count--;
    if (Count == 0) {
      DEBUG((EFI_D_ERROR, "Card is always in busy state\n"));
      Status = EFI_TIMEOUT;
      goto Exit;
    }
  } while (1);

  //
  //Check supported voltage
  //
  Argument = 0;
  if (SDHostIo->HostCapability.V30Support == TRUE) {
    if ((CardData->OCRRegister.V270_V360 & BIT2) == BIT2) {
      Argument |= BIT17;
    } else if ((CardData->OCRRegister.V270_V360 & BIT3) == BIT3) {
      Argument |= BIT18;
    }
  } else if (SDHostIo->HostCapability.V33Support == TRUE) {
     if ((CardData->OCRRegister.V270_V360 & BIT5) == BIT5) {
       Argument |= BIT20;
     } else if ((CardData->OCRRegister.V270_V360 & BIT6) == BIT6) {
       Argument |= BIT21;
     }
  }

  if (Argument == 0) {
     //
     //No matched support voltage
     //
     PutCardInactive (CardData);
     DEBUG((EFI_D_ERROR, "No matched voltage for this card\n"));
     Status = EFI_UNSUPPORTED;
     goto Exit;
  }

  CardData->CardType = SDMemoryCard;
  if (SDCommand8Support == TRUE) {
   CardData->CardType = SDMemoryCard2;
   DEBUG((EFI_D_INFO, "SD_SEND_OP_COND, SD 2.0 or above standard card was identified\n"));
  }

  if ((CardData->OCRRegister.AccessMode & BIT1) == BIT1) {
    CardData->CardType = SDMemoryCard2High;
    DEBUG((EFI_D_INFO, "SD_SEND_OP_COND, SD 2.0 or above high capacity card was identified\n"));
  }



Exit:
  return Status;
}

/**
  MMC card high/low voltage selection function

  @param  CardData               Pointer to CARD_DATA.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_UNSUPPORTED
  @retval EFI_BAD_BUFFER_SIZE

**/
EFI_STATUS
MMCCardVoltageSelection (
  IN  CARD_DATA              *CardData
  )
{
  EFI_STATUS                 Status;
  UINT8                      Retry;
  UINT32                     TimeOut;

  Status   = EFI_SUCCESS;
  //
  //First try the high voltage, then if supported choose the low voltage
  //

    for (Retry = 0; Retry < 3; Retry++) {
      //
      // To bring back the normal MMC card to work
      // after sending the SD command. Otherwise some
      // card could not work

      Status  = SendCommand (
                CardData,
                  GO_IDLE_STATE,
                  0,
                  NoData,
                  NULL,
                  0,
                  ResponseNo,
                  TIMEOUT_COMMAND,
                  NULL
                  );
      if (EFI_ERROR (Status)) {
        DEBUG((EFI_D_ERROR, "GO_IDLE_STATE Fail Status = 0x%x\n", Status));
        continue;
      }
      //
      //CE-ATA device needs long delay
      //
      gBS->Stall ((Retry + 1) * 50 * 1000);

      //
      //Get OCR register to check voltage support, first time the OCR is 0
      //
      Status  = SendCommand (
                CardData,
                  SEND_OP_COND,
                  0,
                  NoData,
                  NULL,
                  0,
                  ResponseR3,
                  TIMEOUT_COMMAND,
                  (UINT32*)&(CardData->OCRRegister)
                  );
      if (!EFI_ERROR (Status)) {
        break;
      }
    }

    if (Retry == 3) {
      DEBUG((EFI_D_ERROR, "SEND_OP_COND Fail Status = 0x%x\n", Status));
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    //
    //TimeOut Value, 5000 * 100 * 1000 = 5 s
    //
    TimeOut = 5000;

    do {
      Status  = SendCommand (
                CardData,
                  SEND_OP_COND,
                  0x40300000,
                  NoData,
                  NULL,
                  0,
                  ResponseR3,
                  TIMEOUT_COMMAND,
                  (UINT32*)&(CardData->OCRRegister)
                  );
      if (EFI_ERROR (Status)) {
        DEBUG((EFI_D_ERROR, "SEND_OP_COND Fail Status = 0x%x\n", Status));
        goto Exit;
      }

      gBS->Stall (1 * 1000);
      TimeOut--;
      if (TimeOut == 0) {
        Status = EFI_TIMEOUT;
      DEBUG((EFI_D_ERROR, "Card is always in busy state\n"));
        goto Exit;
      }
    } while (CardData->OCRRegister.Busy != 1);

  if (CardData->OCRRegister.AccessMode == 2) // eMMC Card uses Sector Addressing - High Capacity
    {
    DEBUG((EFI_D_INFO, "eMMC Card is High Capacity\n"));
    CardData->CardType = MMCCardHighCap;
  }

Exit:
  return Status;

}

/**
  This function set the bus and device width for MMC card

  @param  CardData               Pointer to CARD_DATA.
  @param  Width                  1, 4, 8 bits.

  @retval EFI_SUCCESS
  @retval EFI_UNSUPPORTED
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
MMCCardSetBusWidth (
  IN  CARD_DATA              *CardData,
  IN  UINT8                  BusWidth,
  IN  BOOLEAN                EnableDDRMode
  )
{
  EFI_STATUS                 Status;
  EFI_SD_HOST_IO_PROTOCOL    *SDHostIo;
  SWITCH_ARGUMENT            SwitchArgument;
  UINT8                      Value;

  SDHostIo = CardData->SDHostIo;
  Value = 0;
  switch (BusWidth) {
    case 8:
      if (EnableDDRMode)
        Value = 6;
      else
      Value = 2;
      break;

    case 4:
      if (EnableDDRMode)
        Value = 5;
      else
      Value = 1;
      break;

    case 1:
      if (EnableDDRMode)    // Bus width 1 is not supported in ddr mode
        return EFI_UNSUPPORTED;
      Value = 0;
      break;

    default:
     ASSERT(0);
  }


  ZeroMem(&SwitchArgument, sizeof (SWITCH_ARGUMENT));
  SwitchArgument.CmdSet = 0;
  SwitchArgument.Value  = Value;
  SwitchArgument.Index  = (UINT32)((UINTN)
  (&(CardData->ExtCSDRegister.BUS_WIDTH)) - (UINTN)(&(CardData->ExtCSDRegister)));
  SwitchArgument.Access = WriteByte_Mode;
  Status  = SendCommand (
              CardData,
              SWITCH,
              *(UINT32*)&SwitchArgument,
              NoData,
              NULL,
              0,
              ResponseR1b,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CardStatus)
              );
  if (!EFI_ERROR (Status)) {
     Status  = SendCommand (
                 CardData,
                 SEND_STATUS,
                 (CardData->Address << 16),
                 NoData,
                 NULL,
                 0,
                 ResponseR1,
                 TIMEOUT_COMMAND,
                 (UINT32*)&(CardData->CardStatus)
                 );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "SWITCH %d bits Fail\n", BusWidth));
      goto Exit;
    } else {
      DEBUG((EFI_D_ERROR, "MMCCardSetBusWidth:SWITCH Card Status:0x%x\n", *(UINT32*)&(CardData->CardStatus)));
      Status = SDHostIo->SetBusWidth (SDHostIo, BusWidth);
      if (EFI_ERROR (Status)) {
         DEBUG((EFI_D_ERROR, "SWITCH set %d bits Fail\n", BusWidth));
         goto Exit;
      }
      gBS->Stall (5 * 1000);
    }
  }

  if (!EnableDDRMode) {     // CMD19 and CMD14 are illegal commands in ddr mode
  //if (EFI_ERROR (Status)) {
  //  DEBUG((EFI_D_ERROR, "MMCCardBusWidthTest: Fail to enable high speed mode\n"));
  //  goto Exit;
  //}

  Status = MMCCardBusWidthTest (CardData, BusWidth);
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "MMCCardBusWidthTest %d bit Fail\n", BusWidth));
    goto Exit;
    }
  }

  CardData->CurrentBusWidth = BusWidth;

Exit:
  return Status;
}


/**
  MMC/SD card init function

  @param  CardData             Pointer to CARD_DATA.

  @return EFI_SUCCESS
  @return others

**/
EFI_STATUS
MMCSDCardInit (
  IN  CARD_DATA              *CardData
  )
{
  EFI_STATUS                 Status;
  EFI_SD_HOST_IO_PROTOCOL    *SDHostIo;
  SWITCH_ARGUMENT            SwitchArgument;
  UINT32                     Data;
  UINT32                     Argument;
  UINT32                     nIndex;
  UINT8                      PowerValue;
  BOOLEAN                    EnableDDRMode;

  ASSERT(CardData != NULL);
  SDHostIo                  = CardData->SDHostIo;
  EnableDDRMode             = FALSE;

  CardData->CardType = UnknownCard;
  Status = GetCardType (CardData);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }
  DEBUG((DEBUG_INFO, "CardData->CardType  0x%x\n", CardData->CardType));

  ASSERT (CardData->CardType != UnknownCard);
  //
  //MMC, SD card need host auto stop command support
  //
  SDHostIo->EnableAutoStopCmd (SDHostIo, TRUE);

  if (CardData->CardType == MMCCard) {
    Status = MMCCardVoltageSelection (CardData);
    if (EFI_ERROR(Status)) {
      goto Exit;
    }
  }

  //
  // Get CID Register
  //
  Status  = SendCommand (
              CardData,
              ALL_SEND_CID,
              0,
              NoData,
              NULL,
              0,
              ResponseR2,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CIDRegister)
              );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "ALL_SEND_CID Fail Status = 0x%x\n", Status));
    goto Exit;
  } else {
    // Dump out the Card ID data
    DEBUG((EFI_D_INFO, "Product Name: "));
    for ( nIndex=0; nIndex<6; nIndex++ ) {
      DEBUG((EFI_D_INFO, "%c", CardData->CIDRegister.PNM[nIndex]));
    }
    DEBUG((EFI_D_INFO, "\nApplication ID : %d\n", CardData->CIDRegister.OID));
    DEBUG((EFI_D_INFO, "Manufacturer ID: %d\n", CardData->CIDRegister.MID));
    DEBUG((EFI_D_INFO, "Revision ID    : %d\n", CardData->CIDRegister.PRV));
    DEBUG((EFI_D_INFO, "Serial Number  : %d\n", CardData->CIDRegister.PSN));
  }

  //
  //SET_RELATIVE_ADDR
  //
  if (CardData->CardType == MMCCard  || CardData->CardType == MMCCardHighCap) {
    //
    //Hard code the RCA address
    //
    CardData->Address = 1;

    //
    // Set RCA Register
    //
    Status  = SendCommand (
                CardData,
                SET_RELATIVE_ADDR,
                (CardData->Address << 16),
                NoData,
                NULL,
                0,
                ResponseR1,
                TIMEOUT_COMMAND,
                (UINT32*)&(CardData->CardStatus)
                );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "SET_RELATIVE_ADDR Fail Status = 0x%x\n", Status));
      goto Exit;
    }
  } else {
    Data = 0;
    Status  = SendCommand (
                CardData,
                SET_RELATIVE_ADDR,
                0,
                NoData,
                NULL,
                0,
                ResponseR6,
                TIMEOUT_COMMAND,
                &Data
                );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "SET_RELATIVE_ADDR Fail Status = 0x%x\n", Status));
      goto Exit;
    }

    CardData->Address = (UINT16)(Data >> 16);
    *(UINT32*)&CardData->CardStatus      = Data & 0x1FFF;
    CardData->CardStatus.ERROR           = (Data >> 13) & 0x1;
    CardData->CardStatus.ILLEGAL_COMMAND = (Data >> 14) & 0x1;
    CardData->CardStatus.COM_CRC_ERROR   = (Data >> 15) & 0x1;
    Status = CheckCardStatus (*(UINT32*)&CardData->CardStatus);
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "SET_RELATIVE_ADDR Fail Status = 0x%x\n", Status));
      goto Exit;
    }
  }

  //
  // Get CSD Register
  //
  Status  = SendCommand (
              CardData,
              SEND_CSD,
              (CardData->Address << 16),
              NoData,
              NULL,
              0,
              ResponseR2,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CSDRegister)
              );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "SEND_CSD Fail Status = 0x%x\n", Status));
    goto Exit;
  }

  DEBUG((EFI_D_INFO, "CardData->CSDRegister.SPEC_VERS = 0x%x\n", CardData->CSDRegister.SPEC_VERS));
  DEBUG((EFI_D_INFO, "CardData->CSDRegister.CSD_STRUCTURE = 0x%x\n", CardData->CSDRegister.CSD_STRUCTURE));

  Status = CaculateCardParameter (CardData);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }


  //
  // It is platform and hardware specific, need hadrware engineer input
  //
  if (CardData->CSDRegister.DSR_IMP == 1) {
    //
    // Default is 0x404
    //
    Status  = SendCommand (
                CardData,
                SET_DSR,
                (DEFAULT_DSR_VALUE << 16),
                NoData,
                NULL,
                0,
                ResponseNo,
                TIMEOUT_COMMAND,
                NULL
                );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "SET_DSR Fail Status = 0x%x\n", Status));
      //
      // Assume can operate even fail
      //
    }
  }
  //
  //Change clock frequency from 400KHz to max supported when not in high speed mode
  //
  Status = SDHostIo->SetClockFrequency (SDHostIo, CardData->MaxFrequency);
  if (EFI_ERROR (Status)) {
  DEBUG((EFI_D_ERROR, "MMCSDCardInit:Fail to SetClockFrequency \n"));
  goto Exit;
  }

  //
  //Put the card into tran state
  //
  Status = SendCommand (
             CardData,
             SELECT_DESELECT_CARD,
             (CardData->Address << 16),
             NoData,
             NULL,
             0,
             ResponseR1,
             TIMEOUT_COMMAND,
             (UINT32*)&(CardData->CardStatus)
             );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "SELECT_DESELECT_CARD Fail Status = 0x%x\n", Status));
    goto Exit;
  }

  //
  // No spec requirment, can be adjusted
  //
  gBS->Stall (5 * 1000);
  //
  // No need to do so
  //
  //
  Status  = SendCommand (
              CardData,
              SEND_STATUS,
              (CardData->Address << 16),
              NoData,
              NULL,
              0,
              ResponseR1,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CardStatus)
              );
  if (EFI_ERROR (Status)) {
     DEBUG((EFI_D_ERROR, "SELECT_DESELECT_CARD SEND_STATUS Fail Status = 0x%x\n", Status));
     goto Exit;
  }
  //
  //if the SPEC_VERS indicates a version 4.0 or higher
  //The card is a high speed card and support Switch
  //and Send_ext_csd command
  //otherwise it is an old card
  //

  if (CardData->CardType == MMCCard  || CardData->CardType == MMCCardHighCap) {
    //
    //Only V4.0 and above supports more than 1 bits and high speed
    //
    if (CardData->CSDRegister.SPEC_VERS >= 4) {
    //
      //Get ExtCSDRegister
      //
      Status  = SendCommand (
                  CardData,
                  SEND_EXT_CSD,
                  0x0,
                  InData,
                  CardData->AlignedBuffer,
                  sizeof (EXT_CSD),
                  ResponseR1,
                  TIMEOUT_DATA,
                  (UINT32*)&(CardData->CardStatus)
                  );
      if (EFI_ERROR (Status)) {
        DEBUG((EFI_D_ERROR, "SEND_EXT_CSD Fail Status = 0x%x\n", Status));
        goto Exit;
      }

      CopyMem (&(CardData->ExtCSDRegister), CardData->AlignedBuffer, sizeof (EXT_CSD));

      //
      // Recaculate the block number for >2G MMC card
      //
      Data  = (CardData->ExtCSDRegister.SEC_COUNT[0]) |
              (CardData->ExtCSDRegister.SEC_COUNT[1] << 8) |
              (CardData->ExtCSDRegister.SEC_COUNT[2] << 16) |
              (CardData->ExtCSDRegister.SEC_COUNT[3] << 24);

      if (Data != 0) {
        CardData->BlockNumber = Data;
      }
      DEBUG((DEBUG_INFO, "CardData->BlockNumber  %d\n", Data));
      DEBUG((EFI_D_ERROR, "CardData->ExtCSDRegister.CARD_TYPE -> %d\n", (UINTN)CardData->ExtCSDRegister.CARD_TYPE));
      if ((CardData->ExtCSDRegister.CARD_TYPE & BIT2)||
          (CardData->ExtCSDRegister.CARD_TYPE & BIT3)) {
          //DEBUG((DEBUG_INFO, "To enable DDR mode\n"));
          //EnableDDRMode = TRUE;
      }
      //
      // Check current chipset capability and the plugged-in card
      // whether supports HighSpeed
      //
      if (SDHostIo->HostCapability.HighSpeedSupport) {

        //
        //Change card timing to high speed interface timing
        //
        ZeroMem(&SwitchArgument, sizeof (SWITCH_ARGUMENT));
        SwitchArgument.CmdSet = 0;
        SwitchArgument.Value  = 1;
        SwitchArgument.Index  = (UINT32)((UINTN)
        (&(CardData->ExtCSDRegister.HS_TIMING)) - (UINTN)(&(CardData->ExtCSDRegister)));
        SwitchArgument.Access = WriteByte_Mode;
        Status  = SendCommand (
                    CardData,
                    SWITCH,
                    *(UINT32*)&SwitchArgument,
                    NoData,
                    NULL,
                    0,
                    ResponseR1b,
                    TIMEOUT_COMMAND,
                    (UINT32*)&(CardData->CardStatus)
                    );
        if (EFI_ERROR (Status)) {
          DEBUG((EFI_D_ERROR, "MMCSDCardInit:SWITCH frequency Fail Status = 0x%x\n", Status));
        }

        gBS->Stall (5 * 1000);


        if (!EFI_ERROR (Status)) {
          Status  = SendCommand (
                      CardData,
                      SEND_STATUS,
                      (CardData->Address << 16),
                      NoData,
                      NULL,
                      0,
                      ResponseR1,
                      TIMEOUT_COMMAND,
                      (UINT32*)&(CardData->CardStatus)
                      );
          if (!EFI_ERROR (Status)) {
            if (EnableDDRMode) {
              DEBUG((EFI_D_ERROR, "Enable ddr mode on host controller\n"));
              SDHostIo->SetDDRMode (SDHostIo, TRUE);
            } else  {
              DEBUG((EFI_D_ERROR, "Enable high speed mode on host controller\n"));
              SDHostIo->SetHighSpeedMode (SDHostIo, TRUE);
            }
          //
          // Change host clock to support high speed and enable chispet to
          // support speed
          //
            if ((CardData->ExtCSDRegister.CARD_TYPE & BIT1) != 0) {
              Status = SDHostIo->SetClockFrequency (SDHostIo, FREQUENCY_MMC_PP_HIGH);
            } else if ((CardData->ExtCSDRegister.CARD_TYPE & BIT0) != 0) {
              Status = SDHostIo->SetClockFrequency (SDHostIo, FREQUENCY_MMC_PP);
            } else {
              Status = EFI_UNSUPPORTED;
            }
            if (EFI_ERROR (Status)) {
              DEBUG((EFI_D_ERROR, "MMCSDCardInit:Fail to SetClockFrequency \n"));
              goto Exit;
            }
            //
            // It seems no need to stall after changing bus freqeuncy.
            // It is said that the freqeuncy can be changed at any time. Just appends 8 clocks after command.
            // But SetClock alreay has delay.
            //
          }
        }

      }



      //
      // Prefer wide bus width for performance
      //
      //
      // Set to BusWidth bits mode, only version 4.0 or above support more than 1 bits
      //
      if (SDHostIo->HostCapability.BusWidth8 == TRUE) {
         Status = MMCCardSetBusWidth (CardData, 8, EnableDDRMode);
         if (EFI_ERROR (Status)) {
            //
            // CE-ATA may support 8 bits and 4 bits, but has no software method for detection
            //
            Status = MMCCardSetBusWidth (CardData, 4, EnableDDRMode);
            if (EFI_ERROR (Status)) {
              goto Exit;
            }
         }
      } else if (SDHostIo->HostCapability.BusWidth4 == TRUE) {
         Status = MMCCardSetBusWidth (CardData, 4, EnableDDRMode);
         if (EFI_ERROR (Status)) {
           goto Exit;
         }
      }

      PowerValue = 0;

      if (CardData->CurrentBusWidth == 8) {
        if ((CardData->ExtCSDRegister.CARD_TYPE & BIT1) != 0) {
          PowerValue = CardData->ExtCSDRegister.PWR_CL_52_360;
          PowerValue = PowerValue >> 4;
        } else if ((CardData->ExtCSDRegister.CARD_TYPE & BIT0) != 0) {
          PowerValue = CardData->ExtCSDRegister.PWR_CL_26_360;
          PowerValue = PowerValue >> 4;
        }
      } else if (CardData->CurrentBusWidth == 4) {
         if ((CardData->ExtCSDRegister.CARD_TYPE & BIT1) != 0) {
          PowerValue = CardData->ExtCSDRegister.PWR_CL_52_360;
          PowerValue = PowerValue & 0xF;
         } else if ((CardData->ExtCSDRegister.CARD_TYPE & BIT0) != 0) {
           PowerValue = CardData->ExtCSDRegister.PWR_CL_26_360;
           PowerValue = PowerValue & 0xF;
         }
      }

      if (PowerValue != 0) {
        //
        //Update Power Class
        //
        ZeroMem(&SwitchArgument, sizeof (SWITCH_ARGUMENT));
        SwitchArgument.CmdSet = 0;
        SwitchArgument.Value  = PowerValue;
        SwitchArgument.Index  = (UINT32)((UINTN)
        (&(CardData->ExtCSDRegister.POWER_CLASS)) - (UINTN)(&(CardData->ExtCSDRegister)));
        SwitchArgument.Access = WriteByte_Mode;
        Status  = SendCommand (
                    CardData,
                    SWITCH,
                    *(UINT32*)&SwitchArgument,
                    NoData,
                    NULL,
                    0,
                    ResponseR1b,
                    TIMEOUT_COMMAND,
                    (UINT32*)&(CardData->CardStatus)
                    );
         if (!EFI_ERROR (Status)) {
           Status  = SendCommand (
                       CardData,
                       SEND_STATUS,
                       (CardData->Address << 16),
                       NoData,
                       NULL,
                       0,
                       ResponseR1,
                       TIMEOUT_COMMAND,
                       (UINT32*)&(CardData->CardStatus)
                       );
           if (EFI_ERROR (Status)) {
             DEBUG((EFI_D_ERROR, "SWITCH Power Class Fail Status = 0x%x\n", Status));
           }
           //gBS->Stall (10 * 1000);
         }
      }



    } else {


      DEBUG((EFI_D_ERROR, "MMC Card version %d only supportes 1 bits at lower transfer speed\n",CardData->CSDRegister.SPEC_VERS));
    }
  } else {
      //
      // Pin 1, at power up this line has a 50KOhm pull up enabled in the card.
      // This pull-up should be disconnected by the user, during regular data transfer,
      // with SET_CLR_CARD_DETECT (ACMD42) command
      //
      Status  = SendAppCommand (
                  CardData,
                  SET_CLR_CARD_DETECT,
                  0,
                  NoData,
                  NULL,
                  0,
                  ResponseR1,
                  TIMEOUT_COMMAND,
                  (UINT32*)&(CardData->CardStatus)
                  );
      if (EFI_ERROR (Status)) {
        DEBUG((EFI_D_ERROR, "SET_CLR_CARD_DETECT Fail Status = 0x%x\n", Status));
        goto Exit;
      }

      /*
      //
      // Don't rely on SCR and SD status, some cards have unexpected SCR.
      // It only sets private section, the other bits are 0
      // such as Sandisk Ultra II 4.0G, KinSton mini SD 128M, Toshiba 2.0GB
      // Some card even fail this command, KinSton SD 4GB
      //
      Status  = SendAppCommand (
                  CardData,
                  SEND_SCR,
                  0,
                  InData,
                  (UINT8*)&(CardData->SCRRegister),
                  sizeof(SCR),
                  ResponseR1,
                  TIMEOUT_COMMAND,
                  (UINT32*)&(CardData->CardStatus)
                  );
      if (EFI_ERROR (Status)) {
        goto Exit;
      }

      //
      // SD memory card at least supports 1 and 4 bits.
      //
      // ASSERT ((CardData->SCRRegister.SD_BUS_WIDTH & (BIT0 | BIT2)) == (BIT0 | BIT2));
      */

      //
      // Set Bus Width to 4
      //
      Status  = SendAppCommand (
                  CardData,
                  SET_BUS_WIDTH,
                  SD_BUS_WIDTH_4,
                  NoData,
                  NULL,
                  0,
                  ResponseR1,
                  TIMEOUT_COMMAND,
                  (UINT32*)&(CardData->CardStatus)
                  );
      if (EFI_ERROR (Status)) {
        DEBUG((EFI_D_ERROR, "SET_BUS_WIDTH 4 bits Fail Status = 0x%x\n", Status));
        goto Exit;
      }

      Status = SDHostIo->SetBusWidth (SDHostIo, 4);
      if (EFI_ERROR (Status)) {
        goto Exit;
      }
      CardData->CurrentBusWidth = 4;


      if ((SDHostIo->HostCapability.HighSpeedSupport == FALSE) ||
          ((CardData->CSDRegister.CCC & BIT10) != BIT10)) {
        //
        // Host must support high speed
        // Card must support Switch function
        //
        goto Exit;
      }

      //
      //Mode = 0, group 1, function 1, check operation
      //
      Argument    = 0xFFFF01;
      ZeroMem (&CardData->SwitchStatus, sizeof (SWITCH_STATUS));

      Status  = SendCommand (
                  CardData,
                  SWITCH_FUNC,
                  Argument,
                  InData,
                  CardData->AlignedBuffer,
                  sizeof (SWITCH_STATUS),
                  ResponseR1,
                  TIMEOUT_COMMAND,
                  (UINT32*)&(CardData->CardStatus)
                  );
      if (EFI_ERROR (Status)) {
        goto Exit;
      }
      CopyMem (&(CardData->SwitchStatus), CardData->AlignedBuffer, sizeof (SWITCH_STATUS));

      if ((CardData->SwitchStatus.DataStructureVersion == 0x0) ||
          ((CardData->SwitchStatus.Group1BusyStatus & BIT1) != BIT1)) {
        //
        // 1. SD 1.1 card does not suppport busy bit
        // 2. Ready state
        //
        //

        //
        //Mode = 1, group 1, function 1, BIT31 set means set mode
        //
        Argument = 0xFFFF01 | BIT31;
        ZeroMem (&CardData->SwitchStatus, sizeof (SWITCH_STATUS));

        Status  = SendCommand (
                    CardData,
                    SWITCH_FUNC,
                    Argument,
                    InData,
                    CardData->AlignedBuffer,
                    sizeof (SWITCH_STATUS),
                    ResponseR1,
                    TIMEOUT_COMMAND,
                   (UINT32*)&(CardData->CardStatus)
                   );
         if (EFI_ERROR (Status)) {
            goto Exit;
         }
         CopyMem (&(CardData->SwitchStatus), CardData->AlignedBuffer, sizeof (SWITCH_STATUS));

         if ((CardData->SwitchStatus.DataStructureVersion == 0x0) ||
            ((CardData->SwitchStatus.Group1BusyStatus & BIT1) != BIT1)) {
          //
          // 1. SD 1.1 card does not suppport busy bit
          // 2. Ready state
          //

          //
          // 8 clocks, (1/ 25M) * 8 ==> 320 us, so 1ms > 0.32 ms
          //
          gBS->Stall (1000);

          //
          //Change host clock
          //
          Status = SDHostIo->SetClockFrequency (SDHostIo, FREQUENCY_SD_PP_HIGH);
          if (EFI_ERROR (Status)) {
            goto Exit;
          }

         }
      }
  }
  if (!((CardData->ExtCSDRegister.CARD_TYPE & BIT2) ||
      (CardData->ExtCSDRegister.CARD_TYPE & BIT3))) {

  //
  // Set Block Length, to improve compatibility in case of some cards
  //
  Status  = SendCommand (
                CardData,
              SET_BLOCKLEN,
              512,
              NoData,
              NULL,
              0,
              ResponseR1,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CardStatus)
              );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "SET_BLOCKLEN Fail Status = 0x%x\n", Status));
    goto Exit;
  }
  }
  SDHostIo->SetBlockLength (SDHostIo, 512);


Exit:
  return Status;
}


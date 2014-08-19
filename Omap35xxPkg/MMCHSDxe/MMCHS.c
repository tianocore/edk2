/** @file
  MMC/SD Card driver for OMAP 35xx (SDIO not supported)

  This driver always produces a BlockIo protocol but it starts off with no Media
  present. A TimerCallBack detects when media is inserted or removed and after
  a media change event a call to BlockIo ReadBlocks/WriteBlocks will cause the
  media to be detected (or removed) and the BlockIo Media structure will get
  updated. No MMC/SD Card harward registers are updated until the first BlockIo
  ReadBlocks/WriteBlocks after media has been insterted (booting with a card
  plugged in counts as an insertion event).

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MMCHS.h"

EFI_BLOCK_IO_MEDIA gMMCHSMedia = {
  SIGNATURE_32('s','d','i','o'),            // MediaId
  TRUE,                                     // RemovableMedia
  FALSE,                                    // MediaPresent
  FALSE,                                    // LogicalPartition
  FALSE,                                    // ReadOnly
  FALSE,                                    // WriteCaching
  512,                                      // BlockSize
  4,                                        // IoAlign
  0,                                        // Pad
  0                                         // LastBlock
};

typedef struct {
  VENDOR_DEVICE_PATH  Mmc;
  EFI_DEVICE_PATH     End;
} MMCHS_DEVICE_PATH;

MMCHS_DEVICE_PATH gMmcHsDevicePath = {
  {
    HARDWARE_DEVICE_PATH,
    HW_VENDOR_DP,
    (UINT8)(sizeof(VENDOR_DEVICE_PATH)),
    (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8),
    0xb615f1f5, 0x5088, 0x43cd, 0x80, 0x9c, 0xa1, 0x6e, 0x52, 0x48, 0x7d, 0x00
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
  }
};

CARD_INFO                  gCardInfo;
EMBEDDED_EXTERNAL_DEVICE   *gTPS65950;
EFI_EVENT                  gTimerEvent;
BOOLEAN                    gMediaChange = FALSE;

//
// Internal Functions
//


VOID
ParseCardCIDData (
  UINT32 Response0,
  UINT32 Response1,
  UINT32 Response2,
  UINT32 Response3
  )
{
  gCardInfo.CIDData.MDT = ((Response0 >> 8) & 0xFFF);
  gCardInfo.CIDData.PSN = (((Response0 >> 24) & 0xFF) | ((Response1 & 0xFFFFFF) << 8));
  gCardInfo.CIDData.PRV = ((Response1 >> 24) & 0xFF);
  gCardInfo.CIDData.PNM[4] = ((Response2) & 0xFF);
  gCardInfo.CIDData.PNM[3] = ((Response2 >> 8) & 0xFF);
  gCardInfo.CIDData.PNM[2] = ((Response2 >> 16) & 0xFF);
  gCardInfo.CIDData.PNM[1] = ((Response2 >> 24) & 0xFF);
  gCardInfo.CIDData.PNM[0] = ((Response3) & 0xFF);
  gCardInfo.CIDData.OID = ((Response3 >> 8) & 0xFFFF);
  gCardInfo.CIDData.MID = ((Response3 >> 24) & 0xFF);
}


VOID
UpdateMMCHSClkFrequency (
  UINTN NewCLKD
  )
{
  //Set Clock enable to 0x0 to not provide the clock to the card
  MmioAnd32 (MMCHS_SYSCTL, ~CEN);

  //Set new clock frequency.
  MmioAndThenOr32 (MMCHS_SYSCTL, ~CLKD_MASK, NewCLKD << 6);

  //Poll till Internal Clock Stable
  while ((MmioRead32 (MMCHS_SYSCTL) & ICS_MASK) != ICS);

  //Set Clock enable to 0x1 to provide the clock to the card
  MmioOr32 (MMCHS_SYSCTL, CEN);
}


EFI_STATUS
SendCmd (
  UINTN Cmd,
  UINTN CmdInterruptEnableVal,
  UINTN CmdArgument
  )
{
  UINTN MmcStatus;
  UINTN RetryCount = 0;

  //Check if command line is in use or not. Poll till command line is available.
  while ((MmioRead32 (MMCHS_PSTATE) & DATI_MASK) == DATI_NOT_ALLOWED);

  //Provide the block size.
  MmioWrite32 (MMCHS_BLK, BLEN_512BYTES);

  //Setting Data timeout counter value to max value.
  MmioAndThenOr32 (MMCHS_SYSCTL, ~DTO_MASK, DTO_VAL);

  //Clear Status register.
  MmioWrite32 (MMCHS_STAT, 0xFFFFFFFF);

  //Set command argument register
  MmioWrite32 (MMCHS_ARG, CmdArgument);

  //Enable interrupt enable events to occur
  MmioWrite32 (MMCHS_IE, CmdInterruptEnableVal);

  //Send a command
  MmioWrite32 (MMCHS_CMD, Cmd);

  //Check for the command status.
  while (RetryCount < MAX_RETRY_COUNT) {
    do {
      MmcStatus = MmioRead32 (MMCHS_STAT);
    } while (MmcStatus == 0);

    //Read status of command response
    if ((MmcStatus & ERRI) != 0) {

      //Perform soft-reset for mmci_cmd line.
      MmioOr32 (MMCHS_SYSCTL, SRC);
      while ((MmioRead32 (MMCHS_SYSCTL) & SRC));

      DEBUG ((EFI_D_INFO, "MmcStatus: %x\n", MmcStatus));
      return EFI_DEVICE_ERROR;
    }

    //Check if command is completed.
    if ((MmcStatus & CC) == CC) {
      MmioWrite32 (MMCHS_STAT, CC);
      break;
    }

    RetryCount++;
  }

  if (RetryCount == MAX_RETRY_COUNT) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}


VOID
GetBlockInformation (
  UINTN *BlockSize,
  UINTN *NumBlocks
  )
{
  CSD_SDV2 *CsdSDV2Data;
  UINTN    CardSize;

  if (gCardInfo.CardType == SD_CARD_2_HIGH) {
    CsdSDV2Data = (CSD_SDV2 *)&gCardInfo.CSDData;

    //Populate BlockSize.
    *BlockSize = (0x1UL << CsdSDV2Data->READ_BL_LEN);

    //Calculate Total number of blocks.
    CardSize = CsdSDV2Data->C_SIZELow16 | (CsdSDV2Data->C_SIZEHigh6 << 2);
    *NumBlocks = ((CardSize + 1) * 1024);
  } else {
    //Populate BlockSize.
    *BlockSize = (0x1UL << gCardInfo.CSDData.READ_BL_LEN);

    //Calculate Total number of blocks.
    CardSize = gCardInfo.CSDData.C_SIZELow2 | (gCardInfo.CSDData.C_SIZEHigh10 << 2);
    *NumBlocks = (CardSize + 1) * (1 << (gCardInfo.CSDData.C_SIZE_MULT + 2));
  }

  //For >=2G card, BlockSize may be 1K, but the transfer size is 512 bytes.
  if (*BlockSize > 512) {
    *NumBlocks = MultU64x32(*NumBlocks, *BlockSize/2);
    *BlockSize = 512;
  }

  DEBUG ((EFI_D_INFO, "Card type: %x, BlockSize: %x, NumBlocks: %x\n", gCardInfo.CardType, *BlockSize, *NumBlocks));
}


VOID
CalculateCardCLKD (
  UINTN *ClockFrequencySelect
  )
{
  UINT8    MaxDataTransferRate;
  UINTN    TransferRateValue = 0;
  UINTN    TimeValue = 0 ;
  UINTN    Frequency = 0;

  MaxDataTransferRate = gCardInfo.CSDData.TRAN_SPEED;

  // For SD Cards  we would need to send CMD6 to set
  // speeds abouve 25MHz. High Speed mode 50 MHz and up

  //Calculate Transfer rate unit (Bits 2:0 of TRAN_SPEED)
  switch (MaxDataTransferRate & 0x7) {
    case 0:
      TransferRateValue = 100 * 1000;
      break;

    case 1:
      TransferRateValue = 1 * 1000 * 1000;
      break;

    case 2:
      TransferRateValue = 10 * 1000 * 1000;
      break;

    case 3:
      TransferRateValue = 100 * 1000 * 1000;
      break;

    default:
      DEBUG((EFI_D_ERROR, "Invalid parameter.\n"));
      ASSERT(FALSE);
  }

  //Calculate Time value (Bits 6:3 of TRAN_SPEED)
  switch ((MaxDataTransferRate >> 3) & 0xF) {
    case 1:
      TimeValue = 10;
      break;

    case 2:
      TimeValue = 12;
      break;

    case 3:
      TimeValue = 13;
      break;

    case 4:
      TimeValue = 15;
      break;

    case 5:
      TimeValue = 20;
      break;

    case 6:
      TimeValue = 25;
      break;

    case 7:
      TimeValue = 30;
      break;

    case 8:
      TimeValue = 35;
      break;

    case 9:
      TimeValue = 40;
      break;

    case 10:
      TimeValue = 45;
      break;

    case 11:
      TimeValue = 50;
      break;

    case 12:
      TimeValue = 55;
      break;

    case 13:
      TimeValue = 60;
      break;

    case 14:
      TimeValue = 70;
      break;

    case 15:
      TimeValue = 80;
      break;

    default:
      DEBUG((EFI_D_ERROR, "Invalid parameter.\n"));
      ASSERT(FALSE);
  }

  Frequency = TransferRateValue * TimeValue/10;

  //Calculate Clock divider value to program in MMCHS_SYSCTL[CLKD] field.
  *ClockFrequencySelect = ((MMC_REFERENCE_CLK/Frequency) + 1);

  DEBUG ((EFI_D_INFO, "MaxDataTransferRate: 0x%x, Frequency: %d KHz, ClockFrequencySelect: %x\n", MaxDataTransferRate, Frequency/1000, *ClockFrequencySelect));
}


VOID
GetCardConfigurationData (
  VOID
  )
{
  UINTN  BlockSize;
  UINTN  NumBlocks;
  UINTN  ClockFrequencySelect;

  //Calculate BlockSize and Total number of blocks in the detected card.
  GetBlockInformation(&BlockSize, &NumBlocks);
  gCardInfo.BlockSize = BlockSize;
  gCardInfo.NumBlocks = NumBlocks;

  //Calculate Card clock divider value.
  CalculateCardCLKD(&ClockFrequencySelect);
  gCardInfo.ClockFrequencySelect = ClockFrequencySelect;
}


EFI_STATUS
InitializeMMCHS (
  VOID
  )
{
  UINT8      Data = 0;
  EFI_STATUS Status;

  //Select Device group to belong to P1 device group in Power IC.
  Data = DEV_GRP_P1;
  Status = gTPS65950->Write (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID4, VMMC1_DEV_GRP), 1, &Data);
  ASSERT_EFI_ERROR(Status);

  //Configure voltage regulator for MMC1 in Power IC to output 3.0 voltage.
  Data = VSEL_3_00V;
  Status = gTPS65950->Write (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID4, VMMC1_DEDICATED_REG), 1, &Data);
  ASSERT_EFI_ERROR(Status);

  //After ramping up voltage, set VDDS stable bit to indicate that voltage level is stable.
  MmioOr32 (CONTROL_PBIAS_LITE, (PBIASLITEVMODE0 | PBIASLITEPWRDNZ0 | PBIASSPEEDCTRL0 | PBIASLITEVMODE1 | PBIASLITEWRDNZ1));

  // Enable WP GPIO
  MmioAndThenOr32 (GPIO1_BASE + GPIO_OE, ~BIT23, BIT23);

  // Enable Card Detect
  Data = CARD_DETECT_ENABLE;
  gTPS65950->Write (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID2, TPS65950_GPIO_CTRL), 1, &Data);


  return Status;
}


EFI_STATUS
PerformCardIdenfication (
  VOID
  )
{
  EFI_STATUS Status;
  UINTN      CmdArgument = 0;
  UINTN      Response = 0;
  UINTN      RetryCount = 0;
  BOOLEAN    SDCmd8Supported = FALSE;

  //Enable interrupts.
  MmioWrite32 (MMCHS_IE, (BADA_EN | CERR_EN | DEB_EN | DCRC_EN | DTO_EN | CIE_EN |
    CEB_EN | CCRC_EN | CTO_EN | BRR_EN | BWR_EN | TC_EN | CC_EN));

  //Controller INIT procedure start.
  MmioOr32 (MMCHS_CON, INIT);
  MmioWrite32 (MMCHS_CMD, 0x00000000);
  while (!(MmioRead32 (MMCHS_STAT) & CC));

  //Wait for 1 ms
  gBS->Stall(1000);

  //Set CC bit to 0x1 to clear the flag
  MmioOr32 (MMCHS_STAT, CC);

  //Retry INIT procedure.
  MmioWrite32 (MMCHS_CMD, 0x00000000);
  while (!(MmioRead32 (MMCHS_STAT) & CC));

  //End initialization sequence
  MmioAnd32 (MMCHS_CON, ~INIT);

  MmioOr32 (MMCHS_HCTL, (SDVS_3_0_V | DTW_1_BIT | SDBP_ON));

  //Change clock frequency to 400KHz to fit protocol
  UpdateMMCHSClkFrequency(CLKD_400KHZ);

  MmioOr32 (MMCHS_CON, OD);

  //Send CMD0 command.
  Status = SendCmd (CMD0, CMD0_INT_EN, CmdArgument);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "Cmd0 fails.\n"));
    return Status;
  }

  DEBUG ((EFI_D_INFO, "CMD0 response: %x\n", MmioRead32 (MMCHS_RSP10)));

  //Send CMD5 command.
  Status = SendCmd (CMD5, CMD5_INT_EN, CmdArgument);
  if (Status == EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "CMD5 Success. SDIO card. Follow SDIO card specification.\n"));
    DEBUG ((EFI_D_INFO, "CMD5 response: %x\n", MmioRead32 (MMCHS_RSP10)));
    //NOTE: Returning unsupported error for now. Need to implement SDIO specification.
    return EFI_UNSUPPORTED;
  } else {
    DEBUG ((EFI_D_INFO, "CMD5 fails. Not an SDIO card.\n"));
  }

  MmioOr32 (MMCHS_SYSCTL, SRC);
  gBS->Stall(1000);
  while ((MmioRead32 (MMCHS_SYSCTL) & SRC));

  //Send CMD8 command. (New v2.00 command for Voltage check)
  //Only 2.7V - 3.6V is supported for SD2.0, only SD 2.0 card can pass.
  //MMC & SD1.1 card will fail this command.
  CmdArgument = CMD8_ARG;
  Status = SendCmd (CMD8, CMD8_INT_EN, CmdArgument);
  if (Status == EFI_SUCCESS) {
    Response = MmioRead32 (MMCHS_RSP10);
    DEBUG ((EFI_D_INFO, "CMD8 success. CMD8 response: %x\n", Response));
    if (Response != CmdArgument) {
      return EFI_DEVICE_ERROR;
    }
    DEBUG ((EFI_D_INFO, "Card is SD2.0\n"));
    SDCmd8Supported = TRUE; //Supports high capacity.
  } else {
    DEBUG ((EFI_D_INFO, "CMD8 fails. Not an SD2.0 card.\n"));
  }

  MmioOr32 (MMCHS_SYSCTL, SRC);
  gBS->Stall(1000);
  while ((MmioRead32 (MMCHS_SYSCTL) & SRC));

  //Poll till card is busy
  while (RetryCount < MAX_RETRY_COUNT) {
    //Send CMD55 command.
    CmdArgument = 0;
    Status = SendCmd (CMD55, CMD55_INT_EN, CmdArgument);
    if (Status == EFI_SUCCESS) {
      DEBUG ((EFI_D_INFO, "CMD55 success. CMD55 response: %x\n", MmioRead32 (MMCHS_RSP10)));
      gCardInfo.CardType = SD_CARD;
    } else {
      DEBUG ((EFI_D_INFO, "CMD55 fails.\n"));
      gCardInfo.CardType = MMC_CARD;
    }

    //Send appropriate command for the card type which got detected.
    if (gCardInfo.CardType == SD_CARD) {
      CmdArgument = ((UINTN *) &(gCardInfo.OCRData))[0];

      //Set HCS bit.
      if (SDCmd8Supported) {
        CmdArgument |= HCS;
      }

      Status = SendCmd (ACMD41, ACMD41_INT_EN, CmdArgument);
      if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_INFO, "ACMD41 fails.\n"));
        return Status;
      }
      ((UINT32 *) &(gCardInfo.OCRData))[0] = MmioRead32 (MMCHS_RSP10);
      DEBUG ((EFI_D_INFO, "SD card detected. ACMD41 OCR: %x\n", ((UINT32 *) &(gCardInfo.OCRData))[0]));
    } else if (gCardInfo.CardType == MMC_CARD) {
      CmdArgument = 0;
      Status = SendCmd (CMD1, CMD1_INT_EN, CmdArgument);
      if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_INFO, "CMD1 fails.\n"));
        return Status;
      }
      Response = MmioRead32 (MMCHS_RSP10);
      DEBUG ((EFI_D_INFO, "MMC card detected.. CMD1 response: %x\n", Response));

      //NOTE: For now, I am skipping this since I only have an SD card.
      //Compare card OCR and host OCR (Section 22.6.1.3.2.4)
      return EFI_UNSUPPORTED; //For now, MMC is not supported.
    }

    //Poll the card until it is out of its power-up sequence.
    if (gCardInfo.OCRData.Busy == 1) {

      if (SDCmd8Supported) {
        gCardInfo.CardType = SD_CARD_2;
      }

      //Card is ready. Check CCS (Card capacity status) bit (bit#30).
      //SD 2.0 standard card will response with CCS 0, SD high capacity card will respond with CCS 1.
      if (gCardInfo.OCRData.AccessMode & BIT1) {
        gCardInfo.CardType = SD_CARD_2_HIGH;
        DEBUG ((EFI_D_INFO, "High capacity card.\n"));
      } else {
        DEBUG ((EFI_D_INFO, "Standard capacity card.\n"));
      }

      break;
    }

    gBS->Stall(1000);
    RetryCount++;
  }

  if (RetryCount == MAX_RETRY_COUNT) {
    DEBUG ((EFI_D_ERROR, "Timeout error. RetryCount: %d\n", RetryCount));
    return EFI_TIMEOUT;
  }

  //Read CID data.
  CmdArgument = 0;
  Status = SendCmd (CMD2, CMD2_INT_EN, CmdArgument);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "CMD2 fails. Status: %x\n", Status));
    return Status;
  }

  DEBUG ((EFI_D_INFO, "CMD2 response: %x %x %x %x\n", MmioRead32 (MMCHS_RSP10), MmioRead32 (MMCHS_RSP32), MmioRead32 (MMCHS_RSP54), MmioRead32 (MMCHS_RSP76)));

  //Parse CID register data.
  ParseCardCIDData(MmioRead32 (MMCHS_RSP10), MmioRead32 (MMCHS_RSP32), MmioRead32 (MMCHS_RSP54), MmioRead32 (MMCHS_RSP76));

  //Read RCA
  CmdArgument = 0;
  Status = SendCmd (CMD3, CMD3_INT_EN, CmdArgument);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "CMD3 fails. Status: %x\n", Status));
    return Status;
  }

  //Set RCA for the detected card. RCA is CMD3 response.
  gCardInfo.RCA = (MmioRead32 (MMCHS_RSP10) >> 16);
  DEBUG ((EFI_D_INFO, "CMD3 response: RCA %x\n", gCardInfo.RCA));

  //MMC Bus setting change after card identification.
  MmioAnd32 (MMCHS_CON, ~OD);
  MmioOr32 (MMCHS_HCTL, SDVS_3_0_V);
  UpdateMMCHSClkFrequency(CLKD_400KHZ); //Set the clock frequency to 400KHz.

  return EFI_SUCCESS;
}


EFI_STATUS
GetCardSpecificData (
  VOID
  )
{
  EFI_STATUS Status;
  UINTN      CmdArgument;

  //Send CMD9 to retrieve CSD.
  CmdArgument = gCardInfo.RCA << 16;
  Status = SendCmd (CMD9, CMD9_INT_EN, CmdArgument);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "CMD9 fails. Status: %x\n", Status));
    return Status;
  }

  //Populate 128-bit CSD register data.
  ((UINT32 *)&(gCardInfo.CSDData))[0] = MmioRead32 (MMCHS_RSP10);
  ((UINT32 *)&(gCardInfo.CSDData))[1] = MmioRead32 (MMCHS_RSP32);
  ((UINT32 *)&(gCardInfo.CSDData))[2] = MmioRead32 (MMCHS_RSP54);
  ((UINT32 *)&(gCardInfo.CSDData))[3] = MmioRead32 (MMCHS_RSP76);

  DEBUG ((EFI_D_INFO, "CMD9 response: %x %x %x %x\n", MmioRead32 (MMCHS_RSP10), MmioRead32 (MMCHS_RSP32), MmioRead32 (MMCHS_RSP54), MmioRead32 (MMCHS_RSP76)));

  //Calculate total number of blocks and max. data transfer rate supported by the detected card.
  GetCardConfigurationData();

  return Status;
}


EFI_STATUS
PerformCardConfiguration (
  VOID
  )
{
  UINTN      CmdArgument = 0;
  EFI_STATUS Status;

  //Send CMD7
  CmdArgument = gCardInfo.RCA << 16;
  Status = SendCmd (CMD7, CMD7_INT_EN, CmdArgument);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "CMD7 fails. Status: %x\n", Status));
    return Status;
  }

  if ((gCardInfo.CardType != UNKNOWN_CARD) && (gCardInfo.CardType != MMC_CARD)) {
    // We could read SCR register, but SD Card Phys spec stats any SD Card shall
    // set SCR.SD_BUS_WIDTHS to support 4-bit mode, so why bother?

    // Send ACMD6 (application specific commands must be prefixed with CMD55)
    Status = SendCmd (CMD55, CMD55_INT_EN, CmdArgument);
    if (!EFI_ERROR (Status)) {
      // set device into 4-bit data bus mode
      Status = SendCmd (ACMD6, ACMD6_INT_EN, 0x2);
      if (!EFI_ERROR (Status)) {
        // Set host controler into 4-bit mode
        MmioOr32 (MMCHS_HCTL, DTW_4_BIT);
        DEBUG ((EFI_D_INFO, "SD Memory Card set to 4-bit mode\n"));
      }
    }
  }

  //Send CMD16 to set the block length
  CmdArgument = gCardInfo.BlockSize;
  Status = SendCmd (CMD16, CMD16_INT_EN, CmdArgument);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "CMD16 fails. Status: %x\n", Status));
    return Status;
  }

  //Change MMCHS clock frequency to what detected card can support.
  UpdateMMCHSClkFrequency(gCardInfo.ClockFrequencySelect);

  return EFI_SUCCESS;
}


EFI_STATUS
ReadBlockData (
  IN  EFI_BLOCK_IO_PROTOCOL       *This,
  OUT VOID                        *Buffer
  )
{
  UINTN MmcStatus;
  UINTN *DataBuffer = Buffer;
  UINTN DataSize = This->Media->BlockSize/4;
  UINTN Count;
  UINTN RetryCount = 0;

  //Check controller status to make sure there is no error.
  while (RetryCount < MAX_RETRY_COUNT) {
    do {
      //Read Status.
      MmcStatus = MmioRead32 (MMCHS_STAT);
    } while(MmcStatus == 0);

    //Check if Buffer read ready (BRR) bit is set?
    if (MmcStatus & BRR) {

      //Clear BRR bit
      MmioOr32 (MMCHS_STAT, BRR);

      //Read block worth of data.
      for (Count = 0; Count < DataSize; Count++) {
        *DataBuffer++ = MmioRead32 (MMCHS_DATA);
      }
      break;
    }
    RetryCount++;
  }

  if (RetryCount == MAX_RETRY_COUNT) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
WriteBlockData (
  IN  EFI_BLOCK_IO_PROTOCOL       *This,
  OUT VOID                        *Buffer
  )
{
  UINTN MmcStatus;
  UINTN *DataBuffer = Buffer;
  UINTN DataSize = This->Media->BlockSize/4;
  UINTN Count;
  UINTN RetryCount = 0;

  //Check controller status to make sure there is no error.
  while (RetryCount < MAX_RETRY_COUNT) {
    do {
      //Read Status.
      MmcStatus = MmioRead32 (MMCHS_STAT);
    } while(MmcStatus == 0);

    //Check if Buffer write ready (BWR) bit is set?
    if (MmcStatus & BWR) {

      //Clear BWR bit
      MmioOr32 (MMCHS_STAT, BWR);

      //Write block worth of data.
      for (Count = 0; Count < DataSize; Count++) {
        MmioWrite32 (MMCHS_DATA, *DataBuffer++);
      }

      break;
    }
    RetryCount++;
  }

  if (RetryCount == MAX_RETRY_COUNT) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
DmaBlocks (
  IN EFI_BLOCK_IO_PROTOCOL        *This,
  IN  UINTN                       Lba,
  IN OUT VOID                     *Buffer,
  IN  UINTN                       BlockCount,
  IN  OPERATION_TYPE              OperationType
  )
{
  EFI_STATUS            Status;
  UINTN                 DmaSize = 0;
  UINTN                 Cmd = 0;
  UINTN                 CmdInterruptEnable;
  UINTN                 CmdArgument;
  VOID                  *BufferMap;
  EFI_PHYSICAL_ADDRESS  BufferAddress;
  OMAP_DMA4             Dma4;
  DMA_MAP_OPERATION     DmaOperation;
  EFI_STATUS            MmcStatus;
  UINTN                 RetryCount = 0;

CpuDeadLoop ();
  // Map passed in buffer for DMA xfer
  DmaSize = BlockCount * This->Media->BlockSize;
  Status = DmaMap (DmaOperation, Buffer, &DmaSize, &BufferAddress, &BufferMap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (&DmaOperation, sizeof (DMA_MAP_OPERATION));


  Dma4.DataType = 2;                      // DMA4_CSDPi[1:0]   32-bit elements from MMCHS_DATA

  Dma4.SourceEndiansim = 0;               // DMA4_CSDPi[21]

  Dma4.DestinationEndianism = 0;          // DMA4_CSDPi[19]

  Dma4.SourcePacked = 0;                  // DMA4_CSDPi[6]

  Dma4.DestinationPacked = 0;             // DMA4_CSDPi[13]

  Dma4.NumberOfElementPerFrame = This->Media->BlockSize/4; // DMA4_CENi  (TRM 4K is optimum value)

  Dma4.NumberOfFramePerTransferBlock = BlockCount;         // DMA4_CFNi

  Dma4.ReadPriority = 0;                  // DMA4_CCRi[6]      Low priority read

  Dma4.WritePriority = 0;                 // DMA4_CCRi[23]     Prefetech disabled


  //Populate the command information based on the operation type.
  if (OperationType == READ) {
    Cmd = CMD18; //Multiple block read
    CmdInterruptEnable = CMD18_INT_EN;
    DmaOperation = MapOperationBusMasterCommonBuffer;

    Dma4.ReadPortAccessType =0 ;            // DMA4_CSDPi[8:7]   Can not burst MMCHS_DATA reg

    Dma4.WritePortAccessType = 3;           // DMA4_CSDPi[15:14] Memory burst 16x32

    Dma4.WriteMode = 1;                     // DMA4_CSDPi[17:16] Write posted



    Dma4.SourceStartAddress = MMCHS_DATA;                   // DMA4_CSSAi

    Dma4.DestinationStartAddress = (UINT32)BufferAddress;   // DMA4_CDSAi

    Dma4.SourceElementIndex = 1;                            // DMA4_CSEi

    Dma4.SourceFrameIndex = 0x200;                          // DMA4_CSFi

    Dma4.DestinationElementIndex = 1;                       // DMA4_CDEi

    Dma4.DestinationFrameIndex = 0;                         // DMA4_CDFi



    Dma4.ReadPortAccessMode = 0;            // DMA4_CCRi[13:12]  Always read MMCHS_DATA

    Dma4.WritePortAccessMode = 1;           // DMA4_CCRi[15:14]  Post increment memory address

    Dma4.ReadRequestNumber = 0x1e;          // DMA4_CCRi[4:0]    Syncro with MMCA_DMA_RX (61)

    Dma4.WriteRequestNumber = 1;            // DMA4_CCRi[20:19]  Syncro upper 0x3e == 62 (one based)

  } else if (OperationType == WRITE) {
    Cmd = CMD25; //Multiple block write
    CmdInterruptEnable = CMD25_INT_EN;
    DmaOperation = MapOperationBusMasterRead;

    Dma4.ReadPortAccessType = 3;            // DMA4_CSDPi[8:7]   Memory burst 16x32

    Dma4.WritePortAccessType = 0;           // DMA4_CSDPi[15:14] Can not burst MMCHS_DATA reg

    Dma4.WriteMode = 1;                     // DMA4_CSDPi[17:16] Write posted ???



    Dma4.SourceStartAddress = (UINT32)BufferAddress;        // DMA4_CSSAi

    Dma4.DestinationStartAddress = MMCHS_DATA;              // DMA4_CDSAi

    Dma4.SourceElementIndex = 1;                            // DMA4_CSEi

    Dma4.SourceFrameIndex = 0x200;                          // DMA4_CSFi

    Dma4.DestinationElementIndex = 1;                       // DMA4_CDEi

    Dma4.DestinationFrameIndex = 0;                         // DMA4_CDFi



    Dma4.ReadPortAccessMode = 1;            // DMA4_CCRi[13:12]  Post increment memory address

    Dma4.WritePortAccessMode = 0;           // DMA4_CCRi[15:14]  Always write MMCHS_DATA

    Dma4.ReadRequestNumber = 0x1d;          // DMA4_CCRi[4:0]    Syncro with MMCA_DMA_TX (60)

    Dma4.WriteRequestNumber = 1;            // DMA4_CCRi[20:19]  Syncro upper 0x3d == 61 (one based)

  } else {
    return EFI_INVALID_PARAMETER;
  }


  EnableDmaChannel (2, &Dma4);


  //Set command argument based on the card access mode (Byte mode or Block mode)
  if (gCardInfo.OCRData.AccessMode & BIT1) {
    CmdArgument = Lba;
  } else {
    CmdArgument = Lba * This->Media->BlockSize;
  }

  //Send Command.
  Status = SendCmd (Cmd, CmdInterruptEnable, CmdArgument);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "CMD fails. Status: %x\n", Status));
    return Status;
  }

    //Check for the Transfer completion.
  while (RetryCount < MAX_RETRY_COUNT) {
    //Read Status
    do {
      MmcStatus = MmioRead32 (MMCHS_STAT);
    } while (MmcStatus == 0);

    //Check if Transfer complete (TC) bit is set?
    if (MmcStatus & TC) {
      break;
    } else {
      DEBUG ((EFI_D_ERROR, "MmcStatus for TC: %x\n", MmcStatus));
      //Check if DEB, DCRC or DTO interrupt occured.
      if ((MmcStatus & DEB) | (MmcStatus & DCRC) | (MmcStatus & DTO)) {
        //There was an error during the data transfer.

        //Set SRD bit to 1 and wait until it return to 0x0.
        MmioOr32 (MMCHS_SYSCTL, SRD);
        while((MmioRead32 (MMCHS_SYSCTL) & SRD) != 0x0);

        DisableDmaChannel (2, DMA4_CSR_BLOCK, DMA4_CSR_ERR);
        DmaUnmap (BufferMap);
        return EFI_DEVICE_ERROR;
      }
    }
    RetryCount++;
  }

  DisableDmaChannel (2, DMA4_CSR_BLOCK, DMA4_CSR_ERR);
  Status = DmaUnmap (BufferMap);

  if (RetryCount == MAX_RETRY_COUNT) {
    DEBUG ((EFI_D_ERROR, "TransferBlockData timed out.\n"));
    return EFI_TIMEOUT;
  }

  return Status;
}


EFI_STATUS
TransferBlock (
  IN EFI_BLOCK_IO_PROTOCOL        *This,
  IN  UINTN                       Lba,
  IN OUT VOID                     *Buffer,
  IN  OPERATION_TYPE              OperationType
  )
{
  EFI_STATUS Status;
  UINTN      MmcStatus;
  UINTN      RetryCount = 0;
  UINTN      Cmd = 0;
  UINTN      CmdInterruptEnable = 0;
  UINTN      CmdArgument = 0;


  //Populate the command information based on the operation type.
  if (OperationType == READ) {
    Cmd = CMD17; //Single block read
    CmdInterruptEnable = CMD18_INT_EN;
  } else if (OperationType == WRITE) {
    Cmd = CMD24; //Single block write
    CmdInterruptEnable = CMD24_INT_EN;
  }

  //Set command argument based on the card access mode (Byte mode or Block mode)
  if (gCardInfo.OCRData.AccessMode & BIT1) {
    CmdArgument = Lba;
  } else {
    CmdArgument = Lba * This->Media->BlockSize;
  }

  //Send Command.
  Status = SendCmd (Cmd, CmdInterruptEnable, CmdArgument);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "CMD fails. Status: %x\n", Status));
    return Status;
  }

  //Read or Write data.
  if (OperationType == READ) {
    Status = ReadBlockData (This, Buffer);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "ReadBlockData fails.\n"));
      return Status;
    }
  } else if (OperationType == WRITE) {
    Status = WriteBlockData (This, Buffer);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "WriteBlockData fails.\n"));
      return Status;
    }
  }

  //Check for the Transfer completion.
  while (RetryCount < MAX_RETRY_COUNT) {
    //Read Status
    do {
      MmcStatus = MmioRead32 (MMCHS_STAT);
    } while (MmcStatus == 0);

    //Check if Transfer complete (TC) bit is set?
    if (MmcStatus & TC) {
      break;
    } else {
      DEBUG ((EFI_D_ERROR, "MmcStatus for TC: %x\n", MmcStatus));
      //Check if DEB, DCRC or DTO interrupt occured.
      if ((MmcStatus & DEB) | (MmcStatus & DCRC) | (MmcStatus & DTO)) {
        //There was an error during the data transfer.

        //Set SRD bit to 1 and wait until it return to 0x0.
        MmioOr32 (MMCHS_SYSCTL, SRD);
        while((MmioRead32 (MMCHS_SYSCTL) & SRD) != 0x0);

        return EFI_DEVICE_ERROR;
      }
    }
    RetryCount++;
  }

  if (RetryCount == MAX_RETRY_COUNT) {
    DEBUG ((EFI_D_ERROR, "TransferBlockData timed out.\n"));
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

BOOLEAN
CardPresent (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  //
  // Card detect is a GPIO0 on the TPS65950
  //
  Status = gTPS65950->Read (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID2, GPIODATAIN1), 1, &Data);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if ((Data & CARD_DETECT_BIT) == CARD_DETECT_BIT) {
    // No Card present
    return FALSE;
  } else {
    return TRUE;
  }
}

EFI_STATUS
DetectCard (
  VOID
  )
{
  EFI_STATUS    Status;

  if (!CardPresent ()) {
    return EFI_NO_MEDIA;
  }

  //Initialize MMC host controller clocks.
  Status = InitializeMMCHS ();
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "Initialize MMC host controller fails. Status: %x\n", Status));
    return Status;
  }

  //Software reset of the MMCHS host controller.
  MmioWrite32 (MMCHS_SYSCONFIG, SOFTRESET);
  gBS->Stall(1000);
  while ((MmioRead32 (MMCHS_SYSSTATUS) & RESETDONE_MASK) != RESETDONE);

  //Soft reset for all.
  MmioWrite32 (MMCHS_SYSCTL, SRA);
  gBS->Stall(1000);
  while ((MmioRead32 (MMCHS_SYSCTL) & SRA) != 0x0);

  //Voltage capabilities initialization. Activate VS18 and VS30.
  MmioOr32 (MMCHS_CAPA, (VS30 | VS18));

  //Wakeup configuration
  MmioOr32 (MMCHS_SYSCONFIG, ENAWAKEUP);
  MmioOr32 (MMCHS_HCTL, IWE);

  //MMCHS Controller default initialization
  MmioOr32 (MMCHS_CON, (OD | DW8_1_4_BIT | CEATA_OFF));

  MmioWrite32 (MMCHS_HCTL, (SDVS_3_0_V | DTW_1_BIT | SDBP_OFF));

  //Enable internal clock
  MmioOr32 (MMCHS_SYSCTL, ICE);

  //Set the clock frequency to 80KHz.
  UpdateMMCHSClkFrequency (CLKD_80KHZ);

  //Enable SD bus power.
  MmioOr32 (MMCHS_HCTL, (SDBP_ON));

  //Poll till SD bus power bit is set.
  while ((MmioRead32 (MMCHS_HCTL) & SDBP_MASK) != SDBP_ON);

  //Card idenfication
  Status = PerformCardIdenfication ();
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "No MMC/SD card detected.\n"));
    return Status;
  }

  //Get CSD (Card specific data) for the detected card.
  Status = GetCardSpecificData();
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //Configure the card in data transfer mode.
  Status = PerformCardConfiguration();
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //Patch the Media structure.
  gMMCHSMedia.LastBlock    = (gCardInfo.NumBlocks - 1);
  gMMCHSMedia.BlockSize    = gCardInfo.BlockSize;
  gMMCHSMedia.ReadOnly     = (MmioRead32 (GPIO1_BASE + GPIO_DATAIN) & BIT23) == BIT23;
  gMMCHSMedia.MediaPresent = TRUE;
  gMMCHSMedia.MediaId++;

  DEBUG ((EFI_D_INFO, "SD Card Media Change on Handle 0x%08x\n", gImageHandle));

  return Status;
}

#define MAX_MMCHS_TRANSFER_SIZE  0x4000

EFI_STATUS
SdReadWrite (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN  UINTN                   Lba,
  OUT VOID                    *Buffer,
  IN  UINTN                   BufferSize,
  IN  OPERATION_TYPE          OperationType
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINTN      RetryCount = 0;
  UINTN      BlockCount;
  UINTN      BytesToBeTranferedThisPass = 0;
  UINTN      BytesRemainingToBeTransfered;
  EFI_TPL    OldTpl;

  BOOLEAN    Update;



  Update               = FALSE;

  if (gMediaChange) {
    Update = TRUE;
    Status = DetectCard  ();
    if (EFI_ERROR (Status)) {
      // We detected a removal
      gMMCHSMedia.MediaPresent = FALSE;
      gMMCHSMedia.LastBlock    = 0;
      gMMCHSMedia.BlockSize    = 512;  // Should be zero but there is a bug in DiskIo
      gMMCHSMedia.ReadOnly     = FALSE;
    }
    gMediaChange             = FALSE;
  } else if (!gMMCHSMedia.MediaPresent) {
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  if (Update) {
    DEBUG ((EFI_D_INFO, "SD Card ReinstallProtocolInterface ()\n"));
    gBS->ReinstallProtocolInterface (
          gImageHandle,
          &gEfiBlockIoProtocolGuid,
          &gBlockIo,
          &gBlockIo
          );
    return EFI_MEDIA_CHANGED;
  }

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  if (Buffer == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (Lba > This->Media->LastBlock) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if ((BufferSize % This->Media->BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Done;
  }

  //Check if the data lines are not in use.
  while ((RetryCount++ < MAX_RETRY_COUNT) && ((MmioRead32 (MMCHS_PSTATE) & DATI_MASK) != DATI_ALLOWED));
  if (RetryCount == MAX_RETRY_COUNT) {
    Status = EFI_TIMEOUT;
    goto Done;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  BytesRemainingToBeTransfered = BufferSize;
  while (BytesRemainingToBeTransfered > 0) {

    if (gMediaChange) {
      Status = EFI_NO_MEDIA;
      DEBUG ((EFI_D_INFO, "SdReadWrite() EFI_NO_MEDIA due to gMediaChange\n"));
      goto DoneRestoreTPL;
    }

    // Turn OFF DMA path until it is debugged
    // BytesToBeTranferedThisPass = (BytesToBeTranferedThisPass >= MAX_MMCHS_TRANSFER_SIZE) ? MAX_MMCHS_TRANSFER_SIZE : BytesRemainingToBeTransfered;
    BytesToBeTranferedThisPass   = This->Media->BlockSize;

    BlockCount = BytesToBeTranferedThisPass/This->Media->BlockSize;

    if (BlockCount > 1) {
      Status = DmaBlocks (This, Lba, Buffer, BlockCount, OperationType);
    } else {
      //Transfer a block worth of data.
      Status = TransferBlock (This, Lba, Buffer, OperationType);
    }

    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR, "TransferBlockData fails. %x\n", Status));
      goto DoneRestoreTPL;
    }

    BytesRemainingToBeTransfered -= BytesToBeTranferedThisPass;
    Lba    += BlockCount;
    Buffer = (UINT8 *)Buffer + This->Media->BlockSize;
  }

DoneRestoreTPL:

  gBS->RestoreTPL (OldTpl);

Done:

  return Status;

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
MMCHSReset (
  IN EFI_BLOCK_IO_PROTOCOL          *This,
  IN BOOLEAN                        ExtendedVerification
  )
{
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

EFI_STATUS

**/
EFI_STATUS
EFIAPI
MMCHSReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL          *This,
  IN UINT32                         MediaId,
  IN EFI_LBA                        Lba,
  IN UINTN                          BufferSize,
  OUT VOID                          *Buffer
  )
{
  EFI_STATUS Status;

  //Perform Read operation.
  Status = SdReadWrite (This, (UINTN)Lba, Buffer, BufferSize, READ);

  return Status;

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
MMCHSWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL          *This,
  IN UINT32                         MediaId,
  IN EFI_LBA                        Lba,
  IN UINTN                          BufferSize,
  IN VOID                           *Buffer
  )
{
  EFI_STATUS  Status;

  //Perform write operation.
  Status = SdReadWrite (This, (UINTN)Lba, Buffer, BufferSize, WRITE);


  return Status;

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
MMCHSFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}


EFI_BLOCK_IO_PROTOCOL gBlockIo = {
  EFI_BLOCK_IO_INTERFACE_REVISION,   // Revision
  &gMMCHSMedia,                      // *Media
  MMCHSReset,                        // Reset
  MMCHSReadBlocks,                   // ReadBlocks
  MMCHSWriteBlocks,                  // WriteBlocks
  MMCHSFlushBlocks                   // FlushBlocks
};


/**

  Timer callback to convert card present hardware into a boolean that indicates

  a media change event has happened. If you just check the GPIO you could see

  card 1 and then check again after card 1 was removed and card 2 was inserted

  and you would still see media present. Thus you need the timer tick to catch

  the toggle event.



  @param  Event                 Event whose notification function is being invoked.

  @param  Context               The pointer to the notification function's context,

                                which is implementation-dependent. Not used.



**/
VOID
EFIAPI
TimerCallback (
  IN  EFI_EVENT   Event,
  IN  VOID        *Context
  )
{
  BOOLEAN Present;

  Present = CardPresent ();
  if (gMMCHSMedia.MediaPresent) {
    if (!Present && !gMediaChange) {
      gMediaChange = TRUE;
    }
  } else {
    if (Present && !gMediaChange) {
      gMediaChange = TRUE;
    }
  }
}


EFI_STATUS
EFIAPI
MMCHSInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEmbeddedExternalDeviceProtocolGuid, NULL, (VOID **)&gTPS65950);
  ASSERT_EFI_ERROR(Status);

  ZeroMem (&gCardInfo, sizeof (CARD_INFO));

  Status = gBS->CreateEvent (EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, TimerCallback, NULL, &gTimerEvent);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->SetTimer (gTimerEvent, TimerPeriodic, FixedPcdGet32 (PcdMmchsTimerFreq100NanoSeconds));
  ASSERT_EFI_ERROR (Status);

  //Publish BlockIO.
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiBlockIoProtocolGuid,    &gBlockIo,
                  &gEfiDevicePathProtocolGuid, &gMmcHsDevicePath,
                  NULL
                  );
  return Status;
}

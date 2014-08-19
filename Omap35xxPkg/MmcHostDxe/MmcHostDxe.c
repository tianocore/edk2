/** @file
*
*  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.
*  Copyright (c) 2011 - 2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "MmcHostDxe.h"

EMBEDDED_EXTERNAL_DEVICE   *gTPS65950;
UINT8                      mMaxDataTransferRate = 0;
UINT32                     mRca = 0;
BOOLEAN                    mBitModeSet = FALSE;


typedef struct {
  VENDOR_DEVICE_PATH  Mmc;
  EFI_DEVICE_PATH     End;
} MMCHS_DEVICE_PATH;

MMCHS_DEVICE_PATH gMMCDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      { (UINT8)(sizeof(VENDOR_DEVICE_PATH)), (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8) },
    },
    { 0xb615f1f5, 0x5088, 0x43cd, { 0x80, 0x9c, 0xa1, 0x6e, 0x52, 0x48, 0x7d, 0x00 } }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
  }
};

BOOLEAN
IgnoreCommand (
  UINT32 Command
  )
{
  switch(Command) {
    case MMC_CMD12:
      return TRUE;
    case MMC_CMD13:
      return TRUE;
    default:
      return FALSE;
  }
}

UINT32
TranslateCommand (
  UINT32 Command
  )
{
  UINT32 Translation;

  switch(Command) {
    case MMC_CMD2:
      Translation = CMD2;
      break;
    case MMC_CMD3:
      Translation = CMD3;
      break;
    /*case MMC_CMD6:
      Translation = CMD6;
      break;*/
    case MMC_CMD7:
      Translation = CMD7;
      break;
    case MMC_CMD8:
      Translation = CMD8;
      break;
    case MMC_CMD9:
      Translation = CMD9;
      break;
    /*case MMC_CMD12:
      Translation = CMD12;
      break;
    case MMC_CMD13:
      Translation = CMD13;
      break;*/
    case MMC_CMD16:
      Translation = CMD16;
      break;
    case MMC_CMD17:
      Translation = 0x113A0014;//CMD17;
      break;
    case MMC_CMD24:
      Translation = CMD24 | 4;
      break;
    case MMC_CMD55:
      Translation = CMD55;
      break;
    case MMC_ACMD41:
      Translation = ACMD41;
      break;
    default:
      Translation = Command;
  }

  return Translation;
}

VOID
CalculateCardCLKD (
  UINTN *ClockFrequencySelect
  )
{
  UINTN    TransferRateValue = 0;
  UINTN    TimeValue = 0 ;
  UINTN    Frequency = 0;

  DEBUG ((DEBUG_BLKIO, "CalculateCardCLKD()\n"));

  // For SD Cards  we would need to send CMD6 to set
  // speeds abouve 25MHz. High Speed mode 50 MHz and up

  // Calculate Transfer rate unit (Bits 2:0 of TRAN_SPEED)
  switch (mMaxDataTransferRate & 0x7) { // 2
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
      DEBUG ((DEBUG_BLKIO, "Invalid parameter.\n"));
      ASSERT(FALSE);
      return;
  }

  //Calculate Time value (Bits 6:3 of TRAN_SPEED)
  switch ((mMaxDataTransferRate >> 3) & 0xF) { // 6
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
      DEBUG ((DEBUG_BLKIO, "Invalid parameter.\n"));
      ASSERT(FALSE);
      return;
  }

  Frequency = TransferRateValue * TimeValue/10;

  // Calculate Clock divider value to program in MMCHS_SYSCTL[CLKD] field.
  *ClockFrequencySelect = ((MMC_REFERENCE_CLK/Frequency) + 1);

  DEBUG ((DEBUG_BLKIO, "mMaxDataTransferRate: 0x%x, Frequency: %d KHz, ClockFrequencySelect: %x\n", mMaxDataTransferRate, Frequency/1000, *ClockFrequencySelect));
}

VOID
UpdateMMCHSClkFrequency (
  UINTN NewCLKD
  )
{
  DEBUG ((DEBUG_BLKIO, "UpdateMMCHSClkFrequency()\n"));

  // Set Clock enable to 0x0 to not provide the clock to the card
  MmioAnd32 (MMCHS_SYSCTL, ~CEN);

  // Set new clock frequency.
  MmioAndThenOr32 (MMCHS_SYSCTL, ~CLKD_MASK, NewCLKD << 6);

  // Poll till Internal Clock Stable
  while ((MmioRead32 (MMCHS_SYSCTL) & ICS_MASK) != ICS);

  // Set Clock enable to 0x1 to provide the clock to the card
  MmioOr32 (MMCHS_SYSCTL, CEN);
}

EFI_STATUS
InitializeMMCHS (
  VOID
  )
{
  UINT8      Data;
  EFI_STATUS Status;

  DEBUG ((DEBUG_BLKIO, "InitializeMMCHS()\n"));

  // Select Device group to belong to P1 device group in Power IC.
  Data = DEV_GRP_P1;
  Status = gTPS65950->Write (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID4, VMMC1_DEV_GRP), 1, &Data);
  ASSERT_EFI_ERROR(Status);

  // Configure voltage regulator for MMC1 in Power IC to output 3.0 voltage.
  Data = VSEL_3_00V;
  Status = gTPS65950->Write (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID4, VMMC1_DEDICATED_REG), 1, &Data);
  ASSERT_EFI_ERROR(Status);

  // After ramping up voltage, set VDDS stable bit to indicate that voltage level is stable.
  MmioOr32 (CONTROL_PBIAS_LITE, (PBIASLITEVMODE0 | PBIASLITEPWRDNZ0 | PBIASSPEEDCTRL0 | PBIASLITEVMODE1 | PBIASLITEWRDNZ1));

  // Enable WP GPIO
  MmioAndThenOr32 (GPIO1_BASE + GPIO_OE, ~BIT23, BIT23);

  // Enable Card Detect
  Data = CARD_DETECT_ENABLE;
  gTPS65950->Write (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID2, TPS65950_GPIO_CTRL), 1, &Data);

  return Status;
}

BOOLEAN
MMCIsCardPresent (
  IN EFI_MMC_HOST_PROTOCOL     *This
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

  return !(Data & CARD_DETECT_BIT);
}

BOOLEAN
MMCIsReadOnly (
  IN EFI_MMC_HOST_PROTOCOL     *This
  )
{
  /* Note:
   * On our BeagleBoard the SD card WP pin is always read as TRUE.
   * Probably something wrong with GPIO configuration.
   * BeagleBoard-xM uses microSD cards so there is no write protect at all.
   * Hence commenting out SD card WP pin read status.
   */
  //return (MmioRead32 (GPIO1_BASE + GPIO_DATAIN) & BIT23) == BIT23;
  return 0;

}

// TODO
EFI_GUID mPL180MciDevicePathGuid = EFI_CALLER_ID_GUID;

EFI_STATUS
MMCBuildDevicePath (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL    *NewDevicePathNode;

  NewDevicePathNode = CreateDeviceNode(HARDWARE_DEVICE_PATH,HW_VENDOR_DP,sizeof(VENDOR_DEVICE_PATH));
  CopyGuid(&((VENDOR_DEVICE_PATH*)NewDevicePathNode)->Guid,&mPL180MciDevicePathGuid);
  *DevicePath = NewDevicePathNode;
  return EFI_SUCCESS;
}

EFI_STATUS
MMCSendCommand (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN MMC_CMD                   MmcCmd,
  IN UINT32                    Argument
  )
{
  UINTN MmcStatus;
  UINTN RetryCount = 0;

  if (IgnoreCommand(MmcCmd))
    return EFI_SUCCESS;

  MmcCmd = TranslateCommand(MmcCmd);

  //DEBUG ((EFI_D_ERROR, "MMCSendCommand(%d)\n", MmcCmd));

  // Check if command line is in use or not. Poll till command line is available.
  while ((MmioRead32 (MMCHS_PSTATE) & DATI_MASK) == DATI_NOT_ALLOWED);

  // Provide the block size.
  MmioWrite32 (MMCHS_BLK, BLEN_512BYTES);

  // Setting Data timeout counter value to max value.
  MmioAndThenOr32 (MMCHS_SYSCTL, ~DTO_MASK, DTO_VAL);

  // Clear Status register.
  MmioWrite32 (MMCHS_STAT, 0xFFFFFFFF);

  // Set command argument register
  MmioWrite32 (MMCHS_ARG, Argument);

  //TODO: fix this
  //Enable interrupt enable events to occur
  //MmioWrite32 (MMCHS_IE, CmdInterruptEnableVal);

  // Send a command
  MmioWrite32 (MMCHS_CMD, MmcCmd);

  // Check for the command status.
  while (RetryCount < MAX_RETRY_COUNT) {
    do {
      MmcStatus = MmioRead32 (MMCHS_STAT);
    } while (MmcStatus == 0);

    // Read status of command response
    if ((MmcStatus & ERRI) != 0) {

      // Perform soft-reset for mmci_cmd line.
      MmioOr32 (MMCHS_SYSCTL, SRC);
      while ((MmioRead32 (MMCHS_SYSCTL) & SRC));

      //DEBUG ((EFI_D_INFO, "MmcStatus: 0x%x\n", MmcStatus));
      return EFI_DEVICE_ERROR;
    }

    // Check if command is completed.
    if ((MmcStatus & CC) == CC) {
      MmioWrite32 (MMCHS_STAT, CC);
      break;
    }

    RetryCount++;
  }

  if (RetryCount == MAX_RETRY_COUNT) {
    DEBUG ((DEBUG_BLKIO, "MMCSendCommand: Timeout\n"));
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
MMCNotifyState (
  IN EFI_MMC_HOST_PROTOCOL    *This,
  IN MMC_STATE                State
  )
{
  EFI_STATUS              Status;
  UINTN                   FreqSel;

  switch(State) {
    case MmcInvalidState:
      ASSERT(0);
      break;
    case MmcHwInitializationState:
      mBitModeSet = FALSE;

      DEBUG ((DEBUG_BLKIO, "MMCHwInitializationState()\n"));
      Status = InitializeMMCHS ();
      if (EFI_ERROR(Status)) {
        DEBUG ((DEBUG_BLKIO, "Initialize MMC host controller fails. Status: %x\n", Status));
        return Status;
      }

      // Software reset of the MMCHS host controller.
      MmioWrite32 (MMCHS_SYSCONFIG, SOFTRESET);
      gBS->Stall(1000);
      while ((MmioRead32 (MMCHS_SYSSTATUS) & RESETDONE_MASK) != RESETDONE);

      // Soft reset for all.
      MmioWrite32 (MMCHS_SYSCTL, SRA);
      gBS->Stall(1000);
      while ((MmioRead32 (MMCHS_SYSCTL) & SRA) != 0x0);

      //Voltage capabilities initialization. Activate VS18 and VS30.
      MmioOr32 (MMCHS_CAPA, (VS30 | VS18));

      // Wakeup configuration
      MmioOr32 (MMCHS_SYSCONFIG, ENAWAKEUP);
      MmioOr32 (MMCHS_HCTL, IWE);

      // MMCHS Controller default initialization
      MmioOr32 (MMCHS_CON, (OD | DW8_1_4_BIT | CEATA_OFF));

      MmioWrite32 (MMCHS_HCTL, (SDVS_3_0_V | DTW_1_BIT | SDBP_OFF));

      // Enable internal clock
      MmioOr32 (MMCHS_SYSCTL, ICE);

      // Set the clock frequency to 80KHz.
      UpdateMMCHSClkFrequency (CLKD_80KHZ);

      // Enable SD bus power.
      MmioOr32 (MMCHS_HCTL, (SDBP_ON));

      // Poll till SD bus power bit is set.
      while ((MmioRead32 (MMCHS_HCTL) & SDBP_MASK) != SDBP_ON);

      // Enable interrupts.
      MmioWrite32 (MMCHS_IE, (BADA_EN | CERR_EN | DEB_EN | DCRC_EN | DTO_EN | CIE_EN |
        CEB_EN | CCRC_EN | CTO_EN | BRR_EN | BWR_EN | TC_EN | CC_EN));

      // Controller INIT procedure start.
      MmioOr32 (MMCHS_CON, INIT);
      MmioWrite32 (MMCHS_CMD, 0x00000000);
      while (!(MmioRead32 (MMCHS_STAT) & CC));

      // Wait for 1 ms
      gBS->Stall (1000);

      // Set CC bit to 0x1 to clear the flag
      MmioOr32 (MMCHS_STAT, CC);

      // Retry INIT procedure.
      MmioWrite32 (MMCHS_CMD, 0x00000000);
      while (!(MmioRead32 (MMCHS_STAT) & CC));

      // End initialization sequence
      MmioAnd32 (MMCHS_CON, ~INIT);

      MmioOr32 (MMCHS_HCTL, (SDVS_3_0_V | DTW_1_BIT | SDBP_ON));

      // Change clock frequency to 400KHz to fit protocol
      UpdateMMCHSClkFrequency(CLKD_400KHZ);

      MmioOr32 (MMCHS_CON, OD);
      break;
    case MmcIdleState:
      break;
    case MmcReadyState:
      break;
    case MmcIdentificationState:
      break;
    case MmcStandByState:
      CalculateCardCLKD (&FreqSel);
      UpdateMMCHSClkFrequency (FreqSel);
      break;
    case MmcTransferState:
      if (!mBitModeSet) {
        Status = MMCSendCommand (This, CMD55, mRca << 16);
        if (!EFI_ERROR (Status)) {
          // Set device into 4-bit data bus mode
          Status = MMCSendCommand (This, ACMD6, 0x2);
          if (!EFI_ERROR (Status)) {
            // Set host controler into 4-bit mode
            MmioOr32 (MMCHS_HCTL, DTW_4_BIT);
            DEBUG ((DEBUG_BLKIO, "SD Memory Card set to 4-bit mode\n"));
            mBitModeSet = TRUE;
          }
        }
      }
      break;
    case MmcSendingDataState:
      break;
    case MmcReceiveDataState:
      break;
    case MmcProgrammingState:
      break;
    case MmcDisconnectState:
    default:
      ASSERT(0);
  }
  return EFI_SUCCESS;
}

EFI_STATUS
MMCReceiveResponse (
  IN EFI_MMC_HOST_PROTOCOL     *This,
  IN MMC_RESPONSE_TYPE         Type,
  IN UINT32*                   Buffer
  )
{
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Type == MMC_RESPONSE_TYPE_R2) {
    Buffer[0] = MmioRead32 (MMCHS_RSP10);
    Buffer[1] = MmioRead32 (MMCHS_RSP32);
    Buffer[2] = MmioRead32 (MMCHS_RSP54);
    Buffer[3] = MmioRead32 (MMCHS_RSP76);
  } else {
    Buffer[0] = MmioRead32 (MMCHS_RSP10);
  }

  if (Type == MMC_RESPONSE_TYPE_CSD) {
    mMaxDataTransferRate = Buffer[3] & 0xFF;
  } else if (Type == MMC_RESPONSE_TYPE_RCA) {
    mRca = Buffer[0] >> 16;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
MMCReadBlockData (
  IN EFI_MMC_HOST_PROTOCOL      *This,
  IN EFI_LBA                    Lba,
  IN UINTN                      Length,
  IN UINT32*                    Buffer
  )
{
  UINTN MmcStatus;
  UINTN Count;
  UINTN RetryCount = 0;

  DEBUG ((DEBUG_BLKIO, "MMCReadBlockData(LBA: 0x%x, Length: 0x%x, Buffer: 0x%x)\n", Lba, Length, Buffer));

  // Check controller status to make sure there is no error.
  while (RetryCount < MAX_RETRY_COUNT) {
    do {
      // Read Status.
      MmcStatus = MmioRead32 (MMCHS_STAT);
    } while(MmcStatus == 0);

    // Check if Buffer read ready (BRR) bit is set?
    if (MmcStatus & BRR) {

      // Clear BRR bit
      MmioOr32 (MMCHS_STAT, BRR);

      for (Count = 0; Count < Length / 4; Count++) {
        *Buffer++ = MmioRead32(MMCHS_DATA);
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
MMCWriteBlockData (
  IN EFI_MMC_HOST_PROTOCOL    *This,
  IN EFI_LBA                  Lba,
  IN UINTN                    Length,
  IN UINT32*                  Buffer
  )
{
  UINTN MmcStatus;
  UINTN Count;
  UINTN RetryCount = 0;

  // Check controller status to make sure there is no error.
  while (RetryCount < MAX_RETRY_COUNT) {
    do {
      // Read Status.
      MmcStatus = MmioRead32 (MMCHS_STAT);
    } while(MmcStatus == 0);

    // Check if Buffer write ready (BWR) bit is set?
    if (MmcStatus & BWR) {

      // Clear BWR bit
      MmioOr32 (MMCHS_STAT, BWR);

      // Write block worth of data.
      for (Count = 0; Count < Length / 4; Count++) {
        MmioWrite32 (MMCHS_DATA, *Buffer++);
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

EFI_MMC_HOST_PROTOCOL gMMCHost = {
  MMC_HOST_PROTOCOL_REVISION,
  MMCIsCardPresent,
  MMCIsReadOnly,
  MMCBuildDevicePath,
  MMCNotifyState,
  MMCSendCommand,
  MMCReceiveResponse,
  MMCReadBlockData,
  MMCWriteBlockData
};

EFI_STATUS
MMCInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    Handle = NULL;

  DEBUG ((DEBUG_BLKIO, "MMCInitialize()\n"));

  Status = gBS->LocateProtocol (&gEmbeddedExternalDeviceProtocolGuid, NULL, (VOID **)&gTPS65950);
  ASSERT_EFI_ERROR(Status);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiMmcHostProtocolGuid,         &gMMCHost,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

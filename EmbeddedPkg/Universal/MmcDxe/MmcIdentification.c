/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>

#include "Mmc.h"

typedef union {
  UINT32    Raw;
  OCR       Ocr;
} OCR_RESPONSE;

#define MAX_RETRY_COUNT        1000
#define CMD_RETRY_COUNT        20
#define RCA_SHIFT_OFFSET       16
#define EMMC_CARD_SIZE         512
#define EMMC_ECSD_SIZE_OFFSET  53

#define EXTCSD_BUS_WIDTH  183
#define EXTCSD_HS_TIMING  185

#define EMMC_TIMING_BACKWARD  0
#define EMMC_TIMING_HS        1
#define EMMC_TIMING_HS200     2
#define EMMC_TIMING_HS400     3

#define EMMC_BUS_WIDTH_1BIT      0
#define EMMC_BUS_WIDTH_4BIT      1
#define EMMC_BUS_WIDTH_8BIT      2
#define EMMC_BUS_WIDTH_DDR_4BIT  5
#define EMMC_BUS_WIDTH_DDR_8BIT  6

#define EMMC_SWITCH_ERROR  (1 << 7)

#define SD_BUS_WIDTH_1BIT  (1 << 0)
#define SD_BUS_WIDTH_4BIT  (1 << 2)

#define SD_CCC_SWITCH  (1 << 10)

#define DEVICE_STATE(x)  (((x) >> 9) & 0xf)
typedef enum _EMMC_DEVICE_STATE {
  EMMC_IDLE_STATE = 0,
  EMMC_READY_STATE,
  EMMC_IDENT_STATE,
  EMMC_STBY_STATE,
  EMMC_TRAN_STATE,
  EMMC_DATA_STATE,
  EMMC_RCV_STATE,
  EMMC_PRG_STATE,
  EMMC_DIS_STATE,
  EMMC_BTST_STATE,
  EMMC_SLP_STATE
} EMMC_DEVICE_STATE;

UINT32  mEmmcRcaCount = 0;

STATIC
EFI_STATUS
EFIAPI
EmmcGetDeviceState (
  IN  MMC_HOST_INSTANCE  *MmcHostInstance,
  OUT EMMC_DEVICE_STATE  *State
  )
{
  EFI_MMC_HOST_PROTOCOL  *Host;
  EFI_STATUS             Status;
  UINT32                 Data, RCA;

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Host   = MmcHostInstance->MmcHost;
  RCA    = MmcHostInstance->CardInfo.RCA << RCA_SHIFT_OFFSET;
  Status = Host->SendCommand (Host, MMC_CMD13, RCA);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcGetDeviceState(): Failed to get card status, Status=%r.\n", Status));
    return Status;
  }

  Status = Host->ReceiveResponse (Host, MMC_RESPONSE_TYPE_R1, &Data);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcGetDeviceState(): Failed to get response of CMD13, Status=%r.\n", Status));
    return Status;
  }

  if (Data & EMMC_SWITCH_ERROR) {
    DEBUG ((DEBUG_ERROR, "EmmcGetDeviceState(): Failed to switch expected mode, Status=%r.\n", Status));
    return EFI_DEVICE_ERROR;
  }

  *State = DEVICE_STATE (Data);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
EmmcSetEXTCSD (
  IN MMC_HOST_INSTANCE  *MmcHostInstance,
  UINT32                ExtCmdIndex,
  UINT32                Value
  )
{
  EFI_MMC_HOST_PROTOCOL  *Host;
  EMMC_DEVICE_STATE      State;
  EFI_STATUS             Status;
  UINT32                 Argument;

  Host     = MmcHostInstance->MmcHost;
  Argument = EMMC_CMD6_ARG_ACCESS (3) | EMMC_CMD6_ARG_INDEX (ExtCmdIndex) |
             EMMC_CMD6_ARG_VALUE (Value) | EMMC_CMD6_ARG_CMD_SET (1);
  Status = Host->SendCommand (Host, MMC_CMD6, Argument);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcSetEXTCSD(): Failed to send CMD6, Status=%r.\n", Status));
    return Status;
  }

  // Make sure device exiting prog mode
  do {
    Status = EmmcGetDeviceState (MmcHostInstance, &State);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "EmmcSetEXTCSD(): Failed to get device state, Status=%r.\n", Status));
      return Status;
    }
  } while (State == EMMC_PRG_STATE);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
EmmcIdentificationMode (
  IN MMC_HOST_INSTANCE  *MmcHostInstance,
  IN OCR_RESPONSE       Response
  )
{
  EFI_MMC_HOST_PROTOCOL  *Host;
  EFI_BLOCK_IO_MEDIA     *Media;
  EFI_STATUS             Status;
  EMMC_DEVICE_STATE      State;
  UINT32                 RCA;

  Host  = MmcHostInstance->MmcHost;
  Media = MmcHostInstance->BlockIo.Media;

  // Fetch card identity register
  Status = Host->SendCommand (Host, MMC_CMD2, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcIdentificationMode(): Failed to send CMD2, Status=%r.\n", Status));
    return Status;
  }

  Status = Host->ReceiveResponse (Host, MMC_RESPONSE_TYPE_R2, (UINT32 *)&(MmcHostInstance->CardInfo.CIDData));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcIdentificationMode(): CID retrieval error, Status=%r.\n", Status));
    return Status;
  }

  // Assign a relative address value to the card
  MmcHostInstance->CardInfo.RCA = ++mEmmcRcaCount; // TODO: might need a more sophisticated way of doing this
  RCA                           = MmcHostInstance->CardInfo.RCA << RCA_SHIFT_OFFSET;
  Status                        = Host->SendCommand (Host, MMC_CMD3, RCA);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcIdentificationMode(): RCA set error, Status=%r.\n", Status));
    return Status;
  }

  // Fetch card specific data
  Status = Host->SendCommand (Host, MMC_CMD9, RCA);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcIdentificationMode(): Failed to send CMD9, Status=%r.\n", Status));
    return Status;
  }

  Status = Host->ReceiveResponse (Host, MMC_RESPONSE_TYPE_R2, (UINT32 *)&(MmcHostInstance->CardInfo.CSDData));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcIdentificationMode(): CSD retrieval error, Status=%r.\n", Status));
    return Status;
  }

  // Select the card
  Status = Host->SendCommand (Host, MMC_CMD7, RCA);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcIdentificationMode(): Card selection error, Status=%r.\n", Status));
  }

  if (MMC_HOST_HAS_SETIOS (Host)) {
    // Set 1-bit bus width
    Status = Host->SetIos (Host, 0, 1, EMMCBACKWARD);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "EmmcIdentificationMode(): Set 1-bit bus width error, Status=%r.\n", Status));
      return Status;
    }

    // Set 1-bit bus width for EXTCSD
    Status = EmmcSetEXTCSD (MmcHostInstance, EXTCSD_BUS_WIDTH, EMMC_BUS_WIDTH_1BIT);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "EmmcIdentificationMode(): Set extcsd bus width error, Status=%r.\n", Status));
      return Status;
    }
  }

  // Fetch ECSD
  MmcHostInstance->CardInfo.ECSDData = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (ECSD)));
  if (MmcHostInstance->CardInfo.ECSDData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = Host->SendCommand (Host, MMC_CMD8, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcIdentificationMode(): ECSD fetch error, Status=%r.\n", Status));
  }

  Status = Host->ReadBlockData (Host, 0, 512, (UINT32 *)MmcHostInstance->CardInfo.ECSDData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcIdentificationMode(): ECSD read error, Status=%r.\n", Status));
    goto FreePageExit;
  }

  // Make sure device exiting data mode
  do {
    Status = EmmcGetDeviceState (MmcHostInstance, &State);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "EmmcIdentificationMode(): Failed to get device state, Status=%r.\n", Status));
      goto FreePageExit;
    }
  } while (State == EMMC_DATA_STATE);

  // Set up media
  Media->BlockSize                     = EMMC_CARD_SIZE; // 512-byte support is mandatory for eMMC cards
  Media->MediaId                       = MmcHostInstance->CardInfo.CIDData.PSN;
  Media->ReadOnly                      = MmcHostInstance->CardInfo.CSDData.PERM_WRITE_PROTECT;
  Media->LogicalBlocksPerPhysicalBlock = 1;
  Media->IoAlign                       = 4;
  // Compute last block using bits [215:212] of the ECSD
  Media->LastBlock = MmcHostInstance->CardInfo.ECSDData->SECTOR_COUNT - 1; // eMMC isn't supposed to report this for
  // Cards <2GB in size, but the model does.

  // Setup card type
  MmcHostInstance->CardInfo.CardType = EMMC_CARD;
  return EFI_SUCCESS;

FreePageExit:
  FreePages (MmcHostInstance->CardInfo.ECSDData, EFI_SIZE_TO_PAGES (sizeof (ECSD)));
  return Status;
}

STATIC
EFI_STATUS
InitializeEmmcDevice (
  IN  MMC_HOST_INSTANCE  *MmcHostInstance
  )
{
  EFI_MMC_HOST_PROTOCOL  *Host;
  EFI_STATUS             Status = EFI_SUCCESS;
  ECSD                   *ECSDData;
  UINT32                 BusClockFreq, Idx, BusMode;
  UINT32                 TimingMode[4] = { EMMCHS52DDR1V2, EMMCHS52DDR1V8, EMMCHS52, EMMCHS26 };

  Host     = MmcHostInstance->MmcHost;
  ECSDData = MmcHostInstance->CardInfo.ECSDData;
  if (ECSDData->DEVICE_TYPE == EMMCBACKWARD) {
    return EFI_SUCCESS;
  }

  if (!MMC_HOST_HAS_SETIOS (Host)) {
    return EFI_SUCCESS;
  }

  Status = EmmcSetEXTCSD (MmcHostInstance, EXTCSD_HS_TIMING, EMMC_TIMING_HS);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "InitializeEmmcDevice(): Failed to switch high speed mode, Status:%r.\n", Status));
    return Status;
  }

  for (Idx = 0; Idx < 4; Idx++) {
    switch (TimingMode[Idx]) {
      case EMMCHS52DDR1V2:
      case EMMCHS52DDR1V8:
      case EMMCHS52:
        BusClockFreq = 52000000;
        break;
      case EMMCHS26:
        BusClockFreq = 26000000;
        break;
      default:
        return EFI_UNSUPPORTED;
    }

    Status = Host->SetIos (Host, BusClockFreq, 8, TimingMode[Idx]);
    if (!EFI_ERROR (Status)) {
      switch (TimingMode[Idx]) {
        case EMMCHS52DDR1V2:
        case EMMCHS52DDR1V8:
          BusMode = EMMC_BUS_WIDTH_DDR_8BIT;
          break;
        case EMMCHS52:
        case EMMCHS26:
          BusMode = EMMC_BUS_WIDTH_8BIT;
          break;
        default:
          return EFI_UNSUPPORTED;
      }

      Status = EmmcSetEXTCSD (MmcHostInstance, EXTCSD_BUS_WIDTH, BusMode);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "InitializeEmmcDevice(): Failed to set EXTCSD bus width, Status:%r\n", Status));
      }

      return Status;
    }
  }

  return Status;
}

STATIC
UINT32
CreateSwitchCmdArgument (
  IN  UINT32  Mode,
  IN  UINT8   Group,
  IN  UINT8   Value
  )
{
  UINT32  Argument;

  Argument  = Mode << 31 | 0x00FFFFFF;
  Argument &= ~(0xF << (Group * 4));
  Argument |= Value << (Group * 4);

  return Argument;
}

STATIC
EFI_STATUS
InitializeSdMmcDevice (
  IN  MMC_HOST_INSTANCE  *MmcHostInstance
  )
{
  UINT32                 CmdArg;
  UINT32                 Response[4];
  UINT32                 Buffer[128];
  UINT32                 Speed;
  UINTN                  BlockSize;
  UINTN                  CardSize;
  UINTN                  NumBlocks;
  BOOLEAN                CccSwitch;
  SCR                    Scr;
  EFI_STATUS             Status;
  EFI_MMC_HOST_PROTOCOL  *MmcHost;

  Speed   = SD_DEFAULT_SPEED;
  MmcHost = MmcHostInstance->MmcHost;

  // Send a command to get Card specific data
  CmdArg = MmcHostInstance->CardInfo.RCA << 16;
  Status = MmcHost->SendCommand (MmcHost, MMC_CMD9, CmdArg);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "InitializeSdMmcDevice(MMC_CMD9): Error, Status=%r\n", Status));
    return Status;
  }

  // Read Response
  Status = MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_CSD, Response);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "InitializeSdMmcDevice(): Failed to receive CSD, Status=%r\n", Status));
    return Status;
  }

  PrintCSD (Response);
  if (MMC_CSD_GET_CCC (Response) & SD_CCC_SWITCH) {
    CccSwitch = TRUE;
  } else {
    CccSwitch = FALSE;
  }

  if (MmcHostInstance->CardInfo.CardType == SD_CARD_2_HIGH) {
    CardSize  = HC_MMC_CSD_GET_DEVICESIZE (Response);
    NumBlocks = ((CardSize + 1) * 1024);
    BlockSize = 1 << MMC_CSD_GET_READBLLEN (Response);
  } else {
    CardSize  = MMC_CSD_GET_DEVICESIZE (Response);
    NumBlocks = (CardSize + 1) * (1 << (MMC_CSD_GET_DEVICESIZEMULT (Response) + 2));
    BlockSize = 1 << MMC_CSD_GET_READBLLEN (Response);
  }

  // For >=2G card, BlockSize may be 1K, but the transfer size is 512 bytes.
  if (BlockSize > 512) {
    NumBlocks = MultU64x32 (NumBlocks, BlockSize / 512);
    BlockSize = 512;
  }

  MmcHostInstance->BlockIo.Media->LastBlock    = (NumBlocks - 1);
  MmcHostInstance->BlockIo.Media->BlockSize    = BlockSize;
  MmcHostInstance->BlockIo.Media->ReadOnly     = MmcHost->IsReadOnly (MmcHost);
  MmcHostInstance->BlockIo.Media->MediaPresent = TRUE;
  MmcHostInstance->BlockIo.Media->MediaId++;

  CmdArg = MmcHostInstance->CardInfo.RCA << 16;
  Status = MmcHost->SendCommand (MmcHost, MMC_CMD7, CmdArg);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "InitializeSdMmcDevice(MMC_CMD7): Error and Status = %r\n", Status));
    return Status;
  }

  Status = MmcHost->SendCommand (MmcHost, MMC_CMD55, CmdArg);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a (MMC_CMD55): Error and Status = %r\n", __func__, Status));
    return Status;
  }

  Status = MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_R1, Response);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a (MMC_CMD55): Error and Status = %r\n", __func__, Status));
    return Status;
  }

  if ((Response[0] & MMC_STATUS_APP_CMD) == 0) {
    return EFI_SUCCESS;
  }

  /* SCR */
  Status = MmcHost->SendCommand (MmcHost, MMC_ACMD51, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(MMC_ACMD51): Error and Status = %r\n", __func__, Status));
    return Status;
  } else {
    Status = MmcHost->ReadBlockData (MmcHost, 0, 8, Buffer);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a(MMC_ACMD51): ReadBlockData Error and Status = %r\n", __func__, Status));
      return Status;
    }

    CopyMem (&Scr, Buffer, 8);
    if (Scr.SD_SPEC == 2) {
      if (Scr.SD_SPEC3 == 1) {
        if (Scr.SD_SPEC4 == 1) {
          DEBUG ((DEBUG_INFO, "Found SD Card for Spec Version 4.xx\n"));
        } else {
          DEBUG ((DEBUG_INFO, "Found SD Card for Spec Version 3.0x\n"));
        }
      } else {
        if (Scr.SD_SPEC4 == 0) {
          DEBUG ((DEBUG_INFO, "Found SD Card for Spec Version 2.0\n"));
        } else {
          DEBUG ((DEBUG_ERROR, "Found invalid SD Card\n"));
        }
      }
    } else {
      if ((Scr.SD_SPEC3 == 0) && (Scr.SD_SPEC4 == 0)) {
        if (Scr.SD_SPEC == 1) {
          DEBUG ((DEBUG_INFO, "Found SD Card for Spec Version 1.10\n"));
        } else {
          DEBUG ((DEBUG_INFO, "Found SD Card for Spec Version 1.0\n"));
        }
      } else {
        DEBUG ((DEBUG_ERROR, "Found invalid SD Card\n"));
      }
    }
  }

  if (CccSwitch) {
    /* SD Switch, Mode:0, Group:0, Value:0 */
    CmdArg = CreateSwitchCmdArgument (0, 0, 0);
    Status = MmcHost->SendCommand (MmcHost, MMC_CMD6, CmdArg);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a (MMC_CMD6): Error and Status = %r\n", __func__, Status));
      return Status;
    } else {
      Status = MmcHost->ReadBlockData (MmcHost, 0, SWITCH_CMD_DATA_LENGTH, Buffer);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a (MMC_CMD6): ReadBlockData Error and Status = %r\n", __func__, Status));
        return Status;
      }
    }

    if (!(Buffer[3] & SD_HIGH_SPEED_SUPPORTED)) {
      DEBUG ((DEBUG_INFO, "%a : High Speed not supported by Card\n", __func__));
    } else {
      Speed = SD_HIGH_SPEED;

      /* SD Switch, Mode:1, Group:0, Value:1 */
      CmdArg = CreateSwitchCmdArgument (1, 0, 1);
      Status = MmcHost->SendCommand (MmcHost, MMC_CMD6, CmdArg);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a (MMC_CMD6): Error and Status = %r\n", __func__, Status));
        return Status;
      } else {
        Status = MmcHost->ReadBlockData (MmcHost, 0, SWITCH_CMD_DATA_LENGTH, Buffer);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a (MMC_CMD6): ReadBlockData Error and Status = %r\n", __func__, Status));
          return Status;
        }

        if ((Buffer[4] & SWITCH_CMD_SUCCESS_MASK) != 0x01000000) {
          DEBUG ((DEBUG_ERROR, "Problem switching SD card into high-speed mode\n"));
          return Status;
        }
      }
    }
  }

  if (Scr.SD_BUS_WIDTHS & SD_BUS_WIDTH_4BIT) {
    CmdArg = MmcHostInstance->CardInfo.RCA << 16;
    Status = MmcHost->SendCommand (MmcHost, MMC_CMD55, CmdArg);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a (MMC_CMD55): Error and Status = %r\n", __func__, Status));
      return Status;
    }

    /* Width: 4 */
    Status = MmcHost->SendCommand (MmcHost, MMC_CMD6, 2);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a (MMC_CMD6): Error and Status = %r\n", __func__, Status));
      return Status;
    }
  }

  if (MMC_HOST_HAS_SETIOS (MmcHost)) {
    Status = MmcHost->SetIos (MmcHost, Speed, BUSWIDTH_4, EMMCBACKWARD);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a (SetIos): Error and Status = %r\n", __func__, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MmcIdentificationMode (
  IN MMC_HOST_INSTANCE  *MmcHostInstance
  )
{
  EFI_STATUS             Status;
  UINT32                 Response[4];
  UINTN                  Timeout;
  UINTN                  CmdArg;
  BOOLEAN                IsHCS;
  EFI_MMC_HOST_PROTOCOL  *MmcHost;
  OCR_RESPONSE           OcrResponse;

  MmcHost = MmcHostInstance->MmcHost;
  CmdArg  = 0;
  IsHCS   = FALSE;

  if (MmcHost == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // We can get into this function if we restart the identification mode
  if (MmcHostInstance->State == MmcHwInitializationState) {
    // Initialize the MMC Host HW
    Status = MmcNotifyState (MmcHostInstance, MmcHwInitializationState);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Error MmcHwInitializationState, Status=%r.\n", Status));
      return Status;
    }
  }

  Status = MmcHost->SendCommand (MmcHost, MMC_CMD0, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MmcIdentificationMode(MMC_CMD0): Error, Status=%r.\n", Status));
    return Status;
  }

  Status = MmcNotifyState (MmcHostInstance, MmcIdleState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Error MmcIdleState, Status=%r.\n", Status));
    return Status;
  }

  // Send CMD1 to get OCR (MMC)
  // This command only valid for MMC and eMMC
  Timeout = MAX_RETRY_COUNT;
  do {
    Status = MmcHost->SendCommand (MmcHost, MMC_CMD1, EMMC_CMD1_CAPACITY_GREATER_THAN_2GB);
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_OCR, (UINT32 *)&OcrResponse);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Failed to receive OCR, Status=%r.\n", Status));
      return Status;
    }

    Timeout--;
  } while (!OcrResponse.Ocr.PowerUp && (Timeout > 0));

  if (Status == EFI_SUCCESS) {
    if (!OcrResponse.Ocr.PowerUp) {
      DEBUG ((DEBUG_ERROR, "MmcIdentificationMode(MMC_CMD1): Card initialisation failure, Status=%r.\n", Status));
      return EFI_DEVICE_ERROR;
    }

    OcrResponse.Ocr.PowerUp = 0;
    if (OcrResponse.Raw == EMMC_CMD1_CAPACITY_GREATER_THAN_2GB) {
      MmcHostInstance->CardInfo.OCRData.AccessMode = BIT1;
    } else {
      MmcHostInstance->CardInfo.OCRData.AccessMode = 0x0;
    }

    // Check whether MMC or eMMC
    if ((OcrResponse.Raw == EMMC_CMD1_CAPACITY_GREATER_THAN_2GB) ||
        (OcrResponse.Raw == EMMC_CMD1_CAPACITY_LESS_THAN_2GB))
    {
      return EmmcIdentificationMode (MmcHostInstance, OcrResponse);
    }
  }

  // Are we using SDIO ?
  Status = MmcHost->SendCommand (MmcHost, MMC_CMD5, 0);
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "MmcIdentificationMode(MMC_CMD5): Error - SDIO not supported, Status=%r.\n", Status));
    return EFI_UNSUPPORTED;
  }

  // Check which kind of card we are using. Ver2.00 or later SD Memory Card (PL180 is SD v1.1)
  CmdArg = (0x0UL << 12 | BIT8 | 0xCEUL << 0);
  Status = MmcHost->SendCommand (MmcHost, MMC_CMD8, CmdArg);
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Card is SD2.0 => Supports high capacity\n"));
    IsHCS  = TRUE;
    Status = MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_R7, Response);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Failed to receive response to CMD8, Status=%r.\n", Status));
      return Status;
    }

    PrintResponseR1 (Response[0]);
    // Check if it is valid response
    if (Response[0] != CmdArg) {
      DEBUG ((DEBUG_ERROR, "The Card is not usable\n"));
      return EFI_UNSUPPORTED;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Not a SD2.0 Card\n"));
  }

  // We need to wait for the MMC or SD card is ready => (gCardInfo.OCRData.PowerUp == 1)
  Timeout = MAX_RETRY_COUNT;
  while (Timeout > 0) {
    // SD Card or MMC Card ? CMD55 indicates to the card that the next command is an application specific command
    Status = MmcHost->SendCommand (MmcHost, MMC_CMD55, 0);
    if (Status == EFI_SUCCESS) {
      DEBUG ((DEBUG_INFO, "Card should be SD\n"));
      if (IsHCS) {
        MmcHostInstance->CardInfo.CardType = SD_CARD_2;
      } else {
        MmcHostInstance->CardInfo.CardType = SD_CARD;
      }

      // Note: The first time CmdArg will be zero
      CmdArg = ((UINTN *)&(MmcHostInstance->CardInfo.OCRData))[0];
      if (IsHCS) {
        CmdArg |= BIT30;
      }

      Status = MmcHost->SendCommand (MmcHost, MMC_ACMD41, CmdArg);
      if (!EFI_ERROR (Status)) {
        Status = MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_OCR, Response);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Failed to receive OCR, Status=%r.\n", Status));
          return Status;
        }

        ((UINT32 *)&(MmcHostInstance->CardInfo.OCRData))[0] = Response[0];
      }
    } else {
      DEBUG ((DEBUG_INFO, "Card should be MMC\n"));
      MmcHostInstance->CardInfo.CardType = MMC_CARD;

      Status = MmcHost->SendCommand (MmcHost, MMC_CMD1, 0x800000);
      if (!EFI_ERROR (Status)) {
        Status = MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_OCR, Response);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Failed to receive OCR, Status=%r.\n", Status));
          return Status;
        }

        ((UINT32 *)&(MmcHostInstance->CardInfo.OCRData))[0] = Response[0];
      }
    }

    if (!EFI_ERROR (Status)) {
      if (!MmcHostInstance->CardInfo.OCRData.PowerUp) {
        gBS->Stall (1);
        Timeout--;
      } else {
        if ((MmcHostInstance->CardInfo.CardType == SD_CARD_2) && (MmcHostInstance->CardInfo.OCRData.AccessMode & BIT1)) {
          MmcHostInstance->CardInfo.CardType = SD_CARD_2_HIGH;
          DEBUG ((DEBUG_ERROR, "High capacity card.\n"));
        }

        break;  // The MMC/SD card is ready. Continue the Identification Mode
      }
    } else {
      gBS->Stall (1);
      Timeout--;
    }
  }

  if (Timeout == 0) {
    DEBUG ((DEBUG_ERROR, "MmcIdentificationMode(): No Card\n"));
    return EFI_NO_MEDIA;
  } else {
    PrintOCR (Response[0]);
  }

  Status = MmcNotifyState (MmcHostInstance, MmcReadyState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Error MmcReadyState\n"));
    return Status;
  }

  Status = MmcHost->SendCommand (MmcHost, MMC_CMD2, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MmcIdentificationMode(MMC_CMD2): Error\n"));
    return Status;
  }

  Status = MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_CID, Response);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Failed to receive CID, Status=%r.\n", Status));
    return Status;
  }

  PrintCID (Response);

  Status = MmcHost->NotifyState (MmcHost, MmcIdentificationState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Error MmcIdentificationState\n"));
    return Status;
  }

  //
  // Note, SD specifications say that "if the command execution causes a state change, it
  // will be visible to the host in the response to the next command"
  // The status returned for this CMD3 will be 2 - identification
  //
  CmdArg = 1;
  Status = MmcHost->SendCommand (MmcHost, MMC_CMD3, CmdArg);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MmcIdentificationMode(MMC_CMD3): Error\n"));
    return Status;
  }

  Status = MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_RCA, Response);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Failed to receive RCA, Status=%r.\n", Status));
    return Status;
  }

  PrintRCA (Response[0]);

  // For MMC card, RCA is assigned by CMD3 while CMD3 dumps the RCA for SD card
  if (MmcHostInstance->CardInfo.CardType != MMC_CARD) {
    MmcHostInstance->CardInfo.RCA = Response[0] >> 16;
  } else {
    MmcHostInstance->CardInfo.RCA = CmdArg;
  }

  Status = MmcNotifyState (MmcHostInstance, MmcStandByState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Error MmcStandByState\n"));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InitializeMmcDevice (
  IN  MMC_HOST_INSTANCE  *MmcHostInstance
  )
{
  EFI_STATUS             Status;
  EFI_MMC_HOST_PROTOCOL  *MmcHost;
  UINTN                  BlockCount;

  BlockCount = 1;
  MmcHost    = MmcHostInstance->MmcHost;

  Status = MmcIdentificationMode (MmcHostInstance);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "InitializeMmcDevice(): Error in Identification Mode, Status=%r\n", Status));
    return Status;
  }

  Status = MmcNotifyState (MmcHostInstance, MmcTransferState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "InitializeMmcDevice(): Error MmcTransferState, Status=%r\n", Status));
    return Status;
  }

  if (MmcHostInstance->CardInfo.CardType != EMMC_CARD) {
    Status = InitializeSdMmcDevice (MmcHostInstance);
  } else {
    Status = InitializeEmmcDevice (MmcHostInstance);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Set Block Length
  Status = MmcHost->SendCommand (MmcHost, MMC_CMD16, MmcHostInstance->BlockIo.Media->BlockSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "InitializeMmcDevice(MMC_CMD16): Error MmcHostInstance->BlockIo.Media->BlockSize: %d and Error = %r\n",
      MmcHostInstance->BlockIo.Media->BlockSize,
      Status
      ));
    return Status;
  }

  // Block Count (not used). Could return an error for SD card
  if (MmcHostInstance->CardInfo.CardType == MMC_CARD) {
    Status = MmcHost->SendCommand (MmcHost, MMC_CMD23, BlockCount);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "InitializeMmcDevice(MMC_CMD23): Error, Status=%r\n", Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

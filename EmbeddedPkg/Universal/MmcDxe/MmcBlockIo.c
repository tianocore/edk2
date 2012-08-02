/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
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

#include <Protocol/MmcHost.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>

#include "Mmc.h"

// Untested ...
//#define USE_STREAM

#define MAX_RETRY_COUNT  1000
#define CMD_RETRY_COUNT  20

EFI_STATUS
MmcNotifyState (
  IN MMC_HOST_INSTANCE *MmcHostInstance,
  IN MMC_STATE State
  )
{
  MmcHostInstance->State = State;
  return MmcHostInstance->MmcHost->NotifyState (MmcHostInstance->MmcHost, State);
}

VOID
PrintOCR (
  IN UINT32 Ocr
  )
{
  UINTN minv, maxv, volts;
  UINTN loop;

  minv  = 36;  // 3.6
  maxv  = 20;  // 2.0
  volts = 20;  // 2.0

  // The MMC register bits [23:8] indicate the working range of the card
  for (loop = 8; loop < 24; loop++) {
    if (Ocr & (1 << loop)) {
      if (minv > volts) minv = volts;
      if (maxv < volts) maxv = volts + 1;
    }
    volts = volts + 1;
  }

  DEBUG((EFI_D_ERROR, "- PrintOCR Ocr (0x%X)\n",Ocr));
  DEBUG((EFI_D_ERROR, "\t- Card operating voltage: %d.%d to %d.%d\n", minv/10, minv % 10, maxv/10, maxv % 10));
  if (((Ocr >> 29) & 3) == 0) {
    DEBUG((EFI_D_ERROR, "\t- AccessMode: Byte Mode\n"));
  } else {
    DEBUG((EFI_D_ERROR, "\t- AccessMode: Block Mode (0x%X)\n",((Ocr >> 29) & 3)));
  }

  if (Ocr & MMC_OCR_POWERUP) {
    DEBUG((EFI_D_ERROR, "\t- PowerUp\n"));
  } else {
    DEBUG((EFI_D_ERROR, "\t- Voltage Not Supported\n"));
  }
}

VOID PrintCID (
  IN UINT32* Cid
  )
{
  DEBUG((EFI_D_ERROR, "- PrintCID\n"));
  DEBUG((EFI_D_ERROR, "\t- Manufacturing date: %d/%d\n",(Cid[0] >> 8) & 0xF,(Cid[0] >> 12) & 0xFF));
  DEBUG((EFI_D_ERROR, "\t- Product serial number: 0x%X%X\n",Cid[1] & 0xFFFFFF,(Cid[0] >> 24) & 0xFF));
  DEBUG((EFI_D_ERROR, "\t- Product revision: %d\n",Cid[1] >> 24));
  //DEBUG((EFI_D_ERROR, "\t- Product name: %s\n",(char*)(Cid + 2)));
  DEBUG((EFI_D_ERROR, "\t- OEM ID: %c%c\n",(Cid[3] >> 8) & 0xFF,(Cid[3] >> 16) & 0xFF));
}

#if !defined(MDEPKG_NDEBUG)
CONST CHAR8* mStrUnit[] = { "100kbit/s","1Mbit/s","10Mbit/s","100MBit/s","Unkbown","Unkbown","Unkbown","Unkbown" };
CONST CHAR8* mStrValue[] = { "1.0","1.2","1.3","1.5","2.0","2.5","3.0","3.5","4.0","4.5","5.0","Unknown","Unknown","Unknown","Unknown" };
#endif

VOID
PrintCSD (
  IN UINT32* Csd
  )
{
  UINTN Value;

  if (((Csd[2] >> 30) & 0x3) == 0) {
    DEBUG((EFI_D_ERROR, "- PrintCSD Version 1.01-1.10/Version 2.00/Standard Capacity\n"));
  } else if (((Csd[2] >> 30) & 0x3) == 1) {
    DEBUG((EFI_D_ERROR, "- PrintCSD Version 2.00/High Capacity\n"));
  } else {
    DEBUG((EFI_D_ERROR, "- PrintCSD Version Higher than v3.3\n"));
  }

  DEBUG((EFI_D_ERROR, "\t- Supported card command class: 0x%X\n",MMC_CSD_GET_CCC(Csd)));
  DEBUG((EFI_D_ERROR, "\t- Speed: %a %a\n",mStrValue[(MMC_CSD_GET_TRANSPEED(Csd) >> 3) & 0xF],mStrUnit[MMC_CSD_GET_TRANSPEED(Csd) & 7]));
  DEBUG((EFI_D_ERROR, "\t- Maximum Read Data Block: %d\n",2 << (MMC_CSD_GET_READBLLEN(Csd)-1)));
  DEBUG((EFI_D_ERROR, "\t- Maximum Write Data Block: %d\n",2 << (MMC_CSD_GET_WRITEBLLEN(Csd)-1)));

  if (!MMC_CSD_GET_FILEFORMATGRP(Csd)) {
    Value = MMC_CSD_GET_FILEFORMAT(Csd);
    if (Value == 0)         DEBUG((EFI_D_ERROR, "\t- Format(0): Hard disk-like file system with partition table\n"));
    else if (Value == 1)    DEBUG((EFI_D_ERROR, "\t- Format(1): DOS FAT (floppy-like) with boot sector only (no partition table)\n"));
    else if (Value == 2)    DEBUG((EFI_D_ERROR, "\t- Format(2): Universal File Format\n"));
    else                    DEBUG((EFI_D_ERROR, "\t- Format(3): Others/Unknown\n"));
  } else {
    DEBUG((EFI_D_ERROR, "\t- Format: Reserved\n"));
  }
}

VOID
PrintRCA (
  IN UINT32 Rca
  )
{
  DEBUG((EFI_D_ERROR, "- PrintRCA: 0x%X\n",Rca));
  DEBUG((EFI_D_ERROR, "\t- Status: 0x%X\n",Rca & 0xFFFF));
  DEBUG((EFI_D_ERROR, "\t- RCA: 0x%X\n",(Rca >> 16) & 0xFFFF));
}

VOID
PrintResponseR1 (
  IN  UINT32 Response
  )
{
  DEBUG((EFI_D_INFO, "Response: 0x%X\n",Response));
  if (Response & (1 << 8))                  DEBUG((EFI_D_INFO, "\t- READY_FOR_DATA\n"));

  if (((Response >> 9) & 0xF) == 0)         DEBUG((EFI_D_INFO, "\t- State: Idle\n"));
  else if (((Response >> 9) & 0xF) == 1)    DEBUG((EFI_D_INFO, "\t- State: Ready\n"));
  else if (((Response >> 9) & 0xF) == 2)    DEBUG((EFI_D_INFO, "\t- State: Ident\n"));
  else if (((Response >> 9) & 0xF) == 3)    DEBUG((EFI_D_INFO, "\t- State: StandBy\n"));
  else if (((Response >> 9) & 0xF) == 4)    DEBUG((EFI_D_INFO, "\t- State: Tran\n"));
  else if (((Response >> 9) & 0xF) == 5)    DEBUG((EFI_D_INFO, "\t- State: Data\n"));
  else if (((Response >> 9) & 0xF) == 6)    DEBUG((EFI_D_INFO, "\t- State: Rcv\n"));
  else if (((Response >> 9) & 0xF) == 7)    DEBUG((EFI_D_INFO, "\t- State: Prg\n"));
  else if (((Response >> 9) & 0xF) == 8)    DEBUG((EFI_D_INFO, "\t- State: Dis\n"));
  else                                      DEBUG((EFI_D_INFO, "\t- State: Reserved\n"));
}

EFI_STATUS
EFIAPI
MmcGetCardStatus(
	IN MMC_HOST_INSTANCE     *MmcHostInstance
  )
{
  EFI_STATUS              Status;
  UINT32                  Response[4];
  UINTN                   CmdArg;
  EFI_MMC_HOST_PROTOCOL   *MmcHost;

  Status = EFI_SUCCESS;
  MmcHost = MmcHostInstance->MmcHost;
  CmdArg = 0;

  if (MmcHost == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if(MmcHostInstance->State != MmcHwInitializationState){
    //Get the Status of the card.
    CmdArg = MmcHostInstance->CardInfo.RCA << 16;
    Status = MmcHost->SendCommand (MmcHost, MMC_CMD13, CmdArg);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "MmcGetCardStatus(MMC_CMD13): Error and Status = %r\n", Status));
      return Status;
    }

    //Read Response
    MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_R1,Response);
    PrintResponseR1(Response[0]);
  }

  return Status;
}

EFI_STATUS
EFIAPI
MmcIdentificationMode (
  IN MMC_HOST_INSTANCE     *MmcHostInstance
  )
{
  EFI_STATUS              Status;
  UINT32                  Response[4];
  UINTN                   Timeout;
  UINTN                   CmdArg;
  BOOLEAN                 IsHCS;
  EFI_MMC_HOST_PROTOCOL   *MmcHost;

  MmcHost = MmcHostInstance->MmcHost;
  CmdArg = 0;
  IsHCS = FALSE;

  if (MmcHost == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // We can get into this function if we restart the identification mode
  if (MmcHostInstance->State == MmcHwInitializationState) {
    // Initialize the MMC Host HW
    Status = MmcNotifyState (MmcHostInstance, MmcHwInitializationState);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "MmcIdentificationMode() : Error MmcHwInitializationState\n"));
      return Status;
    }
  } else {
    //Note: Could even be used in all cases. But it looks this command could put the state machine into inactive for some cards
    Status = MmcHost->SendCommand (MmcHost, MMC_CMD0, 0);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD0): Error\n"));
      return Status;
    }
  }

  Status = MmcNotifyState (MmcHostInstance, MmcIdleState);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "MmcIdentificationMode() : Error MmcIdleState\n"));
    return Status;
  }

  // Are we using SDIO ?
  Status = MmcHost->SendCommand (MmcHost, MMC_CMD5, 0);
  if (Status == EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD5): Error - SDIO not supported.\n"));
    return EFI_UNSUPPORTED;
  }

  // Check which kind of card we are using. Ver2.00 or later SD Memory Card (PL180 is SD v1.1)
  CmdArg = (0x0UL << 12 | BIT8 | 0xCEUL << 0);
  Status = MmcHost->SendCommand (MmcHost, MMC_CMD8, CmdArg);
  if (Status == EFI_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Card is SD2.0 => Supports high capacity\n"));
    IsHCS = TRUE;
    MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_R7,Response);
    PrintResponseR1(Response[0]);
    //check if it is valid response
    if(Response[0] != CmdArg){
      DEBUG ((EFI_D_ERROR, "The Card is not usable\n"));
      return EFI_UNSUPPORTED;
    }
  } else {
    DEBUG ((EFI_D_ERROR, "Not a SD2.0 Card\n"));
  }

  // We need to wait for the MMC or SD card is ready => (gCardInfo.OCRData.PowerUp == 1)
  Timeout = MAX_RETRY_COUNT;
  while (Timeout > 0) {
    // SD Card or MMC Card ? CMD55 indicates to the card that the next command is an application specific command
    Status = MmcHost->SendCommand (MmcHost, MMC_CMD55, 0);
    if (Status == EFI_SUCCESS) {
        DEBUG ((EFI_D_INFO, "Card should be SD\n"));
        if (IsHCS) {
            MmcHostInstance->CardInfo.CardType = SD_CARD_2;
        } else {
            MmcHostInstance->CardInfo.CardType = SD_CARD;
        }

        // Note: The first time CmdArg will be zero
        CmdArg = ((UINTN *) &(MmcHostInstance->CardInfo.OCRData))[0];
        if (IsHCS) {
            CmdArg |= BIT30;
        }
        Status = MmcHost->SendCommand (MmcHost, MMC_ACMD41, CmdArg);
        if (!EFI_ERROR(Status)) {
          MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_OCR,Response);
          ((UINT32 *) &(MmcHostInstance->CardInfo.OCRData))[0] = Response[0];
        }
    } else {
        DEBUG ((EFI_D_INFO, "Card should be MMC\n"));
        MmcHostInstance->CardInfo.CardType = MMC_CARD;

        Status = MmcHost->SendCommand (MmcHost, MMC_CMD1, 0x800000);
        if (!EFI_ERROR(Status)) {
          MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_OCR,Response);
          ((UINT32 *) &(MmcHostInstance->CardInfo.OCRData))[0] = Response[0];
        }
    }

    if (!EFI_ERROR(Status)) {
      if (!MmcHostInstance->CardInfo.OCRData.PowerUp) {
          MicroSecondDelay(1);
          Timeout--;
      } else {
          if ((MmcHostInstance->CardInfo.CardType == SD_CARD_2) && (MmcHostInstance->CardInfo.OCRData.AccessMode & BIT1)) {
              MmcHostInstance->CardInfo.CardType = SD_CARD_2_HIGH;
              DEBUG ((EFI_D_ERROR, "High capacity card.\n"));
          }
          break;  // The MMC/SD card is ready. Continue the Identification Mode
      }
    } else {
      MicroSecondDelay(1);
      Timeout--;
    }
  }

  if (Timeout == 0) {
    DEBUG((EFI_D_ERROR, "MmcIdentificationMode(): No Card\n"));
    return EFI_NO_MEDIA;
  } else {
    PrintOCR(Response[0]);
  }

  Status = MmcNotifyState (MmcHostInstance, MmcReadyState);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "MmcIdentificationMode() : Error MmcReadyState\n"));
    return Status;
  }

  Status = MmcHost->SendCommand (MmcHost, MMC_CMD2, 0);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD2): Error\n"));
    return Status;
  }
  MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_CID,Response);
  PrintCID(Response);

  Status = MmcNotifyState (MmcHostInstance, MmcIdentificationState);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "MmcIdentificationMode() : Error MmcIdentificationState\n"));
    return Status;
  }

  //
  // Note, SD specifications say that "if the command execution causes a state change, it
  // will be visible to the host in the response to the next command"
  // The status returned for this CMD3 will be 2 - identification
  //
  CmdArg = 1;
  Status = MmcHost->SendCommand (MmcHost, MMC_CMD3, CmdArg);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD3): Error\n"));
    return Status;
  }

  MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_RCA,Response);
  PrintRCA(Response[0]);

  // For MMC card, RCA is assigned by CMD3 while CMD3 dumps the RCA for SD card
  if (MmcHostInstance->CardInfo.CardType != MMC_CARD) {
    MmcHostInstance->CardInfo.RCA = Response[0] >> 16;
  } else {
    MmcHostInstance->CardInfo.RCA = CmdArg;
  }

  Status = MmcNotifyState (MmcHostInstance, MmcStandByState);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "MmcIdentificationMode() : Error MmcStandByState\n"));
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS InitializeMmcDevice (
  IN  MMC_HOST_INSTANCE   *MmcHostInstance
  )
{
  UINT32                  Response[4];
  EFI_STATUS              Status;
  UINTN                   CardSize, NumBlocks, BlockSize, CmdArg;
  EFI_MMC_HOST_PROTOCOL   *MmcHost;
  UINTN                   BlockCount = 1;
  
  MmcHost = MmcHostInstance->MmcHost;

  MmcIdentificationMode (MmcHostInstance);

  //Send a command to get Card specific data
  CmdArg = MmcHostInstance->CardInfo.RCA << 16;
  Status = MmcHost->SendCommand (MmcHost, MMC_CMD9, CmdArg);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD9): Error, Status=%r\n", Status));
    return Status;
  }
  //Read Response
  MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_CSD,Response);
  PrintCSD(Response);

  if (MmcHostInstance->CardInfo.CardType == SD_CARD_2_HIGH) {
    CardSize = HC_MMC_CSD_GET_DEVICESIZE(Response);
    NumBlocks = ((CardSize + 1) * 1024);
    BlockSize = 1 << MMC_CSD_GET_READBLLEN(Response);
  } else {
    CardSize = MMC_CSD_GET_DEVICESIZE(Response);
    NumBlocks = (CardSize + 1) * (1 << (MMC_CSD_GET_DEVICESIZEMULT(Response) + 2));
    BlockSize = 1 << MMC_CSD_GET_READBLLEN(Response);
  }

  //For >=2G card, BlockSize may be 1K, but the transfer size is 512 bytes.
  if (BlockSize > 512) {
    NumBlocks = MultU64x32(NumBlocks, BlockSize/512);
    BlockSize = 512;
  }

  MmcHostInstance->BlockIo.Media->LastBlock    = (NumBlocks - 1);
  MmcHostInstance->BlockIo.Media->BlockSize    = BlockSize;
  MmcHostInstance->BlockIo.Media->ReadOnly     = MmcHost->IsReadOnly (MmcHost);
  MmcHostInstance->BlockIo.Media->MediaPresent = TRUE;
  MmcHostInstance->BlockIo.Media->MediaId++;

  CmdArg = MmcHostInstance->CardInfo.RCA << 16;
  Status = MmcHost->SendCommand (MmcHost, MMC_CMD7, CmdArg);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD7): Error and Status = %r\n", Status));
    return Status;
  }

  Status = MmcNotifyState (MmcHostInstance, MmcTransferState);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "MmcIdentificationMode() : Error MmcTransferState\n"));
    return Status;
  }

  // Set Block Length
  Status = MmcHost->SendCommand (MmcHost, MMC_CMD16, MmcHostInstance->BlockIo.Media->BlockSize);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD16): Error MmcHostInstance->BlockIo.Media->BlockSize: %d and Error = %r\n",MmcHostInstance->BlockIo.Media->BlockSize, Status));
    return Status;
  }

  // Block Count (not used). Could return an error for SD card
  if (MmcHostInstance->CardInfo.CardType == MMC_CARD) {
    MmcHost->SendCommand (MmcHost, MMC_CMD23, BlockCount);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MmcReset (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN BOOLEAN                  ExtendedVerification
  )
{
  MMC_HOST_INSTANCE       *MmcHostInstance;

  MmcHostInstance = MMC_HOST_INSTANCE_FROM_BLOCK_IO_THIS(This);

  if (MmcHostInstance->MmcHost == NULL) {
    // Nothing to do
    return EFI_SUCCESS;
  }

  // If a card is not present then clear all media settings
  if (!MmcHostInstance->MmcHost->IsCardPresent (MmcHostInstance->MmcHost)) {
    MmcHostInstance->BlockIo.Media->MediaPresent = FALSE;
    MmcHostInstance->BlockIo.Media->LastBlock    = 0;
    MmcHostInstance->BlockIo.Media->BlockSize    = 512;  // Should be zero but there is a bug in DiskIo
    MmcHostInstance->BlockIo.Media->ReadOnly     = FALSE;

    // Indicate that the driver requires initialization
    MmcHostInstance->State = MmcHwInitializationState;

    return EFI_SUCCESS;
  }

  // Implement me. Either send a CMD0 (could not work for some MMC host) or just turn off/turn
  //      on power and restart Identification mode
  return EFI_SUCCESS;
}

EFI_STATUS
MmcDetectCard (
  EFI_MMC_HOST_PROTOCOL     *MmcHost
  )
{
  if (!MmcHost->IsCardPresent (MmcHost)) {
    return EFI_NO_MEDIA;
  } else {
    return EFI_SUCCESS;
  }
}

#define MMCI0_BLOCKLEN 512
#define MMCI0_TIMEOUT  10000

EFI_STATUS
MmcIoBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN UINTN                    Transfer,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  OUT VOID                    *Buffer
  )
{
  UINT32                  Response[4];
  EFI_STATUS              Status;
  UINTN                   CmdArg;
  INTN                    Timeout;
  UINTN                   Cmd;
  MMC_HOST_INSTANCE       *MmcHostInstance;
  EFI_MMC_HOST_PROTOCOL   *MmcHost;
  UINTN                   BytesRemainingToBeTransfered;
  UINTN                   BlockCount = 1;

  MmcHostInstance = MMC_HOST_INSTANCE_FROM_BLOCK_IO_THIS(This);
  ASSERT(MmcHostInstance != 0);
  MmcHost = MmcHostInstance->MmcHost;
  ASSERT(MmcHost);

  if ((MmcHost == 0)|| (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Check if a Card is Present
  if (!MmcHostInstance->BlockIo.Media->MediaPresent) {
    return EFI_NO_MEDIA;
  }

  // All blocks must be within the device
  if ((Lba + (BufferSize / This->Media->BlockSize)) > (This->Media->LastBlock + 1)){
    return EFI_INVALID_PARAMETER;
  }

  // The buffer size must not be zero and it must be an exact multiple of the block size
  if ((BufferSize == 0) || ((BufferSize % This->Media->BlockSize) != 0)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (This->Media->MediaId != MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if((Transfer == MMC_IOBLOCKS_WRITE) && (This->Media->ReadOnly == TRUE)) {
    return EFI_WRITE_PROTECTED;
  }

  BytesRemainingToBeTransfered = BufferSize;
  while (BytesRemainingToBeTransfered > 0) {

    // Check if the Card is in Ready status
    CmdArg = MmcHostInstance->CardInfo.RCA << 16;
    Response[0] = 0;
    Timeout = 20;
    while(!(Response[0] & MMC_R0_READY_FOR_DATA) && (MMC_R0_CURRENTSTATE(Response) != MMC_R0_STATE_TRAN) && Timeout--) {
      Status = MmcHost->SendCommand (MmcHost, MMC_CMD13, CmdArg);
      if (!EFI_ERROR(Status)) {
        MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_R1,Response);
      }
    }

    if (0 == Timeout) {
      DEBUG((EFI_D_ERROR, "The Card is busy\n"));
      return EFI_NOT_READY;
    }

    //Set command argument based on the card access mode (Byte mode or Block mode)
    if (MmcHostInstance->CardInfo.OCRData.AccessMode & BIT1) {
      CmdArg = Lba;
    } else {
      CmdArg = Lba * This->Media->BlockSize;
    }

    if (Transfer == MMC_IOBLOCKS_READ) {
#ifndef USE_STREAM
      // Read a single block
      Cmd = MMC_CMD17;
#else
      //TODO: Should we support read stream (MMC_CMD11)
#endif
    } else {
#ifndef USE_STREAM
      // Write a single block
      Cmd = MMC_CMD24;
#else
      //TODO: Should we support write stream (MMC_CMD20)
#endif
    }
    Status = MmcHost->SendCommand (MmcHost, Cmd, CmdArg);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD%d): Error %r\n",Cmd, Status));
      return Status;
    }

    if (Transfer == MMC_IOBLOCKS_READ) {
#ifndef USE_STREAM
      // Read one block of Data
      Status = MmcHost->ReadBlockData (MmcHost, Lba,This->Media->BlockSize,Buffer);
      if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_BLKIO, "MmcIdentificationMode(): Error Read Block Data and Status = %r\n", Status));
        return Status;
      }
#else
      //TODO: Read a steam
      ASSERT(0);
#endif
      Status = MmcNotifyState (MmcHostInstance, MmcProgrammingState);
      if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcIdentificationMode() : Error MmcProgrammingState\n"));
        return Status;
      }
    } else {
#ifndef USE_STREAM
      // Write one block of Data
      Status = MmcHost->WriteBlockData (MmcHost, Lba,This->Media->BlockSize,Buffer);
      if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_BLKIO, "MmcIdentificationMode(): Error Write Block Data and Status = %r\n", Status));
        return Status;
      }
#else
      //TODO: Write a steam
      ASSERT(0);
#endif
    }

    // Command 12 - Stop transmission (ends read)
    Status = MmcHost->SendCommand (MmcHost, MMC_CMD12, 0);
    if (!EFI_ERROR(Status)) {
      MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_R1b,Response);
    }

    // Command 13 - Read status and wait for programming to complete (return to tran)
    Timeout = MMCI0_TIMEOUT;
    CmdArg = MmcHostInstance->CardInfo.RCA << 16;
    Response[0] = 0;
    while(!(Response[0] & MMC_R0_READY_FOR_DATA) && (MMC_R0_CURRENTSTATE(Response) != MMC_R0_STATE_TRAN) && Timeout--) {
      Status = MmcHost->SendCommand (MmcHost, MMC_CMD13, CmdArg);
      if (!EFI_ERROR(Status)) {
        MmcHost->ReceiveResponse (MmcHost, MMC_RESPONSE_TYPE_R1,Response);
      }
      NanoSecondDelay(100);
      Timeout--;
    }

    Status = MmcNotifyState (MmcHostInstance, MmcTransferState);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "MmcIdentificationMode() : Error MmcTransferState\n"));
      return Status;
    }

    BytesRemainingToBeTransfered -= This->Media->BlockSize;
    Lba    += BlockCount;
    Buffer = (UINT8 *)Buffer + This->Media->BlockSize;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MmcReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  OUT VOID                    *Buffer
  )
{
  return MmcIoBlocks (This, MMC_IOBLOCKS_READ, MediaId, Lba, BufferSize, Buffer);
}

EFI_STATUS
EFIAPI
MmcWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer
  )
{
  return MmcIoBlocks (This, MMC_IOBLOCKS_WRITE, MediaId, Lba, BufferSize, Buffer);
}

EFI_STATUS
EFIAPI
MmcFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}


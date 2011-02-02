/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
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

#define MAX_RETRY_COUNT  200

EFI_STATUS
MmcNotifyState (
  MMC_HOST_INSTANCE *MmcHostInstance,
  MMC_STATE State
  ) {
    MmcHostInstance->State = State;
    return MmcHostInstance->MmcHost->NotifyState(State);
}

VOID PrintOCR(UINT32 ocr) {
    float minv, maxv, volts;
    int loop;

    minv  = 3.6;
    maxv  = 2.0;
    volts = 2.0;

    // The MMC register bits [23:8] indicate the working range of the card
    for (loop = 8; loop < 24; loop++) {
        if (ocr & (1 << loop)) {
            if (minv > volts) minv = volts;
            if (maxv < volts) maxv = volts + 0.1;
        }
        volts = volts + 0.1;
    }

    DEBUG((EFI_D_ERROR, "- PrintOCR ocr (0x%X)\n",ocr));
    //DEBUG((EFI_D_ERROR, "\t- Card operating voltage: %fV to %fV\n", minv, maxv));
    if (((ocr >> 29) & 3) == 0)
        DEBUG((EFI_D_ERROR, "\t- AccessMode: Byte Mode\n"));
    else
        DEBUG((EFI_D_ERROR, "\t- AccessMode: Block Mode (0x%X)\n",((ocr >> 29) & 3)));

    if (ocr & MMC_OCR_POWERUP)
        DEBUG((EFI_D_ERROR, "\t- PowerUp\n"));
    else
        DEBUG((EFI_D_ERROR, "\t- Voltage Not Supported\n"));
}

VOID PrintCID(UINT32* cid) {
    DEBUG((EFI_D_ERROR, "- PrintCID\n"));
    DEBUG((EFI_D_ERROR, "\t- Manufacturing date: %d/%d\n",(cid[0] >> 8) & 0xF,(cid[0] >> 12) & 0xFF));
    DEBUG((EFI_D_ERROR, "\t- Product serial number: 0x%X%X\n",cid[1] & 0xFFFFFF,(cid[0] >> 24) & 0xFF));
    DEBUG((EFI_D_ERROR, "\t- Product revision: %d\n",cid[1] >> 24));
    //DEBUG((EFI_D_ERROR, "\t- Product name: %s\n",(char*)(cid + 2)));
    DEBUG((EFI_D_ERROR, "\t- OEM ID: %c%c\n",(cid[3] >> 8) & 0xFF,(cid[3] >> 16) & 0xFF));
}

VOID PrintCSD(UINT32* csd) {
    UINTN val32;
    CONST CHAR8* str_unit[] = { "100kbit/s","1Mbit/s","10Mbit/s","100MBit/s","Unkbown","Unkbown","Unkbown","Unkbown" };
    CONST CHAR8* str_value[] = { "1.0","1.2","1.3","1.5","2.0","2.5","3.0","3.5","4.0","4.5","5.0","Unknown","Unknown","Unknown","Unknown" };

    if (((csd[2] >> 30) & 0x3) == 0)
        DEBUG((EFI_D_ERROR, "- PrintCSD Version 1.01-1.10/Version 2.00/Standard Capacity\n"));
    else if (((csd[2] >> 30) & 0x3) == 1)
        DEBUG((EFI_D_ERROR, "- PrintCSD Version 2.00/High Capacity\n"));
    else
        DEBUG((EFI_D_ERROR, "- PrintCSD Version Higher than v3.3\n"));

    DEBUG((EFI_D_ERROR, "\t- Supported card command class: 0x%X\n",MMC_CSD_GET_CCC(csd)));
    DEBUG((EFI_D_ERROR, "\t- Speed: %a %a\n",str_value[(MMC_CSD_GET_TRANSPEED(csd) >> 3) & 0xF],str_unit[MMC_CSD_GET_TRANSPEED(csd) & 7]));
    DEBUG((EFI_D_ERROR, "\t- Maximum Read Data Block: %d\n",2 << (MMC_CSD_GET_READBLLEN(csd)-1)));
    DEBUG((EFI_D_ERROR, "\t- Maximum Write Data Block: %d\n",2 << (MMC_CSD_GET_WRITEBLLEN(csd)-1)));
    
    if (!MMC_CSD_GET_FILEFORMATGRP(csd)) {
        val32 = MMC_CSD_GET_FILEFORMAT(csd);
        if (val32 == 0)         DEBUG((EFI_D_ERROR, "\t- Format(0): Hard disk-like file system with partition table\n"));
        else if (val32 == 1)    DEBUG((EFI_D_ERROR, "\t- Format(1): DOS FAT (floppy-like) with boot sector only (no partition table)\n"));
        else if (val32 == 2)    DEBUG((EFI_D_ERROR, "\t- Format(2): Universal File Format\n"));
        else                    DEBUG((EFI_D_ERROR, "\t- Format(3): Others/Unknown\n"));
    } else {
        DEBUG((EFI_D_ERROR, "\t- Format: Reserved\n"));
    }
}

VOID PrintRCA(UINT32 rca) {
    DEBUG((EFI_D_ERROR, "- PrintRCA: 0x%X\n",rca));
    DEBUG((EFI_D_ERROR, "\t- Status: 0x%X\n",rca & 0xFFFF));
    DEBUG((EFI_D_ERROR, "\t- RCA: 0x%X\n",(rca >> 16) & 0xFFFF));
}

VOID PrintResponseR1(UINT32 response) {
    DEBUG((EFI_D_ERROR, "Response: 0x%X\n",response));
    if (response & (1 << 8))                 DEBUG((EFI_D_ERROR, "\t- READY_FOR_DATA\n"));

    if (((response >> 9) & 0xF) == 0)         DEBUG((EFI_D_ERROR, "\t- State: Idle\n"));
    else if (((response >> 9) & 0xF) == 1)    DEBUG((EFI_D_ERROR, "\t- State: Ready\n"));
    else if (((response >> 9) & 0xF) == 2)    DEBUG((EFI_D_ERROR, "\t- State: Ident\n"));
    else if (((response >> 9) & 0xF) == 3)    DEBUG((EFI_D_ERROR, "\t- State: StandBy\n"));
    else if (((response >> 9) & 0xF) == 4)    DEBUG((EFI_D_ERROR, "\t- State: Tran\n"));
    else if (((response >> 9) & 0xF) == 5)    DEBUG((EFI_D_ERROR, "\t- State: Data\n"));
    else if (((response >> 9) & 0xF) == 6)    DEBUG((EFI_D_ERROR, "\t- State: Rcv\n"));
    else if (((response >> 9) & 0xF) == 7)    DEBUG((EFI_D_ERROR, "\t- State: Prg\n"));
    else if (((response >> 9) & 0xF) == 8)    DEBUG((EFI_D_ERROR, "\t- State: Dis\n"));
    else                                     DEBUG((EFI_D_ERROR, "\t- State: Reserved\n"));
}

EFI_STATUS
EFIAPI
MmcIdentificationMode (
  MMC_HOST_INSTANCE     *MmcHostInstance
  ) {
    EFI_STATUS              Status;
    UINT32                  Response[4];
    UINTN                   timer;
    UINTN                   CmdArg;
    BOOLEAN                 bHCS;
    EFI_MMC_HOST_PROTOCOL   *MmcHost;
    
    MmcHost = MmcHostInstance->MmcHost;
    CmdArg = 0;
    bHCS = FALSE;

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
        Status = MmcHost->SendCommand(MMC_CMD0, 0);
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
    Status = MmcHost->SendCommand(MMC_CMD5, 0);
    if (Status == EFI_SUCCESS) {
        DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD5): Error - SDIO not supported.\n"));
        return EFI_UNSUPPORTED;
    }

    // Check which kind of card we are using. Ver2.00 or later SD Memory Card (PL180 is SD v1.1)
    CmdArg = (0x0UL << 12 | BIT8 | 0xCEUL << 0);
    Status = MmcHost->SendCommand(MMC_CMD8, CmdArg);
    if (Status == EFI_SUCCESS) {
        DEBUG ((EFI_D_ERROR, "Card is SD2.0 => Supports high capacity\n"));
        bHCS = TRUE;
        MmcHost->ReceiveResponse(MMC_RESPONSE_TYPE_R7,Response);
        PrintResponseR1(Response[0]);
    } else {
        DEBUG ((EFI_D_ERROR, "Not a SD2.0 Card\n"));
    }

    // We need to wait for the MMC or SD card is ready => (gCardInfo.OCRData.Busy == 1)
    timer = MAX_RETRY_COUNT;
    while (timer > 0) {
        // SD Card or MMC Card ? CMD55 indicates to the card that the next command is an application specific command
        Status = MmcHost->SendCommand(MMC_CMD55, 0);
        if (Status == EFI_SUCCESS) {
            DEBUG ((EFI_D_INFO, "Card should be SD\n"));
            if (bHCS) {
                MmcHostInstance->CardInfo.CardType = SD_CARD_2;
            } else {
                MmcHostInstance->CardInfo.CardType = SD_CARD;
            }

            // Note: The first time CmdArg will be zero
            CmdArg = ((UINTN *) &(MmcHostInstance->CardInfo.OCRData))[0];
            if (bHCS) {
                CmdArg |= BIT30;
            }
            Status = MmcHost->SendCommand(MMC_ACMD41, CmdArg);
            if (EFI_ERROR(Status)) {
                DEBUG((EFI_D_ERROR, "MmcIdentificationMode(ACMD41): Error\n"));
                return Status;
            }
            MmcHost->ReceiveResponse(MMC_RESPONSE_TYPE_OCR,Response);
            ((UINT32 *) &(MmcHostInstance->CardInfo.OCRData))[0] = Response[0];
        } else {
            DEBUG ((EFI_D_INFO, "Card should be MMC\n"));
            MmcHostInstance->CardInfo.CardType = MMC_CARD;

            Status = MmcHost->SendCommand(MMC_CMD1, 0x800000);
            if (EFI_ERROR(Status)) {
                DEBUG((EFI_D_ERROR, "MmcIdentificationMode(ACMD41): Error\n"));
                return Status;
            }
            MmcHost->ReceiveResponse(MMC_RESPONSE_TYPE_OCR,Response);
            ((UINT32 *) &(MmcHostInstance->CardInfo.OCRData))[0] = Response[0];
        }

        if (MmcHostInstance->CardInfo.OCRData.Busy == 0) {
            MicroSecondDelay(10*1000);
            timer--;
        } else {
            if ((MmcHostInstance->CardInfo.CardType == SD_CARD_2) && (MmcHostInstance->CardInfo.OCRData.AccessMode & BIT1)) {
                MmcHostInstance->CardInfo.CardType = SD_CARD_2_HIGH;
                DEBUG ((EFI_D_ERROR, "High capacity card.\n"));
            }
            break;  // The MMC/SD card is ready. Continue the Identification Mode
        }
    }

    if (timer == 0) {
        DEBUG((EFI_D_ERROR, "MmcIdentificationMode(): No Card\n"));
        ASSERT(0);
        return EFI_NO_MEDIA;
    } else {
        PrintOCR(Response[0]);
    }

    Status = MmcNotifyState (MmcHostInstance, MmcReadyState);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcIdentificationMode() : Error MmcReadyState\n"));
        return Status;
    }

    Status = MmcHost->SendCommand(MMC_CMD2, 0);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD2): Error\n"));
        ASSERT(0);
        return Status;
    }
    MmcHost->ReceiveResponse(MMC_RESPONSE_TYPE_CID,Response);
    PrintCID(Response);

    Status = MmcNotifyState (MmcHostInstance, MmcIdentificationState);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcIdentificationMode() : Error MmcIdentificationState\n"));
        return Status;
    }

    CmdArg = 0;
    Status = MmcHost->SendCommand(MMC_CMD3, CmdArg);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD3): Error\n"));
        return Status;
    }
    MmcHost->ReceiveResponse(MMC_RESPONSE_TYPE_RCA,Response);
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

EFI_STATUS
EFIAPI
MmcReset (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN BOOLEAN                  ExtendedVerification
  ) {
    // Implement me. Either send a CMD0 (could not work for some MMC host) or just turn off/turn
    //      on power and restart Identification mode
    return EFI_SUCCESS;
}

EFI_STATUS
MmcDetectCard (
  EFI_MMC_HOST_PROTOCOL     *MmcHost
  )
{
    if (!MmcHost->IsCardPresent()) {
        return EFI_NO_MEDIA;
    } else {
        return EFI_SUCCESS;
    }
}

#define MMCI0_BLOCKLEN 512
#define MMCI0_TIMEOUT  10000

EFI_STATUS MmcIoBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN UINTN                    Transfer,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  OUT VOID                    *Buffer
  ) {
    UINT32                  Response[4];
    EFI_STATUS              Status;
    UINTN                   CardSize, NumBlocks, BlockSize, CmdArg;
    UINTN                   timer;
    UINTN                   Cmd;
    MMC_HOST_INSTANCE       *MmcHostInstance;
    EFI_MMC_HOST_PROTOCOL   *MmcHost;
    UINTN                   BytesRemainingToBeTransfered;
    UINTN                   BlockCount = 1;

    MmcHostInstance = MMC_HOST_INSTANCE_FROM_BLOCK_IO_THIS(This);
    ASSERT(MmcHostInstance != 0);
    MmcHost = MmcHostInstance->MmcHost;
    ASSERT(MmcHost);

    if (MmcHost == 0) {
        return EFI_INVALID_PARAMETER;
    }

    // Check if a Card is Present
    if (!MmcHost->IsCardPresent()) {
        MmcHostInstance->BlockIo.Media->MediaPresent = FALSE;
        MmcHostInstance->BlockIo.Media->LastBlock    = 0;
        MmcHostInstance->BlockIo.Media->BlockSize    = 512;  // Should be zero but there is a bug in DiskIo
        MmcHostInstance->BlockIo.Media->ReadOnly     = FALSE; 
        return EFI_NO_MEDIA;
    }

    // If the driver has not been initialized yet then go into Iddentification Mode
    if (MmcHostInstance->State == MmcHwInitializationState) {
        MmcIdentificationMode (MmcHostInstance);

        CmdArg = MmcHostInstance->CardInfo.RCA << 16;
        Status = MmcHost->SendCommand(MMC_CMD9, CmdArg);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD9): Error\n"));
            ASSERT(0);
            return Status;
        }
        MmcHost->ReceiveResponse(MMC_RESPONSE_TYPE_CSD,Response);
        PrintCSD(Response);


        if (MmcHostInstance->CardInfo.CardType == SD_CARD_2_HIGH) {
            ASSERT(0);  //TODO: Implementation needed
            CardSize = MMC_CSD_GET_DEVICESIZE(Response);
            NumBlocks = ((CardSize + 1) * 1024);;
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
        MmcHostInstance->BlockIo.Media->ReadOnly     = MmcHost->IsReadOnly();
        MmcHostInstance->BlockIo.Media->MediaPresent = TRUE; 
        MmcHostInstance->BlockIo.Media->MediaId++; 

        CmdArg = MmcHostInstance->CardInfo.RCA << 16;
        Status = MmcHost->SendCommand(MMC_CMD7, CmdArg);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD7): Error\n"));
            ASSERT(0);
            return Status;
        }
        
        Status = MmcNotifyState (MmcHostInstance, MmcTransferState);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "MmcIdentificationMode() : Error MmcTransferState\n"));
            return Status;
        }
    } else {
        // Maybe test if the card has changed to update gMmcMedia information
        if (MmcHostInstance->State == MmcTransferState) {
            //DEBUG((EFI_D_ERROR, "MmcIdentificationMode() : MmcTransferState\n"));
        } else if (MmcHostInstance->State == MmcStandByState) {
            DEBUG((EFI_D_ERROR, "MmcIdentificationMode() : MmcStandByState\n"));
        } else {
            ASSERT(0);
        }
    }

    if (Lba > This->Media->LastBlock) {
        ASSERT(0);
        return EFI_INVALID_PARAMETER;
    }
  
    if ((BufferSize % This->Media->BlockSize) != 0) {
        ASSERT(0);
        return EFI_BAD_BUFFER_SIZE;
    }

    BytesRemainingToBeTransfered = BufferSize;
    while (BytesRemainingToBeTransfered > 0) {
        // Set Block Length
        Status = MmcHost->SendCommand(MMC_CMD16, This->Media->BlockSize);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD16): Error This->Media->BlockSize:%d\n",This->Media->BlockSize));
            ASSERT(0);
            return Status;
        }

        // Block Count (not used). Could return an error for SD card
        MmcHost->SendCommand(MMC_CMD23, BlockCount);

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
        Status = MmcHost->SendCommand(Cmd, CmdArg);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "MmcIdentificationMode(MMC_CMD%d): Error\n",Cmd));
            ASSERT(0);
            return Status;
        }

        if (Transfer == MMC_IOBLOCKS_READ) {
#ifndef USE_STREAM
            // Read one block of Data
            Status = MmcHost->ReadBlockData(Lba,This->Media->BlockSize,Buffer);
            if (EFI_ERROR(Status)) {
                DEBUG((EFI_D_BLKIO, "MmcIdentificationMode(): Error Read Block Data"));
                ASSERT(0);
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
            Status = MmcHost->WriteBlockData(Lba,This->Media->BlockSize,Buffer);
            if (EFI_ERROR(Status)) {
                DEBUG((EFI_D_BLKIO, "MmcIdentificationMode(): Error Write Block Data"));
                ASSERT(0);
                return Status;
            }
#else
            //TODO: Write a steam
            ASSERT(0);
#endif
        }

        // Command 12 - Stop transmission (ends read)
        Status = MmcHost->SendCommand(MMC_CMD12, 0);
        MmcHost->ReceiveResponse(MMC_RESPONSE_TYPE_R1b,Response);

        // Command 13 - Read status and wait for programming to complete (return to tran)
        timer = MMCI0_TIMEOUT;
        while ((MMC_R0_CURRENTSTATE(Response) != MMC_R0_STATE_TRAN) && timer) {
            MmcHost->SendCommand(MMC_CMD13, 0);
            MmcHost->ReceiveResponse(MMC_RESPONSE_TYPE_R1,Response);
            NanoSecondDelay(100);
            timer--;
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
  ) {
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
  ) {
    return MmcIoBlocks (This, MMC_IOBLOCKS_WRITE, MediaId, Lba, BufferSize, Buffer);
}

EFI_STATUS
EFIAPI
MmcFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  ) {
    return EFI_SUCCESS;
}


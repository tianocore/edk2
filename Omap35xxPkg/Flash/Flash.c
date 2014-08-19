/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Flash.h"

NAND_PART_INFO_TABLE gNandPartInfoTable[1] = {
  { 0x2C, 0xBA, 17, 11 }
};

NAND_FLASH_INFO *gNandFlashInfo = NULL;
UINT8           *gEccCode;
UINTN           gNum512BytesChunks = 0;

//

// Device path for SemiHosting. It contains our autogened Caller ID GUID.

//

typedef struct {

  VENDOR_DEVICE_PATH        Guid;

  EFI_DEVICE_PATH_PROTOCOL  End;

} FLASH_DEVICE_PATH;



FLASH_DEVICE_PATH gDevicePath = {
  {
    { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, { sizeof (VENDOR_DEVICE_PATH), 0 } },
    EFI_CALLER_ID_GUID
  },
  { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0} }
};



//Actual page address = Column address + Page address + Block address.
UINTN
GetActualPageAddressInBytes (
  UINTN BlockIndex,
  UINTN PageIndex
)
{
  //BlockAddressStart = Start of the Block address in actual NAND
  //PageAddressStart = Start of the Page address in actual NAND
  return ((BlockIndex << gNandFlashInfo->BlockAddressStart) + (PageIndex << gNandFlashInfo->PageAddressStart));
}

VOID
NandSendCommand (
  UINT8 Command
)
{
  MmioWrite16(GPMC_NAND_COMMAND_0, Command);
}

VOID
NandSendAddress (
  UINT8 Address
)
{
  MmioWrite16(GPMC_NAND_ADDRESS_0, Address);
}

UINT16
NandReadStatus (
  VOID
  )
{
  //Send READ STATUS command
  NandSendCommand(READ_STATUS_CMD);

  //Read status.
  return MmioRead16(GPMC_NAND_DATA_0);
}

VOID
NandSendAddressCycles (
  UINTN Address
)
{
  //Column address
  NandSendAddress(Address & 0xff);
  Address >>= 8;

  //Column address
  NandSendAddress(Address & 0x07);
  Address >>= 3;

  //Page and Block address
  NandSendAddress(Address & 0xff);
  Address >>= 8;

  //Block address
  NandSendAddress(Address & 0xff);
  Address >>= 8;

  //Block address
  NandSendAddress(Address & 0x01);
}

VOID
GpmcInit (
  VOID
  )
{
  //Enable Smart-idle mode.
  MmioWrite32 (GPMC_SYSCONFIG, SMARTIDLEMODE);

  //Set IRQSTATUS and IRQENABLE to the reset value
  MmioWrite32 (GPMC_IRQSTATUS, 0x0);
  MmioWrite32 (GPMC_IRQENABLE, 0x0);

  //Disable GPMC timeout control.
  MmioWrite32 (GPMC_TIMEOUT_CONTROL, TIMEOUTDISABLE);

  //Set WRITEPROTECT bit to enable write access.
  MmioWrite32 (GPMC_CONFIG, WRITEPROTECT_HIGH);

  //NOTE: Following GPMC_CONFIGi_0 register settings are taken from u-boot memory dump.
  MmioWrite32 (GPMC_CONFIG1_0, DEVICETYPE_NAND | DEVICESIZE_X16);
  MmioWrite32 (GPMC_CONFIG2_0, CSRDOFFTIME | CSWROFFTIME);
  MmioWrite32 (GPMC_CONFIG3_0, ADVRDOFFTIME | ADVWROFFTIME);
  MmioWrite32 (GPMC_CONFIG4_0, OEONTIME | OEOFFTIME | WEONTIME | WEOFFTIME);
  MmioWrite32 (GPMC_CONFIG5_0, RDCYCLETIME | WRCYCLETIME | RDACCESSTIME | PAGEBURSTACCESSTIME);
  MmioWrite32 (GPMC_CONFIG6_0, WRACCESSTIME | WRDATAONADMUXBUS | CYCLE2CYCLEDELAY | CYCLE2CYCLESAMECSEN);
  MmioWrite32 (GPMC_CONFIG7_0, MASKADDRESS_128MB | CSVALID | BASEADDRESS);
}

EFI_STATUS
NandDetectPart (
  VOID
)
{
  UINT8      NandInfo = 0;
  UINT8      PartInfo[5];
  UINTN      Index;
  BOOLEAN    Found = FALSE;

  //Send READ ID command
  NandSendCommand(READ_ID_CMD);

  //Send one address cycle.
  NandSendAddress(0);

  //Read 5-bytes to idenfity code programmed into the NAND flash devices.
  //BYTE 0 = Manufacture ID
  //Byte 1 = Device ID
  //Byte 2, 3, 4 = Nand part specific information (Page size, Block size etc)
  for (Index = 0; Index < sizeof(PartInfo); Index++) {
    PartInfo[Index] = MmioRead16(GPMC_NAND_DATA_0);
  }

  //Check if the ManufactureId and DeviceId are part of the currently supported nand parts.
  for (Index = 0; Index < sizeof(gNandPartInfoTable)/sizeof(NAND_PART_INFO_TABLE); Index++) {
    if (gNandPartInfoTable[Index].ManufactureId == PartInfo[0] && gNandPartInfoTable[Index].DeviceId == PartInfo[1]) {
      gNandFlashInfo->BlockAddressStart = gNandPartInfoTable[Index].BlockAddressStart;
      gNandFlashInfo->PageAddressStart = gNandPartInfoTable[Index].PageAddressStart;
      Found = TRUE;
      break;
    }
  }

  if (Found == FALSE) {
    DEBUG ((EFI_D_ERROR, "Nand part is not currently supported. Manufacture id: %x, Device id: %x\n", PartInfo[0], PartInfo[1]));
    return EFI_NOT_FOUND;
  }

  //Populate NAND_FLASH_INFO based on the result of READ ID command.
  gNandFlashInfo->ManufactureId = PartInfo[0];
  gNandFlashInfo->DeviceId = PartInfo[1];
  NandInfo = PartInfo[3];

  if (PAGE_SIZE(NandInfo) == PAGE_SIZE_2K_VAL) {
    gNandFlashInfo->PageSize = PAGE_SIZE_2K;
  } else {
    DEBUG ((EFI_D_ERROR, "Unknown Page size.\n"));
    return EFI_DEVICE_ERROR;
  }

  if (SPARE_AREA_SIZE(NandInfo) == SPARE_AREA_SIZE_64B_VAL) {
    gNandFlashInfo->SparePageSize = SPARE_AREA_SIZE_64B;
  } else {
    DEBUG ((EFI_D_ERROR, "Unknown Spare area size.\n"));
    return EFI_DEVICE_ERROR;
  }

  if (BLOCK_SIZE(NandInfo) == BLOCK_SIZE_128K_VAL) {
    gNandFlashInfo->BlockSize = BLOCK_SIZE_128K;
  } else {
    DEBUG ((EFI_D_ERROR, "Unknown Block size.\n"));
    return EFI_DEVICE_ERROR;
  }

  if (ORGANIZATION(NandInfo) == ORGANIZATION_X8) {
    gNandFlashInfo->Organization = 0;
  } else if (ORGANIZATION(NandInfo) == ORGANIZATION_X16) {
    gNandFlashInfo->Organization = 1;
  }

  //Calculate total number of blocks.
  gNandFlashInfo->NumPagesPerBlock = DivU64x32(gNandFlashInfo->BlockSize, gNandFlashInfo->PageSize);

  return EFI_SUCCESS;
}

VOID
NandConfigureEcc (
  VOID
  )
{
  //Define ECC size 0 and size 1 to 512 bytes
  MmioWrite32 (GPMC_ECC_SIZE_CONFIG, (ECCSIZE0_512BYTES | ECCSIZE1_512BYTES));
}

VOID
NandEnableEcc (
  VOID
  )
{
  //Clear all the ECC result registers and select ECC result register 1
  MmioWrite32 (GPMC_ECC_CONTROL, (ECCCLEAR | ECCPOINTER_REG1));

  //Enable ECC engine on CS0
  MmioWrite32 (GPMC_ECC_CONFIG, (ECCENABLE | ECCCS_0 | ECC16B));
}

VOID
NandDisableEcc (
  VOID
  )
{
  //Turn off ECC engine.
  MmioWrite32 (GPMC_ECC_CONFIG, ECCDISABLE);
}

VOID
NandCalculateEcc (
  VOID
  )
{
  UINTN Index;
  UINTN EccResultRegister;
  UINTN EccResult;

  //Capture 32-bit ECC result for each 512-bytes chunk.
  //In our case PageSize is 2K so read ECC1-ECC4 result registers and
  //generate total of 12-bytes of ECC code for the particular page.

  EccResultRegister = GPMC_ECC1_RESULT;

  for (Index = 0; Index < gNum512BytesChunks; Index++) {

    EccResult = MmioRead32 (EccResultRegister);

    //Calculate ECC code from 32-bit ECC result value.
    //NOTE: Following calculation is not part of TRM. We got this information
    //from Beagleboard mailing list.
    gEccCode[Index * 3] = EccResult & 0xFF;
    gEccCode[(Index * 3) + 1] = (EccResult >> 16) & 0xFF;
    gEccCode[(Index * 3) + 2] = (((EccResult >> 20) & 0xF0) | ((EccResult >> 8) & 0x0F));

    //Point to next ECC result register.
    EccResultRegister += 4;
  }
}

EFI_STATUS
NandReadPage (
  IN  UINTN                         BlockIndex,
  IN  UINTN                         PageIndex,
  OUT VOID                          *Buffer,
  OUT UINT8                         *SpareBuffer
)
{
  UINTN      Address;
  UINTN      Index;
  UINTN      NumMainAreaWords = (gNandFlashInfo->PageSize/2);
  UINTN      NumSpareAreaWords = (gNandFlashInfo->SparePageSize/2);
  UINT16     *MainAreaWordBuffer = Buffer;
  UINT16     *SpareAreaWordBuffer = (UINT16 *)SpareBuffer;
  UINTN      Timeout = MAX_RETRY_COUNT;

  //Generate device address in bytes to access specific block and page index
  Address = GetActualPageAddressInBytes(BlockIndex, PageIndex);

  //Send READ command
  NandSendCommand(PAGE_READ_CMD);

  //Send 5 Address cycles to access specific device address
  NandSendAddressCycles(Address);

  //Send READ CONFIRM command
  NandSendCommand(PAGE_READ_CONFIRM_CMD);

  //Poll till device is busy.
  while (Timeout) {
    if ((NandReadStatus() & NAND_READY) == NAND_READY) {
      break;
    }
    Timeout--;
  }

  if (Timeout == 0) {
    DEBUG ((EFI_D_ERROR, "Read page timed out.\n"));
    return EFI_TIMEOUT;
  }

  //Reissue READ command
  NandSendCommand(PAGE_READ_CMD);

  //Enable ECC engine.
  NandEnableEcc();

  //Read data into the buffer.
  for (Index = 0; Index < NumMainAreaWords; Index++) {
    *MainAreaWordBuffer++ = MmioRead16(GPMC_NAND_DATA_0);
  }

  //Read spare area into the buffer.
  for (Index = 0; Index < NumSpareAreaWords; Index++) {
    *SpareAreaWordBuffer++ = MmioRead16(GPMC_NAND_DATA_0);
  }

  //Calculate ECC.
  NandCalculateEcc();

  //Turn off ECC engine.
  NandDisableEcc();

  //Perform ECC correction.
  //Need to implement..

  return EFI_SUCCESS;
}

EFI_STATUS
NandWritePage (
  IN  UINTN                         BlockIndex,
  IN  UINTN                         PageIndex,
  OUT VOID                          *Buffer,
  IN  UINT8                         *SpareBuffer
)
{
  UINTN      Address;
  UINT16     *MainAreaWordBuffer = Buffer;
  UINT16     *SpareAreaWordBuffer = (UINT16 *)SpareBuffer;
  UINTN      Index;
  UINTN      NandStatus;
  UINTN      Timeout = MAX_RETRY_COUNT;

  //Generate device address in bytes to access specific block and page index
  Address = GetActualPageAddressInBytes(BlockIndex, PageIndex);

  //Send SERIAL DATA INPUT command
  NandSendCommand(PROGRAM_PAGE_CMD);

  //Send 5 Address cycles to access specific device address
  NandSendAddressCycles(Address);

  //Enable ECC engine.
  NandEnableEcc();

  //Data input from Buffer
  for (Index = 0; Index < (gNandFlashInfo->PageSize/2); Index++) {
    MmioWrite16(GPMC_NAND_DATA_0, *MainAreaWordBuffer++);

    //After each write access, device has to wait to accept data.
    //Currently we may not be programming proper timing parameters to
    //the GPMC_CONFIGi_0 registers and we would need to figure that out.
    //Without following delay, page programming fails.
    gBS->Stall(1);
  }

  //Calculate ECC.
  NandCalculateEcc();

  //Turn off ECC engine.
  NandDisableEcc();

  //Prepare Spare area buffer with ECC codes.
  SetMem(SpareBuffer, gNandFlashInfo->SparePageSize, 0xFF);
  CopyMem(&SpareBuffer[ECC_POSITION], gEccCode, gNum512BytesChunks * 3);

  //Program spare area with calculated ECC.
  for (Index = 0; Index < (gNandFlashInfo->SparePageSize/2); Index++) {
    MmioWrite16(GPMC_NAND_DATA_0, *SpareAreaWordBuffer++);
  }

  //Send PROGRAM command
  NandSendCommand(PROGRAM_PAGE_CONFIRM_CMD);

  //Poll till device is busy.
  NandStatus = 0;
  while (Timeout) {
    NandStatus = NandReadStatus();
    if ((NandStatus & NAND_READY) == NAND_READY) {
      break;
    }
    Timeout--;
  }

  if (Timeout == 0) {
    DEBUG ((EFI_D_ERROR, "Program page timed out.\n"));
    return EFI_TIMEOUT;
  }

  //Bit0 indicates Pass/Fail status
  if (NandStatus & NAND_FAILURE) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
NandEraseBlock (
  IN UINTN BlockIndex
)
{
  UINTN      Address;
  UINTN      NandStatus;
  UINTN      Timeout = MAX_RETRY_COUNT;

  //Generate device address in bytes to access specific block and page index
  Address = GetActualPageAddressInBytes(BlockIndex, 0);

  //Send ERASE SETUP command
  NandSendCommand(BLOCK_ERASE_CMD);

  //Send 3 address cycles to device to access Page address and Block address
  Address >>= 11; //Ignore column addresses

  NandSendAddress(Address & 0xff);
  Address >>= 8;

  NandSendAddress(Address & 0xff);
  Address >>= 8;

  NandSendAddress(Address & 0xff);

  //Send ERASE CONFIRM command
  NandSendCommand(BLOCK_ERASE_CONFIRM_CMD);

  //Poll till device is busy.
  NandStatus = 0;
  while (Timeout) {
    NandStatus = NandReadStatus();
    if ((NandStatus & NAND_READY) == NAND_READY) {
      break;
    }
    Timeout--;
    gBS->Stall(1);
  }

  if (Timeout == 0) {
    DEBUG ((EFI_D_ERROR, "Erase block timed out for Block: %d.\n", BlockIndex));
    return EFI_TIMEOUT;
  }

  //Bit0 indicates Pass/Fail status
  if (NandStatus & NAND_FAILURE) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
NandReadBlock (
  IN UINTN                          StartBlockIndex,
  IN UINTN                          EndBlockIndex,
  OUT VOID                          *Buffer,
  OUT VOID                          *SpareBuffer
)
{
  UINTN      BlockIndex;
  UINTN      PageIndex;
  EFI_STATUS Status = EFI_SUCCESS;

  for (BlockIndex = StartBlockIndex; BlockIndex <= EndBlockIndex; BlockIndex++) {
    //For each block read number of pages
    for (PageIndex = 0; PageIndex < gNandFlashInfo->NumPagesPerBlock; PageIndex++) {
      Status = NandReadPage(BlockIndex, PageIndex, Buffer, SpareBuffer);
      if (EFI_ERROR(Status)) {
        return Status;
      }
      Buffer = ((UINT8 *)Buffer + gNandFlashInfo->PageSize);
    }
  }

  return Status;
}

EFI_STATUS
NandWriteBlock (
  IN UINTN                          StartBlockIndex,
  IN UINTN                          EndBlockIndex,
  OUT VOID                          *Buffer,
  OUT VOID                          *SpareBuffer
  )
{
  UINTN      BlockIndex;
  UINTN      PageIndex;
  EFI_STATUS Status = EFI_SUCCESS;

  for (BlockIndex = StartBlockIndex; BlockIndex <= EndBlockIndex; BlockIndex++) {
    //Page programming.
    for (PageIndex = 0; PageIndex < gNandFlashInfo->NumPagesPerBlock; PageIndex++) {
      Status = NandWritePage(BlockIndex, PageIndex, Buffer, SpareBuffer);
      if (EFI_ERROR(Status)) {
        return Status;
      }
      Buffer = ((UINT8 *)Buffer + gNandFlashInfo->PageSize);
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
NandFlashReset (
  IN EFI_BLOCK_IO_PROTOCOL          *This,
  IN BOOLEAN                        ExtendedVerification
  )
{
  UINTN BusyStall = 50;                            // microSeconds
  UINTN ResetBusyTimeout = (1000000 / BusyStall);  // 1 Second

  //Send RESET command to device.
  NandSendCommand(RESET_CMD);

  //Wait for 1ms before we check status register.
  gBS->Stall(1000);

  //Check BIT#5 & BIT#6 in Status register to make sure RESET is done.
  while ((NandReadStatus() & NAND_RESET_STATUS) != NAND_RESET_STATUS) {

    //In case of extended verification, wait for extended amount of time
    //to make sure device is reset.
    if (ExtendedVerification) {
      if (ResetBusyTimeout == 0) {
        return EFI_DEVICE_ERROR;
      }

      gBS->Stall(BusyStall);
      ResetBusyTimeout--;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
NandFlashReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL          *This,
  IN UINT32                         MediaId,
  IN EFI_LBA                        Lba,
  IN UINTN                          BufferSize,
  OUT VOID                          *Buffer
  )
{
  UINTN      NumBlocks;
  UINTN      EndBlockIndex;
  EFI_STATUS Status;
  UINT8      *SpareBuffer = NULL;

  if (Buffer == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (Lba > LAST_BLOCK) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if ((BufferSize % gNandFlashInfo->BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto exit;
  }

  NumBlocks = DivU64x32(BufferSize, gNandFlashInfo->BlockSize);
  EndBlockIndex = ((UINTN)Lba + NumBlocks) - 1;

  SpareBuffer = (UINT8 *)AllocatePool(gNandFlashInfo->SparePageSize);
  if (SpareBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit;
  }

  //Read block
  Status = NandReadBlock((UINTN)Lba, EndBlockIndex, Buffer, SpareBuffer);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Read block fails: %x\n", Status));
    goto exit;
  }

exit:
  if (SpareBuffer != NULL) {
    FreePool (SpareBuffer);
  }

  return Status;
}

EFI_STATUS
EFIAPI
NandFlashWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL          *This,
  IN UINT32                         MediaId,
  IN EFI_LBA                        Lba,
  IN UINTN                          BufferSize,
  IN VOID                           *Buffer
  )
{
  UINTN      BlockIndex;
  UINTN      NumBlocks;
  UINTN      EndBlockIndex;
  EFI_STATUS Status;
  UINT8      *SpareBuffer = NULL;

  if (Buffer == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (Lba > LAST_BLOCK) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if ((BufferSize % gNandFlashInfo->BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto exit;
  }

  NumBlocks = DivU64x32(BufferSize, gNandFlashInfo->BlockSize);
  EndBlockIndex = ((UINTN)Lba + NumBlocks) - 1;

  SpareBuffer = (UINT8 *)AllocatePool(gNandFlashInfo->SparePageSize);
  if (SpareBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit;
  }

  // Erase block
  for (BlockIndex = (UINTN)Lba; BlockIndex <= EndBlockIndex; BlockIndex++) {
    Status = NandEraseBlock(BlockIndex);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "Erase block failed. Status: %x\n", Status));
      goto exit;
    }
  }

  // Program data
  Status = NandWriteBlock((UINTN)Lba, EndBlockIndex, Buffer, SpareBuffer);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Block write fails: %x\n", Status));
    goto exit;
  }

exit:
  if (SpareBuffer != NULL) {
    FreePool (SpareBuffer);
  }

  return Status;
}

EFI_STATUS
EFIAPI
NandFlashFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}



EFI_BLOCK_IO_MEDIA gNandFlashMedia = {
  SIGNATURE_32('n','a','n','d'),            // MediaId
  FALSE,                                    // RemovableMedia
  TRUE,                                     // MediaPresent
  FALSE,                                    // LogicalPartition
  FALSE,                                    // ReadOnly
  FALSE,                                    // WriteCaching
  0,                                        // BlockSize
  2,                                        // IoAlign
  0,                                        // Pad
  0                                         // LastBlock
};

EFI_BLOCK_IO_PROTOCOL BlockIo =
{
  EFI_BLOCK_IO_INTERFACE_REVISION,  // Revision
  &gNandFlashMedia,                  // *Media
  NandFlashReset,                   // Reset
  NandFlashReadBlocks,              // ReadBlocks
  NandFlashWriteBlocks,             // WriteBlocks
  NandFlashFlushBlocks              // FlushBlocks
};

EFI_STATUS
NandFlashInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  gNandFlashInfo = (NAND_FLASH_INFO *)AllocateZeroPool (sizeof(NAND_FLASH_INFO));

  //Initialize GPMC module.
  GpmcInit();

  //Reset NAND part
  NandFlashReset(&BlockIo, FALSE);

  //Detect NAND part and populate gNandFlashInfo structure
  Status = NandDetectPart ();
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Nand part id detection failure: Status: %x\n", Status));
    return Status;
  }

  //Count total number of 512Bytes chunk based on the page size.
  if (gNandFlashInfo->PageSize == PAGE_SIZE_512B) {
    gNum512BytesChunks = 1;
  } else if (gNandFlashInfo->PageSize == PAGE_SIZE_2K) {
    gNum512BytesChunks = 4;
  } else if (gNandFlashInfo->PageSize == PAGE_SIZE_4K) {
    gNum512BytesChunks = 8;
  }

  gEccCode = (UINT8 *)AllocatePool(gNum512BytesChunks * 3);
  if (gEccCode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //Configure ECC
  NandConfigureEcc ();

  //Patch EFI_BLOCK_IO_MEDIA structure.
  gNandFlashMedia.BlockSize = gNandFlashInfo->BlockSize;
  gNandFlashMedia.LastBlock = LAST_BLOCK;

  //Publish BlockIO.
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiBlockIoProtocolGuid, &BlockIo,
                  &gEfiDevicePathProtocolGuid, &gDevicePath,
                  NULL
                  );
  return Status;
}


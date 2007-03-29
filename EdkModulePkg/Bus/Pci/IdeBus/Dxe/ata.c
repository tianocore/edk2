/** @file
  Copyright (c) 2006 - 2007 Intel Corporation. <BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  2002-6: Add Atapi6 enhancement, support >120GB hard disk, including
  update - ATAIdentity() func
  update - AtaBlockIoReadBlocks() func
  update - AtaBlockIoWriteBlocks() func
  add    - AtaAtapi6Identify() func
  add    - AtaReadSectorsExt() func
  add    - AtaWriteSectorsExt() func
  add    - AtaPioDataInExt() func
  add    - AtaPioDataOutExt() func

**/

#include "idebus.h"

/**
  Sends out an ATA Identify Command to the specified device.

  This function is called by DiscoverIdeDevice() during its device
  identification. It sends out the ATA Identify Command to the
  specified device. Only ATA device responses to this command. If
  the command succeeds, it returns the Identify data structure which
  contains information about the device. This function extracts the
  information it needs to fill the IDE_BLK_IO_DEV data structure,
  including device type, media block size, media capacity, and etc.

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure,used
  to record all the information of the IDE device.

  @retval EFI_SUCCESS Identify ATA device successfully.

  @retval EFI_DEVICE_ERROR ATA Identify Device Command failed or
  device is not ATA device.

  @note
  parameter IdeDev will be updated in this function.

**/
EFI_STATUS
ATAIdentify (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
{
  EFI_STATUS        Status;
  EFI_IDENTIFY_DATA *AtaIdentifyPointer;
  UINT32            Capacity;
  UINT8             DeviceSelect;

  //
  //  AtaIdentifyPointer is used for accommodating returned IDENTIFY data of
  //  the ATA Identify command
  //
  AtaIdentifyPointer = (EFI_IDENTIFY_DATA *) AllocateZeroPool (sizeof (EFI_IDENTIFY_DATA));

  //
  //  use ATA PIO Data In protocol to send ATA Identify command
  //  and receive data from device
  //
  DeviceSelect  = (UINT8) ((IdeDev->Device) << 4);
  Status = AtaPioDataIn (
            IdeDev,
            (VOID *) AtaIdentifyPointer,
            sizeof (EFI_IDENTIFY_DATA),
            IDENTIFY_DRIVE_CMD,
            DeviceSelect,
            0,
            0,
            0,
            0
            );
  //
  // If ATA Identify command succeeds, then according to the received
  // IDENTIFY data,
  // identify the device type ( ATA or not ).
  // If ATA device, fill the information in IdeDev.
  // If not ATA device, return IDE_DEVICE_ERROR
  //
  if (!EFI_ERROR (Status)) {

    IdeDev->pIdData = AtaIdentifyPointer;

    //
    // Print ATA Module Name
    //
    PrintAtaModuleName (IdeDev);

    //
    // bit 15 of pAtaIdentify->config is used to identify whether device is
    // ATA device or ATAPI device.
    // if 0, means ATA device; if 1, means ATAPI device.
    //
    if ((AtaIdentifyPointer->AtaData.config & 0x8000) == 0x00) {
      //
      // Detect if support S.M.A.R.T. If yes, enable it as default
      //
      AtaSMARTSupport (IdeDev);

      //
      // Check whether this device needs 48-bit addressing (ATAPI-6 ata device)
      //
      Status = AtaAtapi6Identify (IdeDev);
      if (!EFI_ERROR (Status)) {
        //
        // It's a disk with >120GB capacity, initialized in AtaAtapi6Identify()
        //
        return EFI_SUCCESS;
      }
      //
      // This is a hard disk <= 120GB capacity, treat it as normal hard disk
      //
      IdeDev->Type = IdeHardDisk;

      //
      // Block Media Information:
      // Media->LogicalPartition , Media->WriteCaching will be filled
      // in the DiscoverIdeDevcie() function.
      //
      IdeDev->BlkIo.Media->IoAlign        = 4;
      IdeDev->BlkIo.Media->MediaId        = 1;
      IdeDev->BlkIo.Media->RemovableMedia = FALSE;
      IdeDev->BlkIo.Media->MediaPresent   = TRUE;
      IdeDev->BlkIo.Media->ReadOnly       = FALSE;
      IdeDev->BlkIo.Media->BlockSize      = 0x200;

      //
      // Calculate device capacity
      //
      Capacity = ((UINT32)AtaIdentifyPointer->AtaData.user_addressable_sectors_hi << 16) |
                  AtaIdentifyPointer->AtaData.user_addressable_sectors_lo ;
      IdeDev->BlkIo.Media->LastBlock = Capacity - 1;

      return EFI_SUCCESS;

    }
  }

  gBS->FreePool (AtaIdentifyPointer);
  //
  // Make sure the pIdData will not be freed again.
  //
  IdeDev->pIdData = NULL;

  return EFI_DEVICE_ERROR;
}


/**
  This function is called by ATAIdentify() to identity whether this disk
  supports ATA/ATAPI6 48bit addressing, ie support >120G capacity

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @retval  EFI_SUCCESS The disk specified by IdeDev is a Atapi6 supported one
  and 48-bit addressing must be used

  @retval  EFI_UNSUPPORTED The disk dosn't not support Atapi6 or it supports but
  the capacity is below 120G, 48bit addressing is not
  needed

  @note
  This function must be called after DEVICE_IDENTITY command has been
  successfully returned

**/
EFI_STATUS
AtaAtapi6Identify (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
{
  UINT8             Index;
  EFI_LBA           TmpLba;
  EFI_LBA           Capacity;
  EFI_IDENTIFY_DATA *Atapi6IdentifyStruct;

  if (IdeDev->pIdData == NULL) {
    return EFI_UNSUPPORTED;
  }

  Atapi6IdentifyStruct = IdeDev->pIdData;

  if ((Atapi6IdentifyStruct->AtapiData.cmd_set_support_83 & bit10) == 0) {
    //
    // The device dosn't support 48 bit addressing
    //
    return EFI_UNSUPPORTED;
  }

  //
  // 48 bit address feature set is supported, get maximum capacity
  //
  Capacity = Atapi6IdentifyStruct->AtapiData.max_user_lba_for_48bit_addr[0];
  for (Index = 1; Index < 4; Index++) {
    //
    // Lower byte goes first: word[100] is the lowest word, word[103] is highest
    //
    TmpLba = Atapi6IdentifyStruct->AtapiData.max_user_lba_for_48bit_addr[Index];
    Capacity |= LShiftU64 (TmpLba, 16 * Index);
  }

  if (Capacity > MAX_28BIT_ADDRESSING_CAPACITY) {
    //
    // Capacity exceeds 120GB. 48-bit addressing is really needed
    //
    IdeDev->Type = Ide48bitAddressingHardDisk;

    //
    // Fill block media information:Media->LogicalPartition ,
    // Media->WriteCaching will be filledin the DiscoverIdeDevcie() function.
    //
    IdeDev->BlkIo.Media->IoAlign        = 4;
    IdeDev->BlkIo.Media->MediaId        = 1;
    IdeDev->BlkIo.Media->RemovableMedia = FALSE;
    IdeDev->BlkIo.Media->MediaPresent   = TRUE;
    IdeDev->BlkIo.Media->ReadOnly       = FALSE;
    IdeDev->BlkIo.Media->BlockSize      = 0x200;
    IdeDev->BlkIo.Media->LastBlock      = Capacity - 1;

    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

/**
  This function is called by ATAIdentify() or ATAPIIdentify()
  to print device's module name.

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

**/
VOID
PrintAtaModuleName (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
{
  if (IdeDev->pIdData == NULL) {
    return ;
  }

  SwapStringChars (IdeDev->ModelName, IdeDev->pIdData->AtaData.ModelName, 40);
  IdeDev->ModelName[40] = 0x00;
}

/**
  This function is used to send out ATA commands conforms to the
  PIO Data In Protocol.

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @param[in] *Buffer
  buffer contained data transferred from device to host.

  @param[in] ByteCount
  data size in byte unit of the buffer.

  @param[in] AtaCommand
  value of the Command Register

  @param[in] Head
  value of the Head/Device Register

  @param[in] SectorCount
  value of the Sector Count Register

  @param[in] SectorNumber
  value of the Sector Number Register

  @param[in] CylinderLsb
  value of the low byte of the Cylinder Register

  @param[in] CylinderMsb
  value of the high byte of the Cylinder Register

  @retval EFI_SUCCESS send out the ATA command and device send required
  data successfully.

  @retval EFI_DEVICE_ERROR command sent failed.

**/
EFI_STATUS
AtaPioDataIn (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  UINT32          ByteCount,
  IN  UINT8           AtaCommand,
  IN  UINT8           Head,
  IN  UINT8           SectorCount,
  IN  UINT8           SectorNumber,
  IN  UINT8           CylinderLsb,
  IN  UINT8           CylinderMsb
  )
{
  UINTN       WordCount;
  UINTN       Increment;
  UINT16      *Buffer16;
  EFI_STATUS  Status;

  Status = WaitForBSYClear (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  //  e0:1110,0000-- bit7 and bit5 are reserved bits.
  //           bit6 set means LBA mode
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((IdeDev->Device << 4) | 0xe0 | Head)
    );

  //
  // All ATAPI device's ATA commands can be issued regardless of the
  // state of the DRDY
  //
  if (IdeDev->Type == IdeHardDisk) {

    Status = DRDYReady (IdeDev, ATATIMEOUT);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
  }
  //
  // set all the command parameters
  // Before write to all the following registers, BSY and DRQ must be 0.
  //
  Status = DRQClear2 (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (AtaCommand == SET_FEATURES_CMD) {
    IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, 0x03);
  }

  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorCount, SectorCount);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorNumber, SectorNumber);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb, CylinderLsb);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb, CylinderMsb);

  //
  // send command via Command Register
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, AtaCommand);

  Buffer16 = (UINT16 *) Buffer;

  //
  // According to PIO data in protocol, host can perform a series of reads to
  // the data register after each time device set DRQ ready;
  // The data size of "a series of read" is command specific.
  // For most ATA command, data size received from device will not exceed
  // 1 sector, hence the data size for "a series of read" can be the whole data
  // size of one command request.
  // For ATA command such as Read Sector command, the data size of one ATA
  // command request is often larger than 1 sector, according to the
  // Read Sector command, the data size of "a series of read" is exactly 1
  // sector.
  // Here for simplification reason, we specify the data size for
  // "a series of read" to 1 sector (256 words) if data size of one ATA command
  // request is larger than 256 words.
  //
  Increment = 256;

  //
  // used to record bytes of currently transfered data
  //
  WordCount = 0;

  while (WordCount < ByteCount / 2) {
    //
    // Poll DRQ bit set, data transfer can be performed only when DRQ is ready.
    //
    Status = DRQReady2 (IdeDev, ATATIMEOUT);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Status = CheckErrorStatus (IdeDev);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    //
    // Get the byte count for one series of read
    //
    if ((WordCount + Increment) > ByteCount / 2) {
      Increment = ByteCount / 2 - WordCount;
    }

    IDEReadPortWMultiple (
      IdeDev->PciIo,
      IdeDev->IoPort->Data,
      Increment,
      Buffer16
      );

    WordCount += Increment;
    Buffer16 += Increment;

  }

  DRQClear (IdeDev, ATATIMEOUT);

  return CheckErrorStatus (IdeDev);
}

/**
  This function is used to send out ATA commands conforms to the
  PIO Data Out Protocol.

  @param *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @param *Buffer      buffer contained data transferred from host to device.
  @param ByteCount    data size in byte unit of the buffer.
  @param AtaCommand   value of the Command Register
  @param Head         value of the Head/Device Register
  @param SectorCount  value of the Sector Count Register
  @param SectorNumber value of the Sector Number Register
  @param CylinderLsb  value of the low byte of the Cylinder Register
  @param CylinderMsb  value of the high byte of the Cylinder Register

  @retval EFI_SUCCESS send out the ATA command and device received required
  data successfully.

  @retval EFI_DEVICE_ERROR command sent failed.

**/
EFI_STATUS
AtaPioDataOut (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  UINT32          ByteCount,
  IN  UINT8           AtaCommand,
  IN  UINT8           Head,
  IN  UINT8           SectorCount,
  IN  UINT8           SectorNumber,
  IN  UINT8           CylinderLsb,
  IN  UINT8           CylinderMsb
  )
{
  UINTN       WordCount;
  UINTN       Increment;
  UINT16      *Buffer16;
  EFI_STATUS  Status;

  Status = WaitForBSYClear (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // select device via Head/Device register.
  // Before write Head/Device register, BSY and DRQ must be 0.
  //
  Status = DRQClear2 (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // e0:1110,0000-- bit7 and bit5 are reserved bits.
  //          bit6 set means LBA mode
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((IdeDev->Device << 4) | 0xe0 | Head)
    );

  Status = DRDYReady (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // set all the command parameters
  // Before write to all the following registers, BSY and DRQ must be 0.
  //
  Status = DRQClear2 (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorCount, SectorCount);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorNumber, SectorNumber);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb, CylinderLsb);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb, CylinderMsb);

  //
  // send command via Command Register
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, AtaCommand);

  Buffer16 = (UINT16 *) Buffer;

  //
  // According to PIO data out protocol, host can perform a series of
  // writes to the data register after each time device set DRQ ready;
  // The data size of "a series of read" is command specific.
  // For most ATA command, data size written to device will not exceed 1 sector,
  // hence the data size for "a series of write" can be the data size of one
  // command request.
  // For ATA command such as Write Sector command, the data size of one
  // ATA command request is often larger than 1 sector, according to the
  // Write Sector command, the data size of "a series of read" is exactly
  // 1 sector.
  // Here for simplification reason, we specify the data size for
  // "a series of write" to 1 sector (256 words) if data size of one ATA command
  // request is larger than 256 words.
  //
  Increment = 256;
  WordCount = 0;

  while (WordCount < ByteCount / 2) {

    //
    // DRQReady2-- read Alternate Status Register to determine the DRQ bit
    // data transfer can be performed only when DRQ is ready.
    //
    Status = DRQReady2 (IdeDev, ATATIMEOUT);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Status = CheckErrorStatus (IdeDev);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

   //
   // Check the remaining byte count is less than 512 bytes
   //
   if ((WordCount + Increment) > ByteCount / 2) {
      Increment = ByteCount / 2 - WordCount;
    }
    //
    // perform a series of write without check DRQ ready
    //

    IDEWritePortWMultiple (
      IdeDev->PciIo,
      IdeDev->IoPort->Data,
      Increment,
      Buffer16
      );
    WordCount += Increment;
    Buffer16 += Increment;

  }

  DRQClear (IdeDev, ATATIMEOUT);

  return CheckErrorStatus (IdeDev);
}

/**
  This function is used to analyze the Status Register and print out
  some debug information and if there is ERR bit set in the Status
  Register, the Error Register's value is also be parsed and print out.

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @retval EFI_SUCCESS       No err information in the Status Register.
  @retval EFI_DEVICE_ERROR  Any err information in the Status Register.

**/
EFI_STATUS
CheckErrorStatus (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
{
  UINT8 StatusRegister;
  UINT8 ErrorRegister;

  StatusRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Status);

  DEBUG_CODE_BEGIN ();

    if (StatusRegister & DWF) {
      DEBUG (
        (EFI_D_BLKIO,
        "CheckErrorStatus()-- %02x : Error : Write Fault\n",
        StatusRegister)
        );
    }

    if (StatusRegister & CORR) {
      DEBUG (
        (EFI_D_BLKIO,
        "CheckErrorStatus()-- %02x : Error : Corrected Data\n",
        StatusRegister)
        );
    }

    if (StatusRegister & ERR) {
      ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);

      if (ErrorRegister & BBK_ERR) {
      DEBUG (
        (EFI_D_BLKIO,
        "CheckErrorStatus()-- %02x : Error : Bad Block Detected\n",
        ErrorRegister)
        );
      }

      if (ErrorRegister & UNC_ERR) {
        DEBUG (
          (EFI_D_BLKIO,
          "CheckErrorStatus()-- %02x : Error : Uncorrectable Data\n",
          ErrorRegister)
          );
      }

      if (ErrorRegister & MC_ERR) {
        DEBUG (
          (EFI_D_BLKIO,
          "CheckErrorStatus()-- %02x : Error : Media Change\n",
          ErrorRegister)
          );
      }

      if (ErrorRegister & ABRT_ERR) {
        DEBUG (
          (EFI_D_BLKIO,
          "CheckErrorStatus()-- %02x : Error : Abort\n",
          ErrorRegister)
          );
      }

      if (ErrorRegister & TK0NF_ERR) {
        DEBUG (
          (EFI_D_BLKIO,
          "CheckErrorStatus()-- %02x : Error : Track 0 Not Found\n",
          ErrorRegister)
          );
      }

      if (ErrorRegister & AMNF_ERR) {
        DEBUG (
          (EFI_D_BLKIO,
          "CheckErrorStatus()-- %02x : Error : Address Mark Not Found\n",
          ErrorRegister)
          );
      }
    }

  DEBUG_CODE_END ();

  if ((StatusRegister & (ERR | DWF | CORR)) == 0) {
    return EFI_SUCCESS;
  }

  return EFI_DEVICE_ERROR;

}

/**
  This function is called by the AtaBlkIoReadBlocks() to perform
  reading from media in block unit.

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @param[in] *DataBuffer
  A pointer to the destination buffer for the data.

  @param[in] Lba
  The starting logical block address to read from
  on the device media.

  @param[in] NumberOfBlocks
  The number of transfer data blocks.

  @return return status is fully dependent on the return status
  of AtaPioDataIn() function.

**/
EFI_STATUS
AtaReadSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
{
  EFI_STATUS  Status;
  UINTN       BlocksRemaining;
  UINT32      Lba32;
  UINT8       Lba0;
  UINT8       Lba1;
  UINT8       Lba2;
  UINT8       Lba3;
  UINT8       AtaCommand;
  UINT8       SectorCount8;
  UINT16      SectorCount;
  UINTN       ByteCount;
  VOID        *Buffer;

  Buffer = DataBuffer;

  //
  // Using ATA Read Sector(s) command (opcode=0x20) with PIO DATA IN protocol
  //
  AtaCommand      = READ_SECTORS_CMD;


  BlocksRemaining = NumberOfBlocks;

  Lba32           = (UINT32) Lba;

  Status          = EFI_SUCCESS;

  while (BlocksRemaining > 0) {

    //
    // in ATA-3 spec, LBA is in 28 bit width
    //
    Lba0  = (UINT8) Lba32;
    Lba1  = (UINT8) (Lba32 >> 8);
    Lba2  = (UINT8) (Lba32 >> 16);
    //
    // low 4 bit of Lba3 stands for LBA bit24~bit27.
    //
    Lba3 = (UINT8) ((Lba32 >> 24) & 0x0f);

    if (BlocksRemaining >= 0x100) {

      //
      //  SectorCount8 is sent to Sector Count register, 0x00 means 256
      //  sectors to be read
      //
      SectorCount8 = 0x00;
      //
      //  SectorCount is used to record the number of sectors to be read
      //
      SectorCount = 256;
    } else {

      SectorCount8  = (UINT8) BlocksRemaining;
      SectorCount   = (UINT16) BlocksRemaining;
    }

    //
    // ByteCount is the number of bytes that will be read
    //
    ByteCount = SectorCount * (IdeDev->BlkIo.Media->BlockSize);

    //
    // call AtaPioDataIn() to send Read Sector Command and receive data read
    //
    Status = AtaPioDataIn (
              IdeDev,
              Buffer,
              (UINT32) ByteCount,
              AtaCommand,
              Lba3,
              SectorCount8,
              Lba0,
              Lba1,
              Lba2
              );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Lba32 += SectorCount;
    Buffer = ((UINT8 *) Buffer + ByteCount);
    BlocksRemaining -= SectorCount;
  }

  return Status;
}

/**
  This function is called by the AtaBlkIoWriteBlocks() to perform
  writing onto media in block unit.

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure,used
  to record all the information of the IDE device.

  @param[in] *BufferData
  A pointer to the source buffer for the data.

  @param[in] Lba
  The starting logical block address to write onto
  the device media.

  @param[in] NumberOfBlocks
  The number of transfer data blocks.

  @return return status is fully dependent on the return status
  of AtaPioDataOut() function.

**/
EFI_STATUS
AtaWriteSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *BufferData,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
{
  EFI_STATUS  Status;
  UINTN       BlocksRemaining;
  UINT32      Lba32;
  UINT8       Lba0;
  UINT8       Lba1;
  UINT8       Lba2;
  UINT8       Lba3;
  UINT8       AtaCommand;
  UINT8       SectorCount8;
  UINT16      SectorCount;
  UINTN       ByteCount;
  VOID        *Buffer;

  Buffer = BufferData;

  //
  // Using Write Sector(s) command (opcode=0x30) with PIO DATA OUT protocol
  //
  AtaCommand      = WRITE_SECTORS_CMD;

  BlocksRemaining = NumberOfBlocks;

  Lba32           = (UINT32) Lba;

  Status          = EFI_SUCCESS;

  while (BlocksRemaining > 0) {

    Lba0  = (UINT8) Lba32;
    Lba1  = (UINT8) (Lba32 >> 8);
    Lba2  = (UINT8) (Lba32 >> 16);
    Lba3  = (UINT8) ((Lba32 >> 24) & 0x0f);

    if (BlocksRemaining >= 0x100) {

      //
      //  SectorCount8 is sent to Sector Count register, 0x00 means 256 sectors
      //  to be written
      //
      SectorCount8 = 0x00;
      //
      //  SectorCount is used to record the number of sectors to be written
      //
      SectorCount = 256;
    } else {

      SectorCount8  = (UINT8) BlocksRemaining;
      SectorCount   = (UINT16) BlocksRemaining;
    }

    ByteCount = SectorCount * (IdeDev->BlkIo.Media->BlockSize);

    Status = AtaPioDataOut (
              IdeDev,
              Buffer,
              (UINT32) ByteCount,
              AtaCommand,
              Lba3,
              SectorCount8,
              Lba0,
              Lba1,
              Lba2
              );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Lba32 += SectorCount;
    Buffer = ((UINT8 *) Buffer + ByteCount);
    BlocksRemaining -= SectorCount;
  }

  return Status;
}

/**
  This function is used to implement the Soft Reset on the specified
  device. But, the ATA Soft Reset mechanism is so strong a reset method
  that it will force resetting on both devices connected to the
  same cable.

  It is called by IdeBlkIoReset(), a interface function of Block
  I/O protocol.

  This function can also be used by the ATAPI device to perform reset when
  ATAPI Reset command is failed.

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @retval EFI_SUCCESS       Soft reset completes successfully.
  @retval EFI_DEVICE_ERROR  Any step during the reset process is failed.

  @note
  The registers initial values after ATA soft reset are different
  to the ATA device and ATAPI device.

**/
EFI_STATUS
AtaSoftReset (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
{

  UINT8 DeviceControl;

  DeviceControl = 0;
  //
  // set SRST bit to initiate soft reset
  //
  DeviceControl |= SRST;

  //
  // disable Interrupt
  //
  DeviceControl |= bit1;

  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Alt.DeviceControl, DeviceControl);

  //
  // SRST should assert for at least 5 us, we use 10 us for
  // better compatibility
  //
  gBS->Stall (10);

  //
  // Enable interrupt to support UDMA, and clear SRST bit
  //
  DeviceControl = 0;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Alt.DeviceControl, DeviceControl);

  //
  // Wait for at least 2 ms to check BSY status, we use 10 ms
  // for better compatibility
  //
  gBS->Stall(10000);
  //
  // slave device needs at most 31s to clear BSY
  //
  if (WaitForBSYClear (IdeDev, 31000) == EFI_TIMEOUT) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  This function is the ATA implementation for ReadBlocks in the
  Block I/O Protocol interface.

  @param[in] *IdeBlkIoDevice
  Indicates the calling context.

  @param[in] MediaId
  The media id that the read request is for.

  @param[in] LBA
  The starting logical block address to read from
  on the device.

  @param[in] BufferSize
  The size of the Buffer in bytes. This must be a
  multiple of the intrinsic block size of the device.

  @param[out] *Buffer
  A pointer to the destination buffer for the data.
  The caller is responsible for either having implicit
  or explicit ownership of the memory that data is read into.

  @retval EFI_SUCCESS       Read Blocks successfully.
  @retval EFI_DEVICE_ERROR  Read Blocks failed.
  @retval EFI_NO_MEDIA      There is no media in the device.
  @retval EFI_MEDIA_CHANGE  The MediaId is not for the current media.

  @retval EFI_BAD_BUFFER_SIZE
  The BufferSize parameter is not a multiple of the
  intrinsic block size of the device.

  @retval EFI_INVALID_PARAMETER
  The read request contains LBAs that are not valid,
  or the data buffer is not valid.

  @note
  If Read Block error because of device error, this function will call
  AtaSoftReset() function to reset device.

**/
EFI_STATUS
AtaBlkIoReadBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          LBA,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  )
{
  EFI_BLOCK_IO_MEDIA  *Media;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  EFI_STATUS          Status;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  Status = EFI_SUCCESS;

  //
  //  Get the intrinsic block size
  //
  Media           = IdeBlkIoDevice->BlkIo.Media;
  BlockSize       = Media->BlockSize;

  NumberOfBlocks  = BufferSize / BlockSize;

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (!(Media->MediaPresent)) {
    return EFI_NO_MEDIA;
  }

  if (LBA > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((LBA + NumberOfBlocks - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Media->IoAlign > 1) && (((UINTN) Buffer & (Media->IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  if (IdeBlkIoDevice->Type == Ide48bitAddressingHardDisk) {
    //
    // For ATA/ATAPI-6 device(capcity > 120GB), use ATA-6 read block mechanism
    //
    if (IdeBlkIoDevice->UdmaMode.Valid) {
      Status = AtaUdmaReadExt (IdeBlkIoDevice, Buffer, LBA, NumberOfBlocks);
    } else {
      Status = AtaReadSectorsExt (IdeBlkIoDevice, Buffer, LBA, NumberOfBlocks);
    }
  } else {
    //
    // For ATA-3 compatible device, use ATA-3 read block mechanism
    //
    if (IdeBlkIoDevice->UdmaMode.Valid) {
      Status = AtaUdmaRead (IdeBlkIoDevice, Buffer, LBA, NumberOfBlocks);
    } else {
      Status = AtaReadSectors (IdeBlkIoDevice, Buffer, LBA, NumberOfBlocks);
    }
  }

  if (EFI_ERROR (Status)) {
    AtaSoftReset (IdeBlkIoDevice);
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;

}

/**
  This function is the ATA implementation for WriteBlocks in the
  Block I/O Protocol interface.

  @param[in] *IdeBlkIoDevice
  Indicates the calling context.

  @param[in] MediaId
  The media id that the write request is for.

  @param[in] LBA
  The starting logical block address to write onto
  the device.

  @param[in] BufferSize
  The size of the Buffer in bytes. This must be a
  multiple of the intrinsic block size of the device.

  @param[out] *Buffer
  A pointer to the source buffer for the data.
  The caller is responsible for either having implicit
  or explicit ownership of the memory that data is
  written from.

  @retval EFI_SUCCESS       Write Blocks successfully.
  @retval EFI_DEVICE_ERROR  Write Blocks failed.
  @retval EFI_NO_MEDIA      There is no media in the device.
  @retval EFI_MEDIA_CHANGE  The MediaId is not for the current media.

  @retval EFI_BAD_BUFFER_SIZE
  The BufferSize parameter is not a multiple of the
  intrinsic block size of the device.

  @retval EFI_INVALID_PARAMETER
  The write request contains LBAs that are not valid,
  or the data buffer is not valid.

  @note
  If Write Block error because of device error, this function will call
  AtaSoftReset() function to reset device.

**/
EFI_STATUS
AtaBlkIoWriteBlocks (
  IN  IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN  UINT32           MediaId,
  IN  EFI_LBA          LBA,
  IN  UINTN            BufferSize,
  OUT VOID             *Buffer
  )
{

  EFI_BLOCK_IO_MEDIA  *Media;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  EFI_STATUS          Status;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  Status = EFI_SUCCESS;

  //
  // Get the intrinsic block size
  //
  Media           = IdeBlkIoDevice->BlkIo.Media;
  BlockSize       = Media->BlockSize;
  NumberOfBlocks  = BufferSize / BlockSize;

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (LBA > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((LBA + NumberOfBlocks - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Media->IoAlign > 1) && (((UINTN) Buffer & (Media->IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  if (IdeBlkIoDevice->Type == Ide48bitAddressingHardDisk) {
    //
    // For ATA/ATAPI-6 device(capcity > 120GB), use ATA-6 write block mechanism
    //
    if (IdeBlkIoDevice->UdmaMode.Valid) {
      Status = AtaUdmaWriteExt (IdeBlkIoDevice, Buffer, LBA, NumberOfBlocks);
    } else {
      Status = AtaWriteSectorsExt (IdeBlkIoDevice, Buffer, LBA, NumberOfBlocks);
    }
  } else {
    //
    // For ATA-3 compatible device, use ATA-3 write block mechanism
    //
    if (IdeBlkIoDevice->UdmaMode.Valid) {
      Status = AtaUdmaWrite (IdeBlkIoDevice, Buffer, LBA, NumberOfBlocks);
    } else {
      Status = AtaWriteSectors (IdeBlkIoDevice, Buffer, LBA, NumberOfBlocks);
    }
  }

  if (EFI_ERROR (Status)) {
    AtaSoftReset (IdeBlkIoDevice);
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  This function is called by the AtaBlkIoReadBlocks() to perform
  reading from media in block unit. The function has been enhanced to
  support >120GB access and transfer at most 65536 blocks per command

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @param[in] *DataBuffer    A pointer to the destination buffer for the data.
  @param[in] StartLba       The starting logical block address to read from
  on the device media.
  @param[in] NumberOfBlocks The number of transfer data blocks.

  @return return status is fully dependent on the return status
  of AtaPioDataInExt() function.

**/
EFI_STATUS
AtaReadSectorsExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
{
  EFI_STATUS  Status;
  UINTN       BlocksRemaining;
  EFI_LBA     Lba64;
  UINT8       AtaCommand;
  UINT16      SectorCount;
  UINT32      ByteCount;
  VOID        *Buffer;

  //
  // Using ATA "Read Sectors Ext" command(opcode=0x24) with PIO DATA IN protocol
  //
  AtaCommand      = READ_SECTORS_EXT_CMD;
  Buffer          = DataBuffer;
  BlocksRemaining = NumberOfBlocks;
  Lba64           = StartLba;
  Status          = EFI_SUCCESS;

  while (BlocksRemaining > 0) {

    if (BlocksRemaining >= 0x10000) {
      //
      //  SectorCount is used to record the number of sectors to be read
      //  Max 65536 sectors can be transfered at a time.
      //
      SectorCount = 0xffff;
    } else {
      SectorCount = (UINT16) BlocksRemaining;
    }

    //
    // ByteCount is the number of bytes that will be read
    //
    ByteCount = SectorCount * (IdeDev->BlkIo.Media->BlockSize);

    //
    // call AtaPioDataInExt() to send Read Sector Command and receive data read
    //
    Status = AtaPioDataInExt (
              IdeDev,
              Buffer,
              ByteCount,
              AtaCommand,
              Lba64,
              SectorCount
              );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Lba64 += SectorCount;
    Buffer = ((UINT8 *) Buffer + ByteCount);
    BlocksRemaining -= SectorCount;
  }

  return Status;
}

/**
  This function is called by the AtaBlkIoWriteBlocks() to perform
  writing onto media in block unit. The function has been enhanced to
  support >120GB access and transfer at most 65536 blocks per command

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure,used
  to record all the information of the IDE device.

  @param[in] *DataBuffer
  A pointer to the source buffer for the data.

  @param[in] Lba
  The starting logical block address to write onto
  the device media.

  @param[in] NumberOfBlocks
  The number of transfer data blocks.

  @return status is fully dependent on the return status
  of AtaPioDataOutExt() function.

**/
EFI_STATUS
AtaWriteSectorsExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
{
  EFI_STATUS  Status;
  EFI_LBA     Lba64;
  UINTN       BlocksRemaining;
  UINT8       AtaCommand;
  UINT16      SectorCount;
  UINT32      ByteCount;
  VOID        *Buffer;

  //
  // Using ATA "Write Sectors Ext" cmd(opcode=0x24) with PIO DATA OUT protocol
  //
  AtaCommand      = WRITE_SECTORS_EXT_CMD;
  Lba64           = StartLba;
  Buffer          = DataBuffer;
  BlocksRemaining = NumberOfBlocks;

  Status          = EFI_SUCCESS;

  while (BlocksRemaining > 0) {

    if (BlocksRemaining >= 0x10000) {
      //
      //  SectorCount is used to record the number of sectors to be written.
      //  Max 65536 sectors can be transfered at a time.
      //
      SectorCount = 0xffff;
    } else {
      SectorCount = (UINT16) BlocksRemaining;
    }

    //
    // ByteCount is the number of bytes that will be written
    //
    ByteCount = SectorCount * (IdeDev->BlkIo.Media->BlockSize);

    //
    // Call AtaPioDataOutExt() to send "Write Sectors Ext" Command
    //
    Status = AtaPioDataOutExt (
              IdeDev,
              Buffer,
              ByteCount,
              AtaCommand,
              Lba64,
              SectorCount
              );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Lba64 += SectorCount;
    Buffer = ((UINT8 *) Buffer + ByteCount);
    BlocksRemaining -= SectorCount;
  }

  return Status;
}

/**
  This function is used to send out ATA commands conforms to the
  PIO Data In Protocol, supporting ATA/ATAPI-6 standard

  Comparing with ATA-3 data in protocol, we have two differents here:<BR>
  1. Do NOT wait for DRQ clear before sending command into IDE device.(the
  wait will frequently fail... cause writing function return error)

  2. Do NOT wait for DRQ clear after all data readed.(the wait greatly
  slow down writing performance by 100 times!)

  @param[in] *IdeDev pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @param[in,out] *Buffer  buffer contained data transferred from device to host.
  @param[in] ByteCount    data size in byte unit of the buffer.
  @param[in] AtaCommand   value of the Command Register
  @param[in] StartLba     the start LBA of this transaction
  @param[in] SectorCount  the count of sectors to be transfered

  @retval EFI_SUCCESS send out the ATA command and device send required
  data successfully.

  @retval EFI_DEVICE_ERROR command sent failed.

**/
EFI_STATUS
AtaPioDataInExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  OUT VOID        *Buffer,
  IN  UINT32          ByteCount,
  IN  UINT8           AtaCommand,
  IN  EFI_LBA         StartLba,
  IN  UINT16          SectorCount
  )
{
  UINT8       DevSel;
  UINT8       SectorCount8;
  UINT8       LbaLow;
  UINT8       LbaMid;
  UINT8       LbaHigh;
  UINTN       WordCount;
  UINTN       Increment;
  UINT16      *Buffer16;
  EFI_STATUS  Status;

  Status = WaitForBSYClear (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Select device, set bit6 as 1 to indicate LBA mode is used
  //
  DevSel = (UINT8) (IdeDev->Device << 4);
  DevSel |= 0x40;
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    DevSel
    );

  //
  // Wait for DRDY singnal asserting. ATAPI device needn't wait
  //
  if ( (IdeDev->Type == IdeHardDisk)  ||
        (IdeDev->Type == Ide48bitAddressingHardDisk)) {

    Status = DRDYReady (IdeDev, ATATIMEOUT);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Fill feature register if needed
  //
  if (AtaCommand == SET_FEATURES_CMD) {
    IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, 0x03);
  }

  //
  // Fill the sector count register, which is a two-byte FIFO. Need write twice.
  //
  SectorCount8 = (UINT8) (SectorCount >> 8);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorCount, SectorCount8);

  SectorCount8 = (UINT8) SectorCount;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorCount, SectorCount8);

  //
  // Fill the start LBA registers, which are also two-byte FIFO
  //
  LbaLow  = (UINT8) RShiftU64 (StartLba, 24);
  LbaMid  = (UINT8) RShiftU64 (StartLba, 32);
  LbaHigh = (UINT8) RShiftU64 (StartLba, 40);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorNumber, LbaLow);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb, LbaMid);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb, LbaHigh);

  LbaLow  = (UINT8) StartLba;
  LbaMid  = (UINT8) RShiftU64 (StartLba, 8);
  LbaHigh = (UINT8) RShiftU64 (StartLba, 16);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorNumber, LbaLow);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb, LbaMid);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb, LbaHigh);

  //
  // Send command via Command Register, invoking the processing of this command
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, AtaCommand);

  Buffer16 = (UINT16 *) Buffer;

  //
  // According to PIO data in protocol, host can perform a series of reads to
  // the data register after each time device set DRQ ready;
  //

  //
  // 256 words
  //
  Increment = 256;

  //
  // used to record bytes of currently transfered data
  //
  WordCount = 0;

  while (WordCount < ByteCount / 2) {
    //
    // Poll DRQ bit set, data transfer can be performed only when DRQ is ready.
    //
    Status = DRQReady2 (IdeDev, ATATIMEOUT);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Status = CheckErrorStatus (IdeDev);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    //
    // Get the byte count for one series of read
    //
    if ((WordCount + Increment) > ByteCount / 2) {
      Increment = ByteCount / 2 - WordCount;
    }

    IDEReadPortWMultiple (
      IdeDev->PciIo,
      IdeDev->IoPort->Data,
      Increment,
      Buffer16
      );

    WordCount += Increment;
    Buffer16 += Increment;

  }

  return CheckErrorStatus (IdeDev);
}

/**
  This function is used to send out ATA commands conforms to the
  PIO Data Out Protocol, supporting ATA/ATAPI-6 standard

  Comparing with ATA-3 data out protocol, we have two differents here:<BR>
  1. Do NOT wait for DRQ clear before sending command into IDE device.(the
  wait will frequently fail... cause writing function return error)

  2. Do NOT wait for DRQ clear after all data readed.(the wait greatly
  slow down writing performance by 100 times!)

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @param[in] *Buffer      buffer contained data transferred from host to device.
  @param[in] ByteCount    data size in byte unit of the buffer.
  @param[in] AtaCommand   value of the Command Register
  @param[in] StartLba     the start LBA of this transaction
  @param[in] SectorCount  the count of sectors to be transfered

  @retval EFI_SUCCESS send out the ATA command and device receive required
  data successfully.

  @retval EFI_DEVICE_ERROR command sent failed.

**/
EFI_STATUS
AtaPioDataOutExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  UINT32          ByteCount,
  IN  UINT8           AtaCommand,
  IN  EFI_LBA         StartLba,
  IN  UINT16          SectorCount
  )
{
  UINT8       DevSel;
  UINT8       SectorCount8;
  UINT8       LbaLow;
  UINT8       LbaMid;
  UINT8       LbaHigh;
  UINTN       WordCount;
  UINTN       Increment;
  UINT16      *Buffer16;
  EFI_STATUS  Status;

  Status = WaitForBSYClear (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Select device. Set bit6 as 1 to indicate LBA mode is used
  //
  DevSel = (UINT8) (IdeDev->Device << 4);
  DevSel |= 0x40;
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    DevSel
    );

  //
  // Wait for DRDY singnal asserting.
  //
  Status = DRDYReady (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Fill feature register if needed
  //
  if (AtaCommand == SET_FEATURES_CMD) {
    IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, 0x03);
  }

  //
  // Fill the sector count register, which is a two-byte FIFO. Need write twice.
  //
  SectorCount8 = (UINT8) (SectorCount >> 8);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorCount, SectorCount8);

  SectorCount8 = (UINT8) SectorCount;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorCount, SectorCount8);

  //
  // Fill the start LBA registers, which are also two-byte FIFO
  //
  LbaLow  = (UINT8) RShiftU64 (StartLba, 24);
  LbaMid  = (UINT8) RShiftU64 (StartLba, 32);
  LbaHigh = (UINT8) RShiftU64 (StartLba, 40);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorNumber, LbaLow);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb, LbaMid);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb, LbaHigh);

  LbaLow  = (UINT8) StartLba;
  LbaMid  = (UINT8) RShiftU64 (StartLba, 8);
  LbaHigh = (UINT8) RShiftU64 (StartLba, 16);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorNumber, LbaLow);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb, LbaMid);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb, LbaHigh);

  //
  // Send command via Command Register, invoking the processing of this command
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, AtaCommand);

  Buffer16 = (UINT16 *) Buffer;

  //
  // According to PIO Data Out protocol, host can perform a series of writes to
  // the data register after each time device set DRQ ready;
  //
  Increment = 256;

  //
  // used to record bytes of currently transfered data
  //
  WordCount = 0;

  while (WordCount < ByteCount / 2) {
    //
    // Poll DRQ bit set, data transfer can be performed only when DRQ is ready.
    //
    Status = DRQReady2 (IdeDev, ATATIMEOUT);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Status = CheckErrorStatus (IdeDev);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    //
    // Write data into device by one series of writing to data register
    //
    if ((WordCount + Increment) > ByteCount / 2) {
      Increment = ByteCount / 2 - WordCount;
    }

    IDEWritePortWMultiple (
      IdeDev->PciIo,
      IdeDev->IoPort->Data,
      Increment,
      Buffer16
      );

    WordCount += Increment;
    Buffer16 += Increment;

  }
  //
  // while
  //

  return CheckErrorStatus (IdeDev);
}


/**
  Enable SMART of the disk if supported

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure,used
  to record all the information of the IDE device.

**/
VOID
AtaSMARTSupport (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
{
  EFI_STATUS        Status;
  BOOLEAN           SMARTSupported;
  UINT8             Device;
  EFI_IDENTIFY_DATA *TmpAtaIdentifyPointer;
  UINT8             DeviceSelect;
  UINT8             LBAMid;
  UINT8             LBAHigh;

  //
  // Detect if the device supports S.M.A.R.T.
  //
  if ((IdeDev->pIdData->AtaData.command_set_supported_83 & 0xc000) != 0x4000) {
    //
    // Data in word 82 is not valid (bit15 shall be zero and bit14 shall be to one)
    //
    return ;
  } else {
    if ((IdeDev->pIdData->AtaData.command_set_supported_82 & 0x0001) != 0x0001) {
      //
      // S.M.A.R.T is not supported by the device
      //
      SMARTSupported = FALSE;
    } else {
      SMARTSupported = TRUE;
    }
  }

  if (!SMARTSupported) {
    //
    // Report nonsupport status code
    //
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_ATA_BUS_SMART_NOTSUPPORTED)
      );
  } else {
    //
    // Enable this feature
    //
    REPORT_STATUS_CODE (
      EFI_PROGRESS_CODE,
      (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_ATA_BUS_SMART_ENABLE)
      );

    Device = (UINT8) ((IdeDev->Device << 4) | 0xe0);
    Status = AtaNonDataCommandIn (
              IdeDev,
              ATA_SMART_CMD,
              Device,
              ATA_SMART_ENABLE_OPERATION,
              0,
              0,
              ATA_CONSTANT_4F,
              ATA_CONSTANT_C2
              );
    //
    // Detect if this feature is enabled
    //
    TmpAtaIdentifyPointer = (EFI_IDENTIFY_DATA *) AllocateZeroPool (sizeof (EFI_IDENTIFY_DATA));

    DeviceSelect          = (UINT8) ((IdeDev->Device) << 4);
    Status = AtaPioDataIn (
              IdeDev,
              (VOID *) TmpAtaIdentifyPointer,
              sizeof (EFI_IDENTIFY_DATA),
              IDENTIFY_DRIVE_CMD,
              DeviceSelect,
              0,
              0,
              0,
              0
              );
    if (EFI_ERROR (Status)) {
      gBS->FreePool (TmpAtaIdentifyPointer);
      return ;
    }

    //
    // Check if the feature is enabled
    //
    if ((TmpAtaIdentifyPointer->AtaData.command_set_feature_enb_85 & 0x0001) == 0x0001) {
      //
      // Read status data
      //
      AtaNonDataCommandIn (
        IdeDev,
        ATA_SMART_CMD,
        Device,
        ATA_SMART_RETURN_STATUS,
        0,
        0,
        ATA_CONSTANT_4F,
        ATA_CONSTANT_C2
        );
      LBAMid  = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb);
      LBAHigh = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb);

      if ((LBAMid == 0x4f) && (LBAHigh == 0xc2)) {
        //
        // The threshold exceeded condition is not detected by the device
        //
        REPORT_STATUS_CODE (
              EFI_PROGRESS_CODE,
              (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_ATA_BUS_SMART_UNDERTHRESHOLD)
              );

      } else if ((LBAMid == 0xf4) && (LBAHigh == 0x2c)) {
        //
        // The threshold exceeded condition is  detected by the device
        //
        REPORT_STATUS_CODE (
              EFI_PROGRESS_CODE,
              (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_ATA_BUS_SMART_OVERTHRESHOLD)
              );
      }

    } else {
      //
      // Report disabled status code
      //
      REPORT_STATUS_CODE (
            EFI_ERROR_CODE | EFI_ERROR_MINOR,
            (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_ATA_BUS_SMART_DISABLED)
            );
    }

    gBS->FreePool (TmpAtaIdentifyPointer);
  }

  return ;
}

/**
  Send ATA Ext command into device with NON_DATA protocol

  @param  IdeDev Standard IDE device private data structure
  @param  AtaCommand The ATA command to be sent
  @param  Device The value in Device register
  @param  Feature The value in Feature register
  @param  SectorCount The value in SectorCount register
  @param  LbaAddress The LBA address in 48-bit mode

  @retval  EFI_SUCCESS Reading succeed
  @retval  EFI_DEVICE_ERROR Error executing commands on this device

**/
EFI_STATUS
AtaCommandIssueExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT8           AtaCommand,
  IN  UINT8           Device,
  IN  UINT16          Feature,
  IN  UINT16          SectorCount,
  IN  EFI_LBA         LbaAddress
  )
{
  EFI_STATUS  Status;
  UINT8       SectorCount8;
  UINT8       Feature8;
  UINT8       LbaLow;
  UINT8       LbaMid;
  UINT8       LbaHigh;

  Status = WaitForBSYClear (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Select device (bit4), set LBA mode(bit6) (use 0xe0 for compatibility)
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((IdeDev->Device << 4) | 0xe0)
    );

  //
  // ATA commands for ATA device must be issued when DRDY is set
  //
  Status = DRDYReady (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Pass parameter into device register block
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Head, Device);

  //
  // Fill the feature register, which is a two-byte FIFO. Need write twice.
  //
  Feature8 = (UINT8) (Feature >> 8);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, Feature8);

  Feature8 = (UINT8) Feature;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, Feature8);

  //
  // Fill the sector count register, which is a two-byte FIFO. Need write twice.
  //
  SectorCount8 = (UINT8) (SectorCount >> 8);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorCount, SectorCount8);

  SectorCount8 = (UINT8) SectorCount;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorCount, SectorCount8);

  //
  // Fill the start LBA registers, which are also two-byte FIFO
  //
  LbaLow = (UINT8) RShiftU64 (LbaAddress, 24);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorNumber, LbaLow);
  LbaLow = (UINT8) LbaAddress;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorNumber, LbaLow);

  LbaMid = (UINT8) RShiftU64 (LbaAddress, 32);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb, LbaMid);
  LbaMid = (UINT8) RShiftU64 (LbaAddress, 8);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb, LbaMid);

  LbaHigh = (UINT8) RShiftU64 (LbaAddress, 40);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb, LbaHigh);
  LbaHigh = (UINT8) RShiftU64 (LbaAddress, 16);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb, LbaHigh);

  //
  // Work around for Segate 160G disk writing
  //
  gBS->Stall (1800);

  //
  // Send command via Command Register
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, AtaCommand);

  //
  // Stall at least 400ns
  //
  gBS->Stall (100);

  return EFI_SUCCESS;
}

/**
  Send ATA Ext command into device with NON_DATA protocol

  @param  IdeDev Standard IDE device private data structure
  @param  AtaCommand The ATA command to be sent
  @param  Device The value in Device register
  @param  Feature The value in Feature register
  @param  SectorCount The value in SectorCount register
  @param  LbaAddress The LBA address in 48-bit mode

  @retval  EFI_SUCCESS Reading succeed
  @retval  EFI_DEVICE_ERROR Error executing commands on this device

**/
EFI_STATUS
AtaCommandIssue (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT8           AtaCommand,
  IN  UINT8           Device,
  IN  UINT16          Feature,
  IN  UINT16          SectorCount,
  IN  EFI_LBA         LbaAddress
  )
{
  EFI_STATUS  Status;
  UINT8       SectorCount8;
  UINT8       Feature8;
  UINT8       Lba0;
  UINT8       Lba1;
  UINT8       Lba2;
  UINT8       Lba3;

  Status = WaitForBSYClear (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Select device (bit4), set LBA mode(bit6) (use 0xe0 for compatibility)
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((IdeDev->Device << 4) | 0xe0)
    );

  //
  // ATA commands for ATA device must be issued when DRDY is set
  //
  Status = DRDYReady (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Lba0  = (UINT8) LbaAddress;
  Lba1  = (UINT8) RShiftU64 (LbaAddress, 8);
  Lba2  = (UINT8) RShiftU64 (LbaAddress, 16);
  Lba3  = (UINT8) RShiftU64 (LbaAddress, 24);
  Device = (UINT8) (Device | Lba3);

  //
  // Pass parameter into device register block
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Head, Device);

  //
  // Fill the feature register, which is a two-byte FIFO. Need write twice.
  //
  Feature8 = (UINT8) Feature;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, Feature8);

  //
  // Fill the sector count register, which is a two-byte FIFO. Need write twice.
  //
  SectorCount8 = (UINT8) SectorCount;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorCount, SectorCount8);

  //
  // Fill the start LBA registers, which are also two-byte FIFO
  //

  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorNumber, Lba0);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb, Lba1);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb, Lba2);

  //
  // Send command via Command Register
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, AtaCommand);

  //
  // Stall at least 400ns
  //
  gBS->Stall (100);

  return EFI_SUCCESS;
}

/**
  This function is called by the AtaBlkIoReadBlocks() to perform
  reading from media in block unit. The function has been enhanced to
  support >120GB access and transfer at most 65536 blocks per command

  @param[in] *IdeDev pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @param[in] *DataBuffer A pointer to the destination buffer for the data.

  @param[in] StartLba The starting logical block address to read from
  on the device media.

  @param[in] NumberOfBlocks The number of transfer data blocks.

  @return The device status of UDMA operation. If the operation is
  successful, return EFI_SUCCESS.

  TODO:    EFI_UNSUPPORTED - add return value to function comment
  TODO:    EFI_DEVICE_ERROR - add return value to function comment
  TODO:    EFI_DEVICE_ERROR - add return value to function comment
  TODO:    EFI_DEVICE_ERROR - add return value to function comment
**/
EFI_STATUS
AtaUdmaReadExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
{
  return DoAtaUdma (IdeDev, DataBuffer, StartLba, NumberOfBlocks, AtaUdmaReadExtOp);
}

/**
  This function is called by the AtaBlkIoReadBlocks() to perform
  reading from media in block unit. The function has been enhanced to
  support >120GB access and transfer at most 65536 blocks per command

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @param[in] *DataBuffer    A pointer to the destination buffer for the data.
  @param[in] StartLba       The starting logical block address to read from
  on the device media.
  @param[in] NumberOfBlocks The number of transfer data blocks.

  @return The device status of UDMA operation. If the operation is
  successful, return EFI_SUCCESS.

  TODO:    EFI_UNSUPPORTED - add return value to function comment
  TODO:    EFI_DEVICE_ERROR - add return value to function comment
  TODO:    EFI_DEVICE_ERROR - add return value to function comment
  TODO:    EFI_DEVICE_ERROR - add return value to function comment
**/
EFI_STATUS
AtaUdmaRead (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
{
  return DoAtaUdma (IdeDev, DataBuffer, StartLba, NumberOfBlocks, AtaUdmaReadOp);
}

/**
  This function is called by the AtaBlkIoWriteBlocks() to perform
  writing to media in block unit. The function has been enhanced to
  support >120GB access and transfer at most 65536 blocks per command

  @param[in] *IdeDev pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @param[in] *DataBuffer A pointer to the source buffer for the data.

  @param[in] StartLba The starting logical block address to write to
  on the device media.

  @param[in] NumberOfBlocks The number of transfer data blocks.

  @return The device status of UDMA operation. If the operation is
  successful, return EFI_SUCCESS.

  TODO:    EFI_UNSUPPORTED - add return value to function comment
  TODO:    EFI_DEVICE_ERROR - add return value to function comment
  TODO:    EFI_DEVICE_ERROR - add return value to function comment
**/
EFI_STATUS
AtaUdmaWriteExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
{
  return DoAtaUdma (IdeDev, DataBuffer, StartLba, NumberOfBlocks, AtaUdmaWriteExtOp);
}

/**
  This function is called by the AtaBlkIoWriteBlocks() to perform
  writing to media in block unit. The function has been enhanced to
  support >120GB access and transfer at most 65536 blocks per command

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @param[in] *DataBuffer
  A pointer to the source buffer for the data.

  @param[in] StartLba
  The starting logical block address to write to
  on the device media.

  @param[in] NumberOfBlocks
  The number of transfer data blocks.

  @return The device status of UDMA operation. If the operation is
  successful, return EFI_SUCCESS.

  TODO:    EFI_UNSUPPORTED - add return value to function comment
  TODO:    EFI_DEVICE_ERROR - add return value to function comment
  TODO:    EFI_DEVICE_ERROR - add return value to function comment
**/
EFI_STATUS
AtaUdmaWrite (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
{
  return DoAtaUdma (IdeDev, DataBuffer, StartLba, NumberOfBlocks, AtaUdmaWriteOp);
}

/**
  Perform an ATA Udma operation (Read, ReadExt, Write, WriteExt).

  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @param[in] *DataBuffer
  A pointer to the source buffer for the data.

  @param[in] StartLba
  The starting logical block address to write to
  on the device media.

  @param[in] NumberOfBlocks
  The number of transfer data blocks.

  @param[in] UdmaOp
  The perform operations could be AtaUdmaReadOp, AtaUdmaReadExOp,
  AtaUdmaWriteOp, AtaUdmaWriteExOp

  @return The device status of UDMA operation. If the operation is
  successful, return EFI_SUCCESS.

**/
EFI_STATUS
DoAtaUdma (
  IN  IDE_BLK_IO_DEV      *IdeDev,
  IN  VOID                *DataBuffer,
  IN  EFI_LBA             StartLba,
  IN  UINTN               NumberOfBlocks,
  IN  ATA_UDMA_OPERATION  UdmaOp
  )
{
  IDE_DMA_PRD                   *PrdAddr;
  IDE_DMA_PRD                   *UsedPrdAddr;
  IDE_DMA_PRD                   *TempPrdAddr;
  UINT8                         RegisterValue;
  UINT8                         Device;
  UINT64                        IoPortForBmic;
  UINT64                        IoPortForBmis;
  UINT64                        IoPortForBmid;
  EFI_STATUS                    Status;
  UINTN                         PrdTableNum;
  UINTN                         ByteCount;
  UINTN                         ByteAvailable;
  UINT8                         *PrdBuffer;
  UINTN                         RemainBlockNum;
  UINT8                         DeviceControl;
  UINT32                        Count;
  UINTN                         PageCount;
  VOID                          *Map;
  VOID                          *MemPage;
  EFI_PHYSICAL_ADDRESS          DeviceAddress;
  UINTN                         MaxDmaCommandSectors;
  EFI_PCI_IO_PROTOCOL_OPERATION PciIoProtocolOp;
  UINT8                         AtaCommand;

  switch (UdmaOp) {
  case AtaUdmaReadOp:
    MaxDmaCommandSectors = MAX_DMA_COMMAND_SECTORS;
    PciIoProtocolOp      = EfiPciIoOperationBusMasterWrite;
    AtaCommand           = READ_DMA_CMD;
    break;
  case AtaUdmaReadExtOp:
    MaxDmaCommandSectors = MAX_DMA_EXT_COMMAND_SECTORS;
    PciIoProtocolOp      = EfiPciIoOperationBusMasterWrite;
    AtaCommand           = READ_DMA_EXT_CMD;
    break;
  case AtaUdmaWriteOp:
    MaxDmaCommandSectors = MAX_DMA_COMMAND_SECTORS;
    PciIoProtocolOp      = EfiPciIoOperationBusMasterRead;
    AtaCommand           = WRITE_DMA_CMD;
    break;
  case AtaUdmaWriteExtOp:
    MaxDmaCommandSectors = MAX_DMA_EXT_COMMAND_SECTORS;
    PciIoProtocolOp      = EfiPciIoOperationBusMasterRead;
    AtaCommand           = WRITE_DMA_EXT_CMD;
    break;
  default:
    return EFI_UNSUPPORTED;
    break;
  }

  //
  // Channel and device differential
  //
  Device = (UINT8) ((IdeDev->Device << 4) | 0xe0);

  //
  // Enable interrupt to support UDMA and Select device
  //
  DeviceControl = 0;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Alt.DeviceControl, DeviceControl);

  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Head, Device);

  if (IdePrimary == IdeDev->Channel) {
    IoPortForBmic = IdeDev->IoPort->BusMasterBaseAddr + BMICP_OFFSET;
    IoPortForBmis = IdeDev->IoPort->BusMasterBaseAddr + BMISP_OFFSET;
    IoPortForBmid = IdeDev->IoPort->BusMasterBaseAddr + BMIDP_OFFSET;
  } else {
    if (IdeSecondary == IdeDev->Channel) {
      IoPortForBmic = IdeDev->IoPort->BusMasterBaseAddr + BMICS_OFFSET;
      IoPortForBmis = IdeDev->IoPort->BusMasterBaseAddr + BMISS_OFFSET;
      IoPortForBmid = IdeDev->IoPort->BusMasterBaseAddr + BMIDS_OFFSET;
    } else {
      return EFI_UNSUPPORTED;
    }
  }

  RemainBlockNum = NumberOfBlocks;
  while (RemainBlockNum > 0) {

    if (RemainBlockNum >= MaxDmaCommandSectors) {
      //
      //  SectorCount is used to record the number of sectors to be read
      //  Max 65536 sectors can be transfered at a time.
      //
      NumberOfBlocks = MaxDmaCommandSectors;
      RemainBlockNum -= MaxDmaCommandSectors;
    } else {
      NumberOfBlocks  = (UINT16) RemainBlockNum;
      RemainBlockNum  = 0;
    }

    //
    // Calculate the number of PRD table to make sure the memory region
    // not cross 64K boundary
    //
    ByteCount   = NumberOfBlocks * IdeDev->BlkIo.Media->BlockSize;
    PrdTableNum = ((ByteCount >> 16) + 1) + 1;

    //
    // Build PRD table
    //
    PageCount = EFI_SIZE_TO_PAGES (2 * PrdTableNum * sizeof (IDE_DMA_PRD));
    Status = IdeDev->PciIo->AllocateBuffer (
                    IdeDev->PciIo,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    PageCount,
                    &MemPage,
                    0
                    );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
    ZeroMem ((VOID *) ((UINTN) MemPage), EFI_PAGES_TO_SIZE (PageCount));

    PrdAddr = (IDE_DMA_PRD *) ((UINTN) MemPage);
    //
    // To make sure PRD is allocated in one 64K page
    //
    if (((UINTN) PrdAddr & 0x0FFFF) > (((UINTN) PrdAddr + PrdTableNum * sizeof (IDE_DMA_PRD) - 1) & 0x0FFFF)) {
      UsedPrdAddr = (IDE_DMA_PRD *) ((UINTN) ((UINT8 *) PrdAddr + 0x10000) & 0xFFFF0000);
    } else {
      if ((UINTN) PrdAddr & 0x03) {
        UsedPrdAddr = (IDE_DMA_PRD *) ((UINTN) ((UINT8 *) PrdAddr + 0x04) & 0xFFFFFFFC);
      } else {
        UsedPrdAddr = PrdAddr;
      }
    }

    //
    // Build the PRD table
    //
    Status = IdeDev->PciIo->Map (
                       IdeDev->PciIo,
                       PciIoProtocolOp,
                       DataBuffer,
                       &ByteCount,
                       &DeviceAddress,
                       &Map
                       );
    if (EFI_ERROR (Status)) {
      IdeDev->PciIo->FreeBuffer (IdeDev->PciIo, PageCount, MemPage);
      return EFI_OUT_OF_RESOURCES;
    }
    PrdBuffer   = (VOID *) ((UINTN) DeviceAddress);
    TempPrdAddr = UsedPrdAddr;
    while (TRUE) {

      ByteAvailable = 0x10000 - ((UINTN) PrdBuffer & 0xFFFF);

      if (ByteCount <= ByteAvailable) {
        TempPrdAddr->RegionBaseAddr = (UINT32) ((UINTN) PrdBuffer);
        TempPrdAddr->ByteCount      = (UINT16) ByteCount;
        TempPrdAddr->EndOfTable     = 0x8000;
        break;
      }

      TempPrdAddr->RegionBaseAddr = (UINT32) ((UINTN) PrdBuffer);
      TempPrdAddr->ByteCount      = (UINT16) ByteAvailable;

      ByteCount -= ByteAvailable;
      PrdBuffer += ByteAvailable;
      TempPrdAddr++;
    }

    //
    // Set the base address to BMID register
    //
    IdeDev->PciIo->Io.Write (
                        IdeDev->PciIo,
                        EfiPciIoWidthUint32,
                        EFI_PCI_IO_PASS_THROUGH_BAR,
                        IoPortForBmid,
                        1,
                        &UsedPrdAddr
                        );

    //
    // Set BMIC register to identify the operation direction
    //
    IdeDev->PciIo->Io.Read (
                        IdeDev->PciIo,
                        EfiPciIoWidthUint8,
                        EFI_PCI_IO_PASS_THROUGH_BAR,
                        IoPortForBmic,
                        1,
                        &RegisterValue
                        );

    if (UdmaOp == AtaUdmaReadExtOp || UdmaOp == AtaUdmaReadOp) {
      RegisterValue |= BMIC_nREAD;
    } else {
      RegisterValue &= ~((UINT8) BMIC_nREAD);
    }

    IdeDev->PciIo->Io.Write (
                        IdeDev->PciIo,
                        EfiPciIoWidthUint8,
                        EFI_PCI_IO_PASS_THROUGH_BAR,
                        IoPortForBmic,
                        1,
                        &RegisterValue
                        );

    //
    // Read BMIS register and clear ERROR and INTR bit
    //
    IdeDev->PciIo->Io.Read (
                        IdeDev->PciIo,
                        EfiPciIoWidthUint8,
                        EFI_PCI_IO_PASS_THROUGH_BAR,
                        IoPortForBmis,
                        1,
                        &RegisterValue
                        );

    RegisterValue |= (BMIS_INTERRUPT | BMIS_ERROR);

    IdeDev->PciIo->Io.Write (
                        IdeDev->PciIo,
                        EfiPciIoWidthUint8,
                        EFI_PCI_IO_PASS_THROUGH_BAR,
                        IoPortForBmis,
                        1,
                        &RegisterValue
                        );

    if (UdmaOp == AtaUdmaWriteExtOp || UdmaOp == AtaUdmaReadExtOp) {
      Status = AtaCommandIssueExt (
                 IdeDev,
                 AtaCommand,
                 Device,
                 0,
                 (UINT16) NumberOfBlocks,
                 StartLba
                 );
    } else {
      Status = AtaCommandIssue (
                 IdeDev,
                 AtaCommand,
                 Device,
                 0,
                 (UINT16) NumberOfBlocks,
                 StartLba
                 );
    }

    if (EFI_ERROR (Status)) {
      IdeDev->PciIo->FreeBuffer (IdeDev->PciIo, PageCount, MemPage);
      IdeDev->PciIo->Unmap (IdeDev->PciIo, Map);
      return EFI_DEVICE_ERROR;
    }

    //
    // Set START bit of BMIC register
    //
    IdeDev->PciIo->Io.Read (
                        IdeDev->PciIo,
                        EfiPciIoWidthUint8,
                        EFI_PCI_IO_PASS_THROUGH_BAR,
                        IoPortForBmic,
                        1,
                        &RegisterValue
                        );

    RegisterValue |= BMIC_START;

    IdeDev->PciIo->Io.Write (
                        IdeDev->PciIo,
                        EfiPciIoWidthUint8,
                        EFI_PCI_IO_PASS_THROUGH_BAR,
                        IoPortForBmic,
                        1,
                        &RegisterValue
                        );

    //
    // Check the INTERRUPT and ERROR bit of BMIS
    // Max transfer number of sectors for one command is 65536(32Mbyte),
    // it will cost 1 second to transfer these data in UDMA mode 2(33.3MBps).
    // So set the variable Count to 2000, for about 2 second timeout time.
    //
    Count = 2000;
    while (TRUE) {

      IdeDev->PciIo->Io.Read (
                          IdeDev->PciIo,
                          EfiPciIoWidthUint8,
                          EFI_PCI_IO_PASS_THROUGH_BAR,
                          IoPortForBmis,
                          1,
                          &RegisterValue
                          );
      if ((RegisterValue & (BMIS_INTERRUPT | BMIS_ERROR)) || (Count == 0)) {
        if ((RegisterValue & BMIS_ERROR) || (Count == 0)) {
          //
          // Clear START bit of BMIC register before return EFI_DEVICE_ERROR
          //
          IdeDev->PciIo->Io.Read (
                              IdeDev->PciIo,
                              EfiPciIoWidthUint8,
                              EFI_PCI_IO_PASS_THROUGH_BAR,
                              IoPortForBmic,
                              1,
                              &RegisterValue
                              );

          RegisterValue &= ~((UINT8)BMIC_START);

          IdeDev->PciIo->Io.Write (
                              IdeDev->PciIo,
                              EfiPciIoWidthUint8,
                              EFI_PCI_IO_PASS_THROUGH_BAR,
                              IoPortForBmic,
                              1,
                              &RegisterValue
                              );
          IdeDev->PciIo->FreeBuffer (IdeDev->PciIo, PageCount, MemPage);
          IdeDev->PciIo->Unmap (IdeDev->PciIo, Map);
          return EFI_DEVICE_ERROR;
        }
        break;
      }

      gBS->Stall (1000);
      Count --;
    }

    IdeDev->PciIo->FreeBuffer (IdeDev->PciIo, PageCount, MemPage);
    IdeDev->PciIo->Unmap (IdeDev->PciIo, Map);
    //
    // Read Status Register of IDE device to clear interrupt
    //
    RegisterValue = IDEReadPortB(IdeDev->PciIo,IdeDev->IoPort->Reg.Status);
    //
    // Clear START bit of BMIC register
    //
    IdeDev->PciIo->Io.Read (
                        IdeDev->PciIo,
                        EfiPciIoWidthUint8,
                        EFI_PCI_IO_PASS_THROUGH_BAR,
                        IoPortForBmic,
                        1,
                        &RegisterValue
                        );

    RegisterValue &= ~((UINT8) BMIC_START);

    IdeDev->PciIo->Io.Write (
                        IdeDev->PciIo,
                        EfiPciIoWidthUint8,
                        EFI_PCI_IO_PASS_THROUGH_BAR,
                        IoPortForBmic,
                        1,
                        &RegisterValue
                        );

    if (RegisterValue & BMIS_ERROR) {
      return EFI_DEVICE_ERROR;
    }

    DataBuffer = (UINT8 *) DataBuffer + NumberOfBlocks * IdeDev->BlkIo.Media->BlockSize;
    StartLba += NumberOfBlocks;
  }

  //
  // Disable interrupt of Select device
  //
  IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Alt.DeviceControl);
  DeviceControl |= IEN_L;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Alt.DeviceControl, DeviceControl);

  return EFI_SUCCESS;
}

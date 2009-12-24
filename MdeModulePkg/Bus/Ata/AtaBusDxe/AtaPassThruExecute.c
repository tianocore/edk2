/** @file
  This file implements ATA pass through transaction for ATA bus driver.

  This file implements the low level execution of ATA pass through transaction.
  It transforms the high level identity, read/write, reset command to ATA pass
  through command and protocol. 
    
  Copyright (c) 2009 Intel Corporation. <BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "AtaBus.h"

//
// Look up table (UdmaValid, IsWrite) for EFI_ATA_PASS_THRU_CMD_PROTOCOL
//
EFI_ATA_PASS_THRU_CMD_PROTOCOL mAtaPassThruCmdProtocols[][2] = {
  {
    EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_IN,
    EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT
  },
  {
    EFI_ATA_PASS_THRU_PROTOCOL_UDMA_DATA_IN,
    EFI_ATA_PASS_THRU_PROTOCOL_UDMA_DATA_OUT,
  }
};

//
// Look up table (UdmaValid, Lba48Bit, IsIsWrite) for ATA_CMD
//
UINT8 mAtaCommands[][2][2] = {
  {
    {
      ATA_CMD_READ_SECTORS,            // 28-bit LBA; PIO read
      ATA_CMD_WRITE_SECTORS            // 28-bit LBA; PIO write
    },
    {
      ATA_CMD_READ_SECTORS_EXT,        // 48-bit LBA; PIO read
      ATA_CMD_WRITE_SECTORS_EXT        // 48-bit LBA; PIO write
    }
  },
  {
    {
      ATA_CMD_READ_DMA,                // 28-bit LBA; DMA read
      ATA_CMD_WRITE_DMA                // 28-bit LBA; DMA write
    },
    {
      ATA_CMD_READ_DMA_EXT,            // 48-bit LBA; DMA read
      ATA_CMD_WRITE_DMA_EXT            // 48-bit LBA; DMA write
    }
  }
};

//
// Look up table (Lba48Bit) for maximum transfer block number
//
UINTN mMaxTransferBlockNumber[] = {
  MAX_28BIT_TRANSFER_BLOCK_NUM,
  MAX_48BIT_TRANSFER_BLOCK_NUM
};


/**
  Wrapper for EFI_ATA_PASS_THRU_PROTOCOL.PassThru().

  This function wraps the PassThru() invocation for ATA pass through function
  for an ATA device. It assembles the ATA pass through command packet for ATA
  transaction.

  @param  AtaDevice         The ATA child device involved for the operation.

  @return The return status from EFI_ATA_PASS_THRU_PROTOCOL.PassThru().

**/
EFI_STATUS
AtaDevicePassThru (
  IN OUT ATA_DEVICE                       *AtaDevice
  )
{
  EFI_STATUS                              Status;
  EFI_ATA_PASS_THRU_PROTOCOL              *AtaPassThru;
  EFI_ATA_PASS_THRU_COMMAND_PACKET        *Packet;

  //
  // Assemble packet
  //
  Packet = &AtaDevice->Packet;
  Packet->Asb = AtaDevice->Asb;
  Packet->Acb = &AtaDevice->Acb;
  Packet->Timeout = ATA_TIMEOUT;

  AtaPassThru = AtaDevice->AtaBusDriverData->AtaPassThru;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          AtaDevice->Port,
                          AtaDevice->PortMultiplierPort,
                          Packet,
                          NULL
                          );
  //
  // Ensure ATA pass through caller and callee have the same
  // interpretation of ATA pass through protocol. 
  //
  ASSERT (Status != EFI_INVALID_PARAMETER);
  ASSERT (Status != EFI_BAD_BUFFER_SIZE);

  return Status;
}


/**
  Wrapper for EFI_ATA_PASS_THRU_PROTOCOL.ResetDevice().

  This function wraps the ResetDevice() invocation for ATA pass through function
  for an ATA device. 

  @param  AtaDevice         The ATA child device involved for the operation.

  @return The return status from EFI_ATA_PASS_THRU_PROTOCOL.PassThru().

**/
EFI_STATUS
ResetAtaDevice (
  IN ATA_DEVICE                           *AtaDevice
  )
{
  EFI_ATA_PASS_THRU_PROTOCOL              *AtaPassThru;
  
  AtaPassThru = AtaDevice->AtaBusDriverData->AtaPassThru;
  
  return AtaPassThru->ResetDevice (
                        AtaPassThru,
                        AtaDevice->Port,
                        AtaDevice->PortMultiplierPort
                        );
}


/**
  Prints ATA model name to ATA device structure.

  This function converts ATA device model name from ATA identify data 
  to a string in ATA device structure. It needs to change the character
  order in the original model name string.

  @param  AtaDevice         The ATA child device involved for the operation.

**/
VOID
PrintAtaModelName (
  IN OUT ATA_DEVICE  *AtaDevice
  )
{
  UINTN   Index;
  CHAR8   *Source;
  CHAR16  *Destination;

  Source = AtaDevice->IdentifyData->AtaData.ModelName;
  Destination = AtaDevice->ModelName;

  //
  // Swap the byte order in the original module name.
  //
  for (Index = 0; Index < MAX_MODEL_NAME_LEN; Index += 2) {
    Destination[Index]      = Source[Index + 1];
    Destination[Index + 1]  = Source[Index];
  }
  AtaDevice->ModelName[MAX_MODEL_NAME_LEN] = L'\0';
}


/**
  Gets ATA device Capacity according to ATA 6.

  This function returns the capacity of the ATA device if it follows
  ATA 6 to support 48 bit addressing.

  @param  AtaDevice         The ATA child device involved for the operation.

  @return The capacity of the ATA device or 0 if the device does not support
          48-bit addressing defined in ATA 6.

**/
EFI_LBA
GetAtapi6Capacity (
  IN ATA_DEVICE                 *AtaDevice
  )
{
  EFI_LBA                       Capacity;
  EFI_LBA                       TmpLba;
  UINTN                         Index;
  ATAPI_IDENTIFY_DATA           *IdentifyData;

  IdentifyData = (ATAPI_IDENTIFY_DATA *) AtaDevice->IdentifyData;
  if ((IdentifyData->cmd_set_support_83 & BIT10) == 0) {
    //
    // The device doesn't support 48 bit addressing
    //
    return 0;
  }

  //
  // 48 bit address feature set is supported, get maximum capacity
  //
  Capacity = 0;
  for (Index = 0; Index < 4; Index++) {
    //
    // Lower byte goes first: word[100] is the lowest word, word[103] is highest
    //
    TmpLba = IdentifyData->max_user_lba_for_48bit_addr[Index];
    Capacity |= LShiftU64 (TmpLba, 16 * Index);
  }

  return Capacity;
}


/**
  Identifies ATA device via the Identify data.

  This function identifies the ATA device and initializes the Media information in 
  Block IO protocol interface.

  @param  AtaDevice         The ATA child device involved for the operation.

  @retval EFI_UNSUPPORTED   The device is not a valid ATA device (hard disk).
  @retval EFI_SUCCESS       The device is successfully identified and Media information
                            is correctly initialized.

**/
EFI_STATUS
IdentifyAtaDevice (
  IN OUT ATA_DEVICE                 *AtaDevice
  )
{
  EFI_ATA_IDENTIFY_DATA             *IdentifyData;
  EFI_BLOCK_IO_MEDIA                *BlockMedia;
  EFI_LBA                           Capacity;
  UINT16                            PhyLogicSectorSupport;
  UINT16                            UdmaMode;

  IdentifyData = &AtaDevice->IdentifyData->AtaData;

  if ((IdentifyData->config & BIT15) != 0) {
    //
    // This is not an hard disk
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Check whether the WORD 88 (supported UltraDMA by drive) is valid
  //
  if ((IdentifyData->field_validity & BIT2) != 0) {
    UdmaMode = IdentifyData->ultra_dma_mode;
    if ((UdmaMode & (BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6)) != 0) {
      //
      // If BIT0~BIT6 is selected, then UDMA is supported
      //
      AtaDevice->UdmaValid = TRUE;
    }
  }

  Capacity = GetAtapi6Capacity (AtaDevice);
  if (Capacity > MAX_28BIT_ADDRESSING_CAPACITY) {
    //
    // Capacity exceeds 120GB. 48-bit addressing is really needed
    //
    AtaDevice->Lba48Bit = TRUE;
  } else {
    //
    // This is a hard disk <= 120GB capacity, treat it as normal hard disk
    //
    Capacity = ((UINT32)IdentifyData->user_addressable_sectors_hi << 16) | IdentifyData->user_addressable_sectors_lo;
    AtaDevice->Lba48Bit = FALSE;
  }

  //
  // Block Media Information:
  //
  BlockMedia = &AtaDevice->BlockMedia;
  BlockMedia->LastBlock = Capacity - 1;
  //
  // Check whether Long Physical Sector Feature is supported
  //
  PhyLogicSectorSupport = IdentifyData->phy_logic_sector_support;
  if ((PhyLogicSectorSupport & (BIT14 | BIT15)) == BIT14) {
    //
    // Check whether one physical block contains multiple physical blocks
    //
    if ((PhyLogicSectorSupport & BIT13) != 0) {
      BlockMedia->LogicalBlocksPerPhysicalBlock = (UINT32) (1 << (PhyLogicSectorSupport & 0x000f));
      //
      // Check lowest alignment of logical blocks within physical block
      //
      if ((IdentifyData->alignment_logic_in_phy_blocks & (BIT14 | BIT15)) == BIT14) {
        BlockMedia->LowestAlignedLba = (EFI_LBA) (IdentifyData->alignment_logic_in_phy_blocks & 0x3fff);
      }
    }
    //
    // Check logical block size
    //
    if ((PhyLogicSectorSupport & BIT12) != 0) {
      BlockMedia->BlockSize = (UINT32) (((IdentifyData->logic_sector_size_hi << 16) | IdentifyData->logic_sector_size_lo) * sizeof (UINT16));
    }
    AtaDevice->BlockIo.Revision = EFI_BLOCK_IO_PROTOCOL_REVISION2;
  }
  //
  // Get ATA model name from identify data structure. 
  //
  PrintAtaModelName (AtaDevice); 

  return EFI_SUCCESS;
}


/**
  Discovers whether it is a valid ATA device.

  This function issues ATA_CMD_IDENTIFY_DRIVE command to the ATA device to identify it.
  If the command is executed successfully, it then identifies it and initializes
  the Media information in Block IO protocol interface.

  @param  AtaDevice         The ATA child device involved for the operation.

  @retval EFI_SUCCESS       The device is successfully identified and Media information
                            is correctly initialized.
  @return others            Some error occurs when discovering the ATA device. 

**/
EFI_STATUS
DiscoverAtaDevice (
  IN OUT ATA_DEVICE                 *AtaDevice
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             *Acb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  *Packet;
  UINTN                             Retry;

  //
  // Prepare for ATA command block.
  //
  Acb = ZeroMem (&AtaDevice->Acb, sizeof (*Acb));
  Acb->AtaCommand = ATA_CMD_IDENTIFY_DRIVE;

  //
  // Prepare for ATA pass through packet.
  //
  Packet = ZeroMem (&AtaDevice->Packet, sizeof (*Packet));
  Packet->InDataBuffer = AtaDevice->IdentifyData;
  Packet->InTransferLength = sizeof (*AtaDevice->IdentifyData);
  Packet->Protocol = EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_IN;
  Packet->Length = EFI_ATA_PASS_THRU_LENGTH_BYTES | EFI_ATA_PASS_THRU_LENGTH_SECTOR_COUNT;

  Retry = MAX_RETRY_TIMES;
  do {
    Status = AtaDevicePassThru (AtaDevice);
    if (!EFI_ERROR (Status)) {
      //
      // The command is issued successfully
      //
      Status = IdentifyAtaDevice (AtaDevice);
      if (!EFI_ERROR (Status)) {
        return Status;
      }
    }
  } while (Retry-- > 0);

  return Status;
}

/**
  Transfer data from ATA device.

  This function performs one ATA pass through transaction to transfer data from/to
  ATA device. It chooses the appropriate ATA command and protocol to invoke PassThru
  interface of ATA pass through.

  @param  AtaDevice         The ATA child device involved for the operation.
  @param  Buffer            The pointer to the current transaction buffer.
  @param  StartLba          The starting logical block address to be accessed.
  @param  TransferLength    The block number or sector count of the transfer.
  @param  IsWrite           Indicates whether it is a write operation.

  @retval EFI_SUCCESS       The data transfer is complete successfully.
  @return others            Some error occurs when transferring data. 

**/
EFI_STATUS
TransferAtaDevice (
  IN OUT ATA_DEVICE                 *AtaDevice,
  IN OUT VOID                       *Buffer,
  IN EFI_LBA                        StartLba,
  IN UINT32                         TransferLength,
  IN BOOLEAN                        IsWrite
  )
{
  EFI_ATA_COMMAND_BLOCK             *Acb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  *Packet;

  //
  // Ensure AtaDevice->Lba48Bit and IsWrite are valid boolean values 
  //
  ASSERT ((UINTN) AtaDevice->Lba48Bit < 2);
  ASSERT ((UINTN) IsWrite < 2);
  //
  // Prepare for ATA command block.
  //
  Acb = ZeroMem (&AtaDevice->Acb, sizeof (*Acb));
  Acb->AtaCommand = mAtaCommands[AtaDevice->UdmaValid][AtaDevice->Lba48Bit][IsWrite];
  Acb->AtaSectorNumber = (UINT8) StartLba;
  Acb->AtaCylinderLow = (UINT8) RShiftU64 (StartLba, 8);
  Acb->AtaCylinderHigh = (UINT8) RShiftU64 (StartLba, 16);
  Acb->AtaDeviceHead = (UINT8) (BIT7 | BIT6 | BIT5 | (AtaDevice->PortMultiplierPort << 4)); 
  Acb->AtaSectorCount = (UINT8) TransferLength;
  if (AtaDevice->Lba48Bit) {
    Acb->AtaSectorNumberExp = (UINT8) RShiftU64 (StartLba, 24);
    Acb->AtaCylinderLowExp = (UINT8) RShiftU64 (StartLba, 32);
    Acb->AtaCylinderHighExp = (UINT8) RShiftU64 (StartLba, 40);
    Acb->AtaSectorCountExp = (UINT8) (TransferLength >> 8);
  } else {
    Acb->AtaDeviceHead = (UINT8) (Acb->AtaDeviceHead | RShiftU64 (StartLba, 24));
  }

  //
  // Prepare for ATA pass through packet.
  //
  Packet = ZeroMem (&AtaDevice->Packet, sizeof (*Packet));
  if (IsWrite) {
    Packet->OutDataBuffer = Buffer;
    Packet->OutTransferLength = TransferLength;
  } else {
    Packet->InDataBuffer = Buffer;
    Packet->InTransferLength = TransferLength;
  }
  Packet->Protocol = mAtaPassThruCmdProtocols[AtaDevice->UdmaValid][IsWrite];
  Packet->Length = EFI_ATA_PASS_THRU_LENGTH_SECTOR_COUNT;

  return AtaDevicePassThru (AtaDevice); 
}

/**
  Read or write a number of blocks from ATA device.

  This function performs ATA pass through transactions to read/write data from/to
  ATA device. It may separate the read/write request into several ATA pass through
  transactions.

  @param  AtaDevice         The ATA child device involved for the operation.
  @param  Buffer            The pointer to the current transaction buffer.
  @param  StartLba          The starting logical block address to be accessed.
  @param  NumberOfBlocks    The block number or sector count of the transfer.
  @param  IsWrite           Indicates whether it is a write operation.

  @retval EFI_SUCCESS       The data transfer is complete successfully.
  @return others            Some error occurs when transferring data. 

**/
EFI_STATUS 
AccessAtaDevice(
  IN OUT ATA_DEVICE                 *AtaDevice,
  IN OUT UINT8                      *Buffer,
  IN EFI_LBA                        StartLba,
  IN UINTN                          NumberOfBlocks,
  IN BOOLEAN                        IsWrite
  )
{
  EFI_STATUS                        Status;
  UINTN                             MaxTransferBlockNumber;
  UINTN                             TransferBlockNumber;
  UINTN                             BlockSize;
 
  //
  // Ensure AtaDevice->Lba48Bit is a valid boolean value 
  //
  ASSERT ((UINTN) AtaDevice->Lba48Bit < 2);
  MaxTransferBlockNumber = mMaxTransferBlockNumber[AtaDevice->Lba48Bit];
  BlockSize = AtaDevice->BlockMedia.BlockSize;
  do {
    if (NumberOfBlocks > MaxTransferBlockNumber) {
      TransferBlockNumber = MaxTransferBlockNumber;
      NumberOfBlocks -= MaxTransferBlockNumber;
    } else  {
      TransferBlockNumber = NumberOfBlocks;
      NumberOfBlocks  = 0;
    }

    Status = TransferAtaDevice (AtaDevice, Buffer, StartLba, (UINT32) TransferBlockNumber, IsWrite);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    StartLba += TransferBlockNumber;
    Buffer   += TransferBlockNumber * BlockSize;
  } while (NumberOfBlocks > 0);

  return Status;
}

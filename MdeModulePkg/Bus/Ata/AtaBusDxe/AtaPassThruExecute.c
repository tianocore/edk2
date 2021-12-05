/** @file
  This file implements ATA pass through transaction for ATA bus driver.

  This file implements the low level execution of ATA pass through transaction.
  It transforms the high level identity, read/write, reset command to ATA pass
  through command and protocol.

  NOTE: This file also implements the StorageSecurityCommandProtocol(SSP). For input
  parameter SecurityProtocolSpecificData, ATA spec has no explicitly definition
  for Security Protocol Specific layout. This implementation uses big endian for
  Cylinder register.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "AtaBus.h"

#define ATA_CMD_TRUST_NON_DATA     0x5B
#define ATA_CMD_TRUST_RECEIVE      0x5C
#define ATA_CMD_TRUST_RECEIVE_DMA  0x5D
#define ATA_CMD_TRUST_SEND         0x5E
#define ATA_CMD_TRUST_SEND_DMA     0x5F

//
// Look up table (UdmaValid, IsWrite) for EFI_ATA_PASS_THRU_CMD_PROTOCOL
//
EFI_ATA_PASS_THRU_CMD_PROTOCOL  mAtaPassThruCmdProtocols[][2] = {
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
UINT8  mAtaCommands[][2][2] = {
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
// Look up table (UdmaValid, IsTrustSend) for ATA_CMD
//
UINT8  mAtaTrustCommands[2][2] = {
  {
    ATA_CMD_TRUST_RECEIVE,            // PIO read
    ATA_CMD_TRUST_SEND                // PIO write
  },
  {
    ATA_CMD_TRUST_RECEIVE_DMA,        // DMA read
    ATA_CMD_TRUST_SEND_DMA            // DMA write
  }
};

//
// Look up table (Lba48Bit) for maximum transfer block number
//
UINTN  mMaxTransferBlockNumber[] = {
  MAX_28BIT_TRANSFER_BLOCK_NUM,
  MAX_48BIT_TRANSFER_BLOCK_NUM
};

/**
  Wrapper for EFI_ATA_PASS_THRU_PROTOCOL.PassThru().

  This function wraps the PassThru() invocation for ATA pass through function
  for an ATA device. It assembles the ATA pass through command packet for ATA
  transaction.

  @param[in, out]  AtaDevice   The ATA child device involved for the operation.
  @param[in, out]  TaskPacket  Pointer to a Pass Thru Command Packet. Optional,
                               if it is NULL, blocking mode, and use the packet
                               in AtaDevice. If it is not NULL, non blocking mode,
                               and pass down this Packet.
  @param[in, out]  Event       If Event is NULL, then blocking I/O is performed.
                               If Event is not NULL and non-blocking I/O is
                               supported,then non-blocking I/O is performed,
                               and Event will be signaled when the write
                               request is completed.

  @return The return status from EFI_ATA_PASS_THRU_PROTOCOL.PassThru().

**/
EFI_STATUS
AtaDevicePassThru (
  IN OUT ATA_DEVICE                        *AtaDevice,
  IN OUT EFI_ATA_PASS_THRU_COMMAND_PACKET  *TaskPacket  OPTIONAL,
  IN OUT EFI_EVENT                         Event OPTIONAL
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_PASS_THRU_PROTOCOL        *AtaPassThru;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  *Packet;

  //
  // Assemble packet. If it is non blocking mode, the Ata driver should keep each
  // subtask and clean them when the event is signaled.
  //
  if (TaskPacket != NULL) {
    Packet      = TaskPacket;
    Packet->Asb = AllocateAlignedBuffer (AtaDevice, sizeof (EFI_ATA_STATUS_BLOCK));
    if (Packet->Asb == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Packet->Asb, AtaDevice->Asb, sizeof (EFI_ATA_STATUS_BLOCK));
    Packet->Acb = AllocateCopyPool (sizeof (EFI_ATA_COMMAND_BLOCK), &AtaDevice->Acb);
  } else {
    Packet      = &AtaDevice->Packet;
    Packet->Asb = AtaDevice->Asb;
    Packet->Acb = &AtaDevice->Acb;
  }

  AtaPassThru = AtaDevice->AtaBusDriverData->AtaPassThru;

  Status = AtaPassThru->PassThru (
                          AtaPassThru,
                          AtaDevice->Port,
                          AtaDevice->PortMultiplierPort,
                          Packet,
                          Event
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
  IN ATA_DEVICE  *AtaDevice
  )
{
  EFI_ATA_PASS_THRU_PROTOCOL  *AtaPassThru;

  AtaPassThru = AtaDevice->AtaBusDriverData->AtaPassThru;

  //
  // Report Status Code to indicate reset happens
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_RESET),
    AtaDevice->AtaBusDriverData->ParentDevicePath
    );

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

  Source      = AtaDevice->IdentifyData->ModelName;
  Destination = AtaDevice->ModelName;

  //
  // Swap the byte order in the original module name.
  //
  for (Index = 0; Index < MAX_MODEL_NAME_LEN; Index += 2) {
    Destination[Index]     = Source[Index + 1];
    Destination[Index + 1] = Source[Index];
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
  IN ATA_DEVICE  *AtaDevice
  )
{
  EFI_LBA            Capacity;
  EFI_LBA            TmpLba;
  UINTN              Index;
  ATA_IDENTIFY_DATA  *IdentifyData;

  IdentifyData = AtaDevice->IdentifyData;
  if ((IdentifyData->command_set_supported_83 & BIT10) == 0) {
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
    TmpLba    = IdentifyData->maximum_lba_for_48bit_addressing[Index];
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
  IN OUT ATA_DEVICE  *AtaDevice
  )
{
  ATA_IDENTIFY_DATA   *IdentifyData;
  EFI_BLOCK_IO_MEDIA  *BlockMedia;
  EFI_LBA             Capacity;
  UINT16              PhyLogicSectorSupport;
  UINT16              UdmaMode;

  IdentifyData = AtaDevice->IdentifyData;

  if ((IdentifyData->config & BIT15) != 0) {
    //
    // This is not an hard disk
    //
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "AtaBus - Identify Device: Port %x PortMultiplierPort %x\n", AtaDevice->Port, AtaDevice->PortMultiplierPort));

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
    Capacity            = ((UINT32)IdentifyData->user_addressable_sectors_hi << 16) | IdentifyData->user_addressable_sectors_lo;
    AtaDevice->Lba48Bit = FALSE;
  }

  //
  // Block Media Information:
  //
  BlockMedia            = &AtaDevice->BlockMedia;
  BlockMedia->LastBlock = Capacity - 1;
  BlockMedia->IoAlign   = AtaDevice->AtaBusDriverData->AtaPassThru->Mode->IoAlign;
  //
  // Check whether Long Physical Sector Feature is supported
  //
  PhyLogicSectorSupport = IdentifyData->phy_logic_sector_support;
  if ((PhyLogicSectorSupport & (BIT14 | BIT15)) == BIT14) {
    //
    // Check whether one physical block contains multiple physical blocks
    //
    if ((PhyLogicSectorSupport & BIT13) != 0) {
      BlockMedia->LogicalBlocksPerPhysicalBlock = (UINT32)(1 << (PhyLogicSectorSupport & 0x000f));
      //
      // Check lowest alignment of logical blocks within physical block
      //
      if ((IdentifyData->alignment_logic_in_phy_blocks & (BIT14 | BIT15)) == BIT14) {
        BlockMedia->LowestAlignedLba = (EFI_LBA)((BlockMedia->LogicalBlocksPerPhysicalBlock - ((UINT32)IdentifyData->alignment_logic_in_phy_blocks & 0x3fff)) %
                                                 BlockMedia->LogicalBlocksPerPhysicalBlock);
      }
    }

    //
    // Check logical block size
    //
    if ((PhyLogicSectorSupport & BIT12) != 0) {
      BlockMedia->BlockSize = (UINT32)(((IdentifyData->logic_sector_size_hi << 16) | IdentifyData->logic_sector_size_lo) * sizeof (UINT16));
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
  IN OUT ATA_DEVICE  *AtaDevice
  )
{
  EFI_STATUS                        Status;
  EFI_ATA_COMMAND_BLOCK             *Acb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  *Packet;
  UINTN                             Retry;

  //
  // Prepare for ATA command block.
  //
  Acb                = ZeroMem (&AtaDevice->Acb, sizeof (EFI_ATA_COMMAND_BLOCK));
  Acb->AtaCommand    = ATA_CMD_IDENTIFY_DRIVE;
  Acb->AtaDeviceHead = (UINT8)(BIT7 | BIT6 | BIT5 | (AtaDevice->PortMultiplierPort == 0xFFFF ? 0 : (AtaDevice->PortMultiplierPort << 4)));

  //
  // Prepare for ATA pass through packet.
  //
  Packet                   = ZeroMem (&AtaDevice->Packet, sizeof (EFI_ATA_PASS_THRU_COMMAND_PACKET));
  Packet->InDataBuffer     = AtaDevice->IdentifyData;
  Packet->InTransferLength = sizeof (ATA_IDENTIFY_DATA);
  Packet->Protocol         = EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_IN;
  Packet->Length           = EFI_ATA_PASS_THRU_LENGTH_BYTES | EFI_ATA_PASS_THRU_LENGTH_SECTOR_COUNT;
  Packet->Timeout          = ATA_TIMEOUT;

  Retry = MAX_RETRY_TIMES;
  do {
    Status = AtaDevicePassThru (AtaDevice, NULL, NULL);
    if (!EFI_ERROR (Status)) {
      //
      // The command is issued successfully
      //
      Status = IdentifyAtaDevice (AtaDevice);
      return Status;
    }
  } while (Retry-- > 0);

  return Status;
}

/**
  Transfer data from ATA device.

  This function performs one ATA pass through transaction to transfer data from/to
  ATA device. It chooses the appropriate ATA command and protocol to invoke PassThru
  interface of ATA pass through.

  @param[in, out]  AtaDevice       The ATA child device involved for the operation.
  @param[in, out]  TaskPacket      Pointer to a Pass Thru Command Packet. Optional,
                                   if it is NULL, blocking mode, and use the packet
                                   in AtaDevice. If it is not NULL, non blocking mode,
                                   and pass down this Packet.
  @param[in, out]  Buffer          The pointer to the current transaction buffer.
  @param[in]       StartLba        The starting logical block address to be accessed.
  @param[in]       TransferLength  The block number or sector count of the transfer.
  @param[in]       IsWrite         Indicates whether it is a write operation.
  @param[in]       Event           If Event is NULL, then blocking I/O is performed.
                                   If Event is not NULL and non-blocking I/O is
                                   supported,then non-blocking I/O is performed,
                                   and Event will be signaled when the write
                                   request is completed.

  @retval EFI_SUCCESS       The data transfer is complete successfully.
  @return others            Some error occurs when transferring data.

**/
EFI_STATUS
TransferAtaDevice (
  IN OUT ATA_DEVICE                        *AtaDevice,
  IN OUT EFI_ATA_PASS_THRU_COMMAND_PACKET  *TaskPacket  OPTIONAL,
  IN OUT VOID                              *Buffer,
  IN EFI_LBA                               StartLba,
  IN UINT32                                TransferLength,
  IN BOOLEAN                               IsWrite,
  IN EFI_EVENT                             Event OPTIONAL
  )
{
  EFI_ATA_COMMAND_BLOCK             *Acb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  *Packet;

  //
  // Ensure AtaDevice->UdmaValid, AtaDevice->Lba48Bit and IsWrite are valid boolean values
  //
  ASSERT ((UINTN)AtaDevice->UdmaValid < 2);
  ASSERT ((UINTN)AtaDevice->Lba48Bit < 2);
  ASSERT ((UINTN)IsWrite < 2);
  //
  // Prepare for ATA command block.
  //
  Acb                  = ZeroMem (&AtaDevice->Acb, sizeof (EFI_ATA_COMMAND_BLOCK));
  Acb->AtaCommand      = mAtaCommands[AtaDevice->UdmaValid][AtaDevice->Lba48Bit][IsWrite];
  Acb->AtaSectorNumber = (UINT8)StartLba;
  Acb->AtaCylinderLow  = (UINT8)RShiftU64 (StartLba, 8);
  Acb->AtaCylinderHigh = (UINT8)RShiftU64 (StartLba, 16);
  Acb->AtaDeviceHead   = (UINT8)(BIT7 | BIT6 | BIT5 | (AtaDevice->PortMultiplierPort == 0xFFFF ? 0 : (AtaDevice->PortMultiplierPort << 4)));
  Acb->AtaSectorCount  = (UINT8)TransferLength;
  if (AtaDevice->Lba48Bit) {
    Acb->AtaSectorNumberExp = (UINT8)RShiftU64 (StartLba, 24);
    Acb->AtaCylinderLowExp  = (UINT8)RShiftU64 (StartLba, 32);
    Acb->AtaCylinderHighExp = (UINT8)RShiftU64 (StartLba, 40);
    Acb->AtaSectorCountExp  = (UINT8)(TransferLength >> 8);
  } else {
    Acb->AtaDeviceHead = (UINT8)(Acb->AtaDeviceHead | RShiftU64 (StartLba, 24));
  }

  //
  // Prepare for ATA pass through packet.
  //
  if (TaskPacket != NULL) {
    Packet = ZeroMem (TaskPacket, sizeof (EFI_ATA_PASS_THRU_COMMAND_PACKET));
  } else {
    Packet = ZeroMem (&AtaDevice->Packet, sizeof (EFI_ATA_PASS_THRU_COMMAND_PACKET));
  }

  if (IsWrite) {
    Packet->OutDataBuffer     = Buffer;
    Packet->OutTransferLength = TransferLength;
  } else {
    Packet->InDataBuffer     = Buffer;
    Packet->InTransferLength = TransferLength;
  }

  Packet->Protocol = mAtaPassThruCmdProtocols[AtaDevice->UdmaValid][IsWrite];
  Packet->Length   = EFI_ATA_PASS_THRU_LENGTH_SECTOR_COUNT;
  //
  // |------------------------|-----------------|------------------------|-----------------|
  // | ATA PIO Transfer Mode  |  Transfer Rate  | ATA DMA Transfer Mode  |  Transfer Rate  |
  // |------------------------|-----------------|------------------------|-----------------|
  // |       PIO Mode 0       |  3.3Mbytes/sec  | Single-word DMA Mode 0 |  2.1Mbytes/sec  |
  // |------------------------|-----------------|------------------------|-----------------|
  // |       PIO Mode 1       |  5.2Mbytes/sec  | Single-word DMA Mode 1 |  4.2Mbytes/sec  |
  // |------------------------|-----------------|------------------------|-----------------|
  // |       PIO Mode 2       |  8.3Mbytes/sec  | Single-word DMA Mode 2 |  8.4Mbytes/sec  |
  // |------------------------|-----------------|------------------------|-----------------|
  // |       PIO Mode 3       | 11.1Mbytes/sec  | Multi-word DMA Mode 0  |  4.2Mbytes/sec  |
  // |------------------------|-----------------|------------------------|-----------------|
  // |       PIO Mode 4       | 16.6Mbytes/sec  | Multi-word DMA Mode 1  | 13.3Mbytes/sec  |
  // |------------------------|-----------------|------------------------|-----------------|
  //
  // As AtaBus is used to manage ATA devices, we have to use the lowest transfer rate to
  // calculate the possible maximum timeout value for each read/write operation.
  // The timeout value is rounded up to nearest integer and here an additional 30s is added
  // to follow ATA spec in which it mentioned that the device may take up to 30s to respond
  // commands in the Standby/Idle mode.
  //
  if (AtaDevice->UdmaValid) {
    //
    // Calculate the maximum timeout value for DMA read/write operation.
    //
    Packet->Timeout = EFI_TIMER_PERIOD_SECONDS (DivU64x32 (MultU64x32 (TransferLength, AtaDevice->BlockMedia.BlockSize), 2100000) + 31);
  } else {
    //
    // Calculate the maximum timeout value for PIO read/write operation
    //
    Packet->Timeout = EFI_TIMER_PERIOD_SECONDS (DivU64x32 (MultU64x32 (TransferLength, AtaDevice->BlockMedia.BlockSize), 3300000) + 31);
  }

  return AtaDevicePassThru (AtaDevice, TaskPacket, Event);
}

/**
  Free SubTask.

  @param[in, out]  Task      Pointer to task to be freed.

**/
VOID
EFIAPI
FreeAtaSubTask (
  IN OUT ATA_BUS_ASYN_SUB_TASK  *Task
  )
{
  if (Task->Packet.Asb != NULL) {
    FreeAlignedBuffer (Task->Packet.Asb, sizeof (EFI_ATA_STATUS_BLOCK));
  }

  if (Task->Packet.Acb != NULL) {
    FreePool (Task->Packet.Acb);
  }

  FreePool (Task);
}

/**
  Terminate any in-flight non-blocking I/O requests by signaling an EFI_ABORTED
  in the TransactionStatus member of the EFI_BLOCK_IO2_TOKEN for the non-blocking
  I/O. After that it is safe to free any Token or Buffer data structures that
  were allocated to initiate the non-blockingI/O requests that were in-flight for
  this device.

  @param[in]  AtaDevice     The ATA child device involved for the operation.

**/
VOID
EFIAPI
AtaTerminateNonBlockingTask (
  IN ATA_DEVICE  *AtaDevice
  )
{
  BOOLEAN            SubTaskEmpty;
  EFI_TPL            OldTpl;
  ATA_BUS_ASYN_TASK  *AtaTask;
  LIST_ENTRY         *Entry;
  LIST_ENTRY         *List;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  //
  // Abort all executing tasks from now.
  //
  AtaDevice->Abort = TRUE;

  List = &AtaDevice->AtaTaskList;
  for (Entry = GetFirstNode (List); !IsNull (List, Entry);) {
    AtaTask                           = ATA_ASYN_TASK_FROM_ENTRY (Entry);
    AtaTask->Token->TransactionStatus = EFI_ABORTED;
    gBS->SignalEvent (AtaTask->Token->Event);

    Entry = RemoveEntryList (Entry);
    FreePool (AtaTask);
  }

  gBS->RestoreTPL (OldTpl);

  do {
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    //
    // Wait for executing subtasks done.
    //
    SubTaskEmpty = IsListEmpty (&AtaDevice->AtaSubTaskList);
    gBS->RestoreTPL (OldTpl);
  } while (!SubTaskEmpty);

  //
  // Aborting operation has been done. From now on, don't need to abort normal operation.
  //
  OldTpl           = gBS->RaiseTPL (TPL_NOTIFY);
  AtaDevice->Abort = FALSE;
  gBS->RestoreTPL (OldTpl);
}

/**
  Call back function when the event is signaled.

  @param[in]  Event     The Event this notify function registered to.
  @param[in]  Context   Pointer to the context data registered to the
                        Event.

**/
VOID
EFIAPI
AtaNonBlockingCallBack (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  ATA_BUS_ASYN_SUB_TASK  *Task;
  ATA_BUS_ASYN_TASK      *AtaTask;
  ATA_DEVICE             *AtaDevice;
  LIST_ENTRY             *Entry;
  EFI_STATUS             Status;

  Task = (ATA_BUS_ASYN_SUB_TASK *)Context;
  gBS->CloseEvent (Event);

  AtaDevice = Task->AtaDevice;

  //
  // Check the command status.
  // If there is error during the sub task source allocation, the error status
  // should be returned to the caller directly, so here the Task->Token may already
  // be deleted by the caller and no need to update the status.
  //
  if ((!(*Task->IsError)) && ((Task->Packet.Asb->AtaStatus & 0x01) == 0x01)) {
    Task->Token->TransactionStatus = EFI_DEVICE_ERROR;
  }

  if (AtaDevice->Abort) {
    Task->Token->TransactionStatus = EFI_ABORTED;
  }

  DEBUG ((
    DEBUG_BLKIO,
    "NON-BLOCKING EVENT FINISHED!- STATUS = %r\n",
    Task->Token->TransactionStatus
    ));

  //
  // Reduce the SubEventCount, till it comes to zero.
  //
  (*Task->UnsignalledEventCount)--;
  DEBUG ((DEBUG_BLKIO, "UnsignalledEventCount = %d\n", *Task->UnsignalledEventCount));

  //
  // Remove the SubTask from the Task list.
  //
  RemoveEntryList (&Task->TaskEntry);
  if ((*Task->UnsignalledEventCount) == 0) {
    //
    // All Sub tasks are done, then signal the upper layer event.
    // Except there is error during the sub task source allocation.
    //
    if (!(*Task->IsError)) {
      gBS->SignalEvent (Task->Token->Event);
      DEBUG ((DEBUG_BLKIO, "Signal the upper layer event!\n"));
    }

    FreePool (Task->UnsignalledEventCount);
    FreePool (Task->IsError);

    //
    // Finish all subtasks and move to the next task in AtaTaskList.
    //
    if (!IsListEmpty (&AtaDevice->AtaTaskList)) {
      Entry   = GetFirstNode (&AtaDevice->AtaTaskList);
      AtaTask = ATA_ASYN_TASK_FROM_ENTRY (Entry);
      DEBUG ((DEBUG_BLKIO, "Start to embark a new Ata Task\n"));
      DEBUG ((DEBUG_BLKIO, "AtaTask->NumberOfBlocks = %x; AtaTask->Token=%x\n", AtaTask->NumberOfBlocks, AtaTask->Token));
      Status = AccessAtaDevice (
                 AtaTask->AtaDevice,
                 AtaTask->Buffer,
                 AtaTask->StartLba,
                 AtaTask->NumberOfBlocks,
                 AtaTask->IsWrite,
                 AtaTask->Token
                 );
      if (EFI_ERROR (Status)) {
        AtaTask->Token->TransactionStatus = Status;
        gBS->SignalEvent (AtaTask->Token->Event);
      }

      RemoveEntryList (Entry);
      FreePool (AtaTask);
    }
  }

  DEBUG ((
    DEBUG_BLKIO,
    "PACKET INFO: Write=%s, Length=%x, LowCylinder=%x, HighCylinder=%x, SectionNumber=%x\n",
    Task->Packet.OutDataBuffer != NULL ? L"YES" : L"NO",
    Task->Packet.OutDataBuffer != NULL ? Task->Packet.OutTransferLength : Task->Packet.InTransferLength,
    Task->Packet.Acb->AtaCylinderLow,
    Task->Packet.Acb->AtaCylinderHigh,
    Task->Packet.Acb->AtaSectorCount
    ));

  //
  // Free the buffer of SubTask.
  //
  FreeAtaSubTask (Task);
}

/**
  Read or write a number of blocks from ATA device.

  This function performs ATA pass through transactions to read/write data from/to
  ATA device. It may separate the read/write request into several ATA pass through
  transactions.

  @param[in, out]  AtaDevice       The ATA child device involved for the operation.
  @param[in, out]  Buffer          The pointer to the current transaction buffer.
  @param[in]       StartLba        The starting logical block address to be accessed.
  @param[in]       NumberOfBlocks  The block number or sector count of the transfer.
  @param[in]       IsWrite         Indicates whether it is a write operation.
  @param[in, out]  Token           A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS       The data transfer is complete successfully.
  @return others            Some error occurs when transferring data.

**/
EFI_STATUS
AccessAtaDevice (
  IN OUT ATA_DEVICE           *AtaDevice,
  IN OUT UINT8                *Buffer,
  IN EFI_LBA                  StartLba,
  IN UINTN                    NumberOfBlocks,
  IN BOOLEAN                  IsWrite,
  IN OUT EFI_BLOCK_IO2_TOKEN  *Token
  )
{
  EFI_STATUS             Status;
  UINTN                  MaxTransferBlockNumber;
  UINTN                  TransferBlockNumber;
  UINTN                  BlockSize;
  ATA_BUS_ASYN_SUB_TASK  *SubTask;
  UINTN                  *EventCount;
  UINTN                  TempCount;
  ATA_BUS_ASYN_TASK      *AtaTask;
  EFI_EVENT              SubEvent;
  UINTN                  Index;
  BOOLEAN                *IsError;
  EFI_TPL                OldTpl;

  TempCount  = 0;
  Status     = EFI_SUCCESS;
  EventCount = NULL;
  IsError    = NULL;
  Index      = 0;
  SubTask    = NULL;
  SubEvent   = NULL;
  AtaTask    = NULL;

  //
  // Ensure AtaDevice->Lba48Bit is a valid boolean value
  //
  ASSERT ((UINTN)AtaDevice->Lba48Bit < 2);
  MaxTransferBlockNumber = mMaxTransferBlockNumber[AtaDevice->Lba48Bit];
  BlockSize              = AtaDevice->BlockMedia.BlockSize;

  //
  // Initial the return status and shared account for Non Blocking.
  //
  if ((Token != NULL) && (Token->Event != NULL)) {
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

    if (!IsListEmpty (&AtaDevice->AtaSubTaskList)) {
      AtaTask = AllocateZeroPool (sizeof (ATA_BUS_ASYN_TASK));
      if (AtaTask == NULL) {
        gBS->RestoreTPL (OldTpl);
        return EFI_OUT_OF_RESOURCES;
      }

      AtaTask->AtaDevice      = AtaDevice;
      AtaTask->Buffer         = Buffer;
      AtaTask->IsWrite        = IsWrite;
      AtaTask->NumberOfBlocks = NumberOfBlocks;
      AtaTask->Signature      = ATA_TASK_SIGNATURE;
      AtaTask->StartLba       = StartLba;
      AtaTask->Token          = Token;

      InsertTailList (&AtaDevice->AtaTaskList, &AtaTask->TaskEntry);
      gBS->RestoreTPL (OldTpl);
      return EFI_SUCCESS;
    }

    gBS->RestoreTPL (OldTpl);

    Token->TransactionStatus = EFI_SUCCESS;
    EventCount               = AllocateZeroPool (sizeof (UINTN));
    if (EventCount == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    IsError = AllocateZeroPool (sizeof (BOOLEAN));
    if (IsError == NULL) {
      FreePool (EventCount);
      return EFI_OUT_OF_RESOURCES;
    }

    DEBUG ((DEBUG_BLKIO, "Allocation IsError Addr=%x\n", IsError));
    *IsError    = FALSE;
    TempCount   = (NumberOfBlocks + MaxTransferBlockNumber - 1) / MaxTransferBlockNumber;
    *EventCount = TempCount;
    DEBUG ((DEBUG_BLKIO, "AccessAtaDevice, NumberOfBlocks=%x\n", NumberOfBlocks));
    DEBUG ((DEBUG_BLKIO, "AccessAtaDevice, MaxTransferBlockNumber=%x\n", MaxTransferBlockNumber));
    DEBUG ((DEBUG_BLKIO, "AccessAtaDevice, EventCount=%x\n", TempCount));
  } else {
    while (!IsListEmpty (&AtaDevice->AtaTaskList) || !IsListEmpty (&AtaDevice->AtaSubTaskList)) {
      //
      // Stall for 100us.
      //
      MicroSecondDelay (100);
    }
  }

  do {
    if (NumberOfBlocks > MaxTransferBlockNumber) {
      TransferBlockNumber = MaxTransferBlockNumber;
      NumberOfBlocks     -= MaxTransferBlockNumber;
    } else {
      TransferBlockNumber = NumberOfBlocks;
      NumberOfBlocks      = 0;
    }

    //
    // Create sub event for the sub ata task. Non-blocking mode.
    //
    if ((Token != NULL) && (Token->Event != NULL)) {
      SubTask  = NULL;
      SubEvent = NULL;

      SubTask = AllocateZeroPool (sizeof (ATA_BUS_ASYN_SUB_TASK));
      if (SubTask == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
      }

      OldTpl                         = gBS->RaiseTPL (TPL_NOTIFY);
      SubTask->UnsignalledEventCount = EventCount;
      SubTask->Signature             = ATA_SUB_TASK_SIGNATURE;
      SubTask->AtaDevice             = AtaDevice;
      SubTask->Token                 = Token;
      SubTask->IsError               = IsError;
      InsertTailList (&AtaDevice->AtaSubTaskList, &SubTask->TaskEntry);
      gBS->RestoreTPL (OldTpl);

      Status = gBS->CreateEvent (
                      EVT_NOTIFY_SIGNAL,
                      TPL_NOTIFY,
                      AtaNonBlockingCallBack,
                      SubTask,
                      &SubEvent
                      );
      //
      // If resource allocation fail, the un-signalled event count should equal to
      // the original one minus the unassigned subtasks number.
      //
      if (EFI_ERROR (Status)) {
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
      }

      Status = TransferAtaDevice (AtaDevice, &SubTask->Packet, Buffer, StartLba, (UINT32)TransferBlockNumber, IsWrite, SubEvent);
    } else {
      //
      // Blocking Mode.
      //
      DEBUG ((DEBUG_BLKIO, "Blocking AccessAtaDevice, TransferBlockNumber=%x; StartLba = %x\n", TransferBlockNumber, StartLba));
      Status = TransferAtaDevice (AtaDevice, NULL, Buffer, StartLba, (UINT32)TransferBlockNumber, IsWrite, NULL);
    }

    if (EFI_ERROR (Status)) {
      goto EXIT;
    }

    Index++;
    StartLba += TransferBlockNumber;
    Buffer   += TransferBlockNumber * BlockSize;
  } while (NumberOfBlocks > 0);

EXIT:
  if ((Token != NULL) && (Token->Event != NULL)) {
    //
    // Release resource at non-blocking mode.
    //
    if (EFI_ERROR (Status)) {
      OldTpl                   = gBS->RaiseTPL (TPL_NOTIFY);
      Token->TransactionStatus = Status;
      *EventCount              = (*EventCount) - (TempCount - Index);
      *IsError                 = TRUE;

      if (*EventCount == 0) {
        FreePool (EventCount);
        FreePool (IsError);
      }

      if (SubTask != NULL) {
        RemoveEntryList (&SubTask->TaskEntry);
        FreeAtaSubTask (SubTask);
      }

      if (SubEvent != NULL) {
        gBS->CloseEvent (SubEvent);
      }

      gBS->RestoreTPL (OldTpl);
    }
  }

  return Status;
}

/**
  Trust transfer data from/to ATA device.

  This function performs one ATA pass through transaction to do a trust transfer from/to
  ATA device. It chooses the appropriate ATA command and protocol to invoke PassThru
  interface of ATA pass through.

  @param  AtaDevice                    The ATA child device involved for the operation.
  @param  Buffer                       The pointer to the current transaction buffer.
  @param  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                       the security protocol command to be sent.
  @param  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                       of the security protocol command to be sent.
  @param  TransferLength               The block number or sector count of the transfer.
  @param  IsTrustSend                  Indicates whether it is a trust send operation or not.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT
                                       if the time required to execute the receive data command
                                       is greater than Timeout.
  @param  TransferLengthOut            A pointer to a buffer to store the size in bytes of the data
                                       written to the buffer. Ignore it when IsTrustSend is TRUE.

  @retval EFI_SUCCESS       The data transfer is complete successfully.
  @return others            Some error occurs when transferring data.

**/
EFI_STATUS
EFIAPI
TrustTransferAtaDevice (
  IN OUT ATA_DEVICE  *AtaDevice,
  IN OUT VOID        *Buffer,
  IN UINT8           SecurityProtocolId,
  IN UINT16          SecurityProtocolSpecificData,
  IN UINTN           TransferLength,
  IN BOOLEAN         IsTrustSend,
  IN UINT64          Timeout,
  OUT UINTN          *TransferLengthOut
  )
{
  EFI_ATA_COMMAND_BLOCK             *Acb;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  *Packet;
  EFI_STATUS                        Status;
  VOID                              *NewBuffer;
  EFI_ATA_PASS_THRU_PROTOCOL        *AtaPassThru;

  //
  // Ensure AtaDevice->UdmaValid and IsTrustSend are valid boolean values
  //
  ASSERT ((UINTN)AtaDevice->UdmaValid < 2);
  ASSERT ((UINTN)IsTrustSend < 2);
  //
  // Prepare for ATA command block.
  //
  Acb = ZeroMem (&AtaDevice->Acb, sizeof (EFI_ATA_COMMAND_BLOCK));
  if (TransferLength == 0) {
    Acb->AtaCommand = ATA_CMD_TRUST_NON_DATA;
  } else {
    Acb->AtaCommand = mAtaTrustCommands[AtaDevice->UdmaValid][IsTrustSend];
  }

  Acb->AtaFeatures     = SecurityProtocolId;
  Acb->AtaSectorCount  = (UINT8)(TransferLength / 512);
  Acb->AtaSectorNumber = (UINT8)((TransferLength / 512) >> 8);
  //
  // NOTE: ATA Spec has no explicitly definition for Security Protocol Specific layout.
  // Here use big endian for Cylinder register.
  //
  Acb->AtaCylinderHigh = (UINT8)SecurityProtocolSpecificData;
  Acb->AtaCylinderLow  = (UINT8)(SecurityProtocolSpecificData >> 8);
  Acb->AtaDeviceHead   = (UINT8)(BIT7 | BIT6 | BIT5 | (AtaDevice->PortMultiplierPort == 0xFFFF ? 0 : (AtaDevice->PortMultiplierPort << 4)));

  //
  // Prepare for ATA pass through packet.
  //
  Packet = ZeroMem (&AtaDevice->Packet, sizeof (EFI_ATA_PASS_THRU_COMMAND_PACKET));
  if (TransferLength == 0) {
    Packet->InTransferLength  = 0;
    Packet->OutTransferLength = 0;
    Packet->Protocol          = EFI_ATA_PASS_THRU_PROTOCOL_ATA_NON_DATA;
  } else if (IsTrustSend) {
    //
    // Check the alignment of the incoming buffer prior to invoking underlying ATA PassThru
    //
    AtaPassThru = AtaDevice->AtaBusDriverData->AtaPassThru;
    if ((AtaPassThru->Mode->IoAlign > 1) && !IS_ALIGNED (Buffer, AtaPassThru->Mode->IoAlign)) {
      NewBuffer = AllocateAlignedBuffer (AtaDevice, TransferLength);
      if (NewBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      CopyMem (NewBuffer, Buffer, TransferLength);
      FreePool (Buffer);
      Buffer = NewBuffer;
    }

    Packet->OutDataBuffer     = Buffer;
    Packet->OutTransferLength = (UINT32)TransferLength;
    Packet->Protocol          = mAtaPassThruCmdProtocols[AtaDevice->UdmaValid][IsTrustSend];
  } else {
    Packet->InDataBuffer     = Buffer;
    Packet->InTransferLength = (UINT32)TransferLength;
    Packet->Protocol         = mAtaPassThruCmdProtocols[AtaDevice->UdmaValid][IsTrustSend];
  }

  Packet->Length  = EFI_ATA_PASS_THRU_LENGTH_BYTES;
  Packet->Timeout = Timeout;

  Status = AtaDevicePassThru (AtaDevice, NULL, NULL);
  if (TransferLengthOut != NULL) {
    if (!IsTrustSend) {
      *TransferLengthOut = Packet->InTransferLength;
    }
  }

  return Status;
}

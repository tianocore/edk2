/** @file
  The helper functions for BlockIo and BlockIo2 protocol.

  Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SdDxe.h"

/**
  Nonblocking I/O callback funtion when the event is signaled.

  @param[in]  Event     The Event this notify function registered to.
  @param[in]  Context   Pointer to the context data registered to the
                        Event.

**/
VOID
EFIAPI
AsyncIoCallback (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  SD_REQUEST                  *Request;

  gBS->CloseEvent (Event);

  Request = (SD_REQUEST *) Context;

  DEBUG_CODE_BEGIN ();
    DEBUG ((EFI_D_INFO, "Sd Async Request: CmdIndex[%d] Arg[%08x] %r\n",
            Request->SdMmcCmdBlk.CommandIndex, Request->SdMmcCmdBlk.CommandArgument,
            Request->Packet.TransactionStatus));
  DEBUG_CODE_END ();

  if (EFI_ERROR (Request->Packet.TransactionStatus)) {
    Request->Token->TransactionStatus = Request->Packet.TransactionStatus;
  }

  RemoveEntryList (&Request->Link);

  if (Request->IsEnd) {
    gBS->SignalEvent (Request->Token->Event);
  }

  FreePool (Request);
}

/**
  Send command SET_RELATIVE_ADDRESS to the device to set the device address.

  @param[in]  Device            A pointer to the SD_DEVICE instance.
  @param[out] Rca               The relative device address to assign.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
SdSetRca (
  IN     SD_DEVICE              *Device,
     OUT UINT16                 *Rca
  )
{
  EFI_STATUS                           Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL        *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = SD_SET_RELATIVE_ADDR;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeBcr;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR6;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL);
  if (!EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "Set RCA succeeds with Resp0 = 0x%x\n", SdMmcStatusBlk.Resp0));
    *Rca = (UINT16)(SdMmcStatusBlk.Resp0 >> 16);
  }

  return Status;
}

/**
  Send command SELECT to the device to select/deselect the device.

  @param[in]  Device            A pointer to the SD_DEVICE instance.
  @param[in]  Rca               The relative device address to use.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
SdSelect (
  IN     SD_DEVICE              *Device,
  IN     UINT16                 Rca
  )
{
  EFI_STATUS                           Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL        *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = SD_SELECT_DESELECT_CARD;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  if (Rca != 0) {
    SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1b;
  }
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL);

  return Status;
}

/**
  Send command SEND_STATUS to the device to get device status.

  @param[in]  Device            A pointer to the SD_DEVICE instance.
  @param[in]  Rca               The relative device address to use.
  @param[out] DevStatus         The buffer to store the device status.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
SdSendStatus (
  IN     SD_DEVICE              *Device,
  IN     UINT16                 Rca,
     OUT UINT32                 *DevStatus
  )
{
  EFI_STATUS                           Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL        *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = SD_SEND_STATUS;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL);
  if (!EFI_ERROR (Status)) {
    CopyMem (DevStatus, &SdMmcStatusBlk.Resp0, sizeof (UINT32));
  }
  return Status;
}

/**
  Send command SEND_CSD to the device to get the CSD register data.

  @param[in]  Device            A pointer to the SD_DEVICE instance.
  @param[in]  Rca               The relative device address to use.
  @param[out] Csd               The buffer to store the SD_CSD register data.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
SdGetCsd (
  IN     SD_DEVICE              *Device,
  IN     UINT16                 Rca,
     OUT SD_CSD                 *Csd
  )
{
  EFI_STATUS                           Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL        *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  ZeroMem (Csd, sizeof (SD_CSD));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = SD_SEND_CSD;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR2;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL);

  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    CopyMem (((UINT8*)Csd) + 1, &SdMmcStatusBlk.Resp0, sizeof (SD_CSD) - 1);
  }

  return Status;
}

/**
  Send command SEND_CID to the device to get the CID register data.

  @param[in]  Device            A pointer to the SD_DEVICE instance.
  @param[in]  Rca               The relative device address to use.
  @param[out] Cid               The buffer to store the SD_CID register data.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
SdGetCid (
  IN     SD_DEVICE              *Device,
  IN     UINT16                 Rca,
     OUT SD_CID                 *Cid
  )
{
  EFI_STATUS                           Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL        *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK             SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK              SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET  Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  ZeroMem (Cid, sizeof (SD_CID));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = SD_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = SD_SEND_CID;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR2;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL);

  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    CopyMem (((UINT8*)Cid) + 1, &SdMmcStatusBlk.Resp0, sizeof (SD_CID) - 1);
  }

  return Status;
}

/**
  Read/write single block through sync or async I/O request.

  @param[in]  Device            A pointer to the SD_DEVICE instance.
  @param[in]  Lba               The starting logical block address to be read/written.
                                The caller is responsible for reading/writing to only
                                legitimate locations.
  @param[in]  Buffer            A pointer to the destination/source buffer for the data.
  @param[in]  BufferSize        Size of Buffer, must be a multiple of device block size.
  @param[in]  IsRead            Indicates it is a read or write operation.
  @param[in]  Token             A pointer to the token associated with the transaction.
  @param[in]  IsEnd             A boolean to show whether it's the last cmd in a series of cmds.
                                This parameter is only meaningful in async I/O request.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
SdRwSingleBlock (
  IN  SD_DEVICE                 *Device,
  IN  EFI_LBA                   Lba,
  IN  VOID                      *Buffer,
  IN  UINTN                     BufferSize,
  IN  BOOLEAN                   IsRead,
  IN  EFI_BLOCK_IO2_TOKEN       *Token,
  IN  BOOLEAN                   IsEnd
  )
{
  EFI_STATUS                           Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL        *PassThru;
  SD_REQUEST                           *RwSingleBlkReq;
  EFI_TPL                              OldTpl;

  RwSingleBlkReq = NULL;
  PassThru       = Device->Private->PassThru;

  RwSingleBlkReq = AllocateZeroPool (sizeof (SD_REQUEST));
  if (RwSingleBlkReq == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  RwSingleBlkReq->Signature = SD_REQUEST_SIGNATURE;
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  InsertTailList (&Device->Queue, &RwSingleBlkReq->Link);
  gBS->RestoreTPL (OldTpl);
  RwSingleBlkReq->Packet.SdMmcCmdBlk    = &RwSingleBlkReq->SdMmcCmdBlk;
  RwSingleBlkReq->Packet.SdMmcStatusBlk = &RwSingleBlkReq->SdMmcStatusBlk;
  //
  // Calculate timeout value through the below formula.
  // Timeout = (transfer size) / (2MB/s).
  // Taking 2MB/s as divisor as it's the lowest transfer speed
  // above class 2.
  // Refer to SD Physical Layer Simplified spec section 3.4 for details.
  //
  RwSingleBlkReq->Packet.Timeout = (BufferSize / (2 * 1024 * 1024) + 1) * 1000 * 1000;

  if (IsRead) {
    RwSingleBlkReq->Packet.InDataBuffer     = Buffer;
    RwSingleBlkReq->Packet.InTransferLength = (UINT32)BufferSize;

    RwSingleBlkReq->SdMmcCmdBlk.CommandIndex = SD_READ_SINGLE_BLOCK;
    RwSingleBlkReq->SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
    RwSingleBlkReq->SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  } else {
    RwSingleBlkReq->Packet.OutDataBuffer     = Buffer;
    RwSingleBlkReq->Packet.OutTransferLength = (UINT32)BufferSize;

    RwSingleBlkReq->SdMmcCmdBlk.CommandIndex = SD_WRITE_SINGLE_BLOCK;
    RwSingleBlkReq->SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
    RwSingleBlkReq->SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  }

  if (Device->SectorAddressing) {
    RwSingleBlkReq->SdMmcCmdBlk.CommandArgument = (UINT32)Lba;
  } else {
    RwSingleBlkReq->SdMmcCmdBlk.CommandArgument = (UINT32)MultU64x32 (Lba, Device->BlockMedia.BlockSize);
  }

  RwSingleBlkReq->IsEnd = IsEnd;
  RwSingleBlkReq->Token = Token;

  if ((Token != NULL) && (Token->Event != NULL)) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    AsyncIoCallback,
                    RwSingleBlkReq,
                    &RwSingleBlkReq->Event
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  } else {
    RwSingleBlkReq->Event = NULL;
  }

  Status = PassThru->PassThru (PassThru, Device->Slot, &RwSingleBlkReq->Packet, RwSingleBlkReq->Event);

Error:
  if ((Token != NULL) && (Token->Event != NULL)) {
    //
    // For asynchronous operation, only free request and event in error case.
    // The request and event will be freed in asynchronous callback for success case.
    //
    if (EFI_ERROR (Status) && (RwSingleBlkReq != NULL)) {
      RemoveEntryList (&RwSingleBlkReq->Link);
      if (RwSingleBlkReq->Event != NULL) {
        gBS->CloseEvent (RwSingleBlkReq->Event);
      }
      FreePool (RwSingleBlkReq);
    }
  } else {
    //
    // For synchronous operation, free request whatever the execution result is.
    //
    if (RwSingleBlkReq != NULL) {
      RemoveEntryList (&RwSingleBlkReq->Link);
      FreePool (RwSingleBlkReq);
    }
  }

  return Status;
}

/**
  Read/write multiple blocks through sync or async I/O request.

  @param[in]  Device            A pointer to the SD_DEVICE instance.
  @param[in]  Lba               The starting logical block address to be read/written.
                                The caller is responsible for reading/writing to only
                                legitimate locations.
  @param[in]  Buffer            A pointer to the destination/source buffer for the data.
  @param[in]  BufferSize        Size of Buffer, must be a multiple of device block size.
  @param[in]  IsRead            Indicates it is a read or write operation.
  @param[in]  Token             A pointer to the token associated with the transaction.
  @param[in]  IsEnd             A boolean to show whether it's the last cmd in a series of cmds.
                                This parameter is only meaningful in async I/O request.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
SdRwMultiBlocks (
  IN  SD_DEVICE                 *Device,
  IN  EFI_LBA                   Lba,
  IN  VOID                      *Buffer,
  IN  UINTN                     BufferSize,
  IN  BOOLEAN                   IsRead,
  IN  EFI_BLOCK_IO2_TOKEN       *Token,
  IN  BOOLEAN                   IsEnd
  )
{
  EFI_STATUS                    Status;
  SD_REQUEST                    *RwMultiBlkReq;
  EFI_SD_MMC_PASS_THRU_PROTOCOL *PassThru;
  EFI_TPL                       OldTpl;

  RwMultiBlkReq = NULL;

  PassThru = Device->Private->PassThru;

  RwMultiBlkReq = AllocateZeroPool (sizeof (SD_REQUEST));
  if (RwMultiBlkReq == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  RwMultiBlkReq->Signature = SD_REQUEST_SIGNATURE;
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  InsertTailList (&Device->Queue, &RwMultiBlkReq->Link);
  gBS->RestoreTPL (OldTpl);
  RwMultiBlkReq->Packet.SdMmcCmdBlk    = &RwMultiBlkReq->SdMmcCmdBlk;
  RwMultiBlkReq->Packet.SdMmcStatusBlk = &RwMultiBlkReq->SdMmcStatusBlk;
  //
  // Calculate timeout value through the below formula.
  // Timeout = (transfer size) / (2MB/s).
  // Taking 2MB/s as divisor as it's the lowest transfer speed
  // above class 2.
  // Refer to SD Physical Layer Simplified spec section 3.4 for details.
  //
  RwMultiBlkReq->Packet.Timeout = (BufferSize / (2 * 1024 * 1024) + 1) * 1000 * 1000;

  if (IsRead) {
    RwMultiBlkReq->Packet.InDataBuffer     = Buffer;
    RwMultiBlkReq->Packet.InTransferLength = (UINT32)BufferSize;

    RwMultiBlkReq->SdMmcCmdBlk.CommandIndex = SD_READ_MULTIPLE_BLOCK;
    RwMultiBlkReq->SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
    RwMultiBlkReq->SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  } else {
    RwMultiBlkReq->Packet.OutDataBuffer     = Buffer;
    RwMultiBlkReq->Packet.OutTransferLength = (UINT32)BufferSize;

    RwMultiBlkReq->SdMmcCmdBlk.CommandIndex = SD_WRITE_MULTIPLE_BLOCK;
    RwMultiBlkReq->SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
    RwMultiBlkReq->SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  }

  if (Device->SectorAddressing) {
    RwMultiBlkReq->SdMmcCmdBlk.CommandArgument = (UINT32)Lba;
  } else {
    RwMultiBlkReq->SdMmcCmdBlk.CommandArgument = (UINT32)MultU64x32 (Lba, Device->BlockMedia.BlockSize);
  }

  RwMultiBlkReq->IsEnd = IsEnd;
  RwMultiBlkReq->Token = Token;

  if ((Token != NULL) && (Token->Event != NULL)) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    AsyncIoCallback,
                    RwMultiBlkReq,
                    &RwMultiBlkReq->Event
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  } else {
    RwMultiBlkReq->Event = NULL;
  }

  Status = PassThru->PassThru (PassThru, Device->Slot, &RwMultiBlkReq->Packet, RwMultiBlkReq->Event);

Error:
  if ((Token != NULL) && (Token->Event != NULL)) {
    //
    // For asynchronous operation, only free request and event in error case.
    // The request and event will be freed in asynchronous callback for success case.
    //
    if (EFI_ERROR (Status) && (RwMultiBlkReq != NULL)) {
      RemoveEntryList (&RwMultiBlkReq->Link);
      if (RwMultiBlkReq->Event != NULL) {
        gBS->CloseEvent (RwMultiBlkReq->Event);
      }
      FreePool (RwMultiBlkReq);
    }
  } else {
    //
    // For synchronous operation, free request whatever the execution result is.
    //
    if (RwMultiBlkReq != NULL) {
      RemoveEntryList (&RwMultiBlkReq->Link);
      FreePool (RwMultiBlkReq);
    }
  }

  return Status;
}

/**
  This function transfers data from/to the sd memory card device.

  @param[in]       Device       A pointer to the SD_DEVICE instance.
  @param[in]       MediaId      The media ID that the read/write request is for.
  @param[in]       Lba          The starting logical block address to be read/written.
                                The caller is responsible for reading/writing to only
                                legitimate locations.
  @param[in, out]  Buffer       A pointer to the destination/source buffer for the data.
  @param[in]       BufferSize   Size of Buffer, must be a multiple of device block size.
  @param[in]       IsRead       Indicates it is a read or write operation.
  @param[in, out]  Token        A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS           The data was read/written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be read/written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read/write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read/write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
SdReadWrite (
  IN     SD_DEVICE                      *Device,
  IN     UINT32                         MediaId,
  IN     EFI_LBA                        Lba,
  IN OUT VOID                           *Buffer,
  IN     UINTN                          BufferSize,
  IN     BOOLEAN                        IsRead,
  IN OUT EFI_BLOCK_IO2_TOKEN            *Token
  )
{
  EFI_STATUS                            Status;
  EFI_BLOCK_IO_MEDIA                    *Media;
  UINTN                                 BlockSize;
  UINTN                                 BlockNum;
  UINTN                                 IoAlign;
  UINTN                                 Remaining;
  UINT32                                MaxBlock;
  BOOLEAN                               LastRw;

  Status = EFI_SUCCESS;
  Media  = &Device->BlockMedia;
  LastRw = FALSE;

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (!IsRead && Media->ReadOnly) {
    return EFI_WRITE_PROTECTED;
  }

  //
  // Check parameters.
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    if ((Token != NULL) && (Token->Event != NULL)) {
      Token->TransactionStatus = EFI_SUCCESS;
      gBS->SignalEvent (Token->Event);
    }
    return EFI_SUCCESS;
  }

  BlockSize = Media->BlockSize;
  if ((BufferSize % BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  BlockNum  = BufferSize / BlockSize;
  if ((Lba + BlockNum - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  IoAlign = Media->IoAlign;
  if (IoAlign > 0 && (((UINTN) Buffer & (IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Token != NULL) && (Token->Event != NULL)) {
    Token->TransactionStatus = EFI_SUCCESS;
  }

  //
  // Start to execute data transfer. The max block number in single cmd is 65535 blocks.
  //
  Remaining = BlockNum;
  MaxBlock  = 0xFFFF;

  while (Remaining > 0) {
    if (Remaining <= MaxBlock) {
      BlockNum = Remaining;
      LastRw   = TRUE;
    } else {
      BlockNum = MaxBlock;
    }

    BufferSize = BlockNum * BlockSize;
    if (BlockNum == 1) {
      Status = SdRwSingleBlock (Device, Lba, Buffer, BufferSize, IsRead, Token, LastRw);
    } else {
      Status = SdRwMultiBlocks (Device, Lba, Buffer, BufferSize, IsRead, Token, LastRw);
    }
    if (EFI_ERROR (Status)) {
      return Status;
    }
    DEBUG ((EFI_D_INFO, "Sd%a(): Lba 0x%x BlkNo 0x%x Event %p with %r\n", IsRead ? "Read" : "Write", Lba, BlockNum, Token->Event, Status));

    Lba   += BlockNum;
    Buffer = (UINT8*)Buffer + BufferSize;
    Remaining -= BlockNum;
  }

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
SdReset (
  IN  EFI_BLOCK_IO_PROTOCOL     *This,
  IN  BOOLEAN                   ExtendedVerification
  )
{
  EFI_STATUS                    Status;
  SD_DEVICE                     *Device;
  EFI_SD_MMC_PASS_THRU_PROTOCOL *PassThru;

  Device = SD_DEVICE_DATA_FROM_BLKIO (This);

  PassThru = Device->Private->PassThru;
  Status   = PassThru->ResetDevice (PassThru, Device->Slot);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
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

**/
EFI_STATUS
EFIAPI
SdReadBlocks (
  IN     EFI_BLOCK_IO_PROTOCOL  *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN     UINTN                  BufferSize,
     OUT VOID                   *Buffer
  )
{
  EFI_STATUS             Status;
  SD_DEVICE              *Device;

  Device = SD_DEVICE_DATA_FROM_BLKIO (This);

  Status = SdReadWrite (Device, MediaId, Lba, Buffer, BufferSize, TRUE, NULL);
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
SdWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  )
{
  EFI_STATUS             Status;
  SD_DEVICE              *Device;

  Device = SD_DEVICE_DATA_FROM_BLKIO (This);

  Status = SdReadWrite (Device, MediaId, Lba, Buffer, BufferSize, FALSE, NULL);
  return Status;
}

/**
  Flush the Block Device.

  @param  This              Indicates a pointer to the calling context.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writing back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
SdFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  )
{
  //
  // return directly
  //
  return EFI_SUCCESS;
}

/**
  Reset the Block Device.

  @param[in]  This                 Indicates a pointer to the calling context.
  @param[in]  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS              The device was reset.
  @retval EFI_DEVICE_ERROR         The device is not functioning properly and could
                                   not be reset.

**/
EFI_STATUS
EFIAPI
SdResetEx (
  IN  EFI_BLOCK_IO2_PROTOCOL  *This,
  IN  BOOLEAN                 ExtendedVerification
  )
{
  SD_DEVICE                   *Device;
  LIST_ENTRY                  *Link;
  LIST_ENTRY                  *NextLink;
  SD_REQUEST                  *Request;
  EFI_TPL                     OldTpl;

  Device = SD_DEVICE_DATA_FROM_BLKIO2 (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  for (Link = GetFirstNode (&Device->Queue);
       !IsNull (&Device->Queue, Link);
       Link = NextLink) {
    NextLink = GetNextNode (&Device->Queue, Link);
    RemoveEntryList (Link);

    Request = SD_REQUEST_FROM_LINK (Link);

    gBS->CloseEvent (Request->Event);
    Request->Token->TransactionStatus = EFI_ABORTED;

    if (Request->IsEnd) {
      gBS->SignalEvent (Request->Token->Event);
    }

    FreePool (Request);
  }
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

/**
  Read BufferSize bytes from Lba into Buffer.

  @param[in]       This         Indicates a pointer to the calling context.
  @param[in]       MediaId      Id of the media, changes every time the media is replaced.
  @param[in]       Lba          The starting Logical Block Address to read from.
  @param[in, out]  Token        A pointer to the token associated with the transaction.
  @param[in]       BufferSize   Size of Buffer, must be a multiple of device block size.
  @param[out]      Buffer       A pointer to the destination buffer for the data. The caller is
                                responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The read request was queued if Event is not NULL.
                                The data was read correctly from the device if
                                the Event is NULL.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing
                                the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of the
                                intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack
                                of resources.

**/
EFI_STATUS
EFIAPI
SdReadBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
     OUT VOID                   *Buffer
  )
{
  EFI_STATUS             Status;
  SD_DEVICE              *Device;

  Device = SD_DEVICE_DATA_FROM_BLKIO2 (This);

  Status = SdReadWrite (Device, MediaId, Lba, Buffer, BufferSize, TRUE, Token);
  return Status;
}

/**
  Write BufferSize bytes from Lba into Buffer.

  @param[in]       This         Indicates a pointer to the calling context.
  @param[in]       MediaId      The media ID that the write request is for.
  @param[in]       Lba          The starting logical block address to be written. The
                                caller is responsible for writing to only legitimate
                                locations.
  @param[in, out]  Token        A pointer to the token associated with the transaction.
  @param[in]       BufferSize   Size of Buffer, must be a multiple of device block size.
  @param[in]       Buffer       A pointer to the source buffer for the data.

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
SdWriteBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
  IN     VOID                   *Buffer
  )
{
  EFI_STATUS             Status;
  SD_DEVICE              *Device;

  Device = SD_DEVICE_DATA_FROM_BLKIO2 (This);

  Status = SdReadWrite (Device, MediaId, Lba, Buffer, BufferSize, FALSE, Token);
  return Status;
}

/**
  Flush the Block Device.

  @param[in]       This     Indicates a pointer to the calling context.
  @param[in, out]  Token    A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writing back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
SdFlushBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL  *This,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token
  )
{
  //
  // Signal event and return directly.
  //
  if (Token != NULL && Token->Event != NULL) {
    Token->TransactionStatus = EFI_SUCCESS;
    gBS->SignalEvent (Token->Event);
  }

  return EFI_SUCCESS;
}



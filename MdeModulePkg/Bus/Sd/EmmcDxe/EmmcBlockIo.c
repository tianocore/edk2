/** @file
  The helper functions for BlockIo and BlockIo2 protocol.

  Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EmmcDxe.h"

/**
  Nonblocking I/O callback function when the event is signaled.

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
  EMMC_REQUEST                *Request;
  EFI_STATUS                  Status;

  Status = gBS->CloseEvent (Event);
  if (EFI_ERROR (Status)) {
    return;
  }

  Request = (EMMC_REQUEST *) Context;

  DEBUG_CODE_BEGIN ();
    DEBUG ((EFI_D_INFO, "Emmc Async Request: CmdIndex[%d] Arg[%08x] %r\n",
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
  Send command SELECT to the device to select/deselect the device.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[in]  Rca               The relative device address to use.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcSelect (
  IN     EMMC_DEVICE                  *Device,
  IN     UINT16                       Rca
  )
{
  EFI_STATUS                          Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL       *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK            SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK             SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SELECT_DESELECT_CARD;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL);

  return Status;
}

/**
  Send command SEND_STATUS to the device to get device status.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[in]  Rca               The relative device address to use.
  @param[out] DevStatus         The buffer to store the device status.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcSendStatus (
  IN     EMMC_DEVICE                  *Device,
  IN     UINT16                       Rca,
     OUT UINT32                       *DevStatus
  )
{
  EFI_STATUS                          Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL       *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK            SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK             SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SEND_STATUS;
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

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[in]  Rca               The relative device address to use.
  @param[out] Csd               The buffer to store the EMMC_CSD register data.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcGetCsd (
  IN     EMMC_DEVICE                  *Device,
  IN     UINT16                       Rca,
     OUT EMMC_CSD                     *Csd
  )
{
  EFI_STATUS                          Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL       *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK            SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK             SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  ZeroMem (Csd, sizeof (EMMC_CSD));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SEND_CSD;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR2;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL);
  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    CopyMem (((UINT8*)Csd) + 1, &SdMmcStatusBlk.Resp0, sizeof (EMMC_CSD) - 1);
  }

  return Status;
}

/**
  Send command SEND_CID to the device to get the CID register data.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[in]  Rca               The relative device address to use.
  @param[out] Cid               The buffer to store the EMMC_CID register data.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcGetCid (
  IN     EMMC_DEVICE            *Device,
  IN     UINT16                 Rca,
     OUT EMMC_CID               *Cid
  )
{
  EFI_STATUS                          Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL       *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK            SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK             SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  ZeroMem (Cid, sizeof (EMMC_CID));

  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SEND_CID;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR2;
  SdMmcCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL);
  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    CopyMem (((UINT8*)Cid) + 1, &SdMmcStatusBlk.Resp0, sizeof (EMMC_CID) - 1);
  }

  return Status;
}

/**
  Send command SEND_EXT_CSD to the device to get the EXT_CSD register data.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[out] ExtCsd            The buffer to store the EXT_CSD register data.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcGetExtCsd (
  IN     EMMC_DEVICE                  *Device,
     OUT EMMC_EXT_CSD                 *ExtCsd
  )
{
  EFI_STATUS                          Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL       *PassThru;
  EFI_SD_MMC_COMMAND_BLOCK            SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK             SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET Packet;

  PassThru = Device->Private->PassThru;

  ZeroMem (&SdMmcCmdBlk, sizeof (SdMmcCmdBlk));
  ZeroMem (&SdMmcStatusBlk, sizeof (SdMmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));
  ZeroMem (ExtCsd, sizeof (EMMC_EXT_CSD));
  Packet.SdMmcCmdBlk    = &SdMmcCmdBlk;
  Packet.SdMmcStatusBlk = &SdMmcStatusBlk;
  Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SdMmcCmdBlk.CommandIndex = EMMC_SEND_EXT_CSD;
  SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
  SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  SdMmcCmdBlk.CommandArgument = 0x00000000;
  Packet.InDataBuffer     = ExtCsd;
  Packet.InTransferLength = sizeof (EMMC_EXT_CSD);

  Status = PassThru->PassThru (PassThru, Device->Slot, &Packet, NULL);

  return Status;
}

/**
  Set the specified EXT_CSD register field through sync or async I/O request.

  @param[in]  Partition         A pointer to the EMMC_PARTITION instance.
  @param[in]  Offset            The offset of the specified field in EXT_CSD register.
  @param[in]  Value             The byte value written to the field specified by Offset.
  @param[in]  Token             A pointer to the token associated with the transaction.
  @param[in]  IsEnd             A boolean to show whether it's the last cmd in a series of cmds.
                                This parameter is only meaningful in async I/O request.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcSetExtCsd (
  IN  EMMC_PARTITION            *Partition,
  IN  UINT8                     Offset,
  IN  UINT8                     Value,
  IN  EFI_BLOCK_IO2_TOKEN       *Token,
  IN  BOOLEAN                   IsEnd
  )
{
  EFI_STATUS                    Status;
  EMMC_DEVICE                   *Device;
  EMMC_REQUEST                  *SetExtCsdReq;
  EFI_SD_MMC_PASS_THRU_PROTOCOL *PassThru;
  UINT32                        CommandArgument;
  EFI_TPL                       OldTpl;

  SetExtCsdReq = NULL;

  Device   = Partition->Device;
  PassThru = Device->Private->PassThru;

  SetExtCsdReq = AllocateZeroPool (sizeof (EMMC_REQUEST));
  if (SetExtCsdReq == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  SetExtCsdReq->Signature = EMMC_REQUEST_SIGNATURE;
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&Partition->Queue, &SetExtCsdReq->Link);
  gBS->RestoreTPL (OldTpl);
  SetExtCsdReq->Packet.SdMmcCmdBlk    = &SetExtCsdReq->SdMmcCmdBlk;
  SetExtCsdReq->Packet.SdMmcStatusBlk = &SetExtCsdReq->SdMmcStatusBlk;
  SetExtCsdReq->Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SetExtCsdReq->SdMmcCmdBlk.CommandIndex = EMMC_SWITCH;
  SetExtCsdReq->SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SetExtCsdReq->SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1b;
  //
  // Write the Value to the field specified by Offset.
  //
  CommandArgument = (Value << 8) | (Offset << 16) | BIT24 | BIT25;
  SetExtCsdReq->SdMmcCmdBlk.CommandArgument = CommandArgument;

  SetExtCsdReq->IsEnd = IsEnd;
  SetExtCsdReq->Token = Token;

  if ((Token != NULL) && (Token->Event != NULL)) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    AsyncIoCallback,
                    SetExtCsdReq,
                    &SetExtCsdReq->Event
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  } else {
    SetExtCsdReq->Event = NULL;
  }

  Status = PassThru->PassThru (PassThru, Device->Slot, &SetExtCsdReq->Packet, SetExtCsdReq->Event);

Error:
  if ((Token != NULL) && (Token->Event != NULL)) {
    //
    // For asynchronous operation, only free request and event in error case.
    // The request and event will be freed in asynchronous callback for success case.
    //
    if (EFI_ERROR (Status) && (SetExtCsdReq != NULL)) {
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&SetExtCsdReq->Link);
      gBS->RestoreTPL (OldTpl);
      if (SetExtCsdReq->Event != NULL) {
        gBS->CloseEvent (SetExtCsdReq->Event);
      }
      FreePool (SetExtCsdReq);
    }
  } else {
    //
    // For synchronous operation, free request whatever the execution result is.
    //
    if (SetExtCsdReq != NULL) {
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&SetExtCsdReq->Link);
      gBS->RestoreTPL (OldTpl);
      FreePool (SetExtCsdReq);
    }
  }

  return Status;
}

/**
  Set the number of blocks for a block read/write cmd through sync or async I/O request.

  @param[in]  Partition         A pointer to the EMMC_PARTITION instance.
  @param[in]  BlockNum          The number of blocks for transfer.
  @param[in]  Token             A pointer to the token associated with the transaction.
  @param[in]  IsEnd             A boolean to show whether it's the last cmd in a series of cmds.
                                This parameter is only meaningful in async I/O request.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcSetBlkCount (
  IN  EMMC_PARTITION            *Partition,
  IN  UINT16                    BlockNum,
  IN  EFI_BLOCK_IO2_TOKEN       *Token,
  IN  BOOLEAN                   IsEnd
  )
{
  EFI_STATUS                    Status;
  EMMC_DEVICE                   *Device;
  EMMC_REQUEST                  *SetBlkCntReq;
  EFI_SD_MMC_PASS_THRU_PROTOCOL *PassThru;
  EFI_TPL                       OldTpl;

  SetBlkCntReq = NULL;

  Device   = Partition->Device;
  PassThru = Device->Private->PassThru;

  SetBlkCntReq = AllocateZeroPool (sizeof (EMMC_REQUEST));
  if (SetBlkCntReq == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  SetBlkCntReq->Signature = EMMC_REQUEST_SIGNATURE;
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&Partition->Queue, &SetBlkCntReq->Link);
  gBS->RestoreTPL (OldTpl);
  SetBlkCntReq->Packet.SdMmcCmdBlk    = &SetBlkCntReq->SdMmcCmdBlk;
  SetBlkCntReq->Packet.SdMmcStatusBlk = &SetBlkCntReq->SdMmcStatusBlk;
  SetBlkCntReq->Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  SetBlkCntReq->SdMmcCmdBlk.CommandIndex = EMMC_SET_BLOCK_COUNT;
  SetBlkCntReq->SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  SetBlkCntReq->SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  SetBlkCntReq->SdMmcCmdBlk.CommandArgument = BlockNum;

  SetBlkCntReq->IsEnd = IsEnd;
  SetBlkCntReq->Token = Token;

  if ((Token != NULL) && (Token->Event != NULL)) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    AsyncIoCallback,
                    SetBlkCntReq,
                    &SetBlkCntReq->Event
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  } else {
    SetBlkCntReq->Event = NULL;
  }

  Status = PassThru->PassThru (PassThru, Device->Slot, &SetBlkCntReq->Packet, SetBlkCntReq->Event);

Error:
  if ((Token != NULL) && (Token->Event != NULL)) {
    //
    // For asynchronous operation, only free request and event in error case.
    // The request and event will be freed in asynchronous callback for success case.
    //
    if (EFI_ERROR (Status) && (SetBlkCntReq != NULL)) {
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&SetBlkCntReq->Link);
      gBS->RestoreTPL (OldTpl);
      if (SetBlkCntReq->Event != NULL) {
        gBS->CloseEvent (SetBlkCntReq->Event);
      }
      FreePool (SetBlkCntReq);
    }
  } else {
    //
    // For synchronous operation, free request whatever the execution result is.
    //
    if (SetBlkCntReq != NULL) {
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&SetBlkCntReq->Link);
      gBS->RestoreTPL (OldTpl);
      FreePool (SetBlkCntReq);
    }
  }

  return Status;
}

/**
  Read blocks through security protocol cmds with the way of sync or async I/O request.

  @param[in]  Partition                    A pointer to the EMMC_PARTITION instance.
  @param[in]  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                           the security protocol command to be sent.
  @param[in]  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                           of the security protocol command to be sent.
  @param[in]  PayloadBufferSize            Size in bytes of the payload data buffer.
  @param[out] PayloadBuffer                A pointer to a destination buffer to store the security
                                           protocol command specific payload data for the security
                                           protocol command. The caller is responsible for having
                                           either implicit or explicit ownership of the buffer.
  @param[in]  IsRead                       Indicates it is a read or write operation.
  @param[in]  Timeout                      The timeout value, in 100ns units.
  @param[in]  Token                        A pointer to the token associated with the transaction.
  @param[in]  IsEnd                        A boolean to show whether it's the last cmd in a series of cmds.
                                           This parameter is only meaningful in async I/O request.

  @retval EFI_SUCCESS                      The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES             The request could not be executed due to a lack of resources.
  @retval Others                           The request could not be executed successfully.

**/
EFI_STATUS
EmmcProtocolInOut (
  IN     EMMC_PARTITION            *Partition,
  IN     UINT8                     SecurityProtocol,
  IN     UINT16                    SecurityProtocolSpecificData,
  IN     UINTN                     PayloadBufferSize,
     OUT VOID                      *PayloadBuffer,
  IN     BOOLEAN                   IsRead,
  IN     UINT64                    Timeout,
  IN     EFI_BLOCK_IO2_TOKEN       *Token,
  IN     BOOLEAN                   IsEnd
  )
{
  EFI_STATUS                    Status;
  EMMC_DEVICE                   *Device;
  EMMC_REQUEST                  *ProtocolReq;
  EFI_SD_MMC_PASS_THRU_PROTOCOL *PassThru;
  EFI_TPL                       OldTpl;

  ProtocolReq = NULL;

  Device   = Partition->Device;
  PassThru = Device->Private->PassThru;

  ProtocolReq = AllocateZeroPool (sizeof (EMMC_REQUEST));
  if (ProtocolReq == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  ProtocolReq->Signature = EMMC_REQUEST_SIGNATURE;
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&Partition->Queue, &ProtocolReq->Link);
  gBS->RestoreTPL (OldTpl);
  ProtocolReq->Packet.SdMmcCmdBlk    = &ProtocolReq->SdMmcCmdBlk;
  ProtocolReq->Packet.SdMmcStatusBlk = &ProtocolReq->SdMmcStatusBlk;

  if (IsRead) {
    ProtocolReq->Packet.InDataBuffer     = PayloadBuffer;
    ProtocolReq->Packet.InTransferLength = (UINT32)PayloadBufferSize;

    ProtocolReq->SdMmcCmdBlk.CommandIndex = EMMC_PROTOCOL_RD;
    ProtocolReq->SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
    ProtocolReq->SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  } else {
    ProtocolReq->Packet.OutDataBuffer     = PayloadBuffer;
    ProtocolReq->Packet.OutTransferLength = (UINT32)PayloadBufferSize;

    ProtocolReq->SdMmcCmdBlk.CommandIndex = EMMC_PROTOCOL_WR;
    ProtocolReq->SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
    ProtocolReq->SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  }

  ProtocolReq->SdMmcCmdBlk.CommandArgument = (SecurityProtocol << 8) | (SecurityProtocolSpecificData << 16);
  //
  // Convert to 1 microsecond unit.
  //
  ProtocolReq->Packet.Timeout = DivU64x32 (Timeout, 10) + 1;

  ProtocolReq->IsEnd = IsEnd;
  ProtocolReq->Token = Token;

  if ((Token != NULL) && (Token->Event != NULL)) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    AsyncIoCallback,
                    ProtocolReq,
                    &ProtocolReq->Event
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  } else {
    ProtocolReq->Event = NULL;
  }

  Status = PassThru->PassThru (PassThru, Device->Slot, &ProtocolReq->Packet, ProtocolReq->Event);

Error:
  if ((Token != NULL) && (Token->Event != NULL)) {
    //
    // For asynchronous operation, only free request and event in error case.
    // The request and event will be freed in asynchronous callback for success case.
    //
    if (EFI_ERROR (Status) && (ProtocolReq != NULL)) {
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&ProtocolReq->Link);
      gBS->RestoreTPL (OldTpl);
      if (ProtocolReq->Event != NULL) {
        gBS->CloseEvent (ProtocolReq->Event);
      }
      FreePool (ProtocolReq);
    }
  } else {
    //
    // For synchronous operation, free request whatever the execution result is.
    //
    if (ProtocolReq != NULL) {
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&ProtocolReq->Link);
      gBS->RestoreTPL (OldTpl);
      FreePool (ProtocolReq);
    }
  }

  return Status;
}

/**
  Read/write multiple blocks through sync or async I/O request.

  @param[in]  Partition         A pointer to the EMMC_PARTITION instance.
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
EmmcRwMultiBlocks (
  IN  EMMC_PARTITION            *Partition,
  IN  EFI_LBA                   Lba,
  IN  VOID                      *Buffer,
  IN  UINTN                     BufferSize,
  IN  BOOLEAN                   IsRead,
  IN  EFI_BLOCK_IO2_TOKEN       *Token,
  IN  BOOLEAN                   IsEnd
  )
{
  EFI_STATUS                    Status;
  EMMC_DEVICE                   *Device;
  EMMC_REQUEST                  *RwMultiBlkReq;
  EFI_SD_MMC_PASS_THRU_PROTOCOL *PassThru;
  EFI_TPL                       OldTpl;

  RwMultiBlkReq = NULL;

  Device   = Partition->Device;
  PassThru = Device->Private->PassThru;

  RwMultiBlkReq = AllocateZeroPool (sizeof (EMMC_REQUEST));
  if (RwMultiBlkReq == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  RwMultiBlkReq->Signature = EMMC_REQUEST_SIGNATURE;
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&Partition->Queue, &RwMultiBlkReq->Link);
  gBS->RestoreTPL (OldTpl);
  RwMultiBlkReq->Packet.SdMmcCmdBlk    = &RwMultiBlkReq->SdMmcCmdBlk;
  RwMultiBlkReq->Packet.SdMmcStatusBlk = &RwMultiBlkReq->SdMmcStatusBlk;
  //
  // Calculate timeout value through the below formula.
  // Timeout = (transfer size) / (2MB/s).
  // Taking 2MB/s as divisor is because it's nearest to the eMMC lowest
  // transfer speed (2.4MB/s).
  // Refer to eMMC 5.0 spec section 6.9.1 for details.
  //
  RwMultiBlkReq->Packet.Timeout = (BufferSize / (2 * 1024 * 1024) + 1) * 1000 * 1000;

  if (IsRead) {
    RwMultiBlkReq->Packet.InDataBuffer     = Buffer;
    RwMultiBlkReq->Packet.InTransferLength = (UINT32)BufferSize;

    RwMultiBlkReq->SdMmcCmdBlk.CommandIndex = EMMC_READ_MULTIPLE_BLOCK;
    RwMultiBlkReq->SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
    RwMultiBlkReq->SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  } else {
    RwMultiBlkReq->Packet.OutDataBuffer     = Buffer;
    RwMultiBlkReq->Packet.OutTransferLength = (UINT32)BufferSize;

    RwMultiBlkReq->SdMmcCmdBlk.CommandIndex = EMMC_WRITE_MULTIPLE_BLOCK;
    RwMultiBlkReq->SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAdtc;
    RwMultiBlkReq->SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;
  }

  if (Partition->Device->SectorAddressing) {
    RwMultiBlkReq->SdMmcCmdBlk.CommandArgument = (UINT32)Lba;
  } else {
    RwMultiBlkReq->SdMmcCmdBlk.CommandArgument = (UINT32)MultU64x32 (Lba, Partition->BlockMedia.BlockSize);
  }

  RwMultiBlkReq->IsEnd = IsEnd;
  RwMultiBlkReq->Token = Token;

  if ((Token != NULL) && (Token->Event != NULL)) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
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
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&RwMultiBlkReq->Link);
      gBS->RestoreTPL (OldTpl);
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
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&RwMultiBlkReq->Link);
      gBS->RestoreTPL (OldTpl);
      FreePool (RwMultiBlkReq);
    }
  }

  return Status;
}

/**
  This function transfers data from/to EMMC device.

  @param[in]       Partition    A pointer to the EMMC_PARTITION instance.
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
  @retval EFI_MEDIA_CHANGED     The MediaId does not match the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read/write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EmmcReadWrite (
  IN     EMMC_PARTITION                 *Partition,
  IN     UINT32                         MediaId,
  IN     EFI_LBA                        Lba,
  IN OUT VOID                           *Buffer,
  IN     UINTN                          BufferSize,
  IN     BOOLEAN                        IsRead,
  IN OUT EFI_BLOCK_IO2_TOKEN            *Token
  )
{
  EFI_STATUS                            Status;
  EMMC_DEVICE                           *Device;
  EFI_BLOCK_IO_MEDIA                    *Media;
  UINTN                                 BlockSize;
  UINTN                                 BlockNum;
  UINTN                                 IoAlign;
  UINT8                                 PartitionConfig;
  UINTN                                 Remaining;
  UINT32                                MaxBlock;
  BOOLEAN                               LastRw;

  Status = EFI_SUCCESS;
  Device = Partition->Device;
  Media  = &Partition->BlockMedia;
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
  // Check if needs to switch partition access.
  //
  PartitionConfig = Device->ExtCsd.PartitionConfig;
  if ((PartitionConfig & 0x7) != Partition->PartitionType) {
    PartitionConfig &= (UINT8)~0x7;
    PartitionConfig |= Partition->PartitionType;
    Status = EmmcSetExtCsd (Partition, OFFSET_OF (EMMC_EXT_CSD, PartitionConfig), PartitionConfig, Token, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Device->ExtCsd.PartitionConfig = PartitionConfig;
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
    Status = EmmcSetBlkCount (Partition, (UINT16)BlockNum, Token, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    BufferSize = BlockNum * BlockSize;
    Status = EmmcRwMultiBlocks (Partition, Lba, Buffer, BufferSize, IsRead, Token, LastRw);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    DEBUG ((DEBUG_BLKIO,
      "Emmc%a(): Part %d Lba 0x%x BlkNo 0x%x Event %p with %r\n",
      IsRead ? "Read " : "Write", Partition->PartitionType, Lba, BlockNum,
      (Token != NULL) ? Token->Event : NULL, Status));

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
EmmcReset (
  IN  EFI_BLOCK_IO_PROTOCOL     *This,
  IN  BOOLEAN                   ExtendedVerification
  )
{
  EFI_STATUS                    Status;
  EMMC_PARTITION                *Partition;
  EFI_SD_MMC_PASS_THRU_PROTOCOL *PassThru;

  Partition = EMMC_PARTITION_DATA_FROM_BLKIO (This);

  PassThru = Partition->Device->Private->PassThru;
  Status   = PassThru->ResetDevice (PassThru, Partition->Device->Slot);
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
  @retval EFI_MEDIA_CHANGED     The MediaId does not match the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
EmmcReadBlocks (
  IN     EFI_BLOCK_IO_PROTOCOL  *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN     UINTN                  BufferSize,
     OUT VOID                   *Buffer
  )
{
  EFI_STATUS             Status;
  EMMC_PARTITION         *Partition;

  Partition = EMMC_PARTITION_DATA_FROM_BLKIO (This);

  Status = EmmcReadWrite (Partition, MediaId, Lba, Buffer, BufferSize, TRUE, NULL);
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
  @retval EFI_MEDIA_CHANGED     The MediaId does not match the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
EmmcWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  )
{
  EFI_STATUS             Status;
  EMMC_PARTITION         *Partition;

  Partition = EMMC_PARTITION_DATA_FROM_BLKIO (This);

  Status = EmmcReadWrite (Partition, MediaId, Lba, Buffer, BufferSize, FALSE, NULL);
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
EmmcFlushBlocks (
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
EmmcResetEx (
  IN  EFI_BLOCK_IO2_PROTOCOL  *This,
  IN  BOOLEAN                 ExtendedVerification
  )
{
  EMMC_PARTITION              *Partition;
  LIST_ENTRY                  *Link;
  LIST_ENTRY                  *NextLink;
  EMMC_REQUEST                *Request;
  EFI_TPL                     OldTpl;

  Partition = EMMC_PARTITION_DATA_FROM_BLKIO2 (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  for (Link = GetFirstNode (&Partition->Queue);
       !IsNull (&Partition->Queue, Link);
       Link = NextLink) {
    NextLink = GetNextNode (&Partition->Queue, Link);
    RemoveEntryList (Link);

    Request = EMMC_REQUEST_FROM_LINK (Link);

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
EmmcReadBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
     OUT VOID                   *Buffer
  )
{
  EFI_STATUS             Status;
  EMMC_PARTITION         *Partition;

  Partition = EMMC_PARTITION_DATA_FROM_BLKIO2 (This);

  Status = EmmcReadWrite (Partition, MediaId, Lba, Buffer, BufferSize, TRUE, Token);
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
  @retval EFI_MEDIA_CHANGED     The MediaId does not match the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
EmmcWriteBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
  IN     VOID                   *Buffer
  )
{
  EFI_STATUS             Status;
  EMMC_PARTITION         *Partition;

  Partition = EMMC_PARTITION_DATA_FROM_BLKIO2 (This);

  Status = EmmcReadWrite (Partition, MediaId, Lba, Buffer, BufferSize, FALSE, Token);
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
EmmcFlushBlocksEx (
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

/**
  Send a security protocol command to a device that receives data and/or the result
  of one or more commands sent by SendData.

  The ReceiveData function sends a security protocol command to the given MediaId.
  The security protocol command sent is defined by SecurityProtocolId and contains
  the security protocol specific data SecurityProtocolSpecificData. The function
  returns the data from the security protocol command in PayloadBuffer.

  For devices supporting the SCSI command set, the security protocol command is sent
  using the SECURITY PROTOCOL IN command defined in SPC-4.

  For devices supporting the ATA command set, the security protocol command is sent
  using one of the TRUSTED RECEIVE commands defined in ATA8-ACS if PayloadBufferSize
  is non-zero.

  If the PayloadBufferSize is zero, the security protocol command is sent using the
  Trusted Non-Data command defined in ATA8-ACS.

  If PayloadBufferSize is too small to store the available data from the security
  protocol command, the function shall copy PayloadBufferSize bytes into the
  PayloadBuffer and return EFI_WARN_BUFFER_TOO_SMALL.

  If PayloadBuffer or PayloadTransferSize is NULL and PayloadBufferSize is non-zero,
  the function shall return EFI_INVALID_PARAMETER.

  If the given MediaId does not support security protocol commands, the function shall
  return EFI_UNSUPPORTED. If there is no media in the device, the function returns
  EFI_NO_MEDIA. If the MediaId is not the ID for the current media in the device,
  the function returns EFI_MEDIA_CHANGED.

  If the security protocol fails to complete within the Timeout period, the function
  shall return EFI_TIMEOUT.

  If the security protocol command completes without an error, the function shall
  return EFI_SUCCESS. If the security protocol command completes with an error, the
  function shall return EFI_DEVICE_ERROR.

  @param[in]  This                         Indicates a pointer to the calling context.
  @param[in]  MediaId                      ID of the medium to receive data from.
  @param[in]  Timeout                      The timeout, in 100ns units, to use for the execution
                                           of the security protocol command. A Timeout value of 0
                                           means that this function will wait indefinitely for the
                                           security protocol command to execute. If Timeout is greater
                                           than zero, then this function will return EFI_TIMEOUT
                                           if the time required to execute the receive data command
                                           is greater than Timeout.
  @param[in]  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                           the security protocol command to be sent.
  @param[in]  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                           of the security protocol command to be sent.
  @param[in]  PayloadBufferSize            Size in bytes of the payload data buffer.
  @param[out] PayloadBuffer                A pointer to a destination buffer to store the security
                                           protocol command specific payload data for the security
                                           protocol command. The caller is responsible for having
                                           either implicit or explicit ownership of the buffer.
  @param[out] PayloadTransferSize          A pointer to a buffer to store the size in bytes of the
                                           data written to the payload data buffer.
  @param[in]  IsRead                       Indicates it is a read or write operation.

  @retval EFI_SUCCESS                  The security protocol command completed successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL    The PayloadBufferSize was too small to store the available
                                       data from the device. The PayloadBuffer contains the truncated data.
  @retval EFI_UNSUPPORTED              The given MediaId does not support security protocol commands.
  @retval EFI_DEVICE_ERROR             The security protocol command completed with an error.
  @retval EFI_NO_MEDIA                 There is no media in the device.
  @retval EFI_MEDIA_CHANGED            The MediaId is not for the current media.
  @retval EFI_INVALID_PARAMETER        The PayloadBuffer or PayloadTransferSize is NULL and
                                       PayloadBufferSize is non-zero.
  @retval EFI_TIMEOUT                  A timeout occurred while waiting for the security
                                       protocol command to execute.

**/
EFI_STATUS
EFIAPI
EmmcSecurityProtocolInOut (
  IN     EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *This,
  IN     UINT32                                   MediaId,
  IN     UINT64                                   Timeout,
  IN     UINT8                                    SecurityProtocolId,
  IN     UINT16                                   SecurityProtocolSpecificData,
  IN     UINTN                                    PayloadBufferSize,
     OUT VOID                                     *PayloadBuffer,
     OUT UINTN                                    *PayloadTransferSize,
  IN     BOOLEAN                                  IsRead
  )
{
  EFI_STATUS                       Status;
  EMMC_PARTITION                   *Partition;
  EMMC_DEVICE                      *Device;
  EFI_BLOCK_IO_MEDIA               *Media;
  UINTN                            BlockSize;
  UINTN                            BlockNum;
  UINTN                            IoAlign;
  UINTN                            Remaining;
  UINT32                           MaxBlock;
  UINT8                            PartitionConfig;

  Status    = EFI_SUCCESS;
  Partition = EMMC_PARTITION_DATA_FROM_SSP (This);
  Device    = Partition->Device;
  Media     = &Partition->BlockMedia;

  if (PayloadTransferSize != NULL) {
    *PayloadTransferSize = 0;
  }

  if ((PayloadBuffer == NULL) && (PayloadBufferSize != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (PayloadBufferSize == 0) {
    return EFI_SUCCESS;
  }

  BlockSize = Media->BlockSize;
  if ((PayloadBufferSize % BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  BlockNum  = PayloadBufferSize / BlockSize;

  IoAlign = Media->IoAlign;
  if (IoAlign > 0 && (((UINTN) PayloadBuffer & (IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Security protocol interface is synchronous transfer.
  // Waiting for async I/O list to be empty before any operation.
  //
  while (!IsListEmpty (&Partition->Queue));

  //
  // Check if needs to switch partition access.
  //
  PartitionConfig = Device->ExtCsd.PartitionConfig;
  if ((PartitionConfig & 0x7) != Partition->PartitionType) {
    PartitionConfig &= (UINT8)~0x7;
    PartitionConfig |= Partition->PartitionType;
    Status = EmmcSetExtCsd (Partition, OFFSET_OF (EMMC_EXT_CSD, PartitionConfig), PartitionConfig, NULL, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Device->ExtCsd.PartitionConfig = PartitionConfig;
  }
  //
  // Start to execute data transfer. The max block number in single cmd is 65535 blocks.
  //
  Remaining = BlockNum;
  MaxBlock  = 0xFFFF;

  while (Remaining > 0) {
    if (Remaining <= MaxBlock) {
      BlockNum = Remaining;
    } else {
      BlockNum = MaxBlock;
    }

    Status = EmmcSetBlkCount (Partition, (UINT16)BlockNum, NULL, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    PayloadBufferSize = BlockNum * BlockSize;
    Status = EmmcProtocolInOut (Partition, SecurityProtocolId, SecurityProtocolSpecificData, PayloadBufferSize, PayloadBuffer, IsRead, Timeout, NULL, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    PayloadBuffer = (UINT8*)PayloadBuffer + PayloadBufferSize;
    Remaining    -= BlockNum;
    if (PayloadTransferSize != NULL) {
      *PayloadTransferSize += PayloadBufferSize;
    }
  }

  return Status;
}

/**
  Send a security protocol command to a device that receives data and/or the result
  of one or more commands sent by SendData.

  The ReceiveData function sends a security protocol command to the given MediaId.
  The security protocol command sent is defined by SecurityProtocolId and contains
  the security protocol specific data SecurityProtocolSpecificData. The function
  returns the data from the security protocol command in PayloadBuffer.

  For devices supporting the SCSI command set, the security protocol command is sent
  using the SECURITY PROTOCOL IN command defined in SPC-4.

  For devices supporting the ATA command set, the security protocol command is sent
  using one of the TRUSTED RECEIVE commands defined in ATA8-ACS if PayloadBufferSize
  is non-zero.

  If the PayloadBufferSize is zero, the security protocol command is sent using the
  Trusted Non-Data command defined in ATA8-ACS.

  If PayloadBufferSize is too small to store the available data from the security
  protocol command, the function shall copy PayloadBufferSize bytes into the
  PayloadBuffer and return EFI_WARN_BUFFER_TOO_SMALL.

  If PayloadBuffer or PayloadTransferSize is NULL and PayloadBufferSize is non-zero,
  the function shall return EFI_INVALID_PARAMETER.

  If the given MediaId does not support security protocol commands, the function shall
  return EFI_UNSUPPORTED. If there is no media in the device, the function returns
  EFI_NO_MEDIA. If the MediaId is not the ID for the current media in the device,
  the function returns EFI_MEDIA_CHANGED.

  If the security protocol fails to complete within the Timeout period, the function
  shall return EFI_TIMEOUT.

  If the security protocol command completes without an error, the function shall
  return EFI_SUCCESS. If the security protocol command completes with an error, the
  function shall return EFI_DEVICE_ERROR.

  @param  This                         Indicates a pointer to the calling context.
  @param  MediaId                      ID of the medium to receive data from.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT
                                       if the time required to execute the receive data command
                                       is greater than Timeout.
  @param  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                       the security protocol command to be sent.
  @param  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                       of the security protocol command to be sent.
  @param  PayloadBufferSize            Size in bytes of the payload data buffer.
  @param  PayloadBuffer                A pointer to a destination buffer to store the security
                                       protocol command specific payload data for the security
                                       protocol command. The caller is responsible for having
                                       either implicit or explicit ownership of the buffer.
  @param  PayloadTransferSize          A pointer to a buffer to store the size in bytes of the
                                       data written to the payload data buffer.

  @retval EFI_SUCCESS                  The security protocol command completed successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL    The PayloadBufferSize was too small to store the available
                                       data from the device. The PayloadBuffer contains the truncated data.
  @retval EFI_UNSUPPORTED              The given MediaId does not support security protocol commands.
  @retval EFI_DEVICE_ERROR             The security protocol command completed with an error.
  @retval EFI_NO_MEDIA                 There is no media in the device.
  @retval EFI_MEDIA_CHANGED            The MediaId is not for the current media.
  @retval EFI_INVALID_PARAMETER        The PayloadBuffer or PayloadTransferSize is NULL and
                                       PayloadBufferSize is non-zero.
  @retval EFI_TIMEOUT                  A timeout occurred while waiting for the security
                                       protocol command to execute.

**/
EFI_STATUS
EFIAPI
EmmcSecurityProtocolIn (
  IN EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *This,
  IN UINT32                                   MediaId,
  IN UINT64                                   Timeout,
  IN UINT8                                    SecurityProtocolId,
  IN UINT16                                   SecurityProtocolSpecificData,
  IN UINTN                                    PayloadBufferSize,
  OUT VOID                                    *PayloadBuffer,
  OUT UINTN                                   *PayloadTransferSize
  )
{
  EFI_STATUS                  Status;

  if ((PayloadTransferSize == NULL) && PayloadBufferSize != 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EmmcSecurityProtocolInOut (
             This,
             MediaId,
             Timeout,
             SecurityProtocolId,
             SecurityProtocolSpecificData,
             PayloadBufferSize,
             PayloadBuffer,
             PayloadTransferSize,
             TRUE
             );

  return Status;
}

/**
  Send a security protocol command to a device.

  The SendData function sends a security protocol command containing the payload
  PayloadBuffer to the given MediaId. The security protocol command sent is
  defined by SecurityProtocolId and contains the security protocol specific data
  SecurityProtocolSpecificData. If the underlying protocol command requires a
  specific padding for the command payload, the SendData function shall add padding
  bytes to the command payload to satisfy the padding requirements.

  For devices supporting the SCSI command set, the security protocol command is sent
  using the SECURITY PROTOCOL OUT command defined in SPC-4.

  For devices supporting the ATA command set, the security protocol command is sent
  using one of the TRUSTED SEND commands defined in ATA8-ACS if PayloadBufferSize
  is non-zero. If the PayloadBufferSize is zero, the security protocol command is
  sent using the Trusted Non-Data command defined in ATA8-ACS.

  If PayloadBuffer is NULL and PayloadBufferSize is non-zero, the function shall
  return EFI_INVALID_PARAMETER.

  If the given MediaId does not support security protocol commands, the function
  shall return EFI_UNSUPPORTED. If there is no media in the device, the function
  returns EFI_NO_MEDIA. If the MediaId is not the ID for the current media in the
  device, the function returns EFI_MEDIA_CHANGED.

  If the security protocol fails to complete within the Timeout period, the function
  shall return EFI_TIMEOUT.

  If the security protocol command completes without an error, the function shall return
  EFI_SUCCESS. If the security protocol command completes with an error, the function
  shall return EFI_DEVICE_ERROR.

  @param  This                         Indicates a pointer to the calling context.
  @param  MediaId                      ID of the medium to receive data from.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT
                                       if the time required to execute the receive data command
                                       is greater than Timeout.
  @param  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                       the security protocol command to be sent.
  @param  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                       of the security protocol command to be sent.
  @param  PayloadBufferSize            Size in bytes of the payload data buffer.
  @param  PayloadBuffer                A pointer to a destination buffer to store the security
                                       protocol command specific payload data for the security
                                       protocol command.

  @retval EFI_SUCCESS                  The security protocol command completed successfully.
  @retval EFI_UNSUPPORTED              The given MediaId does not support security protocol commands.
  @retval EFI_DEVICE_ERROR             The security protocol command completed with an error.
  @retval EFI_NO_MEDIA                 There is no media in the device.
  @retval EFI_MEDIA_CHANGED            The MediaId is not for the current media.
  @retval EFI_INVALID_PARAMETER        The PayloadBuffer is NULL and PayloadBufferSize is non-zero.
  @retval EFI_TIMEOUT                  A timeout occurred while waiting for the security
                                       protocol command to execute.

**/
EFI_STATUS
EFIAPI
EmmcSecurityProtocolOut (
  IN EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *This,
  IN UINT32                                   MediaId,
  IN UINT64                                   Timeout,
  IN UINT8                                    SecurityProtocolId,
  IN UINT16                                   SecurityProtocolSpecificData,
  IN UINTN                                    PayloadBufferSize,
  IN VOID                                     *PayloadBuffer
  )
{
  EFI_STATUS          Status;

  Status = EmmcSecurityProtocolInOut (
             This,
             MediaId,
             Timeout,
             SecurityProtocolId,
             SecurityProtocolSpecificData,
             PayloadBufferSize,
             PayloadBuffer,
             NULL,
             FALSE
             );

  return Status;
}

/**
  Set the erase start address through sync or async I/O request.

  @param[in]  Partition         A pointer to the EMMC_PARTITION instance.
  @param[in]  StartLba          The starting logical block address to be erased.
  @param[in]  Token             A pointer to the token associated with the transaction.
  @param[in]  IsEnd             A boolean to show whether it's the last cmd in a series of cmds.
                                This parameter is only meaningful in async I/O request.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcEraseBlockStart (
  IN  EMMC_PARTITION            *Partition,
  IN  EFI_LBA                   StartLba,
  IN  EFI_BLOCK_IO2_TOKEN       *Token,
  IN  BOOLEAN                   IsEnd
  )
{
  EFI_STATUS                           Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL        *PassThru;
  EMMC_DEVICE                          *Device;
  EMMC_REQUEST                         *EraseBlockStart;
  EFI_TPL                              OldTpl;

  EraseBlockStart = NULL;

  Device   = Partition->Device;
  PassThru = Device->Private->PassThru;

  EraseBlockStart = AllocateZeroPool (sizeof (EMMC_REQUEST));
  if (EraseBlockStart == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  EraseBlockStart->Signature = EMMC_REQUEST_SIGNATURE;
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&Partition->Queue, &EraseBlockStart->Link);
  gBS->RestoreTPL (OldTpl);
  EraseBlockStart->Packet.SdMmcCmdBlk    = &EraseBlockStart->SdMmcCmdBlk;
  EraseBlockStart->Packet.SdMmcStatusBlk = &EraseBlockStart->SdMmcStatusBlk;
  EraseBlockStart->Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  EraseBlockStart->SdMmcCmdBlk.CommandIndex = EMMC_ERASE_GROUP_START;
  EraseBlockStart->SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  EraseBlockStart->SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;

  if (Device->SectorAddressing) {
    EraseBlockStart->SdMmcCmdBlk.CommandArgument = (UINT32)StartLba;
  } else {
    EraseBlockStart->SdMmcCmdBlk.CommandArgument = (UINT32)MultU64x32 (StartLba, Partition->BlockMedia.BlockSize);
  }

  EraseBlockStart->IsEnd = IsEnd;
  EraseBlockStart->Token = Token;

  if ((Token != NULL) && (Token->Event != NULL)) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    AsyncIoCallback,
                    EraseBlockStart,
                    &EraseBlockStart->Event
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  } else {
    EraseBlockStart->Event = NULL;
  }

  Status = PassThru->PassThru (PassThru, Device->Slot, &EraseBlockStart->Packet, EraseBlockStart->Event);

Error:
  if ((Token != NULL) && (Token->Event != NULL)) {
    //
    // For asynchronous operation, only free request and event in error case.
    // The request and event will be freed in asynchronous callback for success case.
    //
    if (EFI_ERROR (Status) && (EraseBlockStart != NULL)) {
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&EraseBlockStart->Link);
      gBS->RestoreTPL (OldTpl);
      if (EraseBlockStart->Event != NULL) {
        gBS->CloseEvent (EraseBlockStart->Event);
      }
      FreePool (EraseBlockStart);
    }
  } else {
    //
    // For synchronous operation, free request whatever the execution result is.
    //
    if (EraseBlockStart != NULL) {
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&EraseBlockStart->Link);
      gBS->RestoreTPL (OldTpl);
      FreePool (EraseBlockStart);
    }
  }

  return Status;
}

/**
  Set the erase end address through sync or async I/O request.

  @param[in]  Partition         A pointer to the EMMC_PARTITION instance.
  @param[in]  EndLba            The ending logical block address to be erased.
  @param[in]  Token             A pointer to the token associated with the transaction.
  @param[in]  IsEnd             A boolean to show whether it's the last cmd in a series of cmds.
                                This parameter is only meaningful in async I/O request.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcEraseBlockEnd (
  IN  EMMC_PARTITION            *Partition,
  IN  EFI_LBA                   EndLba,
  IN  EFI_BLOCK_IO2_TOKEN       *Token,
  IN  BOOLEAN                   IsEnd
  )
{
  EFI_STATUS                           Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL        *PassThru;
  EMMC_DEVICE                          *Device;
  EMMC_REQUEST                         *EraseBlockEnd;
  EFI_TPL                              OldTpl;

  EraseBlockEnd = NULL;

  Device   = Partition->Device;
  PassThru = Device->Private->PassThru;

  EraseBlockEnd = AllocateZeroPool (sizeof (EMMC_REQUEST));
  if (EraseBlockEnd == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  EraseBlockEnd->Signature = EMMC_REQUEST_SIGNATURE;
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&Partition->Queue, &EraseBlockEnd->Link);
  gBS->RestoreTPL (OldTpl);
  EraseBlockEnd->Packet.SdMmcCmdBlk    = &EraseBlockEnd->SdMmcCmdBlk;
  EraseBlockEnd->Packet.SdMmcStatusBlk = &EraseBlockEnd->SdMmcStatusBlk;
  EraseBlockEnd->Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  EraseBlockEnd->SdMmcCmdBlk.CommandIndex = EMMC_ERASE_GROUP_END;
  EraseBlockEnd->SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  EraseBlockEnd->SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1;

  if (Device->SectorAddressing) {
    EraseBlockEnd->SdMmcCmdBlk.CommandArgument = (UINT32)EndLba;
  } else {
    EraseBlockEnd->SdMmcCmdBlk.CommandArgument = (UINT32)MultU64x32 (EndLba, Partition->BlockMedia.BlockSize);
  }

  EraseBlockEnd->IsEnd = IsEnd;
  EraseBlockEnd->Token = Token;

  if ((Token != NULL) && (Token->Event != NULL)) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    AsyncIoCallback,
                    EraseBlockEnd,
                    &EraseBlockEnd->Event
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  } else {
    EraseBlockEnd->Event = NULL;
  }

  Status = PassThru->PassThru (PassThru, Device->Slot, &EraseBlockEnd->Packet, EraseBlockEnd->Event);

Error:
  if ((Token != NULL) && (Token->Event != NULL)) {
    //
    // For asynchronous operation, only free request and event in error case.
    // The request and event will be freed in asynchronous callback for success case.
    //
    if (EFI_ERROR (Status) && (EraseBlockEnd != NULL)) {
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&EraseBlockEnd->Link);
      gBS->RestoreTPL (OldTpl);
      if (EraseBlockEnd->Event != NULL) {
        gBS->CloseEvent (EraseBlockEnd->Event);
      }
      FreePool (EraseBlockEnd);
    }
  } else {
    //
    // For synchronous operation, free request whatever the execution result is.
    //
    if (EraseBlockEnd != NULL) {
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&EraseBlockEnd->Link);
      gBS->RestoreTPL (OldTpl);
      FreePool (EraseBlockEnd);
    }
  }

  return Status;
}

/**
  Erase specified blocks through sync or async I/O request.

  @param[in]  Partition         A pointer to the EMMC_PARTITION instance.
  @param[in]  Token             A pointer to the token associated with the transaction.
  @param[in]  IsEnd             A boolean to show whether it's the last cmd in a series of cmds.
                                This parameter is only meaningful in async I/O request.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcEraseBlock (
  IN  EMMC_PARTITION            *Partition,
  IN  EFI_BLOCK_IO2_TOKEN       *Token,
  IN  BOOLEAN                   IsEnd
  )
{
  EFI_STATUS                           Status;
  EFI_SD_MMC_PASS_THRU_PROTOCOL        *PassThru;
  EMMC_DEVICE                          *Device;
  EMMC_REQUEST                         *EraseBlock;
  EFI_TPL                              OldTpl;

  EraseBlock = NULL;

  Device   = Partition->Device;
  PassThru = Device->Private->PassThru;

  EraseBlock = AllocateZeroPool (sizeof (EMMC_REQUEST));
  if (EraseBlock == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  EraseBlock->Signature = EMMC_REQUEST_SIGNATURE;
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&Partition->Queue, &EraseBlock->Link);
  gBS->RestoreTPL (OldTpl);
  EraseBlock->Packet.SdMmcCmdBlk    = &EraseBlock->SdMmcCmdBlk;
  EraseBlock->Packet.SdMmcStatusBlk = &EraseBlock->SdMmcStatusBlk;
  EraseBlock->Packet.Timeout        = EMMC_GENERIC_TIMEOUT;

  EraseBlock->SdMmcCmdBlk.CommandIndex = EMMC_ERASE;
  EraseBlock->SdMmcCmdBlk.CommandType  = SdMmcCommandTypeAc;
  EraseBlock->SdMmcCmdBlk.ResponseType = SdMmcResponseTypeR1b;
  if ((Device->ExtCsd.SecFeatureSupport & BIT4) != 0) {
    //
    // Perform a Trim operation which applies the erase operation to write blocks
    // instead of erase groups. (Spec JESD84-B51, eMMC Electrical Standard 5.1,
    // Section 6.6.10 and 6.10.4)
    //
    EraseBlock->SdMmcCmdBlk.CommandArgument = 1;
  }

  EraseBlock->IsEnd = IsEnd;
  EraseBlock->Token = Token;

  if ((Token != NULL) && (Token->Event != NULL)) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    AsyncIoCallback,
                    EraseBlock,
                    &EraseBlock->Event
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  } else {
    EraseBlock->Event = NULL;
  }

  Status = PassThru->PassThru (PassThru, Device->Slot, &EraseBlock->Packet, EraseBlock->Event);

Error:
  if ((Token != NULL) && (Token->Event != NULL)) {
    //
    // For asynchronous operation, only free request and event in error case.
    // The request and event will be freed in asynchronous callback for success case.
    //
    if (EFI_ERROR (Status) && (EraseBlock != NULL)) {
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&EraseBlock->Link);
      gBS->RestoreTPL (OldTpl);
      if (EraseBlock->Event != NULL) {
        gBS->CloseEvent (EraseBlock->Event);
      }
      FreePool (EraseBlock);
    }
  } else {
    //
    // For synchronous operation, free request whatever the execution result is.
    //
    if (EraseBlock != NULL) {
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      RemoveEntryList (&EraseBlock->Link);
      gBS->RestoreTPL (OldTpl);
      FreePool (EraseBlock);
    }
  }

  return Status;
}

/**
  Write zeros to specified blocks.

  @param[in]  Partition         A pointer to the EMMC_PARTITION instance.
  @param[in]  StartLba          The starting logical block address to write zeros.
  @param[in]  Size              The size in bytes to fill with zeros. This must be a multiple of
                                the physical block size of the device.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcWriteZeros (
  IN  EMMC_PARTITION            *Partition,
  IN  EFI_LBA                   StartLba,
  IN  UINTN                     Size
  )
{
  EFI_STATUS                           Status;
  UINT8                                *Buffer;
  UINT32                               MediaId;

  Buffer = AllocateZeroPool (Size);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MediaId = Partition->BlockMedia.MediaId;

  Status = EmmcReadWrite (Partition, MediaId, StartLba, Buffer, Size, FALSE, NULL);
  FreePool (Buffer);

  return Status;
}

/**
  Erase a specified number of device blocks.

  @param[in]       This           Indicates a pointer to the calling context.
  @param[in]       MediaId        The media ID that the erase request is for.
  @param[in]       Lba            The starting logical block address to be
                                  erased. The caller is responsible for erasing
                                  only legitimate locations.
  @param[in, out]  Token          A pointer to the token associated with the
                                  transaction.
  @param[in]       Size           The size in bytes to be erased. This must be
                                  a multiple of the physical block size of the
                                  device.

  @retval EFI_SUCCESS             The erase request was queued if Event is not
                                  NULL. The data was erased correctly to the
                                  device if the Event is NULL.to the device.
  @retval EFI_WRITE_PROTECTED     The device cannot be erased due to write
                                  protection.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the erase operation.
  @retval EFI_INVALID_PARAMETER   The erase request contains LBAs that are not
                                  valid.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.

**/
EFI_STATUS
EFIAPI
EmmcEraseBlocks (
  IN     EFI_ERASE_BLOCK_PROTOCOL      *This,
  IN     UINT32                        MediaId,
  IN     EFI_LBA                       Lba,
  IN OUT EFI_ERASE_BLOCK_TOKEN         *Token,
  IN     UINTN                         Size
  )
{
  EFI_STATUS                            Status;
  EFI_BLOCK_IO_MEDIA                    *Media;
  UINTN                                 BlockSize;
  UINTN                                 BlockNum;
  EFI_LBA                               FirstLba;
  EFI_LBA                               LastLba;
  EFI_LBA                               StartGroupLba;
  EFI_LBA                               EndGroupLba;
  UINT32                                EraseGroupSize;
  UINT32                                Remainder;
  UINTN                                 WriteZeroSize;
  UINT8                                 PartitionConfig;
  EMMC_PARTITION                        *Partition;
  EMMC_DEVICE                           *Device;

  Status    = EFI_SUCCESS;
  Partition = EMMC_PARTITION_DATA_FROM_ERASEBLK (This);
  Device    = Partition->Device;
  Media     = &Partition->BlockMedia;

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Media->ReadOnly) {
    return EFI_WRITE_PROTECTED;
  }

  //
  // Check parameters.
  //
  BlockSize = Media->BlockSize;
  if ((Size % BlockSize) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  BlockNum  = Size / BlockSize;
  if ((Lba + BlockNum - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Token != NULL) && (Token->Event != NULL)) {
    Token->TransactionStatus = EFI_SUCCESS;
  }

  FirstLba = Lba;
  LastLba  = Lba + BlockNum - 1;

  //
  // Check if needs to switch partition access.
  //
  PartitionConfig = Device->ExtCsd.PartitionConfig;
  if ((PartitionConfig & 0x7) != Partition->PartitionType) {
    PartitionConfig &= (UINT8)~0x7;
    PartitionConfig |= Partition->PartitionType;
    Status = EmmcSetExtCsd (Partition, OFFSET_OF (EMMC_EXT_CSD, PartitionConfig), PartitionConfig, (EFI_BLOCK_IO2_TOKEN*)Token, FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Device->ExtCsd.PartitionConfig = PartitionConfig;
  }

  if ((Device->ExtCsd.SecFeatureSupport & BIT4) == 0) {
    //
    // If the Trim operation is not supported by the device, handle the erase
    // of blocks that do not form a complete erase group separately.
    //
    EraseGroupSize = This->EraseLengthGranularity;

    DivU64x32Remainder (FirstLba, EraseGroupSize, &Remainder);
    StartGroupLba = (Remainder == 0) ? FirstLba : (FirstLba + EraseGroupSize - Remainder);

    DivU64x32Remainder (LastLba + 1, EraseGroupSize, &Remainder);
    EndGroupLba = LastLba + 1 - Remainder;

    //
    // If the size to erase is smaller than the erase group size, the whole
    // erase operation is performed by writing zeros.
    //
    if (BlockNum < EraseGroupSize) {
      Status = EmmcWriteZeros (Partition, FirstLba, Size);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      DEBUG ((
        DEBUG_INFO,
        "EmmcEraseBlocks(): Lba 0x%x BlkNo 0x%x Event %p with %r\n",
        Lba,
        BlockNum,
        (Token != NULL) ? Token->Event : NULL,
        Status
        ));

      if ((Token != NULL) && (Token->Event != NULL)) {
        Token->TransactionStatus = EFI_SUCCESS;
        gBS->SignalEvent (Token->Event);
      }
      return EFI_SUCCESS;
    }

    //
    // If the starting LBA to erase is not aligned with the start of an erase
    // group, write zeros to erase the data from starting LBA to the end of the
    // current erase group.
    //
    if (StartGroupLba > FirstLba) {
      WriteZeroSize = (UINTN)(StartGroupLba - FirstLba) * BlockSize;
      Status = EmmcWriteZeros (Partition, FirstLba, WriteZeroSize);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    //
    // If the ending LBA to erase is not aligned with the end of an erase
    // group, write zeros to erase the data from the start of the erase group
    // to the ending LBA.
    //
    if (EndGroupLba <= LastLba) {
      WriteZeroSize = (UINTN)(LastLba + 1 - EndGroupLba) * BlockSize;
      Status = EmmcWriteZeros (Partition, EndGroupLba, WriteZeroSize);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    //
    // Check whether there is erase group to erase.
    //
    if (EndGroupLba <= StartGroupLba) {
      DEBUG ((
        DEBUG_INFO,
        "EmmcEraseBlocks(): Lba 0x%x BlkNo 0x%x Event %p with %r\n",
        Lba,
        BlockNum,
        (Token != NULL) ? Token->Event : NULL,
        Status
        ));

      if ((Token != NULL) && (Token->Event != NULL)) {
        Token->TransactionStatus = EFI_SUCCESS;
        gBS->SignalEvent (Token->Event);
      }
      return EFI_SUCCESS;
    }

    FirstLba = StartGroupLba;
    LastLba  = EndGroupLba - 1;
  }

  Status = EmmcEraseBlockStart (Partition, FirstLba, (EFI_BLOCK_IO2_TOKEN*)Token, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EmmcEraseBlockEnd (Partition, LastLba, (EFI_BLOCK_IO2_TOKEN*)Token, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EmmcEraseBlock (Partition, (EFI_BLOCK_IO2_TOKEN*)Token, TRUE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "EmmcEraseBlocks(): Lba 0x%x BlkNo 0x%x Event %p with %r\n",
    Lba,
    BlockNum,
    (Token != NULL) ? Token->Event : NULL,
    Status
    ));

  return Status;
}


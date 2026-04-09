/** @file
  Mtftp6 support functions implementation.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Mtftp6Impl.h"

/**
  Allocate a MTFTP block range, then init it to the range of [Start, End].

  @param[in]  Start                  The start block number.
  @param[in]  End                    The last block number in the range.

  @return Range                      The range of the allocated block buffer.

**/
MTFTP6_BLOCK_RANGE *
Mtftp6AllocateRange (
  IN UINT16  Start,
  IN UINT16  End
  )
{
  MTFTP6_BLOCK_RANGE  *Range;

  Range = AllocateZeroPool (sizeof (MTFTP6_BLOCK_RANGE));

  if (Range == NULL) {
    return NULL;
  }

  InitializeListHead (&Range->Link);
  Range->Start = Start;
  Range->End   = End;
  Range->Bound = End;

  return Range;
}

/**
  Initialize the block range for either RRQ or WRQ. RRQ and WRQ have
  different requirements for Start and End. For example, during startup,
  WRQ initializes its whole valid block range to [0, 0xffff]. This
  is because the server will send an ACK0 to inform the user to start the
  upload. When the client receives an ACK0, it will remove 0 from the range,
  get the next block number, which is 1, then upload the BLOCK1. For RRQ
  without option negotiation, the server will directly send the BLOCK1
  in response to the client's RRQ. When received BLOCK1, the client will
  remove it from the block range and send an ACK. It also works if there
  is option negotiation.

  @param[in]  Head                   The block range head to initialize.
  @param[in]  Start                  The Start block number.
  @param[in]  End                    The last block number.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory for initial block range.
  @retval EFI_SUCCESS            The initial block range is created.

**/
EFI_STATUS
Mtftp6InitBlockRange (
  IN LIST_ENTRY  *Head,
  IN UINT16      Start,
  IN UINT16      End
  )
{
  MTFTP6_BLOCK_RANGE  *Range;

  Range = Mtftp6AllocateRange (Start, End);

  if (Range == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InsertTailList (Head, &Range->Link);
  return EFI_SUCCESS;
}

/**
  Get the first valid block number on the range list.

  @param[in]  Head                   The block range head.

  @retval     ==-1                   If the block range is empty.
  @retval     >-1                    The first valid block number.

**/
INTN
Mtftp6GetNextBlockNum (
  IN LIST_ENTRY  *Head
  )
{
  MTFTP6_BLOCK_RANGE  *Range;

  if (IsListEmpty (Head)) {
    return -1;
  }

  Range = NET_LIST_HEAD (Head, MTFTP6_BLOCK_RANGE, Link);
  return Range->Start;
}

/**
  Set the last block number of the block range list. It
  removes all the blocks after the Last. MTFTP initialize the
  block range to the maximum possible range, such as [0, 0xffff]
  for WRQ. When it gets the last block number, it calls
  this function to set the last block number.

  @param[in]  Head                   The block range list.
  @param[in]  Last                   The last block number.

**/
VOID
Mtftp6SetLastBlockNum (
  IN LIST_ENTRY  *Head,
  IN UINT16      Last
  )
{
  MTFTP6_BLOCK_RANGE  *Range;

  //
  // Iterate from the tail to head to remove the block number
  // after the last.
  //
  while (!IsListEmpty (Head)) {
    Range = NET_LIST_TAIL (Head, MTFTP6_BLOCK_RANGE, Link);

    if (Range->Start > Last) {
      RemoveEntryList (&Range->Link);
      FreePool (Range);
      continue;
    }

    if (Range->End > Last) {
      Range->End = Last;
    }

    return;
  }
}

/**
  Remove the block number from the block range list.

  @param[in]  Head                   The block range list to remove from.
  @param[in]  Num                    The block number to remove.
  @param[in]  Completed              Whether Num is the last block number.
  @param[out] BlockCounter           The continuous block counter instead of the value after roll-over.

  @retval EFI_NOT_FOUND          The block number isn't in the block range list.
  @retval EFI_SUCCESS            The block number has been removed from the list.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.

**/
EFI_STATUS
Mtftp6RemoveBlockNum (
  IN LIST_ENTRY  *Head,
  IN UINT16      Num,
  IN BOOLEAN     Completed,
  OUT UINT64     *BlockCounter
  )
{
  MTFTP6_BLOCK_RANGE  *Range;
  MTFTP6_BLOCK_RANGE  *NewRange;
  LIST_ENTRY          *Entry;

  NET_LIST_FOR_EACH (Entry, Head) {
    //
    // Each block represents a hole [Start, End] in the file,
    // skip to the first range with End >= Num
    //
    Range = NET_LIST_USER_STRUCT (Entry, MTFTP6_BLOCK_RANGE, Link);

    if (Range->End < Num) {
      continue;
    }

    //
    // There are three different cases for Start
    // 1. (Start > Num) && (End >= Num):
    //    because all the holes before this one has the condition of
    //    End < Num, so this block number has been removed.
    //
    // 2. (Start == Num) && (End >= Num):
    //    Need to increase the Start by one, and if End == Num, this
    //    hole has been removed completely, remove it.
    //
    // 3. (Start < Num) && (End >= Num):
    //    if End == Num, only need to decrease the End by one because
    //    we have (Start < Num) && (Num == End), so (Start <= End - 1).
    //    if (End > Num), the hold is split into two holes, with
    //    [Start, Num - 1] and [Num + 1, End].
    //
    if (Range->Start > Num) {
      return EFI_NOT_FOUND;
    } else if (Range->Start == Num) {
      Range->Start++;

      //
      // Note that: RFC 1350 does not mention block counter roll-over,
      // but several TFTP hosts implement the roll-over be able to accept
      // transfers of unlimited size. There is no consensus, however, whether
      // the counter should wrap around to zero or to one. Many implementations
      // wrap to zero, because this is the simplest to implement. Here we choose
      // this solution.
      //
      *BlockCounter = Num;

      if (Range->Round > 0) {
        *BlockCounter += Range->Bound +  MultU64x32 (Range->Round - 1, (UINT32)(Range->Bound + 1)) + 1;
      }

      if (Range->Start > Range->Bound) {
        Range->Start = 0;
        Range->Round++;
      }

      if ((Range->Start > Range->End) || Completed) {
        RemoveEntryList (&Range->Link);
        FreePool (Range);
      }

      return EFI_SUCCESS;
    } else {
      if (Range->End == Num) {
        Range->End--;
      } else {
        NewRange = Mtftp6AllocateRange ((UINT16)(Num + 1), (UINT16)Range->End);

        if (NewRange == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        Range->End = Num - 1;
        NetListInsertAfter (&Range->Link, &NewRange->Link);
      }

      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Configure the opened Udp6 instance until the corresponding Ip6 instance
  has been configured.

  @param[in]  UdpIo                  The pointer to the Udp6 Io.
  @param[in]  UdpCfgData             The pointer to the Udp6 configure data.

  @retval EFI_SUCCESS            Configure the Udp6 instance successfully.
  @retval EFI_NO_MAPPING         The corresponding Ip6 instance has not
                                 been configured yet.

**/
EFI_STATUS
Mtftp6GetMapping (
  IN UDP_IO                *UdpIo,
  IN EFI_UDP6_CONFIG_DATA  *UdpCfgData
  )
{
  EFI_IP6_MODE_DATA  Ip6Mode;
  EFI_UDP6_PROTOCOL  *Udp6;
  EFI_STATUS         Status;
  EFI_EVENT          Event;

  Event = NULL;
  Udp6  = UdpIo->Protocol.Udp6;

  //
  // Create a timer to check whether the Ip6 instance configured or not.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = gBS->SetTimer (
                  Event,
                  TimerRelative,
                  MTFTP6_GET_MAPPING_TIMEOUT * MTFTP6_TICK_PER_SECOND
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Check the Ip6 mode data till timeout.
  //
  while (EFI_ERROR (gBS->CheckEvent (Event))) {
    Udp6->Poll (Udp6);

    Status = Udp6->GetModeData (Udp6, NULL, &Ip6Mode, NULL, NULL);

    if (!EFI_ERROR (Status)) {
      if (Ip6Mode.AddressList != NULL) {
        FreePool (Ip6Mode.AddressList);
      }

      if (Ip6Mode.GroupTable != NULL) {
        FreePool (Ip6Mode.GroupTable);
      }

      if (Ip6Mode.RouteTable != NULL) {
        FreePool (Ip6Mode.RouteTable);
      }

      if (Ip6Mode.NeighborCache != NULL) {
        FreePool (Ip6Mode.NeighborCache);
      }

      if (Ip6Mode.PrefixTable != NULL) {
        FreePool (Ip6Mode.PrefixTable);
      }

      if (Ip6Mode.IcmpTypeList != NULL) {
        FreePool (Ip6Mode.IcmpTypeList);
      }

      if (Ip6Mode.IsConfigured) {
        //
        // Continue to configure the Udp6 instance.
        //
        Status = Udp6->Configure (Udp6, UdpCfgData);
      } else {
        Status = EFI_NO_MAPPING;
      }
    }
  }

ON_EXIT:

  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  return Status;
}

/**
  The dummy configure routine for create a new Udp6 Io.

  @param[in]  UdpIo                  The pointer to the Udp6 Io.
  @param[in]  Context                The pointer to the context.

  @retval EFI_SUCCESS                This value is always returned.

**/
EFI_STATUS
EFIAPI
Mtftp6ConfigDummyUdpIo (
  IN UDP_IO  *UdpIo,
  IN VOID    *Context
  )
{
  return EFI_SUCCESS;
}

/**
  The configure routine for Mtftp6 instance to transmit/receive.

  @param[in]  UdpIo                  The pointer to the Udp6 Io.
  @param[in]  ServerIp               The pointer to the server address.
  @param[in]  ServerPort             The pointer to the server port.
  @param[in]  LocalIp                The pointer to the local address.
  @param[in]  LocalPort              The pointer to the local port.

  @retval EFI_SUCCESS            Configured the Udp6 Io for Mtftp6 successfully.
  @retval EFI_NO_MAPPING         The corresponding Ip6 instance has not been
                                 configured yet.

**/
EFI_STATUS
Mtftp6ConfigUdpIo (
  IN UDP_IO            *UdpIo,
  IN EFI_IPv6_ADDRESS  *ServerIp,
  IN UINT16            ServerPort,
  IN EFI_IPv6_ADDRESS  *LocalIp,
  IN UINT16            LocalPort
  )
{
  EFI_STATUS            Status;
  EFI_UDP6_PROTOCOL     *Udp6;
  EFI_UDP6_CONFIG_DATA  *Udp6Cfg;

  Udp6    = UdpIo->Protocol.Udp6;
  Udp6Cfg = &(UdpIo->Config.Udp6);

  ZeroMem (Udp6Cfg, sizeof (EFI_UDP6_CONFIG_DATA));

  //
  // Set the Udp6 Io configure data.
  //
  Udp6Cfg->AcceptPromiscuous  = FALSE;
  Udp6Cfg->AcceptAnyPort      = FALSE;
  Udp6Cfg->AllowDuplicatePort = FALSE;
  Udp6Cfg->TrafficClass       = 0;
  Udp6Cfg->HopLimit           = 128;
  Udp6Cfg->ReceiveTimeout     = 0;
  Udp6Cfg->TransmitTimeout    = 0;
  Udp6Cfg->StationPort        = LocalPort;
  Udp6Cfg->RemotePort         = ServerPort;

  CopyMem (
    &Udp6Cfg->StationAddress,
    LocalIp,
    sizeof (EFI_IPv6_ADDRESS)
    );

  CopyMem (
    &Udp6Cfg->RemoteAddress,
    ServerIp,
    sizeof (EFI_IPv6_ADDRESS)
    );

  //
  // Configure the Udp6 instance with current configure data.
  //
  Status = Udp6->Configure (Udp6, Udp6Cfg);

  if (Status == EFI_NO_MAPPING) {
    return Mtftp6GetMapping (UdpIo, Udp6Cfg);
  }

  return Status;
}

/**
  Build and transmit the request packet for the Mtftp6 instance.

  @param[in]  Instance               The pointer to the Mtftp6 instance.
  @param[in]  Operation              The operation code of this packet.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory for the request.
  @retval EFI_SUCCESS            The request is built and sent.
  @retval Others                 Failed to transmit the packet.

**/
EFI_STATUS
Mtftp6SendRequest (
  IN MTFTP6_INSTANCE  *Instance,
  IN UINT16           Operation
  )
{
  EFI_MTFTP6_PACKET  *Packet;
  EFI_MTFTP6_OPTION  *Options;
  EFI_MTFTP6_TOKEN   *Token;
  RETURN_STATUS      Status;
  NET_BUF            *Nbuf;
  UINT8              *Mode;
  UINT8              *Cur;
  UINTN              Index;
  UINT32             BufferLength;
  UINTN              FileNameLength;
  UINTN              ModeLength;
  UINTN              OptionStrLength;
  UINTN              ValueStrLength;

  Token   = Instance->Token;
  Options = Token->OptionList;
  Mode    = Token->ModeStr;

  if (Mode == NULL) {
    Mode = (UINT8 *)"octet";
  }

  //
  // The header format of RRQ/WRQ packet is:
  //
  //   2 bytes     string    1 byte     string   1 byte
  //   ------------------------------------------------
  //  | Opcode |  Filename  |   0  |    Mode    |   0  |
  //   ------------------------------------------------
  //
  // The common option format is:
  //
  //    string     1 byte     string   1 byte
  //   ---------------------------------------
  //  | OptionStr |   0  |  ValueStr  |   0   |
  //   ---------------------------------------
  //

  //
  // Compute the size of new Mtftp6 packet.
  //
  FileNameLength = AsciiStrLen ((CHAR8 *)Token->Filename);
  ModeLength     = AsciiStrLen ((CHAR8 *)Mode);
  BufferLength   = (UINT32)FileNameLength + (UINT32)ModeLength + 4;

  for (Index = 0; Index < Token->OptionCount; Index++) {
    OptionStrLength = AsciiStrLen ((CHAR8 *)Options[Index].OptionStr);
    ValueStrLength  = AsciiStrLen ((CHAR8 *)Options[Index].ValueStr);
    BufferLength   += (UINT32)OptionStrLength + (UINT32)ValueStrLength + 2;
  }

  //
  // Allocate a packet then copy the data.
  //
  if ((Nbuf = NetbufAlloc (BufferLength)) == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy the opcode, filename and mode into packet.
  //
  Packet = (EFI_MTFTP6_PACKET *)NetbufAllocSpace (Nbuf, BufferLength, FALSE);
  ASSERT (Packet != NULL);

  Packet->OpCode = HTONS (Operation);
  BufferLength  -= sizeof (Packet->OpCode);

  Cur    = Packet->Rrq.Filename;
  Status = AsciiStrCpyS ((CHAR8 *)Cur, BufferLength, (CHAR8 *)Token->Filename);
  ASSERT_EFI_ERROR (Status);
  BufferLength -= (UINT32)(FileNameLength + 1);
  Cur          += FileNameLength + 1;
  Status        = AsciiStrCpyS ((CHAR8 *)Cur, BufferLength, (CHAR8 *)Mode);
  ASSERT_EFI_ERROR (Status);
  BufferLength -= (UINT32)(ModeLength + 1);
  Cur          += ModeLength + 1;

  //
  // Copy all the extension options into the packet.
  //
  for (Index = 0; Index < Token->OptionCount; ++Index) {
    OptionStrLength = AsciiStrLen ((CHAR8 *)Options[Index].OptionStr);
    ValueStrLength  = AsciiStrLen ((CHAR8 *)Options[Index].ValueStr);

    Status = AsciiStrCpyS ((CHAR8 *)Cur, BufferLength, (CHAR8 *)Options[Index].OptionStr);
    ASSERT_EFI_ERROR (Status);
    BufferLength -= (UINT32)(OptionStrLength + 1);
    Cur          += OptionStrLength + 1;

    Status = AsciiStrCpyS ((CHAR8 *)Cur, BufferLength, (CHAR8 *)Options[Index].ValueStr);
    ASSERT_EFI_ERROR (Status);
    BufferLength -= (UINT32)(ValueStrLength + 1);
    Cur          += ValueStrLength + 1;
  }

  //
  // Save the packet buf for retransmit
  //
  if (Instance->LastPacket != NULL) {
    NetbufFree (Instance->LastPacket);
  }

  Instance->LastPacket = Nbuf;
  Instance->CurRetry   = 0;

  return Mtftp6TransmitPacket (Instance, Nbuf);
}

/**
  Build and send an error packet.

  @param[in]  Instance               The pointer to the Mtftp6 instance.
  @param[in]  ErrCode                The error code in the packet.
  @param[in]  ErrInfo                The error message in the packet.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory for the error packet.
  @retval EFI_SUCCESS            The error packet is transmitted.
  @retval Others                 Failed to transmit the packet.

**/
EFI_STATUS
Mtftp6SendError (
  IN MTFTP6_INSTANCE  *Instance,
  IN UINT16           ErrCode,
  IN UINT8            *ErrInfo
  )
{
  NET_BUF            *Nbuf;
  EFI_MTFTP6_PACKET  *TftpError;
  UINT32             Len;

  //
  // Allocate a packet then copy the data.
  //
  Len  = (UINT32)(AsciiStrLen ((CHAR8 *)ErrInfo) + sizeof (EFI_MTFTP6_ERROR_HEADER));
  Nbuf = NetbufAlloc (Len);

  if (Nbuf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TftpError = (EFI_MTFTP6_PACKET *)NetbufAllocSpace (Nbuf, Len, FALSE);

  if (TftpError == NULL) {
    NetbufFree (Nbuf);
    return EFI_OUT_OF_RESOURCES;
  }

  TftpError->OpCode          = HTONS (EFI_MTFTP6_OPCODE_ERROR);
  TftpError->Error.ErrorCode = HTONS (ErrCode);

  AsciiStrCpyS ((CHAR8 *)TftpError->Error.ErrorMessage, AsciiStrLen ((CHAR8 *)ErrInfo) + 1, (CHAR8 *)ErrInfo);

  //
  // Save the packet buf for retransmit
  //
  if (Instance->LastPacket != NULL) {
    NetbufFree (Instance->LastPacket);
  }

  Instance->LastPacket = Nbuf;
  Instance->CurRetry   = 0;

  return Mtftp6TransmitPacket (Instance, Nbuf);
}

/**
  The callback function called when the packet is transmitted.

  @param[in]  Packet                 The pointer to the packet.
  @param[in]  UdpEpt                 The pointer to the Udp6 access point.
  @param[in]  IoStatus               The result of the transmission.
  @param[in]  Context                The pointer to the context.

**/
VOID
EFIAPI
Mtftp6OnPacketSent (
  IN NET_BUF        *Packet,
  IN UDP_END_POINT  *UdpEpt,
  IN EFI_STATUS     IoStatus,
  IN VOID           *Context
  )
{
  NetbufFree (Packet);
  *(BOOLEAN *)Context = TRUE;
}

/**
  Send the packet for the Mtftp6 instance.

  @param[in]  Instance               The pointer to the Mtftp6 instance.
  @param[in]  Packet                 The pointer to the packet to be sent.

  @retval EFI_SUCCESS            The packet was sent out
  @retval Others                 Failed to transmit the packet.

**/
EFI_STATUS
Mtftp6TransmitPacket (
  IN MTFTP6_INSTANCE  *Instance,
  IN NET_BUF          *Packet
  )
{
  EFI_UDP6_PROTOCOL     *Udp6;
  EFI_UDP6_CONFIG_DATA  Udp6CfgData;
  EFI_STATUS            Status;
  UINT16                *Temp;
  UINT16                Value;
  UINT16                OpCode;

  ZeroMem (&Udp6CfgData, sizeof (EFI_UDP6_CONFIG_DATA));
  Udp6 = Instance->UdpIo->Protocol.Udp6;

  //
  // Set the live time of the packet.
  //
  Instance->PacketToLive = Instance->IsMaster ? Instance->Timeout : (Instance->Timeout * 2);

  Temp = (UINT16 *)NetbufGetByte (Packet, 0, NULL);
  ASSERT (Temp != NULL);

  Value  = *Temp;
  OpCode = NTOHS (Value);

  if ((OpCode == EFI_MTFTP6_OPCODE_RRQ) || (OpCode == EFI_MTFTP6_OPCODE_DIR) || (OpCode == EFI_MTFTP6_OPCODE_WRQ)) {
    //
    // For the Rrq, Dir, Wrq requests of the operation, configure the Udp6Io as
    // (serverip, 69, localip, localport) to send.
    // Usually local address and local port are both default as zero.
    //
    Status = Udp6->Configure (Udp6, NULL);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = Mtftp6ConfigUdpIo (
               Instance->UdpIo,
               &Instance->ServerIp,
               Instance->ServerCmdPort,
               &Instance->Config->StationIp,
               Instance->Config->LocalPort
               );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Get the current local address and port by get Udp6 mode data.
    //
    Status = Udp6->GetModeData (Udp6, &Udp6CfgData, NULL, NULL, NULL);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    NET_GET_REF (Packet);

    Instance->IsTransmitted = FALSE;

    Status = UdpIoSendDatagram (
               Instance->UdpIo,
               Packet,
               NULL,
               NULL,
               Mtftp6OnPacketSent,
               &Instance->IsTransmitted
               );

    if (EFI_ERROR (Status)) {
      NET_PUT_REF (Packet);
      return Status;
    }

    //
    // Poll till the packet sent out from the ip6 queue.
    //
    gBS->RestoreTPL (Instance->OldTpl);

    while (!Instance->IsTransmitted) {
      Udp6->Poll (Udp6);
    }

    Instance->OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

    //
    // For the subsequent exchange of such requests, reconfigure the Udp6Io as
    // (serverip, 0, localip, localport) to receive.
    // Currently local address and local port are specified by Udp6 mode data.
    //
    Status = Udp6->Configure (Udp6, NULL);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = Mtftp6ConfigUdpIo (
               Instance->UdpIo,
               &Instance->ServerIp,
               Instance->ServerDataPort,
               &Udp6CfgData.StationAddress,
               Udp6CfgData.StationPort
               );
  } else {
    //
    // For the data exchange, configure the Udp6Io as (serverip, dataport,
    // localip, localport) to send/receive.
    // Currently local address and local port are specified by Udp6 mode data.
    //
    Status = Udp6->GetModeData (Udp6, &Udp6CfgData, NULL, NULL, NULL);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (Udp6CfgData.RemotePort != Instance->ServerDataPort) {
      Status = Udp6->Configure (Udp6, NULL);

      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = Mtftp6ConfigUdpIo (
                 Instance->UdpIo,
                 &Instance->ServerIp,
                 Instance->ServerDataPort,
                 &Udp6CfgData.StationAddress,
                 Udp6CfgData.StationPort
                 );

      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    NET_GET_REF (Packet);

    Instance->IsTransmitted = FALSE;

    Status = UdpIoSendDatagram (
               Instance->UdpIo,
               Packet,
               NULL,
               NULL,
               Mtftp6OnPacketSent,
               &Instance->IsTransmitted
               );

    if (EFI_ERROR (Status)) {
      NET_PUT_REF (Packet);
    }

    //
    // Poll till the packet sent out from the ip6 queue.
    //
    gBS->RestoreTPL (Instance->OldTpl);

    while (!Instance->IsTransmitted) {
      Udp6->Poll (Udp6);
    }

    Instance->OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  }

  return Status;
}

/**
  Check packet for GetInfo callback routine.

  GetInfo is implemented with EfiMtftp6ReadFile. It's used to inspect
  the first packet from server, then abort the session.

  @param[in]  This                   The pointer to the Mtftp6 protocol.
  @param[in]  Token                  The pointer to the Mtftp6 token.
  @param[in]  PacketLen              The length of the packet.
  @param[in]  Packet                 The pointer to the received packet.

  @retval EFI_ABORTED            Abort the Mtftp6 operation.

**/
EFI_STATUS
EFIAPI
Mtftp6CheckPacket (
  IN EFI_MTFTP6_PROTOCOL  *This,
  IN EFI_MTFTP6_TOKEN     *Token,
  IN UINT16               PacketLen,
  IN EFI_MTFTP6_PACKET    *Packet
  )
{
  MTFTP6_GETINFO_CONTEXT  *Context;
  UINT16                  OpCode;

  Context = (MTFTP6_GETINFO_CONTEXT *)Token->Context;
  OpCode  = NTOHS (Packet->OpCode);

  //
  // Set the GetInfo's return status according to the OpCode.
  //
  switch (OpCode) {
    case EFI_MTFTP6_OPCODE_ERROR:
      Context->Status = EFI_TFTP_ERROR;
      break;

    case EFI_MTFTP6_OPCODE_OACK:
      Context->Status = EFI_SUCCESS;
      break;

    default:
      Context->Status = EFI_PROTOCOL_ERROR;
  }

  //
  // Allocate buffer then copy the packet over. Use gBS->AllocatePool
  // in case NetAllocatePool will implements something tricky.
  //
  *(Context->Packet) = AllocateZeroPool (PacketLen);

  if (*(Context->Packet) == NULL) {
    Context->Status = EFI_OUT_OF_RESOURCES;
    return EFI_ABORTED;
  }

  *(Context->PacketLen) = PacketLen;
  CopyMem (*(Context->Packet), Packet, PacketLen);

  return EFI_ABORTED;
}

/**
  Clean up the current Mtftp6 operation.

  @param[in]  Instance               The pointer to the Mtftp6 instance.
  @param[in]  Result                 The result to be returned to the user.

**/
VOID
Mtftp6OperationClean (
  IN MTFTP6_INSTANCE  *Instance,
  IN EFI_STATUS       Result
  )
{
  LIST_ENTRY          *Entry;
  LIST_ENTRY          *Next;
  MTFTP6_BLOCK_RANGE  *Block;

  //
  // Clean up the current token and event.
  //
  if (Instance->Token != NULL) {
    Instance->Token->Status = Result;
    if (Instance->Token->Event != NULL) {
      gBS->SignalEvent (Instance->Token->Event);
    }

    Instance->Token = NULL;
  }

  //
  // Clean up the corresponding Udp6Io.
  //
  if (Instance->UdpIo != NULL) {
    UdpIoCleanIo (Instance->UdpIo);
  }

  if (Instance->McastUdpIo != NULL) {
    gBS->CloseProtocol (
           Instance->McastUdpIo->UdpHandle,
           &gEfiUdp6ProtocolGuid,
           Instance->McastUdpIo->Image,
           Instance->Handle
           );
    UdpIoFreeIo (Instance->McastUdpIo);
    Instance->McastUdpIo = NULL;
  }

  //
  // Clean up the stored last packet.
  //
  if (Instance->LastPacket != NULL) {
    NetbufFree (Instance->LastPacket);
    Instance->LastPacket = NULL;
  }

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &Instance->BlkList) {
    Block = NET_LIST_USER_STRUCT (Entry, MTFTP6_BLOCK_RANGE, Link);
    RemoveEntryList (Entry);
    FreePool (Block);
  }

  //
  // Reinitialize the corresponding fields of the Mtftp6 operation.
  //
  ZeroMem (&Instance->ExtInfo, sizeof (MTFTP6_EXT_OPTION_INFO));
  ZeroMem (&Instance->ServerIp, sizeof (EFI_IPv6_ADDRESS));
  ZeroMem (&Instance->McastIp, sizeof (EFI_IPv6_ADDRESS));

  Instance->ServerCmdPort  = 0;
  Instance->ServerDataPort = 0;
  Instance->McastPort      = 0;
  Instance->BlkSize        = 0;
  Instance->Operation      = 0;
  Instance->WindowSize     = 1;
  Instance->TotalBlock     = 0;
  Instance->AckedBlock     = 0;
  Instance->LastBlk        = 0;
  Instance->PacketToLive   = 0;
  Instance->MaxRetry       = 0;
  Instance->CurRetry       = 0;
  Instance->Timeout        = 0;
  Instance->IsMaster       = TRUE;
}

/**
  Start the Mtftp6 instance to perform the operation, such as read file,
  write file, and read directory.

  @param[in]  This                   The MTFTP session.
  @param[in]  Token                  The token than encapsules the user's request.
  @param[in]  OpCode                 The operation to perform.

  @retval EFI_INVALID_PARAMETER  Some of the parameters are invalid.
  @retval EFI_NOT_STARTED        The MTFTP session hasn't been configured.
  @retval EFI_ALREADY_STARTED    There is pending operation for the session.
  @retval EFI_SUCCESS            The operation is successfully started.

**/
EFI_STATUS
Mtftp6OperationStart (
  IN EFI_MTFTP6_PROTOCOL  *This,
  IN EFI_MTFTP6_TOKEN     *Token,
  IN UINT16               OpCode
  )
{
  MTFTP6_INSTANCE  *Instance;
  EFI_STATUS       Status;

  if ((This == NULL) ||
      (Token == NULL) ||
      (Token->Filename == NULL) ||
      ((Token->OptionCount != 0) && (Token->OptionList == NULL)) ||
      ((Token->OverrideData != NULL) && !NetIp6IsValidUnicast (&Token->OverrideData->ServerIp))
      )
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // At least define one method to collect the data for download.
  //
  if (((OpCode == EFI_MTFTP6_OPCODE_RRQ) || (OpCode == EFI_MTFTP6_OPCODE_DIR)) &&
      (Token->Buffer == NULL) &&
      (Token->CheckPacket == NULL)
      )
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // At least define one method to provide the data for upload.
  //
  if ((OpCode == EFI_MTFTP6_OPCODE_WRQ) && (Token->Buffer == NULL) && (Token->PacketNeeded == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = MTFTP6_INSTANCE_FROM_THIS (This);

  if (Instance->Config == NULL) {
    return EFI_NOT_STARTED;
  }

  if (Instance->Token != NULL) {
    return EFI_ACCESS_DENIED;
  }

  Status           = EFI_SUCCESS;
  Instance->OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance->Operation = OpCode;

  //
  // Parse the extension options in the request packet.
  //
  if (Token->OptionCount != 0) {
    Status = Mtftp6ParseExtensionOption (
               Token->OptionList,
               Token->OptionCount,
               TRUE,
               Instance->Operation,
               &Instance->ExtInfo
               );

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  //
  // Initialize runtime data from config data or override data.
  //
  Instance->Token          = Token;
  Instance->ServerCmdPort  = Instance->Config->InitialServerPort;
  Instance->ServerDataPort = 0;
  Instance->MaxRetry       = Instance->Config->TryCount;
  Instance->Timeout        = Instance->Config->TimeoutValue;
  Instance->IsMaster       = TRUE;

  CopyMem (
    &Instance->ServerIp,
    &Instance->Config->ServerIp,
    sizeof (EFI_IPv6_ADDRESS)
    );

  if (Token->OverrideData != NULL) {
    Instance->ServerCmdPort = Token->OverrideData->ServerPort;
    Instance->MaxRetry      = Token->OverrideData->TryCount;
    Instance->Timeout       = Token->OverrideData->TimeoutValue;

    CopyMem (
      &Instance->ServerIp,
      &Token->OverrideData->ServerIp,
      sizeof (EFI_IPv6_ADDRESS)
      );
  }

  //
  // Set default value for undefined parameters.
  //
  if (Instance->ServerCmdPort == 0) {
    Instance->ServerCmdPort = MTFTP6_DEFAULT_SERVER_CMD_PORT;
  }

  if (Instance->BlkSize == 0) {
    Instance->BlkSize = MTFTP6_DEFAULT_BLK_SIZE;
  }

  if (Instance->WindowSize == 0) {
    Instance->WindowSize = MTFTP6_DEFAULT_WINDOWSIZE;
  }

  if (Instance->MaxRetry == 0) {
    Instance->MaxRetry = MTFTP6_DEFAULT_MAX_RETRY;
  }

  if (Instance->Timeout == 0) {
    Instance->Timeout = MTFTP6_DEFAULT_TIMEOUT;
  }

  Token->Status = EFI_NOT_READY;

  //
  // Switch the routines by the operation code.
  //
  switch (OpCode) {
    case EFI_MTFTP6_OPCODE_RRQ:
      Status = Mtftp6RrqStart (Instance, OpCode);
      break;

    case EFI_MTFTP6_OPCODE_DIR:
      Status = Mtftp6RrqStart (Instance, OpCode);
      break;

    case EFI_MTFTP6_OPCODE_WRQ:
      Status = Mtftp6WrqStart (Instance, OpCode);
      break;

    default:
      Status = EFI_DEVICE_ERROR;
      goto ON_ERROR;
  }

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Return immediately for asynchronous or poll the instance for synchronous.
  //
  gBS->RestoreTPL (Instance->OldTpl);

  if (Token->Event == NULL) {
    while (Token->Status == EFI_NOT_READY) {
      This->Poll (This);
    }

    return Token->Status;
  }

  return EFI_SUCCESS;

ON_ERROR:

  Mtftp6OperationClean (Instance, Status);
  gBS->RestoreTPL (Instance->OldTpl);

  return Status;
}

/**
  The timer ticking routine for the Mtftp6 instance.

  @param[in]  Event                  The pointer to the ticking event.
  @param[in]  Context                The pointer to the context.

**/
VOID
EFIAPI
Mtftp6OnTimerTick (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  MTFTP6_SERVICE    *Service;
  MTFTP6_INSTANCE   *Instance;
  LIST_ENTRY        *Entry;
  LIST_ENTRY        *Next;
  EFI_MTFTP6_TOKEN  *Token;
  EFI_STATUS        Status;

  Service = (MTFTP6_SERVICE *)Context;

  //
  // Iterate through all the children of the Mtftp service instance. Time
  // out the packet. If maximum retries reached, clean the session up.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &Service->Children) {
    Instance = NET_LIST_USER_STRUCT (Entry, MTFTP6_INSTANCE, Link);

    if (Instance->Token == NULL) {
      continue;
    }

    if (Instance->PacketToLive > 0) {
      Instance->PacketToLive--;
      continue;
    }

    Instance->CurRetry++;
    Token = Instance->Token;

    if (Token->TimeoutCallback != NULL) {
      //
      // Call the timeout callback routine if has.
      //
      Status = Token->TimeoutCallback (&Instance->Mtftp6, Token);

      if (EFI_ERROR (Status)) {
        Mtftp6SendError (
          Instance,
          EFI_MTFTP6_ERRORCODE_REQUEST_DENIED,
          (UINT8 *)"User aborted the transfer in time out"
          );
        Mtftp6OperationClean (Instance, EFI_ABORTED);
        continue;
      }
    }

    //
    // Retransmit the packet if haven't reach the maximum retry count,
    // otherwise exit the transfer.
    //
    if (Instance->CurRetry < Instance->MaxRetry) {
      Mtftp6TransmitPacket (Instance, Instance->LastPacket);
    } else {
      Mtftp6OperationClean (Instance, EFI_TIMEOUT);
      continue;
    }
  }
}

/** @file
  Help functions to access UDP service, it is used by both the DHCP and MTFTP.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Protocol/Udp4.h>
#include <Protocol/Udp6.h>

#include <Library/UdpIoLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DpcLib.h>


/**
  Free a UDP_TX_TOKEN. The TX event is closed.

  @param[in]  TxToken                 The UDP_TX_TOKEN to release.

**/
VOID
UdpIoFreeTxToken (
  IN UDP_TX_TOKEN           *TxToken
  )
{

  if (TxToken->UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {
    gBS->CloseEvent (TxToken->Token.Udp4.Event);
  } else if (TxToken->UdpIo->UdpVersion == UDP_IO_UDP6_VERSION) {
    gBS->CloseEvent (TxToken->Token.Udp6.Event);
  } else {
    ASSERT (FALSE);
  }

  FreePool (TxToken);
}

/**
  Free a UDP_RX_TOKEN. The RX event is closed.

  @param[in]  RxToken                 The UDP_RX_TOKEN to release.

**/
VOID
UdpIoFreeRxToken (
  IN UDP_RX_TOKEN           *RxToken
  )
{
  if (RxToken->UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {
    gBS->CloseEvent (RxToken->Token.Udp4.Event);
  } else if (RxToken->UdpIo->UdpVersion == UDP_IO_UDP6_VERSION) {
    gBS->CloseEvent (RxToken->Token.Udp6.Event);
  } else {
    ASSERT (FALSE);
  }

  FreePool (RxToken);
}

/**
  The callback function when the packet is sent by UDP.

  It will remove the packet from the local list then call
  the packet owner's callback function set by UdpIoSendDatagram.

  @param[in]  Context               The UDP TX Token.

**/
VOID
EFIAPI
UdpIoOnDgramSentDpc (
  IN VOID                   *Context
  )
{
  UDP_TX_TOKEN              *TxToken;

  TxToken = (UDP_TX_TOKEN *) Context;
  ASSERT (TxToken->Signature == UDP_IO_TX_SIGNATURE);
  ASSERT ((TxToken->UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) ||
          (TxToken->UdpIo->UdpVersion == UDP_IO_UDP6_VERSION));

  RemoveEntryList (&TxToken->Link);

  if (TxToken->UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {
    TxToken->CallBack (TxToken->Packet, NULL, TxToken->Token.Udp4.Status, TxToken->Context);
  } else {
    TxToken->CallBack (TxToken->Packet, NULL, TxToken->Token.Udp6.Status, TxToken->Context);
  }

  UdpIoFreeTxToken (TxToken);
}

/**
  Request UdpIoOnDgramSentDpc as a DPC at TPL_CALLBACK.

  @param[in]  Event                 The event signaled.
  @param[in]  Context               The UDP TX Token.

**/
VOID
EFIAPI
UdpIoOnDgramSent (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  //
  // Request UdpIoOnDgramSentDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, UdpIoOnDgramSentDpc, Context);
}

/**
  Recycle the received UDP data.

  @param[in]  Context               The UDP_RX_TOKEN.

**/
VOID
EFIAPI
UdpIoRecycleDgram (
  IN VOID                   *Context
  )
{
  UDP_RX_TOKEN              *RxToken;

  RxToken = (UDP_RX_TOKEN *) Context;

  if (RxToken->UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {
    gBS->SignalEvent (RxToken->Token.Udp4.Packet.RxData->RecycleSignal);
  } else if (RxToken->UdpIo->UdpVersion == UDP_IO_UDP6_VERSION) {
    gBS->SignalEvent (RxToken->Token.Udp6.Packet.RxData->RecycleSignal);
  } else {
    ASSERT (FALSE);
  }

  UdpIoFreeRxToken (RxToken);
}

/**
  The event handle for UDP receive request.

  It will build a NET_BUF from the received UDP data, then deliver it
  to the receiver.

  @param[in]  Context               The UDP RX token.

**/
VOID
EFIAPI
UdpIoOnDgramRcvdDpc (
  IN VOID                   *Context
  )
{
  EFI_STATUS                Status;
  VOID                      *Token;
  VOID                      *RxData;
  VOID                      *Session;
  UDP_RX_TOKEN              *RxToken;
  UDP_END_POINT             EndPoint;
  NET_BUF                   *Netbuf;

  RxToken = (UDP_RX_TOKEN *) Context;

  ZeroMem (&EndPoint, sizeof(UDP_END_POINT));

  ASSERT ((RxToken->Signature == UDP_IO_RX_SIGNATURE) &&
          (RxToken == RxToken->UdpIo->RecvRequest));

  ASSERT ((RxToken->UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) ||
          (RxToken->UdpIo->UdpVersion == UDP_IO_UDP6_VERSION));

  //
  // Clear the receive request first in case that the caller
  // wants to restart the receive in the callback.
  //
  RxToken->UdpIo->RecvRequest = NULL;

  if (RxToken->UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {
    Token  = &RxToken->Token.Udp4;
    RxData = ((EFI_UDP4_COMPLETION_TOKEN *) Token)->Packet.RxData;
    Status = ((EFI_UDP4_COMPLETION_TOKEN *) Token)->Status;
  } else {
    Token  = &RxToken->Token.Udp6;
    RxData = ((EFI_UDP6_COMPLETION_TOKEN *) Token)->Packet.RxData;
    Status = ((EFI_UDP6_COMPLETION_TOKEN *) Token)->Status;
  }

  if (EFI_ERROR (Status) || RxData == NULL) {
    if (Status != EFI_ABORTED) {
      //
      // Invoke the CallBack only if the reception is not actively aborted.
      //
      RxToken->CallBack (NULL, NULL, Status, RxToken->Context);
    }

    UdpIoFreeRxToken (RxToken);
    return;
  }

  //
  // Build a NET_BUF from the UDP receive data, then deliver it up.
  //
  if (RxToken->UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {
    if (((EFI_UDP4_RECEIVE_DATA *) RxData)->DataLength == 0) {
      //
      // Discard zero length data payload packet.
      //
      goto Resume;
    }

    Netbuf = NetbufFromExt (
               (NET_FRAGMENT *)((EFI_UDP4_RECEIVE_DATA *) RxData)->FragmentTable,
               ((EFI_UDP4_RECEIVE_DATA *) RxData)->FragmentCount,
               0,
               (UINT32) RxToken->HeadLen,
               UdpIoRecycleDgram,
               RxToken
               );

    if (Netbuf == NULL) {
      gBS->SignalEvent (((EFI_UDP4_RECEIVE_DATA *) RxData)->RecycleSignal);
      RxToken->CallBack (NULL, NULL, EFI_OUT_OF_RESOURCES, RxToken->Context);

      UdpIoFreeRxToken (RxToken);
      return;
    }

    Session             = &((EFI_UDP4_RECEIVE_DATA *) RxData)->UdpSession;
    EndPoint.LocalPort  = ((EFI_UDP4_SESSION_DATA *) Session)->DestinationPort;
    EndPoint.RemotePort = ((EFI_UDP4_SESSION_DATA *) Session)->SourcePort;

    CopyMem (
      &EndPoint.LocalAddr,
      &((EFI_UDP4_SESSION_DATA *) Session)->DestinationAddress,
      sizeof (EFI_IPv4_ADDRESS)
      );

    CopyMem (
      &EndPoint.RemoteAddr,
      &((EFI_UDP4_SESSION_DATA *) Session)->SourceAddress,
      sizeof (EFI_IPv4_ADDRESS)
      );

    EndPoint.LocalAddr.Addr[0]  = NTOHL (EndPoint.LocalAddr.Addr[0]);
    EndPoint.RemoteAddr.Addr[0] = NTOHL (EndPoint.RemoteAddr.Addr[0]);
  } else {
    if (((EFI_UDP6_RECEIVE_DATA *) RxData)->DataLength == 0) {
      //
      // Discard zero length data payload packet.
      //
      goto Resume;
    }

    Netbuf = NetbufFromExt (
               (NET_FRAGMENT *)((EFI_UDP6_RECEIVE_DATA *) RxData)->FragmentTable,
               ((EFI_UDP6_RECEIVE_DATA *) RxData)->FragmentCount,
               0,
               (UINT32) RxToken->HeadLen,
               UdpIoRecycleDgram,
               RxToken
               );

    if (Netbuf == NULL) {
      gBS->SignalEvent (((EFI_UDP6_RECEIVE_DATA *) RxData)->RecycleSignal);
      RxToken->CallBack (NULL, NULL, EFI_OUT_OF_RESOURCES, RxToken->Context);

      UdpIoFreeRxToken (RxToken);
      return;
    }

    Session             = &((EFI_UDP6_RECEIVE_DATA *) RxData)->UdpSession;
    EndPoint.LocalPort  = ((EFI_UDP6_SESSION_DATA *) Session)->DestinationPort;
    EndPoint.RemotePort = ((EFI_UDP6_SESSION_DATA *) Session)->SourcePort;

    CopyMem (
      &EndPoint.LocalAddr,
      &((EFI_UDP6_SESSION_DATA *) Session)->DestinationAddress,
      sizeof (EFI_IPv6_ADDRESS)
      );

    CopyMem (
      &EndPoint.RemoteAddr,
      &((EFI_UDP6_SESSION_DATA *) Session)->SourceAddress,
      sizeof (EFI_IPv6_ADDRESS)
      );

    Ip6Swap128 (&EndPoint.LocalAddr.v6);
    Ip6Swap128 (&EndPoint.RemoteAddr.v6);
  }

  RxToken->CallBack (Netbuf, &EndPoint, EFI_SUCCESS, RxToken->Context);
  return;

Resume:
  if (RxToken->UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {
    gBS->SignalEvent (((EFI_UDP4_RECEIVE_DATA *) RxData)->RecycleSignal);
    RxToken->UdpIo->Protocol.Udp4->Receive (RxToken->UdpIo->Protocol.Udp4, &RxToken->Token.Udp4);
  } else {
    gBS->SignalEvent (((EFI_UDP6_RECEIVE_DATA *) RxData)->RecycleSignal);
    RxToken->UdpIo->Protocol.Udp6->Receive (RxToken->UdpIo->Protocol.Udp6, &RxToken->Token.Udp6);
  }
}

/**
  Request UdpIoOnDgramRcvdDpc() as a DPC at TPL_CALLBACK.

  @param[in]  Event                 The UDP receive request event.
  @param[in]  Context               The UDP RX token.

**/
VOID
EFIAPI
UdpIoOnDgramRcvd (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  //
  // Request UdpIoOnDgramRcvdDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, UdpIoOnDgramRcvdDpc, Context);
}

/**
  Create a UDP_RX_TOKEN to wrap the request.

  @param[in]  UdpIo                 The UdpIo to receive packets from.
  @param[in]  CallBack              The function to call when receive finished.
  @param[in]  Context               The opaque parameter to the CallBack.
  @param[in]  HeadLen               The head length to reserve for the packet.

  @return The Wrapped request or NULL if failed to allocate resources or some errors happened.

**/
UDP_RX_TOKEN *
UdpIoCreateRxToken (
  IN UDP_IO                 *UdpIo,
  IN UDP_IO_CALLBACK        CallBack,
  IN VOID                   *Context,
  IN UINT32                 HeadLen
  )
{
  UDP_RX_TOKEN              *Token;
  EFI_STATUS                Status;

  ASSERT ((UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) ||
          (UdpIo->UdpVersion == UDP_IO_UDP6_VERSION));

  Token = AllocatePool (sizeof (UDP_RX_TOKEN));

  if (Token == NULL) {
    return NULL;
  }

  Token->Signature              = UDP_IO_RX_SIGNATURE;
  Token->UdpIo                  = UdpIo;
  Token->CallBack               = CallBack;
  Token->Context                = Context;
  Token->HeadLen                = HeadLen;

  if (UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {

    Token->Token.Udp4.Status        = EFI_NOT_READY;
    Token->Token.Udp4.Packet.RxData = NULL;

    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    UdpIoOnDgramRcvd,
                    Token,
                    &Token->Token.Udp4.Event
                    );
    } else {

    Token->Token.Udp6.Status        = EFI_NOT_READY;
    Token->Token.Udp6.Packet.RxData = NULL;

    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    UdpIoOnDgramRcvd,
                    Token,
                    &Token->Token.Udp6.Event
                    );
  }


  if (EFI_ERROR (Status)) {
    FreePool (Token);
    return NULL;
  }

  return Token;
}

/**
  Wrap a transmit request into a new created UDP_TX_TOKEN.

  If Packet is NULL, then ASSERT().
  If Udp version is not UDP_IO_UDP4_VERSION or UDP_IO_UDP6_VERSION, then ASSERT().

  @param[in]  UdpIo                 The UdpIo to send packet to.
  @param[in]  Packet                The user's packet.
  @param[in]  EndPoint              The local and remote access point.
  @param[in]  Gateway               The overridden next hop.
  @param[in]  CallBack              The function to call when transmission completed.
  @param[in]  Context               The opaque parameter to the call back.

  @return The wrapped transmission request or NULL if failed to allocate resources
          or for some errors.

**/
UDP_TX_TOKEN *
UdpIoCreateTxToken (
  IN UDP_IO                 *UdpIo,
  IN NET_BUF                *Packet,
  IN UDP_END_POINT          *EndPoint OPTIONAL,
  IN EFI_IP_ADDRESS         *Gateway  OPTIONAL,
  IN UDP_IO_CALLBACK        CallBack,
  IN VOID                   *Context
  )
{
  UDP_TX_TOKEN              *TxToken;
  VOID                      *Token;
  VOID                      *Data;
  EFI_STATUS                Status;
  UINT32                    Count;
  UINTN                     Size;
  IP4_ADDR                  Ip;

  ASSERT (Packet != NULL);
  ASSERT ((UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) ||
          (UdpIo->UdpVersion == UDP_IO_UDP6_VERSION));

  if (UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {
    Size = sizeof (UDP_TX_TOKEN) + sizeof (EFI_UDP4_FRAGMENT_DATA) * (Packet->BlockOpNum - 1);
  } else {
    Size = sizeof (UDP_TX_TOKEN) + sizeof (EFI_UDP6_FRAGMENT_DATA) * (Packet->BlockOpNum - 1);
  }

  TxToken = AllocatePool (Size);

  if (TxToken == NULL) {
    return NULL;
  }

  TxToken->Signature = UDP_IO_TX_SIGNATURE;
  InitializeListHead (&TxToken->Link);

  TxToken->UdpIo     = UdpIo;
  TxToken->CallBack  = CallBack;
  TxToken->Packet    = Packet;
  TxToken->Context   = Context;

  Token              = &(TxToken->Token);
  Count              = Packet->BlockOpNum;

  if (UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {

    ((EFI_UDP4_COMPLETION_TOKEN *) Token)->Status = EFI_NOT_READY;

    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    UdpIoOnDgramSent,
                    TxToken,
                    &((EFI_UDP4_COMPLETION_TOKEN *) Token)->Event
                    );

    if (EFI_ERROR (Status)) {
      FreePool (TxToken);
      return NULL;
    }

    Data = &(TxToken->Data.Udp4);
    ((EFI_UDP4_COMPLETION_TOKEN *) Token)->Packet.TxData = Data;

    ((EFI_UDP4_TRANSMIT_DATA *) Data)->UdpSessionData    = NULL;
    ((EFI_UDP4_TRANSMIT_DATA *) Data)->GatewayAddress    = NULL;
    ((EFI_UDP4_TRANSMIT_DATA *) Data)->DataLength        = Packet->TotalSize;

    NetbufBuildExt (
      Packet,
      (NET_FRAGMENT *)((EFI_UDP4_TRANSMIT_DATA *) Data)->FragmentTable,
      &Count
      );

    ((EFI_UDP4_TRANSMIT_DATA *) Data)->FragmentCount     = Count;

    if (EndPoint != NULL) {
      Ip = HTONL (EndPoint->LocalAddr.Addr[0]);
      CopyMem (
        &TxToken->Session.Udp4.SourceAddress,
        &Ip,
        sizeof (EFI_IPv4_ADDRESS)
        );

      Ip = HTONL (EndPoint->RemoteAddr.Addr[0]);
      CopyMem (
        &TxToken->Session.Udp4.DestinationAddress,
        &Ip,
        sizeof (EFI_IPv4_ADDRESS)
        );

      TxToken->Session.Udp4.SourcePort                   = EndPoint->LocalPort;
      TxToken->Session.Udp4.DestinationPort              = EndPoint->RemotePort;
      ((EFI_UDP4_TRANSMIT_DATA *) Data)->UdpSessionData  = &(TxToken->Session.Udp4);
    }

    if (Gateway != NULL && (Gateway->Addr[0] != 0)) {
      Ip = HTONL (Gateway->Addr[0]);
      CopyMem (&TxToken->Gateway, &Ip, sizeof (EFI_IPv4_ADDRESS));
      ((EFI_UDP4_TRANSMIT_DATA *) Data)->GatewayAddress = &TxToken->Gateway;
    }

  } else {

    ((EFI_UDP6_COMPLETION_TOKEN *) Token)->Status = EFI_NOT_READY;

    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    UdpIoOnDgramSent,
                    TxToken,
                    &((EFI_UDP6_COMPLETION_TOKEN *) Token)->Event
                    );

    if (EFI_ERROR (Status)) {
      FreePool (TxToken);
      return NULL;
    }

    Data = &(TxToken->Data.Udp6);
    ((EFI_UDP6_COMPLETION_TOKEN *) Token)->Packet.TxData  = Data;
    ((EFI_UDP6_TRANSMIT_DATA *) Data)->UdpSessionData     = NULL;
    ((EFI_UDP6_TRANSMIT_DATA *) Data)->DataLength         = Packet->TotalSize;

    NetbufBuildExt (
      Packet,
      (NET_FRAGMENT *)((EFI_UDP6_TRANSMIT_DATA *) Data)->FragmentTable,
      &Count
      );

    ((EFI_UDP6_TRANSMIT_DATA *) Data)->FragmentCount      = Count;

    if (EndPoint != NULL) {
      CopyMem (
        &TxToken->Session.Udp6.SourceAddress,
        &EndPoint->LocalAddr.v6,
        sizeof(EFI_IPv6_ADDRESS)
        );

      CopyMem (
        &TxToken->Session.Udp6.DestinationAddress,
        &EndPoint->RemoteAddr.v6,
        sizeof(EFI_IPv6_ADDRESS)
        );

      TxToken->Session.Udp6.SourcePort                   = EndPoint->LocalPort;
      TxToken->Session.Udp6.DestinationPort              = EndPoint->RemotePort;
      ((EFI_UDP6_TRANSMIT_DATA *) Data)->UdpSessionData  = &(TxToken->Session.Udp6);
    }
  }

  return TxToken;
}

/**
  Creates a UDP_IO to access the UDP service. It creates and configures
  a UDP child.

  If Configure is NULL, then ASSERT().
  If Udp version is not UDP_IO_UDP4_VERSION or UDP_IO_UDP6_VERSION, then ASSERT().

  It locates the UDP service binding prototype on the Controller parameter
  uses the UDP service binding prototype to create a UDP child (also known as
  a UDP instance) configures the UDP child by calling Configure function prototype.
  Any failures in creating or configuring the UDP child return NULL for failure.

  @param[in]  Controller            The controller that has the UDP service binding.
                                    protocol installed.
  @param[in]  ImageHandle           The image handle for the driver.
  @param[in]  Configure             The function to configure the created UDP child.
  @param[in]  UdpVersion            The UDP protocol version, UDP4 or UDP6.
  @param[in]  Context               The opaque parameter for the Configure function.

  @return Newly-created UDP_IO or NULL if failed.

**/
UDP_IO *
EFIAPI
UdpIoCreateIo (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            ImageHandle,
  IN  UDP_IO_CONFIG         Configure,
  IN  UINT8                 UdpVersion,
  IN  VOID                  *Context
  )
{
  UDP_IO                    *UdpIo;
  EFI_STATUS                Status;

  ASSERT (Configure != NULL);
  ASSERT ((UdpVersion == UDP_IO_UDP4_VERSION) || (UdpVersion == UDP_IO_UDP6_VERSION));

  UdpIo = AllocatePool (sizeof (UDP_IO));

  if (UdpIo == NULL) {
    return NULL;
  }

  UdpIo->UdpVersion   = UdpVersion;
  UdpIo->Signature    = UDP_IO_SIGNATURE;
  InitializeListHead (&UdpIo->Link);
  UdpIo->RefCnt       = 1;

  UdpIo->Controller   = Controller;
  UdpIo->Image        = ImageHandle;

  InitializeListHead (&UdpIo->SentDatagram);
  UdpIo->RecvRequest  = NULL;
  UdpIo->UdpHandle    = NULL;

  if (UdpVersion == UDP_IO_UDP4_VERSION) {
    //
    // Create a UDP child then open and configure it
    //
    Status = NetLibCreateServiceChild (
               Controller,
               ImageHandle,
               &gEfiUdp4ServiceBindingProtocolGuid,
               &UdpIo->UdpHandle
               );

    if (EFI_ERROR (Status)) {
      goto FREE_MEM;
    }

    Status = gBS->OpenProtocol (
                    UdpIo->UdpHandle,
                    &gEfiUdp4ProtocolGuid,
                    (VOID **) &UdpIo->Protocol.Udp4,
                    ImageHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );

    if (EFI_ERROR (Status)) {
      goto FREE_CHILD;
    }

    if (EFI_ERROR (Configure (UdpIo, Context))) {
      goto CLOSE_PROTOCOL;
    }

    Status = UdpIo->Protocol.Udp4->GetModeData (
                                     UdpIo->Protocol.Udp4,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &UdpIo->SnpMode
                                     );

    if (EFI_ERROR (Status)) {
      goto CLOSE_PROTOCOL;
    }

  } else {

    Status = NetLibCreateServiceChild (
               Controller,
               ImageHandle,
               &gEfiUdp6ServiceBindingProtocolGuid,
               &UdpIo->UdpHandle
               );

    if (EFI_ERROR (Status)) {
      goto FREE_MEM;
    }

    Status = gBS->OpenProtocol (
                    UdpIo->UdpHandle,
                    &gEfiUdp6ProtocolGuid,
                    (VOID **) &UdpIo->Protocol.Udp6,
                    ImageHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );

    if (EFI_ERROR (Status)) {
      goto FREE_CHILD;
    }

    if (EFI_ERROR (Configure (UdpIo, Context))) {
      goto CLOSE_PROTOCOL;
    }

    Status = UdpIo->Protocol.Udp6->GetModeData (
                                     UdpIo->Protocol.Udp6,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &UdpIo->SnpMode
                                     );

    if (EFI_ERROR (Status)) {
      goto CLOSE_PROTOCOL;
    }
  }

  return UdpIo;

CLOSE_PROTOCOL:
  if (UdpVersion == UDP_IO_UDP4_VERSION) {
    gBS->CloseProtocol (UdpIo->UdpHandle, &gEfiUdp4ProtocolGuid, ImageHandle, Controller);
  } else {
    gBS->CloseProtocol (UdpIo->UdpHandle, &gEfiUdp6ProtocolGuid, ImageHandle, Controller);
  }

FREE_CHILD:
  if (UdpVersion == UDP_IO_UDP4_VERSION) {
    NetLibDestroyServiceChild (
      Controller,
      ImageHandle,
      &gEfiUdp4ServiceBindingProtocolGuid,
      UdpIo->UdpHandle
      );
  } else {
    NetLibDestroyServiceChild (
      Controller,
      ImageHandle,
      &gEfiUdp6ServiceBindingProtocolGuid,
      UdpIo->UdpHandle
      );
  }

FREE_MEM:
  FreePool (UdpIo);
  return NULL;
}

/**
  Cancel all the sent datagram that pass the selection criteria of ToCancel.

  If ToCancel is NULL, all the datagrams are cancelled.
  If Udp version is not UDP_IO_UDP4_VERSION or UDP_IO_UDP6_VERSION, then ASSERT().

  @param[in]  UdpIo                 The UDP_IO to cancel packet.
  @param[in]  IoStatus              The IoStatus to return to the packet owners.
  @param[in]  ToCancel              The select function to test whether to cancel this
                                    packet or not.
  @param[in]  Context               The opaque parameter to the ToCancel.

**/
VOID
EFIAPI
UdpIoCancelDgrams (
  IN UDP_IO                 *UdpIo,
  IN EFI_STATUS             IoStatus,
  IN UDP_IO_TO_CANCEL       ToCancel,        OPTIONAL
  IN VOID                   *Context         OPTIONAL
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  UDP_TX_TOKEN              *TxToken;

  ASSERT ((UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) ||
          (UdpIo->UdpVersion == UDP_IO_UDP6_VERSION));

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &UdpIo->SentDatagram) {
    TxToken = NET_LIST_USER_STRUCT (Entry, UDP_TX_TOKEN, Link);

    if ((ToCancel == NULL) || (ToCancel (TxToken, Context))) {

      if (UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {
        UdpIo->Protocol.Udp4->Cancel (UdpIo->Protocol.Udp4, &TxToken->Token.Udp4);
      } else {
        UdpIo->Protocol.Udp6->Cancel (UdpIo->Protocol.Udp6, &TxToken->Token.Udp6);
      }
    }
  }
}

/**
  Free the UDP_IO and all its related resources.

  If Udp version is not UDP_IO_UDP4_VERSION or UDP_IO_UDP6_VERSION, then ASSERT().

  The function will cancel all sent datagram and receive request.

  @param[in]  UdpIo             The UDP_IO to free.

  @retval EFI_SUCCESS           The UDP_IO is freed.
  @retval Others                Failed to free UDP_IO.

**/
EFI_STATUS
EFIAPI
UdpIoFreeIo (
  IN  UDP_IO           *UdpIo
  )
{
  EFI_STATUS           Status;
  UDP_RX_TOKEN         *RxToken;

  ASSERT ((UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) ||
          (UdpIo->UdpVersion == UDP_IO_UDP6_VERSION));

  //
  // Cancel all the sent datagram and receive requests. The
  // callbacks of transmit requests are executed to allow the
  // caller to release the resource. The callback of receive
  // request are NOT executed. This is because it is most
  // likely that the current user of the UDP IO port is closing
  // itself.
  //
  UdpIoCancelDgrams (UdpIo, EFI_ABORTED, NULL, NULL);

  if (UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {

    if ((RxToken = UdpIo->RecvRequest) != NULL) {
      Status = UdpIo->Protocol.Udp4->Cancel (UdpIo->Protocol.Udp4, &RxToken->Token.Udp4);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    //
    // Close then destroy the Udp4 child
    //
    Status = gBS->CloseProtocol (
                    UdpIo->UdpHandle,
                    &gEfiUdp4ProtocolGuid,
                    UdpIo->Image,
                    UdpIo->Controller
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = NetLibDestroyServiceChild (
               UdpIo->Controller,
               UdpIo->Image,
               &gEfiUdp4ServiceBindingProtocolGuid,
               UdpIo->UdpHandle
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

  } else {

    if ((RxToken = UdpIo->RecvRequest) != NULL) {
      Status = UdpIo->Protocol.Udp6->Cancel (UdpIo->Protocol.Udp6, &RxToken->Token.Udp6);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    //
    // Close then destroy the Udp6 child
    //
    Status = gBS->CloseProtocol (
               UdpIo->UdpHandle,
               &gEfiUdp6ProtocolGuid,
               UdpIo->Image,
               UdpIo->Controller
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = NetLibDestroyServiceChild (
               UdpIo->Controller,
               UdpIo->Image,
               &gEfiUdp6ServiceBindingProtocolGuid,
               UdpIo->UdpHandle
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (!IsListEmpty(&UdpIo->Link)) {
    RemoveEntryList (&UdpIo->Link);
  }

  FreePool (UdpIo);
  return EFI_SUCCESS;
}


/**
  Clean up the UDP_IO without freeing it. The function is called when
  user wants to re-use the UDP_IO later.

  If Udp version is not UDP_IO_UDP4_VERSION or UDP_IO_UDP6_VERSION, then ASSERT().

  It will release all the transmitted datagrams and receive request. It will
  also configure NULL for the UDP instance.

  @param[in]  UdpIo                 The UDP_IO to clean up.

**/
VOID
EFIAPI
UdpIoCleanIo (
  IN  UDP_IO                *UdpIo
  )
{
  UDP_RX_TOKEN              *RxToken;

  ASSERT ((UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) ||
          (UdpIo->UdpVersion == UDP_IO_UDP6_VERSION));

  //
  // Cancel all the sent datagram and receive requests.
  //
  UdpIoCancelDgrams (UdpIo, EFI_ABORTED, NULL, NULL);

  if (UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {
    if ((RxToken = UdpIo->RecvRequest) != NULL) {
      UdpIo->Protocol.Udp4->Cancel (UdpIo->Protocol.Udp4, &RxToken->Token.Udp4);
    }

    UdpIo->Protocol.Udp4->Configure (UdpIo->Protocol.Udp4, NULL);

  } else {
    if ((RxToken = UdpIo->RecvRequest) != NULL) {
      UdpIo->Protocol.Udp6->Cancel (UdpIo->Protocol.Udp6, &RxToken->Token.Udp6);
    }

    UdpIo->Protocol.Udp6->Configure (UdpIo->Protocol.Udp6, NULL);
  }
}

/**
  Send a packet through the UDP_IO.

  If Udp version is not UDP_IO_UDP4_VERSION or UDP_IO_UDP6_VERSION, then ASSERT().

  The packet will be wrapped in UDP_TX_TOKEN. Function Callback will be called
  when the packet is sent. The optional parameter EndPoint overrides the default
  address pair if specified.

  @param[in]  UdpIo                 The UDP_IO to send the packet through.
  @param[in]  Packet                The packet to send.
  @param[in]  EndPoint              The local and remote access point. Override the
                                    default address pair set during configuration.
  @param[in]  Gateway               The gateway to use.
  @param[in]  CallBack              The function being called when packet is
                                    transmitted or failed.
  @param[in]  Context               The opaque parameter passed to CallBack.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for the packet.
  @retval EFI_SUCCESS           The packet is successfully delivered to UDP  for
                                transmission.

**/
EFI_STATUS
EFIAPI
UdpIoSendDatagram (
  IN  UDP_IO                *UdpIo,
  IN  NET_BUF               *Packet,
  IN  UDP_END_POINT         *EndPoint OPTIONAL,
  IN  EFI_IP_ADDRESS        *Gateway  OPTIONAL,
  IN  UDP_IO_CALLBACK       CallBack,
  IN  VOID                  *Context
  )
{
  UDP_TX_TOKEN              *TxToken;
  EFI_STATUS                Status;

  ASSERT ((UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) ||
          (UdpIo->UdpVersion == UDP_IO_UDP6_VERSION));

  TxToken = UdpIoCreateTxToken (UdpIo, Packet, EndPoint, Gateway, CallBack, Context);

  if (TxToken == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Insert the tx token into SendDatagram list before transmitting it. Remove
  // it from the list if the returned status is not EFI_SUCCESS.
  //
  InsertHeadList (&UdpIo->SentDatagram, &TxToken->Link);

  if (UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {
    Status = UdpIo->Protocol.Udp4->Transmit (UdpIo->Protocol.Udp4, &TxToken->Token.Udp4);
  } else {
    Status = UdpIo->Protocol.Udp6->Transmit (UdpIo->Protocol.Udp6, &TxToken->Token.Udp6);
  }

  if (EFI_ERROR (Status)) {
    RemoveEntryList (&TxToken->Link);
    UdpIoFreeTxToken (TxToken);
    return Status;
  }

  return EFI_SUCCESS;
}


/**
  The select function to cancel a single sent datagram.

  @param[in]  Token                 The UDP_TX_TOKEN to test against
  @param[in]  Context               The NET_BUF of the sent datagram

  @retval TRUE              The packet is to be cancelled.
  @retval FALSE             The packet is not to be cancelled.
**/
BOOLEAN
EFIAPI
UdpIoCancelSingleDgram (
  IN UDP_TX_TOKEN           *Token,
  IN VOID                   *Context
  )
{
  NET_BUF                   *Packet;

  Packet = (NET_BUF *) Context;

  if (Token->Packet == Packet) {
    return TRUE;
  }

  return FALSE;
}

/**
  Cancel a single sent datagram.

  @param[in]  UdpIo                 The UDP_IO to cancel the packet from
  @param[in]  Packet                The packet to cancel

**/
VOID
EFIAPI
UdpIoCancelSentDatagram (
  IN  UDP_IO                *UdpIo,
  IN  NET_BUF               *Packet
  )
{
  UdpIoCancelDgrams (UdpIo, EFI_ABORTED, UdpIoCancelSingleDgram, Packet);
}

/**
  Issue a receive request to the UDP_IO.

  If Udp version is not UDP_IO_UDP4_VERSION or UDP_IO_UDP6_VERSION, then ASSERT().

  This function is called when upper-layer needs packet from UDP for processing.
  Only one receive request is acceptable at a time so a common usage model is
  to invoke this function inside its Callback function when the former packet
  is processed.

  @param[in]  UdpIo                 The UDP_IO to receive the packet from.
  @param[in]  CallBack              The call back function to execute when the packet
                                    is received.
  @param[in]  Context               The opaque context passed to Callback.
  @param[in]  HeadLen               The length of the upper-layer's protocol header.

  @retval EFI_ALREADY_STARTED   There is already a pending receive request. Only
                                one receive request is supported at a time.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate needed resources.
  @retval EFI_SUCCESS           The receive request is issued successfully.
  @retval EFI_UNSUPPORTED       The UDP version in UDP_IO is not supported.

**/
EFI_STATUS
EFIAPI
UdpIoRecvDatagram (
  IN  UDP_IO                *UdpIo,
  IN  UDP_IO_CALLBACK       CallBack,
  IN  VOID                  *Context,
  IN  UINT32                HeadLen
  )
{
  UDP_RX_TOKEN              *RxToken;
  EFI_STATUS                Status;

  ASSERT ((UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) ||
          (UdpIo->UdpVersion == UDP_IO_UDP6_VERSION));

  if (UdpIo->RecvRequest != NULL) {
    return EFI_ALREADY_STARTED;
  }

  RxToken = UdpIoCreateRxToken (UdpIo, CallBack, Context, HeadLen);

  if (RxToken == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UdpIo->RecvRequest = RxToken;
  if (UdpIo->UdpVersion == UDP_IO_UDP4_VERSION) {
    Status = UdpIo->Protocol.Udp4->Receive (UdpIo->Protocol.Udp4, &RxToken->Token.Udp4);
  } else {
    Status = UdpIo->Protocol.Udp6->Receive (UdpIo->Protocol.Udp6, &RxToken->Token.Udp6);
  }

  if (EFI_ERROR (Status)) {
    UdpIo->RecvRequest = NULL;
    UdpIoFreeRxToken (RxToken);
  }

  return Status;
}

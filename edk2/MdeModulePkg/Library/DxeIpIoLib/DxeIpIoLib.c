/** @file

Copyright (c) 2005 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IpIo.c

Abstract:

  The implementation of the IpIo layer.


**/

#include <PiDxe.h>

#include <Protocol/Udp4.h>

#include <Library/IpIoLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>


#define NET_PROTO_HDR(Buf, Type)  ((Type *) ((Buf)->BlockOp[0].Head))
#define ICMP_ERRLEN(IpHdr) \
  (sizeof(IP4_ICMP_HEAD) + EFI_IP4_HEADER_LEN(IpHdr) + 8)

LIST_ENTRY  mActiveIpIoList = {
  &mActiveIpIoList,
  &mActiveIpIoList
};

EFI_IP4_CONFIG_DATA  mIpIoDefaultIpConfigData = {
  EFI_IP_PROTO_UDP,
  FALSE,
  TRUE,
  FALSE,
  FALSE,
  FALSE,
  {{0, 0, 0, 0}},
  {{0, 0, 0, 0}},
  0,
  255,
  FALSE,
  FALSE,
  0,
  0
};

ICMP_ERROR_INFO  mIcmpErrMap[10] = {
  {FALSE, TRUE},
  {FALSE, TRUE},
  {TRUE, TRUE},
  {TRUE, TRUE},
  {TRUE, TRUE},
  {FALSE, TRUE},
  {FALSE, TRUE},
  {FALSE, TRUE},
  {FALSE, FALSE},
  {FALSE, TRUE}
};

VOID
EFIAPI
IpIoTransmitHandlerDpc (
  IN VOID      *Context
  );

VOID
EFIAPI
IpIoTransmitHandler (
  IN EFI_EVENT Event,
  IN VOID      *Context
  );


/**
  This function create an IP child, open the IP protocol and return the opened
  IP protocol as Interface.

  @param  ControllerHandle      The controller handle.
  @param  ImageHandle           The image handle.
  @param  ChildHandle           Pointer to the buffer to save the IP child handle.
  @param  Interface             Pointer used to get the IP protocol interface.

  @retval EFI_SUCCESS           The IP child is created and the IP protocol
                                interface is retrieved.
  @retval Other                 The required operation failed.

**/
EFI_STATUS
IpIoCreateIpChildOpenProtocol (
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  ImageHandle,
  IN  EFI_HANDLE  *ChildHandle,
  OUT VOID        **Interface
  )
{
  EFI_STATUS  Status;

  //
  // Create an ip child.
  //
  Status = NetLibCreateServiceChild (
             ControllerHandle,
             ImageHandle,
             &gEfiIp4ServiceBindingProtocolGuid,
             ChildHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the ip protocol installed on the *ChildHandle.
  //
  Status = gBS->OpenProtocol (
                  *ChildHandle,
                  &gEfiIp4ProtocolGuid,
                  Interface,
                  ImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    //
    // On failure, destroy the ip child.
    //
    NetLibDestroyServiceChild (
      ControllerHandle,
      ImageHandle,
      &gEfiIp4ServiceBindingProtocolGuid,
      *ChildHandle
      );
  }

  return Status;
}


/**
  This function close the previously openned IP protocol and destroy the IP child.

  @param  ControllerHandle      The controller handle.
  @param  ImageHandle           the image handle.
  @param  ChildHandle           The child handle of the IP child.

  @retval EFI_SUCCESS           The IP protocol is closed and the relevant IP child
                                is destroyed.
  @retval Other                 The required operation failed.

**/
EFI_STATUS
IpIoCloseProtocolDestroyIpChild (
  IN EFI_HANDLE  ControllerHandle,
  IN EFI_HANDLE  ImageHandle,
  IN EFI_HANDLE  ChildHandle
  )
{
  EFI_STATUS  Status;

  //
  // Close the previously openned ip protocol.
  //
  gBS->CloseProtocol (
         ChildHandle,
         &gEfiIp4ProtocolGuid,
         ImageHandle,
         ControllerHandle
         );

  //
  // Destroy the ip child.
  //
  Status = NetLibDestroyServiceChild (
             ControllerHandle,
             ImageHandle,
             &gEfiIp4ServiceBindingProtocolGuid,
             ChildHandle
             );

  return Status;
}


/**
  This function handles ICMP packets.

  @param  IpIo                  Pointer to the IP_IO instance.
  @param  Pkt                   Pointer to the ICMP packet.
  @param  Session               Pointer to the net session of this ICMP packet.

  @retval EFI_SUCCESS           The ICMP packet is handled successfully.
  @retval EFI_ABORTED           This type of ICMP packet is not supported.

**/
EFI_STATUS
IpIoIcmpHandler (
  IN IP_IO                *IpIo,
  IN NET_BUF              *Pkt,
  IN EFI_NET_SESSION_DATA *Session
  )
{
  IP4_ICMP_ERROR_HEAD  *IcmpHdr;
  EFI_IP4_HEADER       *IpHdr;
  ICMP_ERROR           IcmpErr;
  UINT8                *PayLoadHdr;
  UINT8                Type;
  UINT8                Code;
  UINT32               TrimBytes;

  IcmpHdr = NET_PROTO_HDR (Pkt, IP4_ICMP_ERROR_HEAD);
  IpHdr   = (EFI_IP4_HEADER *) (&IcmpHdr->IpHead);

  //
  // Check the ICMP packet length.
  //
  if (Pkt->TotalSize < ICMP_ERRLEN (IpHdr)) {

    return EFI_ABORTED;
  }

  Type = IcmpHdr->Head.Type;
  Code = IcmpHdr->Head.Code;

  //
  // Analyze the ICMP Error in this ICMP pkt
  //
  switch (Type) {
  case ICMP_TYPE_UNREACH:
    switch (Code) {
    case ICMP_CODE_UNREACH_NET:
    case ICMP_CODE_UNREACH_HOST:
    case ICMP_CODE_UNREACH_PROTOCOL:
    case ICMP_CODE_UNREACH_PORT:
    case ICMP_CODE_UNREACH_SRCFAIL:
      IcmpErr = (ICMP_ERROR) (ICMP_ERR_UNREACH_NET + Code);

      break;

    case ICMP_CODE_UNREACH_NEEDFRAG:
      IcmpErr = ICMP_ERR_MSGSIZE;

      break;

    case ICMP_CODE_UNREACH_NET_UNKNOWN:
    case ICMP_CODE_UNREACH_NET_PROHIB:
    case ICMP_CODE_UNREACH_TOSNET:
      IcmpErr = ICMP_ERR_UNREACH_NET;

      break;

    case ICMP_CODE_UNREACH_HOST_UNKNOWN:
    case ICMP_CODE_UNREACH_ISOLATED:
    case ICMP_CODE_UNREACH_HOST_PROHIB:
    case ICMP_CODE_UNREACH_TOSHOST:
      IcmpErr = ICMP_ERR_UNREACH_HOST;

      break;

    default:
      return EFI_ABORTED;

      break;
    }

    break;

  case ICMP_TYPE_TIMXCEED:
    if (Code > 1) {
      return EFI_ABORTED;
    }

    IcmpErr = (ICMP_ERROR) (Code + ICMP_ERR_TIMXCEED_INTRANS);

    break;

  case ICMP_TYPE_PARAMPROB:
    if (Code > 1) {
      return EFI_ABORTED;
    }

    IcmpErr = ICMP_ERR_PARAMPROB;

    break;

  case ICMP_TYPE_SOURCEQUENCH:
    if (Code != 0) {
      return EFI_ABORTED;
    }

    IcmpErr = ICMP_ERR_QUENCH;

    break;

  default:
    return EFI_ABORTED;

    break;
  }

  //
  // Notify user the ICMP pkt only containing payload except
  // IP and ICMP header
  //
  PayLoadHdr = (UINT8 *) ((UINT8 *) IpHdr + EFI_IP4_HEADER_LEN (IpHdr));
  TrimBytes  = (UINT32) (PayLoadHdr - (UINT8 *) IcmpHdr);

  NetbufTrim (Pkt, TrimBytes, TRUE);

  IpIo->PktRcvdNotify (EFI_ICMP_ERROR, IcmpErr, Session, Pkt, IpIo->RcvdContext);

  return EFI_SUCCESS;
}


/**
  Free function for receive token of IP_IO. It is used to
  signal the recycle event to notify IP to recycle the
  data buffer.

  @param  Event                 The event to be signaled.

**/
VOID
IpIoExtFree (
  IN VOID  *Event
  )
{
  gBS->SignalEvent ((EFI_EVENT) Event);
}


/**
  Create a send entry to wrap a packet before sending
  out it through IP.

  @param  IpIo                  Pointer to the IP_IO instance.
  @param  Pkt                   Pointer to the packet.
  @param  Sender                Pointer to the IP sender.
  @param  NotifyData            Pointer to the notify data.
  @param  Dest                  Pointer to the destination IP address.
  @param  Override              Pointer to the overriden IP_IO data.

  @return Pointer to the data structure created to wrap the packet. If NULL,
          resource limit occurred.

**/
IP_IO_SEND_ENTRY *
IpIoCreateSndEntry (
  IN IP_IO             *IpIo,
  IN NET_BUF           *Pkt,
  IN EFI_IP4_PROTOCOL  *Sender,
  IN VOID              *Context    OPTIONAL,
  IN VOID              *NotifyData OPTIONAL,
  IN IP4_ADDR          Dest,
  IN IP_IO_OVERRIDE    *Override
  )
{
  IP_IO_SEND_ENTRY          *SndEntry;
  EFI_IP4_COMPLETION_TOKEN  *SndToken;
  EFI_IP4_TRANSMIT_DATA     *TxData;
  EFI_STATUS                Status;
  EFI_IP4_OVERRIDE_DATA     *OverrideData;
  volatile UINT32           Index;

  //
  // Allocate resource for SndEntry
  //
  SndEntry = AllocatePool (sizeof (IP_IO_SEND_ENTRY));
  if (NULL == SndEntry) {
    return NULL;
  }

  //
  // Allocate resource for SndToken
  //
  SndToken = AllocatePool (sizeof (EFI_IP4_COMPLETION_TOKEN));
  if (NULL == SndToken) {
    goto ReleaseSndEntry;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IpIoTransmitHandler,
                  SndEntry,
                  &(SndToken->Event)
                  );
  if (EFI_ERROR (Status)) {
    goto ReleaseSndToken;
  }

  //
  // Allocate resource for TxData
  //
  TxData = AllocatePool (
    sizeof (EFI_IP4_TRANSMIT_DATA) +
    sizeof (EFI_IP4_FRAGMENT_DATA) * (Pkt->BlockOpNum - 1)
    );

  if (NULL == TxData) {
    goto ReleaseEvent;
  }

  //
  // Allocate resource for OverrideData if needed
  //
  OverrideData = NULL;
  if (NULL != Override) {

    OverrideData = AllocatePool (sizeof (EFI_IP4_OVERRIDE_DATA));
    if (NULL == OverrideData) {
      goto ReleaseResource;
    }
    //
    // Set the fields of OverrideData
    //
    CopyMem (OverrideData, Override, sizeof (*OverrideData));
  }

  //
  // Set the fields of TxData
  //
  CopyMem (&TxData->DestinationAddress, &Dest, sizeof (EFI_IPv4_ADDRESS));
  TxData->OverrideData                  = OverrideData;
  TxData->OptionsLength                 = 0;
  TxData->OptionsBuffer                 = NULL;
  TxData->TotalDataLength               = Pkt->TotalSize;
  TxData->FragmentCount                 = Pkt->BlockOpNum;


  for (Index = 0; Index < Pkt->BlockOpNum; Index++) {
    TxData->FragmentTable[Index].FragmentBuffer = Pkt->BlockOp[Index].Head;
    TxData->FragmentTable[Index].FragmentLength = Pkt->BlockOp[Index].Size;
  }

  //
  // Set the fields of SndToken
  //
  SndToken->Packet.TxData = TxData;

  //
  // Set the fields of SndEntry
  //
  SndEntry->IpIo        = IpIo;
  SndEntry->Ip      = Sender;
  SndEntry->Context     = Context;
  SndEntry->NotifyData  = NotifyData;

  SndEntry->Pkt         = Pkt;
  NET_GET_REF (Pkt);

  SndEntry->SndToken = SndToken;

  InsertTailList (&IpIo->PendingSndList, &SndEntry->Entry);

  return SndEntry;

ReleaseResource:
  gBS->FreePool (TxData);

ReleaseEvent:
  gBS->CloseEvent (SndToken->Event);

ReleaseSndToken:
  gBS->FreePool (SndToken);

ReleaseSndEntry:
  gBS->FreePool (SndEntry);

  return NULL;
}


/**
  Destroy the SndEntry.
  
  This function pairs with IpIoCreateSndEntry().

  @param  SndEntry              Pointer to the send entry to be destroyed.

**/
VOID
IpIoDestroySndEntry (
  IN IP_IO_SEND_ENTRY  *SndEntry
  )
{
  EFI_IP4_TRANSMIT_DATA  *TxData;

  TxData = SndEntry->SndToken->Packet.TxData;

  if (NULL != TxData->OverrideData) {
    gBS->FreePool (TxData->OverrideData);
  }

  gBS->FreePool (TxData);
  NetbufFree (SndEntry->Pkt);
  gBS->CloseEvent (SndEntry->SndToken->Event);

  gBS->FreePool (SndEntry->SndToken);
  RemoveEntryList (&SndEntry->Entry);

  gBS->FreePool (SndEntry);
}


/**
  Notify function for IP transmit token.

  @param  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
IpIoTransmitHandlerDpc (
  IN VOID      *Context
  )
{
  IP_IO             *IpIo;
  IP_IO_SEND_ENTRY  *SndEntry;

  SndEntry  = (IP_IO_SEND_ENTRY *) Context;

  IpIo      = SndEntry->IpIo;

  if (IpIo->PktSentNotify && SndEntry->NotifyData) {
    IpIo->PktSentNotify (
            SndEntry->SndToken->Status,
            SndEntry->Context,
            SndEntry->Ip,
            SndEntry->NotifyData
            );
  }

  IpIoDestroySndEntry (SndEntry);
}

/**
  Notify function for IP transmit token.

  @param  Event                 The event signaled.
  @param  Context               The context passed in by the event notifier.

**/

VOID
EFIAPI
IpIoTransmitHandler (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  //
  // Request IpIoTransmitHandlerDpc as a DPC at TPL_CALLBACK
  //
  NetLibQueueDpc (TPL_CALLBACK, IpIoTransmitHandlerDpc, Context);
}


/**
  The dummy handler for the dummy IP receive token.

  @param  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
IpIoDummyHandlerDpc (
  IN VOID      *Context
  )
{
  IP_IO_IP_INFO             *IpInfo;
  EFI_IP4_COMPLETION_TOKEN  *DummyToken;

  IpInfo      = (IP_IO_IP_INFO *) Context;
  DummyToken  = &(IpInfo->DummyRcvToken);

  if (EFI_ABORTED == DummyToken->Status) {
    //
    // The reception is actively aborted by the consumer, directly return.
    //
    return;
  } else if (EFI_SUCCESS == DummyToken->Status) {
    ASSERT (DummyToken->Packet.RxData);

    gBS->SignalEvent (DummyToken->Packet.RxData->RecycleSignal);
  }

  IpInfo->Ip->Receive (IpInfo->Ip, DummyToken);
}


/**
  Request IpIoDummyHandlerDpc as a DPC at TPL_CALLBACK.

  @param  Event                 The event signaled.
  @param  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
IpIoDummyHandler (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  //
  // Request IpIoDummyHandlerDpc as a DPC at TPL_CALLBACK
  //
  NetLibQueueDpc (TPL_CALLBACK, IpIoDummyHandlerDpc, Context);
}


/**
  Notify function for the IP receive token, used to process
  the received IP packets.

  @param  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
IpIoListenHandlerDpc (
  IN VOID      *Context
  )
{
  IP_IO                 *IpIo;
  EFI_STATUS            Status;
  EFI_IP4_RECEIVE_DATA  *RxData;
  EFI_IP4_PROTOCOL      *Ip;
  EFI_NET_SESSION_DATA  Session;
  NET_BUF               *Pkt;

  IpIo    = (IP_IO *) Context;

  Ip      = IpIo->Ip;
  Status  = IpIo->RcvToken.Status;
  RxData  = IpIo->RcvToken.Packet.RxData;

  if (EFI_ABORTED == Status) {
    //
    // The reception is actively aborted by the consumer, directly return.
    //
    return;
  }

  if (((EFI_SUCCESS != Status) && (EFI_ICMP_ERROR != Status)) || (NULL == RxData)) {
    //
    // Only process the normal packets and the icmp error packets, if RxData is NULL
    // with Status == EFI_SUCCESS or EFI_ICMP_ERROR, just resume the receive although
    // this should be a bug of the low layer (IP).
    //
    goto Resume;
  }

  if (NULL == IpIo->PktRcvdNotify) {
    goto CleanUp;
  }

  if ((EFI_IP4 (RxData->Header->SourceAddress) != 0) &&
    !Ip4IsUnicast (EFI_NTOHL (RxData->Header->SourceAddress), 0)) {
    //
    // The source address is not zero and it's not a unicast IP address, discard it.
    //
    goto CleanUp;
  }

  //
  // Create a netbuffer representing packet
  //
  Pkt = NetbufFromExt (
          (NET_FRAGMENT *) RxData->FragmentTable,
          RxData->FragmentCount,
          0,
          0,
          IpIoExtFree,
          RxData->RecycleSignal
          );
  if (NULL == Pkt) {
    goto CleanUp;
  }

  //
  // Create a net session
  //
  Session.Source = EFI_IP4 (RxData->Header->SourceAddress);
  Session.Dest   = EFI_IP4 (RxData->Header->DestinationAddress);
  Session.IpHdr  = RxData->Header;

  if (EFI_SUCCESS == Status) {

    IpIo->PktRcvdNotify (EFI_SUCCESS, (ICMP_ERROR) 0, &Session, Pkt, IpIo->RcvdContext);
  } else {
    //
    // Status is EFI_ICMP_ERROR
    //
    Status = IpIoIcmpHandler (IpIo, Pkt, &Session);
    if (EFI_ERROR (Status)) {
      NetbufFree (Pkt);
    }
  }

  goto Resume;

CleanUp:
  gBS->SignalEvent (RxData->RecycleSignal);

Resume:
  Ip->Receive (Ip, &(IpIo->RcvToken));
}


/**
  Request IpIoListenHandlerDpc as a DPC at TPL_CALLBACK

  @param  Event                 The event signaled.
  @param  Context               The context passed in by the event notifier.

  @return None.

**/
VOID
EFIAPI
IpIoListenHandler (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  //
  // Request IpIoListenHandlerDpc as a DPC at TPL_CALLBACK
  //
  NetLibQueueDpc (TPL_CALLBACK, IpIoListenHandlerDpc, Context);
}


/**
  Create a new IP_IO instance.
  
  This function uses IP4 service binding protocol in Controller to create an IP4
  child (aka IP4 instance).

  @param  Image                 The image handle of the driver or application that
                                consumes IP_IO.
  @param  Controller            The controller handle that has IP4 service binding
                                protocol installed.

  @return Pointer to a newly created IP_IO instance, or NULL if failed.

**/
IP_IO *
EFIAPI
IpIoCreate (
  IN EFI_HANDLE Image,
  IN EFI_HANDLE Controller
  )
{
  EFI_STATUS  Status;
  IP_IO       *IpIo;

  IpIo = AllocateZeroPool (sizeof (IP_IO));
  if (NULL == IpIo) {
    return NULL;
  }

  InitializeListHead (&(IpIo->PendingSndList));
  InitializeListHead (&(IpIo->IpList));
  IpIo->Controller  = Controller;
  IpIo->Image       = Image;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IpIoListenHandler,
                  IpIo,
                  &(IpIo->RcvToken.Event)
                  );
  if (EFI_ERROR (Status)) {
    goto ReleaseIpIo;
  }

  //
  // Create an IP child and open IP protocol
  //
  Status = IpIoCreateIpChildOpenProtocol (
             Controller,
             Image,
             &IpIo->ChildHandle,
             (VOID **)&(IpIo->Ip)
             );
  if (EFI_ERROR (Status)) {
    goto ReleaseIpIo;
  }

  return IpIo;

ReleaseIpIo:

  if (NULL != IpIo->RcvToken.Event) {
    gBS->CloseEvent (IpIo->RcvToken.Event);
  }

  gBS->FreePool (IpIo);

  return NULL;
}


/**
  Open an IP_IO instance for use.
  
  This function is called after IpIoCreate(). It is used for configuring the IP
  instance and register the callbacks and their context data for sending and
  receiving IP packets.

  @param  IpIo                  Pointer to an IP_IO instance that needs to open.
  @param  OpenData              The configuration data and callbacks for the IP_IO
                                instance.

  @retval EFI_SUCCESS           The IP_IO instance opened with OpenData
                                successfully.
  @retval Other                 Error condition occurred.

**/
EFI_STATUS
EFIAPI
IpIoOpen (
  IN IP_IO           *IpIo,
  IN IP_IO_OPEN_DATA *OpenData
  )
{
  EFI_STATUS        Status;
  EFI_IP4_PROTOCOL  *Ip;

  if (IpIo->IsConfigured) {
    return EFI_ACCESS_DENIED;
  }

  Ip = IpIo->Ip;

  //
  // configure ip
  //
  Status = Ip->Configure (Ip, &OpenData->IpConfigData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // bugbug: to delete the default route entry in this Ip, if it is:
  // (0.0.0.0, 0.0.0.0, 0.0.0.0). Delete this statement if Ip modified
  // its code
  //
  Status = Ip->Routes (Ip, TRUE, &mZeroIp4Addr, &mZeroIp4Addr, &mZeroIp4Addr);

  if (EFI_ERROR (Status) && (EFI_NOT_FOUND != Status)) {
    return Status;
  }

  IpIo->PktRcvdNotify = OpenData->PktRcvdNotify;
  IpIo->PktSentNotify = OpenData->PktSentNotify;

  IpIo->RcvdContext   = OpenData->RcvdContext;
  IpIo->SndContext    = OpenData->SndContext;

  IpIo->Protocol      = OpenData->IpConfigData.DefaultProtocol;

  //
  // start to listen incoming packet
  //
  Status = Ip->Receive (Ip, &(IpIo->RcvToken));
  if (EFI_ERROR (Status)) {
    Ip->Configure (Ip, NULL);
    goto ErrorExit;
  }

  IpIo->IsConfigured = TRUE;
  InsertTailList (&mActiveIpIoList, &IpIo->Entry);

ErrorExit:

  return Status;
}


/**
  Stop an IP_IO instance.
  
  This function is paired with IpIoOpen(). The IP_IO will be unconfigured and all
  the pending send/receive tokens will be canceled.

  @param  IpIo                  Pointer to the IP_IO instance that needs to stop.

  @retval EFI_SUCCESS           The IP_IO instance stopped successfully.
  @retval Other                 Error condition occurred.

**/
EFI_STATUS
EFIAPI
IpIoStop (
  IN IP_IO *IpIo
  )
{
  EFI_STATUS        Status;
  EFI_IP4_PROTOCOL  *Ip;
  IP_IO_IP_INFO     *IpInfo;

  if (!IpIo->IsConfigured) {
    return EFI_SUCCESS;
  }

  //
  // Remove the IpIo from the active IpIo list.
  //
  RemoveEntryList (&IpIo->Entry);

  Ip = IpIo->Ip;

  //
  // Configure NULL Ip
  //
  Status = Ip->Configure (Ip, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IpIo->IsConfigured = FALSE;

  //
  // Detroy the Ip List used by IpIo
  //

  while (!IsListEmpty (&(IpIo->IpList))) {
    IpInfo = NET_LIST_HEAD (&(IpIo->IpList), IP_IO_IP_INFO, Entry);

    IpIoRemoveIp (IpIo, IpInfo);
  }

  //
  // All pending snd tokens should be flushed by reseting the IP instances.
  //
  ASSERT (IsListEmpty (&IpIo->PendingSndList));

  //
  // Close the receive event.
  //
  gBS->CloseEvent (IpIo->RcvToken.Event);

  return EFI_SUCCESS;
}


/**
  Destroy an IP_IO instance.
  
  This function is paired with IpIoCreate(). The IP_IO will be closed first.
  Resource will be freed afterwards. See IpIoClose().

  @param  IpIo                  Pointer to the IP_IO instance that needs to be
                                destroyed.

  @retval EFI_SUCCESS           The IP_IO instance destroyed successfully.
  @retval Other                 Error condition occurred.

**/
EFI_STATUS
EFIAPI
IpIoDestroy (
  IN IP_IO *IpIo
  )
{
  //
  // Stop the IpIo.
  //
  IpIoStop (IpIo);

  //
  // Close the IP protocol and destroy the child.
  //
  IpIoCloseProtocolDestroyIpChild (IpIo->Controller, IpIo->Image, IpIo->ChildHandle);

  gBS->FreePool (IpIo);

  return EFI_SUCCESS;
}


/**
  Send out an IP packet.
  
  This function is called after IpIoOpen(). The data to be sent are wrapped in
  Pkt. The IP instance wrapped in IpIo is used for sending by default but can be
  overriden by Sender. Other sending configs, like source address and gateway
  address etc., are specified in OverrideData.

  @param  IpIo                  Pointer to an IP_IO instance used for sending IP
                                packet.
  @param  Pkt                   Pointer to the IP packet to be sent.
  @param  Sender                The IP protocol instance used for sending.
  @param  Context               Optional context data
  @param  NotifyData            Optional notify data
  @param  Dest                  The destination IP address to send this packet to.
  @param  OverrideData          The data to override some configuration of the IP
                                instance used for sending.

  @retval EFI_SUCCESS           The operation is completed successfully.
  @retval EFI_NOT_STARTED       The IpIo is not configured.
  @retval EFI_OUT_OF_RESOURCES  Failed due to resource limit.

**/
EFI_STATUS
EFIAPI
IpIoSend (
  IN IP_IO           *IpIo,
  IN NET_BUF         *Pkt,
  IN IP_IO_IP_INFO   *Sender        OPTIONAL,
  IN VOID            *Context       OPTIONAL,
  IN VOID            *NotifyData    OPTIONAL,
  IN IP4_ADDR        Dest,
  IN IP_IO_OVERRIDE  *OverrideData  OPTIONAL
  )
{
  EFI_STATUS        Status;
  EFI_IP4_PROTOCOL  *Ip;
  IP_IO_SEND_ENTRY  *SndEntry;

  if (!IpIo->IsConfigured) {
    return EFI_NOT_STARTED;
  }

  Ip = (NULL == Sender) ? IpIo->Ip : Sender->Ip;

  //
  // create a new SndEntry
  //
  SndEntry = IpIoCreateSndEntry (IpIo, Pkt, Ip, Context, NotifyData, Dest, OverrideData);
  if (NULL == SndEntry) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Send this Packet
  //
  Status = Ip->Transmit (Ip, SndEntry->SndToken);
  if (EFI_ERROR (Status)) {
    IpIoDestroySndEntry (SndEntry);
  }

  return Status;
}


/**
  Cancel the IP transmit token which wraps this Packet.

  @param  IpIo                  Pointer to the IP_IO instance.
  @param  Packet                Pointer to the packet of NET_BUF to cancel.

**/
VOID
EFIAPI
IpIoCancelTxToken (
  IN IP_IO  *IpIo,
  IN VOID   *Packet
  )
{
  LIST_ENTRY        *Node;
  IP_IO_SEND_ENTRY  *SndEntry;
  EFI_IP4_PROTOCOL  *Ip;

  ASSERT (IpIo && Packet);

  NET_LIST_FOR_EACH (Node, &IpIo->PendingSndList) {

    SndEntry = NET_LIST_USER_STRUCT (Node, IP_IO_SEND_ENTRY, Entry);

    if (SndEntry->Pkt == Packet) {

      Ip = SndEntry->Ip;
      Ip->Cancel (Ip, SndEntry->SndToken);

      break;
    }
  }

}


/**
  Add a new IP instance for sending data.
  
  The function is used to add the IP_IO to the IP_IO sending list. The caller
  can later use IpIoFindSender() to get the IP_IO and call IpIoSend() to send
  data.

  @param  IpIo                  Pointer to a IP_IO instance to add a new IP
                                instance for sending purpose.

  @return Pointer to the created IP_IO_IP_INFO structure, NULL if failed.

**/
IP_IO_IP_INFO *
EFIAPI
IpIoAddIp (
  IN IP_IO  *IpIo
  )
{
  EFI_STATUS     Status;
  IP_IO_IP_INFO  *IpInfo;

  ASSERT (IpIo);

  IpInfo = AllocatePool (sizeof (IP_IO_IP_INFO));
  if (IpInfo == NULL) {
    return IpInfo;
  }

  //
  // Init this IpInfo, set the Addr and SubnetMask to 0 before we configure the IP
  // instance.
  //
  InitializeListHead (&IpInfo->Entry);
  IpInfo->ChildHandle = NULL;
  IpInfo->Addr        = 0;
  IpInfo->SubnetMask  = 0;
  IpInfo->RefCnt      = 1;

  //
  // Create the IP instance and open the Ip4 protocol.
  //
  Status = IpIoCreateIpChildOpenProtocol (
             IpIo->Controller,
             IpIo->Image,
             &IpInfo->ChildHandle,
             (VOID **) &IpInfo->Ip
             );
  if (EFI_ERROR (Status)) {
    goto ReleaseIpInfo;
  }

  //
  // Create the event for the DummyRcvToken.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IpIoDummyHandler,
                  IpInfo,
                  &IpInfo->DummyRcvToken.Event
                  );
  if (EFI_ERROR (Status)) {
    goto ReleaseIpChild;
  }

  //
  // Link this IpInfo into the IpIo.
  //
  InsertTailList (&IpIo->IpList, &IpInfo->Entry);

  return IpInfo;

ReleaseIpChild:

  IpIoCloseProtocolDestroyIpChild (
    IpIo->Controller,
    IpIo->Image,
    IpInfo->ChildHandle
    );

ReleaseIpInfo:

  gBS->FreePool (IpInfo);

  return NULL;
}


/**
  Configure the IP instance of this IpInfo and start the receiving if Ip4ConfigData
  is not NULL.

  @param  IpInfo                Pointer to the IP_IO_IP_INFO instance.
  @param  Ip4ConfigData         The IP4 configure data used to configure the IP
                                instance, if NULL the IP instance is reset. If
                                UseDefaultAddress is set to TRUE, and the configure
                                operation succeeds, the default address information
                                is written back in this Ip4ConfigData.

  @retval EFI_STATUS            The status returned by IP4->Configure or
                                IP4->Receive.
  @retval Other                 Configuration fails.

**/
EFI_STATUS
EFIAPI
IpIoConfigIp (
  IN     IP_IO_IP_INFO        *IpInfo,
  IN OUT EFI_IP4_CONFIG_DATA  *Ip4ConfigData OPTIONAL
  )
{
  EFI_STATUS         Status;
  EFI_IP4_PROTOCOL   *Ip;
  EFI_IP4_MODE_DATA  Ip4ModeData;

  ASSERT (IpInfo);

  if (IpInfo->RefCnt > 1) {
    //
    // This IP instance is shared, don't reconfigure it until it has only one
    // consumer. Currently, only the tcp children cloned from their passive parent
    // will share the same IP. So this cases only happens while Ip4ConfigData is NULL,
    // let the last consumer clean the IP instance.
    //
    return EFI_SUCCESS;
  }

  Ip = IpInfo->Ip;

  Status = Ip->Configure (Ip, Ip4ConfigData);
  if (EFI_ERROR (Status)) {
    goto OnExit;
  }

  if (Ip4ConfigData != NULL) {

    if (Ip4ConfigData->UseDefaultAddress) {
      Ip->GetModeData (Ip, &Ip4ModeData, NULL, NULL);

      Ip4ConfigData->StationAddress = Ip4ModeData.ConfigData.StationAddress;
      Ip4ConfigData->SubnetMask     = Ip4ModeData.ConfigData.SubnetMask;
    }

    CopyMem (&IpInfo->Addr, &Ip4ConfigData->StationAddress, sizeof (IP4_ADDR));
    CopyMem (&IpInfo->SubnetMask, &Ip4ConfigData->SubnetMask, sizeof (IP4_ADDR));

    Status = Ip->Receive (Ip, &IpInfo->DummyRcvToken);
    if (EFI_ERROR (Status)) {
      Ip->Configure (Ip, NULL);
    }
  } else {

    //
    // The IP instance is reseted, set the stored Addr and SubnetMask to zero.
    //
    IpInfo->Addr       = 0;
    IpInfo->SubnetMask =0;
  }

OnExit:

  return Status;
}


/**
  Destroy an IP instance maintained in IpIo->IpList for
  sending purpose.
  
  This function pairs with IpIoAddIp(). The IpInfo is previously created by
  IpIoAddIp(). The IP_IO_IP_INFO::RefCnt is decremented and the IP instance
  will be dstroyed if the RefCnt is zero.

  @param  IpIo                  Pointer to the IP_IO instance.
  @param  IpInfo                Pointer to the IpInfo to be removed.

**/
VOID
EFIAPI
IpIoRemoveIp (
  IN IP_IO            *IpIo,
  IN IP_IO_IP_INFO    *IpInfo
  )
{
  ASSERT (IpInfo->RefCnt > 0);

  NET_PUT_REF (IpInfo);

  if (IpInfo->RefCnt > 0) {

    return;
  }

  RemoveEntryList (&IpInfo->Entry);

  IpInfo->Ip->Configure (IpInfo->Ip, NULL);

  IpIoCloseProtocolDestroyIpChild (IpIo->Controller, IpIo->Image, IpInfo->ChildHandle);

  gBS->CloseEvent (IpInfo->DummyRcvToken.Event);

  gBS->FreePool (IpInfo);
}


/**
  Find the first IP protocol maintained in IpIo whose local
  address is the same with Src.
  
  This function is called when the caller needs the IpIo to send data to the
  specified Src. The IpIo was added previously by IpIoAddIp().

  @param  IpIo                  Pointer to the pointer of the IP_IO instance.
  @param  Src                   The local IP address.

  @return Pointer to the IP protocol can be used for sending purpose and its local
          address is the same with Src.

**/
IP_IO_IP_INFO *
EFIAPI
IpIoFindSender (
  IN OUT IP_IO     **IpIo,
  IN     IP4_ADDR  Src
  )
{
  LIST_ENTRY      *IpIoEntry;
  IP_IO           *IpIoPtr;
  LIST_ENTRY      *IpInfoEntry;
  IP_IO_IP_INFO   *IpInfo;

  NET_LIST_FOR_EACH (IpIoEntry, &mActiveIpIoList) {
    IpIoPtr = NET_LIST_USER_STRUCT (IpIoEntry, IP_IO, Entry);

    if ((*IpIo != NULL) && (*IpIo != IpIoPtr)) {
      continue;
    }

    NET_LIST_FOR_EACH (IpInfoEntry, &IpIoPtr->IpList) {
      IpInfo = NET_LIST_USER_STRUCT (IpInfoEntry, IP_IO_IP_INFO, Entry);

      if (IpInfo->Addr == Src) {
        *IpIo = IpIoPtr;
        return IpInfo;
      }
    }
  }

  //
  // No match.
  //
  return NULL;
}


/**
  Get the ICMP error map information.
  
  The ErrorStatus will be returned. The IsHard and Notify are optional. If they
  are not NULL, this routine will fill them.

  @param  IcmpError             IcmpError Type
  @param  IsHard                Whether it is a hard error
  @param  Notify                Whether it need to notify SockError

  @return ICMP Error Status

**/
EFI_STATUS
EFIAPI
IpIoGetIcmpErrStatus (
  IN  ICMP_ERROR  IcmpError,
  OUT BOOLEAN     *IsHard  OPTIONAL,
  OUT BOOLEAN     *Notify  OPTIONAL
  )
{
  ASSERT ((IcmpError >= ICMP_ERR_UNREACH_NET) && (IcmpError <= ICMP_ERR_PARAMPROB));

  if (IsHard != NULL) {
    *IsHard = mIcmpErrMap[IcmpError].IsHard;
  }

  if (Notify != NULL) {
    *Notify = mIcmpErrMap[IcmpError].Notify;
  }

  switch (IcmpError) {
  case ICMP_ERR_UNREACH_NET:
    return  EFI_NETWORK_UNREACHABLE;

  case ICMP_ERR_TIMXCEED_INTRANS:
  case ICMP_ERR_TIMXCEED_REASS:
  case ICMP_ERR_UNREACH_HOST:
    return  EFI_HOST_UNREACHABLE;

  case ICMP_ERR_UNREACH_PROTOCOL:
    return  EFI_PROTOCOL_UNREACHABLE;

  case ICMP_ERR_UNREACH_PORT:
    return  EFI_PORT_UNREACHABLE;

  case ICMP_ERR_MSGSIZE:
  case ICMP_ERR_UNREACH_SRCFAIL:
  case ICMP_ERR_QUENCH:
  case ICMP_ERR_PARAMPROB:
    return  EFI_ICMP_ERROR;
  }

  //
  // will never run here!
  //
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}


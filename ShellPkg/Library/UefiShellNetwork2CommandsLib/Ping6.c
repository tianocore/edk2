/** @file
  The implementation for Ping6 application.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellNetwork2CommandsLib.h"

#define PING6_DEFAULT_TIMEOUT  5000
#define PING6_MAX_SEND_NUMBER  10000
#define PING6_MAX_BUFFER_SIZE  32768
#define PING6_ONE_SECOND       10000000
#define STALL_1_MILLI_SECOND   1000

#pragma pack(1)

typedef struct _ICMP6_ECHO_REQUEST_REPLY {
  UINT8     Type;
  UINT8     Code;
  UINT16    Checksum;
  UINT16    Identifier;
  UINT16    SequenceNum;
  UINT32    TimeStamp;
  UINT8     Data[1];
} ICMP6_ECHO_REQUEST_REPLY;

#pragma pack()

typedef struct _PING6_ICMP6_TX_INFO {
  LIST_ENTRY                  Link;
  UINT16                      SequenceNum;
  UINT32                      TimeStamp;
  EFI_IP6_COMPLETION_TOKEN    *Token;
} PING6_ICMP6_TX_INFO;

typedef struct _PING6_PRIVATE_DATA {
  EFI_HANDLE                  ImageHandle;
  EFI_HANDLE                  NicHandle;
  EFI_HANDLE                  Ip6ChildHandle;
  EFI_IP6_PROTOCOL            *Ip6;
  EFI_EVENT                   Timer;

  UINT32                      TimerPeriod;
  UINT32                      RttTimerTick;
  EFI_EVENT                   RttTimer;

  EFI_STATUS                  Status;
  LIST_ENTRY                  TxList;
  EFI_IP6_COMPLETION_TOKEN    RxToken;
  UINT16                      RxCount;
  UINT16                      TxCount;
  UINT64                      RttSum;
  UINT64                      RttMin;
  UINT64                      RttMax;
  UINT32                      SequenceNum;

  EFI_IPv6_ADDRESS            SrcAddress;
  EFI_IPv6_ADDRESS            DstAddress;
  UINT32                      SendNum;
  UINT32                      BufferSize;
} PING6_PRIVATE_DATA;

SHELL_PARAM_ITEM  Ping6ParamList[] = {
  {
    L"-l",
    TypeValue
  },
  {
    L"-n",
    TypeValue
  },
  {
    L"-s",
    TypeValue
  },
  {
    L"-?",
    TypeFlag
  },
  {
    NULL,
    TypeMax
  },
};

//
// Global Variables in Ping6 application.
//
CONST CHAR16           *mIp6DstString;
CONST CHAR16           *mIp6SrcString;
EFI_CPU_ARCH_PROTOCOL  *Cpu = NULL;

/**
  RTT timer tick routine.

  @param[in]    Event    A EFI_EVENT type event.
  @param[in]    Context  The pointer to Context.

**/
VOID
EFIAPI
Ping6RttTimerTickRoutine (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINT32  *RttTimerTick;

  RttTimerTick = (UINT32 *)Context;
  (*RttTimerTick)++;
}

/**
  Get the timer period of the system.

  This function tries to get the system timer period by creating
  an 1ms period timer.

  @return     System timer period in MS, or 0 if operation failed.

**/
UINT32
Ping6GetTimerPeriod (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      RttTimerTick;
  EFI_EVENT   TimerEvent;
  UINT32      StallCounter;
  EFI_TPL     OldTpl;

  RttTimerTick = 0;
  StallCounter = 0;

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  Ping6RttTimerTickRoutine,
                  &RttTimerTick,
                  &TimerEvent
                  );
  if (EFI_ERROR (Status)) {
    return 0;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  Status = gBS->SetTimer (
                  TimerEvent,
                  TimerPeriodic,
                  TICKS_PER_MS
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (TimerEvent);
    return 0;
  }

  while (RttTimerTick < 10) {
    gBS->Stall (STALL_1_MILLI_SECOND);
    ++StallCounter;
  }

  gBS->RestoreTPL (OldTpl);

  gBS->SetTimer (TimerEvent, TimerCancel, 0);
  gBS->CloseEvent (TimerEvent);

  return StallCounter / RttTimerTick;
}

/**
  Initialize the timer event for RTT (round trip time).

  @param[in]    Private    The pointer to PING6_PRIVATE_DATA.

  @retval EFI_SUCCESS      RTT timer is started.
  @retval Others           Failed to start the RTT timer.

**/
EFI_STATUS
Ping6InitRttTimer (
  IN  PING6_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;

  Private->TimerPeriod = Ping6GetTimerPeriod ();
  if (Private->TimerPeriod == 0) {
    return EFI_ABORTED;
  }

  Private->RttTimerTick = 0;
  Status                = gBS->CreateEvent (
                                 EVT_TIMER | EVT_NOTIFY_SIGNAL,
                                 TPL_NOTIFY,
                                 Ping6RttTimerTickRoutine,
                                 &Private->RttTimerTick,
                                 &Private->RttTimer
                                 );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->SetTimer (
                  Private->RttTimer,
                  TimerPeriodic,
                  TICKS_PER_MS
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (Private->RttTimer);
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Free RTT timer event resource.

  @param[in]    Private    The pointer to PING6_PRIVATE_DATA.

**/
VOID
Ping6FreeRttTimer (
  IN  PING6_PRIVATE_DATA  *Private
  )
{
  if (Private->RttTimer != NULL) {
    gBS->SetTimer (Private->RttTimer, TimerCancel, 0);
    gBS->CloseEvent (Private->RttTimer);
  }
}

/**
  Read the current time.

  @param[in]    Private    The pointer to PING6_PRIVATE_DATA.

  @retval the current tick value.
**/
UINT32
Ping6ReadTime (
  IN  PING6_PRIVATE_DATA  *Private
  )
{
  return Private->RttTimerTick;
}

/**
  Get and calculate the duration in ms.

  @param[in]  Private  The pointer to PING6_PRIVATE_DATA.
  @param[in]  Begin    The start point of time.
  @param[in]  End      The end point of time.

  @return The duration in ms.

**/
UINT32
Ping6CalculateTick (
  IN PING6_PRIVATE_DATA  *Private,
  IN UINT32              Begin,
  IN UINT32              End
  )
{
  if (End < Begin) {
    return (0);
  }

  return (End - Begin) * Private->TimerPeriod;
}

/**
  Destroy IPING6_ICMP6_TX_INFO, and recollect the memory.

  @param[in]    TxInfo    The pointer to PING6_ICMP6_TX_INFO.

**/
VOID
Ping6DestroyTxInfo (
  IN PING6_ICMP6_TX_INFO  *TxInfo
  )
{
  EFI_IP6_TRANSMIT_DATA  *TxData;
  EFI_IP6_FRAGMENT_DATA  *FragData;
  UINTN                  Index;

  ASSERT (TxInfo != NULL);

  if (TxInfo->Token != NULL) {
    if (TxInfo->Token->Event != NULL) {
      gBS->CloseEvent (TxInfo->Token->Event);
    }

    TxData = TxInfo->Token->Packet.TxData;
    if (TxData != NULL) {
      if (TxData->OverrideData != NULL) {
        FreePool (TxData->OverrideData);
      }

      if (TxData->ExtHdrs != NULL) {
        FreePool (TxData->ExtHdrs);
      }

      for (Index = 0; Index < TxData->FragmentCount; Index++) {
        FragData = TxData->FragmentTable[Index].FragmentBuffer;
        if (FragData != NULL) {
          FreePool (FragData);
        }
      }
    }

    FreePool (TxInfo->Token);
  }

  FreePool (TxInfo);
}

/**
  Match the request, and reply with SequenceNum/TimeStamp.

  @param[in]    Private    The pointer to PING6_PRIVATE_DATA.
  @param[in]    Packet     The pointer to ICMP6_ECHO_REQUEST_REPLY.

  @retval EFI_SUCCESS      The match is successful.
  @retval EFI_NOT_FOUND    The reply can't be matched with any request.

**/
EFI_STATUS
Ping6OnMatchEchoReply (
  IN PING6_PRIVATE_DATA        *Private,
  IN ICMP6_ECHO_REQUEST_REPLY  *Packet
  )
{
  PING6_ICMP6_TX_INFO  *TxInfo;
  LIST_ENTRY           *Entry;
  LIST_ENTRY           *NextEntry;

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Private->TxList) {
    TxInfo = BASE_CR (Entry, PING6_ICMP6_TX_INFO, Link);

    if ((TxInfo->SequenceNum == Packet->SequenceNum) && (TxInfo->TimeStamp == Packet->TimeStamp)) {
      Private->RxCount++;
      RemoveEntryList (&TxInfo->Link);
      Ping6DestroyTxInfo (TxInfo);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  The original intention is to send a request.
  Currently, the application retransmits an icmp6 echo request packet
  per second in sendnumber times that is specified by the user.
  Because nothing can be done here, all things move to the timer routine.

  @param[in]    Event      A EFI_EVENT type event.
  @param[in]    Context    The pointer to Context.

**/
VOID
EFIAPI
Ping6OnEchoRequestSent6 (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
}

/**
  receive reply, match and print reply infomation.

  @param[in]    Event      A EFI_EVENT type event.
  @param[in]    Context    The pointer to context.

**/
VOID
EFIAPI
Ping6OnEchoReplyReceived6 (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                Status;
  PING6_PRIVATE_DATA        *Private;
  EFI_IP6_COMPLETION_TOKEN  *RxToken;
  EFI_IP6_RECEIVE_DATA      *RxData;
  ICMP6_ECHO_REQUEST_REPLY  *Reply;
  UINT32                    PayLoad;
  UINT32                    Rtt;

  Private = (PING6_PRIVATE_DATA *)Context;

  if (Private->Status == EFI_ABORTED) {
    return;
  }

  RxToken = &Private->RxToken;
  RxData  = RxToken->Packet.RxData;
  Reply   = RxData->FragmentTable[0].FragmentBuffer;
  PayLoad = RxData->DataLength;

  if (RxData->Header->NextHeader != IP6_ICMP) {
    goto ON_EXIT;
  }

  if (!IP6_IS_MULTICAST (&Private->DstAddress) &&
      !EFI_IP6_EQUAL (&RxData->Header->SourceAddress, &Private->DstAddress))
  {
    goto ON_EXIT;
  }

  if ((Reply->Type != ICMP_V6_ECHO_REPLY) || (Reply->Code != 0)) {
    goto ON_EXIT;
  }

  if (PayLoad != Private->BufferSize) {
    goto ON_EXIT;
  }

  //
  // Check whether the reply matches the sent request before.
  //
  Status = Ping6OnMatchEchoReply (Private, Reply);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Display statistics on this icmp6 echo reply packet.
  //
  Rtt = Ping6CalculateTick (Private, Reply->TimeStamp, Ping6ReadTime (Private));

  Private->RttSum += Rtt;
  Private->RttMin  = Private->RttMin > Rtt ? Rtt : Private->RttMin;
  Private->RttMax  = Private->RttMax < Rtt ? Rtt : Private->RttMax;

  ShellPrintHiiEx (
    -1,
    -1,
    NULL,
    STRING_TOKEN (STR_PING6_REPLY_INFO),
    gShellNetwork2HiiHandle,
    PayLoad,
    mIp6DstString,
    Reply->SequenceNum,
    RxData->Header->HopLimit,
    Rtt,
    Rtt + Private->TimerPeriod
    );

ON_EXIT:

  if (Private->RxCount < Private->SendNum) {
    //
    // Continue to receive icmp6 echo reply packets.
    //
    RxToken->Status = EFI_ABORTED;

    Status = Private->Ip6->Receive (Private->Ip6, RxToken);

    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_IP6_RECEIVE), gShellNetwork2HiiHandle, Status);
      Private->Status = EFI_ABORTED;
    }
  } else {
    //
    // All reply have already been received from the dest host.
    //
    Private->Status = EFI_SUCCESS;
  }

  //
  // Singal to recycle the each rxdata here, not at the end of process.
  //
  gBS->SignalEvent (RxData->RecycleSignal);
}

/**
  Initial EFI_IP6_COMPLETION_TOKEN.

  @param[in]    Private        The pointer of PING6_PRIVATE_DATA.
  @param[in]    TimeStamp      The TimeStamp of request.
  @param[in]    SequenceNum    The SequenceNum of request.

  @return The pointer of EFI_IP6_COMPLETION_TOKEN.

**/
EFI_IP6_COMPLETION_TOKEN *
Ping6GenerateToken (
  IN PING6_PRIVATE_DATA  *Private,
  IN UINT32              TimeStamp,
  IN UINT16              SequenceNum
  )
{
  EFI_STATUS                Status;
  EFI_IP6_COMPLETION_TOKEN  *Token;
  EFI_IP6_TRANSMIT_DATA     *TxData;
  ICMP6_ECHO_REQUEST_REPLY  *Request;

  Request = AllocateZeroPool (Private->BufferSize);

  if (Request == NULL) {
    return NULL;
  }

  //
  // Assembly icmp6 echo request packet.
  //
  Request->Type        = ICMP_V6_ECHO_REQUEST;
  Request->Code        = 0;
  Request->SequenceNum = SequenceNum;
  Request->TimeStamp   = TimeStamp;
  Request->Identifier  = 0;
  //
  // Leave check sum to ip6 layer, since it has no idea of source address
  // selection.
  //
  Request->Checksum = 0;

  TxData = AllocateZeroPool (sizeof (EFI_IP6_TRANSMIT_DATA));

  if (TxData == NULL) {
    FreePool (Request);
    return NULL;
  }

  //
  // Assembly ipv6 token for transmit.
  //
  TxData->OverrideData                    = 0;
  TxData->ExtHdrsLength                   = 0;
  TxData->ExtHdrs                         = NULL;
  TxData->DataLength                      = Private->BufferSize;
  TxData->FragmentCount                   = 1;
  TxData->FragmentTable[0].FragmentBuffer = (VOID *)Request;
  TxData->FragmentTable[0].FragmentLength = Private->BufferSize;

  Token = AllocateZeroPool (sizeof (EFI_IP6_COMPLETION_TOKEN));

  if (Token == NULL) {
    FreePool (Request);
    FreePool (TxData);
    return NULL;
  }

  Token->Status        = EFI_ABORTED;
  Token->Packet.TxData = TxData;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ping6OnEchoRequestSent6,
                  Private,
                  &Token->Event
                  );

  if (EFI_ERROR (Status)) {
    FreePool (Request);
    FreePool (TxData);
    FreePool (Token);
    return NULL;
  }

  return Token;
}

/**
  Transmit the EFI_IP6_COMPLETION_TOKEN.

  @param[in]    Private    The pointer of PING6_PRIVATE_DATA.

  @retval EFI_SUCCESS             Transmitted successfully.
  @retval EFI_OUT_OF_RESOURCES    No memory is available on the platform.
  @retval others                  Transmitted unsuccessfully.

**/
EFI_STATUS
Ping6SendEchoRequest (
  IN PING6_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS           Status;
  PING6_ICMP6_TX_INFO  *TxInfo;

  TxInfo = AllocateZeroPool (sizeof (PING6_ICMP6_TX_INFO));

  if (TxInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TxInfo->TimeStamp   = Ping6ReadTime (Private);
  TxInfo->SequenceNum = (UINT16)(Private->TxCount + 1);

  TxInfo->Token = Ping6GenerateToken (
                    Private,
                    TxInfo->TimeStamp,
                    TxInfo->SequenceNum
                    );

  if (TxInfo->Token == NULL) {
    Ping6DestroyTxInfo (TxInfo);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = Private->Ip6->Transmit (Private->Ip6, TxInfo->Token);

  if (EFI_ERROR (Status)) {
    Ping6DestroyTxInfo (TxInfo);
    return Status;
  }

  InsertTailList (&Private->TxList, &TxInfo->Link);
  Private->TxCount++;

  return EFI_SUCCESS;
}

/**
  Place a completion token into the receive packet queue to receive the echo reply.

  @param[in]    Private    The pointer of PING6_PRIVATE_DATA.

  @retval EFI_SUCCESS      Put the token into the receive packet queue successfully.
  @retval others           Put the token into the receive packet queue unsuccessfully.

**/
EFI_STATUS
Ping6OnReceiveEchoReply (
  IN PING6_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;

  ZeroMem (&Private->RxToken, sizeof (EFI_IP6_COMPLETION_TOKEN));

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ping6OnEchoReplyReceived6,
                  Private,
                  &Private->RxToken.Event
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private->RxToken.Status = EFI_NOT_READY;

  Status = Private->Ip6->Receive (Private->Ip6, &Private->RxToken);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_IP6_RECEIVE), gShellNetwork2HiiHandle, Status);
  }

  return Status;
}

/**
  Remove the timeout request from the list.

  @param[in]    Event    A EFI_EVENT type event.
  @param[in]    Context  The pointer to Context.

**/
VOID
EFIAPI
Ping6OnTimerRoutine6 (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS           Status;
  PING6_PRIVATE_DATA   *Private;
  PING6_ICMP6_TX_INFO  *TxInfo;
  LIST_ENTRY           *Entry;
  LIST_ENTRY           *NextEntry;
  UINT64               Time;

  Private = (PING6_PRIVATE_DATA *)Context;

  //
  // Retransmit icmp6 echo request packets per second in sendnumber times.
  //
  if (Private->TxCount < Private->SendNum) {
    Status = Ping6SendEchoRequest (Private);
    if (Private->TxCount != 0) {
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_SEND_REQUEST), gShellNetwork2HiiHandle, Private->TxCount + 1);
      }
    }
  }

  //
  // Check whether any icmp6 echo request in the list timeout.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Private->TxList) {
    TxInfo = BASE_CR (Entry, PING6_ICMP6_TX_INFO, Link);
    Time   = Ping6CalculateTick (Private, TxInfo->TimeStamp, Ping6ReadTime (Private));

    //
    // Remove the timeout echo request from txlist.
    //
    if (Time > PING6_DEFAULT_TIMEOUT) {
      if (EFI_ERROR (TxInfo->Token->Status)) {
        Private->Ip6->Cancel (Private->Ip6, TxInfo->Token);
      }

      //
      // Remove the timeout icmp6 echo request from list.
      //
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_TIMEOUT), gShellNetwork2HiiHandle, TxInfo->SequenceNum);

      RemoveEntryList (&TxInfo->Link);
      Ping6DestroyTxInfo (TxInfo);

      if (IsListEmpty (&Private->TxList) && (Private->TxCount == Private->SendNum)) {
        //
        // All the left icmp6 echo request in the list timeout.
        //
        Private->Status = EFI_TIMEOUT;
      }
    }
  }
}

/**
  Create a valid IP6 instance.

  @param[in]    Private    The pointer of PING6_PRIVATE_DATA.

  @retval EFI_SUCCESS              Create a valid IP6 instance successfully.
  @retval EFI_ABORTED              Locate handle with ip6 service binding protocol unsuccessfully.
  @retval EFI_INVALID_PARAMETER    The source address is unspecified when the destination address is a link -ocal address.
  @retval EFI_OUT_OF_RESOURCES     No memory is available on the platform.
  @retval EFI_NOT_FOUND            The source address is not found.
**/
EFI_STATUS
Ping6CreateIpInstance (
  IN  PING6_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS                     Status;
  UINTN                          HandleIndex;
  UINTN                          HandleNum;
  EFI_HANDLE                     *HandleBuffer;
  BOOLEAN                        UnspecifiedSrc;
  EFI_STATUS                     MediaStatus;
  EFI_SERVICE_BINDING_PROTOCOL   *Ip6Sb;
  EFI_IP6_CONFIG_PROTOCOL        *Ip6Cfg;
  EFI_IP6_CONFIG_DATA            Ip6Config;
  EFI_IP6_CONFIG_INTERFACE_INFO  *IfInfo;
  UINTN                          IfInfoSize;
  EFI_IPv6_ADDRESS               *Addr;
  UINTN                          AddrIndex;

  HandleBuffer   = NULL;
  UnspecifiedSrc = FALSE;
  MediaStatus    = EFI_SUCCESS;
  Ip6Sb          = NULL;
  IfInfo         = NULL;
  IfInfoSize     = 0;

  //
  // Locate all the handles with ip6 service binding protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiIp6ServiceBindingProtocolGuid,
                  NULL,
                  &HandleNum,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) || (HandleNum == 0)) {
    return EFI_ABORTED;
  }

  if (NetIp6IsUnspecifiedAddr (&Private->SrcAddress)) {
    //
    // SrcAddress is unspecified. So, both connected and configured interface will be automatic selected.
    //
    UnspecifiedSrc = TRUE;
  }

  //
  // Source address is required when pinging a link-local address.
  //
  if (NetIp6IsLinkLocalAddr (&Private->DstAddress) && UnspecifiedSrc) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_SOURCE), gShellNetwork2HiiHandle);
    Status = EFI_INVALID_PARAMETER;
    goto ON_ERROR;
  }

  //
  // For each ip6 protocol, check interface addresses list.
  //
  for (HandleIndex = 0; HandleIndex < HandleNum; HandleIndex++) {
    Ip6Sb      = NULL;
    IfInfo     = NULL;
    IfInfoSize = 0;

    if (UnspecifiedSrc) {
      //
      // Check media.
      //
      NetLibDetectMediaWaitTimeout (HandleBuffer[HandleIndex], 0, &MediaStatus);
      if (MediaStatus != EFI_SUCCESS) {
        //
        // Skip this one.
        //
        continue;
      }
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[HandleIndex],
                    &gEfiIp6ServiceBindingProtocolGuid,
                    (VOID **)&Ip6Sb
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Ip6config protocol and ip6 service binding protocol are installed
    // on the same handle.
    //
    Status = gBS->HandleProtocol (
                    HandleBuffer[HandleIndex],
                    &gEfiIp6ConfigProtocolGuid,
                    (VOID **)&Ip6Cfg
                    );

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Get the interface information size.
    //
    Status = Ip6Cfg->GetData (
                       Ip6Cfg,
                       Ip6ConfigDataTypeInterfaceInfo,
                       &IfInfoSize,
                       NULL
                       );

    if (Status != EFI_BUFFER_TOO_SMALL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_IP6CFG_GETDATA), gShellNetwork2HiiHandle, Status);
      goto ON_ERROR;
    }

    IfInfo = AllocateZeroPool (IfInfoSize);

    if (IfInfo == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }

    //
    // Get the interface info.
    //
    Status = Ip6Cfg->GetData (
                       Ip6Cfg,
                       Ip6ConfigDataTypeInterfaceInfo,
                       &IfInfoSize,
                       IfInfo
                       );

    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_IP6CFG_GETDATA), gShellNetwork2HiiHandle, Status);
      goto ON_ERROR;
    }

    //
    // Check whether the source address is one of the interface addresses.
    //
    for (AddrIndex = 0; AddrIndex < IfInfo->AddressInfoCount; AddrIndex++) {
      Addr = &(IfInfo->AddressInfo[AddrIndex].Address);

      if (UnspecifiedSrc) {
        if (!NetIp6IsUnspecifiedAddr (Addr) && !NetIp6IsLinkLocalAddr (Addr)) {
          //
          // Select the interface automatically.
          //
          CopyMem (&Private->SrcAddress, Addr, sizeof (Private->SrcAddress));
          break;
        }
      } else if (EFI_IP6_EQUAL (&Private->SrcAddress, Addr)) {
        //
        // Match a certain interface address.
        //
        break;
      }
    }

    if (AddrIndex < IfInfo->AddressInfoCount) {
      //
      // Found a nic handle with right interface address.
      //
      break;
    }

    FreePool (IfInfo);
    IfInfo = NULL;
  }

  //
  // No exact interface address matched.
  //

  if (HandleIndex == HandleNum) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_CONFIGD_NIC_NF), gShellNetwork2HiiHandle);
    Status = EFI_NOT_FOUND;
    goto ON_ERROR;
  }

  Private->NicHandle = HandleBuffer[HandleIndex];

  ASSERT (Ip6Sb != NULL);
  Status = Ip6Sb->CreateChild (Ip6Sb, &Private->Ip6ChildHandle);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Private->Ip6ChildHandle,
                  &gEfiIp6ProtocolGuid,
                  (VOID **)&Private->Ip6,
                  Private->ImageHandle,
                  Private->Ip6ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  ZeroMem (&Ip6Config, sizeof (EFI_IP6_CONFIG_DATA));

  //
  // Configure the ip6 instance for icmp6 packet exchange.
  //
  Ip6Config.DefaultProtocol   = 58;
  Ip6Config.AcceptAnyProtocol = FALSE;
  Ip6Config.AcceptIcmpErrors  = TRUE;
  Ip6Config.AcceptPromiscuous = FALSE;
  Ip6Config.TrafficClass      = 0;
  Ip6Config.HopLimit          = 128;
  Ip6Config.FlowLabel         = 0;
  Ip6Config.ReceiveTimeout    = 0;
  Ip6Config.TransmitTimeout   = 0;

  IP6_COPY_ADDRESS (&Ip6Config.StationAddress, &Private->SrcAddress);

  IP6_COPY_ADDRESS (&Ip6Config.DestinationAddress, &Private->DstAddress);

  Status = Private->Ip6->Configure (Private->Ip6, &Ip6Config);

  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_IP6_CONFIG), gShellNetwork2HiiHandle, Status);
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  if (IfInfo != NULL) {
    FreePool (IfInfo);
  }

  if ((Ip6Sb != NULL) && (Private->Ip6ChildHandle != NULL)) {
    Ip6Sb->DestroyChild (Ip6Sb, Private->Ip6ChildHandle);
  }

  return Status;
}

/**
  Destroy the IP6 instance.

  @param[in]    Private    The pointer of PING6_PRIVATE_DATA.

**/
VOID
Ping6DestroyIpInstance (
  IN PING6_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS                    Status;
  EFI_SERVICE_BINDING_PROTOCOL  *Ip6Sb;

  gBS->CloseProtocol (
         Private->Ip6ChildHandle,
         &gEfiIp6ProtocolGuid,
         Private->ImageHandle,
         Private->Ip6ChildHandle
         );

  Status = gBS->HandleProtocol (
                  Private->NicHandle,
                  &gEfiIp6ServiceBindingProtocolGuid,
                  (VOID **)&Ip6Sb
                  );

  if (!EFI_ERROR (Status)) {
    Ip6Sb->DestroyChild (Ip6Sb, Private->Ip6ChildHandle);
  }
}

/**
  The Ping6 Process.

  @param[in]   ImageHandle    The firmware allocated handle for the UEFI image.
  @param[in]   SendNumber     The send request count.
  @param[in]   BufferSize     The send buffer size.
  @param[in]   SrcAddress     The source IPv6 address.
  @param[in]   DstAddress     The destination IPv6 address.

  @retval SHELL_SUCCESS    The ping6 processed successfullly.
  @retval others           The ping6 processed unsuccessfully.

**/
SHELL_STATUS
ShellPing6 (
  IN EFI_HANDLE        ImageHandle,
  IN UINT32            SendNumber,
  IN UINT32            BufferSize,
  IN EFI_IPv6_ADDRESS  *SrcAddress,
  IN EFI_IPv6_ADDRESS  *DstAddress
  )
{
  EFI_STATUS           Status;
  EFI_INPUT_KEY        Key;
  PING6_PRIVATE_DATA   *Private;
  PING6_ICMP6_TX_INFO  *TxInfo;
  LIST_ENTRY           *Entry;
  LIST_ENTRY           *NextEntry;
  SHELL_STATUS         ShellStatus;

  ShellStatus = SHELL_SUCCESS;
  Private     = AllocateZeroPool (sizeof (PING6_PRIVATE_DATA));

  if (Private == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellNetwork2HiiHandle, L"Ping6");
    ShellStatus = SHELL_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Private->ImageHandle = ImageHandle;
  Private->SendNum     = SendNumber;
  Private->BufferSize  = BufferSize;
  Private->RttMin      = ~((UINT64)(0x0));
  Private->Status      = EFI_NOT_READY;

  InitializeListHead (&Private->TxList);

  IP6_COPY_ADDRESS (&Private->SrcAddress, SrcAddress);
  IP6_COPY_ADDRESS (&Private->DstAddress, DstAddress);

  //
  // Open and configure a ip6 instance for ping6.
  //
  Status = Ping6CreateIpInstance (Private);

  if (EFI_ERROR (Status)) {
    ShellStatus = SHELL_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // Print the command line itself.
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_START), gShellNetwork2HiiHandle, mIp6DstString, Private->BufferSize);
  //
  // Create a ipv6 token to receive the first icmp6 echo reply packet.
  //
  Status = Ping6OnReceiveEchoReply (Private);

  if (EFI_ERROR (Status)) {
    ShellStatus = SHELL_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // Create and start timer to send icmp6 echo request packet per second.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ping6OnTimerRoutine6,
                  Private,
                  &Private->Timer
                  );

  if (EFI_ERROR (Status)) {
    ShellStatus = SHELL_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // Start a timer to calculate the RTT.
  //
  Status = Ping6InitRttTimer (Private);
  if (EFI_ERROR (Status)) {
    ShellStatus = SHELL_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // Create a ipv6 token to send the first icmp6 echo request packet.
  //
  Status = Ping6SendEchoRequest (Private);
  //
  // EFI_NOT_READY for IPsec is enable and IKE is not established.
  //
  if (EFI_ERROR (Status) && (Status != EFI_NOT_READY)) {
    ShellStatus = SHELL_ACCESS_DENIED;
    if (Status == EFI_NOT_FOUND) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_NOSOURCE_INDOMAIN), gShellNetwork2HiiHandle, mIp6DstString);
    }

    goto ON_EXIT;
  }

  Status = gBS->SetTimer (
                  Private->Timer,
                  TimerPeriodic,
                  PING6_ONE_SECOND
                  );

  if (EFI_ERROR (Status)) {
    ShellStatus = SHELL_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // Control the ping6 process by two factors:
  // 1. Hot key
  // 2. Private->Status
  //   2.1. success means all icmp6 echo request packets get reply packets.
  //   2.2. timeout means the last icmp6 echo reply request timeout to get reply.
  //   2.3. noready means ping6 process is on-the-go.
  //
  while (Private->Status == EFI_NOT_READY) {
    Private->Ip6->Poll (Private->Ip6);

    //
    // Terminate the ping6 process by 'esc' or 'ctl-c'.
    //
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

    if (!EFI_ERROR (Status)) {
      if ((Key.UnicodeChar == 0x1b) || (Key.UnicodeChar == 0x03) ||
          ((Key.UnicodeChar == 0) && (Key.ScanCode == SCAN_ESC)))
      {
        goto ON_STAT;
      }
    }
  }

ON_STAT:
  //
  // Display the statistics in all.
  //
  gBS->SetTimer (Private->Timer, TimerCancel, 0);

  if (Private->TxCount != 0) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_PING6_STAT),
      gShellNetwork2HiiHandle,
      Private->TxCount,
      Private->RxCount,
      (100 * (Private->TxCount - Private->RxCount)) / Private->TxCount,
      Private->RttSum
      );
  }

  if (Private->RxCount != 0) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_PING6_RTT),
      gShellNetwork2HiiHandle,
      Private->RttMin,
      Private->RttMin + Private->TimerPeriod,
      Private->RttMax,
      Private->RttMax + Private->TimerPeriod,
      DivU64x64Remainder (Private->RttSum, Private->RxCount, NULL),
      DivU64x64Remainder (Private->RttSum, Private->RxCount, NULL) + Private->TimerPeriod
      );
  }

ON_EXIT:

  if (Private != NULL) {
    Private->Status = EFI_ABORTED;

    NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Private->TxList) {
      TxInfo = BASE_CR (Entry, PING6_ICMP6_TX_INFO, Link);

      Status = Private->Ip6->Cancel (Private->Ip6, TxInfo->Token);

      RemoveEntryList (&TxInfo->Link);
      Ping6DestroyTxInfo (TxInfo);
    }

    Ping6FreeRttTimer (Private);

    if (Private->Timer != NULL) {
      gBS->CloseEvent (Private->Timer);
    }

    if (Private->Ip6 != NULL) {
      Status = Private->Ip6->Cancel (Private->Ip6, &Private->RxToken);
    }

    if (Private->RxToken.Event != NULL) {
      gBS->CloseEvent (Private->RxToken.Event);
    }

    if (Private->Ip6ChildHandle != NULL) {
      Ping6DestroyIpInstance (Private);
    }

    FreePool (Private);
  }

  return ShellStatus;
}

/**
  Function for 'ping6' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).

  @retval SHELL_SUCCESS  The ping6 processed successfullly.
  @retval others         The ping6 processed unsuccessfully.

**/
SHELL_STATUS
EFIAPI
ShellCommandRunPing6 (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS        Status;
  SHELL_STATUS      ShellStatus;
  EFI_IPv6_ADDRESS  DstAddress;
  EFI_IPv6_ADDRESS  SrcAddress;
  UINT64            BufferSize;
  UINTN             SendNumber;
  LIST_ENTRY        *ParamPackage;
  CONST CHAR16      *ValueStr;
  CONST CHAR16      *ValueStrPtr;
  UINTN             NonOptionCount;
  CHAR16            *ProblemParam;

  ProblemParam = NULL;
  ShellStatus  = SHELL_SUCCESS;

  Status = ShellCommandLineParseEx (Ping6ParamList, &ParamPackage, &ProblemParam, TRUE, FALSE);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_INPUT), gShellNetwork2HiiHandle);
    ShellStatus = SHELL_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  SendNumber = 10;
  BufferSize = 16;

  //
  // Parse the parameter of count number.
  //
  ValueStr    = ShellCommandLineGetValue (ParamPackage, L"-n");
  ValueStrPtr = ValueStr;
  if (ValueStr != NULL) {
    SendNumber = ShellStrToUintn (ValueStrPtr);

    //
    // ShellStrToUintn will return 0 when input is 0 or an invalid input string.
    //
    if ((SendNumber == 0) || (SendNumber > PING6_MAX_SEND_NUMBER)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_SEND_NUMBER), gShellNetwork2HiiHandle, ValueStr);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  }

  //
  // Parse the parameter of buffer size.
  //
  ValueStr    = ShellCommandLineGetValue (ParamPackage, L"-l");
  ValueStrPtr = ValueStr;
  if (ValueStr != NULL) {
    BufferSize = ShellStrToUintn (ValueStrPtr);

    //
    // ShellStrToUintn will return 0 when input is 0 or an invalid input string.
    //
    if ((BufferSize < 16) || (BufferSize > PING6_MAX_BUFFER_SIZE)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_BUFFER_SIZE), gShellNetwork2HiiHandle, ValueStr);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  }

  ZeroMem (&SrcAddress, sizeof (EFI_IPv6_ADDRESS));
  ZeroMem (&DstAddress, sizeof (EFI_IPv6_ADDRESS));

  //
  // Parse the parameter of source ip address.
  //
  ValueStr    = ShellCommandLineGetValue (ParamPackage, L"-s");
  ValueStrPtr = ValueStr;
  if (ValueStr != NULL) {
    mIp6SrcString = ValueStr;
    Status        = NetLibStrToIp6 (ValueStrPtr, &SrcAddress);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_IP), gShellNetwork2HiiHandle, ValueStr);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  }

  //
  // Parse the parameter of destination ip address.
  //
  NonOptionCount = ShellCommandLineGetCount (ParamPackage);
  ValueStr       = ShellCommandLineGetRawValue (ParamPackage, (UINT32)(NonOptionCount-1));
  if (NonOptionCount != 2) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_INPUT), gShellNetwork2HiiHandle);
    ShellStatus = SHELL_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  ValueStrPtr = ValueStr;
  if (ValueStr != NULL) {
    mIp6DstString = ValueStr;
    Status        = NetLibStrToIp6 (ValueStrPtr, &DstAddress);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_IP), gShellNetwork2HiiHandle, ValueStr);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  }

  //
  // Enter into ping6 process.
  //
  ShellStatus = ShellPing6 (
                  ImageHandle,
                  (UINT32)SendNumber,
                  (UINT32)BufferSize,
                  &SrcAddress,
                  &DstAddress
                  );

ON_EXIT:
  ShellCommandLineFreeVarList (ParamPackage);
  return ShellStatus;
}

/** @file
  The implementation for Ping6 application.

  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/ShellLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/NetLib.h>

#include <Protocol/Cpu.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/Ip6.h>
#include <Protocol/Ip6Config.h>

#include "Ping6.h"

SHELL_PARAM_ITEM    Ping6ParamList[] = {
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
EFI_HII_HANDLE    mHiiHandle;
CONST CHAR16      *mIp6DstString;
CONST CHAR16      *mIp6SrcString;
UINT64            mFrequency = 0;
/**
  Get and calculate the frequency in tick/ms.
  The result is saved in the globle variable mFrequency

  @retval EFI_SUCCESS    Calculated the frequency successfully.
  @retval Others         Failed to calculate the frequency.

**/
EFI_STATUS
Ping6GetFrequency (
  VOID
  )
{
  EFI_STATUS               Status;
  EFI_CPU_ARCH_PROTOCOL    *Cpu;
  UINT64                   CurrentTick;
  UINT64                   TimerPeriod;

  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **) &Cpu);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Cpu->GetTimerValue (Cpu, 0, &CurrentTick, &TimerPeriod);

  if (EFI_ERROR (Status)) {
    //
    // For NT32 Simulator only. 358049 is a similar value to keep timer granularity.
    // Set the timer period by ourselves.
    //
    TimerPeriod = (UINT64) NTTIMERPERIOD;
  }
  //
  // The timer period is in femtosecond (1 femtosecond is 1e-15 second).
  // So 1e+12 is divided by timer period to produce the freq in tick/ms.
  //
  mFrequency = DivU64x64Remainder (1000000000000ULL, TimerPeriod, NULL);

  return EFI_SUCCESS;
}

/**
  Get and calculate the duration in ms.

  @param[in]  Begin    The start point of time.
  @param[in]  End      The end point of time.

  @return The duration in ms.

**/
UINT64
Ping6CalculateTick (
  IN UINT64    Begin,
  IN UINT64    End
  )
{
  ASSERT (End > Begin);
  return DivU64x64Remainder (End - Begin, mFrequency, NULL);
}

/**
  Destroy IPING6_ICMP6_TX_INFO, and recollect the memory.

  @param[in]    TxInfo    The pointer to PING6_ICMP6_TX_INFO.

**/
VOID
Ping6DestroyTxInfo (
  IN PING6_ICMP6_TX_INFO    *TxInfo
  )
{
  EFI_IP6_TRANSMIT_DATA    *TxData;
  EFI_IP6_FRAGMENT_DATA    *FragData;
  UINTN                    Index;

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
Ping6MatchEchoReply (
  IN PING6_PRIVATE_DATA          *Private,
  IN ICMP6_ECHO_REQUEST_REPLY    *Packet
  )
{
  PING6_ICMP6_TX_INFO    *TxInfo;
  LIST_ENTRY             *Entry;
  LIST_ENTRY             *NextEntry;

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
  Because nothing can be done here, all things move to the timer rountine.

  @param[in]    Event      A EFI_EVENT type event.
  @param[in]    Context    The pointer to Context.

**/
VOID
EFIAPI
Ping6OnEchoRequestSent (
  IN EFI_EVENT    Event,
  IN VOID         *Context
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
Ping6OnEchoReplyReceived (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_STATUS                  Status;
  PING6_PRIVATE_DATA          *Private;
  EFI_IP6_COMPLETION_TOKEN    *RxToken;
  EFI_IP6_RECEIVE_DATA        *RxData;
  ICMP6_ECHO_REQUEST_REPLY    *Reply;
  UINT32                      PayLoad;
  UINT64                      Rtt;
  CHAR8                       Near;

  Private = (PING6_PRIVATE_DATA *) Context;

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
      !EFI_IP6_EQUAL (&RxData->Header->SourceAddress, &Private->DstAddress)) {
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
  Status = Ping6MatchEchoReply (Private, Reply);
  if (EFI_ERROR(Status)) {
    goto ON_EXIT;
  }
  //
  // Display statistics on this icmp6 echo reply packet.
  //
  Rtt  = Ping6CalculateTick (Reply->TimeStamp, ReadTime ());
  if (Rtt != 0) {
    Near = (CHAR8) '=';
  } else {
    Near = (CHAR8) '<';
  }

  Private->RttSum += Rtt;
  Private->RttMin  = Private->RttMin > Rtt ? Rtt : Private->RttMin;
  Private->RttMax  = Private->RttMax < Rtt ? Rtt : Private->RttMax;

  ShellPrintHiiEx (
    -1,
    -1,
    NULL,
    STRING_TOKEN (STR_PING6_REPLY_INFO),
    mHiiHandle,
    PayLoad,
    mIp6DstString,
    Reply->SequenceNum,
    RxData->Header->HopLimit,
    Near,
    Rtt
    );

ON_EXIT:

  if (Private->RxCount < Private->SendNum) {
    //
    // Continue to receive icmp6 echo reply packets.
    //
    RxToken->Status = EFI_ABORTED;

    Status = Private->Ip6->Receive (Private->Ip6, RxToken);

    if (EFI_ERROR (Status)) {
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
  IN PING6_PRIVATE_DATA    *Private,
  IN UINT64                TimeStamp,
  IN UINT16                SequenceNum
  )
{
  EFI_STATUS                  Status;
  EFI_IP6_COMPLETION_TOKEN    *Token;
  EFI_IP6_TRANSMIT_DATA       *TxData;
  ICMP6_ECHO_REQUEST_REPLY    *Request;

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
  Request->Checksum    = 0;

  TxData = AllocateZeroPool (sizeof (EFI_IP6_TRANSMIT_DATA));

  if (TxData == NULL) {
    FreePool (Request);
    return NULL;
  }
  //
  // Assembly ipv6 token for transmit.
  //
  TxData->OverrideData       = 0;
  TxData->ExtHdrsLength      = 0;
  TxData->ExtHdrs            = NULL;
  TxData->DataLength         = Private->BufferSize;
  TxData->FragmentCount      = 1;
  TxData->FragmentTable[0].FragmentBuffer = (VOID *) Request;
  TxData->FragmentTable[0].FragmentLength = Private->BufferSize;

  Token = AllocateZeroPool (sizeof (EFI_IP6_COMPLETION_TOKEN));

  if (Token == NULL) {
    FreePool (Request);
    FreePool (TxData);
    return NULL;
  }

  Token->Status         = EFI_ABORTED;
  Token->Packet.TxData  = TxData;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ping6OnEchoRequestSent,
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
  IN PING6_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS             Status;
  PING6_ICMP6_TX_INFO    *TxInfo;

  TxInfo = AllocateZeroPool (sizeof (PING6_ICMP6_TX_INFO));

  if (TxInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TxInfo->TimeStamp   = ReadTime ();
  TxInfo->SequenceNum = (UINT16) (Private->TxCount + 1);

  TxInfo->Token       = Ping6GenerateToken (
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
Ping6ReceiveEchoReply (
  IN PING6_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS    Status;

  ZeroMem (&Private->RxToken, sizeof (EFI_IP6_COMPLETION_TOKEN));

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ping6OnEchoReplyReceived,
                  Private,
                  &Private->RxToken.Event
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private->RxToken.Status = EFI_NOT_READY;

  return Private->Ip6->Receive (Private->Ip6, &Private->RxToken);
}

/**
  Remove the timeout request from the list.

  @param[in]    Event    A EFI_EVENT type event.
  @param[in]    Context  The pointer to Context.

**/
VOID
EFIAPI
Ping6OnTimerRoutine (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_STATUS             Status;
  PING6_PRIVATE_DATA     *Private;
  PING6_ICMP6_TX_INFO    *TxInfo;
  LIST_ENTRY             *Entry;
  LIST_ENTRY             *NextEntry;
  UINT64                 Time;

  Private = (PING6_PRIVATE_DATA *) Context;

  //
  // Retransmit icmp6 echo request packets per second in sendnumber times.
  //
  if (Private->TxCount < Private->SendNum) {

    Status = Ping6SendEchoRequest (Private);
    if (Private->TxCount != 0){
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_SEND_REQUEST), mHiiHandle, Private->TxCount + 1);
      }
    }
  }
  //
  // Check whether any icmp6 echo request in the list timeout.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Private->TxList) {
    TxInfo = BASE_CR (Entry, PING6_ICMP6_TX_INFO, Link);
    Time   = Ping6CalculateTick (TxInfo->TimeStamp, ReadTime ());

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
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_TIMEOUT), mHiiHandle, TxInfo->SequenceNum);

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
Ping6CreateIp6Instance (
  IN  PING6_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS                       Status;
  UINTN                            HandleIndex;
  UINTN                            HandleNum;
  EFI_HANDLE                       *HandleBuffer;
  EFI_SERVICE_BINDING_PROTOCOL     *Ip6Sb;
  EFI_IP6_CONFIG_PROTOCOL          *Ip6Cfg;
  EFI_IP6_CONFIG_DATA              Ip6Config;
  EFI_IP6_CONFIG_INTERFACE_INFO    *IfInfo;
  UINTN                            IfInfoSize;
  EFI_IPv6_ADDRESS                 *Addr;
  UINTN                            AddrIndex;

  HandleBuffer = NULL;
  Ip6Sb        = NULL;
  IfInfo       = NULL;
  IfInfoSize   = 0;

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
  //
  // Source address is required when pinging a link-local address on multi-
  // interfaces host.
  //
  if (NetIp6IsLinkLocalAddr (&Private->DstAddress) &&
      NetIp6IsUnspecifiedAddr (&Private->SrcAddress) &&
      (HandleNum > 1)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_SOURCE), mHiiHandle);
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

    Status = gBS->HandleProtocol (
                    HandleBuffer[HandleIndex],
                    &gEfiIp6ServiceBindingProtocolGuid,
                    (VOID **) &Ip6Sb
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    if (NetIp6IsUnspecifiedAddr (&Private->SrcAddress)) {
      //
      // No need to match interface address.
      //
      break;
    } else {
      //
      // Ip6config protocol and ip6 service binding protocol are installed
      // on the same handle.
      //
      Status = gBS->HandleProtocol (
                      HandleBuffer[HandleIndex],
                      &gEfiIp6ConfigProtocolGuid,
                      (VOID **) &Ip6Cfg
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
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_IP6CFG_GETDATA), mHiiHandle, Status);
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
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_IP6CFG_GETDATA), mHiiHandle, Status);
        goto ON_ERROR;
      }
      //
      // Check whether the source address is one of the interface addresses.
      //
      for (AddrIndex = 0; AddrIndex < IfInfo->AddressInfoCount; AddrIndex++) {

        Addr = &(IfInfo->AddressInfo[AddrIndex].Address);
        if (EFI_IP6_EQUAL (&Private->SrcAddress, Addr)) {
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
    }

    FreePool (IfInfo);
    IfInfo = NULL;
  }
  //
  // No exact interface address matched.
  //

  if (HandleIndex == HandleNum) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_SOURCE_NOT_FOUND), mHiiHandle, mIp6SrcString);
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
                  (VOID **) &Private->Ip6,
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
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_IP6_CONFIG), mHiiHandle, Status);
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
Ping6DestroyIp6Instance (
  IN PING6_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS                      Status;
  EFI_SERVICE_BINDING_PROTOCOL    *Ip6Sb;

  gBS->CloseProtocol (
         Private->Ip6ChildHandle,
         &gEfiIp6ProtocolGuid,
         Private->ImageHandle,
         Private->Ip6ChildHandle
         );

  Status = gBS->HandleProtocol (
                  Private->NicHandle,
                  &gEfiIp6ServiceBindingProtocolGuid,
                  (VOID **) &Ip6Sb
                  );

  if (!EFI_ERROR(Status)) {
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

  @retval EFI_SUCCESS    The ping6 processed successfullly.
  @retval others         The ping6 processed unsuccessfully.

**/
EFI_STATUS
Ping6 (
  IN EFI_HANDLE          ImageHandle,
  IN UINT32              SendNumber,
  IN UINT32              BufferSize,
  IN EFI_IPv6_ADDRESS    *SrcAddress,
  IN EFI_IPv6_ADDRESS    *DstAddress
  )
{
  EFI_STATUS             Status;
  EFI_INPUT_KEY          Key;
  PING6_PRIVATE_DATA     *Private;
  PING6_ICMP6_TX_INFO    *TxInfo;
  LIST_ENTRY             *Entry;
  LIST_ENTRY             *NextEntry;

  Private = AllocateZeroPool (sizeof (PING6_PRIVATE_DATA));

  ASSERT (Private != NULL);

  Private->ImageHandle = ImageHandle;
  Private->SendNum     = SendNumber;
  Private->BufferSize  = BufferSize;
  Private->RttMin      = ~((UINT64 )(0x0));
  Private->Status      = EFI_NOT_READY;

  InitializeListHead (&Private->TxList);

  IP6_COPY_ADDRESS (&Private->SrcAddress, SrcAddress);
  IP6_COPY_ADDRESS (&Private->DstAddress, DstAddress);

  //
  // Open and configure a ip6 instance for ping6.
  //
  Status = Ping6CreateIp6Instance (Private);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }
  //
  // Print the command line itself.
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_START), mHiiHandle, mIp6DstString, Private->BufferSize);
  //
  // Create a ipv6 token to receive the first icmp6 echo reply packet.
  //
  Status = Ping6ReceiveEchoReply (Private);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }
  //
  // Create and start timer to send icmp6 echo request packet per second.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ping6OnTimerRoutine,
                  Private,
                  &Private->Timer
                  );

  if (EFI_ERROR (Status)) {
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
    if(Status == EFI_NOT_FOUND) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_NOSOURCE_INDOMAIN), mHiiHandle, mIp6DstString);
    }

    goto ON_EXIT;
  }

  Status = gBS->SetTimer (
                  Private->Timer,
                  TimerPeriodic,
                  PING6_ONE_SECOND
                  );

  if (EFI_ERROR (Status)) {
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

    if (!EFI_ERROR(Status)) {
      if ((Key.UnicodeChar == 0x1b) || (Key.UnicodeChar == 0x03) ||
         ((Key.UnicodeChar == 0) && (Key.ScanCode == SCAN_ESC))) {
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
      mHiiHandle,
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
      mHiiHandle,
      Private->RttMin,
      Private->RttMax,
      DivU64x64Remainder (Private->RttSum, Private->RxCount, NULL)
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
      Ping6DestroyIp6Instance (Private);
    }

    FreePool (Private);
  }

  return Status;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers, including
  both device drivers and bus drivers.

  The entry point for the Ping6 application that parses the command line input and calls the Ping6 process.

  @param[in] ImageHandle    The firmware allocated handle for the UEFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS               The operation completed successfully.
  @retval EFI_INVALID_PARAMETETR    Input parameters combination is invalid.
  @retval Others                    Some errors occur.

**/
EFI_STATUS
EFIAPI
InitializePing6 (
  IN  EFI_HANDLE          ImageHandle,
  IN  EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS          Status;
  EFI_IPv6_ADDRESS    DstAddress;
  EFI_IPv6_ADDRESS    SrcAddress;
  UINT64              BufferSize;
  UINTN               SendNumber;
  LIST_ENTRY          *ParamPackage;
  CONST CHAR16        *ValueStr;
  CONST CHAR16        *ValueStrPtr;
  UINTN               NonOptionCount;

  //
  // Register our string package with HII and return the handle to it.
  //
  mHiiHandle = HiiAddPackages (&gEfiCallerIdGuid, ImageHandle, Ping6Strings, NULL);
  ASSERT (mHiiHandle != NULL);

  Status = ShellCommandLineParseEx (Ping6ParamList, &ParamPackage, NULL, TRUE, FALSE);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_INPUT), mHiiHandle);
    goto ON_EXIT;
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-?")) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_HELP), mHiiHandle);
    goto ON_EXIT;
  }

  SendNumber = 10;
  BufferSize = 16;

  //
  // Parse the paramter of count number.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"-n");
  ValueStrPtr = ValueStr;
  if (ValueStr != NULL) {
    SendNumber = ShellStrToUintn (ValueStrPtr);

    //
    // ShellStrToUintn will return 0 when input is 0 or an invalid input string.
    //
    if ((SendNumber == 0) || (SendNumber > PING6_MAX_SEND_NUMBER)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_SEND_NUMBER), mHiiHandle, ValueStr);
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  }
  //
  // Parse the paramter of buffer size.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"-l");
  ValueStrPtr = ValueStr;
  if (ValueStr != NULL) {
    BufferSize = ShellStrToUintn (ValueStrPtr);

    //
    // ShellStrToUintn will return 0 when input is 0 or an invalid input string.
    //
    if ((BufferSize < 16) || (BufferSize > PING6_MAX_BUFFER_SIZE)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_BUFFER_SIZE), mHiiHandle, ValueStr);
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  }

  ZeroMem (&SrcAddress, sizeof (EFI_IPv6_ADDRESS));
  ZeroMem (&DstAddress, sizeof (EFI_IPv6_ADDRESS));

  //
  // Parse the paramter of source ip address.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"-s");
  ValueStrPtr = ValueStr;
  if (ValueStr != NULL) {
    mIp6SrcString = ValueStr;
    Status = NetLibStrToIp6 (ValueStrPtr, &SrcAddress);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_IP), mHiiHandle, ValueStr);
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  }
  //
  // Parse the paramter of destination ip address.
  //
  NonOptionCount = ShellCommandLineGetCount(ParamPackage);
  ValueStr = ShellCommandLineGetRawValue (ParamPackage, (UINT32)(NonOptionCount-1));
  if (NonOptionCount != 2) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_INPUT), mHiiHandle);
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }
  ValueStrPtr = ValueStr;
  if (ValueStr != NULL) {
    mIp6DstString = ValueStr;
    Status = NetLibStrToIp6 (ValueStrPtr, &DstAddress);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING6_INVALID_IP), mHiiHandle, ValueStr);
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  }
  //
  // Get frequency to calculate the time from ticks.
  //
  Status = Ping6GetFrequency ();

  if (EFI_ERROR(Status)) {
    goto ON_EXIT;
  }
  //
  // Enter into ping6 process.
  //
  Status = Ping6 (
             ImageHandle,
             (UINT32)SendNumber,
             (UINT32)BufferSize,
             &SrcAddress,
             &DstAddress
             );

ON_EXIT:
  ShellCommandLineFreeVarList (ParamPackage);
  HiiRemovePackages (mHiiHandle);
  return Status;
}

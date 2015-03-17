/** @file
  The implementation for Ping shell command.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellNetwork1CommandsLib.h"

#define PING_IP4_COPY_ADDRESS(Dest, Src) (CopyMem ((Dest), (Src), sizeof (EFI_IPv4_ADDRESS)))

UINT64          CurrentTick = 0;

//
// Function templates to match the IPv4 and IPv6 commands that we use.
//
typedef 
EFI_STATUS
(EFIAPI *PING_IPX_POLL)(
  IN VOID          *This
  );  

typedef 
EFI_STATUS
(EFIAPI *PING_IPX_TRANSMIT)(
  IN VOID          *This,
  IN VOID          *Token
  );

typedef 
EFI_STATUS
(EFIAPI *PING_IPX_RECEIVE)(
  IN VOID          *This,
  IN VOID          *Token
  ); 

typedef
EFI_STATUS
(EFIAPI *PING_IPX_CANCEL)(
  IN VOID          *This,
  IN VOID          *Token OPTIONAL
  );

///
/// A set of pointers to either IPv6 or IPv4 functions.  
/// Unknown which one to the ping command.
///
typedef struct {
  PING_IPX_TRANSMIT             Transmit;
  PING_IPX_RECEIVE              Receive;
  PING_IPX_CANCEL               Cancel;
  PING_IPX_POLL                 Poll;
}PING_IPX_PROTOCOL;


typedef union {
  VOID                  *RxData;
  VOID                  *TxData;
} PING_PACKET;

//
// PING_IPX_COMPLETION_TOKEN
// structures are used for both transmit and receive operations. 
// This version is IP-unaware.
//
typedef struct {
  EFI_EVENT               Event;
  EFI_STATUS              Status;
  PING_PACKET             Packet;
} PING_IPX_COMPLETION_TOKEN;

#pragma pack(1)
typedef struct _ICMPX_ECHO_REQUEST_REPLY {
  UINT8                       Type;
  UINT8                       Code;
  UINT16                      Checksum;
  UINT16                      Identifier;
  UINT16                      SequenceNum;
  UINT64                      TimeStamp;
  UINT8                       Data[1];
} ICMPX_ECHO_REQUEST_REPLY;
#pragma pack()

typedef struct _PING_ICMP_TX_INFO {
  LIST_ENTRY                Link;
  UINT16                    SequenceNum;
  UINT64                    TimeStamp;
  PING_IPX_COMPLETION_TOKEN *Token;
} PING_ICMPX_TX_INFO;

#define DEFAULT_TIMEOUT       5000
#define MAX_SEND_NUMBER       10000
#define MAX_BUFFER_SIZE       32768
#define DEFAULT_TIMER_PERIOD  358049
#define ONE_SECOND            10000000
#define PING_IP_CHOICE_IP4    1
#define PING_IP_CHOICE_IP6    2
#define DEFAULT_SEND_COUNT    10
#define DEFAULT_BUFFER_SIZE   16
#define ICMP_V4_ECHO_REQUEST  0x8
#define ICMP_V4_ECHO_REPLY    0x0

#define PING_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('P', 'i', 'n', 'g')
typedef struct _PING_PRIVATE_DATA {
  UINT32                      Signature;
  EFI_HANDLE                  NicHandle;
  EFI_HANDLE                  IpChildHandle;
  EFI_EVENT                   Timer;

  EFI_STATUS                  Status;
  LIST_ENTRY                  TxList;
  UINT16                      RxCount;
  UINT16                      TxCount;
  UINT64                      RttSum;
  UINT64                      RttMin;
  UINT64                      RttMax;
  UINT32                      SequenceNum;

  UINT32                      SendNum;
  UINT32                      BufferSize;
  UINT32                      IpChoice;

  PING_IPX_PROTOCOL           ProtocolPointers;
  VOID                        *IpProtocol;
  UINT8                       SrcAddress[MAX(sizeof(EFI_IPv6_ADDRESS)        , sizeof(EFI_IPv4_ADDRESS)          )];
  UINT8                       DstAddress[MAX(sizeof(EFI_IPv6_ADDRESS)        , sizeof(EFI_IPv4_ADDRESS)          )];
  PING_IPX_COMPLETION_TOKEN   RxToken;
} PING_PRIVATE_DATA;

/**
  Calculate the internet checksum (see RFC 1071).

  @param[in] Packet  Buffer which contains the data to be checksummed.
  @param[in] Length  Length to be checksummed.

  @retval Checksum     Returns the 16 bit ones complement of 
                       ones complement sum of 16 bit words
**/
UINT16
EFIAPI
NetChecksum (
  IN UINT8   *Buffer,
  IN UINT32  Length
  )
{
  UINT32  Sum;
  UINT8   Odd;
  UINT16  *Packet;

  Packet  = (UINT16 *) Buffer;

  Sum     = 0;
  Odd     = (UINT8) (Length & 1);
  Length >>= 1;
  while ((Length--) != 0) {
    Sum += *Packet++;
  }

  if (Odd != 0) {
    Sum += *(UINT8 *) Packet;
  }

  Sum = (Sum & 0xffff) + (Sum >> 16);

  //
  // in case above carried
  //
  Sum += Sum >> 16;

  return (UINT16) Sum;
}

/**
  Reads and returns the current value of register.
  In IA64, the register is the Interval Timer Vector (ITV).
  In X86(IA32/X64), the register is the Time Stamp Counter (TSC)

  @return The current value of the register.

**/

STATIC CONST SHELL_PARAM_ITEM    PingParamList[] = {
  {
    L"-l",
    TypeValue
  },
  {
    L"-n",
    TypeValue
  },
  {
    L"-_s",
    TypeValue
  },
  {
    L"-_ip6",
    TypeFlag
  },
  {
    NULL,
    TypeMax
  },
};

//
// Global Variables in Ping command.
//
STATIC CONST CHAR16      *mDstString;
STATIC CONST CHAR16      *mSrcString;
STATIC UINT64            mFrequency = 0;
EFI_CPU_ARCH_PROTOCOL    *gCpu = NULL;

/**
  Read the current time.

  @retval the current tick value.
**/
UINT64
EFIAPI
ReadTime (
  VOID
  )
{
  UINT64                 TimerPeriod;
  EFI_STATUS             Status;

  ASSERT (gCpu != NULL);

  Status = gCpu->GetTimerValue (gCpu, 0, &CurrentTick, &TimerPeriod);
  if (EFI_ERROR (Status)) {
    //
    // The WinntGetTimerValue will return EFI_UNSUPPORTED. Set the
    // TimerPeriod by ourselves.
    //
    CurrentTick += 1000000;
  }
  
  return CurrentTick;
}


/**
  Get and calculate the frequency in ticks/ms.
  The result is saved in the global variable mFrequency

  @retval EFI_SUCCESS    Calculated the frequency successfully.
  @retval Others         Failed to calculate the frequency.

**/
EFI_STATUS
EFIAPI
GetFrequency (
  VOID
  )
{
  EFI_STATUS               Status;
  UINT64                   CurrentTick;
  UINT64                   TimerPeriod;

  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **) &gCpu);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gCpu->GetTimerValue (gCpu, 0, &CurrentTick, &TimerPeriod);

  if (EFI_ERROR (Status)) {
    TimerPeriod = DEFAULT_TIMER_PERIOD;
  }

  //
  // The timer period is in femtosecond (1 femtosecond is 1e-15 second).
  // So 1e+12 is divided by timer period to produce the freq in ticks/ms.
  //
  mFrequency = DivU64x64Remainder (1000000000000ULL, TimerPeriod, NULL);

  return EFI_SUCCESS;
}

/**
  Calculate a duration in ms.

  @param[in]  Begin     The start point of time.
  @param[in]  End       The end point of time.

  @return               The duration in ms.
  @retval 0             The parameters were not valid.
**/
UINT64
EFIAPI
CalculateTick (
  IN UINT64    Begin,
  IN UINT64    End
  )
{
  if (End <= Begin) {
    return (0);
  }
  return DivU64x64Remainder (End - Begin, mFrequency, NULL);
}

/**
  Destroy PING_ICMPX_TX_INFO, and recollect the memory.

  @param[in]    TxInfo    The pointer to PING_ICMPX_TX_INFO.
  @param[in]    IpChoice  Whether the token is IPv4 or IPv6
**/
VOID
EFIAPI
PingDestroyTxInfo (
  IN PING_ICMPX_TX_INFO    *TxInfo,
  IN UINT32                IpChoice
  )
{
  EFI_IP6_TRANSMIT_DATA    *Ip6TxData;
  EFI_IP4_TRANSMIT_DATA    *Ip4TxData;
  EFI_IP6_FRAGMENT_DATA    *FragData;
  UINTN                    Index;

  if (TxInfo == NULL) {
    return;
  }

  if (TxInfo->Token != NULL) {

    if (TxInfo->Token->Event != NULL) {
      gBS->CloseEvent (TxInfo->Token->Event);
    }

    if (TxInfo->Token->Packet.TxData != NULL) {
      if (IpChoice == PING_IP_CHOICE_IP6) {
        Ip6TxData = TxInfo->Token->Packet.TxData;

        if (Ip6TxData->OverrideData != NULL) {
          FreePool (Ip6TxData->OverrideData);
        }

        if (Ip6TxData->ExtHdrs != NULL) {
          FreePool (Ip6TxData->ExtHdrs);
        }

        for (Index = 0; Index < Ip6TxData->FragmentCount; Index++) {
          FragData = Ip6TxData->FragmentTable[Index].FragmentBuffer;
          if (FragData != NULL) {
            FreePool (FragData);
          }
        }
      } else {
        Ip4TxData = TxInfo->Token->Packet.TxData;

        if (Ip4TxData->OverrideData != NULL) {
          FreePool (Ip4TxData->OverrideData);
        }

        for (Index = 0; Index < Ip4TxData->FragmentCount; Index++) {
          FragData = Ip4TxData->FragmentTable[Index].FragmentBuffer;
          if (FragData != NULL) {
            FreePool (FragData);
          }
        }
      }
    }

    FreePool (TxInfo->Token);
  }

  FreePool (TxInfo);
}

/**
  Match the request, and reply with SequenceNum/TimeStamp.

  @param[in]    Private    The pointer to PING_PRIVATE_DATA.
  @param[in]    Packet     The pointer to ICMPX_ECHO_REQUEST_REPLY.

  @retval EFI_SUCCESS      The match is successful.
  @retval EFI_NOT_FOUND    The reply can't be matched with any request.

**/
EFI_STATUS
EFIAPI
Ping6MatchEchoReply (
  IN PING_PRIVATE_DATA           *Private,
  IN ICMPX_ECHO_REQUEST_REPLY    *Packet
  )
{
  PING_ICMPX_TX_INFO     *TxInfo;
  LIST_ENTRY             *Entry;
  LIST_ENTRY             *NextEntry;

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Private->TxList) {
    TxInfo = BASE_CR (Entry, PING_ICMPX_TX_INFO, Link);

    if ((TxInfo->SequenceNum == Packet->SequenceNum) && (TxInfo->TimeStamp == Packet->TimeStamp)) {
      Private->RxCount++;
      RemoveEntryList (&TxInfo->Link);
      PingDestroyTxInfo (TxInfo, Private->IpChoice);
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
  PING_PRIVATE_DATA           *Private;
  ICMPX_ECHO_REQUEST_REPLY    *Reply;
  UINT32                      PayLoad;
  UINT64                      Rtt;
  CHAR8                       Near;

  Private = (PING_PRIVATE_DATA *) Context;

  if (Private == NULL || Private->Status == EFI_ABORTED || Private->Signature != PING_PRIVATE_DATA_SIGNATURE) {
    return;
  }

  if (Private->RxToken.Packet.RxData == NULL) {
    return;
  }

  if (Private->IpChoice == PING_IP_CHOICE_IP6) {
    Reply   = ((EFI_IP6_RECEIVE_DATA*)Private->RxToken.Packet.RxData)->FragmentTable[0].FragmentBuffer;
    PayLoad = ((EFI_IP6_RECEIVE_DATA*)Private->RxToken.Packet.RxData)->DataLength;
    if (((EFI_IP6_RECEIVE_DATA*)Private->RxToken.Packet.RxData)->Header->NextHeader != IP6_ICMP) {
      goto ON_EXIT;
    }
    if (!IP6_IS_MULTICAST ((EFI_IPv6_ADDRESS*)&Private->DstAddress) && 
        !EFI_IP6_EQUAL (&((EFI_IP6_RECEIVE_DATA*)Private->RxToken.Packet.RxData)->Header->SourceAddress, (EFI_IPv6_ADDRESS*)&Private->DstAddress)) {
      goto ON_EXIT;
    }

    if ((Reply->Type != ICMP_V6_ECHO_REPLY) || (Reply->Code != 0)) {
      goto ON_EXIT;
    }
  } else {
    Reply   = ((EFI_IP4_RECEIVE_DATA*)Private->RxToken.Packet.RxData)->FragmentTable[0].FragmentBuffer;
    PayLoad = ((EFI_IP4_RECEIVE_DATA*)Private->RxToken.Packet.RxData)->DataLength;
    if (!IP4_IS_MULTICAST (EFI_IP4(*(EFI_IPv4_ADDRESS*)Private->DstAddress)) && 
        !EFI_IP4_EQUAL (&((EFI_IP4_RECEIVE_DATA*)Private->RxToken.Packet.RxData)->Header->SourceAddress, (EFI_IPv4_ADDRESS*)&Private->DstAddress)) {
      goto ON_EXIT;
    }

    if ((Reply->Type != ICMP_V4_ECHO_REPLY) || (Reply->Code != 0)) {
      goto ON_EXIT;
    }
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
  Rtt  = CalculateTick (Reply->TimeStamp, ReadTime ());
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
    STRING_TOKEN (STR_PING_REPLY_INFO),
    gShellNetwork1HiiHandle,
    PayLoad,
    mDstString,
    Reply->SequenceNum,
    Private->IpChoice == PING_IP_CHOICE_IP6?((EFI_IP6_RECEIVE_DATA*)Private->RxToken.Packet.RxData)->Header->HopLimit:0,
    Near,
    Rtt
    );

ON_EXIT:

  if (Private->RxCount < Private->SendNum) {
    //
    // Continue to receive icmp echo reply packets.
    //
    Private->RxToken.Status = EFI_ABORTED;

    Status = Private->ProtocolPointers.Receive (Private->IpProtocol, &Private->RxToken);

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
  gBS->SignalEvent (Private->IpChoice == PING_IP_CHOICE_IP6?((EFI_IP6_RECEIVE_DATA*)Private->RxToken.Packet.RxData)->RecycleSignal:((EFI_IP4_RECEIVE_DATA*)Private->RxToken.Packet.RxData)->RecycleSignal);
}

/**
  Create a PING_IPX_COMPLETION_TOKEN.

  @param[in]    Private        The pointer of PING_PRIVATE_DATA.
  @param[in]    TimeStamp      The TimeStamp of request.
  @param[in]    SequenceNum    The SequenceNum of request.

  @return The pointer of PING_IPX_COMPLETION_TOKEN.

**/
PING_IPX_COMPLETION_TOKEN *
EFIAPI
PingGenerateToken (
  IN PING_PRIVATE_DATA    *Private,
  IN UINT64                TimeStamp,
  IN UINT16                SequenceNum
  )
{
  EFI_STATUS                  Status;
  PING_IPX_COMPLETION_TOKEN   *Token;
  VOID                        *TxData;
  ICMPX_ECHO_REQUEST_REPLY    *Request;
  UINT16                        HeadSum;
  UINT16                        TempChecksum;

  Request = AllocateZeroPool (Private->BufferSize);
  if (Request == NULL) {
    return NULL;
  }
  TxData = AllocateZeroPool (Private->IpChoice==PING_IP_CHOICE_IP6?sizeof (EFI_IP6_TRANSMIT_DATA):sizeof (EFI_IP4_TRANSMIT_DATA));
  if (TxData == NULL) {
    FreePool (Request);
    return NULL;
  }
  Token = AllocateZeroPool (sizeof (PING_IPX_COMPLETION_TOKEN));
  if (Token == NULL) {
    FreePool (Request);
    FreePool (TxData);
    return NULL;
  }

  //
  // Assembly echo request packet.
  //
  Request->Type        = (UINT8)(Private->IpChoice==PING_IP_CHOICE_IP6?ICMP_V6_ECHO_REQUEST:ICMP_V4_ECHO_REQUEST);
  Request->Code        = 0;
  Request->SequenceNum = SequenceNum;
  Request->Identifier  = 0;
  Request->Checksum    = 0;

  //
  // Assembly token for transmit.
  //
  if (Private->IpChoice==PING_IP_CHOICE_IP6) {
    ((EFI_IP6_TRANSMIT_DATA*)TxData)->ExtHdrsLength                   = 0;
    ((EFI_IP6_TRANSMIT_DATA*)TxData)->ExtHdrs                         = NULL;
    ((EFI_IP6_TRANSMIT_DATA*)TxData)->OverrideData                    = 0;
    ((EFI_IP6_TRANSMIT_DATA*)TxData)->DataLength                      = Private->BufferSize;
    ((EFI_IP6_TRANSMIT_DATA*)TxData)->FragmentCount                   = 1;
    ((EFI_IP6_TRANSMIT_DATA*)TxData)->FragmentTable[0].FragmentBuffer = (VOID *) Request;
    ((EFI_IP6_TRANSMIT_DATA*)TxData)->FragmentTable[0].FragmentLength = Private->BufferSize;
  } else {
    ((EFI_IP4_TRANSMIT_DATA*)TxData)->OptionsLength                   = 0;
    ((EFI_IP4_TRANSMIT_DATA*)TxData)->OptionsBuffer                   = NULL;
    ((EFI_IP4_TRANSMIT_DATA*)TxData)->OverrideData                    = 0;
    ((EFI_IP4_TRANSMIT_DATA*)TxData)->TotalDataLength                 = Private->BufferSize;
    ((EFI_IP4_TRANSMIT_DATA*)TxData)->FragmentCount                   = 1;
    ((EFI_IP4_TRANSMIT_DATA*)TxData)->FragmentTable[0].FragmentBuffer = (VOID *) Request;
    ((EFI_IP4_TRANSMIT_DATA*)TxData)->FragmentTable[0].FragmentLength = Private->BufferSize;
    ((EFI_IP4_TRANSMIT_DATA*)TxData)->DestinationAddress.Addr[0]      = Private->DstAddress[0];
    ((EFI_IP4_TRANSMIT_DATA*)TxData)->DestinationAddress.Addr[1]      = Private->DstAddress[1];
    ((EFI_IP4_TRANSMIT_DATA*)TxData)->DestinationAddress.Addr[2]      = Private->DstAddress[2];
    ((EFI_IP4_TRANSMIT_DATA*)TxData)->DestinationAddress.Addr[3]      = Private->DstAddress[3];

    HeadSum = NetChecksum ((UINT8 *) Request, Private->BufferSize);
    Request->TimeStamp   = TimeStamp;
    TempChecksum = NetChecksum ((UINT8 *) &Request->TimeStamp, sizeof (UINT64));
    Request->Checksum = (UINT16)(~NetAddChecksum (HeadSum, TempChecksum));
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
  Transmit the PING_IPX_COMPLETION_TOKEN.

  @param[in]    Private    The pointer of PING_PRIVATE_DATA.

  @retval EFI_SUCCESS             Transmitted successfully.
  @retval EFI_OUT_OF_RESOURCES    No memory is available on the platform.
  @retval others                  Transmitted unsuccessfully.

**/
EFI_STATUS
EFIAPI
PingSendEchoRequest (
  IN PING_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS             Status;
  PING_ICMPX_TX_INFO     *TxInfo;

  TxInfo = AllocateZeroPool (sizeof (PING_ICMPX_TX_INFO));

  if (TxInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TxInfo->TimeStamp   = ReadTime ();
  TxInfo->SequenceNum = (UINT16) (Private->TxCount + 1);
  TxInfo->Token       = PingGenerateToken (
                          Private,
                          TxInfo->TimeStamp,
                          TxInfo->SequenceNum
                          );

  if (TxInfo->Token == NULL) {
    PingDestroyTxInfo (TxInfo, Private->IpChoice);
    return EFI_OUT_OF_RESOURCES;
  }

  ASSERT(Private->ProtocolPointers.Transmit != NULL);
  Status = Private->ProtocolPointers.Transmit (Private->IpProtocol, TxInfo->Token);

  if (EFI_ERROR (Status)) {
    PingDestroyTxInfo (TxInfo, Private->IpChoice);
    return Status;
  }

  InsertTailList (&Private->TxList, &TxInfo->Link);
  Private->TxCount++;

  return EFI_SUCCESS;
}

/**
  Place a completion token into the receive packet queue to receive the echo reply.

  @param[in]    Private    The pointer of PING_PRIVATE_DATA.

  @retval EFI_SUCCESS      Put the token into the receive packet queue successfully.
  @retval others           Put the token into the receive packet queue unsuccessfully.

**/
EFI_STATUS
EFIAPI
Ping6ReceiveEchoReply (
  IN PING_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS    Status;

  ZeroMem (&Private->RxToken, sizeof (PING_IPX_COMPLETION_TOKEN));

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

  return (Private->ProtocolPointers.Receive (Private->IpProtocol, &Private->RxToken));
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
  PING_PRIVATE_DATA      *Private;
  PING_ICMPX_TX_INFO     *TxInfo;
  LIST_ENTRY             *Entry;
  LIST_ENTRY             *NextEntry;
  UINT64                 Time;

  Private = (PING_PRIVATE_DATA *) Context;
  if (Private->Signature != PING_PRIVATE_DATA_SIGNATURE) {
    Private->Status = EFI_NOT_FOUND;
    return;
  }

  //
  // Retransmit icmp6 echo request packets per second in sendnumber times.
  //
  if (Private->TxCount < Private->SendNum) {

    Status = PingSendEchoRequest (Private);
    if (Private->TxCount != 0){
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING_SEND_REQUEST), gShellNetwork1HiiHandle, Private->TxCount + 1);
      }
    }
  }
  //
  // Check whether any icmp6 echo request in the list timeout.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Private->TxList) {
    TxInfo = BASE_CR (Entry, PING_ICMPX_TX_INFO, Link);
    Time   = CalculateTick (TxInfo->TimeStamp, ReadTime ());

    //
    // Remove the timeout echo request from txlist.
    //
    if (Time > DEFAULT_TIMEOUT) {

      if (EFI_ERROR (TxInfo->Token->Status)) {
        Private->ProtocolPointers.Cancel (Private->IpProtocol, TxInfo->Token);
      }
      //
      // Remove the timeout icmp6 echo request from list.
      //
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING_TIMEOUT), gShellNetwork1HiiHandle, TxInfo->SequenceNum);

      RemoveEntryList (&TxInfo->Link);
      PingDestroyTxInfo (TxInfo, Private->IpChoice);

      //
      // We dont need to wait for this some other time...
      //
      Private->RxCount++;

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
  Determine if a IP4 address is Link Local.

  169.254.1.0 through 169.254.254.255 is link local.

  @param[in] Address  The address to test.

  @retval TRUE      It is.
  @retval FALSE     It is not.
**/
BOOLEAN
EFIAPI
PingNetIp4IsLinkLocalAddr (
  IN CONST EFI_IPv4_ADDRESS *Address
  )
{
  return ((BOOLEAN)(Address->Addr[0] == 169 && Address->Addr[1] == 254 && Address->Addr[2] >= 1 && Address->Addr[2] <= 254));
}

/**
  Determine if a IP4 address is unspecified.

  @param[in] Address  The address to test.

  @retval TRUE      It is.
  @retval FALSE     It is not.
**/
BOOLEAN
EFIAPI
PingNetIp4IsUnspecifiedAddr (
  IN CONST EFI_IPv4_ADDRESS *Address
  )
{
  return  ((BOOLEAN)((ReadUnaligned32 ((UINT32*)&Address->Addr[0])) == 0x00000000));
}

/**
  Create a valid IP instance.

  @param[in]    Private    The pointer of PING_PRIVATE_DATA.

  @retval EFI_SUCCESS              Create a valid IPx instance successfully.
  @retval EFI_ABORTED              Locate handle with ipx service binding protocol unsuccessfully.
  @retval EFI_INVALID_PARAMETER    The source address is unspecified when the destination address is a link-local address.
  @retval EFI_OUT_OF_RESOURCES     No memory is available on the platform.
  @retval EFI_NOT_FOUND            The source address is not found.
**/
EFI_STATUS
EFIAPI
PingCreateIpInstance (
  IN  PING_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS                       Status;
  UINTN                            HandleIndex;
  UINTN                            HandleNum;
  EFI_HANDLE                       *HandleBuffer;
  EFI_SERVICE_BINDING_PROTOCOL     *EfiSb;
  VOID                             *IpXCfg;
  EFI_IP6_CONFIG_DATA              Ip6Config;
  EFI_IP4_CONFIG_DATA              Ip4Config;
  VOID                             *IpXInterfaceInfo;
  UINTN                            IfInfoSize;
  EFI_IPv6_ADDRESS                 *Addr;
  UINTN                            AddrIndex;

  HandleBuffer      = NULL;
  EfiSb             = NULL;
  IpXInterfaceInfo  = NULL;
  IfInfoSize        = 0;

  //
  // Locate all the handles with ip6 service binding protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  Private->IpChoice == PING_IP_CHOICE_IP6?&gEfiIp6ServiceBindingProtocolGuid:&gEfiIp4ServiceBindingProtocolGuid,
                  NULL,
                  &HandleNum,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) || (HandleNum == 0) || (HandleBuffer == NULL)) {
    return EFI_ABORTED;
  }
  //
  // Source address is required when pinging a link-local address on multi-
  // interfaces host.
  //
  if (Private->IpChoice == PING_IP_CHOICE_IP6) {
    if (NetIp6IsLinkLocalAddr ((EFI_IPv6_ADDRESS*)&Private->DstAddress) &&
        NetIp6IsUnspecifiedAddr ((EFI_IPv6_ADDRESS*)&Private->SrcAddress) &&
        (HandleNum > 1)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellNetwork1HiiHandle, L"ping", mSrcString);  
      Status = EFI_INVALID_PARAMETER;
      goto ON_ERROR;
    }
  } else {
    ASSERT(Private->IpChoice == PING_IP_CHOICE_IP4);
    if (PingNetIp4IsLinkLocalAddr ((EFI_IPv4_ADDRESS*)&Private->DstAddress) &&
        PingNetIp4IsUnspecifiedAddr ((EFI_IPv4_ADDRESS*)&Private->SrcAddress) &&
        (HandleNum > 1)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellNetwork1HiiHandle, L"ping", mSrcString);  
      Status = EFI_INVALID_PARAMETER;
      goto ON_ERROR;
    }
  }
  //
  // For each ip6 protocol, check interface addresses list.
  //
  for (HandleIndex = 0; HandleIndex < HandleNum; HandleIndex++) {

    EfiSb             = NULL;
    IpXInterfaceInfo  = NULL;
    IfInfoSize        = 0;

    Status = gBS->HandleProtocol (
                    HandleBuffer[HandleIndex],
                    Private->IpChoice == PING_IP_CHOICE_IP6?&gEfiIp6ServiceBindingProtocolGuid:&gEfiIp4ServiceBindingProtocolGuid,
                    (VOID **) &EfiSb
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    if (Private->IpChoice == PING_IP_CHOICE_IP6?NetIp6IsUnspecifiedAddr ((EFI_IPv6_ADDRESS*)&Private->SrcAddress):PingNetIp4IsUnspecifiedAddr ((EFI_IPv4_ADDRESS*)&Private->SrcAddress)) {
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
                      Private->IpChoice == PING_IP_CHOICE_IP6?&gEfiIp6ConfigProtocolGuid:&gEfiIp4ConfigProtocolGuid,
                      (VOID **) &IpXCfg
                      );

      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }
      //
      // Get the interface information size.
      //
      if (Private->IpChoice == PING_IP_CHOICE_IP6) {
        Status = ((EFI_IP6_CONFIG_PROTOCOL*)IpXCfg)->GetData (
                           IpXCfg,
                           Ip6ConfigDataTypeInterfaceInfo,
                           &IfInfoSize,
                           NULL
                           );
      } else {
        Status = ((EFI_IP4_CONFIG_PROTOCOL*)IpXCfg)->GetData (
                           IpXCfg,
                           &IfInfoSize,
                           NULL
                           );
      }
      
      //
      // Skip the ones not in current use.
      //
      if (Status == EFI_NOT_STARTED) {
        continue;
      }

      if (Status != EFI_BUFFER_TOO_SMALL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING_GETDATA), gShellNetwork1HiiHandle, Status);
        goto ON_ERROR;
      }

      IpXInterfaceInfo = AllocateZeroPool (IfInfoSize);

      if (IpXInterfaceInfo == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_ERROR;
      }
      //
      // Get the interface info.
      //
      if (Private->IpChoice == PING_IP_CHOICE_IP6) {
        Status = ((EFI_IP6_CONFIG_PROTOCOL*)IpXCfg)->GetData (
                           IpXCfg,
                           Ip6ConfigDataTypeInterfaceInfo,
                           &IfInfoSize,
                           IpXInterfaceInfo
                           );
      } else {
        Status = ((EFI_IP4_CONFIG_PROTOCOL*)IpXCfg)->GetData (
                           IpXCfg,
                           &IfInfoSize,
                           IpXInterfaceInfo
                           );
      }

      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING_GETDATA), gShellNetwork1HiiHandle, Status);
        goto ON_ERROR;
      }
      //
      // Check whether the source address is one of the interface addresses.
      //
      if (Private->IpChoice == PING_IP_CHOICE_IP6) {
        for (AddrIndex = 0; AddrIndex < ((EFI_IP6_CONFIG_INTERFACE_INFO*)IpXInterfaceInfo)->AddressInfoCount; AddrIndex++) {

          Addr = &(((EFI_IP6_CONFIG_INTERFACE_INFO*)IpXInterfaceInfo)->AddressInfo[AddrIndex].Address);
          if (EFI_IP6_EQUAL (&Private->SrcAddress, Addr)) {
            //
            // Match a certain interface address.
            //
            break;
          }
        }

        if (AddrIndex < ((EFI_IP6_CONFIG_INTERFACE_INFO*)IpXInterfaceInfo)->AddressInfoCount) {
          //
          // Found a nic handle with right interface address.
          //
          break;
        }
      } else {
        //
        // IP4 address check
        //
        if (EFI_IP4_EQUAL (&Private->SrcAddress, &((EFI_IP4_IPCONFIG_DATA*)IpXInterfaceInfo)->StationAddress)) {
          //
          // Match a certain interface address.
          //
          break;
        }
      }
    }

    FreePool (IpXInterfaceInfo);
    IpXInterfaceInfo = NULL;
  }
  //
  // No exact interface address matched.
  //

  if (HandleIndex == HandleNum) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING_CONFIGD_NIC_NF), gShellNetwork1HiiHandle, L"ping");  
    Status = EFI_NOT_FOUND;
    goto ON_ERROR;
  }

  Private->NicHandle = HandleBuffer[HandleIndex];

  ASSERT (EfiSb != NULL);
  Status = EfiSb->CreateChild (EfiSb, &Private->IpChildHandle);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }
  if (Private->IpChoice == PING_IP_CHOICE_IP6) {
    Status = gBS->OpenProtocol (
                    Private->IpChildHandle,
                    &gEfiIp6ProtocolGuid,
                    &Private->IpProtocol,
                    gImageHandle,
                    Private->IpChildHandle,
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

    IP6_COPY_ADDRESS (&Ip6Config.StationAddress,     &Private->SrcAddress);
    IP6_COPY_ADDRESS (&Ip6Config.DestinationAddress, &Private->DstAddress);

    Status = ((EFI_IP6_PROTOCOL*)(Private->IpProtocol))->Configure (Private->IpProtocol, &Ip6Config);

    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING_CONFIG), gShellNetwork1HiiHandle, Status);
      goto ON_ERROR;
    }

    Private->ProtocolPointers.Transmit  = (PING_IPX_TRANSMIT )((EFI_IP6_PROTOCOL*)Private->IpProtocol)->Transmit;
    Private->ProtocolPointers.Receive   = (PING_IPX_RECEIVE  )((EFI_IP6_PROTOCOL*)Private->IpProtocol)->Receive;
    Private->ProtocolPointers.Cancel    = (PING_IPX_CANCEL   )((EFI_IP6_PROTOCOL*)Private->IpProtocol)->Cancel;
    Private->ProtocolPointers.Poll      = (PING_IPX_POLL     )((EFI_IP6_PROTOCOL*)Private->IpProtocol)->Poll;
  } else {
    Status = gBS->OpenProtocol (
                    Private->IpChildHandle,
                    &gEfiIp4ProtocolGuid,
                    &Private->IpProtocol,
                    gImageHandle,
                    Private->IpChildHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }


    ZeroMem (&Ip4Config, sizeof (EFI_IP4_CONFIG_DATA));

    //
    // Configure the ip4 instance for icmp4 packet exchange.
    //
//    PING_IP4_COPY_ADDRESS (&Ip4Config.StationAddress,     &Private->SrcAddress);
//    Ip4Config.SubnetMask.Addr[0] = 0xFF;
//    Ip4Config.SubnetMask.Addr[1] = 0xFF;
//    Ip4Config.SubnetMask.Addr[2] = 0xFF;
//    Ip4Config.SubnetMask.Addr[3] = 0x00;
    Ip4Config.DefaultProtocol   = 1;
    Ip4Config.AcceptAnyProtocol = FALSE;
    Ip4Config.AcceptBroadcast   = FALSE;
    Ip4Config.AcceptIcmpErrors  = TRUE;
    Ip4Config.AcceptPromiscuous = FALSE;
    Ip4Config.DoNotFragment     = FALSE;
    Ip4Config.RawData           = FALSE;
    Ip4Config.ReceiveTimeout    = 0;
    Ip4Config.TransmitTimeout   = 0;
    Ip4Config.UseDefaultAddress = TRUE;
    Ip4Config.TimeToLive        = 128;
    Ip4Config.TypeOfService     = 0;

    Status = ((EFI_IP4_PROTOCOL*)(Private->IpProtocol))->Configure (Private->IpProtocol, &Ip4Config);

    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING_CONFIG), gShellNetwork1HiiHandle, Status);
      goto ON_ERROR;
    }

    Private->ProtocolPointers.Transmit  = (PING_IPX_TRANSMIT )((EFI_IP4_PROTOCOL*)Private->IpProtocol)->Transmit;
    Private->ProtocolPointers.Receive   = (PING_IPX_RECEIVE  )((EFI_IP4_PROTOCOL*)Private->IpProtocol)->Receive;
    Private->ProtocolPointers.Cancel    = (PING_IPX_CANCEL   )((EFI_IP4_PROTOCOL*)Private->IpProtocol)->Cancel;
    Private->ProtocolPointers.Poll      = (PING_IPX_POLL     )((EFI_IP4_PROTOCOL*)Private->IpProtocol)->Poll;  
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  return EFI_SUCCESS;

ON_ERROR:
  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  if (IpXInterfaceInfo != NULL) {
    FreePool (IpXInterfaceInfo);
  }

  if ((EfiSb != NULL) && (Private->IpChildHandle != NULL)) {
    EfiSb->DestroyChild (EfiSb, Private->IpChildHandle);
  }

  return Status;
}

/**
  Destroy the IP instance.

  @param[in]    Private    The pointer of PING_PRIVATE_DATA.

**/
VOID
EFIAPI
Ping6DestroyIp6Instance (
  IN PING_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS                      Status;
  EFI_SERVICE_BINDING_PROTOCOL    *IpSb;

  gBS->CloseProtocol (
         Private->IpChildHandle,
         Private->IpChoice == PING_IP_CHOICE_IP6?&gEfiIp6ProtocolGuid:&gEfiIp4ProtocolGuid,
         gImageHandle,
         Private->IpChildHandle
         );

  Status = gBS->HandleProtocol (
                  Private->NicHandle,
                  Private->IpChoice == PING_IP_CHOICE_IP6?&gEfiIp6ServiceBindingProtocolGuid:&gEfiIp4ServiceBindingProtocolGuid,
                  (VOID **) &IpSb
                  );

  if (!EFI_ERROR(Status)) {
    IpSb->DestroyChild (IpSb, Private->IpChildHandle);
  }
}

/**
  The Ping Process.

  @param[in]   SendNumber     The send request count.
  @param[in]   BufferSize     The send buffer size.
  @param[in]   SrcAddress     The source address.
  @param[in]   DstAddress     The destination address.
  @param[in]   IpChoice       The choice between IPv4 and IPv6.

  @retval SHELL_SUCCESS  The ping processed successfullly.
  @retval others         The ping processed unsuccessfully.
**/
SHELL_STATUS
EFIAPI
ShellPing (
  IN UINT32              SendNumber,
  IN UINT32              BufferSize,
  IN EFI_IPv6_ADDRESS    *SrcAddress,
  IN EFI_IPv6_ADDRESS    *DstAddress,
  IN UINT32              IpChoice
  )
{
  EFI_STATUS             Status;
  PING_PRIVATE_DATA      *Private;
  PING_ICMPX_TX_INFO     *TxInfo;
  LIST_ENTRY             *Entry;
  LIST_ENTRY             *NextEntry;
  SHELL_STATUS           ShellStatus;

  ShellStatus = SHELL_SUCCESS;
  Private     = AllocateZeroPool (sizeof (PING_PRIVATE_DATA));

  if (Private == NULL) {
    return (SHELL_OUT_OF_RESOURCES);
  }

  Private->IpChoice    = IpChoice;
  Private->Signature   = PING_PRIVATE_DATA_SIGNATURE;
  Private->SendNum     = SendNumber;
  Private->BufferSize  = BufferSize;
  Private->RttMin      = ~((UINT64 )(0x0));
  Private->Status      = EFI_NOT_READY;

  CopyMem(&Private->SrcAddress, SrcAddress, sizeof(Private->SrcAddress));
  CopyMem(&Private->DstAddress, DstAddress, sizeof(Private->DstAddress));

  InitializeListHead (&Private->TxList);

  //
  // Open and configure a ip instance for us.
  //
  Status = PingCreateIpInstance (Private);

  if (EFI_ERROR (Status)) {
    ShellStatus = SHELL_ACCESS_DENIED;
    goto ON_EXIT;
  }
  //
  // Print the command line itself.
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING_START), gShellNetwork1HiiHandle, mDstString, Private->BufferSize);
  //
  // Create a ipv6 token to receive the first icmp6 echo reply packet.
  //
  Status = Ping6ReceiveEchoReply (Private);

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
                  Ping6OnTimerRoutine,
                  Private,
                  &Private->Timer
                  );

  if (EFI_ERROR (Status)) {
    ShellStatus = SHELL_ACCESS_DENIED;
    goto ON_EXIT;
  }
  //
  // Create a ipv6 token to send the first icmp6 echo request packet.
  //
  Status = PingSendEchoRequest (Private);
  //
  // EFI_NOT_READY for IPsec is enable and IKE is not established.
  //
  if (EFI_ERROR (Status) && (Status != EFI_NOT_READY)) {
    ShellStatus = SHELL_ACCESS_DENIED;
    if(Status == EFI_NOT_FOUND) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING_NOSOURCE_INDO), gShellNetwork1HiiHandle, mDstString);
    } else if (Status == RETURN_NO_MAPPING) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING_NOROUTE_FOUND), gShellNetwork1HiiHandle, mDstString, mSrcString);
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PING_NETWORK_ERROR), gShellNetwork1HiiHandle, L"ping", Status);  
    }

    goto ON_EXIT;
  }

  Status = gBS->SetTimer (
                  Private->Timer,
                  TimerPeriodic,
                  ONE_SECOND
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
    Status = Private->ProtocolPointers.Poll (Private->IpProtocol);
    if (ShellGetExecutionBreakFlag()) {
      Private->Status = EFI_ABORTED;
      goto ON_STAT;
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
      STRING_TOKEN (STR_PING_STAT),
      gShellNetwork1HiiHandle,
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
      STRING_TOKEN (STR_PING_RTT),
      gShellNetwork1HiiHandle,
      Private->RttMin,
      Private->RttMax,
      DivU64x64Remainder (Private->RttSum, Private->RxCount, NULL)
      );
  }

ON_EXIT:

  if (Private != NULL) {

    NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Private->TxList) {
      TxInfo = BASE_CR (Entry, PING_ICMPX_TX_INFO, Link);

      if (Private->IpProtocol != NULL && Private->ProtocolPointers.Cancel != NULL) {
        Status = Private->ProtocolPointers.Cancel (Private->IpProtocol, TxInfo->Token);
      }

      RemoveEntryList (&TxInfo->Link);
      PingDestroyTxInfo (TxInfo, Private->IpChoice);
    }

    if (Private->Timer != NULL) {
      gBS->CloseEvent (Private->Timer);
    }

    if (Private->IpProtocol != NULL && Private->ProtocolPointers.Cancel != NULL) {
      Status = Private->ProtocolPointers.Cancel (Private->IpProtocol, &Private->RxToken);
    }

    if (Private->RxToken.Event != NULL) {
      gBS->CloseEvent (Private->RxToken.Event);
    }

    if (Private->IpChildHandle != NULL) {
      Ping6DestroyIp6Instance (Private);
    }

    FreePool (Private);
  }

  return ShellStatus;
}

/**
  Function for 'ping' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunPing (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  SHELL_STATUS        ShellStatus;
  EFI_IPv6_ADDRESS    DstAddress;
  EFI_IPv6_ADDRESS    SrcAddress;
  UINT64              BufferSize;
  UINTN               SendNumber;
  LIST_ENTRY          *ParamPackage;
  CONST CHAR16        *ValueStr;
  UINTN               NonOptionCount;
  UINT32              IpChoice;
  CHAR16              *ProblemParam;

  //
  // we use IPv6 buffers to hold items... 
  // make sure this is enough space!
  //
  ASSERT(sizeof(EFI_IPv4_ADDRESS        ) <= sizeof(EFI_IPv6_ADDRESS         ));
  ASSERT(sizeof(EFI_IP4_COMPLETION_TOKEN) <= sizeof(EFI_IP6_COMPLETION_TOKEN ));

  IpChoice = PING_IP_CHOICE_IP4;

  ShellStatus = SHELL_SUCCESS;
  ProblemParam = NULL;

  Status = ShellCommandLineParseEx (PingParamList, &ParamPackage, &ProblemParam, TRUE, FALSE);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellNetwork1HiiHandle, L"ping", ProblemParam);
    ShellStatus = SHELL_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-_ip6")) {
    IpChoice = PING_IP_CHOICE_IP6;
  }

  //
  // Parse the paramter of count number.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"-n");
  if (ValueStr != NULL) {
    SendNumber = ShellStrToUintn (ValueStr);

    //
    // ShellStrToUintn will return 0 when input is 0 or an invalid input string.
    //
    if ((SendNumber == 0) || (SendNumber > MAX_SEND_NUMBER)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellNetwork1HiiHandle, L"ping", ValueStr);  
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  } else {
    SendNumber = DEFAULT_SEND_COUNT;
  }
  //
  // Parse the paramter of buffer size.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"-l");
  if (ValueStr != NULL) {
    BufferSize = ShellStrToUintn (ValueStr);

    //
    // ShellStrToUintn will return 0 when input is 0 or an invalid input string.
    //
    if ((BufferSize < 16) || (BufferSize > MAX_BUFFER_SIZE)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellNetwork1HiiHandle, L"ping", ValueStr);  
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  } else {
    BufferSize = DEFAULT_BUFFER_SIZE;
  }

  ZeroMem (&SrcAddress, sizeof (EFI_IPv6_ADDRESS));
  ZeroMem (&DstAddress, sizeof (EFI_IPv6_ADDRESS));

  //
  // Parse the paramter of source ip address.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"-_s");
  if (ValueStr != NULL) {
    mSrcString = ValueStr;
    if (IpChoice == PING_IP_CHOICE_IP6) {
      Status = NetLibStrToIp6 (ValueStr, &SrcAddress);
    } else {
      Status = NetLibStrToIp4 (ValueStr, (EFI_IPv4_ADDRESS*)&SrcAddress);
    }
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellNetwork1HiiHandle, L"ping", ValueStr);  
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  }
  //
  // Parse the paramter of destination ip address.
  //
  NonOptionCount = ShellCommandLineGetCount(ParamPackage);
  if (NonOptionCount < 2) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellNetwork1HiiHandle, L"ping");  
    ShellStatus = SHELL_INVALID_PARAMETER;
    goto ON_EXIT;
  }
  if (NonOptionCount > 2) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellNetwork1HiiHandle, L"ping");  
    ShellStatus = SHELL_INVALID_PARAMETER;
    goto ON_EXIT;
  }
  ValueStr = ShellCommandLineGetRawValue (ParamPackage, 1);
  if (ValueStr != NULL) {
    mDstString = ValueStr;
    if (IpChoice == PING_IP_CHOICE_IP6) {
      Status = NetLibStrToIp6 (ValueStr, &DstAddress);
    } else {
      Status = NetLibStrToIp4 (ValueStr, (EFI_IPv4_ADDRESS*)&DstAddress);
    }
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellNetwork1HiiHandle, L"ping", ValueStr);  
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  }
  //
  // Get frequency to calculate the time from ticks.
  //
  Status = GetFrequency ();

  if (EFI_ERROR(Status)) {
    goto ON_EXIT;
  }
  //
  // Enter into ping process.
  //
  ShellStatus = ShellPing (
             (UINT32)SendNumber,
             (UINT32)BufferSize,
             &SrcAddress,
             &DstAddress,
             IpChoice
             );

ON_EXIT:
  ShellCommandLineFreeVarList (ParamPackage);
  return ShellStatus;
}

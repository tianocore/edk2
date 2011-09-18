/** @file
  The interface function declaration of shell application Ping6 (Ping for v6 series).

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PING6_H_
#define _PING6_H_

#define PING6_DEFAULT_TIMEOUT      5000
#define PING6_MAX_SEND_NUMBER      10000
#define PING6_MAX_BUFFER_SIZE      32768
#define PING6_ONE_SECOND           10000000

//
// A similar amount of time that passes in femtoseconds
// for each increment of TimerValue. It is for NT32 only.
//
#define NTTIMERPERIOD    358049

#pragma pack(1)

typedef struct _ICMP6_ECHO_REQUEST_REPLY {
  UINT8                       Type;
  UINT8                       Code;
  UINT16                      Checksum;
  UINT16                      Identifier;
  UINT16                      SequenceNum;
  UINT64                      TimeStamp;
  UINT8                       Data[1];
} ICMP6_ECHO_REQUEST_REPLY;

#pragma pack()

typedef struct _PING6_ICMP6_TX_INFO {
  LIST_ENTRY                  Link;
  UINT16                      SequenceNum;
  UINT64                      TimeStamp;
  EFI_IP6_COMPLETION_TOKEN    *Token;
} PING6_ICMP6_TX_INFO;

typedef struct _PING6_PRIVATE_DATA {
  EFI_HANDLE                  ImageHandle;
  EFI_HANDLE                  NicHandle;
  EFI_HANDLE                  Ip6ChildHandle;
  EFI_IP6_PROTOCOL            *Ip6;
  EFI_EVENT                   Timer;

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

/**
  Reads and returns the current value of register.
  In IA64, the register is the Interval Timer Vector (ITV).
  In X86(IA32/X64), the register is the Time Stamp Counter (TSC)

  @return The current value of the register.

**/
UINT64
ReadTime (
  VOID
  );

#endif

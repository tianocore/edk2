/** @file
  This library provides IpIo layer upon EFI IP4 Protocol.

Copyright (c) 2005 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IP_IO_H_
#define _IP_IO_H_

#include <PiDxe.h>
#include <Protocol/Ip4.h>
#include <Library/IpIoLib.h>
#include <Library/NetLib.h>

//
// type and code define for ICMP protocol error got
// from IP
//
#define ICMP_TYPE_UNREACH              3
#define ICMP_TYPE_TIMXCEED            11
#define ICMP_TYPE_PARAMPROB            12
#define ICMP_TYPE_SOURCEQUENCH         4

#define ICMP_CODE_UNREACH_NET          0
#define ICMP_CODE_UNREACH_HOST         1
#define ICMP_CODE_UNREACH_PROTOCOL     2
#define ICMP_CODE_UNREACH_PORT         3
#define ICMP_CODE_UNREACH_NEEDFRAG     4
#define ICMP_CODE_UNREACH_SRCFAIL      5
#define ICMP_CODE_UNREACH_NET_UNKNOWN  6
#define ICMP_CODE_UNREACH_HOST_UNKNOWN 7
#define ICMP_CODE_UNREACH_ISOLATED     8
#define ICMP_CODE_UNREACH_NET_PROHIB   9
#define ICMP_CODE_UNREACH_HOST_PROHIB  10
#define ICMP_CODE_UNREACH_TOSNET       11
#define ICMP_CODE_UNREACH_TOSHOST      12

//
// this error will be delivered to the
// listening transportation layer protocol
// consuming IpIO
//
typedef enum {
  ICMP_ERR_UNREACH_NET      = 0,
  ICMP_ERR_UNREACH_HOST,
  ICMP_ERR_UNREACH_PROTOCOL,
  ICMP_ERR_UNREACH_PORT,
  ICMP_ERR_MSGSIZE,
  ICMP_ERR_UNREACH_SRCFAIL,
  ICMP_ERR_TIMXCEED_INTRANS,
  ICMP_ERR_TIMXCEED_REASS,
  ICMP_ERR_QUENCH,
  ICMP_ERR_PARAMPROB
} ICMP_ERROR;

typedef struct _ICMP_ERROR_INFO {
  BOOLEAN     IsHard;
  BOOLEAN     Notify;
} ICMP_ERROR_INFO;

#define EFI_IP4_HEADER_LEN(HdrPtr) ((HdrPtr)->HeaderLength << 2)

extern EFI_IP4_CONFIG_DATA  mIpIoDefaultIpConfigData;

typedef struct _EFI_NET_SESSION_DATA {
  IP4_ADDR        Source;
  IP4_ADDR        Dest;
  EFI_IP4_HEADER  *IpHdr;
} EFI_NET_SESSION_DATA;

typedef
VOID
(*PKT_RCVD_NOTIFY) (
  IN EFI_STATUS           Status,  // rcvd pkt result
  IN ICMP_ERROR           IcmpErr, // if Status == EFI_ICMP_ERROR, this
                                  // field is valid for user
  IN EFI_NET_SESSION_DATA *NetSession, // the communication point
  IN NET_BUF              *Pkt,    // packet received
  IN VOID                 *Context // the Context provided by user for recive data
  );

typedef
VOID
(*PKT_SENT_NOTIFY) (
  IN EFI_STATUS  Status,      // sent pkt result
  IN VOID        *Context,    // the context provided by user for sending data
  IN VOID        *Sender,     // the sender to be notified
  IN VOID        *NotifyData  // sent pkt related data to notify
  );

typedef struct _IP_IO {

  //
  // the node used to link this IpIo to the active IpIo list.
  //
  LIST_ENTRY                    Entry;

  // the list used to maintain the IP instance for different sending purpose.
  //
  LIST_ENTRY                    IpList;

  //
  // the ip instance consumed by this IP IO
  //
  EFI_HANDLE                    Controller;
  EFI_HANDLE                    Image;
  EFI_HANDLE                    ChildHandle;
  EFI_IP4_PROTOCOL              *Ip;
  BOOLEAN                       IsConfigured;

  //
  // some ip config data can be changed
  //
  UINT8                         Protocol;

  //
  // token and event used to get data from IP
  //
  EFI_IP4_COMPLETION_TOKEN      RcvToken;

  //
  // list entry used to link the token passed to IP_IO
  //
  LIST_ENTRY                    PendingSndList;

  //
  // User interface used to get notify from IP_IO
  //
  VOID                          *RcvdContext;
  VOID                          *SndContext;
  PKT_RCVD_NOTIFY               PktRcvdNotify;
  PKT_SENT_NOTIFY               PktSentNotify;
} IP_IO;

typedef struct _IP_IO_OPEN_DATA {
  EFI_IP4_CONFIG_DATA IpConfigData;
  VOID                *RcvdContext;
  VOID                *SndContext;
  PKT_RCVD_NOTIFY     PktRcvdNotify;
  PKT_SENT_NOTIFY     PktSentNotify;
} IP_IO_OPEN_DATA;

typedef struct _IP_IO_SEND_ENTRY {
  LIST_ENTRY                Entry;
  IP_IO                     *IpIo;
  VOID                      *Context;
  VOID                      *NotifyData;
  EFI_IP4_PROTOCOL          *Ip;
  NET_BUF                   *Pkt;
  EFI_IP4_COMPLETION_TOKEN  *SndToken;
} IP_IO_SEND_ENTRY;

typedef EFI_IP4_OVERRIDE_DATA IP_IO_OVERRIDE;

typedef struct _IP_IO_IP_INFO {
  IP4_ADDR                  Addr;
  IP4_ADDR                  SubnetMask;
  LIST_ENTRY                Entry;
  EFI_HANDLE                ChildHandle;
  EFI_IP4_PROTOCOL          *Ip;
  EFI_IP4_COMPLETION_TOKEN  DummyRcvToken;
  INTN                      RefCnt;
} IP_IO_IP_INFO;

/**
  Create a new IP_IO instance.

  @param  Image                 The image handle of an IP_IO consumer protocol.
  @param  Controller            The controller handle of an IP_IO consumer protocol
                                installed on.

  @return Pointer to a newly created IP_IO instance.

**/
IP_IO *
EFIAPI
IpIoCreate (
  IN EFI_HANDLE Image,
  IN EFI_HANDLE Controller
  );

/**
  Destroy an IP_IO instance.

  @param  IpIo                  Pointer to the IP_IO instance that needs to
                                destroy.

  @retval EFI_SUCCESS           The IP_IO instance destroyed successfully.
  @retval other                 Error condition occurred.

**/
EFI_STATUS
EFIAPI
IpIoDestroy (
  IN IP_IO *IpIo
  );

/**
  Stop an IP_IO instance.

  @param  IpIo                  Pointer to the IP_IO instance that needs to stop.

  @retval EFI_SUCCESS           The IP_IO instance stopped successfully.
  @retval other                 Error condition occurred.

**/
EFI_STATUS
EFIAPI
IpIoStop (
  IN IP_IO *IpIo
  );

/**
  Open an IP_IO instance for use.

  @param  IpIo                  Pointer to an IP_IO instance that needs to open.
  @param  OpenData              The configuration data for the IP_IO instance.

  @retval EFI_SUCCESS           The IP_IO instance opened with OpenData
                                successfully.
  @retval other                 Error condition occurred.

**/
EFI_STATUS
IpIoOpen (
  IN IP_IO           *IpIo,
  IN IP_IO_OPEN_DATA *OpenData
  );

/**
  Send out an IP packet.

  @param  IpIo                  Pointer to an IP_IO instance used for sending IP
                                packet.
  @param  Pkt                   Pointer to the IP packet to be sent.
  @param  Sender                The IP protocol instance used for sending.
  @param  Context               
  @param  NotifyData            
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
  IN IP_IO_IP_INFO   *Sender,
  IN VOID            *Context    OPTIONAL,
  IN VOID            *NotifyData OPTIONAL,
  IN IP4_ADDR        Dest,
  IN IP_IO_OVERRIDE  *OverrideData
  );

/**
  Cancel the IP transmit token which wraps this Packet.

  @param  IpIo                  Pointer to the IP_IO instance.
  @param  Packet                Pointer to the packet to cancel.

**/
VOID
EFIAPI
IpIoCancelTxToken (
  IN IP_IO  *IpIo,
  IN VOID   *Packet
  );

/**
  Add a new IP instance for sending data.

  @param  IpIo                  Pointer to a IP_IO instance to add a new IP
                                instance for sending purpose.

  @return Pointer to the created IP_IO_IP_INFO structure, NULL is failed.

**/
IP_IO_IP_INFO *
EFIAPI
IpIoAddIp (
  IN IP_IO  *IpIo
  );

/**
  Configure the IP instance of this IpInfo and start the receiving if Ip4ConfigData
  is not NULL.

  @param  IpInfo                Pointer to the IP_IO_IP_INFO instance.
  @param  Ip4ConfigData         The IP4 configure data used to configure the ip
                                instance, if NULL the ip instance is reseted. If
                                UseDefaultAddress is set to TRUE, and the configure
                                operation succeeds, the default address information
                                is written back in this Ip4ConfigData.

  @retval EFI_STATUS            The status returned by IP4->Configure or
                                IP4->Receive.

**/
EFI_STATUS
EFIAPI
IpIoConfigIp (
  IN     IP_IO_IP_INFO        *IpInfo,
  IN OUT EFI_IP4_CONFIG_DATA  *Ip4ConfigData OPTIONAL
  );

/**
  Destroy an IP instance maintained in IpIo->IpList for
  sending purpose.

  @param  IpIo                  Pointer to the IP_IO instance.
  @param  IpInfo                Pointer to the IpInfo to be removed.

  @return None.

**/
VOID
EFIAPI
IpIoRemoveIp (
  IN IP_IO            *IpIo,
  IN IP_IO_IP_INFO    *IpInfo
  );

/**
  Find the first IP protocol maintained in IpIo whose local
  address is the same with Src.

  @param  IpIo                  Pointer to the pointer of the IP_IO instance.
  @param  Src                   The local IP address.

  @return Pointer to the IP protocol can be used for sending purpose and its local
  @return address is the same with Src.

**/
IP_IO_IP_INFO *
EFIAPI
IpIoFindSender (
  IN OUT IP_IO     **IpIo,
  IN     IP4_ADDR  Src
  );

/**
  Get the ICMP error map information, the ErrorStatus will be returned.
  The IsHard and Notify are optional. If they are not NULL, this rouine will
  fill them.
  We move IcmpErrMap[] to local variable to enable EBC build.

  @param  IcmpError             IcmpError Type
  @param  IsHard                Whether it is a hard error
  @param  Notify                Whether it need to notify SockError

  @return ICMP Error Status

**/
EFI_STATUS
EFIAPI
IpIoGetIcmpErrStatus (
  IN  ICMP_ERROR  IcmpError,
  OUT BOOLEAN     *IsHard, OPTIONAL
  OUT BOOLEAN     *Notify OPTIONAL
  );

#endif

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

///
/// This error will be delivered to the
/// listening transportation layer protocol
/// that consumes IpIO.
///
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

///
/// The helper struct for IpIoGetIcmpErrStatus(). It is internal-use only.
///
typedef struct _ICMP_ERROR_INFO {
  BOOLEAN     IsHard;
  BOOLEAN     Notify;
} ICMP_ERROR_INFO;

/**
  Get the IP header length from EFI_IP4_HEADER struct.
  
  @param HdrPtr   A pointer to EFI_IP4_HEADER
  
  @return The IP header length
**/
#define EFI_IP4_HEADER_LEN(HdrPtr) ((HdrPtr)->HeaderLength << 2)

extern EFI_IP4_CONFIG_DATA  mIpIoDefaultIpConfigData;

///
/// The IP session for an IP receive packet.
///
typedef struct _EFI_NET_SESSION_DATA {
  IP4_ADDR        Source;     ///< Source IP of the received packet
  IP4_ADDR        Dest;       ///< Destination IP of the received packet
  EFI_IP4_HEADER  *IpHdr;     ///< IP4 header of the received packet
} EFI_NET_SESSION_DATA;

/**
  The prototype is called back when an IP packet is received.
  
  @param Status        Result of the receive request
  @param IcmpErr       Valid when Status is EFI_ICMP_ERROR
  @param NetSession    The IP session for the received packet
  @param Pkt           Packet received
  @param Context       The data provided by user for the received packet when
                       the callback is registered in IP_IO_OPEN_DATA::RcvdContext.
  
**/
typedef
VOID
(*PKT_RCVD_NOTIFY) (
  IN EFI_STATUS           Status, 
  IN ICMP_ERROR           IcmpErr,
  IN EFI_NET_SESSION_DATA *NetSession,
  IN NET_BUF              *Pkt,
  IN VOID                 *Context
  );

/**
  The prototype is called back when an IP packet is sent.
  
  @param Status        Result of the sending
  @param Context       The data provided by user for the received packet when
                       the callback is registered in IP_IO_OPEN_DATA::SndContext.
  @param Sender        A pointer to EFI_IP4_PROTOCOL for sender
  @param NotifyData    Context data specified when calling IpIoSend()
  
**/
typedef
VOID
(*PKT_SENT_NOTIFY) (
  IN EFI_STATUS  Status,
  IN VOID        *Context,
  IN VOID        *Sender,
  IN VOID        *NotifyData
  );

///
/// The data structure wraps Ip4 instance. It is used by IpIo Library to do all
/// Ip4 operations.
///
typedef struct _IP_IO {
  ///
  /// The node used to link this IpIo to the active IpIo list.
  ///
  LIST_ENTRY                    Entry;

  ///
  /// The list used to maintain the IP instance for different sending purpose.
  ///
  LIST_ENTRY                    IpList;
  
  EFI_HANDLE                    Controller;
  EFI_HANDLE                    Image;
  EFI_HANDLE                    ChildHandle;
  //
  // The IP instance consumed by this IP_IO
  //
  EFI_IP4_PROTOCOL              *Ip;
  BOOLEAN                       IsConfigured;

  ///
  /// some ip config data can be changed
  ///
  UINT8                         Protocol;

  ///
  /// Token and event used to get data from IP
  ///
  EFI_IP4_COMPLETION_TOKEN      RcvToken;

  ///
  /// List entry used to link the token passed to IP_IO
  ///
  LIST_ENTRY                    PendingSndList;

  //
  // User interface used to get notify from IP_IO
  //
  VOID                          *RcvdContext;    ///< See IP_IO_OPEN_DATA::RcvdContext
  VOID                          *SndContext;     ///< See IP_IO_OPEN_DATA::SndContext
  PKT_RCVD_NOTIFY               PktRcvdNotify;   ///< See IP_IO_OPEN_DATA::PktRcvdNotify
  PKT_SENT_NOTIFY               PktSentNotify;   ///< See IP_IO_OPEN_DATA::PktSentNotify
} IP_IO;

///
/// The struct is used for user to pass IP configuration and callbacks to IP_IO.
/// It is used by IpIoOpen().
///
typedef struct _IP_IO_OPEN_DATA {
  EFI_IP4_CONFIG_DATA IpConfigData;    ///< Configuration of the IP instance
  VOID                *RcvdContext;    ///< Context data used by receive callback
  VOID                *SndContext;     ///< Context data used by send callback
  PKT_RCVD_NOTIFY     PktRcvdNotify;   ///< Receive callback
  PKT_SENT_NOTIFY     PktSentNotify;   ///< Send callback
} IP_IO_OPEN_DATA;

///
/// Internal struct book-keeping send request of IP_IO.
///
/// An IP_IO_SEND_ENTRY will be created each time a send request is issued to
/// IP_IO via IpIoSend().
///
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

///
/// The IP_IO_IP_INFO is used in IpIoSend() to override the default IP instance
/// in IP_IO.
///
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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

#endif

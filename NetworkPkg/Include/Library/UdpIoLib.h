/** @file
  This library is used to share code between UEFI network stack modules.
  It provides the helper routines to access UDP service. It is used by both DHCP and MTFTP.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UDP_IO_H_
#define _UDP_IO_H_

#include <Protocol/Udp4.h>
#include <Protocol/Udp6.h>

#include <Library/NetLib.h>

typedef struct _UDP_IO UDP_IO;

///
/// Signatures used by UdpIo Library.
///

#define UDP_IO_RX_SIGNATURE  SIGNATURE_32 ('U', 'D', 'P', 'R')
#define UDP_IO_TX_SIGNATURE  SIGNATURE_32 ('U', 'D', 'P', 'T')
#define UDP_IO_SIGNATURE     SIGNATURE_32 ('U', 'D', 'P', 'I')

#define UDP_IO_UDP4_VERSION  4
#define UDP_IO_UDP6_VERSION  6

///
/// The UDP address pair.
///
typedef struct {
  EFI_IP_ADDRESS    LocalAddr;
  UINT16            LocalPort;
  EFI_IP_ADDRESS    RemoteAddr;
  UINT16            RemotePort;
} UDP_END_POINT;

/**
  Prototype called when receiving or sending packets to or from a UDP point.

  This prototype is used by both receive and sending when calling
  UdpIoRecvDatagram() or UdpIoSendDatagram(). When receiving, Netbuf is allocated by the
  UDP access point and released by the user. When sending, the user allocates the NetBuf,
  which is then provided to the callback as a reference.

  @param[in] Packet       The packet received or sent.
  @param[in] EndPoint     The UDP address pair corresponds to the UDP IO.
  @param[in] IoStatus     The packet receiving or sending status.
  @param[in] Context      The user-defined data when calling UdpIoRecvDatagram() or
                          UdpIoSendDatagram().
**/
typedef
VOID
(EFIAPI *UDP_IO_CALLBACK)(
  IN NET_BUF                *Packet,
  IN UDP_END_POINT          *EndPoint,
  IN EFI_STATUS             IoStatus,
  IN VOID                   *Context
  );

///
/// This structure is used internally by the UdpIo Library.
///
/// Each receive request is wrapped in an UDP_RX_TOKEN. Upon completion,
/// the CallBack will be called. Only one receive request is sent to UDP at a
/// time. HeadLen gives the length of the application's header. UDP_IO will
/// make the application's header continuous before delivering up.
///
typedef union {
  EFI_UDP4_COMPLETION_TOKEN    Udp4;
  EFI_UDP6_COMPLETION_TOKEN    Udp6;
} UDP_COMPLETION_TOKEN;

typedef struct {
  UINT32                  Signature;
  UDP_IO                  *UdpIo;

  UDP_IO_CALLBACK         CallBack;
  VOID                    *Context;
  UINT32                  HeadLen;

  UDP_COMPLETION_TOKEN    Token;
} UDP_RX_TOKEN;

///
/// This structure is used internally by UdpIo Library.
///
/// Each transmit request is wrapped in an UDP_TX_TOKEN. Upon completion,
/// the CallBack will be called. There can be several transmit requests. All transmit
/// requests are linked in a list.
///

typedef union {
  EFI_UDP4_SESSION_DATA    Udp4;
  EFI_UDP6_SESSION_DATA    Udp6;
} UDP_SESSION_DATA;

typedef union {
  EFI_UDP4_TRANSMIT_DATA    Udp4;
  EFI_UDP6_TRANSMIT_DATA    Udp6;
} UDP_TRANSMIT_DATA;

typedef struct {
  UINT32                  Signature;
  LIST_ENTRY              Link;
  UDP_IO                  *UdpIo;
  UDP_IO_CALLBACK         CallBack;
  NET_BUF                 *Packet;
  VOID                    *Context;
  EFI_IPv4_ADDRESS        Gateway;
  UDP_SESSION_DATA        Session;
  UDP_COMPLETION_TOKEN    Token;
  UDP_TRANSMIT_DATA       Data;
} UDP_TX_TOKEN;

///
/// Type defined as UDP_IO.
///
/// This data structure wraps the UDP instance and configuration.
/// UdpIo Library uses this structure for all Udp4 or Udp6 operations.
///
struct _UDP_IO {
  UINT32                     Signature;
  LIST_ENTRY                 Link;
  INTN                       RefCnt;
  UINT8                      UdpVersion;

  //
  // Handle used to create/destroy UDP child
  //
  EFI_HANDLE                 Controller;
  EFI_HANDLE                 Image;
  EFI_HANDLE                 UdpHandle;

  EFI_SIMPLE_NETWORK_MODE    SnpMode;

  LIST_ENTRY                 SentDatagram;  ///< A list of UDP_TX_TOKEN.
  UDP_RX_TOKEN               *RecvRequest;

  union {
    EFI_UDP4_PROTOCOL    *Udp4;
    EFI_UDP6_PROTOCOL    *Udp6;
  } Protocol;

  union {
    EFI_UDP4_CONFIG_DATA    Udp4;
    EFI_UDP6_CONFIG_DATA    Udp6;
  } Config;
};

/**
  The prototype called when UdpIo Library configures a UDP instance.

  The prototype is set and called when creating a UDP_IO in UdpIoCreatePort().

  @param[in] UdpIo         The UDP_IO to configure.
  @param[in] Context       The user-defined data when calling UdpIoCreatePort().

  @retval EFI_SUCCESS  The configuration succeeded.
  @retval Others       The UDP_IO fails to configure indicating
                       UdpIoCreatePort() should fail.
**/
typedef
EFI_STATUS
(EFIAPI *UDP_IO_CONFIG)(
  IN UDP_IO                 *UdpIo,
  IN VOID                   *Context
  );

/**
  The select function to decide whether to cancel the UDP_TX_TOKEN.

  @param[in] Token        The UDP_TX_TOKEN to decide whether to cancel.
  @param[in] Context      User-defined data in UdpIoCancelDgrams().

  @retval TRUE        Cancel the UDP_TX_TOKEN.
  @retval FALSE       Do not cancel this UDP_TX_TOKEN.

**/
typedef
BOOLEAN
(EFIAPI *UDP_IO_TO_CANCEL)(
  IN UDP_TX_TOKEN           *Token,
  IN VOID                   *Context
  );

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
  IN UDP_IO            *UdpIo,
  IN EFI_STATUS        IoStatus,
  IN UDP_IO_TO_CANCEL  ToCancel         OPTIONAL,
  IN VOID              *Context         OPTIONAL
  );

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

  @return The newly-created UDP_IO, or NULL if failed.

**/
UDP_IO *
EFIAPI
UdpIoCreateIo (
  IN  EFI_HANDLE     Controller,
  IN  EFI_HANDLE     ImageHandle,
  IN  UDP_IO_CONFIG  Configure,
  IN  UINT8          UdpVersion,
  IN  VOID           *Context
  );

/**
  Free the UDP_IO and all its related resources.

  If Udp version is not UDP_IO_UDP4_VERSION or UDP_IO_UDP6_VERSION, then ASSERT().

  The function cancels all sent datagrams and receive requests.

  @param[in]  UdpIo             The UDP_IO to free.

  @retval EFI_SUCCESS           The UDP_IO is freed.
  @retval Others                Failed to free UDP_IO.

**/
EFI_STATUS
EFIAPI
UdpIoFreeIo (
  IN  UDP_IO  *UdpIo
  );

/**
  Cleans up the UDP_IO without freeing it. Call this function
  if you intend to later re-use the UDP_IO.

  If Udp version is not UDP_IO_UDP4_VERSION or UDP_IO_UDP6_VERSION, then ASSERT().

  This function releases all transmitted datagrams and receive requests and configures NULL for the UDP instance.

  @param[in]  UdpIo                 The UDP_IO to clean up.

**/
VOID
EFIAPI
UdpIoCleanIo (
  IN  UDP_IO  *UdpIo
  );

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
  @retval EFI_SUCCESS           The packet is successfully delivered to UDP for
                                transmission.

**/
EFI_STATUS
EFIAPI
UdpIoSendDatagram (
  IN  UDP_IO           *UdpIo,
  IN  NET_BUF          *Packet,
  IN  UDP_END_POINT    *EndPoint OPTIONAL,
  IN  EFI_IP_ADDRESS   *Gateway  OPTIONAL,
  IN  UDP_IO_CALLBACK  CallBack,
  IN  VOID             *Context
  );

/**
  Cancel a single sent datagram.

  @param[in]  UdpIo                 The UDP_IO from which to cancel the packet
  @param[in]  Packet                The packet to cancel

**/
VOID
EFIAPI
UdpIoCancelSentDatagram (
  IN  UDP_IO   *UdpIo,
  IN  NET_BUF  *Packet
  );

/**
  Issue a receive request to the UDP_IO.

  If Udp version is not UDP_IO_UDP4_VERSION or UDP_IO_UDP6_VERSION, then ASSERT().

  This function is called when upper-layer needs packet from UDP for processing.
  Only one receive request is acceptable at a time. Therefore, one common usage model is
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
  @retval EFI_SUCCESS           The receive request was issued successfully.
  @retval EFI_UNSUPPORTED       The UDP version in UDP_IO is not supported.

**/
EFI_STATUS
EFIAPI
UdpIoRecvDatagram (
  IN  UDP_IO           *UdpIo,
  IN  UDP_IO_CALLBACK  CallBack,
  IN  VOID             *Context,
  IN  UINT32           HeadLen
  );

#endif

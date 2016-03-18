/** @file

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_IP4_OUTPUT_H__
#define __EFI_IP4_OUTPUT_H__

/**
  The default callback function for system generated packet.
  It will free the packet.

  @param  Ip4Instance          The IP4 child that issued the transmission.  It most
                               like is NULL.
  @param  Packet               The packet that transmitted.
  @param  IoStatus             The result of the transmission, succeeded or failed.
  @param  LinkFlag             Not used when transmission. check IP4_FRAME_CALLBACK
                               for reference.
  @param  Context              The context provided by us

**/
VOID
Ip4SysPacketSent (
  IP4_PROTOCOL              *Ip4Instance,
  NET_BUF                   *Packet,
  EFI_STATUS                IoStatus,
  UINT32                    LinkFlag,
  VOID                      *Context
  );

/**
  Transmit an IP4 packet. The packet comes either from the IP4
  child's consumer (IpInstance != NULL) or the IP4 driver itself
  (IpInstance == NULL). It will route the packet, fragment it,
  then transmit all the fragments through some interface.

  @param[in]  IpSb             The IP4 service instance to transmit the packet
  @param[in]  IpInstance       The IP4 child that issues the transmission.  It is
                               NULL if the packet is from the system.
  @param[in]  Packet           The user data to send, excluding the IP header.
  @param[in]  Head             The caller supplied header. The caller should set
                               the following header fields: Tos, TotalLen, Id, tl,
                               Fragment, Protocol, Src and Dst. All the fields are
                               in host byte  order. This function will fill in the
                               Ver, HeadLen,  Fragment, and checksum. The Fragment
                               only need to include the DF flag. Ip4Output will
                               compute the MF and offset for  you.
  @param[in]  Option           The original option to append to the IP headers
  @param[in]  OptLen           The length of the option
  @param[in]  GateWay          The next hop address to transmit packet to.
                               255.255.255.255 means broadcast.
  @param[in]  Callback         The callback function to issue when transmission
                               completed.
  @param[in]  Context          The opaque context for the callback

  @retval EFI_NO_MAPPING       There is no interface to the destination.
  @retval EFI_NOT_FOUND        There is no route to the destination
  @retval EFI_SUCCESS          The packet is successfully transmitted.
  @retval Others               Failed to transmit the packet.

**/
EFI_STATUS
Ip4Output (
  IN IP4_SERVICE            *IpSb,
  IN IP4_PROTOCOL           *IpInstance  OPTIONAL,
  IN NET_BUF                *Packet,
  IN IP4_HEAD               *Head,
  IN UINT8                  *Option,
  IN UINT32                 OptLen,
  IN IP4_ADDR               GateWay,
  IN IP4_FRAME_CALLBACK     Callback,
  IN VOID                   *Context
  );

/**
  Cancel the Packet and all its fragments.

  @param  IpIf                 The interface from which the Packet is sent
  @param  Packet               The Packet to cancel
  @param  IoStatus             The status returns to the sender.

**/
VOID
Ip4CancelPacket (
  IN IP4_INTERFACE    *IpIf,
  IN NET_BUF          *Packet,
  IN EFI_STATUS       IoStatus
  );

/**
  Prepend an IP4 head to the Packet. It will copy the options and
  build the IP4 header fields. Used for IP4 fragmentation.

  @param  Packet           The packet to prepend IP4 header to
  @param  Head             The caller supplied header. The caller should set
                           the following header fields: Tos, TotalLen, Id,
                           Fragment, Ttl, Protocol, Src and Dst. All the fields
                           are in host byte order. This function will fill in
                           the Ver, HeadLen, and checksum.
  @param  Option           The orginal IP4 option to copy from
  @param  OptLen           The length of the IP4 option

  @retval EFI_BAD_BUFFER_SIZE  There is no enought room in the head space of
                               Packet.
  @retval EFI_SUCCESS          The IP4 header is successfully added to the packet.

**/
EFI_STATUS
Ip4PrependHead (
  IN OUT NET_BUF                *Packet,
  IN     IP4_HEAD               *Head,
  IN     UINT8                  *Option,
  IN     UINT32                 OptLen
  );

extern UINT16  mIp4Id;

#endif

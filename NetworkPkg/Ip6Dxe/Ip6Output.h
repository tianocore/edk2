/** @file
  The internal functions and routines to transmit the IP6 packet.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_IP6_OUTPUT_H__
#define __EFI_IP6_OUTPUT_H__

extern UINT32 mIp6Id;

/**
  Output all the available source addresses to the list entry head SourceList. The
  number of source addresses are also returned.

  @param[in]       IpSb             Points to a IP6 service binding instance.
  @param[in]       Destination      The IPv6 destination address.
  @param[out]      Source           The selected IPv6 source address according to
                                    the Destination.

  @retval EFI_SUCCESS           The source addresses were copied to the list entry
                                head SourceList.
  @retval EFI_NO_MAPPING        The IPv6 stack is not auto configured.

**/
EFI_STATUS
Ip6SelectSourceAddress (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IPv6_ADDRESS       *Destination,
  OUT EFI_IPv6_ADDRESS      *Source
  );

/**
  The default callback function for system generated packet.
  It will free the packet.

  @param[in]  Packet        The packet that transmitted.
  @param[in]  IoStatus      The result of the transmission: succeeded or failed.
  @param[in]  LinkFlag      Not used when transmission. Check IP6_FRAME_CALLBACK
                            for reference.
  @param[in]  Context       The context provided by us.

**/
VOID
Ip6SysPacketSent (
  NET_BUF                   *Packet,
  EFI_STATUS                IoStatus,
  UINT32                    LinkFlag,
  VOID                      *Context
  );

/**
  Transmit an IP6 packet. The packet comes either from the IP6
  child's consumer (IpInstance != NULL) or the IP6 driver itself
  (IpInstance == NULL). It will route the packet, fragment it,
  then transmit all the fragments through an interface.

  @param[in]  IpSb             The IP6 service instance to transmit the packet.
  @param[in]  Interface        The IP6 interface to transmit the packet. Ignored
                               if NULL.
  @param[in]  IpInstance       The IP6 child that issues the transmission.  It is
                               NULL if the packet is from the system.
  @param[in]  Packet           The user data to send, excluding the IP header.
  @param[in]  Head             The caller supplied header. The caller should set
                               the  following header fields: NextHeader, HopLimit,
                               Src, Dest, FlowLabel, PayloadLength. This function
                               will fill in the Ver, TrafficClass.
  @param[in]  ExtHdrs          The extension headers to append to the IPv6 basic
                               header.
  @param[in]  ExtHdrsLen       The length of the extension headers.
  @param[in]  Callback         The callback function to issue when transmission
                               completed.
  @param[in]  Context          The opaque context for the callback.

  @retval EFI_INVALID_PARAMETER Any input parameter or the packet is invalid.
  @retval EFI_NO_MAPPING        There is no interface to the destination.
  @retval EFI_NOT_FOUND         There is no route to the destination.
  @retval EFI_SUCCESS           The packet successfully transmitted.
  @retval EFI_OUT_OF_RESOURCES  Failed to finish the operation due to lack of
                                resources.
  @retval Others                Failed to transmit the packet.

**/
EFI_STATUS
Ip6Output (
  IN IP6_SERVICE            *IpSb,
  IN IP6_INTERFACE          *Interface   OPTIONAL,
  IN IP6_PROTOCOL           *IpInstance  OPTIONAL,
  IN NET_BUF                *Packet,
  IN EFI_IP6_HEADER         *Head,
  IN UINT8                  *ExtHdrs,
  IN UINT32                 ExtHdrsLen,
  IN IP6_FRAME_CALLBACK     Callback,
  IN VOID                   *Context
  );

/**
  Remove all the frames on the interface that pass the FrameToCancel,
  either queued on ARP queues, or that have already been delivered to
  MNP and not yet recycled.

  @param[in]  Interface     Interface to remove the frames from.
  @param[in]  IoStatus      The transmit status returned to the frames' callback.
  @param[in]  FrameToCancel Function to select the frame to cancel; NULL to select all.
  @param[in]  Context       Opaque parameters passed to FrameToCancel. Ignored if
                            FrameToCancel is NULL.

**/
VOID
Ip6CancelFrames (
  IN IP6_INTERFACE          *Interface,
  IN EFI_STATUS             IoStatus,
  IN IP6_FRAME_TO_CANCEL    FrameToCancel   OPTIONAL,
  IN VOID                   *Context        OPTIONAL
  );

/**
  Cancel the Packet and all its fragments.

  @param[in]  IpIf                 The interface from which the Packet is sent.
  @param[in]  Packet               The Packet to cancel.
  @param[in]  IoStatus             The status returns to the sender.

**/
VOID
Ip6CancelPacket (
  IN IP6_INTERFACE    *IpIf,
  IN NET_BUF          *Packet,
  IN EFI_STATUS       IoStatus
  );

#endif

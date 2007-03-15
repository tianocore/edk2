/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  pxe_bc_ip.c

Abstract:

--*/


#include "Bc.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
BOOLEAN
OnSameSubnet (
  IN UINTN           IpLength,
  IN EFI_IP_ADDRESS  *Ip1,
  IN EFI_IP_ADDRESS  *Ip2,
  IN EFI_IP_ADDRESS  *SubnetMask
  )
/*++

  Routine Description:
    Check if two IP addresses are on the same subnet.

  Arguments:
    IpLength   - Length of IP address in bytes.
    Ip1        - IP address to check.
    Ip2        - IP address to check.
    SubnetMask - Subnet mask to check with.

  Returns:
    TRUE       - IP addresses are on the same subnet.
    FALSE      - IP addresses are on different subnets.

--*/
{
  if (IpLength == 0 || Ip1 == NULL || Ip2 == NULL || SubnetMask == NULL) {
    return FALSE;
  }

  while (IpLength-- != 0) {
    if ((Ip1->v6.Addr[IpLength] ^ Ip2->v6.Addr[IpLength]) & SubnetMask->v6.Addr[IpLength]) {
      return FALSE;
    }
  }

  return TRUE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
VOID
IpAddRouter (
  IN PXE_BASECODE_DEVICE *Private,
  IN EFI_IP_ADDRESS      *RouterIpPtr
  )
/*++

  Routine Description:
    Add router to router table.

  Arguments:
    Private     - Pointer PxeBc instance data.
    RouterIpPtr - Pointer to router IP address.

  Returns:
    Nothing

--*/
{
  EFI_PXE_BASE_CODE_MODE  *PxeBcMode;
  UINTN                   Index;

  if (Private == NULL || RouterIpPtr == NULL) {
    return ;
  }

  PxeBcMode = Private->EfiBc.Mode;

  //
  // if we are filled up or this is not on the same subnet, forget it
  //
  if ((PxeBcMode->RouteTableEntries == PXE_ROUTER_TABLE_SIZE) ||
    !OnSameSubnet(Private->IpLength, &PxeBcMode->StationIp, RouterIpPtr, &PxeBcMode->SubnetMask)) {
    return ;
  }
  //
  // make sure we don't already have it
  //
  for (Index = 0; Index < PxeBcMode->RouteTableEntries; ++Index) {
    if (!CompareMem (
          &PxeBcMode->RouteTable[Index].GwAddr,
          RouterIpPtr,
          Private->IpLength
          )) {
      return ;
    }
  }
  //
  // keep it
  //
  ZeroMem (
    &PxeBcMode->RouteTable[PxeBcMode->RouteTableEntries],
    sizeof (EFI_PXE_BASE_CODE_ROUTE_ENTRY)
    );

  CopyMem (
    &PxeBcMode->RouteTable[PxeBcMode->RouteTableEntries++].GwAddr,
    RouterIpPtr,
    Private->IpLength
    );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// return router ip to use for DestIp (0 if none)
//
STATIC
EFI_IP_ADDRESS *
GetRouterIp (
  PXE_BASECODE_DEVICE *Private,
  EFI_IP_ADDRESS      *DestIpPtr
  )
{
  EFI_PXE_BASE_CODE_MODE  *PxeBcMode;
  UINTN                   Index;

  if (Private == NULL || DestIpPtr == NULL) {
    return NULL;
  }

  PxeBcMode = Private->EfiBc.Mode;

  for (Index = 0; Index < PxeBcMode->RouteTableEntries; ++Index) {
    if (OnSameSubnet (
          Private->IpLength,
          &PxeBcMode->RouteTable[Index].IpAddr,
          DestIpPtr,
          &PxeBcMode->RouteTable[Index].SubnetMask
          )) {
      return &PxeBcMode->RouteTable[Index].GwAddr;
    }
  }

  return NULL;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// routine to send ipv4 packet
// ipv4 header of length HdrLth in TransmitBufferPtr
// routine fills in ipv4hdr Ver_Hdl, TotalLength, and Checksum, moves in Data
// and gets dest MAC address
//
#define IP_TX_BUFFER  ((IPV4_BUFFER *) Private->TransmitBufferPtr)
#define IP_TX_HEADER  IP_TX_BUFFER->IpHeader

EFI_STATUS
Ipv4Xmt (
  PXE_BASECODE_DEVICE         *Private,
  UINT32                      GatewayIp,
  UINTN                       IpHeaderLength,
  UINTN                       TotalHeaderLength,
  VOID                        *Data,
  UINTN                       DataLength,
  EFI_PXE_BASE_CODE_FUNCTION  Function
  )
{
  EFI_MAC_ADDRESS             DestMac;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  EFI_PXE_BASE_CODE_MODE      *PxeBcMode;
  EFI_STATUS                  StatCode;
  UINTN                       PacketLength;

  Snp           = Private->SimpleNetwork;
  PxeBcMode     = Private->EfiBc.Mode;
  StatCode      = EFI_SUCCESS;
  PacketLength  = TotalHeaderLength + DataLength;

  //
  // get dest MAC address
  // multicast - convert to hw equiv
  // unicast on same net, use arp
  // on different net, arp for router
  //
  if (IP_TX_HEADER.DestAddr.L == BROADCAST_IPv4) {
    CopyMem (&DestMac, &Snp->Mode->BroadcastAddress, sizeof (DestMac));
  } else if (IS_MULTICAST (&IP_TX_HEADER.DestAddr)) {
    StatCode = (*Snp->MCastIpToMac) (Snp, PxeBcMode->UsingIpv6, (EFI_IP_ADDRESS *) &IP_TX_HEADER.DestAddr, &DestMac);
  } else {
    UINT32  Ip;

    if (OnSameSubnet (
          Private->IpLength,
          &PxeBcMode->StationIp,
          (EFI_IP_ADDRESS *) &IP_TX_HEADER.DestAddr,
          &PxeBcMode->SubnetMask
          )) {
      Ip = IP_TX_HEADER.DestAddr.L;
    } else if (GatewayIp != 0) {
      Ip = GatewayIp;
    } else {
      EFI_IP_ADDRESS  *TmpIp;

      TmpIp = GetRouterIp (Private, (EFI_IP_ADDRESS *) &IP_TX_HEADER.DestAddr);

      if (TmpIp == NULL) {
        DEBUG (
          (EFI_D_WARN,
          "\nIpv4Xmit()  Exit #1  %xh (%r)",
          EFI_NO_RESPONSE,
          EFI_NO_RESPONSE)
          );

        return EFI_NO_RESPONSE;
        //
        // no router
        //
      }

      Ip = TmpIp->Addr[0];
    }

    if (!GetHwAddr (
          Private,
          (EFI_IP_ADDRESS *) &Ip,
          (EFI_MAC_ADDRESS *) &DestMac
          )) {
      if (!PxeBcMode->AutoArp) {
        DEBUG (
          (EFI_D_WARN,
          "\nIpv4Xmit()  Exit #2  %xh (%r)",
          EFI_DEVICE_ERROR,
          EFI_DEVICE_ERROR)
          );

        return EFI_DEVICE_ERROR;
      } else {
        StatCode = DoArp (
                    Private,
                    (EFI_IP_ADDRESS *) &Ip,
                    (EFI_MAC_ADDRESS *) &DestMac
                    );
      }
    }
  }

  if (EFI_ERROR (StatCode)) {
    DEBUG ((EFI_D_WARN, "\nIpv4Xmit()  Exit #3  %xh (%r)", StatCode, StatCode));
    return StatCode;
  }
  //
  // fill in packet info
  //
  SET_IPV4_VER_HDL (&IP_TX_HEADER, IpHeaderLength);
  IP_TX_HEADER.TotalLength    = HTONS (PacketLength);
  IP_TX_HEADER.HeaderChecksum = IpChecksum ((UINT16 *) &IP_TX_HEADER, IpHeaderLength);
  CopyMem (((UINT8 *) &IP_TX_HEADER) + TotalHeaderLength, Data, DataLength);

  //
  // send it
  //
  return SendPacket (
          Private,
          (UINT8 *) &IP_TX_HEADER - Snp->Mode->MediaHeaderSize,
          &IP_TX_HEADER,
          PacketLength,
          &DestMac,
          PXE_PROTOCOL_ETHERNET_IP,
          Function
          );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// send ipv4 packet with option
//
EFI_STATUS
Ipv4SendWOp (
  PXE_BASECODE_DEVICE         *Private,
  UINT32                      GatewayIp,
  UINT8                       *Msg,
  UINTN                       MessageLength,
  UINT8                       Prot,
  UINT8                       *Option,
  UINTN                       OptionLength,
  UINT32                      DestIp,
  EFI_PXE_BASE_CODE_FUNCTION  Function
  )
{
  EFI_PXE_BASE_CODE_MODE  *PxeBcMode;
  UINTN                   HdrLth;

  PxeBcMode = Private->EfiBc.Mode;
  HdrLth    = sizeof (IPV4_HEADER) + OptionLength;

  ZeroMem ((VOID *) &IP_TX_HEADER, sizeof (IPV4_HEADER));
  IP_TX_HEADER.TimeToLive     = PxeBcMode->TTL;
  IP_TX_HEADER.TypeOfService  = PxeBcMode->ToS;
  IP_TX_HEADER.Protocol       = Prot;
  IP_TX_HEADER.SrcAddr.L      = *(UINT32 *) &PxeBcMode->StationIp;
  IP_TX_HEADER.DestAddr.L     = DestIp;
  IP_TX_HEADER.Id             = Random (Private);
  CopyMem (IP_TX_BUFFER->u.Data, Option, OptionLength);
  return Ipv4Xmt (
          Private,
          GatewayIp,
          HdrLth,
          HdrLth,
          Msg,
          MessageLength,
          Function
          );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// send MessageLength message at MessagePtr - higher level protocol header already in TransmitBufferPtr, length HdrSize
//
EFI_STATUS
Ip4Send (
  PXE_BASECODE_DEVICE                          *Private,  // pointer to instance data
  UINTN               MayFrag,                            //
  UINT8                                    Prot,          // protocol
  UINT32                          SrcIp,                  // Source IP address
  UINT32                 DestIp,                          // Destination IP address
  UINT32              GatewayIp,                          // used if not NULL and needed
  UINTN               HdrSize,                            // protocol header byte length
  UINT8               *MessagePtr,                        // pointer to data
  UINTN               MessageLength                       // data byte length
  )
{
  EFI_STATUS  StatCode;
  UINTN       TotDataLength;

  TotDataLength = HdrSize + MessageLength;

  if (TotDataLength > MAX_IPV4_DATA_SIZE) {
    DEBUG (
      (EFI_D_WARN,
      "\nIp4Send()  Exit #1  %xh (%r)",
      EFI_BAD_BUFFER_SIZE,
      EFI_BAD_BUFFER_SIZE)
      );

    return EFI_BAD_BUFFER_SIZE;
  }

  ZeroMem ((VOID *) &IP_TX_HEADER, sizeof (IPV4_HEADER));
  IP_TX_HEADER.TimeToLive = DEFAULT_TTL;
  IP_TX_HEADER.Protocol   = Prot;
  IP_TX_HEADER.SrcAddr.L  = SrcIp;
  IP_TX_HEADER.DestAddr.L = DestIp;
  IP_TX_HEADER.Id         = Random (Private);

  if (!MayFrag) {
    *(UINT8 *) (&IP_TX_HEADER.FragmentFields) = IP_NO_FRAG >> 8;
  }
  //
  // check for need to fragment
  //
  if (TotDataLength > MAX_IPV4_FRAME_DATA_SIZE) {
    UINTN   DataLengthSent;
    UINT16  FragmentOffset;

    FragmentOffset = IP_MORE_FRAG;
    //
    // frag offset field
    //
    if (!MayFrag) {
      DEBUG (
        (EFI_D_WARN,
        "\nIp4Send()  Exit #2  %xh (%r)",
        EFI_BAD_BUFFER_SIZE,
        EFI_BAD_BUFFER_SIZE)
        );

      return EFI_BAD_BUFFER_SIZE;
    }
    //
    // send out in fragments - first includes upper level header
    // all are max and include more frag bit except last
    //
    * (UINT8 *) (&IP_TX_HEADER.FragmentFields) = IP_MORE_FRAG >> 8;

#define IPV4_FRAG_SIZE    (MAX_IPV4_FRAME_DATA_SIZE & 0xfff8)
#define IPV4_FRAG_OFF_INC (IPV4_FRAG_SIZE >> 3)

    DataLengthSent = IPV4_FRAG_SIZE - HdrSize;

    StatCode = Ipv4Xmt (
                Private,
                GatewayIp,
                sizeof (IPV4_HEADER),
                sizeof (IPV4_HEADER) + HdrSize,
                MessagePtr,
                DataLengthSent,
                Private->Function
                );

    if (EFI_ERROR (StatCode)) {
      DEBUG (
        (EFI_D_WARN,
        "\nIp4Send()  Exit #3  %xh (%r)",
        StatCode,
        StatCode)
        );

      return StatCode;
    }

    MessagePtr += DataLengthSent;
    MessageLength -= DataLengthSent;
    FragmentOffset += IPV4_FRAG_OFF_INC;
    IP_TX_HEADER.FragmentFields = HTONS (FragmentOffset);

    while (MessageLength > IPV4_FRAG_SIZE) {
      StatCode = Ipv4Xmt (
                  Private,
                  GatewayIp,
                  sizeof (IPV4_HEADER),
                  sizeof (IPV4_HEADER),
                  MessagePtr,
                  IPV4_FRAG_SIZE,
                  Private->Function
                  );

      if (EFI_ERROR (StatCode)) {
        DEBUG (
          (EFI_D_WARN,
          "\nIp4Send()  Exit #3  %xh (%r)",
          StatCode,
          StatCode)
          );

        return StatCode;
      }

      MessagePtr += IPV4_FRAG_SIZE;
      MessageLength -= IPV4_FRAG_SIZE;
      FragmentOffset += IPV4_FRAG_OFF_INC;
      IP_TX_HEADER.FragmentFields = HTONS (FragmentOffset);
    }

    * (UINT8 *) (&IP_TX_HEADER.FragmentFields) &= ~(IP_MORE_FRAG >> 8);
    HdrSize = 0;
  }
  //
  // transmit
  //
  return Ipv4Xmt (
          Private,
          GatewayIp,
          sizeof (IPV4_HEADER),
          sizeof (IPV4_HEADER) + HdrSize,
          MessagePtr,
          MessageLength,
          Private->Function
          );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// return true if dst IP in receive header matched with what's enabled
//
STATIC
BOOLEAN
IPgood (
  PXE_BASECODE_DEVICE *Private,
  IPV4_HEADER         *IpHeader
  )
{
  EFI_PXE_BASE_CODE_MODE  *PxeBcMode;
  UINTN                   Index;

  PxeBcMode = Private->EfiBc.Mode;

  if (PxeBcMode->IpFilter.Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS) {
    return TRUE;
  }

  if ((PxeBcMode->IpFilter.Filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS_MULTICAST) &&
      IS_MULTICAST (&IpHeader->DestAddr)
        ) {
    return TRUE;
  }

  if ((PxeBcMode->IpFilter.Filters & EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP) &&
      PxeBcMode->StationIp.Addr[0] == IpHeader->DestAddr.L
      ) {
    return TRUE;
  }

  if ((PxeBcMode->IpFilter.Filters & EFI_PXE_BASE_CODE_IP_FILTER_BROADCAST) && IpHeader->DestAddr.L == BROADCAST_IPv4) {
    return TRUE;
  }

  for (Index = 0; Index < PxeBcMode->IpFilter.IpCnt; ++Index) {
    if (IpHeader->DestAddr.L == PxeBcMode->IpFilter.IpList[Index].Addr[0]) {
      return TRUE;
    }
  }

  return FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// receive up to MessageLength message into MessagePtr for protocol Prot
// return message length, src/dest ips if select any, and pointer to protocol
// header routine will filter based on source and/or dest ip if OpFlags set.
//
EFI_STATUS
IpReceive (
  PXE_BASECODE_DEVICE *Private,
  PXE_OPFLAGS         OpFlags,
  EFI_IP_ADDRESS      *SrcIpPtr,
  EFI_IP_ADDRESS      *DestIpPtr,
  UINT8               Prot,
  VOID                *HeaderPtr,
  UINTN               HdrSize,
  UINT8               *MessagePtr,
  UINTN               *MessageLengthPtr,
  EFI_EVENT           TimeoutEvent
  )
{
  EFI_PXE_BASE_CODE_MODE  *PxeBcMode;
  EFI_STATUS              StatCode;
  UINTN                   ByteCount;
  UINTN                   FragmentCount;
  UINTN                   ExpectedPacketLength;
  UINTN                   Id;
  BOOLEAN                 GotFirstFragment;
  BOOLEAN                 GotLastFragment;

  DEBUG (
    (EFI_D_NET,
    "\nIpReceive()  Hdr=%Xh  HdrSz=%d  Data=%Xh  DataSz=%d",
    HeaderPtr,
    HdrSize,
    MessagePtr,
    *MessageLengthPtr)
    );

  PxeBcMode                     = Private->EfiBc.Mode;
  PxeBcMode->IcmpErrorReceived  = FALSE;

  ExpectedPacketLength          = 0;
  GotFirstFragment              = FALSE;
  GotLastFragment               = FALSE;
  FragmentCount                 = 0;
  ByteCount                     = 0;
  Id = 0;

  for (;;) {
    IPV4_HEADER IpHdr;
    UINTN       FFlds;
    UINTN       TotalLength;
    UINTN       FragmentOffset;
    UINTN       HeaderSize;
    UINTN       BufferSize;
    UINTN       IpHeaderLength;
    UINTN       DataLength;
    UINT16      Protocol;
    UINT8       *NextHdrPtr;
    UINT8       *PacketPtr;

    StatCode = WaitForReceive (
                Private,
                Private->Function,
                TimeoutEvent,
                &HeaderSize,
                &BufferSize,
                &Protocol
                );

    if (EFI_ERROR (StatCode)) {
      return StatCode;
    }

    PacketPtr = Private->ReceiveBufferPtr + HeaderSize;

    if (Protocol == PXE_PROTOCOL_ETHERNET_ARP) {
      HandleArpReceive (
        Private,
        (ARP_PACKET *) PacketPtr,
        Private->ReceiveBufferPtr
        );

      continue;
    }

    if (Protocol != PXE_PROTOCOL_ETHERNET_IP) {
      continue;
    }

#define IpRxHeader  ((IPV4_HEADER *) PacketPtr)

    //
    // filter for version & check sum
    //
    IpHeaderLength = IPV4_HEADER_LENGTH (IpRxHeader);

    if ((IpRxHeader->VersionIhl >> 4) != IPVER4) {
      continue;
    }

    if (IpChecksum ((UINT16 *) IpRxHeader, IpHeaderLength)) {
      continue;
    }

    CopyMem (&IpHdr, IpRxHeader, sizeof (IpHdr));
    //IpHdr       = *IpRxHeader;
    TotalLength = NTOHS (IpHdr.TotalLength);

    if (IpHdr.Protocol == PROT_TCP) {
      //
      // The NextHdrPtr is used to seed the header buffer we are passing back.
      // That being the case, we want to see everything in pPkt which contains
      // everything but the ethernet (or whatever) frame.  IP + TCP in this case.
      //
      DataLength  = TotalLength;
      NextHdrPtr  = PacketPtr;
    } else {
      DataLength  = TotalLength - IpHeaderLength;
      NextHdrPtr  = PacketPtr + IpHeaderLength;
    }
    //
    // If this is an ICMP, it might not be for us.
    // Double check the state of the IP stack and the
    // packet fields before assuming it is an ICMP
    // error.  ICMP requests are not supported by the
    // PxeBc IP stack and should be ignored.
    //
    if (IpHdr.Protocol == PROT_ICMP) {
      ICMPV4_HEADER *Icmpv4;

      Icmpv4 = (ICMPV4_HEADER *) NextHdrPtr;

      //
      // For now only obvious ICMP error replies will be accepted by
      // this stack.  This still makes us vulnerable to DoS attacks.
      // But at least we will not be killed by DHCP daemons.
      //
      switch (Icmpv4->Type) {
      case ICMP_REDIRECT:
      case ICMP_ECHO:
      case ICMP_ROUTER_ADV:
      case ICMP_ROUTER_SOLICIT:
      case ICMP_TIMESTAMP:
      case ICMP_TIMESTAMP_REPLY:
      case ICMP_INFO_REQ:
      case ICMP_INFO_REQ_REPLY:
      case ICMP_SUBNET_MASK_REQ:
      case ICMP_SUBNET_MASK_REPLY:
      default:
        continue;

      //
      // %%TBD - This should be implemented.
      //
      case ICMP_ECHO_REPLY:
        continue;

      case ICMP_DEST_UNREACHABLE:
      case ICMP_TIME_EXCEEDED:
      case ICMP_PARAMETER_PROBLEM:
      case ICMP_SOURCE_QUENCH:
        PxeBcMode->IcmpErrorReceived = TRUE;

        CopyMem (
          &PxeBcMode->IcmpError,
          NextHdrPtr,
          sizeof (EFI_PXE_BASE_CODE_ICMP_ERROR)
          );

        DEBUG (
          (EFI_D_NET,
          "\nIpReceive()  Exit #1  %Xh (%r)",
          EFI_ICMP_ERROR,
          EFI_ICMP_ERROR)
          );
      }

      return EFI_ICMP_ERROR;
    }

    if (IpHdr.Protocol == PROT_IGMP) {
      HandleIgmp (Private, (IGMPV2_MESSAGE *) NextHdrPtr, DataLength);

      DEBUG ((EFI_D_NET, "\n  IGMP"));
      continue;
    }
    //
    // check for protocol
    //
    if (IpHdr.Protocol != Prot) {
      continue;
    }
    //
    // do filtering
    //
    if (!(OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP) && SrcIpPtr && SrcIpPtr->Addr[0] != IpHdr.SrcAddr.L) {
      DEBUG ((EFI_D_NET, "\n  Not expected source IP address."));
      continue;
    }

    if (OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_USE_FILTER) {
      if (!IPgood (Private, &IpHdr)) {
        continue;
      }
    } else if (!(OpFlags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_IP)) {
      if (DestIpPtr == NULL) {
        if (PxeBcMode->StationIp.Addr[0] != IpHdr.DestAddr.L) {
          continue;
        }
      } else if (DestIpPtr->Addr[0] != IpHdr.DestAddr.L) {
        continue;
      }
    }
    //
    // get some data we need
    //
    FFlds           = NTOHS (IpHdr.FragmentFields);
    FragmentOffset  = ((FFlds & IP_FRAG_OFF_MSK) << 3);

    /* Keep count of fragments that belong to this session.
     * If we get packets with a different IP ID number,
     * ignore them.  Ignored packets should be handled
     * by the upper level protocol.
     */
    if (FragmentCount == 0) {
      Id = IpHdr.Id;

      if (DestIpPtr != NULL) {
        DestIpPtr->Addr[0] = IpHdr.DestAddr.L;
      }

      if (SrcIpPtr != NULL) {
        SrcIpPtr->Addr[0] = IpHdr.SrcAddr.L;
      }
    } else {
      if (IpHdr.Id != Id) {
        continue;
      }
    }

    ++FragmentCount;

    /* Fragment management.
     */
    if (FragmentOffset == 0) {
      /* This is the first fragment (may also be the
       * only fragment).
       */
      GotFirstFragment = TRUE;

      /* If there is a separate protocol header buffer,
       * copy the header, adjust the data pointer and
       * the data length.
       */
      if (HdrSize != 0) {
        CopyMem (HeaderPtr, NextHdrPtr, HdrSize);

        NextHdrPtr += HdrSize;
        DataLength -= HdrSize;
      }
    } else {
      /* If there is a separate protocol header buffer,
       * adjust the fragment offset.
       */
      FragmentOffset -= HdrSize;
    }

    /* See if this is the last fragment.
     */
    if (!(FFlds & IP_MORE_FRAG)) {
      //
      // This is the last fragment (may also be the only fragment).
      //
      GotLastFragment = TRUE;

      /* Compute the expected length of the assembled
       * packet.  This will be used to decide if we
       * have gotten all of the fragments.
       */
      ExpectedPacketLength = FragmentOffset + DataLength;
    }

    DEBUG (
      (EFI_D_NET,
      "\n  ID = %Xh  Off = %d  Len = %d",
      Id,
      FragmentOffset,
      DataLength)
      );

    /* Check for receive buffer overflow.
     */
    if (FragmentOffset + DataLength > *MessageLengthPtr) {
      /* There is not enough space in the receive
       * buffer for the fragment.
       */
      DEBUG (
        (EFI_D_NET,
        "\nIpReceive()  Exit #3  %Xh (%r)",
        EFI_BUFFER_TOO_SMALL,
        EFI_BUFFER_TOO_SMALL)
        );

      return EFI_BUFFER_TOO_SMALL;
    }

    /* Copy data into receive buffer.
     */
    if (DataLength != 0) {
      DEBUG ((EFI_D_NET, "  To = %Xh", MessagePtr + FragmentOffset));

      CopyMem (MessagePtr + FragmentOffset, NextHdrPtr, DataLength);
      ByteCount += DataLength;
    }

    /* If we have seen the first and last fragments and
     * the receive byte count is at least as large as the
     * expected byte count, return SUCCESS.
     *
     * We could be tricked by receiving a fragment twice
     * but the upper level protocol should figure this
     * out.
     */
    if (GotFirstFragment && GotLastFragment && ByteCount >= ExpectedPacketLength) {
      *MessageLengthPtr = ExpectedPacketLength;
      return EFI_SUCCESS;
    }
  }
}

/* eof - pxe_bc_ip.c */

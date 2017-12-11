/** @file

Copyright (c) 2005 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ip4Impl.h"

IP4_ICMP_CLASS
mIcmpClass[] = {
  {ICMP_ECHO_REPLY,         ICMP_QUERY_MESSAGE  },
  {1,                       ICMP_INVALID_MESSAGE},
  {2,                       ICMP_INVALID_MESSAGE},
  {ICMP_DEST_UNREACHABLE,   ICMP_ERROR_MESSAGE  },
  {ICMP_SOURCE_QUENCH,      ICMP_ERROR_MESSAGE  },
  {ICMP_REDIRECT,           ICMP_ERROR_MESSAGE  },
  {6,                       ICMP_INVALID_MESSAGE},
  {7,                       ICMP_INVALID_MESSAGE},
  {ICMP_ECHO_REQUEST,       ICMP_QUERY_MESSAGE  },
  {9,                       ICMP_INVALID_MESSAGE},
  {10,                      ICMP_INVALID_MESSAGE},
  {ICMP_TIME_EXCEEDED,      ICMP_ERROR_MESSAGE  },
  {ICMP_PARAMETER_PROBLEM,  ICMP_ERROR_MESSAGE  },
  {ICMP_TIMESTAMP ,         ICMP_QUERY_MESSAGE  },
  {14,                      ICMP_INVALID_MESSAGE},
  {ICMP_INFO_REQUEST ,      ICMP_QUERY_MESSAGE  },
  {ICMP_INFO_REPLY ,        ICMP_QUERY_MESSAGE  },
};

EFI_IP4_ICMP_TYPE
mIp4SupportedIcmp[23] = {
  {ICMP_ECHO_REPLY,        ICMP_DEFAULT_CODE        },

  {ICMP_DEST_UNREACHABLE,  ICMP_NET_UNREACHABLE     },
  {ICMP_DEST_UNREACHABLE,  ICMP_HOST_UNREACHABLE    },
  {ICMP_DEST_UNREACHABLE,  ICMP_PROTO_UNREACHABLE   },
  {ICMP_DEST_UNREACHABLE,  ICMP_PORT_UNREACHABLE    },
  {ICMP_DEST_UNREACHABLE,  ICMP_FRAGMENT_FAILED     },
  {ICMP_DEST_UNREACHABLE,  ICMP_SOURCEROUTE_FAILED  },
  {ICMP_DEST_UNREACHABLE,  ICMP_NET_UNKNOWN         },
  {ICMP_DEST_UNREACHABLE,  ICMP_HOST_UNKNOWN        },
  {ICMP_DEST_UNREACHABLE,  ICMP_SOURCE_ISOLATED     },
  {ICMP_DEST_UNREACHABLE,  ICMP_NET_PROHIBITED      },
  {ICMP_DEST_UNREACHABLE,  ICMP_HOST_PROHIBITED     },
  {ICMP_DEST_UNREACHABLE,  ICMP_NET_UNREACHABLE_TOS },
  {ICMP_DEST_UNREACHABLE,  ICMP_HOST_UNREACHABLE_TOS},

  {ICMP_SOURCE_QUENCH,     ICMP_DEFAULT_CODE        },

  {ICMP_REDIRECT,          ICMP_NET_REDIRECT        },
  {ICMP_REDIRECT,          ICMP_HOST_REDIRECT       },
  {ICMP_REDIRECT,          ICMP_NET_TOS_REDIRECT    },
  {ICMP_REDIRECT,          ICMP_HOST_TOS_REDIRECT   },

  {ICMP_ECHO_REQUEST,      ICMP_DEFAULT_CODE        },

  {ICMP_TIME_EXCEEDED,     ICMP_TIMEOUT_IN_TRANSIT  },
  {ICMP_TIME_EXCEEDED,     ICMP_TIMEOUT_REASSEMBLE  },

  {ICMP_PARAMETER_PROBLEM, ICMP_DEFAULT_CODE        },
};



/**
  Process the ICMP redirect. Find the instance then update
  its route cache.
  
  All kinds of redirect is treated as host redirect as
  specified by RFC1122 3.3.1.2:
  "Since the subnet mask appropriate to the destination
  address is generally not known, a Network Redirect
  message SHOULD be treated identically to a Host Redirect
  message;"

  @param[in]  IpSb               The IP4 service binding instance that received 
                                 the packet.
  @param[in]  Head               The IP head of the received ICMPpacket.
  @param[in]  Packet             The content of the ICMP redirect packet with IP
                                 head removed.
  @param[in]  Icmp               The buffer to store the ICMP error message if
                                 something is wrong.

  @retval EFI_INVALID_PARAMETER  The parameter is invalid
  @retval EFI_SUCCESS            Successfully updated the route caches

**/
EFI_STATUS
Ip4ProcessIcmpRedirect (
  IN IP4_SERVICE            *IpSb,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet,
  IN IP4_ICMP_ERROR_HEAD    *Icmp
  )
{
  LIST_ENTRY                *Entry;
  IP4_PROTOCOL              *Ip4Instance;
  IP4_ROUTE_CACHE_ENTRY     *CacheEntry;
  IP4_INTERFACE             *IpIf;
  IP4_ADDR                  Gateway;
  IP4_ADDR                  Src;
  IP4_ADDR                  Dst;

  //
  // Find the interface whose IP address is the source of the
  // orgianl IP packet.
  //
  IpIf    = Ip4FindInterface (IpSb, NTOHL (Icmp->IpHead.Src));
  Gateway = NTOHL (Icmp->Fourth);

  //
  // discard the packet if the new gateway address it specifies
  // is not on the same connected net through which the Redirect
  // arrived. (RFC1122 3.2.2.2).
  //
  if ((IpIf == NULL) || !IP4_NET_EQUAL (Gateway, IpIf->Ip, IpIf->SubnetMask)) {
    NetbufFree (Packet);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Update each IP child's route cache on the interface.
  //
  NET_LIST_FOR_EACH (Entry, &IpIf->IpInstances) {
    Ip4Instance = NET_LIST_USER_STRUCT (Entry, IP4_PROTOCOL, AddrLink);

    if (Ip4Instance->RouteTable == NULL) {
      continue;
    }

    Dst = NTOHL (Icmp->IpHead.Dst);
    Src = NTOHL (Icmp->IpHead.Src);
    CacheEntry = Ip4FindRouteCache (Ip4Instance->RouteTable, Dst, Src);

    //
    // Only update the route cache's gateway if the source of the
    // Redirect is the current first-hop gateway
    //
    if ((CacheEntry != NULL) && (NTOHL (Head->Src) == CacheEntry->NextHop)) {
      CacheEntry->NextHop = Gateway;
    }
  }

  NetbufFree (Packet);
  return EFI_SUCCESS;
}


/**
  Process the ICMP error packet. If it is an ICMP redirect packet,
  update call Ip4ProcessIcmpRedirect to update the IP instance's
  route cache, otherwise, deliver the packet to upper layer.

  @param[in]  IpSb               The IP4 service that received the packet.
  @param[in]  Head               The IP4 head of the ICMP error packet
  @param[in]  Packet             The content of the ICMP error with IP4 head
                                 removed.

  @retval EFI_SUCCESS            The ICMP error is processed successfully.
  @retval EFI_INVALID_PARAMETER  The packet is invalid
  @retval Others                 Failed to process the packet.
  
**/
EFI_STATUS
Ip4ProcessIcmpError (
  IN IP4_SERVICE            *IpSb,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet
  )
{
  IP4_ICMP_ERROR_HEAD       Icmp;

  if (Packet->TotalSize < sizeof (Icmp)) {
    NetbufFree (Packet);
    return EFI_INVALID_PARAMETER;
  }

  NetbufCopy (Packet, 0, sizeof (Icmp), (UINT8 *) &Icmp);

  //
  // If it is an ICMP redirect error, update the route cache
  // as RFC1122. Otherwise, demultiplex it to IP instances.
  //
  if (Icmp.Head.Type == ICMP_REDIRECT) {
    return Ip4ProcessIcmpRedirect (IpSb, Head, Packet, &Icmp);
  }

  IP4_GET_CLIP_INFO (Packet)->Status = EFI_ICMP_ERROR;
  return Ip4Demultiplex (IpSb, Head, Packet, NULL, 0);
}


/**
  Replay an ICMP echo request.

  @param[in]  IpSb               The IP4 service that receivd the packet
  @param[in]  Head               The IP4 head of the ICMP error packet
  @param[in]  Packet             The content of the ICMP error with IP4 head
                                 removed.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resource.
  @retval EFI_SUCCESS            The ICMP Echo request is successfully answered.
  @retval Others                 Failed to answer the ICMP echo request.

**/
EFI_STATUS
Ip4IcmpReplyEcho (
  IN IP4_SERVICE            *IpSb,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet
  )
{
  IP4_ICMP_QUERY_HEAD       *Icmp;
  NET_BUF                   *Data;
  EFI_STATUS                Status;
  IP4_HEAD                  ReplyHead;

  //
  // make a copy the packet, it is really a bad idea to
  // send the MNP's buffer back to MNP.
  //
  Data = NetbufDuplicate (Packet, NULL, IP4_MAX_HEADLEN);

  if (Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Change the ICMP type to echo reply, exchange the source
  // and destination, then send it. The source is updated to
  // use specific destination. See RFC1122. SRR/RR option
  // update is omitted.
  //
  Icmp                = (IP4_ICMP_QUERY_HEAD *) NetbufGetByte (Data, 0, NULL);
  ASSERT (Icmp != NULL);
  Icmp->Head.Type     = ICMP_ECHO_REPLY;
  Icmp->Head.Checksum = 0;
  Icmp->Head.Checksum = (UINT16) (~NetblockChecksum ((UINT8 *) Icmp, Data->TotalSize));

  ReplyHead.Tos       = 0;
  ReplyHead.Fragment  = 0;
  ReplyHead.Ttl       = 64;
  ReplyHead.Protocol  = EFI_IP_PROTO_ICMP;
  ReplyHead.Src       = 0;

  //
  // Ip4Output will select a source for us
  //
  ReplyHead.Dst = Head->Src;

  Status = Ip4Output (
             IpSb,
             NULL,
             Data,
             &ReplyHead,
             NULL,
             0,
             IP4_ALLZERO_ADDRESS,
             Ip4SysPacketSent,
             NULL
             );
  if (EFI_ERROR (Status)) {
    NetbufFree (Data);
  }

ON_EXIT:
  NetbufFree (Packet);
  return Status;
}


/**
  Process the ICMP query message. If it is an ICMP echo
  request, answer it. Otherwise deliver it to upper layer.

  @param[in]  IpSb               The IP4 service that receivd the packet
  @param[in]  Head               The IP4 head of the ICMP query packet
  @param[in]  Packet             The content of the ICMP query with IP4 head
                                 removed.

  @retval EFI_INVALID_PARAMETER  The packet is invalid
  @retval EFI_SUCCESS            The ICMP query message is processed
  @retval Others                 Failed to process ICMP query.

**/
EFI_STATUS
Ip4ProcessIcmpQuery (
  IN IP4_SERVICE            *IpSb,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet
  )
{
  IP4_ICMP_QUERY_HEAD       Icmp;

  if (Packet->TotalSize < sizeof (Icmp)) {
    NetbufFree (Packet);
    return EFI_INVALID_PARAMETER;
  }

  NetbufCopy (Packet, 0, sizeof (Icmp), (UINT8 *) &Icmp);

  if (Icmp.Head.Type == ICMP_ECHO_REQUEST) {
    return Ip4IcmpReplyEcho (IpSb, Head, Packet);
  }

  return Ip4Demultiplex (IpSb, Head, Packet, NULL, 0);
}


/**
  Handle the ICMP packet. First validate the message format,
  then according to the message types, process it as query or
  error packet.

  @param[in]  IpSb               The IP4 service that receivd the packet.
  @param[in]  Head               The IP4 head of the ICMP query packet.
  @param[in]  Packet             The content of the ICMP query with IP4 head
                                 removed.

  @retval EFI_INVALID_PARAMETER  The packet is malformated.
  @retval EFI_SUCCESS            The ICMP message is successfully processed.
  @retval Others                 Failed to handle ICMP packet.

**/
EFI_STATUS
Ip4IcmpHandle (
  IN IP4_SERVICE            *IpSb,
  IN IP4_HEAD               *Head,
  IN NET_BUF                *Packet
  )
{
  IP4_ICMP_HEAD             Icmp;
  UINT16                    Checksum;

  if (Packet->TotalSize < sizeof (Icmp)) {
    goto DROP;
  }

  NetbufCopy (Packet, 0, sizeof (Icmp), (UINT8 *) &Icmp);

  if (Icmp.Type > ICMP_TYPE_MAX) {
    goto DROP;
  }

  Checksum = (UINT16) (~NetbufChecksum (Packet));
  if ((Icmp.Checksum != 0) && (Checksum != 0)) {
    goto DROP;
  }

  if (mIcmpClass[Icmp.Type].IcmpClass == ICMP_ERROR_MESSAGE) {
    return Ip4ProcessIcmpError (IpSb, Head, Packet);

  } else if (mIcmpClass[Icmp.Type].IcmpClass == ICMP_QUERY_MESSAGE) {
    return Ip4ProcessIcmpQuery (IpSb, Head, Packet);

  }

DROP:
  NetbufFree (Packet);
  return EFI_INVALID_PARAMETER;
}

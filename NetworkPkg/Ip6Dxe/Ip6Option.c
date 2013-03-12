/** @file
  IP6 option support functions and routines.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ip6Impl.h"

/**
  Validate the IP6 option format for both the packets we received
  and that we will transmit. It will compute the ICMPv6 error message fields
  if the option is malformated.

  @param[in]  IpSb              The IP6 service data.
  @param[in]  Packet            The to be validated packet.
  @param[in]  Option            The first byte of the option.
  @param[in]  OptionLen         The length of the whole option.
  @param[in]  Pointer           Identifies the octet offset within
                                the invoking packet where the error was detected.


  @retval TRUE     The option is properly formatted.
  @retval FALSE    The option is malformated.

**/
BOOLEAN
Ip6IsOptionValid (
  IN IP6_SERVICE            *IpSb,
  IN NET_BUF                *Packet,
  IN UINT8                  *Option,
  IN UINT8                  OptionLen,
  IN UINT32                 Pointer
  )
{
  UINT8                      Offset;
  UINT8                      OptionType;

  Offset = 0;

  while (Offset < OptionLen) {
    OptionType = *(Option + Offset);

    switch (OptionType) {
    case Ip6OptionPad1:
      //
      // It is a Pad1 option
      //
      Offset++;
      break;
    case Ip6OptionPadN:
      //
      // It is a PadN option
      //
      Offset = (UINT8) (Offset + *(Option + Offset + 1) + 2);
      break;
    case Ip6OptionRouterAlert:
      //
      // It is a Router Alert Option
      //
      Offset += 4;
      break;
    default:
      //
      // The highest-order two bits specify the action must be taken if
      // the processing IPv6 node does not recognize the option type.
      //
      switch (OptionType & Ip6OptionMask) {
      case Ip6OptionSkip:
        Offset = (UINT8) (Offset + *(Option + Offset + 1));
        break;
      case Ip6OptionDiscard:
        return FALSE;
      case Ip6OptionParameterProblem:
        Pointer = Pointer + Offset + sizeof (EFI_IP6_HEADER);
        Ip6SendIcmpError (
          IpSb,
          Packet,
          NULL,
          &Packet->Ip.Ip6->SourceAddress,
          ICMP_V6_PARAMETER_PROBLEM,
          2,
          &Pointer
          );
        return FALSE;
      case Ip6OptionMask:
        if (!IP6_IS_MULTICAST (&Packet->Ip.Ip6->DestinationAddress)) {
          Pointer = Pointer + Offset + sizeof (EFI_IP6_HEADER);
          Ip6SendIcmpError (
            IpSb,
            Packet,
            NULL,
            &Packet->Ip.Ip6->SourceAddress,
            ICMP_V6_PARAMETER_PROBLEM,
            2,
            &Pointer
            );
        }

        return FALSE;
        break;
      }

      break;
    }

  }

  return TRUE;
}

/**
  Validate the IP6 option format for both the packets we received
  and that we will transmit. It supports the defined options in Neighbor
  Discovery messages.

  @param[in]  Option            The first byte of the option.
  @param[in]  OptionLen         The length of the whole option.

  @retval TRUE     The option is properly formatted.
  @retval FALSE    The option is malformated.

**/
BOOLEAN
Ip6IsNDOptionValid (
  IN UINT8                  *Option,
  IN UINT16                 OptionLen
  )
{
  UINT16                    Offset;
  UINT8                     OptionType;
  UINT16                    Length;

  Offset = 0;

  while (Offset < OptionLen) {
    OptionType = *(Option + Offset);
     Length    = (UINT16) (*(Option + Offset + 1) * 8);

    switch (OptionType) {
    case Ip6OptionPrefixInfo:
      if (Length != 32) {
        return FALSE;
      }

      break;

    case Ip6OptionMtu:
      if (Length != 8) {
        return FALSE;
      }

      break;

    default:
      //
      // Check the length of Ip6OptionEtherSource, Ip6OptionEtherTarget, and
      // Ip6OptionRedirected here. For unrecognized options, silently ignore
      // and continue processsing the message.
      //
      if (Length == 0) {
        return FALSE;
      }

      break;
    }

    Offset = (UINT16) (Offset + Length);
  }

  return TRUE;
}


/**
  Validate whether the NextHeader is a known valid protocol or one of the user configured
  protocols from the upper layer.

  @param[in]  IpSb          The IP6 service instance.
  @param[in]  NextHeader    The next header field.

  @retval TRUE              The NextHeader is a known valid protocol or user configured.
  @retval FALSE             The NextHeader is not a known valid protocol.

**/
BOOLEAN
Ip6IsValidProtocol (
  IN IP6_SERVICE            *IpSb,
  IN UINT8                  NextHeader
  )
{
  LIST_ENTRY                *Entry;
  IP6_PROTOCOL              *IpInstance;

  if (NextHeader == EFI_IP_PROTO_TCP ||
      NextHeader == EFI_IP_PROTO_UDP ||
      NextHeader == IP6_ICMP ||
      NextHeader == IP6_ESP
      ) {
    return TRUE;
  }

  if (IpSb == NULL) {
    return FALSE;
  }

  if (IpSb->Signature != IP6_SERVICE_SIGNATURE) {
    return FALSE;
  }

  NET_LIST_FOR_EACH (Entry, &IpSb->Children) {
    IpInstance = NET_LIST_USER_STRUCT_S (Entry, IP6_PROTOCOL, Link, IP6_PROTOCOL_SIGNATURE);
    if (IpInstance->State == IP6_STATE_CONFIGED) {
      if (IpInstance->ConfigData.DefaultProtocol == NextHeader) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/**
  Validate the IP6 extension header format for both the packets we received
  and that we will transmit. It will compute the ICMPv6 error message fields
  if the option is mal-formated.

  @param[in]  IpSb          The IP6 service instance. This is an optional parameter.
  @param[in]  Packet        The data of the packet. Ignored if NULL.
  @param[in]  NextHeader    The next header field in IPv6 basic header.
  @param[in]  ExtHdrs       The first byte of the option.
  @param[in]  ExtHdrsLen    The length of the whole option.
  @param[in]  Rcvd          The option is from the packet we received if TRUE,
                            otherwise, the option we want to transmit.
  @param[out] FormerHeader  The offset of NextHeader which points to Fragment
                            Header when we received, of the ExtHdrs.
                            Ignored if we transmit.
  @param[out] LastHeader    The pointer of NextHeader of the last extension
                            header processed by IP6.
  @param[out] RealExtsLen   The length of extension headers processed by IP6 layer.
                            This is an optional parameter that may be NULL.
  @param[out] UnFragmentLen The length of unfragmented length of extension headers.
                            This is an optional parameter that may be NULL.
  @param[out] Fragmented    Indicate whether the packet is fragmented.
                            This is an optional parameter that may be NULL.

  @retval     TRUE          The option is properly formated.
  @retval     FALSE         The option is malformated.

**/
BOOLEAN
Ip6IsExtsValid (
  IN IP6_SERVICE            *IpSb           OPTIONAL,
  IN NET_BUF                *Packet         OPTIONAL,
  IN UINT8                  *NextHeader,
  IN UINT8                  *ExtHdrs,
  IN UINT32                 ExtHdrsLen,
  IN BOOLEAN                Rcvd,
  OUT UINT32                *FormerHeader   OPTIONAL,
  OUT UINT8                 **LastHeader,
  OUT UINT32                *RealExtsLen    OPTIONAL,
  OUT UINT32                *UnFragmentLen  OPTIONAL,
  OUT BOOLEAN               *Fragmented     OPTIONAL
  )
{
  UINT32                     Pointer;
  UINT32                     Offset;
  UINT8                      *Option;
  UINT8                      OptionLen;
  BOOLEAN                    Flag;
  UINT8                      CountD;
  UINT8                      CountA;
  IP6_FRAGMENT_HEADER        *FragmentHead;
  UINT16                     FragmentOffset;
  IP6_ROUTING_HEADER         *RoutingHead;

  if (RealExtsLen != NULL) {
    *RealExtsLen = 0;
  }

  if (UnFragmentLen != NULL) {
    *UnFragmentLen = 0;
  }

  if (Fragmented != NULL) {
    *Fragmented = FALSE;
  }

  *LastHeader = NextHeader;

  if (ExtHdrs == NULL && ExtHdrsLen == 0) {
    return TRUE;
  }

  if ((ExtHdrs == NULL && ExtHdrsLen != 0) || (ExtHdrs != NULL && ExtHdrsLen == 0)) {
    return FALSE;
  }

  Pointer = 0;
  Offset  = 0;
  Flag    = FALSE;
  CountD  = 0;
  CountA  = 0;

  while (Offset <= ExtHdrsLen) {

    switch (*NextHeader) {
    case IP6_HOP_BY_HOP:
      if (Offset != 0) {
        if (!Rcvd) {
          return FALSE;
        }
        //
        // Hop-by-Hop Options header is restricted to appear immediately after an IPv6 header only.
        // If not, generate a ICMP parameter problem message with code value of 1.
        //
        if (Pointer == 0) {
          Pointer = sizeof (EFI_IP6_HEADER);
        } else {
          Pointer = Offset + sizeof (EFI_IP6_HEADER);
        }

        if ((IpSb != NULL) && (Packet != NULL) &&
            !IP6_IS_MULTICAST (&Packet->Ip.Ip6->DestinationAddress)) {
          Ip6SendIcmpError (
            IpSb,
            Packet,
            NULL,
            &Packet->Ip.Ip6->SourceAddress,
            ICMP_V6_PARAMETER_PROBLEM,
            1,
            &Pointer
            );
        }
        return FALSE;
      }

      Flag = TRUE;

    //
    // Fall through
    //
    case IP6_DESTINATION:
      if (*NextHeader == IP6_DESTINATION) {
        CountD++;
      }

      if (CountD > 2) {
        return FALSE;
      }

      NextHeader = ExtHdrs + Offset;
      Pointer    = Offset;

      Offset++;
      Option     = ExtHdrs + Offset;
      OptionLen  = (UINT8) ((*Option + 1) * 8 - 2);
      Option++;
      Offset++;

      if (IpSb != NULL && Packet != NULL && !Ip6IsOptionValid (IpSb, Packet, Option, OptionLen, Offset)) {
        return FALSE;
      }

      Offset = Offset + OptionLen;

      if (Flag) {
        if (UnFragmentLen != NULL) {
          *UnFragmentLen = Offset;
        }

        Flag = FALSE;
      }

      break;

    case IP6_ROUTING:
      NextHeader = ExtHdrs + Offset;
      RoutingHead = (IP6_ROUTING_HEADER *) NextHeader;

      //
      // Type 0 routing header is defined in RFC2460 and deprecated in RFC5095.
      // Thus all routing types are processed as unrecognized.
      //
      if (RoutingHead->SegmentsLeft == 0) {
        //
        // Ignore the routing header and proceed to process the next header.
        //
        Offset = Offset + (RoutingHead->HeaderLen + 1) * 8;

        if (UnFragmentLen != NULL) {
          *UnFragmentLen = Offset;
        }

      } else {
        //
        // Discard the packet and send an ICMP Parameter Problem, Code 0, message
        // to the packet's source address, pointing to the unrecognized routing
        // type.
        //
        Pointer = Offset + 2 + sizeof (EFI_IP6_HEADER);
        if ((IpSb != NULL) && (Packet != NULL) &&
            !IP6_IS_MULTICAST (&Packet->Ip.Ip6->DestinationAddress)) {
          Ip6SendIcmpError (
            IpSb,
            Packet,
            NULL,
            &Packet->Ip.Ip6->SourceAddress,
            ICMP_V6_PARAMETER_PROBLEM,
            0,
            &Pointer
            );
        }

        return FALSE;
      }

      break;

    case IP6_FRAGMENT:

      //
      // RFC2402, AH header should after fragment header.
      //
      if (CountA > 1) {
        return FALSE;
      }

      //
      // RFC2460, ICMP Parameter Problem message with code 0 should be sent
      // if the length of a fragment is not a multiple of 8 octects and the M
      // flag of that fragment is 1, pointing to the Payload length field of the
      // fragment packet.
      //
      if (IpSb != NULL && Packet != NULL && (ExtHdrsLen % 8) != 0) {
        //
        // Check whether it is the last fragment.
        //
        FragmentHead = (IP6_FRAGMENT_HEADER *) (ExtHdrs + Offset);
        if (FragmentHead == NULL) {
          return FALSE;
        }

        FragmentOffset = NTOHS (FragmentHead->FragmentOffset);

        if (((FragmentOffset & 0x1) == 0x1) &&
            !IP6_IS_MULTICAST (&Packet->Ip.Ip6->DestinationAddress)) {
          Pointer = sizeof (UINT32);
          Ip6SendIcmpError (
            IpSb,
            Packet,
            NULL,
            &Packet->Ip.Ip6->SourceAddress,
            ICMP_V6_PARAMETER_PROBLEM,
            0,
            &Pointer
            );
          return FALSE;
        }
      }

      if (Fragmented != NULL) {
        *Fragmented = TRUE;
      }

      if (Rcvd && FormerHeader != NULL) {
        *FormerHeader = (UINT32) (NextHeader - ExtHdrs);
      }

      NextHeader = ExtHdrs + Offset;
      Offset     = Offset + 8;
      break;

    case IP6_AH:
      if (++CountA > 1) {
        return FALSE;
      }

      Option     = ExtHdrs + Offset;
      NextHeader = Option;
      Option++;
      //
      // RFC2402, Payload length is specified in 32-bit words, minus "2".
      //
      OptionLen  = (UINT8) ((*Option + 2) * 4);
      Offset     = Offset + OptionLen;
      break;

    case IP6_NO_NEXT_HEADER:
      *LastHeader = NextHeader;
      return FALSE;
      break;

    default:
      if (Ip6IsValidProtocol (IpSb, *NextHeader)) {

        *LastHeader = NextHeader;

        if (RealExtsLen != NULL) {
          *RealExtsLen = Offset;
        }

        return TRUE;
      }

      //
      // The Next Header value is unrecognized by the node, discard the packet and
      // send an ICMP parameter problem message with code value of 1.
      //
      if (Offset == 0) {
        //
        // The Next Header directly follows IPv6 basic header.
        //
        Pointer = 6;
      } else {
        if (Pointer == 0) {
          Pointer = sizeof (EFI_IP6_HEADER);
        } else {
          Pointer = Offset + sizeof (EFI_IP6_HEADER);
        }
      }

      if ((IpSb != NULL) && (Packet != NULL) &&
          !IP6_IS_MULTICAST (&Packet->Ip.Ip6->DestinationAddress)) {
        Ip6SendIcmpError (
          IpSb,
          Packet,
          NULL,
          &Packet->Ip.Ip6->SourceAddress,
          ICMP_V6_PARAMETER_PROBLEM,
          1,
          &Pointer
          );
      }
      return FALSE;
    }
  }

  *LastHeader = NextHeader;

  if (RealExtsLen != NULL) {
    *RealExtsLen = Offset;
  }

  return TRUE;
}

/**
  Generate an IPv6 router alert option in network order and output it through Buffer.

  @param[out]     Buffer         Points to a buffer to record the generated option.
  @param[in, out] BufferLen      The length of Buffer, in bytes.
  @param[in]      NextHeader     The 8-bit selector indicates the type of header
                                 immediately following the Hop-by-Hop Options header.

  @retval EFI_BUFFER_TOO_SMALL   The Buffer is too small to contain the generated
                                 option. BufferLen is updated for the required size.

  @retval EFI_SUCCESS            The option is generated and filled in to Buffer.

**/
EFI_STATUS
Ip6FillHopByHop (
  OUT UINT8                  *Buffer,
  IN OUT UINTN               *BufferLen,
  IN UINT8                   NextHeader
  )
{
  UINT8                      BufferArray[8];

  if (*BufferLen < 8) {
    *BufferLen = 8;
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Form the Hop-By-Hop option in network order.
  // NextHeader (1 octet) + HdrExtLen (1 octet) + RouterAlertOption(4 octets) + PadN
  // The Hdr Ext Len is the length in 8-octet units, and does not including the first 8 octets.
  //
  ZeroMem (BufferArray, sizeof (BufferArray));
  BufferArray[0] = NextHeader;
  BufferArray[2] = 0x5;
  BufferArray[3] = 0x2;
  BufferArray[6] = 1;

  CopyMem (Buffer, BufferArray, sizeof (BufferArray));
  return EFI_SUCCESS;
}

/**
  Insert a Fragment Header to the Extension headers and output it in UpdatedExtHdrs.

  @param[in]  IpSb             The IP6 service instance to transmit the packet.
  @param[in]  NextHeader       The extension header type of first extension header.
  @param[in]  LastHeader       The extension header type of last extension header.
  @param[in]  ExtHdrs          The length of the original extension header.
  @param[in]  ExtHdrsLen       The length of the extension headers.
  @param[in]  FragmentOffset   The fragment offset of the data following the header.
  @param[out] UpdatedExtHdrs   The updated ExtHdrs with Fragment header inserted.
                               It's caller's responsiblity to free this buffer.

  @retval EFI_OUT_OF_RESOURCES Failed to finish the operation due to lake of
                               resource.
  @retval EFI_UNSUPPORTED      The extension header specified in ExtHdrs is not
                               supported currently.
  @retval EFI_SUCCESS          The operation performed successfully.

**/
EFI_STATUS
Ip6FillFragmentHeader (
  IN  IP6_SERVICE           *IpSb,
  IN  UINT8                 NextHeader,
  IN  UINT8                 LastHeader,
  IN  UINT8                 *ExtHdrs,
  IN  UINT32                ExtHdrsLen,
  IN  UINT16                FragmentOffset,
  OUT UINT8                 **UpdatedExtHdrs
  )
{
  UINT32                    Length;
  UINT8                     *Buffer;
  UINT32                    FormerHeader;
  UINT32                    Offset;
  UINT32                    Part1Len;
  UINT32                    HeaderLen;
  UINT8                     Current;
  IP6_FRAGMENT_HEADER       FragmentHead;

  if (UpdatedExtHdrs == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Length = ExtHdrsLen + sizeof (IP6_FRAGMENT_HEADER);
  Buffer = AllocatePool (Length);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Offset         = 0;
  Part1Len       = 0;
  FormerHeader   = 0;
  Current        = NextHeader;

  while ((ExtHdrs != NULL) && (Offset <= ExtHdrsLen)) {
    switch (NextHeader) {
    case IP6_ROUTING:
    case IP6_HOP_BY_HOP:
    case IP6_DESTINATION:
      Current      = NextHeader;
      NextHeader   = *(ExtHdrs + Offset);

      if ((Current == IP6_DESTINATION) && (NextHeader != IP6_ROUTING)) {
        //
        // Destination Options header should occur at most twice, once before
        // a Routing header and once before the upper-layer header. Here we
        // find the one before the upper-layer header. Insert the Fragment
        // Header before it.
        //
        CopyMem (Buffer, ExtHdrs, Part1Len);
        *(Buffer + FormerHeader) = IP6_FRAGMENT;
        //
        // Exit the loop.
        //
        Offset = ExtHdrsLen + 1;
        break;
      }


      FormerHeader = Offset;
      HeaderLen    = (*(ExtHdrs + Offset + 1) + 1) * 8;
      Part1Len     = Part1Len + HeaderLen;
      Offset       = Offset + HeaderLen;
      break;

    case IP6_FRAGMENT:
      Current    = NextHeader;

      if (Part1Len != 0) {
        CopyMem (Buffer, ExtHdrs, Part1Len);
      }

      *(Buffer + FormerHeader) = IP6_FRAGMENT;

      //
      // Exit the loop.
      //
      Offset = ExtHdrsLen + 1;
      break;

    case IP6_AH:
      Current    = NextHeader;
      NextHeader = *(ExtHdrs + Offset);
      //
      // RFC2402, Payload length is specified in 32-bit words, minus "2".
      //
      HeaderLen  = (*(ExtHdrs + Offset + 1) + 2) * 4;
      Part1Len   = Part1Len + HeaderLen;
      Offset     = Offset + HeaderLen;
      break;

    default:
      if (Ip6IsValidProtocol (IpSb, NextHeader)) {
        Current = NextHeader;
        CopyMem (Buffer, ExtHdrs, Part1Len);
        *(Buffer + FormerHeader) = IP6_FRAGMENT;
        //
        // Exit the loop.
        //
        Offset = ExtHdrsLen + 1;
        break;
      }

      FreePool (Buffer);
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Append the Fragment header. If the fragment offset indicates the fragment
  // is the first fragment.
  //
  if ((FragmentOffset & IP6_FRAGMENT_OFFSET_MASK) == 0) {
    FragmentHead.NextHeader = Current;
  } else {
    FragmentHead.NextHeader = LastHeader;
  }

  FragmentHead.Reserved       = 0;
  FragmentHead.FragmentOffset = HTONS (FragmentOffset);
  FragmentHead.Identification = mIp6Id;

  CopyMem (Buffer + Part1Len, &FragmentHead, sizeof (IP6_FRAGMENT_HEADER));

  if ((ExtHdrs != NULL) && (Part1Len < ExtHdrsLen)) {
    //
    // Append the part2 (fragmentable part) of Extension headers
    //
    CopyMem (
      Buffer + Part1Len + sizeof (IP6_FRAGMENT_HEADER),
      ExtHdrs + Part1Len,
      ExtHdrsLen - Part1Len
      );
  }

  *UpdatedExtHdrs = Buffer;

  return EFI_SUCCESS;
}


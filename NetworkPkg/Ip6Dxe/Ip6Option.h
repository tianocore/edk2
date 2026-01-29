/** @file
  Definition of IP6 option process routines.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_IP6_OPTION_H__
#define __EFI_IP6_OPTION_H__

#define IP6_FRAGMENT_OFFSET_MASK  (~0x3)

//
// For more information see RFC 8200, Section 4.3, 4.4, and 4.6
//
//  This example format is from section 4.6
//  This does not apply to fragment headers
//
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |  Next Header  |  Hdr Ext Len  |                               |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
//    |                                                               |
//    .                                                               .
//    .                  Header-Specific Data                         .
//    .                                                               .
//    |                                                               |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//      Next Header           8-bit selector.  Identifies the type of
//                            header immediately following the extension
//                            header.  Uses the same values as the IPv4
//                            Protocol field [IANA-PN].
//
//      Hdr Ext Len           8-bit unsigned integer.  Length of the
//                            Destination Options header in 8-octet units,
//                            not including the first 8 octets.

//
// These defines apply to the following:
//   1. Hop by Hop
//   2. Routing
//   3. Destination
//
typedef struct _IP6_EXT_HDR {
  ///
  /// The Next Header field identifies the type of header immediately
  ///
  UINT8    NextHeader;
  ///
  /// The Hdr Ext Len field specifies the length of the Hop-by-Hop Options
  ///
  UINT8    HdrExtLen;
  ///
  /// Header-Specific Data
  ///
} IP6_EXT_HDR;

STATIC_ASSERT (
  sizeof (IP6_EXT_HDR) == 2,
  "The combined size of Next Header and Len is two 8 bit fields"
  );

//
// IPv6 extension headers contain an 8-bit length field which describes the size of
// the header. However, the length field only includes the size of the extension
// header options, not the size of the first 8 bytes of the header. Therefore, in
// order to calculate the full size of the extension header, we add 1 (to account
// for the first 8 bytes omitted by the length field reporting) and then multiply
// by 8 (since the size is represented in 8-byte units).
//
// a is the length field of the extension header (UINT8)
// The result may be up to 2046 octets (UINT16)
//
#define IP6_HDR_EXT_LEN(a)  (((UINT16)((UINT8)(a)) + 1) * 8)

// This is the maxmimum length permissible by a extension header
// Length is UINT8 of 8 octets not including the first 8 octets
#define IP6_MAX_EXT_DATA_LENGTH  (IP6_HDR_EXT_LEN (MAX_UINT8) - sizeof(IP6_EXT_HDR))
STATIC_ASSERT (
  IP6_MAX_EXT_DATA_LENGTH == 2046,
  "Maximum data length is ((MAX_UINT8 + 1) * 8) - 2"
  );

typedef struct _IP6_FRAGMENT_HEADER {
  UINT8     NextHeader;
  UINT8     Reserved;
  UINT16    FragmentOffset;
  UINT32    Identification;
} IP6_FRAGMENT_HEADER;

typedef struct _IP6_ROUTING_HEADER {
  UINT8    NextHeader;
  UINT8    HeaderLen;
  UINT8    RoutingType;
  UINT8    SegmentsLeft;
} IP6_ROUTING_HEADER;

typedef enum {
  Ip6OptionPad1             = 0,
  Ip6OptionPadN             = 1,
  Ip6OptionRouterAlert      = 5,
  Ip6OptionSkip             = 0,
  Ip6OptionDiscard          = 0x40,
  Ip6OptionParameterProblem = 0x80,
  Ip6OptionMask             = 0xc0,

  Ip6OptionEtherSource = 1,
  Ip6OptionEtherTarget = 2,
  Ip6OptionPrefixInfo  = 3,
  Ip6OptionRedirected  = 4,
  Ip6OptionMtu         = 5
} IP6_OPTION_TYPE;

/**
  Validate the IP6 extension header format for both the packets we received
  and that we will transmit. It will compute the ICMPv6 error message fields
  if the option is mal-formatted.

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

  @retval     TRUE          The option is properly formatted.
  @retval     FALSE         The option is malformatted.

**/
BOOLEAN
Ip6IsExtsValid (
  IN IP6_SERVICE  *IpSb           OPTIONAL,
  IN NET_BUF      *Packet         OPTIONAL,
  IN UINT8        *NextHeader,
  IN UINT8        *ExtHdrs,
  IN UINT32       ExtHdrsLen,
  IN BOOLEAN      Rcvd,
  OUT UINT32      *FormerHeader   OPTIONAL,
  OUT UINT8       **LastHeader,
  OUT UINT32      *RealExtsLen    OPTIONAL,
  OUT UINT32      *UnFragmentLen  OPTIONAL,
  OUT BOOLEAN     *Fragmented     OPTIONAL
  );

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
  OUT UINT8     *Buffer,
  IN OUT UINTN  *BufferLen,
  IN UINT8      NextHeader
  );

/**
  Insert a Fragment Header to the Extension headers and output it in UpdatedExtHdrs.

  @param[in]  IpSb             The IP6 service instance to transmit the packet.
  @param[in]  NextHeader       The extension header type of first extension header.
  @param[in]  LastHeader       The extension header type of last extension header.
  @param[in]  ExtHdrs          The length of the original extension header.
  @param[in]  ExtHdrsLen       The length of the extension headers.
  @param[in]  FragmentOffset   The fragment offset of the data following the header.
  @param[out] UpdatedExtHdrs   The updated ExtHdrs with Fragment header inserted.
                               It's caller's responsibility to free this buffer.

  @retval EFI_OUT_OF_RESOURCES Failed to finish the operation due to lake of
                               resource.
  @retval EFI_UNSUPPORTED      The extension header specified in ExtHdrs is not
                               supported currently.
  @retval EFI_SUCCESS          The operation performed successfully.

**/
EFI_STATUS
Ip6FillFragmentHeader (
  IN  IP6_SERVICE  *IpSb,
  IN  UINT8        NextHeader,
  IN  UINT8        LastHeader,
  IN  UINT8        *ExtHdrs,
  IN  UINT32       ExtHdrsLen,
  IN  UINT16       FragmentOffset,
  OUT UINT8        **UpdatedExtHdrs
  );

/**
  Copy the extension headers from the original to buffer. A Fragment header is
  appended to the end.

  @param[in]       NextHeader       The 8-bit selector indicates the type of
                                    the fragment header's next header.
  @param[in]       ExtHdrs          The length of the original extension header.
  @param[in]       LastHeader       The pointer of next header of last extension header.
  @param[in]       FragmentOffset   The fragment offset of the data following the header.
  @param[in]       UnFragmentHdrLen The length of unfragmented length of extension headers.
  @param[in, out]  Buf              The buffer to copy options to.
  @param[in, out]  BufLen           The length of the buffer.

  @retval EFI_SUCCESS           The options are copied over.
  @retval EFI_BUFFER_TOO_SMALL  The buffer caller provided is too small.

**/
EFI_STATUS
Ip6CopyExts (
  IN UINT8       NextHeader,
  IN UINT8       *ExtHdrs,
  IN UINT8       *LastHeader,
  IN UINT16      FragmentOffset,
  IN UINT32      UnFragmentHdrLen,
  IN OUT UINT8   *Buf,
  IN OUT UINT32  *BufLen
  );

/**
  Validate the IP6 option format for both the packets we received
  and that we will transmit. It supports the defined options in Neighbor
  Discovery messages.

  @param[in]  Option            The first byte of the option.
  @param[in]  OptionLen         The length of the whole option.

  @retval TRUE     The option is properly formatted.
  @retval FALSE    The option is malformatted.

**/
BOOLEAN
Ip6IsNDOptionValid (
  IN UINT8   *Option,
  IN UINT16  OptionLen
  );

#endif

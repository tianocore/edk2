/** @file
  To validate, parse and process the DHCP options.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_DHCP4_OPTION_H__
#define __EFI_DHCP4_OPTION_H__

///
/// DHCP option tags (types)
///

#define DHCP_OPTION_MAGIC         0x63538263 // Network byte order
#define DHCP_MAX_OPTIONS          256


//
// DHCP option types, this is used to validate the DHCP options.
//
#define DHCP_OPTION_SWITCH        1
#define DHCP_OPTION_INT8          2
#define DHCP_OPTION_INT16         3
#define DHCP_OPTION_INT32         4
#define DHCP_OPTION_IP            5
#define DHCP_OPTION_IPPAIR        6

//
// Value of DHCP overload option
//
#define DHCP_OVERLOAD_FILENAME    1
#define DHCP_OVERLOAD_SVRNAME     2
#define DHCP_OVERLOAD_BOTH        3

///
/// The DHCP option structure. This structure extends the EFI_DHCP_OPTION
/// structure to support options longer than 255 bytes, such as classless route.
///
typedef struct {
  UINT8                     Tag;
  UINT16                    Len;
  UINT8                     *Data;
} DHCP_OPTION;

///
/// Structures used to parse the DHCP options with RFC3396 support.
///
typedef struct {
  UINT8                     Index;
  UINT16                    Offset;
} DHCP_OPTION_COUNT;

typedef struct {
  DHCP_OPTION_COUNT         *OpCount;
  DHCP_OPTION               *Options;
  UINT8                     *Buf;
} DHCP_OPTION_CONTEXT;

///
/// The options that matters to DHCP driver itself. The user of
/// DHCP clients may be interested in other options, such as
/// classless route, who can parse the DHCP offer to get them.
///
typedef struct {
  IP4_ADDR                  NetMask;  // DHCP4_TAG_NETMASK
  IP4_ADDR                  Router;   // DHCP4_TAG_ROUTER, only the first router is used

  //
  // DHCP specific options
  //
  UINT8                     DhcpType; // DHCP4_TAG_MSG_TYPE
  UINT8                     Overload; // DHCP4_TAG_OVERLOAD
  IP4_ADDR                  ServerId; // DHCP4_TAG_SERVER_ID
  UINT32                    Lease;    // DHCP4_TAG_LEASE
  UINT32                    T1;       // DHCP4_TAG_T1
  UINT32                    T2;       // DHCP4_TAG_T2
} DHCP_PARAMETER;

///
/// Structure used to describe and validate the format of DHCP options.
/// Type is the options' data type, such as DHCP_OPTION_INT8. MinOccur
/// is the minium occurance of this data type. MaxOccur is defined
/// similarly. If MaxOccur is -1, it means that there is no limit on the
/// maximum occurance. Alert tells whether DHCP client should further
/// inspect the option to parse DHCP_PARAMETER.
///
typedef struct {
  UINT8                     Tag;
  INTN                      Type;
  INTN                      MinOccur;
  INTN                      MaxOccur;
  BOOLEAN                   Alert;
} DHCP_OPTION_FORMAT;

typedef
EFI_STATUS
(*DHCP_CHECK_OPTION) (
  IN UINT8                  Tag,
  IN UINT8                  Len,
  IN UINT8                  *Data,
  IN VOID                   *Context
  );

/**
  Iterate through a DHCP message to visit each option. First inspect
  all the options in the OPTION field. Then if overloaded, inspect
  the options in FILENAME and SERVERNAME fields. One option may be
  encoded in several places. See RFC 3396 Encoding Long Options in DHCP

  @param[in]  Packet                 The DHCP packet to check the options for
  @param[in]  Check                  The callback function to be called for each option
                                     found
  @param[in]  Context                The opaque parameter for Check

  @retval EFI_SUCCESS            The DHCP packet's options are well formated
  @retval EFI_INVALID_PARAMETER  The DHCP packet's options are not well formated

**/
EFI_STATUS
DhcpIterateOptions (
  IN  EFI_DHCP4_PACKET      *Packet,
  IN  DHCP_CHECK_OPTION     Check         OPTIONAL,
  IN  VOID                  *Context
  );

/**
  Validate the packet's options. If necessary, allocate
  and fill in the interested parameters.

  @param[in]  Packet                 The packet to validate the options
  @param[out] Para                   The variable to save the DHCP parameters.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory to validate the packet.
  @retval EFI_INVALID_PARAMETER  The options are mal-formated
  @retval EFI_SUCCESS            The options are parsed into OptionPoint

**/
EFI_STATUS
DhcpValidateOptions (
  IN  EFI_DHCP4_PACKET      *Packet,
  OUT DHCP_PARAMETER        **Para       OPTIONAL
  );

/**
  Parse the options of a DHCP packet. It supports RFC 3396: Encoding
  Long Options in DHCP. That is, it will combine all the option value
  of all the occurances of each option.
  A little bit of implemenation:
  It adopts the "Key indexed counting" algorithm. First, it allocates
  an array of 256 DHCP_OPTION_COUNTs because DHCP option tag is encoded
  as a UINT8. It then iterates the DHCP packet to get data length of
  each option by calling DhcpIterOptions with DhcpGetOptionLen. Now, it
  knows the number of present options and their length. It allocates a
  array of DHCP_OPTION and a continuous buffer after the array to put
  all the options' data. Each option's data is pointed to by the Data
  field in DHCP_OPTION structure. At last, it call DhcpIterateOptions
  with DhcpFillOption to fill each option's data to its position in the
  buffer.

  @param[in]  Packet                 The DHCP packet to parse the options
  @param[out] Count                  The number of valid dhcp options present in the
                                     packet
  @param[out] OptionPoint            The array that contains the DHCP options. Caller
                                     should free it.

  @retval EFI_NOT_FOUND          Cannot find any option.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory to parse the packet.
  @retval EFI_INVALID_PARAMETER  The options are mal-formated
  @retval EFI_SUCCESS            The options are parsed into OptionPoint

**/
EFI_STATUS
DhcpParseOption (
  IN  EFI_DHCP4_PACKET      *Packet,
  OUT INTN                  *Count,
  OUT DHCP_OPTION           **OptionPoint
  );

/**
  Append an option to the memory, if the option is longer than
  255 bytes, splits it into several options.

  @param[out] Buf                    The buffer to append the option to
  @param[in]  Tag                    The option's tag
  @param[in]  DataLen                The length of the option's data
  @param[in]  Data                   The option's data

  @return The position to append the next option

**/
UINT8 *
DhcpAppendOption (
  OUT UINT8                  *Buf,
  IN  UINT8                  Tag,
  IN  UINT16                 DataLen,
  IN  UINT8                  *Data
  );

/**
  Build a new DHCP packet from a seed packet. Options may be deleted or
  appended. The caller should free the NewPacket when finished using it.

  @param[in]  SeedPacket             The seed packet to start with
  @param[in]  DeleteCount            The number of options to delete
  @param[in]  DeleteList             The options to delete from the packet
  @param[in]  AppendCount            The number of options to append
  @param[in]  AppendList             The options to append to the packet
  @param[out] NewPacket              The new packet, allocated and built by this
                                     function.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory
  @retval EFI_INVALID_PARAMETER  The options in SeekPacket are mal-formated
  @retval EFI_SUCCESS            The packet is build.

**/
EFI_STATUS
DhcpBuild (
  IN  EFI_DHCP4_PACKET        *SeedPacket,
  IN  UINT32                  DeleteCount,
  IN  UINT8                   *DeleteList     OPTIONAL,
  IN  UINT32                  AppendCount,
  IN  EFI_DHCP4_PACKET_OPTION *AppendList[]   OPTIONAL,
  OUT EFI_DHCP4_PACKET        **NewPacket
  );

#endif

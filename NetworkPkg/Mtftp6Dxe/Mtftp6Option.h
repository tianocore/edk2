/** @file
  Mtftp6 option parse functions declaration.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_MTFTP6_OPTION_H__
#define __EFI_MTFTP6_OPTION_H__

#include <Uefi.h>

#include <Protocol/ServiceBinding.h>

#include <Library/NetLib.h>
#include <Library/UdpIoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#define MTFTP6_SUPPORTED_OPTIONS_NUM  4
#define MTFTP6_OPCODE_LEN             2
#define MTFTP6_ERRCODE_LEN            2
#define MTFTP6_BLKNO_LEN              2
#define MTFTP6_DATA_HEAD_LEN          4

//
// The bit map definition for Mtftp6 extension options.
//
#define MTFTP6_OPT_BLKSIZE_BIT        0x01
#define MTFTP6_OPT_TIMEOUT_BIT        0x02
#define MTFTP6_OPT_TSIZE_BIT          0x04
#define MTFTP6_OPT_MCAST_BIT          0x08

extern CHAR8 *mMtftp6SupportedOptions[MTFTP6_SUPPORTED_OPTIONS_NUM];

typedef struct {
  UINT16                    BlkSize;
  UINT8                     Timeout;
  UINT32                    Tsize;
  EFI_IPv6_ADDRESS          McastIp;
  UINT16                    McastPort;
  BOOLEAN                   IsMaster;
  UINT32                    BitMap;
} MTFTP6_EXT_OPTION_INFO;

/**
  Parse the Ascii string of multi-cast option.

  @param[in]  Str           The pointer to the Ascii string of multi-cast option.
  @param[in]  ExtInfo       The pointer to the option information to be filled.

  @retval EFI_SUCCESS            Parse the multicast option successfully.
  @retval EFI_INVALID_PARAMETER  The string is malformatted.

**/
EFI_STATUS
Mtftp6ParseMcastOption (
  IN UINT8                  *Str,
  IN MTFTP6_EXT_OPTION_INFO *ExtInfo
  );


/**
  Parse the MTFTP6 extesion options.

  @param[in]  Options       The pointer to the extension options list.
  @param[in]  Count         The num of the extension options.
  @param[in]  IsRequest     If FALSE, the extension options is included
                            by a request packet.
  @param[in]  ExtInfo       The pointer to the option information to be filled.

  @retval EFI_SUCCESS            Parse the multi-cast option successfully.
  @retval EFI_INVALID_PARAMETER  An option is malformatted.
  @retval EFI_UNSUPPORTED        An option is not supported.

**/
EFI_STATUS
Mtftp6ParseExtensionOption (
  IN EFI_MTFTP6_OPTION        *Options,
  IN UINT32                   Count,
  IN BOOLEAN                  IsRequest,
  IN MTFTP6_EXT_OPTION_INFO   *ExtInfo
  );


/**
  Go through the packet to fill the options array with the start
  addresses of each MTFTP option name/value pair.

  @param[in]      Packet                 The packet to be checked.
  @param[in]      PacketLen              The length of the packet.
  @param[in, out] Count                  The num of the Options on input.
                                         The actual one on output.
  @param[in]      Options                The option array to be filled
                                         it's optional.

  @retval EFI_SUCCESS            The packet has been parsed successfully.
  @retval EFI_INVALID_PARAMETER  The packet is malformatted
  @retval EFI_BUFFER_TOO_SMALL   The Options array is too small
  @retval EFI_PROTOCOL_ERROR     An unexpected MTFTPv6 packet was received.

**/
EFI_STATUS
Mtftp6ParsePacketOption (
  IN     EFI_MTFTP6_PACKET     *Packet,
  IN     UINT32                PacketLen,
  IN OUT UINT32                *Count,
  IN     EFI_MTFTP6_OPTION     *Options          OPTIONAL
  );


/**
  Go through the packet, generate option list array and fill it
  by the result of parse options.

  @param[in]      Packet                 The packet to be checked.
  @param[in]      PacketLen              The length of the packet.
  @param[in, out] OptionCount            The num of the Options on input.
                                         The actual one on output.
  @param[out]     OptionList             The option list array to be generated
                                         and filled. It is optional.

  @retval EFI_SUCCESS            The packet has been parsed successfully.
  @retval EFI_INVALID_PARAMETER  The packet is malformatted.
  @retval EFI_PROTOCOL_ERROR     An option is malformatted.
  @retval EFI_NOT_FOUND          The packet has no options.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory for the array.
  @retval EFI_BUFFER_TOO_SMALL   The size of option list array is too small.

**/
EFI_STATUS
Mtftp6ParseStart (
  IN     EFI_MTFTP6_PACKET      *Packet,
  IN     UINT32                 PacketLen,
  IN OUT UINT32                 *OptionCount,
     OUT EFI_MTFTP6_OPTION      **OptionList          OPTIONAL
  );

#endif

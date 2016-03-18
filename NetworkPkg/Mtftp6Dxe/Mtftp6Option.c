/** @file
  Mtftp6 option parse functions implementation.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Mtftp6Impl.h"

CHAR8 *mMtftp6SupportedOptions[MTFTP6_SUPPORTED_OPTIONS_NUM] = {
  "blksize",
  "timeout",
  "tsize",
  "multicast"
};


/**
  Parse the NULL terminated ASCII string of multicast option.

  @param[in]  Str           The pointer to the Ascii string of multicast option.
  @param[in]  ExtInfo       The pointer to the option information to be filled.

  @retval EFI_SUCCESS            Parse the multicast option successfully.
  @retval EFI_INVALID_PARAMETER  The string is malformatted.
  @retval EFI_OUT_OF_RESOURCES   Failed to perform the operation due to lack of
                                 resources.

**/
EFI_STATUS
Mtftp6ParseMcastOption (
  IN UINT8                  *Str,
  IN MTFTP6_EXT_OPTION_INFO *ExtInfo
  )
{
  EFI_STATUS                Status;
  UINT32                    Num;
  CHAR8                     *Ip6Str;
  CHAR8                     *TempStr;

  //
  // The multicast option is formated like "addr,port,mc"
  // The server can also omit the ip and port, use ",,1"
  //
  if (*Str == ',') {

    ZeroMem (&ExtInfo->McastIp, sizeof (EFI_IPv6_ADDRESS));
  } else {

    Ip6Str = (CHAR8 *) AllocateCopyPool (AsciiStrSize ((CHAR8 *) Str), Str);
    if (Ip6Str == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // The IPv6 address locates before comma in the input Str.
    //
    TempStr = Ip6Str;
    while ((*TempStr != '\0') && (*TempStr != ',')) {
      TempStr++;
    }

    *TempStr = '\0';

    Status = NetLibAsciiStrToIp6 (Ip6Str, &ExtInfo->McastIp);
    FreePool (Ip6Str);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    while ((*Str != '\0') && (*Str != ',')) {
      Str++;
    }
  }

  if (*Str != ',') {
    return EFI_INVALID_PARAMETER;
  }

  Str++;

  //
  // Convert the port setting. the server can send us a port number or
  // empty string. such as the port in ",,1"
  //
  if (*Str == ',') {

    ExtInfo->McastPort = 0;
  } else {

    Num = (UINT32) AsciiStrDecimalToUintn ((CHAR8 *) Str);

    if (Num > 65535) {
      return EFI_INVALID_PARAMETER;
    }

    ExtInfo->McastPort = (UINT16) Num;

    while (NET_IS_DIGIT (*Str)) {
      Str++;
    }
  }

  if (*Str != ',') {
    return EFI_INVALID_PARAMETER;
  }

  Str++;

  //
  // Check the master/slave setting, 1 for master, 0 for slave.
  //
  Num = (UINT32) AsciiStrDecimalToUintn ((CHAR8 *) Str);

  if (Num != 0 && Num != 1) {
    return EFI_INVALID_PARAMETER;
  }

  ExtInfo->IsMaster = (BOOLEAN) (Num == 1);

  while (NET_IS_DIGIT (*Str)) {
    Str++;
  }

  if (*Str != '\0') {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}


/**
  Parse the MTFTP6 extesion options.

  @param[in]  Options       The pointer to the extension options list.
  @param[in]  Count         The num of the extension options.
  @param[in]  IsRequest     If FALSE, the extension options is included
                            by a request packet.
  @param[in]  ExtInfo       The pointer to the option information to be filled.

  @retval EFI_SUCCESS            Parse the multicast option successfully.
  @retval EFI_INVALID_PARAMETER  There is one option is malformatted at least.
  @retval EFI_UNSUPPORTED        There is one option is not supported at least.

**/
EFI_STATUS
Mtftp6ParseExtensionOption (
  IN EFI_MTFTP6_OPTION        *Options,
  IN UINT32                   Count,
  IN BOOLEAN                  IsRequest,
  IN MTFTP6_EXT_OPTION_INFO   *ExtInfo
  )
{
  EFI_STATUS                  Status;
  EFI_MTFTP6_OPTION           *Opt;
  UINT32                      Index;
  UINT32                      Value;

  ExtInfo->BitMap = 0;

  for (Index = 0; Index < Count; Index++) {

    Opt = Options + Index;

    if (Opt->OptionStr == NULL || Opt->ValueStr == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    if (AsciiStriCmp ((CHAR8 *) Opt->OptionStr, "blksize") == 0) {
      //
      // block size option, valid value is between [8, 65464]
      //
      Value = (UINT32) AsciiStrDecimalToUintn ((CHAR8 *) Opt->ValueStr);

      if ((Value < 8) || (Value > 65464)) {
        return EFI_INVALID_PARAMETER;
      }

      ExtInfo->BlkSize = (UINT16) Value;
      ExtInfo->BitMap |= MTFTP6_OPT_BLKSIZE_BIT;

    } else if (AsciiStriCmp ((CHAR8 *) Opt->OptionStr, "timeout") == 0) {
      //
      // timeout option, valid value is between [1, 255]
      //
      Value = (UINT32) AsciiStrDecimalToUintn ((CHAR8 *) Opt->ValueStr);

      if (Value < 1 || Value > 255) {
        return EFI_INVALID_PARAMETER;
      }

      ExtInfo->Timeout = (UINT8) Value;
      ExtInfo->BitMap |= MTFTP6_OPT_TIMEOUT_BIT;

    } else if (AsciiStriCmp ((CHAR8 *) Opt->OptionStr, "tsize") == 0) {
      //
      // tsize option, the biggest transfer supported is 4GB with block size option
      //
      ExtInfo->Tsize   = (UINT32) AsciiStrDecimalToUintn ((CHAR8 *) Opt->ValueStr);
      ExtInfo->BitMap |= MTFTP6_OPT_TSIZE_BIT;

    } else if (AsciiStriCmp ((CHAR8 *) Opt->OptionStr, "multicast") == 0) {
      //
      // Multicast option, if it is a request, the value must be a zero string,
      // otherwise, it must be like "addr,port,mc" string, mc indicates master.
      //
      if (!IsRequest) {

        Status = Mtftp6ParseMcastOption (Opt->ValueStr, ExtInfo);

        if (EFI_ERROR (Status)) {
          return Status;
        }
      } else if (*(Opt->ValueStr) != '\0') {

        return EFI_INVALID_PARAMETER;
      }

      ExtInfo->BitMap |= MTFTP6_OPT_MCAST_BIT;

    } else if (IsRequest) {
      //
      // If it's a request, unsupported; else if it's a reply, ignore.
      //
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}


/**
  Go through the packet to fill the options array with the start
  addresses of each MTFTP option name/value pair.

  @param[in]      Packet                 The packet to be checked.
  @param[in]      PacketLen              The length of the packet.
  @param[in, out] Count                  The num of the Options on input.
                                         The actual one on output.
  @param[in]      Options                The option array to be filled.
                                         It is optional.

  @retval EFI_SUCCESS            The packet has been parsed successfully.
  @retval EFI_INVALID_PARAMETER  The packet is malformatted.
  @retval EFI_BUFFER_TOO_SMALL   The Options array is too small.
  @retval EFI_PROTOCOL_ERROR     An unexpected MTFTPv6 packet was received.

**/
EFI_STATUS
Mtftp6ParsePacketOption (
  IN     EFI_MTFTP6_PACKET     *Packet,
  IN     UINT32                PacketLen,
  IN OUT UINT32                *Count,
  IN     EFI_MTFTP6_OPTION     *Options          OPTIONAL
  )
{
  UINT8                        *Cur;
  UINT8                        *Last;
  UINT8                        Num;
  UINT8                        *Name;
  UINT8                        *Value;

  Num   = 0;
  Cur   = (UINT8 *) Packet + MTFTP6_OPCODE_LEN;
  Last  = (UINT8 *) Packet + PacketLen - 1;

  //
  // process option name and value pairs.
  // The last byte is always zero.
  //
  while (Cur < Last) {
    Name = Cur;

    while (*Cur != 0) {
      Cur++;
    }

    if (Cur == Last) {
      return EFI_PROTOCOL_ERROR;
    }

    Value = ++Cur;

    while (*Cur != 0) {
      Cur++;
    }

    Num++;

    if (Options != NULL && Num <= *Count) {
      Options[Num - 1].OptionStr  = Name;
      Options[Num - 1].ValueStr   = Value;
    }

    Cur++;
  }

  //
  // Return buffer too small if the buffer passed-in isn't enough.
  //
  if (*Count < Num || Options == NULL) {
    *Count = Num;
    return EFI_BUFFER_TOO_SMALL;
  }

  *Count = Num;
  return EFI_SUCCESS;
}


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
  @retval EFI_PROTOCOL_ERROR     There is one option is malformatted at least.
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
  )
{
  EFI_STATUS                    Status;

  if (PacketLen == 0 || Packet == NULL || OptionCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *OptionCount = 0;

  if (OptionList != NULL) {
    *OptionList = NULL;
  }

  if (NTOHS (Packet->OpCode) != EFI_MTFTP6_OPCODE_OACK) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The last byte must be zero to terminate the options.
  //
  if (*((UINT8 *) Packet + PacketLen - 1) != 0) {
    return EFI_PROTOCOL_ERROR;
  }

  //
  // Parse packet with NULL buffer for the first time to get the number
  // of options in the packet.
  //
  Status = Mtftp6ParsePacketOption (Packet, PacketLen, OptionCount, NULL);

  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  //
  // Return not found if there is no option parsed.
  //
  if (*OptionCount == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Only need parse out the number of options.
  //
  if (OptionList == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Allocate the buffer according to the option number parsed before.
  //
  *OptionList = AllocateZeroPool (*OptionCount * sizeof (EFI_MTFTP6_OPTION));

  if (*OptionList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Parse packet with allocated buffer for the second time to fill the pointer array
  // of the options in the packet.
  //
  Status = Mtftp6ParsePacketOption (Packet, PacketLen, OptionCount, *OptionList);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

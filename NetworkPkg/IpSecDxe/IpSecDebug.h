/** @file
  The definition of functions and MACROs used for IPsec debug information printting.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _EFI_IPSEC_DEBUG_H_
#define _EFI_IPSEC_DEBUG_H_

#include "IkeCommon.h"
#include "IkePacket.h"

#define IPSEC_DUMP_ADDRESS(Level, Ip, Version)           IpSecDumpAddress (Level, Ip, Version)
#define IKEV1_DUMP_STATE(Previous, Current)              IkeDumpState (Previous, Current, 1)
#define IKEV2_DUMP_STATE(Previous, Current)              IkeDumpState (Previous, Current, 2)
#define IPSEC_DUMP_PACKET(Packet, Direction, IpVersion)  IpSecDumpPacket (Packet, Direction, IpVersion)
#define IPSEC_DUMP_PAYLOAD(IkePayload)                   IpSecDumpPayload (IkePayload, 1)
#define IKEV2_DUMP_PAYLOAD(IkePayload)                   IpSecDumpPayload (IkePayload, 2)
#define IPSEC_DUMP_BUF(Title, Data, DataSize)            IpSecDumpBuf (Title, Data, DataSize)

#define IPSEC_DEBUG_BYTE_PER_LINE                       8
#define IKE_STATE_NUM                                   7



/**
  Print the IP address.

  @param[in]  Level     Debug print error level. Pass to DEBUG().
  @param[in]  Ip        Point to specified IP address.
  @param[in]  IpVersion The IP Version.

**/
VOID
IpSecDumpAddress (
  IN UINTN               Level,
  IN EFI_IP_ADDRESS      *Ip,
  IN UINT8               IpVersion
  );

/**
  Print IKE Current states.

  @param[in]  Previous    The Previous state of IKE.
  @param[in]  Current     The current state of IKE.
  @param[in]  IkeVersion  The version of IKE.

**/
VOID
IkeDumpState (
  IN UINT32              Previous,
  IN UINT32              Current, 
  IN UINT8               IkeVersion
  );

/**
  Print the IKE Packet.

  @param[in]  Packet      Point to IKE packet to be printed.
  @param[in]  Direction   Point to the IKE packet is inbound or outbound.
  @param[in]  IpVersion   Specified IP Version.

**/
VOID
IpSecDumpPacket (
  IN IKE_PACKET            *Packet,
  IN EFI_IPSEC_TRAFFIC_DIR Direction,
  IN UINT8                 IpVersion
  );

/**
  Print the IKE Paylolad.

  @param[in]  IkePayload  Point to payload to be printed.
  @param[in]  IkeVersion  The specified version of IKE.
 
**/
VOID
IpSecDumpPayload (
  IN IKE_PAYLOAD           *IkePayload,
  IN UINT8                 IkeVersion
  );

/**
  Print the buffer in form of Hex.

  @param[in]  Title       The strings to be printed before the data of the buffer.
  @param[in]  Data        Point to buffer to be printed.
  @param[in]  DataSize    The size of the buffer to be printed.

**/
VOID
IpSecDumpBuf (
  IN CHAR8                 *Title,
  IN UINT8                 *Data,
  IN UINTN                 DataSize
  );

#endif

/** @file
  IKE Packet related definitions and function declarations.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
  
**/

#ifndef _IKE_V1_PACKET_H_
#define _IKE_V1_PACKET_H_

#include "Ike.h"

#define IKE_PACKET_REF(p) ((p)->RefCount++)

/**
  Allocate a buffer for the IKE_PACKET and intitalize its Header and payloadlist.

  @return The pointer of the IKE_PACKET.

**/
IKE_PACKET *
IkePacketAlloc (
  VOID
  );


/**
  Free the IkePacket by the specified IKE_PACKET pointer.

  @param[in]  IkePacket  The pointer of the IKE_PACKET to be freed.

**/
VOID
IkePacketFree (
  IN IKE_PACKET *IkePacket
  );


/**
  Copy the NetBuf into a IKE_PACKET sturcture.
  
  Create a IKE_PACKET and fill the received IKE header into the header of IKE_PACKET 
  and copy the recieved packet without IKE HEADER to the PayloadBuf of IKE_PACKET.

  @param[in]  Netbuf      The pointer of the Netbuf which contains the whole received 
                          IKE packet.

  @return The pointer of the IKE_PACKET which contains the received packet.

**/
IKE_PACKET *
IkePacketFromNetbuf (
  IN NET_BUF *Netbuf
  );

/**
  Convert the format from IKE_PACKET to NetBuf.

  @param[in]  SessionCommon  Pointer of related IKE_COMMON_SESSION
  @param[in]  IkePacket      Pointer of IKE_PACKET to be copy to NetBuf
  @param[in]  IkeType        The IKE type to pointer the packet is for which IKE 
                             phase. Now it supports IKE_SA_TYPE, IKE_CHILDSA_TYPE, 
                             IKE_INFO_TYPE.

  @return A pointer of Netbuff which contains the contents of the IKE_PACKE in network order.
**/
NET_BUF *
IkeNetbufFromPacket (
  IN UINT8               *SessionCommon,
  IN IKE_PACKET          *IkePacket,
  IN UINTN               IkeType
  );

#endif

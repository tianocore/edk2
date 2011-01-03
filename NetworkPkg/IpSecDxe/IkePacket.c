/** @file
  IKE Packet related operation.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecDebug.h"
#include "Ikev2/Utility.h"

/**
  Allocate a buffer for the IKE_PACKET and intitalize its Header and payloadlist.

  @return The pointer of the IKE_PACKET.

**/
IKE_PACKET *
IkePacketAlloc (
  VOID
  )
{
  IKE_PACKET  *IkePacket;

  IkePacket = (IKE_PACKET *) AllocateZeroPool (sizeof (IKE_PACKET));
  if (IkePacket == NULL) {
    return NULL;
  }

  IkePacket->RefCount = 1;
  InitializeListHead (&IkePacket->PayloadList);
  
  IkePacket->Header = (IKE_HEADER *) AllocateZeroPool (sizeof (IKE_HEADER));
  if (IkePacket->Header == NULL) {
    FreePool (IkePacket);
    return NULL;
  }
  return IkePacket;
}

/**
  Free the IkePacket by the specified IKE_PACKET pointer.

  @param[in]  IkePacket  The pointer of the IKE_PACKET to be freed.

**/
VOID
IkePacketFree (
  IN IKE_PACKET *IkePacket
  )
{
  LIST_ENTRY  *Entry;
  IKE_PAYLOAD *IkePayload;

  if (IkePacket == NULL) {
    return;
  }
  //
  // Check if the Packet is referred by others.
  //
  if (--IkePacket->RefCount == 0) {
    //
    // Free IkePacket header
    //
    if (!IkePacket->IsHdrExt && IkePacket->Header != NULL) {
      FreePool (IkePacket->Header);
    }
    //
    // Free the PayloadsBuff
    //
    if (!IkePacket->IsPayloadsBufExt && IkePacket->PayloadsBuf != NULL) {
      FreePool (IkePacket->PayloadsBuf);
    }
    //
    // Iterate payloadlist and free all payloads
    //
    for (Entry = (IkePacket)->PayloadList.ForwardLink; Entry != &(IkePacket)->PayloadList;) {
      IkePayload  = IKE_PAYLOAD_BY_PACKET (Entry);
      Entry       = Entry->ForwardLink;

      IkePayloadFree (IkePayload);
    }

    FreePool (IkePacket);
  }
}

/**
  Callback funtion of NetbufFromExt()
  
  @param[in]  Arg  The data passed from the NetBufFromExe(). 

**/
VOID
EFIAPI
IkePacketNetbufFree (
  IN VOID  *Arg
  )
{
  //
  // TODO: add something if need.
  //
}

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
  )
{
  IKE_PACKET  *IkePacket;

  IkePacket = NULL;
  if (Netbuf->TotalSize < sizeof (IKE_HEADER)) {
    goto Error;
  }

  IkePacket = IkePacketAlloc ();
  if (IkePacket == NULL) {
    return NULL;
  }
  //
  // Copy the IKE header from Netbuf to IkePacket->Hdr
  //
  NetbufCopy (Netbuf, 0, sizeof (IKE_HEADER), (UINT8 *) IkePacket->Header);
  //
  // Net order to host order
  //
  IkeHdrNetToHost (IkePacket->Header);
  if (IkePacket->Header->Length < Netbuf->TotalSize) {
    goto Error;
  }

  IkePacket->PayloadTotalSize = IkePacket->Header->Length - sizeof (IKE_HEADER);
  IkePacket->PayloadsBuf      = (UINT8 *) AllocateZeroPool (IkePacket->PayloadTotalSize);

  if (IkePacket->PayloadsBuf == NULL) {
    goto Error;
  }
  //
  // Copy the IKE packet without the header into the IkePacket->PayloadsBuf.
  //
  NetbufCopy (Netbuf, sizeof (IKE_HEADER), (UINT32) IkePacket->PayloadTotalSize, IkePacket->PayloadsBuf);
  return IkePacket;

Error:
  if (IkePacket != NULL) {
    IkePacketFree (IkePacket);
  }

  return NULL;
}

/**
  Convert the format from IKE_PACKET to NetBuf.

  @param[in]  SessionCommon  Pointer of related IKE_COMMON_SESSION
  @param[in]  IkePacket      Pointer of IKE_PACKET to be copy to NetBuf
  @param[in]  IkeType        The IKE type to pointer the packet is for which IKE 
                             phase. Now it supports IKE_SA_TYPE, IKE_CHILDSA_TYPE, 
                             IKE_INFO_TYPE.

  @return a pointer of Netbuff which contains the IKE_PACKE in network order.
  
**/
NET_BUF *
IkeNetbufFromPacket (
  IN UINT8               *SessionCommon,
  IN IKE_PACKET          *IkePacket,
  IN UINTN               IkeType
  )
{
  NET_BUF       *Netbuf;
  NET_FRAGMENT  *Fragments;
  UINTN         Index;
  UINTN         NumPayloads;
  LIST_ENTRY    *PacketEntry;
  LIST_ENTRY    *Entry;
  IKE_PAYLOAD   *IkePayload;

  if (!IkePacket->IsEncoded) {
    IkePacket->IsEncoded = TRUE;
    //
    // Convert Host order to Network order for IKE_PACKET header and payloads
    // Encryption payloads if needed
    //
    if (((IKEV2_SESSION_COMMON *) SessionCommon)->IkeVer == 2) {
      Ikev2EncodePacket ((IKEV2_SESSION_COMMON *) SessionCommon, IkePacket, IkeType);
    } else {
      //
      //If IKEv1 support, check it here.
      //
      return NULL;
    }
  }

  NumPayloads = 0;
  //
  // Get the number of the payloads
  //
  NET_LIST_FOR_EACH (PacketEntry, &(IkePacket)->PayloadList) {
  
    NumPayloads++;
  }
  //
  // Allocate the Framgents according to the numbers of the IkePayload
  //
  Fragments = (NET_FRAGMENT *) AllocateZeroPool ((1 + NumPayloads) * sizeof (NET_FRAGMENT));
  if (Fragments == NULL) {
    return NULL;
  }

  Fragments[0].Bulk = (UINT8 *) IkePacket->Header;
  Fragments[0].Len  = sizeof (IKE_HEADER);
  Index             = 0;

  //
  // Set payloads to the Framgments.
  //
  NET_LIST_FOR_EACH (Entry, &(IkePacket)->PayloadList) {
    IkePayload = IKE_PAYLOAD_BY_PACKET (Entry);

    Fragments[Index + 1].Bulk = IkePayload->PayloadBuf;
    Fragments[Index + 1].Len  = (UINT32) IkePayload->PayloadSize;
    Index++;
  }

  Netbuf = NetbufFromExt (
             Fragments,
             (UINT32) (NumPayloads + 1),
             0,
             0,
             IkePacketNetbufFree,
             NULL
             );
  
  FreePool (Fragments);
  return Netbuf;
}


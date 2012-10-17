/** @file
  The Interfaces of IPsec debug information printing.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecImpl.h"
#include "IpSecDebug.h"

//
// The print title for IKEv1 variety phase.
//
CHAR8 *mIkev1StateStr[IKE_STATE_NUM] = {
  "IKEv1_MAIN_1",
  "IKEv1_MAIN_2",
  "IKEv1_MAIN_3",
  "IKEv1_MAIN_ESTABLISHED",
  "IKEv1_QUICK_1",
  "IKEv1_QUICK_2",
  "IKEv1_QUICK_ESTABLISHED"
};

//
// The print title for IKEv2 variety phase.
//
CHAR8 *mIkev2StateStr[IKE_STATE_NUM] = {
  "IKEv2_STATE_INIT",
  "IKEv2_STATE_AUTH",
  "IKEv2_STATE_SA_ESTABLISH",
  "IKEv2_STATE_CREATE_CHILD",
  "IKEv2_STATE_SA_REKEYING",
  "IKEv2_STATE_CHILD_SA_ESTABLISHED",
  "IKEv2_STATE_SA_DELETING"
};

//
// The print title for IKEv1 variety Exchagne.
//
CHAR8 *mExchangeStr[] = {
  "IKEv1 Main Exchange",
  "IKEv1 Info Exchange",
  "IKEv1 Quick Exchange",
  "IKEv2 Initial Exchange",
  "IKEv2 Auth Exchange",
  "IKEv2 Create Child Exchange",
  "IKEv2 Info Exchange",
  "IKE   Unknow Exchange"
};

//
// The print title for IKEv1 variety Payload.
//
CHAR8 *mIkev1PayloadStr[] = {
  "IKEv1 None Payload",
  "IKEv1 SA Payload",
  "IKEv1 Proposal Payload",
  "IKEv1 Transform Payload",
  "IKEv1 KE Payload",
  "IKEv1 ID Payload",
  "IKEv1 Certificate Payload",
  "IKEv1 Certificate Request Payload",
  "IKEv1 Hash Payload",
  "IKEv1 Signature Payload",
  "IKEv1 Nonce Payload",
  "IKEv1 Notify Payload",
  "IKEv1 Delete Payload",
  "IKEv1 Vendor Payload"
};

//
// The print title for IKEv2 variety Payload.
//
CHAR8* mIkev2PayloadStr[] = {
  "IKEv2 SA Payload",
  "IKEv2 Key Payload",
  "IKEv2 Identity Initial Payload",
  "IKEv2 Identity Respond Payload",
  "IKEv2 Certificate Payload",
  "IKEv2 Certificate Request Payload",
  "IKEv2 Auth Payload",
  "IKEv2 Nonce Payload",
  "IKEv2 Notify Payload",
  "IKEv2 Delet Payload",
  "IKEv2 Vendor Payload",
  "IKEv2 Traffic Selector Initiator Payload",
  "IKEv2 Traffic Selector Respond Payload",
  "IKEv2 Encrypt Payload",
  "IKEv2 Configuration Payload",
  "IKEv2 Extensible Authentication Payload"
};

/**
  Print the IP address.

  @param[in]  Level     Debug print error level. Pass to DEBUG().
  @param[in]  Ip        Point to a specified IP address.
  @param[in]  IpVersion The IP Version.

**/
VOID
IpSecDumpAddress (
  IN UINTN               Level,
  IN EFI_IP_ADDRESS      *Ip,
  IN UINT8               IpVersion
  )
{
  if (IpVersion == IP_VERSION_6) {
    DEBUG (
      (Level,
      "%x%x:%x%x:%x%x:%x%x",
      Ip->v6.Addr[0],
      Ip->v6.Addr[1],
      Ip->v6.Addr[2],
      Ip->v6.Addr[3],
      Ip->v6.Addr[4],
      Ip->v6.Addr[5],
      Ip->v6.Addr[6],
      Ip->v6.Addr[7])
      );
    DEBUG (
      (Level,
      ":%x%x:%x%x:%x%x:%x%x\n",
      Ip->v6.Addr[8],
      Ip->v6.Addr[9],
      Ip->v6.Addr[10],
      Ip->v6.Addr[11],
      Ip->v6.Addr[12],
      Ip->v6.Addr[13],
      Ip->v6.Addr[14],
      Ip->v6.Addr[15])
      );
  } else {
    DEBUG (
      (Level,
      "%d.%d.%d.%d\n",
      Ip->v4.Addr[0],
      Ip->v4.Addr[1],
      Ip->v4.Addr[2],
      Ip->v4.Addr[3])
      );
  }

}

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
  )
{
  if (Previous >= IKE_STATE_NUM || Current >= IKE_STATE_NUM) {
    return; 
  }
  
  if (Previous == Current) {
    if (IkeVersion == 1) {
      DEBUG ((DEBUG_INFO, "\n****Current state is %a\n", mIkev1StateStr[Previous]));
    } else if (IkeVersion == 2) {
      DEBUG ((DEBUG_INFO, "\n****Current state is %a\n", mIkev2StateStr[Previous]));
    }    
  } else {
    if (IkeVersion == 1) {
      DEBUG ((DEBUG_INFO, "\n****Change state from %a to %a\n", mIkev1StateStr[Previous], mIkev1StateStr[Current]));
    } else {
      DEBUG ((DEBUG_INFO, "\n****Change state from %a to %a\n", mIkev2StateStr[Previous], mIkev2StateStr[Current]));
    }    
  }
}

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
  )
{
  CHAR8                     *TypeStr;
  UINTN                     PacketSize;
  UINT64                    InitCookie;
  UINT64                    RespCookie;

  ASSERT (Packet != NULL);

  PacketSize = Packet->PayloadTotalSize + sizeof (IKE_HEADER);
  InitCookie = (Direction == EfiIPsecOutBound) ? HTONLL (Packet->Header->InitiatorCookie) : Packet->Header->InitiatorCookie;
  RespCookie = (Direction == EfiIPsecOutBound) ? HTONLL (Packet->Header->ResponderCookie) : Packet->Header->ResponderCookie;

  switch (Packet->Header->ExchangeType) {
  case IKE_XCG_TYPE_IDENTITY_PROTECT:
    TypeStr = mExchangeStr[0];
    break;

  case IKE_XCG_TYPE_INFO:
    TypeStr = mExchangeStr[1];
    break;

  case IKE_XCG_TYPE_QM:
    TypeStr = mExchangeStr[2];
    break;
    
  case IKE_XCG_TYPE_SA_INIT:
    TypeStr = mExchangeStr[3];
    break;

  case IKE_XCG_TYPE_AUTH:
    TypeStr = mExchangeStr[4];
    break;

  case IKE_XCG_TYPE_CREATE_CHILD_SA:
    TypeStr = mExchangeStr[5];
    break;

  case IKE_XCG_TYPE_INFO2:
    TypeStr = mExchangeStr[6];
    break;
    
  default:
    TypeStr = mExchangeStr[7];
    break;
  }

  if (Direction == EfiIPsecOutBound) {
    DEBUG ((DEBUG_INFO, "\n>>>Sending %d bytes %a to ", PacketSize, TypeStr));
  } else {
    DEBUG ((DEBUG_INFO, "\n>>>Receiving %d bytes %a from ", PacketSize, TypeStr));
  }

  IpSecDumpAddress (DEBUG_INFO, &Packet->RemotePeerIp, IpVersion);

  DEBUG ((DEBUG_INFO, "   InitiatorCookie:0x%lx ResponderCookie:0x%lx\n", InitCookie, RespCookie));
  DEBUG (
    (DEBUG_INFO,
    "   Version: 0x%x Flags:0x%x ExchangeType:0x%x\n",
    Packet->Header->Version,
    Packet->Header->Flags,
    Packet->Header->ExchangeType)
    );
  DEBUG (
    (DEBUG_INFO,
    "   MessageId:0x%x NextPayload:0x%x\n",
    Packet->Header->MessageId,
    Packet->Header->NextPayload)
    );

}

/**
  Print the IKE Paylolad.

  @param[in]  IkePayload  Point to payload to be printed.
  @param[in]  IkeVersion  The specified version of IKE.
 
**/
VOID
IpSecDumpPayload (
  IN IKE_PAYLOAD           *IkePayload,
  IN UINT8                 IkeVersion
  )
{
  if (IkeVersion == 1) {
    DEBUG ((DEBUG_INFO, "+%a\n", mIkev1PayloadStr[IkePayload->PayloadType]));
  }  else {
    //
    // For IKEV2 the first Payload type is started from 33.
    //
    DEBUG ((DEBUG_INFO, "+%a\n", mIkev2PayloadStr[IkePayload->PayloadType - 33]));
  }
  IpSecDumpBuf ("Payload data", IkePayload->PayloadBuf, IkePayload->PayloadSize);
}

/**
  Print the buffer in form of Hex.

  @param[in]  Title       The strings to be printed before the data of the buffer.
  @param[in]  Data        Points to buffer to be printed.
  @param[in]  DataSize    The size of the buffer to be printed.

**/
VOID
IpSecDumpBuf (
  IN CHAR8                 *Title,
  IN UINT8                 *Data,
  IN UINTN                 DataSize
  )
{
  UINTN Index;
  UINTN DataIndex;
  UINTN BytesRemaining;
  UINTN BytesToPrint;

  DataIndex       = 0;
  BytesRemaining  = DataSize;

  DEBUG ((DEBUG_INFO, "==%a %d bytes==\n", Title, DataSize));

  while (BytesRemaining > 0) {

    BytesToPrint = (BytesRemaining > IPSEC_DEBUG_BYTE_PER_LINE) ? IPSEC_DEBUG_BYTE_PER_LINE : BytesRemaining;

    for (Index = 0; Index < BytesToPrint; Index++) {
      DEBUG ((DEBUG_INFO, " 0x%02x,", Data[DataIndex++]));
    }

    DEBUG ((DEBUG_INFO, "\n"));
    BytesRemaining -= BytesToPrint;
  }

}

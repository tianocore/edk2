/** @file
  Common operation of the IKE.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IKE_COMMON_H_
#define _IKE_COMMON_H_

#include <Protocol/Udp4.h>
#include <Protocol/Udp6.h>
#include <Protocol/Ip4Config.h>
 
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UdpIoLib.h>
#include <Library/BaseCryptLib.h>

#include "Ikev2/Ikev2.h"
#include "IpSecImpl.h"
#include "IkePacket.h"
#include "IpSecCryptIo.h"


#define IKE_DEFAULT_PORT              500
#define IKE_DEFAULT_TIMEOUT_INTERVAL  10000 // 10s
#define IKE_NONCE_SIZE                16
#define IKE_MAX_RETRY                 4
#define IKE_SPI_BASE                  0x10000
#define IKE_PAYLOAD_SIGNATURE         SIGNATURE_32('I','K','E','P')
#define IKE_PAYLOAD_BY_PACKET(a)      CR(a,IKE_PAYLOAD,ByPacket,IKE_PAYLOAD_SIGNATURE)


#define IKE_PACKET_APPEND_PAYLOAD(IkePacket,IkePayload)                 \
  do {                                                                  \
    InsertTailList(&(IkePacket)->PayloadList, &(IkePayload)->ByPacket); \
  } while (0)

#define IKE_PACKET_REMOVE_PAYLOAD(IkePacket,IkePayload)                 \
  do {                                                                  \
    RemoveEntryList(&(IkePayload)->ByPacket);                           \
  } while (0)

#define IKE_PACKET_END_PAYLOAD(IkePacket, Node)                        \
  Node = GetFirstNode (&(IkePacket)->PayloadList);                      \
  while (!IsNodeAtEnd (&(IkePacket)->PayloadList, Node)) {             \
    Node = GetNextNode (&(IkePacket)->PayloadList, Node);              \
  }                                                                     \

/**
  Call Crypto Lib to generate a random value with eight-octet length.
  
  @return the 64 byte vaule.

**/
UINT64
IkeGenerateCookie (
  VOID
  );

/**
  Generate the random data for Nonce payload.

  @param[in]  NonceSize      Size of the data in bytes.
  
  @return Buffer which contains the random data of the spcified size. 

**/
UINT8 *
IkeGenerateNonce (
  IN UINTN              NonceSize
  );

/**
  Convert the IKE Header from Network order to Host order.

  @param[in, out]  Header    The pointer of the IKE_HEADER.

**/
VOID
IkeHdrNetToHost (
  IN OUT IKE_HEADER *Header
  );


/**
  Convert the IKE Header from Host order to Network order.

  @param[in, out] Header     The pointer of the IKE_HEADER.

**/
VOID
IkeHdrHostToNet (
  IN OUT IKE_HEADER *Header
  );

/**
  Allocate a buffer of IKE_PAYLOAD and set its Signature.

  @return A buffer of IKE_PAYLOAD.

**/
IKE_PAYLOAD *
IkePayloadAlloc (
  VOID
  );

/**
  Free a specified IKE_PAYLOAD buffer.

  @param[in]  IkePayload   Pointer of IKE_PAYLOAD to be freed.

**/
VOID
IkePayloadFree (
  IN IKE_PAYLOAD *IkePayload
  );

/**
  Generate an unused SPI

  @return a SPI in 4 bytes.

**/
UINT32
IkeGenerateSpi (
  VOID
  );

/**
  Generate a random data for IV

  @param[in]  IvBuffer  The pointer of the IV buffer.
  @param[in]  IvSize    The IV size.

  @retval     EFI_SUCCESS  Create a random data for IV.
  @retval     otherwise    Failed.

**/
EFI_STATUS
IkeGenerateIv (
  IN UINT8                           *IvBuffer,
  IN UINTN                           IvSize
  );

/**
  Get the IKE Version from the IKE_SA_SESSION.

  @param[in]  Session  Pointer of the IKE_SA_SESSION.

**/
UINT8
IkeGetVersionFromSession (
  IN UINT8                    *Session
  );

/**
  Find SPD entry by a specified SPD selector.

  @param[in] SpdSel       Point to SPD Selector to be searched for.

  @retval Point to Spd Entry if the SPD entry found.
  @retval NULL if not found.

**/
IPSEC_SPD_ENTRY *
IkeSearchSpdEntry (
  IN EFI_IPSEC_SPD_SELECTOR             *SpdSel
  );

extern EFI_GUID               mZeroGuid;
extern MODP_GROUP             OakleyModpGroup[];
extern IKE_ALG_GUID_INFO      mIPsecEncrAlgInfo[];
extern IKE_ALG_GUID_INFO      mIPsecAuthAlgInfo[];

#endif


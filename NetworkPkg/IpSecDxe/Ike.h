/** @file
  The common definition of IPsec Key Exchange (IKE).

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#ifndef _IKE_H_
#define _IKE_H_

#include <Library/UdpIoLib.h>
#include <Library/BaseCryptLib.h>
#include "IpSecImpl.h"

#define IKE_VERSION_MAJOR_MASK  0xf0
#define IKE_VERSION_MINOR_MASK  0x0f

#define IKE_MAJOR_VERSION(v)    (((v) & IKE_VERSION_MAJOR_MASK) >> 4)
#define IKE_MINOR_VERSION(v)    ((v) & IKE_VERSION_MINOR_MASK)

//
// Protocol Value Use in IKEv1 and IKEv2
//
#define IPSEC_PROTO_ISAKMP    1
#define IPSEC_PROTO_IPSEC_AH  2
#define IPSEC_PROTO_IPSEC_ESP 3
#define IPSEC_PROTO_IPCOMP    4 // For IKEv1 this value is reserved

//
//  For Algorithm search in support list.Last two types are for IKEv2 only.
//
#define IKE_ENCRYPT_TYPE      0
#define IKE_AUTH_TYPE         1
#define IKE_PRF_TYPE          2
#define IKE_DH_TYPE           3

//
// Encryption Algorithm present in IKEv1 phasrs2 and IKEv2 transform payload (Transform Type 1)
//
#define IPSEC_ESP_DES_IV64            1
#define IPSEC_ESP_DES                 2
#define IPSEC_ESP_3DES                3
#define IPSEC_ESP_RC5                 4
#define IPSEC_ESP_IDEA                5
#define IPSEC_ESP_CAST                6
#define IPSEC_ESP_BLOWFISH            7
#define IPSEC_ESP_3IDEA               8
#define IPSEC_ESP_DES_IV32            9
#define IPSEC_ESP_RC4                 10  // It's reserved in IKEv2 
#define IPSEC_ESP_NULL                11
#define IPSEC_ESP_AES                 12

#define IKE_XCG_TYPE_NONE             0
#define IKE_XCG_TYPE_BASE             1
#define IKE_XCG_TYPE_IDENTITY_PROTECT 2
#define IKE_XCG_TYPE_AUTH_ONLY        3
#define IKE_XCG_TYPE_AGGR             4
#define IKE_XCG_TYPE_INFO             5
#define IKE_XCG_TYPE_QM               32
#define IKE_XCG_TYPE_NGM              33
#define IKE_XCG_TYPE_SA_INIT          34
#define IKE_XCG_TYPE_AUTH             35
#define IKE_XCG_TYPE_CREATE_CHILD_SA  36
#define IKE_XCG_TYPE_INFO2            37

#define IKE_LIFE_TYPE_SECONDS         1
#define IKE_LIFE_TYPE_KILOBYTES       2

//
// Deafult IKE SA lifetime and CHILD SA lifetime
//
#define IKE_SA_DEFAULT_LIFETIME       1200
#define CHILD_SA_DEFAULT_LIFETIME     3600

//
// Next payload type presented within Proposal payload
//
#define IKE_PROPOSAL_NEXT_PAYLOAD_MORE  2
#define IKE_PROPOSAL_NEXT_PAYLOAD_NONE  0

//
// Next payload type presented within Transform payload
//
#define IKE_TRANSFORM_NEXT_PAYLOAD_MORE 3
#define IKE_TRANSFORM_NEXT_PAYLOAD_NONE 0

//
// Max size of the SA attribute
//
#define MAX_SA_ATTRS_SIZE     48
#define SA_ATTR_FORMAT_BIT    0x8000
//
// The definition for Information Message ID.
//
#define INFO_MID_SIGNATURE    SIGNATURE_32 ('I', 'N', 'F', 'M')

//
// Type for the IKE SESSION COMMON
//
typedef enum {
  IkeSessionTypeIkeSa,
  IkeSessionTypeChildSa,
  IkeSessionTypeInfo,
  IkeSessionTypeMax
} IKE_SESSION_TYPE;

//
// The DH Group ID defined RFC3526 and RFC 2409
//
typedef enum {
  OakleyGroupModp768  = 1,
  OakleyGroupModp1024 = 2,
  OakleyGroupGp155    = 3,  // Unsupported Now.
  OakleyGroupGp185    = 4,  // Unsupported Now.
  OakleyGroupModp1536 = 5,

  OakleyGroupModp2048 = 14,
  OakleyGroupModp3072 = 15,
  OakleyGroupModp4096 = 16,
  OakleyGroupModp6144 = 17,
  OakleyGroupModp8192 = 18,
  OakleyGroupMax
} OAKLEY_GROUP_ID;

//
// IKE Header
//
#pragma pack(1)
typedef struct {
  UINT64  InitiatorCookie;
  UINT64  ResponderCookie;
  UINT8   NextPayload;
  UINT8   Version;
  UINT8   ExchangeType;
  UINT8   Flags;
  UINT32  MessageId;
  UINT32  Length;
} IKE_HEADER;
#pragma pack()

typedef union {
  UINT16  AttrLength;
  UINT16  AttrValue;
} IKE_SA_ATTR_UNION; 

//
// SA Attribute present in Transform Payload
//
#pragma pack(1)
typedef struct {
  UINT16            AttrType;
  IKE_SA_ATTR_UNION Attr;
} IKE_SA_ATTRIBUTE;
#pragma pack()

//
// Contains the IKE packet information. 
//
typedef struct {
  UINTN               RefCount;
  BOOLEAN             IsHdrExt;
  IKE_HEADER          *Header;
  BOOLEAN             IsPayloadsBufExt;
  UINT8               *PayloadsBuf; // The whole IkePakcet trimed the IKE header.
  UINTN               PayloadTotalSize;
  LIST_ENTRY          PayloadList;
  EFI_IP_ADDRESS      RemotePeerIp;
  BOOLEAN             IsEncoded;    // whether HTON is done when sending the packet
  UINT32              Spi;          // For the Delete Information Exchange
  BOOLEAN             IsDeleteInfo; // For the Delete Information Exchange
  IPSEC_PRIVATE_DATA  *Private;     // For the Delete Information Exchange
} IKE_PACKET;

//
// The generic structure to all kinds of IKE payloads.
//
typedef struct {
  UINT32      Signature;
  BOOLEAN     IsPayloadBufExt;
  UINT8       PayloadType;
  UINT8       *PayloadBuf;
  UINTN       PayloadSize;
  LIST_ENTRY  ByPacket;
} IKE_PAYLOAD;

//
// Udp Service
//
typedef struct {
  UINT32          Signature;
  UINT8           IpVersion;
  LIST_ENTRY      List;
  LIST_ENTRY      *ListHead;
  EFI_HANDLE      NicHandle;
  EFI_HANDLE      ImageHandle;
  UDP_IO          *Input;
  UDP_IO          *Output;
  EFI_IP_ADDRESS  DefaultAddress;
  BOOLEAN         IsConfigured;
} IKE_UDP_SERVICE;

//
// Each IKE session has its own Key sets for local peer and remote peer.
//
typedef struct {
  EFI_IPSEC_ALGO_INFO LocalPeerInfo;
  EFI_IPSEC_ALGO_INFO RemotePeerInfo;
} SA_KEYMATS;

//
// Each algorithm has its own Id, Guid, BlockSize and KeyLength.
// This struct contains these information for each algorithm. It is generic structure
// for both encryption and authentication algorithm. 
// For authentication algorithm, the AlgSize means IcvSize. For encryption algorithm,
// it means IvSize.
//
#pragma pack(1)
typedef struct {
  UINT8     AlgorithmId;       // Encryption or Authentication Id used by ESP/AH
  EFI_GUID  *AlgGuid;
  UINT8     AlgSize;     // IcvSize or IvSize
  UINT8     BlockSize;
  UINTN     KeyMateLen;
} IKE_ALG_GUID_INFO;   // For IPsec Authentication and Encryption Algorithm.
#pragma pack()

//
// Structure used to store the DH group
//
typedef struct {
  UINT8 GroupId;
  UINTN Size;
  UINT8 *Modulus;
  UINTN GroupGenerator;
} MODP_GROUP;

/**
  This is prototype definition of general interface to phase the payloads
  after/before the decode/encode.

  @param[in]  SessionCommon    Point to the SessionCommon
  @param[in]  PayloadBuf       Point to the buffer of Payload.
  @param[in]  PayloadSize      The size of the PayloadBuf in bytes.
  @param[in]  PayloadType      The type of Payload.

**/
typedef
VOID
(*IKE_ON_PAYLOAD_FROM_NET) (
  IN UINT8    *SessionCommon,
  IN UINT8    *PayloadBuf,
  IN UINTN    PayloadSize,
  IN UINT8    PayloadType
  );

#endif


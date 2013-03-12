/** @file
  IKEv2 related definitions.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _IKE_V2_H_
#define _IKE_V2_H_

#include "Ike.h"
#include "Payload.h"

#define IKEV2_TS_ANY_PORT                     0xffff
#define IKEV2_TS_ANY_PROTOCOL                 0

#define IKEV2_DELET_CHILDSA_LIST              0
#define IKEV2_ESTABLISHING_CHILDSA_LIST       1
#define IKEV2_ESTABLISHED_CHILDSA_LIST        2

#define IKEV2_SA_SESSION_SIGNATURE            SIGNATURE_32 ('I', 'K', 'E', 'I')
#define IKEV2_SA_SESSION_FROM_COMMON(a)       CR (a, IKEV2_SA_SESSION, SessionCommon, IKEV2_SA_SESSION_SIGNATURE)
#define IKEV2_SA_SESSION_BY_SESSION(a)        CR (a, IKEV2_SA_SESSION, BySessionTable, IKEV2_SA_SESSION_SIGNATURE)
#define IKEV2_SA_SESSION_BY_ESTABLISHED(a)    CR (a, IKEV2_SA_SESSION, ByEstablishedTable, IKEV2_SA_SESSION_SIGNATURE)

#define IKEV2_CHILD_SA_SESSION_SIGNATURE      SIGNATURE_32 ('I', 'K', 'E', 'C')
#define IKEV2_CHILD_SA_SESSION_FROM_COMMON(a) CR (a, IKEV2_CHILD_SA_SESSION, SessionCommon, IKEV2_CHILD_SA_SESSION_SIGNATURE)
#define IKEV2_CHILD_SA_SESSION_BY_IKE_SA(a)   CR (a, IKEV2_CHILD_SA_SESSION, ByIkeSa, IKEV2_CHILD_SA_SESSION_SIGNATURE)
#define IKEV2_CHILD_SA_SESSION_BY_DEL_SA(a)   CR (a, IKEV2_CHILD_SA_SESSION, ByDelete, IKEV2_CHILD_SA_SESSION_SIGNATURE)

#define IS_IKEV2_SA_SESSION(s)                ((s)->Common.IkeSessionType == IkeSessionTypeIkeSa)
#define IKEV2_SA_FIRST_PROPOSAL(Sa)           (IKEV2_PROPOSAL *)((IKEV2_SA *)(Sa)+1)
#define IKEV2_NEXT_TRANSFORM_WITH_SIZE(Transform,TransformSize)         \
        (IKEV2_TRANSFORM *) ((UINT8 *)(Transform) + (TransformSize))

#define IKEV2_NEXT_PROPOSAL_WITH_SIZE(Proposal, ProposalSize)           \
        (IKEV2_PROPOSAL *) ((UINT8 *)(Proposal) + (ProposalSize))

#define IKEV2_PROPOSAL_FIRST_TRANSFORM(Proposal)                        \
        (IKEV2_TRANSFORM *)((UINT8 *)((IKEV2_PROPOSAL *)(Proposal)+1) + \
                      (((IKEV2_PROPOSAL *)(Proposal))->SpiSize))
#define IKEV2_PROPOSAL_FIRST_TRANSFORM(Proposal)                        \
        (IKEV2_TRANSFORM *)((UINT8 *)((IKEV2_PROPOSAL *)(Proposal)+1) + \
                      (((IKEV2_PROPOSAL *)(Proposal))->SpiSize))

typedef enum {
  IkeStateInit,
  IkeStateAuth,
  IkeStateIkeSaEstablished,
  IkeStateCreateChild,
  IkeStateSaRekeying,
  IkeStateChildSaEstablished,
  IkeStateSaDeleting,
  IkeStateMaximum
} IKEV2_SESSION_STATE;

typedef enum {
  IkeRequestTypeCreateChildSa,
  IkeRequestTypeRekeyChildSa,
  IkeRequestTypeRekeyIkeSa,
  IkeRequestTypeMaximum
} IKEV2_CREATE_CHILD_REQUEST_TYPE;

typedef struct {
  UINT8            *GxBuffer;
  UINTN            GxSize;
  UINT8            *GyBuffer;
  UINTN            GySize;
  UINT8            *GxyBuffer;
  UINTN            GxySize;
  UINT8            *DhContext;
} IKEV2_DH_BUFFER;

typedef struct {
  IKEV2_DH_BUFFER   *DhBuffer;
  UINT8             *SkdKey;
  UINTN             SkdKeySize;
  UINT8             *SkAiKey;
  UINTN             SkAiKeySize;
  UINT8             *SkArKey;
  UINTN             SkArKeySize;
  UINT8             *SkEiKey;
  UINTN             SkEiKeySize;
  UINT8             *SkErKey;
  UINTN             SkErKeySize;
  UINT8             *SkPiKey;
  UINTN             SkPiKeySize;
  UINT8             *SkPrKey;
  UINTN             SkPrKeySize;
} IKEV2_SESSION_KEYS;

typedef struct {
  UINT16  LifeType;
  UINT64  LifeDuration;
  UINT16  EncAlgId;
  UINTN   EnckeyLen;
  UINT16  Prf;
  UINT16  IntegAlgId;
  UINTN   IntegKeyLen;
  UINT16  DhGroup;
  UINT8   ExtSeq;
} IKEV2_SA_PARAMS;

//
// Internal Payload
//
typedef struct {
  IKEV2_SA  SaHeader;
  UINTN     NumProposals;
  //
  // IKE_PROPOSAL_DATA  Proposals[1];
  //
} IKEV2_SA_DATA;

typedef struct {
  UINT8 ProposalIndex;
  UINT8 ProtocolId;
  UINT8 *Spi;
  UINT8 NumTransforms;
  //
  // IKE_TRANSFORM_DATA Transforms[1];
  //
} IKEV2_PROPOSAL_DATA;

typedef struct {
  UINT8             TransformIndex;
  UINT8             TransformType;
  UINT16            TransformId;
  IKE_SA_ATTRIBUTE  Attribute;
} IKEV2_TRANSFORM_DATA;

typedef struct {
  UINT8                   IkeVer;
  IKE_SESSION_TYPE        IkeSessionType;
  BOOLEAN                 IsInitiator;
  BOOLEAN                 IsOnDeleting;  // Flag to indicate whether the SA is on deleting.
  IKEV2_SESSION_STATE     State;
  EFI_EVENT               TimeoutEvent;
  UINT64                  TimeoutInterval;
  UINTN                   RetryCount;
  IKE_PACKET              *LastSentPacket;
  IKEV2_SA_PARAMS         *SaParams;
  UINT16                  PreferDhGroup;
  EFI_IP_ADDRESS          RemotePeerIp;
  EFI_IP_ADDRESS          LocalPeerIp;
  IKE_ON_PAYLOAD_FROM_NET BeforeDecodePayload;
  IKE_ON_PAYLOAD_FROM_NET AfterEncodePayload;
  IKE_UDP_SERVICE         *UdpService;
  IPSEC_PRIVATE_DATA      *Private;
} IKEV2_SESSION_COMMON;

typedef struct {
  UINT32                Signature;
  IKEV2_SESSION_COMMON  SessionCommon;
  UINT64                InitiatorCookie;
  UINT64                ResponderCookie;
  //
  // Initiator: SA proposals to be sent
  // Responder: SA proposals to be matched
  //
  IKEV2_SA_DATA         *SaData; // SA Private struct used for SA payload generation
  IKEV2_SESSION_KEYS    *IkeKeys;
  UINT8                 *NiBlock;
  UINTN                 NiBlkSize;
  UINT8                 *NrBlock;
  UINTN                 NrBlkSize;
  UINT8                 *NCookie;                     // Buffer Contains the Notify Cookie
  UINTN                 NCookieSize;                  // Size of NCookie
  IPSEC_PAD_ENTRY       *Pad;
  IPSEC_SPD_ENTRY       *Spd;                         // SPD that requested the negotiation, TODO: better use SPD selector
  LIST_ENTRY            ChildSaSessionList;
  LIST_ENTRY            ChildSaEstablishSessionList;  // For Establish Child SA.
  LIST_ENTRY            InfoMIDList;                  // For Information MID
  LIST_ENTRY            DeleteSaList;                 // For deteling Child SA.
  UINT8                 *InitPacket;
  UINTN                 InitPacketSize;
  UINT8                 *RespPacket;
  UINTN                 RespPacketSize;
  UINT32                MessageId;
  LIST_ENTRY            BySessionTable;               // Use for all IkeSaSession Links
} IKEV2_SA_SESSION;

typedef struct {
  UINT32                 Signature;
  IKEV2_SESSION_COMMON   SessionCommon;
  IKEV2_SA_SESSION       *IkeSaSession;
  UINT32                 MessageId;
  IKEV2_SA_DATA          *SaData;
  UINT8                  IpsecProtocol;
  UINT32                 LocalPeerSpi;
  UINT32                 RemotePeerSpi;
  UINT8                  *NiBlock;
  UINTN                  NiBlkSize;
  UINT8                  *NrBlock;
  UINTN                  NrBlkSize;
  SA_KEYMATS             ChildKeymats;
  IKEV2_DH_BUFFER        *DhBuffer;    //New DH exchnaged by CREATE_CHILD_SA
  IPSEC_SPD_ENTRY        *Spd;
  EFI_IPSEC_SPD_SELECTOR *SpdSelector;
  UINT16                 ProtoId;
  UINT16                 RemotePort;
  UINT16                 LocalPort;
  LIST_ENTRY             ByIkeSa;
  LIST_ENTRY             ByDelete;
} IKEV2_CHILD_SA_SESSION;

typedef enum {
  Ikev2InfoNotify,
  Ikev2InfoDelete,
  Ikev2InfoLiveCheck
} IKEV2_INFO_TYPE;

//
// This struct is used to pass the detail infromation to the InfoGenerator() for
// the response Information Exchange Message creatation.
//
typedef struct {
  UINT32               MessageId;
  IKEV2_INFO_TYPE      InfoType;
} IKEV2_INFO_EXCHANGE_CONTEXT;

typedef struct {
  UINTN DataSize;
  UINT8 *Data;
} PRF_DATA_FRAGMENT;

typedef 
IKE_PACKET *
(*IKEV2_PACKET_GENERATOR) (
  IN UINT8                             *SaSession,
  IN VOID                              *Context
);

typedef
EFI_STATUS
(*IKEV2_PACKET_PARSER) (
  IN UINT8                             *SaSession,
  IN IKE_PACKET                        *IkePacket
);

typedef struct {
  IKEV2_PACKET_PARSER                  Parser;
  IKEV2_PACKET_GENERATOR               Generator;
} IKEV2_PACKET_HANDLER;

extern IKEV2_PACKET_HANDLER            mIkev2Initial[][2];
extern IKEV2_PACKET_HANDLER            mIkev2CreateChild;
extern IKEV2_PACKET_HANDLER            mIkev2Info;

#endif


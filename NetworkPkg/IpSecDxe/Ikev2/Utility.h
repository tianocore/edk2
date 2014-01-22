/** @file
  The interfaces of IKE/Child session operations and payload related operations 
  used by IKE Exchange Process.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IKE_V2_UTILITY_H_
#define _IKE_V2_UTILITY_H_

#include "Ikev2.h"
#include "IkeCommon.h"
#include "IpSecCryptIo.h"

#include <Library/PcdLib.h>

#define IKEV2_SUPPORT_ENCRYPT_ALGORITHM_NUM    2
#define IKEV2_SUPPORT_PRF_ALGORITHM_NUM        1
#define IKEV2_SUPPORT_DH_ALGORITHM_NUM         2
#define IKEV2_SUPPORT_AUTH_ALGORITHM_NUM       1

/**
  Allocate buffer for IKEV2_SA_SESSION and initialize it.

  @param[in] Private        Pointer to IPSEC_PRIVATE_DATA.
  @param[in] UdpService     Pointer to IKE_UDP_SERVICE related to this IKE SA Session.

  @return Pointer to IKEV2_SA_SESSION.

**/
IKEV2_SA_SESSION *
Ikev2SaSessionAlloc (
  IN IPSEC_PRIVATE_DATA       *Private,
  IN IKE_UDP_SERVICE          *UdpService
  );

/**
  Register Establish IKEv2 SA into Private->Ikev2EstablishedList.

  @param[in]  IkeSaSession  Pointer to IKEV2_SA_SESSION to be registered.
  @param[in]  Private       Pointer to IPSEC_PRAVATE_DATA.

**/
VOID
Ikev2SaSessionReg (
  IN IKEV2_SA_SESSION          *IkeSaSession,
  IN IPSEC_PRIVATE_DATA        *Private
  );

/**
  Find a IKEV2_SA_SESSION by the remote peer IP.

  @param[in]  SaSessionList     SaSession List to be searched.
  @param[in]  RemotePeerIp      Pointer to specified IP address.

  @return Pointer to IKEV2_SA_SESSION if find one or NULL.

**/
IKEV2_SA_SESSION *
Ikev2SaSessionLookup (
  IN LIST_ENTRY           *SaSessionList,
  IN EFI_IP_ADDRESS       *RemotePeerIp
  );

/**
  Insert a IKE_SA_SESSION into IkeSaSession list. The IkeSaSession list is either
  Private->Ikev2SaSession list or Private->Ikev2EstablishedList list.

  @param[in]  SaSessionList   Pointer to list to be inserted into.
  @param[in]  IkeSaSession    Pointer to IKEV2_SA_SESSION to be inserted. 
  @param[in]  RemotePeerIp    Pointer to EFI_IP_ADDRESSS to indicate the 
                              unique IKEV2_SA_SESSION.

**/
VOID
Ikev2SaSessionInsert (
  IN LIST_ENTRY           *SaSessionList,
  IN IKEV2_SA_SESSION     *IkeSaSession,
  IN EFI_IP_ADDRESS       *RemotePeerIp
  );

/**
  Remove the SA Session by Remote Peer IP.

  @param[in]  SaSessionList   Pointer to list to be searched.
  @param[in]  RemotePeerIp    Pointer to EFI_IP_ADDRESS to use for SA Session search.

  @retval Pointer to IKEV2_SA_SESSION with the specified remote IP address. 

**/
IKEV2_SA_SESSION *
Ikev2SaSessionRemove (
  IN LIST_ENTRY           *SaSessionList,
  IN EFI_IP_ADDRESS       *RemotePeerIp
  );


/**
  Marking a SA session as on deleting.

  @param[in]  IkeSaSession  Pointer to IKEV2_SA_SESSION.

  @retval     EFI_SUCCESS   Find the related SA session and marked it.

**/
EFI_STATUS
Ikev2SaSessionOnDeleting (
  IN IKEV2_SA_SESSION          *IkeSaSession
  );

/**
  After IKE/Child SA is estiblished, close the time event and free sent packet.

  @param[in]   SessionCommon   Pointer to a Session Common.

**/
VOID
Ikev2SessionCommonRefresh (
  IN IKEV2_SESSION_COMMON      *SessionCommon
  );

/**
  Free specified IKEV2 SA Session. 

  @param[in]    IkeSaSession   Pointer to IKEV2_SA_SESSION to be freed.

**/
VOID
Ikev2SaSessionFree (
  IN IKEV2_SA_SESSION         *IkeSaSession
  );

/**
  Free specified Seession Common. The session common would belong to a IKE SA or 
  a Child SA.

  @param[in]   SessionCommon   Pointer to a Session Common.

**/
VOID
Ikev2SaSessionCommonFree (
  IN IKEV2_SESSION_COMMON      *SessionCommon
  );

/**
  Increase the MessageID in IkeSaSession.

  @param[in] IkeSaSession Pointer to a specified IKEV2_SA_SESSION.

**/
VOID
Ikev2SaSessionIncreaseMessageId (
  IN IKEV2_SA_SESSION         *IkeSaSession
  );

/**
  Allocate Momery for IKEV2 Child SA Session.
  
  @param[in]   UdpService     Pointer to IKE_UDP_SERVICE.
  @param[in]   IkeSaSession   Pointer to IKEV2_SA_SESSION related to this Child SA 
                              Session.

  @retval  Pointer of a new created IKEV2 Child SA Session.

**/
IKEV2_CHILD_SA_SESSION *
Ikev2ChildSaSessionAlloc (
  IN IKE_UDP_SERVICE          *UdpService,
  IN IKEV2_SA_SESSION         *IkeSaSession
  );

/**
  Register a established IKEv2 Child SA into IkeSaSession->ChildSaEstablishSessionList. 
  If the there is IKEV2_CHILD_SA_SESSION with same remote peer IP, remove the old one 
  then register the new one.

  @param[in]  ChildSaSession  Pointer to IKEV2_CHILD_SA_SESSION to be registered.
  @param[in]  Private         Pointer to IPSEC_PRAVATE_DATA.

**/
VOID
Ikev2ChildSaSessionReg (
  IN IKEV2_CHILD_SA_SESSION    *ChildSaSession,
  IN IPSEC_PRIVATE_DATA        *Private
  );

/**
  This function find the Child SA by the specified Spi.

  This functin find a ChildSA session by searching the ChildSaSessionlist of
  the input IKEV2_SA_SESSION by specified MessageID.
  
  @param[in]  SaSessionList      Pointer to List to be searched.
  @param[in]  Spi                Specified SPI.

  @return Pointer to IKEV2_CHILD_SA_SESSION.

**/
IKEV2_CHILD_SA_SESSION *
Ikev2ChildSaSessionLookupBySpi (
  IN LIST_ENTRY           *SaSessionList,
  IN UINT32               Spi
  );

/**
  Find the ChildSaSession by it's MessagId.

  @param[in] SaSessionList  Pointer to a ChildSaSession List.
  @param[in] Mid            The messageId used to search ChildSaSession.

  @return Pointer to IKEV2_CHILD_SA_SESSION.

**/
IKEV2_CHILD_SA_SESSION *
Ikev2ChildSaSessionLookupByMid (
  IN LIST_ENTRY           *SaSessionList,
  IN UINT32               Mid
  );

/**
  Insert a Child SA Session into the specified ChildSa list..

  @param[in]  SaSessionList   Pointer to list to be inserted in.
  @param[in]  ChildSaSession  Pointer to IKEV2_CHILD_SA_SESSION to be inserted.

**/
VOID
Ikev2ChildSaSessionInsert (
  IN LIST_ENTRY               *SaSessionList,
  IN IKEV2_CHILD_SA_SESSION   *ChildSaSession
  );

/**
  Remove the IKEV2_CHILD_SA_SESSION from IkeSaSessionList.
  
  @param[in]  SaSessionList      The SA Session List to be iterated.
  @param[in]  Spi                Spi used to identify the IKEV2_CHILD_SA_SESSION.
  @param[in]  ListType           The type of the List to indicate whether it is a 
                                 Established. 

  @return The point to IKEV2_CHILD_SA_SESSION.
  
**/
IKEV2_CHILD_SA_SESSION *
Ikev2ChildSaSessionRemove (
  IN LIST_ENTRY           *SaSessionList,
  IN UINT32               Spi, 
  IN UINT8                ListType  
  );

/**
  Mark a specified Child SA Session as on deleting.

  @param[in]  ChildSaSession   Pointer to IKEV2_CHILD_SA_SESSION.

  @retval     EFI_SUCCESS      Operation is successful.

**/
EFI_STATUS
Ikev2ChildSaSessionOnDeleting (
  IN IKEV2_CHILD_SA_SESSION   *ChildSaSession
  );

/**
  Free the memory located for the specified IKEV2_CHILD_SA_SESSION. 

  @param[in]  ChildSaSession  Pointer to IKEV2_CHILD_SA_SESSION.

**/
VOID
Ikev2ChildSaSessionFree (
  IN IKEV2_CHILD_SA_SESSION   *ChildSaSession
  );

/**
  Free the specified DhBuffer.

  @param[in] DhBuffer   Pointer to IKEV2_DH_BUFFER to be freed.
  
**/
VOID
Ikev2DhBufferFree (
  IN IKEV2_DH_BUFFER *DhBuffer
  );

/**
  Delete the specified established Child SA.

  This function delete the Child SA directly and dont send the Information Packet to
  remote peer.

  @param[in]  IkeSaSession   Pointer to a IKE SA Session used to be searched for.
  @param[in]  Spi            SPI used to find the Child SA.

  @retval     EFI_NOT_FOUND  Pointer of IKE SA Session is NULL.
  @retval     EFI_NOT_FOUND  There is no specified Child SA related with the input
                             SPI under this IKE SA Session.
  @retval     EFI_SUCCESS    Delete the Child SA successfully.

**/
EFI_STATUS
Ikev2ChildSaSilentDelete (
  IN IKEV2_SA_SESSION       *IkeSaSession,
  IN UINT32                 Spi
  );

/**
  This function is to parse a request IKE packet and return its request type.
  The request type is one of IKE CHILD SA creation, IKE SA rekeying and 
  IKE CHILD SA rekeying.

  @param[in] IkePacket  IKE packet to be prased.

  return the type of the IKE packet.

**/
IKEV2_CREATE_CHILD_REQUEST_TYPE
Ikev2ChildExchangeRequestType(
  IN IKE_PACKET               *IkePacket
  );

/**
  This function finds the SPI from Create Child Sa Exchange Packet.
 
  @param[in] IkePacket       Pointer to IKE_PACKET to be searched.

  @retval SPI number.

**/
UINT32
Ikev2ChildExchangeRekeySpi(
  IN IKE_PACKET               *IkePacket
  );


/**
  Associate a SPD selector to the Child SA Session.

  This function is called when the Child SA is not the first child SA of its 
  IKE SA. It associate a SPD to this Child SA.

  @param[in, out]  ChildSaSession     Pointer to the Child SA Session to be associated to 
                                      a SPD selector.

  @retval EFI_SUCCESS        Associate one SPD selector to this Child SA Session successfully.
  @retval EFI_NOT_FOUND      Can't find the related SPD selector.

**/
EFI_STATUS
Ikev2ChildSaAssociateSpdEntry (
  IN OUT IKEV2_CHILD_SA_SESSION *ChildSaSession
  );

/**
  Validate the IKE header of received IKE packet.

  @param[in]   IkeSaSession  Pointer to IKEV2_SA_SESSION related to this IKE packet.
  @param[in]   IkeHdr        Pointer to IKE header of received IKE packet.

  @retval TRUE   If the IKE header is valid.
  @retval FALSE  If the IKE header is invalid.

**/
BOOLEAN
Ikev2ValidateHeader (
  IN IKEV2_SA_SESSION         *IkeSaSession,
  IN IKE_HEADER               *IkeHdr
  );

/**
  Create and intialize IKEV2_SA_DATA for speicifed IKEV2_SESSION_COMMON.

  This function will be only called by the initiator. The responder's IKEV2_SA_DATA
  will be generated during parsed the initiator packet.

  @param[in]  SessionCommon  Pointer to IKEV2_SESSION_COMMON related to.

  @retval a Pointer to a new IKEV2_SA_DATA or NULL.

**/
IKEV2_SA_DATA *
Ikev2InitializeSaData (
  IN IKEV2_SESSION_COMMON     *SessionCommon
  );

/**
  Store the SA into SAD.

  @param[in]  ChildSaSession  Pointer to IKEV2_CHILD_SA_SESSION.

**/
VOID
Ikev2StoreSaData (
  IN IKEV2_CHILD_SA_SESSION   *ChildSaSession
  );

/**
  Routine process before the payload decoding.

  @param[in] SessionCommon  Pointer to ChildSa SessionCommon.
  @param[in] PayloadBuf     Pointer to the payload.
  @param[in] PayloadSize    Size of PayloadBuf in byte.
  @param[in] PayloadType    Type of Payload.

**/
VOID
Ikev2ChildSaBeforeDecodePayload (
  IN UINT8 *SessionCommon,
  IN UINT8 *PayloadBuf,
  IN UINTN PayloadSize,
  IN UINT8 PayloadType
  );

/**
  Routine Process after the encode payload.

  @param[in] SessionCommon  Pointer to ChildSa SessionCommon.
  @param[in] PayloadBuf     Pointer to the payload.
  @param[in] PayloadSize    Size of PayloadBuf in byte.
  @param[in] PayloadType    Type of Payload.

**/
VOID
Ikev2ChildSaAfterEncodePayload (
  IN UINT8 *SessionCommon,
  IN UINT8 *PayloadBuf,
  IN UINTN PayloadSize,
  IN UINT8 PayloadType
  );

/**
  Generate Ikev2 SA payload according to SessionSaData

  @param[in] SessionSaData   The data used in SA payload.
  @param[in] NextPayload     The payload type presented in NextPayload field of 
                             SA Payload header.
  @param[in] Type            The SA type. It MUST be neither (1) for IKE_SA or
                             (2) for CHILD_SA or (3) for INFO.

  @retval a Pointer to SA IKE payload.
  
**/
IKE_PAYLOAD *
Ikev2GenerateSaPayload (
  IN IKEV2_SA_DATA    *SessionSaData,
  IN UINT8            NextPayload,
  IN IKE_SESSION_TYPE Type
  );

/**
  Generate a ID payload.

  @param[in] CommonSession   Pointer to IKEV2_SESSION_COMMON related to ID payload.
  @param[in] NextPayload     The payload type presented in the NextPayload field 
                             of ID Payload header.

  @retval Pointer to ID IKE payload.

**/
IKE_PAYLOAD *
Ikev2GenerateIdPayload (
  IN IKEV2_SESSION_COMMON *CommonSession,
  IN UINT8                NextPayload
  );

/**
  Generate a ID payload.

  @param[in] CommonSession   Pointer to IKEV2_SESSION_COMMON related to ID payload.
  @param[in] NextPayload     The payload type presented in the NextPayload field 
                             of ID Payload header.
  @param[in] InCert          Pointer to the Certificate which distinguished name
                             will be added into the Id payload.
  @param[in] CertSize        Size of the Certificate.

  @retval Pointer to ID IKE payload.

**/
IKE_PAYLOAD *
Ikev2GenerateCertIdPayload (
  IN IKEV2_SESSION_COMMON *CommonSession,
  IN UINT8                NextPayload, 
  IN UINT8                *InCert,
  IN UINTN                CertSize
  );

/**
  Generate a Nonce payload contenting the input parameter NonceBuf.

  @param[in]  NonceBuf       The nonce buffer content the whole Nonce payload block 
                            except the payload header.
  @param[in]  NonceSize      The buffer size of the NonceBuf
  @param[in]  NextPayload   The payload type presented in the NextPayload field 
                            of Nonce Payload header.

  @retval Pointer to Nonce IKE paload.

**/
IKE_PAYLOAD *
Ikev2GenerateNoncePayload (
  IN UINT8            *NonceBuf,
  IN UINTN            NonceSize,
  IN UINT8            NextPayload
  );

/**
  Generate the Notify payload.

  Since the structure of Notify payload which defined in RFC 4306 is simple, so
  there is no internal data structure for Notify payload. This function generate 
  Notify payload defined in RFC 4306, but all the fields in this payload are still 
  in host order and need call Ikev2EncodePayload() to convert those fields from 
  the host order to network order beforing sending it.

  @param[in]  ProtocolId        The protocol type ID. For IKE_SA it MUST be one (1).
                                For IPsec SAs it MUST be neither (2) for AH or (3)
                                for ESP.
  @param[in]  NextPayload       The next paylaod type in NextPayload field of 
                                the Notify payload.
  @param[in]  SpiSize           Size of the SPI in SPI size field of the Notify Payload.
  @param[in]  MessageType       The message type in NotifyMessageType field of the 
                                Notify Payload.
  @param[in]  SpiBuf            Pointer to buffer contains the SPI value.
  @param[in]  NotifyData        Pointer to buffer contains the notification data.
  @param[in]  NotifyDataSize    The size of NotifyData in bytes.
  

  @retval Pointer to IKE Notify Payload.

**/
IKE_PAYLOAD *
Ikev2GenerateNotifyPayload (
  IN UINT8            ProtocolId,
  IN UINT8            NextPayload,
  IN UINT8            SpiSize,
  IN UINT16           MessageType,
  IN UINT8            *SpiBuf,
  IN UINT8            *NotifyData,
  IN UINTN            NotifyDataSize
  );

/**
  Generate the Delete payload.

  Since the structure of Delete payload which defined in RFC 4306 is simple, 
  there is no internal data structure for Delete payload. This function generate 
  Delete payload defined in RFC 4306, but all the fields in this payload are still 
  in host order and need call Ikev2EncodePayload() to convert those fields from 
  the host order to network order beforing sending it.

  @param[in]  IkeSaSession      Pointer to IKE SA Session to be used of Delete payload generation.
  @param[in]  NextPayload       The next paylaod type in NextPayload field of 
                                the Delete payload.
  @param[in]  SpiSize           Size of the SPI in SPI size field of the Delete Payload.
  @param[in]  SpiNum            Number of SPI in NumofSPIs field of the Delete Payload.
  @param[in]  SpiBuf            Pointer to buffer contains the SPI value.

  @retval Pointer to IKE Delete Payload.

**/
IKE_PAYLOAD *
Ikev2GenerateDeletePayload (
  IN IKEV2_SA_SESSION  *IkeSaSession,
  IN UINT8             NextPayload,
  IN UINT8             SpiSize,
  IN UINT16            SpiNum,
  IN UINT8             *SpiBuf  
  );

/**
  Generate the Configuration payload.

  This function generates a configuration payload defined in RFC 4306, but all the 
  fields in this payload are still in host order and need call Ikev2EncodePayload() 
  to convert those fields from the host order to network order beforing sending it.

  @param[in]  IkeSaSession      Pointer to IKE SA Session to be used for Delete payload
                                generation.
  @param[in]  NextPayload       The next paylaod type in NextPayload field of 
                                the Delete payload.
  @param[in]  CfgType           The attribute type in the Configuration attribute.

  @retval Pointer to IKE CP Payload.

**/
IKE_PAYLOAD *
Ikev2GenerateCpPayload (
  IN IKEV2_SA_SESSION  *IkeSaSession,
  IN UINT8             NextPayload,
  IN UINT8             CfgType
  );

/**
  Generate a Authentication Payload.

  This function is used for both Authentication generation and verification. When the 
  IsVerify is TRUE, it create a Auth Data for verification. This function choose the 
  related IKE_SA_INIT Message for Auth data creation according to the IKE Session's type
  and the value of IsVerify parameter.

  @param[in]  IkeSaSession  Pointer to IKEV2_SA_SESSION related to.
  @param[in]  IdPayload     Pointer to the ID payload to be used for Authentication 
                            payload generation.
  @param[in]  NextPayload   The type filled into the Authentication Payload next 
                            payload field.
  @param[in]  IsVerify      If it is TURE, the Authentication payload is used for
                            verification.

  @return pointer to IKE Authentication payload for pre-shard key method.

**/
IKE_PAYLOAD *
Ikev2PskGenerateAuthPayload (
  IN IKEV2_SA_SESSION *IkeSaSession,
  IN IKE_PAYLOAD      *IdPayload,
  IN UINT8            NextPayload,
  IN BOOLEAN          IsVerify
  );

/**
  Generate a Authentication Payload for Certificate Auth method.  

  This function has two functions. One is creating a local Authentication 
  Payload for sending and other is creating the remote Authentication data 
  for verification when the IsVerify is TURE.

  @param[in]  IkeSaSession      Pointer to IKEV2_SA_SESSION related to.
  @param[in]  IdPayload         Pointer to the ID payload to be used for Authentication 
                                payload generation.
  @param[in]  NextPayload       The type filled into the Authentication Payload 
                                next payload field.
  @param[in]  IsVerify          If it is TURE, the Authentication payload is used 
                                for verification.
  @param[in]  UefiPrivateKey    Pointer to the UEFI private key. Ignore it when 
                                verify the authenticate payload.
  @param[in]  UefiPrivateKeyLen The size of UefiPrivateKey in bytes. Ignore it 
                                when verify the authenticate payload.
  @param[in]  UefiKeyPwd        Pointer to the password of UEFI private key. 
                                Ignore it when verify the authenticate payload.
  @param[in]  UefiKeyPwdLen     The size of UefiKeyPwd in bytes.Ignore it when 
                                verify the authenticate payload.

  @return pointer to IKE Authentication payload for certification method.

**/
IKE_PAYLOAD *
Ikev2CertGenerateAuthPayload (
  IN IKEV2_SA_SESSION *IkeSaSession,
  IN IKE_PAYLOAD      *IdPayload,
  IN UINT8            NextPayload,
  IN BOOLEAN          IsVerify,
  IN UINT8            *UefiPrivateKey,
  IN UINTN            UefiPrivateKeyLen,
  IN UINT8            *UefiKeyPwd,
  IN UINTN            UefiKeyPwdLen
  );

/**
  Generate TS payload.

  This function generates TSi or TSr payload according to type of next payload.
  If the next payload is Responder TS, gereate TSi Payload. Otherwise, generate
  TSr payload
  
  @param[in] ChildSa        Pointer to IKEV2_CHILD_SA_SESSION related to this TS payload.
  @param[in] NextPayload    The payload type presented in the NextPayload field 
                            of ID Payload header.
  @param[in] IsTunnel       It indicates that if the Ts Payload is after the CP payload.
                            If yes, it means the Tsi and Tsr payload should be with
                            Max port range and address range and protocol is marked
                            as zero.

  @retval Pointer to Ts IKE payload.

**/
IKE_PAYLOAD *
Ikev2GenerateTsPayload (
  IN IKEV2_CHILD_SA_SESSION *ChildSa,
  IN UINT8                  NextPayload,
  IN BOOLEAN                IsTunnel
  );

/**
  Parser the Notify Cookie payload.

  This function parses the Notify Cookie payload.If the Notify ProtocolId is not
  IPSEC_PROTO_ISAKMP or if the SpiSize is not zero or if the MessageType is not
  the COOKIE, return EFI_INVALID_PARAMETER.

  @param[in]      IkeNCookie    Pointer to the IKE_PAYLOAD which contians the 
                                Notify Cookie payload.
                                the Notify payload.
  @param[in, out] IkeSaSession  Pointer to the relevant IKE SA Session.

  @retval EFI_SUCCESS           The Notify Cookie Payload is valid.
  @retval EFI_INVALID_PARAMETER The Notify Cookie Payload is invalid.
  @retval EFI_OUT_OF_RESOURCE   The required resource can't be allocated.

**/
EFI_STATUS
Ikev2ParserNotifyCookiePayload (
  IN     IKE_PAYLOAD      *IkeNCookie,
  IN OUT IKEV2_SA_SESSION *IkeSaSession
  );

/**
  Generate the Certificate payload or Certificate Request Payload.

  Since the Certificate Payload structure is same with Certificate Request Payload, 
  the only difference is that one contains the Certificate Data, other contains
  the acceptable certificateion CA. This function generate Certificate payload 
  or Certificate Request Payload defined in RFC 4306, but all the fields 
  in the payload are still in host order and need call Ikev2EncodePayload() 
  to convert those fields from the host order to network order beforing sending it.

  @param[in]  IkeSaSession      Pointer to IKE SA Session to be used of Delete payload 
                                generation.
  @param[in]  NextPayload       The next paylaod type in NextPayload field of 
                                the Delete payload.
  @param[in]  Certificate       Pointer of buffer contains the certification data.
  @param[in]  CertificateLen    The length of Certificate in byte.
  @param[in]  EncodeType        Specified the Certificate Encodeing which is defined
                                in RFC 4306.
  @param[in]  IsRequest         To indicate create Certificate Payload or Certificate
                                Request Payload. If it is TURE, create Certificate
                                Request Payload. Otherwise, create Certificate Payload.

  @retval  a Pointer to IKE Payload whose payload buffer containing the Certificate
           payload or Certificated Request payload.

**/
IKE_PAYLOAD *
Ikev2GenerateCertificatePayload (
  IN IKEV2_SA_SESSION  *IkeSaSession,
  IN UINT8             NextPayload,
  IN UINT8             *Certificate,
  IN UINTN             CertificateLen,
  IN UINT8             EncodeType,
  IN BOOLEAN           IsRequest
  );
  
/**
  General interface of payload encoding.

  This function encode the internal data structure into payload which 
  is defined in RFC 4306. The IkePayload->PayloadBuf used to store both the input 
  payload and converted payload. Only the SA payload use the interal structure 
  to store the attribute. Other payload use structure which is same with the RFC 
  defined, for this kind payloads just do host order to network order change of 
  some fields.

  @param[in]      SessionCommon       Pointer to IKE Session Common used to encode the payload.
  @param[in, out] IkePayload          Pointer to IKE payload to be encode as input, and
                                      store the encoded result as output.

  @retval EFI_INVALID_PARAMETER  Meet error when encode the SA payload.
  @retval EFI_SUCCESS            Encode successfully.

**/
EFI_STATUS
Ikev2EncodePayload (
  IN     UINT8          *SessionCommon,
  IN OUT IKE_PAYLOAD    *IkePayload
  );

/**
  The general interface of decode Payload.

  This function convert the received Payload into internal structure.

  @param[in]      SessionCommon     Pointer to IKE Session Common to use for decoding.
  @param[in, out] IkePayload        Pointer to IKE payload to be decode as input, and
                                    store the decoded result as output. 

  @retval EFI_INVALID_PARAMETER  Meet error when decode the SA payload.
  @retval EFI_SUCCESS            Decode successfully.

**/
EFI_STATUS
Ikev2DecodePayload (
  IN     UINT8       *SessionCommon,
  IN OUT IKE_PAYLOAD *IkePayload
  );

/**
  Decrypt IKE packet.

  This function decrpt the Encrypted IKE packet and put the result into IkePacket->PayloadBuf.

  @param[in]      SessionCommon       Pointer to IKEV2_SESSION_COMMON containing 
                                      some parameter used during decrypting.
  @param[in, out] IkePacket           Point to IKE_PACKET to be decrypted as input, 
                                      and the decrypted reslult as output.
  @param[in, out] IkeType             The type of IKE. IKE_SA_TYPE, IKE_INFO_TYPE and
                                      IKE_CHILD_TYPE are supportted.

  @retval EFI_INVALID_PARAMETER      If the IKE packet length is zero or the 
                                     IKE packet length is not Algorithm Block Size
                                     alignment.
  @retval EFI_SUCCESS                Decrypt IKE packet successfully.
  
**/
EFI_STATUS
Ikev2DecryptPacket (
  IN     IKEV2_SESSION_COMMON *SessionCommon,
  IN OUT IKE_PACKET           *IkePacket,
  IN OUT UINTN                IkeType
  );

/**
  Encrypt IKE packet.

  This function encrypt IKE packet before sending it. The Encrypted IKE packet
  is put in to IKEV2 Encrypted Payload.
  
  @param[in]        SessionCommon     Pointer to IKEV2_SESSION_COMMON related to the IKE packet.
  @param[in, out]   IkePacket         Pointer to IKE packet to be encrypted.

  @retval      EFI_SUCCESS       Operation is successful.
  @retval      Others            OPeration is failed.

**/
EFI_STATUS
Ikev2EncryptPacket (
  IN     IKEV2_SESSION_COMMON *SessionCommon,
  IN OUT IKE_PACKET           *IkePacket
  );

/**
  Encode the IKE packet.

  This function put all Payloads into one payload then encrypt it if needed.

  @param[in]      SessionCommon      Pointer to IKEV2_SESSION_COMMON containing 
                                     some parameter used during IKE packet encoding.
  @param[in, out] IkePacket          Pointer to IKE_PACKET to be encoded as input, 
                                     and the encoded reslult as output.
  @param[in]      IkeType            The type of IKE. IKE_SA_TYPE, IKE_INFO_TYPE and
                                     IKE_CHILD_TYPE are supportted.

  @retval         EFI_SUCCESS        Encode IKE packet successfully.
  @retval         Otherwise          Encode IKE packet failed.

**/
EFI_STATUS
Ikev2EncodePacket (
  IN     IKEV2_SESSION_COMMON *SessionCommon,
  IN OUT IKE_PACKET           *IkePacket,
  IN     UINTN                IkeType
  );

/**
  Decode the IKE packet.

  This function first decrypts the IKE packet if needed , then separats the whole 
  IKE packet from the IkePacket->PayloadBuf into IkePacket payload list.
  
  @param[in]      SessionCommon          Pointer to IKEV1_SESSION_COMMON containing 
                                         some parameter used by IKE packet decoding.
  @param[in, out] IkePacket              The IKE Packet to be decoded on input, and 
                                         the decoded result on return.
  @param[in]      IkeType                The type of IKE. IKE_SA_TYPE, IKE_INFO_TYPE and
                                         IKE_CHILD_TYPE are supportted.

  @retval         EFI_SUCCESS            The IKE packet is decoded successfull.
  @retval         Otherwise              The IKE packet decoding is failed.

**/
EFI_STATUS
Ikev2DecodePacket (
  IN     IKEV2_SESSION_COMMON  *SessionCommon,
  IN OUT IKE_PACKET            *IkePacket,
  IN     UINTN                 IkeType
  );

/**
  Save some useful payloads after accepting the Packet.

  @param[in] SessionCommon   Pointer to IKEV2_SESSION_COMMON related to the operation.
  @param[in] IkePacket       Pointer to received IkePacet.
  @param[in] IkeType         The type used to indicate it is in IkeSa or ChildSa or Info
                             exchange.

**/
VOID
Ikev2OnPacketAccepted (
  IN IKEV2_SESSION_COMMON *SessionCommon,
  IN IKE_PACKET           *IkePacket,
  IN UINT8                IkeType
  );

/**
  Send out IKEV2 packet.

  @param[in]  IkeUdpService     Pointer to IKE_UDP_SERVICE used to send the IKE packet.
  @param[in]  SessionCommon     Pointer to IKEV1_SESSION_COMMON related to the IKE packet.
  @param[in]  IkePacket         Pointer to IKE_PACKET to be sent out.
  @param[in]  IkeType           The type of IKE to point what's kind of the IKE 
                                packet is to be sent out. IKE_SA_TYPE, IKE_INFO_TYPE 
                                and IKE_CHILD_TYPE are supportted.

  @retval     EFI_SUCCESS       The operation complete successfully.
  @retval     Otherwise         The operation is failed.

**/
EFI_STATUS
Ikev2SendIkePacket (
  IN IKE_UDP_SERVICE    *IkeUdpService,
  IN UINT8              *SessionCommon,
  IN IKE_PACKET         *IkePacket,
  IN UINTN              IkeType
  );

/**
  Callback function for the IKE life time is over.

  This function will mark the related IKE SA Session as deleting and trigger a 
  Information negotiation.

  @param[in]    Event     The time out event.
  @param[in]    Context   Pointer to data passed by caller.
  
**/
VOID
EFIAPI
Ikev2LifetimeNotify (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  );

/**
  This function will be called if the TimeOut Event is signaled.

  @param[in]  Event      The signaled Event.
  @param[in]  Context    The data passed by caller.

**/
VOID
EFIAPI
Ikev2ResendNotify (
  IN EFI_EVENT                 Event,
  IN VOID                      *Context
  );

/**
  Generate a Key Exchange payload according to the DH group type and save the 
  public Key into IkeSaSession IkeKey field.

  @param[in, out] IkeSaSession    Pointer of the IKE_SA_SESSION.
  @param[in]      NextPayload     The payload type presented in the NextPayload field of Key 
                                  Exchange Payload header.

  @retval Pointer to Key IKE payload.

**/
IKE_PAYLOAD *
Ikev2GenerateKePayload (
  IN OUT IKEV2_SA_SESSION *IkeSaSession, 
  IN     UINT8            NextPayload      
  );

/**
  Check if the SPD is related to the input Child SA Session.

  This function is the subfunction of Ikev1AssociateSpdEntry(). It is the call
  back function of IpSecVisitConfigData(). 
  

  @param[in]  Type               Type of the input Config Selector.
  @param[in]  Selector           Pointer to the Configure Selector to be checked. 
  @param[in]  Data               Pointer to the Configure Selector's Data passed 
                                 from the caller.
  @param[in]  SelectorSize       The buffer size of Selector.
  @param[in]  DataSize           The buffer size of the Data.
  @param[in]  Context            The data passed from the caller. It is a Child
                                 SA Session in this context.

  @retval EFI_SUCCESS        The SPD Selector is not related to the Child SA Session. 
  @retval EFI_ABORTED        The SPD Selector is related to the Child SA session and 
                             set the ChildSaSession->Spd to point to this SPD Selector.

**/
EFI_STATUS
Ikev2MatchSpdEntry (
  IN EFI_IPSEC_CONFIG_DATA_TYPE     Type,
  IN EFI_IPSEC_CONFIG_SELECTOR      *Selector,
  IN VOID                           *Data,
  IN UINTN                          SelectorSize,
  IN UINTN                          DataSize,
  IN VOID                           *Context
  );

/**
  Check if the Algorithm ID is supported.

  @param[in]  AlgorithmId The specified Algorithm ID.
  @param[in]  Type        The type used to indicate the Algorithm is for Encrypt or
                          Authentication.

  @retval     TRUE        If the Algorithm ID is supported.
  @retval     FALSE       If the Algorithm ID is not supported.

**/
BOOLEAN
Ikev2IsSupportAlg (
  IN UINT16 AlgorithmId,
  IN UINT8  Type
  );

/**
  Generate a ChildSa Session and insert it into related IkeSaSession.

  @param[in]  IkeSaSession    Pointer to related IKEV2_SA_SESSION.
  @param[in]  UdpService      Pointer to related IKE_UDP_SERVICE.

  @return pointer of IKEV2_CHILD_SA_SESSION.

**/
IKEV2_CHILD_SA_SESSION *
Ikev2ChildSaSessionCreate (
  IN IKEV2_SA_SESSION   *IkeSaSession,
  IN IKE_UDP_SERVICE     *UdpService
  ) ;

/**
  Parse the received Initial Exchange Packet.
  
  This function parse the SA Payload and Key Payload to find out the cryptographic 
  suite for the further IKE negotiation and fill it into the IKE SA Session's 
  CommonSession->SaParams.

  @param[in, out]  IkeSaSession  Pointer to related IKEV2_SA_SESSION.
  @param[in]       SaPayload     The received packet.
  @param[in]       Type          The received packet IKE header flag. 

  @retval          TRUE          If the SA proposal in Packet is acceptable.
  @retval          FALSE         If the SA proposal in Packet is not acceptable.

**/
BOOLEAN
Ikev2SaParseSaPayload (
  IN OUT IKEV2_SA_SESSION *IkeSaSession,
  IN     IKE_PAYLOAD      *SaPayload,
  IN     UINT8            Type
  );

/**
  Parse the received Authentication Exchange Packet.
  
  This function parse the SA Payload and Key Payload to find out the cryptographic
  suite for the ESP and fill it into the Child SA Session's CommonSession->SaParams.
  
  @param[in, out]  ChildSaSession  Pointer to IKEV2_CHILD_SA_SESSION related to 
                                   this Authentication Exchange.
  @param[in]       SaPayload       The received packet.
  @param[in]       Type            The IKE header's flag of received packet . 
  
  @retval          TRUE            If the SA proposal in Packet is acceptable.
  @retval          FALSE           If the SA proposal in Packet is not acceptable.

**/
BOOLEAN
Ikev2ChildSaParseSaPayload (
  IN OUT IKEV2_CHILD_SA_SESSION *ChildSaSession,
  IN     IKE_PAYLOAD            *SaPayload,
  IN     UINT8                  Type
  );

/**
  Generate Key buffer from fragments.

  If the digest length of specified HashAlgId is larger than or equal with the 
  required output key length, derive the key directly. Otherwise, Key Material 
  needs to be PRF-based concatenation according to 2.13 of RFC 4306: 
  prf+ (K,S) = T1 | T2 | T3 | T4 | ..., T1 = prf (K, S | 0x01),
  T2 = prf (K, T1 | S | 0x02), T3 = prf (K, T2 | S | 0x03),T4 = prf (K, T3 | S | 0x04)
  then derive the key from this key material.
  
  @param[in]       HashAlgId        The Hash Algorithm ID used to generate key.
  @param[in]       HashKey          Pointer to a key buffer which contains hash key.
  @param[in]       HashKeyLength    The length of HashKey in bytes.
  @param[in, out]  OutputKey        Pointer to buffer which is used to receive the 
                                    output key.
  @param[in]       OutputKeyLength  The length of OutPutKey buffer.
  @param[in]       Fragments        Pointer to the data to be used to generate key.
  @param[in]       NumFragments     The numbers of the Fragement.

  @retval EFI_SUCCESS            The operation complete successfully.
  @retval EFI_INVALID_PARAMETER  If NumFragments is zero.
  @retval EFI_OUT_OF_RESOURCES   If the required resource can't be allocated.
  @retval Others                 The operation is failed.

**/
EFI_STATUS
Ikev2SaGenerateKey (
  IN     UINT8                 HashAlgId,
  IN     UINT8                 *HashKey,
  IN     UINTN                 HashKeyLength,
  IN OUT UINT8                 *OutputKey,
  IN     UINTN                 OutputKeyLength,
  IN     PRF_DATA_FRAGMENT    *Fragments,
  IN     UINTN                 NumFragments
  );

/**
  Copy ChildSaSession->Spd->Selector to ChildSaSession->SpdSelector.

  ChildSaSession->SpdSelector stores the real Spdselector for its SA. Sometime,
  the SpdSelector in ChildSaSession is more accurated or the scope is smaller 
  than the one in ChildSaSession->Spd, especially for the tunnel mode.
    
  @param[in, out]  ChildSaSession  Pointer to IKEV2_CHILD_SA_SESSION related to.
  
**/
VOID
Ikev2ChildSaSessionSpdSelectorCreate (
  IN OUT IKEV2_CHILD_SA_SESSION *ChildSaSession
  );

extern IKE_ALG_GUID_INFO mIPsecEncrAlgInfo[];
#endif


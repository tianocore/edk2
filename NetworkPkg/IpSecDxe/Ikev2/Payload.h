/** @file
  The Definitions related to IKEv2 payload.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _IKE_V2_PAYLOAD_H_
#define _IKE_V2_PAYLOAD_H_

//
// Payload Type for IKEv2
//
#define IKEV2_PAYLOAD_TYPE_NONE     0
#define IKEV2_PAYLOAD_TYPE_SA       33
#define IKEV2_PAYLOAD_TYPE_KE       34
#define IKEV2_PAYLOAD_TYPE_ID_INIT  35
#define IKEV2_PAYLOAD_TYPE_ID_RSP   36
#define IKEV2_PAYLOAD_TYPE_CERT     37
#define IKEV2_PAYLOAD_TYPE_CERTREQ  38
#define IKEV2_PAYLOAD_TYPE_AUTH     39
#define IKEV2_PAYLOAD_TYPE_NONCE    40
#define IKEV2_PAYLOAD_TYPE_NOTIFY   41
#define IKEV2_PAYLOAD_TYPE_DELETE   42
#define IKEV2_PAYLOAD_TYPE_VENDOR   43
#define IKEV2_PAYLOAD_TYPE_TS_INIT  44
#define IKEV2_PAYLOAD_TYPE_TS_RSP   45
#define IKEV2_PAYLOAD_TYPE_ENCRYPT  46
#define IKEV2_PAYLOAD_TYPE_CP       47
#define IKEV2_PAYLOAD_TYPE_EAP      48

//
// IKE header Flag for IKEv2
//
#define IKE_HEADER_FLAGS_INIT       0x08
#define IKE_HEADER_FLAGS_RESPOND    0x20
#define IKE_HEADER_FLAGS_CHILD_INIT 0

//
// IKE Header Exchange Type for IKEv2
//
#define IKEV2_EXCHANGE_TYPE_INIT         34
#define IKEV2_EXCHANGE_TYPE_AUTH         35
#define IKEV2_EXCHANGE_TYPE_CREATE_CHILD 36
#define IKEV2_EXCHANGE_TYPE_INFO         37

#pragma pack(1)
typedef struct {
  UINT8   NextPayload;
  UINT8   Reserved;
  UINT16  PayloadLength;
} IKEV2_COMMON_PAYLOAD_HEADER;
#pragma pack()

#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  // 
  // Proposals
  //
} IKEV2_SA;
#pragma pack()

#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  UINT8                       ProposalIndex;
  UINT8                       ProtocolId;
  UINT8                       SpiSize;
  UINT8                       NumTransforms;
} IKEV2_PROPOSAL;
#pragma pack()

//
// IKEv2 Transform Type Values presented within Transform Payload
//
#define IKEV2_TRANSFORM_TYPE_ENCR      1  // Encryption Algorithm
#define IKEV2_TRANSFORM_TYPE_PRF       2  // Pseduo-Random Func
#define IKEV2_TRANSFORM_TYPE_INTEG     3  // Integrity Algorithm
#define IKEV2_TRANSFORM_TYPE_DH        4  // DH Group
#define IKEV2_TRANSFORM_TYPE_ESN       5  // Extended Sequence Number

//
// IKEv2 Transform ID for Encrypt Algorithm (ENCR)
// 
#define IKEV2_TRANSFORM_ID_ENCR_DES_IV64 1
#define IKEV2_TRANSFORM_ID_ENCR_DES      2
#define IKEV2_TRANSFORM_ID_ENCR_3DES     3
#define IKEV2_TRANSFORM_ID_ENCR_RC5      4
#define IKEV2_TRANSFORM_ID_ENCR_IDEA     5
#define IKEV2_TRANSFORM_ID_ENCR_CAST     6
#define IKEV2_TRANSFORM_ID_ENCR_BLOWFISH 7
#define IKEV2_TRANSFORM_ID_ENCR_3IDEA    8
#define IKEV2_TRANSFORM_ID_ENCR_DES_IV32 9
#define IKEV2_TRANSFORM_ID_ENCR_NULL     11
#define IKEV2_TRANSFORM_ID_ENCR_AES_CBC  12
#define IKEV2_TRANSFORM_ID_ENCR_AES_CTR  13

//
// IKEv2 Transform ID for Pseudo-Random Function (PRF)
//
#define IKEV2_TRANSFORM_ID_PRF_HMAC_MD5     1
#define IKEV2_TRANSFORM_ID_PRF_HMAC_SHA1    2
#define IKEV2_TRANSFORM_ID_PRF_HMAC_TIGER   3
#define IKEV2_TRANSFORM_ID_PRF_AES128_XCBC  4

//
// IKEv2 Transform ID for Integrity Algorithm (INTEG)
//
#define IKEV2_TRANSFORM_ID_AUTH_NONE              0
#define IKEV2_TRANSFORM_ID_AUTH_HMAC_MD5_96       1
#define IKEV2_TRANSFORM_ID_AUTH_HMAC_SHA1_96      2
#define IKEV2_TRANSFORM_ID_AUTH_HMAC_DES_MAC      3
#define IKEV2_TRANSFORM_ID_AUTH_HMAC_KPDK_MD5     4
#define IKEV2_TRANSFORM_ID_AUTH_HMAC_AES_XCBC_96  5

//
// IKEv2 Transform ID for Diffie-Hellman Group (DH)
//
#define IKEV2_TRANSFORM_ID_DH_768MODP             1
#define IKEV2_TRANSFORM_ID_DH_1024MODP            2
#define IKEV2_TRANSFORM_ID_DH_2048MODP            14

//
// IKEv2 Attribute Type Values
//
#define IKEV2_ATTRIBUTE_TYPE_KEYLEN               14

//
// Transform Payload
//
#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  UINT8                       TransformType;
  UINT8                       Reserved;
  UINT16                      TransformId;
  //
  // SA Attributes
  //
} IKEV2_TRANSFORM;
#pragma pack()

#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  UINT16                      DhGroup;
  UINT16                      Reserved;
  //
  // Remaining part contains the key exchanged
  //
} IKEV2_KEY_EXCHANGE;
#pragma pack()

//
// Identification Type Values presented within Ikev2 ID payload
//
#define IKEV2_ID_TYPE_IPV4_ADDR        1
#define IKEV2_ID_TYPE_FQDN             2
#define IKEV2_ID_TYPE_RFC822_ADDR      3
#define IKEV2_ID_TYPE_IPV6_ADDR        5
#define IKEV2_ID_TYPE_DER_ASN1_DN      9
#define IKEV2_ID_TYPE_DER_ASN1_GN      10
#define IKEV2_ID_TYPE_KEY_ID           11

//
// Identification Payload
//
#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  UINT8                       IdType;
  UINT8                       Reserver1;
  UINT16                      Reserver2;
  //
  // Identification Data
  //
} IKEV2_ID;
#pragma pack()

//
// Encoding Type presented in IKEV2 Cert Payload
//
#define IKEV2_CERT_ENCODEING_RESERVED                  0
#define IKEV2_CERT_ENCODEING_X509_CERT_WRAP            1
#define IKEV2_CERT_ENCODEING_PGP_CERT                  2
#define IKEV2_CERT_ENCODEING_DNS_SIGN_KEY              3
#define IKEV2_CERT_ENCODEING_X509_CERT_SIGN            4
#define IKEV2_CERT_ENCODEING_KERBEROS_TOKEN            6
#define IKEV2_CERT_ENCODEING_REVOCATION_LIST_CERT      7
#define IKEV2_CERT_ENCODEING_AUTH_REVOCATION_LIST      8
#define IKEV2_CERT_ENCODEING_SPKI_CERT                 9
#define IKEV2_CERT_ENCODEING_X509_CERT_ATTRIBUTE       10
#define IKEV2_CERT_ENCODEING_RAW_RSA_KEY               11
#define IKEV2_CERT_ENCODEING_HASH_AND_URL_OF_X509_CERT 12

//
// IKEV2 Certificate Payload
//
#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  UINT8                       CertEncoding;
  //
  // Cert Data
  //
} IKEV2_CERT;
#pragma pack()

//
// IKEV2 Certificate Request Payload
//
#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  UINT8                       CertEncoding;
  //
  // Cert Authority
  //
} IKEV2_CERT_REQ;
#pragma pack()

//
// Authentication Payload
//
#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  UINT8                       AuthMethod;
  UINT8                       Reserved1;
  UINT16                      Reserved2;
  //
  // Auth Data
  //
} IKEV2_AUTH;
#pragma pack()

//
// Authmethod in Authentication Payload
//
#define IKEV2_AUTH_METHOD_RSA        1; // RSA Digital Signature
#define IKEV2_AUTH_METHOD_SKMI       2; // Shared Key Message Integrity
#define IKEV2_AUTH_METHOD_DSS        3; // DSS Digital Signature

//
// IKEv2 Nonce Payload
//
#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  //
  // Nonce Data
  //
} IKEV2_NONCE;
#pragma pack()

//
// Notification Payload
//
#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  UINT8                       ProtocolId;
  UINT8                       SpiSize;
  UINT16                      MessageType;
  //
  // SPI and Notification Data
  //
} IKEV2_NOTIFY;
#pragma pack()

//
//  Notify Message Types presented within IKEv2 Notify Payload
//
#define IKEV2_NOTIFICATION_UNSUPPORT_CRITICAL_PAYLOAD       1
#define IKEV2_NOTIFICATION_INVALID_IKE_SPI                  4
#define IKEV2_NOTIFICATION_INVALID_MAJOR_VERSION            5
#define IKEV2_NOTIFICATION_INVALID_SYNTAX                   7
#define IKEV2_NOTIFICATION_INVALID_MESSAGE_ID               9
#define IKEV2_NOTIFICATION_INVALID_SPI                     11
#define IKEV2_NOTIFICATION_NO_PROPOSAL_CHOSEN              14
#define IKEV2_NOTIFICATION_INVALID_KEY_PAYLOAD             17
#define IKEV2_NOTIFICATION_AUTHENTICATION_FAILED           24
#define IKEV2_NOTIFICATION_SINGLE_PAIR_REQUIRED            34
#define IKEV2_NOTIFICATION_NO_ADDITIONAL_SAS               35
#define IKEV2_NOTIFICATION_INTERNAL_ADDRESS_FAILURE        36
#define IKEV2_NOTIFICATION_FAILED_CP_REQUIRED              37
#define IKEV2_NOTIFICATION_TS_UNCCEPTABLE                  38
#define IKEV2_NOTIFICATION_INVALID_SELECTORS               39
#define IKEV2_NOTIFICATION_COOKIE                          16390
#define IKEV2_NOTIFICATION_USE_TRANSPORT_MODE              16391
#define IKEV2_NOTIFICATION_REKEY_SA                        16393

//
// IKEv2 Protocol ID
//
//
// IKEv2 Delete Payload
//
#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  UINT8                       ProtocolId;
  UINT8                       SpiSize;
  UINT16                      NumSpis;
  //
  // SPIs
  //
} IKEV2_DELETE;
#pragma pack()

//
// Traffic Selector Payload
//
#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  UINT8                       TSNumbers;
  UINT8                       Reserved1;
  UINT16                      Reserved2;
  //
  // Traffic Selector
  //
} IKEV2_TS;
#pragma pack()

//
// Traffic Selector
//
#pragma pack(1)
typedef struct {
  UINT8                       TSType;
  UINT8                       IpProtocolId;
  UINT16                      SelecorLen;
  UINT16                      StartPort;
  UINT16                      EndPort;
  //
  // Starting Address && Ending Address
  //
} TRAFFIC_SELECTOR;
#pragma pack()

//
// Ts Type in Traffic Selector
//
#define IKEV2_TS_TYPE_IPV4_ADDR_RANGE     7
#define IKEV2_TS_TYPS_IPV6_ADDR_RANGE     8

//
// Vendor Payload
//
#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  //
  // Vendor ID
  //
} IKEV2_VENDOR;
#pragma pack()

//
// Encrypted Payload
//
#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  //
  // IV, Encrypted IKE Payloads, Padding, PAD length, Integrity CheckSum
  //
} IKEV2_ENCRYPTED;
#pragma pack()

#pragma pack(1)
typedef struct {
  UINT8 PadLength;
} IKEV2_PAD_LEN;
#pragma pack()

//
// Configuration Payload
//
#pragma pack(1)
typedef struct {
  IKEV2_COMMON_PAYLOAD_HEADER Header;
  UINT8                       CfgType;
  UINT8                       Reserve1;
  UINT16                      Reserve2;
  //
  // Configuration Attributes
  //
} IKEV2_CFG;
#pragma pack()

//
// Configuration Payload CPG type
//
#define IKEV2_CFG_TYPE_REQUEST    1
#define IKEV2_CFG_TYPE_REPLY      2
#define IKEV2_CFG_TYPE_SET        3
#define IKEV2_CFG_TYPE_ACK        4

//
// Configuration Attributes
//
#pragma pack(1)
typedef struct {
  UINT16    AttritType;
  UINT16    ValueLength;
} IKEV2_CFG_ATTRIBUTES;
#pragma pack()

//
// Configuration Attributes
//
#define IKEV2_CFG_ATTR_INTERNAL_IP4_ADDRESS      1
#define IKEV2_CFG_ATTR_INTERNAL_IP4_NBTMASK      2
#define IKEV2_CFG_ATTR_INTERNAL_IP4_DNS          3
#define IKEV2_CFG_ATTR_INTERNAL_IP4_NBNS         4
#define IKEV2_CFG_ATTR_INTERNA_ADDRESS_BXPIRY    5
#define IKEV2_CFG_ATTR_INTERNAL_IP4_DHCP         6
#define IKEV2_CFG_ATTR_APPLICATION_VERSION       7
#define IKEV2_CFG_ATTR_INTERNAL_IP6_ADDRESS      8
#define IKEV2_CFG_ATTR_INTERNAL_IP6_DNS          10
#define IKEV2_CFG_ATTR_INTERNAL_IP6_NBNS         11
#define IKEV2_CFG_ATTR_INTERNAL_IP6_DHCP         12
#define IKEV2_CFG_ATTR_INTERNAL_IP4_SUBNET       13
#define IKEV2_CFG_ATTR_SUPPORTED_ATTRIBUTES      14
#define IKEV2_CFG_ATTR_IP6_SUBNET                15

#endif


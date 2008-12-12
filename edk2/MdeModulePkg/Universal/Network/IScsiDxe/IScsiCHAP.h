/** @file
  The header file of CHAP configuration.

Copyright (c) 2004 - 2007, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ISCSI_CHAP_H_
#define _ISCSI_CHAP_H_

#define ISCSI_CHAP_AUTH_INFO_GUID \
  { \
    0x786ec0ac, 0x65ae, 0x4d1b, {0xb1, 0x37, 0xd, 0x11, 0xa, 0x48, 0x37, 0x97} \
  }

extern EFI_GUID mIScsiCHAPAuthInfoGuid;

#define ISCSI_AUTH_METHOD_CHAP    "CHAP"

#define ISCSI_KEY_CHAP_ALGORITHM  "CHAP_A"
#define ISCSI_KEY_CHAP_IDENTIFIER "CHAP_I"
#define ISCSI_KEY_CHAP_CHALLENGE  "CHAP_C"
#define ISCSI_KEY_CHAP_NAME       "CHAP_N"
#define ISCSI_KEY_CHAP_RESPONSE   "CHAP_R"

#define ISCSI_CHAP_ALGORITHM_MD5  5

#define ISCSI_CHAP_AUTH_MAX_LEN   1024
///
/// MD5_HASHSIZE
///
#define ISCSI_CHAP_RSP_LEN        16  
typedef enum {
  ISCSI_CHAP_INITIAL,
  ISCSI_CHAP_STEP_ONE,
  ISCSI_CHAP_STEP_TWO,
  ISCSI_CHAP_STEP_THREE,
  ISCSI_CHAP_STEP_FOUR
} ISCSI_CHAP_STEP;

#pragma pack(1)

typedef struct _ISCSI_CHAP_AUTH_CONFIG_NVDATA {
  UINT8 CHAPType;
  CHAR8 CHAPName[ISCSI_CHAP_NAME_MAX_LEN];
  CHAR8 CHAPSecret[ISCSI_CHAP_SECRET_MAX_LEN];
  CHAR8 ReverseCHAPName[ISCSI_CHAP_NAME_MAX_LEN];
  CHAR8 ReverseCHAPSecret[ISCSI_CHAP_SECRET_MAX_LEN];
} ISCSI_CHAP_AUTH_CONFIG_NVDATA;

#pragma pack()

///
/// ISCSI CHAP Authentication Data
///
typedef struct _ISCSI_CHAP_AUTH_DATA {
  ISCSI_CHAP_AUTH_CONFIG_NVDATA AuthConfig;
  UINT32                        InIdentifier;
  UINT8                         InChallenge[ISCSI_CHAP_AUTH_MAX_LEN];
  UINT32                        InChallengeLength;
  //
  // Calculated CHAP Response (CHAP_R) value
  //
  UINT8                         CHAPResponse[ISCSI_CHAP_RSP_LEN];

  //
  // Auth-data to be sent out for mutual authentication
  //
  UINT32                        OutIdentifier;
  UINT8                         OutChallenge[ISCSI_CHAP_AUTH_MAX_LEN];
  UINT32                        OutChallengeLength;
} ISCSI_CHAP_AUTH_DATA;

/**
  This function checks the received iSCSI Login Response during the security
  negotiation stage.
  
  @param[in] Conn             The iSCSI connection.
  @param[in] Transit          The transit flag of the latest iSCSI Login Response.

  @retval EFI_SUCCESS          The Login Response passed the CHAP validation.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.
  @retval EFI_PROTOCOL_ERROR   Some kind of protocol error happend.
  @retval Others               Other errors as indicated.
**/
EFI_STATUS
IScsiCHAPOnRspReceived (
  IN ISCSI_CONNECTION  *Conn,
  IN BOOLEAN           Transit
  );

/**
  This function fills the CHAP authentication information into the login PDU
  during the security negotiation stage in the iSCSI connection login.

  @param[in]  Conn             The iSCSI connection.
  @param[in]  Pdu              The PDU to send out.

  @retval EFI_SUCCESS          All check passed and the phase-related CHAP
                               authentication info is filled into the iSCSI PDU.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.
  @retval EFI_PROTOCOL_ERROR   Some kind of protocol error happend.
**/
EFI_STATUS
IScsiCHAPToSendReq (
  IN ISCSI_CONNECTION  *Conn,
  IN NET_BUF           *Pdu
  );

#endif

/** @file
  The header file of CHAP configuration.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ISCSI_CHAP_H_
#define _ISCSI_CHAP_H_

#define ISCSI_AUTH_METHOD_CHAP  "CHAP"

#define ISCSI_KEY_CHAP_ALGORITHM   "CHAP_A"
#define ISCSI_KEY_CHAP_IDENTIFIER  "CHAP_I"
#define ISCSI_KEY_CHAP_CHALLENGE   "CHAP_C"
#define ISCSI_KEY_CHAP_NAME        "CHAP_N"
#define ISCSI_KEY_CHAP_RESPONSE    "CHAP_R"

//
// Identifiers of supported CHAP hash algorithms:
// https://www.iana.org/assignments/ppp-numbers/ppp-numbers.xhtml#ppp-numbers-9
//
#define ISCSI_CHAP_ALGORITHM_MD5     5
#define ISCSI_CHAP_ALGORITHM_SHA256  7

//
// Byte count of the largest digest over the above-listed
// ISCSI_CHAP_ALGORITHM_* hash algorithms.
//
#define ISCSI_CHAP_MAX_DIGEST_SIZE  SHA256_DIGEST_SIZE

#define ISCSI_CHAP_STEP_ONE    1
#define ISCSI_CHAP_STEP_TWO    2
#define ISCSI_CHAP_STEP_THREE  3
#define ISCSI_CHAP_STEP_FOUR   4

#pragma pack(1)

typedef struct _ISCSI_CHAP_AUTH_CONFIG_NVDATA {
  UINT8    CHAPType;
  CHAR8    CHAPName[ISCSI_CHAP_NAME_STORAGE];
  CHAR8    CHAPSecret[ISCSI_CHAP_SECRET_STORAGE];
  CHAR8    ReverseCHAPName[ISCSI_CHAP_NAME_STORAGE];
  CHAR8    ReverseCHAPSecret[ISCSI_CHAP_SECRET_STORAGE];
} ISCSI_CHAP_AUTH_CONFIG_NVDATA;

#pragma pack()

//
// Typedefs for collecting sets of hash APIs from BaseCryptLib.
//
typedef
UINTN
(EFIAPI *CHAP_HASH_GET_CONTEXT_SIZE)(
  VOID
  );

typedef
BOOLEAN
(EFIAPI *CHAP_HASH_INIT)(
  OUT VOID *Context
  );

typedef
BOOLEAN
(EFIAPI *CHAP_HASH_UPDATE)(
  IN OUT VOID       *Context,
  IN     CONST VOID *Data,
  IN     UINTN      DataSize
  );

typedef
BOOLEAN
(EFIAPI *CHAP_HASH_FINAL)(
  IN OUT VOID  *Context,
  OUT    UINT8 *HashValue
  );

typedef struct {
  UINT8                         Algorithm;   // ISCSI_CHAP_ALGORITHM_*, CHAP_A
  UINT32                        DigestSize;
  CHAP_HASH_GET_CONTEXT_SIZE    GetContextSize;
  CHAP_HASH_INIT                Init;
  CHAP_HASH_UPDATE              Update;
  CHAP_HASH_FINAL               Final;
} CHAP_HASH;

///
/// ISCSI CHAP Authentication Data
///
typedef struct _ISCSI_CHAP_AUTH_DATA {
  ISCSI_CHAP_AUTH_CONFIG_NVDATA    *AuthConfig;
  UINT32                           InIdentifier;
  UINT8                            InChallenge[1024];
  UINT32                           InChallengeLength;
  //
  // The hash algorithm (CHAP_A) that the target selects in
  // ISCSI_CHAP_STEP_TWO.
  //
  CONST CHAP_HASH                  *Hash;
  //
  // Calculated CHAP Response (CHAP_R) value.
  //
  UINT8                            CHAPResponse[ISCSI_CHAP_MAX_DIGEST_SIZE];

  //
  // Auth-data to be sent out for mutual authentication.
  //
  // While the challenge size is technically independent of the hashing
  // algorithm, it is good practice to avoid hashing *fewer bytes* than the
  // digest size. In other words, it's good practice to feed *at least as many
  // bytes* to the hashing algorithm as the hashing algorithm will output.
  //
  UINT32    OutIdentifier;
  UINT8     OutChallenge[ISCSI_CHAP_MAX_DIGEST_SIZE];
} ISCSI_CHAP_AUTH_DATA;

/**
  This function checks the received iSCSI Login Response during the security
  negotiation stage.

  @param[in] Conn             The iSCSI connection.

  @retval EFI_SUCCESS          The Login Response passed the CHAP validation.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.
  @retval EFI_PROTOCOL_ERROR   Some kind of protocol error occurred.
  @retval Others               Other errors as indicated.

**/
EFI_STATUS
IScsiCHAPOnRspReceived (
  IN ISCSI_CONNECTION  *Conn
  );

/**
  This function fills the CHAP authentication information into the login PDU
  during the security negotiation stage in the iSCSI connection login.

  @param[in]       Conn        The iSCSI connection.
  @param[in, out]  Pdu         The PDU to send out.

  @retval EFI_SUCCESS           All check passed and the phase-related CHAP
                                authentication info is filled into the iSCSI
                                PDU.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
  @retval EFI_PROTOCOL_ERROR    Some kind of protocol error occurred.

**/
EFI_STATUS
IScsiCHAPToSendReq (
  IN      ISCSI_CONNECTION  *Conn,
  IN OUT  NET_BUF           *Pdu
  );

/**
  Initialize the CHAP_A=<A1,A2...> *value* string for the entire driver, to be
  sent by the initiator in ISCSI_CHAP_STEP_ONE.

  This function sanity-checks the internal table of supported CHAP hashing
  algorithms, as well.
**/
VOID
IScsiCHAPInitHashList (
  VOID
  );

#endif

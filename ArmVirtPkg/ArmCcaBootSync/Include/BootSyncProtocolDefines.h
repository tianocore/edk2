/** @file
  Arm CCA Boot Sync Protocol defines.

  Copyright (c) 2023 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - BS           - Boot Sync
    - BIB          - Boot information blob

  @par Reference(s):
   - Realm Host Interface (RHI) Specification, version 1.0-alp0
     (https://developer.arm.com/documentation/den0148/)
**/

#pragma once

#include <Uefi/UefiBaseType.h>

/**
  Macros defining the Key, Salt, IV and Tag sizes in bytes.
*/
#define AES_KEY_SIZE  32
#define SALT_SIZE     32
#define IV_SIZE       12
#define TAG_SIZE      16

/** A Macro defining the BIB Protocol version.
*/
#define ARM_CCA_BIB_PROTOCOL_VERSION  0

/** Macros defining the Attestation Report Verification result.
*/
#define ATTESTATION_RESULT_VERIFY_FAILURE  0
#define ATTESTATION_RESULT_VERIFY_SUCCESS  1

/** Macro defining the reason for end of communication as end of transmission.
*/
#define BOOT_SYNC_COMM_END_REASON_EOT      0
#define BOOT_SYNC_COMM_END_PROTOCOL_ERROR  1

/** Macros defining the BIB request options.
*/
#define BIB_REQUEST_OPTION_VARIABLE_DATA  BIT0
#define BIB_REQUEST_OPTION_SECRET_DATA    BIT1

#pragma pack(1)

/** A common data structure prefix for all BIB protocol messages.
*/
typedef struct BootSyncGuidBlob {
  /// GUID identifying the message type.
  EFI_GUID    Name;

  /// Length in bytes of the entire message.
  UINT32      Length;

  // UINT8  Data[n]; // GUID specific data
} BOOT_SYNC_GUID_BLOB;

/** A data structure sent from the Realm to the User Context to
    initiate the BIB prototol Key Exchange.

*/
typedef struct BootSyncKeyExchangeReq {
  /// A header with the GUID:gArmBootSyncKeyXchgReqGuid
  BOOT_SYNC_GUID_BLOB    Header;

  /// The BIB protocol version in the Realm.
  UINT32                 Version;

  /// Salt for Binding Key.
  UINT8                  SaltKeyBinding[SALT_SIZE];

  /// Salt for Encryption Key.
  UINT8                  SaltKeyEncryption[SALT_SIZE];

  /** Size of the Realm's PEM formatted
      Public Key data that follows.
  */
  UINT32                 PemdataLen;

  // UINT8              PemData[n];
} BOOT_SYNC_KEY_XCHG_REQ;

/** A data structure sent from User Context to the Real to
    finalise the BIB protocol Key Exchange.
 */
typedef struct BootSyncKeyExchangeResp {
  /// A header with the GUID:gArmBootSyncKeyXchgRespGuid
  BOOT_SYNC_GUID_BLOB    Header;

  /// The BIB protocol version in the User Context.
  UINT32                 Version;

  /** Size of the User Context's PEM formatted
      Public Key data that follows.
  */
  UINT32                 PemdataLen;

  // UINT8              PemData[n];
} BOOT_SYNC_KEY_XCHG_RESP;

/** A common data structure prefix for all BIB protocol messages that
    contain an encrypted payload.
*/
typedef struct BootSyncEncryptedData {
  /// A header with the GUID:gArmBootSyncKeyEncData
  BOOT_SYNC_GUID_BLOB    Header;

  /// Initial vector.
  UINT8                  Iv[IV_SIZE];

  /// The size of the encrypted data payload that follows.
  UINT32                 EncDataLen;

  /// The AEAD AES GCM authentication tag.
  UINT8                  Tag[TAG_SIZE];

  // UINT8     EncData[n];
} BOOT_SYNC_ENCRYPTED_DATA;

/** A common data structure used to define the multi element data block
    used in the BIB protocol.
*/
typedef struct BootSyncBsbHeader {
  /** A header with the GUID that identifies
      the request or response message.
  */
  BOOT_SYNC_GUID_BLOB    Header;

  /// Number of following BSB elements.
  UINT32                 ElementCount;

  // BOOT_SYNC_BSB_ELEMENT  Element[n];
} BOOT_SYNC_BSB_HEADER;

/** A common data structure used to identify and package a data packet
    within a BIB protocol message.
*/
typedef struct BootSyncBsbElement {
  /** A header with the GUID that identifies
      the relevant payload that follows.
  */
  BOOT_SYNC_GUID_BLOB    Header;

  // UINT8  Data[n]; // BSB Element specific data
} BOOT_SYNC_BSB_ELEMENT;

/** A data structure sent from either party in the BIB
    protocol requesting a FIN (termination of communication)
    or to communicate a negetive acknowledgement in response
    to any request message indicating that the protocol state
    is incorrect.
*/
typedef struct BootSyncFinNack {
  /** A header with either the
      GUID:gArmBootSyncFinGuid or
      GUID:gArmBootSyncNackGuid
  */
  BOOT_SYNC_GUID_BLOB    Header;

  /// The reason for the FIN or NACK message.
  UINT64                 Reason;
} BOOT_SYNC_FIN, BOOT_SYNC_NACK;

/** A data structure sent by the User Context to communicate
    the result of the Attestation Report verification.
*/
typedef struct BootSyncAttResult {
  /// A header with the GUID:gArmBootSyncAttResult
  BOOT_SYNC_BSB_ELEMENT    ElementHdr;

  /** The result of the attestation
      report verification.
  */
  UINT64                   Result;
} BOOT_SYNC_ATTESTATION_RESULT;

typedef struct BootSyncRequestOptions {
  /// A header with the GUID:gArmBootSyncRequestOptions
  BOOT_SYNC_BSB_ELEMENT    ElementHdr;

  /// The BIB request options.
  UINT64                   Options;
} BOOT_SYNC_REQUEST_OPTIONS;

#pragma pack()

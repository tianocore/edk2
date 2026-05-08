/** @file
  Arm CCA token definitions.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-rel0
     (https://developer.arm.com/documentation/den0137/)
**/

#pragma once

/** Macros defining the COSE Sign1 tags
   COSE_Sign1_Tagged = #6.18(COSE_Sign1)
*/
#define COSE_SIGN1_TAG  (18ULL)

/** Macro defining the CCA token collection tag.
  cca-token = #6.399(cca-token-collection) ; CMW Collection
*/
#define CCA_TOKEN_COLLECTION_TAG_REV0  (399ULL)

/** Macro defining the CCA token collection tag.
  cca-token = #6.907(cca-token-CMW) ; CMW Collection
*/
#define CCA_TOKEN_COLLECTION_TAG_REV1  (907ULL)

/** Macro defining the CoAP application/eat+cwt type.
  CMW-CWT-element = (
    [
      263, ; CoAP application/eat+cwt type
      EAT_CWT
    ]
  )
*/
#define CCA_TOKEN_COAP_TYPE  (263ULL)

/** Macros defining the CCA Platform and Realm Delegated token.
  cca-token-collection = {
    44234 => cca-platform-token ; 44234 = 0xACCA
    44241 => cca-realm-delegated-token
  }
*/
#define CCA_PLAT_TOKEN             (44234LL)           // 0xACCA
#define CCA_REALM_DELEGATED_TOKEN  (44241LL)

/* Macros defining the labels for cca-platform-claims.
  cca-platform-claims = (cca-platform-claim-map)
  cca-platform-claim-map = {
    cca-platform-profile
    cca-platform-challenge
    cca-platform-implementation-id
    cca-platform-instance-id
    cca-platform-config
    cca-platform-lifecycle
    cca-platform-sw-components
    ? cca-platform-verification-service
    cca-platform-hash-algo-id
  }
*/
// cca-platform-profile-label = 265 ; EAT profile
#define CCA_PLAT_PROFILE_LABEL  (265LL)

// cca-platform-challenge-label = 10 ; EAT nonce
#define CCA_PLAT_CHALLENGE_LABEL  (10LL)

// cca-platform-implementation-id-label = 2396 ; PSA implementation ID
#define CCA_PLAT_IMPLEMENTATION_ID_LABEL  (2396LL)

// cca-platform-instance-id-label = 256 ; EAT ueid
#define CCA_PLAT_INSTANCE_ID_LABEL  (256LL)

// cca-platform-config-label = 2401 ; PSA platform range
#define CCA_PLAT_CONFIG_LABEL  (2401LL)

// cca-platform-lifecycle-label = 2395 ; PSA lifecycle
#define CCA_PLAT_LIFECYCLE_LABEL  (2395LL)

// cca-platform-sw-components-label = 2399 ; PSA software components
#define CCA_PLAT_SW_COMPONENTS_LABEL  (2399LL)

// cca-platform-verification-service-label = 2400 ; PSA verification service
#define CCA_PLAT_VERIFICATION_SERVICE_LABEL  (2400LL)

// cca-platform-hash-algo-id-label = 2402 ; PSA platform range
#define CCA_PLAT_HASH_ALGO_ID_LABEL  (2402LL)

/* Macros defining the labels for cca-realm-claims.
  cca-realm-claims = (cca-realm-claim-map)
    cca-realm-claim-map = {
      cca-realm-challenge
      ? cca-realm-profile
      cca-realm-personalization-value
      cca-realm-initial-measurement
      cca-realm-extensible-measurements
      cca-realm-hash-algo-id
      cca-realm-public-key
      cca-realm-public-key-hash-algo-id
    }
*/
// cca-realm-challenge-label = 10
#define CCA_REALM_CHALLENGE_LABEL  (10LL)           /* EAT nonce */
// cca-realm-challenge-type = bytes .size 64
#define CCA_REALM_CHALLENGE_SIZE  (64)            /* EAT nonce */

// cca-realm-profile-label = 265 ; EAT profile
#define CCA_REALM_PROFILE_LABEL  (265LL)             /* EAT profile */

// cca-realm-personalization-value-label = 44235
#define CCA_REALM_PERSONALIZATION_VALUE_LABEL  (44235LL)
// cca-realm-personalization-value-type = bytes .size 64
#define CCA_REALM_PERSONALIZATION_SIZE  (64)

// cca-realm-initial-measurement-label = 44238
#define CCA_REALM_INITIAL_MEASUREMENT_LABEL  (44238LL)
// cca-realm-measurement-type = bytes .size 32 / bytes .size 48 / bytes .size 64
#define CCA_REALM_INITIAL_MEASUREMENT_SIZE  (64)

// cca-realm-extensible-measurements-label = 44239
#define CCA_REALM_EXTENSIBLE_MEASUREMENTS_LABEL  (44239LL)
// cca-realm-measurement-type = bytes .size 32 / bytes .size 48 / bytes .size 64
#define CCA_REALM_EXTENSIBLE_MEASUREMENTS_SIZE  (64)

// cca-realm-hash-algo-id-label = 44236
#define CCA_REALM_HASH_ALGO_ID_LABEL  (44236LL)

// cca-realm-public-key-label = 44237
#define CCA_REALM_PUBKEY_LABEL  (44237LL)

// cca-realm-public-key-hash-algo-id-label = 44240
#define CCA_REALM_PUBKEY_HASH_ALGO_ID_LABEL  (44240LL)

/* Macros defining the cca-platform-sw-component properties.
  cca-platform-sw-component = {
    ? 1 => text, ; component type
    2 => cca-hash-type, ; measurement value
    ? 4 => text, ; version
    5 => cca-hash-type, ; signer id
    ? 6 => text, ; hash algorithm identifier
  }
*/
#define CCA_PLAT_SW_COMPONENT_TYPE               (1)
#define CCA_PLAT_SW_COMPONENT_MEASUREMENT_VALUE  (2)
#define CCA_PLAT_SW_COMPONENT_VERSION            (4)
#define CCA_PLAT_SW_COMPONENT_SIGNER_ID          (5)
#define CCA_PLAT_SW_COMPONENT_HASH_ALGORITHM_ID  (6)

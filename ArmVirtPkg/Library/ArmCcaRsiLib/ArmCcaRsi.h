/** @file
  Definitions for Arm CCA Realm Service Interface.

  Copyright (c) 2022 - 2023, ARM Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version A-bet0
     (https://developer.arm.com/documentation/den0137/)
**/

#ifndef ARM_CCA_RSI_H_
#define ARM_CCA_RSI_H_

// FIDs for Realm Service Interface calls.
#define FID_RSI_ATTESTATION_TOKEN_CONTINUE  0xC4000195
#define FID_RSI_ATTESTATION_TOKEN_INIT      0xC4000194
#define FID_RSI_IPA_STATE_GET               0xC4000198
#define FID_RSI_IPA_STATE_SET               0xC4000197
#define FID_RSI_REALM_CONFIG                0xC4000196
#define FID_RSI_VERSION                     0xC4000190

/** RSI Command Return codes
   See Section B4.4.1, RMM Specification, version A-bet0.
   The width of the RsiCommandReturnCode enumeration is 64 bits.
*/
#define RSI_SUCCESS      0ULL
#define RSI_ERROR_INPUT  1ULL
#define RSI_ERROR_STATE  2ULL
#define RSI_INCOMPLETE   3ULL

/** RSI interface Version
   See Section B4.4.3,  RMM Specification, version A-bet0.
   The width of the RsiInterfaceVersion fieldset is 64 bits.
*/
#define RSI_VER_MINOR_MASK   0x00FFULL
#define RSI_VER_MAJOR_MASK   0x7F00ULL
#define RSI_VER_MAJOR_SHIFT  16

#endif // ARM_CCA_RSI_H_
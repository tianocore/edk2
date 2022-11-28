/** @file
  Definitions for Arm CCA Realm Service Interface.

  Copyright (c) 2022 - 2023, ARM Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-rel0
     (https://developer.arm.com/documentation/den0137/)
**/

#pragma once

/** RSI Command Return codes
   See Section B5.4.1, RMM Specification, version 1.0-rel0.
   The width of the RsiCommandReturnCode enumeration is 64 bits.
*/
#define RSI_SUCCESS        0ULL
#define RSI_ERROR_INPUT    1ULL
#define RSI_ERROR_STATE    2ULL
#define RSI_INCOMPLETE     3ULL
#define RSI_ERROR_UNKNOWN  4ULL

/** RSI interface Version
   See Section B5.4.4,  RMM Specification, version 1.0-rel0.
   The width of the RsiInterfaceVersion fieldset is 64 bits.
*/
#define RSI_VER_MINOR_MASK   0x0000FFFFULL
#define RSI_VER_MAJOR_MASK   0x7FFF0000ULL
#define RSI_VER_MAJOR_SHIFT  16
#define RSI_VERSION_MASK     (RSI_VER_MAJOR_MASK | RSI_VER_MINOR_MASK)
#define RMM_VERSION(Major, Minor)  ((Minor & RSI_VER_MINOR_MASK) | \
  ((Major << RSI_VER_MAJOR_SHIFT) & RSI_VER_MAJOR_MASK))

#define RSI_GET_MAJOR_REVISION(Rev) \
  ((Rev & RSI_VER_MAJOR_MASK) >> RSI_VER_MAJOR_SHIFT)

#define RSI_GET_MINOR_REVISION(Rev) \
  ((Rev & RSI_VER_MINOR_MASK))

//
// Static assert checks.
//
STATIC_ASSERT (
  sizeof (ARM_CCA_REALM_CONFIG) == SIZE_4KB,
  "sizeof (ARM_CCA_REALM_CONFIG) == SIZE_4KB"
  );
STATIC_ASSERT (
  OFFSET_OF (ARM_CCA_REALM_CONFIG, IpaWidth) == ARM_CCA_REALM_CFG_OFFSET_IPA_WIDTH,
  "OFFSET_OF (ARM_CCA_REALM_CONFIG, IpaWidth) == ARM_CCA_REALM_CFG_OFFSET_IPA_WIDTH"
  );
STATIC_ASSERT (
  OFFSET_OF (ARM_CCA_REALM_CONFIG, HashAlgorithm) == ARM_CCA_REALM_CFG_OFFSET_HASH_ALGO,
  "OFFSET_OF (ARM_CCA_REALM_CONFIG, HashAlgorithm) == ARM_CCA_REALM_CFG_OFFSET_HASH_ALGO"
  );
STATIC_ASSERT (
  OFFSET_OF (ARM_CCA_REALM_CONFIG, Reserved) == ARM_CCA_REALM_CFG_OFFSET_RESERVED,
  "OFFSET_OF (ARM_CCA_REALM_CONFIG, Reserved) == ARM_CCA_REALM_CFG_OFFSET_RESERVED"
  );
STATIC_ASSERT (
  OFFSET_OF (ARM_CCA_REALM_CONFIG, Rpv) == ARM_CCA_REALM_CFG_OFFSET_RPV,
  "OFFSET_OF (ARM_CCA_REALM_CONFIG, Rpv) == ARM_CCA_REALM_CFG_OFFSET_RPV"
  );
STATIC_ASSERT (
  OFFSET_OF (ARM_CCA_REALM_CONFIG, Reserved1) == ARM_CCA_REALM_CFG_OFFSET_RESERVED1,
  "OFFSET_OF (ARM_CCA_REALM_CONFIG, Reserved1) == ARM_CCA_REALM_CFG_OFFSET_RESERVED1"
  );

STATIC_ASSERT (
  sizeof (ARM_CCA_RSI_HOST_CALL_ARGS) == ARM_CCA_RSI_HOST_CALL_ARGS_SIZE,
  "sizeof (ARM_CCA_RSI_HOST_CALL_ARGS) == ARM_CCA_RSI_HOST_CALL_ARGS_SIZE"
  );

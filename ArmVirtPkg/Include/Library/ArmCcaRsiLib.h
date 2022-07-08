/** @file
  Library that implements the Arm CCA Realm Service Interface calls.

  Copyright (c) 2022 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-rel0
     (https://developer.arm.com/documentation/den0137/)
**/

#pragma once

#include <Base.h>

/**
  A macro defining the size of a Realm Granule.
  See Section A2.2, RMM Specification, version 1.0-rel0
  DNBXXX A Granule is a unit of physical memory whose size is 4KB.
*/
#define ARM_CCA_REALM_GRANULE_SIZE  SIZE_4KB

/* The maximum length of the Realm Personalisation Value (RPV).
*/
#define ARM_CCA_REALM_CFG_RPV_SIZE  64

/* The size of the Realm Config is 4KB.
*/
#define ARM_CCA_REALM_CFG_SIZE  SIZE_4KB

/* Helper macros to define the RealmConfig structure.
*/
#define ARM_CCA_REALM_CFG_OFFSET_IPA_WIDTH  0
#define ARM_CCA_REALM_CFG_OFFSET_HASH_ALGO  (ARM_CCA_REALM_CFG_OFFSET_IPA_WIDTH + sizeof (UINT64))
#define ARM_CCA_REALM_CFG_OFFSET_RESERVED   (ARM_CCA_REALM_CFG_OFFSET_HASH_ALGO + sizeof (UINT8))
#define ARM_CCA_REALM_CFG_OFFSET_RPV        0x200
#define ARM_CCA_REALM_CFG_OFFSET_RESERVED1  (ARM_CCA_REALM_CFG_OFFSET_RPV + ARM_CCA_REALM_CFG_RPV_SIZE)

#pragma pack(1)

/** A structure describing the Realm Configuration.
  See Section B5.4.5 RsiRealmConfig type, RMM Specification, version 1.0-rel0
  The width of the RsiRealmConfig structure is 4096 (0x1000) bytes.
*/
typedef struct ArmCcaRealmConfig {
  /// Width of IPA in bits.
  UINT64    IpaWidth;
  /// Width of the RsiHashAlgorithm enumeration is 8 bits.
  UINT8     HashAlgorithm;
  /// Reserved
  UINT8     Reserved[ARM_CCA_REALM_CFG_OFFSET_RPV - ARM_CCA_REALM_CFG_OFFSET_RESERVED];
  /// Realm Personalisation Value
  UINT8     Rpv[ARM_CCA_REALM_CFG_RPV_SIZE];
  /// Unused bits of the RsiRealmConfig structure should be zero.
  UINT8     Reserved1[ARM_CCA_REALM_CFG_SIZE - ARM_CCA_REALM_CFG_OFFSET_RESERVED1];
} ARM_CCA_REALM_CONFIG;

#pragma pack()

/**
  Read the Realm Configuration.

  @param [out]  Config     Pointer to the address of the buffer to retrieve
                           the Realm configuration.

  Note: The buffer to retrieve the Realm configuration must be aligned to the
        Realm granule size.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
ArmCcaRsiGetRealmConfig (
  IN  ARM_CCA_REALM_CONFIG  *Config
  );

/**
   Get the version of the RSI implementation.

  @param [out] UefiImpl     The version of the RSI specification
                            implemented by the UEFI firmware.
  @param [out] RmmImplLow   The low version of the RSI specification
                            implemented by the RMM.
  @param [out] RmmImplHigh  The high version of the RSI specification
                            implemented by the RMM.

  @retval RETURN_SUCCESS                Success.
  @retval RETURN_UNSUPPORTED            The execution context is not a Realm.
  @retval RETURN_INCOMPATIBLE_VERSION   The Firmware and RMM specification
                                        revisions are not compatible.
  @retval RETURN_INVALID_PARAMETER      A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
ArmCcaRsiGetVersion (
  OUT UINT32 *CONST  UefiImpl,
  OUT UINT32 *CONST  RmmImplLow,
  OUT UINT32 *CONST  RmmImplHigh
  );

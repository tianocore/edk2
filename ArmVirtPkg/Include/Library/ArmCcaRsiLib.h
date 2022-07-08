/** @file
  Library that implements the Arm CCA Realm Service Interface calls.

  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version A-bet0
     (https://developer.arm.com/documentation/den0137/)
**/

#ifndef ARM_CCA_RSI_LIB_
#define ARM_CCA_RSI_LIB_

#include <Base.h>

/**
  A macro defining the size of a Realm Granule.
  See Section A2.2, RMM Specification, version A-bet0
  DNBXXX A Granule is a unit of physical memory whose size is 4KB.
*/
#define REALM_GRANULE_SIZE  SIZE_4KB

/** A structure describing the Realm Configuration.
  See Section B4.4.4 RsiRealmConfig type, RMM Specification, version A-bet0
  The width of the RsiRealmConfig structure is 4096 (0x1000) bytes.
*/
typedef struct RealmConfig {
  // Width of IPA in bits.
  UINT64    IpaWidth;
  // Unused bits of the RsiRealmConfig structure should be zero.
  UINT8     Reserved[SIZE_4KB - sizeof (UINT64)];
} REALM_CONFIG;

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
RsiGetRealmConfig (
  IN  REALM_CONFIG  *Config
  );

/**
   Get the version of the RSI implementation.

  @param [out] Major  The major version of the RSI implementation.
  @param [out] Minor  The minor version of the RSI implementation.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
 */
RETURN_STATUS
EFIAPI
RsiGetVersion (
  OUT UINT16 *CONST  Major,
  OUT UINT16 *CONST  Minor
  );

#endif // ARM_CCA_RSI_LIB_

/** @file
  Library that implements the Arm CCA Realm Service Interface calls.

  Copyright (c) 2022 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state
    - RIM          - Realm Initial Measurement
    - REM          - Realm Extensible Measurement

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

/**
  A macro defining the mask for the RSI RIPAS type.
  See Section B4.4.7 RsiRipas type, RMM Specification, version 1.0-rel0.
*/
#define ARM_CCA_RIPAS_TYPE_MASK  0xFF

/* Maximum measurement data size in bytes.
  See Section C1.17 RmmRealmMeasurement type, RMM Specification, version 1.0-rel0
  The width of the RmmRealmMeasurement type is 512 bits.
*/
#define ARM_CCA_MAX_MEASUREMENT_DATA_SIZE_BYTES  64

/* Minimum and Maximum indices for REMs
  See Section A2.1.3 Realm attributes, RMM Specification, version 1.0-rel0
  IFMPYL - Attributes of a Realm include an array of measurement values. The
  first entry in this array is a RIM. The remaining entries in this array are
  REMs.
*/
#define ARM_CCA_MIN_REM_INDEX  1
#define ARM_CCA_MAX_REM_INDEX  4

/* The RsiRipasChangeFlags fieldset contains flags provided by
   the Realm when requesting a RIPAS change.
   See section B5.4.9 RsiRipasChangeFlags type in the
   RMM Specification, version 1.0-rel0.
   The following macros prefixed RIPAS_CHANGE_FLAGS_xxx
   define the values of the RsiRipasChangeFlags fieldset.
*/

/* A RIPAS change from DESTROYED should not be permitted.
  See section B4.4.8 RsiRipasChangeDestroyed type in the
  RMM Specification, version 1.0-rel0
*/
#define ARM_CCA_RIPAS_CHANGE_FLAGS_RSI_NO_CHANGE_DESTROYED  0

/* A RIPAS change from DESTROYED should be permitted.
  See section B4.4.8 RsiRipasChangeDestroyed type in the
  RMM Specification, version 1.0-rel0
*/
#define ARM_CCA_RIPAS_CHANGE_FLAGS_RSI_CHANGE_DESTROYED  1

/* The RsiResponse type is a value returned by the
   RSI_IPA_STATE_SET command and represents whether
   the Host accepted or rejected a Realm request.
   See section B5.4.6 RsiResponse type in the
   RMM Specification, version 1.0-rel0.
   The width of the RsiResponse enumeration is 1 bit
   and the following macros prefixed RIPAS_CHANGE_RESPONSE_xxx
   define the values of the RsiResponse type.
*/

/* The RIPAS change request to RAM was accepted
   by the host.
*/
#define ARM_CCA_RIPAS_CHANGE_RESPONSE_ACCEPT  0

/* The RIPAS change request to RAM was rejected
   by the host.
*/
#define ARM_CCA_RIPAS_CHANGE_RESPONSE_REJECT  1

/* A mask for the RSI Response bit */
#define ARM_CCA_RSI_RESPONSE_MASK  BIT0

/** An enum describing the RSI RIPAS.
   See Section A5.2.2 Realm IPA state, RMM Specification, version 1.0-rel0
*/
typedef enum ArmCcaRipas {
  RipasEmpty,      ///< Unused IPA location.
  RipasRam,        ///< Private code or data owned by the Realm.
  RipasDestroyed,  ///< An address which is inaccessible to the Realm.
  RipasDev,        ///< MMIO address where an assigned Realm device is mapped.
  RipasMax         ///< A valid RIPAS type value is less than RipasMax.
} ARM_CCA_RIPAS;

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

/* The maximum size of the RsiHostCallArgs structure.
*/
#define ARM_CCA_RSI_HOST_CALL_ARGS_SIZE  0x100

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

/** A structure describing the Host Call arguments
    See Section 5.4.3 RsiHostCall type, RMM Specification, version 1.0-rel0
*/
typedef struct ArmCcaRsiHostCallArgs {
  UINT16    Imm;
  UINT8     Reserved1[6];

  UINT64    Gprs0;
  UINT64    Gprs1;
  UINT64    Gprs2;
  UINT64    Gprs3;
  UINT64    Gprs4;
  UINT64    Gprs5;
  UINT64    Gprs6;
  UINT64    Gprs7;
  UINT64    Gprs8;
  UINT64    Gprs9;
  UINT64    Gprs10;
  UINT64    Gprs11;
  UINT64    Gprs12;
  UINT64    Gprs13;
  UINT64    Gprs14;
  UINT64    Gprs15;
  UINT64    Gprs16;
  UINT64    Gprs17;
  UINT64    Gprs18;
  UINT64    Gprs19;
  UINT64    Gprs20;
  UINT64    Gprs21;
  UINT64    Gprs22;
  UINT64    Gprs23;
  UINT64    Gprs24;
  UINT64    Gprs25;
  UINT64    Gprs26;
  UINT64    Gprs27;
  UINT64    Gprs28;
  UINT64    Gprs29;
  UINT64    Gprs30;
} ARM_CCA_RSI_HOST_CALL_ARGS;

#pragma pack()

/**
  Returns the IPA state for the page pointed by the address.

  @param [in]       Base        Base of target IPA region.
  @param [in, out]  Top         End  of target IPA region on input.
                                Top of IPA region which has the
                                reported RIPAS value on return.
  @param [out]  State           The RIPAS state for the address specified.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
ArmCcaRsiGetIpaState (
  IN      UINT64         *Base,
  IN OUT  UINT64         **Top,
  OUT     ARM_CCA_RIPAS  *State
  );

/**
  Sets the IPA state for the pages pointed by the memory range.

  @param [in]   Address     Address to the start of the memory range.
  @param [in]   Size        Length of the memory range.
  @param [in]   State       The RIPAS state to be configured.
  @param [in]   Flags       The RIPAS change flags.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
  @retval RETURN_ACCESS_DENIED      RIPAS change request was rejected.
**/
RETURN_STATUS
EFIAPI
ArmCcaRsiSetIpaState (
  IN  UINT64         *Address,
  IN  UINT64         Size,
  IN  ARM_CCA_RIPAS  State,
  IN  UINT64         Flags
  );

/**
  Extends a measurement to a REM.

  @param [in] MeasurementIndex     Index of the REM.
  @param [in] Measurement          Pointer to the measurement buffer.
  @param [in] MeasurementSize      Size of the measurement data.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
ArmCcaRsiExtendMeasurement (
  IN        UINTN          MeasurementIndex,
  IN  CONST UINT8  *CONST  Measurement,
  IN        UINTN          MeasurementSize
  );

/**
  Read the measurement value from a REM.

  @param [in]   MeasurementIndex     Index of the REM.
  @param [out]  MeasurementBuffer     Pointer to store the measurement data.
  @param [in]   MeasurementBufferSize Size of the measurement buffer.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
ArmCcaRsiReadMeasurement (
  IN    UINTN          MeasurementIndex,
  OUT   UINT8  *CONST  MeasurementBuffer,
  IN    UINTN          MeasurementBufferSize
  );

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
  Make a Host Call.

  A Host call can be used by a Realm to make a hypercall.
  On Realm execution of HVC, an Unknown exception is taken to the Realm.

  @param [in] Args    Pointer to the IPA of the Host call data
                      structure.

  Note: The IPA of the Host call arguments data structure must be aligned
         to the Realm granule size.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
ArmCcaRsiHostCall (
  IN  ARM_CCA_RSI_HOST_CALL_ARGS  *Args
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

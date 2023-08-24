/** @file
  Library that implements the Arm CCA Realm Service Interface calls.

  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state
    - RIM          - Realm Initial Measurement
    - REM          - Realm Extensible Measurement

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-eac2
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

/**
  A macro defining the mask for the RSI RIPAS type.
  See Section B4.4.5 RsiRipas type, RMM Specification, version A-bet0.
*/
#define RIPAS_TYPE_MASK  0xFF

/* Maximum attestation token size
  RBXKKY The size of an attestation token is no larger than 4KB.
*/
#define MAX_ATTESTATION_TOKEN_SIZE  SIZE_4KB

/* Maximum challenge data size in bits.
*/
#define MAX_CHALLENGE_DATA_SIZE_BITS  512

/* Minimum recommended challenge data size in bits.
*/
#define MIN_CHALLENGE_DATA_SIZE_BITS  256

/* Maximum measurement data size in bytes.
  See Section C1.11 RmmRealmMeasurement type, RMM Specification, version A-bet0
  The width of the RmmRealmMeasurement type is 512 bits.
*/
#define MAX_MEASUREMENT_DATA_SIZE_BYTES  64

/* Minimum and Maximum indices for REMs
  See Section A2.1.3 Realm attributes, RMM Specification, version A-bet0
  IFMPYL - Attributes of a Realm include an array of measurement values. The
  first entry in this array is a RIM. The remaining entries in this array are
  REMs.
*/
#define MIN_REM_INDEX  1
#define MAX_REM_INDEX  4

/* The values of the RsiHashAlgorithm enumeration.
   SHA-256 (Secure Hash Standard (SHS))
*/
#define RSI_HASH_SHA_256  0

/* The values of the RsiHashAlgorithm enumeration.
   SHA-512 (Secure Hash Standard (SHS))
*/
#define RSI_HASH_SHA_512  1

/** An enum describing the RSI RIPAS.
   See Section A5.2.2 Realm IPA state, RMM Specification, version A-bet0
*/
typedef enum Ripas {
  RipasEmpty,      ///< Unused IPA location.
  RipasRam,        ///< Private code or data owned by the Realm.
  RipasMax         ///< A valid RIPAS type value is less than RipasMax.
} RIPAS;

/** A structure describing the Realm Configuration.
  See Section B4.4.5 RsiRealmConfig type, RMM Specification, version 1.0-eac2
  The width of the RsiRealmConfig structure is 4096 (0x1000) bytes.
*/
typedef struct RealmConfig {
  // Width of IPA in bits.
  UINT64    IpaWidth;
  // Width of the RsiHashAlgorithm enumeration is 8 bits.
  UINT8     HashAlgorithm;
  // Unused bits of the RsiRealmConfig structure should be zero.
  UINT8     Reserved[SIZE_4KB - (sizeof (UINT64) + sizeof (UINT8))];
} REALM_CONFIG;

/** A structure describing the Host Call arguments
    See Section 4.4.2 RsiHostCall type, RMM Specification, version 1.0-bet2
*/
typedef struct HostCallArgs {
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
} HOST_CALL_ARGS;

/**
  Retrieve an attestation token from the RMM.

  @param [in]       ChallengeData         Pointer to the challenge data to be
                                          included in the attestation token.
  @param [in]       ChallengeDataSizeBits Size of the challenge data in bits.
  @param [out]      TokenBuffer           Pointer to a buffer to store the
                                          retrieved attestation token.
  @param [in, out]  TokenBufferSize       Size of the token buffer on input and
                                          number of bytes stored in token buffer
                                          on return.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
  @retval RETURN_ABORTED            The operation was aborted as the state
                                    of the Realm or REC does not match the
                                    state expected by the command.
  @retval RETURN_NOT_READY          The operation requested by the command
                                    is not complete.
**/
RETURN_STATUS
EFIAPI
RsiGetAttestationToken (
  IN      CONST UINT8   *CONST  ChallengeData,
  IN            UINT64          ChallengeDataSizeBits,
  OUT           UINT8   *CONST  TokenBuffer,
  IN OUT        UINT64  *CONST  TokenBufferSize
  );

/**
  Returns the IPA state for the page pointed by the address.

  @param [in]   Address     Address to retrive IPA state.
  @param [out]  State       The RIPAS state for the address specified.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
RsiGetIpaState (
  IN   UINT64  *Address,
  OUT  RIPAS   *State
  );

/**
  Sets the IPA state for the pages pointed by the memory range.

  @param [in]   Address     Address to the start of the memory range.
  @param [in]   Size        Length of the memory range.
  @param [in]   State       The RIPAS state to be configured.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
RsiSetIpaState (
  IN  UINT64  *Address,
  IN  UINT64  Size,
  IN  RIPAS   State
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
RsiExtendMeasurement (
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
RsiReadMeasurement (
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
RsiGetRealmConfig (
  IN  REALM_CONFIG  *Config
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
RsiHostCall (
  IN  HOST_CALL_ARGS  *Args
  );

/**
   Get the version of the RSI implementation.

  @param [out] Major  The major version of the RSI implementation.
  @param [out] Minor  The minor version of the RSI implementation.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
RsiGetVersion (
  OUT UINT16 *CONST  Major,
  OUT UINT16 *CONST  Minor
  );

#endif // ARM_CCA_RSI_LIB_

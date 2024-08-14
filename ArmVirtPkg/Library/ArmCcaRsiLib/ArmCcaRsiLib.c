/** @file
  Library that implements the Arm CCA Realm Service Interface calls.

  Copyright (c) 2022 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state
    - REM          - Realm Extensible Measurement

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-rel0
     (https://developer.arm.com/documentation/den0137/)

**/
#include <Base.h>

#include <Library/ArmCcaRsiLib.h>
#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include "ArmCcaRsi.h"

/** The version of RSI specification implemented by this module.
*/
STATIC CONST UINT32  mRsiImplVersion = RMM_VERSION (1, 0);

/**
  Convert the RSI status code to EFI Status code.

  @param [in]   RsiCommandReturnCode  RSI status code.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
  @retval RETURN_ABORTED            The operation was aborted as the state
                                    of the Realm or REC does not match the
                                    state expected by the command.
  @retval RETURN_NOT_READY          The operation requested by the command
                                    is not complete.
 **/
STATIC
RETURN_STATUS
RsiCmdStatusToEfiStatus (
  IN  UINT64  RsiCommandReturnCode
  )
{
  switch (RsiCommandReturnCode) {
    case RSI_SUCCESS:
      return RETURN_SUCCESS;
    case RSI_ERROR_INPUT:
      return RETURN_INVALID_PARAMETER;
    case RSI_ERROR_STATE:
    case RSI_ERROR_UNKNOWN:
      return RETURN_ABORTED;
    case RSI_INCOMPLETE:
      return RETURN_NOT_READY;
    default:
      // Unknown error code.
      ASSERT (0);
      break;
  } // switch

  return RETURN_ABORTED;
}

/**
  Check if the value is aligned to the size of the Realm granule.

  @param [in] Value          Value to check granule alignment.

  @retval TRUE  Value is aligned to the Realm granule size.
  @retval FALSE Value is not aligned to the Realm granule size.
**/
STATIC
BOOLEAN
EFIAPI
IsGranuleAligned (
  IN   UINT64  *Value
  )
{
  if (((UINT64)Value & (REALM_GRANULE_SIZE - 1)) != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Continue the operation to retrieve an attestation token.

  @param [out]     TokenBuffer      Pointer to a buffer to store the
                                    retrieved attestation token.
  @param [in]      Offset           Offset within Token buffer granule
                                    to start of buffer in bytes.
  @param [in,out]  TokenSize        On input size of the token buffer,
                                    and on output size of the token
                                    returned if operation is successful,
                                    otherwise 0.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
  @retval RETURN_ABORTED            The operation was aborted as the state
                                    of the Realm or REC does not match the
                                    state expected by the command.
                                    Or the Token generation failed for an
                                    unknown or IMPDEF reason.
  @retval RETURN_NOT_READY          The operation requested by the command
                                    is not complete.
 **/
STATIC
RETURN_STATUS
EFIAPI
RsiAttestationTokenContinue (
  OUT           UINT8   *CONST  TokenBuffer,
  IN            UINT64   CONST  Offset,
  IN OUT        UINT64  *CONST  TokenSize
  )
{
  RETURN_STATUS  Status;
  ARM_SMC_ARGS   SmcCmd;

  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = FID_RSI_ATTESTATION_TOKEN_CONTINUE;
  // Set the IPA of the Granule to which the token will be written.
  SmcCmd.Arg1 = (UINTN)TokenBuffer;
  // Set the Offset within Granule to start of buffer in bytes
  SmcCmd.Arg2 = (UINTN)Offset;
  // Set the size of the buffer in bytes
  SmcCmd.Arg3 = (UINTN)*TokenSize;

  ArmCallSmc (&SmcCmd);
  Status = RsiCmdStatusToEfiStatus (SmcCmd.Arg0);
  if (!RETURN_ERROR (Status)) {
    // Update the token size
    *TokenSize = SmcCmd.Arg1;
  } else {
    // Clear the TokenBuffer on error.
    ZeroMem (TokenBuffer, *TokenSize);
    *TokenSize = 0;
  }

  return Status;
}

/**
  Initialize the operation to retrieve an attestation token.

  @param [in]       ChallengeData         Pointer to the challenge data to be
                                          included in the attestation token.
  @param [in]       ChallengeDataSizeBits Size of the challenge data in bits.
  @param [out]      MaxTokenSize          Pointer to an integer to retrieve
                                          the maximum attestation token size.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
 **/
STATIC
RETURN_STATUS
EFIAPI
RsiAttestationTokenInit (
  IN      CONST UINT8   *CONST  ChallengeData,
  IN            UINT64          ChallengeDataSizeBits,
  OUT           UINT64  *CONST  MaxTokenSize
  )
{
  RETURN_STATUS  Status;
  ARM_SMC_ARGS   SmcCmd;
  UINT8          *Buffer8;
  CONST UINT8    *Data8;
  UINT64         Count;
  UINT8          TailBits;

  /* See A7.2.2 Attestation token generation, RMM Specification, version A-bet0
     IWTKDD - If the size of the challenge provided by the relying party is less
     than 64 bytes, it should be zero-padded prior to calling
     RSI_ATTESTATION_TOKEN_INIT.

    Therefore, zero out the SmcCmd memory before coping the ChallengeData
    bits.
  */
  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = FID_RSI_ATTESTATION_TOKEN_INIT;
  // Copy challenge data.
  Buffer8 = (UINT8 *)&SmcCmd.Arg1;
  Data8   = ChallengeData;

  // First copy whole bytes
  Count = ChallengeDataSizeBits >> 3;
  CopyMem (Buffer8, Data8, Count);

  // Now copy any remaining tail bits.
  TailBits = ChallengeDataSizeBits & (8 - 1);
  if (TailBits > 0) {
    // Advance buffer pointers.
    Buffer8 += Count;
    Data8   += Count;

    // Copy tail byte.
    *Buffer8 = *Data8;

    // Clear unused tail bits.
    *Buffer8 &= ~(0xFF << TailBits);
  }

  ArmCallSmc (&SmcCmd);
  Status = RsiCmdStatusToEfiStatus (SmcCmd.Arg0);
  if (RETURN_ERROR (Status)) {
    // Set the max token size to zero
    *MaxTokenSize = 0;
  } else {
    *MaxTokenSize = SmcCmd.Arg1;
  }

  return Status;
}

/**
  Free the attestation token buffer.

  @param [in]      TokenBuffer           Pointer to the retrieved
                                         attestation token.
  @param [in]      TokenBufferSize       Size of the token buffer.
**/
VOID
RsiFreeAttestationToken (
  IN           UINT8  *CONST  TokenBuffer,
  IN           UINT64  CONST  TokenBufferSize
  )
{
  if (TokenBuffer != NULL) {
    if (TokenBufferSize > 0) {
      // Scrub the token buffer
      ZeroMem (TokenBuffer, TokenBufferSize);
    }

    FreePool (TokenBuffer);
  }
}

/**
  Retrieve an attestation token from the RMM.

  @param [in]       ChallengeData         Pointer to the challenge data to be
                                          included in the attestation token.
  @param [in]       ChallengeDataSizeBits Size of the challenge data in bits.
  @param [out]      TokenBuffer           Pointer to a buffer to store the
                                          retrieved attestation token.
  @param [out]      TokenBufferSize       Length of token data returned.

  Note: The TokenBuffer allocated must be freed by the caller
  using RsiFreeAttestationToken().

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
  @retval RETURN_ABORTED            The operation was aborted as the state
                                    of the Realm or REC does not match the
                                    state expected by the command.
                                    Or the Token generation failed for an
                                    unknown or IMPDEF reason.
  @retval RETURN_NOT_READY          The operation requested by the command
                                    is not complete.
**/
RETURN_STATUS
EFIAPI
RsiGetAttestationToken (
  IN      CONST UINT8   *CONST  ChallengeData,
  IN            UINT64          ChallengeDataSizeBits,
  OUT           UINT8  **CONST  TokenBuffer,
  OUT           UINT64  *CONST  TokenBufferSize
  )
{
  RETURN_STATUS  Status;
  UINT8          *Granule;
  UINT64         GranuleSize;
  UINT64         Offset;
  UINT8          *Token;
  UINT64         TokenSize;
  UINT64         MaxTokenSize;

  if ((TokenBuffer == NULL) ||
      (TokenBufferSize == NULL) ||
      (ChallengeData == NULL))
  {
    return RETURN_INVALID_PARAMETER;
  }

  if (ChallengeDataSizeBits > MAX_CHALLENGE_DATA_SIZE_BITS) {
    return RETURN_INVALID_PARAMETER;
  }

  /* See A7.2.2 Attestation token generation, RMM Specification, version A-bet0
     IWTKDD - Arm recommends that the challenge should contain at least 32 bytes
     of unique data.
  */
  if (ChallengeDataSizeBits < MIN_CHALLENGE_DATA_SIZE_BITS) {
    DEBUG ((DEBUG_WARN, "Minimum Challenge data size should be 32 bytes\n"));
  }

  Status = RsiAttestationTokenInit (
             ChallengeData,
             ChallengeDataSizeBits,
             &MaxTokenSize
             );
  if (RETURN_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Allocate a granule to retrieve the attestation token chunk.
  Granule = (UINT8 *)AllocateAlignedPages (
                       EFI_SIZE_TO_PAGES (REALM_GRANULE_SIZE),
                       REALM_GRANULE_SIZE
                       );
  if (Granule == NULL) {
    ASSERT (0);
    return RETURN_OUT_OF_RESOURCES;
  }

  // Allocate a buffer to store the retrieved attestation token.
  Token = AllocateZeroPool (MaxTokenSize);
  if (Token == NULL) {
    ASSERT (0);
    Status = RETURN_OUT_OF_RESOURCES;
    goto exit_handler;
  }

  TokenSize = 0;
  do {
    // Retrieve one Granule of data per loop iteration
    ZeroMem (Granule, REALM_GRANULE_SIZE);
    Offset = 0;
    do {
      // Retrieve sub-Granule chunk of data per loop iteration
      GranuleSize = REALM_GRANULE_SIZE - Offset;
      Status      = RsiAttestationTokenContinue (
                      Granule,
                      Offset,
                      &GranuleSize
                      );
      Offset += GranuleSize;
    } while ((Status == RETURN_NOT_READY) && (Offset < REALM_GRANULE_SIZE));

    if (RETURN_ERROR (Status) && (Status != RETURN_NOT_READY)) {
      ASSERT (0);
      goto exit_handler1;
    }

    // "Offset" bytes of data are now ready for consumption from "Granule"
    // Copy the new token data from the Granule.
    CopyMem (&Token[TokenSize], Granule, Offset);
    TokenSize += Offset;
  } while ((Status == RETURN_NOT_READY) && (TokenSize < MaxTokenSize));

  *TokenBuffer     = Token;
  *TokenBufferSize = TokenSize;
  goto exit_handler;

exit_handler1:
  if (Token != NULL) {
    // Scrub the old Token
    ZeroMem (Token, TokenSize);
    FreePool (Token);
  }

  *TokenBuffer     = NULL;
  *TokenBufferSize = 0;

exit_handler:
  // Scrub the Granule buffer
  ZeroMem (Granule, REALM_GRANULE_SIZE);
  FreeAlignedPages (Granule, EFI_SIZE_TO_PAGES (REALM_GRANULE_SIZE));

  return Status;
}

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
RsiGetIpaState (
  IN      UINT64  *Base,
  IN OUT  UINT64  **Top,
  OUT     RIPAS   *State
  )
{
  RETURN_STATUS  Status;
  ARM_SMC_ARGS   SmcCmd;

  if ((State == NULL) ||
      (!IsGranuleAligned (Base)) ||
      (!IsGranuleAligned (*Top)) ||
      (*Top < Base))
  {
    return RETURN_INVALID_PARAMETER;
  }

  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = FID_RSI_IPA_STATE_GET;
  SmcCmd.Arg1 = (UINTN)Base;
  SmcCmd.Arg2 = (UINTN)*Top;

  ArmCallSmc (&SmcCmd);
  Status = RsiCmdStatusToEfiStatus (SmcCmd.Arg0);
  if (!RETURN_ERROR (Status)) {
    *Top   = (UINT64 *)SmcCmd.Arg1;
    *State = (RIPAS)(SmcCmd.Arg2 & RIPAS_TYPE_MASK);
  }

  return Status;
}

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
RsiSetIpaState (
  IN  UINT64  *Address,
  IN  UINT64  Size,
  IN  RIPAS   State,
  IN  UINT64  Flags
  )
{
  RETURN_STATUS  Status;
  UINT64         *BaseAddress;
  UINT64         *EndAddress;
  ARM_SMC_ARGS   SmcCmd;

  if ((Size == 0) ||
      (!IsGranuleAligned ((UINT64 *)Size)) ||
      (!IsGranuleAligned (Address))        ||
      ((State != RipasEmpty) && (State != RipasRam)))
  {
    return RETURN_INVALID_PARAMETER;
  }

  BaseAddress = Address;
  // Divide Size by 8 for the pointer arithmetic
  // to work, as we are adding to UINT64*.
  EndAddress = Address + (Size >> 3);

  while (Size > 0) {
    ZeroMem (&SmcCmd, sizeof (SmcCmd));
    SmcCmd.Arg0 = FID_RSI_IPA_STATE_SET;
    SmcCmd.Arg1 = (UINTN)BaseAddress;
    SmcCmd.Arg2 = (UINTN)EndAddress;
    SmcCmd.Arg3 = (UINTN)State;
    SmcCmd.Arg4 = Flags;

    ArmCallSmc (&SmcCmd);
    Status = RsiCmdStatusToEfiStatus (SmcCmd.Arg0);
    if (RETURN_ERROR (Status)) {
      break;
    }

    BaseAddress = (UINT64 *)SmcCmd.Arg1;
    Size        = EndAddress - BaseAddress;

    if ((SmcCmd.Arg2 & RSI_RESPONSE_MASK) == RIPAS_CHANGE_RESPONSE_REJECT) {
      Status = RETURN_ACCESS_DENIED;
      break;
    }
  }   // while

  return Status;
}

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
  )
{
  ARM_SMC_ARGS  SmcCmd;
  UINT64        *Data64;

  if ((MeasurementIndex < MIN_REM_INDEX)  ||
      (MeasurementIndex > MAX_REM_INDEX)  ||
      (Measurement == NULL)               ||
      (MeasurementSize == 0)              ||
      (MeasurementSize > MAX_MEASUREMENT_DATA_SIZE_BYTES))
  {
    return RETURN_INVALID_PARAMETER;
  }

  ZeroMem (&SmcCmd, sizeof (SmcCmd));

  SmcCmd.Arg0 = FID_RSI_MEASUREMENT_EXTEND;
  SmcCmd.Arg1 = MeasurementIndex;
  SmcCmd.Arg2 = MeasurementSize;

  Data64 = &SmcCmd.Arg3;
  CopyMem (Data64, Measurement, MeasurementSize);

  ArmCallSmc (&SmcCmd);
  return RsiCmdStatusToEfiStatus (SmcCmd.Arg0);
}

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
  )
{
  RETURN_STATUS  Status;
  ARM_SMC_ARGS   SmcCmd;
  UINT64         *Data64;

  if ((MeasurementIndex < MIN_REM_INDEX)  ||
      (MeasurementIndex > MAX_REM_INDEX)  ||
      (MeasurementBuffer == NULL))
  {
    return RETURN_INVALID_PARAMETER;
  }

  if (MeasurementBufferSize < MAX_MEASUREMENT_DATA_SIZE_BYTES) {
    return RETURN_BUFFER_TOO_SMALL;
  }

  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = FID_RSI_MEASUREMENT_READ;
  SmcCmd.Arg1 = MeasurementIndex;

  ArmCallSmc (&SmcCmd);
  Status = RsiCmdStatusToEfiStatus (SmcCmd.Arg0);
  if (!RETURN_ERROR (Status)) {
    Data64 = &SmcCmd.Arg1;
    CopyMem (MeasurementBuffer, Data64, MAX_MEASUREMENT_DATA_SIZE_BYTES);
  }

  return Status;
}

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
  OUT REALM_CONFIG  *Config
  )
{
  ARM_SMC_ARGS  SmcCmd;

  if ((Config == NULL) || (!IsGranuleAligned ((UINT64 *)Config))) {
    return RETURN_INVALID_PARAMETER;
  }

  // Add static asserts to check that the Realm Config is as what we expect.
  STATIC_ASSERT (sizeof (REALM_CONFIG) == SIZE_4KB);
  STATIC_ASSERT (
    OFFSET_OF (REALM_CONFIG, IpaWidth) == REALM_CFG_OFFSET_IPA_WIDTH
    );
  STATIC_ASSERT (
    OFFSET_OF (REALM_CONFIG, HashAlgorithm) == REALM_CFG_OFFSET_HASH_ALGO
    );
  STATIC_ASSERT (
    OFFSET_OF (REALM_CONFIG, Reserved) == REALM_CFG_OFFSET_RESERVED
    );
  STATIC_ASSERT (
    OFFSET_OF (REALM_CONFIG, Rpv) == REALM_CFG_OFFSET_RPV
    );
  STATIC_ASSERT (
    OFFSET_OF (REALM_CONFIG, Reserved1) == REALM_CFG_OFFSET_RESERVED1
    );

  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = FID_RSI_REALM_CONFIG;
  SmcCmd.Arg1 = (UINTN)Config;

  ArmCallSmc (&SmcCmd);
  return RsiCmdStatusToEfiStatus (SmcCmd.Arg0);
}

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
  )
{
  ARM_SMC_ARGS  SmcCmd;

  // The RMM specification, version 1.0-eac1, relaxes the alignment
  // requirement for RSI_HOST_CALL from 4KB to 256B. Also see RMM
  // specification, sections B4.3.3 RSI_HOST_CALL command and
  // section B4.3.3.2 Failure conditions.
  if ((Args == NULL) || (((UINT64)Args & (0x100 - 1)) != 0)) {
    return RETURN_INVALID_PARAMETER;
  }

  // See RMM specification, version 1.0-bet1, Section B4.4.2 RsiHostCall type
  // The width of the RsiHostCall structure is 256 (0x100) bytes.
  STATIC_ASSERT (sizeof (HOST_CALL_ARGS) == 0x100);

  // Clear the reserved fields
  ZeroMem (&Args->Reserved1, sizeof (Args->Reserved1));

  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = FID_RSI_HOST_CALL;
  SmcCmd.Arg1 = (UINTN)Args;

  ArmCallSmc (&SmcCmd);
  return RsiCmdStatusToEfiStatus (SmcCmd.Arg0);
}

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
RsiGetVersion (
  OUT UINT32 *CONST  UefiImpl,
  OUT UINT32 *CONST  RmmImplLow,
  OUT UINT32 *CONST  RmmImplHigh
  )
{
  RETURN_STATUS  Status;
  ARM_SMC_ARGS   SmcCmd;

  if ((UefiImpl == NULL) || (RmmImplLow == NULL) || (RmmImplHigh == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = FID_RSI_VERSION;
  SmcCmd.Arg1 = mRsiImplVersion;
  ArmCallSmc (&SmcCmd);
  if (SmcCmd.Arg0 == MAX_UINT64) {
    // This FID is not implemented, which means
    // we are not running in a Realm, therefore
    // return the error code as unsupported.
    return RETURN_UNSUPPORTED;
  }

  *RmmImplLow  = (SmcCmd.Arg1 & RSI_VERSION_MASK);
  *RmmImplHigh = (SmcCmd.Arg2 & RSI_VERSION_MASK);
  *UefiImpl    = mRsiImplVersion;

  // The RSI_VERSION command does not have any failure
  // conditions see section B5.3.10.2 Failure conditions
  // Therefore the only defined return values are
  // RSI_SUCCESS and RSI_ERROR_INPUT.
  Status = RsiCmdStatusToEfiStatus (SmcCmd.Arg0);
  if (Status == RETURN_INVALID_PARAMETER) {
    // RSI_VERSION returns RSI_ERROR_INPUT when
    // the RMM does not support an interface revision
    // which is compatible with the requested revision.
    // Since RSI_ERROR_INPUT is mapped to RETURN_INVALID_PARAMETER
    // by RsiCmdStatusToEfiStatus(), return the status code as
    // RETURN_INCOMPATIBLE_VERSION.
    return RETURN_INCOMPATIBLE_VERSION;
  }

  // Add an assert in case RMM returns a different error code than expected.
  ASSERT (Status == RETURN_SUCCESS);
  return Status;
}

/**
  Get the features supported by the RSI implementation.

  RMM implementations across different CCA platforms may support
  disparate features and may offer disparate configuration options
  for Realms. The features supported by an RSI implementation are
  discovered by reading feature pseudo-register values using the
  RSI_FEATURES command.

  @param [in]   FeatureRegIndex    The Feature Register Index.
  @param [out]  FeatureRegValue    The Feature Register Value.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
**/
RETURN_STATUS
EFIAPI
RsiGetFeatures (
  IN    UINT64  FeatureRegIndex,
  OUT   UINT64  *FeatureRegValue
  )
{
  ARM_SMC_ARGS  SmcCmd;

  if (FeatureRegValue == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = FID_RSI_FEATURES;
  SmcCmd.Arg1 = FeatureRegIndex;

  ArmCallSmc (&SmcCmd);
  *FeatureRegValue = SmcCmd.Arg1;
  return RsiCmdStatusToEfiStatus (SmcCmd.Arg0);
}

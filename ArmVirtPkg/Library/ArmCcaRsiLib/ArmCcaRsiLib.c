/** @file
  Library that implements the Arm CCA Realm Service Interface calls.

  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Rsi or RSI   - Realm Service Interface
    - IPA          - Intermediate Physical Address
    - RIPAS        - Realm IPA state
    - REM          - Realm Extensible Measurement

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-eac0
     (https://developer.arm.com/documentation/den0137/)

**/
#include <Base.h>

#include <Library/ArmCcaRsiLib.h>
#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include "ArmCcaRsi.h"

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
  Check if the address is aligned to the size of the Realm granule.

  @param [in] Address          Address to check granule alignment.

  @retval TRUE  Address is aligned to the Realm granule size.
  @retval FALSE Address is not aligned to the Realm granule size.
**/
STATIC
BOOLEAN
EFIAPI
AddrIsGranuleAligned (
  IN   UINT64  *Address
  )
{
  if (((UINT64)Address & (REALM_GRANULE_SIZE - 1)) != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Continue the operation to retrieve an attestation token.

  @param [out]     TokenBuffer      Pointer to a buffer to store the
                                    retrieved attestation token.
  @param [in,out]  TokenSize        On input size of the token buffer,
                                    and on output size of the token
                                    returned if operation is successful,
                                    otherwise 0.

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
EFIAPI
RsiAttestationTokenContinue (
  OUT           UINT8   *CONST  TokenBuffer,
  IN OUT        UINT64  *CONST  TokenSize
  )
{
  RETURN_STATUS  Status;
  ARM_SMC_ARGS   SmcCmd;

  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = FID_RSI_ATTESTATION_TOKEN_CONTINUE;
  // Set the IPA of the Granule to which the token will be written.
  SmcCmd.Arg1 = (UINTN)TokenBuffer;

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
  @param [in]       TokenBuffer           Pointer to a buffer to store the
                                          retrieved attestation token.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter is invalid.
 **/
STATIC
RETURN_STATUS
EFIAPI
RsiAttestationTokenInit (
  IN      CONST UINT8   *CONST  ChallengeData,
  IN            UINT64          ChallengeDataSizeBits,
  IN            UINT8   *CONST  TokenBuffer
  )
{
  ARM_SMC_ARGS  SmcCmd;
  UINT8         *Buffer8;
  CONST UINT8   *Data8;
  UINT64        Count;
  UINT8         TailBits;

  /* See A7.2.2 Attestation token generation, RMM Specification, version A-bet0
     IWTKDD - If the size of the challenge provided by the relying party is less
     than 64 bytes, it should be zero-padded prior to calling
     RSI_ATTESTATION_TOKEN_INIT.

    Therefore, zero out the SmcCmd memory before coping the ChallengeData
    bits.
  */
  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = FID_RSI_ATTESTATION_TOKEN_INIT;
  // Set the IPA of the Granule to which the token will be written.
  SmcCmd.Arg1 = (UINTN)TokenBuffer;

  // Copy challenge data.
  Buffer8 = (UINT8 *)&SmcCmd.Arg2;
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
  return RsiCmdStatusToEfiStatus (SmcCmd.Arg0);
}

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
  )
{
  RETURN_STATUS  Status;

  if ((TokenBuffer == NULL) ||
      (TokenBufferSize == NULL) ||
      (ChallengeData == NULL))
  {
    return RETURN_INVALID_PARAMETER;
  }

  if (*TokenBufferSize < MAX_ATTESTATION_TOKEN_SIZE) {
    *TokenBufferSize = MAX_ATTESTATION_TOKEN_SIZE;
    return RETURN_BAD_BUFFER_SIZE;
  }

  if (!AddrIsGranuleAligned ((UINT64 *)TokenBuffer)) {
    DEBUG ((DEBUG_ERROR, "ERROR : Token buffer not granule aligned\n"));
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
             TokenBuffer
             );
  if (RETURN_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  /* Loop until the token is ready or there is an error.
  */
  do {
    Status = RsiAttestationTokenContinue (TokenBuffer, TokenBufferSize);
  } while (Status == RETURN_NOT_READY);

  return Status;
}

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
  )
{
  RETURN_STATUS  Status;
  ARM_SMC_ARGS   SmcCmd;

  if ((State == NULL) || (!AddrIsGranuleAligned (Address))) {
    return RETURN_INVALID_PARAMETER;
  }

  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = FID_RSI_IPA_STATE_GET;
  SmcCmd.Arg1 = (UINTN)Address;

  ArmCallSmc (&SmcCmd);
  Status = RsiCmdStatusToEfiStatus (SmcCmd.Arg0);
  if (!RETURN_ERROR (Status)) {
    *State = (RIPAS)(SmcCmd.Arg1 & RIPAS_TYPE_MASK);
  }

  return Status;
}

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
  )
{
  RETURN_STATUS  Status;
  UINT64         *BaseAddress;
  UINT64         *EndAddress;
  ARM_SMC_ARGS   SmcCmd;

  if ((Size == 0) ||
      ((Size & (REALM_GRANULE_SIZE - 1)) != 0) ||
      (!AddrIsGranuleAligned (Address)))
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

    ArmCallSmc (&SmcCmd);
    Status = RsiCmdStatusToEfiStatus (SmcCmd.Arg0);
    if (RETURN_ERROR (Status)) {
      break;
    }

    BaseAddress = (UINT64 *)SmcCmd.Arg1;
    Size        = EndAddress - BaseAddress;
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

  if ((Config == NULL) || (!AddrIsGranuleAligned ((UINT64 *)Config))) {
    return RETURN_INVALID_PARAMETER;
  }

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

  if ((Args == NULL) || (!AddrIsGranuleAligned ((UINT64 *)Args))) {
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
  )
{
  ARM_SMC_ARGS  SmcCmd;

  if ((Major == NULL) || (Minor == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&SmcCmd, sizeof (SmcCmd));
  SmcCmd.Arg0 = FID_RSI_VERSION;

  ArmCallSmc (&SmcCmd);
  *Minor = SmcCmd.Arg0 & RSI_VER_MINOR_MASK;
  *Major = (SmcCmd.Arg0 & RSI_VER_MAJOR_MASK) >> RSI_VER_MAJOR_SHIFT;
  return RETURN_SUCCESS;
}

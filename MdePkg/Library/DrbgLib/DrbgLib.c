/** @file
  Drbg library.
  Cf. [1] s9 DRBG Mechanism Functions

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - [1] NIST Special Publication 800-90A Revision 1, June 2015, Recommendation
        for Random Number Generation Using Deterministic Random Bit Generators.
        (https://csrc.nist.gov/publications/detail/sp/800-90a/rev-1/final)
  - [2] NIST Special Publication 800-90B, Recommendation for the Entropy
        Sources Used for Random Bit Generation.
        (https://csrc.nist.gov/publications/detail/sp/800-90b/final)
  - [3] (Second Draft) NIST Special Publication 800-90C, Recommendation for
        Random Bit Generator (RBG) Constructions.
        (https://csrc.nist.gov/publications/detail/sp/800-90c/draft)
  - [4] NIST Special Publication 800-57 Part 1 Revision 5, May 2020,
        Recommendation for Key Management:Part 1 - General.
        (https://csrc.nist.gov/publications/detail/sp/800-57-part-1/rev-5/final)
  - [5] Unified Extensible Firmware Interface (UEFI) Specification,
        Version 2.8 Errata B, May 2020
        (https://www.uefi.org/specifications)

  @par Glossary:
    - TRNG - True Random Number Generator
    - Sec  - Security
    - DRBG - Deterministic Random Bits Generator
    - CTR  - Counter
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

#include "Common.h"
#include "GetEntropyInput.h"

/** Check the internal state of the Drbg handle.

  @param  [in] DrbgHandle   The Drbg handle.

  @retval TRUE  The Drbg handle has a valid internal state.
  @retval FALSE Otherwise.
**/
STATIC
BOOLEAN
CheckInternalState (
  IN  DRBG_HANDLE  DrbgHandle
  )
{
  if ((DrbgHandle == NULL)  ||
      EFI_ERROR (DrbgHandle->DrbgCheckInternalState (DrbgHandle)))
  {
    ASSERT (DrbgHandle != NULL);
    ASSERT_EFI_ERROR (DrbgHandle->DrbgCheckInternalState (DrbgHandle));
    return FALSE;
  }

  return TRUE;
}

/** Reseed a DRBG instance.

  Implementation of Reseed_function.
  Cf. [1] s9.2 'Reseeding a DRBG Instantiation'

  @param  [in] PredResRequest   Indicates whether prediction resistance
                                is to be provided during the request.
                                Might not be supported by all Drbgs.
  @param  [in] AddInput         An optional additional input.
                                Might not be supported by all Drbgs.
  @param  [in] AddInputLen      Additional input length (in bits).
                                Might not be supported by all Drbgs.
  @param  [in, out] DrbgHandle  The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
DrbgReseedFn (
  IN        BOOLEAN  PredResRequest,
  IN  CONST CHAR8    *AddInput,
  IN        UINTN    AddInputLen,
  IN  OUT   VOID     *Handle
  )
{
  EFI_STATUS   Status;
  DRBG_HANDLE  DrbgHandle;
  BIT_STREAM   *AddInputStream;
  BIT_STREAM   *EntropyBitsStream;

  DrbgHandle = (DRBG_HANDLE)Handle;

  // 1. Using state_handle, obtain the current internal state.
  // If state_handle indicates an invalid or unused internal state,
  // return (ERROR_FLAG).
  if (((AddInput == NULL) ^ (AddInputLen == 0)) ||
      !CheckInternalState (DrbgHandle))
  {
    ASSERT (!((AddInput == NULL) ^ (AddInputLen == 0)));
    ASSERT (CheckInternalState (DrbgHandle));
    return EFI_INVALID_PARAMETER;
  }

  AddInputStream    = NULL;
  EntropyBitsStream = NULL;

  // 2. If prediction_resistance_request is set, and prediction_resistance_flag
  // is not set, then return (ERROR_FLAG).
  if (PredResRequest && !DrbgHandle->IntState.PredResFlag) {
    ASSERT (!(PredResRequest && !DrbgHandle->IntState.PredResFlag));
    return EFI_INVALID_PARAMETER;
  }

  // 3. If the length of the additional_input > max_additional_input_length,
  // return (ERROR_FLAG).
  if (AddInputLen > DrbgHandle->DrbgVal.MaxAddInputLen) {
    ASSERT (AddInputLen <= DrbgHandle->DrbgVal.MaxAddInputLen);
    return EFI_INVALID_PARAMETER;
  }

  // 4. (status, entropy_input) = Get_entropy_input (security_strength,
  // min_length, max_length, prediction_resistance_request).
  // 5. If (status != SUCCESS), return (status).
  //
  // Note: in this implementation, there is no difference between
  // ERROR_FLAG and CATASTROPHIC_ERROR_FLAG.
  Status = DrbgHandle->DrbgGetEntropyInput (
                         DrbgHandle,
                         DrbgHandle->DrbgVal.MinLen,
                         &EntropyBitsStream
                         );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Create a BitStream for AddInput, even for a NULL AddInput. AddInput is
  // used during instantiation but doesn't persist in the Drbg handle.
  Status = BitStreamInit (
             (UINT8 *)AddInput,
             AddInputLen,
             &AddInputStream
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 6. new_working_state = Reseed_algorithm (working_state, entropy_input,
  // additional_input).
  // 7. Replace the working_state in the internal state for the DRBG
  // instantiation (e.g., as indicated by state_handle) with the values of
  // new_working_state obtained in step 6.
  Status = DrbgHandle->DrbgReseedAlgo (
                         EntropyBitsStream,
                         AddInputStream,
                         DrbgHandle
                         );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    // Fall trough
  }

ExitHandler:
  if (AddInputStream != NULL) {
    BitStreamFree (&AddInputStream);
  }

  if (EntropyBitsStream != NULL) {
    BitStreamFree (&EntropyBitsStream);
  }

  // 8. Return (SUCCESS).
  return Status;
}

/** Create a Drbg instance.

  Implementation of Instantiate_function.
  Cf. [1] s9.1 Instantiating a DRBG

  @param  [in] DrbgMechanism    DRBG mechanism chosen.
  @param  [in] DrbgEntropySrc   Entropy source chosen.
  @param  [in] ReqSecStrength   Requested security strength (in bits).
                                The security strenght granted can be different.
  @param  [in] PredRes          Prediction resistance flag.
                                If relevant, instantiate a DRBG that supports
                                prediction resistance.
                                Might not be supported by all Drbgs.
  @param  [in] PersStr          Personnalization string.
                                Might not be supported by all Drbgs.
  @param  [in] PersStrLen       Personnalization string length (in bits).
                                Might not be supported by all Drbgs.
  @param  [out] HandlePtr   Pointer containting the created Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
DrbgInstantiateFn (
  IN        DRBG_MECHANISM    DrbgMechanism,
  IN        DRBG_ENTROPY_SRC  DrbgEntropySrc,
  IN        UINTN             ReqSecStrength,
  IN        BOOLEAN           PredRes,
  IN  CONST CHAR8             *PersStr,
  IN        UINTN             PersStrLen,
  OUT       VOID              **HandlePtr
  )
{
  EFI_STATUS   Status;
  BIT_STREAM   *EntropyBitsStream;
  BIT_STREAM   *PersStrBitStream;
  DRBG_HANDLE  DrbgHandle;

  if ((ReqSecStrength == 0)                   ||
      ((PersStr == NULL) ^ (PersStrLen == 0)) ||
      (HandlePtr == NULL))
  {
    ASSERT (ReqSecStrength != 0);
    ASSERT (!((PersStr == NULL) ^ (PersStrLen == 0)));
    ASSERT (HandlePtr != NULL);
    return EFI_INVALID_PARAMETER;
  }

  // Allocate a Drbg.
  DrbgHandle = AllocateZeroPool (sizeof (DRBG_INFO));
  if (DrbgHandle == NULL) {
    ASSERT (DrbgHandle != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  // Init the handle according to the mechanism.
  switch (DrbgMechanism) {
    case DrbgMechansimCtr:
      Status = CtrInitHandle (DrbgHandle);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto ExitHandler;
      }

      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
  }

  // Init the handle according to entropy.
  switch (DrbgEntropySrc) {
    case DrbgEntropyNoCondFn:
      DrbgHandle->DrbgGetEntropyInput = GetEntropyInputNoCondFn;
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
  }

  EntropyBitsStream = NULL;
  PersStrBitStream  = NULL;

  // 1. If requested_instantiation_security_strength >
  //                  highest_supported_security_strength,
  // then return (ERROR_FLAG, Invalid).
  if (ReqSecStrength > DrbgHandle->DrbgVal.HighestSuppSecStrength) {
    ASSERT (ReqSecStrength <= DrbgHandle->DrbgVal.HighestSuppSecStrength);
    goto ExitHandler;
  }

  // 2. If prediction_resistance_flag is set, and prediction resistance
  // is not supported, then return (ERROR_FLAG, Invalid).
  if (!DrbgHandle->PredResSupported && PredRes) {
    ASSERT (!(!DrbgHandle->PredResSupported && PredRes));
    goto ExitHandler;
  }

  if (PersStr != NULL) {
    // 3. If the length of the personalization_string >
    //                max_personalization_string_length,
    // return (ERROR_FLAG, Invalid).
    if (PersStrLen > DrbgHandle->DrbgVal.MaxPersStrLen) {
      ASSERT (PersStrLen <= DrbgHandle->DrbgVal.MaxPersStrLen);
      goto ExitHandler;
    }
  }

  // 4. Set security_strength to the lowest security strength greater than or
  // equal to requested_instantiation_security_strength from the set
  // {112, 128, 192, 256}.
  //
  // Note: [5], Section 37.5 Random Number Generator Protocol:
  // 'When a Deterministic Random Bit Generator (DRBG) is used on the
  // output of a (raw) entropy source, its security level must be at
  // least 256 bits.'
  // So set the security strength to 256.
  //
  // Note2: Set it here so CTR_DRBG_Instantiate_algorithm has access to it.
  DrbgHandle->IntState.SecStrength = SecStrength256bits;
  DEBUG ((
    DEBUG_INFO,
    "Requested security strength = %d bits. " \
    "Setting security strength for DRBG to %d bits.\n",
    ReqSecStrength,
    SecStrength256bits
    ));

  // Create a BitStream for PersStr, even for a NULL PersStr. PersStr is
  // used during instantiation but doesn't persist in the Drbg handle.
  Status = BitStreamInit (
             (UINT8 *)PersStr,
             PersStrLen,
             &PersStrBitStream
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 5. Null step.
  // 6. (status, entropy_input) = Get_entropy_input (security_strength,
  // min_length, max_length, prediction_resistance_request).
  //
  // Note: in this implementation, there is no difference between
  // ERROR_FLAG and CATASTROPHIC_ERROR_FLAG.
  //
  // 7. If (status != SUCCESS), return (status, Invalid).
  Status = DrbgHandle->DrbgGetEntropyInput (
                         DrbgHandle,
                         DrbgHandle->DrbgVal.MinLen,
                         &EntropyBitsStream
                         );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 8. Obtain a nonce.
  Status = DrbgHandle->DrbgGetNonce (DrbgHandle, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 9. initial_working_state = Instantiate_algorithm (entropy_input, nonce,
  //            personalization_string, security_strength).
  Status = DrbgHandle->DrbgInstantiateAlgo (
                         EntropyBitsStream,
                         PersStrBitStream,
                         DrbgHandle
                         );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 10. Get a state_handle for a currently empty internal state. If an empty
  // internal state cannot be found, return (ERROR_FLAG, Invalid).
  // 11. Set the internal state for the new instantiation (e.g., as indicated
  // by state_handle) to the initial values for the internal state (i.e., set
  // the working_state to the values returned as initial_working_state in
  // step 9) and any other values required for the working_state (see
  // Section 10), and set the administrative information to the appropriate
  // values (e.g., the values of security_strength and the
  // prediction_resistance_flag).
  DrbgHandle->IntState.PredResFlag = PredRes;
  // DrbgInstantiateAlgo already sets ReseedCounter

ExitHandler:
  if (EntropyBitsStream != NULL) {
    BitStreamFree (&EntropyBitsStream);
  }

  if (PersStrBitStream != NULL) {
    BitStreamFree (&PersStrBitStream);
  }

  if (EFI_ERROR (Status)) {
    FreePool (DrbgHandle);
    DrbgHandle = NULL;
  }

  // 12. Return (SUCCESS, state_handle).
  *HandlePtr = (VOID **)DrbgHandle;
  return Status;
}

/** Generate a random number.

  Implementation of Generate_function.
  Cf. [1] s9.3.1 The Generate Function

  @param  [in] ReqSecStrength   Requested security strength (in bits).
                                If the DrbgHandle cannot satisfy the request,
                                an error is returned.
  @param  [in] PredResReq       Request prediction resistance.
                                If the DrbgHandle cannot satisfy the request,
                                an error is returned.
  @param  [in] AddInput         Additional input.
                                Might not be supported by all Drbgs.
  @param  [in] AddInputLen      Additional input length (in bits).
                                Might not be supported by all Drbgs.
  @param  [in] ReqNbBits        Number of random bits requested.
  @param  [in, out] OutBuffer   If success, contains the random bits.
                                The buffer must be at least ReqNbBits bits
                                long.
  @param  [in, out] DrbgHandle  The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
DrbgGenerateFn (
  IN        UINTN    ReqSecStrength,
  IN        BOOLEAN  PredResReq,
  IN  CONST CHAR8    *AddInput,
  IN        UINTN    AddInputLen,
  IN        UINTN    ReqNbBits,
  IN  OUT   UINT8    *OutBuffer,
  IN  OUT   VOID     *Handle
  )
{
  EFI_STATUS   Status;
  DRBG_HANDLE  DrbgHandle;
  BOOLEAN      ReseedReq;
  BIT_STREAM   *AddInputStream;
  BIT_STREAM   *RandomBitsStream;

  DrbgHandle = (DRBG_HANDLE)Handle;

  // 1. Using state_handle, obtain the current internal state for the
  // instantiation. If state_handle indicates an invalid or unused internal
  // state, then return (ERROR_FLAG, Null).
  if ((ReqSecStrength == 0)                     ||
      ((AddInput != NULL) ^ (AddInputLen != 0)) ||
      (ReqNbBits == 0)                          ||
      (OutBuffer == NULL)                       ||
      !CheckInternalState (DrbgHandle))
  {
    ASSERT (ReqSecStrength != 0);
    ASSERT (!((AddInput != NULL) ^ (AddInputLen != 0)));
    ASSERT (ReqNbBits != 0);
    ASSERT (OutBuffer != NULL);
    ASSERT (CheckInternalState (DrbgHandle));
    return EFI_INVALID_PARAMETER;
  }

  AddInputStream   = NULL;
  RandomBitsStream = NULL;

  // 2. If requested_number_of_bits > max_number_of_bits_per_request,
  // then return (ERROR_FLAG, Null).
  if (ReqNbBits > DrbgHandle->DrbgVal.MaxNbBitsPerRequest) {
    ASSERT (ReqNbBits <= DrbgHandle->DrbgVal.MaxNbBitsPerRequest);
    return EFI_INVALID_PARAMETER;
  }

  // 3. If requested_security_strength > the security_strength indicated
  // in the internal state, then return (ERROR_FLAG, Null).
  if (ReqSecStrength > DrbgHandle->IntState.SecStrength) {
    ASSERT (ReqSecStrength <= DrbgHandle->IntState.SecStrength);
    return EFI_INVALID_PARAMETER;
  }

  // 4. If the length of the additional_input > max_additional_input_length,
  // then return (ERROR_FLAG, Null).
  if (AddInputLen > DrbgHandle->DrbgVal.MaxAddInputLen) {
    ASSERT (AddInputLen <= DrbgHandle->DrbgVal.MaxAddInputLen);
    return EFI_INVALID_PARAMETER;
  }

  // 5. If prediction_resistance_request is set, and prediction_resistance_flag
  // is not set, then return (ERROR_FLAG, Null).
  if (PredResReq && !DrbgHandle->IntState.PredResFlag) {
    ASSERT (!(PredResReq && !DrbgHandle->IntState.PredResFlag));
    return EFI_INVALID_PARAMETER;
  }

  // 6. Clear the reseed_required_flag.
  ReseedReq = FALSE;

  // 7. If reseed_required_flag is set, or if prediction_resistance_request is
  // set, then
Step7:
  if (ReseedReq || PredResReq) {
    // 7.1 status = Reseed_function (state_handle,
    //                  prediction_resistance_request, additional_input).
    // 7.2 If (status != SUCCESS), then return (status, Null).
    // 7.3 Using state_handle, obtain the new internal state.
    //
    // Note: in this implementation, there is no difference between
    // ERROR_FLAG and CATASTROPHIC_ERROR_FLAG.
    Status = DrbgReseedFn (
               PredResReq,
               AddInput,
               AddInputLen,
               DrbgHandle
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ExitHandler;
    }

    // 7.4 additional_input = the Null string.
    AddInput    = NULL;
    AddInputLen = 0;

    // 7.5 Clear the reseed_required_flag.
    ReseedReq = FALSE;
  }

  // Create a BitStream for AddInput, even for a NULL AddInput. AddInput is
  // used during instantiation but doesn't persist in the Drbg handle.
  Status = BitStreamInit (
             (UINT8 *)AddInput,
             AddInputLen,
             &AddInputStream
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // 8. (status, pseudorandom_bits, new_working_state) =
  // Generate_algorithm (working_state, requested_number_of_bits,
  //            additional_input).
  Status = DrbgHandle->DrbgGenerateAlgo (
                         AddInputStream,
                         ReqNbBits,
                         &RandomBitsStream,
                         DrbgHandle
                         );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_READY)) {
    ASSERT_EFI_ERROR (Status);
    goto ExitHandler;
  }

  // Free AddInputStream now that it has been used.
  if (AddInputStream != NULL) {
    BitStreamFree (&AddInputStream);
  }

  // 9. If status indicates that a reseed is required before the requested
  // bits can be generated, then
  if (Status == EFI_NOT_READY) {
    if (DrbgHandle->ReseedSupported) {
      // 9.1 Set the reseed_required_flag.
      ReseedReq = TRUE;

      // 9.2 If the prediction_resistance_flag is set, then set the
      // prediction_resistance request indication.
      if (DrbgHandle->IntState.PredResFlag) {
        PredResReq = TRUE;
      }

      // 9.3 Go to step 7.
      goto Step7;
    } else {
      // Implementation notes:
      // If a reseed capability is not supported, or a reseed is not desired,
      // then generate process steps 6 and 7 are removed; generate process
      // step 9 is replaced by:
      // 9. If status indicates that a reseed is required before the requested
      // bits can be generated, then
      // 9.1 status = Uninstantiate_function (state_handle).
      //
      // No need to check the returned status.
      DrbgUninstantiateFn (DrbgHandle);

      // 9.2 Return an indication that the DRBG instantiation can no longer be used.
      goto ExitHandler;
    }
  }

  // The Drbg succeeded, copy the random bits.
  Status = BitStreamToBuffer (RandomBitsStream, OutBuffer);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    // Fall through.
  }

  // 10. Replace the old working_state in the internal state of the DRBG
  // instantiation (e.g., as indicated by state_handle) with the values of
  // new_working_state.
  // 11. Return (SUCCESS, pseudorandom_bits).
ExitHandler:
  if (AddInputStream != NULL) {
    BitStreamFree (&AddInputStream);
  }

  if (RandomBitsStream != NULL) {
    BitStreamFree (&RandomBitsStream);
  }

  return Status;
}

/** Remove a DRBG instance.

  Implementation of Uninstantiate_function.
  Cf. [1] s9.4 Removing a DRBG Instantiation

  @param  [in, out] DrbgHandle    The Drbg handle.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
DrbgUninstantiateFn (
  IN  OUT VOID  *Handle
  )
{
  DRBG_HANDLE  DrbgHandle;

  DrbgHandle = (DRBG_HANDLE)Handle;
  return DrbgHandle->DrbgUninstantiateFn (DrbgHandle);
}

/** @file
  Arm Firmware TRNG interface library.

  Copyright (c) 2021 - 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - [1] Arm True Random Number Generator Firmware, Interface 1.0,
        Platform Design Document.
        (https://developer.arm.com/documentation/den0098/latest/)
  - [2] NIST Special Publication 800-90B, Recommendation for the Entropy
        Sources Used for Random Bit Generation.
        (https://csrc.nist.gov/publications/detail/sp/800-90b/final)

  @par Glossary:
    - TRNG - True Random Number Generator
    - FID  - Function ID
**/

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/ArmMonitorLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "ArmTrngDefs.h"

/** Convert TRNG status codes to RETURN status codes.

  @param [in]  TrngStatus    TRNG status code.

  @retval  RETURN_SUCCESS            Success.
  @retval  RETURN_UNSUPPORTED        Function not implemented or
                                     negative return code.
  @retval  RETURN_INVALID_PARAMETER  A parameter is invalid.
  @retval  RETURN_NOT_READY          No Entropy available.
**/
STATIC
RETURN_STATUS
TrngStatusToReturnStatus (
  IN  INT32  TrngStatus
  )
{
  switch (TrngStatus) {
    case TRNG_STATUS_NOT_SUPPORTED:
      return RETURN_UNSUPPORTED;

    case TRNG_STATUS_INVALID_PARAMETER:
      return RETURN_INVALID_PARAMETER;

    case TRNG_STATUS_NO_ENTROPY:
      return RETURN_NOT_READY;

    case TRNG_STATUS_SUCCESS:
      return RETURN_SUCCESS;

    default:
      if (TrngStatus < 0) {
        return RETURN_UNSUPPORTED;
      }

      return RETURN_SUCCESS;
  }
}

/** Get the version of the Arm TRNG backend.

  A TRNG may be implemented by the system firmware, in which case this
  function shall return the version of the Arm TRNG backend.
  The implementation must return NOT_SUPPORTED if a Back end is not present.

  @param [out]  MajorRevision     Major revision.
  @param [out]  MinorRevision     Minor revision.

  @retval  RETURN_SUCCESS            The function completed successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid parameter.
  @retval  RETURN_UNSUPPORTED        Backend not present.
**/
RETURN_STATUS
EFIAPI
GetArmTrngVersion (
  OUT UINT16  *MajorRevision,
  OUT UINT16  *MinorRevision
  )
{
  RETURN_STATUS     Status;
  ARM_MONITOR_ARGS  Parameters;
  INT32             Revision;

  if ((MajorRevision == NULL) || (MinorRevision == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  ZeroMem (&Parameters, sizeof (Parameters));

  Parameters.Arg0 = ARM_SMC_ID_TRNG_VERSION;
  ArmMonitorCall (&Parameters);

  Revision = (INT32)Parameters.Arg0;
  Status   = TrngStatusToReturnStatus (Revision);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  *MinorRevision = (Revision & TRNG_REV_MINOR_MASK);
  *MajorRevision = ((Revision >> TRNG_REV_MAJOR_SHIFT) & TRNG_REV_MAJOR_MASK);
  return RETURN_SUCCESS;
}

/** Get the features supported by the Arm TRNG backend.

  The caller can determine if functions defined in the Arm TRNG ABI are
  present in the ABI implementation.

  @param [in]  FunctionId         Function Id.
  @param [out] Capability         Function specific capability if present.

  @retval  RETURN_SUCCESS            The function completed successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid parameter.
  @retval  RETURN_UNSUPPORTED        Function not implemented.
**/
STATIC
RETURN_STATUS
EFIAPI
GetArmTrngFeatures (
  IN  CONST UINT32  FunctionId,
  OUT       UINT32  *Capability      OPTIONAL
  )
{
  ARM_MONITOR_ARGS  Parameters;
  RETURN_STATUS     Status;

  ZeroMem (&Parameters, sizeof (Parameters));

  Parameters.Arg0 = ARM_SMC_ID_TRNG_FEATURES;
  Parameters.Arg1 = FunctionId;
  ArmMonitorCall (&Parameters);

  Status = TrngStatusToReturnStatus (Parameters.Arg0);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  if (Capability != NULL) {
    *Capability = (UINT32)Parameters.Arg0;
  }

  return RETURN_SUCCESS;
}

/** Get the UUID of the Arm TRNG backend.

  A TRNG may be implemented by the system firmware, in which case this
  function shall return the UUID of the TRNG backend.
  Returning the Arm TRNG UUID is optional and if not implemented,
  RETURN_UNSUPPORTED shall be returned.

  Note: The caller must not rely on the returned UUID as a trustworthy Arm TRNG
        Back end identity

  @param [out]  Guid              UUID of the Arm TRNG backend.

  @retval  RETURN_SUCCESS            The function completed successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid parameter.
  @retval  RETURN_UNSUPPORTED        Function not implemented.
**/
RETURN_STATUS
EFIAPI
GetArmTrngUuid (
  OUT GUID  *Guid
  )
{
  ARM_MONITOR_ARGS  Parameters;

  if (Guid == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  ZeroMem (&Parameters, sizeof (Parameters));

  Parameters.Arg0 = ARM_SMC_ID_TRNG_GET_UUID;
  ArmMonitorCall (&Parameters);

  // Only invalid value is TRNG_STATUS_NOT_SUPPORTED (-1).
  if ((INT32)Parameters.Arg0 == TRNG_STATUS_NOT_SUPPORTED) {
    return TrngStatusToReturnStatus ((INT32)Parameters.Arg0);
  }

  Guid->Data1 = (Parameters.Arg0 & MAX_UINT32);
  Guid->Data2 = (Parameters.Arg1 & MAX_UINT16);
  Guid->Data3 = ((Parameters.Arg1 >> 16) & MAX_UINT16);

  Guid->Data4[0] = (Parameters.Arg2 & MAX_UINT8);
  Guid->Data4[1] = ((Parameters.Arg2 >> 8) & MAX_UINT8);
  Guid->Data4[2] = ((Parameters.Arg2 >> 16) & MAX_UINT8);
  Guid->Data4[3] = ((Parameters.Arg2 >> 24) & MAX_UINT8);

  Guid->Data4[4] = (Parameters.Arg3 & MAX_UINT8);
  Guid->Data4[5] = ((Parameters.Arg3 >> 8) & MAX_UINT8);
  Guid->Data4[6] = ((Parameters.Arg3 >> 16) & MAX_UINT8);
  Guid->Data4[7] = ((Parameters.Arg3 >> 24) & MAX_UINT8);

  DEBUG ((DEBUG_INFO, "FW-TRNG: UUID %g\n", Guid));

  return RETURN_SUCCESS;
}

/** Returns maximum number of entropy bits that can be returned in a single
    call.

  @return Returns the maximum number of Entropy bits that can be returned
          in a single call to GetArmTrngEntropy().
**/
UINTN
EFIAPI
GetArmTrngMaxSupportedEntropyBits (
  VOID
  )
{
  return MAX_ENTROPY_BITS;
}

/** Returns N bits of conditioned entropy.

  See [2] Section 2.3.1 GetEntropy: An Interface to the Entropy Source
    GetEntropy
      Input:
        bits_of_entropy: the requested amount of entropy
      Output:
        entropy_bitstring: The string that provides the requested entropy.
      status: A Boolean value that is TRUE if the request has been satisfied,
              and is FALSE otherwise.

  @param  [in]   EntropyBits  Number of entropy bits requested.
  @param  [in]   BufferSize   Size of the Buffer in bytes.
  @param  [out]  Buffer       Buffer to return the entropy bits.

  @retval  RETURN_SUCCESS            The function completed successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid parameter.
  @retval  RETURN_UNSUPPORTED        Function not implemented.
  @retval  RETURN_BAD_BUFFER_SIZE    Buffer size is too small.
  @retval  RETURN_NOT_READY          No Entropy available.
**/
RETURN_STATUS
EFIAPI
GetArmTrngEntropy (
  IN  UINTN  EntropyBits,
  IN  UINTN  BufferSize,
  OUT UINT8  *Buffer
  )
{
  RETURN_STATUS     Status;
  ARM_MONITOR_ARGS  Parameters;
  UINTN             EntropyBytes;
  UINTN             LastValidBits;
  UINTN             BytesToClear;
  UINTN             EntropyData[3];

  if ((EntropyBits == 0)                ||
      (EntropyBits > MAX_ENTROPY_BITS)  ||
      (Buffer == NULL))
  {
    return RETURN_INVALID_PARAMETER;
  }

  EntropyBytes = (EntropyBits + 7) >> 3;
  if (EntropyBytes > BufferSize) {
    return RETURN_BAD_BUFFER_SIZE;
  }

  ZeroMem (Buffer, BufferSize);
  ZeroMem (&Parameters, sizeof (Parameters));

  Parameters.Arg0 = ARM_SMC_ID_TRNG_RND;
  Parameters.Arg1 = EntropyBits;
  ArmMonitorCall (&Parameters);

  Status = TrngStatusToReturnStatus ((INT32)Parameters.Arg0);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  // The entropy data is returned in the Parameters.Arg<3..1>
  // With the lower order bytes in Parameters.Arg3 and the higher
  // order bytes being stored in Parameters.Arg1.
  EntropyData[0] = Parameters.Arg3;
  EntropyData[1] = Parameters.Arg2;
  EntropyData[2] = Parameters.Arg1;

  CopyMem (Buffer, EntropyData, EntropyBytes);

  // Mask off any unused top bytes, in accordance with specification.
  BytesToClear = BufferSize - EntropyBytes;
  if (BytesToClear != 0) {
    ZeroMem (&Buffer[EntropyBytes], BytesToClear);
  }

  // Clear the unused MSB bits of the last byte.
  LastValidBits = EntropyBits & 0x7;
  if (LastValidBits != 0) {
    Buffer[EntropyBytes - 1] &= (0xFF >> (8 - LastValidBits));
  }

  return Status;
}

/** The constructor checks that the FW-TRNG interface is supported
    by the host firmware.

  It will ASSERT() if FW-TRNG is not supported.
  It will always return RETURN_SUCCESS.

  @retval RETURN_SUCCESS   The constructor always returns RETURN_SUCCESS.
**/
RETURN_STATUS
EFIAPI
ArmTrngLibConstructor (
  VOID
  )
{
  ARM_MONITOR_ARGS  Parameters;
  RETURN_STATUS     Status;
  UINT16            MajorRev;
  UINT16            MinorRev;
  GUID              Guid;

  ZeroMem (&Parameters, sizeof (Parameters));

  Parameters.Arg0 = SMCCC_VERSION;
  ArmMonitorCall (&Parameters);
  Status = TrngStatusToReturnStatus ((INT32)Parameters.Arg0);
  if (RETURN_ERROR (Status)) {
    goto ErrorHandler;
  }

  // Cf [1] s2.1.3 'Caller responsibilities',
  // SMCCC version must be greater or equal than 1.1
  if ((INT32)Parameters.Arg0 < 0x10001) {
    goto ErrorHandler;
  }

  Status = GetArmTrngVersion (&MajorRev, &MinorRev);
  if (RETURN_ERROR (Status)) {
    goto ErrorHandler;
  }

  // Check that the required features are present.
  Status = GetArmTrngFeatures (ARM_SMC_ID_TRNG_RND, NULL);
  if (RETURN_ERROR (Status)) {
    goto ErrorHandler;
  }

  // Check if TRNG UUID is supported and if so trace the GUID.
  Status = GetArmTrngFeatures (ARM_SMC_ID_TRNG_GET_UUID, NULL);
  if (RETURN_ERROR (Status)) {
    goto ErrorHandler;
  }

  DEBUG_CODE_BEGIN ();

  Status = GetArmTrngUuid (&Guid);
  if (RETURN_ERROR (Status)) {
    goto ErrorHandler;
  }

  DEBUG ((
    DEBUG_INFO,
    "FW-TRNG: Version %d.%d, GUID {%g}\n",
    MajorRev,
    MinorRev,
    &Guid
    ));

  DEBUG_CODE_END ();

  return RETURN_SUCCESS;

ErrorHandler:
  DEBUG ((DEBUG_ERROR, "ArmTrngLib could not be correctly initialized.\n"));
  return RETURN_SUCCESS;
}

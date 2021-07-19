/** @file
  Arm Firmware TRNG interface library.

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - [1] Arm True Random Number Generator Firmware, Interface 1.0,
        Platform Design Document.
        (https://developer.arm.com/documentation/den0098/latest/)
  - [2] NIST Special Publication 800-90A Revision 1, June 2015, Recommendation
        for Random Number Generation Using Deterministic Random Bit Generators.
        (https://csrc.nist.gov/publications/detail/sp/800-90a/rev-1/final)
  - [3] NIST Special Publication 800-90B, Recommendation for the Entropy
        Sources Used for Random Bit Generation.
        (https://csrc.nist.gov/publications/detail/sp/800-90b/final)
  - [4] (Second Draft) NIST Special Publication 800-90C, Recommendation for
        Random Bit Generator (RBG) Constructions.
        (https://csrc.nist.gov/publications/detail/sp/800-90c/draft)

  @par Glossary:
    - TRNG - True Random Number Generator
    - FID  - Function ID
**/

#include <Base.h>
#include <Library/ArmHvcLib.h>
#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "ArmFwTrngDefs.h"

/** Convert TRNG status codes to EFI status codes.

  @param [in]  TrngStatus    TRNG status code.

  @retval  RETURN_SUCCESS            Success.
  @retval  RETURN_UNSUPPORTED        Function not implemented.
  @retval  RETURN_INVALID_PARAMETER  A parameter is invalid.
  @retval  RETURN_NOT_READY          No Entropy available.
**/
STATIC
RETURN_STATUS
TrngStatusToEfiStatus (
  IN  INT32   TrngStatus
  )
{
  switch (TrngStatus) {
    case TRNG_NOT_SUPPORTED:
      return RETURN_UNSUPPORTED;

    case TRNG_INVALID_PARAMETER:
      return RETURN_INVALID_PARAMETER;

    case TRNG_NO_ENTROPY:
      return RETURN_NOT_READY;

    case TRNG_STATUS_SUCCESS:
    default:
      return RETURN_SUCCESS;
  }
}

/** Invoke the monitor call using the appropriate conduit.
    If PcdMonitorConduitHvc is TRUE use the HVC conduit else use SMC conduit.

  @param [in, out]  Args    Arguments passed to and returned from the monitor.

  @return  VOID
**/
STATIC
VOID
ArmCallMonitor (
  IN OUT ARM_MONITOR_ARGS   *Args
  )
{
  if (FeaturePcdGet (PcdMonitorConduitHvc)) {
    ArmCallHvc ((ARM_HVC_ARGS*)Args);
  } else {
    ArmCallSmc ((ARM_SMC_ARGS*)Args);
  }
}

/** Get the version of the TRNG backend.

  A TRNG may be implemented by the system firmware, in which case this
  function shall return the version of the TRNG backend.
  The implementation must return NOT_SUPPORTED if a Back end is not present.

  @param [out]  MajorRevision     Major revision.
  @param [out]  MinorRevision     Minor revision.

  @retval  RETURN_SUCCESS            The function completed successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid parameter.
  @retval  RETURN_UNSUPPORTED        Backend not present.
**/
RETURN_STATUS
EFIAPI
GetTrngVersion (
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

  /*
    Cf. [1], 2.1 TRNG_VERSION
    Function ID (W0) 0x8400_0050
    Parameters
        W1-W7 Reserved (MBZ)
    Returns
        Success (W0 > 0) W0[31] MBZ
        W0[30:16] Major revision
        W0[15:0] Minor revision
        W1 - W3 Reserved (MBZ)
    Error (W0 < 0)
        NOT_SUPPORTED Function not implemented
  */
  Parameters.Arg0 = FID_TRNG_VERSION;
  ArmCallMonitor (&Parameters);

  Revision = (INT32)Parameters.Arg0;
  // Convert status codes to EFI status codes.
  Status = TrngStatusToEfiStatus (Revision);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *MinorRevision = (Revision & TRNG_REV_MINOR_MASK);
  *MajorRevision = ((Revision >> TRNG_REV_MAJOR_SHIFT) & TRNG_REV_MAJOR_MASK);
  return RETURN_SUCCESS;
}

#ifndef MDEPKG_NDEBUG
/** Get the features supported by the TRNG backend.

  The caller can determine if functions defined in the TRNG ABI are
  present in the ABI implementation.

  @param [in]  FunctionId         Function Id.
  @param [out] Capability         Function specific capability if present
                                  otherwise Zero is returned.

  @retval  RETURN_SUCCESS            The function completed successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid parameter.
  @retval  RETURN_UNSUPPORTED        Function not implemented.
**/
STATIC
RETURN_STATUS
EFIAPI
GetTrngFeatures (
  IN  CONST UINT32  FunctionId,
  OUT       UINT32  *Capability      OPTIONAL
  )
{
  ARM_MONITOR_ARGS  Parameters;

  ZeroMem (&Parameters, sizeof (Parameters));

  /*
    Cf. [1], Section 2.2 TRNG_FEATURES
    Function ID (W0) 0x8400_0051
    Parameters
        W1 trng_func_id
        W2-W7 Reserved (MBZ)
    Returns
        Success (W0 >= 0)
          SUCCESS Function is implemented.
            > 0     Function is implemented and
                    has specific capabilities,
                    see function definition.
          Error (W0 < 0)
            NOT_SUPPORTED Function with FID=trng_func_id
            is not implemented
  */
  Parameters.Arg0 = FID_TRNG_FEATURES;
  Parameters.Arg1 = FunctionId;
  ArmCallMonitor (&Parameters);
  if (Parameters.Arg0 < TRNG_STATUS_SUCCESS) {
    return RETURN_UNSUPPORTED;
  }

  if (Capability != NULL) {
    *Capability = Parameters.Arg0;
  }

  return RETURN_SUCCESS;
}
#endif  //MDEPKG_NDEBUG

/** Get the UUID of the TRNG backend.

  A TRNG may be implemented by the system firmware, in which case this
  function shall return the UUID of the TRNG backend.
  Returning the TRNG UUID is optional and if not implemented, RETURN_UNSUPPORTED
  shall be returned.

  Note: The caller must not rely on the returned UUID as a trustworthy TRNG
        Back end identity

  @param [out]  Guid              UUID of the TRNG backend.

  @retval  RETURN_SUCCESS            The function completed successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid parameter.
  @retval  RETURN_UNSUPPORTED        Function not implemented.
**/
RETURN_STATUS
EFIAPI
GetTrngUuid (
  OUT GUID  *Guid
  )
{
  RETURN_STATUS     Status;
  ARM_MONITOR_ARGS  Parameters;

  ZeroMem (&Parameters, sizeof (Parameters));

  /*
    Cf. [1], Section 2.3 TRNG_GET_UUID
    Function ID (W0) 0x8400_0052
    Parameters
        W1-W7 Reserved (MBZ)
    Returns
        Success (W0 != -1)
            W0 UUID[31:0]
            W1 UUID[63:32]
            W2 UUID[95:64]
            W3 UUID[127:96]
        Error (W0 = -1)
            W0 NOT_SUPPORTED
  */
  Parameters.Arg0 = FID_TRNG_GET_UUID;
  ArmCallMonitor (&Parameters);

  // Convert status codes to EFI status codes.
  Status = TrngStatusToEfiStatus ((INT32)Parameters.Arg0);
  if (EFI_ERROR (Status)) {
    return Status;
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
          in a single call to GetEntropy().
**/
UINTN
EFIAPI
GetTrngMaxSupportedEntropyBits (
  VOID
  )
{
  return MAX_ENTROPY_BITS;
}

/** Returns N bits of conditioned entropy.

  See [3] Section 2.3.1 GetEntropy: An Interface to the Entropy Source
    GetEntropy
      Input:
        bits_of_entropy: the requested amount of entropy
      Output:
        entropy_bitstring: The string that provides the requested entropy.
      status: A Boolean value that is TRUE if the request has been satisfied,
              and is FALSE otherwise.

  Note: In this implementation this function returns a status code instead
        of a boolean value.
        This is also compatible with the definition of Get_Entropy, see [2]
        Section 7.4 Entropy Source Calls.
          (status, entropy_bitstring) = Get_Entropy (
                                          requested_entropy,
                                          max_length
                                          )

  @param  [in]   EntropyBits  Number of entropy bits requested.
  @param  [out]  Buffer       Buffer to return the entropy bits.
  @param  [in]   BufferSize   Size of the Buffer in bytes.

  @retval  RETURN_SUCCESS            The function completed successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid parameter.
  @retval  RETURN_UNSUPPORTED        Function not implemented.
  @retval  RETURN_BAD_BUFFER_SIZE    Buffer size is too small.
  @retval  RETURN_NOT_READY          No Entropy available.
**/
RETURN_STATUS
EFIAPI
GetEntropy (
  IN  CONST UINTN   EntropyBits,
  OUT       UINT8   *Buffer,
  IN  CONST UINTN   BufferSize
  )
{
  RETURN_STATUS     Status;
  ARM_MONITOR_ARGS  Parameters;
  UINTN             EntropyBytes;
  UINTN             LastValidBits;
  UINTN             ArgSelector;
  UINTN             BytesToClear;

  // [1] Section 2.4.3 Caller responsibilities.
  // The caller cannot request more than MAX_BITS bits of conditioned
  // entropy per call.
  if ((EntropyBits == 0) || (EntropyBits > MAX_ENTROPY_BITS)) {
    return RETURN_INVALID_PARAMETER;
  }

  EntropyBytes = (EntropyBits + 7) >> 3;
  if (EntropyBytes > BufferSize) {
    return RETURN_BAD_BUFFER_SIZE;
  }

  ZeroMem (Buffer, BufferSize);
  ZeroMem (&Parameters, sizeof (Parameters));

  /*
    Cf. [1], Section 2.4 TRNG_RND
    Function ID (W0)  0x8400_0053
                      0xC400_0053
    SMC32 Parameters
        W1    N bits of entropy (1 6 N 6 96)
        W2-W7 Reserved (MBZ)
    SMC64 Parameters
        X1    N bits of entropy (1 6 N 6 192)
        X2-X7 Reserved (MBZ)
    SMC32 Returns
        Success (W0 = 0):
          W0 MBZ
          W1 Entropy[95:64]
          W2 Entropy[63:32]
          W3 Entropy[31:0]
    Error (W0 < 0)
          W0 NOT_SUPPORTED
          NO_ENTROPY
          INVALID_PARAMETERS
          W1 - W3 Reserved (MBZ)
    SMC64 Returns
          Success (X0 = 0):
          X0 MBZ
          X1 Entropy[191:128]
          X2 Entropy[127:64]
          X3 Entropy[63:0]
    Error (X0 < 0)
          X0 NOT_SUPPORTED
          NO_ENTROPY
          INVALID_PARAMETERS
          X1 - X3 Reserved (MBZ)
  */
  Parameters.Arg0 = FID_TRNG_RND;
  Parameters.Arg1 = EntropyBits;
  ArmCallMonitor (&Parameters);

  // Convert status codes to EFI status codes.
  Status = TrngStatusToEfiStatus ((INT32)Parameters.Arg0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Extract Data
  // ArgSelector = ((EntropyBytes + 3) >> 2); for AArch32
  // ArgSelector = ((EntropyBytes + 7) >> 3); for AArch64
  // ((sizeof (UINTN) >> 2) + 1) is 3 or 2 depending on size of UINTN
  ArgSelector = ((EntropyBytes + (sizeof (UINTN) - 1)) >>
                 ((sizeof (UINTN) >> 2) + 1));

  switch (ArgSelector) {
    case 3:
      CopyMem (&Buffer[(sizeof (UINTN) * 2)], &Parameters.Arg1, sizeof (UINTN));

    case 2:
      CopyMem (&Buffer[sizeof (UINTN)], &Parameters.Arg2, sizeof (UINTN));

    case 1:
      CopyMem (&Buffer[0], &Parameters.Arg3, sizeof (UINTN));
      break;

    default:
      ASSERT (0);
      return RETURN_INVALID_PARAMETER;
  } // switch


  // [1] Section 2.4.3 Caller responsibilities.
  // The caller must ensure that only the value in Entropy[N-1:0] is consumed
  // and that the remaining bits in Entropy[MAX_BITS-1:N] are ignored.
  // Therefore, Clear the unused upper bytes.
  BytesToClear = (sizeof (UINTN) * ArgSelector) - EntropyBytes;
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
ArmFwTrngLibConstructor (
  VOID
  )
{
  RETURN_STATUS Status;
  UINT16        MajorRev;
  UINT16        MinorRev;
  GUID          Guid;

  Status = GetTrngVersion (&MajorRev, &MinorRev);
  if (EFI_ERROR (Status)) {
    return RETURN_SUCCESS;
  }

#ifndef MDEPKG_NDEBUG
  // Check that the required features are present.
  Status = GetTrngFeatures (FID_TRNG_RND, NULL);
  if (EFI_ERROR (Status)) {
    return RETURN_SUCCESS;
  }

  // Check if TRNG UUID is supported and if so trace the GUID.
  Status = GetTrngFeatures (FID_TRNG_GET_UUID, NULL);
  if (EFI_ERROR (Status)) {
    return RETURN_SUCCESS;
  }
#endif

  Status = GetTrngUuid (&Guid);
  if (EFI_ERROR (Status)) {
    return RETURN_SUCCESS;
  }

  DEBUG ((
    DEBUG_INFO,
    "FW-TRNG: Version %d.%d, GUID {%g}\n",
    MajorRev,
    MinorRev,
    Guid
    ));

  return RETURN_SUCCESS;
}

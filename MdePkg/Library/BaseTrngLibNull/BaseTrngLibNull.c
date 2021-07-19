/** @file
  Null version of TRNG (True Random Number Generator) services.

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - TRNG - True Random Number Generator
**/

#include <Library/DebugLib.h>
#include <Library/TrngLib.h>

/** Get the TRNG version.

  A TRNG may be implemented by the system firmware, in which case this
  function shall return the version information for the TRNG implementation.
  Returning version information is optional and if not implemented,
  RETURN_UNSUPPORTED shall be returned.

  @param [out]  MajorRevision     Major revision.
  @param [out]  MinorRevision     Minor revision.

  @retval  RETURN_SUCCESS            The function completed successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid parameter.
  @retval  RETURN_UNSUPPORTED        Function not implemented.
**/
RETURN_STATUS
EFIAPI
GetTrngVersion (
  OUT UINT16  *MajorRevision,
  OUT UINT16  *MinorRevision
  )
{
  ASSERT (FALSE);
  return RETURN_UNSUPPORTED;
}

/** Get the UUID of the TRNG backend.

  A TRNG may be implemented by the system firmware, in which case this
  function shall return the UUID for the TRNG implementation.
  Returning the TRNG UUID is optional and if not implemented, RETURN_UNSUPPORTED
  shall be returned.

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
  ASSERT (FALSE);
  return RETURN_UNSUPPORTED;
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
  ASSERT (FALSE);
  return 0;
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
  ASSERT (FALSE);
  return RETURN_UNSUPPORTED;
}

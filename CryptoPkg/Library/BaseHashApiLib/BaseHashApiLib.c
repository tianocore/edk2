/** @file
  Unified Hash API Implementation

  This file implements the Unified Hash API.

  This API, when called, will calculate the Hash using the
  hashing algorithm specified by PcdHashApiLibPolicy.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <IndustryStandard/Tpm20.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/HashApiLib.h>

/**
  Retrieves the size, in bytes, of the context buffer required for hash operations.

  @return  The size, in bytes, of the context buffer required for hash operations.
**/
UINTN
EFIAPI
HashApiGetContextSize (
  VOID
  )
{
  switch (PcdGet32 (PcdHashApiLibPolicy)) {
    case HASH_ALG_SHA1:
      return Sha1GetContextSize ();
      break;

    case HASH_ALG_SHA256:
      return Sha256GetContextSize ();
      break;

    case HASH_ALG_SHA384:
      return Sha384GetContextSize ();
      break;

    case HASH_ALG_SHA512:
      return Sha512GetContextSize ();
      break;

    case HASH_ALG_SM3_256:
      return Sm3GetContextSize ();
      break;

    default:
      ASSERT (FALSE);
      return 0;
      break;
  }
}

/**
  Init hash sequence.

  @param[out] HashContext   Hash context.

  @retval TRUE         Hash start and HashHandle returned.
  @retval FALSE        Hash Init unsuccessful.
**/
BOOLEAN
EFIAPI
HashApiInit (
  OUT HASH_API_CONTEXT  HashContext
  )
{
  switch (PcdGet32 (PcdHashApiLibPolicy)) {
    case HASH_ALG_SHA1:
      return Sha1Init (HashContext);
      break;

    case HASH_ALG_SHA256:
      return Sha256Init (HashContext);
      break;

    case HASH_ALG_SHA384:
      return Sha384Init (HashContext);
      break;

    case HASH_ALG_SHA512:
      return Sha512Init (HashContext);
      break;

    case HASH_ALG_SM3_256:
      return Sm3Init (HashContext);
      break;

    default:
      ASSERT (FALSE);
      return FALSE;
      break;
  }
}

/**
  Makes a copy of an existing hash context.

  @param[in]  HashContext     Hash context.
  @param[out] NewHashContext  New copy of hash context.

  @retval TRUE         Hash context copy succeeded.
  @retval FALSE        Hash context copy failed.
**/
BOOLEAN
EFIAPI
HashApiDuplicate (
  IN  HASH_API_CONTEXT  HashContext,
  OUT HASH_API_CONTEXT  NewHashContext
  )
{
  switch (PcdGet32 (PcdHashApiLibPolicy)) {
    case HASH_ALG_SHA1:
      return Sha1Duplicate (HashContext, NewHashContext);
      break;

    case HASH_ALG_SHA256:
      return Sha256Duplicate (HashContext, NewHashContext);
      break;

    case HASH_ALG_SHA384:
      return Sha384Duplicate (HashContext, NewHashContext);
      break;

    case HASH_ALG_SHA512:
      return Sha512Duplicate (HashContext, NewHashContext);
      break;

    case HASH_ALG_SM3_256:
      return Sm3Duplicate (HashContext, NewHashContext);
      break;

    default:
      ASSERT (FALSE);
      return FALSE;
      break;
  }
}

/**
  Update hash data.

  @param[in] HashContext   Hash context.
  @param[in] DataToHash    Data to be hashed.
  @param[in] DataToHashLen Data size.

  @retval TRUE         Hash updated.
  @retval FALSE        Hash updated unsuccessful.
**/
BOOLEAN
EFIAPI
HashApiUpdate (
  IN HASH_API_CONTEXT  HashContext,
  IN VOID              *DataToHash,
  IN UINTN             DataToHashLen
  )
{
  switch (PcdGet32 (PcdHashApiLibPolicy)) {
    case HASH_ALG_SHA1:
      return Sha1Update (HashContext, DataToHash, DataToHashLen);
      break;

    case HASH_ALG_SHA256:
      return Sha256Update (HashContext, DataToHash, DataToHashLen);
      break;

    case HASH_ALG_SHA384:
      return Sha384Update (HashContext, DataToHash, DataToHashLen);
      break;

    case HASH_ALG_SHA512:
      return Sha512Update (HashContext, DataToHash, DataToHashLen);
      break;

    case HASH_ALG_SM3_256:
      return Sm3Update (HashContext, DataToHash, DataToHashLen);
      break;

    default:
      ASSERT (FALSE);
      return FALSE;
      break;
  }
}

/**
  Hash complete.

  @param[in]  HashContext  Hash context.
  @param[out] Digest       Hash Digest.

  @retval TRUE         Hash complete and Digest is returned.
  @retval FALSE        Hash complete unsuccessful.
**/
BOOLEAN
EFIAPI
HashApiFinal (
  IN  HASH_API_CONTEXT  HashContext,
  OUT UINT8             *Digest
  )
{
  switch (PcdGet32 (PcdHashApiLibPolicy)) {
    case HASH_ALG_SHA1:
      return Sha1Final (HashContext, Digest);
      break;

    case HASH_ALG_SHA256:
      return Sha256Final (HashContext, Digest);
      break;

    case HASH_ALG_SHA384:
      return Sha384Final (HashContext, Digest);
      break;

    case HASH_ALG_SHA512:
      return Sha512Final (HashContext, Digest);
      break;

    case HASH_ALG_SM3_256:
      return Sm3Final (HashContext, Digest);
      break;

    default:
      ASSERT (FALSE);
      return FALSE;
      break;
  }
}

/**
  Computes hash message digest of a input data buffer.

  @param[in]  DataToHash     Data to be hashed.
  @param[in]  DataToHashLen  Data size.
  @param[out] Digest         Hash Digest.

  @retval TRUE   Hash digest computation succeeded.
  @retval FALSE  Hash digest computation failed.
**/
BOOLEAN
EFIAPI
HashApiHashAll (
  IN  CONST VOID  *DataToHash,
  IN  UINTN       DataToHashLen,
  OUT UINT8       *Digest
  )
{
  switch (PcdGet32 (PcdHashApiLibPolicy)) {
    case HASH_ALG_SHA1:
      return Sha1HashAll (DataToHash, DataToHashLen, Digest);
      break;

    case HASH_ALG_SHA256:
      return Sha256HashAll (DataToHash, DataToHashLen, Digest);
      break;

    case HASH_ALG_SHA384:
      return Sha384HashAll (DataToHash, DataToHashLen, Digest);
      break;

    case HASH_ALG_SHA512:
      return Sha512HashAll (DataToHash, DataToHashLen, Digest);
      break;

    case HASH_ALG_SM3_256:
      return Sm3HashAll (DataToHash, DataToHashLen, Digest);
      break;

    default:
      ASSERT (FALSE);
      return FALSE;
      break;
  }
}

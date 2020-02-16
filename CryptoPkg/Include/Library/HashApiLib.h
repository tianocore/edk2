/** @file
  Unified Hash API Defines

  This API when called will calculate the Hash using the
  hashing algorithm specified by PcdHashApiLibPolicy.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HASH_API_LIB_H_
#define __HASH_API_LIB_H_

typedef VOID  *HASH_API_CONTEXT;

/**
  Retrieves the size, in bytes, of the context buffer required for hash operations.

  @return  The size, in bytes, of the context buffer required for hash operations.
**/
UINTN
EFIAPI
HashApiGetContextSize (
  VOID
  );

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
  );

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
  );

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
  );

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
  );

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
  );

#endif

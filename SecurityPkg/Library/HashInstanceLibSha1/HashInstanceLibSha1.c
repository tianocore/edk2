/** @file
  This library is BaseCrypto SHA1 hash instance.
  It can be registered to BaseCrypto router, to serve as hash engine.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HashLib.h>

/**
  The function set SHA1 to digest list.

  @param DigestList digest list
  @param Sha1Digest SHA1 digest
**/
VOID
Tpm2SetSha1ToDigestList (
  IN TPML_DIGEST_VALUES *DigestList,
  IN UINT8              *Sha1Digest
  )
{
  DigestList->count = 1;
  DigestList->digests[0].hashAlg = TPM_ALG_SHA1;
  CopyMem (
    DigestList->digests[0].digest.sha1,
    Sha1Digest,
    SHA1_DIGEST_SIZE
    );
}

/**
  Start hash sequence.

  @param HashHandle Hash handle.

  @retval EFI_SUCCESS          Hash sequence start and HandleHandle returned.
  @retval EFI_OUT_OF_RESOURCES No enough resource to start hash.
**/
EFI_STATUS
EFIAPI
Sha1HashInit (
  OUT HASH_HANDLE    *HashHandle
  )
{
  VOID     *Sha1Ctx;
  UINTN    CtxSize;

  CtxSize = Sha1GetContextSize ();
  Sha1Ctx = AllocatePool (CtxSize);
  ASSERT (Sha1Ctx != NULL);

  Sha1Init (Sha1Ctx);

  *HashHandle = (HASH_HANDLE)Sha1Ctx;

  return EFI_SUCCESS;
}

/**
  Update hash sequence data.

  @param HashHandle    Hash handle.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.

  @retval EFI_SUCCESS     Hash sequence updated.
**/
EFI_STATUS
EFIAPI
Sha1HashUpdate (
  IN HASH_HANDLE    HashHandle,
  IN VOID           *DataToHash,
  IN UINTN          DataToHashLen
  )
{
  VOID     *Sha1Ctx;

  Sha1Ctx = (VOID *)HashHandle;
  Sha1Update (Sha1Ctx, DataToHash, DataToHashLen);

  return EFI_SUCCESS;
}

/**
  Complete hash sequence complete.

  @param HashHandle    Hash handle.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS     Hash sequence complete and DigestList is returned.
**/
EFI_STATUS
EFIAPI
Sha1HashFinal (
  IN HASH_HANDLE         HashHandle,
  OUT TPML_DIGEST_VALUES *DigestList
  )
{
  UINT8         Digest[SHA1_DIGEST_SIZE];
  VOID          *Sha1Ctx;

  Sha1Ctx = (VOID *)HashHandle;
  Sha1Final (Sha1Ctx, Digest);

  FreePool (Sha1Ctx);

  Tpm2SetSha1ToDigestList (DigestList, Digest);

  return EFI_SUCCESS;
}

HASH_INTERFACE  mSha1InternalHashInstance = {
  HASH_ALGORITHM_SHA1_GUID,
  Sha1HashInit,
  Sha1HashUpdate,
  Sha1HashFinal,
};

/**
  The function register SHA1 instance.

  @retval EFI_SUCCESS   SHA1 instance is registered, or system dose not surpport registr SHA1 instance
**/
EFI_STATUS
EFIAPI
HashInstanceLibSha1Constructor (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = RegisterHashInterfaceLib (&mSha1InternalHashInstance);
  if ((Status == EFI_SUCCESS) || (Status == EFI_UNSUPPORTED)) {
    //
    // Unsupported means platform policy does not need this instance enabled.
    //
    return EFI_SUCCESS;
  }
  return Status;
}

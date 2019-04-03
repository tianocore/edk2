/** @file
  This library is BaseCrypto SHA384 hash instance.
  It can be registered to BaseCrypto router, to serve as hash engine.

Copyright (c) 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HashLib.h>

/**
  The function set SHA384 to digest list.

  @param DigestList   digest list
  @param Sha384Digest SHA384 digest
**/
VOID
Tpm2SetSha384ToDigestList (
  IN TPML_DIGEST_VALUES *DigestList,
  IN UINT8              *Sha384Digest
  )
{
  DigestList->count = 1;
  DigestList->digests[0].hashAlg = TPM_ALG_SHA384;
  CopyMem (
    DigestList->digests[0].digest.sha384,
    Sha384Digest,
    SHA384_DIGEST_SIZE
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
Sha384HashInit (
  OUT HASH_HANDLE    *HashHandle
  )
{
  VOID     *Sha384Ctx;
  UINTN    CtxSize;

  CtxSize = Sha384GetContextSize ();
  Sha384Ctx = AllocatePool (CtxSize);
  ASSERT (Sha384Ctx != NULL);

  Sha384Init (Sha384Ctx);

  *HashHandle = (HASH_HANDLE)Sha384Ctx;

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
Sha384HashUpdate (
  IN HASH_HANDLE    HashHandle,
  IN VOID           *DataToHash,
  IN UINTN          DataToHashLen
  )
{
  VOID     *Sha384Ctx;

  Sha384Ctx = (VOID *)HashHandle;
  Sha384Update (Sha384Ctx, DataToHash, DataToHashLen);

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
Sha384HashFinal (
  IN HASH_HANDLE         HashHandle,
  OUT TPML_DIGEST_VALUES *DigestList
  )
{
  UINT8         Digest[SHA384_DIGEST_SIZE];
  VOID          *Sha384Ctx;

  Sha384Ctx = (VOID *)HashHandle;
  Sha384Final (Sha384Ctx, Digest);

  FreePool (Sha384Ctx);

  Tpm2SetSha384ToDigestList (DigestList, Digest);

  return EFI_SUCCESS;
}

HASH_INTERFACE  mSha384InternalHashInstance = {
  HASH_ALGORITHM_SHA384_GUID,
  Sha384HashInit,
  Sha384HashUpdate,
  Sha384HashFinal,
};

/**
  The function register SHA384 instance.

  @retval EFI_SUCCESS   SHA384 instance is registered, or system dose not surpport registr SHA384 instance
**/
EFI_STATUS
EFIAPI
HashInstanceLibSha384Constructor (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = RegisterHashInterfaceLib (&mSha384InternalHashInstance);
  if ((Status == EFI_SUCCESS) || (Status == EFI_UNSUPPORTED)) {
    //
    // Unsupported means platform policy does not need this instance enabled.
    //
    return EFI_SUCCESS;
  }
  return Status;
}

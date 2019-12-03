/** @file
  This library is BaseCrypto SHA512 hash instance.
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
  The function set SHA512 to digest list.

  @param DigestList   digest list
  @param Sha512Digest SHA512 digest
**/
VOID
Tpm2SetSha512ToDigestList (
  IN TPML_DIGEST_VALUES *DigestList,
  IN UINT8              *Sha512Digest
  )
{
  DigestList->count = 1;
  DigestList->digests[0].hashAlg = TPM_ALG_SHA512;
  CopyMem (
    DigestList->digests[0].digest.sha512,
    Sha512Digest,
    SHA512_DIGEST_SIZE
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
Sha512HashInit (
  OUT HASH_HANDLE    *HashHandle
  )
{
  VOID     *Sha512Ctx;
  UINTN    CtxSize;

  CtxSize = Sha512GetContextSize ();
  Sha512Ctx = AllocatePool (CtxSize);
  ASSERT (Sha512Ctx != NULL);

  Sha512Init (Sha512Ctx);

  *HashHandle = (HASH_HANDLE)Sha512Ctx;

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
Sha512HashUpdate (
  IN HASH_HANDLE    HashHandle,
  IN VOID           *DataToHash,
  IN UINTN          DataToHashLen
  )
{
  VOID     *Sha512Ctx;

  Sha512Ctx = (VOID *)HashHandle;
  Sha512Update (Sha512Ctx, DataToHash, DataToHashLen);

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
Sha512HashFinal (
  IN HASH_HANDLE         HashHandle,
  OUT TPML_DIGEST_VALUES *DigestList
  )
{
  UINT8         Digest[SHA512_DIGEST_SIZE];
  VOID          *Sha512Ctx;

  Sha512Ctx = (VOID *)HashHandle;
  Sha512Final (Sha512Ctx, Digest);

  FreePool (Sha512Ctx);

  Tpm2SetSha512ToDigestList (DigestList, Digest);

  return EFI_SUCCESS;
}

HASH_INTERFACE  mSha512InternalHashInstance = {
  HASH_ALGORITHM_SHA512_GUID,
  Sha512HashInit,
  Sha512HashUpdate,
  Sha512HashFinal,
};

/**
  The function register SHA512 instance.

  @retval EFI_SUCCESS   SHA512 instance is registered, or system does not support register SHA512 instance
**/
EFI_STATUS
EFIAPI
HashInstanceLibSha512Constructor (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = RegisterHashInterfaceLib (&mSha512InternalHashInstance);
  if ((Status == EFI_SUCCESS) || (Status == EFI_UNSUPPORTED)) {
    //
    // Unsupported means platform policy does not need this instance enabled.
    //
    return EFI_SUCCESS;
  }
  return Status;
}

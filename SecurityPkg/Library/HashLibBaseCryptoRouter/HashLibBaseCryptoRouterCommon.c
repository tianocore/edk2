/** @file
  Ihis is BaseCrypto router support function.

Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved. <BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HashLib.h>
#include <Protocol/Tcg2Protocol.h>

typedef struct {
  EFI_GUID  Guid;
  UINT32    Mask;
} TPM2_HASH_MASK;

TPM2_HASH_MASK mTpm2HashMask[] = {
  {HASH_ALGORITHM_SHA1_GUID,         HASH_ALG_SHA1},
  {HASH_ALGORITHM_SHA256_GUID,       HASH_ALG_SHA256},
  {HASH_ALGORITHM_SHA384_GUID,       HASH_ALG_SHA384},
  {HASH_ALGORITHM_SHA512_GUID,       HASH_ALG_SHA512},
};

/**
  The function get hash mask info from algorithm.

  @param HashGuid Hash Guid

  @return HashMask
**/
UINT32
EFIAPI
Tpm2GetHashMaskFromAlgo (
  IN EFI_GUID  *HashGuid
  )
{
  UINTN  Index;
  for (Index = 0; Index < sizeof(mTpm2HashMask)/sizeof(mTpm2HashMask[0]); Index++) {
    if (CompareGuid (HashGuid, &mTpm2HashMask[Index].Guid)) {
      return mTpm2HashMask[Index].Mask;
    }
  }
  return 0;
}

/**
  The function set digest to digest list.

  @param DigestList digest list
  @param Digest     digest data
**/
VOID
EFIAPI
Tpm2SetHashToDigestList (
  IN OUT TPML_DIGEST_VALUES *DigestList,
  IN TPML_DIGEST_VALUES     *Digest
  )
{
  CopyMem (
    &DigestList->digests[DigestList->count],
    &Digest->digests[0],
    sizeof(Digest->digests[0])
    );
  DigestList->count ++;
}

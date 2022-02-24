/** @file
  This library is BaseCrypto router for Tdx.

Copyright (c) 2021 - 2022, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/HashLib.h>
#include <Library/TdxLib.h>
#include <Protocol/CcMeasurement.h>
#include "HashLibBaseCryptoRouterCommon.h"

//
// Currently TDX supports SHA384.
//
#define TDX_HASH_COUNT  1
HASH_INTERFACE  mHashInterface[TDX_HASH_COUNT] = {
  {
    { 0 }, NULL, NULL, NULL
  }
};

UINTN        mHashInterfaceCount      = 0;
HASH_HANDLE  mHashCtx[TDX_HASH_COUNT] = { 0 };

/**
  Start hash sequence.

  @param HashHandle Hash handle.

  @retval EFI_SUCCESS          Hash sequence start and HandleHandle returned.
  @retval EFI_OUT_OF_RESOURCES No enough resource to start hash.
**/
EFI_STATUS
EFIAPI
HashStart (
  OUT HASH_HANDLE  *HashHandle
  )
{
  HASH_HANDLE  *HashCtx;

  if (mHashInterfaceCount == 0) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  HashCtx = mHashCtx;
  mHashInterface[0].HashInit (&HashCtx[0]);

  *HashHandle = (HASH_HANDLE)HashCtx;

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
HashUpdate (
  IN HASH_HANDLE  HashHandle,
  IN VOID         *DataToHash,
  IN UINTN        DataToHashLen
  )
{
  HASH_HANDLE  *HashCtx;

  if (mHashInterfaceCount == 0) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  HashCtx = (HASH_HANDLE *)HashHandle;
  mHashInterface[0].HashUpdate (HashCtx[0], DataToHash, DataToHashLen);

  return EFI_SUCCESS;
}

/**
  Hash sequence complete and extend to PCR.

  @param HashHandle    Hash handle.
  @param PcrIndex      PCR to be extended.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS     Hash sequence complete and DigestList is returned.
**/
EFI_STATUS
EFIAPI
HashCompleteAndExtend (
  IN HASH_HANDLE          HashHandle,
  IN TPMI_DH_PCR          PcrIndex,
  IN VOID                 *DataToHash,
  IN UINTN                DataToHashLen,
  OUT TPML_DIGEST_VALUES  *DigestList
  )
{
  TPML_DIGEST_VALUES  Digest;
  HASH_HANDLE         *HashCtx;
  EFI_STATUS          Status;

  if (mHashInterfaceCount == 0) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  HashCtx = (HASH_HANDLE *)HashHandle;
  ZeroMem (DigestList, sizeof (*DigestList));

  mHashInterface[0].HashUpdate (HashCtx[0], DataToHash, DataToHashLen);
  mHashInterface[0].HashFinal (HashCtx[0], &Digest);
  Tpm2SetHashToDigestList (DigestList, &Digest);

  ASSERT (DigestList->count == 1 && DigestList->digests[0].hashAlg == TPM_ALG_SHA384);

  Status = TdExtendRtmr (
             (UINT32 *)DigestList->digests[0].digest.sha384,
             SHA384_DIGEST_SIZE,
             (UINT8)PcrIndex
             );

  ASSERT (!EFI_ERROR (Status));
  return Status;
}

/**
  Hash data and extend to RTMR.

  @param PcrIndex      PCR to be extended.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS     Hash data and DigestList is returned.
**/
EFI_STATUS
EFIAPI
HashAndExtend (
  IN TPMI_DH_PCR          PcrIndex,
  IN VOID                 *DataToHash,
  IN UINTN                DataToHashLen,
  OUT TPML_DIGEST_VALUES  *DigestList
  )
{
  HASH_HANDLE  HashHandle;
  EFI_STATUS   Status;

  if (mHashInterfaceCount == 0) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  ASSERT (TdIsEnabled ());

  HashStart (&HashHandle);
  HashUpdate (HashHandle, DataToHash, DataToHashLen);
  Status = HashCompleteAndExtend (HashHandle, PcrIndex, NULL, 0, DigestList);

  return Status;
}

/**
  This service register Hash.

  @param HashInterface  Hash interface

  @retval EFI_SUCCESS          This hash interface is registered successfully.
  @retval EFI_UNSUPPORTED      System does not support register this interface.
  @retval EFI_ALREADY_STARTED  System already register this interface.
**/
EFI_STATUS
EFIAPI
RegisterHashInterfaceLib (
  IN HASH_INTERFACE  *HashInterface
  )
{
  UINT32  HashMask;

  ASSERT (TdIsEnabled ());

  //
  // Check allow
  //
  HashMask = Tpm2GetHashMaskFromAlgo (&HashInterface->HashGuid);
  ASSERT (HashMask == HASH_ALG_SHA384);

  if (HashMask != HASH_ALG_SHA384) {
    return EFI_UNSUPPORTED;
  }

  if (mHashInterfaceCount >= ARRAY_SIZE (mHashInterface)) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (&mHashInterface[mHashInterfaceCount], HashInterface, sizeof (*HashInterface));
  mHashInterfaceCount++;

  return EFI_SUCCESS;
}

/** @file
  This library is HashLib for Tdx.

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

EFI_GUID  mSha384Guid = HASH_ALGORITHM_SHA384_GUID;

//
// Currently TDX supports SHA384.
//
HASH_INTERFACE  mHashInterface =  {
  { 0 }, NULL, NULL, NULL
};

UINTN  mHashInterfaceCount = 0;

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
  HASH_HANDLE  HashCtx;

  if (mHashInterfaceCount == 0) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  HashCtx = 0;
  mHashInterface.HashInit (&HashCtx);

  *HashHandle = HashCtx;

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
  if (mHashInterfaceCount == 0) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  mHashInterface.HashUpdate (HashHandle, DataToHash, DataToHashLen);

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
  EFI_STATUS          Status;

  if (mHashInterfaceCount == 0) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  ZeroMem (DigestList, sizeof (*DigestList));

  mHashInterface.HashUpdate (HashHandle, DataToHash, DataToHashLen);
  mHashInterface.HashFinal (HashHandle, &Digest);

  CopyMem (
    &DigestList->digests[0],
    &Digest.digests[0],
    sizeof (Digest.digests[0])
    );
  DigestList->count++;

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
  //
  // HashLibTdx is designed for Tdx guest. So if it is not Tdx guest,
  // return EFI_UNSUPPORTED.
  //
  if (!TdIsEnabled ()) {
    return EFI_UNSUPPORTED;
  }

  //
  // Only SHA384 is allowed.
  //
  if (!CompareGuid (&mSha384Guid, &HashInterface->HashGuid)) {
    return EFI_UNSUPPORTED;
  }

  if (mHashInterfaceCount != 0) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (&mHashInterface, HashInterface, sizeof (*HashInterface));
  mHashInterfaceCount++;

  return EFI_SUCCESS;
}

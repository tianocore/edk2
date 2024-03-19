/** @file
  This library is BaseCrypto router. It will redirect hash request to each individual
  hash handler registered, such as SHA1, SHA256.
  Platform can use PcdTpm2HashMask to mask some hash engines.

Copyright (c) 2013 - 2024, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/HashLib.h>
#include <Protocol/Tcg2Protocol.h>

#include "HashLibBaseCryptoRouterCommon.h"

HASH_INTERFACE  mHashInterface[HASH_COUNT] = {
  {
    { 0 }, NULL, NULL, NULL
  }
};
UINTN           mHashInterfaceCount = 0;

UINT32  mSupportedHashMaskLast    = 0;
UINT32  mSupportedHashMaskCurrent = 0;

/**
  Check mismatch of supported HashMask between modules
  that may link different HashInstanceLib instances.

**/
VOID
CheckSupportedHashMaskMismatch (
  VOID
  )
{
  if (mSupportedHashMaskCurrent != mSupportedHashMaskLast) {
    DEBUG ((
      DEBUG_WARN,
      "WARNING: There is mismatch of supported HashMask (0x%x - 0x%x) between modules\n",
      mSupportedHashMaskCurrent,
      mSupportedHashMaskLast
      ));
    DEBUG ((DEBUG_WARN, "that are linking different HashInstanceLib instances!\n"));
  }
}

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
  UINTN        Index;
  UINT32       HashMask;

  if (mHashInterfaceCount == 0) {
    return EFI_UNSUPPORTED;
  }

  CheckSupportedHashMaskMismatch ();

  HashCtx = AllocatePool (sizeof (*HashCtx) * mHashInterfaceCount);
  ASSERT (HashCtx != NULL);

  for (Index = 0; Index < mHashInterfaceCount; Index++) {
    HashMask = Tpm2GetHashMaskFromAlgo (&mHashInterface[Index].HashGuid);
    if ((HashMask & PcdGet32 (PcdTpm2HashMask)) != 0) {
      mHashInterface[Index].HashInit (&HashCtx[Index]);
    }
  }

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
  UINTN        Index;
  UINT32       HashMask;

  if (mHashInterfaceCount == 0) {
    return EFI_UNSUPPORTED;
  }

  CheckSupportedHashMaskMismatch ();

  HashCtx = (HASH_HANDLE *)HashHandle;

  for (Index = 0; Index < mHashInterfaceCount; Index++) {
    HashMask = Tpm2GetHashMaskFromAlgo (&mHashInterface[Index].HashGuid);
    if ((HashMask & PcdGet32 (PcdTpm2HashMask)) != 0) {
      mHashInterface[Index].HashUpdate (HashCtx[Index], DataToHash, DataToHashLen);
    }
  }

  return EFI_SUCCESS;
}

/**
  Extend to TPM NvIndex.

  @param[in]  NvIndex            The NV Index of the area to extend.
  @param[in]  DataSize           The data size to extend.
  @param[in]  Data               The data to extend.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
  @retval EFI_NOT_FOUND          The command was returned successfully, but NvIndex is not found.
**/
EFI_STATUS
EFIAPI
Tpm2ExtendNvIndex (
  TPMI_RH_NV_INDEX  NvIndex,
  UINT16            DataSize,
  BYTE              *Data
  )
{
  EFI_STATUS        Status;
  TPMI_RH_NV_AUTH   AuthHandle;
  TPM2B_MAX_BUFFER  NvExtendData;

  AuthHandle = TPM_RH_PLATFORM;
  ZeroMem (&NvExtendData, sizeof (NvExtendData));
  CopyMem (NvExtendData.buffer, Data, DataSize);
  NvExtendData.size = DataSize;
  Status            = Tpm2NvExtend (
                        AuthHandle,
                        NvIndex,
                        NULL,
                        &NvExtendData
                        );
  if (EFI_ERROR (Status)) {
    DEBUG (
      (DEBUG_ERROR, "Extend TPM NV index failed, Index: 0x%x Status: %d\n",
       NvIndex, Status)
      );
  }

  return Status;
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
  TPML_DIGEST_VALUES               Digest;
  HASH_HANDLE                      *HashCtx;
  UINTN                            Index;
  EFI_STATUS                       Status;
  UINT32                           HashMask;
  TPML_DIGEST_VALUES               TcgPcrEvent2Digest;
  EFI_TCG2_EVENT_ALGORITHM_BITMAP  TpmHashAlgorithmBitmap;
  UINT32                           ActivePcrBanks;
  UINT32                           *BufferPtr;
  UINT32                           DigestListBinSize;

  if (mHashInterfaceCount == 0) {
    return EFI_UNSUPPORTED;
  }

  CheckSupportedHashMaskMismatch ();

  HashCtx = (HASH_HANDLE *)HashHandle;
  ZeroMem (DigestList, sizeof (*DigestList));

  for (Index = 0; Index < mHashInterfaceCount; Index++) {
    HashMask = Tpm2GetHashMaskFromAlgo (&mHashInterface[Index].HashGuid);
    if ((HashMask & PcdGet32 (PcdTpm2HashMask)) != 0) {
      mHashInterface[Index].HashUpdate (HashCtx[Index], DataToHash, DataToHashLen);
      mHashInterface[Index].HashFinal (HashCtx[Index], &Digest);
      Tpm2SetHashToDigestList (DigestList, &Digest);
    }
  }

  FreePool (HashCtx);

  if (PcrIndex <= MAX_PCR_INDEX) {
    Status = Tpm2PcrExtend (
               PcrIndex,
               DigestList
               );
  } else {
    Status = Tpm2GetCapabilitySupportedAndActivePcrs (&TpmHashAlgorithmBitmap, &ActivePcrBanks);
    ASSERT_EFI_ERROR (Status);
    ActivePcrBanks = ActivePcrBanks & mSupportedHashMaskCurrent;
    ZeroMem (&TcgPcrEvent2Digest, sizeof (TcgPcrEvent2Digest));
    BufferPtr         = CopyDigestListToBuffer (&TcgPcrEvent2Digest, DigestList, ActivePcrBanks);
    DigestListBinSize = (UINT32)((UINT8 *)BufferPtr - (UINT8 *)&TcgPcrEvent2Digest);

    //
    // Extend to TPM NvIndex
    //
    Status = Tpm2ExtendNvIndex (
               PcrIndex,
               (UINT16)DigestListBinSize,
               (BYTE *)&TcgPcrEvent2Digest
               );
  }

  return Status;
}

/**
  Hash data and extend to PCR.

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
    return EFI_UNSUPPORTED;
  }

  CheckSupportedHashMaskMismatch ();

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
  UINTN       Index;
  UINT32      HashMask;
  UINT32      Tpm2HashMask;
  EFI_STATUS  Status;

  //
  // Check allow
  //
  HashMask     = Tpm2GetHashMaskFromAlgo (&HashInterface->HashGuid);
  Tpm2HashMask = PcdGet32 (PcdTpm2HashMask);

  if ((Tpm2HashMask != 0) &&
      ((HashMask & Tpm2HashMask) == 0))
  {
    return EFI_UNSUPPORTED;
  }

  if (mHashInterfaceCount >= sizeof (mHashInterface)/sizeof (mHashInterface[0])) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Check duplication
  //
  for (Index = 0; Index < mHashInterfaceCount; Index++) {
    if (CompareGuid (&mHashInterface[Index].HashGuid, &HashInterface->HashGuid)) {
      DEBUG ((DEBUG_ERROR, "Hash Interface (%g) has been registered\n", &HashInterface->HashGuid));
      return EFI_ALREADY_STARTED;
    }
  }

  //
  // Record hash algorithm bitmap of CURRENT module which consumes HashLib.
  //
  mSupportedHashMaskCurrent = PcdGet32 (PcdTcg2HashAlgorithmBitmap) | HashMask;
  Status                    = PcdSet32S (PcdTcg2HashAlgorithmBitmap, mSupportedHashMaskCurrent);
  ASSERT_EFI_ERROR (Status);

  CopyMem (&mHashInterface[mHashInterfaceCount], HashInterface, sizeof (*HashInterface));
  mHashInterfaceCount++;

  return EFI_SUCCESS;
}

/**
  The constructor function of HashLibBaseCryptoRouterDxe.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor executed correctly.

**/
EFI_STATUS
EFIAPI
HashLibBaseCryptoRouterDxeConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Record hash algorithm bitmap of LAST module which also consumes HashLib.
  //
  mSupportedHashMaskLast = PcdGet32 (PcdTcg2HashAlgorithmBitmap);

  //
  // Set PcdTcg2HashAlgorithmBitmap to 0 in CONSTRUCTOR for CURRENT module.
  //
  Status = PcdSet32S (PcdTcg2HashAlgorithmBitmap, 0);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

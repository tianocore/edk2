/** @file
  A HashLib instance for Arm CCA.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2021 - 2022, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HashLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Protocol/CcMeasurement.h>

#include <Library/ArmCcaLib.h>
#include <Library/ArmCcaRsiLib.h>

/**
  Hash Interface for Arm CCA.
  This depends on the hash algorithm
  for the Realm i.e. SHA256 or SHA512.
*/
HASH_INTERFACE  mHashInterface =  {
  { 0 }, NULL, NULL, NULL
};

/**
  The count of hash interfaces registered.
*/
UINTN  mHashInterfaceCount = 0;

/**
  The hash algorithm ID for the Realm.
  This is discovered by reading the Realm Config
  and can either be SHA256 or SHA512.
*/
TPM_ALG_ID  mAlgoId = TPM_ALG_ERROR;

/**
 A structure mapping the Arm CCA hash information.
*/
typedef struct {
  /// The Realm hash algorithm.
  UINT8            RealmHashAlgorithm;
  /// The GUID for the hash function.
  EFI_GUID         HashGuid;
  /// The algorithm ID for the hash function.
  TPMI_ALG_HASH    HashAlgo;
  /// The hash size.
  UINT16           HashSize;
  /// The hash mask.
  UINT32           HashMask;
} ARMCCA_HASH_INFO;

/**
  A map of the hash algorithms supported by a Realm.
*/
STATIC ARMCCA_HASH_INFO  mHashInfo[] = {
  {
    ARM_CCA_RSI_HASH_SHA_256,
    HASH_ALGORITHM_SHA256_GUID,
    TPM_ALG_SHA256,
    SHA256_DIGEST_SIZE,
    HASH_ALG_SHA256
  },
  {
    ARM_CCA_RSI_HASH_SHA_512,
    HASH_ALGORITHM_SHA512_GUID,
    TPM_ALG_SHA512,
    SHA512_DIGEST_SIZE,
    HASH_ALG_SHA512
  }
};

/**
  Get hash algorithm ID corresponding to the Realm Hash algorithm.

  @param[in]    RealmHashAlgorithm   The Realm Hash Algorithm.

  @return Hash Algorithm ID if success else TPM_ALG_ERROR.
**/
STATIC
TPM_ALG_ID
RealmHashAlgoToTpmAlgoId (
  UINT8  RealmHashAlgorithm
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mHashInfo); Index++) {
    if (mHashInfo[Index].RealmHashAlgorithm == RealmHashAlgorithm) {
      return mHashInfo[Index].HashAlgo;
    }
  }

  // No suitable algorithm found return.
  return TPM_ALG_ERROR;
}

/**
  Get the hash information based on the Hash Algo.

  @param[in]     HashAlgo           Hash Algorithm Id.

  @return Pointer to the Hash information on success or NULL.
**/
STATIC
ARMCCA_HASH_INFO *
GetHashInfoFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mHashInfo); Index++) {
    if (mHashInfo[Index].HashAlgo == HashAlgo) {
      return &mHashInfo[Index];
    }
  }

  ASSERT (FALSE);
  return NULL;
}

/**
  Get hash size based on Algo

  @param[in]     HashAlgo           Hash Algorithm Id.

  @return Size of the hash.
**/
STATIC
UINT16
GetHashSizeFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  ARMCCA_HASH_INFO  *HashInfo;

  HashInfo = GetHashInfoFromAlgo (HashAlgo);
  if (HashInfo != NULL) {
    return HashInfo->HashSize;
  }

  ASSERT (FALSE);
  return 0;
}

/**
  Read the Realm configuration to get the Realm Hash algorithm.

  @param[out] RealmHashAlgorithm  The Realm Hash algorithm

  @retval EFI_SUCCESS            Success, the Realm Hash algorithm is returned.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
  @retval EFI_UNSUPPORTED        Unsupported algorithm.
**/
STATIC
EFI_STATUS
EFIAPI
GetRealmHashAlgorithm (
  OUT UINT8  *RealmHashAlgorithm
  )
{
  EFI_STATUS            Status;
  ARM_CCA_REALM_CONFIG  *Config;
  UINT8                 HashAlgorithm;

  Config = AllocateAlignedPages (
             EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_REALM_CONFIG)),
             ARM_CCA_REALM_GRANULE_SIZE
             );
  if (Config == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Config, sizeof (ARM_CCA_REALM_CONFIG));

  Status = ArmCcaRsiGetRealmConfig (Config);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    FreeAlignedPages (
      Config,
      EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_REALM_CONFIG))
      );
    return Status;
  }

  HashAlgorithm = Config->HashAlgorithm;

  FreeAlignedPages (Config, EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_REALM_CONFIG)));

  if ((HashAlgorithm != ARM_CCA_RSI_HASH_SHA_256) &&
      (HashAlgorithm != ARM_CCA_RSI_HASH_SHA_512))
  {
    ASSERT (0);
    return EFI_UNSUPPORTED;
  }

  *RealmHashAlgorithm = HashAlgorithm;
  return EFI_SUCCESS;
}

/**
  Start hash sequence.

  @param HashHandle Hash handle.

  @retval EFI_SUCCESS             Hash sequence start and HandleHandle returned.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_UNSUPPORTED         Unsupported hash algorithm.

**/
EFI_STATUS
EFIAPI
HashStart (
  OUT HASH_HANDLE  *HashHandle
  )
{
  if (mHashInterfaceCount == 0) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  if (HashHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *HashHandle = 0;
  return mHashInterface.HashInit (HashHandle);
}

/**
  Update hash sequence data.

  @param HashHandle    Hash handle.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.

  @retval EFI_SUCCESS             Hash sequence updated.
  @retval EFI_UNSUPPORTED         Unsupported hash algorithm.
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

  return mHashInterface.HashUpdate (HashHandle, DataToHash, DataToHashLen);
}

/**
  Hash sequence complete and extend to Measurement Register.

  @param HashHandle    Hash handle.
  @param PcrIndex      PCR to be extended.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS             Hash sequence complete and DigestList
                                  is returned.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_UNSUPPORTED         Unsupported hash algorithm.
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

  if (DigestList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (DigestList, sizeof (*DigestList));

  Status = mHashInterface.HashUpdate (HashHandle, DataToHash, DataToHashLen);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = mHashInterface.HashFinal (HashHandle, &Digest);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (
    &DigestList->digests[0],
    &Digest.digests[0],
    sizeof (Digest.digests[0])
    );
  DigestList->count++;

  ASSERT (
    (DigestList->count == 1) &&
    (DigestList->digests[0].hashAlg == mAlgoId)
    );

  Status = ArmCcaRsiExtendMeasurement (
             (UINTN)PcrIndex,
             (UINT8 *)&DigestList->digests[0].digest,
             GetHashSizeFromAlgo (mAlgoId)
             );

  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Hash data and extend to Measurement Register.

  @param PcrIndex      PCR to be extended.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS             Hash data and DigestList is returned.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_UNSUPPORTED         Unsupported hash algorithm or the execution
                                  context is not a Realm.
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

  if ((mHashInterfaceCount == 0) || !ArmCcaIsRealm ()) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  if ((DigestList == NULL) || (DataToHash == NULL) || (DataToHashLen == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = HashStart (&HashHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HashUpdate (HashHandle, DataToHash, DataToHashLen);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return HashCompleteAndExtend (HashHandle, PcrIndex, NULL, 0, DigestList);
}

/**
  This service register Hash.

  @param HashInterface  Hash interface

  @retval EFI_SUCCESS          This hash interface is registered successfully.
  @retval EFI_ALREADY_STARTED  System has already registered this interface.
  @retval EFI_UNSUPPORTED      Unsupported hash algorithm or the execution
                               context is not a Realm.
**/
EFI_STATUS
EFIAPI
RegisterHashInterfaceLib (
  IN HASH_INTERFACE  *HashInterface
  )
{
  EFI_STATUS        Status;
  UINT8             RealmHashAlgorithm;
  ARMCCA_HASH_INFO  *HashInfo;

  //
  // This HashLib is designed for Realm guests.
  // So if it is not Realm guest, return as unsupported.
  //
  if (!ArmCcaIsRealm ()) {
    return EFI_UNSUPPORTED;
  }

  Status = GetRealmHashAlgorithm (&RealmHashAlgorithm);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  mAlgoId = RealmHashAlgoToTpmAlgoId (RealmHashAlgorithm);
  if (mAlgoId == TPM_ALG_ERROR) {
    return EFI_UNSUPPORTED;
  }

  HashInfo = GetHashInfoFromAlgo (mAlgoId);
  if (HashInfo == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Only SHA256 & SHA512 is allowed.
  //
  if (!CompareGuid (&HashInfo->HashGuid, &HashInterface->HashGuid)) {
    // Unsupported means platform policy does not need this instance enabled.
    return EFI_UNSUPPORTED;
  }

  if (mHashInterfaceCount != 0) {
    // If we reach here then a suitable hash algorithm was already registered.
    ASSERT (FALSE);
    return EFI_ALREADY_STARTED;
  }

  CopyMem (&mHashInterface, HashInterface, sizeof (*HashInterface));
  mHashInterfaceCount++;

  return EFI_SUCCESS;
}

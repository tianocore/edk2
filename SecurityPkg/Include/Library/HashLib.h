/** @file
  Ihis library abstract TPM2 hash calculation.
  The platform can choose multiply hash, while caller just need invoke these API.
  Then all hash value will be returned and/or extended.

Copyright (c) 2013, Intel Corporation. All rights reserved. <BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HASH_LIB_H_
#define _HASH_LIB_H_

#include <Uefi.h>
#include <Protocol/Hash.h>

typedef UINTN  HASH_HANDLE;

/**
  Start hash sequence.

  @param HashHandle Hash handle.

  @retval EFI_SUCCESS          Hash sequence start and HandleHandle returned.
  @retval EFI_OUT_OF_RESOURCES No enough resource to start hash.
**/
EFI_STATUS
EFIAPI
HashStart (
  OUT HASH_HANDLE    *HashHandle
  );

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
  IN HASH_HANDLE    HashHandle,
  IN VOID           *DataToHash,
  IN UINTN          DataToHashLen
  );

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
  IN HASH_HANDLE         HashHandle,
  IN TPMI_DH_PCR         PcrIndex,
  IN VOID                *DataToHash,
  IN UINTN               DataToHashLen,
  OUT TPML_DIGEST_VALUES *DigestList
  );

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
  IN TPMI_DH_PCR                    PcrIndex,
  IN VOID                           *DataToHash,
  IN UINTN                          DataToHashLen,
  OUT TPML_DIGEST_VALUES            *DigestList
  );

/**
  Start hash sequence.

  @param HashHandle Hash handle.

  @retval EFI_SUCCESS          Hash sequence start and HandleHandle returned.
  @retval EFI_OUT_OF_RESOURCES No enough resource to start hash.
**/
typedef
EFI_STATUS
(EFIAPI *HASH_INIT) (
  OUT HASH_HANDLE    *HashHandle
  );

/**
  Update hash sequence data.

  @param HashHandle    Hash handle.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.

  @retval EFI_SUCCESS     Hash sequence updated.
**/
typedef
EFI_STATUS
(EFIAPI *HASH_UPDATE) (
  IN HASH_HANDLE    HashHandle,
  IN VOID           *DataToHash,
  IN UINTN          DataToHashLen
  );

/**
  Complete hash sequence complete.

  @param HashHandle    Hash handle.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS     Hash sequence complete and DigestList is returned.
**/
typedef
EFI_STATUS
(EFIAPI *HASH_FINAL) (
  IN HASH_HANDLE         HashHandle,
  OUT TPML_DIGEST_VALUES *DigestList
  );

#define HASH_ALGORITHM_SHA1_GUID    EFI_HASH_ALGORITHM_SHA1_GUID
#define HASH_ALGORITHM_SHA256_GUID  EFI_HASH_ALGORITHM_SHA256_GUID
#define HASH_ALGORITHM_SHA384_GUID  EFI_HASH_ALGORITHM_SHA384_GUID
#define HASH_ALGORITHM_SHA512_GUID  EFI_HASH_ALGORITHM_SHA512_GUID

typedef struct {
  EFI_GUID                           HashGuid;
  HASH_INIT                          HashInit;
  HASH_UPDATE                        HashUpdate;
  HASH_FINAL                         HashFinal;
} HASH_INTERFACE;

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
  IN HASH_INTERFACE   *HashInterface
  );

#endif

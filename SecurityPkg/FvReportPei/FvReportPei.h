/** @file
  Definitions for OBB FVs verification.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FV_REPORT_PEI_H__
#define __FV_REPORT_PEI_H__

#include <PiPei.h>

#include <IndustryStandard/Tpm20.h>

#include <Ppi/FirmwareVolumeInfoStoredHashFv.h>
#include <Ppi/FirmwareVolumeShadowPpi.h>

#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/ReportStatusCodeLib.h>

#define HASH_INFO_PTR(PreHashedFvPpi)  \
  (HASH_INFO *)((UINT8 *)(PreHashedFvPpi) + sizeof (EDKII_PEI_FIRMWARE_VOLUME_INFO_PREHASHED_FV_PPI))

#define HASH_VALUE_PTR(HashInfo)   \
  (VOID *)((UINT8 *)(HashInfo) + sizeof (HASH_INFO))

/**
  Computes the message digest of a input data buffer.

  This function performs message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives digest value.

  @retval TRUE   The digest computation succeeded.
  @retval FALSE  The digest computation failed.

**/
typedef
BOOLEAN
(EFIAPI *HASH_ALL_METHOD)(
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  );

/**
  Initializes user-supplied memory as hash context for subsequent use.

  @param[out]  HashContext  Pointer to hash context being initialized.

  @retval TRUE   Hash context initialization succeeded.
  @retval FALSE  Hash context initialization failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *HASH_INIT_METHOD)(
  OUT  VOID  *HashContext
  );

/**
  Digests the input data and updates hash context.

  @param[in, out]  HashContext  Pointer to the hash context.
  @param[in]       Data         Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize     Size of Data buffer in bytes.

  @retval TRUE   Hash data digest succeeded.
  @retval FALSE  Hash data digest failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *HASH_UPDATE_METHOD)(
  IN OUT  VOID        *HashContext,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

/**
  Completes computation of the hash digest value.

  @param[in, out]  HashContext  Pointer to the hash context.
  @param[out]      HashValue    Pointer to a buffer that receives the hash digest
                                value.

  @retval TRUE   Hash digest computation succeeded.
  @retval FALSE  Hash digest computation failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *HASH_FINAL_METHOD)(
  IN OUT  VOID   *HashContext,
  OUT     UINT8  *HashValue
  );

typedef struct {
  UINT16                HashAlgId;
  UINTN                 HashSize;
  HASH_INIT_METHOD      HashInit;
  HASH_UPDATE_METHOD    HashUpdate;
  HASH_FINAL_METHOD     HashFinal;
  HASH_ALL_METHOD       HashAll;
} HASH_ALG_INFO;

#endif //__FV_REPORT_PEI_H__

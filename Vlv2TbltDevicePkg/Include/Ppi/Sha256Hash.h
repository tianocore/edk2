/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:

  Speaker.h

Abstract:

  EFI Speaker Interface Protocol



--*/

#ifndef _PEI_SHA256_HASH_H
#define _PEI_SHA256_HASH_H

//
// Global ID Speaker Interface
//
#define PEI_SHA256_HASH_PPI_GUID \
  { \
    0x950e191b, 0x8524, 0x4f51,  0x80, 0xa1, 0x5c, 0x4f, 0x1b, 0x3, 0xf3, 0x5c  \
  }

typedef struct _PEI_SHA256_HASH_PPI PEI_SHA256_HASH_PPI;

/**
  @return  The size, in bytes, of the context buffer required for hash operations.

**/
typedef
UINTN
(EFIAPI *HASH_GET_CONTEXT_SIZE)(
  VOID
  );

/**
  Initializes user-supplied memory pointed by HashContext as hash context for
  subsequent use.

  If HashContext is NULL, then ASSERT().

  @param[in, out]  HashContext  Pointer to  Context being initialized.

  @retval TRUE   HASH context initialization succeeded.
  @retval FALSE  HASH context initialization failed.

**/
typedef
BOOLEAN
(EFIAPI *HASH_INIT)(
  IN OUT  VOID  *HashContext
  );

/**
  Performs digest on a data buffer of the specified length. This function can
  be called multiple times to compute the digest of long or discontinuous data streams.

  If HashContext is NULL, then ASSERT().

  @param[in, out]  HashContext  Pointer to the MD5 context.
  @param[in]       Data         Pointer to the buffer containing the data to be hashed.
  @param[in]       DataLength   Length of Data buffer in bytes.

  @retval TRUE     HASH data digest succeeded.
  @retval FALSE    Invalid HASH context. After HashFinal function has been called, the
                   HASH context cannot be reused.

**/
typedef
BOOLEAN
(EFIAPI *HASH_UPDATE)(
  IN OUT  VOID        *HashContext,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  );

/**
  Completes hash computation and retrieves the digest value into the specified
  memory. After this function has been called, the context cannot be used again.

  If HashContext is NULL, then ASSERT().
  If HashValue is NULL, then ASSERT().

  @param[in, out]  HashContext  Pointer to the MD5 context
  @param[out]      HashValue    Pointer to a buffer that receives the HASH digest
                                value.

  @retval TRUE   HASH digest computation succeeded.
  @retval FALSE  HASH digest computation failed.

**/
typedef
BOOLEAN
(EFIAPI *HASH_FINAL)(
  IN OUT  VOID   *HashContext,
  OUT     UINT8  *HashValue
  );

//
// Ppi definition
//
typedef struct _PEI_SHA256_HASH_PPI {
  //
  // Pointer to Hash GetContentSize function
  //
  HASH_GET_CONTEXT_SIZE    GetContextSize;
  //
  // Pointer to Hash Init function
  //
  HASH_INIT                HashInit;
  //
  // Pointer to Hash Update function
  //
  HASH_UPDATE              HashUpdate;
  //
  // Pointer to Hash Final function
  //
  HASH_FINAL               HashFinal;

} PEI_SHA256_HASH_PPI;

extern EFI_GUID gPeiSha256HashPpiGuid;
#endif

/** @file
  The internal header file includes the common header files, defines
  internal structure and functions used by ImageVerificationLib.

Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __IMAGEVERIFICATIONLIB_H__
#define __IMAGEVERIFICATIONLIB_H__

#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>
#include <Library/SecurityManagementLib.h>
#include <Library/PeCoffLib.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/DevicePath.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/VariableWrite.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/AuthenticatedVariableFormat.h>
#include <IndustryStandard/PeImage.h>

#define EFI_CERT_TYPE_RSA2048_SHA256_SIZE  256
#define EFI_CERT_TYPE_RSA2048_SIZE         256
#define MAX_NOTIFY_STRING_LEN              64
#define TWO_BYTE_ENCODE                    0x82

#define ALIGNMENT_SIZE  8
#define ALIGN_SIZE(a)  (((a) % ALIGNMENT_SIZE) ? ALIGNMENT_SIZE - ((a) % ALIGNMENT_SIZE) : 0)

//
// Image type definitions
//
#define IMAGE_UNKNOWN               0x00000000
#define IMAGE_FROM_FV               0x00000001
#define IMAGE_FROM_OPTION_ROM       0x00000002
#define IMAGE_FROM_REMOVABLE_MEDIA  0x00000003
#define IMAGE_FROM_FIXED_MEDIA      0x00000004

//
// Authorization policy bit definition
//
#define ALWAYS_EXECUTE                       0x00000000
#define NEVER_EXECUTE                        0x00000001
#define ALLOW_EXECUTE_ON_SECURITY_VIOLATION  0x00000002
#define DEFER_EXECUTE_ON_SECURITY_VIOLATION  0x00000003
#define DENY_EXECUTE_ON_SECURITY_VIOLATION   0x00000004
#define QUERY_USER_ON_SECURITY_VIOLATION     0x00000005

//
// Support hash types
//
#define HASHALG_SHA1    0x00000000
#define HASHALG_SHA224  0x00000001
#define HASHALG_SHA256  0x00000002
#define HASHALG_SHA384  0x00000003
#define HASHALG_SHA512  0x00000004
#define HASHALG_MAX     0x00000005

//
// Set max digest size as SHA512 Output (64 bytes) by far
//
#define MAX_DIGEST_SIZE  SHA512_DIGEST_SIZE
//
//
// PKCS7 Certificate definition
//
typedef struct {
  WIN_CERTIFICATE    Hdr;
  UINT8              CertData[1];
} WIN_CERTIFICATE_EFI_PKCS;

/**
  Retrieves the size, in bytes, of the context buffer required for hash operations.

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
// Hash Algorithm Table
//
typedef struct {
  //
  // Name for Hash Algorithm
  //
  CHAR16                   *Name;
  //
  // Digest Length
  //
  UINTN                    DigestLength;
  //
  // Hash Algorithm OID ASN.1 Value
  //
  UINT8                    *OidValue;
  //
  // Length of Hash OID Value
  //
  UINTN                    OidLength;
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
} HASH_TABLE;

#endif

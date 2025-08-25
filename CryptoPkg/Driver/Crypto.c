/** @file
  Implements the EDK II Crypto Protocol/PPI services using the library services
  from BaseCryptLib and TlsLib.

  Copyright (C) Microsoft Corporation. All rights reserved.
  Copyright (c) 2019 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/TlsLib.h>
#include <Protocol/Crypto.h>
#include <Pcd/PcdCryptoServiceFamilyEnable.h>

/**
  A macro used to retrieve the FixedAtBuild PcdCryptoServiceFamilyEnable with a
  typecast to its associcted structure type PCD_CRYPTO_SERVICE_FAMILY_ENABLE.
**/
#define EDKII_CRYPTO_PCD  ((const PCD_CRYPTO_SERVICE_FAMILY_ENABLE *)\
  (FixedPcdGetPtr (PcdCryptoServiceFamilyEnable)))

/**
  A macro used to call a non-void BaseCryptLib function if it is enabled.

  If a BaseCryptLib function is not enabled, there will be no references to it
  from this module and will be optimized away reducing the size of this module.

  @param  Enable            The name of the enable field in PCD
                            PcdCryptoServiceFamilyEnable for the BaseCryptLib
                            function being called.  If the value of this field
                            is non-zero, then the BaseCryptLib function is
                            enabled.
  @param  Function          The name of the BaseCryptLib function.
  @param  Args              The argument list to pass to Function.
  @param  ErrorReturnValue  The value to return if the BaseCryptLib function is
                            not enabled.

**/
#define CALL_BASECRYPTLIB(Enable, Function, Args, ErrorReturnValue) \
  EDKII_CRYPTO_PCD->Enable                                          \
    ? Function Args                                                 \
    : (BaseCryptLibServiceNotEnabled (#Function), ErrorReturnValue)

/**
  A macro used to call a void BaseCryptLib function if it is enabled.

  If a BaseCryptLib function is not enabled, there will be no references to it
  from this module and will be optimized away reducing the size of this module.

  @param  Enable            The name of the enable field in PCD
                            PcdCryptoServiceFamilyEnable for the BaseCryptLib
                            function being called.  If the value of this field
                            is non-zero, then the BaseCryptLib function is
                            enabled.
  @param  Function          The name of the BaseCryptLib function.
  @param  Args              The argument list to pass to Function.

**/
#define CALL_VOID_BASECRYPTLIB(Enable, Function, Args)  \
  EDKII_CRYPTO_PCD->Enable                              \
    ? Function Args                                     \
    : BaseCryptLibServiceNotEnabled (#Function)

/**
  Internal worker function that prints a debug message and asserts if a call is
  made to a BaseCryptLib function that is not enabled in the EDK II Crypto
  Protocol/PPI.

  If this debug message and assert are observed, then a module is using
  BaseCryptLib function that is not enabled in a Crypto driver.  The
  PcdCryptoServiceFamilyEnable should be updated to enable the missing service.

  @param[in]  FunctionName  Null-terminated ASCII string that is the name of an
                            EDK II Crypto service.

**/
static
VOID
BaseCryptLibServiceNotEnabled (
  IN CONST CHAR8  *FunctionName
  )
{
  DEBUG ((DEBUG_ERROR, "[%a] Function %a() is not enabled\n", gEfiCallerBaseName, FunctionName));
  ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
}

/**
  Internal worker function that prints a debug message and asserts if a call is
  made to a BaseCryptLib function that is deprecated and unsupported any longer.

  @param[in]  FunctionName  Null-terminated ASCII string that is the name of an
                            EDK II Crypto service.

**/
static
VOID
BaseCryptLibServiceDeprecated (
  IN CONST CHAR8  *FunctionName
  )
{
  DEBUG ((DEBUG_ERROR, "[%a] Function %a() is deprecated and unsupported any longer\n", gEfiCallerBaseName, FunctionName));
  ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
}

/**
  Returns the version of the EDK II Crypto Protocol.

  @return  The version of the EDK II Crypto Protocol.

**/
UINTN
EFIAPI
CryptoServiceGetCryptoVersion (
  VOID
  )
{
  return EDKII_CRYPTO_VERSION;
}

// =====================================================================================
//    One-Way Cryptographic Hash Primitives
// =====================================================================================

/**
  MD4 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
DeprecatedCryptoServiceMd4GetContextSize (
  VOID
  )
{
  return BaseCryptLibServiceDeprecated ("Md4GetContextSize"), 0;
}

/**
  MD4 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[out]  Md4Context  Pointer to MD4 context being initialized.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceMd4Init (
  OUT  VOID  *Md4Context
  )
{
  return BaseCryptLibServiceDeprecated ("Md4Init"), FALSE;
}

/**
  MD4 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in]  Md4Context     Pointer to MD4 context being copied.
  @param[out] NewMd4Context  Pointer to new MD4 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceMd4Duplicate (
  IN   CONST VOID  *Md4Context,
  OUT  VOID        *NewMd4Context
  )
{
  return BaseCryptLibServiceDeprecated ("Md4Duplicate"), FALSE;
}

/**
  MD4 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in, out]  Md4Context  Pointer to the MD4 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize    Size of Data buffer in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceMd4Update (
  IN OUT  VOID        *Md4Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return BaseCryptLibServiceDeprecated ("Md4Update"), FALSE;
}

/**
  MD4 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in, out]  Md4Context  Pointer to the MD4 context.
  @param[out]      HashValue   Pointer to a buffer that receives the MD4 digest
                               value (16 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceMd4Final (
  IN OUT  VOID   *Md4Context,
  OUT     UINT8  *HashValue
  )
{
  return BaseCryptLibServiceDeprecated ("Md4Final"), FALSE;
}

/**
  MD4 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the MD4 digest
                           value (16 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceMd4HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  )
{
  return BaseCryptLibServiceDeprecated ("Md4HashAll"), FALSE;
}

#ifndef ENABLE_MD5_DEPRECATED_INTERFACES

/**
  Retrieves the size, in bytes, of the context buffer required for MD5 hash operations.

  If this interface is not supported, then return zero.

  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
DeprecatedCryptoServiceMd5GetContextSize (
  VOID
  )
{
  return BaseCryptLibServiceDeprecated ("Md5GetContextSize"), 0;
}

/**
  Initializes user-supplied memory pointed by Md5Context as MD5 hash context for
  subsequent use.

  If Md5Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  Md5Context  Pointer to MD5 context being initialized.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceMd5Init (
  OUT  VOID  *Md5Context
  )
{
  return BaseCryptLibServiceDeprecated ("Md5Init"), FALSE;
}

/**
  Makes a copy of an existing MD5 context.

  If Md5Context is NULL, then return FALSE.
  If NewMd5Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Md5Context     Pointer to MD5 context being copied.
  @param[out] NewMd5Context  Pointer to new MD5 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceMd5Duplicate (
  IN   CONST VOID  *Md5Context,
  OUT  VOID        *NewMd5Context
  )
{
  return BaseCryptLibServiceDeprecated ("Md5Init"), FALSE;
}

/**
  Digests the input data and updates MD5 context.

  This function performs MD5 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  MD5 context should be already correctly initialized by Md5Init(), and should not be finalized
  by Md5Final(). Behavior with invalid context is undefined.

  If Md5Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Md5Context  Pointer to the MD5 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize    Size of Data buffer in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceMd5Update (
  IN OUT  VOID        *Md5Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return BaseCryptLibServiceDeprecated ("Md5Init"), FALSE;
}

/**
  Completes computation of the MD5 digest value.

  This function completes MD5 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the MD5 context cannot
  be used again.
  MD5 context should be already correctly initialized by Md5Init(), and should not be
  finalized by Md5Final(). Behavior with invalid MD5 context is undefined.

  If Md5Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Md5Context  Pointer to the MD5 context.
  @param[out]      HashValue   Pointer to a buffer that receives the MD5 digest
                               value (16 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceMd5Final (
  IN OUT  VOID   *Md5Context,
  OUT     UINT8  *HashValue
  )
{
  return BaseCryptLibServiceDeprecated ("Md5Final"), FALSE;
}

/**
  Computes the MD5 message digest of a input data buffer.

  This function performs the MD5 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the MD5 digest
                           value (16 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceMd5HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  )
{
  return BaseCryptLibServiceDeprecated ("Md5HashAll"), FALSE;
}

#else

/**
  Retrieves the size, in bytes, of the context buffer required for MD5 hash operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for MD5 hash operations.
  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
CryptoServiceMd5GetContextSize (
  VOID
  )
{
  return CALL_BASECRYPTLIB (Md5.Services.GetContextSize, Md5GetContextSize, (), 0);
}

/**
  Initializes user-supplied memory pointed by Md5Context as MD5 hash context for
  subsequent use.

  If Md5Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  Md5Context  Pointer to MD5 context being initialized.

  @retval TRUE   MD5 context initialization succeeded.
  @retval FALSE  MD5 context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceMd5Init (
  OUT  VOID  *Md5Context
  )
{
  return CALL_BASECRYPTLIB (Md5.Services.Init, Md5Init, (Md5Context), FALSE);
}

/**
  Makes a copy of an existing MD5 context.

  If Md5Context is NULL, then return FALSE.
  If NewMd5Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Md5Context     Pointer to MD5 context being copied.
  @param[out] NewMd5Context  Pointer to new MD5 context.

  @retval TRUE   MD5 context copy succeeded.
  @retval FALSE  MD5 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceMd5Duplicate (
  IN   CONST VOID  *Md5Context,
  OUT  VOID        *NewMd5Context
  )
{
  return CALL_BASECRYPTLIB (Md5.Services.Duplicate, Md5Duplicate, (Md5Context, NewMd5Context), FALSE);
}

/**
  Digests the input data and updates MD5 context.

  This function performs MD5 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  MD5 context should be already correctly initialized by Md5Init(), and should not be finalized
  by Md5Final(). Behavior with invalid context is undefined.

  If Md5Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Md5Context  Pointer to the MD5 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize    Size of Data buffer in bytes.

  @retval TRUE   MD5 data digest succeeded.
  @retval FALSE  MD5 data digest failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceMd5Update (
  IN OUT  VOID        *Md5Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return CALL_BASECRYPTLIB (Md5.Services.Update, Md5Update, (Md5Context, Data, DataSize), FALSE);
}

/**
  Completes computation of the MD5 digest value.

  This function completes MD5 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the MD5 context cannot
  be used again.
  MD5 context should be already correctly initialized by Md5Init(), and should not be
  finalized by Md5Final(). Behavior with invalid MD5 context is undefined.

  If Md5Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Md5Context  Pointer to the MD5 context.
  @param[out]      HashValue   Pointer to a buffer that receives the MD5 digest
                               value (16 bytes).

  @retval TRUE   MD5 digest computation succeeded.
  @retval FALSE  MD5 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceMd5Final (
  IN OUT  VOID   *Md5Context,
  OUT     UINT8  *HashValue
  )
{
  return CALL_BASECRYPTLIB (Md5.Services.Final, Md5Final, (Md5Context, HashValue), FALSE);
}

/**
  Computes the MD5 message digest of a input data buffer.

  This function performs the MD5 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the MD5 digest
                           value (16 bytes).

  @retval TRUE   MD5 digest computation succeeded.
  @retval FALSE  MD5 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceMd5HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  )
{
  return CALL_BASECRYPTLIB (Md5.Services.HashAll, Md5HashAll, (Data, DataSize, HashValue), FALSE);
}

#endif

#ifdef DISABLE_SHA1_DEPRECATED_INTERFACES

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-1 hash operations.

  If this interface is not supported, then return zero.

  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
DeprecatedCryptoServiceSha1GetContextSize (
  VOID
  )
{
  return BaseCryptLibServiceDeprecated ("Sha1GetContextSize"), 0;
}

/**
  Initializes user-supplied memory pointed by Sha1Context as SHA-1 hash context for
  subsequent use.

  If Sha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  Sha1Context  Pointer to SHA-1 context being initialized.

  @retval TRUE   SHA-1 context initialization succeeded.
  @retval FALSE  SHA-1 context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceSha1Init (
  OUT  VOID  *Sha1Context
  )
{
  return BaseCryptLibServiceDeprecated ("Sha1Init"), FALSE;
}

/**
  Makes a copy of an existing SHA-1 context.

  If Sha1Context is NULL, then return FALSE.
  If NewSha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Sha1Context     Pointer to SHA-1 context being copied.
  @param[out] NewSha1Context  Pointer to new SHA-1 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceSha1Duplicate (
  IN   CONST VOID  *Sha1Context,
  OUT  VOID        *NewSha1Context
  )
{
  return BaseCryptLibServiceDeprecated ("Sha1Duplicate"), FALSE;
}

/**
  Digests the input data and updates SHA-1 context.

  This function performs SHA-1 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-1 context should be already correctly initialized by Sha1Init(), and should not be finalized
  by Sha1Final(). Behavior with invalid context is undefined.

  If Sha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context.
  @param[in]       Data         Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize     Size of Data buffer in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceSha1Update (
  IN OUT  VOID        *Sha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return BaseCryptLibServiceDeprecated ("Sha1Update"), FALSE;
}

/**
  Completes computation of the SHA-1 digest value.

  This function completes SHA-1 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-1 context cannot
  be used again.
  SHA-1 context should be already correctly initialized by Sha1Init(), and should not be
  finalized by Sha1Final(). Behavior with invalid SHA-1 context is undefined.

  If Sha1Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context.
  @param[out]      HashValue    Pointer to a buffer that receives the SHA-1 digest
                                value (20 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceSha1Final (
  IN OUT  VOID   *Sha1Context,
  OUT     UINT8  *HashValue
  )
{
  return BaseCryptLibServiceDeprecated ("Sha1Final"), FALSE;
}

/**
  Computes the SHA-1 message digest of a input data buffer.

  This function performs the SHA-1 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SHA-1 digest
                           value (20 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceSha1HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  )
{
  return BaseCryptLibServiceDeprecated ("Sha1HashAll"), FALSE;
}

#else

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-1 hash operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for SHA-1 hash operations.
  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
CryptoServiceSha1GetContextSize (
  VOID
  )
{
  return CALL_BASECRYPTLIB (Sha1.Services.GetContextSize, Sha1GetContextSize, (), 0);
}

/**
  Initializes user-supplied memory pointed by Sha1Context as SHA-1 hash context for
  subsequent use.

  If Sha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  Sha1Context  Pointer to SHA-1 context being initialized.

  @retval TRUE   SHA-1 context initialization succeeded.
  @retval FALSE  SHA-1 context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceSha1Init (
  OUT  VOID  *Sha1Context
  )
{
  return CALL_BASECRYPTLIB (Sha1.Services.Init, Sha1Init, (Sha1Context), FALSE);
}

/**
  Makes a copy of an existing SHA-1 context.

  If Sha1Context is NULL, then return FALSE.
  If NewSha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Sha1Context     Pointer to SHA-1 context being copied.
  @param[out] NewSha1Context  Pointer to new SHA-1 context.

  @retval TRUE   SHA-1 context copy succeeded.
  @retval FALSE  SHA-1 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceSha1Duplicate (
  IN   CONST VOID  *Sha1Context,
  OUT  VOID        *NewSha1Context
  )
{
  return CALL_BASECRYPTLIB (Sha1.Services.Duplicate, Sha1Duplicate, (Sha1Context, NewSha1Context), FALSE);
}

/**
  Digests the input data and updates SHA-1 context.

  This function performs SHA-1 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-1 context should be already correctly initialized by Sha1Init(), and should not be finalized
  by Sha1Final(). Behavior with invalid context is undefined.

  If Sha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context.
  @param[in]       Data         Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize     Size of Data buffer in bytes.

  @retval TRUE   SHA-1 data digest succeeded.
  @retval FALSE  SHA-1 data digest failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceSha1Update (
  IN OUT  VOID        *Sha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return CALL_BASECRYPTLIB (Sha1.Services.Update, Sha1Update, (Sha1Context, Data, DataSize), FALSE);
}

/**
  Completes computation of the SHA-1 digest value.

  This function completes SHA-1 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-1 context cannot
  be used again.
  SHA-1 context should be already correctly initialized by Sha1Init(), and should not be
  finalized by Sha1Final(). Behavior with invalid SHA-1 context is undefined.

  If Sha1Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context.
  @param[out]      HashValue    Pointer to a buffer that receives the SHA-1 digest
                                value (20 bytes).

  @retval TRUE   SHA-1 digest computation succeeded.
  @retval FALSE  SHA-1 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceSha1Final (
  IN OUT  VOID   *Sha1Context,
  OUT     UINT8  *HashValue
  )
{
  return CALL_BASECRYPTLIB (Sha1.Services.Final, Sha1Final, (Sha1Context, HashValue), FALSE);
}

/**
  Computes the SHA-1 message digest of a input data buffer.

  This function performs the SHA-1 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SHA-1 digest
                           value (20 bytes).

  @retval TRUE   SHA-1 digest computation succeeded.
  @retval FALSE  SHA-1 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceSha1HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  )
{
  return CALL_BASECRYPTLIB (Sha1.Services.HashAll, Sha1HashAll, (Data, DataSize, HashValue), FALSE);
}

#endif

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-256 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-256 hash operations.

**/
UINTN
EFIAPI
CryptoServiceSha256GetContextSize (
  VOID
  )
{
  return CALL_BASECRYPTLIB (Sha256.Services.GetContextSize, Sha256GetContextSize, (), 0);
}

/**
  Initializes user-supplied memory pointed by Sha256Context as SHA-256 hash context for
  subsequent use.

  If Sha256Context is NULL, then return FALSE.

  @param[out]  Sha256Context  Pointer to SHA-256 context being initialized.

  @retval TRUE   SHA-256 context initialization succeeded.
  @retval FALSE  SHA-256 context initialization failed.

**/
BOOLEAN
EFIAPI
CryptoServiceSha256Init (
  OUT  VOID  *Sha256Context
  )
{
  return CALL_BASECRYPTLIB (Sha256.Services.Init, Sha256Init, (Sha256Context), FALSE);
}

/**
  Makes a copy of an existing SHA-256 context.

  If Sha256Context is NULL, then return FALSE.
  If NewSha256Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Sha256Context     Pointer to SHA-256 context being copied.
  @param[out] NewSha256Context  Pointer to new SHA-256 context.

  @retval TRUE   SHA-256 context copy succeeded.
  @retval FALSE  SHA-256 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceSha256Duplicate (
  IN   CONST VOID  *Sha256Context,
  OUT  VOID        *NewSha256Context
  )
{
  return CALL_BASECRYPTLIB (Sha256.Services.Duplicate, Sha256Duplicate, (Sha256Context, NewSha256Context), FALSE);
}

/**
  Digests the input data and updates SHA-256 context.

  This function performs SHA-256 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-256 context should be already correctly initialized by Sha256Init(), and should not be finalized
  by Sha256Final(). Behavior with invalid context is undefined.

  If Sha256Context is NULL, then return FALSE.

  @param[in, out]  Sha256Context  Pointer to the SHA-256 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize       Size of Data buffer in bytes.

  @retval TRUE   SHA-256 data digest succeeded.
  @retval FALSE  SHA-256 data digest failed.

**/
BOOLEAN
EFIAPI
CryptoServiceSha256Update (
  IN OUT  VOID        *Sha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return CALL_BASECRYPTLIB (Sha256.Services.Update, Sha256Update, (Sha256Context, Data, DataSize), FALSE);
}

/**
  Completes computation of the SHA-256 digest value.

  This function completes SHA-256 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-256 context cannot
  be used again.
  SHA-256 context should be already correctly initialized by Sha256Init(), and should not be
  finalized by Sha256Final(). Behavior with invalid SHA-256 context is undefined.

  If Sha256Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sha256Context  Pointer to the SHA-256 context.
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-256 digest
                                  value (32 bytes).

  @retval TRUE   SHA-256 digest computation succeeded.
  @retval FALSE  SHA-256 digest computation failed.

**/
BOOLEAN
EFIAPI
CryptoServiceSha256Final (
  IN OUT  VOID   *Sha256Context,
  OUT     UINT8  *HashValue
  )
{
  return CALL_BASECRYPTLIB (Sha256.Services.Final, Sha256Final, (Sha256Context, HashValue), FALSE);
}

/**
  Computes the SHA-256 message digest of a input data buffer.

  This function performs the SHA-256 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SHA-256 digest
                           value (32 bytes).

  @retval TRUE   SHA-256 digest computation succeeded.
  @retval FALSE  SHA-256 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceSha256HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  )
{
  return CALL_BASECRYPTLIB (Sha256.Services.HashAll, Sha256HashAll, (Data, DataSize, HashValue), FALSE);
}

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-384 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-384 hash operations.

**/
UINTN
EFIAPI
CryptoServiceSha384GetContextSize (
  VOID
  )
{
  return CALL_BASECRYPTLIB (Sha384.Services.GetContextSize, Sha384GetContextSize, (), 0);
}

/**
  Initializes user-supplied memory pointed by Sha384Context as SHA-384 hash context for
  subsequent use.

  If Sha384Context is NULL, then return FALSE.

  @param[out]  Sha384Context  Pointer to SHA-384 context being initialized.

  @retval TRUE   SHA-384 context initialization succeeded.
  @retval FALSE  SHA-384 context initialization failed.

**/
BOOLEAN
EFIAPI
CryptoServiceSha384Init (
  OUT  VOID  *Sha384Context
  )
{
  return CALL_BASECRYPTLIB (Sha384.Services.Init, Sha384Init, (Sha384Context), FALSE);
}

/**
  Makes a copy of an existing SHA-384 context.

  If Sha384Context is NULL, then return FALSE.
  If NewSha384Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Sha384Context     Pointer to SHA-384 context being copied.
  @param[out] NewSha384Context  Pointer to new SHA-384 context.

  @retval TRUE   SHA-384 context copy succeeded.
  @retval FALSE  SHA-384 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceSha384Duplicate (
  IN   CONST VOID  *Sha384Context,
  OUT  VOID        *NewSha384Context
  )
{
  return CALL_BASECRYPTLIB (Sha384.Services.Duplicate, Sha384Duplicate, (Sha384Context, NewSha384Context), FALSE);
}

/**
  Digests the input data and updates SHA-384 context.

  This function performs SHA-384 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-384 context should be already correctly initialized by Sha384Init(), and should not be finalized
  by Sha384Final(). Behavior with invalid context is undefined.

  If Sha384Context is NULL, then return FALSE.

  @param[in, out]  Sha384Context  Pointer to the SHA-384 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize       Size of Data buffer in bytes.

  @retval TRUE   SHA-384 data digest succeeded.
  @retval FALSE  SHA-384 data digest failed.

**/
BOOLEAN
EFIAPI
CryptoServiceSha384Update (
  IN OUT  VOID        *Sha384Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return CALL_BASECRYPTLIB (Sha384.Services.Update, Sha384Update, (Sha384Context, Data, DataSize), FALSE);
}

/**
  Completes computation of the SHA-384 digest value.

  This function completes SHA-384 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-384 context cannot
  be used again.
  SHA-384 context should be already correctly initialized by Sha384Init(), and should not be
  finalized by Sha384Final(). Behavior with invalid SHA-384 context is undefined.

  If Sha384Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sha384Context  Pointer to the SHA-384 context.
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-384 digest
                                  value (48 bytes).

  @retval TRUE   SHA-384 digest computation succeeded.
  @retval FALSE  SHA-384 digest computation failed.

**/
BOOLEAN
EFIAPI
CryptoServiceSha384Final (
  IN OUT  VOID   *Sha384Context,
  OUT     UINT8  *HashValue
  )
{
  return CALL_BASECRYPTLIB (Sha384.Services.Final, Sha384Final, (Sha384Context, HashValue), FALSE);
}

/**
  Computes the SHA-384 message digest of a input data buffer.

  This function performs the SHA-384 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SHA-384 digest
                           value (48 bytes).

  @retval TRUE   SHA-384 digest computation succeeded.
  @retval FALSE  SHA-384 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceSha384HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  )
{
  return CALL_BASECRYPTLIB (Sha384.Services.HashAll, Sha384HashAll, (Data, DataSize, HashValue), FALSE);
}

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-512 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-512 hash operations.

**/
UINTN
EFIAPI
CryptoServiceSha512GetContextSize (
  VOID
  )
{
  return CALL_BASECRYPTLIB (Sha512.Services.GetContextSize, Sha512GetContextSize, (), 0);
}

/**
  Initializes user-supplied memory pointed by Sha512Context as SHA-512 hash context for
  subsequent use.

  If Sha512Context is NULL, then return FALSE.

  @param[out]  Sha512Context  Pointer to SHA-512 context being initialized.

  @retval TRUE   SHA-512 context initialization succeeded.
  @retval FALSE  SHA-512 context initialization failed.

**/
BOOLEAN
EFIAPI
CryptoServiceSha512Init (
  OUT  VOID  *Sha512Context
  )
{
  return CALL_BASECRYPTLIB (Sha512.Services.Init, Sha512Init, (Sha512Context), FALSE);
}

/**
  Makes a copy of an existing SHA-512 context.

  If Sha512Context is NULL, then return FALSE.
  If NewSha512Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Sha512Context     Pointer to SHA-512 context being copied.
  @param[out] NewSha512Context  Pointer to new SHA-512 context.

  @retval TRUE   SHA-512 context copy succeeded.
  @retval FALSE  SHA-512 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceSha512Duplicate (
  IN   CONST VOID  *Sha512Context,
  OUT  VOID        *NewSha512Context
  )
{
  return CALL_BASECRYPTLIB (Sha512.Services.Duplicate, Sha512Duplicate, (Sha512Context, NewSha512Context), FALSE);
}

/**
  Digests the input data and updates SHA-512 context.

  This function performs SHA-512 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-512 context should be already correctly initialized by Sha512Init(), and should not be finalized
  by Sha512Final(). Behavior with invalid context is undefined.

  If Sha512Context is NULL, then return FALSE.

  @param[in, out]  Sha512Context  Pointer to the SHA-512 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize       Size of Data buffer in bytes.

  @retval TRUE   SHA-512 data digest succeeded.
  @retval FALSE  SHA-512 data digest failed.

**/
BOOLEAN
EFIAPI
CryptoServiceSha512Update (
  IN OUT  VOID        *Sha512Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return CALL_BASECRYPTLIB (Sha512.Services.Update, Sha512Update, (Sha512Context, Data, DataSize), FALSE);
}

/**
  Completes computation of the SHA-512 digest value.

  This function completes SHA-512 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-512 context cannot
  be used again.
  SHA-512 context should be already correctly initialized by Sha512Init(), and should not be
  finalized by Sha512Final(). Behavior with invalid SHA-512 context is undefined.

  If Sha512Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sha512Context  Pointer to the SHA-512 context.
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-512 digest
                                  value (64 bytes).

  @retval TRUE   SHA-512 digest computation succeeded.
  @retval FALSE  SHA-512 digest computation failed.

**/
BOOLEAN
EFIAPI
CryptoServiceSha512Final (
  IN OUT  VOID   *Sha512Context,
  OUT     UINT8  *HashValue
  )
{
  return CALL_BASECRYPTLIB (Sha512.Services.Final, Sha512Final, (Sha512Context, HashValue), FALSE);
}

/**
  Computes the SHA-512 message digest of a input data buffer.

  This function performs the SHA-512 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SHA-512 digest
                           value (64 bytes).

  @retval TRUE   SHA-512 digest computation succeeded.
  @retval FALSE  SHA-512 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceSha512HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  )
{
  return CALL_BASECRYPTLIB (Sha512.Services.HashAll, Sha512HashAll, (Data, DataSize, HashValue), FALSE);
}

/**
  Retrieves the size, in bytes, of the context buffer required for SM3 hash operations.

  @return  The size, in bytes, of the context buffer required for SM3 hash operations.

**/
UINTN
EFIAPI
CryptoServiceSm3GetContextSize (
  VOID
  )
{
  return CALL_BASECRYPTLIB (Sm3.Services.GetContextSize, Sm3GetContextSize, (), 0);
}

/**
  Initializes user-supplied memory pointed by Sm3Context as SM3 hash context for
  subsequent use.

  If Sm3Context is NULL, then return FALSE.

  @param[out]  Sm3Context  Pointer to SM3 context being initialized.

  @retval TRUE   SM3 context initialization succeeded.
  @retval FALSE  SM3 context initialization failed.

**/
BOOLEAN
EFIAPI
CryptoServiceSm3Init (
  OUT  VOID  *Sm3Context
  )
{
  return CALL_BASECRYPTLIB (Sm3.Services.Init, Sm3Init, (Sm3Context), FALSE);
}

/**
  Makes a copy of an existing SM3 context.

  If Sm3Context is NULL, then return FALSE.
  If NewSm3Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Sm3Context     Pointer to SM3 context being copied.
  @param[out] NewSm3Context  Pointer to new SM3 context.

  @retval TRUE   SM3 context copy succeeded.
  @retval FALSE  SM3 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceSm3Duplicate (
  IN   CONST VOID  *Sm3Context,
  OUT  VOID        *NewSm3Context
  )
{
  return CALL_BASECRYPTLIB (Sm3.Services.Duplicate, Sm3Duplicate, (Sm3Context, NewSm3Context), FALSE);
}

/**
  Digests the input data and updates SM3 context.

  This function performs SM3 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SM3 context should be already correctly initialized by Sm3Init(), and should not be finalized
  by Sm3Final(). Behavior with invalid context is undefined.

  If Sm3Context is NULL, then return FALSE.

  @param[in, out]  Sm3Context     Pointer to the SM3 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize       Size of Data buffer in bytes.

  @retval TRUE   SM3 data digest succeeded.
  @retval FALSE  SM3 data digest failed.

**/
BOOLEAN
EFIAPI
CryptoServiceSm3Update (
  IN OUT  VOID        *Sm3Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return CALL_BASECRYPTLIB (Sm3.Services.Update, Sm3Update, (Sm3Context, Data, DataSize), FALSE);
}

/**
  Completes computation of the SM3 digest value.

  This function completes SM3 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SM3 context cannot
  be used again.
  SM3 context should be already correctly initialized by Sm3Init(), and should not be
  finalized by Sm3Final(). Behavior with invalid SM3 context is undefined.

  If Sm3Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sm3Context     Pointer to the SM3 context.
  @param[out]      HashValue      Pointer to a buffer that receives the SM3 digest
                                  value (32 bytes).

  @retval TRUE   SM3 digest computation succeeded.
  @retval FALSE  SM3 digest computation failed.

**/
BOOLEAN
EFIAPI
CryptoServiceSm3Final (
  IN OUT  VOID   *Sm3Context,
  OUT     UINT8  *HashValue
  )
{
  return CALL_BASECRYPTLIB (Sm3.Services.Final, Sm3Final, (Sm3Context, HashValue), FALSE);
}

/**
  Computes the SM3 message digest of a input data buffer.

  This function performs the SM3 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SM3 digest
                           value (32 bytes).

  @retval TRUE   SM3 digest computation succeeded.
  @retval FALSE  SM3 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceSm3HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  )
{
  return CALL_BASECRYPTLIB (Sm3.Services.HashAll, Sm3HashAll, (Data, DataSize, HashValue), FALSE);
}

// =====================================================================================
//    MAC (Message Authentication Code) Primitive
// =====================================================================================

/**
  HMAC MD5 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @retval  NULL  This interface is not supported.

**/
VOID *
EFIAPI
DeprecatedCryptoServiceHmacMd5New (
  VOID
  )
{
  return BaseCryptLibServiceDeprecated ("HmacMd5New"), NULL;
}

/**
  HMAC MD5 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in]  HmacMd5Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
EFIAPI
DeprecatedCryptoServiceHmacMd5Free (
  IN  VOID  *HmacMd5Ctx
  )
{
  BaseCryptLibServiceDeprecated ("HmacMd5Free");
}

/**
  HMAC MD5 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[out]  HmacMd5Context  Pointer to HMAC-MD5 context.
  @param[in]   Key             Pointer to the user-supplied key.
  @param[in]   KeySize         Key size in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceHmacMd5SetKey (
  OUT  VOID         *HmacMd5Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  return BaseCryptLibServiceDeprecated ("HmacMd5SetKey"), FALSE;
}

/**
  HMAC MD5 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in]  HmacMd5Context     Pointer to HMAC-MD5 context being copied.
  @param[out] NewHmacMd5Context  Pointer to new HMAC-MD5 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceHmacMd5Duplicate (
  IN   CONST VOID  *HmacMd5Context,
  OUT  VOID        *NewHmacMd5Context
  )
{
  return BaseCryptLibServiceDeprecated ("HmacMd5Duplicate"), FALSE;
}

/**
  HMAC MD5 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in, out]  HmacMd5Context  Pointer to the HMAC-MD5 context.
  @param[in]       Data            Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize        Size of Data buffer in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceHmacMd5Update (
  IN OUT  VOID        *HmacMd5Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return BaseCryptLibServiceDeprecated ("HmacMd5Update"), FALSE;
}

/**
  HMAC MD5 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in, out]  HmacMd5Context  Pointer to the HMAC-MD5 context.
  @param[out]      HmacValue       Pointer to a buffer that receives the HMAC-MD5 digest
                                   value (16 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceHmacMd5Final (
  IN OUT  VOID   *HmacMd5Context,
  OUT     UINT8  *HmacValue
  )
{
  return BaseCryptLibServiceDeprecated ("HmacMd5Final"), FALSE;
}

/**
  HMAC SHA1 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @return  NULL   This interface is not supported.

**/
VOID *
EFIAPI
DeprecatedCryptoServiceHmacSha1New (
  VOID
  )
{
  return BaseCryptLibServiceDeprecated ("HmacSha1New"), NULL;
}

/**
  HMAC SHA1 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in]  HmacSha1Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
EFIAPI
DeprecatedCryptoServiceHmacSha1Free (
  IN  VOID  *HmacSha1Ctx
  )
{
  BaseCryptLibServiceDeprecated ("HmacSha1Free");
}

/**
  HMAC SHA1 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[out]  HmacSha1Context  Pointer to HMAC-SHA1 context.
  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceHmacSha1SetKey (
  OUT  VOID         *HmacSha1Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  return BaseCryptLibServiceDeprecated ("HmacSha1SetKey"), FALSE;
}

/**
  HMAC SHA1 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in]  HmacSha1Context     Pointer to HMAC-SHA1 context being copied.
  @param[out] NewHmacSha1Context  Pointer to new HMAC-SHA1 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceHmacSha1Duplicate (
  IN   CONST VOID  *HmacSha1Context,
  OUT  VOID        *NewHmacSha1Context
  )
{
  return BaseCryptLibServiceDeprecated ("HmacSha1Duplicate"), FALSE;
}

/**
  HMAC SHA1 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in, out]  HmacSha1Context Pointer to the HMAC-SHA1 context.
  @param[in]       Data            Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize        Size of Data buffer in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceHmacSha1Update (
  IN OUT  VOID        *HmacSha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return BaseCryptLibServiceDeprecated ("HmacSha1Update"), FALSE;
}

/**
  HMAC SHA1 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in, out]  HmacSha1Context  Pointer to the HMAC-SHA1 context.
  @param[out]      HmacValue        Pointer to a buffer that receives the HMAC-SHA1 digest
                                    value (20 bytes).

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceHmacSha1Final (
  IN OUT  VOID   *HmacSha1Context,
  OUT     UINT8  *HmacValue
  )
{
  return BaseCryptLibServiceDeprecated ("HmacSha1Final"), FALSE;
}

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-SHA256 use.

  @return  Pointer to the HMAC_CTX context that has been initialized.
           If the allocations fails, HmacSha256New() returns NULL.

**/
VOID *
EFIAPI
CryptoServiceHmacSha256New (
  VOID
  )
{
  return CALL_BASECRYPTLIB (HmacSha256.Services.New, HmacSha256New, (), NULL);
}

/**
  Release the specified HMAC_CTX context.

  @param[in]  HmacSha256Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
EFIAPI
CryptoServiceHmacSha256Free (
  IN  VOID  *HmacSha256Ctx
  )
{
  CALL_VOID_BASECRYPTLIB (HmacSha256.Services.Free, HmacSha256Free, (HmacSha256Ctx));
}

/**
  Set user-supplied key for subsequent use. It must be done before any
  calling to HmacSha256Update().

  If HmacSha256Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  HmacSha256Context  Pointer to HMAC-SHA256 context.
  @param[in]   Key                Pointer to the user-supplied key.
  @param[in]   KeySize            Key size in bytes.

  @retval TRUE   The Key is set successfully.
  @retval FALSE  The Key is set unsuccessfully.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceHmacSha256SetKey (
  OUT  VOID         *HmacSha256Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  return CALL_BASECRYPTLIB (HmacSha256.Services.SetKey, HmacSha256SetKey, (HmacSha256Context, Key, KeySize), FALSE);
}

/**
  Makes a copy of an existing HMAC-SHA256 context.

  If HmacSha256Context is NULL, then return FALSE.
  If NewHmacSha256Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  HmacSha256Context     Pointer to HMAC-SHA256 context being copied.
  @param[out] NewHmacSha256Context  Pointer to new HMAC-SHA256 context.

  @retval TRUE   HMAC-SHA256 context copy succeeded.
  @retval FALSE  HMAC-SHA256 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceHmacSha256Duplicate (
  IN   CONST VOID  *HmacSha256Context,
  OUT  VOID        *NewHmacSha256Context
  )
{
  return CALL_BASECRYPTLIB (HmacSha256.Services.Duplicate, HmacSha256Duplicate, (HmacSha256Context, NewHmacSha256Context), FALSE);
}

/**
  Digests the input data and updates HMAC-SHA256 context.

  This function performs HMAC-SHA256 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-SHA256 context should be initialized by HmacSha256New(), and should not be finalized
  by HmacSha256Final(). Behavior with invalid context is undefined.

  If HmacSha256Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacSha256Context Pointer to the HMAC-SHA256 context.
  @param[in]       Data              Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize          Size of Data buffer in bytes.

  @retval TRUE   HMAC-SHA256 data digest succeeded.
  @retval FALSE  HMAC-SHA256 data digest failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceHmacSha256Update (
  IN OUT  VOID        *HmacSha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return CALL_BASECRYPTLIB (HmacSha256.Services.Update, HmacSha256Update, (HmacSha256Context, Data, DataSize), FALSE);
}

/**
  Completes computation of the HMAC-SHA256 digest value.

  This function completes HMAC-SHA256 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-SHA256 context cannot
  be used again.
  HMAC-SHA256 context should be initialized by HmacSha256New(), and should not be finalized
  by HmacSha256Final(). Behavior with invalid HMAC-SHA256 context is undefined.

  If HmacSha256Context is NULL, then return FALSE.
  If HmacValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacSha256Context  Pointer to the HMAC-SHA256 context.
  @param[out]      HmacValue          Pointer to a buffer that receives the HMAC-SHA256 digest
                                      value (32 bytes).

  @retval TRUE   HMAC-SHA256 digest computation succeeded.
  @retval FALSE  HMAC-SHA256 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceHmacSha256Final (
  IN OUT  VOID   *HmacSha256Context,
  OUT     UINT8  *HmacValue
  )
{
  return CALL_BASECRYPTLIB (HmacSha256.Services.Final, HmacSha256Final, (HmacSha256Context, HmacValue), FALSE);
}

/**
  Computes the HMAC-SHA256 digest of a input data buffer.

  This function performs the HMAC-SHA256 digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be digested.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[in]   Key         Pointer to the user-supplied key.
  @param[in]   KeySize     Key size in bytes.
  @param[out]  HmacValue   Pointer to a buffer that receives the HMAC-SHA256 digest
                           value (32 bytes).

  @retval TRUE   HMAC-SHA256 digest computation succeeded.
  @retval FALSE  HMAC-SHA256 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceHmacSha256All (
  IN   CONST VOID   *Data,
  IN   UINTN        DataSize,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  OUT  UINT8        *HmacValue
  )
{
  return CALL_BASECRYPTLIB (HmacSha256.Services.All, HmacSha256All, (Data, DataSize, Key, KeySize, HmacValue), FALSE);
}

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-SHA384 use.

  @return  Pointer to the HMAC_CTX context that has been initialized.
           If the allocations fails, HmacSha384New() returns NULL.

**/
VOID *
EFIAPI
CryptoServiceHmacSha384New (
  VOID
  )
{
  return CALL_BASECRYPTLIB (HmacSha384.Services.New, HmacSha384New, (), NULL);
}

/**
  Release the specified HMAC_CTX context.

  @param[in]  HmacSha384Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
EFIAPI
CryptoServiceHmacSha384Free (
  IN  VOID  *HmacSha384Ctx
  )
{
  CALL_VOID_BASECRYPTLIB (HmacSha384.Services.Free, HmacSha384Free, (HmacSha384Ctx));
}

/**
  Set user-supplied key for subsequent use. It must be done before any
  calling to HmacSha384Update().

  If HmacSha384Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  HmacSha384Context  Pointer to HMAC-SHA384 context.
  @param[in]   Key                Pointer to the user-supplied key.
  @param[in]   KeySize            Key size in bytes.

  @retval TRUE   The Key is set successfully.
  @retval FALSE  The Key is set unsuccessfully.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceHmacSha384SetKey (
  OUT  VOID         *HmacSha384Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  return CALL_BASECRYPTLIB (HmacSha384.Services.SetKey, HmacSha384SetKey, (HmacSha384Context, Key, KeySize), FALSE);
}

/**
  Makes a copy of an existing HMAC-SHA384 context.

  If HmacSha384Context is NULL, then return FALSE.
  If NewHmacSha384Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  HmacSha384Context     Pointer to HMAC-SHA384 context being copied.
  @param[out] NewHmacSha384Context  Pointer to new HMAC-SHA384 context.

  @retval TRUE   HMAC-SHA384 context copy succeeded.
  @retval FALSE  HMAC-SHA384 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceHmacSha384Duplicate (
  IN   CONST VOID  *HmacSha384Context,
  OUT  VOID        *NewHmacSha384Context
  )
{
  return CALL_BASECRYPTLIB (HmacSha384.Services.Duplicate, HmacSha256Duplicate, (HmacSha384Context, NewHmacSha384Context), FALSE);
}

/**
  Digests the input data and updates HMAC-SHA384 context.

  This function performs HMAC-SHA384 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-SHA384 context should be initialized by HmacSha384New(), and should not be finalized
  by HmacSha384Final(). Behavior with invalid context is undefined.

  If HmacSha384Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacSha384Context Pointer to the HMAC-SHA384 context.
  @param[in]       Data              Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize          Size of Data buffer in bytes.

  @retval TRUE   HMAC-SHA384 data digest succeeded.
  @retval FALSE  HMAC-SHA384 data digest failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceHmacSha384Update (
  IN OUT  VOID        *HmacSha384Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  return CALL_BASECRYPTLIB (HmacSha384.Services.Update, HmacSha384Update, (HmacSha384Context, Data, DataSize), FALSE);
}

/**
  Completes computation of the HMAC-SHA384 digest value.

  This function completes HMAC-SHA384 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-SHA384 context cannot
  be used again.
  HMAC-SHA384 context should be initialized by HmacSha384New(), and should not be finalized
  by HmacSha384Final(). Behavior with invalid HMAC-SHA384 context is undefined.

  If HmacSha384Context is NULL, then return FALSE.
  If HmacValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacSha384Context  Pointer to the HMAC-SHA384 context.
  @param[out]      HmacValue          Pointer to a buffer that receives the HMAC-SHA384 digest
                                      value (48 bytes).

  @retval TRUE   HMAC-SHA384 digest computation succeeded.
  @retval FALSE  HMAC-SHA384 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceHmacSha384Final (
  IN OUT  VOID   *HmacSha384Context,
  OUT     UINT8  *HmacValue
  )
{
  return CALL_BASECRYPTLIB (HmacSha384.Services.Final, HmacSha384Final, (HmacSha384Context, HmacValue), FALSE);
}

/**
  Computes the HMAC-SHA384 digest of a input data buffer.

  This function performs the HMAC-SHA384 digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be digested.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[in]   Key         Pointer to the user-supplied key.
  @param[in]   KeySize     Key size in bytes.
  @param[out]  HmacValue   Pointer to a buffer that receives the HMAC-SHA384 digest
                           value (48 bytes).

  @retval TRUE   HMAC-SHA384 digest computation succeeded.
  @retval FALSE  HMAC-SHA384 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceHmacSha384All (
  IN   CONST VOID   *Data,
  IN   UINTN        DataSize,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  OUT  UINT8        *HmacValue
  )
{
  return CALL_BASECRYPTLIB (HmacSha384.Services.All, HmacSha384All, (Data, DataSize, Key, KeySize, HmacValue), FALSE);
}

// =====================================================================================
//    Symmetric Cryptography Primitive
// =====================================================================================

/**
  TDES is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
DeprecatedCryptoServiceTdesGetContextSize (
  VOID
  )
{
  return BaseCryptLibServiceDeprecated ("TdesGetContextSize"), 0;
}

/**
  TDES is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[out]  TdesContext  Pointer to TDES context being initialized.
  @param[in]   Key          Pointer to the user-supplied TDES key.
  @param[in]   KeyLength    Length of TDES key in bits.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceTdesInit (
  OUT  VOID         *TdesContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  )
{
  return BaseCryptLibServiceDeprecated ("TdesInit"), FALSE;
}

/**
  TDES is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceTdesEcbEncrypt (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  )
{
  return BaseCryptLibServiceDeprecated ("TdesEcbEncrypt"), FALSE;
}

/**
  TDES is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be decrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[out]  Output       Pointer to a buffer that receives the TDES decryption output.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceTdesEcbDecrypt (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  )
{
  return BaseCryptLibServiceDeprecated ("TdesEcbDecrypt"), FALSE;
}

/**
  TDES is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[in]   Ivec         Pointer to initialization vector.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceTdesCbcEncrypt (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  )
{
  return BaseCryptLibServiceDeprecated ("TdesCbcEncrypt"), FALSE;
}

/**
  TDES is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[in]   Ivec         Pointer to initialization vector.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceTdesCbcDecrypt (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  )
{
  return BaseCryptLibServiceDeprecated ("TdesCbcDecrypt"), FALSE;
}

/**
  Retrieves the size, in bytes, of the context buffer required for AES operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for AES operations.
  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
CryptoServiceAesGetContextSize (
  VOID
  )
{
  return CALL_BASECRYPTLIB (Aes.Services.GetContextSize, AesGetContextSize, (), 0);
}

/**
  Initializes user-supplied memory as AES context for subsequent use.

  This function initializes user-supplied memory pointed by AesContext as AES context.
  In addition, it sets up all AES key materials for subsequent encryption and decryption
  operations.
  There are 3 options for key length, 128 bits, 192 bits, and 256 bits.

  If AesContext is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeyLength is not valid, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  AesContext  Pointer to AES context being initialized.
  @param[in]   Key         Pointer to the user-supplied AES key.
  @param[in]   KeyLength   Length of AES key in bits.

  @retval TRUE   AES context initialization succeeded.
  @retval FALSE  AES context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceAesInit (
  OUT  VOID         *AesContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  )
{
  return CALL_BASECRYPTLIB (Aes.Services.Init, AesInit, (AesContext, Key, KeyLength), FALSE);
}

/**
  AES ECB Mode is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceAesEcbEncrypt (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  )
{
  return BaseCryptLibServiceDeprecated ("AesEcbEncrypt"), FALSE;
}

/**
  AES ECB Mode is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be decrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[out]  Output      Pointer to a buffer that receives the AES decryption output.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceAesEcbDecrypt (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  )
{
  return BaseCryptLibServiceDeprecated ("AesEcbDecrypt"), FALSE;
}

/**
  Performs AES encryption on a data buffer of the specified size in CBC mode.

  This function performs AES encryption on data buffer pointed by Input, of specified
  size of InputSize, in CBC mode.
  InputSize must be multiple of block size (16 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  Initialization vector should be one block size (16 bytes).
  AesContext should be already correctly initialized by AesInit(). Behavior with
  invalid AES context is undefined.

  If AesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (16 bytes), then return FALSE.
  If Ivec is NULL, then return FALSE.
  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[in]   Ivec        Pointer to initialization vector.
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval TRUE   AES encryption succeeded.
  @retval FALSE  AES encryption failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceAesCbcEncrypt (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  )
{
  return CALL_BASECRYPTLIB (Aes.Services.CbcEncrypt, AesCbcEncrypt, (AesContext, Input, InputSize, Ivec, Output), FALSE);
}

/**
  Performs AES decryption on a data buffer of the specified size in CBC mode.

  This function performs AES decryption on data buffer pointed by Input, of specified
  size of InputSize, in CBC mode.
  InputSize must be multiple of block size (16 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  Initialization vector should be one block size (16 bytes).
  AesContext should be already correctly initialized by AesInit(). Behavior with
  invalid AES context is undefined.

  If AesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (16 bytes), then return FALSE.
  If Ivec is NULL, then return FALSE.
  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[in]   Ivec        Pointer to initialization vector.
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval TRUE   AES decryption succeeded.
  @retval FALSE  AES decryption failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceAesCbcDecrypt (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  )
{
  return CALL_BASECRYPTLIB (Aes.Services.CbcDecrypt, AesCbcDecrypt, (AesContext, Input, InputSize, Ivec, Output), FALSE);
}

/**
  ARC4 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @retval  0   This interface is not supported.

**/
UINTN
EFIAPI
DeprecatedCryptoServiceArc4GetContextSize (
  VOID
  )
{
  return BaseCryptLibServiceDeprecated ("Arc4GetContextSize"), 0;
}

/**
  ARC4 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[out]  Arc4Context  Pointer to ARC4 context being initialized.
  @param[in]   Key          Pointer to the user-supplied ARC4 key.
  @param[in]   KeySize      Size of ARC4 key in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceArc4Init (
  OUT  VOID         *Arc4Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  )
{
  return BaseCryptLibServiceDeprecated ("Arc4Init"), FALSE;
}

/**
  ARC4 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in, out]  Arc4Context  Pointer to the ARC4 context.
  @param[in]       Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]       InputSize    Size of the Input buffer in bytes.
  @param[out]      Output       Pointer to a buffer that receives the ARC4 encryption output.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceArc4Encrypt (
  IN OUT  VOID         *Arc4Context,
  IN      CONST UINT8  *Input,
  IN      UINTN        InputSize,
  OUT     UINT8        *Output
  )
{
  return BaseCryptLibServiceDeprecated ("Arc4Encrypt"), FALSE;
}

/**
  ARC4 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in, out]  Arc4Context  Pointer to the ARC4 context.
  @param[in]       Input        Pointer to the buffer containing the data to be decrypted.
  @param[in]       InputSize    Size of the Input buffer in bytes.
  @param[out]      Output       Pointer to a buffer that receives the ARC4 decryption output.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceArc4Decrypt (
  IN OUT  VOID   *Arc4Context,
  IN      UINT8  *Input,
  IN      UINTN  InputSize,
  OUT     UINT8  *Output
  )
{
  return BaseCryptLibServiceDeprecated ("Arc4Decrypt"), FALSE;
}

/**
  ARC4 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

  @param[in, out]  Arc4Context  Pointer to the ARC4 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
DeprecatedCryptoServiceArc4Reset (
  IN OUT  VOID  *Arc4Context
  )
{
  return BaseCryptLibServiceDeprecated ("Arc4Reset"), FALSE;
}

// =====================================================================================
//    Asymmetric Cryptography Primitive
// =====================================================================================

/**
  Allocates and initializes one RSA context for subsequent use.

  @return  Pointer to the RSA context that has been initialized.
           If the allocations fails, RsaNew() returns NULL.

**/
VOID *
EFIAPI
CryptoServiceRsaNew (
  VOID
  )
{
  return CALL_BASECRYPTLIB (Rsa.Services.New, RsaNew, (), NULL);
}

/**
  Release the specified RSA context.

  If RsaContext is NULL, then return FALSE.

  @param[in]  RsaContext  Pointer to the RSA context to be released.

**/
VOID
EFIAPI
CryptoServiceRsaFree (
  IN  VOID  *RsaContext
  )
{
  CALL_VOID_BASECRYPTLIB (Rsa.Services.Free, RsaFree, (RsaContext));
}

/**
  Sets the tag-designated key component into the established RSA context.

  This function sets the tag-designated RSA key component into the established
  RSA context from the user-specified non-negative integer (octet string format
  represented in RSA PKCS#1).
  If BigNumber is NULL, then the specified key component in RSA context is cleared.

  If RsaContext is NULL, then return FALSE.

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[in]       BigNumber   Pointer to octet integer buffer.
                               If NULL, then the specified key component in RSA
                               context is cleared.
  @param[in]       BnSize      Size of big number buffer in bytes.
                               If BigNumber is NULL, then it is ignored.

  @retval  TRUE   RSA key component was set successfully.
  @retval  FALSE  Invalid RSA key component tag.

**/
BOOLEAN
EFIAPI
CryptoServiceRsaSetKey (
  IN OUT  VOID         *RsaContext,
  IN      RSA_KEY_TAG  KeyTag,
  IN      CONST UINT8  *BigNumber,
  IN      UINTN        BnSize
  )
{
  return CALL_BASECRYPTLIB (Rsa.Services.SetKey, RsaSetKey, (RsaContext, KeyTag, BigNumber, BnSize), FALSE);
}

/**
  Gets the tag-designated RSA key component from the established RSA context.

  This function retrieves the tag-designated RSA key component from the
  established RSA context as a non-negative integer (octet string format
  represented in RSA PKCS#1).
  If specified key component has not been set or has been cleared, then returned
  BnSize is set to 0.
  If the BigNumber buffer is too small to hold the contents of the key, FALSE
  is returned and BnSize is set to the required buffer size to obtain the key.

  If RsaContext is NULL, then return FALSE.
  If BnSize is NULL, then return FALSE.
  If BnSize is large enough but BigNumber is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[out]      BigNumber   Pointer to octet integer buffer.
  @param[in, out]  BnSize      On input, the size of big number buffer in bytes.
                               On output, the size of data returned in big number buffer in bytes.

  @retval  TRUE   RSA key component was retrieved successfully.
  @retval  FALSE  Invalid RSA key component tag.
  @retval  FALSE  BnSize is too small.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceRsaGetKey (
  IN OUT  VOID         *RsaContext,
  IN      RSA_KEY_TAG  KeyTag,
  OUT     UINT8        *BigNumber,
  IN OUT  UINTN        *BnSize
  )
{
  return CALL_BASECRYPTLIB (Rsa.Services.GetKey, RsaGetKey, (RsaContext, KeyTag, BigNumber, BnSize), FALSE);
}

/**
  Generates RSA key components.

  This function generates RSA key components. It takes RSA public exponent E and
  length in bits of RSA modulus N as input, and generates all key components.
  If PublicExponent is NULL, the default RSA public exponent (0x10001) will be used.

  Before this function can be invoked, pseudorandom number generator must be correctly
  initialized by RandomSeed().

  If RsaContext is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  RsaContext           Pointer to RSA context being set.
  @param[in]       ModulusLength        Length of RSA modulus N in bits.
  @param[in]       PublicExponent       Pointer to RSA public exponent.
  @param[in]       PublicExponentSize   Size of RSA public exponent buffer in bytes.

  @retval  TRUE   RSA key component was generated successfully.
  @retval  FALSE  Invalid RSA key component tag.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceRsaGenerateKey (
  IN OUT  VOID         *RsaContext,
  IN      UINTN        ModulusLength,
  IN      CONST UINT8  *PublicExponent,
  IN      UINTN        PublicExponentSize
  )
{
  return CALL_BASECRYPTLIB (Rsa.Services.GenerateKey, RsaGenerateKey, (RsaContext, ModulusLength, PublicExponent, PublicExponentSize), FALSE);
}

/**
  Validates key components of RSA context.
  NOTE: This function performs integrity checks on all the RSA key material, so
        the RSA key structure must contain all the private key data.

  This function validates key components of RSA context in following aspects:
  - Whether p is a prime
  - Whether q is a prime
  - Whether n = p * q
  - Whether d*e = 1  mod lcm(p-1,q-1)

  If RsaContext is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  RsaContext  Pointer to RSA context to check.

  @retval  TRUE   RSA key components are valid.
  @retval  FALSE  RSA key components are not valid.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceRsaCheckKey (
  IN  VOID  *RsaContext
  )
{
  return CALL_BASECRYPTLIB (Rsa.Services.CheckKey, RsaCheckKey, (RsaContext), FALSE);
}

/**
  Carries out the RSA-SSA signature generation with EMSA-PKCS1-v1_5 encoding scheme.

  This function carries out the RSA-SSA signature generation with EMSA-PKCS1-v1_5 encoding scheme defined in
  RSA PKCS#1.
  If the Signature buffer is too small to hold the contents of signature, FALSE
  is returned and SigSize is set to the required buffer size to obtain the signature.

  If RsaContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If HashSize is not equal to the size of MD5, SHA-1 or SHA-256 digest, then return FALSE.
  If SigSize is large enough but Signature is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      RsaContext   Pointer to RSA context for signature generation.
  @param[in]      MessageHash  Pointer to octet message hash to be signed.
  @param[in]      HashSize     Size of the message hash in bytes.
  @param[out]     Signature    Pointer to buffer to receive RSA PKCS1-v1_5 signature.
  @param[in, out] SigSize      On input, the size of Signature buffer in bytes.
                               On output, the size of data returned in Signature buffer in bytes.

  @retval  TRUE   Signature successfully generated in PKCS1-v1_5.
  @retval  FALSE  Signature generation failed.
  @retval  FALSE  SigSize is too small.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceRsaPkcs1Sign (
  IN      VOID         *RsaContext,
  IN      CONST UINT8  *MessageHash,
  IN      UINTN        HashSize,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  )
{
  return CALL_BASECRYPTLIB (Rsa.Services.Pkcs1Sign, RsaPkcs1Sign, (RsaContext, MessageHash, HashSize, Signature, SigSize), FALSE);
}

/**
  Verifies the RSA-SSA signature with EMSA-PKCS1-v1_5 encoding scheme defined in
  RSA PKCS#1.

  If RsaContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If HashSize is not equal to the size of MD5, SHA-1, SHA-256 digest, then return FALSE.

  @param[in]  RsaContext   Pointer to RSA context for signature verification.
  @param[in]  MessageHash  Pointer to octet message hash to be checked.
  @param[in]  HashSize     Size of the message hash in bytes.
  @param[in]  Signature    Pointer to RSA PKCS1-v1_5 signature to be verified.
  @param[in]  SigSize      Size of signature in bytes.

  @retval  TRUE   Valid signature encoded in PKCS1-v1_5.
  @retval  FALSE  Invalid signature or invalid RSA context.

**/
BOOLEAN
EFIAPI
CryptoServiceRsaPkcs1Verify (
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *MessageHash,
  IN  UINTN        HashSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize
  )
{
  return CALL_BASECRYPTLIB (Rsa.Services.Pkcs1Verify, RsaPkcs1Verify, (RsaContext, MessageHash, HashSize, Signature, SigSize), FALSE);
}

/**
  Retrieve the RSA Private Key from the password-protected PEM key data.

  If PemData is NULL, then return FALSE.
  If RsaContext is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[in]  PemSize      Size of the PEM key data in bytes.
  @param[in]  Password     NULL-terminated passphrase used for encrypted PEM key data.
  @param[out] RsaContext   Pointer to new-generated RSA context which contain the retrieved
                           RSA private key component. Use RsaFree() function to free the
                           resource.

  @retval  TRUE   RSA Private Key was retrieved successfully.
  @retval  FALSE  Invalid PEM key data or incorrect password.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceRsaGetPrivateKeyFromPem (
  IN   CONST UINT8  *PemData,
  IN   UINTN        PemSize,
  IN   CONST CHAR8  *Password,
  OUT  VOID         **RsaContext
  )
{
  return CALL_BASECRYPTLIB (Rsa.Services.GetPrivateKeyFromPem, RsaGetPrivateKeyFromPem, (PemData, PemSize, Password, RsaContext), FALSE);
}

/**
  Retrieve the RSA Public Key from one DER-encoded X509 certificate.

  If Cert is NULL, then return FALSE.
  If RsaContext is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]  CertSize     Size of the X509 certificate in bytes.
  @param[out] RsaContext   Pointer to new-generated RSA context which contain the retrieved
                           RSA public key component. Use RsaFree() function to free the
                           resource.

  @retval  TRUE   RSA Public Key was retrieved successfully.
  @retval  FALSE  Fail to retrieve RSA public key from X509 certificate.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceRsaGetPublicKeyFromX509 (
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  VOID         **RsaContext
  )
{
  return CALL_BASECRYPTLIB (Rsa.Services.GetPublicKeyFromX509, RsaGetPublicKeyFromX509, (Cert, CertSize, RsaContext), FALSE);
}

/**
  Retrieve the subject bytes from one X.509 certificate.

  If Cert is NULL, then return FALSE.
  If SubjectSize is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[out]     CertSubject  Pointer to the retrieved certificate subject bytes.
  @param[in, out] SubjectSize  The size in bytes of the CertSubject buffer on input,
                               and the size of buffer returned CertSubject on output.

  @retval  TRUE   The certificate subject retrieved successfully.
  @retval  FALSE  Invalid certificate, or the SubjectSize is too small for the result.
                  The SubjectSize will be updated with the required size.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceX509GetSubjectName (
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     UINT8        *CertSubject,
  IN OUT  UINTN        *SubjectSize
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetSubjectName, X509GetSubjectName, (Cert, CertSize, CertSubject, SubjectSize), FALSE);
}

/**
  Retrieve the common name (CN) string from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     CommonName       Buffer to contain the retrieved certificate common
                                   name string (UTF8). At most CommonNameSize bytes will be
                                   written and the string will be null terminated. May be
                                   NULL in order to determine the size buffer needed.
  @param[in,out]  CommonNameSize   The size in bytes of the CommonName buffer on input,
                                   and the size of buffer returned CommonName on output.
                                   If CommonName is NULL then the amount of space needed
                                   in buffer (including the final null) is returned.

  @retval RETURN_SUCCESS           The certificate CommonName retrieved successfully.
  @retval RETURN_INVALID_PARAMETER If Cert is NULL.
                                   If CommonNameSize is NULL.
                                   If CommonName is not NULL and *CommonNameSize is 0.
                                   If Certificate is invalid.
  @retval RETURN_NOT_FOUND         If no CommonName entry exists.
  @retval RETURN_BUFFER_TOO_SMALL  If the CommonName is NULL. The required buffer size
                                   (including the final null) is returned in the
                                   CommonNameSize parameter.
  @retval RETURN_UNSUPPORTED       The operation is not supported.

**/
RETURN_STATUS
EFIAPI
CryptoServiceX509GetCommonName (
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     CHAR8        *CommonName   OPTIONAL,
  IN OUT  UINTN        *CommonNameSize
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetCommonName, X509GetCommonName, (Cert, CertSize, CommonName, CommonNameSize), RETURN_UNSUPPORTED);
}

/**
  Retrieve the organization name (O) string from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     NameBuffer       Buffer to contain the retrieved certificate organization
                                   name string. At most NameBufferSize bytes will be
                                   written and the string will be null terminated. May be
                                   NULL in order to determine the size buffer needed.
  @param[in,out]  NameBufferSize   The size in bytes of the Name buffer on input,
                                   and the size of buffer returned Name on output.
                                   If NameBuffer is NULL then the amount of space needed
                                   in buffer (including the final null) is returned.

  @retval RETURN_SUCCESS           The certificate Organization Name retrieved successfully.
  @retval RETURN_INVALID_PARAMETER If Cert is NULL.
                                   If NameBufferSize is NULL.
                                   If NameBuffer is not NULL and *CommonNameSize is 0.
                                   If Certificate is invalid.
  @retval RETURN_NOT_FOUND         If no Organization Name entry exists.
  @retval RETURN_BUFFER_TOO_SMALL  If the NameBuffer is NULL. The required buffer size
                                   (including the final null) is returned in the
                                   CommonNameSize parameter.
  @retval RETURN_UNSUPPORTED       The operation is not supported.

**/
RETURN_STATUS
EFIAPI
CryptoServiceX509GetOrganizationName (
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     CHAR8        *NameBuffer   OPTIONAL,
  IN OUT  UINTN        *NameBufferSize
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetOrganizationName, X509GetOrganizationName, (Cert, CertSize, NameBuffer, NameBufferSize), RETURN_UNSUPPORTED);
}

/**
  Verify one X509 certificate was issued by the trusted CA.

  If Cert is NULL, then return FALSE.
  If CACert is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate to be verified.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[in]      CACert       Pointer to the DER-encoded trusted CA certificate.
  @param[in]      CACertSize   Size of the CA Certificate in bytes.

  @retval  TRUE   The certificate was issued by the trusted CA.
  @retval  FALSE  Invalid certificate or the certificate was not issued by the given
                  trusted CA.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceX509VerifyCert (
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *CACert,
  IN  UINTN        CACertSize
  )
{
  return CALL_BASECRYPTLIB (X509.Services.VerifyCert, X509VerifyCert, (Cert, CertSize, CACert, CACertSize), FALSE);
}

/**
  Construct a X509 object from DER-encoded certificate data.

  If Cert is NULL, then return FALSE.
  If SingleX509Cert is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Cert            Pointer to the DER-encoded certificate data.
  @param[in]  CertSize        The size of certificate data in bytes.
  @param[out] SingleX509Cert  The generated X509 object.

  @retval     TRUE            The X509 object generation succeeded.
  @retval     FALSE           The operation failed.
  @retval     FALSE           This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceX509ConstructCertificate (
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  UINT8        **SingleX509Cert
  )
{
  return CALL_BASECRYPTLIB (X509.Services.ConstructCertificate, X509ConstructCertificate, (Cert, CertSize, SingleX509Cert), FALSE);
}

/**
  Construct a X509 stack object from a list of DER-encoded certificate data.

  If X509Stack is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  X509Stack  On input, pointer to an existing or NULL X509 stack object.
                              On output, pointer to the X509 stack object with new
                              inserted X509 certificate.
  @param[in]       Args       VA_LIST marker for the variable argument list.
                              A list of DER-encoded single certificate data followed
                              by certificate size. A NULL terminates the list. The
                              pairs are the arguments to X509ConstructCertificate().

  @retval     TRUE            The X509 stack construction succeeded.
  @retval     FALSE           The construction operation failed.
  @retval     FALSE           This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceX509ConstructCertificateStackV (
  IN OUT  UINT8    **X509Stack,
  IN      VA_LIST  Args
  )
{
  return CALL_BASECRYPTLIB (X509.Services.ConstructCertificateStackV, X509ConstructCertificateStackV, (X509Stack, Args), FALSE);
}

/**
  Construct a X509 stack object from a list of DER-encoded certificate data.

  If X509Stack is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  X509Stack  On input, pointer to an existing or NULL X509 stack object.
                              On output, pointer to the X509 stack object with new
                              inserted X509 certificate.
  @param           ...        A list of DER-encoded single certificate data followed
                              by certificate size. A NULL terminates the list. The
                              pairs are the arguments to X509ConstructCertificate().

  @retval     TRUE            The X509 stack construction succeeded.
  @retval     FALSE           The construction operation failed.
  @retval     FALSE           This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceX509ConstructCertificateStack (
  IN OUT  UINT8  **X509Stack,
  ...
  )
{
  VA_LIST  Args;
  BOOLEAN  Result;

  VA_START (Args, X509Stack);
  Result = CryptoServiceX509ConstructCertificateStackV (X509Stack, Args);
  VA_END (Args);
  return Result;
}

/**
  Release the specified X509 object.

  If the interface is not supported, then ASSERT().

  @param[in]  X509Cert  Pointer to the X509 object to be released.

**/
VOID
EFIAPI
CryptoServiceX509Free (
  IN  VOID  *X509Cert
  )
{
  CALL_VOID_BASECRYPTLIB (X509.Services.Free, X509Free, (X509Cert));
}

/**
  Release the specified X509 stack object.

  If the interface is not supported, then ASSERT().

  @param[in]  X509Stack  Pointer to the X509 stack object to be released.

**/
VOID
EFIAPI
CryptoServiceX509StackFree (
  IN  VOID  *X509Stack
  )
{
  CALL_VOID_BASECRYPTLIB (X509.Services.StackFree, X509StackFree, (X509Stack));
}

/**
  Retrieve the TBSCertificate from one given X.509 certificate.

  @param[in]      Cert         Pointer to the given DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[out]     TBSCert      DER-Encoded To-Be-Signed certificate.
  @param[out]     TBSCertSize  Size of the TBS certificate in bytes.

  If Cert is NULL, then return FALSE.
  If TBSCert is NULL, then return FALSE.
  If TBSCertSize is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @retval  TRUE   The TBSCertificate was retrieved successfully.
  @retval  FALSE  Invalid X.509 certificate.

**/
BOOLEAN
EFIAPI
CryptoServiceX509GetTBSCert (
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  OUT UINT8        **TBSCert,
  OUT UINTN        *TBSCertSize
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetTBSCert, X509GetTBSCert, (Cert, CertSize, TBSCert, TBSCertSize), FALSE);
}

/**
  Retrieve the version from one X.509 certificate.

  If Cert is NULL, then return FALSE.
  If CertSize is 0, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[out]     Version      Pointer to the retrieved version integer.

  @retval TRUE           The certificate version retrieved successfully.
  @retval FALSE          If  Cert is NULL or CertSize is Zero.
  @retval FALSE          The operation is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceX509GetVersion (
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     UINTN        *Version
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetVersion, X509GetVersion, (Cert, CertSize, Version), FALSE);
}

/**
  Retrieve the serialNumber from one X.509 certificate.

  If Cert is NULL, then return FALSE.
  If CertSize is 0, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[out]     SerialNumber  Pointer to the retrieved certificate SerialNumber bytes.
  @param[in, out] SerialNumberSize  The size in bytes of the SerialNumber buffer on input,
                               and the size of buffer returned SerialNumber on output.

  @retval TRUE                     The certificate serialNumber retrieved successfully.
  @retval FALSE                    If Cert is NULL or CertSize is Zero.
                                   If SerialNumberSize is NULL.
                                   If Certificate is invalid.
  @retval FALSE                    If no SerialNumber exists.
  @retval FALSE                    If the SerialNumber is NULL. The required buffer size
                                   (including the final null) is returned in the
                                   SerialNumberSize parameter.
  @retval FALSE                    The operation is not supported.
**/
BOOLEAN
EFIAPI
CryptoServiceX509GetSerialNumber (
  IN      CONST UINT8 *Cert,
  IN      UINTN CertSize,
  OUT     UINT8 *SerialNumber, OPTIONAL
  IN OUT  UINTN         *SerialNumberSize
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetSerialNumber, X509GetSerialNumber, (Cert, CertSize, SerialNumber, SerialNumberSize), FALSE);
}

/**
  Retrieve the issuer bytes from one X.509 certificate.

  If Cert is NULL, then return FALSE.
  If CertIssuerSize is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[out]     CertIssuer  Pointer to the retrieved certificate subject bytes.
  @param[in, out] CertIssuerSize  The size in bytes of the CertIssuer buffer on input,
                               and the size of buffer returned CertSubject on output.

  @retval  TRUE   The certificate issuer retrieved successfully.
  @retval  FALSE  Invalid certificate, or the CertIssuerSize is too small for the result.
                  The CertIssuerSize will be updated with the required size.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceX509GetIssuerName (
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     UINT8        *CertIssuer,
  IN OUT  UINTN        *CertIssuerSize
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetIssuerName, X509GetIssuerName, (Cert, CertSize, CertIssuer, CertIssuerSize), FALSE);
}

/**
  Retrieve the Signature Algorithm from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     Oid              Signature Algorithm Object identifier buffer.
  @param[in,out]  OidSize          Signature Algorithm Object identifier buffer size

  @retval TRUE                     The certificate Extension data retrieved successfully.
  @retval FALSE                    If Cert is NULL.
                                   If OidSize is NULL.
                                   If Oid is not NULL and *OidSize is 0.
                                   If Certificate is invalid.
  @retval FALSE                    If no SignatureType.
  @retval FALSE                    If the Oid is NULL. The required buffer size
                                   is returned in the OidSize.
  @retval FALSE                    The operation is not supported.
**/
BOOLEAN
EFIAPI
CryptoServiceX509GetSignatureAlgorithm (
  IN CONST UINT8 *Cert,
  IN       UINTN CertSize,
  OUT   UINT8 *Oid, OPTIONAL
  IN OUT   UINTN       *OidSize
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetSignatureAlgorithm, X509GetSignatureAlgorithm, (Cert, CertSize, Oid, OidSize), FALSE);
}

/**
  Retrieve Extension data from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[in]      Oid              Object identifier buffer
  @param[in]      OidSize          Object identifier buffer size
  @param[out]     ExtensionData    Extension bytes.
  @param[in, out] ExtensionDataSize Extension bytes size.

  @retval TRUE                     The certificate Extension data retrieved successfully.
  @retval TRUE                     The Certificate Extension is found, but the oid extension is not found.
  @retval FALSE                    If Cert is NULL.
                                   If ExtensionDataSize is NULL.
                                   If ExtensionData is not NULL and *ExtensionDataSize is 0.
                                   If Certificate is invalid.
  @retval FALSE                    If the ExtensionData is NULL. The required buffer size
                                   is returned in the ExtensionDataSize parameter.
  @retval FALSE                    The operation is not supported.
**/
BOOLEAN
EFIAPI
CryptoServiceX509GetExtensionData (
  IN     CONST UINT8  *Cert,
  IN     UINTN        CertSize,
  IN     CONST UINT8  *Oid,
  IN     UINTN        OidSize,
  OUT UINT8           *ExtensionData,
  IN OUT UINTN        *ExtensionDataSize
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetExtensionData, X509GetExtensionData, (Cert, CertSize, Oid, OidSize, ExtensionData, ExtensionDataSize), FALSE);
}

/**
  Retrieve the Extended Key Usage from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     Usage            Key Usage bytes.
  @param[in, out] UsageSize        Key Usage buffer sizs in bytes.

  @retval TRUE                     The Usage bytes retrieve successfully.
  @retval FALSE                    If Cert is NULL.
                                   If CertSize is NULL.
                                   If Usage is not NULL and *UsageSize is 0.
                                   If Cert is invalid.
  @retval FALSE                    If the Usage is NULL. The required buffer size
                                   is returned in the UsageSize parameter.
  @retval FALSE                    The operation is not supported.
**/
BOOLEAN
EFIAPI
CryptoServiceX509GetExtendedKeyUsage (
  IN     CONST UINT8  *Cert,
  IN     UINTN        CertSize,
  OUT UINT8           *Usage,
  IN OUT UINTN        *UsageSize
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetExtendedKeyUsage, X509GetExtendedKeyUsage, (Cert, CertSize, Usage, UsageSize), FALSE);
}

/**
  Retrieve the Validity from one X.509 certificate

  If Cert is NULL, then return FALSE.
  If CertIssuerSize is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[in]      From         notBefore Pointer to DateTime object.
  @param[in,out]  FromSize     notBefore DateTime object size.
  @param[in]      To           notAfter Pointer to DateTime object.
  @param[in,out]  ToSize       notAfter DateTime object size.

  Note: X509CompareDateTime to compare DateTime oject
        x509SetDateTime to get a DateTime object from a DateTimeStr

  @retval  TRUE   The certificate Validity retrieved successfully.
  @retval  FALSE  Invalid certificate, or Validity retrieve failed.
  @retval  FALSE  This interface is not supported.
**/
BOOLEAN
EFIAPI
CryptoServiceX509GetValidity  (
  IN     CONST UINT8  *Cert,
  IN     UINTN        CertSize,
  IN     UINT8        *From,
  IN OUT UINTN        *FromSize,
  IN     UINT8        *To,
  IN OUT UINTN        *ToSize
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetValidity, X509GetValidity, (Cert, CertSize, From, FromSize, To, ToSize), FALSE);
}

/**
  Format a DateTimeStr to DataTime object in DataTime Buffer

  If DateTimeStr is NULL, then return FALSE.
  If DateTimeSize is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      DateTimeStr      DateTime string like YYYYMMDDhhmmssZ
                                   Ref: https://www.w3.org/TR/NOTE-datetime
                                   Z stand for UTC time
  @param[out]     DateTime         Pointer to a DateTime object.
  @param[in,out]  DateTimeSize     DateTime object buffer size.

  @retval TRUE                     The DateTime object create successfully.
  @retval FALSE                    If DateTimeStr is NULL.
                                   If DateTimeSize is NULL.
                                   If DateTime is not NULL and *DateTimeSize is 0.
                                   If Year Month Day Hour Minute Second combination is invalid datetime.
  @retval FALSE                    If the DateTime is NULL. The required buffer size
                                   (including the final null) is returned in the
                                   DateTimeSize parameter.
  @retval FALSE                    The operation is not supported.
**/
BOOLEAN
EFIAPI
CryptoServiceX509FormatDateTime (
  IN CONST CHAR8  *DateTimeStr,
  OUT VOID        *DateTime,
  IN OUT UINTN    *DateTimeSize
  )
{
  return CALL_BASECRYPTLIB (X509.Services.FormatDateTime, X509FormatDateTime, (DateTimeStr, DateTime, DateTimeSize), FALSE);
}

/**
  Compare DateTime1 object and DateTime2 object.

  If DateTime1 is NULL, then return -2.
  If DateTime2 is NULL, then return -2.
  If DateTime1 == DateTime2, then return 0
  If DateTime1 > DateTime2, then return 1
  If DateTime1 < DateTime2, then return -1

  @param[in]      DateTime1         Pointer to a DateTime Ojbect
  @param[in]      DateTime2         Pointer to a DateTime Object

  @retval  0      If DateTime1 == DateTime2
  @retval  1      If DateTime1 > DateTime2
  @retval  -1     If DateTime1 < DateTime2
**/
INT32
EFIAPI
CryptoServiceX509CompareDateTime (
  IN  CONST VOID  *DateTime1,
  IN  CONST VOID  *DateTime2
  )
{
  return CALL_BASECRYPTLIB (X509.Services.CompareDateTime, X509CompareDateTime, (DateTime1, DateTime2), FALSE);
}

/**
  Retrieve the Key Usage from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     Usage            Key Usage (CRYPTO_X509_KU_*)

  @retval  TRUE   The certificate Key Usage retrieved successfully.
  @retval  FALSE  Invalid certificate, or Usage is NULL
  @retval  FALSE  This interface is not supported.
**/
BOOLEAN
EFIAPI
CryptoServiceX509GetKeyUsage (
  IN    CONST UINT8  *Cert,
  IN    UINTN        CertSize,
  OUT   UINTN        *Usage
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetKeyUsage, X509GetKeyUsage, (Cert, CertSize, Usage), FALSE);
}

/**
  Verify one X509 certificate was issued by the trusted CA.
  @param[in]      RootCert          Trusted Root Certificate buffer

  @param[in]      RootCertLength    Trusted Root Certificate buffer length
  @param[in]      CertChain         One or more ASN.1 DER-encoded X.509 certificates
                                    where the first certificate is signed by the Root
                                    Certificate or is the Root Cerificate itself. and
                                    subsequent cerificate is signed by the preceding
                                    cerificate.
  @param[in]      CertChainLength   Total length of the certificate chain, in bytes.

  @retval  TRUE   All cerificates was issued by the first certificate in X509Certchain.
  @retval  FALSE  Invalid certificate or the certificate was not issued by the given
                  trusted CA.
**/
BOOLEAN
EFIAPI
CryptoServiceX509VerifyCertChain (
  IN CONST UINT8  *RootCert,
  IN UINTN        RootCertLength,
  IN CONST UINT8  *CertChain,
  IN UINTN        CertChainLength
  )
{
  return CALL_BASECRYPTLIB (X509.Services.VerifyCertChain, X509VerifyCertChain, (RootCert, RootCertLength, CertChain, CertChainLength), FALSE);
}

/**
  Get one X509 certificate from CertChain.

  @param[in]      CertChain         One or more ASN.1 DER-encoded X.509 certificates
                                    where the first certificate is signed by the Root
                                    Certificate or is the Root Cerificate itself. and
                                    subsequent cerificate is signed by the preceding
                                    cerificate.
  @param[in]      CertChainLength   Total length of the certificate chain, in bytes.

  @param[in]      CertIndex         Index of certificate.

  @param[out]     Cert              The certificate at the index of CertChain.
  @param[out]     CertLength        The length certificate at the index of CertChain.

  @retval  TRUE   Success.
  @retval  FALSE  Failed to get certificate from certificate chain.
**/
BOOLEAN
EFIAPI
CryptoServiceX509GetCertFromCertChain (
  IN CONST UINT8   *CertChain,
  IN UINTN         CertChainLength,
  IN CONST INT32   CertIndex,
  OUT CONST UINT8  **Cert,
  OUT UINTN        *CertLength
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetCertFromCertChain, X509GetCertFromCertChain, (CertChain, CertChainLength, CertIndex, Cert, CertLength), FALSE);
}

/**
  Retrieve the tag and length of the tag.

  @param Ptr      The position in the ASN.1 data
  @param End      End of data
  @param Length   The variable that will receive the length
  @param Tag      The expected tag

  @retval      TRUE   Get tag successful
  @retval      FALSe  Failed to get tag or tag not match
**/
BOOLEAN
EFIAPI
CryptoServiceAsn1GetTag (
  IN OUT UINT8    **Ptr,
  IN CONST UINT8  *End,
  OUT UINTN       *Length,
  IN     UINT32   Tag
  )
{
  return CALL_BASECRYPTLIB (X509.Services.Asn1GetTag, Asn1GetTag, (Ptr, End, Length, Tag), FALSE);
}

/**
  Retrieve the basic constraints from one X.509 certificate.

  @param[in]      Cert                     Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize                 size of the X509 certificate in bytes.
  @param[out]     BasicConstraints         basic constraints bytes.
  @param[in, out] BasicConstraintsSize     basic constraints buffer sizs in bytes.

  @retval TRUE                     The basic constraints retrieve successfully.
  @retval FALSE                    If cert is NULL.
                                   If cert_size is NULL.
                                   If basic_constraints is not NULL and *basic_constraints_size is 0.
                                   If cert is invalid.
  @retval FALSE                    The required buffer size is small.
                                   The return buffer size is basic_constraints_size parameter.
  @retval FALSE                    If no Extension entry match oid.
  @retval FALSE                    The operation is not supported.
 **/
BOOLEAN
EFIAPI
CryptoServiceX509GetExtendedBasicConstraints             (
  CONST UINT8  *Cert,
  UINTN        CertSize,
  UINT8        *BasicConstraints,
  UINTN        *BasicConstraintsSize
  )
{
  return CALL_BASECRYPTLIB (X509.Services.GetExtendedBasicConstraints, X509GetExtendedBasicConstraints, (Cert, CertSize, BasicConstraints, BasicConstraintsSize), FALSE);
}

/**
  Derives a key from a password using a salt and iteration count, based on PKCS#5 v2.0
  password based encryption key derivation function PBKDF2, as specified in RFC 2898.

  If Password or Salt or OutKey is NULL, then return FALSE.
  If the hash algorithm could not be determined, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  PasswordLength  Length of input password in bytes.
  @param[in]  Password        Pointer to the array for the password.
  @param[in]  SaltLength      Size of the Salt in bytes.
  @param[in]  Salt            Pointer to the Salt.
  @param[in]  IterationCount  Number of iterations to perform. Its value should be
                              greater than or equal to 1.
  @param[in]  DigestSize      Size of the message digest to be used (eg. SHA256_DIGEST_SIZE).
                              NOTE: DigestSize will be used to determine the hash algorithm.
                                    Only SHA1_DIGEST_SIZE or SHA256_DIGEST_SIZE is supported.
  @param[in]  KeyLength       Size of the derived key buffer in bytes.
  @param[out] OutKey          Pointer to the output derived key buffer.

  @retval  TRUE   A key was derived successfully.
  @retval  FALSE  One of the pointers was NULL or one of the sizes was too large.
  @retval  FALSE  The hash algorithm could not be determined from the digest size.
  @retval  FALSE  The key derivation operation failed.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServicePkcs5HashPassword (
  IN  UINTN        PasswordLength,
  IN  CONST CHAR8  *Password,
  IN  UINTN        SaltLength,
  IN  CONST UINT8  *Salt,
  IN  UINTN        IterationCount,
  IN  UINTN        DigestSize,
  IN  UINTN        KeyLength,
  OUT UINT8        *OutKey
  )
{
  return CALL_BASECRYPTLIB (Pkcs.Services.Pkcs5HashPassword, Pkcs5HashPassword, (PasswordLength, Password, SaltLength, Salt, IterationCount, DigestSize, KeyLength, OutKey), FALSE);
}

/**
  Encrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the
  encrypted message in a newly allocated buffer.

  Things that can cause a failure include:
  - X509 key size does not match any known key size.
  - Fail to parse X509 certificate.
  - Fail to allocate an intermediate buffer.
  - Null pointer provided for a non-optional parameter.
  - Data size is too large for the provided key size (max size is a function of key size
    and hash digest size).

  @param[in]  PublicKey           A pointer to the DER-encoded X509 certificate that
                                  will be used to encrypt the data.
  @param[in]  PublicKeySize       Size of the X509 cert buffer.
  @param[in]  InData              Data to be encrypted.
  @param[in]  InDataSize          Size of the data buffer.
  @param[in]  PrngSeed            [Optional] If provided, a pointer to a random seed buffer
                                  to be used when initializing the PRNG. NULL otherwise.
  @param[in]  PrngSeedSize        [Optional] If provided, size of the random seed buffer.
                                  0 otherwise.
  @param[out] EncryptedData       Pointer to an allocated buffer containing the encrypted
                                  message.
  @param[out] EncryptedDataSize   Size of the encrypted message buffer.

  @retval     TRUE                Encryption was successful.
  @retval     FALSE               Encryption failed.

**/
BOOLEAN
EFIAPI
CryptoServicePkcs1v2Encrypt (
  IN   CONST UINT8  *PublicKey,
  IN   UINTN        PublicKeySize,
  IN   UINT8        *InData,
  IN   UINTN        InDataSize,
  IN   CONST UINT8  *PrngSeed   OPTIONAL,
  IN   UINTN        PrngSeedSize   OPTIONAL,
  OUT  UINT8        **EncryptedData,
  OUT  UINTN        *EncryptedDataSize
  )
{
  return CALL_BASECRYPTLIB (Pkcs.Services.Pkcs1v2Encrypt, Pkcs1v2Encrypt, (PublicKey, PublicKeySize, InData, InDataSize, PrngSeed, PrngSeedSize, EncryptedData, EncryptedDataSize), FALSE);
}

/**
  Encrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the
  encrypted message in a newly allocated buffer.

  Things that can cause a failure include:
  - X509 key size does not match any known key size.
  - Fail to allocate an intermediate buffer.
  - Null pointer provided for a non-optional parameter.
  - Data size is too large for the provided key size (max size is a function of key size
    and hash digest size).

  @param[in]  RsaContext          A pointer to an RSA context created by RsaNew() and
                                  provisioned with a public key using RsaSetKey().
  @param[in]  InData              Data to be encrypted.
  @param[in]  InDataSize          Size of the data buffer.
  @param[in]  PrngSeed            [Optional] If provided, a pointer to a random seed buffer
                                  to be used when initializing the PRNG. NULL otherwise.
  @param[in]  PrngSeedSize        [Optional] If provided, size of the random seed buffer.
                                  0 otherwise.
  @param[in]  DigestLen           [Optional] If provided, size of the hash used:
                                  SHA1_DIGEST_SIZE
                                  SHA256_DIGEST_SIZE
                                  SHA384_DIGEST_SIZE
                                  SHA512_DIGEST_SIZE
                                  0 to use default (SHA1)
  @param[out] EncryptedData       Pointer to an allocated buffer containing the encrypted
                                  message.
  @param[out] EncryptedDataSize   Size of the encrypted message buffer.

  @retval     TRUE                Encryption was successful.
  @retval     FALSE               Encryption failed.

**/
BOOLEAN
EFIAPI
CryptoServiceRsaOaepEncrypt (
  IN   VOID         *RsaContext,
  IN   UINT8        *InData,
  IN   UINTN        InDataSize,
  IN   CONST UINT8  *PrngSeed   OPTIONAL,
  IN   UINTN        PrngSeedSize   OPTIONAL,
  IN   UINT16       DigestLen   OPTIONAL,
  OUT  UINT8        **EncryptedData,
  OUT  UINTN        *EncryptedDataSize
  )
{
  return CALL_BASECRYPTLIB (Rsa.Services.RsaOaepEncrypt, RsaOaepEncrypt, (RsaContext, InData, InDataSize, PrngSeed, PrngSeedSize, DigestLen, EncryptedData, EncryptedDataSize), FALSE);
}

/**
  Decrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the
  decrypted message in a newly allocated buffer.

  Things that can cause a failure include:
  - Fail to parse private key.
  - Fail to allocate an intermediate buffer.
  - Null pointer provided for a non-optional parameter.

  @param[in]  PrivateKey          A pointer to the DER-encoded private key.
  @param[in]  PrivateKeySize      Size of the private key buffer.
  @param[in]  EncryptedData       Data to be decrypted.
  @param[in]  EncryptedDataSize   Size of the encrypted buffer.
  @param[out] OutData             Pointer to an allocated buffer containing the encrypted
                                  message.
  @param[out] OutDataSize         Size of the encrypted message buffer.

  @retval     TRUE                Encryption was successful.
  @retval     FALSE               Encryption failed.

**/
BOOLEAN
EFIAPI
CryptoServicePkcs1v2Decrypt (
  IN   CONST UINT8  *PrivateKey,
  IN   UINTN        PrivateKeySize,
  IN   UINT8        *EncryptedData,
  IN   UINTN        EncryptedDataSize,
  OUT  UINT8        **OutData,
  OUT  UINTN        *OutDataSize
  )
{
  return CALL_BASECRYPTLIB (Pkcs.Services.Pkcs1v2Decrypt, Pkcs1v2Decrypt, (PrivateKey, PrivateKeySize, EncryptedData, EncryptedDataSize, OutData, OutDataSize), FALSE);
}

/**
  Decrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the
  decrypted message in a newly allocated buffer.

  Things that can cause a failure include:
  - Fail to parse private key.
  - Fail to allocate an intermediate buffer.
  - Null pointer provided for a non-optional parameter.

  @param[in]  RsaContext          A pointer to an RSA context created by RsaNew() and
                                  provisioned with a private key using RsaSetKey().
  @param[in]  EncryptedData       Data to be decrypted.
  @param[in]  EncryptedDataSize   Size of the encrypted buffer.
  @param[in]  DigestLen           [Optional] If provided, size of the hash used:
                                  SHA1_DIGEST_SIZE
                                  SHA256_DIGEST_SIZE
                                  SHA384_DIGEST_SIZE
                                  SHA512_DIGEST_SIZE
                                  0 to use default (SHA1)
  @param[out] OutData             Pointer to an allocated buffer containing the encrypted
                                  message.
  @param[out] OutDataSize         Size of the encrypted message buffer.

  @retval     TRUE                Encryption was successful.
  @retval     FALSE               Encryption failed.

**/
BOOLEAN
EFIAPI
CryptoServiceRsaOaepDecrypt (
  IN   VOID    *RsaContext,
  IN   UINT8   *EncryptedData,
  IN   UINTN   EncryptedDataSize,
  IN   UINT16  DigestLen   OPTIONAL,
  OUT  UINT8   **OutData,
  OUT  UINTN   *OutDataSize
  )
{
  return CALL_BASECRYPTLIB (Rsa.Services.RsaOaepDecrypt, RsaOaepDecrypt, (RsaContext, EncryptedData, EncryptedDataSize, DigestLen, OutData, OutDataSize), FALSE);
}

/**
  Get the signer's certificates from PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard". The input signed data could be wrapped
  in a ContentInfo structure.

  If P7Data, CertStack, StackLength, TrustedCert or CertLength is NULL, then
  return FALSE. If P7Length overflow, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[out] CertStack    Pointer to Signer's certificates retrieved from P7Data.
                           It's caller's responsibility to free the buffer with
                           Pkcs7FreeSigners().
                           This data structure is EFI_CERT_STACK type.
  @param[out] StackLength  Length of signer's certificates in bytes.
  @param[out] TrustedCert  Pointer to a trusted certificate from Signer's certificates.
                           It's caller's responsibility to free the buffer with
                           Pkcs7FreeSigners().
  @param[out] CertLength   Length of the trusted certificate in bytes.

  @retval  TRUE            The operation is finished successfully.
  @retval  FALSE           Error occurs during the operation.
  @retval  FALSE           This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServicePkcs7GetSigners (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT UINT8        **CertStack,
  OUT UINTN        *StackLength,
  OUT UINT8        **TrustedCert,
  OUT UINTN        *CertLength
  )
{
  return CALL_BASECRYPTLIB (Pkcs.Services.Pkcs7GetSigners, Pkcs7GetSigners, (P7Data, P7Length, CertStack, StackLength, TrustedCert, CertLength), FALSE);
}

/**
  Wrap function to use free() to free allocated memory for certificates.

  If this interface is not supported, then ASSERT().

  @param[in]  Certs        Pointer to the certificates to be freed.

**/
VOID
EFIAPI
CryptoServicePkcs7FreeSigners (
  IN  UINT8  *Certs
  )
{
  CALL_VOID_BASECRYPTLIB (Pkcs.Services.Pkcs7FreeSigners, Pkcs7FreeSigners, (Certs));
}

/**
  Retrieves all embedded certificates from PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard", and outputs two certificate lists chained and
  unchained to the signer's certificates.
  The input signed data could be wrapped in a ContentInfo structure.

  @param[in]  P7Data            Pointer to the PKCS#7 message.
  @param[in]  P7Length          Length of the PKCS#7 message in bytes.
  @param[out] SignerChainCerts  Pointer to the certificates list chained to signer's
                                certificate. It's caller's responsibility to free the buffer
                                with Pkcs7FreeSigners().
                                This data structure is EFI_CERT_STACK type.
  @param[out] ChainLength       Length of the chained certificates list buffer in bytes.
  @param[out] UnchainCerts      Pointer to the unchained certificates lists. It's caller's
                                responsibility to free the buffer with Pkcs7FreeSigners().
                                This data structure is EFI_CERT_STACK type.
  @param[out] UnchainLength     Length of the unchained certificates list buffer in bytes.

  @retval  TRUE         The operation is finished successfully.
  @retval  FALSE        Error occurs during the operation.

**/
BOOLEAN
EFIAPI
CryptoServicePkcs7GetCertificatesList (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT UINT8        **SignerChainCerts,
  OUT UINTN        *ChainLength,
  OUT UINT8        **UnchainCerts,
  OUT UINTN        *UnchainLength
  )
{
  return CALL_BASECRYPTLIB (Pkcs.Services.Pkcs7GetCertificatesList, Pkcs7GetCertificatesList, (P7Data, P7Length, SignerChainCerts, ChainLength, UnchainCerts, UnchainLength), FALSE);
}

/**
  Creates a PKCS#7 signedData as described in "PKCS #7: Cryptographic Message
  Syntax Standard, version 1.5". This interface is only intended to be used for
  application to perform PKCS#7 functionality validation.

  If this interface is not supported, then return FALSE.

  @param[in]  PrivateKey       Pointer to the PEM-formatted private key data for
                               data signing.
  @param[in]  PrivateKeySize   Size of the PEM private key data in bytes.
  @param[in]  KeyPassword      NULL-terminated passphrase used for encrypted PEM
                               key data.
  @param[in]  InData           Pointer to the content to be signed.
  @param[in]  InDataSize       Size of InData in bytes.
  @param[in]  SignCert         Pointer to signer's DER-encoded certificate to sign with.
  @param[in]  OtherCerts       Pointer to an optional additional set of certificates to
                               include in the PKCS#7 signedData (e.g. any intermediate
                               CAs in the chain).
  @param[out] SignedData       Pointer to output PKCS#7 signedData. It's caller's
                               responsibility to free the buffer with FreePool().
  @param[out] SignedDataSize   Size of SignedData in bytes.

  @retval     TRUE             PKCS#7 data signing succeeded.
  @retval     FALSE            PKCS#7 data signing failed.
  @retval     FALSE            This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServicePkcs7Sign (
  IN   CONST UINT8  *PrivateKey,
  IN   UINTN        PrivateKeySize,
  IN   CONST UINT8  *KeyPassword,
  IN   UINT8        *InData,
  IN   UINTN        InDataSize,
  IN   UINT8        *SignCert,
  IN   UINT8        *OtherCerts      OPTIONAL,
  OUT  UINT8        **SignedData,
  OUT  UINTN        *SignedDataSize
  )
{
  return CALL_BASECRYPTLIB (Pkcs.Services.Pkcs7Sign, Pkcs7Sign, (PrivateKey, PrivateKeySize, KeyPassword, InData, InDataSize, SignCert, OtherCerts, SignedData, SignedDataSize), FALSE);
}

/**
  Verifies the validity of a PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard". The input signed data could be wrapped
  in a ContentInfo structure.

  If P7Data, TrustedCert or InData is NULL, then return FALSE.
  If P7Length, CertLength or DataLength overflow, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[in]  TrustedCert  Pointer to a trusted/root certificate encoded in DER, which
                           is used for certificate chain verification.
  @param[in]  CertLength   Length of the trusted certificate in bytes.
  @param[in]  InData       Pointer to the content to be verified.
  @param[in]  DataLength   Length of InData in bytes.

  @retval  TRUE  The specified PKCS#7 signed data is valid.
  @retval  FALSE Invalid PKCS#7 signed data.
  @retval  FALSE This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServicePkcs7Verify (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  IN  CONST UINT8  *TrustedCert,
  IN  UINTN        CertLength,
  IN  CONST UINT8  *InData,
  IN  UINTN        DataLength
  )
{
  return CALL_BASECRYPTLIB (Pkcs.Services.Pkcs7Verify, Pkcs7Verify, (P7Data, P7Length, TrustedCert, CertLength, InData, DataLength), FALSE);
}

/**
  This function receives a PKCS7 formatted signature, and then verifies that
  the specified Enhanced or Extended Key Usages (EKU's) are present in the end-entity
  leaf signing certificate.
  Note that this function does not validate the certificate chain.

  Applications for custom EKU's are quite flexible. For example, a policy EKU
  may be present in an Issuing Certificate Authority (CA), and any sub-ordinate
  certificate issued might also contain this EKU, thus constraining the
  sub-ordinate certificate.  Other applications might allow a certificate
  embedded in a device to specify that other Object Identifiers (OIDs) are
  present which contains binary data specifying custom capabilities that
  the device is able to do.

  @param[in]  Pkcs7Signature       The PKCS#7 signed information content block. An array
                                   containing the content block with both the signature,
                                   the signer's certificate, and any necessary intermediate
                                   certificates.
  @param[in]  Pkcs7SignatureSize   Number of bytes in Pkcs7Signature.
  @param[in]  RequiredEKUs         Array of null-terminated strings listing OIDs of
                                   required EKUs that must be present in the signature.
  @param[in]  RequiredEKUsSize     Number of elements in the RequiredEKUs string array.
  @param[in]  RequireAllPresent    If this is TRUE, then all of the specified EKU's
                                   must be present in the leaf signer.  If it is
                                   FALSE, then we will succeed if we find any
                                   of the specified EKU's.

  @retval EFI_SUCCESS              The required EKUs were found in the signature.
  @retval EFI_INVALID_PARAMETER    A parameter was invalid.
  @retval EFI_NOT_FOUND            One or more EKU's were not found in the signature.

**/
RETURN_STATUS
EFIAPI
CryptoServiceVerifyEKUsInPkcs7Signature (
  IN  CONST UINT8   *Pkcs7Signature,
  IN  CONST UINT32  SignatureSize,
  IN  CONST CHAR8   *RequiredEKUs[],
  IN  CONST UINT32  RequiredEKUsSize,
  IN  BOOLEAN       RequireAllPresent
  )
{
  return CALL_BASECRYPTLIB (Pkcs.Services.VerifyEKUsInPkcs7Signature, VerifyEKUsInPkcs7Signature, (Pkcs7Signature, SignatureSize, RequiredEKUs, RequiredEKUsSize, RequireAllPresent), FALSE);
}

/**
  Extracts the attached content from a PKCS#7 signed data if existed. The input signed
  data could be wrapped in a ContentInfo structure.

  If P7Data, Content, or ContentSize is NULL, then return FALSE. If P7Length overflow,
  then return FALSE. If the P7Data is not correctly formatted, then return FALSE.

  Caution: This function may receive untrusted input. So this function will do
           basic check for PKCS#7 data structure.

  @param[in]   P7Data       Pointer to the PKCS#7 signed data to process.
  @param[in]   P7Length     Length of the PKCS#7 signed data in bytes.
  @param[out]  Content      Pointer to the extracted content from the PKCS#7 signedData.
                            It's caller's responsibility to free the buffer with FreePool().
  @param[out]  ContentSize  The size of the extracted content in bytes.

  @retval     TRUE          The P7Data was correctly formatted for processing.
  @retval     FALSE         The P7Data was not correctly formatted for processing.

**/
BOOLEAN
EFIAPI
CryptoServicePkcs7GetAttachedContent (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT VOID         **Content,
  OUT UINTN        *ContentSize
  )
{
  return CALL_BASECRYPTLIB (Pkcs.Services.Pkcs7GetAttachedContent, Pkcs7GetAttachedContent, (P7Data, P7Length, Content, ContentSize), FALSE);
}

/**
  Verifies the validity of a PE/COFF Authenticode Signature as described in "Windows
  Authenticode Portable Executable Signature Format".

  If AuthData is NULL, then return FALSE.
  If ImageHash is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  AuthData     Pointer to the Authenticode Signature retrieved from signed
                           PE/COFF image to be verified.
  @param[in]  DataSize     Size of the Authenticode Signature in bytes.
  @param[in]  TrustedCert  Pointer to a trusted/root certificate encoded in DER, which
                           is used for certificate chain verification.
  @param[in]  CertSize     Size of the trusted certificate in bytes.
  @param[in]  ImageHash    Pointer to the original image file hash value. The procedure
                           for calculating the image hash value is described in Authenticode
                           specification.
  @param[in]  HashSize     Size of Image hash value in bytes.

  @retval  TRUE   The specified Authenticode Signature is valid.
  @retval  FALSE  Invalid Authenticode Signature.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceAuthenticodeVerify (
  IN  CONST UINT8  *AuthData,
  IN  UINTN        DataSize,
  IN  CONST UINT8  *TrustedCert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *ImageHash,
  IN  UINTN        HashSize
  )
{
  return CALL_BASECRYPTLIB (Pkcs.Services.AuthenticodeVerify, AuthenticodeVerify, (AuthData, DataSize, TrustedCert, CertSize, ImageHash, HashSize), FALSE);
}

/**
  Verifies the validity of a RFC3161 Timestamp CounterSignature embedded in PE/COFF Authenticode
  signature.

  If AuthData is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  AuthData     Pointer to the Authenticode Signature retrieved from signed
                           PE/COFF image to be verified.
  @param[in]  DataSize     Size of the Authenticode Signature in bytes.
  @param[in]  TsaCert      Pointer to a trusted/root TSA certificate encoded in DER, which
                           is used for TSA certificate chain verification.
  @param[in]  CertSize     Size of the trusted certificate in bytes.
  @param[out] SigningTime  Return the time of timestamp generation time if the timestamp
                           signature is valid.

  @retval  TRUE   The specified Authenticode includes a valid RFC3161 Timestamp CounterSignature.
  @retval  FALSE  No valid RFC3161 Timestamp CounterSignature in the specified Authenticode data.

**/
BOOLEAN
EFIAPI
CryptoServiceImageTimestampVerify (
  IN  CONST UINT8  *AuthData,
  IN  UINTN        DataSize,
  IN  CONST UINT8  *TsaCert,
  IN  UINTN        CertSize,
  OUT EFI_TIME     *SigningTime
  )
{
  return CALL_BASECRYPTLIB (Pkcs.Services.ImageTimestampVerify, ImageTimestampVerify, (AuthData, DataSize, TsaCert, CertSize, SigningTime), FALSE);
}

// =====================================================================================
//    DH Key Exchange Primitive
// =====================================================================================

/**
  Allocates and Initializes one Diffie-Hellman Context for subsequent use.

  @return  Pointer to the Diffie-Hellman Context that has been initialized.
           If the allocations fails, DhNew() returns NULL.
           If the interface is not supported, DhNew() returns NULL.

**/
VOID *
EFIAPI
CryptoServiceDhNew (
  VOID
  )
{
  return CALL_BASECRYPTLIB (Dh.Services.New, DhNew, (), NULL);
}

/**
  Release the specified DH context.

  If the interface is not supported, then ASSERT().

  @param[in]  DhContext  Pointer to the DH context to be released.

**/
VOID
EFIAPI
CryptoServiceDhFree (
  IN  VOID  *DhContext
  )
{
  CALL_VOID_BASECRYPTLIB (Dh.Services.Free, DhFree, (DhContext));
}

/**
  Generates DH parameter.

  Given generator g, and length of prime number p in bits, this function generates p,
  and sets DH context according to value of g and p.

  Before this function can be invoked, pseudorandom number generator must be correctly
  initialized by RandomSeed().

  If DhContext is NULL, then return FALSE.
  If Prime is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  DhContext    Pointer to the DH context.
  @param[in]       Generator    Value of generator.
  @param[in]       PrimeLength  Length in bits of prime to be generated.
  @param[out]      Prime        Pointer to the buffer to receive the generated prime number.

  @retval TRUE   DH parameter generation succeeded.
  @retval FALSE  Value of Generator is not supported.
  @retval FALSE  PRNG fails to generate random prime number with PrimeLength.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceDhGenerateParameter (
  IN OUT  VOID   *DhContext,
  IN      UINTN  Generator,
  IN      UINTN  PrimeLength,
  OUT     UINT8  *Prime
  )
{
  return CALL_BASECRYPTLIB (Dh.Services.GenerateParameter, DhGenerateParameter, (DhContext, Generator, PrimeLength, Prime), FALSE);
}

/**
  Sets generator and prime parameters for DH.

  Given generator g, and prime number p, this function and sets DH
  context accordingly.

  If DhContext is NULL, then return FALSE.
  If Prime is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  DhContext    Pointer to the DH context.
  @param[in]       Generator    Value of generator.
  @param[in]       PrimeLength  Length in bits of prime to be generated.
  @param[in]       Prime        Pointer to the prime number.

  @retval TRUE   DH parameter setting succeeded.
  @retval FALSE  Value of Generator is not supported.
  @retval FALSE  Value of Generator is not suitable for the Prime.
  @retval FALSE  Value of Prime is not a prime number.
  @retval FALSE  Value of Prime is not a safe prime number.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceDhSetParameter (
  IN OUT  VOID         *DhContext,
  IN      UINTN        Generator,
  IN      UINTN        PrimeLength,
  IN      CONST UINT8  *Prime
  )
{
  return CALL_BASECRYPTLIB (Dh.Services.SetParameter, DhSetParameter, (DhContext, Generator, PrimeLength, Prime), FALSE);
}

/**
  Generates DH public key.

  This function generates random secret exponent, and computes the public key, which is
  returned via parameter PublicKey and PublicKeySize. DH context is updated accordingly.
  If the PublicKey buffer is too small to hold the public key, FALSE is returned and
  PublicKeySize is set to the required buffer size to obtain the public key.

  If DhContext is NULL, then return FALSE.
  If PublicKeySize is NULL, then return FALSE.
  If PublicKeySize is large enough but PublicKey is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  DhContext      Pointer to the DH context.
  @param[out]      PublicKey      Pointer to the buffer to receive generated public key.
  @param[in, out]  PublicKeySize  On input, the size of PublicKey buffer in bytes.
                                 On output, the size of data returned in PublicKey buffer in bytes.

  @retval TRUE   DH public key generation succeeded.
  @retval FALSE  DH public key generation failed.
  @retval FALSE  PublicKeySize is not large enough.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceDhGenerateKey (
  IN OUT  VOID   *DhContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  )
{
  return CALL_BASECRYPTLIB (Dh.Services.GenerateKey, DhGenerateKey, (DhContext, PublicKey, PublicKeySize), FALSE);
}

/**
  Computes exchanged common key.

  Given peer's public key, this function computes the exchanged common key, based on its own
  context including value of prime modulus and random secret exponent.

  If DhContext is NULL, then return FALSE.
  If PeerPublicKey is NULL, then return FALSE.
  If KeySize is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeySize is not large enough, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  DhContext          Pointer to the DH context.
  @param[in]       PeerPublicKey      Pointer to the peer's public key.
  @param[in]       PeerPublicKeySize  Size of peer's public key in bytes.
  @param[out]      Key                Pointer to the buffer to receive generated key.
  @param[in, out]  KeySize            On input, the size of Key buffer in bytes.
                                     On output, the size of data returned in Key buffer in bytes.

  @retval TRUE   DH exchanged key generation succeeded.
  @retval FALSE  DH exchanged key generation failed.
  @retval FALSE  KeySize is not large enough.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceDhComputeKey (
  IN OUT  VOID         *DhContext,
  IN      CONST UINT8  *PeerPublicKey,
  IN      UINTN        PeerPublicKeySize,
  OUT     UINT8        *Key,
  IN OUT  UINTN        *KeySize
  )
{
  return CALL_BASECRYPTLIB (Dh.Services.ComputeKey, DhComputeKey, (DhContext, PeerPublicKey, PeerPublicKeySize, Key, KeySize), FALSE);
}

// =====================================================================================
//    Pseudo-Random Generation Primitive
// =====================================================================================

/**
  Sets up the seed value for the pseudorandom number generator.

  This function sets up the seed value for the pseudorandom number generator.
  If Seed is not NULL, then the seed passed in is used.
  If Seed is NULL, then default seed is used.
  If this interface is not supported, then return FALSE.

  @param[in]  Seed      Pointer to seed value.
                        If NULL, default seed is used.
  @param[in]  SeedSize  Size of seed value.
                        If Seed is NULL, this parameter is ignored.

  @retval TRUE   Pseudorandom number generator has enough entropy for random generation.
  @retval FALSE  Pseudorandom number generator does not have enough entropy for random generation.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceRandomSeed (
  IN  CONST  UINT8  *Seed  OPTIONAL,
  IN  UINTN         SeedSize
  )
{
  return CALL_BASECRYPTLIB (Random.Services.Seed, RandomSeed, (Seed, SeedSize), FALSE);
}

/**
  Generates a pseudorandom byte stream of the specified size.

  If Output is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  Output  Pointer to buffer to receive random value.
  @param[in]   Size    Size of random bytes to generate.

  @retval TRUE   Pseudorandom byte stream generated successfully.
  @retval FALSE  Pseudorandom number generator fails to generate due to lack of entropy.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceRandomBytes (
  OUT  UINT8  *Output,
  IN   UINTN  Size
  )
{
  return CALL_BASECRYPTLIB (Random.Services.Bytes, RandomBytes, (Output, Size), FALSE);
}

// =====================================================================================
//    Key Derivation Function Primitive
// =====================================================================================

/**
  Derive key data using HMAC-SHA256 based KDF.

  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.
  @param[in]   Salt             Pointer to the salt(non-secret) value.
  @param[in]   SaltSize         Salt size in bytes.
  @param[in]   Info             Pointer to the application specific info.
  @param[in]   InfoSize         Info size in bytes.
  @param[out]  Out              Pointer to buffer to receive hkdf value.
  @param[in]   OutSize          Size of hkdf bytes to generate.

  @retval TRUE   Hkdf generated successfully.
  @retval FALSE  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
CryptoServiceHkdfSha256ExtractAndExpand (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Salt,
  IN   UINTN        SaltSize,
  IN   CONST UINT8  *Info,
  IN   UINTN        InfoSize,
  OUT  UINT8        *Out,
  IN   UINTN        OutSize
  )
{
  return CALL_BASECRYPTLIB (Hkdf.Services.Sha256ExtractAndExpand, HkdfSha256ExtractAndExpand, (Key, KeySize, Salt, SaltSize, Info, InfoSize, Out, OutSize), FALSE);
}

/**
  Derive SHA256 HMAC-based Extract key Derivation Function (HKDF).

  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          key size in bytes.
  @param[in]   Salt             Pointer to the salt(non-secret) value.
  @param[in]   SaltSize         salt size in bytes.
  @param[out]  PrkOut           Pointer to buffer to receive hkdf value.
  @param[in]   PrkOutSize       size of hkdf bytes to generate.

  @retval true   Hkdf generated successfully.
  @retval false  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
CryptoServiceHkdfSha256Extract (
  IN CONST UINT8  *Key,
  IN UINTN        KeySize,
  IN CONST UINT8  *Salt,
  IN UINTN        SaltSize,
  OUT UINT8       *PrkOut,
  UINTN           PrkOutSize
  )
{
  return CALL_BASECRYPTLIB (Hkdf.Services.Sha256Extract, HkdfSha256Extract, (Key, KeySize, Salt, SaltSize, PrkOut, PrkOutSize), FALSE);
}

/**
  Derive SHA256 HMAC-based Expand Key Derivation Function (HKDF).

  @param[in]   Prk              Pointer to the user-supplied key.
  @param[in]   PrkSize          Key size in bytes.
  @param[in]   Info             Pointer to the application specific info.
  @param[in]   InfoSize         Info size in bytes.
  @param[out]  Out              Pointer to buffer to receive hkdf value.
  @param[in]   OutSize          Size of hkdf bytes to generate.

  @retval TRUE   Hkdf generated successfully.
  @retval FALSE  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
CryptoServiceHkdfSha256Expand (
  IN   CONST UINT8  *Prk,
  IN   UINTN        PrkSize,
  IN   CONST UINT8  *Info,
  IN   UINTN        InfoSize,
  OUT  UINT8        *Out,
  IN   UINTN        OutSize
  )
{
  return CALL_BASECRYPTLIB (Hkdf.Services.Sha256Expand, HkdfSha256Expand, (Prk, PrkSize, Info, InfoSize, Out, OutSize), FALSE);
}

/**
  Derive SHA384 HMAC-based Extract-and-Expand Key Derivation Function (HKDF).

  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.
  @param[in]   Salt             Pointer to the salt(non-secret) value.
  @param[in]   SaltSize         Salt size in bytes.
  @param[in]   Info             Pointer to the application specific info.
  @param[in]   InfoSize         Info size in bytes.
  @param[out]  Out              Pointer to buffer to receive hkdf value.
  @param[in]   OutSize          Size of hkdf bytes to generate.

  @retval TRUE   Hkdf generated successfully.
  @retval FALSE  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
CryptoServiceHkdfSha384ExtractAndExpand (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Salt,
  IN   UINTN        SaltSize,
  IN   CONST UINT8  *Info,
  IN   UINTN        InfoSize,
  OUT  UINT8        *Out,
  IN   UINTN        OutSize
  )
{
  return CALL_BASECRYPTLIB (Hkdf.Services.Sha384ExtractAndExpand, HkdfSha384ExtractAndExpand, (Key, KeySize, Salt, SaltSize, Info, InfoSize, Out, OutSize), FALSE);
}

/**
  Derive SHA384 HMAC-based Extract key Derivation Function (HKDF).

  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          key size in bytes.
  @param[in]   Salt             Pointer to the salt(non-secret) value.
  @param[in]   SaltSize         salt size in bytes.
  @param[out]  PrkOut           Pointer to buffer to receive hkdf value.
  @param[in]   PrkOutSize       size of hkdf bytes to generate.

  @retval true   Hkdf generated successfully.
  @retval false  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
CryptoServiceHkdfSha384Extract (
  IN CONST UINT8  *Key,
  IN UINTN        KeySize,
  IN CONST UINT8  *Salt,
  IN UINTN        SaltSize,
  OUT UINT8       *PrkOut,
  UINTN           PrkOutSize
  )
{
  return CALL_BASECRYPTLIB (Hkdf.Services.Sha384Extract, HkdfSha384Extract, (Key, KeySize, Salt, SaltSize, PrkOut, PrkOutSize), FALSE);
}

/**
  Derive SHA384 HMAC-based Expand Key Derivation Function (HKDF).

  @param[in]   Prk              Pointer to the user-supplied key.
  @param[in]   PrkSize          Key size in bytes.
  @param[in]   Info             Pointer to the application specific info.
  @param[in]   InfoSize         Info size in bytes.
  @param[out]  Out              Pointer to buffer to receive hkdf value.
  @param[in]   OutSize          Size of hkdf bytes to generate.

  @retval TRUE   Hkdf generated successfully.
  @retval FALSE  Hkdf generation failed.

**/
BOOLEAN
EFIAPI
CryptoServiceHkdfSha384Expand (
  IN   CONST UINT8  *Prk,
  IN   UINTN        PrkSize,
  IN   CONST UINT8  *Info,
  IN   UINTN        InfoSize,
  OUT  UINT8        *Out,
  IN   UINTN        OutSize
  )
{
  return CALL_BASECRYPTLIB (Hkdf.Services.Sha384Expand, HkdfSha384Expand, (Prk, PrkSize, Info, InfoSize, Out, OutSize), FALSE);
}

/**
  Initializes the OpenSSL library.

  This function registers ciphers and digests used directly and indirectly
  by SSL/TLS, and initializes the readable error messages.
  This function must be called before any other action takes places.

  @retval TRUE   The OpenSSL library has been initialized.
  @retval FALSE  Failed to initialize the OpenSSL library.

**/
BOOLEAN
EFIAPI
CryptoServiceTlsInitialize (
  VOID
  )
{
  return CALL_BASECRYPTLIB (Tls.Services.Initialize, TlsInitialize, (), FALSE);
}

/**
  Free an allocated SSL_CTX object.

  @param[in]  TlsCtx    Pointer to the SSL_CTX object to be released.

**/
VOID
EFIAPI
CryptoServiceTlsCtxFree (
  IN   VOID  *TlsCtx
  )
{
  CALL_VOID_BASECRYPTLIB (Tls.Services.CtxFree, TlsCtxFree, (TlsCtx));
}

/**
  Creates a new SSL_CTX object as framework to establish TLS/SSL enabled
  connections.

  @param[in]  MajorVer    Major Version of TLS/SSL Protocol.
  @param[in]  MinorVer    Minor Version of TLS/SSL Protocol.

  @return  Pointer to an allocated SSL_CTX object.
           If the creation failed, TlsCtxNew() returns NULL.

**/
VOID *
EFIAPI
CryptoServiceTlsCtxNew (
  IN     UINT8  MajorVer,
  IN     UINT8  MinorVer
  )
{
  return CALL_BASECRYPTLIB (Tls.Services.CtxNew, TlsCtxNew, (MajorVer, MinorVer), NULL);
}

/**
  Free an allocated TLS object.

  This function removes the TLS object pointed to by Tls and frees up the
  allocated memory. If Tls is NULL, nothing is done.

  @param[in]  Tls    Pointer to the TLS object to be freed.

**/
VOID
EFIAPI
CryptoServiceTlsFree (
  IN     VOID  *Tls
  )
{
  CALL_VOID_BASECRYPTLIB (Tls.Services.Free, TlsFree, (Tls));
}

/**
  Create a new TLS object for a connection.

  This function creates a new TLS object for a connection. The new object
  inherits the setting of the underlying context TlsCtx: connection method,
  options, verification setting.

  @param[in]  TlsCtx    Pointer to the SSL_CTX object.

  @return  Pointer to an allocated SSL object.
           If the creation failed, TlsNew() returns NULL.

**/
VOID *
EFIAPI
CryptoServiceTlsNew (
  IN     VOID  *TlsCtx
  )
{
  return CALL_BASECRYPTLIB (Tls.Services.New, TlsNew, (TlsCtx), NULL);
}

/**
  Checks if the TLS handshake was done.

  This function will check if the specified TLS handshake was done.

  @param[in]  Tls    Pointer to the TLS object for handshake state checking.

  @retval  TRUE     The TLS handshake was done.
  @retval  FALSE    The TLS handshake was not done.

**/
BOOLEAN
EFIAPI
CryptoServiceTlsInHandshake (
  IN     VOID  *Tls
  )
{
  return CALL_BASECRYPTLIB (Tls.Services.InHandshake, TlsInHandshake, (Tls), FALSE);
}

/**
  Perform a TLS/SSL handshake.

  This function will perform a TLS/SSL handshake.

  @param[in]       Tls            Pointer to the TLS object for handshake operation.
  @param[in]       BufferIn       Pointer to the most recently received TLS Handshake packet.
  @param[in]       BufferInSize   Packet size in bytes for the most recently received TLS
                                  Handshake packet.
  @param[out]      BufferOut      Pointer to the buffer to hold the built packet.
  @param[in, out]  BufferOutSize  Pointer to the buffer size in bytes. On input, it is
                                  the buffer size provided by the caller. On output, it
                                  is the buffer size in fact needed to contain the
                                  packet.

  @retval EFI_SUCCESS             The required TLS packet is built successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  Tls is NULL.
                                  BufferIn is NULL but BufferInSize is NOT 0.
                                  BufferInSize is 0 but BufferIn is NOT NULL.
                                  BufferOutSize is NULL.
                                  BufferOut is NULL if *BufferOutSize is not zero.
  @retval EFI_BUFFER_TOO_SMALL    BufferOutSize is too small to hold the response packet.
  @retval EFI_ABORTED             Something wrong during handshake.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsDoHandshake (
  IN     VOID   *Tls,
  IN     UINT8  *BufferIn  OPTIONAL,
  IN     UINTN  BufferInSize  OPTIONAL,
  OUT UINT8     *BufferOut  OPTIONAL,
  IN OUT UINTN  *BufferOutSize
  )
{
  return CALL_BASECRYPTLIB (Tls.Services.DoHandshake, TlsDoHandshake, (Tls, BufferIn, BufferInSize, BufferOut, BufferOutSize), EFI_UNSUPPORTED);
}

/**
  Handle Alert message recorded in BufferIn. If BufferIn is NULL and BufferInSize is zero,
  TLS session has errors and the response packet needs to be Alert message based on error type.

  @param[in]       Tls            Pointer to the TLS object for state checking.
  @param[in]       BufferIn       Pointer to the most recently received TLS Alert packet.
  @param[in]       BufferInSize   Packet size in bytes for the most recently received TLS
                                  Alert packet.
  @param[out]      BufferOut      Pointer to the buffer to hold the built packet.
  @param[in, out]  BufferOutSize  Pointer to the buffer size in bytes. On input, it is
                                  the buffer size provided by the caller. On output, it
                                  is the buffer size in fact needed to contain the
                                  packet.

  @retval EFI_SUCCESS             The required TLS packet is built successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  Tls is NULL.
                                  BufferIn is NULL but BufferInSize is NOT 0.
                                  BufferInSize is 0 but BufferIn is NOT NULL.
                                  BufferOutSize is NULL.
                                  BufferOut is NULL if *BufferOutSize is not zero.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_BUFFER_TOO_SMALL    BufferOutSize is too small to hold the response packet.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsHandleAlert (
  IN     VOID   *Tls,
  IN     UINT8  *BufferIn  OPTIONAL,
  IN     UINTN  BufferInSize  OPTIONAL,
  OUT UINT8     *BufferOut  OPTIONAL,
  IN OUT UINTN  *BufferOutSize
  )
{
  return CALL_BASECRYPTLIB (Tls.Services.HandleAlert, TlsHandleAlert, (Tls, BufferIn, BufferInSize, BufferOut, BufferOutSize), EFI_UNSUPPORTED);
}

/**
  Build the CloseNotify packet.

  @param[in]       Tls            Pointer to the TLS object for state checking.
  @param[in, out]  Buffer         Pointer to the buffer to hold the built packet.
  @param[in, out]  BufferSize     Pointer to the buffer size in bytes. On input, it is
                                  the buffer size provided by the caller. On output, it
                                  is the buffer size in fact needed to contain the
                                  packet.

  @retval EFI_SUCCESS             The required TLS packet is built successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  Tls is NULL.
                                  BufferSize is NULL.
                                  Buffer is NULL if *BufferSize is not zero.
  @retval EFI_BUFFER_TOO_SMALL    BufferSize is too small to hold the response packet.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsCloseNotify (
  IN     VOID   *Tls,
  IN OUT UINT8  *Buffer,
  IN OUT UINTN  *BufferSize
  )
{
  return CALL_BASECRYPTLIB (Tls.Services.CloseNotify, TlsCloseNotify, (Tls, Buffer, BufferSize), EFI_UNSUPPORTED);
}

/**
  Attempts to read bytes from one TLS object and places the data in Buffer.

  This function will attempt to read BufferSize bytes from the TLS object
  and places the data in Buffer.

  @param[in]      Tls           Pointer to the TLS object.
  @param[in,out]  Buffer        Pointer to the buffer to store the data.
  @param[in]      BufferSize    The size of Buffer in bytes.

  @retval  >0    The amount of data successfully read from the TLS object.
  @retval  <=0   No data was successfully read.

**/
INTN
EFIAPI
CryptoServiceTlsCtrlTrafficOut (
  IN     VOID   *Tls,
  IN OUT VOID   *Buffer,
  IN     UINTN  BufferSize
  )
{
  return CALL_BASECRYPTLIB (Tls.Services.CtrlTrafficOut, TlsCtrlTrafficOut, (Tls, Buffer, BufferSize), 0);
}

/**
  Attempts to write data from the buffer to TLS object.

  This function will attempt to write BufferSize bytes data from the Buffer
  to the TLS object.

  @param[in]  Tls           Pointer to the TLS object.
  @param[in]  Buffer        Pointer to the data buffer.
  @param[in]  BufferSize    The size of Buffer in bytes.

  @retval  >0    The amount of data successfully written to the TLS object.
  @retval <=0    No data was successfully written.

**/
INTN
EFIAPI
CryptoServiceTlsCtrlTrafficIn (
  IN     VOID   *Tls,
  IN     VOID   *Buffer,
  IN     UINTN  BufferSize
  )
{
  return CALL_BASECRYPTLIB (Tls.Services.CtrlTrafficIn, TlsCtrlTrafficIn, (Tls, Buffer, BufferSize), 0);
}

/**
  Attempts to read bytes from the specified TLS connection into the buffer.

  This function tries to read BufferSize bytes data from the specified TLS
  connection into the Buffer.

  @param[in]      Tls           Pointer to the TLS connection for data reading.
  @param[in,out]  Buffer        Pointer to the data buffer.
  @param[in]      BufferSize    The size of Buffer in bytes.

  @retval  >0    The read operation was successful, and return value is the
                 number of bytes actually read from the TLS connection.
  @retval  <=0   The read operation was not successful.

**/
INTN
EFIAPI
CryptoServiceTlsRead (
  IN     VOID   *Tls,
  IN OUT VOID   *Buffer,
  IN     UINTN  BufferSize
  )
{
  return CALL_BASECRYPTLIB (Tls.Services.Read, TlsRead, (Tls, Buffer, BufferSize), 0);
}

/**
  Attempts to write data to a TLS connection.

  This function tries to write BufferSize bytes data from the Buffer into the
  specified TLS connection.

  @param[in]  Tls           Pointer to the TLS connection for data writing.
  @param[in]  Buffer        Pointer to the data buffer.
  @param[in]  BufferSize    The size of Buffer in bytes.

  @retval  >0    The write operation was successful, and return value is the
                 number of bytes actually written to the TLS connection.
  @retval <=0    The write operation was not successful.

**/
INTN
EFIAPI
CryptoServiceTlsWrite (
  IN     VOID   *Tls,
  IN     VOID   *Buffer,
  IN     UINTN  BufferSize
  )
{
  return CALL_BASECRYPTLIB (Tls.Services.Write, TlsWrite, (Tls, Buffer, BufferSize), 0);
}

/**
  Shutdown a TLS connection.

  Shutdown the TLS connection without releasing the resources, meaning a new
  connection can be started without calling TlsNew() and without setting
  certificates etc.

  @param[in]       Tls            Pointer to the TLS object to shutdown.

  @retval EFI_SUCCESS             The TLS is shutdown successfully.
  @retval EFI_INVALID_PARAMETER   Tls is NULL.
  @retval EFI_PROTOCOL_ERROR      Some other error occurred.
**/
EFI_STATUS
EFIAPI
CryptoServiceTlsShutdown (
  IN     VOID  *Tls
  )
{
  return CALL_BASECRYPTLIB (Tls.Services.Shutdown, TlsShutdown, (Tls), EFI_UNSUPPORTED);
}

/**
  Set a new TLS/SSL method for a particular TLS object.

  This function sets a new TLS/SSL method for a particular TLS object.

  @param[in]  Tls         Pointer to a TLS object.
  @param[in]  MajorVer    Major Version of TLS/SSL Protocol.
  @param[in]  MinorVer    Minor Version of TLS/SSL Protocol.

  @retval  EFI_SUCCESS           The TLS/SSL method was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       Unsupported TLS/SSL method.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetVersion (
  IN     VOID   *Tls,
  IN     UINT8  MajorVer,
  IN     UINT8  MinorVer
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.Version, TlsSetVersion, (Tls, MajorVer, MinorVer), EFI_UNSUPPORTED);
}

/**
  Set TLS object to work in client or server mode.

  This function prepares a TLS object to work in client or server mode.

  @param[in]  Tls         Pointer to a TLS object.
  @param[in]  IsServer    Work in server mode.

  @retval  EFI_SUCCESS           The TLS/SSL work mode was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       Unsupported TLS/SSL work mode.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetConnectionEnd (
  IN     VOID     *Tls,
  IN     BOOLEAN  IsServer
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.ConnectionEnd, TlsSetConnectionEnd, (Tls, IsServer), EFI_UNSUPPORTED);
}

/**
  Set the ciphers list to be used by the TLS object.

  This function sets the ciphers for use by a specified TLS object.

  @param[in]  Tls          Pointer to a TLS object.
  @param[in]  CipherId     Array of UINT16 cipher identifiers. Each UINT16
                           cipher identifier comes from the TLS Cipher Suite
                           Registry of the IANA, interpreting Byte1 and Byte2
                           in network (big endian) byte order.
  @param[in]  CipherNum    The number of cipher in the list.

  @retval  EFI_SUCCESS           The ciphers list was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       No supported TLS cipher was found in CipherId.
  @retval  EFI_OUT_OF_RESOURCES  Memory allocation failed.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetCipherList (
  IN     VOID    *Tls,
  IN     UINT16  *CipherId,
  IN     UINTN   CipherNum
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.CipherList, TlsSetCipherList, (Tls, CipherId, CipherNum), EFI_UNSUPPORTED);
}

/**
  Set the compression method for TLS/SSL operations.

  This function handles TLS/SSL integrated compression methods.

  @param[in]  CompMethod    The compression method ID.

  @retval  EFI_SUCCESS        The compression method for the communication was
                              set successfully.
  @retval  EFI_UNSUPPORTED    Unsupported compression method.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetCompressionMethod (
  IN     UINT8  CompMethod
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.CompressionMethod, TlsSetCompressionMethod, (CompMethod), EFI_UNSUPPORTED);
}

/**
  Set peer certificate verification mode for the TLS connection.

  This function sets the verification mode flags for the TLS connection.

  @param[in]  Tls           Pointer to the TLS object.
  @param[in]  VerifyMode    A set of logically or'ed verification mode flags.

**/
VOID
EFIAPI
CryptoServiceTlsSetVerify (
  IN     VOID    *Tls,
  IN     UINT32  VerifyMode
  )
{
  CALL_VOID_BASECRYPTLIB (TlsSet.Services.Verify, TlsSetVerify, (Tls, VerifyMode));
}

/**
  Set the specified host name to be verified.

  @param[in]  Tls           Pointer to the TLS object.
  @param[in]  Flags         The setting flags during the validation.
  @param[in]  HostName      The specified host name to be verified.

  @retval  EFI_SUCCESS           The HostName setting was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_ABORTED           Invalid HostName setting.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetVerifyHost (
  IN     VOID    *Tls,
  IN     UINT32  Flags,
  IN     CHAR8   *HostName
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.VerifyHost, TlsSetVerifyHost, (Tls, Flags, HostName), EFI_UNSUPPORTED);
}

/**
  Sets a TLS/SSL session ID to be used during TLS/SSL connect.

  This function sets a session ID to be used when the TLS/SSL connection is
  to be established.

  @param[in]  Tls             Pointer to the TLS object.
  @param[in]  SessionId       Session ID data used for session resumption.
  @param[in]  SessionIdLen    Length of Session ID in bytes.

  @retval  EFI_SUCCESS           Session ID was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       No available session for ID setting.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetSessionId (
  IN     VOID    *Tls,
  IN     UINT8   *SessionId,
  IN     UINT16  SessionIdLen
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.SessionId, TlsSetSessionId, (Tls, SessionId, SessionIdLen), EFI_UNSUPPORTED);
}

/**
  Adds the CA to the cert store when requesting Server or Client authentication.

  This function adds the CA certificate to the list of CAs when requesting
  Server or Client authentication for the chosen TLS connection.

  @param[in]  Tls         Pointer to the TLS object.
  @param[in]  Data        Pointer to the data buffer of a DER-encoded binary
                          X.509 certificate or PEM-encoded X.509 certificate.
  @param[in]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_INVALID_PARAMETER   The parameter is invalid.
  @retval  EFI_OUT_OF_RESOURCES    Required resources could not be allocated.
  @retval  EFI_ABORTED             Invalid X.509 certificate.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetCaCertificate (
  IN     VOID   *Tls,
  IN     VOID   *Data,
  IN     UINTN  DataSize
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.CaCertificate, TlsSetCaCertificate, (Tls, Data, DataSize), EFI_UNSUPPORTED);
}

/**
  Loads the local public certificate into the specified TLS object.

  This function loads the X.509 certificate into the specified TLS object
  for TLS negotiation.

  @param[in]  Tls         Pointer to the TLS object.
  @param[in]  Data        Pointer to the data buffer of a DER-encoded binary
                          X.509 certificate or PEM-encoded X.509 certificate.
  @param[in]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_INVALID_PARAMETER   The parameter is invalid.
  @retval  EFI_OUT_OF_RESOURCES    Required resources could not be allocated.
  @retval  EFI_ABORTED             Invalid X.509 certificate.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetHostPublicCert (
  IN     VOID   *Tls,
  IN     VOID   *Data,
  IN     UINTN  DataSize
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.HostPublicCert, TlsSetHostPublicCert, (Tls, Data, DataSize), EFI_UNSUPPORTED);
}

/**
  Adds the local private key to the specified TLS object.

  This function adds the local private key (DER-encoded or PEM-encoded or PKCS#8 private
  key) into the specified TLS object for TLS negotiation.

  @param[in]  Tls         Pointer to the TLS object.
  @param[in]  Data        Pointer to the data buffer of a DER-encoded or PEM-encoded
                          or PKCS#8 private key.
  @param[in]  DataSize    The size of data buffer in bytes.
  @param[in]  Password    Pointer to NULL-terminated private key password, set it to NULL
                          if private key not encrypted.

  @retval  EFI_SUCCESS     The operation succeeded.
  @retval  EFI_UNSUPPORTED This function is not supported.
  @retval  EFI_ABORTED     Invalid private key data.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetHostPrivateKeyEx (
  IN     VOID   *Tls,
  IN     VOID   *Data,
  IN     UINTN  DataSize,
  IN     VOID   *Password  OPTIONAL
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.HostPrivateKeyEx, TlsSetHostPrivateKeyEx, (Tls, Data, DataSize, Password), EFI_UNSUPPORTED);
}

/**
  Adds the local private key to the specified TLS object.

  This function adds the local private key (DER-encoded or PEM-encoded or PKCS#8 private
  key) into the specified TLS object for TLS negotiation.

  @param[in]  Tls         Pointer to the TLS object.
  @param[in]  Data        Pointer to the data buffer of a DER-encoded or PEM-encoded
                          or PKCS#8 private key.
  @param[in]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS     The operation succeeded.
  @retval  EFI_UNSUPPORTED This function is not supported.
  @retval  EFI_ABORTED     Invalid private key data.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetHostPrivateKey (
  IN     VOID   *Tls,
  IN     VOID   *Data,
  IN     UINTN  DataSize
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.HostPrivateKey, TlsSetHostPrivateKey, (Tls, Data, DataSize), EFI_UNSUPPORTED);
}

/**
  Adds the CA-supplied certificate revocation list for certificate validation.

  This function adds the CA-supplied certificate revocation list data for
  certificate validity checking.

  @param[in]  Data        Pointer to the data buffer of a DER-encoded CRL data.
  @param[in]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS     The operation succeeded.
  @retval  EFI_UNSUPPORTED This function is not supported.
  @retval  EFI_ABORTED     Invalid CRL data.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetCertRevocationList (
  IN     VOID   *Data,
  IN     UINTN  DataSize
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.CertRevocationList, TlsSetCertRevocationList, (Data, DataSize), EFI_UNSUPPORTED);
}

/**
  Set the Tls security level.

  This function Set the Tls security level.
  If Tls is NULL, nothing is done.

  @param[in]  Tls                Pointer to the TLS object.
  @param[in]  Level              Pointer to the Tls Security level need to set.

  @retval  EFI_SUCCESS           The Tls security level was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameters are invalid.
  @retval  EFI_UNSUPPORTED       The requested TLS set security level is not supported.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetSecurityLevel (
  IN VOID   *Tls,
  IN UINT8  Level
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.SecurityLevel, TlsSetSecurityLevel, (Tls, Level), EFI_UNSUPPORTED);
}

/**
  Set the signature algorithm list to used by the TLS object.

  This function sets the signature algorithms for use by a specified TLS object.

  @param[in]  Tls                Pointer to a TLS object.
  @param[in]  Data               Array of UINT8 of signature algorithms. The array consists of
                                 pairs of the hash algorithm and the signature algorithm as defined
                                 in RFC 5246
  @param[in]  DataSize           The length the SignatureAlgoList. Must be divisible by 2.

  @retval  EFI_SUCCESS           The signature algorithm list was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameters are invalid.
  @retval  EFI_UNSUPPORTED       No supported TLS signature algorithm was found in SignatureAlgoList
  @retval  EFI_OUT_OF_RESOURCES  Memory allocation failed.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetSignatureAlgoList (
  IN     VOID   *Tls,
  IN     UINT8  *Data,
  IN     UINTN  DataSize
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.SignatureAlgoList, TlsSetSignatureAlgoList, (Tls, Data, DataSize), EFI_UNSUPPORTED);
}

/**
  Set the EC curve to be used for TLS flows

  This function sets the EC curve to be used for TLS flows.

  @param[in]  Tls                Pointer to a TLS object.
  @param[in]  Data               An EC named curve as defined in section 5.1.1 of RFC 4492.
  @param[in]  DataSize           Size of Data, it should be sizeof (UINT32)

  @retval  EFI_SUCCESS           The EC curve was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameters are invalid.
  @retval  EFI_UNSUPPORTED       The requested TLS EC curve is not supported

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsSetEcCurve (
  IN     VOID   *Tls,
  IN     UINT8  *Data,
  IN     UINTN  DataSize
  )
{
  return CALL_BASECRYPTLIB (TlsSet.Services.EcCurve, TlsSetEcCurve, (Tls, Data, DataSize), EFI_UNSUPPORTED);
}

/**
  Gets the protocol version used by the specified TLS connection.

  This function returns the protocol version used by the specified TLS
  connection.

  If Tls is NULL, then ASSERT().

  @param[in]  Tls    Pointer to the TLS object.

  @return  The protocol version of the specified TLS connection.

**/
UINT16
EFIAPI
CryptoServiceTlsGetVersion (
  IN     VOID  *Tls
  )
{
  return CALL_BASECRYPTLIB (TlsGet.Services.Version, TlsGetVersion, (Tls), 0);
}

/**
  Gets the connection end of the specified TLS connection.

  This function returns the connection end (as client or as server) used by
  the specified TLS connection.

  If Tls is NULL, then ASSERT().

  @param[in]  Tls    Pointer to the TLS object.

  @return  The connection end used by the specified TLS connection.

**/
UINT8
EFIAPI
CryptoServiceTlsGetConnectionEnd (
  IN     VOID  *Tls
  )
{
  return CALL_BASECRYPTLIB (TlsGet.Services.ConnectionEnd, TlsGetConnectionEnd, (Tls), 0);
}

/**
  Gets the cipher suite used by the specified TLS connection.

  This function returns current cipher suite used by the specified
  TLS connection.

  @param[in]      Tls         Pointer to the TLS object.
  @param[in,out]  CipherId    The cipher suite used by the TLS object.

  @retval  EFI_SUCCESS           The cipher suite was returned successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       Unsupported cipher suite.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsGetCurrentCipher (
  IN     VOID    *Tls,
  IN OUT UINT16  *CipherId
  )
{
  return CALL_BASECRYPTLIB (TlsGet.Services.CurrentCipher, TlsGetCurrentCipher, (Tls, CipherId), EFI_UNSUPPORTED);
}

/**
  Gets the compression methods used by the specified TLS connection.

  This function returns current integrated compression methods used by
  the specified TLS connection.

  @param[in]      Tls              Pointer to the TLS object.
  @param[in,out]  CompressionId    The current compression method used by
                                   the TLS object.

  @retval  EFI_SUCCESS           The compression method was returned successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_ABORTED           Invalid Compression method.
  @retval  EFI_UNSUPPORTED       This function is not supported.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsGetCurrentCompressionId (
  IN     VOID   *Tls,
  IN OUT UINT8  *CompressionId
  )
{
  return CALL_BASECRYPTLIB (TlsGet.Services.CurrentCompressionId, TlsGetCurrentCompressionId, (Tls, CompressionId), EFI_UNSUPPORTED);
}

/**
  Gets the verification mode currently set in the TLS connection.

  This function returns the peer verification mode currently set in the
  specified TLS connection.

  If Tls is NULL, then ASSERT().

  @param[in]  Tls    Pointer to the TLS object.

  @return  The verification mode set in the specified TLS connection.

**/
UINT32
EFIAPI
CryptoServiceTlsGetVerify (
  IN     VOID  *Tls
  )
{
  return CALL_BASECRYPTLIB (TlsGet.Services.Verify, TlsGetVerify, (Tls), 0);
}

/**
  Gets the session ID used by the specified TLS connection.

  This function returns the TLS/SSL session ID currently used by the
  specified TLS connection.

  @param[in]      Tls             Pointer to the TLS object.
  @param[in,out]  SessionId       Buffer to contain the returned session ID.
  @param[in,out]  SessionIdLen    The length of Session ID in bytes.

  @retval  EFI_SUCCESS           The Session ID was returned successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       Invalid TLS/SSL session.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsGetSessionId (
  IN     VOID    *Tls,
  IN OUT UINT8   *SessionId,
  IN OUT UINT16  *SessionIdLen
  )
{
  return CALL_BASECRYPTLIB (TlsGet.Services.SessionId, TlsGetSessionId, (Tls, SessionId, SessionIdLen), EFI_UNSUPPORTED);
}

/**
  Gets the client random data used in the specified TLS connection.

  This function returns the TLS/SSL client random data currently used in
  the specified TLS connection.

  @param[in]      Tls             Pointer to the TLS object.
  @param[in,out]  ClientRandom    Buffer to contain the returned client
                                  random data (32 bytes).

**/
VOID
EFIAPI
CryptoServiceTlsGetClientRandom (
  IN     VOID   *Tls,
  IN OUT UINT8  *ClientRandom
  )
{
  CALL_VOID_BASECRYPTLIB (TlsGet.Services.ClientRandom, TlsGetClientRandom, (Tls, ClientRandom));
}

/**
  Gets the server random data used in the specified TLS connection.

  This function returns the TLS/SSL server random data currently used in
  the specified TLS connection.

  @param[in]      Tls             Pointer to the TLS object.
  @param[in,out]  ServerRandom    Buffer to contain the returned server
                                  random data (32 bytes).

**/
VOID
EFIAPI
CryptoServiceTlsGetServerRandom (
  IN     VOID   *Tls,
  IN OUT UINT8  *ServerRandom
  )
{
  CALL_VOID_BASECRYPTLIB (TlsGet.Services.ServerRandom, TlsGetServerRandom, (Tls, ServerRandom));
}

/**
  Gets the master key data used in the specified TLS connection.

  This function returns the TLS/SSL master key material currently used in
  the specified TLS connection.

  @param[in]      Tls            Pointer to the TLS object.
  @param[in,out]  KeyMaterial    Buffer to contain the returned key material.

  @retval  EFI_SUCCESS           Key material was returned successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       Invalid TLS/SSL session.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsGetKeyMaterial (
  IN     VOID   *Tls,
  IN OUT UINT8  *KeyMaterial
  )
{
  return CALL_BASECRYPTLIB (TlsGet.Services.KeyMaterial, TlsGetKeyMaterial, (Tls, KeyMaterial), EFI_UNSUPPORTED);
}

/**
  Gets the CA Certificate from the cert store.

  This function returns the CA certificate for the chosen
  TLS connection.

  @param[in]      Tls         Pointer to the TLS object.
  @param[out]     Data        Pointer to the data buffer to receive the CA
                              certificate data sent to the client.
  @param[in,out]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_UNSUPPORTED         This function is not supported.
  @retval  EFI_BUFFER_TOO_SMALL    The Data is too small to hold the data.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsGetCaCertificate (
  IN     VOID   *Tls,
  OUT    VOID   *Data,
  IN OUT UINTN  *DataSize
  )
{
  return CALL_BASECRYPTLIB (TlsGet.Services.CaCertificate, TlsGetCaCertificate, (Tls, Data, DataSize), EFI_UNSUPPORTED);
}

/**
  Gets the local public Certificate set in the specified TLS object.

  This function returns the local public certificate which was currently set
  in the specified TLS object.

  @param[in]      Tls         Pointer to the TLS object.
  @param[out]     Data        Pointer to the data buffer to receive the local
                              public certificate.
  @param[in,out]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_INVALID_PARAMETER   The parameter is invalid.
  @retval  EFI_NOT_FOUND           The certificate is not found.
  @retval  EFI_BUFFER_TOO_SMALL    The Data is too small to hold the data.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsGetHostPublicCert (
  IN     VOID   *Tls,
  OUT    VOID   *Data,
  IN OUT UINTN  *DataSize
  )
{
  return CALL_BASECRYPTLIB (TlsGet.Services.HostPublicCert, TlsGetHostPublicCert, (Tls, Data, DataSize), EFI_UNSUPPORTED);
}

/**
  Gets the local private key set in the specified TLS object.

  This function returns the local private key data which was currently set
  in the specified TLS object.

  @param[in]      Tls         Pointer to the TLS object.
  @param[out]     Data        Pointer to the data buffer to receive the local
                              private key data.
  @param[in,out]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_UNSUPPORTED         This function is not supported.
  @retval  EFI_BUFFER_TOO_SMALL    The Data is too small to hold the data.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsGetHostPrivateKey (
  IN     VOID   *Tls,
  OUT    VOID   *Data,
  IN OUT UINTN  *DataSize
  )
{
  return CALL_BASECRYPTLIB (TlsGet.Services.HostPrivateKey, TlsGetHostPrivateKey, (Tls, Data, DataSize), EFI_UNSUPPORTED);
}

/**
  Gets the CA-supplied certificate revocation list data set in the specified
  TLS object.

  This function returns the CA-supplied certificate revocation list data which
  was currently set in the specified TLS object.

  @param[out]     Data        Pointer to the data buffer to receive the CRL data.
  @param[in,out]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_UNSUPPORTED         This function is not supported.
  @retval  EFI_BUFFER_TOO_SMALL    The Data is too small to hold the data.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsGetCertRevocationList (
  OUT    VOID   *Data,
  IN OUT UINTN  *DataSize
  )
{
  return CALL_BASECRYPTLIB (TlsGet.Services.CertRevocationList, TlsGetCertRevocationList, (Data, DataSize), EFI_UNSUPPORTED);
}

/**
  Derive keying material from a TLS connection.

  This function exports keying material using the mechanism described in RFC
  5705.

  @param[in]      Tls          Pointer to the TLS object
  @param[in]      Label        Description of the key for the PRF function
  @param[in]      Context      Optional context
  @param[in]      ContextLen   The length of the context value in bytes
  @param[out]     KeyBuffer    Buffer to hold the output of the TLS-PRF
  @param[in]      KeyBufferLen The length of the KeyBuffer

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_INVALID_PARAMETER   The TLS object is invalid.
  @retval  EFI_PROTOCOL_ERROR      Some other error occurred.

**/
EFI_STATUS
EFIAPI
CryptoServiceTlsGetExportKey (
  IN     VOID        *Tls,
  IN     CONST VOID  *Label,
  IN     CONST VOID  *Context,
  IN     UINTN       ContextLen,
  OUT    VOID        *KeyBuffer,
  IN     UINTN       KeyBufferLen
  )
{
  return CALL_BASECRYPTLIB (
           TlsGet.Services.ExportKey,
           TlsGetExportKey,
           (Tls, Label, Context, ContextLen,
            KeyBuffer, KeyBufferLen),
           EFI_UNSUPPORTED
           );
}

/**
  Carries out the RSA-SSA signature generation with EMSA-PSS encoding scheme.

  This function carries out the RSA-SSA signature generation with EMSA-PSS encoding scheme defined in
  RFC 8017.
  Mask generation function is the same as the message digest algorithm.
  If the Signature buffer is too small to hold the contents of signature, FALSE
  is returned and SigSize is set to the required buffer size to obtain the signature.

  If RsaContext is NULL, then return FALSE.
  If Message is NULL, then return FALSE.
  If MsgSize is zero or > INT_MAX, then return FALSE.
  If DigestLen is NOT 32, 48 or 64, return FALSE.
  If SaltLen is not equal to DigestLen, then return FALSE.
  If SigSize is large enough but Signature is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      RsaContext   Pointer to RSA context for signature generation.
  @param[in]      Message      Pointer to octet message to be signed.
  @param[in]      MsgSize      Size of the message in bytes.
  @param[in]      DigestLen    Length of the digest in bytes to be used for RSA signature operation.
  @param[in]      SaltLen      Length of the salt in bytes to be used for PSS encoding.
  @param[out]     Signature    Pointer to buffer to receive RSA PSS signature.
  @param[in, out] SigSize      On input, the size of Signature buffer in bytes.
                               On output, the size of data returned in Signature buffer in bytes.

  @retval  TRUE   Signature successfully generated in RSASSA-PSS.
  @retval  FALSE  Signature generation failed.
  @retval  FALSE  SigSize is too small.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceRsaPssSign (
  IN      VOID         *RsaContext,
  IN      CONST UINT8  *Message,
  IN      UINTN        MsgSize,
  IN      UINT16       DigestLen,
  IN      UINT16       SaltLen,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  )
{
  return CALL_BASECRYPTLIB (RsaPss.Services.Sign, RsaPssSign, (RsaContext, Message, MsgSize, DigestLen, SaltLen, Signature, SigSize), FALSE);
}

/**
  Verifies the RSA signature with RSASSA-PSS signature scheme defined in RFC 8017.
  Implementation determines salt length automatically from the signature encoding.
  Mask generation function is the same as the message digest algorithm.
  Salt length should be equal to digest length.

  @param[in]  RsaContext      Pointer to RSA context for signature verification.
  @param[in]  Message         Pointer to octet message to be verified.
  @param[in]  MsgSize         Size of the message in bytes.
  @param[in]  Signature       Pointer to RSASSA-PSS signature to be verified.
  @param[in]  SigSize         Size of signature in bytes.
  @param[in]  DigestLen       Length of digest for RSA operation.
  @param[in]  SaltLen         Salt length for PSS encoding.

  @retval  TRUE   Valid signature encoded in RSASSA-PSS.
  @retval  FALSE  Invalid signature or invalid RSA context.

**/
BOOLEAN
EFIAPI
CryptoServiceRsaPssVerify (
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *Message,
  IN  UINTN        MsgSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize,
  IN  UINT16       DigestLen,
  IN  UINT16       SaltLen
  )
{
  return CALL_BASECRYPTLIB (RsaPss.Services.Verify, RsaPssVerify, (RsaContext, Message, MsgSize, Signature, SigSize, DigestLen, SaltLen), FALSE);
}

/**
  Parallel hash function ParallelHash256, as defined in NIST's Special Publication 800-185,
  published December 2016.

  @param[in]   Input            Pointer to the input message (X).
  @param[in]   InputByteLen     The number(>0) of input bytes provided for the input data.
  @param[in]   BlockSize        The size of each block (B).
  @param[out]  Output           Pointer to the output buffer.
  @param[in]   OutputByteLen    The desired number of output bytes (L).
  @param[in]   Customization    Pointer to the customization string (S).
  @param[in]   CustomByteLen    The length of the customization string in bytes.

  @retval TRUE   ParallelHash256 digest computation succeeded.
  @retval FALSE  ParallelHash256 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CryptoServiceParallelHash256HashAll (
  IN CONST VOID   *Input,
  IN       UINTN  InputByteLen,
  IN       UINTN  BlockSize,
  OUT      VOID   *Output,
  IN       UINTN  OutputByteLen,
  IN CONST VOID   *Customization,
  IN       UINTN  CustomByteLen
  )
{
  return CALL_BASECRYPTLIB (ParallelHash.Services.HashAll, ParallelHash256HashAll, (Input, InputByteLen, BlockSize, Output, OutputByteLen, Customization, CustomByteLen), FALSE);
}

/**
  Performs AEAD AES-GCM authenticated encryption on a data buffer and additional authenticated data (AAD).

  IvSize must be 12, otherwise FALSE is returned.
  KeySize must be 16, 24 or 32, otherwise FALSE is returned.
  TagSize must be 12, 13, 14, 15, 16, otherwise FALSE is returned.

  @param[in]   Key         Pointer to the encryption key.
  @param[in]   KeySize     Size of the encryption key in bytes.
  @param[in]   Iv          Pointer to the IV value.
  @param[in]   IvSize      Size of the IV value in bytes.
  @param[in]   AData       Pointer to the additional authenticated data (AAD).
  @param[in]   ADataSize   Size of the additional authenticated data (AAD) in bytes.
  @param[in]   DataIn      Pointer to the input data buffer to be encrypted.
  @param[in]   DataInSize  Size of the input data buffer in bytes.
  @param[out]  TagOut      Pointer to a buffer that receives the authentication tag output.
  @param[in]   TagSize     Size of the authentication tag in bytes.
  @param[out]  DataOut     Pointer to a buffer that receives the encryption output.
  @param[out]  DataOutSize Size of the output data buffer in bytes.

  @retval TRUE   AEAD AES-GCM authenticated encryption succeeded.
  @retval FALSE  AEAD AES-GCM authenticated encryption failed.

**/
BOOLEAN
EFIAPI
CryptoServiceAeadAesGcmEncrypt (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Iv,
  IN   UINTN        IvSize,
  IN   CONST UINT8  *AData,
  IN   UINTN        ADataSize,
  IN   CONST UINT8  *DataIn,
  IN   UINTN        DataInSize,
  OUT  UINT8        *TagOut,
  IN   UINTN        TagSize,
  OUT  UINT8        *DataOut,
  OUT  UINTN        *DataOutSize
  )
{
  return CALL_BASECRYPTLIB (AeadAesGcm.Services.Encrypt, AeadAesGcmEncrypt, (Key, KeySize, Iv, IvSize, AData, ADataSize, DataIn, DataInSize, TagOut, TagSize, DataOut, DataOutSize), FALSE);
}

/**
  Performs AEAD AES-GCM authenticated decryption on a data buffer and additional authenticated data (AAD).

  IvSize must be 12, otherwise FALSE is returned.
  KeySize must be 16, 24 or 32, otherwise FALSE is returned.
  TagSize must be 12, 13, 14, 15, 16, otherwise FALSE is returned.
  If additional authenticated data verification fails, FALSE is returned.

  @param[in]   Key         Pointer to the encryption key.
  @param[in]   KeySize     Size of the encryption key in bytes.
  @param[in]   Iv          Pointer to the IV value.
  @param[in]   IvSize      Size of the IV value in bytes.
  @param[in]   AData       Pointer to the additional authenticated data (AAD).
  @param[in]   ADataSize   Size of the additional authenticated data (AAD) in bytes.
  @param[in]   DataIn      Pointer to the input data buffer to be decrypted.
  @param[in]   DataInSize  Size of the input data buffer in bytes.
  @param[in]   Tag         Pointer to a buffer that contains the authentication tag.
  @param[in]   TagSize     Size of the authentication tag in bytes.
  @param[out]  DataOut     Pointer to a buffer that receives the decryption output.
  @param[out]  DataOutSize Size of the output data buffer in bytes.

  @retval TRUE   AEAD AES-GCM authenticated decryption succeeded.
  @retval FALSE  AEAD AES-GCM authenticated decryption failed.

**/
BOOLEAN
EFIAPI
CryptoServiceAeadAesGcmDecrypt (
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Iv,
  IN   UINTN        IvSize,
  IN   CONST UINT8  *AData,
  IN   UINTN        ADataSize,
  IN   CONST UINT8  *DataIn,
  IN   UINTN        DataInSize,
  IN   CONST UINT8  *Tag,
  IN   UINTN        TagSize,
  OUT  UINT8        *DataOut,
  OUT  UINTN        *DataOutSize
  )
{
  return CALL_BASECRYPTLIB (AeadAesGcm.Services.Decrypt, AeadAesGcmDecrypt, (Key, KeySize, Iv, IvSize, AData, ADataSize, DataIn, DataInSize, Tag, TagSize, DataOut, DataOutSize), FALSE);
}

// =====================================================================================
//    Big number primitives
// =====================================================================================

/**
  Allocate new Big Number.

  @retval New BigNum opaque structure or NULL on failure.
**/
VOID *
EFIAPI
CryptoServiceBigNumInit (
  VOID
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.Init, BigNumInit, (), NULL);
}

/**
  Allocate new Big Number and assign the provided value to it.

  @param[in]   Buf    Big endian encoded buffer.
  @param[in]   Len    Buffer length.

  @retval New BigNum opaque structure or NULL on failure.
**/
VOID *
EFIAPI
CryptoServiceBigNumFromBin (
  IN CONST UINT8  *Buf,
  IN UINTN        Len
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.FromBin, BigNumFromBin, (Buf, Len), NULL);
}

/**
  Convert the absolute value of Bn into big-endian form and store it at Buf.
  The Buf array should have at least BigNumBytes() in it.

  @param[in]   Bn     Big number to convert.
  @param[out]  Buf    Output buffer.

  @retval The length of the big-endian number placed at Buf or -1 on error.
**/
INTN
EFIAPI
CryptoServiceBigNumToBin (
  IN CONST VOID  *Bn,
  OUT UINT8      *Buf
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.ToBin, BigNumToBin, (Bn, Buf), -1);
}

/**
  Free the Big Number.

  @param[in]   Bn      Big number to free.
  @param[in]   Clear   TRUE if the buffer should be cleared.
**/
VOID
EFIAPI
CryptoServiceBigNumFree (
  IN VOID     *Bn,
  IN BOOLEAN  Clear
  )
{
  CALL_VOID_BASECRYPTLIB (Bn.Services.Free, BigNumFree, (Bn, Clear));
}

/**
  Calculate the sum of two Big Numbers.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[out]  BnRes   The result of BnA + BnB.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceBigNumAdd (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB,
  OUT VOID       *BnRes
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.Add, BigNumAdd, (BnA, BnB, BnRes), FALSE);
}

/**
  Subtract two Big Numbers.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[out]  BnRes   The result of BnA - BnB.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceBigNumSub (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB,
  OUT VOID       *BnRes
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.Sub, BigNumSub, (BnA, BnB, BnRes), FALSE);
}

/**
  Calculate remainder: BnRes = BnA % BnB.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[out]  BnRes   The result of BnA % BnB.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceBigNumMod (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB,
  OUT VOID       *BnRes
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.Mod, BigNumMod, (BnA, BnB, BnRes), FALSE);
}

/**
  Compute BnA to the BnP-th power modulo BnM.
  Please note, all "out" Big number arguments should be properly initialized.
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnP     Big number (power).
  @param[in]   BnM     Big number (modulo).
  @param[out]  BnRes   The result of (BnA ^ BnP) % BnM.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceBigNumExpMod (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnP,
  IN CONST VOID  *BnM,
  OUT VOID       *BnRes
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.ExpMod, BigNumExpMod, (BnA, BnP, BnM, BnRes), FALSE);
}

/**
  Compute BnA inverse modulo BnM.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnM     Big number (modulo).
  @param[out]  BnRes   The result, such that (BnA * BnRes) % BnM == 1.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceBigNumInverseMod (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnM,
  OUT VOID       *BnRes
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.InverseMod, BigNumInverseMod, (BnA, BnM, BnRes), FALSE);
}

/**
  Divide two Big Numbers.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[out]  BnRes   The result, such that BnA / BnB.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceBigNumDiv (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB,
  OUT VOID       *BnRes
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.Div, BigNumDiv, (BnA, BnB, BnRes), FALSE);
}

/**
  Multiply two Big Numbers modulo BnM.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[in]   BnM     Big number (modulo).
  @param[out]  BnRes   The result, such that (BnA * BnB) % BnM.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceBigNumMulMod (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB,
  IN CONST VOID  *BnM,
  OUT VOID       *BnRes
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.MulMod, BigNumMulMod, (BnA, BnB, BnM, BnRes), FALSE);
}

/**
  Compare two Big Numbers.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.

  @retval 0          BnA == BnB.
  @retval 1          BnA > BnB.
  @retval -1         BnA < BnB.
**/
INTN
EFIAPI
CryptoServiceBigNumCmp (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.Cmp, BigNumCmp, (BnA, BnB), 0);
}

/**
  Get number of bits in Bn.

  @param[in]   Bn     Big number.

  @retval Number of bits.
**/
UINTN
EFIAPI
CryptoServiceBigNumBits (
  IN CONST VOID  *Bn
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.Bits, BigNumBits, (Bn), 0);
}

/**
  Get number of bytes in Bn.

  @param[in]   Bn     Big number.

  @retval Number of bytes.
**/
UINTN
EFIAPI
CryptoServiceBigNumBytes (
  IN CONST VOID  *Bn
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.Bytes, BigNumBytes, (Bn), 0);
}

/**
  Checks if Big Number equals to the given Num.

  @param[in]   Bn     Big number.
  @param[in]   Num    Number.

  @retval TRUE   iff Bn == Num.
  @retval FALSE  otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceBigNumIsWord (
  IN CONST VOID  *Bn,
  IN UINTN       Num
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.IsWord, BigNumIsWord, (Bn, Num), FALSE);
}

/**
  Checks if Big Number is odd.

  @param[in]   Bn     Big number.

  @retval TRUE   Bn is odd (Bn % 2 == 1).
  @retval FALSE  otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceBigNumIsOdd (
  IN CONST VOID  *Bn
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.IsOdd, BigNumIsOdd, (Bn), FALSE);
}

/**
  Copy Big number.

  @param[out]  BnDst     Destination.
  @param[in]   BnSrc     Source.

  @retval BnDst on success.
  @retval NULL otherwise.
**/
VOID *
EFIAPI
CryptoServiceBigNumCopy (
  OUT VOID       *BnDst,
  IN CONST VOID  *BnSrc
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.Copy, BigNumCopy, (BnDst, BnSrc), NULL);
}

/**
  Get constant Big number with value of "1".
  This may be used to save expensive allocations.

  @retval Big Number with value of 1.
**/
CONST VOID *
EFIAPI
CryptoServiceBigNumValueOne (
  VOID
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.ValueOne, BigNumValueOne, (), NULL);
}

/**
  Shift right Big Number.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   Bn      Big number.
  @param[in]   N       Number of bits to shift.
  @param[out]  BnRes   The result.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceBigNumRShift (
  IN CONST VOID  *Bn,
  IN UINTN       N,
  OUT VOID       *BnRes
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.RShift, BigNumRShift, (Bn, N, BnRes), FALSE);
}

/**
  Mark Big Number for constant time computations.
  This function should be called before any constant time computations are
  performed on the given Big number.

  @param[in]   Bn     Big number.
**/
VOID
EFIAPI
CryptoServiceBigNumConstTime (
  IN VOID  *Bn
  )
{
  CALL_VOID_BASECRYPTLIB (Bn.Services.ConstTime, BigNumConstTime, (Bn));
}

/**
  Calculate square modulo.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnM     Big number (modulo).
  @param[out]  BnRes   The result, such that (BnA ^ 2) % BnM.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceBigNumSqrMod (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnM,
  OUT VOID       *BnRes
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.SqrMod, BigNumSqrMod, (BnA, BnM, BnRes), FALSE);
}

/**
  Create new Big Number computation context. This is an opaque structure
  which should be passed to any function that requires it. The BN context is
  needed to optimize calculations and expensive allocations.

  @retval Big Number context struct or NULL on failure.
**/
VOID *
EFIAPI
CryptoServiceBigNumNewContext (
  VOID
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.NewContext, BigNumNewContext, (), NULL);
}

/**
  Free Big Number context that was allocated with BigNumNewContext().

  @param[in]   BnCtx     Big number context to free.
**/
VOID
EFIAPI
CryptoServiceBigNumContextFree (
  IN VOID  *BnCtx
  )
{
  CALL_VOID_BASECRYPTLIB (Bn.Services.ContextFree, BigNumContextFree, (BnCtx));
}

/**
  Set Big Number to a given value.

  @param[in]   Bn     Big number to set.
  @param[in]   Val    Value to set.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceBigNumSetUint (
  IN VOID   *Bn,
  IN UINTN  Val
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.SetUint, BigNumSetUint, (Bn, Val), FALSE);
}

/**
  Add two Big Numbers modulo BnM.

  @param[in]   BnA       Big number.
  @param[in]   BnB       Big number.
  @param[in]   BnM       Big number (modulo).
  @param[out]  BnRes     The result, such that (BnA + BnB) % BnM.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceBigNumAddMod (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB,
  IN CONST VOID  *BnM,
  OUT VOID       *BnRes
  )
{
  return CALL_BASECRYPTLIB (Bn.Services.AddMod, BigNumAddMod, (BnA, BnB, BnM, BnRes), FALSE);
}

// =====================================================================================
//    Basic Elliptic Curve Primitives
// =====================================================================================

/**
  Initialize new opaque EcGroup object. This object represents an EC curve and
  and is used for calculation within this group. This object should be freed
  using EcGroupFree() function.

  @param[in]  CryptoNid   Identifying number for the ECC curve (Defined in
                          BaseCryptLib.h).

  @retval EcGroup object  On success.
  @retval NULL            On failure.
**/
VOID *
EFIAPI
CryptoServiceEcGroupInit (
  IN UINTN  CryptoNid
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.GroupInit, EcGroupInit, (CryptoNid), NULL);
}

/**
  Get EC curve parameters. While elliptic curve equation is Y^2 mod P = (X^3 + AX + B) Mod P.
  This function will set the provided Big Number objects  to the corresponding
  values. The caller needs to make sure all the "out" BigNumber parameters
  are properly initialized.
  @param[in]  EcGroup    EC group object.
  @param[out] BnPrime    Group prime number.
  @param[out] BnA        A coefficient.
  @param[out] BnB        B coefficient.
  @param[in]  BnCtx      BN context.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceEcGroupGetCurve (
  IN CONST VOID  *EcGroup,
  OUT VOID       *BnPrime,
  OUT VOID       *BnA,
  OUT VOID       *BnB,
  IN VOID        *BnCtx
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.GroupGetCurve, EcGroupGetCurve, (EcGroup, BnPrime, BnA, BnB, BnCtx), FALSE);
}

/**
  Get EC group order.
  This function will set the provided Big Number object to the corresponding
  value. The caller needs to make sure that the "out" BigNumber parameter
  is properly initialized.

  @param[in]  EcGroup   EC group object.
  @param[out] BnOrder   Group prime number.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceEcGroupGetOrder (
  IN VOID   *EcGroup,
  OUT VOID  *BnOrder
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.GroupGetOrder, EcGroupGetOrder, (EcGroup, BnOrder), FALSE);
}

/**
  Free previously allocated EC group object using EcGroupInit().

  @param[in]  EcGroup   EC group object to free.
**/
VOID
EFIAPI
CryptoServiceEcGroupFree (
  IN VOID  *EcGroup
  )
{
  CALL_VOID_BASECRYPTLIB (Ec.Services.GroupFree, EcGroupFree, (EcGroup));
}

/**
  Initialize new opaque EC Point object. This object represents an EC point
  within the given EC group (curve).

  @param[in]  EC Group, properly initialized using EcGroupInit().

  @retval EC Point object  On success.
  @retval NULL             On failure.
**/
VOID *
EFIAPI
CryptoServiceEcPointInit (
  IN CONST VOID  *EcGroup
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.PointInit, EcPointInit, (EcGroup), NULL);
}

/**
  Free previously allocated EC Point object using EcPointInit().

  @param[in]  EcPoint   EC Point to free.
  @param[in]  Clear     TRUE iff the memory should be cleared.
**/
VOID
EFIAPI
CryptoServiceEcPointDeInit (
  IN VOID     *EcPoint,
  IN BOOLEAN  Clear
  )
{
  CALL_VOID_BASECRYPTLIB (Ec.Services.PointDeInit, EcPointDeInit, (EcPoint, Clear));
}

/**
  Get EC point affine (x,y) coordinates.
  This function will set the provided Big Number objects to the corresponding
  values. The caller needs to make sure all the "out" BigNumber parameters
  are properly initialized.

  @param[in]  EcGroup    EC group object.
  @param[in]  EcPoint    EC point object.
  @param[out] BnX        X coordinate.
  @param[out] BnY        Y coordinate.
  @param[in]  BnCtx      BN context, created with BigNumNewContext().

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceEcPointGetAffineCoordinates (
  IN CONST VOID  *EcGroup,
  IN CONST VOID  *EcPoint,
  OUT VOID       *BnX,
  OUT VOID       *BnY,
  IN VOID        *BnCtx
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.PointGetAffineCoordinates, EcPointGetAffineCoordinates, (EcGroup, EcPoint, BnX, BnY, BnCtx), FALSE);
}

/**
  Set EC point affine (x,y) coordinates.

  @param[in]  EcGroup    EC group object.
  @param[in]  EcPoint    EC point object.
  @param[in]  BnX        X coordinate.
  @param[in]  BnY        Y coordinate.
  @param[in]  BnCtx      BN context, created with BigNumNewContext().

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceEcPointSetAffineCoordinates (
  IN CONST VOID  *EcGroup,
  IN VOID        *EcPoint,
  IN CONST VOID  *BnX,
  IN CONST VOID  *BnY,
  IN VOID        *BnCtx
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.PointSetAffineCoordinates, EcPointSetAffineCoordinates, (EcGroup, EcPoint, BnX, BnY, BnCtx), FALSE);
}

/**
  EC Point addition. EcPointResult = EcPointA + EcPointB.
  @param[in]  EcGroup          EC group object.
  @param[out] EcPointResult    EC point to hold the result. The point should
                               be properly initialized.
  @param[in]  EcPointA         EC Point.
  @param[in]  EcPointB         EC Point.
  @param[in]  BnCtx            BN context, created with BigNumNewContext().

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceEcPointAdd (
  IN CONST VOID  *EcGroup,
  OUT VOID       *EcPointResult,
  IN CONST VOID  *EcPointA,
  IN CONST VOID  *EcPointB,
  IN VOID        *BnCtx
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.PointAdd, EcPointAdd, (EcGroup, EcPointResult, EcPointA, EcPointB, BnCtx), FALSE);
}

/**
  Variable EC point multiplication. EcPointResult = EcPoint * BnPScalar.

  @param[in]  EcGroup          EC group object.
  @param[out] EcPointResult    EC point to hold the result. The point should
                               be properly initialized.
  @param[in]  EcPoint          EC Point.
  @param[in]  BnPScalar        P Scalar.
  @param[in]  BnCtx            BN context, created with BigNumNewContext().

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceEcPointMul (
  IN CONST VOID  *EcGroup,
  OUT VOID       *EcPointResult,
  IN CONST VOID  *EcPoint,
  IN CONST VOID  *BnPScalar,
  IN VOID        *BnCtx
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.PointMul, EcPointMul, (EcGroup, EcPointResult, EcPoint, BnPScalar, BnCtx), FALSE);
}

/**
  Calculate the inverse of the supplied EC point.

  @param[in]     EcGroup   EC group object.
  @param[in,out] EcPoint   EC point to invert.
  @param[in]     BnCtx     BN context, created with BigNumNewContext().

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceEcPointInvert (
  IN CONST VOID  *EcGroup,
  IN OUT VOID    *EcPoint,
  IN VOID        *BnCtx
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.PointInvert, EcPointInvert, (EcGroup, EcPoint, BnCtx), FALSE);
}

/**
  Check if the supplied point is on EC curve.

  @param[in]  EcGroup   EC group object.
  @param[in]  EcPoint   EC point to check.
  @param[in]  BnCtx     BN context, created with BigNumNewContext().

  @retval TRUE          On curve.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceEcPointIsOnCurve (
  IN CONST VOID  *EcGroup,
  IN CONST VOID  *EcPoint,
  IN VOID        *BnCtx
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.PointIsOnCurve, EcPointIsOnCurve, (EcGroup, EcPoint, BnCtx), FALSE);
}

/**
  Check if the supplied point is at infinity.

  @param[in]  EcGroup   EC group object.
  @param[in]  EcPoint   EC point to check.

  @retval TRUE          At infinity.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceEcPointIsAtInfinity (
  IN CONST VOID  *EcGroup,
  IN CONST VOID  *EcPoint
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.PointIsAtInfinity, EcPointIsAtInfinity, (EcGroup, EcPoint), FALSE);
}

/**
  Check if EC points are equal.

  @param[in]  EcGroup   EC group object.
  @param[in]  EcPointA  EC point A.
  @param[in]  EcPointB  EC point B.
  @param[in]  BnCtx     BN context, created with BigNumNewContext().

  @retval TRUE          A == B.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceEcPointEqual (
  IN CONST VOID  *EcGroup,
  IN CONST VOID  *EcPointA,
  IN CONST VOID  *EcPointB,
  IN VOID        *BnCtx
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.PointEqual, EcPointEqual, (EcGroup, EcPointA, EcPointB, BnCtx), FALSE);
}

/**
  Set EC point compressed coordinates. Points can be described in terms of
  their compressed coordinates. For a point (x, y), for any given value for x
  such that the point is on the curve there will only ever be two possible
  values for y. Therefore, a point can be set using this function where BnX is
  the x coordinate and YBit is a value 0 or 1 to identify which of the two
  possible values for y should be used.

  @param[in]  EcGroup    EC group object.
  @param[in]  EcPoint    EC Point.
  @param[in]  BnX        X coordinate.
  @param[in]  YBit       0 or 1 to identify which Y value is used.
  @param[in]  BnCtx      BN context, created with BigNumNewContext().

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
CryptoServiceEcPointSetCompressedCoordinates (
  IN CONST VOID  *EcGroup,
  IN VOID        *EcPoint,
  IN CONST VOID  *BnX,
  IN UINT8       YBit,
  IN VOID        *BnCtx
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.PointSetCompressedCoordinates, EcPointSetCompressedCoordinates, (EcGroup, EcPoint, BnX, YBit, BnCtx), FALSE);
}

// =====================================================================================
//    Elliptic Curve Diffie Hellman Primitives
// =====================================================================================

/**
  Allocates and Initializes one Elliptic Curve Context for subsequent use
  with the NID.

  @param[in]  Nid cipher NID
  @return     Pointer to the Elliptic Curve Context that has been initialized.
              If the allocations fails, EcNewByNid() returns NULL.
**/
VOID *
EFIAPI
CryptoServiceEcNewByNid (
  IN UINTN  Nid
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.NewByNid, EcNewByNid, (Nid), NULL);
}

/**
  Release the specified EC context.

  @param[in]  EcContext  Pointer to the EC context to be released.
**/
VOID
EFIAPI
CryptoServiceEcFree (
  IN  VOID  *EcContext
  )
{
  CALL_VOID_BASECRYPTLIB (Ec.Services.Free, EcFree, (EcContext));
}

/**
  Generates EC key and returns EC public key (X, Y), Please note, this function uses
  pseudo random number generator. The caller must make sure RandomSeed()
  function was properly called before.
  The Ec context should be correctly initialized by EcNewByNid.
  This function generates random secret, and computes the public key (X, Y), which is
  returned via parameter Public, PublicSize.
  X is the first half of Public with size being PublicSize / 2,
  Y is the second half of Public with size being PublicSize / 2.
  EC context is updated accordingly.
  If the Public buffer is too small to hold the public X, Y, FALSE is returned and
  PublicSize is set to the required buffer size to obtain the public X, Y.
  For P-256, the PublicSize is 64. First 32-byte is X, Second 32-byte is Y.
  For P-384, the PublicSize is 96. First 48-byte is X, Second 48-byte is Y.
  For P-521, the PublicSize is 132. First 66-byte is X, Second 66-byte is Y.
  If EcContext is NULL, then return FALSE.
  If PublicSize is NULL, then return FALSE.
  If PublicSize is large enough but Public is NULL, then return FALSE.
  @param[in, out]  EcContext      Pointer to the EC context.
  @param[out]      PublicKey      Pointer to t buffer to receive generated public X,Y.
  @param[in, out]  PublicKeySize  On input, the size of Public buffer in bytes.
                                  On output, the size of data returned in Public buffer in bytes.
  @retval TRUE   EC public X,Y generation succeeded.
  @retval FALSE  EC public X,Y generation failed.
  @retval FALSE  PublicKeySize is not large enough.
**/
BOOLEAN
EFIAPI
CryptoServiceEcGenerateKey (
  IN OUT  VOID   *EcContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.GenerateKey, EcGenerateKey, (EcContext, PublicKey, PublicKeySize), FALSE);
}

/**
  Gets the public key component from the established EC context.
  The Ec context should be correctly initialized by EcNewByNid, and successfully
  generate key pair from EcGenerateKey().
  For P-256, the PublicSize is 64. First 32-byte is X, Second 32-byte is Y.
  For P-384, the PublicSize is 96. First 48-byte is X, Second 48-byte is Y.
  For P-521, the PublicSize is 132. First 66-byte is X, Second 66-byte is Y.
  @param[in, out]  EcContext      Pointer to EC context being set.
  @param[out]      PublicKey      Pointer to t buffer to receive generated public X,Y.
  @param[in, out]  PublicKeySize  On input, the size of Public buffer in bytes.
                                  On output, the size of data returned in Public buffer in bytes.
  @retval  TRUE   EC key component was retrieved successfully.
  @retval  FALSE  Invalid EC key component.
**/
BOOLEAN
EFIAPI
CryptoServiceEcGetPubKey (
  IN OUT  VOID   *EcContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.GetPubKey, EcGetPubKey, (EcContext, PublicKey, PublicKeySize), FALSE);
}

/**
  Computes exchanged common key.
  Given peer's public key (X, Y), this function computes the exchanged common key,
  based on its own context including value of curve parameter and random secret.
  X is the first half of PeerPublic with size being PeerPublicSize / 2,
  Y is the second half of PeerPublic with size being PeerPublicSize / 2.
  If EcContext is NULL, then return FALSE.
  If PeerPublic is NULL, then return FALSE.
  If PeerPublicSize is 0, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeySize is not large enough, then return FALSE.
  For P-256, the PeerPublicSize is 64. First 32-byte is X, Second 32-byte is Y.
  For P-384, the PeerPublicSize is 96. First 48-byte is X, Second 48-byte is Y.
  For P-521, the PeerPublicSize is 132. First 66-byte is X, Second 66-byte is Y.
  @param[in, out]  EcContext          Pointer to the EC context.
  @param[in]       PeerPublic         Pointer to the peer's public X,Y.
  @param[in]       PeerPublicSize     Size of peer's public X,Y in bytes.
  @param[in]       CompressFlag       Flag of PeerPublic is compressed or not.
  @param[out]      Key                Pointer to the buffer to receive generated key.
  @param[in, out]  KeySize            On input, the size of Key buffer in bytes.
                                      On output, the size of data returned in Key buffer in bytes.
  @retval TRUE   EC exchanged key generation succeeded.
  @retval FALSE  EC exchanged key generation failed.
  @retval FALSE  KeySize is not large enough.
**/
BOOLEAN
EFIAPI
CryptoServiceEcDhComputeKey (
  IN OUT  VOID         *EcContext,
  IN      CONST UINT8  *PeerPublic,
  IN      UINTN        PeerPublicSize,
  IN      CONST INT32  *CompressFlag,
  OUT     UINT8        *Key,
  IN OUT  UINTN        *KeySize
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.DhComputeKey, EcDhComputeKey, (EcContext, PeerPublic, PeerPublicSize, CompressFlag, Key, KeySize), FALSE);
}

/**
  Retrieve the EC Public Key from one DER-encoded X509 certificate.

  @param[in]  Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]  CertSize     Size of the X509 certificate in bytes.
  @param[out] EcContext    Pointer to new-generated EC DSA context which contain the retrieved
                           EC public key component. Use EcFree() function to free the
                           resource.

  If Cert is NULL, then return FALSE.
  If EcContext is NULL, then return FALSE.

  @retval  TRUE   EC Public Key was retrieved successfully.
  @retval  FALSE  Fail to retrieve EC public key from X509 certificate.

**/
BOOLEAN
EFIAPI
CryptoServiceEcGetPublicKeyFromX509 (
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  VOID         **EcContext
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.GetPublicKeyFromX509, EcGetPublicKeyFromX509, (Cert, CertSize, EcContext), FALSE);
}

/**
  Retrieve the EC Private Key from the password-protected PEM key data.

  @param[in]  PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[in]  PemSize      Size of the PEM key data in bytes.
  @param[in]  Password     NULL-terminated passphrase used for encrypted PEM key data.
  @param[out] EcContext    Pointer to new-generated EC DSA context which contain the retrieved
                           EC private key component. Use EcFree() function to free the
                           resource.

  If PemData is NULL, then return FALSE.
  If EcContext is NULL, then return FALSE.

  @retval  TRUE   EC Private Key was retrieved successfully.
  @retval  FALSE  Invalid PEM key data or incorrect password.

**/
BOOLEAN
EFIAPI
CryptoServiceEcGetPrivateKeyFromPem (
  IN   CONST UINT8  *PemData,
  IN   UINTN        PemSize,
  IN   CONST CHAR8  *Password,
  OUT  VOID         **EcContext
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.GetPrivateKeyFromPem, EcGetPrivateKeyFromPem, (PemData, PemSize, Password, EcContext), FALSE);
}

/**
  Carries out the EC-DSA signature.

  This function carries out the EC-DSA signature.
  If the Signature buffer is too small to hold the contents of signature, FALSE
  is returned and SigSize is set to the required buffer size to obtain the signature.

  If EcContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If HashSize need match the HashNid. HashNid could be SHA256, SHA384, SHA512, SHA3_256, SHA3_384, SHA3_512.
  If SigSize is large enough but Signature is NULL, then return FALSE.

  For P-256, the SigSize is 64. First 32-byte is R, Second 32-byte is S.
  For P-384, the SigSize is 96. First 48-byte is R, Second 48-byte is S.
  For P-521, the SigSize is 132. First 66-byte is R, Second 66-byte is S.

  @param[in]       EcContext    Pointer to EC context for signature generation.
  @param[in]       HashNid      hash NID
  @param[in]       MessageHash  Pointer to octet message hash to be signed.
  @param[in]       HashSize     Size of the message hash in bytes.
  @param[out]      Signature    Pointer to buffer to receive EC-DSA signature.
  @param[in, out]  SigSize      On input, the size of Signature buffer in bytes.
                                On output, the size of data returned in Signature buffer in bytes.

  @retval  TRUE   Signature successfully generated in EC-DSA.
  @retval  FALSE  Signature generation failed.
  @retval  FALSE  SigSize is too small.

**/
BOOLEAN
EFIAPI
CryptoServiceEcDsaSign (
  IN      VOID         *EcContext,
  IN      UINTN        HashNid,
  IN      CONST UINT8  *MessageHash,
  IN      UINTN        HashSize,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.DsaSign, EcDsaSign, (EcContext, HashNid, MessageHash, HashSize, Signature, SigSize), FALSE);
}

/**
  Verifies the EC-DSA signature.

  If EcContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If HashSize need match the HashNid. HashNid could be SHA256, SHA384, SHA512, SHA3_256, SHA3_384, SHA3_512.

  For P-256, the SigSize is 64. First 32-byte is R, Second 32-byte is S.
  For P-384, the SigSize is 96. First 48-byte is R, Second 48-byte is S.
  For P-521, the SigSize is 132. First 66-byte is R, Second 66-byte is S.

  @param[in]  EcContext    Pointer to EC context for signature verification.
  @param[in]  HashNid      hash NID
  @param[in]  MessageHash  Pointer to octet message hash to be checked.
  @param[in]  HashSize     Size of the message hash in bytes.
  @param[in]  Signature    Pointer to EC-DSA signature to be verified.
  @param[in]  SigSize      Size of signature in bytes.

  @retval  TRUE   Valid signature encoded in EC-DSA.
  @retval  FALSE  Invalid signature or invalid EC context.

**/
BOOLEAN
EFIAPI
CryptoServiceEcDsaVerify (
  IN  VOID         *EcContext,
  IN  UINTN        HashNid,
  IN  CONST UINT8  *MessageHash,
  IN  UINTN        HashSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize
  )
{
  return CALL_BASECRYPTLIB (Ec.Services.DsaVerify, EcDsaVerify, (EcContext, HashNid, MessageHash, HashSize, Signature, SigSize), FALSE);
}

const EDKII_CRYPTO_PROTOCOL  mEdkiiCrypto = {
  /// Version
  CryptoServiceGetCryptoVersion,
  /// HMAC MD5 - deprecated and unsupported
  DeprecatedCryptoServiceHmacMd5New,
  DeprecatedCryptoServiceHmacMd5Free,
  DeprecatedCryptoServiceHmacMd5SetKey,
  DeprecatedCryptoServiceHmacMd5Duplicate,
  DeprecatedCryptoServiceHmacMd5Update,
  DeprecatedCryptoServiceHmacMd5Final,
  /// HMAC SHA1 - deprecated and unsupported
  DeprecatedCryptoServiceHmacSha1New,
  DeprecatedCryptoServiceHmacSha1Free,
  DeprecatedCryptoServiceHmacSha1SetKey,
  DeprecatedCryptoServiceHmacSha1Duplicate,
  DeprecatedCryptoServiceHmacSha1Update,
  DeprecatedCryptoServiceHmacSha1Final,
  /// HMAC SHA256
  CryptoServiceHmacSha256New,
  CryptoServiceHmacSha256Free,
  CryptoServiceHmacSha256SetKey,
  CryptoServiceHmacSha256Duplicate,
  CryptoServiceHmacSha256Update,
  CryptoServiceHmacSha256Final,
  /// Md4 - deprecated and unsupported
  DeprecatedCryptoServiceMd4GetContextSize,
  DeprecatedCryptoServiceMd4Init,
  DeprecatedCryptoServiceMd4Duplicate,
  DeprecatedCryptoServiceMd4Update,
  DeprecatedCryptoServiceMd4Final,
  DeprecatedCryptoServiceMd4HashAll,
 #ifndef ENABLE_MD5_DEPRECATED_INTERFACES
  /// Md5 - deprecated and unsupported
  DeprecatedCryptoServiceMd5GetContextSize,
  DeprecatedCryptoServiceMd5Init,
  DeprecatedCryptoServiceMd5Duplicate,
  DeprecatedCryptoServiceMd5Update,
  DeprecatedCryptoServiceMd5Final,
  DeprecatedCryptoServiceMd5HashAll,
 #else
  /// Md5
  CryptoServiceMd5GetContextSize,
  CryptoServiceMd5Init,
  CryptoServiceMd5Duplicate,
  CryptoServiceMd5Update,
  CryptoServiceMd5Final,
  CryptoServiceMd5HashAll,
 #endif
  /// Pkcs
  CryptoServicePkcs1v2Encrypt,
  CryptoServicePkcs5HashPassword,
  CryptoServicePkcs7Verify,
  CryptoServiceVerifyEKUsInPkcs7Signature,
  CryptoServicePkcs7GetSigners,
  CryptoServicePkcs7FreeSigners,
  CryptoServicePkcs7Sign,
  CryptoServicePkcs7GetAttachedContent,
  CryptoServicePkcs7GetCertificatesList,
  CryptoServiceAuthenticodeVerify,
  CryptoServiceImageTimestampVerify,
  /// DH
  CryptoServiceDhNew,
  CryptoServiceDhFree,
  CryptoServiceDhGenerateParameter,
  CryptoServiceDhSetParameter,
  CryptoServiceDhGenerateKey,
  CryptoServiceDhComputeKey,
  /// Random
  CryptoServiceRandomSeed,
  CryptoServiceRandomBytes,
  /// RSA
  CryptoServiceRsaPkcs1Verify,
  CryptoServiceRsaNew,
  CryptoServiceRsaFree,
  CryptoServiceRsaSetKey,
  CryptoServiceRsaGetKey,
  CryptoServiceRsaGenerateKey,
  CryptoServiceRsaCheckKey,
  CryptoServiceRsaPkcs1Sign,
  CryptoServiceRsaPkcs1Verify,
  CryptoServiceRsaGetPrivateKeyFromPem,
  CryptoServiceRsaGetPublicKeyFromX509,
 #ifdef DISABLE_SHA1_DEPRECATED_INTERFACES
  /// Sha1 - deprecated and unsupported
  DeprecatedCryptoServiceSha1GetContextSize,
  DeprecatedCryptoServiceSha1Init,
  DeprecatedCryptoServiceSha1Duplicate,
  DeprecatedCryptoServiceSha1Update,
  DeprecatedCryptoServiceSha1Final,
  DeprecatedCryptoServiceSha1HashAll,
 #else
  /// Sha1
  CryptoServiceSha1GetContextSize,
  CryptoServiceSha1Init,
  CryptoServiceSha1Duplicate,
  CryptoServiceSha1Update,
  CryptoServiceSha1Final,
  CryptoServiceSha1HashAll,
 #endif
  /// Sha256
  CryptoServiceSha256GetContextSize,
  CryptoServiceSha256Init,
  CryptoServiceSha256Duplicate,
  CryptoServiceSha256Update,
  CryptoServiceSha256Final,
  CryptoServiceSha256HashAll,
  /// Sha384
  CryptoServiceSha384GetContextSize,
  CryptoServiceSha384Init,
  CryptoServiceSha384Duplicate,
  CryptoServiceSha384Update,
  CryptoServiceSha384Final,
  CryptoServiceSha384HashAll,
  /// Sha512
  CryptoServiceSha512GetContextSize,
  CryptoServiceSha512Init,
  CryptoServiceSha512Duplicate,
  CryptoServiceSha512Update,
  CryptoServiceSha512Final,
  CryptoServiceSha512HashAll,
  /// X509
  CryptoServiceX509GetSubjectName,
  CryptoServiceX509GetCommonName,
  CryptoServiceX509GetOrganizationName,
  CryptoServiceX509VerifyCert,
  CryptoServiceX509ConstructCertificate,
  CryptoServiceX509ConstructCertificateStack,
  CryptoServiceX509Free,
  CryptoServiceX509StackFree,
  CryptoServiceX509GetTBSCert,
  /// TDES - deprecated and unsupported
  DeprecatedCryptoServiceTdesGetContextSize,
  DeprecatedCryptoServiceTdesInit,
  DeprecatedCryptoServiceTdesEcbEncrypt,
  DeprecatedCryptoServiceTdesEcbDecrypt,
  DeprecatedCryptoServiceTdesCbcEncrypt,
  DeprecatedCryptoServiceTdesCbcDecrypt,
  /// AES - ECB mode is deprecated and unsupported
  CryptoServiceAesGetContextSize,
  CryptoServiceAesInit,
  DeprecatedCryptoServiceAesEcbEncrypt,
  DeprecatedCryptoServiceAesEcbDecrypt,
  CryptoServiceAesCbcEncrypt,
  CryptoServiceAesCbcDecrypt,
  /// Arc4 - deprecated and unsupported
  DeprecatedCryptoServiceArc4GetContextSize,
  DeprecatedCryptoServiceArc4Init,
  DeprecatedCryptoServiceArc4Encrypt,
  DeprecatedCryptoServiceArc4Decrypt,
  DeprecatedCryptoServiceArc4Reset,
  /// SM3
  CryptoServiceSm3GetContextSize,
  CryptoServiceSm3Init,
  CryptoServiceSm3Duplicate,
  CryptoServiceSm3Update,
  CryptoServiceSm3Final,
  CryptoServiceSm3HashAll,
  /// HKDF
  CryptoServiceHkdfSha256ExtractAndExpand,
  /// X509 (Continued)
  CryptoServiceX509ConstructCertificateStackV,
  /// TLS
  CryptoServiceTlsInitialize,
  CryptoServiceTlsCtxFree,
  CryptoServiceTlsCtxNew,
  CryptoServiceTlsFree,
  CryptoServiceTlsNew,
  CryptoServiceTlsInHandshake,
  CryptoServiceTlsDoHandshake,
  CryptoServiceTlsHandleAlert,
  CryptoServiceTlsCloseNotify,
  CryptoServiceTlsCtrlTrafficOut,
  CryptoServiceTlsCtrlTrafficIn,
  CryptoServiceTlsRead,
  CryptoServiceTlsWrite,
  /// TLS Set
  CryptoServiceTlsSetVersion,
  CryptoServiceTlsSetConnectionEnd,
  CryptoServiceTlsSetCipherList,
  CryptoServiceTlsSetCompressionMethod,
  CryptoServiceTlsSetVerify,
  CryptoServiceTlsSetVerifyHost,
  CryptoServiceTlsSetSessionId,
  CryptoServiceTlsSetCaCertificate,
  CryptoServiceTlsSetHostPublicCert,
  CryptoServiceTlsSetHostPrivateKey,
  CryptoServiceTlsSetCertRevocationList,
  /// TLS Get
  CryptoServiceTlsGetVersion,
  CryptoServiceTlsGetConnectionEnd,
  CryptoServiceTlsGetCurrentCipher,
  CryptoServiceTlsGetCurrentCompressionId,
  CryptoServiceTlsGetVerify,
  CryptoServiceTlsGetSessionId,
  CryptoServiceTlsGetClientRandom,
  CryptoServiceTlsGetServerRandom,
  CryptoServiceTlsGetKeyMaterial,
  CryptoServiceTlsGetCaCertificate,
  CryptoServiceTlsGetHostPublicCert,
  CryptoServiceTlsGetHostPrivateKey,
  CryptoServiceTlsGetCertRevocationList,
  /// RSA PSS
  CryptoServiceRsaPssSign,
  CryptoServiceRsaPssVerify,
  /// Parallel hash
  CryptoServiceParallelHash256HashAll,
  /// HMAC SHA256 (continued)
  CryptoServiceHmacSha256All,
  /// HMAC SHA384
  CryptoServiceHmacSha384New,
  CryptoServiceHmacSha384Free,
  CryptoServiceHmacSha384SetKey,
  CryptoServiceHmacSha384Duplicate,
  CryptoServiceHmacSha384Update,
  CryptoServiceHmacSha384Final,
  CryptoServiceHmacSha384All,
  /// HKDF (continued)
  CryptoServiceHkdfSha256Extract,
  CryptoServiceHkdfSha256Expand,
  CryptoServiceHkdfSha384ExtractAndExpand,
  CryptoServiceHkdfSha384Extract,
  CryptoServiceHkdfSha384Expand,
  /// Aead Aes GCM
  CryptoServiceAeadAesGcmEncrypt,
  CryptoServiceAeadAesGcmDecrypt,
  /// Big Numbers
  CryptoServiceBigNumInit,
  CryptoServiceBigNumFromBin,
  CryptoServiceBigNumToBin,
  CryptoServiceBigNumFree,
  CryptoServiceBigNumAdd,
  CryptoServiceBigNumSub,
  CryptoServiceBigNumMod,
  CryptoServiceBigNumExpMod,
  CryptoServiceBigNumInverseMod,
  CryptoServiceBigNumDiv,
  CryptoServiceBigNumMulMod,
  CryptoServiceBigNumCmp,
  CryptoServiceBigNumBits,
  CryptoServiceBigNumBytes,
  CryptoServiceBigNumIsWord,
  CryptoServiceBigNumIsOdd,
  CryptoServiceBigNumCopy,
  CryptoServiceBigNumValueOne,
  CryptoServiceBigNumRShift,
  CryptoServiceBigNumConstTime,
  CryptoServiceBigNumSqrMod,
  CryptoServiceBigNumNewContext,
  CryptoServiceBigNumContextFree,
  CryptoServiceBigNumSetUint,
  CryptoServiceBigNumAddMod,
  /// EC
  CryptoServiceEcGroupInit,
  CryptoServiceEcGroupGetCurve,
  CryptoServiceEcGroupGetOrder,
  CryptoServiceEcGroupFree,
  CryptoServiceEcPointInit,
  CryptoServiceEcPointDeInit,
  CryptoServiceEcPointGetAffineCoordinates,
  CryptoServiceEcPointSetAffineCoordinates,
  CryptoServiceEcPointAdd,
  CryptoServiceEcPointMul,
  CryptoServiceEcPointInvert,
  CryptoServiceEcPointIsOnCurve,
  CryptoServiceEcPointIsAtInfinity,
  CryptoServiceEcPointEqual,
  CryptoServiceEcPointSetCompressedCoordinates,
  CryptoServiceEcNewByNid,
  CryptoServiceEcFree,
  CryptoServiceEcGenerateKey,
  CryptoServiceEcGetPubKey,
  CryptoServiceEcDhComputeKey,
  /// TLS (continued)
  CryptoServiceTlsShutdown,
  /// TLS Set (continued)
  CryptoServiceTlsSetHostPrivateKeyEx,
  CryptoServiceTlsSetSignatureAlgoList,
  CryptoServiceTlsSetEcCurve,
  /// TLS Get (continued)
  CryptoServiceTlsGetExportKey,
  /// Ec (Continued)
  CryptoServiceEcGetPublicKeyFromX509,
  CryptoServiceEcGetPrivateKeyFromPem,
  CryptoServiceEcDsaSign,
  CryptoServiceEcDsaVerify,
  /// X509 (Continued)
  CryptoServiceX509GetVersion,
  CryptoServiceX509GetSerialNumber,
  CryptoServiceX509GetIssuerName,
  CryptoServiceX509GetSignatureAlgorithm,
  CryptoServiceX509GetExtensionData,
  CryptoServiceX509GetExtendedKeyUsage,
  CryptoServiceX509GetValidity,
  CryptoServiceX509FormatDateTime,
  CryptoServiceX509CompareDateTime,
  CryptoServiceX509GetKeyUsage,
  CryptoServiceX509VerifyCertChain,
  CryptoServiceX509GetCertFromCertChain,
  CryptoServiceAsn1GetTag,
  CryptoServiceX509GetExtendedBasicConstraints,
  CryptoServicePkcs1v2Decrypt,
  CryptoServiceRsaOaepEncrypt,
  CryptoServiceRsaOaepDecrypt,
  /// TlsSet (Continued)
  CryptoServiceTlsSetSecurityLevel,
};

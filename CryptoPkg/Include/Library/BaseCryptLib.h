/** @file
  Defines base cryptographic library APIs.
  The Base Cryptographic Library provides implementations of basic cryptography
  primitives (Hash Serials, HMAC, RSA, Diffie-Hellman, etc) for UEFI security
  functionality enabling.

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __BASE_CRYPT_LIB_H__
#define __BASE_CRYPT_LIB_H__

///
/// MD4 digest size in bytes
///
#define MD4_DIGEST_SIZE     16

///
/// MD5 digest size in bytes
///
#define MD5_DIGEST_SIZE     16

///
/// SHA-1 digest size in bytes.
///
#define SHA1_DIGEST_SIZE    20

///
/// SHA-256 digest size in bytes
///
#define SHA256_DIGEST_SIZE  32

///
/// TDES block size in bytes
///
#define TDES_BLOCK_SIZE     8

///
/// AES block size in bytes
///
#define AES_BLOCK_SIZE      16

///
/// RSA Key Tags Definition used in RsaSetKey() function for key component identification.
///
typedef enum {
  RsaKeyN,      ///< RSA public Modulus (N)
  RsaKeyE,      ///< RSA Public exponent (e)
  RsaKeyD,      ///< RSA Private exponent (d)
  RsaKeyP,      ///< RSA secret prime factor of Modulus (p)
  RsaKeyQ,      ///< RSA secret prime factor of Modules (q)
  RsaKeyDp,     ///< p's CRT exponent (== d mod (p - 1))
  RsaKeyDq,     ///< q's CRT exponent (== d mod (q - 1))
  RsaKeyQInv    ///< The CRT coefficient (== 1/q mod p)
} RSA_KEY_TAG;

//=====================================================================================
//    One-Way Cryptographic Hash Primitives
//=====================================================================================

/**
  Retrieves the size, in bytes, of the context buffer required for MD4 hash operations.

  @return  The size, in bytes, of the context buffer required for MD4 hash operations.

**/
UINTN
EFIAPI
Md4GetContextSize (
  VOID
  );

/**
  Initializes user-supplied memory pointed by Md4Context as MD4 hash context for
  subsequent use.

  If Md4Context is NULL, then return FALSE.

  @param[out]  Md4Context  Pointer to MD4 context being initialized.

  @retval TRUE   MD4 context initialization succeeded.
  @retval FALSE  MD4 context initialization failed.

**/
BOOLEAN
EFIAPI
Md4Init (
  OUT  VOID  *Md4Context
  );

/**
  Makes a copy of an existing MD4 context.

  If Md4Context is NULL, then return FALSE.
  If NewMd4Context is NULL, then return FALSE.

  @param[in]  Md4Context     Pointer to MD4 context being copied.
  @param[out] NewMd4Context  Pointer to new MD4 context.

  @retval TRUE   MD4 context copy succeeded.
  @retval FALSE  MD4 context copy failed.

**/
BOOLEAN
EFIAPI
Md4Duplicate (
  IN   CONST VOID  *Md4Context,
  OUT  VOID        *NewMd4Context
  );

/**
  Digests the input data and updates MD4 context.

  This function performs MD4 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  MD4 context should be already correctly intialized by Md4Init(), and should not be finalized
  by Md4Final(). Behavior with invalid context is undefined.

  If Md4Context is NULL, then return FALSE.

  @param[in, out]  Md4Context  Pointer to the MD4 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize    Size of Data buffer in bytes.

  @retval TRUE   MD4 data digest succeeded.
  @retval FALSE  MD4 data digest failed.

**/
BOOLEAN
EFIAPI
Md4Update (
  IN OUT  VOID        *Md4Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

/**
  Completes computation of the MD4 digest value.

  This function completes MD4 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the MD4 context cannot
  be used again.
  MD4 context should be already correctly intialized by Md4Init(), and should not be
  finalized by Md4Final(). Behavior with invalid MD4 context is undefined.

  If Md4Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Md4Context  Pointer to the MD4 context.
  @param[out]      HashValue   Pointer to a buffer that receives the MD4 digest
                               value (16 bytes).

  @retval TRUE   MD4 digest computation succeeded.
  @retval FALSE  MD4 digest computation failed.

**/
BOOLEAN
EFIAPI
Md4Final (
  IN OUT  VOID   *Md4Context,
  OUT     UINT8  *HashValue
  );

/**
  Retrieves the size, in bytes, of the context buffer required for MD5 hash operations.

  @return  The size, in bytes, of the context buffer required for MD5 hash operations.

**/
UINTN
EFIAPI
Md5GetContextSize (
  VOID
  );

/**
  Initializes user-supplied memory pointed by Md5Context as MD5 hash context for
  subsequent use.

  If Md5Context is NULL, then return FALSE.

  @param[out]  Md5Context  Pointer to MD5 context being initialized.

  @retval TRUE   MD5 context initialization succeeded.
  @retval FALSE  MD5 context initialization failed.

**/
BOOLEAN
EFIAPI
Md5Init (
  OUT  VOID  *Md5Context
  );

/**
  Makes a copy of an existing MD5 context.

  If Md5Context is NULL, then return FALSE.
  If NewMd5Context is NULL, then return FALSE.

  @param[in]  Md5Context     Pointer to MD5 context being copied.
  @param[out] NewMd5Context  Pointer to new MD5 context.

  @retval TRUE   MD5 context copy succeeded.
  @retval FALSE  MD5 context copy failed.

**/
BOOLEAN
EFIAPI
Md5Duplicate (
  IN   CONST VOID  *Md5Context,
  OUT  VOID        *NewMd5Context
  );

/**
  Digests the input data and updates MD5 context.

  This function performs MD5 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  MD5 context should be already correctly intialized by Md5Init(), and should not be finalized
  by Md5Final(). Behavior with invalid context is undefined.

  If Md5Context is NULL, then return FALSE.

  @param[in, out]  Md5Context  Pointer to the MD5 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize    Size of Data buffer in bytes.

  @retval TRUE   MD5 data digest succeeded.
  @retval FALSE  MD5 data digest failed.

**/
BOOLEAN
EFIAPI
Md5Update (
  IN OUT  VOID        *Md5Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

/**
  Completes computation of the MD5 digest value.

  This function completes MD5 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the MD5 context cannot
  be used again.
  MD5 context should be already correctly intialized by Md5Init(), and should not be
  finalized by Md5Final(). Behavior with invalid MD5 context is undefined.

  If Md5Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Md5Context  Pointer to the MD5 context.
  @param[out]      HashValue   Pointer to a buffer that receives the MD5 digest
                               value (16 bytes).

  @retval TRUE   MD5 digest computation succeeded.
  @retval FALSE  MD5 digest computation failed.

**/
BOOLEAN
EFIAPI
Md5Final (
  IN OUT  VOID   *Md5Context,
  OUT     UINT8  *HashValue
  );

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-1 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-1 hash operations.

**/
UINTN
EFIAPI
Sha1GetContextSize (
  VOID
  );

/**
  Initializes user-supplied memory pointed by Sha1Context as SHA-1 hash context for
  subsequent use.

  If Sha1Context is NULL, then return FALSE.

  @param[out]  Sha1Context  Pointer to SHA-1 context being initialized.

  @retval TRUE   SHA-1 context initialization succeeded.
  @retval FALSE  SHA-1 context initialization failed.

**/
BOOLEAN
EFIAPI
Sha1Init (
  OUT  VOID  *Sha1Context
  );

/**
  Makes a copy of an existing SHA-1 context.

  If Sha1Context is NULL, then return FALSE.
  If NewSha1Context is NULL, then return FALSE.

  @param[in]  Sha1Context     Pointer to SHA-1 context being copied.
  @param[out] NewSha1Context  Pointer to new SHA-1 context.

  @retval TRUE   SHA-1 context copy succeeded.
  @retval FALSE  SHA-1 context copy failed.

**/
BOOLEAN
EFIAPI
Sha1Duplicate (
  IN   CONST VOID  *Sha1Context,
  OUT  VOID        *NewSha1Context
  );

/**
  Digests the input data and updates SHA-1 context.

  This function performs SHA-1 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-1 context should be already correctly intialized by Sha1Init(), and should not be finalized
  by Sha1Final(). Behavior with invalid context is undefined.

  If Sha1Context is NULL, then return FALSE.

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context.
  @param[in]       Data         Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize     Size of Data buffer in bytes.

  @retval TRUE   SHA-1 data digest succeeded.
  @retval FALSE  SHA-1 data digest failed.

**/
BOOLEAN
EFIAPI
Sha1Update (
  IN OUT  VOID        *Sha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

/**
  Completes computation of the SHA-1 digest value.

  This function completes SHA-1 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-1 context cannot
  be used again.
  SHA-1 context should be already correctly intialized by Sha1Init(), and should not be
  finalized by Sha1Final(). Behavior with invalid SHA-1 context is undefined.

  If Sha1Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context.
  @param[out]      HashValue    Pointer to a buffer that receives the SHA-1 digest
                                value (20 bytes).

  @retval TRUE   SHA-1 digest computation succeeded.
  @retval FALSE  SHA-1 digest computation failed.

**/
BOOLEAN
EFIAPI
Sha1Final (
  IN OUT  VOID   *Sha1Context,
  OUT     UINT8  *HashValue
  );

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-256 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-256 hash operations.

**/
UINTN
EFIAPI
Sha256GetContextSize (
  VOID
  );

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
Sha256Init (
  OUT  VOID  *Sha256Context
  );

/**
  Makes a copy of an existing SHA-256 context.

  If Sha256Context is NULL, then return FALSE.
  If NewSha256Context is NULL, then return FALSE.

  @param[in]  Sha256Context     Pointer to SHA-256 context being copied.
  @param[out] NewSha256Context  Pointer to new SHA-256 context.

  @retval TRUE   SHA-256 context copy succeeded.
  @retval FALSE  SHA-256 context copy failed.

**/
BOOLEAN
EFIAPI
Sha256Duplicate (
  IN   CONST VOID  *Sha256Context,
  OUT  VOID        *NewSha256Context
  );

/**
  Digests the input data and updates SHA-256 context.

  This function performs SHA-256 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SHA-256 context should be already correctly intialized by Sha256Init(), and should not be finalized
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
Sha256Update (
  IN OUT  VOID        *Sha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

/**
  Completes computation of the SHA-256 digest value.

  This function completes SHA-256 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SHA-256 context cannot
  be used again.
  SHA-256 context should be already correctly intialized by Sha256Init(), and should not be
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
Sha256Final (
  IN OUT  VOID   *Sha256Context,
  OUT     UINT8  *HashValue
  );


//=====================================================================================
//    MAC (Message Authentication Code) Primitive
//=====================================================================================

/**
  Retrieves the size, in bytes, of the context buffer required for HMAC-MD5 operations.

  @return  The size, in bytes, of the context buffer required for HMAC-MD5 operations.

**/
UINTN
EFIAPI
HmacMd5GetContextSize (
  VOID
  );

/**
  Initializes user-supplied memory pointed by HmacMd5Context as HMAC-MD5 context for
  subsequent use.

  If HmacMd5Context is NULL, then return FALSE.

  @param[out]  HmacMd5Context  Pointer to HMAC-MD5 context being initialized.
  @param[in]   Key             Pointer to the user-supplied key.
  @param[in]   KeySize         Key size in bytes.

  @retval TRUE   HMAC-MD5 context initialization succeeded.
  @retval FALSE  HMAC-MD5 context initialization failed.

**/
BOOLEAN
EFIAPI
HmacMd5Init (
  OUT  VOID         *HmacMd5Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );

/**
  Makes a copy of an existing HMAC-MD5 context.

  If HmacMd5Context is NULL, then return FALSE.
  If NewHmacMd5Context is NULL, then return FALSE.

  @param[in]  HmacMd5Context     Pointer to HMAC-MD5 context being copied.
  @param[out] NewHmacMd5Context  Pointer to new HMAC-MD5 context.

  @retval TRUE   HMAC-MD5 context copy succeeded.
  @retval FALSE  HMAC-MD5 context copy failed.

**/
BOOLEAN
EFIAPI
HmacMd5Duplicate (
  IN   CONST VOID  *HmacMd5Context,
  OUT  VOID        *NewHmacMd5Context
  );

/**
  Digests the input data and updates HMAC-MD5 context.

  This function performs HMAC-MD5 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-MD5 context should be already correctly intialized by HmacMd5Init(), and should not be
  finalized by HmacMd5Final(). Behavior with invalid context is undefined.

  If HmacMd5Context is NULL, then return FALSE.

  @param[in, out]  HmacMd5Context  Pointer to the HMAC-MD5 context.
  @param[in]       Data            Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize        Size of Data buffer in bytes.

  @retval TRUE   HMAC-MD5 data digest succeeded.
  @retval FALSE  HMAC-MD5 data digest failed.

**/
BOOLEAN
EFIAPI
HmacMd5Update (
  IN OUT  VOID        *HmacMd5Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

/**
  Completes computation of the HMAC-MD5 digest value.

  This function completes HMAC-MD5 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-MD5 context cannot
  be used again.
  HMAC-MD5 context should be already correctly intialized by HmacMd5Init(), and should not be
  finalized by HmacMd5Final(). Behavior with invalid HMAC-MD5 context is undefined.

  If HmacMd5Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  HmacMd5Context  Pointer to the HMAC-MD5 context.
  @param[out]      HashValue       Pointer to a buffer that receives the HMAC-MD5 digest
                                   value (16 bytes).

  @retval TRUE   HMAC-MD5 digest computation succeeded.
  @retval FALSE  HMAC-MD5 digest computation failed.

**/
BOOLEAN
EFIAPI
HmacMd5Final (
  IN OUT  VOID   *HmacMd5Context,
  OUT     UINT8  *HmacValue
  );

/**
  Retrieves the size, in bytes, of the context buffer required for HMAC-SHA1 operations.

  @return  The size, in bytes, of the context buffer required for HMAC-SHA1 operations.

**/
UINTN
EFIAPI
HmacSha1GetContextSize (
  VOID
  );

/**
  Initializes user-supplied memory pointed by HmacSha1Context as HMAC-SHA1 context for
  subsequent use.

  If HmacSha1Context is NULL, then return FALSE.

  @param[out]  HmacSha1Context  Pointer to HMAC-SHA1 context being initialized.
  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.

  @retval TRUE   HMAC-SHA1 context initialization succeeded.
  @retval FALSE  HMAC-SHA1 context initialization failed.

**/
BOOLEAN
EFIAPI
HmacSha1Init (
  OUT  VOID         *HmacSha1Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );

/**
  Makes a copy of an existing HMAC-SHA1 context.

  If HmacSha1Context is NULL, then return FALSE.
  If NewHmacSha1Context is NULL, then return FALSE.

  @param[in]  HmacSha1Context     Pointer to HMAC-SHA1 context being copied.
  @param[out] NewHmacSha1Context  Pointer to new HMAC-SHA1 context.

  @retval TRUE   HMAC-SHA1 context copy succeeded.
  @retval FALSE  HMAC-SHA1 context copy failed.

**/
BOOLEAN
EFIAPI
HmacSha1Duplicate (
  IN   CONST VOID  *HmacSha1Context,
  OUT  VOID        *NewHmacSha1Context
  );

/**
  Digests the input data and updates HMAC-SHA1 context.

  This function performs HMAC-SHA1 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-SHA1 context should be already correctly intialized by HmacSha1Init(), and should not
  be finalized by HmacSha1Final(). Behavior with invalid context is undefined.

  If HmacSha1Context is NULL, then return FALSE.

  @param[in, out]  HmacSha1Context Pointer to the HMAC-SHA1 context.
  @param[in]       Data            Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize        Size of Data buffer in bytes.

  @retval TRUE   HMAC-SHA1 data digest succeeded.
  @retval FALSE  HMAC-SHA1 data digest failed.

**/
BOOLEAN
EFIAPI
HmacSha1Update (
  IN OUT  VOID        *HmacSha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

/**
  Completes computation of the HMAC-SHA1 digest value.

  This function completes HMAC-SHA1 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-SHA1 context cannot
  be used again.
  HMAC-SHA1 context should be already correctly intialized by HmacSha1Init(), and should
  not be finalized by HmacSha1Final(). Behavior with invalid HMAC-SHA1 context is undefined.

  If HmacSha1Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  HmacSha1Context  Pointer to the HMAC-SHA1 context.
  @param[out]      HashValue        Pointer to a buffer that receives the HMAC-SHA1 digest
                                    value (20 bytes).

  @retval TRUE   HMAC-SHA1 digest computation succeeded.
  @retval FALSE  HMAC-SHA1 digest computation failed.

**/
BOOLEAN
EFIAPI
HmacSha1Final (
  IN OUT  VOID   *HmacSha1Context,
  OUT     UINT8  *HmacValue
  );


//=====================================================================================
//    Symmetric Cryptography Primitive
//=====================================================================================

/**
  Retrieves the size, in bytes, of the context buffer required for TDES operations.

  @return  The size, in bytes, of the context buffer required for TDES operations.

**/
UINTN
EFIAPI
TdesGetContextSize (
  VOID
  );

/**
  Initializes user-supplied memory as TDES context for subsequent use.

  This function initializes user-supplied memory pointed by TdesContext as TDES context.
  In addtion, it sets up all TDES key materials for subsequent encryption and decryption
  operations.
  There are 3 key options as follows:
  KeyLength = 64,  Keying option 1: K1 == K2 == K3 (Backward compatibility with DES)
  KeyLength = 128, Keying option 2: K1 != K2 and K3 = K1 (Less Security)
  KeyLength = 192  Keying option 3: K1 != K2 != K3 (Strongest)

  If TdesContext is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeyLength is not valid, then return FALSE.

  @param[out]  TdesContext  Pointer to TDES context being initialized.
  @param[in]   Key          Pointer to the user-supplied TDES key.
  @param[in]   KeyLength    Length of TDES key in bits.

  @retval TRUE   TDES context initialization succeeded.
  @retval FALSE  TDES context initialization failed.

**/
BOOLEAN
EFIAPI
TdesInit (
  OUT  VOID         *TdesContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  );

/**
  Performs TDES encryption on a data buffer of the specified size in ECB mode.

  This function performs TDES encryption on data buffer pointed by Input, of specified
  size of InputSize, in ECB mode.
  InputSize must be multiple of block size (8 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  TdesContext should be already correctly initialized by TdesInit(). Behavior with
  invalid TDES context is undefined.

  If TdesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (8 bytes), then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval TRUE   TDES encryption succeeded.
  @retval FALSE  TDES encryption failed.

**/
BOOLEAN
EFIAPI
TdesEcbEncrypt (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  );

/**
  Performs TDES decryption on a data buffer of the specified size in ECB mode.

  This function performs TDES decryption on data buffer pointed by Input, of specified
  size of InputSize, in ECB mode.
  InputSize must be multiple of block size (8 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  TdesContext should be already correctly initialized by TdesInit(). Behavior with
  invalid TDES context is undefined.

  If TdesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (8 bytes), then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be decrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[out]  Output       Pointer to a buffer that receives the TDES decryption output.

  @retval TRUE   TDES decryption succeeded.
  @retval FALSE  TDES decryption failed.

**/
BOOLEAN
EFIAPI
TdesEcbDecrypt (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  );

/**
  Performs TDES encryption on a data buffer of the specified size in CBC mode.

  This function performs TDES encryption on data buffer pointed by Input, of specified
  size of InputSize, in CBC mode.
  InputSize must be multiple of block size (8 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  Initialization vector should be one block size (8 bytes).
  TdesContext should be already correctly initialized by TdesInit(). Behavior with
  invalid TDES context is undefined.

  If TdesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (8 bytes), then return FALSE.
  If Ivec is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[in]   Ivec         Pointer to initialization vector.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval TRUE   TDES encryption succeeded.
  @retval FALSE  TDES encryption failed.

**/
BOOLEAN
EFIAPI
TdesCbcEncrypt (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  );

/**
  Performs TDES decryption on a data buffer of the specified size in CBC mode.

  This function performs TDES decryption on data buffer pointed by Input, of specified
  size of InputSize, in CBC mode.
  InputSize must be multiple of block size (8 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  Initialization vector should be one block size (8 bytes).
  TdesContext should be already correctly initialized by TdesInit(). Behavior with
  invalid TDES context is undefined.

  If TdesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (8 bytes), then return FALSE.
  If Ivec is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   TdesContext  Pointer to the TDES context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[in]   Ivec         Pointer to initialization vector.
  @param[out]  Output       Pointer to a buffer that receives the TDES encryption output.

  @retval TRUE   TDES decryption succeeded.
  @retval FALSE  TDES decryption failed.

**/
BOOLEAN
EFIAPI
TdesCbcDecrypt (
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  );

/**
  Retrieves the size, in bytes, of the context buffer required for AES operations.

  @return  The size, in bytes, of the context buffer required for AES operations.

**/
UINTN
EFIAPI
AesGetContextSize (
  VOID
  );

/**
  Initializes user-supplied memory as AES context for subsequent use.

  This function initializes user-supplied memory pointed by AesContext as AES context.
  In addtion, it sets up all AES key materials for subsequent encryption and decryption
  operations.
  There are 3 options for key length, 128 bits, 192 bits, and 256 bits.

  If AesContext is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeyLength is not valid, then return FALSE.

  @param[out]  AesContext  Pointer to AES context being initialized.
  @param[in]   Key         Pointer to the user-supplied AES key.
  @param[in]   KeyLength   Length of AES key in bits.

  @retval TRUE   AES context initialization succeeded.
  @retval FALSE  AES context initialization failed.

**/
BOOLEAN
EFIAPI
AesInit (
  OUT  VOID         *AesContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  );

/**
  Performs AES encryption on a data buffer of the specified size in ECB mode.

  This function performs AES encryption on data buffer pointed by Input, of specified
  size of InputSize, in ECB mode.
  InputSize must be multiple of block size (16 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  AesContext should be already correctly initialized by AesInit(). Behavior with
  invalid AES context is undefined.

  If AesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (16 bytes), then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval TRUE   AES encryption succeeded.
  @retval FALSE  AES encryption failed.

**/
BOOLEAN
EFIAPI
AesEcbEncrypt (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  );

/**
  Performs AES decryption on a data buffer of the specified size in ECB mode.

  This function performs AES decryption on data buffer pointed by Input, of specified
  size of InputSize, in ECB mode.
  InputSize must be multiple of block size (16 bytes). This function does not perform
  padding. Caller must perform padding, if necessary, to ensure valid input data size.
  AesContext should be already correctly initialized by AesInit(). Behavior with
  invalid AES context is undefined.

  If AesContext is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If InputSize is not multiple of block size (16 bytes), then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be decrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[out]  Output      Pointer to a buffer that receives the AES decryption output.

  @retval TRUE   AES decryption succeeded.
  @retval FALSE  AES decryption failed.

**/
BOOLEAN
EFIAPI
AesEcbDecrypt (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  );

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

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[in]   Ivec        Pointer to initialization vector.
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval TRUE   AES encryption succeeded.
  @retval FALSE  AES encryption failed.

**/
BOOLEAN
EFIAPI
AesCbcEncrypt (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  );

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

  @param[in]   AesContext  Pointer to the AES context.
  @param[in]   Input       Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize   Size of the Input buffer in bytes.
  @param[in]   Ivec        Pointer to initialization vector.
  @param[out]  Output      Pointer to a buffer that receives the AES encryption output.

  @retval TRUE   AES decryption succeeded.
  @retval FALSE  AES decryption failed.

**/
BOOLEAN
EFIAPI
AesCbcDecrypt (
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  );

/**
  Retrieves the size, in bytes, of the context buffer required for ARC4 operations.

  @return  The size, in bytes, of the context buffer required for ARC4 operations.

**/
UINTN
EFIAPI
Arc4GetContextSize (
  VOID
  );

/**
  Initializes user-supplied memory as ARC4 context for subsequent use.

  This function initializes user-supplied memory pointed by Arc4Context as ARC4 context.
  In addtion, it sets up all ARC4 key materials for subsequent encryption and decryption
  operations.

  If Arc4Context is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeySize does not in the range of [5, 256] bytes, then return FALSE.

  @param[out]  Arc4Context  Pointer to ARC4 context being initialized.
  @param[in]   Key          Pointer to the user-supplied ARC4 key.
  @param[in]   KeySize      Size of ARC4 key in bytes.

  @retval TRUE   ARC4 context initialization succeeded.
  @retval FALSE  ARC4 context initialization failed.

**/
BOOLEAN
EFIAPI
Arc4Init (
  OUT  VOID         *Arc4Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );

/**
  Performs ARC4 encryption on a data buffer of the specified size.

  This function performs ARC4 encryption on data buffer pointed by Input, of specified
  size of InputSize.
  Arc4Context should be already correctly initialized by Arc4Init(). Behavior with
  invalid ARC4 context is undefined.

  If Arc4Context is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   Arc4Context  Pointer to the ARC4 context.
  @param[in]   Input        Pointer to the buffer containing the data to be encrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[out]  Output       Pointer to a buffer that receives the ARC4 encryption output.

  @retval TRUE   ARC4 encryption succeeded.
  @retval FALSE  ARC4 encryption failed.

**/
BOOLEAN
EFIAPI
Arc4Encrypt (
  IN OUT  VOID         *Arc4Context,
  IN      CONST UINT8  *Input,
  IN      UINTN        InputSize,
  OUT     UINT8        *Output
  );

/**
  Performs ARC4 decryption on a data buffer of the specified size.

  This function performs ARC4 decryption on data buffer pointed by Input, of specified
  size of InputSize.
  Arc4Context should be already correctly initialized by Arc4Init(). Behavior with
  invalid ARC4 context is undefined.

  If Arc4Context is NULL, then return FALSE.
  If Input is NULL, then return FALSE.
  If Output is NULL, then return FALSE.

  @param[in]   Arc4Context  Pointer to the ARC4 context.
  @param[in]   Input        Pointer to the buffer containing the data to be decrypted.
  @param[in]   InputSize    Size of the Input buffer in bytes.
  @param[out]  Output       Pointer to a buffer that receives the ARC4 decryption output.

  @retval TRUE   ARC4 decryption succeeded.
  @retval FALSE  ARC4 decryption failed.

**/
BOOLEAN
EFIAPI
Arc4Decrypt (
  IN OUT  VOID   *Arc4Context,
  IN      UINT8  *Input,
  IN      UINTN  InputSize,
  OUT     UINT8  *Output
  );

/**
  Resets the ARC4 context to the initial state.

  The function resets the ARC4 context to the state it had immediately after the
  ARC4Init() function call.
  Contrary to ARC4Init(), Arc4Reset() requires no secret key as input, but ARC4 context
  should be already correctly initialized by ARC4Init().

  If Arc4Context is NULL, then return FALSE.

  @param[in, out]  Arc4Context  Pointer to the ARC4 context.

  @retval TRUE   ARC4 reset succeeded.
  @retval FALSE  ARC4 reset failed.

**/
BOOLEAN
EFIAPI
Arc4Reset (
  IN OUT  VOID  *Arc4Context
  );

//=====================================================================================
//    Asymmetric Cryptography Primitive
//=====================================================================================

/**
  Allocates and initializes one RSA context for subsequent use.

  @return  Pointer to the RSA context that has been initialized.
           If the allocations fails, RsaNew() returns NULL.

**/
VOID *
EFIAPI
RsaNew (
  VOID
  );

/**
  Release the specified RSA context.

  If RsaContext is NULL, then return FALSE.

  @param[in]  RsaContext  Pointer to the RSA context to be released.

**/
VOID
EFIAPI
RsaFree (
  IN  VOID  *RsaContext
  );

/**
  Sets the tag-designated key component into the established RSA context.

  This function sets the tag-designated RSA key component into the established
  RSA context from the user-specified non-negative integer (octet string format
  represented in RSA PKCS#1).
  If BigNumber is NULL, then the specified key componenet in RSA context is cleared.

  If RsaContext is NULL, then return FALSE.

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[in]       BigNumber   Pointer to octet integer buffer.
                               If NULL, then the specified key componenet in RSA
                               context is cleared.
  @param[in]       BnSize      Size of big number buffer in bytes.
                               If BigNumber is NULL, then it is ignored.

  @retval  TRUE   RSA key component was set successfully.
  @retval  FALSE  Invalid RSA key component tag.

**/
BOOLEAN
EFIAPI
RsaSetKey (
  IN OUT  VOID         *RsaContext,
  IN      RSA_KEY_TAG  KeyTag,
  IN      CONST UINT8  *BigNumber,
  IN      UINTN        BnSize
  );

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

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[out]      BigNumber   Pointer to octet integer buffer.
  @param[in, out]  BnSize      On input, the size of big number buffer in bytes.
                               On output, the size of data returned in big number buffer in bytes.

  @retval  TRUE   RSA key component was retrieved successfully.
  @retval  FALSE  Invalid RSA key component tag.
  @retval  FALSE  BnSize is too small.

**/
BOOLEAN
EFIAPI
RsaGetKey (
  IN OUT  VOID         *RsaContext,
  IN      RSA_KEY_TAG  KeyTag,
  OUT     UINT8        *BigNumber,
  IN OUT  UINTN        *BnSize
  );

/**
  Generates RSA key components.

  This function generates RSA key components. It takes RSA public exponent E and
  length in bits of RSA modulus N as input, and generates all key components.
  If PublicExponent is NULL, the default RSA public exponent (0x10001) will be used.

  Before this function can be invoked, pseudorandom number generator must be correctly
  initialized by RandomSeed().

  If RsaContext is NULL, then return FALSE.

  @param[in, out]  RsaContext           Pointer to RSA context being set.
  @param[in]       ModulusLength        Length of RSA modulus N in bits.
  @param[in]       PublicExponent       Pointer to RSA public exponent.
  @param[in]       PublicExponentSize   Size of RSA public exponent buffer in bytes. 

  @retval  TRUE   RSA key component was generated successfully.
  @retval  FALSE  Invalid RSA key component tag.

**/
BOOLEAN
EFIAPI
RsaGenerateKey (
  IN OUT  VOID         *RsaContext,
  IN      UINTN        ModulusLength,
  IN      CONST UINT8  *PublicExponent,
  IN      UINTN        PublicExponentSize
  );

/**
  Validates key components of RSA context.

  This function validates key compoents of RSA context in following aspects:
  - Whether p is a prime
  - Whether q is a prime
  - Whether n = p * q
  - Whether d*e = 1  mod lcm(p-1,q-1)

  If RsaContext is NULL, then return FALSE.

  @param[in]  RsaContext  Pointer to RSA context to check.

  @retval  TRUE   RSA key components are valid.
  @retval  FALSE  RSA key components are not valid.

**/
BOOLEAN
EFIAPI
RsaCheckKey (
  IN  VOID  *RsaContext
  );

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

  @param[in]      RsaContext   Pointer to RSA context for signature generation.
  @param[in]      MessageHash  Pointer to octet message hash to be signed.
  @param[in]      HashSize     Size of the message hash in bytes.
  @param[out]     Signature    Pointer to buffer to receive RSA PKCS1-v1_5 signature.
  @param[in, out] SigSize      On input, the size of Signature buffer in bytes.
                               On output, the size of data returned in Signature buffer in bytes.

  @retval  TRUE   Signature successfully generated in PKCS1-v1_5.
  @retval  FALSE  Signature generation failed.
  @retval  FALSE  SigSize is too small.

**/
BOOLEAN
EFIAPI
RsaPkcs1Sign (
  IN      VOID         *RsaContext,
  IN      CONST UINT8  *MessageHash,
  IN      UINTN        HashSize,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  );

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
RsaPkcs1Verify (
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *MessageHash,
  IN  UINTN        HashSize,
  IN  UINT8        *Signature,
  IN  UINTN        SigSize
  );

/**
  Retrieve the RSA Private Key from the password-protected PEM key data.

  @param[in]  PemData      Pointer to the PEM-encoded key data to be retrieved.
  @param[in]  PemSize      Size of the PEM key data in bytes.
  @param[in]  Password     NULL-terminated passphrase used for encrypted PEM key data.
  @param[out] RsaContext   Pointer to new-generated RSA context which contain the retrieved
                           RSA private key component. Use RsaFree() function to free the
                           resource.

  If PemData is NULL, then return FALSE.
  If RsaContext is NULL, then return FALSE.

  @retval  TRUE   RSA Private Key was retrieved successfully.
  @retval  FALSE  Invalid PEM key data or incorrect password.

**/
BOOLEAN
EFIAPI
RsaGetPrivateKeyFromPem (
  IN   CONST UINT8  *PemData,
  IN   UINTN        PemSize,
  IN   CONST CHAR8  *Password,
  OUT  VOID         **RsaContext
  );

/**
  Retrieve the RSA Public Key from one DER-encoded X509 certificate.

  @param[in]  Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]  CertSize     Size of the X509 certificate in bytes.
  @param[out] RsaContext   Pointer to new-generated RSA context which contain the retrieved
                           RSA public key component. Use RsaFree() function to free the
                           resource.

  If Cert is NULL, then return FALSE.
  If RsaContext is NULL, then return FALSE.

  @retval  TRUE   RSA Public Key was retrieved successfully.
  @retval  FALSE  Fail to retrieve RSA public key from X509 certificate.

**/
BOOLEAN
EFIAPI
RsaGetPublicKeyFromX509 (
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  VOID         **RsaContext
  );

/**
  Retrieve the subject bytes from one X.509 certificate.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[out]     CertSubject  Pointer to the retrieved certificate subject bytes.
  @param[in, out] SubjectSize  The size in bytes of the CertSubject buffer on input,
                               and the size of buffer returned CertSubject on output.

  If Cert is NULL, then return FALSE.
  If SubjectSize is NULL, then return FALSE.

  @retval  TRUE   The certificate subject retrieved successfully.
  @retval  FALSE  Invalid certificate, or the SubjectSize is too small for the result.
                  The SubjectSize will be updated with the required size.

**/
BOOLEAN
EFIAPI
X509GetSubjectName (
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     UINT8        *CertSubject,
  IN OUT  UINTN        *SubjectSize
  );

/**
  Verify one X509 certificate was issued by the trusted CA.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate to be verified.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[in]      CACert       Pointer to the DER-encoded trusted CA certificate.
  @param[in]      CACertSize   Size of the CA Certificate in bytes.

  If Cert is NULL, then return FALSE.
  If CACert is NULL, then return FALSE.

  @retval  TRUE   The certificate was issued by the trusted CA.
  @retval  FALSE  Invalid certificate or the certificate was not issued by the given
                  trusted CA.

**/
BOOLEAN
EFIAPI
X509VerifyCert (
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *CACert,
  IN  UINTN        CACertSize
  );

/**
  Construct a X509 object from DER-encoded certificate data.

  If Cert is NULL, then return FALSE.
  If SingleX509Cert is NULL, then return FALSE.

  @param[in]  Cert            Pointer to the DER-encoded certificate data.
  @param[in]  CertSize        The size of certificate data in bytes.
  @param[out] SingleX509Cert  The generated X509 object.

  @retval     TRUE            The X509 object generation succeeded.
  @retval     FALSE           The operation failed.

**/
BOOLEAN
EFIAPI
X509ConstructCertificate (
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  UINT8        **SingleX509Cert
  );

/**
  Construct a X509 stack object from a list of DER-encoded certificate data.

  If X509Stack is NULL, then return FALSE.

  @param[in, out]  X509Stack  On input, pointer to an existing X509 stack object.
                              On output, pointer to the X509 stack object with new
                              inserted X509 certificate.
  @param           ...        A list of DER-encoded single certificate data followed
                              by certificate size. A NULL terminates the list. The
                              pairs are the arguments to X509ConstructCertificate().
                                 
  @retval     TRUE            The X509 stack construction succeeded.
  @retval     FALSE           The construction operation failed.

**/
BOOLEAN
EFIAPI
X509ConstructCertificateStack (
  IN OUT  UINT8  **X509Stack,
  ...  
  );

/**
  Release the specified X509 object.

  If X509Cert is NULL, then return FALSE.

  @param[in]  X509Cert  Pointer to the X509 object to be released.

**/
VOID
EFIAPI
X509Free (
  IN  VOID  *X509Cert
  );

/**
  Release the specified X509 stack object.

  If X509Stack is NULL, then return FALSE.

  @param[in]  X509Stack  Pointer to the X509 stack object to be released.

**/
VOID
EFIAPI
X509StackFree (
  IN  VOID  *X509Stack
  );

/**
  Get the signer's certificates from PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard". The input signed data could be wrapped
  in a ContentInfo structure.

  If P7Data, CertStack, StackLength, TrustedCert or CertLength is NULL, then
  return FALSE. If P7Length overflow, then return FAlSE.

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[out] CertStack    Pointer to Signer's certificates retrieved from P7Data.
                           It's caller's responsiblity to free the buffer.
  @param[out] StackLength  Length of signer's certificates in bytes.
  @param[out] TrustedCert  Pointer to a trusted certificate from Signer's certificates.
                           It's caller's responsiblity to free the buffer.
  @param[out] CertLength   Length of the trusted certificate in bytes.

  @retval  TRUE            The operation is finished successfully.
  @retval  FALSE           Error occurs during the operation.

**/
BOOLEAN
EFIAPI
Pkcs7GetSigners (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT UINT8        **CertStack,
  OUT UINTN        *StackLength,
  OUT UINT8        **TrustedCert,
  OUT UINTN        *CertLength
  );

/**
  Wrap function to use free() to free allocated memory for certificates.

  @param[in]  Certs        Pointer to the certificates to be freed.

**/
VOID
EFIAPI
Pkcs7FreeSigners (
  IN  UINT8        *Certs
  );

/**
  Creates a PKCS#7 signedData as described in "PKCS #7: Cryptographic Message
  Syntax Standard, version 1.5". This interface is only intended to be used for
  application to perform PKCS#7 functionality validation.

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
  @param[out] SignedData       Pointer to output PKCS#7 signedData.
  @param[out] SignedDataSize   Size of SignedData in bytes.

  @retval     TRUE             PKCS#7 data signing succeeded.
  @retval     FALSE            PKCS#7 data signing failed.

**/
BOOLEAN
EFIAPI
Pkcs7Sign (
  IN   CONST UINT8  *PrivateKey,
  IN   UINTN        PrivateKeySize,
  IN   CONST UINT8  *KeyPassword,
  IN   UINT8        *InData,
  IN   UINTN        InDataSize,
  IN   UINT8        *SignCert,
  IN   UINT8        *OtherCerts      OPTIONAL,
  OUT  UINT8        **SignedData,
  OUT  UINTN        *SignedDataSize
  );

/**
  Verifies the validility of a PKCS#7 signed data as described in "PKCS #7:
  Cryptographic Message Syntax Standard". The input signed data could be wrapped
  in a ContentInfo structure.

  If P7Data, TrustedCert or InData is NULL, then return FALSE.
  If P7Length, CertLength or DataLength overflow, then return FAlSE.

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[in]  TrustedCert  Pointer to a trusted/root certificate encoded in DER, which
                           is used for certificate chain verification.
  @param[in]  CertLength   Length of the trusted certificate in bytes.
  @param[in]  InData       Pointer to the content to be verified.
  @param[in]  DataLength   Length of InData in bytes.

  @retval  TRUE  The specified PKCS#7 signed data is valid.
  @retval  FALSE Invalid PKCS#7 signed data.

**/
BOOLEAN
EFIAPI
Pkcs7Verify (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  IN  CONST UINT8  *TrustedCert,
  IN  UINTN        CertLength,
  IN  CONST UINT8  *InData,
  IN  UINTN        DataLength
  );

/**
  Verifies the validility of a PE/COFF Authenticode Signature as described in "Windows
  Authenticode Portable Executable Signature Format".

  If AuthData is NULL, then return FALSE.
  If ImageHash is NULL, then return FALSE.

  @param[in]  AuthData     Pointer to the Authenticode Signature retrieved from signed
                           PE/COFF image to be verified.
  @param[in]  DataSize     Size of the Authenticode Signature in bytes.
  @param[in]  TrustedCert  Pointer to a trusted/root certificate encoded in DER, which
                           is used for certificate chain verification.
  @param[in]  CertSize     Size of the trusted certificate in bytes.
  @param[in]  ImageHash    Pointer to the original image file hash value. The procudure
                           for calculating the image hash value is described in Authenticode
                           specification.
  @param[in]  HashSize     Size of Image hash value in bytes.

  @retval  TRUE   The specified Authenticode Signature is valid.
  @retval  FALSE  Invalid Authenticode Signature.

**/
BOOLEAN
EFIAPI
AuthenticodeVerify (
  IN  CONST UINT8  *AuthData,
  IN  UINTN        DataSize,
  IN  CONST UINT8  *TrustedCert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *ImageHash,
  IN  UINTN        HashSize
  );

//=====================================================================================
//    DH Key Exchange Primitive
//=====================================================================================

/**
  Allocates and Initializes one Diffie-Hellman Context for subsequent use.

  @return  Pointer to the Diffie-Hellman Context that has been initialized.
           If the allocations fails, DhNew() returns NULL.

**/
VOID *
EFIAPI
DhNew (
  VOID
  );

/**
  Release the specified DH context.

  If DhContext is NULL, then return FALSE.

  @param[in]  DhContext  Pointer to the DH context to be released.

**/
VOID
EFIAPI
DhFree (
  IN  VOID  *DhContext
  );

/**
  Generates DH parameter.

  Given generator g, and length of prime number p in bits, this function generates p,
  and sets DH context according to value of g and p.
  
  Before this function can be invoked, pseudorandom number generator must be correctly
  initialized by RandomSeed().

  If DhContext is NULL, then return FALSE.
  If Prime is NULL, then return FALSE.

  @param[in, out]  DhContext    Pointer to the DH context.
  @param[in]       Generator    Value of generator.
  @param[in]       PrimeLength  Length in bits of prime to be generated.
  @param[out]      Prime        Pointer to the buffer to receive the generated prime number.

  @retval TRUE   DH pamameter generation succeeded.
  @retval FALSE  Value of Generator is not supported.
  @retval FALSE  PRNG fails to generate random prime number with PrimeLength.

**/
BOOLEAN
EFIAPI
DhGenerateParameter (
  IN OUT  VOID   *DhContext,
  IN      UINTN  Generator,
  IN      UINTN  PrimeLength,
  OUT     UINT8  *Prime
  );

/**
  Sets generator and prime parameters for DH.

  Given generator g, and prime number p, this function and sets DH
  context accordingly.

  If DhContext is NULL, then return FALSE.
  If Prime is NULL, then return FALSE.

  @param[in, out]  DhContext    Pointer to the DH context.
  @param[in]       Generator    Value of generator.
  @param[in]       PrimeLength  Length in bits of prime to be generated.
  @param[in]       Prime        Pointer to the prime number.

  @retval TRUE   DH pamameter setting succeeded.
  @retval FALSE  Value of Generator is not supported.
  @retval FALSE  Value of Generator is not suitable for the Prime.
  @retval FALSE  Value of Prime is not a prime number.
  @retval FALSE  Value of Prime is not a safe prime number.

**/
BOOLEAN
EFIAPI
DhSetParameter (
  IN OUT  VOID         *DhContext,
  IN      UINTN        Generator,
  IN      UINTN        PrimeLength,
  IN      CONST UINT8  *Prime
  );

/**
  Generates DH public key.

  This function generates random secret exponent, and computes the public key, which is 
  returned via parameter PublicKey and PublicKeySize. DH context is updated accordingly.
  If the PublicKey buffer is too small to hold the public key, FALSE is returned and
  PublicKeySize is set to the required buffer size to obtain the public key.

  If DhContext is NULL, then return FALSE.
  If PublicKeySize is NULL, then return FALSE.
  If PublicKeySize is large enough but PublicKey is NULL, then return FALSE.

  @param[in, out]  DhContext      Pointer to the DH context.
  @param[out]      PublicKey      Pointer to the buffer to receive generated public key.
  @param[in, out]  PublicKeySize  On input, the size of PublicKey buffer in bytes.
                                 On output, the size of data returned in PublicKey buffer in bytes.

  @retval TRUE   DH public key generation succeeded.
  @retval FALSE  DH public key generation failed.
  @retval FALSE  PublicKeySize is not large enough.

**/
BOOLEAN
EFIAPI
DhGenerateKey (
  IN OUT  VOID   *DhContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  );

/**
  Computes exchanged common key.

  Given peer's public key, this function computes the exchanged common key, based on its own
  context including value of prime modulus and random secret exponent. 

  If DhContext is NULL, then return FALSE.
  If PeerPublicKey is NULL, then return FALSE.
  If KeySize is NULL, then return FALSE.
  If KeySize is large enough but Key is NULL, then return FALSE.

  @param[in, out]  DhContext          Pointer to the DH context.
  @param[in]       PeerPublicKey      Pointer to the peer's public key.
  @param[in]       PeerPublicKeySize  Size of peer's public key in bytes.
  @param[out]      Key                Pointer to the buffer to receive generated key.
  @param[in, out]  KeySize            On input, the size of Key buffer in bytes.
                                     On output, the size of data returned in Key buffer in bytes.

  @retval TRUE   DH exchanged key generation succeeded.
  @retval FALSE  DH exchanged key generation failed.
  @retval FALSE  KeySize is not large enough.

**/
BOOLEAN
EFIAPI
DhComputeKey (
  IN OUT  VOID         *DhContext,
  IN      CONST UINT8  *PeerPublicKey,
  IN      UINTN        PeerPublicKeySize,
  OUT     UINT8        *Key,
  IN OUT  UINTN        *KeySize
  );

//=====================================================================================
//    Pseudo-Random Generation Primitive
//=====================================================================================

/**
  Sets up the seed value for the pseudorandom number generator.

  This function sets up the seed value for the pseudorandom number generator.
  If Seed is not NULL, then the seed passed in is used.
  If Seed is NULL, then default seed is used.

  @param[in]  Seed      Pointer to seed value.
                        If NULL, default seed is used.
  @param[in]  SeedSize  Size of seed value.
                        If Seed is NULL, this parameter is ignored.

  @retval TRUE   Pseudorandom number generator has enough entropy for random generation.
  @retval FALSE  Pseudorandom number generator does not have enough entropy for random generation.

**/
BOOLEAN
EFIAPI
RandomSeed (
  IN  CONST  UINT8  *Seed  OPTIONAL,
  IN  UINTN         SeedSize
  );

/**
  Generates a pseudorandom byte stream of the specified size.

  If Output is NULL, then return FALSE.

  @param[out]  Output  Pointer to buffer to receive random value.
  @param[in]   Size    Size of randome bytes to generate.

  @retval TRUE   Pseudorandom byte stream generated successfully.
  @retval FALSE  Pseudorandom number generator fails to generate due to lack of entropy.

**/
BOOLEAN
EFIAPI
RandomBytes (
  OUT  UINT8  *Output,
  IN   UINTN  Size
  );

#endif // __BASE_CRYPT_LIB_H__

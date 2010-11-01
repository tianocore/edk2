/** @file
  Defines base cryptographic library APIs.
  The Base Cryptographic Library provides implementations of basic cryptography
  primitives (MD5, SHA-1, SHA-256, RSA, etc) for UEFI security functionality enabling.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
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

  If Md5Context is NULL, then ASSERT().

  @param[in, out]  Md5Context  Pointer to MD5 Context being initialized.

  @retval TRUE   MD5 context initialization succeeded.
  @retval FALSE  MD5 context initialization failed.

**/
BOOLEAN
EFIAPI
Md5Init (
  IN OUT  VOID  *Md5Context
  );


/**
  Performs MD5 digest on a data buffer of the specified length. This function can
  be called multiple times to compute the digest of long or discontinuous data streams.

  If Md5Context is NULL, then ASSERT().

  @param[in, out]  Md5Context  Pointer to the MD5 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataLength  Length of Data buffer in bytes.

  @retval TRUE   MD5 data digest succeeded.
  @retval FALSE  Invalid MD5 context. After Md5Final function has been called, the
                 MD5 context cannot be reused.

**/
BOOLEAN
EFIAPI
Md5Update (
  IN OUT  VOID        *Md5Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  );


/**
  Completes MD5 hash computation and retrieves the digest value into the specified
  memory. After this function has been called, the MD5 context cannot be used again.

  If Md5Context is NULL, then ASSERT().
  If HashValue is NULL, then ASSERT().

  @param[in, out]  Md5Context  Pointer to the MD5 context
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
  Initializes user-supplied memory pointed by Sha1Context as the SHA-1 hash context for
  subsequent use.

  If Sha1Context is NULL, then ASSERT().

  @param[in, out]  Sha1Context  Pointer to the SHA-1 Context being initialized.

  @retval TRUE   SHA-1 initialization succeeded.
  @retval FALSE  SHA-1 initialization failed.

**/
BOOLEAN
EFIAPI
Sha1Init (
  IN OUT  VOID  *Sha1Context
  );


/**
  Performs SHA-1 digest on a data buffer of the specified length. This function can
  be called multiple times to compute the digest of long or discontinuous data streams.

  If Sha1Context is NULL, then ASSERT().

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context.
  @param[in]       Data         Pointer to the buffer containing the data to be hashed.
  @param[in]       DataLength   Length of Data buffer in bytes.

  @retval TRUE   SHA-1 data digest succeeded.
  @retval FALSE  Invalid SHA-1 context. After Sha1Final function has been called, the
                 SHA-1 context cannot be reused.

**/
BOOLEAN
EFIAPI
Sha1Update (
  IN OUT  VOID        *Sha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  );


/**
  Completes SHA-1 hash computation and retrieves the digest value into the specified
  memory. After this function has been called, the SHA-1 context cannot be used again.

  If Sha1Context is NULL, then ASSERT().
  If HashValue is NULL, then ASSERT().

  @param[in, out]  Sha1Context  Pointer to the SHA-1 context
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
  Retrieves the size, in bytes, of the context buffer required for SHA-256 operations.

  @return  The size, in bytes, of the context buffer required for SHA-256 operations.

**/
UINTN
EFIAPI
Sha256GetContextSize (
  VOID
  );


/**
  Initializes user-supplied memory pointed by Sha256Context as SHA-256 hash context for
  subsequent use.

  If Sha256Context is NULL, then ASSERT().

  @param[in, out]  Sha256Context  Pointer to SHA-256 Context being initialized.

  @retval TRUE   SHA-256 context initialization succeeded.
  @retval FALSE  SHA-256 context initialization failed.

**/
BOOLEAN
EFIAPI
Sha256Init (
  IN OUT  VOID  *Sha256Context
  );


/**
  Performs SHA-256 digest on a data buffer of the specified length. This function can
  be called multiple times to compute the digest of long or discontinuous data streams.

  If Sha256Context is NULL, then ASSERT().

  @param[in, out]  Sha256Context  Pointer to the SHA-256 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataLength     Length of Data buffer in bytes.

  @retval TRUE   SHA-256 data digest succeeded.
  @retval FALSE  Invalid SHA-256 context. After Sha256Final function has been called, the
                 SHA-256 context cannot be reused.

**/
BOOLEAN
EFIAPI
Sha256Update (
  IN OUT  VOID        *Sha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  );


/**
  Completes SHA-256 hash computation and retrieves the digest value into the specified
  memory. After this function has been called, the SHA-256 context cannot be used again.

  If Sha256Context is NULL, then ASSERT().
  If HashValue is NULL, then ASSERT().

  @param[in, out]  Sha256Context  Pointer to SHA-256 context
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

///
/// No MAC supports for minimum scope required by UEFI
///


//=====================================================================================
//    Symmetric Cryptography Primitive
//=====================================================================================

///
/// No symmetric cryptographic supports for minimum scope required by UEFI
///


//=====================================================================================
//    Asymmetric Cryptography Primitive
//=====================================================================================

/**
  Allocates and Initializes one RSA Context for subsequent use.

  @return  Pointer to the RSA Context that has been initialized.
           If the allocations fails, RsaNew() returns NULL.

**/
VOID *
EFIAPI
RsaNew (
  VOID
  );


/**
  Release the specified RSA Context.

  @param[in]  RsaContext  Pointer to the RSA context to be released.

**/
VOID
EFIAPI
RsaFree (
  IN  VOID  *RsaContext
  );


/**
  Sets the tag-designated RSA key component into the established RSA context from
  the user-specified nonnegative integer (octet string format represented in RSA
  PKCS#1).

  If RsaContext is NULL, then ASSERT().

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[in]       BigNumber   Pointer to octet integer buffer.
  @param[in]       BnLength    Length of big number buffer in bytes.

  @return  TRUE   RSA key component was set successfully.
  @return  FALSE  Invalid RSA key component tag.

**/
BOOLEAN
EFIAPI
RsaSetKey (
  IN OUT VOID         *RsaContext,
  IN     RSA_KEY_TAG  KeyTag,
  IN     CONST UINT8  *BigNumber,
  IN     UINTN        BnLength
  );


/**
  Verifies the RSA-SSA signature with EMSA-PKCS1-v1_5 encoding scheme defined in
  RSA PKCS#1.

  If RsaContext is NULL, then ASSERT().
  If MessageHash is NULL, then ASSERT().
  If Signature is NULL, then ASSERT().
  If HashLength is not equal to the size of MD5, SHA-1 or SHA-256 digest, then ASSERT().

  @param[in]  RsaContext   Pointer to RSA context for signature verification.
  @param[in]  MessageHash  Pointer to octet message hash to be checked.
  @param[in]  HashLength   Length of the message hash in bytes.
  @param[in]  Signature    Pointer to RSA PKCS1-v1_5 signature to be verified.
  @param[in]  SigLength    Length of signature in bytes.

  @return  TRUE   Valid signature encoded in PKCS1-v1_5.
  @return  FALSE  Invalid signature or invalid RSA context.

**/
BOOLEAN
EFIAPI
RsaPkcs1Verify (
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *MessageHash,
  IN  UINTN        HashLength,
  IN  UINT8        *Signature,
  IN  UINTN        SigLength
  );


/**
  Verifies the validility of a PKCS#7 signed data as described in "PKCS #7: Cryptographic
  Message Syntax Standard".

  If P7Data is NULL, then ASSERT().

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[in]  TrustedCert  Pointer to a trusted/root certificate encoded in DER, which
                           is used for certificate chain verification.
  @param[in]  CertLength   Length of the trusted certificate in bytes.
  @param[in]  InData       Pointer to the content to be verified.
  @param[in]  DataLength   Length of InData in bytes.

  @return  TRUE  The specified PKCS#7 signed data is valid.
  @return  FALSE Invalid PKCS#7 signed data.

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


#endif // __BASE_CRYPT_LIB_H__

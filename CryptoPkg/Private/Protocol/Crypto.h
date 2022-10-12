/** @file
  This Protocol provides Crypto services to DXE modules

  Copyright (C) Microsoft Corporation. All rights reserved.
  Copyright (c) 2020 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EDKII_CRYPTO_PROTOCOL_H__
#define __EDKII_CRYPTO_PROTOCOL_H__

#include <Base.h>
#include <Library/BaseCryptLib.h>
#include <Library/PcdLib.h>

///
/// The version of the EDK II Crypto Protocol.
/// As APIs are added to BaseCryptLib, the EDK II Crypto Protocol is extended
/// with new APIs at the end of the EDK II Crypto Protocol structure.  Each time
/// the EDK II Crypto Protocol is extended, this version define must be
/// increased.
///
#define EDKII_CRYPTO_VERSION  16

///
/// EDK II Crypto Protocol forward declaration
///
typedef struct _EDKII_CRYPTO_PROTOCOL EDKII_CRYPTO_PROTOCOL;

/**
  Returns the version of the EDK II Crypto Protocol.

  @return  The version of the EDK II Crypto Protocol.

**/
typedef
UINTN
(EFIAPI *EDKII_CRYPTO_GET_VERSION)(
  VOID
  );

// =====================================================================================
//    MAC (Message Authentication Code) Primitive
// =====================================================================================

/**
  HMAC MD5 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

**/
typedef
VOID *
(EFIAPI *DEPRECATED_EDKII_CRYPTO_HMAC_MD5_NEW)(
  VOID
  );

typedef
VOID
(EFIAPI *DEPRECATED_EDKII_CRYPTO_HMAC_MD5_FREE)(
  IN  VOID  *HmacMd5Ctx
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_HMAC_MD5_SET_KEY)(
  OUT  VOID         *HmacMd5Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_HMAC_MD5_DUPLICATE)(
  IN   CONST VOID  *HmacMd5Context,
  OUT  VOID        *NewHmacMd5Context
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_HMAC_MD5_UPDATE)(
  IN OUT  VOID        *HmacMd5Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_HMAC_MD5_FINAL)(
  IN OUT  VOID   *HmacMd5Context,
  OUT     UINT8  *HmacValue
  );

/**
  HMAC SHA1 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

**/
typedef
VOID *
(EFIAPI *DEPRECATED_EDKII_CRYPTO_HMAC_SHA1_NEW)(
  VOID
  );

typedef
VOID
(EFIAPI *DEPRECATED_EDKII_CRYPTO_HMAC_SHA1_FREE)(
  IN  VOID  *HmacSha1Ctx
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_HMAC_SHA1_SET_KEY)(
  OUT  VOID         *HmacSha1Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_HMAC_SHA1_DUPLICATE)(
  IN   CONST VOID  *HmacSha1Context,
  OUT  VOID        *NewHmacSha1Context
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_HMAC_SHA1_UPDATE)(
  IN OUT  VOID        *HmacSha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_HMAC_SHA1_FINAL)(
  IN OUT  VOID   *HmacSha1Context,
  OUT     UINT8  *HmacValue
  );

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-SHA256 use.

  @return  Pointer to the HMAC_CTX context that has been initialized.
           If the allocations fails, HmacSha256New() returns NULL.

**/
typedef
VOID *
(EFIAPI *EDKII_CRYPTO_HMAC_SHA256_NEW)(
  VOID
  );

/**
  Release the specified HMAC_CTX context.

  @param[in]  HmacSha256Ctx  Pointer to the HMAC_CTX context to be released.

**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_HMAC_SHA256_FREE)(
  IN  VOID  *HmacSha256Ctx
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HMAC_SHA256_SET_KEY)(
  OUT  VOID         *HmacSha256Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HMAC_SHA256_DUPLICATE)(
  IN   CONST VOID  *HmacSha256Context,
  OUT  VOID        *NewHmacSha256Context
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HMAC_SHA256_UPDATE)(
  IN OUT  VOID        *HmacSha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HMAC_SHA256_FINAL)(
  IN OUT  VOID   *HmacSha256Context,
  OUT     UINT8  *HmacValue
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HMAC_SHA256_ALL)(
  IN   CONST VOID   *Data,
  IN   UINTN        DataSize,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  OUT  UINT8        *HmacValue
  );

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-SHA384 use.

  @return  Pointer to the HMAC_CTX context that has been initialized.
           If the allocations fails, HmacSha384New() returns NULL.

**/
typedef
VOID *
(EFIAPI *EDKII_CRYPTO_HMAC_SHA384_NEW)(
  VOID
  );

/**
  Release the specified HMAC_CTX context.

  @param[in]  HmacSha384Ctx  Pointer to the HMAC_CTX context to be released.

**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_HMAC_SHA384_FREE)(
  IN  VOID  *HmacSha384Ctx
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HMAC_SHA384_SET_KEY)(
  OUT  VOID         *HmacSha384Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HMAC_SHA384_DUPLICATE)(
  IN   CONST VOID  *HmacSha384Context,
  OUT  VOID        *NewHmacSha384Context
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HMAC_SHA384_UPDATE)(
  IN OUT  VOID        *HmacSha384Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HMAC_SHA384_FINAL)(
  IN OUT  VOID   *HmacSha384Context,
  OUT     UINT8  *HmacValue
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HMAC_SHA384_ALL)(
  IN   CONST VOID   *Data,
  IN   UINTN        DataSize,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  OUT  UINT8        *HmacValue
  );

// =====================================================================================
//    One-Way Cryptographic Hash Primitives
// =====================================================================================

/**
  MD4 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

**/
typedef
UINTN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_MD4_GET_CONTEXT_SIZE)(
  VOID
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_MD4_INIT)(
  OUT  VOID  *Md4Context
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_MD4_DUPLICATE)(
  IN   CONST VOID  *Md4Context,
  OUT  VOID        *NewMd4Context
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_MD4_UPDATE)(
  IN OUT  VOID        *Md4Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_MD4_FINAL)(
  IN OUT  VOID   *Md4Context,
  OUT     UINT8  *HashValue
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_MD4_HASH_ALL)(
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  );

// ----------------------------------------------------------------------------

/**
  Retrieves the size, in bytes, of the context buffer required for MD5 hash operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for MD5 hash operations.
  @retval  0   This interface is not supported.

**/
typedef
UINTN
(EFIAPI *EDKII_CRYPTO_MD5_GET_CONTEXT_SIZE)(
  VOID
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_MD5_INIT)(
  OUT VOID *Md5Context
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_MD5_DUPLICATE)(
  IN CONST VOID *Md5Context,
  OUT VOID *NewMd5Context
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_MD5_UPDATE)(
  IN OUT VOID *Md5Context,
  IN CONST VOID *Data,
  IN UINTN DataSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_MD5_FINAL)(
  IN OUT VOID *Md5Context,
  OUT UINT8 *HashValue
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_MD5_HASH_ALL)(
  IN CONST VOID *Data,
  IN UINTN DataSize,
  OUT UINT8 *HashValue
  );

// =====================================================================================
//    PKCS
// =====================================================================================

/**
  Encrypts a blob using PKCS1v2 (RSAES-OAEP) schema. On success, will return the encrypted message in
  in a newly allocated buffer.

  Things that can cause a failure include:
  - X509 key size does not match any known key size.
  - Fail to parse X509 certificate.
  - Fail to allocate an intermediate buffer.
  - NULL pointer provided for a non-optional parameter.
  - Data size is too large for the provided key size (max size is a function of key size and hash digest size).

  @param[in]  PublicKey     A pointer to the DER-encoded X509 certificate that will be used to encrypt the data.
  @param[in]  PublicKeySize Size of the X509 cert buffer.
  @param[in]  InData        Data to be encrypted.
  @param[in]  InDataSize    Size of the data buffer.
  @param[in]  PrngSeed      [Optional] If provided, a pointer to a random seed buffer to be used when initializing the PRNG. NULL otherwise.
  @param[in]  PrngSeedSize  [Optional] If provided, size of the random seed buffer. 0 otherwise.
  @param[out] EncryptedData       Pointer to an allocated buffer containing the encrypted message.
  @param[out] EncryptedDataSize   Size of the encrypted message buffer.

  @retval     TRUE  Encryption was successful.
  @retval     FALSE Encryption failed.

**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_PKCS1_ENCRYPT_V2)(
  IN   CONST UINT8                   *PublicKey,
  IN   UINTN                          PublicKeySize,
  IN   UINT8                         *InData,
  IN   UINTN                          InDataSize,
  IN   CONST UINT8                   *PrngSeed OPTIONAL,
  IN   UINTN                          PrngSeedSize OPTIONAL,
  OUT  UINT8                        **EncryptedData,
  OUT  UINTN                         *EncryptedDataSize
  );

// ---------------------------------------------
// PKCS5

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_PKCS5_PW_HASH)(
  IN UINTN                      PasswordSize,
  IN CONST  CHAR8              *Password,
  IN UINTN                      SaltSize,
  IN CONST  UINT8              *Salt,
  IN UINTN                      IterationCount,
  IN UINTN                      DigestSize,
  IN UINTN                      OutputSize,
  OUT UINT8                    *Output
  );

// ---------------------------------------------
// PKCS7

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_PKCS7_VERIFY)(
  IN  CONST UINT8                   *P7Data,
  IN  UINTN                          P7DataLength,
  IN  CONST UINT8                   *TrustedCert,
  IN  UINTN                          TrustedCertLength,
  IN  CONST UINT8                   *Data,
  IN  UINTN                          DataLength
  );

/**
  VerifyEKUsInPkcs7Signature()

  This function receives a PKCS7 formatted signature, and then verifies that
  the specified Enhanced or Extended Key Usages (EKU's) are present in the end-entity
  leaf signing certificate.

  Note that this function does not validate the certificate chain.

  Applications for custom EKU's are quite flexible.  For example, a policy EKU
  may be present in an Issuing Certificate Authority (CA), and any sub-ordinate
  certificate issued might also contain this EKU, thus constraining the
  sub-ordinate certificate.  Other applications might allow a certificate
  embedded in a device to specify that other Object Identifiers (OIDs) are
  present which contains binary data specifying custom capabilities that
  the device is able to do.

  @param[in]  Pkcs7Signature     - The PKCS#7 signed information content block. An array
                                   containing the content block with both the signature,
                                   the signer's certificate, and any necessary intermediate
                                   certificates.

  @param[in]  Pkcs7SignatureSize - Number of bytes in Pkcs7Signature.

  @param[in]  RequiredEKUs       - Array of null-terminated strings listing OIDs of
                                   required EKUs that must be present in the signature.

  @param[in]  RequiredEKUsSize   - Number of elements in the RequiredEKUs string array.

  @param[in]  RequireAllPresent  - If this is TRUE, then all of the specified EKU's
                                   must be present in the leaf signer.  If it is
                                   FALSE, then we will succeed if we find any
                                   of the specified EKU's.

  @retval EFI_SUCCESS            - The required EKUs were found in the signature.
  @retval EFI_INVALID_PARAMETER  - A parameter was invalid.
  @retval EFI_NOT_FOUND          - One or more EKU's were not found in the signature.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_PKCS7_VERIFY_EKU)(
  IN CONST UINT8                *Pkcs7Signature,
  IN CONST UINT32                SignatureSize,
  IN CONST CHAR8                *RequiredEKUs[],
  IN CONST UINT32                RequiredEKUsSize,
  IN BOOLEAN                     RequireAllPresent
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_PKCS7_GET_SIGNERS)(
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT UINT8        **CertStack,
  OUT UINTN        *StackLength,
  OUT UINT8        **TrustedCert,
  OUT UINTN        *CertLength
  );

/**
  Wrap function to use free() to free allocated memory for certificates.

  If this interface is not supported, then ASSERT().

  @param[in]  Certs        Pointer to the certificates to be freed.

**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_PKCS7_FREE_SIGNERS)(
  IN  UINT8        *Certs
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_PKCS7_SIGN)(
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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_PKCS7_GET_ATTACHED_CONTENT)(
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT VOID         **Content,
  OUT UINTN        *ContentSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_PKCS7_GET_CERTIFICATES_LIST)(
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT UINT8        **SignerChainCerts,
  OUT UINTN        *ChainLength,
  OUT UINT8        **UnchainCerts,
  OUT UINTN        *UnchainLength
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_AUTHENTICODE_VERIFY)(
  IN  CONST UINT8  *AuthData,
  IN  UINTN        DataSize,
  IN  CONST UINT8  *TrustedCert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *ImageHash,
  IN  UINTN        HashSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_IMAGE_TIMESTAMP_VERIFY)(
  IN  CONST UINT8  *AuthData,
  IN  UINTN        DataSize,
  IN  CONST UINT8  *TsaCert,
  IN  UINTN        CertSize,
  OUT EFI_TIME     *SigningTime
  );

// =====================================================================================
//    DH Key Exchange Primitive
// =====================================================================================

/**
  Allocates and Initializes one Diffie-Hellman Context for subsequent use.

  @return  Pointer to the Diffie-Hellman Context that has been initialized.
           If the allocations fails, DhNew() returns NULL.
           If the interface is not supported, DhNew() returns NULL.

**/
typedef
VOID *
(EFIAPI *EDKII_CRYPTO_DH_NEW)(
  VOID
  );

/**
  Release the specified DH context.

  If the interface is not supported, then ASSERT().

  @param[in]  DhContext  Pointer to the DH context to be released.

**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_DH_FREE)(
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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_DH_GENERATE_PARAMETER)(
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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_DH_SET_PARAMETER)(
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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_DH_GENERATE_KEY)(
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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_DH_COMPUTE_KEY)(
  IN OUT  VOID         *DhContext,
  IN      CONST UINT8  *PeerPublicKey,
  IN      UINTN        PeerPublicKeySize,
  OUT     UINT8        *Key,
  IN OUT  UINTN        *KeySize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_RANDOM_SEED)(
  IN  CONST  UINT8  *Seed  OPTIONAL,
  IN  UINTN         SeedSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_RANDOM_BYTES)(
  OUT  UINT8  *Output,
  IN   UINTN  Size
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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_RSA_VERIFY_PKCS1)(
  IN  VOID                         *RsaContext,
  IN  CONST UINT8                  *MessageHash,
  IN  UINTN                         HashSize,
  IN  CONST UINT8                  *Signature,
  IN  UINTN                         SigSize
  );

/**
  Allocates and initializes one RSA context for subsequent use.

  @return  Pointer to the RSA context that has been initialized.
           If the allocations fails, RsaNew() returns NULL.

**/
typedef
VOID *
(EFIAPI *EDKII_CRYPTO_RSA_NEW)(
  VOID
  );

/**
  Release the specified RSA context.

  If RsaContext is NULL, then return FALSE.

  @param[in]  RsaContext  Pointer to the RSA context to be released.

**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_RSA_FREE)(
  IN  VOID  *RsaContext
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_RSA_SET_KEY)(
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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_RSA_GET_KEY)(
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
  If this interface is not supported, then return FALSE.

  @param[in, out]  RsaContext           Pointer to RSA context being set.
  @param[in]       ModulusLength        Length of RSA modulus N in bits.
  @param[in]       PublicExponent       Pointer to RSA public exponent.
  @param[in]       PublicExponentSize   Size of RSA public exponent buffer in bytes.

  @retval  TRUE   RSA key component was generated successfully.
  @retval  FALSE  Invalid RSA key component tag.
  @retval  FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_RSA_GENERATE_KEY)(
  IN OUT  VOID         *RsaContext,
  IN      UINTN        ModulusLength,
  IN      CONST UINT8  *PublicExponent,
  IN      UINTN        PublicExponentSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_RSA_CHECK_KEY)(
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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_RSA_PKCS1_SIGN)(
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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_RSA_PKCS1_VERIFY)(
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *MessageHash,
  IN  UINTN        HashSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_RSA_GET_PRIVATE_KEY_FROM_PEM)(
  IN   CONST UINT8  *PemData,
  IN   UINTN        PemSize,
  IN   CONST CHAR8  *Password,
  OUT  VOID         **RsaContext
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_RSA_GET_PUBLIC_KEY_FROM_X509)(
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  VOID         **RsaContext
  );

// ----------------------------------------
// SHA
// ----------------------------------------

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-1 hash operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for SHA-1 hash operations.
  @retval  0   This interface is not supported.

**/
typedef
UINTN
(EFIAPI *EDKII_CRYPTO_SHA1_GET_CONTEXT_SIZE)(
  VOID
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA1_INIT)(
  OUT  VOID  *Sha1Context
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA1_DUPLICATE)(
  IN   CONST VOID  *Sha1Context,
  OUT  VOID        *NewSha1Context
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA1_UPDATE)(
  IN OUT  VOID        *Sha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA1_FINAL)(
  IN OUT  VOID   *Sha1Context,
  OUT     UINT8  *HashValue
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA1_HASH_ALL)(
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  );

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-256 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-256 hash operations.

**/
typedef
UINTN
(EFIAPI *EDKII_CRYPTO_SHA256_GET_CONTEXT_SIZE)(
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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA256_INIT)(
  OUT  VOID  *Sha256Context
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA256_DUPLICATE)(
  IN   CONST VOID  *Sha256Context,
  OUT  VOID        *NewSha256Context
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA256_UPDATE)(
  IN OUT  VOID        *Sha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA256_FINAL)(
  IN OUT  VOID   *Sha256Context,
  OUT     UINT8  *HashValue
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA256_HASH_ALL)(
  IN   CONST VOID                  *Data,
  IN   UINTN                       DataSize,
  OUT  UINT8                       *HashValue
  );

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-384 hash operations.
  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for SHA-384 hash operations.
  @retval  0   This interface is not supported.

**/
typedef
UINTN
(EFIAPI *EDKII_CRYPTO_SHA384_GET_CONTEXT_SIZE)(
  VOID
  );

/**
  Initializes user-supplied memory pointed by Sha384Context as SHA-384 hash context for
  subsequent use.

  If Sha384Context is NULL, then return FALSE.

  @param[out]  Sha384Context  Pointer to SHA-384 context being initialized.

  @retval TRUE   SHA-384 context initialization succeeded.
  @retval FALSE  SHA-384 context initialization failed.

**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA384_INIT)(
  OUT  VOID  *Sha384Context
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA384_DUPLICATE)(
  IN   CONST VOID  *Sha384Context,
  OUT  VOID        *NewSha384Context
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA384_UPDATE)(
  IN OUT  VOID        *Sha384Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA384_FINAL)(
  IN OUT  VOID   *Sha384Context,
  OUT     UINT8  *HashValue
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA384_HASH_ALL)(
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  );

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-512 hash operations.

  @return  The size, in bytes, of the context buffer required for SHA-512 hash operations.

**/
typedef
UINTN
(EFIAPI *EDKII_CRYPTO_SHA512_GET_CONTEXT_SIZE)(
  VOID
  );

/**
  Initializes user-supplied memory pointed by Sha512Context as SHA-512 hash context for
  subsequent use.

  If Sha512Context is NULL, then return FALSE.

  @param[out]  Sha512Context  Pointer to SHA-512 context being initialized.

  @retval TRUE   SHA-512 context initialization succeeded.
  @retval FALSE  SHA-512 context initialization failed.

**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA512_INIT)(
  OUT  VOID  *Sha512Context
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA512_DUPLICATE)(
  IN   CONST VOID  *Sha512Context,
  OUT  VOID        *NewSha512Context
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA512_UPDATE)(
  IN OUT  VOID        *Sha512Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA512_FINAL)(
  IN OUT  VOID   *Sha512Context,
  OUT     UINT8  *HashValue
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SHA512_HASH_ALL)(
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  );

// ----------------------------------------------------------------------------
// X509
// ----------------------------------------------------------------------------

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_GET_SUBJECT_NAME)(
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     UINT8        *CertSubject,
  IN OUT  UINTN        *SubjectSize
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_X509_GET_COMMON_NAME)(
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     CHAR8        *CommonName   OPTIONAL,
  IN OUT  UINTN        *CommonNameSize
  );

/**
  Retrieve the organization name (O) string from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     NameBuffer       Buffer to contain the retrieved certificate organization
                                   name string. At most NameBufferSize bytes will be
                                   written and the string will be null terminated. May be
                                   NULL in order to determine the size buffer needed.
  @param[in,out]  NameBufferSiz e  The size in bytes of the Name buffer on input,
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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_X509_GET_ORGANIZATION_NAME)(
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     CHAR8        *NameBuffer   OPTIONAL,
  IN OUT  UINTN        *NameBufferSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_VERIFY_CERT)(
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *CACert,
  IN  UINTN        CACertSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_CONSTRUCT_CERTIFICATE)(
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  UINT8        **SingleX509Cert
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_CONSTRUCT_CERTIFICATE_STACK)(
  IN OUT  UINT8  **X509Stack,
  ...
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_CONSTRUCT_CERTIFICATE_STACK_V)(
  IN OUT  UINT8    **X509Stack,
  IN      VA_LIST  Args
  );

/**
  Release the specified X509 object.

  If the interface is not supported, then ASSERT().

  @param[in]  X509Cert  Pointer to the X509 object to be released.

**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_X509_FREE)(
  IN  VOID  *X509Cert
  );

/**
  Release the specified X509 stack object.

  If the interface is not supported, then ASSERT().

  @param[in]  X509Stack  Pointer to the X509 stack object to be released.

**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_X509_STACK_FREE)(
  IN  VOID  *X509Stack
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_GET_TBS_CERT)(
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  OUT UINT8        **TBSCert,
  OUT UINTN        *TBSCertSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_GET_VERSION)(
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     UINTN        *Version
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_GET_SERIAL_NUMBER)(
  IN      CONST UINT8 *Cert,
  IN      UINTN CertSize,
  OUT     UINT8 *SerialNumber, OPTIONAL
  IN OUT  UINTN         *SerialNumberSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_GET_ISSUER_NAME)(
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     UINT8        *CertIssuer,
  IN OUT  UINTN        *CertIssuerSize
  );

/**
  Retrieve the Signature Algorithm from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     Oid              Signature Algorithm Object identifier buffer.
  @param[in,out]  OidSize          Signature Algorithm Object identifier buffer size

  @retval TRUE           The certificate Extension data retrieved successfully.
  @retval FALSE                    If Cert is NULL.
                                   If OidSize is NULL.
                                   If Oid is not NULL and *OidSize is 0.
                                   If Certificate is invalid.
  @retval FALSE                    If no SignatureType.
  @retval FALSE                    If the Oid is NULL. The required buffer size
                                   is returned in the OidSize.
  @retval FALSE                    The operation is not supported.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_GET_SIGNATURE_ALGORITHM)(
  IN CONST UINT8 *Cert,
  IN       UINTN CertSize,
  OUT   UINT8 *Oid, OPTIONAL
  IN OUT   UINTN       *OidSize
  );

/**
  Retrieve Extension data from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[in]      Oid              Object identifier buffer
  @param[in]      OidSize          Object identifier buffer size
  @param[out]     ExtensionData    Extension bytes.
  @param[in, out] ExtensionDataSize Extension bytes size.

  @retval TRUE                     The certificate Extension data retrieved successfully.
  @retval FALSE                    If Cert is NULL.
                                   If ExtensionDataSize is NULL.
                                   If ExtensionData is not NULL and *ExtensionDataSize is 0.
                                   If Certificate is invalid.
  @retval FALSE                    If no Extension entry match Oid.
  @retval FALSE                    If the ExtensionData is NULL. The required buffer size
                                   is returned in the ExtensionDataSize parameter.
  @retval FALSE                    The operation is not supported.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_GET_EXTENSION_DATA)(
  IN     CONST UINT8  *Cert,
  IN     UINTN        CertSize,
  IN     CONST UINT8  *Oid,
  IN     UINTN        OidSize,
  OUT UINT8           *ExtensionData,
  IN OUT UINTN        *ExtensionDataSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_GET_EXTENDED_KEY_USAGE)(
  IN     CONST UINT8  *Cert,
  IN     UINTN        CertSize,
  OUT UINT8           *Usage,
  IN OUT UINTN        *UsageSize
  );

/**
  Retrieve the Validity from one X.509 certificate

  If Cert is NULL, then return FALSE.
  If CertIssuerSize is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[out]     From         notBefore Pointer to DateTime object.
  @param[in,out]  FromSize     notBefore DateTime object size.
  @param[out]     To           notAfter Pointer to DateTime object.
  @param[in,out]  ToSize       notAfter DateTime object size.

  Note: X509CompareDateTime to compare DateTime oject
        x509SetDateTime to get a DateTime object from a DateTimeStr

  @retval  TRUE   The certificate Validity retrieved successfully.
  @retval  FALSE  Invalid certificate, or Validity retrieve failed.
  @retval  FALSE  This interface is not supported.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_GET_VALIDITY)(
  IN     CONST UINT8  *Cert,
  IN     UINTN        CertSize,
  IN     UINT8        *From,
  IN OUT UINTN        *FromSize,
  IN     UINT8        *To,
  IN OUT UINTN        *ToSize
  );

/**
  Format a DateTimeStr to DataTime object in DataTime Buffer

  If DateTimeStr is NULL, then return FALSE.
  If DateTimeSize is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      DateTimeStr      DateTime string like YYYYMMDDhhmmssZ
                                   Ref: https://www.w3.org/TR/NOTE-datetime
                                   Z stand for UTC time
  @param[in,out]  DateTime         Pointer to a DateTime object.
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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_FORMAT_DATE_TIME)(
  IN CONST CHAR8  *DateTimeStr,
  OUT VOID      *DateTime,
  IN OUT UINTN  *DateTimeSize
  );

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
typedef
INT32
(EFIAPI *EDKII_CRYPTO_X509_COMPARE_DATE_TIME)(
  IN CONST  VOID  *DateTime1,
  IN CONST  VOID  *DateTime2
  );

/**
  Retrieve the Key Usage from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     Usage            Key Usage (CRYPTO_X509_KU_*)

  @retval  TRUE   The certificate Key Usage retrieved successfully.
  @retval  FALSE  Invalid certificate, or Usage is NULL
  @retval  FALSE  This interface is not supported.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_GET_KEY_USAGE)(
  IN    CONST UINT8  *Cert,
  IN    UINTN        CertSize,
  OUT   UINTN        *Usage
  );

/**
  Verify one X509 certificate was issued by the trusted CA.

  @param[in]      CertChain         One or more ASN.1 DER-encoded X.509 certificates
                                    where the first certificate is signed by the Root
                                    Certificate or is the Root Cerificate itself. and
                                    subsequent cerificate is signed by the preceding
                                    cerificate.
  @param[in]      CertChainLength   Total length of the certificate chain, in bytes.

  @param[in]      RootCert          Trusted Root Certificate buffer

  @param[in]      RootCertLength    Trusted Root Certificate buffer length

  @retval  TRUE   All cerificates was issued by the first certificate in X509Certchain.
  @retval  FALSE  Invalid certificate or the certificate was not issued by the given
                  trusted CA.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_VERIFY_CERT_CHAIN)(
  IN CONST UINT8  *RootCert,
  IN UINTN        RootCertLength,
  IN CONST UINT8  *CertChain,
  IN UINTN        CertChainLength
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_GET_CERT_FROM_CERT_CHAIN)(
  IN CONST UINT8   *CertChain,
  IN UINTN         CertChainLength,
  IN CONST INT32   CertIndex,
  OUT CONST UINT8  **Cert,
  OUT UINTN        *CertLength
  );

/**
  Retrieve the tag and length of the tag.

  @param Ptr      The position in the ASN.1 data
  @param End      End of data
  @param Length   The variable that will receive the length
  @param Tag      The expected tag

  @retval      TRUE   Get tag successful
  @retval      FALSe  Failed to get tag or tag not match
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_ASN1_GET_TAG)(
  IN OUT UINT8   **Ptr,
  IN CONST UINT8   *End,
  OUT UINTN      *Length,
  IN     UINT32  Tag
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_X509_GET_EXTENDED_BASIC_CONSTRAINTS)(
  CONST UINT8  *Cert,
  UINTN        CertSize,
  UINT8        *BasicConstraints,
  UINTN        *BasicConstraintsSize
  );

// =====================================================================================
//    Symmetric Cryptography Primitive
// =====================================================================================

/**
  TDES is deprecated and unsupported any longer.
  Keep the function field for binary compability.

**/
typedef
UINTN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_TDES_GET_CONTEXT_SIZE)(
  VOID
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_TDES_INIT)(
  OUT  VOID         *TdesContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_TDES_ECB_ENCRYPT)(
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_TDES_ECB_DECRYPT)(
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_TDES_CBC_ENCRYPT)(
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_TDES_CBC_DECRYPT)(
  IN   VOID         *TdesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  );

/**
  Retrieves the size, in bytes, of the context buffer required for AES operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for AES operations.
  @retval  0   This interface is not supported.

**/
typedef
UINTN
(EFIAPI *EDKII_CRYPTO_AES_GET_CONTEXT_SIZE)(
  VOID
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_AES_INIT)(
  OUT  VOID         *AesContext,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeyLength
  );

/**
  AES ECB Mode is deprecated and unsupported any longer.
  Keep the function field for binary compability.

**/
typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_AES_ECB_ENCRYPT)(
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  OUT  UINT8        *Output
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_AES_ECB_DECRYPT)(
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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_AES_CBC_ENCRYPT)(
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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_AES_CBC_DECRYPT)(
  IN   VOID         *AesContext,
  IN   CONST UINT8  *Input,
  IN   UINTN        InputSize,
  IN   CONST UINT8  *Ivec,
  OUT  UINT8        *Output
  );

/**
  ARC4 is deprecated and unsupported any longer.
  Keep the function field for binary compability.

**/
typedef
UINTN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_ARC4_GET_CONTEXT_SIZE)(
  VOID
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_ARC4_INIT)(
  OUT  VOID         *Arc4Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_ARC4_ENCRYPT)(
  IN OUT  VOID         *Arc4Context,
  IN      CONST UINT8  *Input,
  IN      UINTN        InputSize,
  OUT     UINT8        *Output
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_ARC4_DECRYPT)(
  IN OUT  VOID   *Arc4Context,
  IN      UINT8  *Input,
  IN      UINTN  InputSize,
  OUT     UINT8  *Output
  );

typedef
BOOLEAN
(EFIAPI *DEPRECATED_EDKII_CRYPTO_ARC4_RESET)(
  IN OUT  VOID  *Arc4Context
  );

/**
  Retrieves the size, in bytes, of the context buffer required for SM3 hash operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for SM3 hash operations.
  @retval  0   This interface is not supported.

**/
typedef
UINTN
(EFIAPI *EDKII_CRYPTO_SM3_GET_CONTEXT_SIZE)(
  VOID
  );

/**
  Initializes user-supplied memory pointed by Sm3Context as SM3 hash context for
  subsequent use.

  If Sm3Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  Sm3Context  Pointer to SM3 context being initialized.

  @retval TRUE   SM3 context initialization succeeded.
  @retval FALSE  SM3 context initialization failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SM3_INIT)(
  OUT VOID *Sm3Context
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SM3_DUPLICATE)(
  IN CONST VOID *Sm3Context,
  OUT VOID *NewSm3Context
  );

/**
  Digests the input data and updates SM3 context.

  This function performs SM3 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SM3 context should be already correctly initialized by Sm3Init(), and should not be finalized
  by Sm3Final(). Behavior with invalid context is undefined.

  If Sm3Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Sm3Context  Pointer to the SM3 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize    Size of Data buffer in bytes.

  @retval TRUE   SM3 data digest succeeded.
  @retval FALSE  SM3 data digest failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SM3_UPDATE)(
  IN OUT VOID *Sm3Context,
  IN CONST VOID *Data,
  IN UINTN DataSize
  );

/**
  Completes computation of the SM3 digest value.

  This function completes SM3 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SM3 context cannot
  be used again.
  SM3 context should be already correctly initialized by Sm3Init(), and should not be
  finalized by Sm3Final(). Behavior with invalid SM3 context is undefined.

  If Sm3Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  Sm3Context  Pointer to the SM3 context.
  @param[out]      HashValue   Pointer to a buffer that receives the SM3 digest
                               value (16 bytes).

  @retval TRUE   SM3 digest computation succeeded.
  @retval FALSE  SM3 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SM3_FINAL)(
  IN OUT VOID *Sm3Context,
  OUT UINT8 *HashValue
  );

/**
  Computes the SM3 message digest of a input data buffer.

  This function performs the SM3 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SM3 digest
                           value (16 bytes).

  @retval TRUE   SM3 digest computation succeeded.
  @retval FALSE  SM3 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_SM3_HASH_ALL)(
  IN CONST VOID *Data,
  IN UINTN DataSize,
  OUT UINT8 *HashValue
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HKDF_SHA_256_EXTRACT_AND_EXPAND)(
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Salt,
  IN   UINTN        SaltSize,
  IN   CONST UINT8  *Info,
  IN   UINTN        InfoSize,
  OUT  UINT8        *Out,
  IN   UINTN        OutSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HKDF_SHA_256_EXTRACT)(
  IN CONST UINT8  *Key,
  IN UINTN        KeySize,
  IN CONST UINT8  *Salt,
  IN UINTN        SaltSize,
  OUT UINT8       *PrkOut,
  UINTN           PrkOutSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HKDF_SHA_256_EXPAND)(
  IN   CONST UINT8  *Prk,
  IN   UINTN        PrkSize,
  IN   CONST UINT8  *Info,
  IN   UINTN        InfoSize,
  OUT  UINT8        *Out,
  IN   UINTN        OutSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HKDF_SHA_384_EXTRACT_AND_EXPAND)(
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize,
  IN   CONST UINT8  *Salt,
  IN   UINTN        SaltSize,
  IN   CONST UINT8  *Info,
  IN   UINTN        InfoSize,
  OUT  UINT8        *Out,
  IN   UINTN        OutSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HKDF_SHA_384_EXTRACT)(
  IN CONST UINT8  *Key,
  IN UINTN        KeySize,
  IN CONST UINT8  *Salt,
  IN UINTN        SaltSize,
  OUT UINT8       *PrkOut,
  UINTN           PrkOutSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_HKDF_SHA_384_EXPAND)(
  IN   CONST UINT8  *Prk,
  IN   UINTN        PrkSize,
  IN   CONST UINT8  *Info,
  IN   UINTN        InfoSize,
  OUT  UINT8        *Out,
  IN   UINTN        OutSize
  );

/**
  Initializes the OpenSSL library.

  This function registers ciphers and digests used directly and indirectly
  by SSL/TLS, and initializes the readable error messages.
  This function must be called before any other action takes places.

  @retval TRUE   The OpenSSL library has been initialized.
  @retval FALSE  Failed to initialize the OpenSSL library.

**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_TLS_INITIALIZE)(
  VOID
  );

/**
  Free an allocated SSL_CTX object.

  @param[in]  TlsCtx    Pointer to the SSL_CTX object to be released.

**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_TLS_CTX_FREE)(
  IN   VOID                  *TlsCtx
  );

/**
  Creates a new SSL_CTX object as framework to establish TLS/SSL enabled
  connections.

  @param[in]  MajorVer    Major Version of TLS/SSL Protocol.
  @param[in]  MinorVer    Minor Version of TLS/SSL Protocol.

  @return  Pointer to an allocated SSL_CTX object.
           If the creation failed, TlsCtxNew() returns NULL.

**/
typedef
VOID *
(EFIAPI *EDKII_CRYPTO_TLS_CTX_NEW)(
  IN     UINT8                    MajorVer,
  IN     UINT8                    MinorVer
  );

/**
  Free an allocated TLS object.

  This function removes the TLS object pointed to by Tls and frees up the
  allocated memory. If Tls is NULL, nothing is done.

  @param[in]  Tls    Pointer to the TLS object to be freed.

**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_TLS_FREE)(
  IN     VOID                     *Tls
  );

/**
  Create a new TLS object for a connection.

  This function creates a new TLS object for a connection. The new object
  inherits the setting of the underlying context TlsCtx: connection method,
  options, verification setting.

  @param[in]  TlsCtx    Pointer to the SSL_CTX object.

  @return  Pointer to an allocated SSL object.
           If the creation failed, TlsNew() returns NULL.

**/
typedef
VOID *
(EFIAPI *EDKII_CRYPTO_TLS_NEW)(
  IN     VOID                     *TlsCtx
  );

/**
  Checks if the TLS handshake was done.

  This function will check if the specified TLS handshake was done.

  @param[in]  Tls    Pointer to the TLS object for handshake state checking.

  @retval  TRUE     The TLS handshake was done.
  @retval  FALSE    The TLS handshake was not done.

**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_TLS_IN_HANDSHAKE)(
  IN     VOID                     *Tls
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_DO_HANDSHAKE)(
  IN     VOID                     *Tls,
  IN     UINT8                    *BufferIn  OPTIONAL,
  IN     UINTN                    BufferInSize  OPTIONAL,
  OUT UINT8                    *BufferOut  OPTIONAL,
  IN OUT UINTN                    *BufferOutSize
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_HANDLE_ALERT)(
  IN     VOID                     *Tls,
  IN     UINT8                    *BufferIn  OPTIONAL,
  IN     UINTN                    BufferInSize  OPTIONAL,
  OUT UINT8                    *BufferOut  OPTIONAL,
  IN OUT UINTN                    *BufferOutSize
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_CLOSE_NOTIFY)(
  IN     VOID                     *Tls,
  IN OUT UINT8                    *Buffer,
  IN OUT UINTN                    *BufferSize
  );

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
typedef
INTN
(EFIAPI *EDKII_CRYPTO_TLS_CTRL_TRAFFIC_OUT)(
  IN     VOID                     *Tls,
  IN OUT VOID                     *Buffer,
  IN     UINTN                    BufferSize
  );

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
typedef
INTN
(EFIAPI *EDKII_CRYPTO_TLS_CTRL_TRAFFIC_IN)(
  IN     VOID                     *Tls,
  IN     VOID                     *Buffer,
  IN     UINTN                    BufferSize
  );

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
typedef
INTN
(EFIAPI *EDKII_CRYPTO_TLS_READ)(
  IN     VOID                     *Tls,
  IN OUT VOID                     *Buffer,
  IN     UINTN                    BufferSize
  );

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
typedef
INTN
(EFIAPI *EDKII_CRYPTO_TLS_WRITE)(
  IN     VOID                     *Tls,
  IN     VOID                     *Buffer,
  IN     UINTN                    BufferSize
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SHUTDOWN)(
  IN     VOID                     *Tls
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SET_VERSION)(
  IN     VOID                     *Tls,
  IN     UINT8                    MajorVer,
  IN     UINT8                    MinorVer
  );

/**
  Set TLS object to work in client or server mode.

  This function prepares a TLS object to work in client or server mode.

  @param[in]  Tls         Pointer to a TLS object.
  @param[in]  IsServer    Work in server mode.

  @retval  EFI_SUCCESS           The TLS/SSL work mode was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       Unsupported TLS/SSL work mode.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SET_CONNECTION_END)(
  IN     VOID                     *Tls,
  IN     BOOLEAN                  IsServer
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SET_CIPHER_LIST)(
  IN     VOID                     *Tls,
  IN     UINT16                   *CipherId,
  IN     UINTN                    CipherNum
  );

/**
  Set the compression method for TLS/SSL operations.

  This function handles TLS/SSL integrated compression methods.

  @param[in]  CompMethod    The compression method ID.

  @retval  EFI_SUCCESS        The compression method for the communication was
                              set successfully.
  @retval  EFI_UNSUPPORTED    Unsupported compression method.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SET_COMPRESSION_METHOD)(
  IN     UINT8                    CompMethod
  );

/**
  Set peer certificate verification mode for the TLS connection.

  This function sets the verification mode flags for the TLS connection.

  @param[in]  Tls           Pointer to the TLS object.
  @param[in]  VerifyMode    A set of logically or'ed verification mode flags.

**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_TLS_SET_VERIFY)(
  IN     VOID                     *Tls,
  IN     UINT32                   VerifyMode
  );

/**
  Set the specified host name to be verified.

  @param[in]  Tls           Pointer to the TLS object.
  @param[in]  Flags         The setting flags during the validation.
  @param[in]  HostName      The specified host name to be verified.

  @retval  EFI_SUCCESS           The HostName setting was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_ABORTED           Invalid HostName setting.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SET_VERIFY_HOST)(
  IN     VOID                     *Tls,
  IN     UINT32                   Flags,
  IN     CHAR8                    *HostName
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SET_SESSIONID)(
  IN     VOID                     *Tls,
  IN     UINT8                    *SessionId,
  IN     UINT16                   SessionIdLen
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SET_CA_CERTIFICATE)(
  IN     VOID                     *Tls,
  IN     VOID                     *Data,
  IN     UINTN                    DataSize
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SET_HOST_PUBLIC_CERT)(
  IN     VOID                     *Tls,
  IN     VOID                     *Data,
  IN     UINTN                    DataSize
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SET_HOST_PRIVATE_KEY_EX)(
  IN     VOID                     *Tls,
  IN     VOID                     *Data,
  IN     UINTN                    DataSize,
  IN     VOID                     *Password  OPTIONAL
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SET_HOST_PRIVATE_KEY)(
  IN     VOID                     *Tls,
  IN     VOID                     *Data,
  IN     UINTN                    DataSize
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SET_CERT_REVOCATION_LIST)(
  IN     VOID                     *Data,
  IN     UINTN                    DataSize
  );

/**
  Gets the protocol version used by the specified TLS connection.

  This function returns the protocol version used by the specified TLS
  connection.

  If Tls is NULL, then ASSERT().

  @param[in]  Tls    Pointer to the TLS object.

  @return  The protocol version of the specified TLS connection.

**/
typedef
UINT16
(EFIAPI *EDKII_CRYPTO_TLS_GET_VERSION)(
  IN     VOID                     *Tls
  );

/**
  Gets the connection end of the specified TLS connection.

  This function returns the connection end (as client or as server) used by
  the specified TLS connection.

  If Tls is NULL, then ASSERT().

  @param[in]  Tls    Pointer to the TLS object.

  @return  The connection end used by the specified TLS connection.

**/
typedef
UINT8
(EFIAPI *EDKII_CRYPTO_TLS_GET_CONNECTION_END)(
  IN     VOID                     *Tls
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_GET_CURRENT_CIPHER)(
  IN     VOID                     *Tls,
  IN OUT UINT16                   *CipherId
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_GET_CURRENT_COMPRESSION_ID)(
  IN     VOID                     *Tls,
  IN OUT UINT8                    *CompressionId
  );

/**
  Gets the verification mode currently set in the TLS connection.

  This function returns the peer verification mode currently set in the
  specified TLS connection.

  If Tls is NULL, then ASSERT().

  @param[in]  Tls    Pointer to the TLS object.

  @return  The verification mode set in the specified TLS connection.

**/
typedef
UINT32
(EFIAPI *EDKII_CRYPTO_TLS_GET_VERIFY)(
  IN     VOID                     *Tls
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_GET_SESSION_ID)(
  IN     VOID                     *Tls,
  IN OUT UINT8                    *SessionId,
  IN OUT UINT16                   *SessionIdLen
  );

/**
  Gets the client random data used in the specified TLS connection.

  This function returns the TLS/SSL client random data currently used in
  the specified TLS connection.

  @param[in]      Tls             Pointer to the TLS object.
  @param[in,out]  ClientRandom    Buffer to contain the returned client
                                  random data (32 bytes).

**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_TLS_GET_CLIENT_RANDOM)(
  IN     VOID                     *Tls,
  IN OUT UINT8                    *ClientRandom
  );

/**
  Gets the server random data used in the specified TLS connection.

  This function returns the TLS/SSL server random data currently used in
  the specified TLS connection.

  @param[in]      Tls             Pointer to the TLS object.
  @param[in,out]  ServerRandom    Buffer to contain the returned server
                                  random data (32 bytes).

**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_TLS_GET_SERVER_RANDOM)(
  IN     VOID                     *Tls,
  IN OUT UINT8                    *ServerRandom
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_GET_KEY_MATERIAL)(
  IN     VOID                     *Tls,
  IN OUT UINT8                    *KeyMaterial
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_GET_CA_CERTIFICATE)(
  IN     VOID                     *Tls,
  OUT    VOID                     *Data,
  IN OUT UINTN                    *DataSize
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_GET_HOST_PUBLIC_CERT)(
  IN     VOID                     *Tls,
  OUT    VOID                     *Data,
  IN OUT UINTN                    *DataSize
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_GET_HOST_PRIVATE_KEY)(
  IN     VOID                     *Tls,
  OUT    VOID                     *Data,
  IN OUT UINTN                    *DataSize
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SET_SIGNATURE_ALGO_LIST)(
  IN     VOID   *Tls,
  IN     UINT8  *Data,
  IN     UINTN  DataSize
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_SET_EC_CURVE)(
  IN     VOID   *Tls,
  IN     UINT8  *Data,
  IN     UINTN  DataSize
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_GET_EXPORT_KEY)(
  IN     VOID                     *Tls,
  IN     CONST VOID              *Label,
  IN     CONST VOID               *Context,
  IN     UINTN                    ContextLen,
  OUT    VOID                     *KeyBuffer,
  IN     UINTN                    KeyBufferLen
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_CRYPTO_TLS_GET_CERT_REVOCATION_LIST)(
  OUT    VOID                     *DATA,
  IN OUT UINTN                    *DataSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_RSA_PSS_SIGN)(
  IN      VOID         *RsaContext,
  IN      CONST UINT8  *Message,
  IN      UINTN        MsgSize,
  IN      UINT16       DigestLen,
  IN      UINT16       SaltLen,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_RSA_PSS_VERIFY)(
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *Message,
  IN  UINTN        MsgSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize,
  IN  UINT16       DigestLen,
  IN  UINT16       SaltLen
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_PARALLEL_HASH_ALL)(
  IN CONST VOID   *Input,
  IN       UINTN  InputByteLen,
  IN       UINTN  BlockSize,
  OUT      VOID   *Output,
  IN       UINTN  OutputByteLen,
  IN CONST VOID   *Customization,
  IN       UINTN  CustomByteLen
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_AEAD_AES_GCM_ENCRYPT)(
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
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_AEAD_AES_GCM_DECRYPT)(
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
  );

// =====================================================================================
//   Big Number Primitive
// =====================================================================================

/**
  Allocate new Big Number.

  @retval New BigNum opaque structure or NULL on failure.
**/
typedef
VOID *
(EFIAPI *EDKII_CRYPTO_BIGNUM_INIT)(
  VOID
  );

/**
  Allocate new Big Number and assign the provided value to it.

  @param[in]   Buf    Big endian encoded buffer.
  @param[in]   Len    Buffer length.

  @retval New EDKII_CRYPTO_BIGNUM_ opaque structure or NULL on failure.
**/
typedef
VOID *
(EFIAPI *EDKII_CRYPTO_BIGNUM_FROM_BIN)(
  IN CONST UINT8 *Buf,
  IN UINTN Len
  );

/**
  Convert the absolute value of Bn into big-endian form and store it at Buf.
  The Buf array should have at least EDKII_CRYPTO_BIGNUM_Bytes() in it.

  @param[in]   Bn     Big number to convert.
  @param[out]  Buf    Output buffer.

  @retval The length of the big-endian number placed at Buf or -1 on error.
**/
typedef
INTN
(EFIAPI *EDKII_CRYPTO_BIGNUM_TO_BIN)(
  IN CONST VOID *Bn,
  OUT UINT8 *Buf
  );

/**
  Free the Big Number.

  @param[in]   Bn      Big number to free.
  @param[in]   Clear   TRUE if the buffer should be cleared.
**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_BIGNUM_FREE)(
  IN VOID *Bn,
  IN BOOLEAN Clear
  );

/**
  Calculate the sum of two Big Numbers.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[out]  BnRes    The result of BnA + BnB.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_BIGNUM_ADD)(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes
  );

/**
  Subtract two Big Numbers.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[out]  BnRes    The result of BnA - BnB.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_BIGNUM_SUB)(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes
  );

/**
  Calculate remainder: BnRes = BnA % BnB.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[out]  BnRes    The result of BnA % BnB.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_BIGNUM_MOD)(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes
  );

/**
  Compute BnA to the BnP-th power modulo BnM.

  @param[in]   BnA     Big number.
  @param[in]   BnP     Big number (power).
  @param[in]   BnM     Big number (modulo).
  @param[out]  BnRes    The result of BnA ^ BnP % BnM.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_BIGNUM_EXP_MOD)(
  IN CONST VOID *BnA,
  IN CONST VOID *BnP,
  IN CONST VOID *BnM,
  OUT VOID *BnRes
  );

/**
  Compute BnA inverse modulo BnM.

  @param[in]   BnA     Big number.
  @param[in]   BnM     Big number (modulo).
  @param[out]  BnRes   The result, such that (BnA * BnRes) % BnM == 1.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_BIGNUM_INVERSE_MOD)(
  IN CONST VOID *BnA,
  IN CONST VOID *BnM,
  OUT VOID *BnRes
  );

/**
  Divide two Big Numbers.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[out]  BnRes   The result, such that BnA / BnB.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_BIGNUM_DIV)(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes
  );

/**
  Multiply two Big Numbers modulo BnM.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[in]   BnM     Big number (modulo).
  @param[out]  BnRes   The result, such that (BnA * BnB) % BnM.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_BIGNUM_MUL_MOD)(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  IN CONST VOID *BnM,
  OUT VOID *BnRes
  );

/**
  Compare two Big Numbers.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.

  @retval 0          BnA == BnB.
  @retval 1          BnA > BnB.
  @retval -1         BnA < BnB.
**/
typedef
INTN
(EFIAPI *EDKII_CRYPTO_BIGNUM_CMP)(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB
  );

/**
  Get number of bits in Bn.

  @param[in]   Bn     Big number.

  @retval Number of bits.
**/
typedef
UINTN
(EFIAPI *EDKII_CRYPTO_BIGNUM_BITS)(
  IN CONST VOID *Bn
  );

/**
  Get number of bytes in Bn.

  @param[in]   Bn     Big number.

  @retval Number of bytes.
**/
typedef
UINTN
(EFIAPI *EDKII_CRYPTO_BIGNUM_BYTES)(
  IN CONST VOID *Bn
  );

/**
  Checks if Big Number equals to the given Num.

  @param[in]   Bn     Big number.
  @param[in]   Num    Number.

  @retval TRUE   iff Bn == Num.
  @retval FALSE  otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_BIGNUM_IS_WORD)(
  IN CONST VOID *Bn,
  IN UINTN Num
  );

/**
  Checks if Big Number is odd.

  @param[in]   Bn     Big number.

  @retval TRUE   Bn is odd (Bn % 2 == 1).
  @retval FALSE  otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_BIGNUM_IS_ODD)(
  IN CONST VOID *Bn
  );

/**
  Copy Big number.

  @param[out]  BnDst     Destination.
  @param[in]   BnSrc     Source.

  @retval BnDst on success.
  @retval NULL otherwise.
**/
typedef
VOID *
(EFIAPI *EDKII_CRYPTO_BIGNUM_COPY)(
  OUT VOID *BnDst,
  IN CONST VOID *BnSrc
  );

/**
  Get constant Big number with value of "1".
  This may be used to save expensive allocations.

  @retval Big Number with value of 1.
**/
typedef
CONST VOID *
(EFIAPI *EDKII_CRYPTO_BIGNUM_VALUE_ONE)(
  VOID
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_BIGNUM_R_SHIFT)(
  IN CONST VOID *Bn,
  IN UINTN N,
  OUT VOID *BnRes
  );

/**
  Mark Big Number for constant time computations.
  This function should be called before any constant time computations are
  performed on the given Big number.

  @param[in]   Bn     Big number.
**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_BIGNUM_CONST_TIME)(
  IN VOID *Bn
  );

/**
  Calculate square modulo.

  @param[in]   BnA     Big number.
  @param[in]   BnM     Big number (modulo).
  @param[out]  BnRes   The result, such that (BnA ^ 2) % BnM.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_BIGNUM_SQR_MOD)(
  IN CONST VOID *BnA,
  IN CONST VOID *BnM,
  OUT VOID *BnRes
  );

/**
  Create new Big Number computation context. This is an opaque structure.
  which should be passed to any function that requires it. The BN context is
  needed to optimize calculations and expensive allocations.

  @retval Big Number context struct or NULL on failure.
**/
typedef
VOID *
(EFIAPI *EDKII_CRYPTO_BIGNUM_NEW_CONTEXT)(
  VOID
  );

/**
  Free Big Number context that was allocated with EDKII_CRYPTO_BIGNUM_NewContext().

  @param[in]   BnCtx     Big number context to free.
**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_BIGNUM_CONTEXT_FREE)(
  IN VOID *BnCtx
  );

/**
  Set Big Number to a given value.

  @param[in]   Bn     Big number to set.
  @param[in]   Val    Value to set.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_BIGNUM_SET_UINT)(
  IN VOID *Bn,
  IN UINTN Val
  );

/**
  Add two Big Numbers modulo BnM.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[in]   BnM     Big number (modulo).
  @param[out]  BnRes   The result, such that (BnA + BnB) % BnM.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_BIGNUM_ADD_MOD)(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  IN CONST VOID *BnM,
  OUT VOID *BnRes
  );

/**
  Initialize new opaque EcGroup object. This object represents an EC curve and
  and is used for calculation within this group. This object should be freed
  using EcGroupFree() function.

  @param[in]  CryptoNid   Identifying number for the ECC curve (Defined in
                          BaseCryptLib.h).

  @retval EcGroup object  On success
  @retval NULL            On failure
**/
typedef
VOID *
(EFIAPI *EDKII_CRYPTO_EC_GROUP_INIT)(
  IN UINTN  CryptoNid
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_GROUP_GET_CURVE)(
  IN CONST VOID *EcGroup,
  OUT VOID *BnPrime,
  OUT VOID *BnA,
  OUT VOID *BnB,
  IN VOID *BnCtx
  );

/**
  Get EC group order.
  This function will set the provided Big Number object to the corresponding
  value. The caller needs to make sure that the "out" BigNumber parameter
  is properly initialized.

  @param[in]  EcGroup   EC group object
  @param[out] BnOrder   Group prime number

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_GROUP_GET_ORDER)(
  IN VOID *EcGroup,
  OUT VOID *BnOrder
  );

/**
  Free previously allocated EC group object using EcGroupInit()

  @param[in]  EcGroup   EC group object to free
**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_EC_GROUP_FREE)(
  IN VOID *EcGroup
  );

/**
  Initialize new opaque EC Point object. This object represents an EC point
  within the given EC group (curve).

  @param[in]  EC Group, properly initialized using EcGroupInit()

  @retval EC Point object  On success
  @retval NULL             On failure
**/
typedef
VOID *
(EFIAPI *EDKII_CRYPTO_EC_POINT_INIT)(
  IN CONST VOID *EcGroup
  );

/**
  Free previously allocated EC Point object using EcPointInit()

  @param[in]  EcPoint   EC Point to free
  @param[in]  Clear     TRUE iff the memory should be cleared
**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_EC_POINT_DE_INIT)(
  IN VOID *EcPoint,
  IN BOOLEAN Clear
  );

/**
  Get EC point affine (x,y) coordinates.
  This function will set the provided Big Number objects to the corresponding
  values. The caller needs to make sure all the "out" BigNumber parameters
  are properly initialized.

  @param[in]  EcGroup    EC group object
  @param[in]  EcPoint    EC point object
  @param[out] BnX        X coordinate
  @param[out] BnY        Y coordinate
  @param[in]  BnCtx      BN context, created with BigNumNewContext()

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_POINT_GET_AFFINE_COORDINATES)(
  IN CONST VOID *EcGroup,
  IN CONST VOID *EcPoint,
  OUT VOID *BnX,
  OUT VOID *BnY,
  IN VOID *BnCtx
  );

/**
  Set EC point affine (x,y) coordinates.

  @param[in]  EcGroup    EC group object
  @param[in]  EcPoint    EC point object
  @param[in]  BnX        X coordinate
  @param[in]  BnY        Y coordinate
  @param[in]  BnCtx      BN context, created with BigNumNewContext()

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_POINT_SET_AFFINE_COORDINATES)(
  IN CONST VOID *EcGroup,
  IN VOID *EcPoint,
  IN CONST VOID *BnX,
  IN CONST VOID *BnY,
  IN VOID *BnCtx
  );

/**
  EC Point addition. EcPointResult = EcPointA + EcPointB

  @param[in]  EcGroup          EC group object
  @param[out] EcPointResult    EC point to hold the result. The point should
                               be properly initialized.
  @param[in]  EcPointA         EC Point
  @param[in]  EcPointB         EC Point
  @param[in]  BnCtx            BN context, created with BigNumNewContext()

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_POINT_ADD)(
  IN CONST VOID *EcGroup,
  OUT VOID *EcPointResult,
  IN CONST VOID *EcPointA,
  IN CONST VOID *EcPointB,
  IN VOID *BnCtx
  );

/**
  Variable EC point multiplication. EcPointResult = EcPoint * BnPScalar

  @param[in]  EcGroup          EC group object
  @param[out] EcPointResult    EC point to hold the result. The point should
                               be properly initialized.
  @param[in]  EcPoint          EC Point
  @param[in]  BnPScalar        P Scalar
  @param[in]  BnCtx            BN context, created with BigNumNewContext()

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_POINT_MUL)(
  IN CONST VOID *EcGroup,
  OUT VOID *EcPointResult,
  IN CONST VOID *EcPoint,
  IN CONST VOID *BnPScalar,
  IN VOID *BnCtx
  );

/**
  Calculate the inverse of the supplied EC point.

  @param[in]     EcGroup   EC group object
  @param[in,out] EcPoint   EC point to invert
  @param[in]     BnCtx     BN context, created with BigNumNewContext()

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_POINT_INVERT)(
  IN CONST VOID *EcGroup,
  IN OUT VOID *EcPoint,
  IN VOID *BnCtx
  );

/**
  Check if the supplied point is on EC curve

  @param[in]  EcGroup   EC group object
  @param[in]  EcPoint   EC point to check
  @param[in]  BnCtx     BN context, created with BigNumNewContext()

  @retval TRUE           On curve
  @retval FALSE          Otherwise
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_POINT_IS_ON_CURVE)(
  IN CONST VOID *EcGroup,
  IN CONST VOID *EcPoint,
  IN VOID *BnCtx
  );

/**
  Check if the supplied point is at infinity

  @param[in]  EcGroup   EC group object
  @param[in]  EcPoint   EC point to check
  @param[in]  BnCtx     BN context, created with BigNumNewContext()

  @retval TRUE          At infinity
  @retval FALSE         Otherwise
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_POINT_IS_AT_INFINITY)(
  IN CONST VOID *EcGroup,
  IN CONST VOID *EcPoint
  );

/**
  Check if EC points are equal

  @param[in]  EcGroup   EC group object
  @param[in]  EcPointA  EC point A
  @param[in]  EcPointB  EC point B
  @param[in]  BnCtx     BN context, created with BigNumNewContext()

  @retval TRUE          A == B
  @retval FALSE         Otherwise
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_POINT_EQUAL)(
  IN CONST VOID *EcGroup,
  IN CONST VOID *EcPointA,
  IN CONST VOID *EcPointB,
  IN VOID *BnCtx
  );

/**
  Set EC point compressed coordinates. Points can be described in terms of
  their compressed coordinates. For a point (x, y), for any given value for x
  such that the point is on the curve there will only ever be two possible
  values for y. Therefore, a point can be set using this function where BnX is
  the x coordinate and YBit is a value 0 or 1 to identify which of the two
  possible values for y should be used.

  @param[in]  EcGroup    EC group object
  @param[in]  EcPoint    EC Point
  @param[in]  BnX        X coordinate
  @param[in]  YBit       0 or 1 to identify which Y value is used
  @param[in]  BnCtx      BN context, created with BigNumNewContext()

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_POINT_SET_COMPRESSED_COORDINATES)(
  IN CONST VOID *EcGroup,
  IN VOID *EcPoint,
  IN CONST VOID *BnX,
  IN UINT8 YBit,
  IN VOID *BnCtx
  );

/**
  Allocates and Initializes one Elliptic Curve Context for subsequent use
  with the NID.

  @param[in]  Nid cipher NID
  @return     Pointer to the Elliptic Curve Context that has been initialized.
              If the allocations fails, EcNewByNid() returns NULL.
**/
typedef
VOID *
(EFIAPI *EDKII_CRYPTO_EC_NEW_BY_NID)(
  IN UINTN  Nid
  );

/**
  Release the specified EC context.

  @param[in]  EcContext  Pointer to the EC context to be released.
**/
typedef
VOID
(EFIAPI *EDKII_CRYPTO_EC_FREE)(
  IN  VOID  *EcContext
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_GENERATE_KEY)(
  IN OUT  VOID   *EcContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_GET_PUB_KEY)(
  IN OUT  VOID   *EcContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_DH_COMPUTE_KEY)(
  IN OUT  VOID         *EcContext,
  IN      CONST UINT8  *PeerPublic,
  IN      UINTN        PeerPublicSize,
  IN      CONST INT32  *CompressFlag,
  OUT     UINT8        *Key,
  IN OUT  UINTN        *KeySize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_GET_PUBLIC_KEY_FROM_X509)(
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  VOID         **EcContext
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_GET_PRIVATE_KEY_FROM_PEM)(
  IN   CONST UINT8  *PemData,
  IN   UINTN        PemSize,
  IN   CONST CHAR8  *Password,
  OUT  VOID         **EcContext
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_DSA_SIGN)(
  IN      VOID         *EcContext,
  IN      UINTN        HashNid,
  IN      CONST UINT8  *MessageHash,
  IN      UINTN        HashSize,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  );

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
typedef
BOOLEAN
(EFIAPI *EDKII_CRYPTO_EC_DSA_VERIFY)(
  IN  VOID         *EcContext,
  IN  UINTN        HashNid,
  IN  CONST UINT8  *MessageHash,
  IN  UINTN        HashSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize
  );

///
/// EDK II Crypto Protocol
///
struct _EDKII_CRYPTO_PROTOCOL {
  /// Version
  EDKII_CRYPTO_GET_VERSION                            GetVersion;
  /// HMAC MD5 - deprecated and unsupported
  DEPRECATED_EDKII_CRYPTO_HMAC_MD5_NEW                DeprecatedHmacMd5New;
  DEPRECATED_EDKII_CRYPTO_HMAC_MD5_FREE               DeprecatedHmacMd5Free;
  DEPRECATED_EDKII_CRYPTO_HMAC_MD5_SET_KEY            DeprecatedHmacMd5SetKey;
  DEPRECATED_EDKII_CRYPTO_HMAC_MD5_DUPLICATE          DeprecatedHmacMd5Duplicate;
  DEPRECATED_EDKII_CRYPTO_HMAC_MD5_UPDATE             DeprecatedHmacMd5Update;
  DEPRECATED_EDKII_CRYPTO_HMAC_MD5_FINAL              DeprecatedHmacMd5Final;
  /// HMAC SHA1 - deprecated and unsupported
  DEPRECATED_EDKII_CRYPTO_HMAC_SHA1_NEW               DeprecatedHmacSha1New;
  DEPRECATED_EDKII_CRYPTO_HMAC_SHA1_FREE              DeprecatedHmacSha1Free;
  DEPRECATED_EDKII_CRYPTO_HMAC_SHA1_SET_KEY           DeprecatedHmacSha1SetKey;
  DEPRECATED_EDKII_CRYPTO_HMAC_SHA1_DUPLICATE         DeprecatedHmacSha1Duplicate;
  DEPRECATED_EDKII_CRYPTO_HMAC_SHA1_UPDATE            DeprecatedHmacSha1Update;
  DEPRECATED_EDKII_CRYPTO_HMAC_SHA1_FINAL             DeprecatedHmacSha1Final;
  /// HMAC SHA256
  EDKII_CRYPTO_HMAC_SHA256_NEW                        HmacSha256New;
  EDKII_CRYPTO_HMAC_SHA256_FREE                       HmacSha256Free;
  EDKII_CRYPTO_HMAC_SHA256_SET_KEY                    HmacSha256SetKey;
  EDKII_CRYPTO_HMAC_SHA256_DUPLICATE                  HmacSha256Duplicate;
  EDKII_CRYPTO_HMAC_SHA256_UPDATE                     HmacSha256Update;
  EDKII_CRYPTO_HMAC_SHA256_FINAL                      HmacSha256Final;
  /// Md4 - deprecated and unsupported
  DEPRECATED_EDKII_CRYPTO_MD4_GET_CONTEXT_SIZE        DeprecatedMd4GetContextSize;
  DEPRECATED_EDKII_CRYPTO_MD4_INIT                    DeprecatedMd4Init;
  DEPRECATED_EDKII_CRYPTO_MD4_DUPLICATE               DeprecatedMd4Duplicate;
  DEPRECATED_EDKII_CRYPTO_MD4_UPDATE                  DeprecatedMd4Update;
  DEPRECATED_EDKII_CRYPTO_MD4_FINAL                   DeprecatedMd4Final;
  DEPRECATED_EDKII_CRYPTO_MD4_HASH_ALL                DeprecatedMd4HashAll;
  /// Md5
  EDKII_CRYPTO_MD5_GET_CONTEXT_SIZE                   Md5GetContextSize;
  EDKII_CRYPTO_MD5_INIT                               Md5Init;
  EDKII_CRYPTO_MD5_DUPLICATE                          Md5Duplicate;
  EDKII_CRYPTO_MD5_UPDATE                             Md5Update;
  EDKII_CRYPTO_MD5_FINAL                              Md5Final;
  EDKII_CRYPTO_MD5_HASH_ALL                           Md5HashAll;
  /// Pkcs
  EDKII_CRYPTO_PKCS1_ENCRYPT_V2                       Pkcs1v2Encrypt;
  EDKII_CRYPTO_PKCS5_PW_HASH                          Pkcs5HashPassword;
  EDKII_CRYPTO_PKCS7_VERIFY                           Pkcs7Verify;
  EDKII_CRYPTO_PKCS7_VERIFY_EKU                       VerifyEKUsInPkcs7Signature;
  EDKII_CRYPTO_PKCS7_GET_SIGNERS                      Pkcs7GetSigners;
  EDKII_CRYPTO_PKCS7_FREE_SIGNERS                     Pkcs7FreeSigners;
  EDKII_CRYPTO_PKCS7_SIGN                             Pkcs7Sign;
  EDKII_CRYPTO_PKCS7_GET_ATTACHED_CONTENT             Pkcs7GetAttachedContent;
  EDKII_CRYPTO_PKCS7_GET_CERTIFICATES_LIST            Pkcs7GetCertificatesList;
  EDKII_CRYPTO_AUTHENTICODE_VERIFY                    AuthenticodeVerify;
  EDKII_CRYPTO_IMAGE_TIMESTAMP_VERIFY                 ImageTimestampVerify;
  /// DH
  EDKII_CRYPTO_DH_NEW                                 DhNew;
  EDKII_CRYPTO_DH_FREE                                DhFree;
  EDKII_CRYPTO_DH_GENERATE_PARAMETER                  DhGenerateParameter;
  EDKII_CRYPTO_DH_SET_PARAMETER                       DhSetParameter;
  EDKII_CRYPTO_DH_GENERATE_KEY                        DhGenerateKey;
  EDKII_CRYPTO_DH_COMPUTE_KEY                         DhComputeKey;
  /// Random
  EDKII_CRYPTO_RANDOM_SEED                            RandomSeed;
  EDKII_CRYPTO_RANDOM_BYTES                           RandomBytes;
  /// RSA
  EDKII_CRYPTO_RSA_VERIFY_PKCS1                       RsaVerifyPkcs1;
  EDKII_CRYPTO_RSA_NEW                                RsaNew;
  EDKII_CRYPTO_RSA_FREE                               RsaFree;
  EDKII_CRYPTO_RSA_SET_KEY                            RsaSetKey;
  EDKII_CRYPTO_RSA_GET_KEY                            RsaGetKey;
  EDKII_CRYPTO_RSA_GENERATE_KEY                       RsaGenerateKey;
  EDKII_CRYPTO_RSA_CHECK_KEY                          RsaCheckKey;
  EDKII_CRYPTO_RSA_PKCS1_SIGN                         RsaPkcs1Sign;
  EDKII_CRYPTO_RSA_PKCS1_VERIFY                       RsaPkcs1Verify;
  EDKII_CRYPTO_RSA_GET_PRIVATE_KEY_FROM_PEM           RsaGetPrivateKeyFromPem;
  EDKII_CRYPTO_RSA_GET_PUBLIC_KEY_FROM_X509           RsaGetPublicKeyFromX509;
  /// Sha1
  EDKII_CRYPTO_SHA1_GET_CONTEXT_SIZE                  Sha1GetContextSize;
  EDKII_CRYPTO_SHA1_INIT                              Sha1Init;
  EDKII_CRYPTO_SHA1_DUPLICATE                         Sha1Duplicate;
  EDKII_CRYPTO_SHA1_UPDATE                            Sha1Update;
  EDKII_CRYPTO_SHA1_FINAL                             Sha1Final;
  EDKII_CRYPTO_SHA1_HASH_ALL                          Sha1HashAll;
  /// Sha256
  EDKII_CRYPTO_SHA256_GET_CONTEXT_SIZE                Sha256GetContextSize;
  EDKII_CRYPTO_SHA256_INIT                            Sha256Init;
  EDKII_CRYPTO_SHA256_DUPLICATE                       Sha256Duplicate;
  EDKII_CRYPTO_SHA256_UPDATE                          Sha256Update;
  EDKII_CRYPTO_SHA256_FINAL                           Sha256Final;
  EDKII_CRYPTO_SHA256_HASH_ALL                        Sha256HashAll;
  /// Sha384
  EDKII_CRYPTO_SHA384_GET_CONTEXT_SIZE                Sha384GetContextSize;
  EDKII_CRYPTO_SHA384_INIT                            Sha384Init;
  EDKII_CRYPTO_SHA384_DUPLICATE                       Sha384Duplicate;
  EDKII_CRYPTO_SHA384_UPDATE                          Sha384Update;
  EDKII_CRYPTO_SHA384_FINAL                           Sha384Final;
  EDKII_CRYPTO_SHA384_HASH_ALL                        Sha384HashAll;
  /// Sha512
  EDKII_CRYPTO_SHA512_GET_CONTEXT_SIZE                Sha512GetContextSize;
  EDKII_CRYPTO_SHA512_INIT                            Sha512Init;
  EDKII_CRYPTO_SHA512_DUPLICATE                       Sha512Duplicate;
  EDKII_CRYPTO_SHA512_UPDATE                          Sha512Update;
  EDKII_CRYPTO_SHA512_FINAL                           Sha512Final;
  EDKII_CRYPTO_SHA512_HASH_ALL                        Sha512HashAll;
  /// X509
  EDKII_CRYPTO_X509_GET_SUBJECT_NAME                  X509GetSubjectName;
  EDKII_CRYPTO_X509_GET_COMMON_NAME                   X509GetCommonName;
  EDKII_CRYPTO_X509_GET_ORGANIZATION_NAME             X509GetOrganizationName;
  EDKII_CRYPTO_X509_VERIFY_CERT                       X509VerifyCert;
  EDKII_CRYPTO_X509_CONSTRUCT_CERTIFICATE             X509ConstructCertificate;
  EDKII_CRYPTO_X509_CONSTRUCT_CERTIFICATE_STACK       X509ConstructCertificateStack;
  EDKII_CRYPTO_X509_FREE                              X509Free;
  EDKII_CRYPTO_X509_STACK_FREE                        X509StackFree;
  EDKII_CRYPTO_X509_GET_TBS_CERT                      X509GetTBSCert;
  /// TDES - deprecated and unsupported
  DEPRECATED_EDKII_CRYPTO_TDES_GET_CONTEXT_SIZE       DeprecatedTdesGetContextSize;
  DEPRECATED_EDKII_CRYPTO_TDES_INIT                   DeprecatedTdesInit;
  DEPRECATED_EDKII_CRYPTO_TDES_ECB_ENCRYPT            DeprecatedTdesEcbEncrypt;
  DEPRECATED_EDKII_CRYPTO_TDES_ECB_DECRYPT            DeprecatedTdesEcbDecrypt;
  DEPRECATED_EDKII_CRYPTO_TDES_CBC_ENCRYPT            DeprecatedTdesCbcEncrypt;
  DEPRECATED_EDKII_CRYPTO_TDES_CBC_DECRYPT            DeprecatedTdesCbcDecrypt;
  /// AES - ECB Mode is deprecated and unsupported
  EDKII_CRYPTO_AES_GET_CONTEXT_SIZE                   AesGetContextSize;
  EDKII_CRYPTO_AES_INIT                               AesInit;
  DEPRECATED_EDKII_CRYPTO_AES_ECB_ENCRYPT             DeprecatedAesEcbEncrypt;
  DEPRECATED_EDKII_CRYPTO_AES_ECB_DECRYPT             DeprecatedAesEcbDecrypt;
  EDKII_CRYPTO_AES_CBC_ENCRYPT                        AesCbcEncrypt;
  EDKII_CRYPTO_AES_CBC_DECRYPT                        AesCbcDecrypt;
  /// Arc4 - deprecated and unsupported
  DEPRECATED_EDKII_CRYPTO_ARC4_GET_CONTEXT_SIZE       DeprecatedArc4GetContextSize;
  DEPRECATED_EDKII_CRYPTO_ARC4_INIT                   DeprecatedArc4Init;
  DEPRECATED_EDKII_CRYPTO_ARC4_ENCRYPT                DeprecatedArc4Encrypt;
  DEPRECATED_EDKII_CRYPTO_ARC4_DECRYPT                DeprecatedArc4Decrypt;
  DEPRECATED_EDKII_CRYPTO_ARC4_RESET                  DeprecatedArc4Reset;
  /// SM3
  EDKII_CRYPTO_SM3_GET_CONTEXT_SIZE                   Sm3GetContextSize;
  EDKII_CRYPTO_SM3_INIT                               Sm3Init;
  EDKII_CRYPTO_SM3_DUPLICATE                          Sm3Duplicate;
  EDKII_CRYPTO_SM3_UPDATE                             Sm3Update;
  EDKII_CRYPTO_SM3_FINAL                              Sm3Final;
  EDKII_CRYPTO_SM3_HASH_ALL                           Sm3HashAll;
  /// HKDF
  EDKII_CRYPTO_HKDF_SHA_256_EXTRACT_AND_EXPAND        HkdfSha256ExtractAndExpand;
  /// X509 (Continued)
  EDKII_CRYPTO_X509_CONSTRUCT_CERTIFICATE_STACK_V     X509ConstructCertificateStackV;
  /// TLS
  EDKII_CRYPTO_TLS_INITIALIZE                         TlsInitialize;
  EDKII_CRYPTO_TLS_CTX_FREE                           TlsCtxFree;
  EDKII_CRYPTO_TLS_CTX_NEW                            TlsCtxNew;
  EDKII_CRYPTO_TLS_FREE                               TlsFree;
  EDKII_CRYPTO_TLS_NEW                                TlsNew;
  EDKII_CRYPTO_TLS_IN_HANDSHAKE                       TlsInHandshake;
  EDKII_CRYPTO_TLS_DO_HANDSHAKE                       TlsDoHandshake;
  EDKII_CRYPTO_TLS_HANDLE_ALERT                       TlsHandleAlert;
  EDKII_CRYPTO_TLS_CLOSE_NOTIFY                       TlsCloseNotify;
  EDKII_CRYPTO_TLS_CTRL_TRAFFIC_OUT                   TlsCtrlTrafficOut;
  EDKII_CRYPTO_TLS_CTRL_TRAFFIC_IN                    TlsCtrlTrafficIn;
  EDKII_CRYPTO_TLS_READ                               TlsRead;
  EDKII_CRYPTO_TLS_WRITE                              TlsWrite;
  /// TLS Set
  EDKII_CRYPTO_TLS_SET_VERSION                        TlsSetVersion;
  EDKII_CRYPTO_TLS_SET_CONNECTION_END                 TlsSetConnectionEnd;
  EDKII_CRYPTO_TLS_SET_CIPHER_LIST                    TlsSetCipherList;
  EDKII_CRYPTO_TLS_SET_COMPRESSION_METHOD             TlsSetCompressionMethod;
  EDKII_CRYPTO_TLS_SET_VERIFY                         TlsSetVerify;
  EDKII_CRYPTO_TLS_SET_VERIFY_HOST                    TlsSetVerifyHost;
  EDKII_CRYPTO_TLS_SET_SESSIONID                      TlsSetSessionId;
  EDKII_CRYPTO_TLS_SET_CA_CERTIFICATE                 TlsSetCaCertificate;
  EDKII_CRYPTO_TLS_SET_HOST_PUBLIC_CERT               TlsSetHostPublicCert;
  EDKII_CRYPTO_TLS_SET_HOST_PRIVATE_KEY               TlsSetHostPrivateKey;
  EDKII_CRYPTO_TLS_SET_CERT_REVOCATION_LIST           TlsSetCertRevocationList;
  /// TLS Get
  EDKII_CRYPTO_TLS_GET_VERSION                        TlsGetVersion;
  EDKII_CRYPTO_TLS_GET_CONNECTION_END                 TlsGetConnectionEnd;
  EDKII_CRYPTO_TLS_GET_CURRENT_CIPHER                 TlsGetCurrentCipher;
  EDKII_CRYPTO_TLS_GET_CURRENT_COMPRESSION_ID         TlsGetCurrentCompressionId;
  EDKII_CRYPTO_TLS_GET_VERIFY                         TlsGetVerify;
  EDKII_CRYPTO_TLS_GET_SESSION_ID                     TlsGetSessionId;
  EDKII_CRYPTO_TLS_GET_CLIENT_RANDOM                  TlsGetClientRandom;
  EDKII_CRYPTO_TLS_GET_SERVER_RANDOM                  TlsGetServerRandom;
  EDKII_CRYPTO_TLS_GET_KEY_MATERIAL                   TlsGetKeyMaterial;
  EDKII_CRYPTO_TLS_GET_CA_CERTIFICATE                 TlsGetCaCertificate;
  EDKII_CRYPTO_TLS_GET_HOST_PUBLIC_CERT               TlsGetHostPublicCert;
  EDKII_CRYPTO_TLS_GET_HOST_PRIVATE_KEY               TlsGetHostPrivateKey;
  EDKII_CRYPTO_TLS_GET_CERT_REVOCATION_LIST           TlsGetCertRevocationList;
  /// RSA PSS
  EDKII_CRYPTO_RSA_PSS_SIGN                           RsaPssSign;
  EDKII_CRYPTO_RSA_PSS_VERIFY                         RsaPssVerify;
  /// Parallel hash
  EDKII_CRYPTO_PARALLEL_HASH_ALL                      ParallelHash256HashAll;
  /// HMAC SHA256 (continued)
  EDKII_CRYPTO_HMAC_SHA256_ALL                        HmacSha256All;
  /// HMAC SHA384
  EDKII_CRYPTO_HMAC_SHA384_NEW                        HmacSha384New;
  EDKII_CRYPTO_HMAC_SHA384_FREE                       HmacSha384Free;
  EDKII_CRYPTO_HMAC_SHA384_SET_KEY                    HmacSha384SetKey;
  EDKII_CRYPTO_HMAC_SHA384_DUPLICATE                  HmacSha384Duplicate;
  EDKII_CRYPTO_HMAC_SHA384_UPDATE                     HmacSha384Update;
  EDKII_CRYPTO_HMAC_SHA384_FINAL                      HmacSha384Final;
  EDKII_CRYPTO_HMAC_SHA384_ALL                        HmacSha384All;
  /// HKDF (continued)
  EDKII_CRYPTO_HKDF_SHA_256_EXTRACT                   HkdfSha256Extract;
  EDKII_CRYPTO_HKDF_SHA_256_EXPAND                    HkdfSha256Expand;
  EDKII_CRYPTO_HKDF_SHA_384_EXTRACT_AND_EXPAND        HkdfSha384ExtractAndExpand;
  EDKII_CRYPTO_HKDF_SHA_384_EXTRACT                   HkdfSha384Extract;
  EDKII_CRYPTO_HKDF_SHA_384_EXPAND                    HkdfSha384Expand;
  /// AEAD AES-GCM
  EDKII_AEAD_AES_GCM_ENCRYPT                          AeadAesGcmEncrypt;
  EDKII_AEAD_AES_GCM_DECRYPT                          AeadAesGcmDecrypt;
  /// BIGNUM
  EDKII_CRYPTO_BIGNUM_INIT                            BigNumInit;
  EDKII_CRYPTO_BIGNUM_FROM_BIN                        BigNumFromBin;
  EDKII_CRYPTO_BIGNUM_TO_BIN                          BigNumToBin;
  EDKII_CRYPTO_BIGNUM_FREE                            BigNumFree;
  EDKII_CRYPTO_BIGNUM_ADD                             BigNumAdd;
  EDKII_CRYPTO_BIGNUM_SUB                             BigNumSub;
  EDKII_CRYPTO_BIGNUM_MOD                             BigNumMod;
  EDKII_CRYPTO_BIGNUM_EXP_MOD                         BigNumExpMod;
  EDKII_CRYPTO_BIGNUM_INVERSE_MOD                     BigNumInverseMod;
  EDKII_CRYPTO_BIGNUM_DIV                             BigNumDiv;
  EDKII_CRYPTO_BIGNUM_MUL_MOD                         BigNumMulMod;
  EDKII_CRYPTO_BIGNUM_CMP                             BigNumCmp;
  EDKII_CRYPTO_BIGNUM_BITS                            BigNumBits;
  EDKII_CRYPTO_BIGNUM_BYTES                           BigNumBytes;
  EDKII_CRYPTO_BIGNUM_IS_WORD                         BigNumIsWord;
  EDKII_CRYPTO_BIGNUM_IS_ODD                          BigNumIsOdd;
  EDKII_CRYPTO_BIGNUM_COPY                            BigNumCopy;
  EDKII_CRYPTO_BIGNUM_VALUE_ONE                       BigNumValueOne;
  EDKII_CRYPTO_BIGNUM_R_SHIFT                         BigNumRShift;
  EDKII_CRYPTO_BIGNUM_CONST_TIME                      BigNumConstTime;
  EDKII_CRYPTO_BIGNUM_SQR_MOD                         BigNumSqrMod;
  EDKII_CRYPTO_BIGNUM_NEW_CONTEXT                     BigNumNewContext;
  EDKII_CRYPTO_BIGNUM_CONTEXT_FREE                    BigNumContextFree;
  EDKII_CRYPTO_BIGNUM_SET_UINT                        BigNumSetUint;
  EDKII_CRYPTO_BIGNUM_ADD_MOD                         BigNumAddMod;
  /// EC
  EDKII_CRYPTO_EC_GROUP_INIT                          EcGroupInit;
  EDKII_CRYPTO_EC_GROUP_GET_CURVE                     EcGroupGetCurve;
  EDKII_CRYPTO_EC_GROUP_GET_ORDER                     EcGroupGetOrder;
  EDKII_CRYPTO_EC_GROUP_FREE                          EcGroupFree;
  EDKII_CRYPTO_EC_POINT_INIT                          EcPointInit;
  EDKII_CRYPTO_EC_POINT_DE_INIT                       EcPointDeInit;
  EDKII_CRYPTO_EC_POINT_GET_AFFINE_COORDINATES        EcPointGetAffineCoordinates;
  EDKII_CRYPTO_EC_POINT_SET_AFFINE_COORDINATES        EcPointSetAffineCoordinates;
  EDKII_CRYPTO_EC_POINT_ADD                           EcPointAdd;
  EDKII_CRYPTO_EC_POINT_MUL                           EcPointMul;
  EDKII_CRYPTO_EC_POINT_INVERT                        EcPointInvert;
  EDKII_CRYPTO_EC_POINT_IS_ON_CURVE                   EcPointIsOnCurve;
  EDKII_CRYPTO_EC_POINT_IS_AT_INFINITY                EcPointIsAtInfinity;
  EDKII_CRYPTO_EC_POINT_EQUAL                         EcPointEqual;
  EDKII_CRYPTO_EC_POINT_SET_COMPRESSED_COORDINATES    EcPointSetCompressedCoordinates;
  EDKII_CRYPTO_EC_NEW_BY_NID                          EcNewByNid;
  EDKII_CRYPTO_EC_FREE                                EcFree;
  EDKII_CRYPTO_EC_GENERATE_KEY                        EcGenerateKey;
  EDKII_CRYPTO_EC_GET_PUB_KEY                         EcGetPubKey;
  EDKII_CRYPTO_EC_DH_COMPUTE_KEY                      EcDhComputeKey;
  /// TLS (continued)
  EDKII_CRYPTO_TLS_SHUTDOWN                           TlsShutdown;
  /// TLS Set (continued)
  EDKII_CRYPTO_TLS_SET_HOST_PRIVATE_KEY_EX            TlsSetHostPrivateKeyEx;
  EDKII_CRYPTO_TLS_SET_SIGNATURE_ALGO_LIST            TlsSetSignatureAlgoList;
  EDKII_CRYPTO_TLS_SET_EC_CURVE                       TlsSetEcCurve;
  /// TLS Get (continued)
  EDKII_CRYPTO_TLS_GET_EXPORT_KEY                     TlsGetExportKey;
  /// Ec (Continued)
  EDKII_CRYPTO_EC_GET_PUBLIC_KEY_FROM_X509            EcGetPublicKeyFromX509;
  EDKII_CRYPTO_EC_GET_PRIVATE_KEY_FROM_PEM            EcGetPrivateKeyFromPem;
  EDKII_CRYPTO_EC_DSA_SIGN                            EcDsaSign;
  EDKII_CRYPTO_EC_DSA_VERIFY                          EcDsaVerify;
  /// X509 (Continued)
  EDKII_CRYPTO_X509_GET_VERSION                       X509GetVersion;
  EDKII_CRYPTO_X509_GET_SERIAL_NUMBER                 X509GetSerialNumber;
  EDKII_CRYPTO_X509_GET_ISSUER_NAME                   X509GetIssuerName;
  EDKII_CRYPTO_X509_GET_SIGNATURE_ALGORITHM           X509GetSignatureAlgorithm;
  EDKII_CRYPTO_X509_GET_EXTENSION_DATA                X509GetExtensionData;
  EDKII_CRYPTO_X509_GET_EXTENDED_KEY_USAGE            X509GetExtendedKeyUsage;
  EDKII_CRYPTO_X509_GET_VALIDITY                      X509GetValidity;
  EDKII_CRYPTO_X509_FORMAT_DATE_TIME                  X509FormatDateTime;
  EDKII_CRYPTO_X509_COMPARE_DATE_TIME                 X509CompareDateTime;
  EDKII_CRYPTO_X509_GET_KEY_USAGE                     X509GetKeyUsage;
  EDKII_CRYPTO_X509_VERIFY_CERT_CHAIN                 X509VerifyCertChain;
  EDKII_CRYPTO_X509_GET_CERT_FROM_CERT_CHAIN          X509GetCertFromCertChain;
  EDKII_CRYPTO_ASN1_GET_TAG                           Asn1GetTag;
  EDKII_CRYPTO_X509_GET_EXTENDED_BASIC_CONSTRAINTS    X509GetExtendedBasicConstraints;
};

extern GUID  gEdkiiCryptoProtocolGuid;

#endif

/** @file
  Definitions related to the Cryptographic Operations in IPsec.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _EFI_IPSEC_CRYPTIO_H_
#define _EFI_IPSEC_CRYPTIO_H_

#include <Protocol/IpSecConfig.h>
#include <Library/DebugLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include "IpSecImpl.h"
#include "IkeCommon.h"

#define IPSEC_ENCRYPT_ALGORITHM_LIST_SIZE 4
#define IPSEC_AUTH_ALGORITHM_LIST_SIZE    3
#define IPSEC_HASH_ALGORITHM_LIST_SIZE    3

///
/// Authentication Algorithm Definition
///   The number value definition is aligned to IANA assignment
///
#define IKE_AALG_NONE                0x00
#define IKE_AALG_SHA1HMAC            0x02
#define IKE_AALG_NULL                0xFB

///
/// Encryption Algorithm Definition
///   The number value definition is aligned to IANA assignment
///
#define IKE_EALG_NONE                0x00
#define IKE_EALG_3DESCBC             0x03
#define IKE_EALG_NULL                0x0B
#define IKE_EALG_AESCBC              0x0C

/**
  Prototype of HMAC GetContextSize.
  
  Retrieves the size, in bytes, of the context buffer required.
  
  @return  The size, in bytes, of the context buffer required.

**/
typedef
UINTN
(EFIAPI *CRYPTO_HMAC_GETCONTEXTSIZE)(
  VOID
  );

/**
  Prototype of HMAC Operation Initiating.
  
  Initialization with a new context.

  @param[out]     Context  Input Context.
  @param[in]      Key      Pointer to the key for HMAC.
  @param[in]      KeySize  The length of the Key in bytes.
 
  @retval TRUE  Initialization Successfully.

**/
typedef
BOOLEAN
(EFIAPI *CRYPTO_HMAC_INIT)(
  OUT           VOID     *Context,
  IN     CONST  UINT8    *Key,
  IN            UINTN    KeySize
  );

/**
  Prototype of HMAC update.
  HMAC update operation. Continue an HMAC message digest operation, processing
  another message block, and updating the HMAC context.

  If Context is NULL, then ASSERT().
  If Data is NULL, then ASSERT().

  @param[in,out]  Context     The Specified Context.
  @param[in,out]  Data        The Input Data to be digested.
  @param[in]      DataLength  The length, in bytes, of Data.

  @retval TRUE   Update data successfully.
  @retval FALSE  The Context has been finalized.

**/
typedef
BOOLEAN
(EFIAPI *CRYPTO_HMAC_UPDATE)(
  IN OUT       VOID  *Context,
  IN     CONST VOID  *Data,
  IN           UINTN DataLength
  );

/**
  Prototype of HMAC finallization.
  Terminate a HMAC message digest operation and output the message digest.

  If Context is NULL, then ASSERT().
  If HashValue is NULL, then ASSERT().

  @param[in,out]  Context     The specified Context.
  @param[out]     HmacValue   Pointer to a 16-byte message digest output buffer.

  @retval TRUE  Finalized successfully.

**/
typedef
BOOLEAN
(EFIAPI *CRYPTO_HMAC_FINAL)(
  IN OUT  VOID   *Context,
     OUT  UINT8  *HmacValue
  );

/**
  Prototype of Block Cipher GetContextSize.

  Retrieves the size, in bytes, of the context buffer required.

  @return  The size, in bytes, of the context buffer required.

**/
typedef
UINTN
(EFIAPI *CRYPTO_CIPHER_GETCONTEXTSIZE)(
  VOID
  );

/**
  Prototype of Block Cipher initiation.
  Intializes the user-supplied key as the specifed context (key materials) for both
  encryption and decryption operations.

  If Context is NULL, then ASSERT().
  If Key is NULL, then generate random key for usage.

  @param[in,out]  Context      The specified Context.
  @param[in]      Key          User-supplied cipher key.
  @param[in]      KeyBits      Key length in bits.

  @retval TRUE  Block Cipher Initialization was successful.

**/
typedef
BOOLEAN
(EFIAPI *CRYPTO_CIPHER_INIT)(
  IN OUT        VOID   *Context,
  IN      CONST UINT8  *Key,
  IN            UINTN  KeyBits
  );

/**
  Prototype of Cipher encryption.
  Encrypts plaintext message with the specified cipher.

  If Context is NULL, then ASSERT().
  if InData is NULL, then ASSERT().
  If Size of input data is not multiple of Cipher algorithm related block size,
  then ASSERT().

  @param[in]      Context      The specified Context.
  @param[in]      InData       The input plaintext data to be encrypted.
  @param[in]      InputSize    The size of input data.
  @param[in]      Ivec         Pointer to Initial Vector data for encryption.
  @param[out]     OutData      The resultant encrypted ciphertext.

  @retval TRUE  Encryption successful.

**/
typedef
BOOLEAN
(EFIAPI *CRYPTO_CIPHER_ENCRYPT)(
  IN            VOID   *Context,
  IN      CONST UINT8  *InData,
  IN            UINTN  InputSize,
  IN      CONST UINT8  *Ivec,
      OUT       UINT8  *OutData
  );

/**
  Prototype of Cipher decryption.
  Decrypts cipher message with specified cipher.

  If Context is NULL, then ASSERT().
  if InData is NULL, then ASSERT().
  If Size of input data is not a multiple of a certaion block size , then ASSERT().

  @param[in]      Context      The specified Context.
  @param[in]      InData       The input ciphertext data to be decrypted.
  @param[in]      InputSize    The InData size.
  @param[in]      Ivec         Pointer to the Initial Vector data for decryption.
  @param[out]     OutData      The resultant decrypted plaintext.

  @retval TRUE  Decryption successful.

**/
typedef
BOOLEAN
(EFIAPI *CRYPTO_CIPHER_DECRYPT)(
  IN           VOID   *Context,
  IN     CONST UINT8  *InData,
  IN           UINTN  InputSize,
  IN     CONST UINT8  *Ivec,
     OUT       UINT8  *OutData
  );

/**
  Prototype of Hash ContextSize.

  Retrieves the size, in bytes, of the context buffer required for specified hash operations.

  @return  The size, in bytes, of the context buffer required for certain hash operations.

**/
typedef
UINTN
(EFIAPI *CRYPTO_HASH_GETCONTEXTSIZE)(
  VOID
  );

/**
  Prototype of Hash Initiate.

  Initializes user-supplied memory pointed by Context as specified hash context for
  subsequent use.

  If Context is NULL, then ASSERT().

  @param[out]  Context  Pointer to specified context being initialized.

  @retval TRUE   context initialization succeeded.
  @retval FALSE  context initialization failed.

**/
typedef
BOOLEAN
(EFIAPI *CRYPTO_HASH_INIT)(
  OUT  VOID  *Context
  );

/**
  Prototype of Hash Update
  
  Digests the input data and updates hash context.

  This function performs digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  Context should be already correctly intialized by HashInit(), and should not be finalized
  by HashFinal(). Behavior with invalid context is undefined.

  If Context is NULL, then ASSERT().

  @param[in, out]  Context      Pointer to the specified context.
  @param[in]       Data         Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize     Size of Data buffer in bytes.

  @retval TRUE   data digest succeeded.
  @retval FALSE  data digest failed.

**/
typedef
BOOLEAN
(EFIAPI *CRYPTO_HASH_UPDATE)(
  IN OUT  VOID        *Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

/**
  Prototype of Hash Finalization.

  Completes computation of the digest value.

  This function completes hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the context cannot
  be used again.
  context should be already correctly intialized by HashInit(), and should not be
  finalized by HashFinal(). Behavior with invalid context is undefined.

  If Context is NULL, then ASSERT().
  If HashValue is NULL, then ASSERT().

  @param[in, out]  Context      Pointer to the specified context.
  @param[out]      HashValue    Pointer to a buffer that receives the digest
                                value.

  @retval TRUE   digest computation succeeded.
  @retval FALSE  digest computation failed.

**/
typedef
BOOLEAN
(EFIAPI *CRYPTO_HASH_FINAL)(
  IN OUT  VOID   *Context,
  OUT     UINT8  *HashValue
  );

//
// The struct used to store the information and operation of Block Cipher algorithm.
//
typedef struct _ENCRYPT_ALGORITHM {
  //
  // The ID of the Algorithm
  //
  UINT8                     AlgorithmId;
  //
  // The Key length of the Algorithm
  //
  UINTN                     KeyLength;
  //
  // Iv Size of the Algorithm
  //
  UINTN                     IvLength;
  //
  // The Block Size of the Algorithm
  //
  UINTN                     BlockSize;
  //
  // The Function pointer of GetContextSize.
  //
  CRYPTO_CIPHER_GETCONTEXTSIZE CipherGetContextSize;
  //
  // The Function pointer of Cipher initiation.
  //
  CRYPTO_CIPHER_INIT           CipherInitiate;
  //
  // The Function pointer of Cipher Encryption.
  //
  CRYPTO_CIPHER_ENCRYPT        CipherEncrypt;
  //
  // The Function pointer of Cipher Decrption.
  //
  CRYPTO_CIPHER_DECRYPT        CipherDecrypt;
} ENCRYPT_ALGORITHM;

//
// The struct used to store the information and operation of Autahentication algorithm.
//
typedef struct _AUTH_ALGORITHM {
  //
  // ID of the Algorithm
  //
  UINT8                    AlgorithmId;
  //
  // The Key length of the Algorithm
  // 
  UINTN                    DigestLength;
  //
  // The ICV length of the Algorithm
  //
  UINTN                    IcvLength;
  //
  // The block size of the Algorithm
  //
  UINTN                    BlockSize;
  //
  // The function pointer of GetContextSize.
  //
  CRYPTO_HMAC_GETCONTEXTSIZE  HmacGetContextSize;
  //
  // The function pointer of Initiation
  //
  CRYPTO_HMAC_INIT            HmacInitiate;
  //
  // The function pointer of HMAC Update.
  //
  CRYPTO_HMAC_UPDATE          HmacUpdate;
  //
  // The fucntion pointer of HMAC Final
  //
  CRYPTO_HMAC_FINAL           HmacFinal;
} AUTH_ALGORITHM;

//
// The struct used to store the informatino and operation of Hash algorithm.
//
typedef struct _HASH_ALGORITHM {
  //
  // ID of the Algorithm
  //
  UINT8                    AlgorithmId;
  //
  // The Key length of the Algorithm
  //
  UINTN                    DigestLength;
  //
  // The ICV length of the Algorithm
  //
  UINTN                    IcvLength;
  //
  // The block size of the Algorithm
  //
  UINTN                    BlockSize;
  //
  // The function pointer of GetContextSize
  //
  CRYPTO_HASH_GETCONTEXTSIZE  HashGetContextSize;
  //
  // The function pointer of Initiation
  //
  CRYPTO_HASH_INIT            HashInitiate;
  //
  // The function pointer of Hash Update
  //
  CRYPTO_HASH_UPDATE          HashUpdate;
  //
  // The fucntion pointer of Hash Final
  //
  CRYPTO_HASH_FINAL           HashFinal;
} HASH_ALGORITHM;

/**
  Get the IV size of specified encryption alogrithm.

  @param[in]  AlgorithmId          The encryption algorithm ID.

  @return The value of IV size.

**/
UINTN
IpSecGetEncryptIvLength (
  IN UINT8 AlgorithmId
  );

/**
  Get the block size of specified encryption alogrithm.

  @param[in]  AlgorithmId          The encryption algorithm ID.

  @return The value of block size.

**/
UINTN
IpSecGetEncryptBlockSize (
  IN UINT8   AlgorithmId
  );

/**
  Get the required key length of the specified encryption alogrithm.

  @param[in]  AlgorithmId          The encryption algorithm ID.

  @return The value of key length.

**/
UINTN
IpSecGetEncryptKeyLength (
  IN UINT8   AlgorithmId
  );

/**
  Get the ICV size of the specified Authenticaion alogrithm.

  @param[in]  AlgorithmId          The Authentication algorithm ID.

  @return The value of ICV size.

**/
UINTN
IpSecGetIcvLength (
  IN UINT8  AlgorithmId
  );

/**
  Get the HMAC digest length by the specified Algorithm ID.

  @param[in]  AlgorithmId  The specified Alogrithm ID.

  @return The digest length of the specified Authentication Algorithm ID.

**/
UINTN
IpSecGetHmacDigestLength (
  IN UINT8  AlgorithmId
  );

/**
  Generate a random data for IV. If the IvSize is zero, not needed to create
  IV and return EFI_SUCCESS.

  @param[in]  IvBuffer  The pointer of the IV buffer.
  @param[in]  IvSize    The IV size in bytes.

  @retval     EFI_SUCCESS  Create random data for IV.

**/
EFI_STATUS
IpSecGenerateIv (
  IN UINT8                           *IvBuffer,
  IN UINTN                           IvSize
  );

/**
  Encrypt the buffer.

  This function calls relevant encryption interface from CryptoLib according to
  the input alogrithm ID. The InData should be multiple of block size. This function
  doesn't perform the padding. If it has the Ivec data, the length of it should be
  same with the block size. The block size is different from the different algorithm.

  @param[in]       AlgorithmId    The Alogrithem identification defined in RFC.
  @param[in]       Key            Pointer to the buffer containing encrypting key.
  @param[in]       KeyBits        The length of the key in bits.
  @param[in]       Ivec           Point to the buffer containning the Initializeion
                                  Vector (IV) data.
  @param[in]       InData         Point to the buffer containing the data to be
                                  encrypted.
  @param[in]       InDataLength   The length of InData in Bytes.
  @param[out]      OutData        Point to the buffer that receives the encryption
                                  output.

  @retval EFI_UNSUPPORTED       The input Algorithm is not supported.
  @retval EFI_OUT_OF_RESOURCE   The required resource can't be allocated.
  @retval EFI_SUCCESS           The operation completed successfully.

**/
EFI_STATUS
IpSecCryptoIoEncrypt (
  IN CONST UINT8      AlgorithmId,
  IN CONST UINT8      *Key,
  IN CONST UINTN      KeyBits,
  IN CONST UINT8      *Ivec, OPTIONAL
  IN       UINT8      *InData,
  IN       UINTN      InDataLength,
     OUT   UINT8      *OutData
  );

/**
  Decrypts the buffer.

  This function calls relevant Decryption interface from CryptoLib according to
  the input alogrithm ID. The InData should be multiple of block size. This function
  doesn't perform the padding. If it has the Ivec data, the length of it should be
  same with the block size. The block size is different from the different algorithm.

  @param[in]       AlgorithmId    The Alogrithem identification defined in RFC.
  @param[in]       Key            Pointer to the buffer containing encrypting key.
  @param[in]       KeyBits        The length of the key in bits.
  @param[in]       Ivec           Point to the buffer containning the Initializeion
                                  Vector (IV) data.
  @param[in]       InData         Point to the buffer containing the data to be
                                  decrypted.
  @param[in]       InDataLength   The length of InData in Bytes.
  @param[out]      OutData        Pointer to the buffer that receives the decryption
                                  output.

  @retval EFI_UNSUPPORTED       The input Algorithm is not supported.
  @retval EFI_OUT_OF_RESOURCE   The required resource can't be allocated.
  @retval EFI_SUCCESS           The operation completed successfully.

**/
EFI_STATUS
IpSecCryptoIoDecrypt (
  IN CONST UINT8      AlgorithmId,
  IN CONST UINT8      *Key,
  IN CONST UINTN      KeyBits,
  IN CONST UINT8      *Ivec, OPTIONAL
  IN       UINT8      *InData,
  IN       UINTN      InDataLength,
     OUT   UINT8      *OutData
  );

/**
  Digests the Payload with key and store the result into the OutData.

  This function calls relevant Hmac interface from CryptoLib according to
  the input alogrithm ID. It computes all datas from InDataFragment and output
  the result into the OutData buffer. If the OutDataSize is larger than the related
  HMAC alogrithm output size, return EFI_INVALID_PARAMETER.
  
  @param[in]      AlgorithmId     The authentication Identification.
  @param[in]      Key             Pointer of the authentication key.
  @param[in]      KeyLength       The length of the Key in bytes.
  @param[in]      InDataFragment  The list contains all data to be authenticated.
  @param[in]      FragmentCount   The size of the InDataFragment.
  @param[out]     OutData         For in, the buffer to receive the output data.
                                  For out, the buffer contains the authenticated data.
  @param[in]      OutDataSize     The size of the buffer of OutData.

  @retval EFI_UNSUPPORTED       If the AuthAlg is not in the support list.
  @retval EFI_INVALID_PARAMETER The OutData buffer size is larger than algorithm digest size.
  @retval EFI_SUCCESS           Authenticate the payload successfully.
  @retval otherwise             Authentication of the payload fails.

**/
EFI_STATUS
IpSecCryptoIoHmac (
  IN     CONST UINT8              AlgorithmId,
  IN     CONST UINT8              *Key,
  IN           UINTN              KeyLength,
  IN           HASH_DATA_FRAGMENT *InDataFragment,
  IN           UINTN              FragmentCount,
     OUT       UINT8              *OutData,
  IN           UINTN              OutDataSize
  );

/**
  Digests the Payload and store the result into the OutData.

  This function calls relevant Hash interface from CryptoLib according to
  the input alogrithm ID. It computes all datas from InDataFragment and output
  the result into the OutData buffer. If the OutDataSize is larger than the related
  Hash alogrithm output size, return EFI_INVALID_PARAMETER.

  @param[in]      AlgorithmId     The authentication Identification.
  @param[in]      InDataFragment  A list contains all data to be authenticated.
  @param[in]      FragmentCount   The size of the InDataFragment.
  @param[out]     OutData         For in, the buffer to receive the output data.
                                  For out, the buffer contains the authenticated data.
  @param[in]      OutDataSize     The size of the buffer of OutData.

  @retval EFI_UNSUPPORTED       If the AuthAlg is not in the support list.
  @retval EFI_SUCCESS           Authenticated the payload successfully.
  @retval EFI_INVALID_PARAMETER If the OutDataSize is larger than the related Hash
                                algorithm could handle.
  @retval otherwise             Authentication of the payload failed.

**/
EFI_STATUS
IpSecCryptoIoHash (
  IN     CONST UINT8              AlgorithmId,
  IN           HASH_DATA_FRAGMENT *InDataFragment,
  IN           UINTN              FragmentCount,
     OUT       UINT8              *OutData,
  IN           UINTN              OutDataSize
  );

/**
  Generates the Diffie-Hellman public key.

  This function first initiate a DHContext, then call the DhSetParameter() to set
  the prime and primelenght, at end call the DhGenerateKey() to generates random
  secret exponent, and computes the public key. The output returned via parameter
  PublicKey and PublicKeySize. DH context is updated accordingly. If the PublicKey
  buffer is too small to hold the public key, EFI_INVALID_PARAMETER is returned
  and PublicKeySize is set to the required buffer size to obtain the public key.

  @param[in, out] DhContext       Pointer to the DH context.
  @param[in]      Generator       Vlaue of generator.
  @param[in]      PrimeLength     Length in bits of prime to be generated.
  @param[in]      Prime           Pointer to the buffer to receive the generated
                                  prime number.
  @param[out]     PublicKey       Pointer to the buffer to receive generated public key.
  @param[in, out] PublicKeySize   For in, the size of PublicKey buffer in bytes.
                                  For out, the size of data returned in PublicKey
                                  buffer in bytes.

  @retval EFI_SUCCESS             The operation perfoms successfully.
  @retval Otherwise               The operation is failed.

**/
EFI_STATUS
IpSecCryptoIoDhGetPublicKey (
  IN OUT   UINT8  **DhContext,
  IN       UINTN  Generator,
  IN       UINTN  PrimeLength,
  IN CONST UINT8  *Prime,
     OUT   UINT8  *PublicKey,
  IN OUT   UINTN  *PublicKeySize
  );

/**
  Generates exchanged common key.

  Given peer's public key, this function computes the exchanged common key, based
  on its own context including value of prime modulus and random secret exponent.

  @param[in, out] DhContext         Pointer to the DH context.
  @param[in]      PeerPublicKey     Pointer to the peer's Public Key.
  @param[in]      PeerPublicKeySize Size of peer's public key in bytes.
  @param[out]     Key               Pointer to the buffer to receive generated key.
  @param[in, out] KeySize           For in, the size of Key buffer in bytes.
                                    For out, the size of data returned in Key
                                    buffer in bytes.

  @retval EFI_SUCCESS              The operation perfoms successfully.
  @retval Otherwise                The operation is failed.

**/
EFI_STATUS
IpSecCryptoIoDhComputeKey (
  IN   OUT   UINT8  *DhContext,
  IN   CONST UINT8  *PeerPublicKey,
  IN         UINTN  PeerPublicKeySize,
       OUT   UINT8  *Key,
  IN   OUT   UINTN  *KeySize
  );

/**
  Releases the DH context. If DhContext is NULL, return EFI_INVALID_PARAMETER.

  @param[in, out]     DhContext         Pointer to the DH context to be freed.

  @retval EFI_SUCCESS              The operation perfoms successfully.
  @retval EFI_INVALID_PARAMETER    The DhContext is NULL.
  
**/
EFI_STATUS
IpSecCryptoIoFreeDh (
  IN   OUT   UINT8  **DhContext
  );

/**
  Generates random numbers of specified size.

  If the Random Generator wasn't initiated, initiate it first, then call RandomBytes.

  @param[out]  OutBuffer        Pointer to buffer to receive random value.
  @param[in]   Bytes            Size of randome bytes to generate.

  @retval EFI_SUCCESS              The operation perfoms successfully.
  @retval Otherwise                The operation is failed.

**/
EFI_STATUS
IpSecCryptoIoGenerateRandomBytes (
  OUT UINT8*    OutBuffer,
  IN  UINTN     Bytes
  );

/**
  Authenticate data with the certificate.

  @param[in]      InData          Pointer to the Data to be signed.
  @param[in]      InDataSize      InData size in bytes.
  @param[in]      PrivateKey      Pointer to the  private key.
  @param[in]      PrivateKeySize  The size of Private Key in bytes.
  @param[in]      KeyPassWord     Pointer to the password for retrieving private key.
  @param[in]      KeyPwdSize      The size of Key Password in bytes.
  @param[out]     OutData         The pointer to the signed data.
  @param[in, out] OutDataSize     Pointer to contain the size of out data.
 
**/
VOID
IpSecCryptoIoAuthDataWithCertificate (
  IN     UINT8   *InData,
  IN     UINTN   InDataSize,
  IN     UINT8   *PrivateKey,
  IN     UINTN   PrivateKeySize,
  IN     UINT8   *KeyPassWord,
  IN     UINTN   KeyPwdSize,
     OUT UINT8   **OutData,
  IN OUT UINTN   *OutDataSize
  );

/**
  Verify the singed data with the public key which is contained in a certificate.

  @param[in]     InCert          Pointer to the Certificate which contains the
                                 public key.
  @param[in]     CertLen         The size of Certificate in bytes.
  @param[in]     InCa            Pointer to the CA certificate
  @param[in]     CaLen           The size of CA certificate in bytes.
  @param[in]     InData          Pointer to octect message hash to be checked.
  @param[in]     InDataSize      Size of the message hash in bytes.
  @param[in]     Singnature      The pointer to the RSA PKCS1-V1_5 signature to be verifed.
  @param[in]     SigSize         Size of signature in bytes.

  @retval  TRUE   Valid signature encoded in PKCS1-v1_5.
  @retval  FALSE  Invalid signature or invalid RSA context.
 
**/
BOOLEAN
IpSecCryptoIoVerifySignDataByCertificate (
  IN     UINT8   *InCert,
  IN     UINTN   CertLen,
  IN     UINT8   *InCa,
  IN     UINTN   CaLen,
  IN     UINT8   *InData,
  IN     UINTN   InDataSize,
  IN     UINT8   *Singnature,
  IN     UINTN   SigSize
  );

/**
  Retrieves the RSA Public Key from one X509 certificate (DER format only).

  @param[in]     InCert            Pointer to the certificate.
  @param[in]     CertLen           The size of the certificate in bytes.
  @param[out]    PublicKey         Pointer to the retrieved public key.
  @param[out]    PublicKeyLen      Size of Public Key in bytes.

  @retval  EFI_SUCCESS            Successfully get the public Key.
  @retval  EFI_INVALID_PARAMETER  The CA certificate is malformed.

**/
EFI_STATUS
IpSecCryptoIoGetPublicKeyFromCert (
  IN     UINT8   *InCert,
  IN     UINTN   CertLen,
  OUT    UINT8   **PublicKey,
  OUT    UINTN   *PublicKeyLen
  );

/**
  Retrieves the subject name from one X509 certificate (DER format only).

  @param[in]     InCert            Pointer to the X509 certificate.
  @param[in]     CertSize          The size of the X509 certificate in bytes.
  @param[out]    CertSubject       Pointer to the retrieved certificate subject.
  @param[out]    SubjectSize       The size of Certificate Subject in bytes.
  
  @retval  EFI_SUCCESS            Retrieved the certificate subject successfully.
  @retval  EFI_INVALID_PARAMETER  The certificate is malformed.
 
**/
EFI_STATUS
IpSecCryptoIoGetSubjectFromCert (
  IN     UINT8   *InCert,
  IN     UINTN   CertSize,
  OUT    UINT8   **CertSubject,
  OUT    UINTN   *SubjectSize
  );

#endif


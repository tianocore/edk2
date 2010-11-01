/** @file
  Definition related to the Security operation.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

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

#define IPSEC_ENCRYPT_ALGORITHM_LIST_SIZE 2
#define IPSEC_AUTH_ALGORITHM_LIST_SIZE    3

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
  Prototype of Hash GetContextSize.

  Retrieves the size, in bytes, of the context buffer required.

  @return  The size, in bytes, of the context buffer required.

**/
typedef
UINTN
(EFIAPI *CPL_HASH_GETCONTEXTSIZE) (
  VOID
  );

/**
  Prototype of Hash Operation Initiating.

  Initialization with a new context.


  @param[in,out]  Context  Input Context.

  @retval TRUE  Initialization Successfully.

**/
typedef
EFI_STATUS
(EFIAPI *CPL_HASH_INIT) (
  IN OUT  VOID     *Context
  );

/**
  Prototype of HASH update.
  Hash update operation. Continue an Hash message digest operation, processing
  another message block, and updating the Hash context.

  If Context is NULL, then ASSERT().
  If Data is NULL, then ASSERT().

  @param[in,out]  Context     The Specified Context.
  @param[in,out]  Data        The Input Data to hash.
  @param[in]      DataLength  The length, in bytes, of Data.

  @retval TRUE   Update data successfully.
  @retval FALSE  The Context has been finalized.

**/
typedef
BOOLEAN
(EFIAPI *CPL_HASH_UPDATE) (
  IN OUT       VOID  *Context,
  IN     CONST VOID  *Data,
  IN           UINTN DataLength
  );

/**
  Prototype of Hash finallization.
  Terminate a Hash message digest operation and output the message digest.

  If Context is NULL, then ASSERT().
  If HashValue is NULL, then ASSERT().

  @param[in,out]  Context     The specified Context.
  @param[out]     HashValue   Pointer to a 16-byte message digest output buffer.

  @retval TRUE  Finalized successfully.

**/
typedef
BOOLEAN
(EFIAPI *CPL_HASH_FINAL) (
  IN OUT  VOID   *Context,
     OUT  UINT8  *HashValue
  );

/**
  Prototype of Cipher GetContextSize.

  Retrieves the size, in bytes, of the context buffer required.

  @return  The size, in bytes, of the context buffer required.

**/
typedef
UINTN
(EFIAPI *CPL_CIPHER_GETCONTEXTSIZE) (
  VOID
  );

/**
  Prototype of Cipher initiation.
  Intializes the user-supplied key as the specifed context (key materials) for both
  encryption and decryption operations.

  If Context is NULL, then ASSERT().
  If Key is NULL, then generate random key for usage.

  @param[in,out]  Context      The specified Context.
  @param[in]      Key          User-supplied TDES key (64/128/192 bits).
  @param[in]      KeyBits      Key length in bits.

  @retval TRUE  TDES Initialization was successful.

**/
typedef
BOOLEAN
(EFIAPI *CPL_CIPHER_INIT) (
  IN OUT        VOID   *Context,
  IN      CONST UINT8  *Key,
  IN      CONST UINTN  KeyBits
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
  @param[out]     OutData      The resultant encrypted ciphertext.
  @param[in]      DataLength   Length of input data in bytes.

  @retval TRUE  Encryption successful.

**/
typedef
BOOLEAN
(EFIAPI *CPL_CIPHER_ENCRYPT) (
  IN            VOID   *Context,
  IN      CONST UINT8  *InData,
      OUT       UINT8  *OutData,
  IN      CONST UINTN  DataLength
  );


/**
  Prototype of Cipher decryption.
  Decrypts cipher message with specified cipher.

  If Context is NULL, then ASSERT().
  if InData is NULL, then ASSERT().
  If Size of input data is not a multiple of a certaion block size , then ASSERT().

  @param[in]      Context      The specified Context.
  @param[in]      InData       The input ciphertext data to be decrypted.
  @param[out]     OutData      The resultant decrypted plaintext.
  @param[in]      DataLength   Length of input data in bytes.

  @retval TRUE  Decryption successful.

**/
typedef
BOOLEAN
(EFIAPI *CPL_CIPHER_DECRYPT) (
  IN     CONST VOID   *Context,
  IN     CONST UINT8  *InData,
     OUT       UINT8  *OutData,
  IN     CONST UINTN  DataLength
  );

//
// The struct used to store the informatino and operation of  Cipher algorithm.
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
CPL_CIPHER_GETCONTEXTSIZE CipherGetContextSize;
//
// The Function pointer of Cipher intitiaion.
//
CPL_CIPHER_INIT           CipherInitiate;
//
// The Function pointer of Cipher Encryption.
//
CPL_CIPHER_ENCRYPT        CipherEncrypt;
//
// The Function pointer of Cipher Decrption.
//
CPL_CIPHER_DECRYPT        CipherDecrypt;
} ENCRYPT_ALGORITHM;

//
// The struct used to store the informatino and operation of  Autahentication algorithm.
//
typedef struct _AUTH_ALGORITHM {
  //
  // ID of the Algorithm
  //
  UINT8                    AlgorithmId;
  //
  // The Key length of the Algorithm
  //
  UINTN                    KeyLength;
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
  CPL_HASH_GETCONTEXTSIZE  HashGetContextSize;
  //
  // The function pointer of Initiatoion
  //
  CPL_HASH_INIT            HashInitiate;
  //
  // The function pointer of Hash Update.
  //
  CPL_HASH_UPDATE          HashUpdate;
  //
  // The fucntion pointer of Hash Final
  //
  CPL_HASH_FINAL           HashFinal;
} AUTH_ALGORITHM;

/**
  Get the IV size of encrypt alogrithm. IV size is different from different algorithm.

  @param[in]  AlgorithmId          The encrypt algorithm ID.

  @return The value of IV size.

**/
UINTN
IpSecGetEncryptIvLength (
  IN UINT8 AlgorithmId
  );

/**
  Get the block size of encrypt alogrithm. Block size is different from different algorithm.

  @param[in]  AlgorithmId          The encrypt algorithm ID.

  @return The value of block size.

**/
UINTN
IpSecGetEncryptBlockSize (
  IN UINT8   AlgorithmId
  );

/**
  Get the ICV size of Authenticaion alogrithm. ICV size is different from different algorithm.

  @param[in]  AuthAlgorithmId          The Authentication algorithm ID.

  @return The value of ICV size.

**/
UINTN
IpSecGetIcvLength (
  IN UINT8  AuthAlgorithmId
  );

/**
  Generate a random data for IV. If the IvSize is zero, not needed to create
  IV and return EFI_SUCCESS.

  @param[in]  IvBuffer  The pointer of the IV buffer.
  @param[in]  IvSize    The IV size.

  @retval     EFI_SUCCESS  Create random data for IV.

**/
EFI_STATUS
IpSecGenerateIv (
  IN UINT8                           *IvBuffer,
  IN UINTN                           IvSize
  );

#endif


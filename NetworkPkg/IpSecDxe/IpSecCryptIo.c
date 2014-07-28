/** @file
  Common interfaces to call Security library.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecCryptIo.h"
//
// The informations for the supported Encrypt/Decrpt Alogrithm.
//
GLOBAL_REMOVE_IF_UNREFERENCED ENCRYPT_ALGORITHM mIpsecEncryptAlgorithmList[IPSEC_ENCRYPT_ALGORITHM_LIST_SIZE] = {
  {IKE_EALG_NULL, 0, 0, 1, NULL, NULL, NULL, NULL},
  {IKE_EALG_NONE, 0, 0, 1, NULL, NULL, NULL, NULL},  
  {IKE_EALG_3DESCBC, 24, 8, 8, TdesGetContextSize, TdesInit, TdesCbcEncrypt, TdesCbcDecrypt},
  {IKE_EALG_AESCBC, 16, 16, 16, AesGetContextSize, AesInit, AesCbcEncrypt, AesCbcDecrypt}
};

//
// The informations for the supported Authentication algorithm
//
GLOBAL_REMOVE_IF_UNREFERENCED AUTH_ALGORITHM mIpsecAuthAlgorithmList[IPSEC_AUTH_ALGORITHM_LIST_SIZE] = {
  {IKE_AALG_NONE, 0, 0, 0, NULL, NULL, NULL, NULL},
  {IKE_AALG_NULL, 0, 0, 0, NULL, NULL, NULL, NULL},
  {IKE_AALG_SHA1HMAC, 20, 12, 64, HmacSha1GetContextSize, HmacSha1Init, HmacSha1Update, HmacSha1Final}
};

//
// The information for the supported Hash aglorithm
//
GLOBAL_REMOVE_IF_UNREFERENCED HASH_ALGORITHM mIpsecHashAlgorithmList[IPSEC_HASH_ALGORITHM_LIST_SIZE] = {
  {IKE_AALG_NONE, 0, 0, 0, NULL, NULL, NULL, NULL},
  {IKE_AALG_NULL, 0, 0, 0, NULL, NULL, NULL, NULL},
  {IKE_AALG_SHA1HMAC, 20, 12, 64, Sha1GetContextSize, Sha1Init, Sha1Update, Sha1Final}
};

BOOLEAN  mInitialRandomSeed = FALSE;

/**
  Get the block size of specified encryption alogrithm.

  @param[in]  AlgorithmId          The encryption algorithm ID.

  @return The value of block size.

**/
UINTN
IpSecGetEncryptBlockSize (
  IN UINT8   AlgorithmId
  )
{
  UINT8 Index;

  for (Index = 0; Index < IPSEC_ENCRYPT_ALGORITHM_LIST_SIZE; Index++) {
    if (AlgorithmId == mIpsecEncryptAlgorithmList[Index].AlgorithmId) {
      return mIpsecEncryptAlgorithmList[Index].BlockSize;
    }
  }

  return (UINTN) -1;
}

/**
  Get the key length of the specified encryption alogrithm.

  @param[in]  AlgorithmId          The encryption algorithm ID.

  @return The value of key length.

**/
UINTN
IpSecGetEncryptKeyLength (
  IN UINT8   AlgorithmId
  )
{
  UINT8 Index;

  for (Index = 0; Index < IPSEC_ENCRYPT_ALGORITHM_LIST_SIZE; Index++) {
    if (AlgorithmId == mIpsecEncryptAlgorithmList[Index].AlgorithmId) {
      return mIpsecEncryptAlgorithmList[Index].KeyLength;
    }
  }

  return (UINTN) -1;
}

/**
  Get the IV size of the specified encryption alogrithm.

  @param[in]  AlgorithmId          The encryption algorithm ID.

  @return The value of IV size.

**/
UINTN
IpSecGetEncryptIvLength (
  IN UINT8 AlgorithmId
  )
{
  UINT8 Index;

  for (Index = 0; Index < IPSEC_ENCRYPT_ALGORITHM_LIST_SIZE; Index++) {
    if (AlgorithmId == mIpsecEncryptAlgorithmList[Index].AlgorithmId) {
      return mIpsecEncryptAlgorithmList[Index].IvLength;
    }
  }

  return (UINTN) -1;
}

/**
  Get the HMAC digest length by the specified Algorithm ID.

  @param[in]  AlgorithmId  The specified Alogrithm ID.

  @return The digest length of the specified Authentication Algorithm ID.

**/
UINTN
IpSecGetHmacDigestLength (
  IN UINT8  AlgorithmId
  )
{
  UINT8 Index;

  for (Index = 0; Index < IPSEC_AUTH_ALGORITHM_LIST_SIZE; Index++) {
    if (mIpsecAuthAlgorithmList[Index].AlgorithmId == AlgorithmId) {
      //
      // Return the Digest Length of the Algorithm.
      //
      return mIpsecAuthAlgorithmList[Index].DigestLength;
    }
  }

  return 0;
}

/**
  Get the ICV size of the specified Authenticaion alogrithm.

  @param[in]  AlgorithmId          The Authentication algorithm ID.

  @return The value of ICV size.

**/
UINTN
IpSecGetIcvLength (
  IN UINT8  AlgorithmId
  )
{
  UINT8 Index;

  for (Index = 0; Index < IPSEC_AUTH_ALGORITHM_LIST_SIZE; Index++) {
    if (AlgorithmId == mIpsecAuthAlgorithmList[Index].AlgorithmId) {
      return mIpsecAuthAlgorithmList[Index].IcvLength;
    }
  }

  return (UINTN) -1;
}

/**
  Generate a random data for IV. If the IvSize is zero, not needed to create
  IV and return EFI_SUCCESS.

  @param[in]  IvBuffer  The pointer of the IV buffer.
  @param[in]  IvSize    The IV size in bytes.

  @retval     EFI_SUCCESS  Create a random data for IV.

**/
EFI_STATUS
IpSecGenerateIv (
  IN UINT8                           *IvBuffer,
  IN UINTN                           IvSize
  )
{
  if (IvSize != 0) {
    return IpSecCryptoIoGenerateRandomBytes (IvBuffer, IvSize);
  }
  
  return EFI_SUCCESS;
}

/**
  Get index of the specified encryption alogrithm from the mIpsecEncryptAlgorithemList.

  @param[in]  AlgorithmId          The encryption algorithm ID.

  @return the index.
  
**/
UINTN
IpSecGetIndexFromEncList (
  IN UINT8   AlgorithmId
  )
{
  UINT8 Index;
  
  for (Index = 0; Index < IPSEC_ENCRYPT_ALGORITHM_LIST_SIZE; Index++) {
    if (AlgorithmId == mIpsecEncryptAlgorithmList[Index].AlgorithmId) {
      return Index;
    }
  }
  
  return (UINTN) -1;
}

/**
  Get index of the specified encryption alogrithm from the mIpsecAuthAlgorithemList.

  @param[in]  AlgorithmId          The encryption algorithm ID.

  @return the index.
  
**/
UINTN
IpSecGetIndexFromAuthList (
  IN UINT8   AlgorithmId
  )
{
  UINT8 Index;
  
  for (Index = 0; Index < IPSEC_AUTH_ALGORITHM_LIST_SIZE; Index++) {
    if (AlgorithmId == mIpsecAuthAlgorithmList[Index].AlgorithmId) {
      //
      // The BlockSize is same with IvSize.
      //
      return Index;
    }
  }
  
  return (UINTN) -1;
}

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
  )
{  
  UINTN         Index;
  UINTN         ContextSize;
  UINT8         *Context;
  EFI_STATUS    Status;
  
  Status = EFI_UNSUPPORTED;
  
  switch (AlgorithmId) {

  case IKE_EALG_NULL:
  case IKE_EALG_NONE:
    CopyMem (OutData, InData, InDataLength);
    return EFI_SUCCESS;

  case IKE_EALG_3DESCBC:
  case IKE_EALG_AESCBC:
    Index = IpSecGetIndexFromEncList (AlgorithmId);
    if (Index == -1) {
      return Status;
    }
    //
    // Get Context Size
    //
    ContextSize = mIpsecEncryptAlgorithmList[Index].CipherGetContextSize ();
    Context     = AllocateZeroPool (ContextSize);

    if (Context == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Initiate Context
    //
    if (mIpsecEncryptAlgorithmList[Index].CipherInitiate (Context, Key, KeyBits)) {
      if (mIpsecEncryptAlgorithmList[Index].CipherEncrypt (Context, InData, InDataLength, Ivec, OutData)) {
        Status = EFI_SUCCESS;
      }
    }
    break;

  default:
    return Status;

  }

  if (Context != NULL) {
    FreePool (Context);
  }
  
  return Status;
}

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
  )
{  
  UINTN         Index;
  UINTN         ContextSize;
  UINT8         *Context;
  EFI_STATUS    Status;

  Status = EFI_UNSUPPORTED;

  switch (AlgorithmId) {

  case IKE_EALG_NULL:
  case IKE_EALG_NONE:
    CopyMem (OutData, InData, InDataLength);
    return EFI_SUCCESS;

  case IKE_EALG_3DESCBC:
  case IKE_EALG_AESCBC:
    Index = IpSecGetIndexFromEncList(AlgorithmId);
    if (Index == -1) {
      return Status;
    }

    //
    // Get Context Size
    //
    ContextSize = mIpsecEncryptAlgorithmList[Index].CipherGetContextSize();
    Context     = AllocateZeroPool (ContextSize);
    if (Context == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Initiate Context
    //
    if (mIpsecEncryptAlgorithmList[Index].CipherInitiate (Context, Key, KeyBits)) {
      if (mIpsecEncryptAlgorithmList[Index].CipherDecrypt (Context, InData, InDataLength, Ivec, OutData)) {
        Status = EFI_SUCCESS;      
      }
    }
    break;

  default:
    return Status;
  }

  if (Context != NULL) {
    FreePool (Context);
  }

  return Status;
}

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
  )
{
  UINTN        ContextSize;
  UINTN        Index;
  UINT8        FragmentIndex;
  UINT8        *HashContext;
  EFI_STATUS   Status;
  UINT8        *OutHashData;
  UINTN        OutHashSize;

  Status      = EFI_UNSUPPORTED;
  OutHashData = NULL;

  OutHashSize = IpSecGetHmacDigestLength (AlgorithmId);
  //
  // If the expected hash data size is larger than the related Hash algorithm
  // output length, return EFI_INVALID_PARAMETER.
  //
  if (OutDataSize > OutHashSize) {
    return EFI_INVALID_PARAMETER;
  }
  OutHashData = AllocatePool (OutHashSize);

  if (OutHashData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  switch (AlgorithmId) {

  case IKE_AALG_NONE :
  case IKE_AALG_NULL :
    return EFI_SUCCESS;

  case IKE_AALG_SHA1HMAC:
    Index = IpSecGetIndexFromAuthList (AlgorithmId);
    if (Index == -1) {
      return Status;
    }

    //
    // Get Context Size
    //
    ContextSize = mIpsecAuthAlgorithmList[Index].HmacGetContextSize();
    HashContext = AllocateZeroPool (ContextSize);

    if (HashContext == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    //
    // Initiate HMAC context and hash the input data.
    //
    if (mIpsecAuthAlgorithmList[Index].HmacInitiate(HashContext, Key, KeyLength)) {
      for (FragmentIndex = 0; FragmentIndex < FragmentCount; FragmentIndex++) {
        if (!mIpsecAuthAlgorithmList[Index].HmacUpdate (
                HashContext,
                InDataFragment[FragmentIndex].Data,
                InDataFragment[FragmentIndex].DataSize
                )
          ) {
          goto Exit;
        }
      }
      if (mIpsecAuthAlgorithmList[Index].HmacFinal (HashContext, OutHashData)) {
        //
        // In some cases, like the Icv computing, the Icv size might be less than
        // the key length size, so copy the part of hash data to the OutData.
        //
        CopyMem (OutData, OutHashData, OutDataSize);
        Status = EFI_SUCCESS;
      }

      goto Exit;
    }    
      
  default:
    return Status;
  }

Exit:
  if (HashContext != NULL) {
    FreePool (HashContext);
  }
  if (OutHashData != NULL) {
    FreePool (OutHashData);
  }

  return Status;
}

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
  )
{
  UINTN        ContextSize;
  UINTN        Index;
  UINT8        FragmentIndex;
  UINT8        *HashContext;
  EFI_STATUS   Status;
  UINT8        *OutHashData;
  UINTN        OutHashSize;

  Status      = EFI_UNSUPPORTED;
  OutHashData = NULL;
  
  OutHashSize = IpSecGetHmacDigestLength (AlgorithmId);
  //
  // If the expected hash data size is larger than the related Hash algorithm
  // output length, return EFI_INVALID_PARAMETER.  
  //
  if (OutDataSize > OutHashSize) {
    return EFI_INVALID_PARAMETER;
  }
  OutHashData = AllocatePool (OutHashSize);
  if (OutHashData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  switch (AlgorithmId) {

  case IKE_AALG_NONE:
  case IKE_AALG_NULL:
    return EFI_SUCCESS;

  case IKE_AALG_SHA1HMAC:
    Index = IpSecGetIndexFromAuthList (AlgorithmId);
    if (Index == -1) {
      return Status;
    }
    //
    // Get Context Size
    //
    ContextSize = mIpsecHashAlgorithmList[Index].HashGetContextSize();
    HashContext = AllocateZeroPool (ContextSize);
    if (HashContext == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }
    
    //
    // Initiate Hash context and hash the input data.
    //
    if (mIpsecHashAlgorithmList[Index].HashInitiate(HashContext)) {
      for (FragmentIndex = 0; FragmentIndex < FragmentCount; FragmentIndex++) {
        if (!mIpsecHashAlgorithmList[Index].HashUpdate (
                HashContext,
                InDataFragment[FragmentIndex].Data,
                InDataFragment[FragmentIndex].DataSize
                )
          ) {
          goto Exit;
        }
      }
      if (mIpsecHashAlgorithmList[Index].HashFinal (HashContext, OutHashData)) {
        //
        // In some cases, like the Icv computing, the Icv size might be less than
        // the key length size, so copy the part of hash data to the OutData.
        //
        CopyMem (OutData, OutHashData, OutDataSize);            
        Status = EFI_SUCCESS;
      }
      
      goto Exit;        
    }    
    
  default:
    return Status;
  }

Exit:
  if (HashContext != NULL) {
    FreePool (HashContext);
  }
  if (OutHashData != NULL) {
    FreePool (OutHashData);
  }

  return Status;
}

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
  ) 
{
  EFI_STATUS   Status;
  
  *DhContext = DhNew ();
  ASSERT (*DhContext != NULL);
  if (!DhSetParameter (*DhContext, Generator, PrimeLength, Prime)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (!DhGenerateKey (*DhContext, PublicKey, PublicKeySize)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }
  return EFI_SUCCESS;

Exit:
  if (*DhContext != NULL) {
    DhFree (*DhContext);
    DhContext = NULL;
  }
  
  return Status;
}

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
  )
{
  if (!DhComputeKey (DhContext, PeerPublicKey, PeerPublicKeySize, Key, KeySize)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Releases the DH context. If DhContext is NULL, return EFI_INVALID_PARAMETER.

  @param[in, out]     DhContext         Pointer to the DH context to be freed.

  @retval EFI_SUCCESS              The operation perfoms successfully.
  @retval EFI_INVALID_PARAMETER    The DhContext is NULL.
  
**/
EFI_STATUS
IpSecCryptoIoFreeDh (
  IN   OUT   UINT8  **DhContext
  )
{ 
  if (*DhContext == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DhFree (*DhContext);
  return EFI_SUCCESS;
}

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
  )
{
  if (!mInitialRandomSeed) {
    RandomSeed (NULL, 0);
    mInitialRandomSeed = TRUE;
  }
  if (RandomBytes (OutBuffer, Bytes)) {
    return EFI_SUCCESS;
  } else {
    return EFI_INVALID_PARAMETER;
  }
}

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
  )
{
  UINT8         *RsaContext;
  UINT8         *Signature;
  UINTN         SigSize;
   
  SigSize   = 0;
  RsaContext = NULL;

  //
  // Retrieve RSA Private Key from password-protected PEM data
  //
  RsaGetPrivateKeyFromPem (
    (CONST UINT8 *)PrivateKey,
    PrivateKeySize,
    (CONST CHAR8 *)KeyPassWord,
    (VOID **) &RsaContext
    );
  if (RsaContext == NULL) {
    return;
  }

  //
  // Sign data
  //
  Signature = NULL;  
  if (!RsaPkcs1Sign (RsaContext, InData, InDataSize, Signature, &SigSize)) {
    Signature = AllocateZeroPool (SigSize);
  } else {
    return;
  } 

  RsaPkcs1Sign (RsaContext, InData, InDataSize, Signature, &SigSize);

  *OutData     = Signature;
  *OutDataSize = SigSize;

  if (RsaContext != NULL) {
    RsaFree (RsaContext);
  }
}

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
  )
{
  UINT8         *RsaContext;
  BOOLEAN       Status;

  //
  // Create the RSA Context
  //
  RsaContext = RsaNew ();
  if (RsaContext == NULL) {
    return FALSE;
  }

  //
  // Verify the validity of X509 Certificate
  //
  if (!X509VerifyCert (InCert, CertLen, InCa, CaLen)) {
    return FALSE;
  }

  //
  // Retrieve the RSA public Key from Certificate
  //
  RsaGetPublicKeyFromX509 ((CONST UINT8 *)InCert, CertLen, (VOID **)&RsaContext);
 
  //
  // Verify data
  //
  Status = RsaPkcs1Verify (RsaContext, InData, InDataSize, Singnature, SigSize);

  if (RsaContext != NULL) {
    RsaFree (RsaContext);
  }

  return Status;
}

/**
  Retrieves the RSA Public Key from one X509 certificate (DER format only).

  @param[in]     InCert            Pointer to the certificate.
  @param[in]     CertLen           The size of the certificate in bytes.
  @param[out]    PublicKey         Pointer to the retrieved public key.
  @param[out]    PublicKeyLen      Size of Public Key in bytes.

  @retval  EFI_SUCCESS            Successfully get the public Key.
  @retval  EFI_INVALID_PARAMETER  The certificate is malformed.

**/
EFI_STATUS
IpSecCryptoIoGetPublicKeyFromCert (
  IN     UINT8   *InCert,
  IN     UINTN   CertLen,
  OUT    UINT8   **PublicKey,
  OUT    UINTN   *PublicKeyLen
  )
{
  UINT8         *RsaContext;
  EFI_STATUS    Status;

  Status = EFI_SUCCESS;

  //
  // Create the RSA Context
  //
  RsaContext = RsaNew ();

  //
  // Retrieve the RSA public key from CA Certificate
  //
  if (!RsaGetPublicKeyFromX509 ((CONST UINT8 *)InCert, CertLen, (VOID **) &RsaContext)) {
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }

  *PublicKeyLen = 0;
 
  RsaGetKey (RsaContext, RsaKeyN, NULL, PublicKeyLen);
 
  *PublicKey = AllocateZeroPool (*PublicKeyLen);
  ASSERT (*PublicKey != NULL);

  if (!RsaGetKey (RsaContext, RsaKeyN, *PublicKey, PublicKeyLen)) {
    Status = EFI_INVALID_PARAMETER;
  }

EXIT:
  if (RsaContext != NULL) {
    RsaFree (RsaContext);
  }

  return Status;
}

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
  )
{
  EFI_STATUS    Status;

  Status = EFI_SUCCESS;

  *SubjectSize = 0;
  X509GetSubjectName (InCert, CertSize, *CertSubject, SubjectSize);

  *CertSubject = AllocateZeroPool (*SubjectSize);
  if (!X509GetSubjectName (InCert, CertSize, *CertSubject, SubjectSize)) {
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

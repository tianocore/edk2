/** @file MockBaseCryptLib.h
  Google Test mocks for BaseCryptLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_BASE_CRYPT_LIB_H_
#define MOCK_BASE_CRYPT_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/BaseCryptLib.h>
}

struct MockBaseCryptLib {
  MOCK_INTERFACE_DECLARATION (MockBaseCryptLib);

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    Md5GetContextSize,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Md5Init,
    (
     OUT  VOID  *Md5Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Md5Duplicate,
    (
     IN   CONST VOID  *Md5Context,
     OUT  VOID        *NewMd5Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Md5Update,
    (
     IN OUT  VOID        *Md5Context,
     IN      CONST VOID  *Data,
     IN      UINTN       DataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Md5Final,
    (
     IN OUT  VOID   *Md5Context,
     OUT     UINT8  *HashValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Md5HashAll,
    (
     IN   CONST VOID  *Data,
     IN   UINTN       DataSize,
     OUT  UINT8       *HashValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    Sha1GetContextSize,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha1Init,
    (
     OUT  VOID  *Sha1Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha1Duplicate,
    (
     IN   CONST VOID  *Sha1Context,
     OUT  VOID        *NewSha1Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha1Update,
    (
     IN OUT  VOID        *Sha1Context,
     IN      CONST VOID  *Data,
     IN      UINTN       DataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha1Final,
    (
     IN OUT  VOID   *Sha1Context,
     OUT     UINT8  *HashValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha1HashAll,
    (
     IN   CONST VOID  *Data,
     IN   UINTN       DataSize,
     OUT  UINT8       *HashValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    Sha256GetContextSize,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha256Init,
    (
     OUT  VOID  *Sha256Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha256Duplicate,
    (
     IN   CONST VOID  *Sha256Context,
     OUT  VOID        *NewSha256Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha256Update,
    (
     IN OUT  VOID        *Sha256Context,
     IN      CONST VOID  *Data,
     IN      UINTN       DataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha256Final,
    (
     IN OUT  VOID   *Sha256Context,
     OUT     UINT8  *HashValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha256HashAll,
    (
     IN   CONST VOID  *Data,
     IN   UINTN       DataSize,
     OUT  UINT8       *HashValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    Sha384GetContextSize,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha384Init,
    (
     OUT  VOID  *Sha384Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha384Duplicate,
    (
     IN   CONST VOID  *Sha384Context,
     OUT  VOID        *NewSha384Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha384Update,
    (
     IN OUT  VOID        *Sha384Context,
     IN      CONST VOID  *Data,
     IN      UINTN       DataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha384Final,
    (
     IN OUT  VOID   *Sha384Context,
     OUT     UINT8  *HashValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha384HashAll,
    (
     IN   CONST VOID  *Data,
     IN   UINTN       DataSize,
     OUT  UINT8       *HashValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    Sha512GetContextSize,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha512Init,
    (
     OUT  VOID  *Sha512Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha512Duplicate,
    (
     IN   CONST VOID  *Sha512Context,
     OUT  VOID        *NewSha512Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha512Update,
    (
     IN OUT  VOID        *Sha512Context,
     IN      CONST VOID  *Data,
     IN      UINTN       DataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha512Final,
    (
     IN OUT  VOID   *Sha512Context,
     OUT     UINT8  *HashValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sha512HashAll,
    (
     IN   CONST VOID  *Data,
     IN   UINTN       DataSize,
     OUT  UINT8       *HashValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    Sm3GetContextSize,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sm3Init,
    (
     OUT  VOID  *Sm3Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sm3Duplicate,
    (
     IN   CONST VOID  *Sm3Context,
     OUT  VOID        *NewSm3Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sm3Update,
    (
     IN OUT  VOID        *Sm3Context,
     IN      CONST VOID  *Data,
     IN      UINTN       DataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sm3Final,
    (
     IN OUT  VOID   *Sm3Context,
     OUT     UINT8  *HashValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Sm3HashAll,
    (
     IN   CONST VOID  *Data,
     IN   UINTN       DataSize,
     OUT  UINT8       *HashValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    HmacSha256New,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    HmacSha256Free,
    (
     IN  VOID  *HmacSha256Ctx
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HmacSha256SetKey,
    (
     OUT  VOID         *HmacSha256Context,
     IN   CONST UINT8  *Key,
     IN   UINTN        KeySize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HmacSha256Duplicate,
    (
     IN   CONST VOID  *HmacSha256Context,
     OUT  VOID        *NewHmacSha256Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HmacSha256Update,
    (
     IN OUT  VOID        *HmacSha256Context,
     IN      CONST VOID  *Data,
     IN      UINTN       DataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HmacSha256Final,
    (
     IN OUT  VOID   *HmacSha256Context,
     OUT     UINT8  *HmacValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HmacSha256All,
    (
     IN   CONST VOID   *Data,
     IN   UINTN        DataSize,
     IN   CONST UINT8  *Key,
     IN   UINTN        KeySize,
     OUT  UINT8        *HmacValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    HmacSha384New,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    HmacSha384Free,
    (
     IN  VOID  *HmacSha384Ctx
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HmacSha384SetKey,
    (
     OUT  VOID         *HmacSha384Context,
     IN   CONST UINT8  *Key,
     IN   UINTN        KeySize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HmacSha384Duplicate,
    (
     IN   CONST VOID  *HmacSha384Context,
     OUT  VOID        *NewHmacSha384Context
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HmacSha384Update,
    (
     IN OUT  VOID        *HmacSha384Context,
     IN      CONST VOID  *Data,
     IN      UINTN       DataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HmacSha384Final,
    (
     IN OUT  VOID   *HmacSha384Context,
     OUT     UINT8  *HmacValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HmacSha384All,
    (
     IN   CONST VOID   *Data,
     IN   UINTN        DataSize,
     IN   CONST UINT8  *Key,
     IN   UINTN        KeySize,
     OUT  UINT8        *HmacValue
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    AesGetContextSize,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    AesInit,
    (
     OUT  VOID         *AesContext,
     IN   CONST UINT8  *Key,
     IN   UINTN        KeyLength
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    AesCbcEncrypt,
    (
     IN   VOID         *AesContext,
     IN   CONST UINT8  *Input,
     IN   UINTN        InputSize,
     IN   CONST UINT8  *Ivec,
     OUT  UINT8        *Output
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    AesCbcDecrypt,
    (
     IN   VOID         *AesContext,
     IN   CONST UINT8  *Input,
     IN   UINTN        InputSize,
     IN   CONST UINT8  *Ivec,
     OUT  UINT8        *Output
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    AeadAesGcmEncrypt,
    (
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
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    AeadAesGcmDecrypt,
    (
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
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    RsaNew,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    RsaFree,
    (
     IN  VOID  *RsaContext
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RsaSetKey,
    (
     IN OUT  VOID         *RsaContext,
     IN      RSA_KEY_TAG  KeyTag,
     IN      CONST UINT8  *BigNumber,
     IN      UINTN        BnSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RsaGetKey,
    (
     IN OUT  VOID         *RsaContext,
     IN      RSA_KEY_TAG  KeyTag,
     OUT     UINT8        *BigNumber,
     IN OUT  UINTN        *BnSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RsaGenerateKey,
    (
     IN OUT  VOID         *RsaContext,
     IN      UINTN        ModulusLength,
     IN      CONST UINT8  *PublicExponent,
     IN      UINTN        PublicExponentSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RsaCheckKey,
    (
     IN  VOID  *RsaContext
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RsaPkcs1Sign,
    (
     IN      VOID         *RsaContext,
     IN      CONST UINT8  *MessageHash,
     IN      UINTN        HashSize,
     OUT     UINT8        *Signature,
     IN OUT  UINTN        *SigSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RsaPkcs1Verify,
    (
     IN  VOID         *RsaContext,
     IN  CONST UINT8  *MessageHash,
     IN  UINTN        HashSize,
     IN  CONST UINT8  *Signature,
     IN  UINTN        SigSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RsaPssSign,
    (
     IN      VOID         *RsaContext,
     IN      CONST UINT8  *Message,
     IN      UINTN        MsgSize,
     IN      UINT16       DigestLen,
     IN      UINT16       SaltLen,
     OUT     UINT8        *Signature,
     IN OUT  UINTN        *SigSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RsaPssVerify,
    (
     IN  VOID         *RsaContext,
     IN  CONST UINT8  *Message,
     IN  UINTN        MsgSize,
     IN  CONST UINT8  *Signature,
     IN  UINTN        SigSize,
     IN  UINT16       DigestLen,
     IN  UINT16       SaltLen
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RsaGetPrivateKeyFromPem,
    (
     IN   CONST UINT8  *PemData,
     IN   UINTN        PemSize,
     IN   CONST CHAR8  *Password,
     OUT  VOID         **RsaContext
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RsaGetPublicKeyFromX509,
    (
     IN   CONST UINT8  *Cert,
     IN   UINTN        CertSize,
     OUT  VOID         **RsaContext
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509GetSubjectName,
    (
     IN      CONST UINT8  *Cert,
     IN      UINTN        CertSize,
     OUT     UINT8        *CertSubject,
     IN OUT  UINTN        *SubjectSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    X509GetCommonName,
    (
     IN      CONST UINT8  *Cert,
     IN      UINTN        CertSize,
     OUT     CHAR8        *CommonName   OPTIONAL,
     IN OUT  UINTN        *CommonNameSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    X509GetOrganizationName,
    (
     IN      CONST UINT8  *Cert,
     IN      UINTN        CertSize,
     OUT     CHAR8        *NameBuffer   OPTIONAL,
     IN OUT  UINTN        *NameBufferSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509VerifyCert,
    (
     IN  CONST UINT8  *Cert,
     IN  UINTN        CertSize,
     IN  CONST UINT8  *CACert,
     IN  UINTN        CACertSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509ConstructCertificate,
    (
     IN   CONST UINT8  *Cert,
     IN   UINTN        CertSize,
     OUT  UINT8        **SingleX509Cert
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    X509Free,
    (
     IN  VOID  *X509Cert
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    X509StackFree,
    (
     IN  VOID  *X509Stack
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509GetTBSCert,
    (
     IN  CONST UINT8  *Cert,
     IN  UINTN        CertSize,
     OUT UINT8        **TBSCert,
     OUT UINTN        *TBSCertSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Pkcs5HashPassword,
    (
     IN  UINTN        PasswordLength,
     IN  CONST CHAR8  *Password,
     IN  UINTN        SaltLength,
     IN  CONST UINT8  *Salt,
     IN  UINTN        IterationCount,
     IN  UINTN        DigestSize,
     IN  UINTN        KeyLength,
     OUT UINT8        *OutKey
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Pkcs1v2Encrypt,
    (
     IN   CONST UINT8  *PublicKey,
     IN   UINTN        PublicKeySize,
     IN   UINT8        *InData,
     IN   UINTN        InDataSize,
     IN   CONST UINT8  *PrngSeed   OPTIONAL,
     IN   UINTN        PrngSeedSize   OPTIONAL,
     OUT  UINT8        **EncryptedData,
     OUT  UINTN        *EncryptedDataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RsaOaepEncrypt,
    (
     IN   VOID         *RsaContext,
     IN   UINT8        *InData,
     IN   UINTN        InDataSize,
     IN   CONST UINT8  *PrngSeed   OPTIONAL,
     IN   UINTN        PrngSeedSize   OPTIONAL,
     IN   UINT16       DigestLen   OPTIONAL,
     OUT  UINT8        **EncryptedData,
     OUT  UINTN        *EncryptedDataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Pkcs1v2Decrypt,
    (
     IN   CONST UINT8  *PrivateKey,
     IN   UINTN        PrivateKeySize,
     IN   UINT8        *EncryptedData,
     IN   UINTN        EncryptedDataSize,
     OUT  UINT8        **OutData,
     OUT  UINTN        *OutDataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RsaOaepDecrypt,
    (
     IN   VOID    *RsaContext,
     IN   UINT8   *EncryptedData,
     IN   UINTN   EncryptedDataSize,
     IN   UINT16  DigestLen   OPTIONAL,
     OUT  UINT8   **OutData,
     OUT  UINTN   *OutDataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Pkcs7GetSigners,
    (
     IN  CONST UINT8  *P7Data,
     IN  UINTN        P7Length,
     OUT UINT8        **CertStack,
     OUT UINTN        *StackLength,
     OUT UINT8        **TrustedCert,
     OUT UINTN        *CertLength
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    Pkcs7FreeSigners,
    (
     IN  UINT8  *Certs
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Pkcs7GetCertificatesList,
    (
     IN  CONST UINT8  *P7Data,
     IN  UINTN        P7Length,
     OUT UINT8        **SignerChainCerts,
     OUT UINTN        *ChainLength,
     OUT UINT8        **UnchainCerts,
     OUT UINTN        *UnchainLength
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Pkcs7Sign,
    (
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
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Pkcs7Verify,
    (
     IN  CONST UINT8  *P7Data,
     IN  UINTN        P7Length,
     IN  CONST UINT8  *TrustedCert,
     IN  UINTN        CertLength,
     IN  CONST UINT8  *InData,
     IN  UINTN        DataLength
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Pkcs7Encrypt,
    (
     IN   UINT8   *X509Stack,
     IN   UINT8   *InData,
     IN   UINTN   InDataSize,
     IN   UINT32  CipherNid,
     IN   UINT32  Flags,
     OUT  UINT8   **ContentInfo,
     OUT  UINTN   *ContentInfoSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    VerifyEKUsInPkcs7Signature,
    (
     IN  CONST UINT8   *Pkcs7Signature,
     IN  CONST UINT32  SignatureSize,
     IN  CONST CHAR8   *RequiredEKUs[],
     IN  CONST UINT32  RequiredEKUsSize,
     IN  BOOLEAN       RequireAllPresent
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Pkcs7GetAttachedContent,
    (
     IN  CONST UINT8  *P7Data,
     IN  UINTN        P7Length,
     OUT VOID         **Content,
     OUT UINTN        *ContentSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    AuthenticodeVerify,
    (
     IN  CONST UINT8  *AuthData,
     IN  UINTN        DataSize,
     IN  CONST UINT8  *TrustedCert,
     IN  UINTN        CertSize,
     IN  CONST UINT8  *ImageHash,
     IN  UINTN        HashSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ImageTimestampVerify,
    (
     IN  CONST UINT8  *AuthData,
     IN  UINTN        DataSize,
     IN  CONST UINT8  *TsaCert,
     IN  UINTN        CertSize,
     OUT EFI_TIME     *SigningTime
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509GetVersion,
    (
     IN      CONST UINT8  *Cert,
     IN      UINTN        CertSize,
     OUT     UINTN        *Version
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509GetSerialNumber,
    (
     IN      CONST UINT8 *Cert,
     IN      UINTN CertSize,
     OUT     UINT8 *SerialNumber, OPTIONAL
     IN OUT  UINTN         *SerialNumberSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509GetIssuerName,
    (
     IN      CONST UINT8  *Cert,
     IN      UINTN        CertSize,
     OUT     UINT8        *CertIssuer,
     IN OUT  UINTN        *CertIssuerSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509GetSignatureAlgorithm,
    (
     IN CONST UINT8 *Cert,
     IN       UINTN CertSize,
     OUT   UINT8 *Oid, OPTIONAL
     IN OUT   UINTN       *OidSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509GetExtensionData,
    (
     IN     CONST UINT8  *Cert,
     IN     UINTN        CertSize,
     IN     CONST UINT8  *Oid,
     IN     UINTN        OidSize,
     OUT UINT8           *ExtensionData,
     IN OUT UINTN        *ExtensionDataSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509GetValidity,
    (
     IN     CONST UINT8  *Cert,
     IN     UINTN        CertSize,
     IN     UINT8        *From,
     IN OUT UINTN        *FromSize,
     IN     UINT8        *To,
     IN OUT UINTN        *ToSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509FormatDateTime,
    (
     IN   CONST CHAR8  *DateTimeStr,
     OUT  VOID         *DateTime,
     IN OUT UINTN      *DateTimeSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    INT32,
    X509CompareDateTime,
    (
     IN  CONST  VOID  *DateTime1,
     IN  CONST  VOID  *DateTime2
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509GetKeyUsage,
    (
     IN    CONST UINT8  *Cert,
     IN    UINTN        CertSize,
     OUT   UINTN        *Usage
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509GetExtendedKeyUsage,
    (
     IN     CONST UINT8  *Cert,
     IN     UINTN        CertSize,
     OUT UINT8           *Usage,
     IN OUT UINTN        *UsageSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509VerifyCertChain,
    (
     IN CONST UINT8  *RootCert,
     IN UINTN        RootCertLength,
     IN CONST UINT8  *CertChain,
     IN UINTN        CertChainLength
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509GetCertFromCertChain,
    (
     IN CONST UINT8   *CertChain,
     IN UINTN         CertChainLength,
     IN CONST INT32   CertIndex,
     OUT CONST UINT8  **Cert,
     OUT UINTN        *CertLength
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    Asn1GetTag,
    (
     IN OUT UINT8    **Ptr,
     IN CONST UINT8  *End,
     OUT UINTN       *Length,
     IN     UINT32   Tag
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    X509GetExtendedBasicConstraints,
    (
     CONST UINT8  *Cert,
     UINTN        CertSize,
     UINT8        *BasicConstraints,
     UINTN        *BasicConstraintsSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    DhNew,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    DhFree,
    (
     IN  VOID  *DhContext
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    DhGenerateParameter,
    (
     IN OUT  VOID   *DhContext,
     IN      UINTN  Generator,
     IN      UINTN  PrimeLength,
     OUT     UINT8  *Prime
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    DhSetParameter,
    (
     IN OUT  VOID         *DhContext,
     IN      UINTN        Generator,
     IN      UINTN        PrimeLength,
     IN      CONST UINT8  *Prime
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    DhGenerateKey,
    (
     IN OUT  VOID   *DhContext,
     OUT     UINT8  *PublicKey,
     IN OUT  UINTN  *PublicKeySize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    DhComputeKey,
    (
     IN OUT  VOID         *DhContext,
     IN      CONST UINT8  *PeerPublicKey,
     IN      UINTN        PeerPublicKeySize,
     OUT     UINT8        *Key,
     IN OUT  UINTN        *KeySize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RandomSeed,
    (
     IN  CONST  UINT8  *Seed  OPTIONAL,
     IN  UINTN         SeedSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    RandomBytes,
    (
     OUT  UINT8  *Output,
     IN   UINTN  Size
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HkdfSha256ExtractAndExpand,
    (
     IN   CONST UINT8  *Key,
     IN   UINTN        KeySize,
     IN   CONST UINT8  *Salt,
     IN   UINTN        SaltSize,
     IN   CONST UINT8  *Info,
     IN   UINTN        InfoSize,
     OUT  UINT8        *Out,
     IN   UINTN        OutSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HkdfSha256Extract,
    (
     IN CONST UINT8  *Key,
     IN UINTN        KeySize,
     IN CONST UINT8  *Salt,
     IN UINTN        SaltSize,
     OUT UINT8       *PrkOut,
     UINTN           PrkOutSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HkdfSha256Expand,
    (
     IN   CONST UINT8  *Prk,
     IN   UINTN        PrkSize,
     IN   CONST UINT8  *Info,
     IN   UINTN        InfoSize,
     OUT  UINT8        *Out,
     IN   UINTN        OutSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HkdfSha384ExtractAndExpand,
    (
     IN   CONST UINT8  *Key,
     IN   UINTN        KeySize,
     IN   CONST UINT8  *Salt,
     IN   UINTN        SaltSize,
     IN   CONST UINT8  *Info,
     IN   UINTN        InfoSize,
     OUT  UINT8        *Out,
     IN   UINTN        OutSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HkdfSha384Extract,
    (
     IN CONST UINT8  *Key,
     IN UINTN        KeySize,
     IN CONST UINT8  *Salt,
     IN UINTN        SaltSize,
     OUT UINT8       *PrkOut,
     UINTN           PrkOutSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HkdfSha384Expand,
    (
     IN   CONST UINT8  *Prk,
     IN   UINTN        PrkSize,
     IN   CONST UINT8  *Info,
     IN   UINTN        InfoSize,
     OUT  UINT8        *Out,
     IN   UINTN        OutSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    BigNumInit,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    BigNumFromBin,
    (
     IN CONST UINT8  *Buf,
     IN UINTN        Len
    )
    );

  MOCK_FUNCTION_DECLARATION (
    INTN,
    BigNumToBin,
    (
     IN CONST VOID  *Bn,
     OUT UINT8      *Buf
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    BigNumFree,
    (
     IN VOID     *Bn,
     IN BOOLEAN  Clear
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    BigNumAdd,
    (
     IN CONST VOID  *BnA,
     IN CONST VOID  *BnB,
     OUT VOID       *BnRes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    BigNumSub,
    (
     IN CONST VOID  *BnA,
     IN CONST VOID  *BnB,
     OUT VOID       *BnRes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    BigNumMod,
    (
     IN CONST VOID  *BnA,
     IN CONST VOID  *BnB,
     OUT VOID       *BnRes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    BigNumExpMod,
    (
     IN CONST VOID  *BnA,
     IN CONST VOID  *BnP,
     IN CONST VOID  *BnM,
     OUT VOID       *BnRes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    BigNumInverseMod,
    (
     IN CONST VOID  *BnA,
     IN CONST VOID  *BnM,
     OUT VOID       *BnRes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    BigNumDiv,
    (
     IN CONST VOID  *BnA,
     IN CONST VOID  *BnB,
     OUT VOID       *BnRes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    BigNumMulMod,
    (
     IN CONST VOID  *BnA,
     IN CONST VOID  *BnB,
     IN CONST VOID  *BnM,
     OUT VOID       *BnRes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    INTN,
    BigNumCmp,
    (
     IN CONST VOID  *BnA,
     IN CONST VOID  *BnB
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    BigNumBits,
    (
     IN CONST VOID  *Bn
    )
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    BigNumBytes,
    (
     IN CONST VOID  *Bn
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    BigNumIsWord,
    (
     IN CONST VOID  *Bn,
     IN UINTN       Num
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    BigNumIsOdd,
    (
     IN CONST VOID  *Bn
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    BigNumCopy,
    (
     OUT VOID       *BnDst,
     IN CONST VOID  *BnSrc
    )
    );

  MOCK_FUNCTION_DECLARATION (
    CONST VOID *,
    BigNumValueOne,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    BigNumRShift,
    (
     IN CONST VOID  *Bn,
     IN UINTN       N,
     OUT VOID       *BnRes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    BigNumConstTime,
    (
     IN VOID  *Bn
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    BigNumSqrMod,
    (
     IN CONST VOID  *BnA,
     IN CONST VOID  *BnM,
     OUT VOID       *BnRes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    BigNumNewContext,
    (
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    BigNumContextFree,
    (
     IN VOID  *BnCtx
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    BigNumSetUint,
    (
     IN VOID   *Bn,
     IN UINTN  Val
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    BigNumAddMod,
    (
     IN CONST VOID  *BnA,
     IN CONST VOID  *BnB,
     IN CONST VOID  *BnM,
     OUT VOID       *BnRes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    EcGroupInit,
    (
     IN UINTN  CryptoNid
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcGroupGetCurve,
    (
     IN CONST VOID  *EcGroup,
     OUT VOID       *BnPrime,
     OUT VOID       *BnA,
     OUT VOID       *BnB,
     IN VOID        *BnCtx
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcGroupGetOrder,
    (
     IN VOID   *EcGroup,
     OUT VOID  *BnOrder
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    EcGroupFree,
    (
     IN VOID  *EcGroup
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    EcPointInit,
    (
     IN CONST VOID  *EcGroup
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    EcPointDeInit,
    (
     IN VOID     *EcPoint,
     IN BOOLEAN  Clear
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcPointGetAffineCoordinates,
    (
     IN CONST VOID  *EcGroup,
     IN CONST VOID  *EcPoint,
     OUT VOID       *BnX,
     OUT VOID       *BnY,
     IN VOID        *BnCtx
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcPointSetAffineCoordinates,
    (
     IN CONST VOID  *EcGroup,
     IN VOID        *EcPoint,
     IN CONST VOID  *BnX,
     IN CONST VOID  *BnY,
     IN VOID        *BnCtx
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcPointAdd,
    (
     IN CONST VOID  *EcGroup,
     OUT VOID       *EcPointResult,
     IN CONST VOID  *EcPointA,
     IN CONST VOID  *EcPointB,
     IN VOID        *BnCtx
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcPointMul,
    (
     IN CONST VOID  *EcGroup,
     OUT VOID       *EcPointResult,
     IN CONST VOID  *EcPoint,
     IN CONST VOID  *BnPScalar,
     IN VOID        *BnCtx
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcPointInvert,
    (
     IN CONST VOID  *EcGroup,
     IN OUT VOID    *EcPoint,
     IN VOID        *BnCtx
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcPointIsOnCurve,
    (
     IN CONST VOID  *EcGroup,
     IN CONST VOID  *EcPoint,
     IN VOID        *BnCtx
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcPointIsAtInfinity,
    (
     IN CONST VOID  *EcGroup,
     IN CONST VOID  *EcPoint
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcPointEqual,
    (
     IN CONST VOID  *EcGroup,
     IN CONST VOID  *EcPointA,
     IN CONST VOID  *EcPointB,
     IN VOID        *BnCtx
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcPointSetCompressedCoordinates,
    (
     IN CONST VOID  *EcGroup,
     IN VOID        *EcPoint,
     IN CONST VOID  *BnX,
     IN UINT8       YBit,
     IN VOID        *BnCtx
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    EcNewByNid,
    (
     IN UINTN  Nid
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    EcFree,
    (
     IN  VOID  *EcContext
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcGenerateKey,
    (
     IN OUT  VOID   *EcContext,
     OUT     UINT8  *PublicKey,
     IN OUT  UINTN  *PublicKeySize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcGetPubKey,
    (
     IN OUT  VOID   *EcContext,
     OUT     UINT8  *PublicKey,
     IN OUT  UINTN  *PublicKeySize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcDhComputeKey,
    (
     IN OUT  VOID         *EcContext,
     IN      CONST UINT8  *PeerPublic,
     IN      UINTN        PeerPublicSize,
     IN      CONST INT32  *CompressFlag,
     OUT     UINT8        *Key,
     IN OUT  UINTN        *KeySize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcGetPrivateKeyFromPem,
    (
     IN   CONST UINT8  *PemData,
     IN   UINTN        PemSize,
     IN   CONST CHAR8  *Password,
     OUT  VOID         **EcContext
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcGetPublicKeyFromX509,
    (
     IN   CONST UINT8  *Cert,
     IN   UINTN        CertSize,
     OUT  VOID         **EcContext
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcDsaSign,
    (
     IN      VOID         *EcContext,
     IN      UINTN        HashNid,
     IN      CONST UINT8  *MessageHash,
     IN      UINTN        HashSize,
     OUT     UINT8        *Signature,
     IN OUT  UINTN        *SigSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EcDsaVerify,
    (
     IN  VOID         *EcContext,
     IN  UINTN        HashNid,
     IN  CONST UINT8  *MessageHash,
     IN  UINTN        HashSize,
     IN  CONST UINT8  *Signature,
     IN  UINTN        SigSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetCryptoProviderVersionString,
    (
     OUT    CHAR8  *Buffer,
     IN OUT UINTN  *BufferSize
    )
    );
};

#endif

/** @file
  Defines the PCD_CRYPTO_SERVICE_FAMILY_ENABLE structure associated with
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable that is used
  to enable/disable crypto services at either the family scope or the
  individual service scope.  Platforms can minimize the number of enabled
  services to reduce size.

  The following services have been deprecated and must never be enabled.
  The associated fields in this data structure are never removed or replaced
  to preseve the binary layout of the data structure.  New services are
  always added to the end of the data structure.
  * HmacMd5 family
  * HmacSha1 family
  * Md4 family
  * Md5 family
  * Tdes family
  * Arc4 family
  * Aes.Services.EcbEncrypt service
  * Aes.Services.EcbDecrypt service

  Is is recommended that the following services always be disabled and may
  be deprecated in the future.
  * Sha1 family

  Copyright (c) 2019 - 2022, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PCD_CRYPTO_SERVICE_FAMILY_ENABLE_H__
#define __PCD_CRYPTO_SERVICE_FAMILY_ENABLE_H__

///
/// Define used to enable all the crypto services in a family
///
#define PCD_CRYPTO_SERVICE_ENABLE_FAMILY  0xFFFFFFFF

///
/// PCD_CRYPTO_SERVICE_FAMILY_ENABLE structure.  Each field in this structure
/// is associated with a service in the EDK II Crypto Protocol/PPI.  This allows
/// each individual service to be enabled/disabled in a DSC file.  Services are
/// also grouped into families.  Unions are used to support enabling or
/// disabling an entire family in a single DSC statement.
///
typedef struct {
  union {
    struct {
      UINT8    New       : 1;  // Deprecated
      UINT8    Free      : 1;  // Deprecated
      UINT8    SetKey    : 1;  // Deprecated
      UINT8    Duplicate : 1;  // Deprecated
      UINT8    Update    : 1;  // Deprecated
      UINT8    Final     : 1;  // Deprecated
    } Services;
    UINT32    Family;          // Deprecated
  } HmacMd5;
  union {
    struct {
      UINT8    New       : 1;  // Deprecated
      UINT8    Free      : 1;  // Deprecated
      UINT8    SetKey    : 1;  // Deprecated
      UINT8    Duplicate : 1;  // Deprecated
      UINT8    Update    : 1;  // Deprecated
      UINT8    Final     : 1;  // Deprecated
    } Services;
    UINT32    Family;          // Deprecated
  } HmacSha1;
  union {
    struct {
      UINT8    New       : 1;
      UINT8    Free      : 1;
      UINT8    SetKey    : 1;
      UINT8    Duplicate : 1;
      UINT8    Update    : 1;
      UINT8    Final     : 1;
      UINT8    All       : 1;
    } Services;
    UINT32    Family;
  } HmacSha256;
  union {
    struct {
      UINT8    New       : 1;
      UINT8    Free      : 1;
      UINT8    SetKey    : 1;
      UINT8    Duplicate : 1;
      UINT8    Update    : 1;
      UINT8    Final     : 1;
      UINT8    All       : 1;
    } Services;
    UINT32    Family;
  } HmacSha384;
  union {
    struct {
      UINT8    GetContextSize : 1;  // Deprecated
      UINT8    Init           : 1;  // Deprecated
      UINT8    Duplicate      : 1;  // Deprecated
      UINT8    Update         : 1;  // Deprecated
      UINT8    Final          : 1;  // Deprecated
      UINT8    HashAll        : 1;  // Deprecated
    } Services;
    UINT32    Family;               // Deprecated
  } Md4;
  union {
    struct {
      UINT8    GetContextSize : 1;  // Deprecated
      UINT8    Init           : 1;  // Deprecated
      UINT8    Duplicate      : 1;  // Deprecated
      UINT8    Update         : 1;  // Deprecated
      UINT8    Final          : 1;  // Deprecated
      UINT8    HashAll        : 1;  // Deprecated
    } Services;
    UINT32    Family;
  } Md5;                            // Deprecated
  union {
    struct {
      UINT8    Pkcs1v2Encrypt             : 1;
      UINT8    Pkcs5HashPassword          : 1;
      UINT8    Pkcs7Verify                : 1;
      UINT8    VerifyEKUsInPkcs7Signature : 1;
      UINT8    Pkcs7GetSigners            : 1;
      UINT8    Pkcs7FreeSigners           : 1;
      UINT8    Pkcs7Sign                  : 1;
      UINT8    Pkcs7GetAttachedContent    : 1;
      UINT8    Pkcs7GetCertificatesList   : 1;
      UINT8    AuthenticodeVerify         : 1;
      UINT8    ImageTimestampVerify       : 1;
      UINT8    Pkcs1v2Decrypt             : 1;
    } Services;
    UINT32    Family;
  } Pkcs;
  union {
    struct {
      UINT8    New               : 1;
      UINT8    Free              : 1;
      UINT8    GenerateParameter : 1;
      UINT8    SetParameter      : 1;
      UINT8    GenerateKey       : 1;
      UINT8    ComputeKey        : 1;
    } Services;
    UINT32    Family;
  } Dh;
  union {
    struct {
      UINT8    Seed  : 1;
      UINT8    Bytes : 1;
    } Services;
    UINT32    Family;
  } Random;
  union {
    struct {
      UINT8    VerifyPkcs1          : 1;
      UINT8    New                  : 1;
      UINT8    Free                 : 1;
      UINT8    SetKey               : 1;
      UINT8    GetKey               : 1;
      UINT8    GenerateKey          : 1;
      UINT8    CheckKey             : 1;
      UINT8    Pkcs1Sign            : 1;
      UINT8    Pkcs1Verify          : 1;
      UINT8    GetPrivateKeyFromPem : 1;
      UINT8    GetPublicKeyFromX509 : 1;
      UINT8    RsaOaepEncrypt       : 1;
      UINT8    RsaOaepDecrypt       : 1;
    } Services;
    UINT32    Family;
  } Rsa;
  union {
    struct {
      UINT8    GetContextSize : 1;  // Recommend disable
      UINT8    Init           : 1;  // Recommend disable
      UINT8    Duplicate      : 1;  // Recommend disable
      UINT8    Update         : 1;  // Recommend disable
      UINT8    Final          : 1;  // Recommend disable
      UINT8    HashAll        : 1;  // Recommend disable
    } Services;
    UINT32    Family;               // Recommend disable
  } Sha1;
  union {
    struct {
      UINT8    GetContextSize : 1;
      UINT8    Init           : 1;
      UINT8    Duplicate      : 1;
      UINT8    Update         : 1;
      UINT8    Final          : 1;
      UINT8    HashAll        : 1;
    } Services;
    UINT32    Family;
  } Sha256;
  union {
    struct {
      UINT8    GetContextSize : 1;
      UINT8    Init           : 1;
      UINT8    Duplicate      : 1;
      UINT8    Update         : 1;
      UINT8    Final          : 1;
      UINT8    HashAll        : 1;
    } Services;
    UINT32    Family;
  } Sha384;
  union {
    struct {
      UINT8    GetContextSize : 1;
      UINT8    Init           : 1;
      UINT8    Duplicate      : 1;
      UINT8    Update         : 1;
      UINT8    Final          : 1;
      UINT8    HashAll        : 1;
    } Services;
    UINT32    Family;
  } Sha512;
  union {
    struct {
      UINT8    GetSubjectName              : 1;
      UINT8    GetCommonName               : 1;
      UINT8    GetOrganizationName         : 1;
      UINT8    VerifyCert                  : 1;
      UINT8    ConstructCertificate        : 1;
      UINT8    ConstructCertificateStack   : 1;
      UINT8    ConstructCertificateStackV  : 1;
      UINT8    Free                        : 1;
      UINT8    StackFree                   : 1;
      UINT8    GetTBSCert                  : 1;
      UINT8    GetVersion                  : 1;
      UINT8    GetSerialNumber             : 1;
      UINT8    GetIssuerName               : 1;
      UINT8    GetSignatureAlgorithm       : 1;
      UINT8    GetExtensionData            : 1;
      UINT8    GetExtendedKeyUsage         : 1;
      UINT8    GetValidity                 : 1;
      UINT8    FormatDateTime              : 1;
      UINT8    CompareDateTime             : 1;
      UINT8    GetKeyUsage                 : 1;
      UINT8    VerifyCertChain             : 1;
      UINT8    GetCertFromCertChain        : 1;
      UINT8    Asn1GetTag                  : 1;
      UINT8    GetExtendedBasicConstraints : 1;
    } Services;
    UINT32    Family;
  } X509;
  union {
    struct {
      UINT8    GetContextSize : 1;  // Deprecated
      UINT8    Init           : 1;  // Deprecated
      UINT8    EcbEncrypt     : 1;  // Deprecated
      UINT8    EcbDecrypt     : 1;  // Deprecated
      UINT8    CbcEncrypt     : 1;  // Deprecated
      UINT8    CbcDecrypt     : 1;  // Deprecated
    } Services;
    UINT32    Family;               // Deprecated
  } Tdes;
  union {
    struct {
      UINT8    GetContextSize : 1;
      UINT8    Init           : 1;
      UINT8    EcbEncrypt     : 1;  // Deprecated
      UINT8    EcbDecrypt     : 1;  // Deprecated
      UINT8    CbcEncrypt     : 1;
      UINT8    CbcDecrypt     : 1;
    } Services;
    UINT32    Family;
  } Aes;
  union {
    struct {
      UINT8    GetContextSize : 1;  // Deprecated
      UINT8    Init           : 1;  // Deprecated
      UINT8    Encrypt        : 1;  // Deprecated
      UINT8    Decrypt        : 1;  // Deprecated
      UINT8    Reset          : 1;  // Deprecated
    } Services;
    UINT32    Family;               // Deprecated
  } Arc4;
  union {
    struct {
      UINT8    GetContextSize : 1;
      UINT8    Init           : 1;
      UINT8    Duplicate      : 1;
      UINT8    Update         : 1;
      UINT8    Final          : 1;
      UINT8    HashAll        : 1;
    } Services;
    UINT32    Family;
  } Sm3;
  union {
    struct {
      UINT8    Sha256ExtractAndExpand : 1;
      UINT8    Sha256Extract          : 1;
      UINT8    Sha256Expand           : 1;
      UINT8    Sha384ExtractAndExpand : 1;
      UINT8    Sha384Extract          : 1;
      UINT8    Sha384Expand           : 1;
    } Services;
    UINT32    Family;
  } Hkdf;
  union {
    struct {
      UINT8    Initialize     : 1;
      UINT8    CtxFree        : 1;
      UINT8    CtxNew         : 1;
      UINT8    Free           : 1;
      UINT8    New            : 1;
      UINT8    InHandshake    : 1;
      UINT8    DoHandshake    : 1;
      UINT8    HandleAlert    : 1;
      UINT8    CloseNotify    : 1;
      UINT8    CtrlTrafficOut : 1;
      UINT8    CtrlTrafficIn  : 1;
      UINT8    Read           : 1;
      UINT8    Write          : 1;
      UINT8    Shutdown       : 1;
    } Services;
    UINT32    Family;
  } Tls;
  union {
    struct {
      UINT8    Version            : 1;
      UINT8    ConnectionEnd      : 1;
      UINT8    CipherList         : 1;
      UINT8    CompressionMethod  : 1;
      UINT8    Verify             : 1;
      UINT8    VerifyHost         : 1;
      UINT8    SessionId          : 1;
      UINT8    CaCertificate      : 1;
      UINT8    HostPublicCert     : 1;
      UINT8    HostPrivateKey     : 1;
      UINT8    CertRevocationList : 1;
      UINT8    HostPrivateKeyEx   : 1;
      UINT8    SignatureAlgoList  : 1;
      UINT8    EcCurve            : 1;
    } Services;
    UINT32    Family;
  } TlsSet;
  union {
    struct {
      UINT8    Version              : 1;
      UINT8    ConnectionEnd        : 1;
      UINT8    CurrentCipher        : 1;
      UINT8    CurrentCompressionId : 1;
      UINT8    Verify               : 1;
      UINT8    SessionId            : 1;
      UINT8    ClientRandom         : 1;
      UINT8    ServerRandom         : 1;
      UINT8    KeyMaterial          : 1;
      UINT8    CaCertificate        : 1;
      UINT8    HostPublicCert       : 1;
      UINT8    HostPrivateKey       : 1;
      UINT8    CertRevocationList   : 1;
      UINT8    ExportKey            : 1;
    } Services;
    UINT32    Family;
  } TlsGet;
  union {
    struct {
      UINT8    Sign   : 1;
      UINT8    Verify : 1;
    } Services;
    UINT32    Family;
  } RsaPss;
  union {
    struct {
      UINT8    HashAll : 1;
    } Services;
    UINT32    Family;
  } ParallelHash;
  union {
    struct {
      UINT8    Encrypt : 1;
      UINT8    Decrypt : 1;
    } Services;
    UINT32    Family;
  } AeadAesGcm;
  union {
    struct {
      UINT8    Init        : 1;
      UINT8    FromBin     : 1;
      UINT8    ToBin       : 1;
      UINT8    Free        : 1;
      UINT8    Add         : 1;
      UINT8    Sub         : 1;
      UINT8    Mod         : 1;
      UINT8    ExpMod      : 1;
      UINT8    InverseMod  : 1;
      UINT8    Div         : 1;
      UINT8    MulMod      : 1;
      UINT8    Cmp         : 1;
      UINT8    Bits        : 1;
      UINT8    Bytes       : 1;
      UINT8    IsWord      : 1;
      UINT8    IsOdd       : 1;
      UINT8    Copy        : 1;
      UINT8    ValueOne    : 1;
      UINT8    RShift      : 1;
      UINT8    ConstTime   : 1;
      UINT8    SqrMod      : 1;
      UINT8    NewContext  : 1;
      UINT8    ContextFree : 1;
      UINT8    SetUint     : 1;
      UINT8    AddMod      : 1;
    } Services;
    UINT32    Family;
  } Bn;
  union {
    struct {
      UINT8    GroupInit                     : 1;
      UINT8    GroupGetCurve                 : 1;
      UINT8    GroupGetOrder                 : 1;
      UINT8    GroupFree                     : 1;
      UINT8    PointInit                     : 1;
      UINT8    PointDeInit                   : 1;
      UINT8    PointGetAffineCoordinates     : 1;
      UINT8    PointSetAffineCoordinates     : 1;
      UINT8    PointAdd                      : 1;
      UINT8    PointMul                      : 1;
      UINT8    PointInvert                   : 1;
      UINT8    PointIsOnCurve                : 1;
      UINT8    PointIsAtInfinity             : 1;
      UINT8    PointEqual                    : 1;
      UINT8    PointSetCompressedCoordinates : 1;
      UINT8    NewByNid                      : 1;
      UINT8    Free                          : 1;
      UINT8    GenerateKey                   : 1;
      UINT8    GetPubKey                     : 1;
      UINT8    DhComputeKey                  : 1;
      UINT8    GetPublicKeyFromX509          : 1;
      UINT8    GetPrivateKeyFromPem          : 1;
      UINT8    DsaSign                       : 1;
      UINT8    DsaVerify                     : 1;
    } Services;
    UINT32    Family;
  } Ec;
} PCD_CRYPTO_SERVICE_FAMILY_ENABLE;

#endif

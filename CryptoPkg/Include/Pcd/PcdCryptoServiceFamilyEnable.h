/** @file
  Defines the PCD_CRYPTO_SERVICE_FAMILY_ENABLE structure associated with
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.

  Copyright (c) 2019 - 2022, Intel Corporation. All rights reserved.<BR>
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
      UINT8    New       : 1;
      UINT8    Free      : 1;
      UINT8    SetKey    : 1;
      UINT8    Duplicate : 1;
      UINT8    Update    : 1;
      UINT8    Final     : 1;
    } Services;
    UINT32    Family;
  } HmacMd5;
  union {
    struct {
      UINT8    New       : 1;
      UINT8    Free      : 1;
      UINT8    SetKey    : 1;
      UINT8    Duplicate : 1;
      UINT8    Update    : 1;
      UINT8    Final     : 1;
    } Services;
    UINT32    Family;
  } HmacSha1;
  union {
    struct {
      UINT8    New       : 1;
      UINT8    Free      : 1;
      UINT8    SetKey    : 1;
      UINT8    Duplicate : 1;
      UINT8    Update    : 1;
      UINT8    Final     : 1;
    } Services;
    UINT32    Family;
  } HmacSha256;
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
  } Md4;
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
  } Md5;
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
    } Services;
    UINT32    Family;
  } Rsa;
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
      UINT8    GetSubjectName             : 1;
      UINT8    GetCommonName              : 1;
      UINT8    GetOrganizationName        : 1;
      UINT8    VerifyCert                 : 1;
      UINT8    ConstructCertificate       : 1;
      UINT8    ConstructCertificateStack  : 1;
      UINT8    ConstructCertificateStackV : 1;
      UINT8    Free                       : 1;
      UINT8    StackFree                  : 1;
      UINT8    GetTBSCert                 : 1;
    } Services;
    UINT32    Family;
  } X509;
  union {
    struct {
      UINT8    GetContextSize : 1;
      UINT8    Init           : 1;
      UINT8    EcbEncrypt     : 1;
      UINT8    EcbDecrypt     : 1;
      UINT8    CbcEncrypt     : 1;
      UINT8    CbcDecrypt     : 1;
    } Services;
    UINT32    Family;
  } Tdes;
  union {
    struct {
      UINT8    GetContextSize : 1;
      UINT8    Init           : 1;
      UINT8    EcbEncrypt     : 1;
      UINT8    EcbDecrypt     : 1;
      UINT8    CbcEncrypt     : 1;
      UINT8    CbcDecrypt     : 1;
    } Services;
    UINT32    Family;
  } Aes;
  union {
    struct {
      UINT8    GetContextSize : 1;
      UINT8    Init           : 1;
      UINT8    Encrypt        : 1;
      UINT8    Decrypt        : 1;
      UINT8    Reset          : 1;
    } Services;
    UINT32    Family;
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
      UINT8    Sha256ExtractAndExpand;
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
} PCD_CRYPTO_SERVICE_FAMILY_ENABLE;

#endif

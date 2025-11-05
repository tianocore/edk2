/** @file
  PKCS#7 SignedData Sign Wrapper and PKCS#7 SignedData Verification Wrapper
  Implementation over mbedtls, Internal headers.

  RFC 2315 - PKCS #7: Cryptographic Message Syntax Version 1.5

Copyright (c) 2023-2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CRYPT_PKCS7_INTERNAL_H_
#define CRYPT_PKCS7_INTERNAL_H_

#include "InternalCryptLib.h"

#include "mbedtls/oid.h"
#include "mbedtls/asn1.h"
#include "mbedtls/asn1write.h"
#include "mbedtls/x509.h"
#include "mbedtls/x509_crt.h"

///
/// PKCS7 OID
///
#define MBEDTLS_OID_PKCS7                            MBEDTLS_OID_PKCS "\x07"
#define MBEDTLS_OID_PKCS7_DATA                       MBEDTLS_OID_PKCS7 "\x01"
#define MBEDTLS_OID_PKCS7_SIGNED_DATA                MBEDTLS_OID_PKCS7 "\x02"
#define MBEDTLS_OID_PKCS7_ENVELOPED_DATA             MBEDTLS_OID_PKCS7 "\x03"
#define MBEDTLS_OID_PKCS7_SIGNED_AND_ENVELOPED_DATA  MBEDTLS_OID_PKCS7 "\x04"
#define MBEDTLS_OID_PKCS7_DIGESTED_DATA              MBEDTLS_OID_PKCS7 "\x05"
#define MBEDTLS_OID_PKCS7_ENCRYPTED_DATA             MBEDTLS_OID_PKCS7 "\x06"

///
/// PKCS7 SignerInfo type
/// https://tools.ietf.org/html/rfc2315#section-9.2
///
typedef struct MbedtlsPkcs7SignerInfo {
  INT32                            Version;
  mbedtls_x509_buf                 Serial;
  mbedtls_x509_name                Issuer;
  mbedtls_x509_buf                 IssuerRaw;
  mbedtls_x509_buf                 AlgIdentifier;
  mbedtls_x509_buf                 SigAlgIdentifier;
  mbedtls_x509_buf                 AuthAttr;
  mbedtls_x509_buf                 Sig;
  struct MbedtlsPkcs7SignerInfo    *Next;
} MbedtlsPkcs7SignerInfo;

///
/// PKCS7 signed data attached data format
///
typedef struct MbedtlsPkcs7Data {
  mbedtls_asn1_buf    Oid;
  mbedtls_asn1_buf    Data;
} MbedtlsPkcs7Data;

///
/// Signed Data
/// https://tools.ietf.org/html/rfc2315#section-9.1
///
typedef struct MbedtlsPkcs7SignedData {
  INT32                            Version;
  mbedtls_asn1_buf                 DigestAlgorithms;
  struct MbedtlsPkcs7Data          ContentInfo;
  mbedtls_x509_crt                 Certificates;
  mbedtls_x509_crl                 Crls;
  struct MbedtlsPkcs7SignerInfo    SignerInfos;
} MbedtlsPkcs7SignedData;

///
/// PKCS7 struct, only support SignedData
///
typedef struct MbedtlsPkcs7 {
  mbedtls_asn1_buf                 ContentTypeOid;
  struct MbedtlsPkcs7SignedData    SignedData;
} MbedtlsPkcs7;

#define EDKII_ASN1_CHK_ADD(g, f)                        \
    do                                                  \
    {                                                   \
        if( ( Ret = (f) ) < 0 )                         \
            return( Ret );                              \
        else                                            \
            (g) += Ret;                                 \
    } while( 0 )

#endif

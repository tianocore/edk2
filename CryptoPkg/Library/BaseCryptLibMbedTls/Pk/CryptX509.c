/** @file
  X.509 Certificate Handler Wrapper Implementation over MbedTLS.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/rsa.h>
#include <mbedtls/ecp.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ecdsa.h>

///
/// OID
///
STATIC CONST UINT8  OID_commonName[] = {
  0x55, 0x04, 0x03
};
STATIC CONST UINT8  OID_organizationName[] = {
  0x55, 0x04, 0x0A
};
STATIC CONST UINT8  OID_extKeyUsage[] = {
  0x55, 0x1D, 0x25
};
STATIC CONST UINT8  OID_BasicConstraints[] = {
  0x55, 0x1D, 0x13
};

/* Profile for backward compatibility. Allows RSA 1024, unlike the default
   profile. */
STATIC mbedtls_x509_crt_profile  gCompatProfile =
{
  /* Hashes from SHA-256 and above. Note that this selection
   * should be aligned with ssl_preset_default_hashes in ssl_tls.c. */
  MBEDTLS_X509_ID_FLAG (MBEDTLS_MD_SHA256) |
  MBEDTLS_X509_ID_FLAG (MBEDTLS_MD_SHA384) |
  MBEDTLS_X509_ID_FLAG (MBEDTLS_MD_SHA512),
  0xFFFFFFF,       /* Any PK alg    */

  /* Curves at or above 128-bit security level. Note that this selection
   * should be aligned with ssl_preset_default_curves in ssl_tls.c. */
  MBEDTLS_X509_ID_FLAG (MBEDTLS_ECP_DP_SECP256R1) |
  MBEDTLS_X509_ID_FLAG (MBEDTLS_ECP_DP_SECP384R1) |
  MBEDTLS_X509_ID_FLAG (MBEDTLS_ECP_DP_SECP521R1) |
  MBEDTLS_X509_ID_FLAG (MBEDTLS_ECP_DP_BP256R1) |
  MBEDTLS_X509_ID_FLAG (MBEDTLS_ECP_DP_BP384R1) |
  MBEDTLS_X509_ID_FLAG (MBEDTLS_ECP_DP_BP512R1) |
  0,
  1024,
};

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
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT UINT8       **SingleX509Cert
  )
{
  mbedtls_x509_crt  *MbedTlsCert;
  INT32             Ret;

  if ((Cert == NULL) || (SingleX509Cert == NULL) || (CertSize == 0)) {
    return FALSE;
  }

  MbedTlsCert = AllocateZeroPool (sizeof (mbedtls_x509_crt));
  if (MbedTlsCert == NULL) {
    return FALSE;
  }

  mbedtls_x509_crt_init (MbedTlsCert);

  *SingleX509Cert = (UINT8 *)(VOID *)MbedTlsCert;
  Ret             = mbedtls_x509_crt_parse_der (MbedTlsCert, Cert, CertSize);
  if (Ret == 0) {
    return TRUE;
  } else {
    mbedtls_x509_crt_free (MbedTlsCert);
    FreePool (MbedTlsCert);
    return FALSE;
  }
}

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
BOOLEAN
EFIAPI
X509ConstructCertificateStackV (
  IN OUT UINT8  **X509Stack,
  IN VA_LIST    Args
  )
{
  UINT8             *Cert;
  UINTN             CertSize;
  INT32             Index;
  INT32             Ret;
  mbedtls_x509_crt  *Crt;

  if (X509Stack == NULL) {
    return FALSE;
  }

  Ret = 0;
  Crt = NULL;
  if (*X509Stack == NULL) {
    Crt = AllocateZeroPool (sizeof (mbedtls_x509_crt));
    if (Crt == NULL) {
      return FALSE;
    }

    mbedtls_x509_crt_init (Crt);
    *X509Stack = (UINT8 *)Crt;
  }

  for (Index = 0; ; Index++) {
    //
    // If Cert is NULL, then it is the end of the list.
    //
    Cert = VA_ARG (Args, UINT8 *);
    if (Cert == NULL) {
      break;
    }

    CertSize = VA_ARG (Args, UINTN);
    if (CertSize == 0) {
      break;
    }

    Ret = mbedtls_x509_crt_parse_der ((mbedtls_x509_crt *)*X509Stack, Cert, CertSize);

    if (Ret != 0) {
      break;
    }
  }

  if (Ret == 0) {
    return TRUE;
  } else {
    if (Crt != NULL) {
      mbedtls_x509_crt_free (Crt);
      FreePool (Crt);
      *X509Stack = NULL;
    }

    return FALSE;
  }
}

/**
  Construct a X509 stack object from a list of DER-encoded certificate data.

  If X509Stack is NULL, then return FALSE.

  @param[in, out]  X509Stack  On input, pointer to an existing or NULL X509 stack object.
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
  IN OUT UINT8  **X509Stack,
  ...
  )
{
  VA_LIST  Args;
  BOOLEAN  Result;

  VA_START (Args, X509Stack);
  Result = X509ConstructCertificateStackV (X509Stack, Args);
  VA_END (Args);
  return Result;
}

/**
  Release the specified X509 object.

  If X509Cert is NULL, then return FALSE.

  @param[in]  X509Cert  Pointer to the X509 object to be released.

**/
VOID
EFIAPI
X509Free (
  IN VOID  *X509Cert
  )
{
  if (X509Cert != NULL) {
    mbedtls_x509_crt_free (X509Cert);
    FreePool (X509Cert);
  }
}

/**
  Release the specified X509 stack object.

  If X509Stack is NULL, then return FALSE.

  @param[in]  X509Stack  Pointer to the X509 stack object to be released.

**/
VOID
EFIAPI
X509StackFree (
  IN VOID  *X509Stack
  )
{
  if (X509Stack == NULL) {
    return;
  }

  mbedtls_x509_crt_free (X509Stack);
}

/**
  Retrieve the tag and length of the tag.

  @param Ptr      The position in the ASN.1 data
  @param End      End of data
  @param Length   The variable that will receive the length
  @param Tag      The expected tag

  @retval      TRUE   Get tag successful
  @retval      FALSe  Failed to get tag or tag not match
**/
BOOLEAN
EFIAPI
Asn1GetTag (
  IN OUT UINT8    **Ptr,
  IN CONST UINT8  *End,
  OUT UINTN       *Length,
  IN UINT32       Tag
  )
{
  if (mbedtls_asn1_get_tag (Ptr, End, Length, (INT32)Tag) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

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
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT UINT8       *CertSubject,
  IN OUT UINTN    *SubjectSize
  )
{
  mbedtls_x509_crt  Crt;
  INT32             Ret;

  if (Cert == NULL) {
    return FALSE;
  }

  mbedtls_x509_crt_init (&Crt);

  Ret = mbedtls_x509_crt_parse_der (&Crt, Cert, CertSize);

  if (Ret == 0) {
    if (CertSubject != NULL) {
      CopyMem (CertSubject, Crt.subject_raw.p, Crt.subject_raw.len);
    }

    *SubjectSize = Crt.subject_raw.len;
  }

  mbedtls_x509_crt_free (&Crt);

  return Ret == 0;
}

/**
  Retrieve a string from one X.509 certificate base on the Request_NID.

  @param[in]      Name             mbedtls_x509_name
  @param[in]      Oid              Oid
  @param[in]      OidSize          Size of Oid
  @param[in,out]     CommonName    Buffer to contain the retrieved certificate common
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
  @retval RETURN_NOT_FOUND         If no NID Name entry exists.
  @retval RETURN_BUFFER_TOO_SMALL  If the CommonName is NULL. The required buffer size
                                   (including the final null) is returned in the
                                   CommonNameSize parameter.
  @retval RETURN_UNSUPPORTED       The operation is not supported.

**/
RETURN_STATUS
EFIAPI
InternalX509GetNIDName (
  IN mbedtls_x509_name  *Name,
  IN CHAR8              *Oid,
  IN UINTN              OidSize,
  IN OUT CHAR8          *CommonName OPTIONAL,
  IN OUT UINTN          *CommonNameSize
  )
{
  CONST mbedtls_asn1_named_data  *data;

  data = mbedtls_asn1_find_named_data (Name, Oid, OidSize);
  if (data != NULL) {
    if (*CommonNameSize <= data->val.len) {
      *CommonNameSize = data->val.len + 1;
      return RETURN_BUFFER_TOO_SMALL;
    }

    if (CommonName != NULL) {
      CopyMem (CommonName, data->val.p, data->val.len);
      CommonName[data->val.len] = '\0';
    }

    *CommonNameSize = data->val.len + 1;
    return RETURN_SUCCESS;
  } else {
    return RETURN_NOT_FOUND;
  }
}

/**
  Get X509 SubjectNIDName by OID.

  @param[in]      Cert             certificate
  @param[in]      CertSize         certificate size.
  @param[in]      Oid              Oid
  @param[in]      OidSize          Size of Oid
  @param[in,out]  CommonName       Buffer to contain the retrieved certificate common
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
  @retval RETURN_NOT_FOUND         If no NID Name entry exists.
  @retval RETURN_BUFFER_TOO_SMALL  If the CommonName is NULL. The required buffer size
                                   (including the final null) is returned in the
                                   CommonNameSize parameter.
  @retval RETURN_UNSUPPORTED       The operation is not supported.

**/
RETURN_STATUS
EFIAPI
InternalX509GetSubjectNIDName (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  IN CHAR8        *Oid,
  IN UINTN        OidSize,
  IN OUT CHAR8    *CommonName OPTIONAL,
  IN OUT UINTN    *CommonNameSize
  )
{
  mbedtls_x509_crt   Crt;
  INT32              Ret;
  mbedtls_x509_name  *Name;
  RETURN_STATUS      ReturnStatus;

  if (Cert == NULL) {
    return FALSE;
  }

  ReturnStatus = RETURN_INVALID_PARAMETER;

  mbedtls_x509_crt_init (&Crt);

  Ret = mbedtls_x509_crt_parse_der (&Crt, Cert, CertSize);

  if (Ret == 0) {
    Name         = &(Crt.subject);
    ReturnStatus = InternalX509GetNIDName (Name, Oid, OidSize, CommonName, CommonNameSize);
  }

  mbedtls_x509_crt_free (&Crt);

  return ReturnStatus;
}

/**
  Get X509 IssuerNIDName by OID.

  @param[in]      Cert             certificate
  @param[in]      CertSize         certificate size.
  @param[in]      Oid              Oid
  @param[in]      OidSize          Size of Oid
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
  @retval RETURN_NOT_FOUND         If no NID Name entry exists.
  @retval RETURN_BUFFER_TOO_SMALL  If the CommonName is NULL. The required buffer size
                                   (including the final null) is returned in the
                                   CommonNameSize parameter.
  @retval RETURN_UNSUPPORTED       The operation is not supported.

**/
RETURN_STATUS
EFIAPI
InternalX509GetIssuerNIDName (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  IN CHAR8        *Oid,
  IN UINTN        OidSize,
  OUT CHAR8       *CommonName OPTIONAL,
  IN OUT UINTN    *CommonNameSize
  )
{
  mbedtls_x509_crt   Crt;
  INT32              Ret;
  mbedtls_x509_name  *Name;
  RETURN_STATUS      ReturnStatus;

  if (Cert == NULL) {
    return FALSE;
  }

  ReturnStatus = RETURN_INVALID_PARAMETER;

  mbedtls_x509_crt_init (&Crt);

  Ret = mbedtls_x509_crt_parse_der (&Crt, Cert, CertSize);

  if (Ret == 0) {
    Name         = &(Crt.issuer);
    ReturnStatus = InternalX509GetNIDName (Name, Oid, OidSize, CommonName, CommonNameSize);
  }

  mbedtls_x509_crt_free (&Crt);

  return ReturnStatus;
}

/**
  Retrieve the common name (CN) string from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     CommonName       Buffer to contain the retrieved certificate common
                                   name string. At most CommonNameSize bytes will be
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
RETURN_STATUS
EFIAPI
X509GetCommonName (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT CHAR8       *CommonName OPTIONAL,
  IN OUT UINTN    *CommonNameSize
  )
{
  return InternalX509GetSubjectNIDName (Cert, CertSize, (CHAR8 *)OID_commonName, sizeof (OID_commonName), CommonName, CommonNameSize);
}

/**
  Retrieve the organization name (O) string from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     NameBuffer       Buffer to contain the retrieved certificate organization
                                   name string. At most NameBufferSize bytes will be
                                   written and the string will be null terminated. May be
                                   NULL in order to determine the size buffer needed.
  @param[in,out]  NameBufferSize   The size in bytes of the Name buffer on input,
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
RETURN_STATUS
EFIAPI
X509GetOrganizationName (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT CHAR8       *NameBuffer OPTIONAL,
  IN OUT UINTN    *NameBufferSize
  )
{
  return InternalX509GetSubjectNIDName (Cert, CertSize, (CHAR8 *)OID_organizationName, sizeof (OID_organizationName), NameBuffer, NameBufferSize);
}

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
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT VOID        **RsaContext
  )
{
  mbedtls_x509_crt     Crt;
  mbedtls_rsa_context  *Rsa;
  INT32                Ret;

  mbedtls_x509_crt_init (&Crt);

  if (mbedtls_x509_crt_parse_der (&Crt, Cert, CertSize) != 0) {
    return FALSE;
  }

  if (mbedtls_pk_get_type (&Crt.pk) != MBEDTLS_PK_RSA) {
    mbedtls_x509_crt_free (&Crt);
    return FALSE;
  }

  Rsa = RsaNew ();
  if (Rsa == NULL) {
    mbedtls_x509_crt_free (&Crt);
    return FALSE;
  }

  Ret = mbedtls_rsa_copy (Rsa, mbedtls_pk_rsa (Crt.pk));
  if (Ret != 0) {
    RsaFree (Rsa);
    mbedtls_x509_crt_free (&Crt);
    return FALSE;
  }

  mbedtls_x509_crt_free (&Crt);

  *RsaContext = Rsa;
  return TRUE;
}

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
BOOLEAN
EFIAPI
EcGetPublicKeyFromX509 (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT VOID        **EcContext
  )
{
  ASSERT (FALSE);
  return FALSE;
}

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
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  IN CONST UINT8  *CACert,
  IN UINTN        CACertSize
  )
{
  INT32                     Ret;
  mbedtls_x509_crt          Ca;
  mbedtls_x509_crt          End;
  UINT32                    VFlag;
  mbedtls_x509_crt_profile  Profile;

  if ((Cert == NULL) || (CACert == NULL)) {
    return FALSE;
  }

  VFlag = 0;
  CopyMem (&Profile, &gCompatProfile, sizeof (mbedtls_x509_crt_profile));

  mbedtls_x509_crt_init (&Ca);
  mbedtls_x509_crt_init (&End);

  Ret = mbedtls_x509_crt_parse_der (&Ca, CACert, CACertSize);

  if (Ret == 0) {
    Ret = mbedtls_x509_crt_parse_der (&End, Cert, CertSize);
  }

  if (Ret == 0) {
    Ret = mbedtls_x509_crt_verify_with_profile (&End, &Ca, NULL, &Profile, NULL, &VFlag, NULL, NULL);
  }

  mbedtls_x509_crt_free (&Ca);
  mbedtls_x509_crt_free (&End);

  return Ret == 0;
}

/**
  Verify one X509 certificate was issued by the trusted CA.

  @param[in]      RootCert          Trusted Root Certificate buffer
  @param[in]      RootCertLength    Trusted Root Certificate buffer length
  @param[in]      CertChain         One or more ASN.1 DER-encoded X.509 certificates
                                    where the first certificate is signed by the Root
                                    Certificate or is the Root Certificate itself. and
                                    subsequent certificate is signed by the preceding
                                    certificate.
  @param[in]      CertChainLength   Total length of the certificate chain, in bytes.

  @retval  TRUE   All certificates was issued by the first certificate in X509Certchain.
  @retval  FALSE  Invalid certificate or the certificate was not issued by the given
                  trusted CA.
**/
BOOLEAN
EFIAPI
X509VerifyCertChain (
  IN CONST UINT8  *RootCert,
  IN UINTN        RootCertLength,
  IN CONST UINT8  *CertChain,
  IN UINTN        CertChainLength
  )
{
  UINTN        Asn1Len;
  UINTN        PrecedingCertLen;
  CONST UINT8  *PrecedingCert;
  UINTN        CurrentCertLen;
  CONST UINT8  *CurrentCert;
  CONST UINT8  *TmpPtr;
  UINT32       Ret;
  BOOLEAN      VerifyFlag;

  VerifyFlag       = FALSE;
  PrecedingCert    = RootCert;
  PrecedingCertLen = RootCertLength;

  CurrentCert = CertChain;

  //
  // Get Current certificate from Certificates buffer and Verify with preciding cert
  //
  do {
    TmpPtr = CurrentCert;
    Ret    = mbedtls_asn1_get_tag ((UINT8 **)&TmpPtr, CertChain + CertChainLength, &Asn1Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    if (Ret != 0) {
      break;
    }

    CurrentCertLen = Asn1Len + (TmpPtr - CurrentCert);

    if (!X509VerifyCert (CurrentCert, CurrentCertLen, PrecedingCert, PrecedingCertLen)) {
      VerifyFlag = FALSE;
      break;
    } else {
      VerifyFlag = TRUE;
    }

    //
    // Save preceding certificate
    //
    PrecedingCert    = CurrentCert;
    PrecedingCertLen = CurrentCertLen;

    //
    // Move current certificate to next;
    //
    CurrentCert = CurrentCert + CurrentCertLen;
  } while (1);

  return VerifyFlag;
}

/**
  Get one X509 certificate from CertChain.

  @param[in]      CertChain         One or more ASN.1 DER-encoded X.509 certificates
                                    where the first certificate is signed by the Root
                                    Certificate or is the Root Certificate itself. and
                                    subsequent certificate is signed by the preceding
                                    certificate.
  @param[in]      CertChainLength   Total length of the certificate chain, in bytes.

  @param[in]      CertIndex         Index of certificate.

  @param[out]     Cert              The certificate at the index of CertChain.
  @param[out]     CertLength        The length certificate at the index of CertChain.

  @retval  TRUE   Success.
  @retval  FALSE  Failed to get certificate from certificate chain.
**/
BOOLEAN
EFIAPI
X509GetCertFromCertChain (
  IN CONST UINT8   *CertChain,
  IN UINTN         CertChainLength,
  IN CONST INT32   CertIndex,
  OUT CONST UINT8  **Cert,
  OUT UINTN        *CertLength
  )
{
  UINTN        Asn1Len;
  INT32        CurrentIndex;
  UINTN        CurrentCertLen;
  CONST UINT8  *CurrentCert;
  CONST UINT8  *TmpPtr;
  INT32        Ret;

  //
  // Check input parameters.
  //
  if ((CertChain == NULL) || (Cert == NULL) ||
      (CertIndex < -1) || (CertLength == NULL))
  {
    return FALSE;
  }

  CurrentCert  = CertChain;
  CurrentIndex = -1;

  //
  // Traverse the certificate chain
  //
  while (TRUE) {
    //
    // Get asn1 tag len
    //
    TmpPtr = CurrentCert;
    Ret    = mbedtls_asn1_get_tag ((UINT8 **)&TmpPtr, CertChain + CertChainLength, &Asn1Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    if (Ret != 0) {
      break;
    }

    CurrentCertLen = Asn1Len + (TmpPtr - CurrentCert);
    CurrentIndex++;

    if (CurrentIndex == CertIndex) {
      *Cert       = CurrentCert;
      *CertLength = CurrentCertLen;
      return TRUE;
    }

    //
    // Move to next
    //
    CurrentCert = CurrentCert + CurrentCertLen;
  }

  //
  // If CertIndex is -1, Return the last certificate
  //
  if ((CertIndex == -1) && (CurrentIndex >= 0)) {
    *Cert       = CurrentCert - CurrentCertLen;
    *CertLength = CurrentCertLen;
    return TRUE;
  }

  return FALSE;
}

/**
  Retrieve the TBSCertificate from one given X.509 certificate.

  @param[in]      Cert         Pointer to the given DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[out]     TBSCert      DER-Encoded To-Be-Signed certificate.
  @param[out]     TBSCertSize  Size of the TBS certificate in bytes.

  If Cert is NULL, then return FALSE.
  If TBSCert is NULL, then return FALSE.
  If TBSCertSize is NULL, then return FALSE.

  @retval  TRUE   The TBSCertificate was retrieved successfully.
  @retval  FALSE  Invalid X.509 certificate.

**/
BOOLEAN
EFIAPI
X509GetTBSCert (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT UINT8       **TBSCert,
  OUT UINTN       *TBSCertSize
  )
{
  UINTN        Length;
  UINTN        Ret;
  UINT8        *Ptr;
  CONST UINT8  *Temp;
  CONST UINT8  *End;

  //
  // Check input parameters.
  //
  if ((Cert == NULL) || (TBSCert == NULL) ||
      (TBSCertSize == NULL) || (CertSize > INT_MAX))
  {
    return FALSE;
  }

  //
  // An X.509 Certificate is: (defined in RFC3280)
  //   Certificate  ::=  SEQUENCE  {
  //     tbsCertificate       TBSCertificate,
  //     signatureAlgorithm   AlgorithmIdentifier,
  //     signature            BIT STRING }
  //
  // and
  //
  //  TBSCertificate  ::=  SEQUENCE  {
  //    version         [0]  Version DEFAULT v1,
  //    ...
  //    }
  //
  // So we can just ASN1-parse the x.509 DER-encoded data. If we strip
  // the first SEQUENCE, the second SEQUENCE is the TBSCertificate.
  //

  Length = 0;

  Ptr = (UINT8 *)Cert;
  End = Cert + CertSize;

  Ret = mbedtls_asn1_get_tag (&Ptr, End, &Length, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
  if (Ret != 0) {
    return FALSE;
  }

  Temp = Ptr;
  End  = Ptr + Length;
  Ret  = mbedtls_asn1_get_tag (&Ptr, End, &Length, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
  if (Ret != 0) {
    return FALSE;
  }

  *TBSCert     = (UINT8 *)Temp;
  *TBSCertSize = Length + (Ptr - Temp);

  return TRUE;
}

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
BOOLEAN
EFIAPI
X509GetVersion (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT UINTN       *Version
  )
{
  mbedtls_x509_crt  Crt;
  INT32             Ret;
  BOOLEAN           ReturnStatus;

  if (Cert == NULL) {
    return FALSE;
  }

  ReturnStatus = FALSE;

  mbedtls_x509_crt_init (&Crt);

  Ret = mbedtls_x509_crt_parse_der (&Crt, Cert, CertSize);

  if (Ret == 0) {
    *Version     = Crt.version - 1;
    ReturnStatus = TRUE;
  }

  mbedtls_x509_crt_free (&Crt);

  return ReturnStatus;
}

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
BOOLEAN
EFIAPI
X509GetSerialNumber (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT UINT8       *SerialNumber OPTIONAL,
  IN OUT UINTN    *SerialNumberSize
  )
{
  mbedtls_x509_crt  Crt;
  INT32             Ret;
  BOOLEAN           ReturnStatus;

  if (Cert == NULL) {
    return FALSE;
  }

  ReturnStatus = FALSE;

  mbedtls_x509_crt_init (&Crt);

  Ret = mbedtls_x509_crt_parse_der (&Crt, Cert, CertSize);

  if (Ret == 0) {
    if (*SerialNumberSize <= Crt.serial.len) {
      *SerialNumberSize = Crt.serial.len + 1;
      ReturnStatus      = FALSE;
      goto Cleanup;
    }

    if (SerialNumber != NULL) {
      CopyMem (SerialNumber, Crt.serial.p, Crt.serial.len);
      SerialNumber[Crt.serial.len] = '\0';
    }

    *SerialNumberSize = Crt.serial.len + 1;
    ReturnStatus      = TRUE;
  }

Cleanup:
  mbedtls_x509_crt_free (&Crt);

  return ReturnStatus;
}

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
BOOLEAN
EFIAPI
X509GetIssuerName (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT UINT8       *CertIssuer,
  IN OUT UINTN    *CertIssuerSize
  )
{
  mbedtls_x509_crt  Crt;
  INT32             Ret;
  BOOLEAN           Status;

  if (Cert == NULL) {
    return FALSE;
  }

  mbedtls_x509_crt_init (&Crt);

  Ret = mbedtls_x509_crt_parse_der (&Crt, Cert, CertSize);

  if (Ret == 0) {
    if (*CertIssuerSize < Crt.issuer_raw.len) {
      *CertIssuerSize = Crt.issuer_raw.len;
      Status          = FALSE;
      goto Cleanup;
    }

    if (CertIssuer != NULL) {
      CopyMem (CertIssuer, Crt.issuer_raw.p, Crt.issuer_raw.len);
    }

    *CertIssuerSize = Crt.issuer_raw.len;
    Status          = TRUE;
  }

Cleanup:
  mbedtls_x509_crt_free (&Crt);

  return Status;
}

/**
  Retrieve the issuer common name (CN) string from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     CommonName       Buffer to contain the retrieved certificate issuer common
                                   name string. At most CommonNameSize bytes will be
                                   written and the string will be null terminated. May be
                                   NULL in order to determine the size buffer needed.
  @param[in,out]  CommonNameSize   The size in bytes of the CommonName buffer on input,
                                   and the size of buffer returned CommonName on output.
                                   If CommonName is NULL then the amount of space needed
                                   in buffer (including the final null) is returned.

  @retval RETURN_SUCCESS           The certificate Issuer CommonName retrieved successfully.
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
RETURN_STATUS
EFIAPI
X509GetIssuerCommonName (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT CHAR8       *CommonName OPTIONAL,
  IN OUT UINTN    *CommonNameSize
  )
{
  return InternalX509GetIssuerNIDName (Cert, CertSize, (CHAR8 *)OID_commonName, sizeof (OID_commonName), CommonName, CommonNameSize);
}

/**
  Retrieve the issuer organization name (O) string from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     NameBuffer       Buffer to contain the retrieved certificate issuer organization
                                   name string. At most NameBufferSize bytes will be
                                   written and the string will be null terminated. May be
                                   NULL in order to determine the size buffer needed.
  @param[in,out]  NameBufferSize   The size in bytes of the Name buffer on input,
                                   and the size of buffer returned Name on output.
                                   If NameBuffer is NULL then the amount of space needed
                                   in buffer (including the final null) is returned.

  @retval RETURN_SUCCESS           The certificate issuer Organization Name retrieved successfully.
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
RETURN_STATUS
EFIAPI
X509GetIssuerOrganizationName (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT CHAR8       *NameBuffer OPTIONAL,
  IN OUT UINTN    *NameBufferSize
  )
{
  return InternalX509GetIssuerNIDName (Cert, CertSize, (CHAR8 *)OID_organizationName, sizeof (OID_organizationName), NameBuffer, NameBufferSize);
}

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
BOOLEAN
EFIAPI
X509GetSignatureAlgorithm (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT UINT8       *Oid OPTIONAL,
  IN OUT UINTN    *OidSize
  )
{
  mbedtls_x509_crt  Crt;
  INT32             Ret;
  BOOLEAN           ReturnStatus;

  if ((Cert == NULL) || (CertSize == 0) || (OidSize == NULL)) {
    return FALSE;
  }

  ReturnStatus = FALSE;

  mbedtls_x509_crt_init (&Crt);

  Ret = mbedtls_x509_crt_parse_der (&Crt, Cert, CertSize);

  if (Ret == 0) {
    if (*OidSize < Crt.sig_oid.len) {
      *OidSize     = Crt.serial.len;
      ReturnStatus = FALSE;
      goto Cleanup;
    }

    if (Oid != NULL) {
      CopyMem (Oid, Crt.sig_oid.p, Crt.sig_oid.len);
    }

    *OidSize     = Crt.sig_oid.len;
    ReturnStatus = TRUE;
  }

Cleanup:
  mbedtls_x509_crt_free (&Crt);

  return ReturnStatus;
}

/**
 Find first Extension data match with given OID

  @param[in]      Start             Pointer to the DER-encoded Extensions Data
  @param[in]      End               Extensions Data size in bytes
  @param[in ]     Oid               OID for match
  @param[in ]     OidSize           OID size in bytes
  @param[out]     FindExtensionData output matched extension data.
  @param[out]     FindExtensionDataLen matched extension data size.

 **/
STATIC
RETURN_STATUS
InternalX509FindExtensionData (
  UINT8        *Start,
  UINT8        *End,
  CONST UINT8  *Oid,
  UINTN        OidSize,
  UINT8        **FindExtensionData,
  UINTN        *FindExtensionDataLen
  )
{
  UINT8          *Ptr;
  UINT8          *ExtensionPtr;
  size_t         ObjLen;
  INT32          Ret;
  RETURN_STATUS  ReturnStatus;
  size_t         FindExtensionLen;
  size_t         HeaderLen;

  ReturnStatus = RETURN_INVALID_PARAMETER;
  Ptr          = Start;

  Ret = 0;

  while (TRUE) {
    /*
     * Extension  ::=  SEQUENCE  {
     *      extnID      OBJECT IDENTIFIER,
     *      critical    BOOLEAN DEFAULT FALSE,
     *      extnValue   OCTET STRING  }
     */
    ExtensionPtr = Ptr;
    Ret          = mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    if (Ret == 0) {
      HeaderLen        = (size_t)(Ptr - ExtensionPtr);
      FindExtensionLen = ObjLen;
      // Get Object Identifier
      Ret = mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_OID);
    } else {
      break;
    }

    if ((Ret == 0) && (CompareMem (Ptr, Oid, OidSize) == 0)) {
      Ptr += ObjLen;

      Ret = mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_BOOLEAN);
      if (Ret == 0) {
        Ptr += ObjLen;
      }

      Ret = mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_OCTET_STRING);
    } else {
      Ret = 1;
    }

    if (Ret == 0) {
      *FindExtensionData    = Ptr;
      *FindExtensionDataLen = ObjLen;
      ReturnStatus          = RETURN_SUCCESS;
      break;
    }

    // move to next
    Ptr = ExtensionPtr + HeaderLen + FindExtensionLen;
    Ret = 0;
  }

  return ReturnStatus;
}

/**
  Retrieve Extension data from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[in]      Oid              Object identifier buffer
  @param[in]      OidSize          Object identifier buffer size
  @param[out]     ExtensionData    Extension bytes.
  @param[in, out] ExtensionDataSize Extension bytes size.

  @retval TRUE                     The certificate Extension data retrieved successfully.
  @retval TRUE                     The Certificate Extension is found, but the oid extension is not found.
  @retval FALSE                    If Cert is NULL.
                                   If ExtensionDataSize is NULL.
                                   If ExtensionData is not NULL and *ExtensionDataSize is 0.
                                   If Certificate is invalid.
  @retval FALSE                    If the ExtensionData is NULL. The required buffer size
                                   is returned in the ExtensionDataSize parameter.
  @retval FALSE                    The operation is not supported.
**/
BOOLEAN
EFIAPI
X509GetExtensionData (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  IN CONST UINT8  *Oid,
  IN UINTN        OidSize,
  OUT UINT8       *ExtensionData,
  IN OUT UINTN    *ExtensionDataSize
  )
{
  mbedtls_x509_crt  Crt;
  INT32             Ret;
  RETURN_STATUS     ReturnStatus;
  BOOLEAN           Status;
  UINT8             *Ptr;
  UINT8             *End;
  size_t            ObjLen;

  if ((Cert == NULL) ||
      (CertSize == 0) ||
      (Oid == NULL) ||
      (OidSize == 0) ||
      (ExtensionDataSize == NULL))
  {
    if (ExtensionDataSize != NULL) {
      *ExtensionDataSize = 0;
    }

    return FALSE;
  }

  ReturnStatus = RETURN_INVALID_PARAMETER;
  Status       = FALSE;

  mbedtls_x509_crt_init (&Crt);

  Ret = mbedtls_x509_crt_parse_der (&Crt, Cert, CertSize);

  if (Ret == 0) {
    Ptr = Crt.v3_ext.p;
    End = Crt.v3_ext.p + Crt.v3_ext.len;
    Ret = mbedtls_asn1_get_tag (&Ptr, End, &ObjLen, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
  }

  if (Ret == 0) {
    ReturnStatus = InternalX509FindExtensionData (Ptr, End, Oid, OidSize, &Ptr, &ObjLen);
    if (!Status) {
      Status             = TRUE;
      *ExtensionDataSize = 0;
      goto Cleanup;
    }
  }

  if (ReturnStatus == RETURN_SUCCESS) {
    if (*ExtensionDataSize < ObjLen) {
      *ExtensionDataSize = ObjLen;
      Status             = FALSE;
      goto Cleanup;
    }

    if (Oid != NULL) {
      CopyMem (ExtensionData, Ptr, ObjLen);
    }

    *ExtensionDataSize = ObjLen;
    Status             = TRUE;
  }

Cleanup:
  mbedtls_x509_crt_free (&Crt);

  return Status;
}

/**
  Retrieve the Validity from one X.509 certificate

  If Cert is NULL, then return FALSE.
  If CertIssuerSize is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[in]      From         notBefore Pointer to DateTime object.
  @param[in,out]  FromSize     notBefore DateTime object size.
  @param[in]      To           notAfter Pointer to DateTime object.
  @param[in,out]  ToSize       notAfter DateTime object size.

  Note: X509CompareDateTime to compare DateTime oject
        x509SetDateTime to get a DateTime object from a DateTimeStr

  @retval  TRUE   The certificate Validity retrieved successfully.
  @retval  FALSE  Invalid certificate, or Validity retrieve failed.
  @retval  FALSE  This interface is not supported.
**/
BOOLEAN
EFIAPI
X509GetValidity (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  IN UINT8        *From,
  IN OUT UINTN    *FromSize,
  IN UINT8        *To,
  IN OUT UINTN    *ToSize
  )
{
  mbedtls_x509_crt  Crt;
  INT32             Ret;
  BOOLEAN           Status;
  UINTN             TSize;
  UINTN             FSize;

  if (Cert == NULL) {
    return FALSE;
  }

  Status = FALSE;

  mbedtls_x509_crt_init (&Crt);

  Ret = mbedtls_x509_crt_parse_der (&Crt, Cert, CertSize);

  if (Ret == 0) {
    FSize = sizeof (mbedtls_x509_time);
    if (*FromSize < FSize) {
      *FromSize = FSize;
      goto _Exit;
    }

    *FromSize = FSize;
    if (From != NULL) {
      CopyMem (From, &(Crt.valid_from), FSize);
    }

    TSize = sizeof (mbedtls_x509_time);
    if (*ToSize < TSize) {
      *ToSize = TSize;
      goto _Exit;
    }

    *ToSize = TSize;
    if (To != NULL) {
      CopyMem (To, &(Crt.valid_to), sizeof (mbedtls_x509_time));
    }

    Status = TRUE;
  }

_Exit:
  mbedtls_x509_crt_free (&Crt);

  return Status;
}

/**
  Retrieve the Key Usage from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     Usage            Key Usage (CRYPTO_X509_KU_*)

  @retval  TRUE   The certificate Key Usage retrieved successfully.
  @retval  FALSE  Invalid certificate, or Usage is NULL
  @retval  FALSE  This interface is not supported.
**/
BOOLEAN
EFIAPI
X509GetKeyUsage (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT UINTN       *Usage
  )
{
  mbedtls_x509_crt  Crt;
  INT32             Ret;
  BOOLEAN           Status;

  if (Cert == NULL) {
    return FALSE;
  }

  Status = FALSE;

  mbedtls_x509_crt_init (&Crt);

  Ret = mbedtls_x509_crt_parse_der (&Crt, Cert, CertSize);

  if (Ret == 0) {
    *Usage = Crt.key_usage;
    Status = TRUE;
  }

  mbedtls_x509_crt_free (&Crt);

  return Status;
}

/**
  Retrieve the Extended Key Usage from one X.509 certificate.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[out]     Usage            Key Usage bytes.
  @param[in, out] UsageSize        Key Usage buffer size in bytes.

  @retval TRUE                     The Usage bytes retrieve successfully.
  @retval FALSE                    If Cert is NULL.
                                   If CertSize is NULL.
                                   If Usage is not NULL and *UsageSize is 0.
                                   If Cert is invalid.
  @retval FALSE                    If the Usage is NULL. The required buffer size
                                   is returned in the UsageSize parameter.
  @retval FALSE                    The operation is not supported.
**/
BOOLEAN
EFIAPI
X509GetExtendedKeyUsage (
  IN CONST UINT8  *Cert,
  IN UINTN        CertSize,
  OUT UINT8       *Usage,
  IN OUT UINTN    *UsageSize
  )
{
  BOOLEAN  ReturnStatus;

  if ((Cert == NULL) || (CertSize == 0) || (UsageSize == NULL)) {
    return FALSE;
  }

  ReturnStatus = X509GetExtensionData ((UINT8 *)Cert, CertSize, (UINT8 *)OID_extKeyUsage, sizeof (OID_extKeyUsage), Usage, UsageSize);

  return ReturnStatus;
}

/**
  Compare DateTime1 object and DateTime2 object time.

  @param[in]      Before         Pointer to a DateTime Ojbect
  @param[in]      After          Pointer to a DateTime Object

  @retval  0      If DateTime1 <= DateTime2
  @retval  1      If DateTime1 > DateTime2
**/
STATIC
INTN
InternalX509CheckTime (
  CONST mbedtls_x509_time  *Before,
  CONST mbedtls_x509_time  *After
  )
{
  if (Before->year > After->year) {
    return (1);
  }

  if ((Before->year == After->year) &&
      (Before->mon > After->mon))
  {
    return (1);
  }

  if ((Before->year == After->year) &&
      (Before->mon == After->mon) &&
      (Before->day > After->day))
  {
    return (1);
  }

  if ((Before->year == After->year) &&
      (Before->mon == After->mon) &&
      (Before->day == After->day) &&
      (Before->hour > After->hour))
  {
    return (1);
  }

  if ((Before->year == After->year) &&
      (Before->mon == After->mon) &&
      (Before->day == After->day) &&
      (Before->hour == After->hour) &&
      (Before->min > After->min))
  {
    return (1);
  }

  if ((Before->year == After->year) &&
      (Before->mon == After->mon) &&
      (Before->day == After->day) &&
      (Before->hour == After->hour) &&
      (Before->min == After->min) &&
      (Before->sec > After->sec))
  {
    return (1);
  }

  return (0);
}

/**
  change string to int.

  @param[in] PStart         Pointer to a string Start
  @param[in] PEnd           Pointer to a string End

  @return number
**/
STATIC
INT32
InternalAtoI (
  CHAR8  *PStart,
  CHAR8  *PEnd
  )
{
  CHAR8  *Ptr;
  INT32  Knum;

  Knum = 0;
  Ptr  = PStart;

  while (Ptr < PEnd) {
    ///
    /// k = k * 2^3 + k * 2^1 = k * 8 + k * 2 = k * 10
    ///
    Knum = (Knum << 3) + (Knum << 1) + (*Ptr) - '0';
    Ptr++;
  }

  return Knum;
}

/**
  Format a DateTime object into DataTime Buffer

  If DateTimeStr is NULL, then return FALSE.
  If DateTimeSize is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      DateTimeStr      DateTime string like YYYYMMDDhhmmssZ
                                   Ref: https://www.w3.org/TR/NOTE-datetime
                                   Z stand for UTC time
  @param[in,out]  DateTime         Pointer to a DateTime object.
  @param[in,out]  DateTimeSize     DateTime object buffer size.

  @retval RETURN_SUCCESS           The DateTime object create successfully.
  @retval RETURN_INVALID_PARAMETER If DateTimeStr is NULL.
                                   If DateTimeSize is NULL.
                                   If DateTime is not NULL and *DateTimeSize is 0.
                                   If Year Month Day Hour Minute Second combination is invalid datetime.
  @retval RETURN_BUFFER_TOO_SMALL  If the DateTime is NULL. The required buffer size
                                   (including the final null) is returned in the
                                   DateTimeSize parameter.
  @retval RETURN_UNSUPPORTED       The operation is not supported.
**/
RETURN_STATUS
EFIAPI
X509SetDateTime (
  CHAR8         *DateTimeStr,
  IN OUT VOID   *DateTime,
  IN OUT UINTN  *DateTimeSize
  )
{
  mbedtls_x509_time  Dt;

  INT32          Year;
  INT32          Month;
  INT32          Day;
  INT32          Hour;
  INT32          Minute;
  INT32          Second;
  RETURN_STATUS  ReturnStatus;
  CHAR8          *Ptr;

  Ptr = DateTimeStr;

  Year    = InternalAtoI (Ptr, Ptr + 4);
  Ptr    += 4;
  Month   = InternalAtoI (Ptr, Ptr + 2);
  Ptr    += 2;
  Day     = InternalAtoI (Ptr, Ptr + 2);
  Ptr    += 2;
  Hour    = InternalAtoI (Ptr, Ptr + 2);
  Ptr    += 2;
  Minute  = InternalAtoI (Ptr, Ptr + 2);
  Ptr    += 2;
  Second  = InternalAtoI (Ptr, Ptr + 2);
  Ptr    += 2;
  Dt.year = (int)Year;
  Dt.mon  = (int)Month;
  Dt.day  = (int)Day;
  Dt.hour = (int)Hour;
  Dt.min  = (int)Minute;
  Dt.sec  = (int)Second;

  if (*DateTimeSize < sizeof (mbedtls_x509_time)) {
    *DateTimeSize = sizeof (mbedtls_x509_time);
    ReturnStatus  = RETURN_BUFFER_TOO_SMALL;
    goto Cleanup;
  }

  if (DateTime != NULL) {
    CopyMem (DateTime, &Dt, sizeof (mbedtls_x509_time));
  }

  *DateTimeSize = sizeof (mbedtls_x509_time);
  ReturnStatus  = RETURN_SUCCESS;
Cleanup:
  return ReturnStatus;
}

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
INT32
EFIAPI
X509CompareDateTime (
  IN CONST VOID  *DateTime1,
  IN CONST VOID  *DateTime2
  )
{
  if ((DateTime1 == NULL) || (DateTime2 == NULL)) {
    return -2;
  }

  if (CompareMem (DateTime2, DateTime1, sizeof (mbedtls_x509_time)) == 0) {
    return 0;
  }

  if (InternalX509CheckTime ((mbedtls_x509_time *)DateTime1, (mbedtls_x509_time *)DateTime2) == 0) {
    return -1;
  } else {
    return 1;
  }
}

/**
  Retrieve the basic constraints from one X.509 certificate.

  @param[in]      Cert                     Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize                 size of the X509 certificate in bytes.
  @param[out]     BasicConstraints         basic constraints bytes.
  @param[in, out] BasicConstraintsSize     basic constraints buffer size in bytes.

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
BOOLEAN
EFIAPI
X509GetExtendedBasicConstraints (
  CONST UINT8  *Cert,
  UINTN        CertSize,
  UINT8        *BasicConstraints,
  UINTN        *BasicConstraintsSize
  )
{
  BOOLEAN  Status;

  if ((Cert == NULL) || (CertSize == 0) || (BasicConstraintsSize == NULL)) {
    return FALSE;
  }

  Status = X509GetExtensionData (
             (UINT8 *)Cert,
             CertSize,
             OID_BasicConstraints,
             sizeof (OID_BasicConstraints),
             BasicConstraints,
             BasicConstraintsSize
             );

  return Status;
}

/**
  Format a DateTimeStr to DataTime object in DataTime Buffer

  If DateTimeStr is NULL, then return FALSE.
  If DateTimeSize is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      DateTimeStr      DateTime string like YYYYMMDDhhmmssZ
                                   Ref: https://www.w3.org/TR/NOTE-datetime
                                   Z stand for UTC time
  @param[out]     DateTime         Pointer to a DateTime object.
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
BOOLEAN
EFIAPI
X509FormatDateTime (
  IN CONST CHAR8  *DateTimeStr,
  OUT VOID        *DateTime,
  IN OUT UINTN    *DateTimeSize
  )
{
  mbedtls_x509_time  *Tm;

  if (*DateTimeSize < sizeof (mbedtls_x509_time)) {
    return FALSE;
  }

  if (DateTime == NULL) {
    return FALSE;
  }

  Tm = (mbedtls_x509_time *)DateTime;

  Tm->year = (DateTimeStr[0] - '0') * 1000 + (DateTimeStr[1] - '0') * 100 +
             (DateTimeStr[2] - '0') * 10 + (DateTimeStr[3] - '0') * 1;

  Tm->mon = (DateTimeStr[4] - '0') * 10 + (DateTimeStr[5] - '0') * 1;

  Tm->day = (DateTimeStr[6] - '0') * 10 + (DateTimeStr[7] - '0') * 1;

  Tm->hour = (DateTimeStr[8] - '0') * 10 + (DateTimeStr[9] - '0') * 1;

  Tm->min = (DateTimeStr[10] - '0') * 10 + (DateTimeStr[11] - '0') * 1;

  Tm->sec = (DateTimeStr[12] - '0') * 10 + (DateTimeStr[13] - '0') * 1;

  return TRUE;
}

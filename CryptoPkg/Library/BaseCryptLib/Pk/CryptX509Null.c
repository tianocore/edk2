/** @file
  X.509 Certificate Handler Wrapper Implementation which does not provide
  real capabilities.

Copyright (c) 2012 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

/**
  Construct a X509 object from DER-encoded certificate data.

  Return FALSE to indicate this interface is not supported.

  @param[in]  Cert            Pointer to the DER-encoded certificate data.
  @param[in]  CertSize        The size of certificate data in bytes.
  @param[out] SingleX509Cert  The generated X509 object.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
X509ConstructCertificate (
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  UINT8        **SingleX509Cert
  )
{
  ASSERT (FALSE);
  return FALSE;
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
  IN OUT  UINT8    **X509Stack,
  IN      VA_LIST  Args
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Construct a X509 stack object from a list of DER-encoded certificate data.

  Return FALSE to indicate this interface is not supported.

  @param[in, out]  X509Stack  On input, pointer to an existing or NULL X509 stack object.
                              On output, pointer to the X509 stack object with new
                              inserted X509 certificate.
  @param           ...        A list of DER-encoded single certificate data followed
                              by certificate size. A NULL terminates the list. The
                              pairs are the arguments to X509ConstructCertificate().

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
X509ConstructCertificateStack (
  IN OUT  UINT8  **X509Stack,
  ...
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Release the specified X509 object.

  If the interface is not supported, then ASSERT().

  @param[in]  X509Cert  Pointer to the X509 object to be released.

**/
VOID
EFIAPI
X509Free (
  IN  VOID  *X509Cert
  )
{
  ASSERT (FALSE);
}

/**
  Release the specified X509 stack object.

  If the interface is not supported, then ASSERT().

  @param[in]  X509Stack  Pointer to the X509 stack object to be released.

**/
VOID
EFIAPI
X509StackFree (
  IN  VOID  *X509Stack
  )
{
  ASSERT (FALSE);
}

/**
  Retrieve the subject bytes from one X.509 certificate.

  Return FALSE to indicate this interface is not supported.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[out]     CertSubject  Pointer to the retrieved certificate subject bytes.
  @param[in, out] SubjectSize  The size in bytes of the CertSubject buffer on input,
                               and the size of buffer returned CertSubject on output.


  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
X509GetSubjectName (
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     UINT8        *CertSubject,
  IN OUT  UINTN        *SubjectSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Retrieve the common name (CN) string from one X.509 certificate.

  Return RETURN_UNSUPPORTED to indicate this interface is not supported.

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

  @retval RETURN_UNSUPPORTED       The operation is not supported.

**/
RETURN_STATUS
EFIAPI
X509GetCommonName (
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     CHAR8        *CommonName   OPTIONAL,
  IN OUT  UINTN        *CommonNameSize
  )
{
  ASSERT (FALSE);
  return RETURN_UNSUPPORTED;
}

/**
  Retrieve the organization name (ON) string from one X.509 certificate.

  Return RETURN_UNSUPPORTED to indicate this interface is not supported.

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

  @retval RETURN_UNSUPPORTED       The operation is not supported.

**/
RETURN_STATUS
EFIAPI
X509GetOrganizationName (
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     CHAR8        *NameBuffer   OPTIONAL,
  IN OUT  UINTN        *NameBufferSize
  )
{
  ASSERT (FALSE);
  return RETURN_UNSUPPORTED;
}

/**
  Retrieve the RSA Public Key from one DER-encoded X509 certificate.

  Return FALSE to indicate this interface is not supported.

  @param[in]  Cert         Pointer to the DER-encoded X509 certificate.
  @param[in]  CertSize     Size of the X509 certificate in bytes.
  @param[out] RsaContext   Pointer to new-generated RSA context which contain the retrieved
                           RSA public key component. Use RsaFree() function to free the
                           resource.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RsaGetPublicKeyFromX509 (
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  VOID         **RsaContext
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Verify one X509 certificate was issued by the trusted CA.

  Return FALSE to indicate this interface is not supported.

  @param[in]      Cert         Pointer to the DER-encoded X509 certificate to be verified.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[in]      CACert       Pointer to the DER-encoded trusted CA certificate.
  @param[in]      CACertSize   Size of the CA Certificate in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
X509VerifyCert (
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *CACert,
  IN  UINTN        CACertSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Retrieve the TBSCertificate from one given X.509 certificate.

  Return FALSE to indicate this interface is not supported.

  @param[in]      Cert         Pointer to the given DER-encoded X509 certificate.
  @param[in]      CertSize     Size of the X509 certificate in bytes.
  @param[out]     TBSCert      DER-Encoded To-Be-Signed certificate.
  @param[out]     TBSCertSize  Size of the TBS certificate in bytes.

  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
X509GetTBSCert (
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  OUT UINT8        **TBSCert,
  OUT UINTN        *TBSCertSize
  )
{
  ASSERT (FALSE);
  return FALSE;
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
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  VOID         **EcContext
  )
{
  ASSERT (FALSE);
  return FALSE;
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
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     UINTN        *Version
  )
{
  ASSERT (FALSE);
  return FALSE;
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
  IN      CONST UINT8 *Cert,
  IN      UINTN CertSize,
  OUT     UINT8 *SerialNumber, OPTIONAL
  IN OUT  UINTN         *SerialNumberSize
  )
{
  ASSERT (FALSE);
  return FALSE;
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
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     UINT8        *CertIssuer,
  IN OUT  UINTN        *CertIssuerSize
  )
{
  ASSERT (FALSE);
  return FALSE;
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
  IN CONST UINT8 *Cert,
  IN       UINTN CertSize,
  OUT   UINT8 *Oid, OPTIONAL
  IN OUT   UINTN       *OidSize
  )
{
  ASSERT (FALSE);
  return FALSE;
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
  @retval FALSE                    If Cert is NULL.
                                   If ExtensionDataSize is NULL.
                                   If ExtensionData is not NULL and *ExtensionDataSize is 0.
                                   If Certificate is invalid.
  @retval FALSE                    If no Extension entry match Oid.
  @retval FALSE                    If the ExtensionData is NULL. The required buffer size
                                   is returned in the ExtensionDataSize parameter.
  @retval FALSE                    The operation is not supported.
**/
BOOLEAN
EFIAPI
X509GetExtensionData (
  IN     CONST UINT8  *Cert,
  IN     UINTN        CertSize,
  IN     CONST UINT8  *Oid,
  IN     UINTN        OidSize,
  OUT UINT8           *ExtensionData,
  IN OUT UINTN        *ExtensionDataSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

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
BOOLEAN
EFIAPI
X509GetExtendedKeyUsage (
  IN     CONST UINT8  *Cert,
  IN     UINTN        CertSize,
  OUT UINT8           *Usage,
  IN OUT UINTN        *UsageSize
  )
{
  ASSERT (FALSE);
  return FALSE;
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
  @param[in]     To           notAfter Pointer to DateTime object.
  @param[in,out]  ToSize       notAfter DateTime object size.

  Note: X509CompareDateTime to compare DateTime oject
        x509SetDateTime to get a DateTime object from a DateTimeStr

  @retval  TRUE   The certificate Validity retrieved successfully.
  @retval  FALSE  Invalid certificate, or Validity retrieve failed.
  @retval  FALSE  This interface is not supported.
**/
BOOLEAN
EFIAPI
X509GetValidity  (
  IN     CONST UINT8  *Cert,
  IN     UINTN        CertSize,
  IN     UINT8        *From,
  IN OUT UINTN        *FromSize,
  IN     UINT8        *To,
  IN OUT UINTN        *ToSize
  )
{
  ASSERT (FALSE);
  return FALSE;
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
  ASSERT (FALSE);
  return FALSE;
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
  IN  CONST  VOID  *DateTime1,
  IN  CONST  VOID  *DateTime2
  )
{
  ASSERT (FALSE);
  return -3;
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
  IN    CONST UINT8  *Cert,
  IN    UINTN        CertSize,
  OUT   UINTN        *Usage
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Verify one X509 certificate was issued by the trusted CA.
  @param[in]      RootCert          Trusted Root Certificate buffer

  @param[in]      RootCertLength    Trusted Root Certificate buffer length
  @param[in]      CertChain         One or more ASN.1 DER-encoded X.509 certificates
                                    where the first certificate is signed by the Root
                                    Certificate or is the Root Cerificate itself. and
                                    subsequent cerificate is signed by the preceding
                                    cerificate.
  @param[in]      CertChainLength   Total length of the certificate chain, in bytes.

  @retval  TRUE   All cerificates was issued by the first certificate in X509Certchain.
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
  ASSERT (FALSE);
  return FALSE;
}

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
  ASSERT (FALSE);
  return FALSE;
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
  IN     UINT32   Tag
  )
{
  ASSERT (FALSE);
  return FALSE;
}

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
BOOLEAN
EFIAPI
X509GetExtendedBasicConstraints             (
  CONST UINT8  *Cert,
  UINTN        CertSize,
  UINT8        *BasicConstraints,
  UINTN        *BasicConstraintsSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

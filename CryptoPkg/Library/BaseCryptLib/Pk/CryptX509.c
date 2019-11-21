/** @file
  X.509 Certificate Handler Wrapper Implementation over OpenSSL.

Copyright (c) 2010 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/x509.h>
#include <openssl/rsa.h>

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
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  UINT8        **SingleX509Cert
  )
{
  X509         *X509Cert;
  CONST UINT8  *Temp;

  //
  // Check input parameters.
  //
  if (Cert == NULL || SingleX509Cert == NULL || CertSize > INT_MAX) {
    return FALSE;
  }

  //
  // Read DER-encoded X509 Certificate and Construct X509 object.
  //
  Temp     = Cert;
  X509Cert = d2i_X509 (NULL, &Temp, (long) CertSize);
  if (X509Cert == NULL) {
    return FALSE;
  }

  *SingleX509Cert = (UINT8 *) X509Cert;

  return TRUE;
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
  UINT8           *Cert;
  UINTN           CertSize;
  X509            *X509Cert;
  STACK_OF(X509)  *CertStack;
  BOOLEAN         Status;
  UINTN           Index;

  //
  // Check input parameters.
  //
  if (X509Stack == NULL) {
    return FALSE;
  }

  Status = FALSE;

  //
  // Initialize X509 stack object.
  //
  CertStack = (STACK_OF(X509) *) (*X509Stack);
  if (CertStack == NULL) {
    CertStack = sk_X509_new_null ();
    if (CertStack == NULL) {
      return Status;
    }
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

    //
    // Construct X509 Object from the given DER-encoded certificate data.
    //
    X509Cert = NULL;
    Status = X509ConstructCertificate (
               (CONST UINT8 *) Cert,
               CertSize,
               (UINT8 **) &X509Cert
               );
    if (!Status) {
      if (X509Cert != NULL) {
        X509_free (X509Cert);
      }
      break;
    }

    //
    // Insert the new X509 object into X509 stack object.
    //
    sk_X509_push (CertStack, X509Cert);
  }

  if (!Status) {
    sk_X509_pop_free (CertStack, X509_free);
  } else {
    *X509Stack = (UINT8 *) CertStack;
  }

  return Status;
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
  IN OUT  UINT8  **X509Stack,
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
  IN  VOID  *X509Cert
  )
{
  //
  // Check input parameters.
  //
  if (X509Cert == NULL) {
    return;
  }

  //
  // Free OpenSSL X509 object.
  //
  X509_free ((X509 *) X509Cert);
}

/**
  Release the specified X509 stack object.

  If X509Stack is NULL, then return FALSE.

  @param[in]  X509Stack  Pointer to the X509 stack object to be released.

**/
VOID
EFIAPI
X509StackFree (
  IN  VOID  *X509Stack
  )
{
  //
  // Check input parameters.
  //
  if (X509Stack == NULL) {
    return;
  }

  //
  // Free OpenSSL X509 stack object.
  //
  sk_X509_pop_free ((STACK_OF(X509) *) X509Stack, X509_free);
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
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     UINT8        *CertSubject,
  IN OUT  UINTN        *SubjectSize
  )
{
  BOOLEAN    Status;
  X509       *X509Cert;
  X509_NAME  *X509Name;
  UINTN      X509NameSize;

  //
  // Check input parameters.
  //
  if (Cert == NULL || SubjectSize == NULL) {
    return FALSE;
  }

  X509Cert = NULL;

  //
  // Read DER-encoded X509 Certificate and Construct X509 object.
  //
  Status = X509ConstructCertificate (Cert, CertSize, (UINT8 **) &X509Cert);
  if ((X509Cert == NULL) || (!Status)) {
    Status = FALSE;
    goto _Exit;
  }

  Status = FALSE;

  //
  // Retrieve subject name from certificate object.
  //
  X509Name = X509_get_subject_name (X509Cert);
  if (X509Name == NULL) {
    goto _Exit;
  }

  X509NameSize = i2d_X509_NAME(X509Name, NULL);
  if (*SubjectSize < X509NameSize) {
    *SubjectSize = X509NameSize;
    goto _Exit;
  }
  *SubjectSize = X509NameSize;
  if (CertSubject != NULL) {
    i2d_X509_NAME(X509Name, &CertSubject);
    Status = TRUE;
  }

_Exit:
  //
  // Release Resources.
  //
  if (X509Cert != NULL) {
    X509_free (X509Cert);
  }

  return Status;
}

/**
  Retrieve a string from one X.509 certificate base on the Request_NID.

  @param[in]      Cert             Pointer to the DER-encoded X509 certificate.
  @param[in]      CertSize         Size of the X509 certificate in bytes.
  @param[in]      Request_NID      NID of string to obtain
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
STATIC
RETURN_STATUS
InternalX509GetNIDName (
  IN      CONST UINT8   *Cert,
  IN      UINTN         CertSize,
  IN      INT32         Request_NID,
  OUT     CHAR8         *CommonName,  OPTIONAL
  IN OUT  UINTN         *CommonNameSize
  )
{
  RETURN_STATUS    ReturnStatus;
  BOOLEAN          Status;
  X509             *X509Cert;
  X509_NAME        *X509Name;
  INT32            Index;
  INTN             Length;
  X509_NAME_ENTRY  *Entry;
  ASN1_STRING      *EntryData;
  UINT8            *UTF8Name;

  ReturnStatus = RETURN_INVALID_PARAMETER;
  UTF8Name     = NULL;

  //
  // Check input parameters.
  //
  if ((Cert == NULL) || (CertSize > INT_MAX) || (CommonNameSize == NULL)) {
    return ReturnStatus;
  }
  if ((CommonName != NULL) && (*CommonNameSize == 0)) {
    return ReturnStatus;
  }

  X509Cert = NULL;
  //
  // Read DER-encoded X509 Certificate and Construct X509 object.
  //
  Status = X509ConstructCertificate (Cert, CertSize, (UINT8 **) &X509Cert);
  if ((X509Cert == NULL) || (!Status)) {
    //
    // Invalid X.509 Certificate
    //
    goto _Exit;
  }

  Status = FALSE;

  //
  // Retrieve subject name from certificate object.
  //
  X509Name = X509_get_subject_name (X509Cert);
  if (X509Name == NULL) {
    //
    // Fail to retrieve subject name content
    //
    goto _Exit;
  }

  //
  // Retrive the string from X.509 Subject base on the Request_NID
  //
  Index = X509_NAME_get_index_by_NID (X509Name, Request_NID, -1);
  if (Index < 0) {
    //
    // No Request_NID name entry exists in X509_NAME object
    //
    *CommonNameSize = 0;
    ReturnStatus    = RETURN_NOT_FOUND;
    goto _Exit;
  }

  Entry = X509_NAME_get_entry (X509Name, Index);
  if (Entry == NULL) {
    //
    // Fail to retrieve name entry data
    //
    *CommonNameSize = 0;
    ReturnStatus    = RETURN_NOT_FOUND;
    goto _Exit;
  }

  EntryData = X509_NAME_ENTRY_get_data (Entry);

  Length = ASN1_STRING_to_UTF8 (&UTF8Name, EntryData);
  if (Length < 0) {
    //
    // Fail to convert the Name string
    //
    *CommonNameSize = 0;
    ReturnStatus    = RETURN_INVALID_PARAMETER;
    goto _Exit;
  }

  if (CommonName == NULL) {
    *CommonNameSize = Length + 1;
    ReturnStatus = RETURN_BUFFER_TOO_SMALL;
  } else {
    *CommonNameSize = MIN ((UINTN)Length, *CommonNameSize - 1) + 1;
    CopyMem (CommonName, UTF8Name, *CommonNameSize - 1);
    CommonName[*CommonNameSize - 1] = '\0';
    ReturnStatus = RETURN_SUCCESS;
  }

_Exit:
  //
  // Release Resources.
  //
  if (X509Cert != NULL) {
    X509_free (X509Cert);
  }
  if (UTF8Name != NULL) {
    OPENSSL_free (UTF8Name);
  }

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
  IN      CONST UINT8  *Cert,
  IN      UINTN        CertSize,
  OUT     CHAR8        *CommonName,  OPTIONAL
  IN OUT  UINTN        *CommonNameSize
  )
{
  return InternalX509GetNIDName (Cert, CertSize, NID_commonName, CommonName, CommonNameSize);
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
  IN      CONST UINT8   *Cert,
  IN      UINTN         CertSize,
  OUT     CHAR8         *NameBuffer,  OPTIONAL
  IN OUT  UINTN         *NameBufferSize
  )
{
  return InternalX509GetNIDName (Cert, CertSize, NID_organizationName, NameBuffer, NameBufferSize);
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
  IN   CONST UINT8  *Cert,
  IN   UINTN        CertSize,
  OUT  VOID         **RsaContext
  )
{
  BOOLEAN   Status;
  EVP_PKEY  *Pkey;
  X509      *X509Cert;

  //
  // Check input parameters.
  //
  if (Cert == NULL || RsaContext == NULL) {
    return FALSE;
  }

  Pkey     = NULL;
  X509Cert = NULL;

  //
  // Read DER-encoded X509 Certificate and Construct X509 object.
  //
  Status = X509ConstructCertificate (Cert, CertSize, (UINT8 **) &X509Cert);
  if ((X509Cert == NULL) || (!Status)) {
    Status = FALSE;
    goto _Exit;
  }

  Status = FALSE;

  //
  // Retrieve and check EVP_PKEY data from X509 Certificate.
  //
  Pkey = X509_get_pubkey (X509Cert);
  if ((Pkey == NULL) || (EVP_PKEY_id (Pkey) != EVP_PKEY_RSA)) {
    goto _Exit;
  }

  //
  // Duplicate RSA Context from the retrieved EVP_PKEY.
  //
  if ((*RsaContext = RSAPublicKey_dup (EVP_PKEY_get0_RSA (Pkey))) != NULL) {
    Status = TRUE;
  }

_Exit:
  //
  // Release Resources.
  //
  if (X509Cert != NULL) {
    X509_free (X509Cert);
  }

  if (Pkey != NULL) {
    EVP_PKEY_free (Pkey);
  }

  return Status;
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
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *CACert,
  IN  UINTN        CACertSize
  )
{
  BOOLEAN         Status;
  X509            *X509Cert;
  X509            *X509CACert;
  X509_STORE      *CertStore;
  X509_STORE_CTX  *CertCtx;

  //
  // Check input parameters.
  //
  if (Cert == NULL || CACert == NULL) {
    return FALSE;
  }

  Status     = FALSE;
  X509Cert   = NULL;
  X509CACert = NULL;
  CertStore  = NULL;
  CertCtx    = NULL;

  //
  // Register & Initialize necessary digest algorithms for certificate verification.
  //
  if (EVP_add_digest (EVP_md5 ()) == 0) {
    goto _Exit;
  }
  if (EVP_add_digest (EVP_sha1 ()) == 0) {
    goto _Exit;
  }
  if (EVP_add_digest (EVP_sha256 ()) == 0) {
    goto _Exit;
  }

  //
  // Read DER-encoded certificate to be verified and Construct X509 object.
  //
  Status = X509ConstructCertificate (Cert, CertSize, (UINT8 **) &X509Cert);
  if ((X509Cert == NULL) || (!Status)) {
    Status = FALSE;
    goto _Exit;
  }

  //
  // Read DER-encoded root certificate and Construct X509 object.
  //
  Status = X509ConstructCertificate (CACert, CACertSize, (UINT8 **) &X509CACert);
  if ((X509CACert == NULL) || (!Status)) {
    Status = FALSE;
    goto _Exit;
  }

  Status = FALSE;

  //
  // Set up X509 Store for trusted certificate.
  //
  CertStore = X509_STORE_new ();
  if (CertStore == NULL) {
    goto _Exit;
  }
  if (!(X509_STORE_add_cert (CertStore, X509CACert))) {
    goto _Exit;
  }

  //
  // Allow partial certificate chains, terminated by a non-self-signed but
  // still trusted intermediate certificate. Also disable time checks.
  //
  X509_STORE_set_flags (CertStore,
                        X509_V_FLAG_PARTIAL_CHAIN | X509_V_FLAG_NO_CHECK_TIME);

  //
  // Set up X509_STORE_CTX for the subsequent verification operation.
  //
  CertCtx = X509_STORE_CTX_new ();
  if (CertCtx == NULL) {
    goto _Exit;
  }
  if (!X509_STORE_CTX_init (CertCtx, CertStore, X509Cert, NULL)) {
    goto _Exit;
  }

  //
  // X509 Certificate Verification.
  //
  Status = (BOOLEAN) X509_verify_cert (CertCtx);
  X509_STORE_CTX_cleanup (CertCtx);

_Exit:
  //
  // Release Resources.
  //
  if (X509Cert != NULL) {
    X509_free (X509Cert);
  }

  if (X509CACert != NULL) {
    X509_free (X509CACert);
  }

  if (CertStore != NULL) {
    X509_STORE_free (CertStore);
  }

  X509_STORE_CTX_free (CertCtx);

  return Status;
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
  IN  CONST UINT8  *Cert,
  IN  UINTN        CertSize,
  OUT UINT8        **TBSCert,
  OUT UINTN        *TBSCertSize
  )
{
  CONST UINT8  *Temp;
  UINT32       Asn1Tag;
  UINT32       ObjClass;
  UINTN        Length;

  //
  // Check input parameters.
  //
  if ((Cert == NULL) || (TBSCert == NULL) ||
      (TBSCertSize == NULL) || (CertSize > INT_MAX)) {
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
  Temp   = Cert;
  Length = 0;
  ASN1_get_object (&Temp, (long *)&Length, (int *)&Asn1Tag, (int *)&ObjClass, (long)CertSize);

  if (Asn1Tag != V_ASN1_SEQUENCE) {
    return FALSE;
  }

  *TBSCert = (UINT8 *)Temp;

  ASN1_get_object (&Temp, (long *)&Length, (int *)&Asn1Tag, (int *)&ObjClass, (long)Length);
  //
  // Verify the parsed TBSCertificate is one correct SEQUENCE data.
  //
  if (Asn1Tag != V_ASN1_SEQUENCE) {
    return FALSE;
  }

  *TBSCertSize = Length + (Temp - *TBSCert);

  return TRUE;
}

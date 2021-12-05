/** @file
  SSL/TLS Configuration Library Wrapper Implementation over OpenSSL.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalTlsLib.h"

typedef struct {
  //
  // IANA/IETF defined Cipher Suite ID
  //
  UINT16         IanaCipher;
  //
  // OpenSSL-used Cipher Suite String
  //
  CONST CHAR8    *OpensslCipher;
  //
  // Length of OpensslCipher
  //
  UINTN          OpensslCipherLength;
} TLS_CIPHER_MAPPING;

//
// Create a TLS_CIPHER_MAPPING initializer from IanaCipher and OpensslCipher so
// that OpensslCipherLength is filled in automatically. IanaCipher must be an
// integer constant expression, and OpensslCipher must be a string literal.
//
#define MAP(IanaCipher, OpensslCipher) \
  { (IanaCipher), (OpensslCipher), sizeof (OpensslCipher) - 1 }

//
// The mapping table between IANA/IETF Cipher Suite definitions and
// OpenSSL-used Cipher Suite name.
//
// Keep the table uniquely sorted by the IanaCipher field, in increasing order.
//
STATIC CONST TLS_CIPHER_MAPPING  TlsCipherMappingTable[] = {
  MAP (0x0001, "NULL-MD5"),                         /// TLS_RSA_WITH_NULL_MD5
  MAP (0x0002, "NULL-SHA"),                         /// TLS_RSA_WITH_NULL_SHA
  MAP (0x0004, "RC4-MD5"),                          /// TLS_RSA_WITH_RC4_128_MD5
  MAP (0x0005, "RC4-SHA"),                          /// TLS_RSA_WITH_RC4_128_SHA
  MAP (0x000A, "DES-CBC3-SHA"),                     /// TLS_RSA_WITH_3DES_EDE_CBC_SHA, mandatory TLS 1.1
  MAP (0x0016, "DHE-RSA-DES-CBC3-SHA"),             /// TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA
  MAP (0x002F, "AES128-SHA"),                       /// TLS_RSA_WITH_AES_128_CBC_SHA, mandatory TLS 1.2
  MAP (0x0030, "DH-DSS-AES128-SHA"),                /// TLS_DH_DSS_WITH_AES_128_CBC_SHA
  MAP (0x0031, "DH-RSA-AES128-SHA"),                /// TLS_DH_RSA_WITH_AES_128_CBC_SHA
  MAP (0x0033, "DHE-RSA-AES128-SHA"),               /// TLS_DHE_RSA_WITH_AES_128_CBC_SHA
  MAP (0x0035, "AES256-SHA"),                       /// TLS_RSA_WITH_AES_256_CBC_SHA
  MAP (0x0036, "DH-DSS-AES256-SHA"),                /// TLS_DH_DSS_WITH_AES_256_CBC_SHA
  MAP (0x0037, "DH-RSA-AES256-SHA"),                /// TLS_DH_RSA_WITH_AES_256_CBC_SHA
  MAP (0x0039, "DHE-RSA-AES256-SHA"),               /// TLS_DHE_RSA_WITH_AES_256_CBC_SHA
  MAP (0x003B, "NULL-SHA256"),                      /// TLS_RSA_WITH_NULL_SHA256
  MAP (0x003C, "AES128-SHA256"),                    /// TLS_RSA_WITH_AES_128_CBC_SHA256
  MAP (0x003D, "AES256-SHA256"),                    /// TLS_RSA_WITH_AES_256_CBC_SHA256
  MAP (0x003E, "DH-DSS-AES128-SHA256"),             /// TLS_DH_DSS_WITH_AES_128_CBC_SHA256
  MAP (0x003F, "DH-RSA-AES128-SHA256"),             /// TLS_DH_RSA_WITH_AES_128_CBC_SHA256
  MAP (0x0067, "DHE-RSA-AES128-SHA256"),            /// TLS_DHE_RSA_WITH_AES_128_CBC_SHA256
  MAP (0x0068, "DH-DSS-AES256-SHA256"),             /// TLS_DH_DSS_WITH_AES_256_CBC_SHA256
  MAP (0x0069, "DH-RSA-AES256-SHA256"),             /// TLS_DH_RSA_WITH_AES_256_CBC_SHA256
  MAP (0x006B, "DHE-RSA-AES256-SHA256"),            /// TLS_DHE_RSA_WITH_AES_256_CBC_SHA256
};

/**
  Gets the OpenSSL cipher suite mapping for the supplied IANA TLS cipher suite.

  @param[in]  CipherId    The supplied IANA TLS cipher suite ID.

  @return  The corresponding OpenSSL cipher suite mapping if found,
           NULL otherwise.

**/
STATIC
CONST TLS_CIPHER_MAPPING *
TlsGetCipherMapping (
  IN     UINT16  CipherId
  )
{
  INTN  Left;
  INTN  Right;
  INTN  Middle;

  //
  // Binary Search Cipher Mapping Table for IANA-OpenSSL Cipher Translation
  //
  Left  = 0;
  Right = ARRAY_SIZE (TlsCipherMappingTable) - 1;

  while (Right >= Left) {
    Middle = (Left + Right) / 2;

    if (CipherId == TlsCipherMappingTable[Middle].IanaCipher) {
      //
      // Translate IANA cipher suite ID to OpenSSL name.
      //
      return &TlsCipherMappingTable[Middle];
    }

    if (CipherId < TlsCipherMappingTable[Middle].IanaCipher) {
      Right = Middle - 1;
    } else {
      Left = Middle + 1;
    }
  }

  //
  // No Cipher Mapping found, return NULL.
  //
  return NULL;
}

/**
  Set a new TLS/SSL method for a particular TLS object.

  This function sets a new TLS/SSL method for a particular TLS object.

  @param[in]  Tls         Pointer to a TLS object.
  @param[in]  MajorVer    Major Version of TLS/SSL Protocol.
  @param[in]  MinorVer    Minor Version of TLS/SSL Protocol.

  @retval  EFI_SUCCESS           The TLS/SSL method was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       Unsupported TLS/SSL method.

**/
EFI_STATUS
EFIAPI
TlsSetVersion (
  IN     VOID   *Tls,
  IN     UINT8  MajorVer,
  IN     UINT8  MinorVer
  )
{
  TLS_CONNECTION  *TlsConn;
  UINT16          ProtoVersion;

  TlsConn = (TLS_CONNECTION *)Tls;
  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ProtoVersion = (MajorVer << 8) | MinorVer;

  //
  // Bound TLS method to the particular specified version.
  //
  switch (ProtoVersion) {
    case TLS1_VERSION:
      //
      // TLS 1.0
      //
      SSL_set_min_proto_version (TlsConn->Ssl, TLS1_VERSION);
      SSL_set_max_proto_version (TlsConn->Ssl, TLS1_VERSION);
      break;
    case TLS1_1_VERSION:
      //
      // TLS 1.1
      //
      SSL_set_min_proto_version (TlsConn->Ssl, TLS1_1_VERSION);
      SSL_set_max_proto_version (TlsConn->Ssl, TLS1_1_VERSION);
      break;
    case TLS1_2_VERSION:
      //
      // TLS 1.2
      //
      SSL_set_min_proto_version (TlsConn->Ssl, TLS1_2_VERSION);
      SSL_set_max_proto_version (TlsConn->Ssl, TLS1_2_VERSION);
      break;
    default:
      //
      // Unsupported Protocol Version
      //
      return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Set TLS object to work in client or server mode.

  This function prepares a TLS object to work in client or server mode.

  @param[in]  Tls         Pointer to a TLS object.
  @param[in]  IsServer    Work in server mode.

  @retval  EFI_SUCCESS           The TLS/SSL work mode was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       Unsupported TLS/SSL work mode.

**/
EFI_STATUS
EFIAPI
TlsSetConnectionEnd (
  IN     VOID     *Tls,
  IN     BOOLEAN  IsServer
  )
{
  TLS_CONNECTION  *TlsConn;

  TlsConn = (TLS_CONNECTION *)Tls;
  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsServer) {
    //
    // Set TLS to work in Client mode.
    //
    SSL_set_connect_state (TlsConn->Ssl);
  } else {
    //
    // Set TLS to work in Server mode.
    // It is unsupported for UEFI version currently.
    //
    // SSL_set_accept_state (TlsConn->Ssl);
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Set the ciphers list to be used by the TLS object.

  This function sets the ciphers for use by a specified TLS object.

  @param[in]  Tls          Pointer to a TLS object.
  @param[in]  CipherId     Array of UINT16 cipher identifiers. Each UINT16
                           cipher identifier comes from the TLS Cipher Suite
                           Registry of the IANA, interpreting Byte1 and Byte2
                           in network (big endian) byte order.
  @param[in]  CipherNum    The number of cipher in the list.

  @retval  EFI_SUCCESS           The ciphers list was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       No supported TLS cipher was found in CipherId.
  @retval  EFI_OUT_OF_RESOURCES  Memory allocation failed.

**/
EFI_STATUS
EFIAPI
TlsSetCipherList (
  IN     VOID    *Tls,
  IN     UINT16  *CipherId,
  IN     UINTN   CipherNum
  )
{
  TLS_CONNECTION            *TlsConn;
  EFI_STATUS                Status;
  CONST TLS_CIPHER_MAPPING  **MappedCipher;
  UINTN                     MappedCipherBytes;
  UINTN                     MappedCipherCount;
  UINTN                     CipherStringSize;
  UINTN                     Index;
  CONST TLS_CIPHER_MAPPING  *Mapping;
  CHAR8                     *CipherString;
  CHAR8                     *CipherStringPosition;

  TlsConn = (TLS_CONNECTION *)Tls;
  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (CipherId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Allocate the MappedCipher array for recording the mappings that we find
  // for the input IANA identifiers in CipherId.
  //
  Status = SafeUintnMult (
             CipherNum,
             sizeof (*MappedCipher),
             &MappedCipherBytes
             );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  MappedCipher = AllocatePool (MappedCipherBytes);
  if (MappedCipher == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Map the cipher IDs, and count the number of bytes for the full
  // CipherString.
  //
  MappedCipherCount = 0;
  CipherStringSize  = 0;
  for (Index = 0; Index < CipherNum; Index++) {
    //
    // Look up the IANA-to-OpenSSL mapping.
    //
    Mapping = TlsGetCipherMapping (CipherId[Index]);
    if (Mapping == NULL) {
      DEBUG ((
        DEBUG_VERBOSE,
        "%a:%a: skipping CipherId=0x%04x\n",
        gEfiCallerBaseName,
        __FUNCTION__,
        CipherId[Index]
        ));
      //
      // Skipping the cipher is valid because CipherId is an ordered
      // preference list of ciphers, thus we can filter it as long as we
      // don't change the relative order of elements on it.
      //
      continue;
    }

    //
    // Accumulate Mapping->OpensslCipherLength into CipherStringSize. If this
    // is not the first successful mapping, account for a colon (":") prefix
    // too.
    //
    if (MappedCipherCount > 0) {
      Status = SafeUintnAdd (CipherStringSize, 1, &CipherStringSize);
      if (EFI_ERROR (Status)) {
        Status = EFI_OUT_OF_RESOURCES;
        goto FreeMappedCipher;
      }
    }

    Status = SafeUintnAdd (
               CipherStringSize,
               Mapping->OpensslCipherLength,
               &CipherStringSize
               );
    if (EFI_ERROR (Status)) {
      Status = EFI_OUT_OF_RESOURCES;
      goto FreeMappedCipher;
    }

    //
    // Record the mapping.
    //
    MappedCipher[MappedCipherCount++] = Mapping;
  }

  //
  // Verify that at least one IANA cipher ID could be mapped; account for the
  // terminating NUL character in CipherStringSize; allocate CipherString.
  //
  if (MappedCipherCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: no CipherId could be mapped\n",
      gEfiCallerBaseName,
      __FUNCTION__
      ));
    Status = EFI_UNSUPPORTED;
    goto FreeMappedCipher;
  }

  Status = SafeUintnAdd (CipherStringSize, 1, &CipherStringSize);
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeMappedCipher;
  }

  CipherString = AllocatePool (CipherStringSize);
  if (CipherString == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeMappedCipher;
  }

  //
  // Go over the collected mappings and populate CipherString.
  //
  CipherStringPosition = CipherString;
  for (Index = 0; Index < MappedCipherCount; Index++) {
    Mapping = MappedCipher[Index];
    //
    // Append the colon (":") prefix except for the first mapping, then append
    // Mapping->OpensslCipher.
    //
    if (Index > 0) {
      *(CipherStringPosition++) = ':';
    }

    CopyMem (
      CipherStringPosition,
      Mapping->OpensslCipher,
      Mapping->OpensslCipherLength
      );
    CipherStringPosition += Mapping->OpensslCipherLength;
  }

  //
  // NUL-terminate CipherString.
  //
  *(CipherStringPosition++) = '\0';
  ASSERT (CipherStringPosition == CipherString + CipherStringSize);

  //
  // Log CipherString for debugging. CipherString can be very long if the
  // caller provided a large CipherId array, so log CipherString in segments of
  // 79 non-newline characters. (MAX_DEBUG_MESSAGE_LENGTH is usually 0x100 in
  // DebugLib instances.)
  //
  DEBUG_CODE_BEGIN ();
  UINTN  FullLength;
  UINTN  SegmentLength;

  FullLength = CipherStringSize - 1;
  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%a: CipherString={\n",
    gEfiCallerBaseName,
    __FUNCTION__
    ));
  for (CipherStringPosition = CipherString;
       CipherStringPosition < CipherString + FullLength;
       CipherStringPosition += SegmentLength)
  {
    SegmentLength = FullLength - (CipherStringPosition - CipherString);
    if (SegmentLength > 79) {
      SegmentLength = 79;
    }

    DEBUG ((DEBUG_VERBOSE, "%.*a\n", SegmentLength, CipherStringPosition));
  }

  DEBUG ((DEBUG_VERBOSE, "}\n"));
  //
  // Restore the pre-debug value of CipherStringPosition by skipping over the
  // trailing NUL.
  //
  CipherStringPosition++;
  ASSERT (CipherStringPosition == CipherString + CipherStringSize);
  DEBUG_CODE_END ();

  //
  // Sets the ciphers for use by the Tls object.
  //
  if (SSL_set_cipher_list (TlsConn->Ssl, CipherString) <= 0) {
    Status = EFI_UNSUPPORTED;
    goto FreeCipherString;
  }

  Status = EFI_SUCCESS;

FreeCipherString:
  FreePool (CipherString);

FreeMappedCipher:
  FreePool (MappedCipher);

  return Status;
}

/**
  Set the compression method for TLS/SSL operations.

  This function handles TLS/SSL integrated compression methods.

  @param[in]  CompMethod    The compression method ID.

  @retval  EFI_SUCCESS        The compression method for the communication was
                              set successfully.
  @retval  EFI_UNSUPPORTED    Unsupported compression method.

**/
EFI_STATUS
EFIAPI
TlsSetCompressionMethod (
  IN     UINT8  CompMethod
  )
{
  COMP_METHOD  *Cm;
  INTN         Ret;

  Cm  = NULL;
  Ret = 0;

  if (CompMethod == 0) {
    //
    // TLS defines one standard compression method, CompressionMethod.null (0),
    // which specifies that data exchanged via the record protocol will not be compressed.
    // So, return EFI_SUCCESS directly (RFC 3749).
    //
    return EFI_SUCCESS;
  } else if (CompMethod == 1) {
    Cm = COMP_zlib ();
  } else {
    return EFI_UNSUPPORTED;
  }

  //
  // Adds the compression method to the list of available
  // compression methods.
  //
  Ret = SSL_COMP_add_compression_method (CompMethod, Cm);
  if (Ret != 0) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Set peer certificate verification mode for the TLS connection.

  This function sets the verification mode flags for the TLS connection.

  @param[in]  Tls           Pointer to the TLS object.
  @param[in]  VerifyMode    A set of logically or'ed verification mode flags.

**/
VOID
EFIAPI
TlsSetVerify (
  IN     VOID    *Tls,
  IN     UINT32  VerifyMode
  )
{
  TLS_CONNECTION  *TlsConn;

  TlsConn = (TLS_CONNECTION *)Tls;
  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL)) {
    return;
  }

  //
  // Set peer certificate verification parameters with NULL callback.
  //
  SSL_set_verify (TlsConn->Ssl, VerifyMode, NULL);
}

/**
  Set the specified host name to be verified.

  @param[in]  Tls           Pointer to the TLS object.
  @param[in]  Flags         The setting flags during the validation.
  @param[in]  HostName      The specified host name to be verified.

  @retval  EFI_SUCCESS           The HostName setting was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_ABORTED           Invalid HostName setting.

**/
EFI_STATUS
EFIAPI
TlsSetVerifyHost (
  IN     VOID    *Tls,
  IN     UINT32  Flags,
  IN     CHAR8   *HostName
  )
{
  TLS_CONNECTION     *TlsConn;
  X509_VERIFY_PARAM  *VerifyParam;
  UINTN              BinaryAddressSize;
  UINT8              BinaryAddress[MAX (NS_INADDRSZ, NS_IN6ADDRSZ)];
  INTN               ParamStatus;

  TlsConn = (TLS_CONNECTION *)Tls;
  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (HostName == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  SSL_set_hostflags (TlsConn->Ssl, Flags);

  VerifyParam = SSL_get0_param (TlsConn->Ssl);
  ASSERT (VerifyParam != NULL);

  BinaryAddressSize = 0;
  if (inet_pton (AF_INET6, HostName, BinaryAddress) == 1) {
    BinaryAddressSize = NS_IN6ADDRSZ;
  } else if (inet_pton (AF_INET, HostName, BinaryAddress) == 1) {
    BinaryAddressSize = NS_INADDRSZ;
  }

  if (BinaryAddressSize > 0) {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a:%a: parsed \"%a\" as an IPv%c address "
      "literal\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      HostName,
      (UINTN)((BinaryAddressSize == NS_IN6ADDRSZ) ? '6' : '4')
      ));
    ParamStatus = X509_VERIFY_PARAM_set1_ip (
                    VerifyParam,
                    BinaryAddress,
                    BinaryAddressSize
                    );
  } else {
    ParamStatus = X509_VERIFY_PARAM_set1_host (VerifyParam, HostName, 0);
  }

  return (ParamStatus == 1) ? EFI_SUCCESS : EFI_ABORTED;
}

/**
  Sets a TLS/SSL session ID to be used during TLS/SSL connect.

  This function sets a session ID to be used when the TLS/SSL connection is
  to be established.

  @param[in]  Tls             Pointer to the TLS object.
  @param[in]  SessionId       Session ID data used for session resumption.
  @param[in]  SessionIdLen    Length of Session ID in bytes.

  @retval  EFI_SUCCESS           Session ID was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       No available session for ID setting.

**/
EFI_STATUS
EFIAPI
TlsSetSessionId (
  IN     VOID    *Tls,
  IN     UINT8   *SessionId,
  IN     UINT16  SessionIdLen
  )
{
  TLS_CONNECTION  *TlsConn;
  SSL_SESSION     *Session;

  TlsConn = (TLS_CONNECTION *)Tls;
  Session = NULL;

  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (SessionId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Session = SSL_get_session (TlsConn->Ssl);
  if (Session == NULL) {
    return EFI_UNSUPPORTED;
  }

  SSL_SESSION_set1_id (Session, (const unsigned char *)SessionId, SessionIdLen);

  return EFI_SUCCESS;
}

/**
  Adds the CA to the cert store when requesting Server or Client authentication.

  This function adds the CA certificate to the list of CAs when requesting
  Server or Client authentication for the chosen TLS connection.

  @param[in]  Tls         Pointer to the TLS object.
  @param[in]  Data        Pointer to the data buffer of a DER-encoded binary
                          X.509 certificate or PEM-encoded X.509 certificate.
  @param[in]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_INVALID_PARAMETER   The parameter is invalid.
  @retval  EFI_OUT_OF_RESOURCES    Required resources could not be allocated.
  @retval  EFI_ABORTED             Invalid X.509 certificate.

**/
EFI_STATUS
EFIAPI
TlsSetCaCertificate (
  IN     VOID   *Tls,
  IN     VOID   *Data,
  IN     UINTN  DataSize
  )
{
  BIO             *BioCert;
  X509            *Cert;
  X509_STORE      *X509Store;
  EFI_STATUS      Status;
  TLS_CONNECTION  *TlsConn;
  SSL_CTX         *SslCtx;
  INTN            Ret;
  UINTN           ErrorCode;

  BioCert   = NULL;
  Cert      = NULL;
  X509Store = NULL;
  Status    = EFI_SUCCESS;
  TlsConn   = (TLS_CONNECTION *)Tls;
  Ret       = 0;

  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (Data == NULL) || (DataSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // DER-encoded binary X.509 certificate or PEM-encoded X.509 certificate.
  // Determine whether certificate is from DER encoding, if so, translate it to X509 structure.
  //
  Cert = d2i_X509 (NULL, (const unsigned char **)&Data, (long)DataSize);
  if (Cert == NULL) {
    //
    // Certificate is from PEM encoding.
    //
    BioCert = BIO_new (BIO_s_mem ());
    if (BioCert == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    if (BIO_write (BioCert, Data, (UINT32)DataSize) <= 0) {
      Status = EFI_ABORTED;
      goto ON_EXIT;
    }

    Cert = PEM_read_bio_X509 (BioCert, NULL, NULL, NULL);
    if (Cert == NULL) {
      Status = EFI_ABORTED;
      goto ON_EXIT;
    }
  }

  SslCtx    = SSL_get_SSL_CTX (TlsConn->Ssl);
  X509Store = SSL_CTX_get_cert_store (SslCtx);
  if (X509Store == NULL) {
    Status = EFI_ABORTED;
    goto ON_EXIT;
  }

  //
  // Add certificate to X509 store
  //
  Ret = X509_STORE_add_cert (X509Store, Cert);
  if (Ret != 1) {
    ErrorCode = ERR_peek_last_error ();
    //
    // Ignore "already in table" errors
    //
    if (!((ERR_GET_FUNC (ErrorCode) == X509_F_X509_STORE_ADD_CERT) &&
          (ERR_GET_REASON (ErrorCode) == X509_R_CERT_ALREADY_IN_HASH_TABLE)))
    {
      Status = EFI_ABORTED;
      goto ON_EXIT;
    }
  }

ON_EXIT:
  if (BioCert != NULL) {
    BIO_free (BioCert);
  }

  if (Cert != NULL) {
    X509_free (Cert);
  }

  return Status;
}

/**
  Loads the local public certificate into the specified TLS object.

  This function loads the X.509 certificate into the specified TLS object
  for TLS negotiation.

  @param[in]  Tls         Pointer to the TLS object.
  @param[in]  Data        Pointer to the data buffer of a DER-encoded binary
                          X.509 certificate or PEM-encoded X.509 certificate.
  @param[in]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_INVALID_PARAMETER   The parameter is invalid.
  @retval  EFI_OUT_OF_RESOURCES    Required resources could not be allocated.
  @retval  EFI_ABORTED             Invalid X.509 certificate.

**/
EFI_STATUS
EFIAPI
TlsSetHostPublicCert (
  IN     VOID   *Tls,
  IN     VOID   *Data,
  IN     UINTN  DataSize
  )
{
  BIO             *BioCert;
  X509            *Cert;
  EFI_STATUS      Status;
  TLS_CONNECTION  *TlsConn;

  BioCert = NULL;
  Cert    = NULL;
  Status  = EFI_SUCCESS;
  TlsConn = (TLS_CONNECTION *)Tls;

  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (Data == NULL) || (DataSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // DER-encoded binary X.509 certificate or PEM-encoded X.509 certificate.
  // Determine whether certificate is from DER encoding, if so, translate it to X509 structure.
  //
  Cert = d2i_X509 (NULL, (const unsigned char **)&Data, (long)DataSize);
  if (Cert == NULL) {
    //
    // Certificate is from PEM encoding.
    //
    BioCert = BIO_new (BIO_s_mem ());
    if (BioCert == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    if (BIO_write (BioCert, Data, (UINT32)DataSize) <= 0) {
      Status = EFI_ABORTED;
      goto ON_EXIT;
    }

    Cert = PEM_read_bio_X509 (BioCert, NULL, NULL, NULL);
    if (Cert == NULL) {
      Status = EFI_ABORTED;
      goto ON_EXIT;
    }
  }

  if (SSL_use_certificate (TlsConn->Ssl, Cert) != 1) {
    Status = EFI_ABORTED;
    goto ON_EXIT;
  }

ON_EXIT:
  if (BioCert != NULL) {
    BIO_free (BioCert);
  }

  if (Cert != NULL) {
    X509_free (Cert);
  }

  return Status;
}

/**
  Adds the local private key to the specified TLS object.

  This function adds the local private key (PEM-encoded RSA or PKCS#8 private
  key) into the specified TLS object for TLS negotiation.

  @param[in]  Tls         Pointer to the TLS object.
  @param[in]  Data        Pointer to the data buffer of a PEM-encoded RSA
                          or PKCS#8 private key.
  @param[in]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS     The operation succeeded.
  @retval  EFI_UNSUPPORTED This function is not supported.
  @retval  EFI_ABORTED     Invalid private key data.

**/
EFI_STATUS
EFIAPI
TlsSetHostPrivateKey (
  IN     VOID   *Tls,
  IN     VOID   *Data,
  IN     UINTN  DataSize
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Adds the CA-supplied certificate revocation list for certificate validation.

  This function adds the CA-supplied certificate revocation list data for
  certificate validity checking.

  @param[in]  Data        Pointer to the data buffer of a DER-encoded CRL data.
  @param[in]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS     The operation succeeded.
  @retval  EFI_UNSUPPORTED This function is not supported.
  @retval  EFI_ABORTED     Invalid CRL data.

**/
EFI_STATUS
EFIAPI
TlsSetCertRevocationList (
  IN     VOID   *Data,
  IN     UINTN  DataSize
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Gets the protocol version used by the specified TLS connection.

  This function returns the protocol version used by the specified TLS
  connection.

  If Tls is NULL, then ASSERT().

  @param[in]  Tls    Pointer to the TLS object.

  @return  The protocol version of the specified TLS connection.

**/
UINT16
EFIAPI
TlsGetVersion (
  IN     VOID  *Tls
  )
{
  TLS_CONNECTION  *TlsConn;

  TlsConn = (TLS_CONNECTION *)Tls;

  ASSERT (TlsConn != NULL);

  return (UINT16)(SSL_version (TlsConn->Ssl));
}

/**
  Gets the connection end of the specified TLS connection.

  This function returns the connection end (as client or as server) used by
  the specified TLS connection.

  If Tls is NULL, then ASSERT().

  @param[in]  Tls    Pointer to the TLS object.

  @return  The connection end used by the specified TLS connection.

**/
UINT8
EFIAPI
TlsGetConnectionEnd (
  IN     VOID  *Tls
  )
{
  TLS_CONNECTION  *TlsConn;

  TlsConn = (TLS_CONNECTION *)Tls;

  ASSERT (TlsConn != NULL);

  return (UINT8)SSL_is_server (TlsConn->Ssl);
}

/**
  Gets the cipher suite used by the specified TLS connection.

  This function returns current cipher suite used by the specified
  TLS connection.

  @param[in]      Tls         Pointer to the TLS object.
  @param[in,out]  CipherId    The cipher suite used by the TLS object.

  @retval  EFI_SUCCESS           The cipher suite was returned successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       Unsupported cipher suite.

**/
EFI_STATUS
EFIAPI
TlsGetCurrentCipher (
  IN     VOID    *Tls,
  IN OUT UINT16  *CipherId
  )
{
  TLS_CONNECTION    *TlsConn;
  CONST SSL_CIPHER  *Cipher;

  TlsConn = (TLS_CONNECTION *)Tls;
  Cipher  = NULL;

  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (CipherId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Cipher = SSL_get_current_cipher (TlsConn->Ssl);
  if (Cipher == NULL) {
    return EFI_UNSUPPORTED;
  }

  *CipherId = (SSL_CIPHER_get_id (Cipher)) & 0xFFFF;

  return EFI_SUCCESS;
}

/**
  Gets the compression methods used by the specified TLS connection.

  This function returns current integrated compression methods used by
  the specified TLS connection.

  @param[in]      Tls              Pointer to the TLS object.
  @param[in,out]  CompressionId    The current compression method used by
                                   the TLS object.

  @retval  EFI_SUCCESS           The compression method was returned successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_ABORTED           Invalid Compression method.
  @retval  EFI_UNSUPPORTED       This function is not supported.

**/
EFI_STATUS
EFIAPI
TlsGetCurrentCompressionId (
  IN     VOID   *Tls,
  IN OUT UINT8  *CompressionId
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Gets the verification mode currently set in the TLS connection.

  This function returns the peer verification mode currently set in the
  specified TLS connection.

  If Tls is NULL, then ASSERT().

  @param[in]  Tls    Pointer to the TLS object.

  @return  The verification mode set in the specified TLS connection.

**/
UINT32
EFIAPI
TlsGetVerify (
  IN     VOID  *Tls
  )
{
  TLS_CONNECTION  *TlsConn;

  TlsConn = (TLS_CONNECTION *)Tls;

  ASSERT (TlsConn != NULL);

  return SSL_get_verify_mode (TlsConn->Ssl);
}

/**
  Gets the session ID used by the specified TLS connection.

  This function returns the TLS/SSL session ID currently used by the
  specified TLS connection.

  @param[in]      Tls             Pointer to the TLS object.
  @param[in,out]  SessionId       Buffer to contain the returned session ID.
  @param[in,out]  SessionIdLen    The length of Session ID in bytes.

  @retval  EFI_SUCCESS           The Session ID was returned successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       Invalid TLS/SSL session.

**/
EFI_STATUS
EFIAPI
TlsGetSessionId (
  IN     VOID    *Tls,
  IN OUT UINT8   *SessionId,
  IN OUT UINT16  *SessionIdLen
  )
{
  TLS_CONNECTION  *TlsConn;
  SSL_SESSION     *Session;
  CONST UINT8     *SslSessionId;

  TlsConn = (TLS_CONNECTION *)Tls;
  Session = NULL;

  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (SessionId == NULL) || (SessionIdLen == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Session = SSL_get_session (TlsConn->Ssl);
  if (Session == NULL) {
    return EFI_UNSUPPORTED;
  }

  SslSessionId = SSL_SESSION_get_id (Session, (unsigned int *)SessionIdLen);
  CopyMem (SessionId, SslSessionId, *SessionIdLen);

  return EFI_SUCCESS;
}

/**
  Gets the client random data used in the specified TLS connection.

  This function returns the TLS/SSL client random data currently used in
  the specified TLS connection.

  @param[in]      Tls             Pointer to the TLS object.
  @param[in,out]  ClientRandom    Buffer to contain the returned client
                                  random data (32 bytes).

**/
VOID
EFIAPI
TlsGetClientRandom (
  IN     VOID   *Tls,
  IN OUT UINT8  *ClientRandom
  )
{
  TLS_CONNECTION  *TlsConn;

  TlsConn = (TLS_CONNECTION *)Tls;

  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (ClientRandom == NULL)) {
    return;
  }

  SSL_get_client_random (TlsConn->Ssl, ClientRandom, SSL3_RANDOM_SIZE);
}

/**
  Gets the server random data used in the specified TLS connection.

  This function returns the TLS/SSL server random data currently used in
  the specified TLS connection.

  @param[in]      Tls             Pointer to the TLS object.
  @param[in,out]  ServerRandom    Buffer to contain the returned server
                                  random data (32 bytes).

**/
VOID
EFIAPI
TlsGetServerRandom (
  IN     VOID   *Tls,
  IN OUT UINT8  *ServerRandom
  )
{
  TLS_CONNECTION  *TlsConn;

  TlsConn = (TLS_CONNECTION *)Tls;

  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (ServerRandom == NULL)) {
    return;
  }

  SSL_get_server_random (TlsConn->Ssl, ServerRandom, SSL3_RANDOM_SIZE);
}

/**
  Gets the master key data used in the specified TLS connection.

  This function returns the TLS/SSL master key material currently used in
  the specified TLS connection.

  @param[in]      Tls            Pointer to the TLS object.
  @param[in,out]  KeyMaterial    Buffer to contain the returned key material.

  @retval  EFI_SUCCESS           Key material was returned successfully.
  @retval  EFI_INVALID_PARAMETER The parameter is invalid.
  @retval  EFI_UNSUPPORTED       Invalid TLS/SSL session.

**/
EFI_STATUS
EFIAPI
TlsGetKeyMaterial (
  IN     VOID   *Tls,
  IN OUT UINT8  *KeyMaterial
  )
{
  TLS_CONNECTION  *TlsConn;
  SSL_SESSION     *Session;

  TlsConn = (TLS_CONNECTION *)Tls;
  Session = NULL;

  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (KeyMaterial == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Session = SSL_get_session (TlsConn->Ssl);

  if (Session == NULL) {
    return EFI_UNSUPPORTED;
  }

  SSL_SESSION_get_master_key (Session, KeyMaterial, SSL3_MASTER_SECRET_SIZE);

  return EFI_SUCCESS;
}

/**
  Gets the CA Certificate from the cert store.

  This function returns the CA certificate for the chosen
  TLS connection.

  @param[in]      Tls         Pointer to the TLS object.
  @param[out]     Data        Pointer to the data buffer to receive the CA
                              certificate data sent to the client.
  @param[in,out]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_UNSUPPORTED         This function is not supported.
  @retval  EFI_BUFFER_TOO_SMALL    The Data is too small to hold the data.

**/
EFI_STATUS
EFIAPI
TlsGetCaCertificate (
  IN     VOID   *Tls,
  OUT    VOID   *Data,
  IN OUT UINTN  *DataSize
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Gets the local public Certificate set in the specified TLS object.

  This function returns the local public certificate which was currently set
  in the specified TLS object.

  @param[in]      Tls         Pointer to the TLS object.
  @param[out]     Data        Pointer to the data buffer to receive the local
                              public certificate.
  @param[in,out]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_INVALID_PARAMETER   The parameter is invalid.
  @retval  EFI_NOT_FOUND           The certificate is not found.
  @retval  EFI_BUFFER_TOO_SMALL    The Data is too small to hold the data.

**/
EFI_STATUS
EFIAPI
TlsGetHostPublicCert (
  IN     VOID   *Tls,
  OUT    VOID   *Data,
  IN OUT UINTN  *DataSize
  )
{
  X509            *Cert;
  TLS_CONNECTION  *TlsConn;

  Cert    = NULL;
  TlsConn = (TLS_CONNECTION *)Tls;

  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (DataSize == NULL) || ((*DataSize != 0) && (Data == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  Cert = SSL_get_certificate (TlsConn->Ssl);
  if (Cert == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Only DER encoding is supported currently.
  //
  if (*DataSize < (UINTN)i2d_X509 (Cert, NULL)) {
    *DataSize = (UINTN)i2d_X509 (Cert, NULL);
    return EFI_BUFFER_TOO_SMALL;
  }

  *DataSize = (UINTN)i2d_X509 (Cert, (unsigned char **)&Data);

  return EFI_SUCCESS;
}

/**
  Gets the local private key set in the specified TLS object.

  This function returns the local private key data which was currently set
  in the specified TLS object.

  @param[in]      Tls         Pointer to the TLS object.
  @param[out]     Data        Pointer to the data buffer to receive the local
                              private key data.
  @param[in,out]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_UNSUPPORTED         This function is not supported.
  @retval  EFI_BUFFER_TOO_SMALL    The Data is too small to hold the data.

**/
EFI_STATUS
EFIAPI
TlsGetHostPrivateKey (
  IN     VOID   *Tls,
  OUT    VOID   *Data,
  IN OUT UINTN  *DataSize
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Gets the CA-supplied certificate revocation list data set in the specified
  TLS object.

  This function returns the CA-supplied certificate revocation list data which
  was currently set in the specified TLS object.

  @param[out]     Data        Pointer to the data buffer to receive the CRL data.
  @param[in,out]  DataSize    The size of data buffer in bytes.

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_UNSUPPORTED         This function is not supported.
  @retval  EFI_BUFFER_TOO_SMALL    The Data is too small to hold the data.

**/
EFI_STATUS
EFIAPI
TlsGetCertRevocationList (
  OUT    VOID   *Data,
  IN OUT UINTN  *DataSize
  )
{
  return EFI_UNSUPPORTED;
}

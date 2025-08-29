/** @file
  SSL/TLS Configuration Library Wrapper Implementation over OpenSSL.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalTlsLib.h"

typedef struct {
  //
  // TLS Algorithm
  //
  UINT8          Algo;
  //
  // TLS Algorithm name
  //
  CONST CHAR8    *Name;
} TLS_ALGO_TO_NAME;

STATIC CONST TLS_ALGO_TO_NAME  TlsHashAlgoToName[] = {
  { TlsHashAlgoNone,   NULL     },
  { TlsHashAlgoMd5,    "MD5"    },
  { TlsHashAlgoSha1,   "SHA1"   },
  { TlsHashAlgoSha224, "SHA224" },
  { TlsHashAlgoSha256, "SHA256" },
  { TlsHashAlgoSha384, "SHA384" },
  { TlsHashAlgoSha512, "SHA512" },
};

STATIC CONST TLS_ALGO_TO_NAME  TlsSignatureAlgoToName[] = {
  { TlsSignatureAlgoAnonymous, NULL    },
  { TlsSignatureAlgoRsa,       "RSA"   },
  { TlsSignatureAlgoDsa,       "DSA"   },
  { TlsSignatureAlgoEcdsa,     "ECDSA" },
};

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
  TLS_CONNECTION    *TlsConn;
  EFI_STATUS        Status;
  CONST SSL_CIPHER  **MappedCipher;
  UINTN             MappedCipherBytes;
  UINTN             MappedCipherCount;
  UINTN             CipherStringSize;
  UINTN             Index;
  INT32             StackIdx;
  CHAR8             *CipherString;
  CHAR8             *CipherStringPosition;

  STACK_OF (SSL_CIPHER)      *OpensslCipherStack;
  CONST SSL_CIPHER  *OpensslCipher;
  CONST CHAR8       *OpensslCipherName;
  UINTN             OpensslCipherNameLength;

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

  OpensslCipherStack = SSL_get_ciphers (TlsConn->Ssl);

  //
  // Map the cipher IDs, and count the number of bytes for the full
  // CipherString.
  //
  MappedCipherCount = 0;
  CipherStringSize  = 0;
  for (Index = 0; OpensslCipherStack != NULL && Index < CipherNum; Index++) {
    //
    // Look up the IANA-to-OpenSSL mapping.
    //
    for (StackIdx = 0; StackIdx < sk_SSL_CIPHER_num (OpensslCipherStack); StackIdx++) {
      OpensslCipher = sk_SSL_CIPHER_value (OpensslCipherStack, StackIdx);
      if (CipherId[Index] == SSL_CIPHER_get_protocol_id (OpensslCipher)) {
        break;
      }
    }

    if (StackIdx == sk_SSL_CIPHER_num (OpensslCipherStack)) {
      DEBUG ((
        DEBUG_VERBOSE,
        "%a:%a: skipping CipherId=0x%04x\n",
        gEfiCallerBaseName,
        __func__,
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
    // Accumulate cipher name string length into CipherStringSize. If this
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
               AsciiStrLen (SSL_CIPHER_get_name (OpensslCipher)),
               &CipherStringSize
               );
    if (EFI_ERROR (Status)) {
      Status = EFI_OUT_OF_RESOURCES;
      goto FreeMappedCipher;
    }

    //
    // Record the mapping.
    //
    MappedCipher[MappedCipherCount++] = OpensslCipher;
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
      __func__
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
    OpensslCipher           = MappedCipher[Index];
    OpensslCipherName       = SSL_CIPHER_get_name (OpensslCipher);
    OpensslCipherNameLength = AsciiStrLen (OpensslCipherName);
    //
    // Append the colon (":") prefix except for the first mapping, then append
    // OpensslCipherName.
    //
    if (Index > 0) {
      *(CipherStringPosition++) = ':';
    }

    CopyMem (
      CipherStringPosition,
      OpensslCipherName,
      OpensslCipherNameLength
      );
    CipherStringPosition += OpensslCipherNameLength;
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
    __func__
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
  FreePool ((VOID *)MappedCipher);

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

  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%a: SNI hostname: %a\n",
    gEfiCallerBaseName,
    __func__,
    HostName
    ));

  if (!SSL_set_tlsext_host_name (TlsConn->Ssl, HostName)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: Could not set hostname %a for SNI\n",
      gEfiCallerBaseName,
      __func__,
      HostName
      ));
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
      __func__,
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
  Callback function to get the server name.

  @param[in]  SSL
  @param[in]  INT32
  @param[in]  Arg

  @retval  INT32
**/
STATIC
INT32
SslServerNameCallback (
  SSL    *Ssl,
  INT32  *Ad,
  VOID   *Arg
  )
{
  const CHAR8  *HostName = NULL;
  TLS_EXT_CTX  *TlsCtx   = (TLS_EXT_CTX *)Arg;

  HostName = SSL_get_servername (Ssl, TLSEXT_NAMETYPE_host_name);

  if (SSL_get_servername_type (Ssl) != -1) {
    TlsCtx->Ack = !SSL_session_reused (Ssl) && HostName != NULL;
  }

  return SSL_TLSEXT_ERR_OK;
}

/**
  Set the specified server name in Server/Client.

  @param[in]  Tls           Pointer to the TLS object.
  @param[in]  SslCtx        Pointer to the SSL object.
  @param[in]  HostName      The specified server name to be set.

  @retval  EFI_SUCCESS      The Server Name was set successfully.
  @retval  EFI_UNSUPPORTED  Failed to set the Server Name.
**/
EFI_STATUS
EFIAPI
TlsSetServerName (
  VOID   *Tls,
  VOID   *SslCtx,
  CHAR8  *HostName
  )
{
  SSL_CTX         *Ctx;
  TLS_CONNECTION  *TlsConn;
  UINT32          RetVal;
  TLS_EXT_CTX     *TlsExtCtx = NULL;

  TlsConn = (TLS_CONNECTION *)Tls;
  Ctx     = (SSL_CTX *)SslCtx;

  TlsExtCtx = AllocateZeroPool (sizeof (TLS_EXT_CTX));
  if (TlsExtCtx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  RetVal = SSL_CTX_set_tlsext_servername_callback (Ctx, SslServerNameCallback);
  if (!RetVal) {
    return EFI_UNSUPPORTED;
  }

  RetVal = SSL_CTX_set_tlsext_servername_arg (Ctx, TlsExtCtx);
  if (!RetVal) {
    return EFI_UNSUPPORTED;
  }

  RetVal = SSL_set_tlsext_host_name (TlsConn->Ssl, HostName);

  if (!RetVal) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
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
    unsigned long  ErrorCode;

    ErrorCode = ERR_peek_last_error ();
    //
    // Ignore "already in table" errors
    //
    if (!((ERR_GET_LIB (ErrorCode) == ERR_LIB_X509) &&
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

  This function adds the local private key (DER-encoded or PEM-encoded or PKCS#8 private
  key) into the specified TLS object for TLS negotiation.

  @param[in]  Tls         Pointer to the TLS object.
  @param[in]  Data        Pointer to the data buffer of a DER-encoded or PEM-encoded
                          or PKCS#8 private key.
  @param[in]  DataSize    The size of data buffer in bytes.
  @param[in]  Password    Pointer to NULL-terminated private key password, set it to NULL
                          if private key not encrypted.

  @retval  EFI_SUCCESS     The operation succeeded.
  @retval  EFI_UNSUPPORTED This function is not supported.
  @retval  EFI_ABORTED     Invalid private key data.

**/
EFI_STATUS
EFIAPI
TlsSetHostPrivateKeyEx (
  IN     VOID   *Tls,
  IN     VOID   *Data,
  IN     UINTN  DataSize,
  IN     VOID   *Password  OPTIONAL
  )
{
  TLS_CONNECTION  *TlsConn;
  BIO             *Bio;
  EVP_PKEY        *Pkey;
  BOOLEAN         Verify;

  TlsConn = (TLS_CONNECTION *)Tls;

  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (Data == NULL) || (DataSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  // Try to parse the private key in DER format or un-encrypted PKC#8
  if (SSL_use_PrivateKey_ASN1 (
        EVP_PKEY_RSA,
        TlsConn->Ssl,
        Data,
        (long)DataSize
        ) == 1)
  {
    goto verify;
  }

  if (SSL_use_PrivateKey_ASN1 (
        EVP_PKEY_DSA,
        TlsConn->Ssl,
        Data,
        (long)DataSize
        ) == 1)
  {
    goto verify;
  }

  if (SSL_use_PrivateKey_ASN1 (
        EVP_PKEY_EC,
        TlsConn->Ssl,
        Data,
        (long)DataSize
        ) == 1)
  {
    goto verify;
  }

  // Try to parse the private key in PEM format or encrypted PKC#8
  Bio = BIO_new_mem_buf (Data, (int)DataSize);
  if (Bio != NULL) {
    Verify = FALSE;
    Pkey   = PEM_read_bio_PrivateKey (Bio, NULL, NULL, Password);
    if ((Pkey != NULL) && (SSL_use_PrivateKey (TlsConn->Ssl, Pkey) == 1)) {
      Verify = TRUE;
    }

    EVP_PKEY_free (Pkey);
    BIO_free (Bio);

    if (Verify) {
      goto verify;
    }
  }

  return EFI_ABORTED;

verify:
  if (SSL_check_private_key (TlsConn->Ssl) == 1) {
    return EFI_SUCCESS;
  }

  return EFI_ABORTED;
}

/**
  Adds the local private key to the specified TLS object.

  This function adds the local private key (DER-encoded or PEM-encoded or PKCS#8 private
  key) into the specified TLS object for TLS negotiation.

  @param[in]  Tls         Pointer to the TLS object.
  @param[in]  Data        Pointer to the data buffer of a DER-encoded or PEM-encoded
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
  return TlsSetHostPrivateKeyEx (Tls, Data, DataSize, NULL);
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
  Set the signature algorithm list to used by the TLS object.

  This function sets the signature algorithms for use by a specified TLS object.

  @param[in]  Tls                Pointer to a TLS object.
  @param[in]  Data               Array of UINT8 of signature algorithms. The array consists of
                                 pairs of the hash algorithm and the signature algorithm as defined
                                 in RFC 5246
  @param[in]  DataSize           The length the SignatureAlgoList. Must be divisible by 2.

  @retval  EFI_SUCCESS           The signature algorithm list was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameters are invalid.
  @retval  EFI_UNSUPPORTED       No supported TLS signature algorithm was found in SignatureAlgoList
  @retval  EFI_OUT_OF_RESOURCES  Memory allocation failed.

**/
EFI_STATUS
EFIAPI
TlsSetSignatureAlgoList (
  IN     VOID   *Tls,
  IN     UINT8  *Data,
  IN     UINTN  DataSize
  )
{
  TLS_CONNECTION  *TlsConn;
  UINTN           Index;
  UINTN           SignAlgoStrSize;
  CHAR8           *SignAlgoStr;
  CHAR8           *Pos;
  UINT8           *SignatureAlgoList;
  EFI_STATUS      Status;

  TlsConn = (TLS_CONNECTION *)Tls;

  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (Data == NULL) || (DataSize < 3) ||
      ((DataSize % 2) == 0) || (Data[0] != DataSize - 1))
  {
    return EFI_INVALID_PARAMETER;
  }

  SignatureAlgoList = Data + 1;
  SignAlgoStrSize   = 0;
  for (Index = 0; Index < Data[0]; Index += 2) {
    CONST CHAR8  *Tmp;

    if (SignatureAlgoList[Index] >= ARRAY_SIZE (TlsHashAlgoToName)) {
      return EFI_INVALID_PARAMETER;
    }

    Tmp = TlsHashAlgoToName[SignatureAlgoList[Index]].Name;
    if (!Tmp) {
      return EFI_INVALID_PARAMETER;
    }

    // Add 1 for the '+'
    SignAlgoStrSize += AsciiStrLen (Tmp) + 1;

    if (SignatureAlgoList[Index + 1] >= ARRAY_SIZE (TlsSignatureAlgoToName)) {
      return EFI_INVALID_PARAMETER;
    }

    Tmp = TlsSignatureAlgoToName[SignatureAlgoList[Index + 1]].Name;
    if (!Tmp) {
      return EFI_INVALID_PARAMETER;
    }

    // Add 1 for the ':' or for the NULL terminator
    SignAlgoStrSize += AsciiStrLen (Tmp) + 1;
  }

  if (!SignAlgoStrSize) {
    return EFI_UNSUPPORTED;
  }

  SignAlgoStr = AllocatePool (SignAlgoStrSize);
  if (SignAlgoStr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Pos = SignAlgoStr;
  for (Index = 0; Index < Data[0]; Index += 2) {
    CONST CHAR8  *Tmp;

    Tmp = TlsHashAlgoToName[SignatureAlgoList[Index]].Name;
    CopyMem (Pos, Tmp, AsciiStrLen (Tmp));
    Pos   += AsciiStrLen (Tmp);
    *Pos++ = '+';

    Tmp = TlsSignatureAlgoToName[SignatureAlgoList[Index + 1]].Name;
    CopyMem (Pos, Tmp, AsciiStrLen (Tmp));
    Pos   += AsciiStrLen (Tmp);
    *Pos++ = ':';
  }

  *(Pos - 1) = '\0';

  if (SSL_set1_sigalgs_list (TlsConn->Ssl, SignAlgoStr) < 1) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    Status = EFI_SUCCESS;
  }

  FreePool (SignAlgoStr);
  return Status;
}

/**
  Set the EC curve to be used for TLS flows

  This function sets the EC curve to be used for TLS flows.

  @param[in]  Tls                Pointer to a TLS object.
  @param[in]  Data               An EC named curve as defined in section 5.1.1 of RFC 4492.
  @param[in]  DataSize           Size of Data, it should be sizeof (UINT32)

  @retval  EFI_SUCCESS           The EC curve was set successfully.
  @retval  EFI_INVALID_PARAMETER The parameters are invalid.
  @retval  EFI_UNSUPPORTED       The requested TLS EC curve is not supported

**/
EFI_STATUS
EFIAPI
TlsSetEcCurve (
  IN     VOID   *Tls,
  IN     UINT8  *Data,
  IN     UINTN  DataSize
  )
{
  TLS_CONNECTION  *TlsConn;
  EC_KEY          *EcKey;
  INT32           Nid;
  INT32           Ret;

  TlsConn = (TLS_CONNECTION *)Tls;

  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL) || (Data == NULL) || (DataSize != sizeof (UINT32))) {
    return EFI_INVALID_PARAMETER;
  }

  switch (*((UINT32 *)Data)) {
    case TlsEcNamedCurveSecp256r1:
      return EFI_UNSUPPORTED;
    case TlsEcNamedCurveSecp384r1:
      Nid = NID_secp384r1;
      break;
    case TlsEcNamedCurveSecp521r1:
      Nid = NID_secp521r1;
      break;
    case TlsEcNamedCurveX25519:
      Nid = NID_X25519;
      break;
    case TlsEcNamedCurveX448:
      Nid = NID_X448;
      break;
    default:
      return EFI_UNSUPPORTED;
  }

  if (SSL_set1_curves (TlsConn->Ssl, &Nid, 1) != 1) {
    return EFI_UNSUPPORTED;
  }

  EcKey = EC_KEY_new_by_curve_name (Nid);
  if (EcKey == NULL) {
    return EFI_UNSUPPORTED;
  }

  Ret = SSL_set_tmp_ecdh (TlsConn->Ssl, EcKey);
  EC_KEY_free (EcKey);

  if (Ret != 1) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
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

/**
  Derive keying material from a TLS connection.

  This function exports keying material using the mechanism described in RFC
  5705.

  @param[in]      Tls          Pointer to the TLS object
  @param[in]      Label        Description of the key for the PRF function
  @param[in]      Context      Optional context
  @param[in]      ContextLen   The length of the context value in bytes
  @param[out]     KeyBuffer    Buffer to hold the output of the TLS-PRF
  @param[in]      KeyBufferLen The length of the KeyBuffer

  @retval  EFI_SUCCESS             The operation succeeded.
  @retval  EFI_INVALID_PARAMETER   The TLS object is invalid.
  @retval  EFI_PROTOCOL_ERROR      Some other error occurred.

**/
EFI_STATUS
EFIAPI
TlsGetExportKey (
  IN     VOID        *Tls,
  IN     CONST VOID  *Label,
  IN     CONST VOID  *Context,
  IN     UINTN       ContextLen,
  OUT    VOID        *KeyBuffer,
  IN     UINTN       KeyBufferLen
  )
{
  TLS_CONNECTION  *TlsConn;

  TlsConn = (TLS_CONNECTION *)Tls;

  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return SSL_export_keying_material (
           TlsConn->Ssl,
           KeyBuffer,
           KeyBufferLen,
           Label,
           AsciiStrLen (Label),
           Context,
           ContextLen,
           Context != NULL
           ) == 1 ?
         EFI_SUCCESS : EFI_PROTOCOL_ERROR;
}

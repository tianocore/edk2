/** @file
  Implementation of EFI TLS Protocol Interfaces.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TlsImpl.h"

EFI_TLS_PROTOCOL  mTlsProtocol = {
  TlsSetSessionData,
  TlsGetSessionData,
  TlsBuildResponsePacket,
  TlsProcessPacket
};

/**
  Set TLS session data.

  The SetSessionData() function set data for a new TLS session. All session data should
  be set before BuildResponsePacket() invoked.

  @param[in]  This                Pointer to the EFI_TLS_PROTOCOL instance.
  @param[in]  DataType            TLS session data type.
  @param[in]  Data                Pointer to session data.
  @param[in]  DataSize            Total size of session data.

  @retval EFI_SUCCESS             The TLS session data is set successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Data is NULL.
                                  DataSize is 0.
                                  DataSize is invalid for DataType.
  @retval EFI_UNSUPPORTED         The DataType is unsupported.
  @retval EFI_ACCESS_DENIED       If the DataType is one of below:
                                  EfiTlsClientRandom
                                  EfiTlsServerRandom
                                  EfiTlsKeyMaterial
  @retval EFI_NOT_READY           Current TLS session state is NOT
                                  EfiTlsSessionStateNotStarted.
  @retval EFI_OUT_OF_RESOURCES    Required system resources could not be allocated.
**/
EFI_STATUS
EFIAPI
TlsSetSessionData (
  IN     EFI_TLS_PROTOCOL           *This,
  IN     EFI_TLS_SESSION_DATA_TYPE  DataType,
  IN     VOID                       *Data,
  IN     UINTN                      DataSize
  )
{
  EFI_STATUS                 Status;
  TLS_INSTANCE               *Instance;
  UINT16                     *CipherId;
  CONST EFI_TLS_CIPHER       *TlsCipherList;
  UINTN                      CipherCount;
  CONST EFI_TLS_VERIFY_HOST  *TlsVerifyHost;
  EFI_TLS_VERIFY             VerifyMethod;
  UINTN                      VerifyMethodSize;
  UINTN                      Index;

  EFI_TPL  OldTpl;

  Status           = EFI_SUCCESS;
  CipherId         = NULL;
  VerifyMethodSize = sizeof (EFI_TLS_VERIFY);

  if ((This == NULL) || (Data == NULL) || (DataSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = TLS_INSTANCE_FROM_PROTOCOL (This);

  if ((DataType != EfiTlsSessionState) && (Instance->TlsSessionState != EfiTlsSessionNotStarted)) {
    Status = EFI_NOT_READY;
    goto ON_EXIT;
  }

  switch (DataType) {
    //
    // Session Configuration
    //
    case EfiTlsVersion:
      if (DataSize != sizeof (EFI_TLS_VERSION)) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      Status = TlsSetVersion (Instance->TlsConn, ((EFI_TLS_VERSION *)Data)->Major, ((EFI_TLS_VERSION *)Data)->Minor);
      break;
    case EfiTlsConnectionEnd:
      if (DataSize != sizeof (EFI_TLS_CONNECTION_END)) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      Status = TlsSetConnectionEnd (Instance->TlsConn, *((EFI_TLS_CONNECTION_END *)Data));
      break;
    case EfiTlsCipherList:
      if (DataSize % sizeof (EFI_TLS_CIPHER) != 0) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      CipherId = AllocatePool (DataSize);
      if (CipherId == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }

      TlsCipherList = (CONST EFI_TLS_CIPHER *)Data;
      CipherCount   = DataSize / sizeof (EFI_TLS_CIPHER);
      for (Index = 0; Index < CipherCount; Index++) {
        CipherId[Index] = ((TlsCipherList[Index].Data1 << 8) |
                           TlsCipherList[Index].Data2);
      }

      Status = TlsSetCipherList (Instance->TlsConn, CipherId, CipherCount);

      FreePool (CipherId);
      break;
    case EfiTlsCompressionMethod:
      //
      // TLS seems only define one CompressionMethod.null, which specifies that data exchanged via the
      // record protocol will not be compressed.
      // More information from OpenSSL: http://www.openssl.org/docs/manmaster/ssl/SSL_COMP_add_compression_method.html
      // The TLS RFC does however not specify compression methods or their corresponding identifiers,
      // so there is currently no compatible way to integrate compression with unknown peers.
      // It is therefore currently not recommended to integrate compression into applications.
      // Applications for non-public use may agree on certain compression methods.
      // Using different compression methods with the same identifier will lead to connection failure.
      //
      for (Index = 0; Index < DataSize / sizeof (EFI_TLS_COMPRESSION); Index++) {
        Status = TlsSetCompressionMethod (*((UINT8 *)Data + Index));
        if (EFI_ERROR (Status)) {
          break;
        }
      }

      break;
    case EfiTlsExtensionData:
      Status = EFI_UNSUPPORTED;
      goto ON_EXIT;
    case EfiTlsVerifyMethod:
      if (DataSize != sizeof (EFI_TLS_VERIFY)) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      TlsSetVerify (Instance->TlsConn, *((UINT32 *)Data));
      break;
    case EfiTlsVerifyHost:
      if (DataSize != sizeof (EFI_TLS_VERIFY_HOST)) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      TlsVerifyHost = (CONST EFI_TLS_VERIFY_HOST *)Data;

      if (((TlsVerifyHost->Flags & EFI_TLS_VERIFY_FLAG_ALWAYS_CHECK_SUBJECT) != 0) &&
          ((TlsVerifyHost->Flags & EFI_TLS_VERIFY_FLAG_NEVER_CHECK_SUBJECT) != 0))
      {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      if (((TlsVerifyHost->Flags & EFI_TLS_VERIFY_FLAG_NO_WILDCARDS) != 0) &&
          (((TlsVerifyHost->Flags & EFI_TLS_VERIFY_FLAG_NO_PARTIAL_WILDCARDS) != 0) ||
           ((TlsVerifyHost->Flags & EFI_TLS_VERIFY_FLAG_MULTI_LABEL_WILDCARDS) != 0)))
      {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      Status = This->GetSessionData (This, EfiTlsVerifyMethod, &VerifyMethod, &VerifyMethodSize);
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

      if ((VerifyMethod & EFI_TLS_VERIFY_PEER) == 0) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      Status = TlsSetVerifyHost (Instance->TlsConn, TlsVerifyHost->Flags, TlsVerifyHost->HostName);
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

      Status = TlsSetServerName (Instance->TlsConn, Instance->Service->TlsCtx, TlsVerifyHost->HostName);
      break;
    case EfiTlsSessionID:
      if (DataSize != sizeof (EFI_TLS_SESSION_ID)) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      Status = TlsSetSessionId (
                 Instance->TlsConn,
                 ((EFI_TLS_SESSION_ID *)Data)->Data,
                 ((EFI_TLS_SESSION_ID *)Data)->Length
                 );
      break;
    case EfiTlsSessionState:
      if (DataSize != sizeof (EFI_TLS_SESSION_STATE)) {
        Status = EFI_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      Instance->TlsSessionState = *(EFI_TLS_SESSION_STATE *)Data;
      break;
    //
    // Session information
    //
    case EfiTlsClientRandom:
      Status = EFI_ACCESS_DENIED;
      break;
    case EfiTlsServerRandom:
      Status = EFI_ACCESS_DENIED;
      break;
    case EfiTlsKeyMaterial:
      Status = EFI_ACCESS_DENIED;
      break;
    //
    // Unsupported type.
    //
    default:
      Status = EFI_UNSUPPORTED;
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Get TLS session data.

  The GetSessionData() function return the TLS session information.

  @param[in]       This           Pointer to the EFI_TLS_PROTOCOL instance.
  @param[in]       DataType       TLS session data type.
  @param[in, out]  Data           Pointer to session data.
  @param[in, out]  DataSize       Total size of session data. On input, it means
                                  the size of Data buffer. On output, it means the size
                                  of copied Data buffer if EFI_SUCCESS, and means the
                                  size of desired Data buffer if EFI_BUFFER_TOO_SMALL.

  @retval EFI_SUCCESS             The TLS session data is got successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  DataSize is NULL.
                                  Data is NULL if *DataSize is not zero.
  @retval EFI_UNSUPPORTED         The DataType is unsupported.
  @retval EFI_NOT_FOUND           The TLS session data is not found.
  @retval EFI_NOT_READY           The DataType is not ready in current session state.
  @retval EFI_BUFFER_TOO_SMALL    The buffer is too small to hold the data.
**/
EFI_STATUS
EFIAPI
TlsGetSessionData (
  IN     EFI_TLS_PROTOCOL           *This,
  IN     EFI_TLS_SESSION_DATA_TYPE  DataType,
  IN OUT VOID                       *Data   OPTIONAL,
  IN OUT UINTN                      *DataSize
  )
{
  EFI_STATUS    Status;
  TLS_INSTANCE  *Instance;

  EFI_TPL  OldTpl;

  Status = EFI_SUCCESS;

  if ((This == NULL) || (DataSize == NULL) || ((Data == NULL) && (*DataSize != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = TLS_INSTANCE_FROM_PROTOCOL (This);

  if ((Instance->TlsSessionState == EfiTlsSessionNotStarted) &&
      ((DataType == EfiTlsSessionID) || (DataType == EfiTlsClientRandom) ||
       (DataType == EfiTlsServerRandom) || (DataType == EfiTlsKeyMaterial)))
  {
    Status = EFI_NOT_READY;
    goto ON_EXIT;
  }

  switch (DataType) {
    case EfiTlsVersion:
      if (*DataSize < sizeof (EFI_TLS_VERSION)) {
        *DataSize = sizeof (EFI_TLS_VERSION);
        Status    = EFI_BUFFER_TOO_SMALL;
        goto ON_EXIT;
      }

      *DataSize         = sizeof (EFI_TLS_VERSION);
      *((UINT16 *)Data) = HTONS (TlsGetVersion (Instance->TlsConn));
      break;
    case EfiTlsConnectionEnd:
      if (*DataSize < sizeof (EFI_TLS_CONNECTION_END)) {
        *DataSize = sizeof (EFI_TLS_CONNECTION_END);
        Status    = EFI_BUFFER_TOO_SMALL;
        goto ON_EXIT;
      }

      *DataSize        = sizeof (EFI_TLS_CONNECTION_END);
      *((UINT8 *)Data) = TlsGetConnectionEnd (Instance->TlsConn);
      break;
    case EfiTlsCipherList:
      //
      // Get the current session cipher suite.
      //
      if (*DataSize < sizeof (EFI_TLS_CIPHER)) {
        *DataSize = sizeof (EFI_TLS_CIPHER);
        Status    = EFI_BUFFER_TOO_SMALL;
        goto ON_EXIT;
      }

      *DataSize         = sizeof (EFI_TLS_CIPHER);
      Status            = TlsGetCurrentCipher (Instance->TlsConn, (UINT16 *)Data);
      *((UINT16 *)Data) = HTONS (*((UINT16 *)Data));
      break;
    case EfiTlsCompressionMethod:
      //
      // Get the current session compression method.
      //
      if (*DataSize < sizeof (EFI_TLS_COMPRESSION)) {
        *DataSize = sizeof (EFI_TLS_COMPRESSION);
        Status    = EFI_BUFFER_TOO_SMALL;
        goto ON_EXIT;
      }

      *DataSize = sizeof (EFI_TLS_COMPRESSION);
      Status    = TlsGetCurrentCompressionId (Instance->TlsConn, (UINT8 *)Data);
      break;
    case EfiTlsExtensionData:
      Status = EFI_UNSUPPORTED;
      goto ON_EXIT;
    case EfiTlsVerifyMethod:
      if (*DataSize < sizeof (EFI_TLS_VERIFY)) {
        *DataSize = sizeof (EFI_TLS_VERIFY);
        Status    = EFI_BUFFER_TOO_SMALL;
        goto ON_EXIT;
      }

      *DataSize         = sizeof (EFI_TLS_VERIFY);
      *((UINT32 *)Data) = TlsGetVerify (Instance->TlsConn);
      break;
    case EfiTlsSessionID:
      if (*DataSize < sizeof (EFI_TLS_SESSION_ID)) {
        *DataSize = sizeof (EFI_TLS_SESSION_ID);
        Status    = EFI_BUFFER_TOO_SMALL;
        goto ON_EXIT;
      }

      *DataSize = sizeof (EFI_TLS_SESSION_ID);
      Status    = TlsGetSessionId (
                    Instance->TlsConn,
                    ((EFI_TLS_SESSION_ID *)Data)->Data,
                    &(((EFI_TLS_SESSION_ID *)Data)->Length)
                    );
      break;
    case EfiTlsSessionState:
      if (*DataSize < sizeof (EFI_TLS_SESSION_STATE)) {
        *DataSize = sizeof (EFI_TLS_SESSION_STATE);
        Status    = EFI_BUFFER_TOO_SMALL;
        goto ON_EXIT;
      }

      *DataSize = sizeof (EFI_TLS_SESSION_STATE);
      CopyMem (Data, &Instance->TlsSessionState, *DataSize);
      break;
    case EfiTlsClientRandom:
      if (*DataSize < sizeof (EFI_TLS_RANDOM)) {
        *DataSize = sizeof (EFI_TLS_RANDOM);
        Status    = EFI_BUFFER_TOO_SMALL;
        goto ON_EXIT;
      }

      *DataSize = sizeof (EFI_TLS_RANDOM);
      TlsGetClientRandom (Instance->TlsConn, (UINT8 *)Data);
      break;
    case EfiTlsServerRandom:
      if (*DataSize < sizeof (EFI_TLS_RANDOM)) {
        *DataSize = sizeof (EFI_TLS_RANDOM);
        Status    = EFI_BUFFER_TOO_SMALL;
        goto ON_EXIT;
      }

      *DataSize = sizeof (EFI_TLS_RANDOM);
      TlsGetServerRandom (Instance->TlsConn, (UINT8 *)Data);
      break;
    case EfiTlsKeyMaterial:
      if (*DataSize < sizeof (EFI_TLS_MASTER_SECRET)) {
        *DataSize = sizeof (EFI_TLS_MASTER_SECRET);
        Status    = EFI_BUFFER_TOO_SMALL;
        goto ON_EXIT;
      }

      *DataSize = sizeof (EFI_TLS_MASTER_SECRET);
      Status    = TlsGetKeyMaterial (Instance->TlsConn, (UINT8 *)Data);
      break;
    //
    // Unsupported type.
    //
    default:
      Status = EFI_UNSUPPORTED;
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Build response packet according to TLS state machine. This function is only valid for
  alert, handshake and change_cipher_spec content type.

  The BuildResponsePacket() function builds TLS response packet in response to the TLS
  request packet specified by RequestBuffer and RequestSize. If RequestBuffer is NULL and
  RequestSize is 0, and TLS session status is EfiTlsSessionNotStarted, the TLS session
  will be initiated and the response packet needs to be ClientHello. If RequestBuffer is
  NULL and RequestSize is 0, and TLS session status is EfiTlsSessionClosing, the TLS
  session will be closed and response packet needs to be CloseNotify. If RequestBuffer is
  NULL and RequestSize is 0, and TLS session status is EfiTlsSessionError, the TLS
  session has errors and the response packet needs to be Alert message based on error
  type.

  @param[in]       This           Pointer to the EFI_TLS_PROTOCOL instance.
  @param[in]       RequestBuffer  Pointer to the most recently received TLS packet. NULL
                                  means TLS need initiate the TLS session and response
                                  packet need to be ClientHello.
  @param[in]       RequestSize    Packet size in bytes for the most recently received TLS
                                  packet. 0 is only valid when RequestBuffer is NULL.
  @param[out]      Buffer         Pointer to the buffer to hold the built packet.
  @param[in, out]  BufferSize     Pointer to the buffer size in bytes. On input, it is
                                  the buffer size provided by the caller. On output, it
                                  is the buffer size in fact needed to contain the
                                  packet.

  @retval EFI_SUCCESS             The required TLS packet is built successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  RequestBuffer is NULL but RequestSize is NOT 0.
                                  RequestSize is 0 but RequestBuffer is NOT NULL.
                                  BufferSize is NULL.
                                  Buffer is NULL if *BufferSize is not zero.
  @retval EFI_BUFFER_TOO_SMALL    BufferSize is too small to hold the response packet.
  @retval EFI_NOT_READY           Current TLS session state is NOT ready to build
                                  ResponsePacket.
  @retval EFI_ABORTED             Something wrong build response packet.
**/
EFI_STATUS
EFIAPI
TlsBuildResponsePacket (
  IN     EFI_TLS_PROTOCOL  *This,
  IN     UINT8             *RequestBuffer  OPTIONAL,
  IN     UINTN             RequestSize  OPTIONAL,
  OUT UINT8                *Buffer  OPTIONAL,
  IN OUT UINTN             *BufferSize
  )
{
  EFI_STATUS    Status;
  TLS_INSTANCE  *Instance;
  EFI_TPL       OldTpl;

  Status = EFI_SUCCESS;

  if ((This == NULL) || (BufferSize == NULL) ||
      ((RequestBuffer == NULL) && (RequestSize != 0)) ||
      ((RequestBuffer != NULL) && (RequestSize == 0)) ||
      ((Buffer == NULL) && (*BufferSize != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = TLS_INSTANCE_FROM_PROTOCOL (This);

  if ((RequestBuffer == NULL) && (RequestSize == 0)) {
    switch (Instance->TlsSessionState) {
      case EfiTlsSessionNotStarted:
        //
        // ClientHello.
        //
        Status = TlsDoHandshake (
                   Instance->TlsConn,
                   NULL,
                   0,
                   Buffer,
                   BufferSize
                   );
        if (EFI_ERROR (Status)) {
          goto ON_EXIT;
        }

        //
        // *BufferSize should not be zero when ClientHello.
        //
        if (*BufferSize == 0) {
          Status = EFI_ABORTED;
          goto ON_EXIT;
        }

        Instance->TlsSessionState = EfiTlsSessionHandShaking;

        break;
      case EfiTlsSessionClosing:
        //
        // TLS session will be closed and response packet needs to be CloseNotify.
        //
        Status = TlsCloseNotify (
                   Instance->TlsConn,
                   Buffer,
                   BufferSize
                   );
        if (EFI_ERROR (Status)) {
          goto ON_EXIT;
        }

        //
        // *BufferSize should not be zero when build CloseNotify message.
        //
        if (*BufferSize == 0) {
          Status = EFI_ABORTED;
          goto ON_EXIT;
        }

        break;
      case EfiTlsSessionError:
        //
        // TLS session has errors and the response packet needs to be Alert
        // message based on error type.
        //
        Status = TlsHandleAlert (
                   Instance->TlsConn,
                   NULL,
                   0,
                   Buffer,
                   BufferSize
                   );
        if (EFI_ERROR (Status)) {
          goto ON_EXIT;
        }

        break;
      default:
        //
        // Current TLS session state is NOT ready to build ResponsePacket.
        //
        Status = EFI_NOT_READY;
    }
  } else {
    //
    // 1. Received packet may have multiple TLS record messages.
    // 2. One TLS record message may have multiple handshake protocol.
    // 3. Some errors may be happened in handshake.
    // TlsDoHandshake() can handle all of those cases.
    //
    if (TlsInHandshake (Instance->TlsConn)) {
      Status = TlsDoHandshake (
                 Instance->TlsConn,
                 RequestBuffer,
                 RequestSize,
                 Buffer,
                 BufferSize
                 );
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

      if (!TlsInHandshake (Instance->TlsConn)) {
        Instance->TlsSessionState = EfiTlsSessionDataTransferring;
      }
    } else {
      //
      // Must be alert message, Decrypt it and build the ResponsePacket.
      //
      ASSERT (((TLS_RECORD_HEADER *)RequestBuffer)->ContentType == TlsContentTypeAlert);

      Status = TlsHandleAlert (
                 Instance->TlsConn,
                 RequestBuffer,
                 RequestSize,
                 Buffer,
                 BufferSize
                 );
      if (EFI_ERROR (Status)) {
        if (Status != EFI_BUFFER_TOO_SMALL) {
          Instance->TlsSessionState = EfiTlsSessionError;
        }

        goto ON_EXIT;
      }
    }
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Decrypt or encrypt TLS packet during session. This function is only valid after
  session connected and for application_data content type.

  The ProcessPacket () function process each inbound or outbound TLS APP packet.

  @param[in]       This           Pointer to the EFI_TLS_PROTOCOL instance.
  @param[in, out]  FragmentTable  Pointer to a list of fragment. The caller will take
                                  responsible to handle the original FragmentTable while
                                  it may be reallocated in TLS driver. If CryptMode is
                                  EfiTlsEncrypt, on input these fragments contain the TLS
                                  header and plain text TLS APP payload; on output these
                                  fragments contain the TLS header and cipher text TLS
                                  APP payload. If CryptMode is EfiTlsDecrypt, on input
                                  these fragments contain the TLS header and cipher text
                                  TLS APP payload; on output these fragments contain the
                                  TLS header and plain text TLS APP payload.
  @param[in]       FragmentCount  Number of fragment.
  @param[in]       CryptMode      Crypt mode.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  FragmentTable is NULL.
                                  FragmentCount is NULL.
                                  CryptoMode is invalid.
  @retval EFI_NOT_READY           Current TLS session state is NOT
                                  EfiTlsSessionDataTransferring.
  @retval EFI_ABORTED             Something wrong decryption the message. TLS session
                                  status will become EfiTlsSessionError. The caller need
                                  call BuildResponsePacket() to generate Error Alert
                                  message and send it out.
  @retval EFI_OUT_OF_RESOURCES    No enough resource to finish the operation.
**/
EFI_STATUS
EFIAPI
TlsProcessPacket (
  IN     EFI_TLS_PROTOCOL       *This,
  IN OUT EFI_TLS_FRAGMENT_DATA  **FragmentTable,
  IN     UINT32                 *FragmentCount,
  IN     EFI_TLS_CRYPT_MODE     CryptMode
  )
{
  EFI_STATUS    Status;
  TLS_INSTANCE  *Instance;

  EFI_TPL  OldTpl;

  Status = EFI_SUCCESS;

  if ((This == NULL) || (FragmentTable == NULL) || (FragmentCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = TLS_INSTANCE_FROM_PROTOCOL (This);

  if (Instance->TlsSessionState != EfiTlsSessionDataTransferring) {
    Status = EFI_NOT_READY;
    goto ON_EXIT;
  }

  //
  // Packet sent or received may have multiple TLS record messages (Application data type).
  // So,on input these fragments contain the TLS header and TLS APP payload;
  // on output these fragments also contain the TLS header and TLS APP payload.
  //
  switch (CryptMode) {
    case EfiTlsEncrypt:
      Status = TlsEncryptPacket (Instance, FragmentTable, FragmentCount);
      break;
    case EfiTlsDecrypt:
      Status = TlsDecryptPacket (Instance, FragmentTable, FragmentCount);
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

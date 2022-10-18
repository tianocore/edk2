/** @file
  SSL/TLS Process Library Wrapper Implementation over OpenSSL.
  The process includes the TLS handshake and packet I/O.

Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalTlsLib.h"

#define MAX_BUFFER_SIZE  32768

/**
  Checks if the TLS handshake was done.

  This function will check if the specified TLS handshake was done.

  @param[in]  Tls    Pointer to the TLS object for handshake state checking.

  @retval  TRUE     The TLS handshake was done.
  @retval  FALSE    The TLS handshake was not done.

**/
BOOLEAN
EFIAPI
TlsInHandshake (
  IN     VOID  *Tls
  )
{
  TLS_CONNECTION  *TlsConn;

  TlsConn = (TLS_CONNECTION *)Tls;
  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL)) {
    return FALSE;
  }

  //
  // Return the status which indicates if the TLS handshake was done.
  //
  return !SSL_is_init_finished (TlsConn->Ssl);
}

/**
  Perform a TLS/SSL handshake.

  This function will perform a TLS/SSL handshake.

  @param[in]       Tls            Pointer to the TLS object for handshake operation.
  @param[in]       BufferIn       Pointer to the most recently received TLS Handshake packet.
  @param[in]       BufferInSize   Packet size in bytes for the most recently received TLS
                                  Handshake packet.
  @param[out]      BufferOut      Pointer to the buffer to hold the built packet.
  @param[in, out]  BufferOutSize  Pointer to the buffer size in bytes. On input, it is
                                  the buffer size provided by the caller. On output, it
                                  is the buffer size in fact needed to contain the
                                  packet.

  @retval EFI_SUCCESS             The required TLS packet is built successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  Tls is NULL.
                                  BufferIn is NULL but BufferInSize is NOT 0.
                                  BufferInSize is 0 but BufferIn is NOT NULL.
                                  BufferOutSize is NULL.
                                  BufferOut is NULL if *BufferOutSize is not zero.
  @retval EFI_BUFFER_TOO_SMALL    BufferOutSize is too small to hold the response packet.
  @retval EFI_ABORTED             Something wrong during handshake.

**/
EFI_STATUS
EFIAPI
TlsDoHandshake (
  IN     VOID   *Tls,
  IN     UINT8  *BufferIn  OPTIONAL,
  IN     UINTN  BufferInSize  OPTIONAL,
  OUT UINT8     *BufferOut  OPTIONAL,
  IN OUT UINTN  *BufferOutSize
  )
{
  TLS_CONNECTION  *TlsConn;
  UINTN           PendingBufferSize;
  INTN            Ret;
  UINTN           ErrorCode;

  TlsConn           = (TLS_CONNECTION *)Tls;
  PendingBufferSize = 0;
  Ret               = 1;

  if ((TlsConn == NULL) || \
      (TlsConn->Ssl == NULL) || (TlsConn->InBio == NULL) || (TlsConn->OutBio == NULL) || \
      (BufferOutSize == NULL) || \
      ((BufferIn == NULL) && (BufferInSize != 0)) || \
      ((BufferIn != NULL) && (BufferInSize == 0)) || \
      ((BufferOut == NULL) && (*BufferOutSize != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((BufferIn == NULL) && (BufferInSize == 0)) {
    //
    // If RequestBuffer is NULL and RequestSize is 0, and TLS session
    // status is EfiTlsSessionNotStarted, the TLS session will be initiated
    // and the response packet needs to be ClientHello.
    //
    PendingBufferSize = (UINTN)BIO_ctrl_pending (TlsConn->OutBio);
    if (PendingBufferSize == 0) {
      SSL_set_connect_state (TlsConn->Ssl);
      Ret               = SSL_do_handshake (TlsConn->Ssl);
      PendingBufferSize = (UINTN)BIO_ctrl_pending (TlsConn->OutBio);
    }
  } else {
    PendingBufferSize = (UINTN)BIO_ctrl_pending (TlsConn->OutBio);
    if (PendingBufferSize == 0) {
      BIO_write (TlsConn->InBio, BufferIn, (UINT32)BufferInSize);
      Ret               = SSL_do_handshake (TlsConn->Ssl);
      PendingBufferSize = (UINTN)BIO_ctrl_pending (TlsConn->OutBio);
    }
  }

  if (Ret < 1) {
    Ret = SSL_get_error (TlsConn->Ssl, (int)Ret);
    if ((Ret == SSL_ERROR_SSL) ||
        (Ret == SSL_ERROR_SYSCALL) ||
        (Ret == SSL_ERROR_ZERO_RETURN))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a SSL_HANDSHAKE_ERROR State=0x%x SSL_ERROR_%a\n",
        __FUNCTION__,
        SSL_get_state (TlsConn->Ssl),
        Ret == SSL_ERROR_SSL ? "SSL" : Ret == SSL_ERROR_SYSCALL ? "SYSCALL" : "ZERO_RETURN"
        ));
      DEBUG_CODE_BEGIN ();
      while (TRUE) {
        ErrorCode = ERR_get_error ();
        if (ErrorCode == 0) {
          break;
        }

        DEBUG ((
          DEBUG_ERROR,
          "%a ERROR 0x%x=L%x:F%x:R%x\n",
          __FUNCTION__,
          ErrorCode,
          ERR_GET_LIB (ErrorCode),
          ERR_GET_FUNC (ErrorCode),
          ERR_GET_REASON (ErrorCode)
          ));
      }

      DEBUG_CODE_END ();
      return EFI_ABORTED;
    }
  }

  if (PendingBufferSize > *BufferOutSize) {
    *BufferOutSize = PendingBufferSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (PendingBufferSize > 0) {
    *BufferOutSize = BIO_read (TlsConn->OutBio, BufferOut, (UINT32)PendingBufferSize);
  } else {
    *BufferOutSize = 0;
  }

  return EFI_SUCCESS;
}

/**
  Handle Alert message recorded in BufferIn. If BufferIn is NULL and BufferInSize is zero,
  TLS session has errors and the response packet needs to be Alert message based on error type.

  @param[in]       Tls            Pointer to the TLS object for state checking.
  @param[in]       BufferIn       Pointer to the most recently received TLS Alert packet.
  @param[in]       BufferInSize   Packet size in bytes for the most recently received TLS
                                  Alert packet.
  @param[out]      BufferOut      Pointer to the buffer to hold the built packet.
  @param[in, out]  BufferOutSize  Pointer to the buffer size in bytes. On input, it is
                                  the buffer size provided by the caller. On output, it
                                  is the buffer size in fact needed to contain the
                                  packet.

  @retval EFI_SUCCESS             The required TLS packet is built successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  Tls is NULL.
                                  BufferIn is NULL but BufferInSize is NOT 0.
                                  BufferInSize is 0 but BufferIn is NOT NULL.
                                  BufferOutSize is NULL.
                                  BufferOut is NULL if *BufferOutSize is not zero.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_BUFFER_TOO_SMALL    BufferOutSize is too small to hold the response packet.

**/
EFI_STATUS
EFIAPI
TlsHandleAlert (
  IN     VOID   *Tls,
  IN     UINT8  *BufferIn  OPTIONAL,
  IN     UINTN  BufferInSize  OPTIONAL,
  OUT UINT8     *BufferOut  OPTIONAL,
  IN OUT UINTN  *BufferOutSize
  )
{
  TLS_CONNECTION  *TlsConn;
  UINTN           PendingBufferSize;
  UINT8           *TempBuffer;
  INTN            Ret;

  TlsConn           = (TLS_CONNECTION *)Tls;
  PendingBufferSize = 0;
  TempBuffer        = NULL;
  Ret               = 0;

  if ((TlsConn == NULL) || \
      (TlsConn->Ssl == NULL) || (TlsConn->InBio == NULL) || (TlsConn->OutBio == NULL) || \
      (BufferOutSize == NULL) || \
      ((BufferIn == NULL) && (BufferInSize != 0)) || \
      ((BufferIn != NULL) && (BufferInSize == 0)) || \
      ((BufferOut == NULL) && (*BufferOutSize != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  PendingBufferSize = (UINTN)BIO_ctrl_pending (TlsConn->OutBio);
  if ((PendingBufferSize == 0) && (BufferIn != NULL) && (BufferInSize != 0)) {
    Ret = BIO_write (TlsConn->InBio, BufferIn, (UINT32)BufferInSize);
    if (Ret != (INTN)BufferInSize) {
      return EFI_ABORTED;
    }

    TempBuffer = (UINT8 *)OPENSSL_malloc (MAX_BUFFER_SIZE);

    //
    // ssl3_send_alert() will be called in ssl3_read_bytes() function.
    // TempBuffer is invalid since it's a Alert message, so just ignore it.
    //
    SSL_read (TlsConn->Ssl, TempBuffer, MAX_BUFFER_SIZE);

    OPENSSL_free (TempBuffer);

    PendingBufferSize = (UINTN)BIO_ctrl_pending (TlsConn->OutBio);
  }

  if (PendingBufferSize > *BufferOutSize) {
    *BufferOutSize = PendingBufferSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (PendingBufferSize > 0) {
    *BufferOutSize = BIO_read (TlsConn->OutBio, BufferOut, (UINT32)PendingBufferSize);
  } else {
    *BufferOutSize = 0;
  }

  return EFI_SUCCESS;
}

/**
  Build the CloseNotify packet.

  @param[in]       Tls            Pointer to the TLS object for state checking.
  @param[in, out]  Buffer         Pointer to the buffer to hold the built packet.
  @param[in, out]  BufferSize     Pointer to the buffer size in bytes. On input, it is
                                  the buffer size provided by the caller. On output, it
                                  is the buffer size in fact needed to contain the
                                  packet.

  @retval EFI_SUCCESS             The required TLS packet is built successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  Tls is NULL.
                                  BufferSize is NULL.
                                  Buffer is NULL if *BufferSize is not zero.
  @retval EFI_BUFFER_TOO_SMALL    BufferSize is too small to hold the response packet.

**/
EFI_STATUS
EFIAPI
TlsCloseNotify (
  IN     VOID   *Tls,
  IN OUT UINT8  *Buffer,
  IN OUT UINTN  *BufferSize
  )
{
  TLS_CONNECTION  *TlsConn;
  UINTN           PendingBufferSize;

  TlsConn           = (TLS_CONNECTION *)Tls;
  PendingBufferSize = 0;

  if ((TlsConn == NULL) || \
      (TlsConn->Ssl == NULL) || (TlsConn->InBio == NULL) || (TlsConn->OutBio == NULL) || \
      (BufferSize == NULL) || \
      ((Buffer == NULL) && (*BufferSize != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  PendingBufferSize = (UINTN)BIO_ctrl_pending (TlsConn->OutBio);
  if (PendingBufferSize == 0) {
    //
    // ssl3_send_alert() and ssl3_dispatch_alert() function will be called.
    //
    SSL_shutdown (TlsConn->Ssl);
    PendingBufferSize = (UINTN)BIO_ctrl_pending (TlsConn->OutBio);
  }

  if (PendingBufferSize > *BufferSize) {
    *BufferSize = PendingBufferSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (PendingBufferSize > 0) {
    *BufferSize = BIO_read (TlsConn->OutBio, Buffer, (UINT32)PendingBufferSize);
  } else {
    *BufferSize = 0;
  }

  return EFI_SUCCESS;
}

/**
  Attempts to read bytes from one TLS object and places the data in Buffer.

  This function will attempt to read BufferSize bytes from the TLS object
  and places the data in Buffer.

  @param[in]      Tls           Pointer to the TLS object.
  @param[in,out]  Buffer        Pointer to the buffer to store the data.
  @param[in]      BufferSize    The size of Buffer in bytes.

  @retval  >0    The amount of data successfully read from the TLS object.
  @retval  <=0   No data was successfully read.

**/
INTN
EFIAPI
TlsCtrlTrafficOut (
  IN     VOID   *Tls,
  IN OUT VOID   *Buffer,
  IN     UINTN  BufferSize
  )
{
  TLS_CONNECTION  *TlsConn;

  TlsConn = (TLS_CONNECTION *)Tls;
  if ((TlsConn == NULL) || (TlsConn->OutBio == 0)) {
    return -1;
  }

  //
  // Read and return the amount of data from the BIO.
  //
  return BIO_read (TlsConn->OutBio, Buffer, (UINT32)BufferSize);
}

/**
  Attempts to write data from the buffer to TLS object.

  This function will attempt to write BufferSize bytes data from the Buffer
  to the TLS object.

  @param[in]  Tls           Pointer to the TLS object.
  @param[in]  Buffer        Pointer to the data buffer.
  @param[in]  BufferSize    The size of Buffer in bytes.

  @retval  >0    The amount of data successfully written to the TLS object.
  @retval <=0    No data was successfully written.

**/
INTN
EFIAPI
TlsCtrlTrafficIn (
  IN     VOID   *Tls,
  IN     VOID   *Buffer,
  IN     UINTN  BufferSize
  )
{
  TLS_CONNECTION  *TlsConn;

  TlsConn = (TLS_CONNECTION *)Tls;
  if ((TlsConn == NULL) || (TlsConn->InBio == 0)) {
    return -1;
  }

  //
  // Write and return the amount of data to the BIO.
  //
  return BIO_write (TlsConn->InBio, Buffer, (UINT32)BufferSize);
}

/**
  Attempts to read bytes from the specified TLS connection into the buffer.

  This function tries to read BufferSize bytes data from the specified TLS
  connection into the Buffer.

  @param[in]      Tls           Pointer to the TLS connection for data reading.
  @param[in,out]  Buffer        Pointer to the data buffer.
  @param[in]      BufferSize    The size of Buffer in bytes.

  @retval  >0    The read operation was successful, and return value is the
                 number of bytes actually read from the TLS connection.
  @retval  <=0   The read operation was not successful.

**/
INTN
EFIAPI
TlsRead (
  IN     VOID   *Tls,
  IN OUT VOID   *Buffer,
  IN     UINTN  BufferSize
  )
{
  TLS_CONNECTION  *TlsConn;

  TlsConn = (TLS_CONNECTION *)Tls;
  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL)) {
    return -1;
  }

  //
  // Read bytes from the specified TLS connection.
  //
  return SSL_read (TlsConn->Ssl, Buffer, (UINT32)BufferSize);
}

/**
  Attempts to write data to a TLS connection.

  This function tries to write BufferSize bytes data from the Buffer into the
  specified TLS connection.

  @param[in]  Tls           Pointer to the TLS connection for data writing.
  @param[in]  Buffer        Pointer to the data buffer.
  @param[in]  BufferSize    The size of Buffer in bytes.

  @retval  >0    The write operation was successful, and return value is the
                 number of bytes actually written to the TLS connection.
  @retval <=0    The write operation was not successful.

**/
INTN
EFIAPI
TlsWrite (
  IN     VOID   *Tls,
  IN     VOID   *Buffer,
  IN     UINTN  BufferSize
  )
{
  TLS_CONNECTION  *TlsConn;

  TlsConn = (TLS_CONNECTION *)Tls;
  if ((TlsConn == NULL) || (TlsConn->Ssl == NULL)) {
    return -1;
  }

  //
  // Write bytes to the specified TLS connection.
  //
  return SSL_write (TlsConn->Ssl, Buffer, (UINT32)BufferSize);
}

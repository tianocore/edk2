/** @file
  The header files of miscellaneous routines specific to Https for HttpDxe driver.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_HTTPS_SUPPORT_H__
#define __EFI_HTTPS_SUPPORT_H__

#define HTTPS_DEFAULT_PORT  443

#define HTTPS_FLAG  "https://"

/**
  Check whether the Url is from Https.

  @param[in]    Url             The pointer to a HTTP or HTTPS URL string.

  @retval TRUE                  The Url is from HTTPS.
  @retval FALSE                 The Url is from HTTP.

**/
BOOLEAN
IsHttpsUrl (
  IN CHAR8  *Url
  );

/**
  Creates a Tls child handle, open EFI_TLS_PROTOCOL and EFI_TLS_CONFIGURATION_PROTOCOL.

  @param[in]  ImageHandle           The firmware allocated handle for the UEFI image.
  @param[out] TlsSb                 Pointer to the TLS SERVICE_BINDING_PROTOCOL.
  @param[out] TlsProto              Pointer to the EFI_TLS_PROTOCOL instance.
  @param[out] TlsConfiguration      Pointer to the EFI_TLS_CONFIGURATION_PROTOCOL instance.

  @return  The child handle with opened EFI_TLS_PROTOCOL and EFI_TLS_CONFIGURATION_PROTOCOL.

**/
EFI_HANDLE
EFIAPI
TlsCreateChild (
  IN  EFI_HANDLE                      ImageHandle,
  OUT EFI_SERVICE_BINDING_PROTOCOL    **TlsSb,
  OUT EFI_TLS_PROTOCOL                **TlsProto,
  OUT EFI_TLS_CONFIGURATION_PROTOCOL  **TlsConfiguration
  );

/**
  Create event for the TLS receive and transmit tokens which are used to receive and
  transmit TLS related messages.

  @param[in, out]  HttpInstance       Pointer to HTTP_PROTOCOL structure.

  @retval EFI_SUCCESS            The events are created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
EFIAPI
TlsCreateTxRxEvent (
  IN OUT HTTP_PROTOCOL  *HttpInstance
  );

/**
  Close events in the TlsTxToken and TlsRxToken.

  @param[in]  HttpInstance   Pointer to HTTP_PROTOCOL structure.

**/
VOID
EFIAPI
TlsCloseTxRxEvent (
  IN  HTTP_PROTOCOL  *HttpInstance
  );

/**
  Read the TlsCaCertificate variable and configure it.

  @param[in, out]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            TlsCaCertificate is configured.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_NOT_FOUND          Fail to get "TlsCaCertificate" variable.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
TlsConfigCertificate (
  IN OUT HTTP_PROTOCOL  *HttpInstance
  );

/**
  Configure TLS session data.

  @param[in, out]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            TLS session data is configured.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
EFIAPI
TlsConfigureSession (
  IN OUT HTTP_PROTOCOL  *HttpInstance
  );

/**
  Transmit the Packet by processing the associated HTTPS token.

  @param[in, out]   HttpInstance    Pointer to HTTP_PROTOCOL structure.
  @param[in]        Packet          The packet to transmit.

  @retval EFI_SUCCESS            The packet is transmitted.
  @retval EFI_INVALID_PARAMETER  HttpInstance is NULL or Packet is NULL.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TlsCommonTransmit (
  IN OUT HTTP_PROTOCOL  *HttpInstance,
  IN     NET_BUF        *Packet
  );

/**
  Receive the Packet by processing the associated HTTPS token.

  @param[in, out]   HttpInstance    Pointer to HTTP_PROTOCOL structure.
  @param[in]        Packet          The packet to transmit.
  @param[in]        Timeout         The time to wait for connection done.

  @retval EFI_SUCCESS            The Packet is received.
  @retval EFI_INVALID_PARAMETER  HttpInstance is NULL or Packet is NULL.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_TIMEOUT            The operation is time out.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
EFIAPI
TlsCommonReceive (
  IN OUT HTTP_PROTOCOL  *HttpInstance,
  IN     NET_BUF        *Packet,
  IN     EFI_EVENT      Timeout
  );

/**
  Receive one TLS PDU. An TLS PDU contains an TLS record header and its
  corresponding record data. These two parts will be put into two blocks of buffers in the
  net buffer.

  @param[in, out]      HttpInstance    Pointer to HTTP_PROTOCOL structure.
  @param[out]          Pdu             The received TLS PDU.
  @param[in]           Timeout         The time to wait for connection done.

  @retval EFI_SUCCESS          An TLS PDU is received.
  @retval EFI_OUT_OF_RESOURCES Can't allocate memory resources.
  @retval EFI_PROTOCOL_ERROR   An unexpected TLS packet was received.
  @retval Others               Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TlsReceiveOnePdu (
  IN OUT HTTP_PROTOCOL  *HttpInstance,
  OUT NET_BUF           **Pdu,
  IN     EFI_EVENT      Timeout
  );

/**
  Connect one TLS session by finishing the TLS handshake process.

  @param[in]  HttpInstance       The HTTP instance private data.
  @param[in]  Timeout            The time to wait for connection done.

  @retval EFI_SUCCESS            The TLS session is established.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_ABORTED            TLS session state is incorrect.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
EFIAPI
TlsConnectSession (
  IN  HTTP_PROTOCOL  *HttpInstance,
  IN  EFI_EVENT      Timeout
  );

/**
  Close the TLS session and send out the close notification message.

  @param[in]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            The TLS session is closed.
  @retval EFI_INVALID_PARAMETER  HttpInstance is NULL or Packet is NULL.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
EFIAPI
TlsCloseSession (
  IN  HTTP_PROTOCOL  *HttpInstance
  );

/**
  Process one message according to the CryptMode.

  @param[in]           HttpInstance    Pointer to HTTP_PROTOCOL structure.
  @param[in]           Message         Pointer to the message buffer needed to processed.
                                       If ProcessMode is EfiTlsEncrypt, the message contain the TLS
                                       header and plain text TLS APP payload.
                                       If ProcessMode is EfiTlsDecrypt, the message contain the TLS
                                       header and cipher text TLS APP payload.
  @param[in]           MessageSize     Pointer to the message buffer size.
  @param[in]           ProcessMode     Process mode.
  @param[in, out]      Fragment        Only one Fragment returned after the Message is
                                       processed successfully.
                                       If ProcessMode is EfiTlsEncrypt, the fragment contain the TLS
                                       header and cipher text TLS APP payload.
                                       If ProcessMode is EfiTlsDecrypt, the fragment contain the TLS
                                       header and plain text TLS APP payload.

  @retval EFI_SUCCESS          Message is processed successfully.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval Others               Other errors as indicated.

**/
EFI_STATUS
EFIAPI
TlsProcessMessage (
  IN     HTTP_PROTOCOL       *HttpInstance,
  IN     UINT8               *Message,
  IN     UINTN               MessageSize,
  IN     EFI_TLS_CRYPT_MODE  ProcessMode,
  IN OUT NET_FRAGMENT        *Fragment
  );

/**
  Receive one fragment decrypted from one TLS record.

  @param[in]           HttpInstance    Pointer to HTTP_PROTOCOL structure.
  @param[in, out]      Fragment        The received Fragment.
  @param[in]           Timeout         The time to wait for connection done.

  @retval EFI_SUCCESS          One fragment is received.
  @retval EFI_OUT_OF_RESOURCES Can't allocate memory resources.
  @retval EFI_ABORTED          Something wrong decryption the message.
  @retval Others               Other errors as indicated.

**/
EFI_STATUS
EFIAPI
HttpsReceive (
  IN     HTTP_PROTOCOL  *HttpInstance,
  IN OUT NET_FRAGMENT   *Fragment,
  IN     EFI_EVENT      Timeout
  );

#endif

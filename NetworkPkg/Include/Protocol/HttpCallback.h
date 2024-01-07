/** @file
  This file defines the EDKII HTTP Callback Protocol interface.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef EDKII_HTTP_CALLBACK_H_
#define EDKII_HTTP_CALLBACK_H_

#define EDKII_HTTP_CALLBACK_PROTOCOL_GUID \
  { \
    0x611114f1, 0xa37b, 0x4468, {0xa4, 0x36, 0x5b, 0xdd, 0xa1, 0x6a, 0xa2, 0x40} \
  }

typedef struct _EDKII_HTTP_CALLBACK_PROTOCOL EDKII_HTTP_CALLBACK_PROTOCOL;

///
/// EDKII_HTTP_CALLBACK_EVENT
///
typedef enum {
  ///
  /// The Status of DNS Event to retrieve the host address.
  /// EventStatus:
  /// EFI_SUCCESS             Operation succeeded.
  /// EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
  /// EFI_DEVICE_ERROR        An unexpected network error occurred.
  /// Others                  Other errors as indicated.
  ///
  HttpEventDns,

  ///
  /// The Status of Event to initiate a nonblocking TCP connection request.
  /// EventStatus:
  /// EFI_SUCCESS            The connection request is successfully initiated.
  /// EFI_NOT_STARTED        This EFI TCP Protocol instance has not been configured.
  /// EFI_DEVICE_ERROR       An unexpected system or network error occurred.
  /// Others                 Other errors as indicated.
  ///
  HttpEventConnectTcp,

  ///
  /// The Status of Event to connect one TLS session by finishing the TLS handshake process.
  /// EventStatus:
  /// EFI_SUCCESS            The TLS session is established.
  /// EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  /// EFI_ABORTED            TLS session state is incorrect.
  /// Others                 Other error as indicated.
  ///
  HttpEventTlsConnectSession,

  ///
  /// The Status of Event to initialize Http session
  /// EventStatus:
  /// EFI_SUCCESS            The initialization of session is done.
  /// Others                 Other error as indicated.
  ///
  HttpEventInitSession,

  ///
  /// The Status of Event to configure TLS configuration data.
  /// EventStatus:
  /// EFI_SUCCESS            The TLS is configured successfully with the default value.
  /// EFI_INVALID_PARAMETER  One or more input parameters to SetSessionData() is invalid.
  /// EFI_NOT_READY          Current TLS session state is NOT EfiTlsSessionStateNotStarted.
  /// EFI_NOT_FOUND          Fail to get 'HttpTlsCipherList' variable.
  /// Others                 Other error as indicated.
  ///
  HttpEventTlsConfigured
} EDKII_HTTP_CALLBACK_EVENT;

/**
  Callback function that is invoked when HTTP event occurs.

  @param[in]  This                Pointer to the EDKII_HTTP_CALLBACK_PROTOCOL instance.
  @param[in]  Event               The event that occurs in the current state.
  @param[in]  EventStatus         The Status of Event, EFI_SUCCESS or other errors.
**/
typedef
VOID
(EFIAPI *EDKII_HTTP_CALLBACK)(
  IN EDKII_HTTP_CALLBACK_PROTOCOL     *This,
  IN EDKII_HTTP_CALLBACK_EVENT        Event,
  IN EFI_STATUS                       EventStatus
  );

///
/// EFI HTTP Callback Protocol is invoked when HTTP event occurs.
///
struct _EDKII_HTTP_CALLBACK_PROTOCOL {
  EDKII_HTTP_CALLBACK    Callback;
};

extern EFI_GUID  gEdkiiHttpCallbackProtocolGuid;

#endif /* EDKII_HTTP_CALLBACK_H_ */

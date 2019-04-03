/** @file
  Header file of Miscellaneous Routines for TlsDxe driver.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_TLS_IMPL_H__
#define __EFI_TLS_IMPL_H__

//
// Libraries
//
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/NetLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/TlsLib.h>

//
// Consumed Protocols
//
#include <Protocol/Tls.h>
#include <Protocol/TlsConfig.h>

#include <IndustryStandard/Tls1.h>

#include "TlsDriver.h"

//
// Protocol instances
//
extern EFI_SERVICE_BINDING_PROTOCOL    mTlsServiceBinding;
extern EFI_TLS_PROTOCOL                mTlsProtocol;
extern EFI_TLS_CONFIGURATION_PROTOCOL  mTlsConfigurationProtocol;

/**
  Encrypt the message listed in fragment.

  @param[in]       TlsInstance    The pointer to the TLS instance.
  @param[in, out]  FragmentTable  Pointer to a list of fragment.
                                  On input these fragments contain the TLS header and
                                  plain text TLS payload;
                                  On output these fragments contain the TLS header and
                                  cipher text TLS payload.
  @param[in]       FragmentCount  Number of fragment.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES    Can't allocate memory resources.
  @retval EFI_ABORTED             TLS session state is incorrect.
  @retval Others                  Other errors as indicated.
**/
EFI_STATUS
TlsEncryptPacket (
  IN     TLS_INSTANCE                  *TlsInstance,
  IN OUT EFI_TLS_FRAGMENT_DATA         **FragmentTable,
  IN     UINT32                        *FragmentCount
  );

/**
  Decrypt the message listed in fragment.

  @param[in]       TlsInstance    The pointer to the TLS instance.
  @param[in, out]  FragmentTable  Pointer to a list of fragment.
                                  On input these fragments contain the TLS header and
                                  cipher text TLS payload;
                                  On output these fragments contain the TLS header and
                                  plain text TLS payload.
  @param[in]       FragmentCount  Number of fragment.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES    Can't allocate memory resources.
  @retval EFI_ABORTED             TLS session state is incorrect.
  @retval Others                  Other errors as indicated.
**/
EFI_STATUS
TlsDecryptPacket (
  IN     TLS_INSTANCE                  *TlsInstance,
  IN OUT EFI_TLS_FRAGMENT_DATA         **FragmentTable,
  IN     UINT32                        *FragmentCount
  );

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
  IN     EFI_TLS_PROTOCOL              *This,
  IN     EFI_TLS_SESSION_DATA_TYPE     DataType,
  IN     VOID                          *Data,
  IN     UINTN                         DataSize
  );

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
  IN     EFI_TLS_PROTOCOL              *This,
  IN     EFI_TLS_SESSION_DATA_TYPE     DataType,
  IN OUT VOID                          *Data,  OPTIONAL
  IN OUT UINTN                         *DataSize
  );

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
  IN     EFI_TLS_PROTOCOL              *This,
  IN     UINT8                         *RequestBuffer, OPTIONAL
  IN     UINTN                         RequestSize, OPTIONAL
     OUT UINT8                         *Buffer, OPTIONAL
  IN OUT UINTN                         *BufferSize
  );

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
  IN     EFI_TLS_PROTOCOL              *This,
  IN OUT EFI_TLS_FRAGMENT_DATA         **FragmentTable,
  IN     UINT32                        *FragmentCount,
  IN     EFI_TLS_CRYPT_MODE            CryptMode
  );

/**
  Set TLS configuration data.

  The SetData() function sets TLS configuration to non-volatile storage or volatile
  storage.

  @param[in]  This                Pointer to the EFI_TLS_CONFIGURATION_PROTOCOL instance.
  @param[in]  DataType            Configuration data type.
  @param[in]  Data                Pointer to configuration data.
  @param[in]  DataSize            Total size of configuration data.

  @retval EFI_SUCCESS             The TLS configuration data is set successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Data is NULL.
                                  DataSize is 0.
  @retval EFI_UNSUPPORTED         The DataType is unsupported.
  @retval EFI_OUT_OF_RESOURCES    Required system resources could not be allocated.
**/
EFI_STATUS
EFIAPI
TlsConfigurationSetData (
  IN     EFI_TLS_CONFIGURATION_PROTOCOL  *This,
  IN     EFI_TLS_CONFIG_DATA_TYPE        DataType,
  IN     VOID                            *Data,
  IN     UINTN                           DataSize
  );

/**
  Get TLS configuration data.

  The GetData() function gets TLS configuration.

  @param[in]       This           Pointer to the EFI_TLS_CONFIGURATION_PROTOCOL instance.
  @param[in]       DataType       Configuration data type.
  @param[in, out]  Data           Pointer to configuration data.
  @param[in, out]  DataSize       Total size of configuration data. On input, it means
                                  the size of Data buffer. On output, it means the size
                                  of copied Data buffer if EFI_SUCCESS, and means the
                                  size of desired Data buffer if EFI_BUFFER_TOO_SMALL.

  @retval EFI_SUCCESS             The TLS configuration data is got successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  DataSize is NULL.
                                  Data is NULL if *DataSize is not zero.
  @retval EFI_UNSUPPORTED         The DataType is unsupported.
  @retval EFI_NOT_FOUND           The TLS configuration data is not found.
  @retval EFI_BUFFER_TOO_SMALL    The buffer is too small to hold the data.
**/
EFI_STATUS
EFIAPI
TlsConfigurationGetData (
  IN     EFI_TLS_CONFIGURATION_PROTOCOL  *This,
  IN     EFI_TLS_CONFIG_DATA_TYPE        DataType,
  IN OUT VOID                            *Data, OPTIONAL
  IN OUT UINTN                           *DataSize
  );

#endif


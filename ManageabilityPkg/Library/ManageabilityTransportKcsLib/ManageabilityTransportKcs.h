/** @file

  Manageability transport KCS internal used definitions.

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/ManageabilityTransportLib.h>

#define MANAGEABILITY_TRANSPORT_KCS_SIGNATURE  SIGNATURE_32 ('M', 'T', 'K', 'C')

///
/// Manageability transport KCS internal data structure.
///
typedef struct {
  UINTN                            Signature;
  MANAGEABILITY_TRANSPORT_TOKEN    Token;
} MANAGEABILITY_TRANSPORT_KCS;

#define MANAGEABILITY_TRANSPORT_KCS_FROM_LINK(a)  CR (a, MANAGEABILITY_TRANSPORT_KCS, Token, MANAGEABILITY_TRANSPORT_KCS_SIGNATURE)

#define IPMI_KCS_GET_STATE(s)  (s >> 6)
#define IPMI_KCS_SET_STATE(s)  (s << 6)

#define MCTP_KCS_MTU_IN_POWER_OF_2  8

#define IPMI_KCS_TIMEOUT_1MS  1000

/**
  This service communicates with BMC using KCS protocol.

  @param[in]      TransmitHeader        KCS packet header.
  @param[in]      TransmitHeaderSize    KCS packet header size in byte.
  @param[in]      TransmitTrailer       KCS packet trailer.
  @param[in]      TransmitTrailerSize   KCS packet trailer size in byte.
  @param[in]      RequestData           Command Request Data.
  @param[in]      RequestDataSize       Size of Command Request Data.
  @param[in]      SessionToken          KCS session token, used to select the
                                        manageability protocol.
  @param[in]      HardwareInfo          KCS register configuration.
  @param[out]     ResponseData          Command Response Data. The completion
                                        code is the first byte of response
                                        data.
  @param[in, out] ResponseDataSize      Size of Command Response Data.
  @param[out]     AdditionalStatus       Additional status of this transaction.

  @retval         EFI_SUCCESS           The command byte stream was
                                        successfully submit to the device and a
                                        response was successfully received.
  @retval         EFI_NOT_FOUND         The command was not successfully sent
                                        to the device or a response was not
                                        successfully received from the device.
  @retval         EFI_NOT_READY         Ipmi Device is not ready for Ipmi
                                        command access.
  @retval         EFI_DEVICE_ERROR      Ipmi Device hardware error.
  @retval         EFI_TIMEOUT           The command time out.
  @retval         EFI_UNSUPPORTED       The command was not successfully sent to
                                        the device.
  @retval         EFI_OUT_OF_RESOURCES  The resource allocation is out of
                                        resource or data size error.
  @retval         EFI_INVALID_PARAMETER  RequestData and RequestDataSize are
                                        inconsistent, or SessionToken or
                                        HardwareInfo is NULL.
**/

EFI_STATUS
EFIAPI
KcsTransportSendCommand (
  IN  MANAGEABILITY_TRANSPORT_HEADER              TransmitHeader OPTIONAL,
  IN  UINT16                                      TransmitHeaderSize,
  IN  MANAGEABILITY_TRANSPORT_TRAILER             TransmitTrailer OPTIONAL,
  IN  UINT16                                      TransmitTrailerSize,
  IN  UINT8                                       *RequestData OPTIONAL,
  IN  UINT32                                      RequestDataSize,
  IN  MANAGEABILITY_TRANSPORT_KCS                 *SessionToken,
  IN  MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO   *HardwareInfo,
  OUT UINT8                                       *ResponseData OPTIONAL,
  IN  OUT UINT32                                  *ResponseDataSize OPTIONAL,
  OUT  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *AdditionalStatus
  );

/**
  This function reads 8-bit value from register address.

  @param[in]      Address               This represents either 16-bit IO address
                                        or 32-bit memory mapped address.
  @param[in]      IsMmio                TRUE if Address is a memory mapped
                                        address, FALSE if it is an I/O address.

  @retval         UINT8                 8-bit value.
**/
UINT8
EFIAPI
KcsRegisterRead8 (
  MANAGEABILITY_TRANSPORT_HARDWARE_IO  Address,
  BOOLEAN                              IsMmio
  );

/**
  This function writes 8-bit value to register address.

  @param[in]      Address               This represents either 16-bit IO address
                                        or 32-bit memory mapped address.
  @param[in]      Value                 8-bit value write to register address
  @param[in]      IsMmio                TRUE if Address is a memory mapped
                                        address, FALSE if it is an I/O address.

**/
VOID
EFIAPI
KcsRegisterWrite8 (
  MANAGEABILITY_TRANSPORT_HARDWARE_IO  Address,
  UINT8                                Value,
  BOOLEAN                              IsMmio
  );

/**
  Get the KCS hardware information of the current boot phase.

  @return Pointer to the KCS hardware information, or NULL if unavailable.
**/
MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO *
EFIAPI
KcsGetHardwareInfo (
  VOID
  );

/**
  Get the KCS session token of the current boot phase.

  @return Pointer to the session token, or NULL if no session is acquired.
**/
MANAGEABILITY_TRANSPORT_KCS *
EFIAPI
KcsGetSessionToken (
  VOID
  );

/**
  Record the KCS session token of the current boot phase.

  @param[in]  SessionToken  Session token to record, or NULL to clear it.

  @retval EFI_SUCCESS       The session token was recorded.
  @retval EFI_DEVICE_ERROR  The phase-specific state storage is unavailable.
**/
EFI_STATUS
EFIAPI
KcsSetSessionToken (
  IN  MANAGEABILITY_TRANSPORT_KCS  *SessionToken
  );

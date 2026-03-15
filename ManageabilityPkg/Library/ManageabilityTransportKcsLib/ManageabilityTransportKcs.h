/** @file

  Manageability transport KCS internal used definitions.

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MANAGEABILITY_TRANSPORT_KCS_LIB_H_
#define MANAGEABILITY_TRANSPORT_KCS_LIB_H_

#include <Library/ManageabilityTransportLib.h>

#define MANAGEABILITY_TRANSPORT_KCS_SIGNATURE  SIGNATURE_32 ('M', 'T', 'K', 'C')

#define KCS_BASE_ADDRESS  mKcsHardwareInfo.IoBaseAddress
#define KCS_REG_DATA_IN   mKcsHardwareInfo.IoDataInAddress
#define KCS_REG_DATA_OUT  mKcsHardwareInfo.IoDataOutAddress
#define KCS_REG_COMMAND   mKcsHardwareInfo.IoCommandAddress
#define KCS_REG_STATUS    mKcsHardwareInfo.IoStatusAddress

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

/// 5 sec, according to IPMI spec
#define IPMI_KCS_TIMEOUT_5_SEC  5000*1000
#define IPMI_KCS_TIMEOUT_1MS    1000

/**
  This service communicates with BMC using KCS protocol.

  @param[in]      TransmitHeader        KCS packet header.
  @param[in]      TransmitHeaderSize    KCS packet header size in byte.
  @param[in]      TransmitTrailer       KCS packet trailer.
  @param[in]      TransmitTrailerSize   KCS packet trailer size in byte.
  @param[in]      RequestData           Command Request Data.
  @param[in]      RequestDataSize       Size of Command Request Data.
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
  OUT UINT8                                       *ResponseData OPTIONAL,
  IN  OUT UINT32                                  *ResponseDataSize OPTIONAL,
  OUT  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *AdditionalStatus
  );

/**
  This function reads 8-bit value from register address.

  @param[in]      Address               This represents either 16-bit IO address
                                        or 32-bit memory mapped address.

  @retval         UINT8                 8-bit value.
**/
UINT8
KcsRegisterRead8 (
  MANAGEABILITY_TRANSPORT_HARDWARE_IO  Address
  );

/**
  This function writes 8-bit value to register address.

  @param[in]      Address               This represents either 16-bit IO address
                                        or 32-bit memory mapped address.
  @param[in]      Value                 8-bit value write to register address

**/
VOID
KcsRegisterWrite8 (
  MANAGEABILITY_TRANSPORT_HARDWARE_IO  Address,
  UINT8                                Value
  );

#endif

/** @file

  Manageability transport Serial internal used definitions.

  Copyright (c) 2024, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MANAGEABILITY_TRANSPORT_SERIAL_LIB_H_
#define MANAGEABILITY_TRANSPORT_SERIAL_LIB_H_

#include <IndustryStandard/IpmiNetFnApp.h>
#include <Library/ManageabilityTransportIpmiLib.h>
#include <Library/ManageabilityTransportLib.h>
#include <IndustryStandard/IpmiSerial.h>

#define MANAGEABILITY_TRANSPORT_SERIAL_SIGNATURE  SIGNATURE_32 ('M', 'T', 'S', 'E')

///
/// Manageability transport Serial internal data structure.
///
typedef struct {
  UINTN                            Signature;
  MANAGEABILITY_TRANSPORT_TOKEN    Token;
} MANAGEABILITY_TRANSPORT_SERIAL;

#define MANAGEABILITY_TRANSPORT_SERIAL_FROM_LINK(a)  CR (a, MANAGEABILITY_TRANSPORT_SERIAL, Token, MANAGEABILITY_TRANSPORT_SERIAL_SIGNATURE)

///
/// IPMI Serial Configure
///
#define IPMI_SERIAL_RS_ADDRESS               FixedPcdGet8  (PcdIpmiSerialResponderAddress)
#define IPMI_SERIAL_RQ_ADDRESS               FixedPcdGet8  (PcdIpmiSerialRequesterAddress)
#define IPMI_SERIAL_RS_LUN                   FixedPcdGet8  (PcdIpmiSerialResponderLun)
#define IPMI_SERIAL_RQ_LUN                   FixedPcdGet8  (PcdIpmiSerialRequesterLun)
#define IPMI_SERIAL_RETRY_COUNT              FixedPcdGet8  (PcdIpmiSerialRequestRetryCount)
#define IPMI_SERIAL_REQUEST_RETRY_INTERVAL   FixedPcdGet32 (PcdIpmiSerialRequestRetryInterval)
#define IPMI_SERIAL_RESPONSE_RETRY_INTERVAL  FixedPcdGet32 (PcdIpmiSerialRequestRetryInterval)

///
/// Table of special characters
///
struct IpmiSerialSpecialChar {
  UINT8    Character;
  UINT8    Escape;
};

/**
  This service communicates with BMC using Serial protocol.

  @param[in]      TransmitHeader        Serial packet header.
  @param[in]      TransmitHeaderSize    Serial packet header size in byte.
  @param[in]      TransmitTrailer       Serial packet trailer.
  @param[in]      TransmitTrailerSize   Serial packet trailer size in byte.
  @param[in]      RequestData           Command Request Data.
  @param[in]      RequestDataSize       Size of Command Request Data.
  @param[out]     ResponseData          Command Response Data. The completion
                                        code is the first byte of response
                                        data.
  @param[in, out] ResponseDataSize      Size of Command Response Data.
  @param[out]     AdditionalStatus      Additional status of this transaction.

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
SerialTransportSendCommand (
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

#endif

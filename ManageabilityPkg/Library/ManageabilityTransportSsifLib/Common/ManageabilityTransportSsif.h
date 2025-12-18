/** @file

  Manageability transport SSIF internal used definitions.

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MANAGEABILITY_TRANSPORT_SSIF_LIB_H_
#define MANAGEABILITY_TRANSPORT_SSIF_LIB_H_

#include <IndustryStandard/IpmiNetFnApp.h>
#include <Library/ManageabilityTransportIpmiLib.h>
#include <Library/ManageabilityTransportLib.h>

#define MANAGEABILITY_TRANSPORT_SSIF_SIGNATURE  SIGNATURE_32 ('S', 'S', 'I', 'F')

///
/// Manageability transport SSIF internal data structure.
///
typedef struct {
  UINTN                            Signature;
  MANAGEABILITY_TRANSPORT_TOKEN    Token;
} MANAGEABILITY_TRANSPORT_SSIF;

#define MANAGEABILITY_TRANSPORT_SSIF_FROM_LINK(a)  CR (a, MANAGEABILITY_TRANSPORT_SSIF, Token, MANAGEABILITY_TRANSPORT_SSIF_SIGNATURE)

/**
  This retrieves the IPMI SSIF capabilities.

  @param[out]     PecSupport          PEC support.
  @param[out]     TransactionSupport  Transaction support.
  @param[out]     MaxRequestSize      The maximum size of the data request.
  @param[out]     MaxResponseSize     The maximum size of the data response.

  @retval         EFI_SUCCESS             The operation completed successfully.
  @retval         EFI_INVALID_PARAMETER   An input parameter is NUL.

**/
EFI_STATUS
EFIAPI
SsifTransportGetCapabilities (
  OUT BOOLEAN  *PecSupport,
  OUT UINT8    *TransactionSupport,
  OUT UINT8    *MaxRequestSize,
  OUT UINT8    *MaxResponseSize
  );

/**
  This service communicates with BMC using SSIF protocol.

  @param[in]      TransmitHeader        SSIF packet header.
  @param[in]      TransmitHeaderSize    SSIF packet header size in byte.
  @param[in]      TransmitTrailer       SSIF packet trailer.
  @param[in]      TransmitTrailerSize   SSIF packet trailer size in byte.
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
SsifTransportSendCommand (
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

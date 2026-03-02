/** @file
  Protocol of EDKII MCTP Protocol.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_MCTP_PROTOCOL_H_
#define EDKII_MCTP_PROTOCOL_H_

#include <IndustryStandard/Mctp.h>

typedef struct  _EDKII_MCTP_PROTOCOL EDKII_MCTP_PROTOCOL;

#define EDKII_MCTP_PROTOCOL_GUID \
  { \
    0xE93465C1, 0x9A31, 0x4C96, 0x92, 0x56, 0x22, 0x0A, 0xE1, 0x80, 0xB4, 0x1B \
  }

#define EDKII_MCTP_PROTOCOL_VERSION_MAJOR  1
#define EDKII_MCTP_PROTOCOL_VERSION_MINOR  0
#define EDKII_MCTP_PROTOCOL_VERSION        ((EDKII_MCTP_PROTOCOL_VERSION_MAJOR << 8) |\
                                       EDKII_MCTP_PROTOCOL_VERSION_MINOR)

/**
  This service enables submitting message via EDKII MCTP protocol.

  @param[in]         This                       EDKII_MCTP_PROTOCOL instance.
  @param[in]         MctpType                   MCTP message type.
  @param[in]         MctpSourceEndpointId       Pointer of MCTP source endpoint ID.
                                                Set to NULL means use platform PCD value
                                                (PcdMctpSourceEndpointId).
  @param[in]         MctpDestinationEndpointId  Pointer of MCTP destination endpoint ID.
                                                Set to NULL means use platform PCD value
                                                (PcdMctpDestinationEndpointId).
  @param[in]         RequestDataIntegrityCheck  Indicates whether MCTP message has
                                                integrity check byte.
  @param[in]         RequestData                Message Data.
  @param[in]         RequestDataSize            Size of message Data.
  @param[in]         RequestTimeout             Timeout value in milliseconds.
                                                MANAGEABILITY_TRANSPORT_NO_TIMEOUT means no timeout value.
  @param[out]        ResponseData               Message Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize           Size of Message Response Data.
  @param[in]         ResponseTimeout            Timeout value in milliseconds.
                                                MANAGEABILITY_TRANSPORT_NO_TIMEOUT means no timeout value.
  @param[out]        AdditionalTransferError    MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS.

  @retval EFI_SUCCESS            The message was successfully send to transport interface and a
                                 response was successfully received.
  @retval EFI_NOT_FOUND          The message was not successfully sent to transport interface or a response
                                 was not successfully received from transport interface.
  @retval EFI_NOT_READY          MCTP transport interface is not ready for MCTP message.
  @retval EFI_DEVICE_ERROR       MCTP transport interface Device hardware error.
  @retval EFI_TIMEOUT            The message time out.
  @retval EFI_UNSUPPORTED        The message was not successfully sent to the transport interface.
  @retval EFI_OUT_OF_RESOURCES   The resource allocation is out of resource or data size error.
  @retval EFI_INVALID_PARAMETER  Both RequestData and ResponseData are NULL
**/
typedef
EFI_STATUS
(EFIAPI *MCTP_SUBMIT_COMMAND)(
  IN     EDKII_MCTP_PROTOCOL  *This,
  IN     UINT8                MctpType,
  IN     UINT8                *MctpSourceEndpointId,
  IN     UINT8                *MctpDestinationEndpointId,
  IN     BOOLEAN              RequestDataIntegrityCheck,
  IN     UINT8                *RequestData,
  IN     UINT32               RequestDataSize,
  IN     UINT32               RequestTimeout,
  OUT    UINT8                *ResponseData,
  IN OUT UINT32               *ResponseDataSize,
  IN     UINT32               ResponseTimeout,
  OUT    MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS *AdditionalTransferError
  );

//
// EDKII_MCTP_PROTOCOL Version 1.0
//
typedef struct {
  MCTP_SUBMIT_COMMAND    MctpSubmitCommand;
} EDKII_MCTP_PROTOCOL_V1_0;

///
/// Definitions of EDKII_MCTP_PROTOCOL.
/// This is a union that can accommodate the new functionalities defined
/// in MCTP Base specification in the future.
/// The new added function must has its own EDKII_MCTP_PROTOCOL
/// structure with the incremental version number.
///   e.g., EDKII_MCTP_PROTOCOL_V1_1.
///
/// The new function must be added base on the last version of
/// EDKII_MCTP_PROTOCOL to keep the backward compatibility.
///
typedef union {
  EDKII_MCTP_PROTOCOL_V1_0    *Version1_0;
} EDKII_MCTP_PROTOCOL_FUNCTION;

struct _EDKII_MCTP_PROTOCOL {
  UINT16                          ProtocolVersion;
  EDKII_MCTP_PROTOCOL_FUNCTION    Functions;
};

extern EFI_GUID  gEdkiiMctpProtocolGuid;

#endif // EDKII_MCTP_PROTOCOL_H_

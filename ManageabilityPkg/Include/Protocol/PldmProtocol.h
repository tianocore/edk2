/** @file
  Protocol of EDKII PLDM Protocol.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_PLDM_PROTOCOL_H_
#define EDKII_PLDM_PROTOCOL_H_

#include <IndustryStandard/Pldm.h>

typedef struct  _EDKII_PLDM_PROTOCOL EDKII_PLDM_PROTOCOL;

#define EDKII_PLDM_PROTOCOL_GUID \
  { \
    0x60997616, 0xDB70, 0x4B5F, 0x86, 0xA4, 0x09, 0x58, 0xA3, 0x71, 0x47, 0xB4 \
  }

#define EDKII_PLDM_PROTOCOL_VERSION_MAJOR  1
#define EDKII_PLDM_PROTOCOL_VERSION_MINOR  0
#define EDKII_PLDM_PROTOCOL_VERSION        ((EDKII_PLDM_PROTOCOL_VERSION_MAJOR << 8) |\
                                       EDKII_PLDM_PROTOCOL_VERSION_MINOR)

/**
  This service enables submitting commands via EDKII PLDM protocol.

  @param[in]         This                       EDKII_PLDM_PROTOCOL instance.
  @param[in]         PldmType                   PLDM message type.
  @param[in]         Command                    PLDM Command of PLDM message type.
  @param[in]         PldmTerminusSourceId       PLDM source teminus ID.
  @param[in]         PldmTerminusDestinationId  PLDM destination teminus ID.
  @param[in]         RequestData                Command Request Data.
  @param[in]         RequestDataSize            Size of Command Request Data.
  @param[out]        ResponseData               Command Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize           Size of Command Response Data.

  @retval EFI_SUCCESS            The command byte stream was successfully submit to the device and a response was successfully received.
  @retval EFI_NOT_FOUND          The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_NOT_READY          PLDM transport interface is not ready for PLDM command access.
  @retval EFI_DEVICE_ERROR       PLDM transport interface Device hardware error.
  @retval EFI_TIMEOUT            The command time out.
  @retval EFI_UNSUPPORTED        The command was not successfully sent to the device.
  @retval EFI_OUT_OF_RESOURCES   The resource allcation is out of resource or data size error.
  @retval EFI_INVALID_PARAMETER  Both RequestData and ResponseData are NULL
**/
typedef
EFI_STATUS
(EFIAPI *PLDM_SUBMIT_COMMAND)(
  IN     EDKII_PLDM_PROTOCOL  *This,
  IN     UINT8                PldmType,
  IN     UINT8                Command,
  IN     UINT8                PldmTerminusSourceId,
  IN     UINT8                PldmTerminusDestinationId,
  IN     UINT8                *RequestData,
  IN     UINT32               RequestDataSize,
  OUT    UINT8                *ResponseData,
  IN OUT UINT32               *ResponseDataSize
  );

//
// EDKII_PLDM_PROTOCOL Version 1.0
//
typedef struct {
  PLDM_SUBMIT_COMMAND    PldmSubmitCommand;
} EDKII_PLDM_PROTOCOL_V1_0;

///
/// Definitions of EDKII_PLDM_PROTOCOL.
/// This is a union that can accommodate the new functionalities defined
/// in PLDM Base specification in the future.
/// The new added function must has its own EDKII_PLDM_PROTOCOL
/// structure with the incremental version number.
///   e.g., EDKII_PLDM_PROTOCOL_V1_1.
///
/// The new function must be added base on the last version of
/// EDKII_PLDM_PROTOCOL to keep the backward compatability.
///
typedef union {
  EDKII_PLDM_PROTOCOL_V1_0    *Version1_0;
} EDKII_PLDM_PROTOCOL_FUNCTION;

struct _EDKII_PLDM_PROTOCOL {
  UINT16                          ProtocolVersion;
  EDKII_PLDM_PROTOCOL_FUNCTION    Functions;
};

extern EFI_GUID  gEdkiiPldmProtocolGuid;

#endif // EDKII_PLDM_PROTOCOL_H_

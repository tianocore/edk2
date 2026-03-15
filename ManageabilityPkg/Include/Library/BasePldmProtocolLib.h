/** @file

  This file defines EDKII Pldm Protocol library and functions.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef EDKII_PLDM_PROTOCOL_LIB_H_
#define EDKII_PLDM_PROTOCOL_LIB_H_

/**
  This function sets the PLDM source termius and destination terminus
  ID for SMBIOS PLDM transfer.

  @param[in]         SourceId       PLDM source teminus ID.
  @param[in]         DestinationId  PLDM destination teminus ID.

  @retval EFI_SUCCESS            The terminus is set successfully.
  @retval EFI_INVALID_PARAMETER  The terminus is set unsuccessfully.
**/
EFI_STATUS
PldmSetTerminus (
  IN  UINT8  SourceId,
  IN  UINT8  DestinationId
  );

/**
  This service enables submitting commands via EDKII PLDM protocol.

  @param[in]         PldmType          PLDM message type.
  @param[in]         Command           PLDM Command of PLDM message type.
  @param[in]         RequestData       Command Request Data.
  @param[in]         RequestDataSize   Size of Command Request Data.
  @param[out]        ResponseData      Command Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize  Size of Command Response Data.

  @retval EFI_SUCCESS            PLDM message was successfully sent to transport interface
                                 and a response was successfully received.
  @retval EFI_NOT_FOUND          Transport interface is not found.
  @retval EFI_NOT_READY          Transport interface is not ready for PLDM message.
  @retval EFI_DEVICE_ERROR       Transport interface has an hardware error.
  @retval EFI_TIMEOUT            Send PLDM message got a timeout.
  @retval EFI_UNSUPPORTED        PLDM message is unsupported.
  @retval EFI_OUT_OF_RESOURCES   The resource allocation is out of resource or data size error.
**/
EFI_STATUS
PldmSubmitCommand (
  IN     UINT8   PldmType,
  IN     UINT8   Command,
  IN     UINT8   *RequestData,
  IN     UINT32  RequestDataSize,
  OUT UINT8      *ResponseData,
  IN OUT UINT32  *ResponseDataSize
  );

#endif

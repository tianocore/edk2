/** @file
  Protocol of Ipmi for both SMS and SMM.

  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _IPMI_PROTOCOL_H_
#define _IPMI_PROTOCOL_H_

typedef struct _IPMI_PROTOCOL IPMI_PROTOCOL;

#define IPMI_PROTOCOL_GUID \
  { \
    0xdbc6381f, 0x5554, 0x4d14, 0x8f, 0xfd, 0x76, 0xd7, 0x87, 0xb8, 0xac, 0xbf \
  }

#define SMM_IPMI_PROTOCOL_GUID \
  { \
    0x5169af60, 0x8c5a, 0x4243, 0xb3, 0xe9, 0x56, 0xc5, 0x6d, 0x18, 0xee, 0x26 \
  }

/**
  This service enables submitting commands via Ipmi.

  @param[in]         This              This point for IPMI_PROTOCOL structure.
  @param[in]         NetFunction       Net function of the command.
  @param[in]         Command           IPMI Command.
  @param[in]         RequestData       Command Request Data.
  @param[in]         RequestDataSize   Size of Command Request Data.
  @param[out]        ResponseData      Command Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize  Size of Command Response Data.

  @retval EFI_SUCCESS            The command byte stream was successfully submit to the device and a response was successfully received.
  @retval EFI_NOT_FOUND          The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_NOT_READY          Ipmi Device is not ready for Ipmi command access.
  @retval EFI_DEVICE_ERROR       Ipmi Device hardware error.
  @retval EFI_TIMEOUT            The command time out.
  @retval EFI_UNSUPPORTED        The command was not successfully sent to the device.
  @retval EFI_OUT_OF_RESOURCES   The resource allcation is out of resource or data size error.
**/
typedef
EFI_STATUS
(EFIAPI *IPMI_SUBMIT_COMMAND)(
  IN     IPMI_PROTOCOL                 *This,
  IN     UINT8                         NetFunction,
  IN     UINT8                         Command,
  IN     UINT8                         *RequestData,
  IN     UINT32                        RequestDataSize,
  OUT UINT8                         *ResponseData,
  IN OUT UINT32                        *ResponseDataSize
  );

//
// IPMI COMMAND PROTOCOL
//
struct _IPMI_PROTOCOL {
  IPMI_SUBMIT_COMMAND    IpmiSubmitCommand;
};

extern EFI_GUID  gIpmiProtocolGuid;
extern EFI_GUID  gSmmIpmiProtocolGuid;

#endif

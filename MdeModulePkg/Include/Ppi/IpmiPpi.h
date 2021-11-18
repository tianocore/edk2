/** @file
  Ppi for Ipmi of SMS.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _IPMI_PPI_H_
#define _IPMI_PPI_H_

typedef struct _PEI_IPMI_PPI PEI_IPMI_PPI;

#define PEI_IPMI_PPI_GUID \
  { \
    0xa9731431, 0xd968, 0x4277, 0xb7, 0x52, 0xa3, 0xa9, 0xa6, 0xae, 0x18, 0x98 \
  }

/**
  This service enables submitting commands via Ipmi.

  @param[in]         This              This point for PEI_IPMI_PPI structure.
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
(EFIAPI *PEI_IPMI_SUBMIT_COMMAND)(
  IN     PEI_IPMI_PPI                      *This,
  IN     UINT8                             NetFunction,
  IN     UINT8                             Command,
  IN     UINT8                             *RequestData,
  IN     UINT32                            RequestDataSize,
  OUT UINT8                             *ResponseData,
  IN OUT UINT32                            *ResponseDataSize
  );

//
// IPMI PPI
//
struct _PEI_IPMI_PPI {
  PEI_IPMI_SUBMIT_COMMAND    IpmiSubmitCommand;
};

extern EFI_GUID  gPeiIpmiPpiGuid;

#endif

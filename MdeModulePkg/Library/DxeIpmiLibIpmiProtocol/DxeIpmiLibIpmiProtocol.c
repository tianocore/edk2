/** @file
  Implementation of Ipmi Library in DXE Phase for SMS.

  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Protocol/IpmiProtocol.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

IPMI_PROTOCOL *mIpmiProtocol = NULL;

/**
  This service enables submitting commands via Ipmi.

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
EFI_STATUS
EFIAPI
IpmiSubmitCommand (
  IN     UINT8     NetFunction,
  IN     UINT8     Command,
  IN     UINT8     *RequestData,
  IN     UINT32    RequestDataSize,
     OUT UINT8     *ResponseData,
  IN OUT UINT32    *ResponseDataSize
  )
{
  EFI_STATUS  Status;

  if (mIpmiProtocol == NULL) {
    Status = gBS->LocateProtocol (
              &gIpmiProtocolGuid,
              NULL,
              (VOID **) &mIpmiProtocol
              );
    if (EFI_ERROR (Status)) {
      //
      // Dxe Ipmi Protocol is not installed. So, IPMI device is not present.
      //
      DEBUG ((EFI_D_ERROR, "IpmiSubmitCommand in Dxe Phase under SMS Status - %r\n", Status));
      return EFI_NOT_FOUND;
    }
  }

  Status = mIpmiProtocol->IpmiSubmitCommand (
                            mIpmiProtocol,
                            NetFunction,
                            Command,
                            RequestData,
                            RequestDataSize,
                            ResponseData,
                            ResponseDataSize
                            );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  return EFI_SUCCESS;
}

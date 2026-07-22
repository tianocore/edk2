/** @file MockIpmiLib.h
  Google Test mocks for IpmiLib

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_IPMI_LIB_H_
#define MOCK_IPMI_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/IpmiLib.h>
}

struct MockIpmiLib {
  MOCK_INTERFACE_DECLARATION (MockIpmiLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiSubmitCommand,
    (
     IN     UINT8   NetFunction,
     IN     UINT8   Command,
     IN     UINT8   *RequestData,
     IN     UINT32  RequestDataSize,
     OUT UINT8      *ResponseData,
     IN OUT UINT32  *ResponseDataSize
    )
    );
};

#endif

/** @file
  Device Node Address Range Parser.

  Copyright (c) 2021 - 2024, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DEVICE_PARSER_H_
#define DEVICE_PARSER_H_

/** Device dispatcher.

  @param [in]  FdtParserHandle  A handle to the parser instance.
  @param [in]  FdtBranch        When searching for DT node name, restrict
                                the search to this Device Tree branch.
  @param [in]  CompatibleInfo   List of "compatible" property values for
                                the device.
  @param [in]  DevDesc          A NULL terminated string containing a short
                                description of the device.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
DeviceDispatcher (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch,
  IN  CONST COMPATIBILITY_INFO         *CompatibleInfo,
  IN  CONST CHAR8                      *DevDesc
  );

#endif // DEVICE_PARSER_H_

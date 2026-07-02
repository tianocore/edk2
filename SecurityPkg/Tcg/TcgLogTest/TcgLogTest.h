/** @file
  TCG Log Test protocol definition.

  Defines the protocol produced by TcgLogTestDxe that allows the TcgLogTestApp
  to retrieve pre-ReadyToBoot test results and to enable/disable the DXE test
  via an NV variable.

  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef TCG_LOG_TEST_H_
#define TCG_LOG_TEST_H_

#include <Uefi.h>

#define TCG_LOG_TEST_PROTOCOL_GUID \
  { 0xA3C12F80, 0x7D9E, 0x4B5A, { 0x91, 0xE4, 0x6C, 0xF8, 0x2D, 0xA1, 0xB7, 0x03 } }

#define TCG_LOG_TEST_ENABLE_VARIABLE_NAME  L"TcgLogTestEnable"

typedef struct _TCG_LOG_TEST_PROTOCOL TCG_LOG_TEST_PROTOCOL;

/**
  Retrieve the pre-ReadyToBoot test log produced by TcgLogTestDxe.

  @param[in]  This       Protocol instance.
  @param[out] LogBuffer  Pointer to the internal log buffer (NULL-terminated).
  @param[out] LogSize    Number of valid bytes in LogBuffer (including NULL).

  @retval EFI_SUCCESS            Log data returned.
  @retval EFI_NOT_STARTED        The DXE test did not run this boot.
  @retval EFI_INVALID_PARAMETER  NULL pointer supplied.
**/
typedef
EFI_STATUS
(EFIAPI *TCG_LOG_TEST_GET_LOG)(
  IN  TCG_LOG_TEST_PROTOCOL  *This,
  OUT CHAR8                  **LogBuffer,
  OUT UINTN                  *LogSize
  );

/**
  Enable or disable the DXE pre-ReadyToBoot test for the next boot by
  writing an NV variable.

  @param[in] This    Protocol instance.
  @param[in] Enable  TRUE to enable the test on next boot, FALSE to disable.

  @retval EFI_SUCCESS  Variable written successfully.
  @retval Other        SetVariable failure.
**/
typedef
EFI_STATUS
(EFIAPI *TCG_LOG_TEST_ENABLE)(
  IN TCG_LOG_TEST_PROTOCOL  *This,
  IN BOOLEAN                Enable
  );

struct _TCG_LOG_TEST_PROTOCOL {
  TCG_LOG_TEST_GET_LOG    GetLog;
  TCG_LOG_TEST_ENABLE     Enable;
};

extern EFI_GUID  gTcgLogTestProtocolGuid;

#endif // TCG_LOG_TEST_H_

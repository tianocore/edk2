/** @file
  This module collects performance data for MM driver boot records and S3 Suspend Performance Record.

  This module registers report status code listener to collect performance data
  for MM driver boot records and S3 Suspend Performance Record.

  Caution: This module requires additional review when modified.
  This driver will have external input - communicate buffer in MM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  FpdtSmiHandler() will receive untrusted input and do basic validation.

  Copyright (c) 2011 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include "FirmwarePerformanceCommon.h"

/**
  The module Entry Point of the Firmware Performance Data Table MM driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI MM System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
FirmwarePerformanceStandaloneMmEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return FirmwarePerformanceCommonEntryPoint ();
}

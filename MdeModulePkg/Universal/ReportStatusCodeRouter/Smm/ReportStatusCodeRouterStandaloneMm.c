/** @file
  Report Status Code Router Driver which produces MM Report Stataus Code Handler Protocol
  and MM Status Code Protocol.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ReportStatusCodeRouterCommon.h"

/**
  Entry point of Generic Status Code Driver.

  This function is the entry point of MM Status Code Router .
  It produces MM Report Stataus Code Handler and Status Code protocol.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
GenericStatusCodeStandaloneMmEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return GenericStatusCodeCommonEntry ();
}

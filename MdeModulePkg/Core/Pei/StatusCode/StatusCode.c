/** @file
  Pei Core Status Code Support

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiMain.h"

/**

  Core version of the Status Code reporter


  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param CodeType        Type of Status Code.
  @param Value           Value to output for Status Code.
  @param Instance        Instance Number of this status code.
  @param CallerId        ID of the caller of this status code.
  @param Data            Optional data associated with this status code.

  @retval EFI_SUCCESS             if status code is successfully reported
  @retval EFI_NOT_AVAILABLE_YET   if StatusCodePpi has not been installed

**/
EFI_STATUS
EFIAPI
PeiReportStatusCode (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN EFI_STATUS_CODE_TYPE        CodeType,
  IN EFI_STATUS_CODE_VALUE       Value,
  IN UINT32                      Instance,
  IN CONST EFI_GUID              *CallerId,
  IN CONST EFI_STATUS_CODE_DATA  *Data OPTIONAL
  )
{
  EFI_STATUS                 Status;
  EFI_PEI_PROGRESS_CODE_PPI  *StatusCodePpi;

  //
  // Locate StatusCode Ppi.
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiStatusCodePpiGuid,
             0,
             NULL,
             (VOID **)&StatusCodePpi
             );

  if (!EFI_ERROR (Status)) {
    Status = StatusCodePpi->ReportStatusCode (
                              PeiServices,
                              CodeType,
                              Value,
                              Instance,
                              CallerId,
                              Data
                              );

    return Status;
  }

  return EFI_NOT_AVAILABLE_YET;
}

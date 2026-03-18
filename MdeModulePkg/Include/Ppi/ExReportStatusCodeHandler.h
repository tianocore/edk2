/** @file
  Define the EFI_PEI_EX_RSC_HANDLER_PPI that Registers ExSerialStatusCodeReportWorker as callback function for ReportStatusCode() notification.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EX_REPORT_STATUS_CODE_HANDLER_PPI_H__
#define __EX_REPORT_STATUS_CODE_HANDLER_PPI_H__

/**
  Registers ExSerialStatusCodeReportWorker as callback function for ReportStatusCode() notification.


  @param[in] PeiServices        Pointer to PEI Services Table.

  @retval EFI_SUCCESS           Function was successfully registered.
  @retval EFI_INVALID_PARAMETER The callback function was NULL.
  @retval EFI_OUT_OF_RESOURCES  The internal buffer ran out of space. No more functions can be
                                registered.
  @retval EFI_ALREADY_STARTED   The function was already registered. It can't be registered again.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_EX_RSC_HANDLER_REGISTER)(
  IN CONST  EFI_PEI_SERVICES        **PeiServices
  );

typedef struct _EFI_EX_PEI_RSC_HANDLER_PPI {
  EFI_PEI_EX_RSC_HANDLER_REGISTER    RegisterExStatusCodeHandler;
} EFI_PEI_EX_RSC_HANDLER_PPI;

extern EFI_GUID  gEfiPeiExStatusCodeHandlerPpiGuid;

#endif // __EX_REPORT_STATUS_CODE_HANDLER_PPI_H__

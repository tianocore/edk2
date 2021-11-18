/** @file
  Report Status Code Handler PEIM which produces general handlers and hook them
  onto the PEI status code router.

  Copyright (c) 2009 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StatusCodeHandlerPei.h"

/**
  Entry point of Status Code PEIM.

  This function is the entry point of this Status Code PEIM.
  It initializes supported status code devices according to PCD settings,
  and installs Status Code PPI.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCESS  The entry point of DXE IPL PEIM executes successfully.

**/
EFI_STATUS
EFIAPI
StatusCodeHandlerPeiEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS               Status;
  EFI_PEI_RSC_HANDLER_PPI  *RscHandlerPpi;

  Status = PeiServicesLocatePpi (
             &gEfiPeiRscHandlerPpiGuid,
             0,
             NULL,
             (VOID **)&RscHandlerPpi
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Dispatch initialization request to sub-statuscode-devices.
  // If enable UseSerial, then initialize serial port.
  // if enable UseMemory, then initialize memory status code worker.
  //
  if (PcdGetBool (PcdStatusCodeUseSerial)) {
    Status = SerialPortInitialize ();
    ASSERT_EFI_ERROR (Status);
    Status = RscHandlerPpi->Register (SerialStatusCodeReportWorker);
    ASSERT_EFI_ERROR (Status);
  }

  if (PcdGetBool (PcdStatusCodeUseMemory)) {
    Status = MemoryStatusCodeInitializeWorker ();
    ASSERT_EFI_ERROR (Status);
    Status = RscHandlerPpi->Register (MemoryStatusCodeReportWorker);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

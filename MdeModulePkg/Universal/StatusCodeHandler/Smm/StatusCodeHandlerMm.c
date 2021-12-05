/** @file
  Status Code Handler Driver which produces general handlers and hook them
  onto the MM status code router.

  Copyright (c) 2009 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StatusCodeHandlerMm.h"

EFI_MM_RSC_HANDLER_PROTOCOL  *mRscHandlerProtocol = NULL;

/**
  Dispatch initialization request to sub status code devices based on
  customized feature flags.

**/
VOID
InitializationDispatcherWorker (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // If enable UseSerial, then initialize serial port.
  // if enable UseRuntimeMemory, then initialize runtime memory status code worker.
  //
  if (PcdGetBool (PcdStatusCodeUseSerial)) {
    //
    // Call Serial Port Lib API to initialize serial port.
    //
    Status = SerialPortInitialize ();
    ASSERT_EFI_ERROR (Status);
  }

  if (PcdGetBool (PcdStatusCodeUseMemory)) {
    Status = MemoryStatusCodeInitializeWorker ();
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Entry point of Common MM Status Code Driver.

  This function is the entry point of MM Status Code Driver.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
StatusCodeHandlerCommonEntry (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gMmst->MmLocateProtocol (
                    &gEfiMmRscHandlerProtocolGuid,
                    NULL,
                    (VOID **)&mRscHandlerProtocol
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Dispatch initialization request to supported devices
  //
  InitializationDispatcherWorker ();

  if (PcdGetBool (PcdStatusCodeUseSerial)) {
    mRscHandlerProtocol->Register (SerialStatusCodeReportWorker);
  }

  if (PcdGetBool (PcdStatusCodeUseMemory)) {
    mRscHandlerProtocol->Register (MemoryStatusCodeReportWorker);
  }

  return EFI_SUCCESS;
}

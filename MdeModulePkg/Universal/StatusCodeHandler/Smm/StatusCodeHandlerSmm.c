/** @file
  Status Code Handler Driver which produces general handlers and hook them
  onto the SMM status code router.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StatusCodeHandlerSmm.h"

EFI_SMM_RSC_HANDLER_PROTOCOL  *mRscHandlerProtocol       = NULL;


/**
  Dispatch initialization request to sub status code devices based on
  customized feature flags.

**/
VOID
InitializationDispatcherWorker (
  VOID
  )
{
  EFI_STATUS                        Status;

  //
  // If enable UseSerial, then initialize serial port.
  // if enable UseRuntimeMemory, then initialize runtime memory status code worker.
  //
  if (FeaturePcdGet (PcdStatusCodeUseSerial)) {
    //
    // Call Serial Port Lib API to initialize serial port.
    //
    Status = SerialPortInitialize ();
    ASSERT_EFI_ERROR (Status);
  }
  if (FeaturePcdGet (PcdStatusCodeUseMemory)) {
    Status = MemoryStatusCodeInitializeWorker ();
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Entry point of SMM Status Code Driver.

  This function is the entry point of SMM Status Code Driver.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
StatusCodeHandlerSmmEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                Status;

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmRscHandlerProtocolGuid,
                    NULL,
                    (VOID **) &mRscHandlerProtocol
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Dispatch initialization request to supported devices
  //
  InitializationDispatcherWorker ();

  if (FeaturePcdGet (PcdStatusCodeUseSerial)) {
    mRscHandlerProtocol->Register (SerialStatusCodeReportWorker);
  }
  if (FeaturePcdGet (PcdStatusCodeUseMemory)) {
    mRscHandlerProtocol->Register (MemoryStatusCodeReportWorker);
  }

  return EFI_SUCCESS;
}

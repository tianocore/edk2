/** @file
  Report Status Code Handler PEIM which produces general handlers and hook them
  onto the PEI status code router.

  Copyright (c) 2009 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StatusCodeHandlerPei.h"

EFI_PEI_EX_RSC_HANDLER_PPI  mExStatusCodeHandlerPpi = {
  RegisterExStatusCodeHandler
};

EFI_PEI_PPI_DESCRIPTOR  mExStatusCodeHandlerPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiExStatusCodeHandlerPpiGuid,
    &mExStatusCodeHandlerPpi
  }
};

// {560A1DA9-CADB-421D-ADAD-A583E2143601}
EFI_GUID  gExStatusCodeHandlerModuleMigrationGuid = {
  0x560a1da9, 0xcadb, 0x421d, { 0xad, 0xad, 0xa5, 0x83, 0xe2, 0x14, 0x36, 0x1 }
};

/**
  Registers ExSerialStatusCodeReportWorker as callback function for ReportStatusCode() notification.


  @param[in] PeiServices        Pointer to PEI Services Table.

  @retval EFI_SUCCESS           Function was successfully registered.
  @retval EFI_INVALID_PARAMETER The callback function was NULL.
  @retval EFI_OUT_OF_RESOURCES  The internal buffer ran out of space. No more functions can be
                                registered.
  @retval EFI_ALREADY_STARTED   The function was already registered. It can't be registered again.

**/
EFI_STATUS
EFIAPI
RegisterExStatusCodeHandler (
  IN CONST  EFI_PEI_SERVICES  **PeiServices
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

  if (PcdGetBool (PcdStatusCodeUseSerial)) {
    Status = RscHandlerPpi->Register (ExSerialStatusCodeReportWorker);
    if (Status != EFI_ALREADY_STARTED) {
      ASSERT_EFI_ERROR (Status);
    }
  }

  return Status;
}

/**
  Entry point of Status Code PEIM.

  This function is the entry point of this Status Code PEIM.
  It initializes supported status code devices according to PCD settings,
  and installs Status Code PPI.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS The entry point of DXE IPL PEIM executes successfully.

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
  UINT32                   DebugPrintErrorLevel;
  UINTN                    ModuleOffset;
  UINTN                    Index;
  BOOLEAN                  ModulePositive;
  SERIAL_PORT_CONFIG       *SerialPortConfig;

  if (PcdGetBool (PcdMigrateTemporaryRamFirmwareVolumes)) {
    Status = ModuleOffsetCalculator (FileHandle, &gExStatusCodeHandlerModuleMigrationGuid, &ModuleOffset, &ModulePositive);
    if (Status == EFI_ALREADY_STARTED) {
      SerialPortConfig = GetFirstGuidHob (&gSerialPortConfigGuid);
      if (SerialPortConfig != NULL) {
        SerialPortConfig = GET_GUID_HOB_DATA (SerialPortConfig);
        if (ModulePositive) {
          for (Index = 0; Index < SERIAL_PORT_CONFIG_MAX; Index++) {
            *(UINTN *)(UINTN)(&SerialPortConfig->Configurations[Index].Initialize) += ModuleOffset;
            *(UINTN *)(UINTN)(&SerialPortConfig->Configurations[Index].WriteBytes) += ModuleOffset;
          }
        } else {
          for (Index = 0; Index < SERIAL_PORT_CONFIG_MAX; Index++) {
            *(UINTN *)(UINTN)(&SerialPortConfig->Configurations[Index].Initialize) -= ModuleOffset;
            *(UINTN *)(UINTN)(&SerialPortConfig->Configurations[Index].WriteBytes) -= ModuleOffset;
          }
        }
      }

      return EFI_SUCCESS;
    }
  }

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

  //
  // If serial logging is disabled. set PcdStatusCodeUseSerial to FALSE.
  //
  DebugPrintErrorLevel = GetDebugPrintErrorLevel ();
  if (DebugPrintErrorLevel == 0) {
    Status = PcdSetBoolS (PcdStatusCodeUseSerial, FALSE);
    ASSERT_EFI_ERROR (Status);
  }

  Status = RegisterExStatusCodeHandler (PeiServices);
  ASSERT_EFI_ERROR (Status);

  //
  // Install Report Status Code Handler PPI
  //
  Status = PeiServicesInstallPpi (mExStatusCodeHandlerPpiList);
  ASSERT_EFI_ERROR (Status);

  if (PcdGetBool (PcdStatusCodeUseMemory)) {
    Status = MemoryStatusCodeInitializeWorker ();
    ASSERT_EFI_ERROR (Status);
    Status = RscHandlerPpi->Register (MemoryStatusCodeReportWorker);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

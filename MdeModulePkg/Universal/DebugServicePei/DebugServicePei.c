/** @file
  This driver installs gEdkiiDebugPpiGuid PPI to provide
  debug services for PEIMs.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>

#include <Ppi/Debug.h>

#include "DebugService.h"

EDKII_DEBUG_PPI  mDebugPpi = {
  PeiDebugBPrint,
  PeiDebugAssert
};

EFI_PEI_PPI_DESCRIPTOR  mDebugServicePpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiDebugPpiGuid,
  (VOID *)&mDebugPpi
};

/**
  Print a debug message to debug output device if the specified error level
  is enabled.

  @param[in] ErrorLevel               The error level of the debug message.
  @param[in] Format                   Format string for the debug message to print.
  @param[in] Marker                   BASE_LIST marker for the variable argument list.

**/
VOID
EFIAPI
PeiDebugBPrint (
  IN UINTN        ErrorLevel,
  IN CONST CHAR8  *Format,
  IN BASE_LIST    Marker
  )
{
  DebugBPrint (ErrorLevel, Format, Marker);
}

/**
  Print an assert message containing a filename, line number, and description.
  This may be followed by a breakpoint or a dead loop.

  @param[in] FileName                 The pointer to the name of the source file that
                                      generated the assert condition.
  @param[in] LineNumber               The line number in the source file that generated
                                      the assert condition
  @param[in] Description              The pointer to the description of the assert condition.

**/
VOID
EFIAPI
PeiDebugAssert (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  )
{
  DebugAssert (FileName, LineNumber, Description);
}

/**
  Entry point of Debug Service PEIM

  This funciton installs EDKII DEBUG PPI

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCESS  The entry point of Debug Service PEIM executes successfully.
  @retval Others      Some error occurs during the execution of this function.

**/
EFI_STATUS
EFIAPI
DebugSerivceInitialize (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  return PeiServicesInstallPpi (&mDebugServicePpi);
}

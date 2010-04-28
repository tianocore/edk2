/** @file
  OEM hook status code handler PEIM which produces general handler and hook it
  onto the PEI status code router.

  Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// The package level header files this module uses
//
#include <WinNtPeim.h>

//
// The protocols, PPI and GUID defintions for this module
//
#include <Ppi/ReportStatusCodeHandler.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/OemHookStatusCodeLib.h>

EFI_STATUS
EFIAPI
OemHookStatusCodeReportWrapper (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN       EFI_STATUS_CODE_TYPE     CodeType,
  IN       EFI_STATUS_CODE_VALUE    Value,
  IN       UINT32                   Instance,
  IN CONST EFI_GUID                 *CallerId, OPTIONAL
  IN CONST EFI_STATUS_CODE_DATA     *Data      OPTIONAL
  )
{
  return OemHookStatusCodeReport (
           CodeType,
           Value,
           Instance,
           (EFI_GUID *) CallerId,
           (EFI_STATUS_CODE_DATA *) Data
           );
}

/**
  Entry point of OEM hook status code handler PEIM.
  
  This function is the entry point of this OEM hook status code handler PEIM.
  It initializes and registers OEM status code handler.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCESS  The entry point executes successfully.

**/
EFI_STATUS
EFIAPI
WinNtOemHookStatusCodeHandlerPeiEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                  Status;
  EFI_PEI_RSC_HANDLER_PPI     *RscHandlerPpi;

  Status = PeiServicesLocatePpi (
             &gEfiPeiRscHandlerPpiGuid,
             0,
             NULL,
             (VOID **) &RscHandlerPpi
             );
  ASSERT_EFI_ERROR (Status);

  OemHookStatusCodeInitialize ();

  Status = RscHandlerPpi->Register (OemHookStatusCodeReportWrapper);                     
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

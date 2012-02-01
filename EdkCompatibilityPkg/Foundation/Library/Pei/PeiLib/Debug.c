/*++

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Debug.c

Abstract:

  Support for Debug primatives. 

--*/

#include "Tiano.h"
#include "Pei.h"
#include "EfiPrintLib.h"
#include "EfiStatusCode.h"
#include "EfiCommonLib.h"
#include EFI_GUID_DEFINITION (StatusCodeCallerId)
#include EFI_GUID_DEFINITION (StatusCodeDataTypeId)

VOID
PeiDebugAssert (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN CHAR8              *FileName,
  IN INTN               LineNumber,
  IN CHAR8              *Description
  )
/*++

Routine Description:

  Worker function for ASSERT(). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded DEADLOOP ().
  
Arguments:

  PeiServices - The PEI core services table.

  FileName    - File name of failing routine.

  LineNumber  - Line number of failing ASSERT().

  Description - Description, usually the assertion,
  
Returns:
  
  None

--*/
{
  UINT64  Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE];

  EfiDebugAssertWorker (FileName, LineNumber, Description, sizeof (Buffer), Buffer);

  //
  // We choose NOT to use PEI_REPORT_STATUS_CODE here, because when debug is enable,
  // we want get enough information if assert.
  //
  (**PeiServices).PeiReportStatusCode (
                    (EFI_PEI_SERVICES**)PeiServices,
                    (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED),
                    (EFI_SOFTWARE_PEI_MODULE | EFI_SW_EC_ILLEGAL_SOFTWARE_STATE),
                    0,
                    &gEfiCallerIdGuid,
                    (EFI_STATUS_CODE_DATA *) Buffer
                    );

  EFI_DEADLOOP ();
}


VOID
PeiDebugPrint (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN  UINTN             ErrorLevel,
  IN  CHAR8             *Format,
  ...
  )
/*++

Routine Description:

  Worker function for DEBUG(). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded do nothing.
  
Arguments:

  PeiServices - The PEI core services table.

  ErrorLevel - If error level is set do the debug print.

  Format     - String to use for the print, followed by Print arguments.

  ...        - Print arguments
  
Returns:
  
  None

--*/
{
  VA_LIST Marker;
  UINT64  Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE];

  VA_START (Marker, Format);
  EfiDebugVPrintWorker (ErrorLevel, Format, Marker, sizeof (Buffer), Buffer);
  VA_END (Marker);

  //
  // We choose NOT to use PEI_REPORT_STATUS_CODE here, because when debug is enable,
  // we want get enough information if assert.
  //
  (**PeiServices).PeiReportStatusCode (
                    (EFI_PEI_SERVICES**)PeiServices,
                    EFI_DEBUG_CODE,
                    (EFI_SOFTWARE_PEI_MODULE | EFI_DC_UNSPECIFIED),
                    0,
                    &gEfiCallerIdGuid,
                    (EFI_STATUS_CODE_DATA *) Buffer
                    );

}

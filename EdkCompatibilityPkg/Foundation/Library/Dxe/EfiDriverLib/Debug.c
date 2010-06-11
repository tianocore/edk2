/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
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
#include "EfiDriverLib.h"
#include "EfiPrintLib.h"
#include "EfiStatusCode.h"
#include EFI_GUID_DEFINITION (StatusCodeCallerId)
#include EFI_GUID_DEFINITION (StatusCodeDataTypeId)
#include EFI_PROTOCOL_DEFINITION (DebugMask)

//
// You would think you should divid by sizeof (UINT64), but EBC does not like
// that!
//
#define EFI_STATUS_CODE_DATA_MAX_SIZE64 (EFI_STATUS_CODE_DATA_MAX_SIZE / 8)

VOID
EfiDebugAssert (
  IN CHAR8    *FileName,
  IN INTN     LineNumber,
  IN CHAR8    *Description
  )
/*++

Routine Description:

  Worker function for ASSERT(). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded BREAKPOINT().

  We use UINT64 buffers due to IPF alignment concerns.

Arguments:

  FileName    - File name of failing routine.

  LineNumber  - Line number of failing ASSERT().

  Description - Descritption, usally the assertion,
  
Returns:
  
  None

--*/
{
  UINT64  Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE];

  EfiDebugAssertWorker (FileName, LineNumber, Description, sizeof (Buffer), Buffer);

  EfiLibReportStatusCode (
    (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED),
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_EC_ILLEGAL_SOFTWARE_STATE),
    0,
    &gEfiCallerIdGuid,
    (EFI_STATUS_CODE_DATA *) Buffer
    );

  //
  // Put dead loop in module that contained the error.
  //
  EFI_DEADLOOP ();
}

VOID
EfiDebugVPrint (
  IN  UINTN   ErrorLevel,
  IN  CHAR8   *Format,
  IN  VA_LIST Marker
  )
/*++

Routine Description:

  Worker function for DEBUG(). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded do nothing.

  We use UINT64 buffers due to IPF alignment concerns.

Arguments:

  ErrorLevel - If error level is set do the debug print.

  Format     - String to use for the print, followed by Print arguments.

  Marker     - VarArgs
  
Returns:
  
  None

--*/
{
  UINT64  Buffer[EFI_STATUS_CODE_DATA_MAX_SIZE];
  UINTN   ImageDebugMask;

  //
  // Check driver debug mask value and global mask
  //
  if (gDebugMaskInterface != NULL) {
    gDebugMaskInterface->GetDebugMask (gDebugMaskInterface, &ImageDebugMask);
    if (!(ErrorLevel & ImageDebugMask)) {
      return ;
    }
  } else if (!(gErrorLevel & ErrorLevel)) {
    return ;
  }

  EfiDebugVPrintWorker (ErrorLevel, Format, Marker, sizeof (Buffer), Buffer);

  EfiLibReportStatusCode (
    EFI_DEBUG_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_DC_UNSPECIFIED),
    (UINT32) ErrorLevel,
    &gEfiCallerIdGuid,
    (EFI_STATUS_CODE_DATA *) Buffer
    );

  return ;
}

VOID
EfiDebugPrint (
  IN  UINTN ErrorLevel,
  IN  CHAR8 *Format,
  ...
  )
/*++

Routine Description:

  Wrapper for EfiDebugVPrint ()
  
Arguments:

  ErrorLevel - If error level is set do the debug print.

  Format     - String to use for the print, followed by Print arguments.

  ...        - Print arguments.

  
Returns:
  
  None

--*/
{
  VA_LIST Marker;

  VA_START (Marker, Format);
  EfiDebugVPrint (ErrorLevel, Format, Marker);
  VA_END (Marker);
}

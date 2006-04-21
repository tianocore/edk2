/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StatusCode.c

Abstract:

  Status Code Architectural Protocol implementation as defined in Tiano
  Architecture Specification.

  This driver also depends on the DataHub, and will log all status code info 
  to the DataHub. Fatal Errors are Printed to Standard Error (StdErr) and not 
  logged to the data hub (If you crash what good is the data in the data hub).

  This driver has limited functionality at runtime and will not log to Data Hub
  at runtime.

  Notes:
  This driver assumes the following ReportStatusCode strategy:
  PEI       -> uses PeiReportStatusCode
  DXE IPL   -> uses PeiReportStatusCode
  early DXE -> uses PeiReportStatusCode via HOB
  DXE       -> This driver
  RT        -> This driver

--*/

#include "StatusCode.h"

EFI_LOCK  mStatusCodeLock;
BOOLEAN   mStatusCodeFlag = FALSE;

//
// Function implemenations
//


EFI_STATUS
EFIAPI
StatusCodeReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data OPTIONAL
  )
/*++

Routine Description:

  Calls into the platform library which dispatches the platform specific
  listeners. For NT environments we still call back into PEI because the 
  ReportStatusCode functionality requires Win32 services and is built into
  the SecMain.exe utility.

Arguments:

  (See Tiano Runtime Specification)

Returns:

  None

--*/
{
  EFI_STATUS  Status;

  //
  // Acquire the lock required to update mStatusCodeFlag
  //
  Status = EfiAcquireLockOrFail (&mStatusCodeLock);
  if (EFI_ERROR (Status)) {
    //
    // Check for reentrancy of the lock
    //
    return EFI_DEVICE_ERROR;
  }
  //
  // Check to see if we are already in the middle of a ReportStatusCode()
  //
  if (mStatusCodeFlag) {
    EfiReleaseLock (&mStatusCodeLock);
    return EFI_DEVICE_ERROR;
  }
  //
  // Set the flag to show we are in the middle of a ReportStatusCode()
  //
  mStatusCodeFlag = TRUE;

  //
  // Release the lock for updating mStatusCodeFlag
  //
  EfiReleaseLock (&mStatusCodeLock);

  //
  // Go do the work required to report a status code
  //
  RtPlatformReportStatusCode (CodeType, Value, Instance, CallerId, Data);

  //
  // Acquire the lock required to update mStatusCodeFlag
  //
  Status = EfiAcquireLockOrFail (&mStatusCodeLock);
  if (EFI_ERROR (Status)) {
    //
    // Check for reentrancy of the lock
    //
    return EFI_DEVICE_ERROR;
  }
  //
  // Clear the flag to show we are no longer in the middle of a ReportStatusCode()
  //
  mStatusCodeFlag = FALSE;

  //
  // Release the lock for updating mStatusCodeFlag
  //
  EfiReleaseLock (&mStatusCodeLock);

  return EFI_SUCCESS;
}
//
// Protocol instance, there can be only one.
//
EFI_STATUS
InitializeStatusCode (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:

  Install Driver to produce Report Status Code Arch Protocol

Arguments:

  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:

  EFI_SUCCESS - Logging Hub protocol installed
  Other       - No protocol installed, unload driver.

--*/
{

  EfiInitializeLock (&mStatusCodeLock, EFI_TPL_HIGH_LEVEL);

  //
  // Call the platform hook to initialize the different listeners.
  //
  RtPlatformStatusCodeInitialize ();

  //
  // Register a protocol that EfiUtilityLib can use to implement DEBUG () and ASSERT ()
  // Macros.
  //
  InstallStatusCodeDebugAssert ();

  return EFI_SUCCESS;
}

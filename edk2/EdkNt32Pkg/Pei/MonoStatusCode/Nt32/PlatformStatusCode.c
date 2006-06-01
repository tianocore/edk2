/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PlatformStatusCode.c 
   
Abstract:

  Contains NT32 specific implementations required to use status codes.

--*/

#include "../MonoStatusCode.h"


BOOLEAN gRunningFromMemory = FALSE;
//
// Platform definitions
//
EFI_PEI_REPORT_STATUS_CODE  mSecReportStatusCode = NULL;

extern EFI_PEI_PROGRESS_CODE_PPI  mStatusCodePpi;

//
// Function implementations
//
EFI_STATUS
EFIAPI
PlatformReportStatusCode (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
/*++

Routine Description:

  Call all status code listeners in the MonoStatusCode.

Arguments:

  Same as ReportStatusCode service
  
Returns:

  EFI_SUCCESS     Always returns success.

--*/
{
  if (mSecReportStatusCode != NULL) {
    mSecReportStatusCode (PeiServices, CodeType, Value, Instance, CallerId, Data);
  }
  MemoryReportStatusCode (CodeType, Value, Instance, CallerId, Data);
  return EFI_SUCCESS;
}

VOID
PlatformInitializeStatusCode (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
/*++

Routine Description:

  Initialize the status code listeners.  This consists of locating the 
  listener produced by SecMain.exe.

Arguments:

  FfsHeader   - FV this PEIM was loaded from.
  PeiServices - General purpose services available to every PEIM.

Returns:

  None

--*/
{
  EFI_STATUS                  Status;
  EFI_PEI_PROGRESS_CODE_PPI   *ReportStatusCodePpi;
  EFI_PEI_PPI_DESCRIPTOR      *ReportStatusCodeDescriptor;

  //
  // Cache the existing status code listener installed by the SEC core.
  // We should actually do a heap allocate, install a PPI, etc, but since we
  // know that we are running from a DLL, we can use global variables, and
  // directly update the status code PPI descriptor
  //
  //
  // Locate SEC status code PPI
  //
  Status = (*PeiServices)->LocatePpi (
                            PeiServices,
                            &gEfiPeiStatusCodePpiGuid,
                            0,
                            &ReportStatusCodeDescriptor,
                            &ReportStatusCodePpi
                            );
  if (EFI_ERROR (Status)) {
    return ;
  }

  mSecReportStatusCode            = ReportStatusCodePpi->ReportStatusCode;
  ReportStatusCodeDescriptor->Ppi = &mStatusCodePpi;

  //
  // Always initialize memory status code listener.
  //
  MemoryStatusCodeInitialize (FfsHeader, PeiServices);

}

EFI_STATUS
EFIAPI
InstallMonoStatusCode (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
/*++

Routine Description:

  Install the PEIM.  Publish the DXE callback as well.

Arguments:

  FfsHeader   - FV this PEIM was loaded from.
  PeiServices - General purpose services available to every PEIM.

Returns:

  EFI_SUCCESS   The function always returns success.

--*/
{
  if (!gRunningFromMemory) {
    //
    // First pass, running from flash, initialize everything
    //
    InitializeMonoStatusCode (FfsHeader, PeiServices);
  } else {
    //
    // Second pass, running from memory, initialize memory listener and
    // publish the DXE listener in a HOB.
    //
    MemoryStatusCodeInitialize (FfsHeader, PeiServices);
    InitializeDxeReportStatusCode (PeiServices);
  }

  return EFI_SUCCESS;
}

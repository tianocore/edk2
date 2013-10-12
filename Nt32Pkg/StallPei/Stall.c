/**@file
  EFI_PEI_STALL implementation for NT32 simulation environment.
  
Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/
#include "WinNtPeim.h"

#include <Ppi/NtThunk.h>
#include <Ppi/Stall.h>
#include <Library/DebugLib.h>

EFI_STATUS
EFIAPI 
Stall (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN CONST EFI_PEI_STALL_PPI    *This,
  IN UINTN                      Microseconds
  );
  
EFI_PEI_STALL_PPI mStallPpi = {1000, Stall};

EFI_PEI_PPI_DESCRIPTOR  mPpiListStall[1] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiStallPpiGuid,
    &mStallPpi
  }
};


/**
  PEIM's entry point.
  
  This routine installs the simulation instance of EFI_PEI_STALL_PPI based
  on Win API Sleep().
  
  @param  FileHandle  Handle of the file being invoked. 
  @param  PeiServices Describes the list of possible PEI Services.

  @retval  EFI_SUCCESS   The PEIM executed normally.
  @retval  !EFI_SUCCESS  The PEIM failed to execute normally.
**/
EFI_STATUS
EFIAPI
InitializeStall (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;
  Status = (*PeiServices)->InstallPpi (PeiServices, &mPpiListStall[0]);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  The Stall() function provides a blocking stall for at least the number 
  of microseconds stipulated in the final argument of the API.

  @param  PeiServices    An indirect pointer to the PEI Services Table
                         published by the PEI Foundation.
  @param  This           Pointer to the local data for the interface.
  @param  Microseconds   Number of microseconds for which to stall.

  @retval EFI_SUCCESS    The service provided at least the required delay.

**/
EFI_STATUS
EFIAPI 
Stall (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN CONST EFI_PEI_STALL_PPI    *This,
  IN UINTN                      Microseconds
  )
{
  EFI_STATUS                Status;
  PEI_NT_THUNK_PPI          *PeiNtService;
  EFI_WIN_NT_THUNK_PROTOCOL *NtThunk;
  
  Status = (**PeiServices).LocatePpi (
                            (const EFI_PEI_SERVICES **)PeiServices,
                            &gPeiNtThunkPpiGuid,
                            0,                  
                            NULL,
                            (VOID**)&PeiNtService
                            );
  ASSERT_EFI_ERROR (Status);
    
  //
  // Calculate the time to sleep.  Win API smallest unit to sleep is 1 millisec
  // so micro second units need be divided by 1000 to convert to ms
  //
  NtThunk = (EFI_WIN_NT_THUNK_PROTOCOL*) PeiNtService->NtThunk();
  NtThunk->Sleep ((DWORD)((Microseconds + 999) / 1000)); 
  
  return EFI_SUCCESS;
}

/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    WinNtStuff.c
    
Abstract:

    Tiano PEIM to abstract construction of firmware volume in a Windows NT environment.

Revision History

**/

//
// The package level header files this module uses
//
#include <PiPei.h>
#include <WinNtPeim.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Protocol/WinNtThunk.h>
#include <Ppi/NtThunk.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/HobLib.h>

EFI_STATUS
EFIAPI
PeimInitializeWinNtThunkPPIToProtocolPeim (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
/*++

Routine Description:

  Perform a call-back into the SEC simulator to get NT Stuff

Arguments:

  PeiServices - General purpose services available to every PEIM.
    
Returns:

  None

--*/
// TODO:    FfsHeader - add argument and description to function comment
{
  EFI_STATUS              Status;
  EFI_PEI_PPI_DESCRIPTOR  *PpiDescriptor;
  PEI_NT_THUNK_PPI        *PeiNtService;
  VOID                    *Ptr;

  DEBUG ((EFI_D_ERROR, "NT 32 WinNT Stuff PEIM Loaded\n"));

  Status = (**PeiServices).LocatePpi (
                            (const EFI_PEI_SERVICES **)PeiServices,
                            &gPeiNtThunkPpiGuid,  // GUID
                            0,                    // INSTANCE
                            &PpiDescriptor,       // EFI_PEI_PPI_DESCRIPTOR
                            (VOID**)&PeiNtService         // PPI
                            );
  ASSERT_EFI_ERROR (Status);

  Ptr = PeiNtService->NtThunk ();

  BuildGuidDataHob (
    &gEfiWinNtThunkProtocolGuid,         // Guid
    &Ptr,                                // Buffer
    sizeof (VOID *)                      // Sizeof Buffer
    );
  return Status;
}

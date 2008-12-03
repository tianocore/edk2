/*++

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    UnixStuff.c
    
Abstract:

    Tiano PEIM to abstract construction of firmware volume in a Unix environment.

Revision History

--*/

#include "PiPei.h"
#include <Ppi/UnixThunk.h>
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>

EFI_STATUS
EFIAPI
PeimInitializeUnixStuff (
  IN       EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
/*++

Routine Description:

  Perform a call-back into the SEC simulator to get Unix Stuff

Arguments:

  PeiServices - General purpose services available to every PEIM.
    
Returns:

  None

--*/
// TODO:    FfsHeader - add argument and description to function comment
{
  EFI_STATUS              Status;
  EFI_PEI_PPI_DESCRIPTOR  *PpiDescriptor;
  PEI_UNIX_THUNK_PPI     *PeiUnixService;
  VOID                    *Ptr;

  DEBUG ((EFI_D_ERROR, "Unix Stuff PEIM Loaded\n"));

  Status = (**PeiServices).LocatePpi (
                            PeiServices,
                            &gPeiUnixThunkPpiGuid,  // GUID
                            0,                    // INSTANCE
                            &PpiDescriptor,       // EFI_PEI_PPI_DESCRIPTOR
                            (VOID **)&PeiUnixService         // PPI
                            );
  ASSERT_EFI_ERROR (Status);

  Ptr = PeiUnixService->UnixThunk ();

  BuildGuidDataHob (
    &gEfiUnixThunkProtocolGuid,         // Guid
    &Ptr,                                // Buffer
    sizeof (VOID *)                      // Sizeof Buffer
    );
  return Status;
}

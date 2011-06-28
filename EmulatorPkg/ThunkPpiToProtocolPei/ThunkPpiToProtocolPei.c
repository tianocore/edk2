/*++ @file
  UEFI/PI PEIM to abstract construction of firmware volume in a Unix environment.

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010 - 2011, Apple Inc. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>

#include <Ppi/EmuThunk.h>
#include <Protocol/EmuThunk.h>



EFI_STATUS
EFIAPI
PeiInitialzeThunkPpiToProtocolPei (
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

**/
{
  EFI_STATUS              Status;
  EFI_PEI_PPI_DESCRIPTOR  *PpiDescriptor;
  EMU_THUNK_PPI           *Thunk;
  VOID                    *Ptr;

  DEBUG ((EFI_D_ERROR, "Emu Thunk PEIM Loaded\n"));

  Status = PeiServicesLocatePpi (
              &gEmuThunkPpiGuid,        // GUID
              0,                        // INSTANCE
              &PpiDescriptor,           // EFI_PEI_PPI_DESCRIPTOR
              (VOID **)&Thunk           // PPI
              );
  ASSERT_EFI_ERROR (Status);

  Ptr = Thunk->Thunk ();

  BuildGuidDataHob (
    &gEmuThunkProtocolGuid,              // Guid
    &Ptr,                                // Buffer
    sizeof (VOID *)                      // Sizeof Buffer
    );
  return Status;
}

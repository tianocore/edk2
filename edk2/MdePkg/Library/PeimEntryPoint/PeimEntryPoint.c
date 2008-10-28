/** @file
  Entry point to a PEIM.

Copyright (c) 2006, Intel Corporation<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiPei.h>


#include <Library/PeimEntryPoint.h>
#include <Library/DebugLib.h>

/**
  Image entry point of Peim.

  @param  FfsHeader   Pointer to FFS header the loaded driver.
  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation

  @return  Status returned by entry points of Peims.

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  if (_gPeimRevision != 0) {
    //
    // Make sure that the PEI spec revision of the platform is >= PEI spec revision of the driver
    //
    ASSERT ((*PeiServices)->Hdr.Revision >= _gPeimRevision);
  }

  //
  // Call constructor for all libraries
  //
  ProcessLibraryConstructorList (FileHandle, PeiServices);

  //
  // Call the driver entry point
  //
  return ProcessModuleEntryPointList (FileHandle, PeiServices);
}


/**
  Wrapper of Peim image entry point.

  @param  FfsHeader   Pointer to FFS header the loaded driver.
  @param  PeiServices An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation

  @return  Status returned by entry points of Peims.

**/
EFI_STATUS
EFIAPI
EfiMain (
  IN EFI_PEI_FILE_HANDLE      FileHandle,
  IN CONST EFI_PEI_SERVICES   **PeiServices
  )
{
  return _ModuleEntryPoint (FileHandle, PeiServices);
}

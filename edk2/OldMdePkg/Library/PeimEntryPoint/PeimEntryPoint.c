
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

/**
  Image entry point of Peim.

  @param  FfsHeader   Pointer to FFS header the loaded driver.
  @param  PeiServices Pointer to the PEI services.

  @return  Status returned by entry points of Peims.

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
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
  ProcessLibraryConstructorList (FfsHeader, PeiServices);

  //
  // Call the driver entry point
  //
  return ProcessModuleEntryPointList (FfsHeader, PeiServices);
}


/**
  Wrapper of Peim image entry point.

  @param  FfsHeader   Pointer to FFS header the loaded driver.
  @param  PeiServices Pointer to the PEI services.

  @return  Status returned by entry points of Peims.

**/
EFI_STATUS
EFIAPI
EfiMain (
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  IN EFI_PEI_SERVICES     **PeiServices
  )
{
  return _ModuleEntryPoint (FfsHeader, PeiServices);
}

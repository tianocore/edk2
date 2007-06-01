/** @file
  Entry point to a the PEI Core.

Copyright (c) 2006, Intel Corporation<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

/**
  Enrty point to PEI core.

  @param  PeiStartupDescriptor Pointer of start up information.
 
  @return Status returned by entry points of core and drivers. 

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor
  )
{
  //
  // Call the PEI Core entry point
  //
  return ProcessModuleEntryPointList (PeiStartupDescriptor, NULL);
}


/**
  Wrapper of enrty point to PEI core.

  @param  PeiStartupDescriptor Pointer of start up information.
 
  @return Status returned by entry points of core and drivers. 

**/
EFI_STATUS
EFIAPI
EfiMain (
  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor
  )
{
  return _ModuleEntryPoint (PeiStartupDescriptor);
}

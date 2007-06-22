/** @file
  Entry point to the DXE Core.

Copyright (c) 2006, Intel Corporation<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

//
// Cache copy of HobList pointer. 
// 
VOID *gHobList = NULL;

/**
  Enrty point to DXE core.

  @param  HobStart Pointer of HobList.

**/
VOID
EFIAPI
_ModuleEntryPoint (
  IN VOID  *HobStart
  )
{
  //
  // Cache a pointer to the HobList
  //
  gHobList = HobStart;

  //
  // Call the DXE Core entry point
  //
  ProcessModuleEntryPointList (HobStart);

  //
  // Should never return
  //
  ASSERT(FALSE);
  CpuDeadLoop ();
}


/**
  Wrapper of enrty point to DXE CORE.

  @param  HobStart Pointer of HobList.

**/
VOID
EFIAPI
EfiMain (
  IN VOID  *HobStart
  )
{
  _ModuleEntryPoint (HobStart);
}

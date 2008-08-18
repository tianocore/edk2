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


#include <PiDxe.h>


#include <Library/DxeCoreEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>

//
// Cache copy of HobList pointer. 
// 
VOID *gHobList = NULL;

/**
  Enrty point to DXE core.

  This function is the entry point to the DXE Foundation. The PEI phase, which executes just before
  DXE, is responsible for loading and invoking the DXE Foundation in system memory. The only
  parameter that is passed to the DXE Foundation is HobStart. This parameter is a pointer to the
  HOB list that describes the system state at the hand-off to the DXE Foundation. At a minimum,
  this system state must include the following:
    - PHIT HOB
    - CPU HOB
    - Description of system memory
    - Description of one or more firmware volumes
  The DXE Foundation is also guaranteed that only one processor is running and that the processor is
  running with interrupts disabled. The implementation of the DXE Foundation must not make any
  assumptions about where the DXE Foundation will be loaded or where the stack is located. In
  general, the DXE Foundation should make as few assumptions about the state of the system as
  possible. This lack of assumptions will allow the DXE Foundation to be portable to the widest
  variety of system architectures.
  
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

/** @file
  Defines PXE Arch type.
  
Copyright (c) 2007, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "PxeArch.h"

UINT16 mSysArch = 0;
/**
  Get the system architecture type.

  @return system architecture type.

**/
UINT16
GetSysArch (
  VOID
  )
{
  if (mSysArch == 0) {
    //
    // This is first call
    // Assign to invalid value
    //
    mSysArch = 0xFFFF;

    //
    // We do not know what is EBC architecture.
    // Maybe we can try to locate DebugSupport protocol to get ISA.
    // TBD now.
    //
  }

  return mSysArch;
}

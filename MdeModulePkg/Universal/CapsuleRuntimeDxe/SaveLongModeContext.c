/** @file
  Create the NULL function to pass build in IA32/IPF/ARM/EBC.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

/**
  Only when PEI is IA32 and DXE is X64, we need transfer to long mode in PEI
  in order to process capsule data above 4GB. So create a NULL function here for
  other cases.
**/
VOID
SaveLongModeContext (
  VOID
  )
{
}

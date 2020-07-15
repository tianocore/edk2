/** @file
  Create the NULL function to pass build in IA32/IPF/ARM/EBC.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

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

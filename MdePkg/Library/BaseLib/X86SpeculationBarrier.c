/** @file
  SpeculationBarrier() function for IA32 and x64.

  Copyright (C) 2018, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseLib.h>

/**
  Uses as a barrier to stop speculative execution.

  Ensures that no later instruction will execute speculatively, until all prior
  instructions have completed.

**/
VOID
EFIAPI
SpeculationBarrier (
  VOID
  )
{
  AsmLfence ();
}

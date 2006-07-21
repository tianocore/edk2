/** @file
  IA-32/x64 AsmReadIdtr()

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  x86ReadIdtr.c

**/

#include "BaseLibInternals.h"

/**
  Reads the current Interrupt Descriptor Table Register(GDTR) descriptor.

  Reads and returns the current IDTR descriptor and returns it in Idtr. This
  function is only available on IA-32 and X64.

  If Idtr is NULL, then ASSERT().

  @param  Idtr  Pointer to a IDTR descriptor.

**/
VOID
EFIAPI
AsmReadIdtr (
  OUT     IA32_DESCRIPTOR           *Idtr
  )
{
  ASSERT (Idtr != NULL);
  InternalX86ReadIdtr (Idtr);
}

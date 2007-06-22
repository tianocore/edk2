/** @file
  IA-32/x64 AsmWriteIdtr()

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  x86WriteIdtr.c

**/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "BaseLibInternals.h"

/**
  Writes the current Interrupt Descriptor Table Register(GDTR) descriptor.

  Writes the current IDTR descriptor and returns it in Idtr. This function is
  only available on IA-32 and X64.

  If Idtr is NULL, then ASSERT().

  @param  Idtr  Pointer to a IDTR descriptor.

**/
VOID
EFIAPI
AsmWriteIdtr (
  IN      CONST IA32_DESCRIPTOR     *Idtr
  )
{
  ASSERT (Idtr != NULL);
  InternalX86WriteIdtr (Idtr);
}

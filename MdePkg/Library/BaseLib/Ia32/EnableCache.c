/** @file
  AsmEnableCache function

  Copyright (c) 2006 - 2008, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

/**
  Enabled caches.

  Flush all caches with a WBINVD instruction, clear the CD bit of CR0 to 0, and clear 
  the NW bit of CR0 to 0

**/
VOID
EFIAPI
AsmEnableCache (
  VOID
  )
{
  _asm {
    wbinvd
    mov     eax, cr0
    btr     eax, 30
    btr     eax, 29
    mov     cr0, eax
  }
}


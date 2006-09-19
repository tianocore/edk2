/** @file
  AsmMonitor function

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#if _MSC_EXTENSIONS

UINTN
EFIAPI
AsmMonitor (
  IN      UINTN                     RegisterEax,
  IN      UINTN                     RegisterEcx,
  IN      UINTN                     RegisterEdx
  )
{
  _asm {
    mov     eax, RegisterEax
    mov     ecx, RegisterEcx
    mov     edx, RegisterEdx
    _emit   0x0f             // monitor
    _emit   0x01
    _emit   0xc8
  }
}

#endif

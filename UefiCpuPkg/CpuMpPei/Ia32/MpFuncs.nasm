;------------------------------------------------------------------------------ ;
; Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   MpFuncs.nasm
;
; Abstract:
;
;   This is the assembly code for MP support
;
;-------------------------------------------------------------------------------

%include "MpEqu.inc"

SECTION .text


global ASM_PFX(AsmInitializeGdt)
ASM_PFX(AsmInitializeGdt):
  push         ebp
  mov          ebp, esp
  pushad
  mov          edi, [ebp + 8]      ; Load GDT register

  lgdt         [edi]      ; and update the GDTR

  push         PROTECT_MODE_CS
  mov          eax, ASM_PFX(SetCodeSelectorFarJump)
  push         eax
  retf
ASM_PFX(SetCodeSelectorFarJump):
  mov          ax, PROTECT_MODE_DS ; Update the Base for the new selectors, too
  mov          ds, ax
  mov          es, ax
  mov          fs, ax
  mov          gs, ax
  mov          ss, ax

  popad
  pop          ebp
  ret

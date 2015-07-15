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
;   MpFuncs32.asm
;
; Abstract:
;
;   This is the assembly code for MP support
;
;-------------------------------------------------------------------------------

.686p
.model  flat

include  MpEqu.inc
.code


AsmInitializeGdt   PROC  near C  PUBLIC
  push         ebp
  mov          ebp, esp
  pushad
  mov          edi, [ebp + 8]      ; Load GDT register

  mov          ax,cs               ; Get the selector data from our code image
  mov          es,ax
  lgdt         FWORD PTR es:[edi]  ; and update the GDTR

  push         PROTECT_MODE_CS
  lea          eax, SetCodeSelectorFarJump
  push         eax
  retf
SetCodeSelectorFarJump:
  mov          ax, PROTECT_MODE_DS ; Update the Base for the new selectors, too
  mov          ds, ax
  mov          es, ax
  mov          fs, ax
  mov          gs, ax
  mov          ss, ax

  popad
  pop          ebp
  ret
AsmInitializeGdt  ENDP

END

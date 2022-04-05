/** @file

  ASM helper stub to invoke hypercall

  Copyright (c) 2021, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DEFAULT REL
SECTION .text

; UINTN
; EFIAPI
; SetMemoryEncDecHypercall3AsmStub (
;   IN UINTN HypercallNum,
;   IN UINTN Arg1,
;   IN UINTN Arg2,
;   IN UINTN Arg3
;   );
global ASM_PFX(SetMemoryEncDecHypercall3AsmStub)
ASM_PFX(SetMemoryEncDecHypercall3AsmStub):
  ; UEFI calling conventions require RBX to
  ; be nonvolatile/callee-saved.
  push rbx
  mov rax, rcx    ; Copy HypercallNumber to rax
  mov rbx, rdx    ; Copy Arg1 to the register expected by KVM
  mov rcx, r8     ; Copy Arg2 to register expected by KVM
  mov rdx, r9     ; Copy Arg3 to register expected by KVM
  vmmcall         ; Call VMMCALL
  pop rbx
  ret

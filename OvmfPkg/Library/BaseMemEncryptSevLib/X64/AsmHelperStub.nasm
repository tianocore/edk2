DEFAULT REL
SECTION .text

; INTN
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
  mov rdx, r9     ; Copy Arg2 to register expected by KVM
  vmmcall         ; Call VMMCALL
  pop rbx
  ret

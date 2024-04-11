DEFAULT REL
SECTION .text

; INTN
; EFIAPI
; __XenVmmcall2 (
;   IN     INTN HypercallNum,
;   IN OUT INTN Arg1,
;   IN OUT INTN Arg2
;   );
global ASM_PFX(__XenVmmcall2)
ASM_PFX(__XenVmmcall2):
  push rdi
  push rsi
  ; Copy HypercallNum to rax
  mov rax, rcx
  ; Copy Arg1 to the register expected by Xen
  mov rdi, rdx
  ; Copy Arg2 to the register expected by Xen
  mov rsi, r8
  ; Call HypercallNum
  vmmcall
  pop rsi
  pop rdi
  ret

; INTN
; EFIAPI
; __XenVmcall2 (
;   IN     INTN HypercallNum,
;   IN OUT INTN Arg1,
;   IN OUT INTN Arg2
;   );
global ASM_PFX(__XenVmcall2)
ASM_PFX(__XenVmcall2):
  push rdi
  push rsi
  ; Copy HypercallNum to rax
  mov rax, rcx
  ; Copy Arg1 to the register expected by Xen
  mov rdi, rdx
  ; Copy Arg2 to the register expected by Xen
  mov rsi, r8
  ; Call HypercallNum
  vmcall
  pop rsi
  pop rdi
  ret


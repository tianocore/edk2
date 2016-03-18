DEFAULT REL
SECTION .text

; INTN
; EFIAPI
; __XenHypercall2 (
;   IN     VOID *HypercallAddr,
;   IN OUT INTN Arg1,
;   IN OUT INTN Arg2
;   );
global ASM_PFX(__XenHypercall2)
ASM_PFX(__XenHypercall2):
  push rdi
  push rsi
  ; Copy HypercallAddr to rax
  mov rax, rcx
  ; Copy Arg1 to the register expected by Xen
  mov rdi, rdx
  ; Copy Arg2 to the register expected by Xen
  mov rsi, r8
  ; Call HypercallAddr
  call rax
  pop rsi
  pop rdi
  ret


.code

; INTN
; EFIAPI
; XenHypercall2 (
;   IN     VOID *HypercallAddr,
;   IN OUT INTN Arg1,
;   IN OUT INTN Arg2
;   );
XenHypercall2 PROC
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
XenHypercall2 ENDP

END

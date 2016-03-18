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
  ; Save only ebx, ecx is supposed to be a scratch register and needs to be
  ; saved by the caller
  push ebx
  ; Copy HypercallAddr to eax
  mov eax, [esp + 8]
  ; Copy Arg1 to the register expected by Xen
  mov ebx, [esp + 12]
  ; Copy Arg2 to the register expected by Xen
  mov ecx, [esp + 16]
  ; Call HypercallAddr
  call eax
  pop ebx
  ret


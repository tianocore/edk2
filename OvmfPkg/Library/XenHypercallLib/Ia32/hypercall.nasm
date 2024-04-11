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
  ; Save only ebx, ecx is supposed to be a scratch register and needs to be
  ; saved by the caller
  push ebx
  ; Copy HypercallNum to eax
  mov eax, [esp + 8]
  ; Copy Arg1 to the register expected by Xen
  mov ebx, [esp + 12]
  ; Copy Arg2 to the register expected by Xen
  mov ecx, [esp + 16]
  ; Call Hypercall
  vmmcall
  pop ebx
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
  ; Save only ebx, ecx is supposed to be a scratch register and needs to be
  ; saved by the caller
  push ebx
  ; Copy HypercallNum to eax
  mov eax, [esp + 8]
  ; Copy Arg1 to the register expected by Xen
  mov ebx, [esp + 12]
  ; Copy Arg2 to the register expected by Xen
  mov ecx, [esp + 16]
  ; Call Hypercall
  vmcall
  pop ebx
  ret


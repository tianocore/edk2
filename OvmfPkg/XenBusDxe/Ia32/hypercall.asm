.code

; INTN
; EFIAPI
; XenHypercall2 (
;   IN     VOID *HypercallAddr,
;   IN OUT INTN Arg1,
;   IN OUT INTN Arg2
;   );
XenHypercall2 PROC
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
XenHypercall2 ENDP

END

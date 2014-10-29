.code

; INT32
; EFIAPI
; TestAndClearBit (
;   IN  INT32 Bit,                // rcx
;   IN  volatile VOID* Address    // rdx
;   );
TestAndClearBit PROC
  lock
  btr [rdx], ecx
  sbb eax, eax
  ret
TestAndClearBit ENDP

END

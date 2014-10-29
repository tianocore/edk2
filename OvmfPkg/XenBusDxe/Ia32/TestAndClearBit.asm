.code

; INT32
; EFIAPI
; TestAndClearBit (
;   IN  INT32 Bit,
;   IN  volatile VOID* Address
;   );
TestAndClearBit PROC
  mov ecx, [esp + 4]
  mov edx, [esp + 8]
  lock btr [edx], ecx
  sbb eax, eax
  ret
TestAndClearBit ENDP

END

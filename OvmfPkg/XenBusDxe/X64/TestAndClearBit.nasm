DEFAULT REL
SECTION .text

; INT32
; EFIAPI
; TestAndClearBit (
;   IN  INT32 Bit,                // rcx
;   IN  volatile VOID* Address    // rdx
;   );
global ASM_PFX(TestAndClearBit)
ASM_PFX(TestAndClearBit):
  lock btr [rdx], ecx
  sbb eax, eax
  ret


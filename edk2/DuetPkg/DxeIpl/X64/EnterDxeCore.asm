      TITLE   EnterDxeCore.asm: Assembly code for the entering DxeCore
;------------------------------------------------------------------------------
;*
;*   Copyright 2006, Intel Corporation                                                         
;*   All rights reserved. This program and the accompanying materials                          
;*   are licensed and made available under the terms and conditions of the BSD License         
;*   which accompanies this distribution.  The full text of the license may be found at        
;*   http://opensource.org/licenses/bsd-license.php                                            
;*                                                                                             
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;*   
;*    EnterDxeCore.asm
;*  
;*   Abstract:
;*
;------------------------------------------------------------------------------

.code

;
; VOID
; EnterDxeMain (
;   IN VOID *StackTop,           // rcx
;   IN VOID *DxeCoreEntryPoint,  // rdx
;   IN VOID *Hob,                // r8
;   IN VOID *PageTable           // r9
;   )
;
EnterDxeMain    PROC
  
  mov   cr3, r9
  sub   rcx, 32
  mov   rsp, rcx
  mov   rcx, r8
  push  0
  jmp   rdx

; should never get here
  jmp $
  ret

EnterDxeMain    ENDP

END

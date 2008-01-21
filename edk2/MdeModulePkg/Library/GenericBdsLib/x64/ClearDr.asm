  title   ClearDr.asm
;------------------------------------------------------------------------------
;
; Copyright (c) 2005, Intel Corporation                                                         
; All rights reserved. This program and the accompanying materials                          
; are licensed and made available under the terms and conditions of the BSD License         
; which accompanies this distribution.  The full text of the license may be found at        
; http://opensource.org/licenses/bsd-license.php                                            
;                                                                                           
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
; 
; Module Name:
;
;   ClearDr.asm
; 
; Abstract:
; 
;   Clear dr0 dr1 register
;
;------------------------------------------------------------------------------

text SEGMENT

;------------------------------------------------------------------------------
;  VOID
;  ClearDebugRegisters (
;    VOID
;    )
;------------------------------------------------------------------------------
ClearDebugRegisters PROC    PUBLIC
    push   rax
    xor    rax, rax
    mov    dr0, rax
    mov    dr1, rax
    pop    rax
    ret
ClearDebugRegisters ENDP

END


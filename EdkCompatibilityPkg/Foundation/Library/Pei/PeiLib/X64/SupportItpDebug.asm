;------------------------------------------------------------------------------
;
; Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials                          
; are licensed and made available under the terms and conditions of the BSD License         
; which accompanies this distribution.  The full text of the license may be found at        
; http://opensource.org/licenses/bsd-license.php                                            
;                                                                                           
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.     
;
; Module Name:
;
;   SupportItpDebug.asm
; 
; Abstract:
; 
;   This is the code for debuging X64, to add a break hook at loading every module
;
;------------------------------------------------------------------------------

; PROC:PRIVATE
  .CODE

;------------------------------------------------------------------------------
;  VOID
;  AsmEfiSetBreakSupport (
;    IN UINTN  LoadAddr    // rcx
;    )
;------------------------------------------------------------------------------

AsmEfiSetBreakSupport  PROC    PUBLIC

    mov dx,  60000
    out dx,  eax
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    ret
    
AsmEfiSetBreakSupport  ENDP
  END


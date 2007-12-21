  title   CpuIoAccess.asm
;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2007, Intel Corporation                                                         
; All rights reserved. This program and the accompanying materials                          
; are licensed and made available under the terms and conditions of the BSD License         
; which accompanies this distribution.  The full text of the license may be found at        
; http://opensource.org/licenses/bsd-license.php                                            
;                                                                                           
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
; 
; Module Name:
;   CpuIoAccess.asm
; 
; Abstract:
;   CPU IO Abstraction
;
;------------------------------------------------------------------------------


.code

;------------------------------------------------------------------------------
;  UINT8
;  CpuIoRead8 (
;    UINT16  Port   // rcx
;    )
;------------------------------------------------------------------------------
CpuIoRead8 PROC        PUBLIC
    xor   eax, eax
    mov    dx, cx
    in     al, dx
    ret
CpuIoRead8  ENDP

;------------------------------------------------------------------------------
;  VOID
;  CpuIoWrite8 (
;    UINT16  Port,    // rcx
;    UINT32  Data     // rdx
;    )
;------------------------------------------------------------------------------
CpuIoWrite8 PROC        PUBLIC
	  mov   eax, edx
    mov    dx, cx
    out    dx, al
    ret
CpuIoWrite8  ENDP


END

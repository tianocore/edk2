  title   CpuIoAccess.asm

;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2005 - 2007, Intel Corporation                                                         
;*   All rights reserved. This program and the accompanying materials                          
;*   are licensed and made available under the terms and conditions of the BSD License         
;*   which accompanies this distribution.  The full text of the license may be found at        
;*   http://opensource.org/licenses/bsd-license.php                                            
;*                                                                                             
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;*   
;*   Module Name:
;*    CpuIoAccess.asm
;*  
;*   Abstract:
;*    Supports x64 CPU IO operation
;*
;------------------------------------------------------------------------------
;
;   
; 
; Abstract:
; 
;   
;------------------------------------------------------------------------------

.CODE

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

;------------------------------------------------------------------------------
;  UINT16
;  CpuIoRead16 (
;    UINT16  Port   // rcx
;    )
;------------------------------------------------------------------------------
CpuIoRead16 PROC        PUBLIC
    xor   eax, eax
    mov    dx, cx
    in     ax, dx
    ret
CpuIoRead16  ENDP

;------------------------------------------------------------------------------
;  VOID
;  CpuIoWrite16 (
;    UINT16  Port,    // rcx
;    UINT32  Data     // rdx
;    )
;------------------------------------------------------------------------------
CpuIoWrite16 PROC        PUBLIC
    mov   eax, edx
    mov    dx, cx
    out    dx, ax
    ret
CpuIoWrite16  ENDP

;------------------------------------------------------------------------------
;  UINT32
;  CpuIoRead32 (
;    UINT16  Port   // rcx
;    )
;------------------------------------------------------------------------------
CpuIoRead32 PROC        PUBLIC
    mov    dx, cx
    in    eax, dx
    ret
CpuIoRead32  ENDP

;------------------------------------------------------------------------------
;  VOID
;  CpuIoWrite32 (
;    UINT16  Port,    // rcx
;    UINT32  Data     // rdx
;    )
;------------------------------------------------------------------------------
CpuIoWrite32 PROC        PUBLIC
    mov   eax, edx
    mov    dx, cx
    out    dx, eax
    ret
CpuIoWrite32  ENDP

END

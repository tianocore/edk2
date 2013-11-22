      TITLE   CpuAsm.asm: 
;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2008 - 2013, Intel Corporation. All rights reserved.<BR>
;*   This program and the accompanying materials                          
;*   are licensed and made available under the terms and conditions of the BSD License         
;*   which accompanies this distribution.  The full text of the license may be found at        
;*   http://opensource.org/licenses/bsd-license.php                                            
;*                                                                                             
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;*   
;*    CpuAsm.asm
;*  
;*   Abstract:
;*
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
; VOID
; SetCodeSelector (
;   UINT16 Selector
;   );
;------------------------------------------------------------------------------
SetCodeSelector PROC PUBLIC
    sub     rsp, 0x10
    lea     rax, setCodeSelectorLongJump
    mov     [rsp], rax
    mov     [rsp+4], cx
    jmp     fword ptr [rsp]
setCodeSelectorLongJump:
    add     rsp, 0x10
    ret
SetCodeSelector ENDP

;------------------------------------------------------------------------------
; VOID
; SetDataSelectors (
;   UINT16 Selector
;   );
;------------------------------------------------------------------------------
SetDataSelectors PROC PUBLIC
    mov     ss, cx
    mov     ds, cx
    mov     es, cx
    mov     fs, cx
    mov     gs, cx
    ret
SetDataSelectors ENDP

END


;*****************************************************************************
;*
;*   Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
;*   This program and the accompanying materials                          
;*   are licensed and made available under the terms and conditions of the BSD License         
;*   which accompanies this distribution.  The full text of the license may be found at        
;*   http://opensource.org/licenses/bsd-license.php                                            
;*                                                                                             
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;*   
;*   Module Name:
;*
;*    Fx.asm
;*  
;*   Abstract:
;*  
;*    AsmFxRestore and AsmFxSave function
;*  
;*****************************************************************************

    .586P
    .model  flat,C
    .xmm
    .code

;------------------------------------------------------------------------------
; VOID
; AsmFxSave (
;   OUT IA32_FX_BUFFER *Buffer
;   );
;------------------------------------------------------------------------------
AsmFxSave PROC
    mov     eax, [esp + 4]
    fxsave  [eax]
    ret
AsmFxSave ENDP

;------------------------------------------------------------------------------
; VOID
; AsmFxRestore (
;   IN CONST IA32_FX_BUFFER *Buffer
;   );
;------------------------------------------------------------------------------
AsmFxRestore  PROC
    mov     eax, [esp + 4]
    fxrstor [eax]
    ret
AsmFxRestore  ENDP

;------------------------------------------------------------------------------
; UINTN
; AsmGetEflags (
;   VOID
;   );
;------------------------------------------------------------------------------
AsmGetEflags PROC
    pushfd
    pop   eax
    ret
AsmGetEflags ENDP

;------------------------------------------------------------------------------
; VOID
; AsmSetEflags (
;   IN UINTN   Eflags
;   );
;------------------------------------------------------------------------------
AsmSetEflags PROC
    push  [esp + 4]
    popfd
    ret
AsmSetEflags ENDP

    END

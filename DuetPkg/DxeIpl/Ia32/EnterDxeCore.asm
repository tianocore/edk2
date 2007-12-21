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

.686p
.model  flat        

.code
.stack
.MMX
.XMM

;
; VOID
; EnterDxeMain (
;   IN VOID *StackTop,
;   IN VOID *DxeCoreEntryPoint,
;   IN VOID *Hob,
;   IN VOID *PageTable
;   )
;
EnterDxeMain    PROC  C    \
  StackTop:DWORD,          \
  DxeCoreEntryPoint:DWORD, \
  Hob:DWORD,               \
  PageTable:DWORD
  
  mov   eax, PageTable
;  mov   cr3, eax     ; load page table
;  mov   eax, cr4
;  bts   eax, 4       ; enable CR4.PSE
;  mov   cr4, eax
;  mov   eax, cr0
;  bts   eax, 31      ; enable CR0.PG
;  mov   cr0, eax
  mov   ecx, DxeCoreEntryPoint
  mov   eax, StackTop
  mov   esp, eax
  mov   edx, Hob
  push  edx
  push  0
  jmp   ecx

; should never get here
  jmp $
  ret

EnterDxeMain    ENDP

END

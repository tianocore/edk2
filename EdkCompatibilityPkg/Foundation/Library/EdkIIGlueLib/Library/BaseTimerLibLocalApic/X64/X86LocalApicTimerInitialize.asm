; Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
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
;   x86LocalApicTimerInitialize.Asm
;
; Abstract:
;
;   Initialize Local Apic Timer
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

EXTERNDEF  InternalX86GetApicBase:PROC

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; CpuInitLocalApicTimer (
;   VOID
;   );
;------------------------------------------------------------------------------
CpuInitLocalApicTimer    PROC
    sub rsp, 28h  ;Reserve home addresses and make RSP 16-byte aligned
    call InternalX86GetApicBase
    add rsp, 28h
    mov dword ptr [rax + 3e0h], 0ah
    bts dword ptr [rax + 320h], 17
    mov dword ptr [rax + 380h], -1
    ret
CpuInitLocalApicTimer    ENDP

    END

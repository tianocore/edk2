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

    .386
    .model  flat,C
    .code

InternalX86GetApicBase    PROTO   C

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; CpuInitLocalApicTimer (
;   VOID
;   );
;------------------------------------------------------------------------------
CpuInitLocalApicTimer    PROC
    call InternalX86GetApicBase
    mov dword ptr ds:[eax + 3e0h], 0ah
    bts dword ptr ds:[eax + 320h], 17
    mov dword ptr ds:[eax + 380h], -1
    ret
CpuInitLocalApicTimer    ENDP

    END

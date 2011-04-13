;
; Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;
; Module Name:
; 
;    AsmDispatchExecute.asm
;
; Abstract:
; 
;   This is the assembly code to transition from long mode to compatibility mode to execute 32-bit code and then 
;   transit back to long mode.
;
;------------------------------------------------------------------------------- 
    .code
;----------------------------------------------------------------------------
; Procedure:    AsmExecute32BitCode
;
; Input:        None
;
; Output:       None
;
; Prototype:    EFI_STATUS
;               AsmExecute32BitCode (
;                 IN UINT64           Function,
;                 IN UINT64           Param1,
;                 IN UINT64           Param2,
;                 IN IA32_DESCRIPTOR  *InternalGdtr
;                 );
;
;
; Description:  A thunk function to execute 32-bit code in long mode. 
;
;----------------------------------------------------------------------------
AsmExecute32BitCode    PROC    
    ;
    ; save orignal GDTR and CS
    ;
    mov     rax, ds
    push    rax
    mov     rax, cs
    push    rax
    sub     rsp, 10h
    sgdt    fword ptr [rsp]
    ;
    ; load internal GDT
    ;
    lgdt    fword ptr [r9]
    ;
    ; Save general purpose register and rflag register
    ;
    pushfq 
    push    rdi
    push    rsi
    push    rbp
    push    rbx
    
    ;
    ; save CR3
    ;
    mov     rax, cr3
    mov     rbp, rax

    ;
    ; Prepare the CS and return address for the transition from 32-bit to 64-bit mode 
    ;        
    mov     rax, 10h              ; load long mode selector    
    shl     rax, 32
    mov     r9, OFFSET ReloadCS   ;Assume the ReloadCS is under 4G
    or      rax, r9
    push    rax
    ;
    ; Save parameters for 32-bit function call
    ;   
    mov     rax, r8
    shl     rax, 32
    or      rax, rdx
    push    rax
    ;
    ; save the 32-bit function entry and the return address into stack which will be 
    ; retrieve in compatibility mode.
    ;
    mov     rax, OFFSET ReturnBack   ;Assume the ReloadCS is under 4G 
    shl     rax, 32
    or      rax, rcx
    push    rax
    
    ;
    ; let rax save DS
    ;
    mov     rax, 018h

    ;
    ; Change to Compatible Segment
    ;
    mov     rcx, 08h               ; load compatible mode selector 
    shl     rcx, 32
    mov     rdx, OFFSET Compatible ; assume address < 4G
    or      rcx, rdx
    push    rcx
    retf

Compatible:
    ; reload DS/ES/SS to make sure they are correct referred to current GDT
    mov     ds, ax
    mov     es, ax
    mov     ss, ax

    ;
    ; Disable paging
    ;
    mov     rcx, cr0
    btc     ecx, 31
    mov     cr0, rcx
    ;
    ; Clear EFER.LME
    ;
    mov     ecx, 0C0000080h
    rdmsr
    btc     eax, 8
    wrmsr

; Now we are in protected mode
    ;
    ; Call 32-bit function. Assume the function entry address and parameter value is less than 4G
    ;    
    pop    rax                 ; Here is the function entry
    ;  
    ; Now the parameter is at the bottom of the stack,  then call in to IA32 function.
    ;
    jmp   rax
ReturnBack:
    pop   rcx                  ; drop param1      
    pop   rcx                  ; drop param2      

    ;
    ; restore CR4
    ;
    mov     rax, cr4
    bts     eax, 5
    mov     cr4, rax

    ;
    ; restore CR3
    ;
    mov     eax, ebp
    mov     cr3, rax

    ;
    ; Set EFER.LME to re-enable ia32-e
    ;
    mov     ecx, 0C0000080h
    rdmsr
    bts     eax, 8
    wrmsr
    ;
    ; Enable paging
    ;
    mov     rax, cr0
    bts     eax, 31
    mov     cr0, rax
; Now we are in compatible mode

    ;
    ; Reload cs register 
    ;   
    retf
ReloadCS:   
    ;
    ; Now we're in Long Mode
    ;
    ;
    ; Restore C register and eax hold the return status from 32-bit function.
    ; Note: Do not touch rax from now which hold the return value from IA32 function
    ;
    pop     rbx
    pop     rbp
    pop     rsi
    pop     rdi
    popfq
    ;
    ; Switch to orignal GDT and CS. here rsp is pointer to the orignal GDT descriptor.
    ;
    lgdt    fword ptr[rsp]
    ;
    ; drop GDT descriptor in stack
    ;
    add     rsp, 10h 
    ;
    ; switch to orignal CS and GDTR
    ;
    pop     r9                 ; get  CS
    shl     r9,  32            ; rcx[32..47] <- Cs       
    mov     rcx, OFFSET @F          
    or      rcx, r9
    push    rcx
    retf
@@:        
    ;
    ; Reload original DS/ES/SS
    ;
    pop     rcx
    mov     ds, rcx
    mov     es, rcx
    mov     ss, rcx
    ret
AsmExecute32BitCode   ENDP

    END

;------------------------------------------------------------------------------ ;
; Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   MpFuncs32.asm
;
; Abstract:
;
;   This is the assembly code for MP support
;
;-------------------------------------------------------------------------------

include  MpEqu.inc
extern   InitializeFloatingPointUnits:PROC

.code
;-------------------------------------------------------------------------------------
;RendezvousFunnelProc  procedure follows. All APs execute their procedure. This
;procedure serializes all the AP processors through an Init sequence. It must be
;noted that APs arrive here very raw...ie: real mode, no stack.
;ALSO THIS PROCEDURE IS EXECUTED BY APs ONLY ON 16 BIT MODE. HENCE THIS PROC
;IS IN MACHINE CODE.
;-------------------------------------------------------------------------------------
RendezvousFunnelProc   PROC  PUBLIC
RendezvousFunnelProcStart::
; At this point CS = 0x(vv00) and ip= 0x0.
; Save BIST information to ebp firstly
    db 66h,  08bh, 0e8h               ; mov        ebp, eax    ; save BIST information

    db 8ch,0c8h                       ; mov        ax,cs
    db 8eh,0d8h                       ; mov        ds,ax
    db 8eh,0c0h                       ; mov        es,ax
    db 8eh,0d0h                       ; mov        ss,ax
    db 33h,0c0h                       ; xor        ax,ax
    db 8eh,0e0h                       ; mov        fs,ax
    db 8eh,0e8h                       ; mov        gs,ax

    db 0BEh                           ; opcode of mov si, mem16
    dw BufferStartLocation            ; mov        si, BufferStartLocation
    db 66h,  8Bh, 1Ch                 ; mov        ebx,dword ptr [si]

    db 0BFh                           ; opcode of mov di, mem16
    dw PmodeOffsetLocation            ; mov        di, PmodeOffsetLocation
    db 66h,  8Bh, 05h                 ; mov        eax,dword ptr [di]
    db 8Bh,  0F8h                     ; mov        di, ax
    db 83h,  0EFh,06h                 ; sub        di, 06h
    db 66h,  03h, 0C3h                ; add        eax, ebx
    db 66h,  89h, 05h                 ; mov        dword ptr [di],eax

    db 0BFh                           ; opcode of mov di, mem16
    dw LmodeOffsetLocation            ; mov        di, LmodeOffsetLocation
    db 66h,  8Bh, 05h                 ; mov        eax,dword ptr [di]
    db 8Bh,  0F8h                     ; mov        di, ax
    db 83h,  0EFh,06h                 ; sub        di, 06h
    db 66h,  03h, 0C3h                ; add        eax, ebx
    db 66h,  89h, 05h                 ; mov        dword ptr [di],eax

    db 0BEh
    dw Cr3Location                    ; mov        si, Cr3Location
    db 66h,  8Bh, 0Ch                 ; mov        ecx,dword ptr [si]     ; ECX is keeping the value of CR3

    db 0BEh                           ; opcode of mov si, mem16
    dw GdtrLocation                   ; mov        si, GdtrLocation
    db 66h                            ; db         66h
    db 2Eh,  0Fh, 01h, 14h            ; lgdt       fword ptr cs:[si]

    db 0BEh
    dw IdtrLocation                   ; mov        si, IdtrLocation
    db 66h                            ; db         66h
    db 2Eh,0Fh, 01h, 1Ch              ; lidt       fword ptr cs:[si]

    db 33h,  0C0h                     ; xor        ax,  ax
    db 8Eh,  0D8h                     ; mov        ds,  ax

    db 0Fh,  20h, 0C0h                ; mov        eax, cr0               ;Get control register 0
    db 66h,  83h, 0C8h, 03h           ; or         eax, 000000003h        ;Set PE bit (bit #0) & MP
    db 0Fh,  22h, 0C0h                ; mov        cr0, eax

    db 66h,  67h, 0EAh                ; far jump
    dd 0h                             ; 32-bit offset
    dw PROTECT_MODE_CS                ; 16-bit selector

Flat32Start::                         ; protected mode entry point
    mov        ax, PROTECT_MODE_DS
    mov        ds, ax
    mov        es, ax
    mov        fs, ax
    mov        gs, ax
    mov        ss, ax

    db 0Fh,  20h,  0E0h           ; mov        eax, cr4
    db 0Fh,  0BAh, 0E8h, 05h      ; bts        eax, 5
    db 0Fh,  22h,  0E0h           ; mov        cr4, eax

    db 0Fh,  22h,  0D9h           ; mov        cr3, ecx

    db 0B9h
    dd 0C0000080h                 ; mov        ecx, 0c0000080h     ; EFER MSR number.
    db 0Fh,  32h                  ; rdmsr                          ; Read EFER.
    db 0Fh,  0BAh, 0E8h, 08h      ; bts        eax, 8              ; Set LME=1.
    db 0Fh,  30h                  ; wrmsr                          ; Write EFER.

    db 0Fh,  20h,  0C0h           ; mov        eax, cr0            ; Read CR0.
    db 0Fh,  0BAh, 0E8h, 1Fh      ; bts        eax, 31             ; Set PG=1.
    db 0Fh,  22h,  0C0h           ; mov        cr0, eax            ; Write CR0.

LONG_JUMP:
    db 67h,  0EAh                 ; far jump
    dd 0h                         ; 32-bit offset
    dw LONG_MODE_CS               ; 16-bit selector

LongModeStart::
    mov        ax,  LONG_MODE_DS
    mov        ds,  ax
    mov        es,  ax
    mov        ss,  ax

    mov        esi, ebx
    mov        edi, esi
    add        edi, LockLocation
    mov        rax, NotVacantFlag

TestLock:
    xchg       qword ptr [edi], rax
    cmp        rax, NotVacantFlag
    jz         TestLock

    mov        edi, esi
    add        edi, NumApsExecutingLoction
    inc        dword ptr [edi]
    mov        ebx, dword ptr [edi]

ProgramStack:
    mov        edi, esi
    add        edi, StackSizeLocation
    mov        rax, qword ptr [edi]
    mov        edi, esi
    add        edi, StackStartAddressLocation
    add        rax, qword ptr [edi]
    mov        rsp, rax
    mov        qword ptr [edi], rax

Releaselock:
    mov        rax, VacantFlag
    mov        edi, esi
    add        edi, LockLocation
    xchg       qword ptr [edi], rax

CProcedureInvoke:
    push       rbp               ; push BIST data
    xor        rbp, rbp          ; clear ebp for call stack trace
    push       rbp
    mov        rbp, rsp

    mov        rax, InitializeFloatingPointUnits
    sub        rsp, 20h
    call       rax               ; Call assembly function to initialize FPU per UEFI spec
    add        rsp, 20h

    mov        edx, ebx          ; edx is NumApsExecuting
    mov        ecx, esi
    add        ecx, LockLocation ; rcx is address of exchange info data buffer

    mov        edi, esi
    add        edi, ApProcedureLocation
    mov        rax, qword ptr [edi]

    sub        rsp, 20h
    call       rax               ; invoke C function
    add        rsp, 20h
    jmp        $

RendezvousFunnelProc   ENDP
RendezvousFunnelProcEnd::

AsmCliHltLoop PROC
    cli
    hlt
    jmp $-2
AsmCliHltLoop ENDP

;-------------------------------------------------------------------------------------
;  AsmGetAddressMap (&AddressMap);
;-------------------------------------------------------------------------------------
AsmGetAddressMap   PROC
    mov        rax, offset RendezvousFunnelProcStart
    mov        qword ptr [rcx], rax
    mov        qword ptr [rcx +  8h], Flat32Start - RendezvousFunnelProcStart
    mov        qword ptr [rcx + 10h], LongModeStart - RendezvousFunnelProcStart
    mov        qword ptr [rcx + 18h], RendezvousFunnelProcEnd - RendezvousFunnelProcStart
    ret
AsmGetAddressMap   ENDP

;-------------------------------------------------------------------------------------
;AsmExchangeRole procedure follows. This procedure executed by current BSP, that is
;about to become an AP. It switches it'stack with the current AP.
;AsmExchangeRole (IN   CPU_EXCHANGE_INFO    *MyInfo, IN   CPU_EXCHANGE_INFO    *OthersInfo);
;-------------------------------------------------------------------------------------
AsmExchangeRole   PROC
    ; DO NOT call other functions in this function, since 2 CPU may use 1 stack
    ; at the same time. If 1 CPU try to call a function, stack will be corrupted.

    push       rax
    push       rbx
    push       rcx
    push       rdx
    push       rsi
    push       rdi
    push       rbp
    push       r8
    push       r9
    push       r10
    push       r11
    push       r12
    push       r13
    push       r14
    push       r15

    mov        rax, cr0
    push       rax

    mov        rax, cr4
    push       rax

    ; rsi contains MyInfo pointer
    mov        rsi, rcx

    ; rdi contains OthersInfo pointer
    mov        rdi, rdx

    ;Store EFLAGS, GDTR and IDTR regiter to stack
    pushfq
    sgdt       fword ptr [rsi + 16]
    sidt       fword ptr [rsi + 26]

    ; Store the its StackPointer
    mov        qword ptr [rsi + 8], rsp

    ; update its switch state to STORED
    mov        byte ptr [rsi], CPU_SWITCH_STATE_STORED

WaitForOtherStored:
    ; wait until the other CPU finish storing its state
    cmp        byte ptr [rdi], CPU_SWITCH_STATE_STORED
    jz         OtherStored
    pause
    jmp        WaitForOtherStored

OtherStored:
    ; Since another CPU already stored its state, load them
    ; load GDTR value
    lgdt       fword ptr [rdi + 16]

    ; load IDTR value
    lidt       fword ptr [rdi + 26]

    ; load its future StackPointer
    mov        rsp, qword ptr [rdi + 8]

    ; update the other CPU's switch state to LOADED
    mov        byte ptr [rdi], CPU_SWITCH_STATE_LOADED

WaitForOtherLoaded:
    ; wait until the other CPU finish loading new state,
    ; otherwise the data in stack may corrupt
    cmp        byte ptr [rsi], CPU_SWITCH_STATE_LOADED
    jz         OtherLoaded
    pause
    jmp        WaitForOtherLoaded

OtherLoaded:
    ; since the other CPU already get the data it want, leave this procedure
    popfq

    pop        rax
    mov        cr4, rax

    pop        rax
    mov        cr0, rax

    pop        r15
    pop        r14
    pop        r13
    pop        r12
    pop        r11
    pop        r10
    pop        r9
    pop        r8
    pop        rbp
    pop        rdi
    pop        rsi
    pop        rdx
    pop        rcx
    pop        rbx
    pop        rax

    ret
AsmExchangeRole   ENDP

AsmInitializeGdt   PROC
    push       rbp
    mov        rbp, rsp

    lgdt       fword PTR [rcx]  ; update the GDTR

    sub        rsp, 0x10
    lea        rax, SetCodeSelectorFarJump
    mov        [rsp], rax
    mov        rdx, LONG_MODE_CS
    mov        [rsp + 4], dx    ; get new CS
    jmp        fword ptr [rsp]
SetCodeSelectorFarJump:
    add        rsp, 0x10

    mov        rax, LONG_MODE_DS          ; get new DS
    mov        ds, ax
    mov        es, ax
    mov        fs, ax
    mov        gs, ax
    mov        ss, ax

    pop        rbp
    ret
AsmInitializeGdt  ENDP

END

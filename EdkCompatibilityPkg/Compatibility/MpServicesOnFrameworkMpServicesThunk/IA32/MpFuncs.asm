;------------------------------------------------------------------------------
; IA32 assembly file for AP startup vector.
;
; Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

.686p
.model  flat        
.code        

include AsmInclude.inc
;-------------------------------------------------------------------------------------
FJMP32  MACRO   Selector, Offset
            DB      066h
            DB      067h
            DB      0EAh            ; far jump
            DD      Offset          ; 32-bit offset
            DW      Selector        ; 16-bit selector
            ENDM

;-------------------------------------------------------------------------------------
;RendezvousFunnelProc  procedure follows. All APs execute their procedure. This
;procedure serializes all the AP processors through an Init sequence. It must be
;noted that APs arrive here very raw...ie: real mode, no stack.
;ALSO THIS PROCEDURE IS EXECUTED BY APs ONLY ON 16 BIT MODE. HENCE THIS PROC
;IS IN MACHINE CODE.
;-------------------------------------------------------------------------------------
;RendezvousFunnelProc (&WakeUpBuffer,MemAddress);

RendezvousFunnelProc   PROC  near C  PUBLIC
RendezvousFunnelProcStart::


; At this point CS = 0x(vv00) and ip= 0x0.

        db 8ch,  0c8h                 ; mov        ax,  cs
        db 8eh,  0d8h                 ; mov        ds,  ax
        db 8eh,  0c0h                 ; mov        es,  ax
        db 8eh,  0d0h                 ; mov        ss,  ax 
        db 33h,  0c0h                 ; xor        ax,  ax
        db 8eh,  0e0h                 ; mov        fs,  ax
        db 8eh,  0e8h                 ; mov        gs,  ax

; Switch to flat mode.

        db 0BEh
        dw BufferStart                ; mov        si, BufferStart
        db 66h,  8Bh, 0Ch             ; mov        ecx,dword ptr [si]          ; ECX is keeping the start address of wakeup buffer

        db 0FAh                       ; cli
        db 0BEh
        dw GdtrProfile                ; mov        si, GdtrProfile
        db 66h                        ; db         66h
        db 2Eh,0Fh, 01h, 14h          ; lgdt       fword ptr cs:[si]

        db 0BEh
        dw IdtrProfile                ; mov        si, IdtrProfile
        db 66h                        ; db         66h
        db 2Eh,0Fh, 01h, 1Ch          ; lidt       fword ptr cs:[si]
        
        db 33h, 0C0h                  ; xor        ax,  ax
        db 8Eh, 0D8h                  ; mov        ds,  ax
        db 0Fh, 20h, 0C0h             ; mov        eax, cr0                    ; Get control register 0
        db 66h, 83h, 0C8h, 01h        ; or         eax, 000000001h             ; Set PE bit (bit #0)
        db 0Fh, 22h, 0C0h             ; mov        cr0, eax


FLAT32_JUMP::
        FJMP32  010h,0h               ; Far jmp using code segment descriptor

ProtectedModeStart::                  ; protected mode entry point

        mov         ax,  8h
        mov         ds,  ax
        mov         es,  ax
        mov         fs,  ax
        mov         gs,  ax
        mov         ss,  ax           ; Flat mode setup.

        ;
        ; ProgramStack
        ;
        mov         ecx, 1bh                          ; Read IA32_APIC_BASE MSR
        rdmsr

        bt          eax, 10                           ; Check for x2apic mode
        jnc         LegacyApicMode
        mov         ecx, 802h                         ; Read APIC_ID
        rdmsr
        mov         ebx, eax                          ; ebx == apicid
        jmp         GetCpuNumber

LegacyApicMode::

        and         eax, 0fffff000h
        add         eax, 20h
        mov         ebx, dword ptr [eax]
        shr         ebx, 24                           ; ebx == apicid

GetCpuNumber::
        
        xor         ecx, ecx
        mov         edi, esi
        add         edi, ProcessorNumber
        mov         ecx, dword ptr [edi + 4 * ebx]    ; ECX = CpuNumber

        mov         edi, esi
        add         edi, StackSize
        mov         eax, dword ptr [edi]
        inc         ecx
        mul         ecx                               ; EAX = StackSize * (CpuNumber + 1)

        mov         edi, esi
        add         edi, StackStart
        mov         ebx, dword ptr [edi]
        add         eax, ebx                          ; EAX = StackStart + StackSize * (CpuNumber + 1)

        mov         esp, eax

        ;
        ; Call C Function
        ;
        mov         edi, esi
        add         edi, RendezvousProc
        mov         ebx, dword ptr [edi]

        test        ebx, ebx
        jz          GoToSleep
        call        ebx                           ; Call C function

GoToSleep::

        cli
        hlt
        jmp         $-2
        
RendezvousFunnelProc   ENDP
RendezvousFunnelProcEnd::
;-------------------------------------------------------------------------------------
;  AsmGetAddressMap (&AddressMap);
;-------------------------------------------------------------------------------------
AsmGetAddressMap   PROC  near C  PUBLIC

        pushad
        mov         ebp,esp
   
        mov         ebx, dword ptr [ebp+24h]
        mov         dword ptr [ebx], RendezvousFunnelProcStart
        mov         dword ptr [ebx+4h], ProtectedModeStart - RendezvousFunnelProcStart
        mov         dword ptr [ebx+8h], FLAT32_JUMP - RendezvousFunnelProcStart
        mov         dword ptr [ebx+0ch], 0
        mov         dword ptr [ebx+10h], 0
        mov         dword ptr [ebx+14h], RendezvousFunnelProcEnd - RendezvousFunnelProcStart
        
        popad
        ret
AsmGetAddressMap   ENDP

END

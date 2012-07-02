#------------------------------------------------------------------------------
# X64 assembly file for AP startup vector.
#
# Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------


.set                   VacantFlag,       0x0
.set                   NotVacantFlag,    0xff

.set                   LockLocation,              RendezvousFunnelProcEnd - RendezvousFunnelProcStart
.set                   StackStartAddressLocation, RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x08
.set                   StackSizeLocation,         RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x10
.set                   CProcedureLocation,        RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x18
.set                   GdtrLocation,              RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x20
.set                   IdtrLocation,              RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x2A
.set                   BufferStartLocation,       RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x34
.set                   Cr3OffsetLocation,         RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x38
.set                   ProcessorNumberLocation,   RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x38

#-------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------
#RendezvousFunnelProc  procedure follows. All APs execute their procedure. This
#procedure serializes all the AP processors through an Init sequence. It must be
#noted that APs arrive here very raw...ie: real mode, no stack.
#ALSO THIS PROCEDURE IS EXECUTED BY APs ONLY ON 16 BIT MODE. HENCE THIS PROC
#IS IN MACHINE CODE.
#-------------------------------------------------------------------------------------
#RendezvousFunnelProc (&WakeUpBuffer,MemAddress);

.text

ASM_GLOBAL ASM_PFX(RendezvousFunnelProc)
ASM_PFX(RendezvousFunnelProc):
RendezvousFunnelProcStart:

# At this point CS = 0x(vv00) and ip= 0x0.

        .byte 0x8c,0xc8               # mov        ax,  cs
        .byte 0x8e,0xd8               # mov        ds,  ax
        .byte 0x8e,0xc0               # mov        es,  ax
        .byte 0x8e,0xd0               # mov        ss,  ax 
        .byte 0x33,0xc0               # xor        ax,  ax
        .byte 0x8e,0xe0               # mov        fs,  ax
        .byte 0x8e,0xe8               # mov        gs,  ax

# Switch to flat mode.

        .byte 0xBE
        .word BufferStartLocation
        .byte 0x66,0x8B,0x14          # mov        edx,dword ptr [si]          ; EDX is keeping the start address of wakeup buffer

        .byte 0xBE
        .word Cr3OffsetLocation
        .byte 0x66,0x8B,0xC           # mov        ecx,dword ptr [si]          ; ECX is keeping the value of CR3

        .byte 0xBE
        .word GdtrLocation
        .byte 0x66                    # db         66h
        .byte 0x2E,0xF,0x1,0x14       # lgdt       fword ptr cs:[si]

        .byte 0xBE
        .word IdtrLocation
        .byte 0x66                    # db         66h
        .byte 0x2E,0xF,0x1,0x1C       # lidt       fword ptr cs:[si]

        .byte 0x33,0xC0               # xor        ax,  ax
        .byte 0x8E,0xD8               # mov        ds,  ax

        .byte 0xF,0x20,0xC0           # mov        eax, cr0                    ; Get control register 0
        .byte 0x66,0x83,0xC8,0x1      # or         eax, 000000001h             ; Set PE bit (bit #0)
        .byte 0xF,0x22,0xC0           # mov        cr0, eax

FLAT32_JUMP:

        .byte 0x66,0x67,0xEA          # far jump
        .long 0x0                     # 32-bit offset
        .word 0x20                    # 16-bit selector

ProtectedModeStart:

        .byte 0x66,0xB8,0x18,0x0      # mov        ax,  18h
        .byte 0x66,0x8E,0xD8          # mov        ds,  ax
        .byte 0x66,0x8E,0xC0          # mov        es,  ax
        .byte 0x66,0x8E,0xE0          # mov        fs,  ax
        .byte 0x66,0x8E,0xE8          # mov        gs,  ax
        .byte 0x66,0x8E,0xD0          # mov        ss,  ax                     ; Flat mode setup.

        .byte 0xF,0x20,0xE0           # mov        eax, cr4
        .byte 0xF,0xBA,0xE8,0x5       # bts        eax, 5
        .byte 0xF,0x22,0xE0           # mov        cr4, eax

        .byte 0xF,0x22,0xD9           # mov        cr3, ecx

        .byte 0x8B,0xF2               # mov        esi, edx                    ; Save wakeup buffer address

        .byte 0xB9
        .long 0xC0000080              # mov        ecx, 0c0000080h             ; EFER MSR number.
        .byte 0xF,0x32                # rdmsr                                  ; Read EFER.
        .byte 0xF,0xBA,0xE8,0x8       # bts        eax, 8                      ; Set LME=1.
        .byte 0xF,0x30                # wrmsr                                  ; Write EFER.

        .byte 0xF,0x20,0xC0           # mov        eax, cr0                    ; Read CR0.
        .byte 0xF,0xBA,0xE8,0x1F      # bts        eax, 31                     ; Set PG=1.
        .byte 0xF,0x22,0xC0           # mov        cr0, eax                    ; Write CR0.

LONG_JUMP:

        .byte 0x67,0xEA               # far jump
        .long 0x0                     # 32-bit offset
        .word 0x38                    # 16-bit selector

LongModeStart:

        movw        $0x30,%ax
        .byte       0x66
        movw        %ax,%ds
        .byte       0x66
        movw        %ax,%es
        .byte       0x66
        movw        %ax,%ss

        #
        # ProgramStack
        #
        movl        $0x1b, %ecx
        rdmsr

        btl         $10, %eax         # Check for x2apic mode
        jnc         LegacyApicMode
        movl        $0x802, %ecx      # Read APIC_ID
        rdmsr
        movl        %eax, %ebx        # ebx == apicid
        jmp         GetCpuNumber

LegacyApicMode:
        andl        $0xfffff000, %eax
        addl        $0x20, %eax
        movl        (%eax), %ebx
        shrl        $24, %ebx         # ebx == apicid
        
GetCpuNumber:        
        xorq        %rcx, %rcx
        movl        %esi,%edi
        addl        $ProcessorNumberLocation, %edi
        movl        (%edi, %ebx, 4), %ecx

        movl        %esi,%edi
        addl        $StackSizeLocation, %edi
        movq        (%edi), %rax
        incq        %rcx
        mulq        %rcx

        movl        %esi,%edi
        addl        $StackStartAddressLocation, %edi
        movq        (%edi), %rbx
        addq        %rbx, %rax

        movq        %rax, %rsp

        #
        # Call C Function
        #
        movl        %esi,%edi
        addl        $CProcedureLocation, %edi
        movq        (%edi), %rax

        testq       %rax, %rax
        jz          GoToSleep

        subq        $0x20, %rsp
        call        *%rax
        addq        $0x20, %rsp

GoToSleep:

        cli
        hlt
        jmp         .-2

RendezvousFunnelProcEnd:


#-------------------------------------------------------------------------------------
#  AsmGetAddressMap (&AddressMap);
#-------------------------------------------------------------------------------------
# comments here for definition of address map
ASM_GLOBAL ASM_PFX(AsmGetAddressMap)
ASM_PFX(AsmGetAddressMap):
#ifdef __APPLE__
        int          $3
#else
        movq         $RendezvousFunnelProcStart, %rax
        movq         %rax, (%rcx)
        movq         $(ProtectedModeStart - RendezvousFunnelProcStart), 0x08(%rcx)
        movq         $(FLAT32_JUMP - RendezvousFunnelProcStart), 0x10(%rcx)
        movq         $(LongModeStart - RendezvousFunnelProcStart), 0x18(%rcx)
        movq         $(LONG_JUMP - RendezvousFunnelProcStart), 0x20(%rcx)
        movq         $(RendezvousFunnelProcEnd - RendezvousFunnelProcStart), 0x28(%rcx)
#endif
        ret

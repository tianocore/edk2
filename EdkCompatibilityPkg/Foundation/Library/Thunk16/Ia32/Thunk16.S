#*****************************************************************************
#*
#*   Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
#*   This program and the accompanying materials                          
#*   are licensed and made available under the terms and conditions of the BSD License         
#*   which accompanies this distribution.  The full text of the license may be found at        
#*   http://opensource.org/licenses/bsd-license.php                                            
#*                                                                                             
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#*   
#*   Module Name:
#*
#*    Thunk.asm
#*  
#*   Abstract:
#*  
#*    Real mode thunk
#*  
#*****************************************************************************
#include <EfiBind.h>

    .686p: 

.globl ASM_PFX(mCode16Size)

.data
mCode16Size:    .long   _TEXT16SIZE

.data

NullSegSel:     .quad   0
_16BitCsSel:    
                .word   -1
                .word   0
                .byte   0
                .byte   0x9b
                .byte   0x8f            # 16-bit segment
                .byte   0
_16BitSsSel:
                .word   -1
                .word   0
                .byte   0
                .byte   0x93
                .byte   0x8f           # 16-bit segment
                .byte   0

_16Gdtr:
                .word      _16Gdtr - NullSegSel - 1
                .long      NullSegSel


.text


ASM_PFX(Thunk16):
    push   %ebp
    push   %ebx
    push   %esi
    push   %edi
    push   %ds
    push   %es
    push   %fs
    push   %gs
    mov    0x24(%esp),%esi
    movzwl 0x32(%esi),%edx
    mov    0xc(%esi),%edi
    add    $0xffffffb0,%edi
    push   %edi                       #; save stack offset
    imul   $0x10,%edx,%eax            #; eax <- edx*16
    add    %eax,%edi                  #; edi <- linear address of 16-bit stack
    push   $0xd
    pop    %ecx
    rep movsl %ds:(%esi),%es:(%edi)   #; copy context to 16-bit stack
    #; copy eflags to stack frame
    mov    -12(%esi), %eax
    mov    %eax, -72(%edi) 
    pop    %ebx                       #; ebx <- 16-bit stack offset
    mov    $L_Lable1,%eax
    stos   %eax,%es:(%edi)
    movl   %cs,%eax
    stos   %ax,%es:(%edi)
    mov    0x28(%esp),%eax
    stos   %ax,%es:(%edi)
    mov    %esp,%eax
    stos   %eax,%es:(%edi)
    movl   %ss,%eax
    stos   %ax,%es:(%edi)
    sgdtl  (%edi)
    sidtl  0x24(%esp)
    mov    %cr0,%esi
    mov    %esi,0x6(%edi)             #; save CR0
    and    $0x7ffffffe,%esi           #; esi <- CR0 to set
    mov    %cr4,%eax
    mov    %eax,0xa(%edi)             #; save CR4
    and    $0xcf,%al                  #; clear PAE & PSE
    mov    %edx,%edi                  #; edi <- 16-bit stack segment
    mov    0x2c(%esp),%edx
    shl    $0x10,%edx
    push   %edx
    pop    %edx
    mov    $(_16BitSsSel - NullSegSel),%dx
    lgdtl  _16Gdtr                    #bugbug mismatch.
    .byte  0xea
    .long  L_16Bit                    #bugbug mismatch.
    .word  _16BitCsSel - NullSegSel
L_16Bit:   
    .byte  0x66 
    movw   %dx,%ss
    mov    %esi,%cr0
    mov    %eax,%cr4
    .byte  0x67
    .byte  0xff
    .byte  0x6c
    .byte  0x24
    .byte  0xfc
    
L_Lable1:
    xor    %eax,%eax
    movw   %ss,%ax
    shl    $0x4,%eax
    add    %esp,%eax
    lss    0x3c(%esp),%esp
    lidtl  0x24(%esp)
    pop    %gs
    pop    %fs
    pop    %es
    pop    %ds
    pop    %edi
    pop    %esi
    pop    %ebx
    pop    %ebp
    ret 

.code16
_Code16Addr:
ASM_PFX(RealMode):
    movw    %di, %ss                    # set up stack
    movl    %ebx, %esp
    lidt    %cs:_16Idtr - _Code16Addr  #lidt    fword ptr cs:[_16Idtr - _Code16Addr]
    .byte   0x66
    popaw
    popw    %ds
    popw    %es
    popw    %fs
    popw    %gs
    sub     60, %esp
    popfw
    testw   $1, 74(%esp)                #(_STK16 ptr [esp + STACK_PARAM_SIZE + sizeof(IA32_REGS)]).ThunkFlags, 1

    jz      1f
    pushf                               # push Flags when it's INT#
1: 
    pushw   %cs
#    push    @FarCallRet - _Code16Addr
    .byte   0x68                        # push /iw
    .word   FarCallRet - _Code16Addr
    jz      2f
    ljmp    *66(%esp)                   #[esp + 6 + STACK_PARAM_SIZE + sizeof(IA32_REGS) - 8]
2: 
    ljmp    *64(%esp)                   #[esp + 4 + STACK_PARAM_SIZE + sizeof(IA32_REGS) - 8]
FarCallRet: 
    add     60, %esp
    pushfl
    pushw   %gs
    pushw   %fs
    pushw   %es
    pushw   %ds
    pushal
    cli
    .byte   0x66                        # sizeof (IA32_REGS) = 13 * 4 = 52
    lgdt    66(%esp)                    #lgdt    (_STK16 ptr [esp + sizeof (IA32_REGS)]).SavedGdtr
    mov     76(%esp), %eax
    movl    %eax, %cr4
    mov     72(%esp), %eax
    movl    %eax, %cr0                   # restore CR0
    ljmpl   *52(%esp)                    
#RealMode ENDP

.text
_16Idtr:
    .word 0x3ff                         #_16Idtr     FWORD   (1 SHL 10) - 1
    .byte 0x00

_TEXT16END: 

_TEXT16SIZE = _TEXT16END - _Code16Addr



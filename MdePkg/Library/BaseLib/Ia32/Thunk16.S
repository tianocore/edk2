#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   Thunk16.S
#
# Abstract:
#
#   Real mode thunk
#
#------------------------------------------------------------------------------

#include <Library/BaseLib.h>

ASM_GLOBAL ASM_PFX(m16Start), ASM_PFX(m16Size), ASM_PFX(mThunk16Attr), ASM_PFX(m16Gdt), ASM_PFX(m16GdtrBase), ASM_PFX(mTransition)
ASM_GLOBAL ASM_PFX(InternalAsmThunk16)

# define the structure of IA32_REGS
.set  _EDI, 0       #size 4
.set  _ESI, 4       #size 4
.set  _EBP, 8       #size 4
.set  _ESP, 12      #size 4
.set  _EBX, 16      #size 4
.set  _EDX, 20      #size 4
.set  _ECX, 24      #size 4
.set  _EAX, 28      #size 4
.set  _DS,  32      #size 2
.set  _ES,  34      #size 2
.set  _FS,  36      #size 2
.set  _GS,  38      #size 2
.set  _EFLAGS, 40   #size 4
.set  _EIP, 44      #size 4
.set  _CS, 48       #size 2
.set  _SS, 50       #size 2
.set  IA32_REGS_SIZE, 52

    .text

ASM_PFX(m16Start):

SavedGdt:     .space  6

ASM_PFX(BackFromUserCode):
    push    %ss
    push    %cs
    .byte   0x66
    call    L_Base1                     # push eip
L_Base1:
    pushfw                              # pushfd actually
    cli                                 # disable interrupts
    push    %gs
    push    %fs
    push    %es
    push    %ds
    pushaw                              # pushad actually
    .byte   0x66, 0xba                  # mov edx, imm32
ASM_PFX(ThunkAttr): .space  4
    testb   $THUNK_ATTRIBUTE_DISABLE_A20_MASK_INT_15, %dl
    jz      1f
    movl    $0x15cd2401, %eax           # mov ax, 2401h & int 15h
    cli                                 # disable interrupts
    jnc     2f
1:
    testb   $THUNK_ATTRIBUTE_DISABLE_A20_MASK_KBD_CTRL, %dl
    jz      2f
    inb     $0x92, %al
    orb     $2, %al
    outb    %al, $0x92                  # deactivate A20M#
2:
    xorw    %ax, %ax                    # xor eax, eax
    movl    %ss, %eax                   # mov ax, ss
    .byte   0x67
    lea     IA32_REGS_SIZE(%esp), %bp
    .byte   0x66
    mov     %ebp, (_ESP - IA32_REGS_SIZE)(%esi)
    mov     (_EIP - IA32_REGS_SIZE)(%esi), %ebx
    shlw    $4, %ax                     # shl eax, 4
    addw    %ax, %bp                    # add ebp, eax
    .byte   0x66, 0xb8                   # mov eax, imm32
SavedCr4:   .space  4
    movl    %eax, %cr4
    lgdtw   %cs:(SavedGdt - L_Base1)(%edi)
    .byte   0x66, 0xb8                   # mov eax, imm32
SavedCr0:   .space  4
    movl    %eax, %cr0
    .byte   0xb8                        # mov ax, imm16
SavedSs:    .space  2
    movl    %eax, %ss
    .byte   0x66, 0xbc                   # mov esp, imm32
SavedEsp:   .space  4
    .byte   0x66
    lret                                # return to protected mode

_EntryPoint:    .long      ASM_PFX(ToUserCode) - ASM_PFX(m16Start)
                .word      0x8
_16Idtr:        .word      0x3ff
                .long      0
_16Gdtr:        .word      GdtEnd - _NullSegDesc - 1
_16GdtrBase:    .long      _NullSegDesc

ASM_PFX(ToUserCode):
    movl    %ss, %edx
    movl    %ecx, %ss                   # set new segment selectors
    movl    %ecx, %ds
    movl    %ecx, %es
    movl    %ecx, %fs
    movl    %ecx, %gs
    movl    %eax, %cr0
    movl    %ebp, %cr4                  # real mode starts at next instruction
    movl    %esi, %ss                   # set up 16-bit stack segment
    xchgw   %bx, %sp                    # set up 16-bit stack pointer
    .byte   0x66
    call    L_Base                      # push eip
L_Base:
    popw    %bp                         # ebp <- offset L_Base
    .byte   0x67;                       # address size override
    push    (IA32_REGS_SIZE + 2)(%esp)
    lea     (L_RealMode - L_Base)(%esi), %eax
    push    %eax
    lret

L_RealMode:
    mov     %edx, %cs:(SavedSs - L_Base)(%esi)
    mov     %bx, %cs:(SavedEsp - L_Base)(%esi)
    lidtw   %cs:(_16Idtr - L_Base)(%esi)
    popaw                               # popad actually
    pop     %ds
    pop     %es
    pop     %fs
    pop     %gs
    popfw                               # popfd
    lretw                               # transfer control to user code

_NullSegDesc:   .quad   0
_16CsDesc:
                .word   -1
                .word   0
                .byte   0
                .byte   0x9b
                .byte   0x8f            # 16-bit segment, 4GB limit
                .byte   0
_16DsDesc:
                .word   -1
                .word   0
                .byte   0
                .byte   0x93
                .byte   0x8f            # 16-bit segment, 4GB limit
                .byte   0
GdtEnd:

#
#   @param  RegSet  The pointer to a IA32_DWORD_REGS structure
#   @param  Transition  The pointer to the transition code
#   @return The address of the 16-bit stack after returning from user code
#
ASM_PFX(InternalAsmThunk16):
    push    %ebp
    push    %ebx
    push    %esi
    push    %edi
    push    %ds
    push    %es
    push    %fs
    push    %gs
    movl    36(%esp), %esi              # esi <- RegSet
    movzwl  _SS(%esi), %edx
    mov     _ESP(%esi), %edi
    add     $(-(IA32_REGS_SIZE + 4)), %edi
    movl    %edi, %ebx                  # ebx <- stack offset
    imul    $0x10, %edx, %eax
    push    $(IA32_REGS_SIZE / 4)
    addl    %eax, %edi                  # edi <- linear address of 16-bit stack
    pop     %ecx
    rep
    movsl                               # copy RegSet
    movl    40(%esp), %eax              # eax <- address of transition code
    movl    %edx, %esi                  # esi <- 16-bit stack segment
    lea     (SavedCr0 - ASM_PFX(m16Start))(%eax), %edx
    movl    %eax, %ecx
    andl    $0xf, %ecx
    shll    $12, %eax
    lea     (ASM_PFX(BackFromUserCode) - ASM_PFX(m16Start))(%ecx), %ecx
    movw    %cx, %ax
    stosl                               # [edi] <- return address of user code
    sgdtl   (SavedGdt - SavedCr0)(%edx)
    sidtl   0x24(%esp)
    movl    %cr0, %eax
    movl    %eax, (%edx)                # save CR0 in SavedCr0
    andl    $0x7ffffffe, %eax           # clear PE, PG bits
    movl    %cr4, %ebp
    mov     %ebp, (SavedCr4 - SavedCr0)(%edx)
    andl    $0x300, %ebp                # clear all but PCE and OSFXSR bits
    pushl   $0x10
    pop     %ecx                        # ecx <- selector for data segments
    lgdtl   (_16Gdtr - SavedCr0)(%edx)
    pushfl
    lcall   *(_EntryPoint - SavedCr0)(%edx)
    popfl
    lidtl   0x24(%esp)
    lea     -IA32_REGS_SIZE(%ebp), %eax
    pop     %gs
    pop     %fs
    pop     %es
    pop     %ds
    pop     %edi
    pop     %esi
    pop     %ebx
    pop     %ebp
    ret

    .const:

ASM_PFX(m16Size):        .word      ASM_PFX(InternalAsmThunk16)  - ASM_PFX(m16Start)
ASM_PFX(mThunk16Attr):   .word      ASM_PFX(ThunkAttr)          - ASM_PFX(m16Start)
ASM_PFX(m16Gdt):         .word      _NullSegDesc        - ASM_PFX(m16Start)
ASM_PFX(m16GdtrBase):    .word      _16GdtrBase         - ASM_PFX(m16Start)
ASM_PFX(mTransition):    .word      _EntryPoint         - ASM_PFX(m16Start)

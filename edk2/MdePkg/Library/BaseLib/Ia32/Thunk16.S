#------------------------------------------------------------------------------
#
# Copyright (c) 2006, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
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

.globl ASM_PFX(m16Start), ASM_PFX(m16Size), ASM_PFX(mThunk16Attr), ASM_PFX(m16Gdt), ASM_PFX(m16GdtrBase), ASM_PFX(mTransition)
.globl ASM_PFX(InternalAsmThunk16)

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
    movl    %ss, %eax
    .byte   0x67, 0x66, 0x8d, 0x6c, 0x24, 0x34, 0x66
    mov     %ebp, 0xffffffd8(%esi)
    mov     0xfffffff8(%esi), %ebx
    shlw    $4, %ax                     # shl eax, 4
    addw    %ax, %bp                    # add ebp, eax
    .byte   0x66, 0xb8                   # mov eax, imm32
SavedCr4:   .space  4
    movl    %eax, %cr4
    lgdtw   %cs:0xfffffff2(%edi)
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
    addr16  pushl 36(%si)
    .byte   0x36
    lea     0xc(%esi), %eax
    push    %eax
    lret

L_RealMode:
    mov     %edx, %cs:0xffffffc5(%esi)
    mov     %bx, %cs:0xffffffcb(%esi)
    lidtw   %cs:0xffffffd7(%esi)
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
#   @param  RegSet  Pointer to a IA32_DWORD_REGS structure
#   @param  Transition  Pointer to the transition code
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
    movzwl  0x32(%esi), %edx
    mov     0xc(%esi), %edi
    add     $0xffffffc8, %edi
    movl    %edi, %ebx                  # ebx <- stack offset
    imul    $0x10, %edx, %eax
    push    $0xd
    addl    %eax, %edi                  # edi <- linear address of 16-bit stack
    pop     %ecx
    rep
    movsl                               # copy RegSet
    movl    40(%esp), %eax              # eax <- address of transition code
    movl    %edx, %esi                  # esi <- 16-bit stack segment
    lea     0x5e(%eax), %edx
    movl    %eax, %ecx
    andl    $0xf, %ecx
    shll    $12, %eax
    lea     0x6(%ecx), %ecx
    movw    %cx, %ax
    stosl                               # [edi] <- return address of user code
    sgdtl   0xffffffa2(%edx)
    sidtl   0x24(%esp)
    movl    %cr0, %eax
    movl    %eax, (%edx)                # save CR0 in SavedCr0
    andl    $0x7ffffffe, %eax           # clear PE, PG bits
    movl    %cr4, %ebp
    mov     %ebp, 0xfffffff1(%edx)
    andl    $0x300, %ebp                # clear all but PCE and OSFXSR bits
    pushl   $0x10
    pop     %ecx                        # ecx <- selector for data segments
    lgdtl   0x20(%edx)
    pushfl
    lcall   *0x14(%edx)
    popfl
    lidtl   0x24(%esp)
    lea     0xffffffcc(%ebp), %eax
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

ASM_PFX(m16Size):        .word      _InternalAsmThunk16 - ASM_PFX(m16Start)
ASM_PFX(mThunk16Attr):   .word      _ThunkAttr          - ASM_PFX(m16Start)
ASM_PFX(m16Gdt):         .word      _NullSegDesc        - ASM_PFX(m16Start)
ASM_PFX(m16GdtrBase):    .word      _16GdtrBase         - ASM_PFX(m16Start)
ASM_PFX(mTransition):    .word      _EntryPoint         - ASM_PFX(m16Start)

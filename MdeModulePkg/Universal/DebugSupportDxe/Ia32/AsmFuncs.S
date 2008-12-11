#/**@file
# Low leve IA32 specific debug support functions.
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
#**/

.globl ASM_PFX(OrigVector)
.globl ASM_PFX(InterruptEntryStub)
.globl ASM_PFX(StubSize)
.globl ASM_PFX(CommonIdtEntry)
.globl ASM_PFX(FxStorSupport)

ASM_PFX(AppEsp):         .long   0x11111111 # ?
ASM_PFX(DebugEsp):       .long   0x22222222 # ?
ASM_PFX(ExtraPush):      .long   0x33333333 # ?
ASM_PFX(ExceptData):     .long   0x44444444 # ?
ASM_PFX(Eflags):         .long   0x55555555 # ?
ASM_PFX(OrigVector):     .long   0x66666666 # ?
ASM_PFX(StubSize):       .long   ASM_PFX(InterruptEntryStubEnd) - ASM_PFX(InterruptEntryStub)

.globl ASM_PFX(FxStorSupport)
ASM_PFX(FxStorSupport):
        push   %ebx
        mov    $0x1,%eax
        cpuid
        mov    %edx,%eax
        and    $0x1000000,%eax
        shr    $0x18,%eax
        pop    %ebx
        ret

.globl ASM_PFX(Vect2Desc)
ASM_PFX(Vect2Desc):
        push   %ebp
        mov    %esp,%ebp
        mov    0xc(%ebp),%eax
        mov    0x8(%ebp),%ecx
        mov    %ax,(%ecx)
        movw   $0x20,0x2(%ecx)
        movw   $0x8e00,0x4(%ecx)
        shr    $0x10,%eax
        mov    %ax,0x6(%ecx)
        leave
        ret

.globl ASM_PFX(InterruptEntryStub)
ASM_PFX(InterruptEntryStub):
        mov    %esp,0x0
        mov    $0x0,%esp
        push   $0x0
        jmp    ASM_PFX(CommonIdtEntry)
.globl ASM_PFX(InterruptEntryStubEnd)
ASM_PFX(InterruptEntryStubEnd):

.globl ASM_PFX(CommonIdtEntry)
ASM_PFX(CommonIdtEntry):
        pusha
        pushf
        pop    %eax
        mov    %eax,0x0
        cmpl   $0x8,0x0
        jne    ASM_PFX(CommonIdtEntry+0x20)
        movl   $0x1,0x0
        jmp    ASM_PFX(CommonIdtEntry+0xa8)
        cmpl   $0xa,0x0
        jne    ASM_PFX(CommonIdtEntry+0x35)
        movl   $0x1,0x0
        jmp    ASM_PFX(CommonIdtEntry+0xa8)
        cmpl   $0xb,0x0
        jne    ASM_PFX(CommonIdtEntry+0x4a)
        movl   $0x1,0x0
        jmp    ASM_PFX(CommonIdtEntry+0xa8)
        cmpl   $0xc,0x0
        jne    ASM_PFX(CommonIdtEntry+0x5f)
        movl   $0x1,0x0
        jmp    ASM_PFX(CommonIdtEntry+0xa8)
        cmpl   $0xd,0x0
        jne    ASM_PFX(CommonIdtEntry+0x74)
        movl   $0x1,0x0
        jmp    ASM_PFX(CommonIdtEntry+0xa8)
        cmpl   $0xe,0x0
        jne    ASM_PFX(CommonIdtEntry+0x89)
        movl   $0x1,0x0
        jmp    ASM_PFX(CommonIdtEntry+0xa8)
        cmpl   $0x11,0x0
        jne    ASM_PFX(CommonIdtEntry+0x9e)
        movl   $0x1,0x0
        jmp    ASM_PFX(CommonIdtEntry+0xa8)
        movl   $0x0,0x0
        cmpl   $0x1,0x0
        jne    ASM_PFX(CommonIdtEntry+0xc8)
        mov    0x0,%eax
        mov    (%eax),%ebx
        mov    %ebx,0x0
        add    $0x4,%eax
        mov    %eax,0x0
        jmp    ASM_PFX(CommonIdtEntry+0xd2)
        movl   $0x0,0x0
        mov    0xc(%esp),%eax
        mov    %eax,0x0
        mov    0x0,%eax
        add    $0xc,%eax
        mov    %eax,0xc(%esp)
        mov    %ss,%eax
        push   %eax
        mov    0x0,%eax
        movzwl 0x4(%eax),%eax
        push   %eax
        mov    %ds,%eax
        push   %eax
        mov    %es,%eax
        push   %eax
        mov    %fs,%eax
        push   %eax
        mov    %gs,%eax
        push   %eax
        mov    0x0,%eax
        pushl  (%eax)
        push   $0x0
        push   $0x0
        sidtl  (%esp)
        push   $0x0
        push   $0x0
        sgdtl  (%esp)
        xor    %eax,%eax
        str    %eax
        push   %eax
        sldt   %eax
        push   %eax
        mov    0x0,%eax
        pushl  0x8(%eax)
        mov    %cr4,%eax
        or     $0x208,%eax
        mov    %eax,%cr4
        push   %eax
        mov    %cr3,%eax
        push   %eax
        mov    %cr2,%eax
        push   %eax
        push   $0x0
        mov    %cr0,%eax
        push   %eax
        mov    %db7,%eax
        push   %eax
        xor    %eax,%eax
        mov    %eax,%db7
        mov    %db6,%eax
        push   %eax
        xor    %eax,%eax
        mov    %eax,%db6
        mov    %db3,%eax
        push   %eax
        mov    %db2,%eax
        push   %eax
        mov    %db1,%eax
        push   %eax
        mov    %db0,%eax
        push   %eax
        sub    $0x200,%esp
        mov    %esp,%edi
        fxsave (%edi)
        mov    0x0,%eax
        push   %eax
        mov    %esp,%eax
        push   %eax
        mov    0x0,%eax
        push   %eax
        call   ASM_PFX(CommonIdtEntry+0x184)
        add    $0x8,%esp
        add    $0x4,%esp
        mov    %esp,%esi
        fxrstor (%esi)
        add    $0x200,%esp
        pop    %eax
        mov    %eax,%db0
        pop    %eax
        mov    %eax,%db1
        pop    %eax
        mov    %eax,%db2
        pop    %eax
        mov    %eax,%db3
        add    $0x4,%esp
        pop    %eax
        mov    %eax,%db7
        pop    %eax
        mov    %eax,%cr0
        add    $0x4,%esp
        pop    %eax
        mov    %eax,%cr2
        pop    %eax
        mov    %eax,%cr3
        pop    %eax
        mov    %eax,%cr4
        mov    0x0,%eax
        popl   0x8(%eax)
        add    $0x18,%esp
        popl   (%eax)
        pop    %gs
        pop    %fs
        pop    %es
        pop    %ds
        popl   0x4(%eax)
        pop    %ss
        mov    0xc(%esp),%ebx
        mov    0x0,%eax
        add    $0xc,%eax
        cmp    %eax,%ebx
        je     ASM_PFX(CommonIdtEntry+0x202)
        mov    0x0,%eax
        mov    (%eax),%ecx
        mov    %ecx,(%ebx)
        mov    0x4(%eax),%ecx
        mov    %ecx,0x4(%ebx)
        mov    0x8(%eax),%ecx
        mov    %ecx,0x8(%ebx)
        mov    %ebx,%eax
        mov    %eax,0x0
        mov    0x0,%eax
        mov    %eax,0xc(%esp)
        cmpl   $0x68,0x0
        jne    PhonyIretd+0xd
        mov    0x0,%eax
        mov    0x8(%eax),%ebx
        and    $0xfffffcff,%ebx
        push   %ebx
        push   %cs
        push   $0x0
        iret

PhonyIretd:
        popa
        mov    0x0,%esp
        jmp    *0x0
        popa
        mov    0x0,%esp
        iret

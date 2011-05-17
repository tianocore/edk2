#/**@file
# Low leve IA32 specific debug support functions.
#
# Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#**/

ASM_GLOBAL ASM_PFX(OrigVector)
ASM_GLOBAL ASM_PFX(InterruptEntryStub)
ASM_GLOBAL ASM_PFX(StubSize)
ASM_GLOBAL ASM_PFX(CommonIdtEntry)
ASM_GLOBAL ASM_PFX(FxStorSupport)

ASM_PFX(StubSize):       .long   ASM_PFX(InterruptEntryStubEnd) - ASM_PFX(InterruptEntryStub)
ASM_PFX(AppEsp):         .long   0x11111111 # ?
ASM_PFX(DebugEsp):       .long   0x22222222 # ?
ASM_PFX(ExtraPush):      .long   0x33333333 # ?
ASM_PFX(ExceptData):     .long   0x44444444 # ?
ASM_PFX(Eflags):         .long   0x55555555 # ?
ASM_PFX(OrigVector):     .long   0x66666666 # ?

#------------------------------------------------------------------------------
# BOOLEAN
# FxStorSupport (
#   void
#   )
#
# Abstract: Returns TRUE if FxStor instructions are supported
#
ASM_GLOBAL ASM_PFX(FxStorSupport)
ASM_PFX(FxStorSupport):
#
# cpuid corrupts ebx which must be preserved per the C calling convention
#
        push   %ebx
        mov    $0x1,%eax
        cpuid
        mov    %edx,%eax
        and    $0x1000000,%eax
        shr    $0x18,%eax
        pop    %ebx
        ret
#------------------------------------------------------------------------------
# void
# Vect2Desc (
#   DESCRIPTOR * DestDesc,
#   void (*Vector) (void)
#   )
#
# Abstract: Encodes an IDT descriptor with the given physical address
#

ASM_GLOBAL ASM_PFX(Vect2Desc)
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

ASM_GLOBAL ASM_PFX(InterruptEntryStub)
ASM_PFX(InterruptEntryStub):
        mov    %esp,0x0                    # save stack top
        mov    $0x0,%esp                   # switch to debugger stack
        push   $0x0                        # push vector number - will be modified before installed
        jmp    ASM_PFX(CommonIdtEntry)     # jump CommonIdtEntry
ASM_GLOBAL ASM_PFX(InterruptEntryStubEnd)
ASM_PFX(InterruptEntryStubEnd):

#------------------------------------------------------------------------------
# CommonIdtEntry
#
# Abstract: This code is not a function, but is the common part for all IDT
#               vectors.
#
ASM_GLOBAL ASM_PFX(CommonIdtEntry)
ASM_PFX(CommonIdtEntry):
##
## At this point, the stub has saved the current application stack esp into AppEsp
## and switched stacks to the debug stack, where it pushed the vector number
##
## The application stack looks like this:
##
##              ...
##              (last application stack entry)
##              eflags from interrupted task
##              CS from interrupted task
##              EIP from interrupted task
##              Error code <-------------------- Only present for some exeption types
##
##


## The stub switched us to the debug stack and pushed the interrupt number.
##
## Next, construct the context record.  It will be build on the debug stack by
## pushing the registers in the correct order so as to create the context structure
## on the debug stack.  The context record must be built from the end back to the
## beginning because the stack grows down...
#
## For reference, the context record looks like this:
##
## typedef
## struct {
##   UINT32             ExceptionData;
##   FX_SAVE_STATE_IA32 FxSaveState;
##   UINT32             Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
##   UINT32             Cr0, Cr2, Cr3, Cr4;
##   UINT32             EFlags;
##   UINT32             Ldtr, Tr;
##   UINT32             Gdtr[2], Idtr[2];
##   UINT32             Eip;
##   UINT32             Gs, Fs, Es, Ds, Cs, Ss;
##   UINT32             Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
## } SYSTEM_CONTEXT_IA32;  // 32 bit system context record

## UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
        pusha
## Save interrupt state eflags register...
        pushf
        pop    %eax
## We need to determine if any extra data was pushed by the exception, and if so, save it
## To do this, we check the exception number pushed by the stub, and cache the
## result in a variable since we'll need this again.
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
## If there's some extra data, save it also, and modify the saved AppEsp to effectively
## pop this value off the application's stack.

        cmpl   $0x1,0x0
        jne    ASM_PFX(CommonIdtEntry+0xc8)
        mov    0x0,%eax
        mov    (%eax),%ebx
        mov    %ebx,0x0
        add    $0x4,%eax
        mov    %eax,0x0
        jmp    ASM_PFX(CommonIdtEntry+0xd2)
        movl   $0x0,0x0
## The "pushad" above pushed the debug stack esp.  Since what we're actually doing
## is building the context record on the debug stack, we need to save the pushed
## debug ESP, and replace it with the application's last stack entry...
        mov    0xc(%esp),%eax
        mov    %eax,0x0
        mov    0x0,%eax
        add    $0xc,%eax
        # application stack has eflags, cs, & eip, so
        # last actual application stack entry is
        # 12 bytes into the application stack.
        mov    %eax,0xc(%esp)
## continue building context record
## UINT32  Gs, Fs, Es, Ds, Cs, Ss;  insure high 16 bits of each is zero
        mov    %ss,%eax
        push   %eax
        
        # CS from application is one entry back in application stack
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

## UINT32  Eip;
        # Eip from application is on top of application stack
        mov    0x0,%eax
        pushl  (%eax)

## UINT32  Gdtr[2], Idtr[2];
        push   $0x0
        push   $0x0
        sidtl  (%esp)
        push   $0x0
        push   $0x0
        sgdtl  (%esp)

## UINT32  Ldtr, Tr;
        xor    %eax,%eax
        str    %eax
        push   %eax
        sldt   %eax
        push   %eax

## UINT32  EFlags;
## Eflags from application is two entries back in application stack
        mov    0x0,%eax
        pushl  0x8(%eax)

## UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
## insure FXSAVE/FXRSTOR is enabled in CR4...
## ... while we're at it, make sure DE is also enabled...
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

## UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
        mov    %db7,%eax
        push   %eax

## clear Dr7 while executing debugger itself
        xor    %eax,%eax
        mov    %eax,%db7
        mov    %db6,%eax
        push   %eax

## insure all status bits in dr6 are clear...
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

## FX_SAVE_STATE_IA32 FxSaveState;
        sub    $0x200,%esp
        mov    %esp,%edi
        # IMPORTANT!! The debug stack has been carefully constructed to
        # insure that esp and edi are 16 byte aligned when we get here.
        # They MUST be.  If they are not, a GP fault will occur.
        fxsave (%edi)

## UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
        cld
                
## UINT32  ExceptionData;
        mov    0x0,%eax
        push   %eax

# call to C code which will in turn call registered handler
# pass in the vector number
        mov    %esp,%eax
        push   %eax
        mov    0x0,%eax
        push   %eax
        call   ASM_PFX(CommonIdtEntry+0x184)
        add    $0x8,%esp

# restore context...
## UINT32  ExceptionData;
        add    $0x4,%esp

## FX_SAVE_STATE_IA32 FxSaveState;
        mov    %esp,%esi
        fxrstor (%esi)
        add    $0x200,%esp

## UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
        pop    %eax
        mov    %eax,%db0
        pop    %eax
        mov    %eax,%db1
        pop    %eax
        mov    %eax,%db2
        pop    %eax
        mov    %eax,%db3

## skip restore of dr6.  We cleared dr6 during the context save.
        add    $0x4,%esp
        pop    %eax
        mov    %eax,%db7

## UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
        pop    %eax
        mov    %eax,%cr0
        add    $0x4,%esp
        pop    %eax
        mov    %eax,%cr2
        pop    %eax
        mov    %eax,%cr3
        pop    %eax
        mov    %eax,%cr4

## UINT32  EFlags;
        mov    0x0,%eax
        popl   0x8(%eax)

## UINT32  Ldtr, Tr;
## UINT32  Gdtr[2], Idtr[2];
## Best not let anyone mess with these particular registers...
        add    $0x18,%esp

## UINT32  Eip;
        popl   (%eax)

## UINT32  SegGs, SegFs, SegEs, SegDs, SegCs, SegSs;
## NOTE - modified segment registers could hang the debugger...  We
##        could attempt to insulate ourselves against this possibility,
##        but that poses risks as well.
##

        pop    %gs
        pop    %fs
        pop    %es
        pop    %ds
        popl   0x4(%eax)
        pop    %ss
        mov    0xc(%esp),%ebx

## The next stuff to restore is the general purpose registers that were pushed
## using the "pushad" instruction.
##
## The value of ESP as stored in the context record is the application ESP
## including the 3 entries on the application stack caused by the exception
## itself. It may have been modified by the debug agent, so we need to
## determine if we need to relocate the application stack.

        mov    0x0,%eax          # move the potentially modified AppEsp into ebx
        add    $0xc,%eax
        cmp    %eax,%ebx
        je     ASM_PFX(CommonIdtEntry+0x202)
        mov    0x0,%eax
        mov    (%eax),%ecx       # EIP
        mov    %ecx,(%ebx)
        mov    0x4(%eax),%ecx    # CS
        mov    %ecx,0x4(%ebx)
        mov    0x8(%eax),%ecx    # EFLAGS
        mov    %ecx,0x8(%ebx)
	
        mov    %ebx,%eax         # modify the saved AppEsp to the new AppEsp
        mov    %eax,0x0
        mov    0x0,%eax          # restore the DebugEsp on the debug stack
	                               # so our "popad" will not cause a stack switch
        mov    %eax,0xc(%esp)    
        cmpl   $0x68,0x0
        jne    PhonyIretd+0xd
## Restore eflags so when we chain, the flags will be exactly as if we were never here.
## We gin up the stack to do an iretd so we can get ALL the flags.
        mov    0x0,%eax
        mov    0x8(%eax),%ebx
        and    $0xfffffcff,%ebx  # special handling for IF and TF
        push   %ebx
        push   %cs
        push   $0x0
        iret

PhonyIretd:
## UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
        popa

## Switch back to application stack
        mov    0x0,%esp
        jmp    *0x0
## Jump to original handler
## UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
        popa
## Switch back to application stack
        mov    0x0,%esp

## We're outa here...
        iret

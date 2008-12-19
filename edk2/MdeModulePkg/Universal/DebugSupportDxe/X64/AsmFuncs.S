#/**@file
# Low leve x64 specific debug support functions.
#
# Copyright (c) 2006 - 2008, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#**/    

.intel_syntax noprefix

.globl ASM_PFX(OrigVector)
.globl ASM_PFX(InterruptEntryStub)
.globl ASM_PFX(StubSize)
.globl ASM_PFX(CommonIdtEntry)
.globl ASM_PFX(FxStorSupport)
     
.data 

ASM_PFX(StubSize):         .long   ASM_PFX(InterruptEntryStubEnd) - ASM_PFX(InterruptEntryStub)
ASM_PFX(AppRsp):           .long   0x11111111 # ?
                           .long   0x11111111 # ?
ASM_PFX(DebugRsp):         .long   0x22222222 # ?
                           .long   0x22222222 # ?
ASM_PFX(ExtraPush):        .long   0x33333333 # ?
                           .long   0x33333333 # ?
ASM_PFX(ExceptData):       .long   0x44444444 # ?
                           .long   0x44444444 # ?
ASM_PFX(Rflags):           .long   0x55555555 # ?
                           .long   0x55555555 # ?
ASM_PFX(OrigVector):       .long   0x66666666 # ?
                           .long   0x66666666 # ?

## The declarations below define the memory region that will be used for the debug stack.
## The context record will be built by pushing register values onto this stack.
## It is imparitive that alignment be carefully managed, since the FXSTOR and
## FXRSTOR instructions will GP fault if their memory operand is not 16 byte aligned.
##
## The stub will switch stacks from the application stack to the debuger stack
## and pushes the exception number.
##
## Then we building the context record on the stack. Since the stack grows down,
## we push the fields of the context record from the back to the front.  There
## are 336 bytes of stack used prior allocating the 512 bytes of stack to be
## used as the memory buffer for the fxstor instruction. Therefore address of
## the buffer used for the FXSTOR instruction is &Eax - 336 - 512, which
## must be 16 byte aligned.
##
## We carefully locate the stack to make this happen.
##
## For reference, the context structure looks like this:
##      struct {
##        UINT64            ExceptionData;
##        FX_SAVE_STATE_X64 FxSaveState;    // 512 bytes, must be 16 byte aligned
##        UINT64            Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
##        UINT64            Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
##        UINT64            RFlags;
##        UINT64            Ldtr, Tr;
##        UINT64            Gdtr[2], Idtr[2];
##        UINT64            Rip;
##        UINT64            Gs, Fs, Es, Ds, Cs, Ss;
##        UINT64            Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
##        UINT64            R8, R9, R10, R11, R12, R13, R14, R15;
##      } SYSTEM_CONTEXT_X64;  // 64 bit system context record

.align          16
DebugStackEnd : .ascii     "DbgStkEnd >>>>>>"    # 16 byte long string - must be 16 bytes to preserve alignment
                .rept  0x1ffc
                  .long    0x00000000  
                .endr
                                                 # 32K should be enough stack
                                                 #   This allocation is coocked to insure
                                                 #   that the the buffer for the FXSTORE instruction
                                                 #   will be 16 byte aligned also.
                                                 #
ASM_PFX(ExceptionNumber):  .long   0x77777777    # first entry will be the vector number pushed by the stub
                           .long   0x77777777    # ?

DebugStackBegin : .ascii    "<<<< DbgStkBegin"   # initial debug ESP == DebugStackBegin, set in stub
           

.text

#------------------------------------------------------------------------------
# BOOLEAN
# FxStorSupport (
#   void
#   )
#
# Abstract: Returns TRUE if FxStor instructions are supported
#
.globl ASM_PFX(FxStorSupport)
ASM_PFX(FxStorSupport):  
#
# cpuid corrupts rbx which must be preserved per the C calling convention
#
                push    rbx
                mov     rax, 1
                cpuid
                mov     eax, edx
                and     rax, 0x01000000
                shr     rax, 24
                pop     rbx
                ret
#------------------------------------------------------------------------------
# void
# Vect2Desc (
#   IA32_IDT_GATE_DESCRIPTOR * DestDesc,  // rcx
#   void (*Vector) (void)   // rdx
#   )
#
# Abstract: Encodes an IDT descriptor with the given physical address
#
.globl ASM_PFX(Vect2Desc)
ASM_PFX(Vect2Desc):
                mov     rax, rdx
                mov     word ptr [rcx], ax                  # write bits 15..0 of offset
                mov     dx, cs
                mov     word ptr [rcx+2], dx                # SYS_CODE_SEL from GDT
                mov     word ptr [rcx+4], 0x0e00 OR 0x8000  # type = 386 interrupt gate, present
                shr     rax, 16
                mov     word ptr [rcx+6], ax                # write bits 31..16 of offset
                shr     rax, 16
                mov     dword ptr [rcx+8], eax              # write bits 63..32 of offset

                ret

#------------------------------------------------------------------------------
# InterruptEntryStub
#
# Abstract: This code is not a function, but is a small piece of code that is
#               copied and fixed up once for each IDT entry that is hooked.
#
.globl ASM_PFX(InterruptEntryStub)
ASM_PFX(InterruptEntryStub):

                push    0                       # push vector number - will be modified before installed
                jmp     ASM_PFX(CommonIdtEntry)
                
.globl ASM_PFX(InterruptEntryStubEnd)
ASM_PFX(InterruptEntryStubEnd):

#------------------------------------------------------------------------------
# CommonIdtEntry
#
# Abstract: This code is not a function, but is the common part for all IDT
#               vectors.
#
.globl ASM_PFX(CommonIdtEntry)
##
## At this point, the stub has saved the current application stack esp into AppRsp
## and switched stacks to the debug stack, where it pushed the vector number
##
## The application stack looks like this:
##
##              ...
##              (last application stack entry)
##              [16 bytes alignment, do not care it]
##              SS from interrupted task
##              RSP from interrupted task
##              rflags from interrupted task
##              CS from interrupted task
##              RIP from interrupted task
##              Error code <-------------------- Only present for some exeption types
##
##              Vector Number <----------------- pushed in our IDT Entry
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
##   UINT64            ExceptionData;
##   FX_SAVE_STATE_X64 FxSaveState;
##   UINT64            Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
##   UINT64            Cr0, Cr2, Cr3, Cr4, Cr8;
##   UINT64            RFlags;
##   UINT64            Ldtr, Tr;
##   UINT64            Gdtr[2], Idtr[2];
##   UINT64            Rip;
##   UINT64            Gs, Fs, Es, Ds, Cs, Ss;
##   UINT64            Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
##   UINT64            R8, R9, R10, R11, R12, R13, R14, R15;
## } SYSTEM_CONTEXT_X64;  // 64
ASM_PFX(CommonIdtEntry):
## NOTE: we save rsp here to prevent compiler put rip reference cause error AppRsp
                push    rax
                mov     rax, qword ptr [rsp][8]          # save vector number
                mov     ASM_PFX(ExceptionNumber), rax             # save vector number
                pop     rax
                add     rsp, 8                           # pop vector number
                mov     ASM_PFX(AppRsp), rsp                      # save stack top
                mov     rsp, offset DebugStackBegin      # switch to debugger stack
                sub     rsp, 8                           # leave space for vector number
## UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
## UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
                push    r15
                push    r14
                push    r13
                push    r12
                push    r11
                push    r10
                push    r9
                push    r8
                push    rax
                push    rcx
                push    rdx
                push    rbx
                push    rsp
                push    rbp
                push    rsi
                push    rdi
## Save interrupt state rflags register...
                pushfq
                pop     rax
                mov     qword ptr ASM_PFX(Rflags), rax
## We need to determine if any extra data was pushed by the exception, and if so, save it
## To do this, we check the exception number pushed by the stub, and cache the
## result in a variable since we'll need this again.
                cmp dword ptr ASM_PFX(ExceptionNumber), 0
                jz      ExtraPushOne
                cmp dword ptr     ASM_PFX(ExceptionNumber), 10
                jz      ExtraPushOne
                cmp dword ptr     ASM_PFX(ExceptionNumber), 11
                jz      ExtraPushOne
                cmp dword ptr     ASM_PFX(ExceptionNumber), 12
                jz      ExtraPushOne
                cmp dword ptr     ASM_PFX(ExceptionNumber), 13
                jz      ExtraPushOne
                cmp dword ptr     ASM_PFX(ExceptionNumber), 14
                jz      ExtraPushOne
                cmp dword ptr     ASM_PFX(ExceptionNumber), 17
                jz      ExtraPushOne
                mov dword ptr     ASM_PFX(ExtraPush), 0
                mov dword ptr     ASM_PFX(ExceptData), 0
                jmp     ExtraPushDone
ExtraPushOne:
                mov dword ptr     ASM_PFX(ExtraPush), 1

## If there's some extra data, save it also, and modify the saved AppRsp to effectively
## pop this value off the application's stack.
                mov     rax, ASM_PFX(AppRsp)
                mov     rbx, [rax]
                mov     ASM_PFX(ExceptData), rbx
                add     rax, 8
                mov     ASM_PFX(AppRsp), rax

ExtraPushDone:

## The "push" above pushed the debug stack rsp.  Since what we're actually doing
## is building the context record on the debug stack, we need to save the pushed
## debug RSP, and replace it with the application's last stack entry...
                mov     rax, [rsp + 24]
                mov     ASM_PFX(DebugRsp), rax
                mov     rax, ASM_PFX(AppRsp)
                add     rax, 40
                # application stack has ss, rsp, rflags, cs, & rip, so
                # last actual application stack entry is 40 bytes
                # into the application stack.
                mov     [rsp + 24], rax

## continue building context record
## UINT64  Gs, Fs, Es, Ds, Cs, Ss;  insure high 16 bits of each is zero
                mov     rax, ss
                push    rax
                # CS from application is one entry back in application stack
                mov     rax, ASM_PFX(AppRsp)
                movzx   rax, word ptr [rax + 8]
                push    rax

                mov     rax, ds
                push    rax
                mov     rax, es
                push    rax
                mov     rax, fs
                push    rax
                mov     rax, gs
                push    rax
## UINT64  Rip;
                # Rip from application is on top of application stack
                mov     rax, ASM_PFX(AppRsp)
                push    qword ptr [rax]
## UINT64  Gdtr[2], Idtr[2];
                push    0
                push    0
                sidt    [rsp]
                push    0
                push    0
                sgdt    [rsp]

## UINT64  Ldtr, Tr;
                xor     rax, rax
                str     ax
                push    rax
                sldt    ax
                push    rax

## UINT64  RFlags;
## Rflags from application is two entries back in application stack
                mov     rax, ASM_PFX(AppRsp)
                push    qword ptr [rax + 16]
## UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
## insure FXSAVE/FXRSTOR is enabled in CR4...
## ... while we're at it, make sure DE is also enabled...
                mov     rax, cr8
                push    rax
                mov     rax, cr4
                or      rax, 0x208
                mov     cr4, rax
                push    rax
                mov     rax, cr3
                push    rax
                mov     rax, cr2
                push    rax
                push    0
                mov     rax, cr0
                push    rax
## UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
                mov     rax, dr7
                push    rax
## clear Dr7 while executing debugger itself
                xor     rax, rax
                mov     dr7, rax

                mov     rax, dr6
                push    rax
## insure all status bits in dr6 are clear...
                xor     rax, rax
                mov     dr6, rax

                mov     rax, dr3
                push    rax
                mov     rax, dr2
                push    rax
                mov     rax, dr1
                push    rax
                mov     rax, dr0
                push    rax

## FX_SAVE_STATE_X64 FxSaveState;
                sub     rsp, 512
                mov     rdi, rsp
                # IMPORTANT!! The debug stack has been carefully constructed to
                # insure that rsp and rdi are 16 byte aligned when we get here.
                # They MUST be.  If they are not, a GP fault will occur.
          
                #     FXSTOR_RDI
                fxsave      [rdi]

## UINT64  ExceptionData;
                mov     rax, ASM_PFX(ExceptData)
                push    rax

# call to C code which will in turn call registered handler
# pass in the vector number
                mov     rdx, rsp
                mov     rcx, ASM_PFX(ExceptionNumber)
                sub     rsp, 40
                call    ASM_PFX(InterruptDistrubutionHub)
                add     rsp, 40
# restore context...
## UINT64  ExceptionData;
                add     rsp, 8

## FX_SAVE_STATE_X64 FxSaveState;
                mov     rsi, rsp
            
                #    FXRSTOR_RSI
                fxrstor     [rsi]

                add     rsp, 512

## UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
                pop     rax
                mov     dr0, rax
                pop     rax
                mov     dr1, rax
                pop     rax
                mov     dr2, rax
                pop     rax
                mov     dr3, rax

## skip restore of dr6.  We cleared dr6 during the context save.
                add     rsp, 8
                pop     rax
                mov     dr7, rax

## UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
                pop     rax
                mov     cr0, rax
                add     rsp, 8
                pop     rax
                mov     cr2, rax
                pop     rax
                mov     cr3, rax
                pop     rax
                mov     cr4, rax
                pop     rax
                mov     cr8, rax
## UINT64  RFlags;
                mov     rax, ASM_PFX(AppRsp)
                pop     qword ptr [rax + 16]
## UINT64  Ldtr, Tr;
## UINT64  Gdtr[2], Idtr[2];
## Best not let anyone mess with these particular registers...
                add     rsp, 48
## UINT64  Rip;
                pop     qword ptr [rax]

## UINT64  Gs, Fs, Es, Ds, Cs, Ss;
## NOTE - modified segment registers could hang the debugger...  We
##        could attempt to insulate ourselves against this possibility,
##        but that poses risks as well.
##

                pop     rax
                # mov     gs, rax
                pop     rax
                # mov     fs, rax
                pop     rax
                mov     es, rax
                pop     rax
                mov     ds, rax
                mov     rax, ASM_PFX(AppRsp)
                pop     qword ptr [rax + 8]
                pop     rax
                mov     ss, rax
## The next stuff to restore is the general purpose registers that were pushed
## using the "push" instruction.
##
## The value of RSP as stored in the context record is the application RSP
## including the 5 entries on the application stack caused by the exception
## itself. It may have been modified by the debug agent, so we need to
## determine if we need to relocate the application stack.

                mov     rbx, [rsp + 24]  # move the potentially modified AppRsp into rbx
                mov     rax, ASM_PFX(AppRsp)
                add     rax, 40
                cmp     rbx, rax
                je      NoAppStackMove

                mov     rax, ASM_PFX(AppRsp)
                mov     rcx, [rax]       # RIP
                mov     [rbx], rcx

                mov     rcx, [rax + 8]   # CS
                mov     [rbx + 8], rcx

                mov     rcx, [rax + 16]  # RFLAGS
                mov     [rbx + 16], rcx

                mov     rcx, [rax + 24]  # RSP
                mov     [rbx + 24], rcx

                mov     rcx, [rax + 32]  # SS
                mov     [rbx + 32], rcx

                mov     rax, rbx         # modify the saved AppRsp to the new AppRsp
                mov     ASM_PFX(AppRsp), rax
NoAppStackMove:
                mov     rax, ASM_PFX(DebugRsp)    # restore the DebugRsp on the debug stack
                                         # so our "pop" will not cause a stack switch
                mov     [rsp + 24], rax

                cmp dword ptr ASM_PFX(ExceptionNumber), 0x068
                jne     NoChain

Chain:

## Restore rflags so when we chain, the flags will be exactly as if we were never here.
## We gin up the stack to do an iretq so we can get ALL the flags.
                mov     rax, ASM_PFX(AppRsp)
                mov     rbx, [rax + 40]
                push    rbx
                mov     rax, ss
                push    rax
                mov     rax, rsp
                add     rax, 16
                push    rax
                mov     rax, ASM_PFX(AppRsp)
                mov     rbx, [rax + 16]
                and     rbx, NOT 0x300 # special handling for IF and TF
                push    rbx
                mov     rax, cs
                push    rax
                mov     rax, offset PhonyIretq
                push    rax
                iretq
PhonyIretq:

## UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
## UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
                pop     rdi
                pop     rsi
                pop     rbp
                pop     rsp
                pop     rbx
                pop     rdx
                pop     rcx
                pop     rax
                pop     r8
                pop     r9
                pop     r10
                pop     r11
                pop     r12
                pop     r13
                pop     r14
                pop     r15

## Switch back to application stack
                mov     rsp, ASM_PFX(AppRsp)
## Jump to original handler
                jmp     ASM_PFX(OrigVector)
NoChain:
## UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
## UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
                pop     rdi
                pop     rsi
                pop     rbp
                pop     rsp
                pop     rbx
                pop     rdx
                pop     rcx
                pop     rax
                pop     r8
                pop     r9
                pop     r10
                pop     r11
                pop     r12
                pop     r13
                pop     r14
                pop     r15

## Switch back to application stack
                mov     rsp, ASM_PFX(AppRsp)

## We're outa here...
                iret

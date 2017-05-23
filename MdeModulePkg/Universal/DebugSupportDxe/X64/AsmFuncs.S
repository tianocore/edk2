///**@file
// Low leve x64 specific debug support functions.
//
// Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
// Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//**/

ASM_GLOBAL ASM_PFX(OrigVector)
ASM_GLOBAL ASM_PFX(InterruptEntryStub)
ASM_GLOBAL ASM_PFX(StubSize)
ASM_GLOBAL ASM_PFX(CommonIdtEntry)
ASM_GLOBAL ASM_PFX(FxStorSupport)

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

// The declarations below define the memory region that will be used for the debug stack.
// The context record will be built by pushing register values onto this stack.
// It is imparitive that alignment be carefully managed, since the FXSTOR and
// FXRSTOR instructions will GP fault if their memory operand is not 16 byte aligned.
//
// The stub will switch stacks from the application stack to the debuger stack
// and pushes the exception number.
//
// Then we building the context record on the stack. Since the stack grows down,
// we push the fields of the context record from the back to the front.  There
// are 336 bytes of stack used prior allocating the 512 bytes of stack to be
// used as the memory buffer for the fxstor instruction. Therefore address of
// the buffer used for the FXSTOR instruction is &Eax - 336 - 512, which
// must be 16 byte aligned.
//
// We carefully locate the stack to make this happen.
//
// For reference, the context structure looks like this:
//      struct {
//        UINT64            ExceptionData;
//        FX_SAVE_STATE_X64 FxSaveState;    // 512 bytes, must be 16 byte aligned
//        UINT64            Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
//        UINT64            Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
//        UINT64            RFlags;
//        UINT64            Ldtr, Tr;
//        UINT64            Gdtr[2], Idtr[2];
//        UINT64            Rip;
//        UINT64            Gs, Fs, Es, Ds, Cs, Ss;
//        UINT64            Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
//        UINT64            R8, R9, R10, R11, R12, R13, R14, R15;
//      } SYSTEM_CONTEXT_X64;  // 64 bit system context record

.p2align 4
DebugStackEnd : .ascii     "DbgStkEnd >>>>>>"    # 16 byte long string - must be 16 bytes to preserve alignment
                .fill 0x1ffc, 4, 0x00000000
                                                 # 32K should be enough stack
                                                 #   This allocation is coocked to insure
                                                 #   that the the buffer for the FXSTORE instruction
                                                 #   will be 16 byte aligned also.
                                                 #
ASM_PFX(ExceptionNumber):  .long   0x77777777    # first entry will be the vector number pushed by the stub
                           .long   0x77777777    # ?

DebugStackBegin : .ascii    "<<<< DbgStkBegin"   # initial debug ESP == DebugStackBegin, set in stub


.text

//------------------------------------------------------------------------------
// BOOLEAN
// FxStorSupport (
//   void
//   )
//
// Abstract: Returns TRUE if FxStor instructions are supported
//
ASM_GLOBAL ASM_PFX(FxStorSupport)
ASM_PFX(FxStorSupport):
//
// cpuid corrupts rbx which must be preserved per the C calling convention
//
                pushq   %rbx
                movq    $1, %rax
                cpuid
                movl    %edx, %eax
                andq    $0x01000000, %rax
                shrq    $24, %rax
                popq    %rbx
                ret
//------------------------------------------------------------------------------
// void
// Vect2Desc (
//   IA32_IDT_GATE_DESCRIPTOR * DestDesc,  // rcx
//   void (*Vector) (void)   // rdx
//   )
//
// Abstract: Encodes an IDT descriptor with the given physical address
//
ASM_GLOBAL ASM_PFX(Vect2Desc)
ASM_PFX(Vect2Desc):
                movq    %rdx, %rax
                movw    %ax, (%rcx)                  # write bits 15..0 of offset
                movw    %cs, %dx
                movw    %dx, 2(%rcx)                 # SYS_CODE_SEL from GDT
                movw    $(0x0e00 | 0x8000), 4(%rcx)  # type = 386 interrupt gate, present
                shrq    $16, %rax
                movw    %ax, 6(%rcx)                 # write bits 31..16 of offset
                shrq    $16, %rax
                movl    %eax, 8(%rcx)                # write bits 63..32 of offset

                ret

//------------------------------------------------------------------------------
// InterruptEntryStub
//
// Abstract: This code is not a function, but is a small piece of code that is
//               copied and fixed up once for each IDT entry that is hooked.
//
ASM_GLOBAL ASM_PFX(InterruptEntryStub)
ASM_PFX(InterruptEntryStub):

                pushq  $0                       # push vector number - will be modified before installed
                jmp     ASM_PFX(CommonIdtEntry)

ASM_GLOBAL ASM_PFX(InterruptEntryStubEnd)
ASM_PFX(InterruptEntryStubEnd):

//------------------------------------------------------------------------------
// CommonIdtEntry
//
// Abstract: This code is not a function, but is the common part for all IDT
//               vectors.
//
ASM_GLOBAL ASM_PFX(CommonIdtEntry)
//
// At this point, the stub has saved the current application stack esp into AppRsp
// and switched stacks to the debug stack, where it pushed the vector number
//
// The application stack looks like this:
//
//              ...
//              (last application stack entry)
//              [16 bytes alignment, do not care it]
//              SS from interrupted task
//              RSP from interrupted task
//              rflags from interrupted task
//              CS from interrupted task
//              RIP from interrupted task
//              Error code <-------------------- Only present for some exeption types
//
//              Vector Number <----------------- pushed in our IDT Entry
//


// The stub switched us to the debug stack and pushed the interrupt number.
//
// Next, construct the context record.  It will be build on the debug stack by
// pushing the registers in the correct order so as to create the context structure
// on the debug stack.  The context record must be built from the end back to the
// beginning because the stack grows down...
//
// For reference, the context record looks like this:
//
// typedef
// struct {
//   UINT64            ExceptionData;
//   FX_SAVE_STATE_X64 FxSaveState;
//   UINT64            Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
//   UINT64            Cr0, Cr2, Cr3, Cr4, Cr8;
//   UINT64            RFlags;
//   UINT64            Ldtr, Tr;
//   UINT64            Gdtr[2], Idtr[2];
//   UINT64            Rip;
//   UINT64            Gs, Fs, Es, Ds, Cs, Ss;
//   UINT64            Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
//   UINT64            R8, R9, R10, R11, R12, R13, R14, R15;
// } SYSTEM_CONTEXT_X64;  // 64
ASM_PFX(CommonIdtEntry):
// NOTE: we save rsp here to prevent compiler put rip reference cause error AppRsp
                pushq   %rax
                movq    (8)(%rsp), %rax                  # save vector number
                movq    %rax, ASM_PFX(ExceptionNumber)(%rip)   # save vector number
                popq    %rax
                addq    $8, %rsp                         # pop vector number
                movq    %rsp, ASM_PFX(AppRsp)(%rip)      # save stack top
                movq    DebugStackBegin(%rip), %rsp      # switch to debugger stack
                subq    $8, %rsp                         # leave space for vector number
// UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
// UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
                pushq    %r15
                pushq    %r14
                pushq    %r13
                pushq    %r12
                pushq    %r11
                pushq    %r10
                pushq    %r9
                pushq    %r8
                pushq    %rax
                pushq    %rcx
                pushq    %rdx
                pushq    %rbx
                pushq    %rsp
                pushq    %rbp
                pushq    %rsi
                pushq    %rdi
// Save interrupt state rflags register...
                pushfq
                popq    %rax
                movq    %rax, ASM_PFX(Rflags)(%rip)
// We need to determine if any extra data was pushed by the exception, and if so, save it
// To do this, we check the exception number pushed by the stub, and cache the
// result in a variable since we'll need this again.
                cmpl    $0, ASM_PFX(ExceptionNumber)(%rip)
                jz      ExtraPushOne
                cmpl    $10, ASM_PFX(ExceptionNumber)(%rip)
                jz      ExtraPushOne
                cmpl    $11, ASM_PFX(ExceptionNumber)(%rip)
                jz      ExtraPushOne
                cmpl    $12, ASM_PFX(ExceptionNumber)(%rip)
                jz      ExtraPushOne
                cmpl    $13, ASM_PFX(ExceptionNumber)(%rip)
                jz      ExtraPushOne
                cmpl    $14, ASM_PFX(ExceptionNumber)(%rip)
                jz      ExtraPushOne
                cmpl    $17, ASM_PFX(ExceptionNumber)(%rip)
                jz      ExtraPushOne
                movl    $0, ASM_PFX(ExtraPush)(%rip)
                movl    $0, ASM_PFX(ExceptData)(%rip)
                jmp     ExtraPushDone
ExtraPushOne:
                movl  $1, ASM_PFX(ExtraPush)(%rip)

// If there's some extra data, save it also, and modify the saved AppRsp to effectively
// pop this value off the application's stack.
                movq    ASM_PFX(AppRsp)(%rip), %rax
                movq    (%rax), %rbx
                movq    %rbx, ASM_PFX(ExceptData)(%rip)
                addq    $8, %rax
                movq    %rax, ASM_PFX(AppRsp)(%rip)

ExtraPushDone:

// The "push" above pushed the debug stack rsp.  Since what we're actually doing
// is building the context record on the debug stack, we need to save the pushed
// debug RSP, and replace it with the application's last stack entry...
                movq    24(%rsp), %rax
                movq    %rax, ASM_PFX(DebugRsp)(%rip)
                movq    ASM_PFX(AppRsp)(%rip), %rax
                movq    24(%rax), %rax
                # application stack has ss, rsp, rflags, cs, & rip, so
                # last actual application stack entry is saved at offset
                # 24 bytes from stack top.
                movq    %rax, 24(%rsp)

// continue building context record
// UINT64  Gs, Fs, Es, Ds, Cs, Ss;  insure high 16 bits of each is zero
                mov     %ss, %rax
                pushq   %rax
                # CS from application is one entry back in application stack
                movq    ASM_PFX(AppRsp)(%rip), %rax
                movzwq  8(%rax), %rax
                pushq   %rax

                mov     %ds, %rax
                pushq   %rax
                mov     %es, %rax
                pushq   %rax
                mov     %fs, %rax
                pushq   %rax
                mov     %gs, %rax
                pushq   %rax
// UINT64  Rip;
                # Rip from application is on top of application stack
                movq    ASM_PFX(AppRsp)(%rip), %rax
                pushq   (%rax)
// UINT64  Gdtr[2], Idtr[2];
                push    $0
                push    $0
                sidtq   (%rsp)
                push    $0
                push    $0
                sgdtq   (%rsp)

// UINT64  Ldtr, Tr;
                xorq    %rax, %rax
                str     %ax
                pushq   %rax
                sldt    %ax
                pushq   %rax

// UINT64  RFlags;
// Rflags from application is two entries back in application stack
                movq    ASM_PFX(AppRsp)(%rip), %rax
                pushq   16(%rax)
// UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
// insure FXSAVE/FXRSTOR is enabled in CR4...
// ... while we're at it, make sure DE is also enabled...
                movq    %cr8, %rax
                pushq   %rax
                movq    %cr4, %rax
                orq     $0x208, %rax
                movq    %rax, %cr4
                pushq   %rax
                movq    %cr3, %rax
                pushq   %rax
                movq    %cr2, %rax
                pushq   %rax
                push    $0
                movq    %cr0, %rax
                pushq   %rax
// UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
                movq    %dr7, %rax
                pushq   %rax
// clear Dr7 while executing debugger itself
                xorq    %rax, %rax
                movq    %rax, %dr7

                movq    %dr6, %rax
                pushq   %rax
// insure all status bits in dr6 are clear...
                xorq    %rax, %rax
                movq    %rax, %dr6

                movq    %dr3, %rax
                pushq   %rax
                movq    %dr2, %rax
                pushq   %rax
                movq    %dr1, %rax
                pushq   %rax
                movq    %dr0, %rax
                pushq   %rax

// FX_SAVE_STATE_X64 FxSaveState;
                subq    $512, %rsp
                movq    %rsp, %rdi
                # IMPORTANT!! The debug stack has been carefully constructed to
                # insure that rsp and rdi are 16 byte aligned when we get here.
                # They MUST be.  If they are not, a GP fault will occur.

                #     FXSTOR_RDI
                fxsave      (%rdi)

// UEFI calling convention for x64 requires that Direction flag in EFLAGs is clear
                cld
                
// UINT64  ExceptionData;
                movq    ASM_PFX(ExceptData)(%rip),  %rax
                pushq   %rax

// call to C code which will in turn call registered handler
// pass in the vector number
                movq    %rsp, %rdx
                movq    ASM_PFX(ExceptionNumber)(%rip), %rcx
                subq    $40, %rsp
                call    ASM_PFX(InterruptDistrubutionHub)
                addq    $40, %rsp
// restore context...
// UINT64  ExceptionData;
                addq    $8, %rsp

// FX_SAVE_STATE_X64 FxSaveState;
                movq    %rsp, %rsi

                #    FXRSTOR_RSI
                fxrstor     (%rsi)

                addq    $512, %rsp

// UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
                popq    %rax
                movq    %rax, %dr0
                popq    %rax
                movq    %rax, %dr1
                popq    %rax
                movq    %rax, %dr2
                popq    %rax
                movq    %rax, %dr3

// skip restore of dr6.  We cleared dr6 during the context save.
                addq    $8, %rsp
                popq    %rax
                movq    %rax, %dr7

// UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
                popq    %rax
                movq    %rax, %cr0
                addq    $8, %rsp
                popq    %rax
                movq    %rax, %cr2
                popq    %rax
                movq    %rax, %cr3
                popq    %rax
                movq    %rax, %cr4
                popq    %rax
                movq    %rax, %cr8
// UINT64  RFlags;
                movq    ASM_PFX(AppRsp)(%rip), %rax
                popq    16(%rax)
// UINT64  Ldtr, Tr;
// UINT64  Gdtr[2], Idtr[2];
// Best not let anyone mess with these particular registers...
                addq    $48, %rsp
// UINT64  Rip;
                popq    (%rax)

// UINT64  Gs, Fs, Es, Ds, Cs, Ss;
// NOTE - modified segment registers could hang the debugger...  We
//        could attempt to insulate ourselves against this possibility,
//        but that poses risks as well.
//

                popq    %rax
                # mov     %rax, %gs
                popq    %rax
                # mov     %rax, %fs
                popq    %rax
                mov     %rax, %es
                popq    %rax
                mov     %rax, %ds
                movq    ASM_PFX(AppRsp)(%rip), %rax
                popq    8(%rax)
                popq    %rax
                mov     %rax, %ss
## The next stuff to restore is the general purpose registers that were pushed
## using the "push" instruction.
##
## The value of RSP as stored in the context record is the application RSP
## including the 5 entries on the application stack caused by the exception
## itself. It may have been modified by the debug agent, so we need to
## determine if we need to relocate the application stack.

                movq    24(%rsp), %rbx  # move the potentially modified AppRsp into rbx
                movq    ASM_PFX(AppRsp)(%rip), %rax
                movq    24(%rax), %rax
                cmpq    %rax, %rbx
                je      NoAppStackMove

                movq    ASM_PFX(AppRsp)(%rip), %rax
                movq    (%rax), %rcx     # RIP
                movq    %rcx, (%rbx)

                movq    8(%rax), %rcx    # CS
                movq    %rcx, 8(%rbx)

                movq    16(%rax), %rcx   # RFLAGS
                movq    %rcx, 16(%rbx)

                movq    24(%rax), %rcx   # RSP
                movq    %rcx, 24(%rbx)

                movq    32(%rax), %rcx   # SS
                movq    %rcx, 32(%rbx)

                movq    %rbx, %rax       # modify the saved AppRsp to the new AppRsp
                movq    %rax, ASM_PFX(AppRsp)(%rip)
NoAppStackMove:
                movq    ASM_PFX(DebugRsp)(%rip), %rax   # restore the DebugRsp on the debug stack
                                         # so our "pop" will not cause a stack switch
                movq    %rax, 24(%rsp)

                cmpl    $0x068, ASM_PFX(ExceptionNumber)(%rip)
                jne     NoChain

Chain:

// Restore rflags so when we chain, the flags will be exactly as if we were never here.
// We gin up the stack to do an iretq so we can get ALL the flags.
                movq    ASM_PFX(AppRsp)(%rip), %rax
                movq    40(%rax), %rbx
                pushq   %rbx
                mov     %ss, %rax
                pushq   %rax
                movq    %rsp, %rax
                addq    $16, %rax
                pushq   %rax
                movq    ASM_PFX(AppRsp)(%rip), %rax
                movq    16(%rax), %rbx
                andq    $0xfffffffffffffcff, %rbx  # special handling for IF and TF
                pushq   %rbx
                mov     %cs, %rax
                pushq   %rax
                movq    PhonyIretq(%rip), %rax
                pushq   %rax
                iretq
PhonyIretq:

// UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
// UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
                popq    %rdi
                popq    %rsi
                popq    %rbp
                popq    %rsp
                popq    %rbx
                popq    %rdx
                popq    %rcx
                popq    %rax
                popq    %r8
                popq    %r9
                popq    %r10
                popq    %r11
                popq    %r12
                popq    %r13
                popq    %r14
                popq    %r15

// Switch back to application stack
                movq    ASM_PFX(AppRsp)(%rip), %rsp
// Jump to original handler
                jmp     ASM_PFX(OrigVector)
NoChain:
// UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
// UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
                popq    %rdi
                popq    %rsi
                popq    %rbp
                popq    %rsp
                popq    %rbx
                popq    %rdx
                popq    %rcx
                popq    %rax
                popq    %r8
                popq    %r9
                popq    %r10
                popq    %r11
                popq    %r12
                popq    %r13
                popq    %r14
                popq    %r15

// Switch back to application stack
                movq    ASM_PFX(AppRsp)(%rip), %rsp

// We're outa here...
                iret

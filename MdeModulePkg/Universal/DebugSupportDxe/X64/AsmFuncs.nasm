;/** @file
;  Low level x64 routines used by the debug support driver.
;
;  Copyright (c) 2007 - 2022, Intel Corporation. All rights reserved.<BR>
;  SPDX-License-Identifier: BSD-2-Clause-Patent
;
;**/

%define EXCPT64_DIVIDE_ERROR 0
%define EXCPT64_DEBUG 1
%define EXCPT64_NMI 2
%define EXCPT64_BREAKPOINT 3
%define EXCPT64_OVERFLOW 4
%define EXCPT64_BOUND 5
%define EXCPT64_INVALID_OPCODE 6
%define EXCPT64_DOUBLE_FAULT 8
%define EXCPT64_INVALID_TSS 10
%define EXCPT64_SEG_NOT_PRESENT 11
%define EXCPT64_STACK_FAULT 12
%define EXCPT64_GP_FAULT 13
%define EXCPT64_PAGE_FAULT 14
%define EXCPT64_FP_ERROR 16
%define EXCPT64_ALIGNMENT_CHECK 17
%define EXCPT64_MACHINE_CHECK 18
%define EXCPT64_SIMD 19

%define FXSTOR_FLAG 0x1000000         ; bit cpuid 24 of feature flags

SECTION .data

global ASM_PFX(OrigVector)
global ASM_PFX(InterruptEntryStub)
global ASM_PFX(StubSize)
global ASM_PFX(CommonIdtEntry)
global ASM_PFX(FxStorSupport)
extern ASM_PFX(InterruptDistrubutionHub)

ASM_PFX(StubSize): dd InterruptEntryStubEnd - ASM_PFX(InterruptEntryStub)
AppRsp: dq 0x1111111111111111 ; ?
DebugRsp: dq 0x2222222222222222 ; ?
ExtraPush: dq 0x3333333333333333 ; ?
ExceptData: dq 0x4444444444444444 ; ?
Rflags: dq 0x5555555555555555 ; ?
ASM_PFX(OrigVector): dq 0x6666666666666666 ; ?

;; The declarations below define the memory region that will be used for the debug stack.
;; The context record will be built by pushing register values onto this stack.
;; It is imparitive that alignment be carefully managed, since the FXSTOR and
;; FXRSTOR instructions will GP fault if their memory operand is not 16 byte aligned.
;;
;; The stub will switch stacks from the application stack to the debuger stack
;; and pushes the exception number.
;;
;; Then we building the context record on the stack. Since the stack grows down,
;; we push the fields of the context record from the back to the front.  There
;; are 336 bytes of stack used prior allocating the 512 bytes of stack to be
;; used as the memory buffer for the fxstor instruction. Therefore address of
;; the buffer used for the FXSTOR instruction is &Eax - 336 - 512, which
;; must be 16 byte aligned.
;;
;; We carefully locate the stack to make this happen.
;;
;; For reference, the context structure looks like this:
;;      struct {
;;        UINT64            ExceptionData;
;;        FX_SAVE_STATE_X64 FxSaveState;    // 512 bytes, must be 16 byte aligned
;;        UINT64            Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
;;        UINT64            Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
;;        UINT64            RFlags;
;;        UINT64            Ldtr, Tr;
;;        UINT64            Gdtr[2], Idtr[2];
;;        UINT64            Rip;
;;        UINT64            Gs, Fs, Es, Ds, Cs, Ss;
;;        UINT64            Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
;;        UINT64            R8, R9, R10, R11, R12, R13, R14, R15;
;;      } SYSTEM_CONTEXT_X64;  // 64 bit system context record

align           16
DebugStackEnd: db "DbgStkEnd >>>>>>"    ;; 16 byte long string - must be 16 bytes to preserve alignment
                times 0x1ffc dd    0x0  ;; 32K should be enough stack
                                        ;;   This allocation is coocked to insure
                                        ;;   that the the buffer for the FXSTORE instruction
                                        ;;   will be 16 byte aligned also.
                                        ;;
ExceptionNumber: dq 0                   ;; first entry will be the vector number pushed by the stub

DebugStackBegin: db "<<<< DbgStkBegin"  ;; initial debug ESP == DebugStackBegin, set in stub

DEFAULT REL
SECTION .text

;------------------------------------------------------------------------------
; BOOLEAN
; FxStorSupport (
;   void
;   )
;
; Abstract: Returns TRUE if FxStor instructions are supported
;
global ASM_PFX(FxStorSupport)
ASM_PFX(FxStorSupport):

;
; cpuid corrupts rbx which must be preserved per the C calling convention
;
                push    rbx
                mov     rax, dword 1
                cpuid
                mov     eax, edx
                and     rax, FXSTOR_FLAG
                shr     rax, 24
                pop     rbx
                ret

;------------------------------------------------------------------------------
; void
; Vect2Desc (
;   IA32_IDT_GATE_DESCRIPTOR * DestDesc,  // rcx
;   void (*Vector) (void)   // rdx
;   )
;
; Abstract: Encodes an IDT descriptor with the given physical address
;
global ASM_PFX(Vect2Desc)
ASM_PFX(Vect2Desc):

                mov     rax, rdx
                mov     word [rcx], ax                  ; write bits 15..0 of offset
                mov     dx, cs
                mov     word [rcx+2], dx                ; SYS_CODE_SEL from GDT
                mov     word [rcx+4], 0xe00 | 0x8000    ; type = 386 interrupt gate, present
                shr     rax, 16
                mov     word [rcx+6], ax                ; write bits 31..16 of offset
                shr     rax, 16
                mov     dword [rcx+8], eax              ; write bits 63..32 of offset

                ret

;------------------------------------------------------------------------------
; InterruptEntryStub
;
; Abstract: This code is not a function, but is a small piece of code that is
;               copied and fixed up once for each IDT entry that is hooked.
;
ASM_PFX(InterruptEntryStub):
                push    0                       ; push vector number - will be modified before installed
                db      0xe9                    ; jump rel32
                dd      0                       ; fixed up to relative address of CommonIdtEntry
InterruptEntryStubEnd:

;------------------------------------------------------------------------------
; CommonIdtEntry
;
; Abstract: This code is not a function, but is the common part for all IDT
;               vectors.
;
ASM_PFX(CommonIdtEntry):
;;
;; At this point, the stub has saved the current application stack esp into AppRsp
;; and switched stacks to the debug stack, where it pushed the vector number
;;
;; The application stack looks like this:
;;
;;              ...
;;              (last application stack entry)
;;              [16 bytes alignment, do not care it]
;;              SS from interrupted task
;;              RSP from interrupted task
;;              rflags from interrupted task
;;              CS from interrupted task
;;              RIP from interrupted task
;;              Error code <-------------------- Only present for some exeption types
;;
;;              Vector Number <----------------- pushed in our IDT Entry
;;

;; The stub switched us to the debug stack and pushed the interrupt number.
;;
;; Next, construct the context record.  It will be build on the debug stack by
;; pushing the registers in the correct order so as to create the context structure
;; on the debug stack.  The context record must be built from the end back to the
;; beginning because the stack grows down...
;
;; For reference, the context record looks like this:
;;
;; typedef
;; struct {
;;   UINT64            ExceptionData;
;;   FX_SAVE_STATE_X64 FxSaveState;
;;   UINT64            Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
;;   UINT64            Cr0, Cr2, Cr3, Cr4, Cr8;
;;   UINT64            RFlags;
;;   UINT64            Ldtr, Tr;
;;   UINT64            Gdtr[2], Idtr[2];
;;   UINT64            Rip;
;;   UINT64            Gs, Fs, Es, Ds, Cs, Ss;
;;   UINT64            Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
;;   UINT64            R8, R9, R10, R11, R12, R13, R14, R15;
;; } SYSTEM_CONTEXT_X64;  // 64 bit system context record

;; NOTE: we save rsp here to prevent compiler put rip reference cause error AppRsp
                push    rax
                mov     rax, qword [rsp+8]               ; save vector number
                mov     [ExceptionNumber], rax           ; save vector number
                pop     rax
                add     rsp, 8                           ; pop vector number
                mov     [AppRsp], rsp                    ; save stack top
                lea     rsp, [DebugStackBegin]           ; switch to debugger stack
                sub     rsp, 8                           ; leave space for vector number

;; UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
;; UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
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

;; Save interrupt state rflags register...
                pushfq
                pop     rax
                mov     [Rflags], rax

;; We need to determine if any extra data was pushed by the exception, and if so, save it
;; To do this, we check the exception number pushed by the stub, and cache the
;; result in a variable since we'll need this again.
                cmp     qword [ExceptionNumber], EXCPT64_DOUBLE_FAULT
                jz      ExtraPushOne
                cmp     qword [ExceptionNumber], EXCPT64_INVALID_TSS
                jz      ExtraPushOne
                cmp     qword [ExceptionNumber], EXCPT64_SEG_NOT_PRESENT
                jz      ExtraPushOne
                cmp     qword [ExceptionNumber], EXCPT64_STACK_FAULT
                jz      ExtraPushOne
                cmp     qword [ExceptionNumber], EXCPT64_GP_FAULT
                jz      ExtraPushOne
                cmp     qword [ExceptionNumber], EXCPT64_PAGE_FAULT
                jz      ExtraPushOne
                cmp     qword [ExceptionNumber], EXCPT64_ALIGNMENT_CHECK
                jz      ExtraPushOne
                mov     qword [ExtraPush], 0
                mov     qword [ExceptData], 0
                jmp     ExtraPushDone
ExtraPushOne:
                mov     qword [ExtraPush], 1

;; If there's some extra data, save it also, and modify the saved AppRsp to effectively
;; pop this value off the application's stack.
                mov     rax, [AppRsp]
                mov     rbx, [rax]
                mov     qword [ExceptData], rbx
                add     rax, 8
                mov     [AppRsp], rax

ExtraPushDone:

;; The "push" above pushed the debug stack rsp.  Since what we're actually doing
;; is building the context record on the debug stack, we need to save the pushed
;; debug RSP, and replace it with the application's last stack entry...
                mov     rax, [rsp + 24]
                mov     [DebugRsp], rax
                mov     rax, [AppRsp]
                mov     rax, QWORD [rax + 24]
                ; application stack has ss, rsp, rflags, cs, & rip, so
                ; last actual application stack entry is saved at offset
                ; 24 bytes from stack top.
                mov     [rsp + 24], rax

;; continue building context record
;; UINT64  Gs, Fs, Es, Ds, Cs, Ss;  insure high 16 bits of each is zero
                mov     rax, ss
                push    rax

                ; CS from application is one entry back in application stack
                mov     rax, [AppRsp]
                movzx   rax, word [rax + 8]
                push    rax

                mov     rax, ds
                push    rax
                mov     rax, es
                push    rax
                mov     rax, fs
                push    rax
                mov     rax, gs
                push    rax

;; UINT64  Rip;
                ; Rip from application is on top of application stack
                mov     rax, [AppRsp]
                push    qword [rax]

;; UINT64  Gdtr[2], Idtr[2];
                push    0
                push    0
                sidt    [rsp]
                push    0
                push    0
                sgdt    [rsp]

;; UINT64  Ldtr, Tr;
                xor     rax, rax
                str     ax
                push    rax
                sldt    ax
                push    rax

;; UINT64  RFlags;
;; Rflags from application is two entries back in application stack
                mov     rax, [AppRsp]
                push    qword [rax + 16]

;; UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
;; insure FXSAVE/FXRSTOR is enabled in CR4...
;; ... while we're at it, make sure DE is also enabled...
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

;; UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
                mov     rax, dr7
                push    rax
;; clear Dr7 while executing debugger itself
                xor     rax, rax
                mov     dr7, rax

                mov     rax, dr6
                push    rax
;; insure all status bits in dr6 are clear...
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

;; FX_SAVE_STATE_X64 FxSaveState;
                sub     rsp, 512
                mov     rdi, rsp
                ; IMPORTANT!! The debug stack has been carefully constructed to
                ; insure that rsp and rdi are 16 byte aligned when we get here.
                ; They MUST be.  If they are not, a GP fault will occur.
                fxsave  [rdi]

;; UEFI calling convention for x64 requires that Direction flag in EFLAGs is clear
                cld

;; UINT64  ExceptionData;
                mov     rax, [ExceptData]
                push    rax

; call to C code which will in turn call registered handler
; pass in the vector number
                mov     rdx, rsp
                mov     rcx, [ExceptionNumber]
                sub     rsp, 40
                call    ASM_PFX(InterruptDistrubutionHub)
                add     rsp, 40

; restore context...
;; UINT64  ExceptionData;
                add     rsp, 8

;; FX_SAVE_STATE_X64 FxSaveState;
                mov     rsi, rsp
                fxrstor [rsi]
                add     rsp, 512

;; UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
                pop     rax
                mov     dr0, rax
                pop     rax
                mov     dr1, rax
                pop     rax
                mov     dr2, rax
                pop     rax
                mov     dr3, rax
;; skip restore of dr6.  We cleared dr6 during the context save.
                add     rsp, 8
                pop     rax
                mov     dr7, rax

;; UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
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

;; UINT64  RFlags;
                mov     rax, [AppRsp]
                pop     qword [rax + 16]

;; UINT64  Ldtr, Tr;
;; UINT64  Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
                add     rsp, 48

;; UINT64  Rip;
                pop     qword [rax]

;; UINT64  Gs, Fs, Es, Ds, Cs, Ss;
;; NOTE - modified segment registers could hang the debugger...  We
;;        could attempt to insulate ourselves against this possibility,
;;        but that poses risks as well.
;;

                pop     rax
                ; mov     gs, rax
                pop     rax
                ; mov     fs, rax
                pop     rax
                mov     es, rax
                pop     rax
                mov     ds, rax
                mov     rax, [AppRsp]
                pop     qword [rax + 8]
                pop     rax
                mov     ss, rax

;; The next stuff to restore is the general purpose registers that were pushed
;; using the "push" instruction.
;;
;; The value of RSP as stored in the context record is the application RSP
;; including the 5 entries on the application stack caused by the exception
;; itself. It may have been modified by the debug agent, so we need to
;; determine if we need to relocate the application stack.

                mov     rbx, [rsp + 24]  ; move the potentially modified AppRsp into rbx
                mov     rax, [AppRsp]
                mov     rax, QWORD [rax + 24]
                cmp     rbx, rax
                je      NoAppStackMove

                mov     rax, [AppRsp]
                mov     rcx, [rax]       ; RIP
                mov     [rbx], rcx

                mov     rcx, [rax + 8]   ; CS
                mov     [rbx + 8], rcx

                mov     rcx, [rax + 16]  ; RFLAGS
                mov     [rbx + 16], rcx

                mov     rcx, [rax + 24]  ; RSP
                mov     [rbx + 24], rcx

                mov     rcx, [rax + 32]  ; SS
                mov     [rbx + 32], rcx

                mov     rax, rbx         ; modify the saved AppRsp to the new AppRsp
                mov     [AppRsp], rax
NoAppStackMove:
                mov     rax, [DebugRsp]  ; restore the DebugRsp on the debug stack
                                         ; so our "pop" will not cause a stack switch
                mov     [rsp + 24], rax

                cmp     qword [ExceptionNumber], 0x68
                jne     NoChain

Chain:

;; Restore rflags so when we chain, the flags will be exactly as if we were never here.
;; We gin up the stack to do an iretq so we can get ALL the flags.
                mov     rax, [AppRsp]
                mov     rbx, [rax + 40]
                push    rbx
                mov     rax, ss
                push    rax
                mov     rax, rsp
                add     rax, 16
                push    rax
                mov     rax, [AppRsp]
                mov     rbx, [rax + 16]
                and     rbx, ~ 0x300 ; special handling for IF and TF
                push    rbx
                mov     rax, cs
                push    rax
                lea     rax, [PhonyIretq]
                push    rax
                iretq
PhonyIretq:

;; UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
;; UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
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

;; Switch back to application stack
                mov     rsp, [AppRsp]

;; Jump to original handler
                jmp     [ASM_PFX(OrigVector)]

NoChain:
;; UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
;; UINT64  R8, R9, R10, R11, R12, R13, R14, R15;
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

;; Switch back to application stack
                mov     rsp, [AppRsp]

;; We're outa here...
                iretq


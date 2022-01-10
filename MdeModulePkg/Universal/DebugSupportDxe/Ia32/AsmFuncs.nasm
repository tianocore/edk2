;/** @file
;  Low leve IA32 specific debug support functions.
;
;  Copyright (c) 2006 - 2022, Intel Corporation. All rights reserved.<BR>
;  SPDX-License-Identifier: BSD-2-Clause-Patent
;
;**/

%define EXCPT32_DIVIDE_ERROR 0
%define EXCPT32_DEBUG 1
%define EXCPT32_NMI 2
%define EXCPT32_BREAKPOINT 3
%define EXCPT32_OVERFLOW 4
%define EXCPT32_BOUND 5
%define EXCPT32_INVALID_OPCODE 6
%define EXCPT32_DOUBLE_FAULT 8
%define EXCPT32_INVALID_TSS 10
%define EXCPT32_SEG_NOT_PRESENT 11
%define EXCPT32_STACK_FAULT 12
%define EXCPT32_GP_FAULT 13
%define EXCPT32_PAGE_FAULT 14
%define EXCPT32_FP_ERROR 16
%define EXCPT32_ALIGNMENT_CHECK 17
%define EXCPT32_MACHINE_CHECK 18
%define EXCPT32_SIMD 19

%define FXSTOR_FLAG 0x1000000         ; bit cpuid 24 of feature flags

SECTION .data

global ASM_PFX(OrigVector)
global ASM_PFX(InterruptEntryStub)
global ASM_PFX(StubSize)
global ASM_PFX(CommonIdtEntry)
global ASM_PFX(FxStorSupport)
extern ASM_PFX(InterruptDistrubutionHub)

ASM_PFX(StubSize): dd InterruptEntryStubEnd - ASM_PFX(InterruptEntryStub)
AppEsp: dd 0x11111111 ; ?
DebugEsp: dd 0x22222222 ; ?
ExtraPush: dd 0x33333333 ; ?
ExceptData: dd 0x44444444 ; ?
Eflags: dd 0x55555555 ; ?
ASM_PFX(OrigVector): dd 0x66666666 ; ?

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
;; are 132 bytes of stack used prior allocating the 512 bytes of stack to be
;; used as the memory buffer for the fxstor instruction. Therefore address of
;; the buffer used for the FXSTOR instruction is &Eax - 132 - 512, which
;; must be 16 byte aligned.
;;
;; We carefully locate the stack to make this happen.
;;
;; For reference, the context structure looks like this:
;;      struct {
;;        UINT32             ExceptionData;
;;        FX_SAVE_STATE_IA32 FxSaveState;    // 512 bytes, must be 16 byte aligned
;;        UINT32             Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
;;        UINT32             Cr0, Cr1, Cr2, Cr3, Cr4;
;;        UINT32             EFlags;
;;        UINT32             Ldtr, Tr;
;;        UINT32             Gdtr[2], Idtr[2];
;;        UINT32             Eip;
;;        UINT32             Gs, Fs, Es, Ds, Cs, Ss;
;;        UINT32             Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
;;      } SYSTEM_CONTEXT_IA32;  // 32 bit system context record

align           16
DebugStackEnd: db "DbgStkEnd >>>>>>"    ;; 16 byte long string - must be 16 bytes to preserve alignment
                times 0x1ffc dd    0x0  ;; 32K should be enough stack
                                        ;;   This allocation is coocked to insure
                                        ;;   that the the buffer for the FXSTORE instruction
                                        ;;   will be 16 byte aligned also.
                                        ;;
ExceptionNumber: dd 0                   ;; first entry will be the vector number pushed by the stub

DebugStackBegin: db "<<<< DbgStkBegin"  ;; initial debug ESP == DebugStackBegin, set in stub

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
; cpuid corrupts ebx which must be preserved per the C calling convention
;
                push    ebx
                mov     eax, 1
                cpuid
                mov     eax, edx
                and     eax, FXSTOR_FLAG
                shr     eax, 24
                pop     ebx
                ret

;------------------------------------------------------------------------------
; void
; Vect2Desc (
;   DESCRIPTOR * DestDesc,
;   void (*Vector) (void)
;   )
;
; Abstract: Encodes an IDT descriptor with the given physical address
;
global ASM_PFX(Vect2Desc)
ASM_PFX(Vect2Desc):
                push    ebp
                mov     ebp, esp
                mov     eax, [ebp + 0xC]
                mov     ecx, [ebp + 0x8]
                mov     word [ecx], ax                  ; write bits 15..0 of offset
                mov     dx, cs
                mov     word [ecx+2], dx                ; SYS_CODE_SEL from GDT
                mov     word [ecx+4], 0xe00 | 0x8000    ; type = 386 interrupt gate, present
                shr     eax, 16
                mov     word [ecx+6], ax                ; write bits 31..16 of offset
                leave
                ret

;------------------------------------------------------------------------------
; InterruptEntryStub
;
; Abstract: This code is not a function, but is a small piece of code that is
;               copied and fixed up once for each IDT entry that is hooked.
;
ASM_PFX(InterruptEntryStub):
                mov     [AppEsp], esp                ; save stack top
                mov     esp, DebugStackBegin         ; switch to debugger stack
                push    0                            ; push vector number - will be modified before installed
                db      0xe9                         ; jump rel32
                dd      0                            ; fixed up to relative address of CommonIdtEntry
InterruptEntryStubEnd:

;------------------------------------------------------------------------------
; CommonIdtEntry
;
; Abstract: This code is not a function, but is the common part for all IDT
;               vectors.
;
ASM_PFX(CommonIdtEntry):
;;
;; At this point, the stub has saved the current application stack esp into AppEsp
;; and switched stacks to the debug stack, where it pushed the vector number
;;
;; The application stack looks like this:
;;
;;              ...
;;              (last application stack entry)
;;              eflags from interrupted task
;;              CS from interrupted task
;;              EIP from interrupted task
;;              Error code <-------------------- Only present for some exeption types
;;
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
;;   UINT32             ExceptionData;
;;   FX_SAVE_STATE_IA32 FxSaveState;
;;   UINT32             Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
;;   UINT32             Cr0, Cr2, Cr3, Cr4;
;;   UINT32             EFlags;
;;   UINT32             Ldtr, Tr;
;;   UINT32             Gdtr[2], Idtr[2];
;;   UINT32             Eip;
;;   UINT32             Gs, Fs, Es, Ds, Cs, Ss;
;;   UINT32             Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
;; } SYSTEM_CONTEXT_IA32;  // 32 bit system context record

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
                pushad

;; Save interrupt state eflags register...
                pushfd
                pop     eax
                mov     [Eflags], eax

;; We need to determine if any extra data was pushed by the exception, and if so, save it
;; To do this, we check the exception number pushed by the stub, and cache the
;; result in a variable since we'll need this again.
                cmp     dword [ExceptionNumber], EXCPT32_DOUBLE_FAULT
                jz      ExtraPushOne
                cmp     dword [ExceptionNumber], EXCPT32_INVALID_TSS
                jz      ExtraPushOne
                cmp     dword [ExceptionNumber], EXCPT32_SEG_NOT_PRESENT
                jz      ExtraPushOne
                cmp     dword [ExceptionNumber], EXCPT32_STACK_FAULT
                jz      ExtraPushOne
                cmp     dword [ExceptionNumber], EXCPT32_GP_FAULT
                jz      ExtraPushOne
                cmp     dword [ExceptionNumber], EXCPT32_PAGE_FAULT
                jz      ExtraPushOne
                cmp     dword [ExceptionNumber], EXCPT32_ALIGNMENT_CHECK
                jz      ExtraPushOne
                mov     dword [ExtraPush], 0
                mov     dword [ExceptData], 0
                jmp     ExtraPushDone

ExtraPushOne:
                mov     dword [ExtraPush], 1

;; If there's some extra data, save it also, and modify the saved AppEsp to effectively
;; pop this value off the application's stack.
                mov     eax, [AppEsp]
                mov     ebx, [eax]
                mov     [ExceptData], ebx
                add     eax, 4
                mov     [AppEsp], eax

ExtraPushDone:

;; The "pushad" above pushed the debug stack esp.  Since what we're actually doing
;; is building the context record on the debug stack, we need to save the pushed
;; debug ESP, and replace it with the application's last stack entry...
                mov     eax, [esp + 12]
                mov     [DebugEsp], eax
                mov     eax, [AppEsp]
                add     eax, 12
                ; application stack has eflags, cs, & eip, so
                ; last actual application stack entry is
                ; 12 bytes into the application stack.
                mov     [esp + 12], eax

;; continue building context record
;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;  insure high 16 bits of each is zero
                mov     eax, ss
                push    eax

                ; CS from application is one entry back in application stack
                mov     eax, [AppEsp]
                movzx   eax, word [eax + 4]
                push    eax

                mov     eax, ds
                push    eax
                mov     eax, es
                push    eax
                mov     eax, fs
                push    eax
                mov     eax, gs
                push    eax

;; UINT32  Eip;
                ; Eip from application is on top of application stack
                mov     eax, [AppEsp]
                push    dword [eax]

;; UINT32  Gdtr[2], Idtr[2];
                push    0
                push    0
                sidt    [esp]
                push    0
                push    0
                sgdt    [esp]

;; UINT32  Ldtr, Tr;
                xor     eax, eax
                str     ax
                push    eax
                sldt    ax
                push    eax

;; UINT32  EFlags;
;; Eflags from application is two entries back in application stack
                mov     eax, [AppEsp]
                push    dword [eax + 8]

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
;; insure FXSAVE/FXRSTOR is enabled in CR4...
;; ... while we're at it, make sure DE is also enabled...
                mov     eax, cr4
                or      eax, 0x208
                mov     cr4, eax
                push    eax
                mov     eax, cr3
                push    eax
                mov     eax, cr2
                push    eax
                push    0
                mov     eax, cr0
                push    eax

;; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
                mov     eax, dr7
                push    eax
;; clear Dr7 while executing debugger itself
                xor     eax, eax
                mov     dr7, eax

                mov     eax, dr6
                push    eax
;; insure all status bits in dr6 are clear...
                xor     eax, eax
                mov     dr6, eax

                mov     eax, dr3
                push    eax
                mov     eax, dr2
                push    eax
                mov     eax, dr1
                push    eax
                mov     eax, dr0
                push    eax

;; FX_SAVE_STATE_IA32 FxSaveState;
                sub     esp, 512
                mov     edi, esp
                ; IMPORTANT!! The debug stack has been carefully constructed to
                ; insure that esp and edi are 16 byte aligned when we get here.
                ; They MUST be.  If they are not, a GP fault will occur.
                fxsave  [edi]

;; UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
                cld

;; UINT32  ExceptionData;
                mov     eax, [ExceptData]
                push    eax

; call to C code which will in turn call registered handler
; pass in the vector number
                mov     eax, esp
                push    eax
                mov     eax, [ExceptionNumber]
                push    eax
                call    ASM_PFX(InterruptDistrubutionHub)
                add     esp, 8

; restore context...
;; UINT32  ExceptionData;
                add     esp, 4

;; FX_SAVE_STATE_IA32 FxSaveState;
                mov     esi, esp
                fxrstor [esi]
                add     esp, 512

;; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
                pop     eax
                mov     dr0, eax
                pop     eax
                mov     dr1, eax
                pop     eax
                mov     dr2, eax
                pop     eax
                mov     dr3, eax
;; skip restore of dr6.  We cleared dr6 during the context save.
                add     esp, 4
                pop     eax
                mov     dr7, eax

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
                pop     eax
                mov     cr0, eax
                add     esp, 4
                pop     eax
                mov     cr2, eax
                pop     eax
                mov     cr3, eax
                pop     eax
                mov     cr4, eax

;; UINT32  EFlags;
                mov     eax, [AppEsp]
                pop     dword [eax + 8]

;; UINT32  Ldtr, Tr;
;; UINT32  Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
                add     esp, 24

;; UINT32  Eip;
                pop     dword [eax]

;; UINT32  SegGs, SegFs, SegEs, SegDs, SegCs, SegSs;
;; NOTE - modified segment registers could hang the debugger...  We
;;        could attempt to insulate ourselves against this possibility,
;;        but that poses risks as well.
;;

                pop     gs
                pop     fs
                pop     es
                pop     ds
                pop     dword [eax + 4]
                pop     ss

;; The next stuff to restore is the general purpose registers that were pushed
;; using the "pushad" instruction.
;;
;; The value of ESP as stored in the context record is the application ESP
;; including the 3 entries on the application stack caused by the exception
;; itself. It may have been modified by the debug agent, so we need to
;; determine if we need to relocate the application stack.

                mov     ebx, [esp + 12]  ; move the potentially modified AppEsp into ebx
                mov     eax, [AppEsp]
                add     eax, 12
                cmp     ebx, eax
                je      NoAppStackMove

                mov     eax, [AppEsp]
                mov     ecx, [eax]       ; EIP
                mov     [ebx], ecx

                mov     ecx, [eax + 4]   ; CS
                mov     [ebx + 4], ecx

                mov     ecx, [eax + 8]   ; EFLAGS
                mov     [ebx + 8], ecx

                mov     eax, ebx         ; modify the saved AppEsp to the new AppEsp
                mov     [AppEsp], eax
NoAppStackMove:
                mov     eax, [DebugEsp]  ; restore the DebugEsp on the debug stack
                                         ; so our "popad" will not cause a stack switch
                mov     [esp + 12], eax

                cmp     dword [ExceptionNumber], 0x68
                jne     NoChain

Chain:

;; Restore eflags so when we chain, the flags will be exactly as if we were never here.
;; We gin up the stack to do an iretd so we can get ALL the flags.
                mov     eax, [AppEsp]
                mov     ebx, [eax + 8]
                and     ebx, ~ 0x300 ; special handling for IF and TF
                push    ebx
                push    cs
                push    PhonyIretd
                iretd
PhonyIretd:

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
                popad

;; Switch back to application stack
                mov     esp, [AppEsp]

;; Jump to original handler
                jmp     [ASM_PFX(OrigVector)]

NoChain:
;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
                popad

;; Switch back to application stack
                mov     esp, [AppEsp]

;; We're outa here...
                iretd


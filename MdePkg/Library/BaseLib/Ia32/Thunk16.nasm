
#include "BaseLibInternals.h"

;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   Thunk.asm
;
; Abstract:
;
;   Real mode thunk
;
;------------------------------------------------------------------------------

global ASM_PFX(m16Size)
global ASM_PFX(mThunk16Attr)
global ASM_PFX(m16Gdt)
global ASM_PFX(m16GdtrBase)
global ASM_PFX(mTransition)
global ASM_PFX(m16Start)

struc IA32_REGS

  ._EDI:       resd      1
  ._ESI:       resd      1
  ._EBP:       resd      1
  ._ESP:       resd      1
  ._EBX:       resd      1
  ._EDX:       resd      1
  ._ECX:       resd      1
  ._EAX:       resd      1
  ._DS:        resw      1
  ._ES:        resw      1
  ._FS:        resw      1
  ._GS:        resw      1
  ._EFLAGS:    resd      1
  ._EIP:       resd      1
  ._CS:        resw      1
  ._SS:        resw      1
  .size:

endstruc

;; .const

SECTION .data

;
; These are global constant to convey information to C code.
;
ASM_PFX(m16Size)         DW      InternalAsmThunk16 - ASM_PFX(m16Start)
ASM_PFX(mThunk16Attr)    DW      _ThunkAttr - ASM_PFX(m16Start)
ASM_PFX(m16Gdt)          DW      _NullSegDesc - ASM_PFX(m16Start)
ASM_PFX(m16GdtrBase)     DW      _16GdtrBase - ASM_PFX(m16Start)
ASM_PFX(mTransition)     DW      _EntryPoint - ASM_PFX(m16Start)

SECTION .text

ASM_PFX(m16Start):

SavedGdt:
            dw  0
            dd  0

;------------------------------------------------------------------------------
; _BackFromUserCode() takes control in real mode after 'retf' has been executed
; by user code. It will be shadowed to somewhere in memory below 1MB.
;------------------------------------------------------------------------------
_BackFromUserCode:
    ;
    ; The order of saved registers on the stack matches the order they appears
    ; in IA32_REGS structure. This facilitates wrapper function to extract them
    ; into that structure.
    ;
    push    ss
    push    cs
    DB      66h
    call    @Base                       ; push eip
@Base:
    pushfw                              ; pushfd actually
    cli                                 ; disable interrupts
    push    gs
    push    fs
    push    es
    push    ds
    pushaw                              ; pushad actually
    DB      66h, 0bah                   ; mov edx, imm32
_ThunkAttr:     dd   0
    test    dl, THUNK_ATTRIBUTE_DISABLE_A20_MASK_INT_15
    jz      @1
    mov     eax, 15cd2401h              ; mov ax, 2401h & int 15h
    cli                                 ; disable interrupts
    jnc     @2
@1:
    test    dl, THUNK_ATTRIBUTE_DISABLE_A20_MASK_KBD_CTRL
    jz      @2
    in      al, 92h
    or      al, 2
    out     92h, al                     ; deactivate A20M#
@2:
    xor     ax, ax                      ; xor eax, eax
    mov     eax, ss                     ; mov ax, ss
    DB      67h
    lea     bp, [esp + IA32_REGS.size]
    ;
    ; esi's in the following 2 instructions are indeed bp in 16-bit code. Fact
    ; is "esi" in 32-bit addressing mode has the same encoding of "bp" in 16-
    ; bit addressing mode.
    ;
    mov     [esi - IA32_REGS.size + IA32_REGS._ESP], bp
    mov     ebx, [esi - IA32_REGS.size + IA32_REGS._EIP]
    shl     ax, 4                       ; shl eax, 4
    add     bp, ax                      ; add ebp, eax
    DB      66h, 0b8h                   ; mov eax, imm32
SavedCr4:   DD      0
    mov     cr4, eax
    DB      66h
    lgdt    [cs:edi + (SavedGdt - @Base)]
    DB      66h, 0b8h                   ; mov eax, imm32
SavedCr0:   DD      0
    mov     cr0, eax
    DB      0b8h                        ; mov ax, imm16
SavedSs     DW      0
    mov     ss, eax
    DB      66h, 0bch                   ; mov esp, imm32
SavedEsp    DD      0
    DB      66h
    retf                                ; return to protected mode

_EntryPoint:
        DD      _ToUserCode - ASM_PFX(m16Start)
        DW      8h
_16Idtr:
        DW      (1 << 10) - 1
        DD      0
_16Gdtr:
        DW      GdtEnd - _NullSegDesc - 1
_16GdtrBase:
        DD      _NullSegDesc

;------------------------------------------------------------------------------
; _ToUserCode() takes control in real mode before passing control to user code.
; It will be shadowed to somewhere in memory below 1MB.
;------------------------------------------------------------------------------
_ToUserCode:
    mov     edx, ss
    mov     ss, ecx                     ; set new segment selectors
    mov     ds, ecx
    mov     es, ecx
    mov     fs, ecx
    mov     gs, ecx
    mov     cr0, eax                    ; real mode starts at next instruction
                                        ;  which (per SDM) *must* be a far JMP.
    DB      0eah
_RealAddr: DW 0, 0

    mov     cr4, ebp
    mov     ss, esi                     ; set up 16-bit stack segment
    xchg    sp, bx                      ; set up 16-bit stack pointer

;   mov     bp, [esp + sizeof(IA32_REGS)
    DB      67h
    mov     ebp, [esp + IA32_REGS.size] ; BackFromUserCode address from stack

;   mov     cs:[bp + (SavedSs - _BackFromUserCode)], dx
    mov     [cs:esi + (SavedSs - _BackFromUserCode)], edx

;   mov     cs:[bp + (SavedEsp - _BackFromUserCode)], ebx
    DB      2eh, 66h, 89h, 9eh
    DW      SavedEsp - _BackFromUserCode

;   lidt    cs:[bp + (_16Idtr - _BackFromUserCode)]
    DB      2eh, 66h, 0fh, 01h, 9eh
    DW      _16Idtr - _BackFromUserCode

    popaw                               ; popad actually
    pop     ds
    pop     es
    pop     fs
    pop     gs
    popfw                                ; popfd

    DB      66h                         ; Use 32-bit addressing for "retf" below
    retf                                ; transfer control to user code

ALIGN   16
_NullSegDesc    DQ      0
_16CsDesc:
                DW      -1
                DW      0
                DB      0
                DB      9bh
                DB      8fh             ; 16-bit segment, 4GB limit
                DB      0
_16DsDesc:
                DW      -1
                DW      0
                DB      0
                DB      93h
                DB      8fh             ; 16-bit segment, 4GB limit
                DB      0
GdtEnd:

;------------------------------------------------------------------------------
; IA32_REGISTER_SET *
; EFIAPI
; InternalAsmThunk16 (
;   IN      IA32_REGISTER_SET         *RegisterSet,
;   IN OUT  VOID                      *Transition
;   );
;------------------------------------------------------------------------------
global ASM_PFX(InternalAsmThunk16)
ASM_PFX(InternalAsmThunk16):
    push    ebp
    push    ebx
    push    esi
    push    edi
    push    ds
    push    es
    push    fs
    push    gs
    mov     esi, [esp + 36]             ; esi <- RegSet, the 1st parameter
    movzx   edx, word [esi + IA32_REGS._SS]
    mov     edi, [esi + IA32_REGS._ESP]
    add     edi, - (IA32_REGS.size + 4) ; reserve stack space
    mov     ebx, edi                    ; ebx <- stack offset
    imul    eax, edx, 16                ; eax <- edx * 16
    push    IA32_REGS.size / 4
    add     edi, eax                    ; edi <- linear address of 16-bit stack
    pop     ecx
    rep     movsd                       ; copy RegSet
    mov     eax, [esp + 40]             ; eax <- address of transition code
    mov     esi, edx                    ; esi <- 16-bit stack segment
    lea     edx, [eax + (SavedCr0 - ASM_PFX(m16Start))]
    mov     ecx, eax
    and     ecx, 0fh
    shl     eax, 12
    lea     ecx, [ecx + (_BackFromUserCode - ASM_PFX(m16Start))]
    mov     ax, cx
    stosd                               ; [edi] <- return address of user code
    add     eax, _RealAddr + 4 - _BackFromUserCode
    mov     [edx + (_RealAddr - SavedCr0)], eax
    sgdt    [edx + (SavedGdt - SavedCr0)]
    sidt    [esp + 36]        ; save IDT stack in argument space
    mov     eax, cr0
    mov     [edx], eax                  ; save CR0 in SavedCr0
    and     eax, 7ffffffeh              ; clear PE, PG bits
    mov     ebp, cr4
    mov     [edx + (SavedCr4 - SavedCr0)], ebp
    and     ebp, ~30h                ; clear PAE, PSE bits
    push    10h
    pop     ecx                         ; ecx <- selector for data segments
    lgdt    [edx + (_16Gdtr - SavedCr0)]
    pushfd                              ; Save df/if indeed
    call    dword far [edx + (_EntryPoint - SavedCr0)]
    popfd
    lidt    [esp + 36]        ; restore protected mode IDTR
    lea     eax, [ebp - IA32_REGS.size] ; eax <- the address of IA32_REGS
    pop     gs
    pop     fs
    pop     es
    pop     ds
    pop     edi
    pop     esi
    pop     ebx
    pop     ebp
    ret

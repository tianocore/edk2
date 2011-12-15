;*****************************************************************************
;*
;*   Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
;*   This program and the accompanying materials                          
;*   are licensed and made available under the terms and conditions of the BSD License         
;*   which accompanies this distribution.  The full text of the license may be found at        
;*   http://opensource.org/licenses/bsd-license.php                                            
;*                                                                                             
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;*   
;*   Module Name:
;*
;*    Thunk.asm
;*  
;*   Abstract:
;*  
;*    Real mode thunk
;*  
;*****************************************************************************

    .686p

EXTERNDEF   C   mCode16Size:DWORD

CONST   SEGMENT FLAT    "DATA"  READONLY

mCode16Size     DD      _TEXT16SIZE

CONST   ENDS

_DATA   SEGMENT FLAT    "DATA"

NullSegSel      DQ      0
_16BitCsSel     LABEL   QWORD
                DW      -1
                DW      0
                DB      0
                DB      9bh
                DB      8fh             ; 16-bit segment
                DB      0
_16BitSsSel     LABEL   QWORD
                DW      -1
                DW      0
                DB      0
                DB      93h
                DB      8fh            ; 16-bit segment
                DB      0

_16Gdtr         LABEL   FWORD
                DW      $ - offset NullSegSel - 1
                DD      offset NullSegSel

_DATA   ENDS

_TEXT   SEGMENT FLAT    "CODE"  PARA

STACK_PARAM_SIZE  EQU  16

IA32_REGS   STRUC   4t
_EDI        DD      ?
_ESI        DD      ?
_EBP        DD      ?
_ESP        DD      ?
_EBX        DD      ?
_EDX        DD      ?
_ECX        DD      ?
_EAX        DD      ?
_DS         DW      ?
_ES         DW      ?
_FS         DW      ?
_GS         DW      ?
_EFLAGS     DD      ?
_EIP        DD      ?
_CS         DW      ?
_SS         DW      ?
IA32_REGS   ENDS

_STK16      STRUC   1t
RetEip      DD      ?
RetCs       DW      ?
ThunkFlags  DW      ?
SavedEsp    DD      ?
SavedSs     DW      ?
SavedGdtr   FWORD   ?
SavedCr0    DD      ?
SavedCr4    DD      ?
_STK16      ENDS

    ASSUME  ds:_DATA

__Thunk16   PROC    USES    ebp ebx esi edi ds  es  fs  gs
    ASSUME  esi:PTR IA32_REGS
    mov     esi, [esp + 36]
    movzx   edx, [esi]._SS
    mov     edi, [esi]._ESP
    add     edi, - sizeof (_STK16) - sizeof (IA32_REGS)
    push    edi                         ; save stack offset
    imul    eax, edx, 16                ; eax <- edx*16
    add     edi, eax                    ; edi <- linear address of 16-bit stack
    push    sizeof (IA32_REGS) / 4
    pop     ecx
    rep     movsd                       ; copy context to 16-bit stack

    ; copy eflags to stack frame
    mov     eax, [esi - sizeof(IA32_REGS)]._EFLAGS
    mov     [edi - sizeof(IA32_REGS) - STACK_PARAM_SIZE - 4], eax

    pop     ebx                         ; ebx <- 16-bit stack offset
    mov     eax, offset @F              ; return offset
    stosd
    mov     eax, cs                     ; return segment
    stosw
    mov     eax, [esp + 40]             ; THUNK flags
    stosw
    mov     eax, esp
    stosd                               ; save esp
    mov     eax, ss                     ; save ss
    stosw
    sgdt    fword ptr [edi]             ; save GDTR
    sidt    fword ptr [esp + 36]        ; save IDTR
    mov     esi, cr0
    mov     [edi + 6], esi              ; save CR0
    and     esi, NOT 80000001h          ; esi <- CR0 to set
    mov     eax, cr4
    mov     [edi + 10], eax             ; save CR4
    and     al, NOT 30h                 ; clear PAE & PSE
    mov     edi, edx                    ; edi <- 16-bit stack segment
    mov     edx, [esp + 44]
    shl     edx, 16
    push    edx
    pop     edx
    mov     dx, _16BitSsSel - NullSegSel
    lgdt    _16Gdtr                     ; load 16-bit GDTR
    DB      0eah
    DD      offset @16Bit
    DW      _16BitCsSel - NullSegSel    ; jmp far 8:@16Bit
@16Bit:
    mov     ss, dx
    mov     cr0, esi                    ; disable protected mode
    mov     cr4, eax                    ; disable PAE & PSE
    db      67h, 0FFh, 06Ch, 024h, 0FCh ; jmp     dword ptr [esp-4]
@@:
    xor     eax, eax
    mov     ax, ss
    shl     eax, 4
    add     eax, esp                    ; eax <- address of 16-bit stack
    lss     esp, fword ptr (_STK16 ptr [esp + sizeof (IA32_REGS)]).SavedEsp
    lidt    fword ptr [esp + 36]        ; restore IDTR
    ret
__Thunk16   ENDP

_TEXT   ENDS

_TEXT16 SEGMENT USE16   "CODE"  PARA

_Code16Addr PROC    C
_Code16Addr ENDP

RealMode    PROC
    mov     ss, di                      ; set up stack
    mov     esp, ebx
    lidt    fword ptr cs:[_16Idtr - _Code16Addr]
    popad
    pop     ds
    pop     es
    pop     fs
    pop     gs
    sub     esp, (sizeof(IA32_REGS) - 12) + STACK_PARAM_SIZE + 4
    popfd
    test    (_STK16 ptr [esp + STACK_PARAM_SIZE + sizeof(IA32_REGS)]).ThunkFlags, 1
    jz      @F
    pushf                               ; push Flags when it's INT#
@@:
    push    cs
;    push    @FarCallRet - _Code16Addr
    DB      68h                         ; push /iw
    DW      @FarCallRet - _Code16Addr
    jz      @F
    jmp     fword ptr [esp + 6 + STACK_PARAM_SIZE + sizeof(IA32_REGS) - 8]
@@:
    jmp     fword ptr [esp + 4 + STACK_PARAM_SIZE + sizeof(IA32_REGS) - 8]
@FarCallRet:
    add     esp, (sizeof(IA32_REGS) - 12) + STACK_PARAM_SIZE + 4
    pushfd
    push    gs
    push    fs
    push    es
    push    ds
    pushad
    cli
    DB      66h
    lgdt    (_STK16 ptr [esp + sizeof (IA32_REGS)]).SavedGdtr
    mov     eax, (_STK16 ptr [esp + sizeof (IA32_REGS)]).SavedCr4
    mov     cr4, eax
    mov     eax, (_STK16 ptr [esp + sizeof (IA32_REGS)]).SavedCr0
    mov     cr0, eax                    ; restore CR0
    jmp     fword ptr (_STK16 ptr [esp + sizeof (IA32_REGS)]).RetEip
RealMode    ENDP

_16Idtr     FWORD   (1 SHL 10) - 1

_TEXT16END:

_TEXT16SIZE = _TEXT16END - _Code16Addr

_TEXT16 ENDS

    END

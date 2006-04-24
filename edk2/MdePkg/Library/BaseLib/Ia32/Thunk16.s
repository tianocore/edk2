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
#   Thunk.asm
#
# Abstract:
#
#   Real mode thunk
#
#------------------------------------------------------------------------------



MISMATCH: "EXTERNDEF   C   mCode16Size:DWORD"

MISMATCH: "CONST   SEGMENT FLAT    "DATA"  READONLY"

MISMATCH: "mCode16Size     DD      _TEXT16SIZE"

MISMATCH: "CONSTS"

MISMATCH: "_DATA   SEGMENT FLAT    "DATA""

MISMATCH: "NullSegSel      DQ      0"
MISMATCH: "_16BitCsSel     LABEL   QWORD"
                .word -1
                .word 0
                .byte 0
                .byte 0x9b
                .byte 0x8f              # 16-bit segment
                .byte 0

MISMATCH: "_16Gdtr         LABEL   FWORD"
MISMATCH: "                DW      $ - offset NullSegSel - 1"
MISMATCH: "                DD      offset NullSegSel"

MISMATCH: "_DATAS"

MISMATCH: "_TEXT   SEGMENT FLAT    "CODE"  PARA"

MISMATCH: "IA32_REGS   STRUC   4t"
MISMATCH: "_EDI        DD      ?"
MISMATCH: "_ESI        DD      ?"
MISMATCH: "_EBP        DD      ?"
MISMATCH: "_ESP        DD      ?"
MISMATCH: "_EBX        DD      ?"
MISMATCH: "_EDX        DD      ?"
MISMATCH: "_ECX        DD      ?"
MISMATCH: "_EAX        DD      ?"
MISMATCH: "_DS         DW      ?"
MISMATCH: "_ES         DW      ?"
MISMATCH: "_FS         DW      ?"
MISMATCH: "_GS         DW      ?"
MISMATCH: "_EFLAGS     DD      ?"
MISMATCH: "_EIP        DD      ?"
MISMATCH: "_CS         DW      ?"
MISMATCH: "_SS         DW      ?"
MISMATCH: "IA32_REGSS"

MISMATCH: "_STK16      STRUC   1t"
MISMATCH: "RetEip      DD      ?"
MISMATCH: "RetCs       DW      ?"
MISMATCH: "ThunkFlags  DW      ?"
MISMATCH: "SavedEsp    DD      ?"
MISMATCH: "SavedSs     DW      ?"
MISMATCH: "SavedGdtr   FWORD   ?"
MISMATCH: "SavedCr0    DD      ?"
MISMATCH: "SavedCr4    DD      ?"
MISMATCH: "_STK16S"

.global _InternalAsmThunk16
MISMATCH: "_InternalAsmThunk16:    USES    ebp ebx esi edi ds  es  fs  gs"
MISMATCH: "    ASSUME  esi:PTR IA32_REGS"
    movl    36(%esp),%esi
MISMATCH: "    movzx   edx, [esi]._SS"
    movl    $[esi]._ESP, %edi
MISMATCH: "    add     edi, - sizeof (_STK16) - sizeof (IA32_REGS)"
    pushl   %edi                        # save stack offset
    imull   $16,%edx,%eax               # eax <- edx*16
    addl    %eax,%edi                   # edi <- linear address of 16-bit stack
MISMATCH: "    push    sizeof (IA32_REGS) / 4"
    popl    %ecx
    rep
    movsl                               # copy context to 16-bit stack
    popl    %ebx                        # ebx <- 16-bit stack offset
MISMATCH: "    mov     eax, offset @F              "
    stosl
    movl    %cs,%eax                    # return segment
    stosw
    movl    40(%esp),%eax               # THUNK flags
    stosw
    movl    %esp,%eax
    stosl                               # save esp
    movl    %ss,%eax                    # save ss
    stosw
MISMATCH: "    sgdt    fword ptr [edi]             "
MISMATCH: "    sidt    fword ptr [esp + 36]        "
    movl    %cr0, %esi
    movl    %esi,6(%edi)                # save CR0
MISMATCH: "    and     esi, NOT 80000001h          "
    movl    %cr4, %eax
    movl    %eax,10(%edi)               # save CR4
MISMATCH: "    and     al, NOT 30h                 "
    movl    %edx,%edi                   # edi <- 16-bit stack segment
    movl    44(%esp),%edx
    shll    $16,%edx
    pushl   %edx
MISMATCH: "    lgdt    _16Gdtr                     "
    .byte 0xea
MISMATCH: "    DD      offset @16Bit"
    .word 8                             # jmp far 8:@16Bit
@16Bit: 
    movl    %esi, %cr0                  # disable protected mode
    movl    %eax, %cr4                  # disable PAE & PSE
    lret
@@: 
    movl    %ss,%eax
    shll    $4,%eax
    addl    %esp,%eax                   # eax <- address of 16-bit stack
MISMATCH: "    lss     esp, fword ptr (_STK16 ptr [esp + sizeof (IA32_REGS)]).SavedEsp"
MISMATCH: "    lidt    fword ptr [esp + 36]        "
    ret


MISMATCH: "_TEXTS"

MISMATCH: "_TEXT16 SEGMENT USE16   "CODE"  PARA"

.global _Code16Addr
MISMATCH: "_Code16Addr:    C"


.global RealMode
RealMode: 
MISMATCH: "    ASSUME  bp:PTR _STK16"
    movw    %di,%ss                     # set up stack
    movl    %ebx,%esp
MISMATCH: "    lidt    fword ptr cs:[_16Idtr - _Code16Addr]"
    popal
    popl    %ds
    popl    %es
    popl    %fs
    popl    %gs
    addw    $4,%sp                      # skip EFlags
MISMATCH: "    test    (_STK16 ptr [esp + 8]).ThunkFlags, 1"
    jz      @F
    pushf
@@: 
    pushw   %cs
MISMATCH: "    push    @FarCallRet - _Code16Addr"
    jz      @F
MISMATCH: "    jmp     fword ptr [esp + 6]"
@@: 
MISMATCH: "    jmp     fword ptr [esp + 4]"
@FarCallRet: 
    pushfl
    pushw   %gs
    pushw   %fs
    pushw   %es
    pushw   %ds
    pushal
    cli
MISMATCH: "    lea     bp, [esp + sizeof (IA32_REGS)]"
    .byte 0x66
MISMATCH: "    lgdt    [bp].SavedGdtr"
    movl    $[bp].SavedCr4, %eax
    movl    %eax, %cr4
    movl    $[bp].SavedCr0, %eax
    movl    %eax, %cr0                  # restore CR0
MISMATCH: "    jmp     fword ptr [bp].RetEip"


MISMATCH: "_16Idtr     FWORD   (1 SHL 10) - 1"

_TEXT16: 

MISMATCH: "_TEXT16SIZE = _TEXT16- _Code16Addr"

MISMATCH: "_TEXT16S"



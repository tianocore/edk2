#/** @file
#  
#    IDT vector entry.
#  
#  Copyright (c) 2007 - 2009, Intel Corporation. All rights reserved.<BR>
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#  
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#  
#**/

    .text
    .code32


    .p2align 3
    ASM_GLOBAL  ASM_PFX(AsmGetVectorTemplatInfo)
    ASM_GLOBAL 	ASM_PFX(AsmVectorFixup)
/*
;
;-----------------------------------------------------------------------
;  Template of IDT Vector Handlers. 
;
;-----------------------------------------------------------------------
*/
VectorTemplateBase:
        pushl %eax
        .byte 0x6a    #  push #VectorNum
VectorNum:
        .byte 0
        movl  CommonInterruptEntry, %eax
        jmp   *%eax
VectorTemplateEnd:


ASM_PFX(AsmGetVectorTemplatInfo):
        movl  4(%esp), %ecx
        movl  $VectorTemplateBase, (%ecx)
        movl  $(VectorTemplateEnd - VectorTemplateBase), %eax
        ret

ASM_PFX(AsmVectorFixup):
        movl  8(%esp), %eax
        movl  4(%esp), %ecx
        movb  %al, (VectorNum - VectorTemplateBase)(%ecx)
        ret

/*
; The follow algorithm is used for the common interrupt routine.

;
; +---------------------+ <-- 16-byte aligned ensured by processor
; +    Old SS           +
; +---------------------+
; +    Old RSP          +
; +---------------------+
; +    RFlags           +
; +---------------------+
; +    CS               +
; +---------------------+
; +    RIP              +
; +---------------------+
; +    Error Code       +
; +---------------------+
; +    Vector Number    +
; +---------------------+
*/

CommonInterruptEntry: 
        cli
1:
        jmp   1b





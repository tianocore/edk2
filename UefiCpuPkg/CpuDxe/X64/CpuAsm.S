#      TITLE   CpuAsm.S: 

#------------------------------------------------------------------------------
#*
#*   Copyright (c) 2008 - 2013, Intel Corporation. All rights reserved.<BR>
#*   This program and the accompanying materials
#*   are licensed and made available under the terms and conditions of the BSD License
#*   which accompanies this distribution.  The full text of the license may be found at
#*   http://opensource.org/licenses/bsd-license.php
#*
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#*
#*    CpuAsm.S
#*
#*   Abstract:
#*
#------------------------------------------------------------------------------


#text  SEGMENT


#------------------------------------------------------------------------------
# VOID
# SetCodeSelector (
#   UINT16 Selector
#   );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(SetCodeSelector)
ASM_PFX(SetCodeSelector):
    subq    $0x10, %rsp 
    leaq    L_setCodeSelectorLongJump(%rip), %rax 
    movq    %rax, (%rsp) 
    movw    %cx, 4(%rsp)
    .byte   0xFF, 0x2C, 0x24     # jmp (%rsp) note:fword jmp
L_setCodeSelectorLongJump:
    addq    $0x10, %rsp
    ret

#------------------------------------------------------------------------------
# VOID
# SetDataSelectors (
#   UINT16 Selector
#   );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(SetDataSelectors)
ASM_PFX(SetDataSelectors):
    movw    %cx, %ss
    movw    %cx, %ds
    movw    %cx, %es
    movw    %cx, %fs
    movw    %cx, %gs
    ret

#text  ENDS

#END



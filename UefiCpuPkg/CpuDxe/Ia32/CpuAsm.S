#------------------------------------------------------------------------------
#*
#*   Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
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


#.MMX
#.XMM

#------------------------------------------------------------------------------
# VOID
# SetCodeSelector (
#   UINT16 Selector
#   );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(SetCodeSelector)
ASM_PFX(SetCodeSelector):
    movl    4(%esp), %ecx
    subl    $0x10, %esp 
    leal    setCodeSelectorLongJump, %eax 
    movl    %eax, (%esp)
    movw    %cx, 4(%esp)
    .byte   0xFF, 0x2C, 0x24   # jmp *(%esp)  note:(FWORD jmp) 
setCodeSelectorLongJump:
    addl    $0x10, %esp 
    ret

#------------------------------------------------------------------------------
# VOID
# SetDataSelectors (
#   UINT16 Selector
#   );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(SetDataSelectors)
ASM_PFX(SetDataSelectors):
    movl    4(%esp), %ecx
    movw    %cx, %ss
    movw    %cx, %ds
    movw    %cx, %es
    movw    %cx, %fs
    movw    %cx, %gs
    ret

#END


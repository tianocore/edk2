#------------------------------------------------------------------------------
#*
#*   Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
#*   This program and the accompanying materials
#*   are licensed and made available under the terms and conditions of the BSD License
#*   which accompanies this distribution.  The full text of the license may be found at
#*   http://opensource.org/licenses/bsd-license.php
#*
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#*
#*    AsmFuncs.S
#*
#*   Abstract:
#*
#*     Assembly function to set segment selectors.
#
#------------------------------------------------------------------------------

.text

#------------------------------------------------------------------------------
# 
# VOID
# EFIAPI
# AsmSetDataSelectors (
#   IN UINT16   SelectorValue
#   );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(AsmSetDataSelectors)
ASM_PFX(AsmSetDataSelectors):
    movl    4(%esp),  %eax
    movw    %ax, %ss
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs 
    ret

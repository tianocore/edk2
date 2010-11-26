#*****************************************************************************
#*
#*   Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
#*   This program and the accompanying materials                          
#*   are licensed and made available under the terms and conditions of the BSD License         
#*   which accompanies this distribution.  The full text of the license may be found at        
#*   http://opensource.org/licenses/bsd-license.php                                            
#*                                                                                             
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#*   
#*   Module Name:
#*
#*    Fx.asm
#*  
#*   Abstract:
#*  
#*    AsmFxRestore and AsmFxSave function
#*  
#include <EfiBind.h>
#------------------------------------------------------------------------------

    .586P: 
    #.MODEL flat,C
    .xmm: 
    .code: 
#------------------------------------------------------------------------------
.globl ASM_PFX(AsmFxSave)
.globl ASM_PFX(AsmFxRestore)
#------------------------------------------------------------------------------
# VOID
# AsmFxSave (
#   OUT IA32_FX_BUFFER *Buffer
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmFxSave):
    movl    4(%esp), %eax
    fxsave  (%eax)
    ret
#AsmFxSave ENDP

#------------------------------------------------------------------------------
# VOID
# AsmFxRestore (
#   IN CONST IA32_FX_BUFFER *Buffer
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmFxRestore):
    movl    4(%esp), %eax
    fxrstor (%eax)
    ret
#AsmFxRestore ENDP

#------------------------------------------------------------------------------
# UINTN
# AsmGetEflags (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmGetEflags):
    pushfl
    pop   %eax
    ret
#AsmGetEflags ENDP

#------------------------------------------------------------------------------
# VOID
# AsmSetEflags (
#   IN UINTN   Eflags
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmSetEflags):
    push  4(%esp)
    popfl
    ret
#AsmSetEflags ENDP

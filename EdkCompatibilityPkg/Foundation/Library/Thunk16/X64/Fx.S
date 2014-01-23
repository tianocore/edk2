#*****************************************************************************
#*
#*   Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
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
#*    Fx.S
#*  
#*   Abstract:
#*  
#*    AsmFxRestore and AsmFxSave function
#*  
#*****************************************************************************
#include <EfiBind.h>

.globl ASM_PFX(AsmFxSave)
.globl ASM_PFX(AsmFxRestore)

    .text

#------------------------------------------------------------------------------
# VOID
# AsmFxSave (
#   OUT IA32_FX_BUFFER *Buffer
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmFxSave):
    fxsave (%rcx)
    retq
    
#------------------------------------------------------------------------------
# VOID
# AsmFxRestore (
#   IN CONST IA32_FX_BUFFER *Buffer
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmFxRestore):
    fxrstor (%rcx)
    retq

#------------------------------------------------------------------------------
# UINTN
# AsmGetEflags (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmGetEflags):
    pushfq
    popq  %rax
    ret
#AsmGetEflags ENDP

#------------------------------------------------------------------------------
# VOID
# AsmSetEflags (
#   IN UINTN   Eflags
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmSetEflags):
    pushq  %rcx
    popfq
    ret
#AsmSetEflags ENDP

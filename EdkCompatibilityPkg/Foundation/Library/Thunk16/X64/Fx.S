#*****************************************************************************
#*
#*   Copyright (c) 2008, Intel Corporation                                                         
#*   All rights reserved. This program and the accompanying materials                          
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

.global ASM_PFX(AsmFxSave)
.global ASM_PFX(AsmFxRestore)

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
    
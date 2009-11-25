#------------------------------------------------------------------------------
#*
#*   Copyright 2009, Intel Corporation
#*   All rights reserved. This program and the accompanying materials
#*   are licensed and made available under the terms and conditions of the BSD License
#*   which accompanies this distribution.  The full text of the license may be found at
#*   http://opensource.org/licenses/bsd-license.php
#*
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#*
#*
#------------------------------------------------------------------------------


#
# Float control word initial value: 
# all exceptions masked, double-precision, round-to-nearest
#
ASM_PFX(mFpuControlWord): .word     0x027F
#
# Multimedia-extensions control word:
# all exceptions masked, round-to-nearest, flush to zero for masked underflow
#
ASM_PFX(mMmxControlWord): .long     0x01F80

#
# Initializes floating point units for requirement of UEFI specification.
#
# This function initializes floating-point control word to 0x027F (all exceptions
# masked,double-precision, round-to-nearest) and multimedia-extensions control word
# (if supported) to 0x1F80 (all exceptions masked, round-to-nearest, flush to zero
# for masked underflow).
#
ASM_GLOBAL ASM_PFX(InitializeFloatingPointUnits)
ASM_PFX(InitializeFloatingPointUnits):

    #
    # Initialize floating point units
    #
    finit
    fldcw   ASM_PFX(mFpuControlWord)
    
    #
    # Set OSFXSR bit 9 in CR4
    #
    movq    %cr4, %rax 
    or      $200, %rax
    movq    %rax, %cr4
    
    ldmxcsr ASM_PFX(mMmxControlWord)
    
    ret


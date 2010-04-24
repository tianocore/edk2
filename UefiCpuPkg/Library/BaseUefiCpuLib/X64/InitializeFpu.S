#------------------------------------------------------------------------------
#*
#*   Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
#*   This program and the accompanying materials
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
    
    #
    # Float control word initial value: 
    # all exceptions masked, double-precision, round-to-nearest
    #
    pushq   $0x027F
    lea     (%rsp), %rax 
    fldcw   (%rax)
    popq    %rax
    
    #
    # Set OSFXSR bit 9 in CR4
    #
    movq    %cr4, %rax 
    or      $0x200, %rax
    movq    %rax, %cr4

    #
    # Multimedia-extensions control word:
    # all exceptions masked, round-to-nearest, flush to zero for masked underflow
    #    
    pushq   $0x01F80
    lea     (%rsp), %rax
    ldmxcsr (%rax)
    popq    %rax
    
    ret


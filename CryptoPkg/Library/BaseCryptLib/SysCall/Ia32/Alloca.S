#------------------------------------------------------------------------------
#
# Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   Alloca.S
#
# Abstract:
#
#   Implementation for allocation of automatically reclaimed memory, which is
#   used to allocate space off the runtime stack.
#   (NOTE: There is a assumption in this code that the page size equal to 4K)
#
#------------------------------------------------------------------------------


    .686:
    .code:

ASM_GLOBAL ASM_PFX(_alloca)

#------------------------------------------------------------------------------
#
# void __cdecl _alloca (unsigned size)
#
#------------------------------------------------------------------------------
ASM_PFX(_alloca):

    pushl   %ecx
    cmpl    $0x1000, %eax
    leal    8(%esp), %ecx
    jb      LastPage

ProbePages:
    subl    $0x1000, %ecx
    subl    $0x1000, %eax
    testl   %eax, 0(%ecx)
    cmpl    $0x1000, %eax
    jae     ProbePages

LastPage:
    subl    %eax, %ecx
    movl    %esp, %eax
    testl   %eax, 0(%ecx)

    movl    %ecx, %esp
    movl    0(%eax), %ecx
    movl    4(%eax), %eax
    pushl   %eax

    ret

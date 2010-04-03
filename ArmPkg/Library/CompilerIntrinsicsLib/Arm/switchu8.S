#------------------------------------------------------------------------------ 
#
# Copyright (c) 2008-2009 Apple Inc. All rights reserved.
#
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

#/** @file
#  Compiler intrinsic for ARM compiler
#
#  Copyright (c) 2008, Apple, Inc.
#  All rights reserved. This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http)://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.;
#
#**/
#        

.text
.p2align 2

.globl ASM_PFX(__switchu8)


ASM_PFX(__switchu8):
    ldrb      ip,[lr,#-1]
    cmp       r3,ip
    ldrccb    r3,[lr,r3]
    ldrcsb    r3,[lr,ip]
    add       ip,lr,r3,LSL #1
    bx        ip


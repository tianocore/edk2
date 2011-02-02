#------------------------------------------------------------------------------ 
#
# Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

\s\s.text
\s\s.align 2
\s\sGCC_ASM_EXPORT(__ucmpdi2)
\s\s
ASM_PFX(__ucmpdi2):
\s\sstmfd\s\ssp!, {r4, r5, r8, lr}
\s\scmp\s\sr1, r3
\s\smov\s\sr8, r0
\s\smov\s\sr4, r2
\s\smov\s\sr5, r3
\s\sbcc\s\sL2
\s\sbhi\s\sL4
\s\scmp\s\sr0, r2
\s\sbcc\s\sL2
\s\smovls\s\sr0, #1
\s\sbls\s\sL8
\s\sb\s\sL4
L2:
\s\smov\s\sr0, #0
\s\sb\s\sL8
L4:
\s\smov\s\sr0, #2
L8:
\s\sldmfd\s\ssp!, {r4, r5, r8, pc}

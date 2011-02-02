#------------------------------------------------------------------------------ 
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
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
\s\sGCC_ASM_EXPORT(__moddi3)

ASM_PFX(__moddi3):
\s\sstmfd\s\ssp!, {r4, r5, r7, lr}
\s\smov\s\sr4, r1, asr #31
\s\sadd\s\sr7, sp, #8
\s\sstmfd\s\ssp!, {r10, r11}
\s\smov\s\sr10, r3, asr #31
\s\ssub\s\ssp, sp, #16
\s\smov\s\sr5, r4
\s\smov\s\sr11, r10
\s\seor\s\sr0, r0, r4
\s\seor\s\sr1, r1, r4
\s\seor\s\sr2, r2, r10
\s\seor\s\sr3, r3, r10
\s\sadd\s\sip, sp, #8
\s\ssubs\s\sr0, r0, r4
\s\ssbc\s\sr1, r1, r5
\s\ssubs\s\sr2, r2, r10
\s\ssbc\s\sr3, r3, r11
\s\sstr\s\sip, [sp, #0]
\s\sbl\s\sASM_PFX(__udivmoddi4)
\s\sldrd\s\sr0, [sp, #8]
\s\seor\s\sr0, r0, r4
\s\seor\s\sr1, r1, r4
\s\ssubs\s\sr0, r0, r4
\s\ssbc\s\sr1, r1, r5
\s\ssub\s\ssp, r7, #16
\s\sldmfd\s\ssp!, {r10, r11}
\s\sldmfd\s\ssp!, {r4, r5, r7, pc}

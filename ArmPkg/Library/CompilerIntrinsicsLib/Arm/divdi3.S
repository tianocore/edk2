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
\s\sGCC_ASM_EXPORT(__divdi3)
\s\s
ASM_PFX(__divdi3):
\s\s@ args = 0, pretend = 0, frame = 0
\s\s@ frame_needed = 1, uses_anonymous_args = 0
\s\sstmfd\s\ssp!, {r4, r5, r7, lr}
\s\smov\s\sr4, r3, asr #31
\s\sadd\s\sr7, sp, #8
\s\sstmfd\s\ssp!, {r10, r11}
\s\smov\s\sr10, r1, asr #31
\s\ssub\s\ssp, sp, #8
\s\smov\s\sr11, r10
\s\smov\s\sr5, r4
\s\seor\s\sr0, r0, r10
\s\seor\s\sr1, r1, r10
\s\seor\s\sr2, r2, r4
\s\seor\s\sr3, r3, r4
\s\ssubs\s\sr2, r2, r4
\s\ssbc\s\sr3, r3, r5
\s\smov\s\sip, #0
\s\ssubs\s\sr0, r0, r10
\s\ssbc\s\sr1, r1, r11
\s\sstr\s\sip, [sp, #0]
\s\sbl\s\sASM_PFX(__udivmoddi4)
\s\seor\s\sr2, r10, r4
\s\seor\s\sr3, r10, r4
\s\seor\s\sr0, r0, r2
\s\seor\s\sr1, r1, r3
\s\ssubs\s\sr0, r0, r2
\s\ssbc\s\sr1, r1, r3
\s\ssub\s\ssp, r7, #16
\s\sldmfd\s\ssp!, {r10, r11}
\s\sldmfd\s\ssp!, {r4, r5, r7, pc}

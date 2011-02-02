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
\s\sGCC_ASM_EXPORT(__divsi3)
\s\s
ASM_PFX(__divsi3):
\s\seor\s\sr3, r0, r0, asr #31
\s\seor\s\sr2, r1, r1, asr #31
\s\sstmfd\s\ssp!, {r4, r5, r7, lr}
\s\smov\s\sr5, r0, asr #31
\s\sadd\s\sr7, sp, #8
\s\smov\s\sr4, r1, asr #31
\s\ssub\s\sr0, r3, r0, asr #31
\s\ssub\s\sr1, r2, r1, asr #31
\s\sbl\s\sASM_PFX(__udivsi3)
\s\seor\s\sr1, r5, r4
\s\seor\s\sr0, r0, r1
\s\srsb\s\sr0, r1, r0
\s\sldmfd\s\ssp!, {r4, r5, r7, pc}

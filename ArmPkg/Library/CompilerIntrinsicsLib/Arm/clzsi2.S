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
\s\sGCC_ASM_EXPORT(__clzsi2)

ASM_PFX(__clzsi2):
\s\s@ frame_needed = 1, uses_anonymous_args = 0
\s\sstmfd\s\ssp!, {r7, lr}
\s\sadd\s\sr7, sp, #0
\s\smovs\s\sr3, r0, lsr #16
\s\smovne\s\sr3, #16
\s\smoveq\s\sr3, #0
\s\smovne\s\sr9, #0
\s\smoveq\s\sr9, #16
\s\smov\s\sr3, r0, lsr r3
\s\stst\s\sr3, #65280
\s\smovne\s\sr0, #8
\s\smoveq\s\sr0, #0
\s\smovne\s\slr, #0
\s\smoveq\s\slr, #8
\s\smov\s\sr3, r3, lsr r0
\s\stst\s\sr3, #240
\s\smovne\s\sr0, #4
\s\smoveq\s\sr0, #0
\s\smovne\s\sip, #0
\s\smoveq\s\sip, #4
\s\smov\s\sr3, r3, lsr r0
\s\stst\s\sr3, #12
\s\smovne\s\sr0, #2
\s\smoveq\s\sr0, #0
\s\smovne\s\sr1, #0
\s\smoveq\s\sr1, #2
\s\smov\s\sr2, r3, lsr r0
\s\sadd\s\sr3, lr, r9
\s\sadd\s\sr0, r3, ip
\s\sadd\s\sr1, r0, r1
\s\smov\s\sr0, r2, lsr #1
\s\seor\s\sr0, r0, #1
\s\sands\s\sr0, r0, #1
\s\smvnne\s\sr0, #0
\s\srsb\s\sr3, r2, #2
\s\sand\s\sr0, r0, r3
\s\sadd\s\sr0, r1, r0
\s\sldmfd\s\ssp!, {r7, pc}

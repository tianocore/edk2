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
\s\sGCC_ASM_EXPORT(__udivsi3)

ASM_PFX(__udivsi3):
\s\scmp\s\sr1, #0
\s\scmpne\s\sr0, #0
\s\sstmfd\s\ssp!, {r4, r5, r7, lr}
\s\sadd\s\sr7, sp, #8
\s\sbeq\s\sL2
\s\sclz\s\sr2, r1
\s\sclz\s\sr3, r0
\s\srsb\s\sr3, r3, r2
\s\scmp\s\sr3, #31
\s\sbhi\s\sL2
\s\sldmeqfd\s\ssp!, {r4, r5, r7, pc}
\s\sadd\s\sr5, r3, #1
\s\srsb\s\sr3, r3, #31
\s\smov\s\slr, #0
\s\smov\s\sr2, r0, asl r3
\s\smov\s\sip, r0, lsr r5
\s\smov\s\sr4, lr
\s\sb\s\sL8
L9:
\s\smov\s\sr0, r2, lsr #31
\s\sorr\s\sip, r0, ip, asl #1
\s\sorr\s\sr2, r3, lr
\s\srsb\s\sr3, ip, r1
\s\ssub\s\sr3, r3, #1
\s\sand\s\sr0, r1, r3, asr #31
\s\smov\s\slr, r3, lsr #31
\s\srsb\s\sip, r0, ip
\s\sadd\s\sr4, r4, #1
L8:
\s\scmp\s\sr4, r5
\s\smov\s\sr3, r2, asl #1
\s\sbne\s\sL9
\s\sorr\s\sr0, r3, lr
\s\sldmfd\s\ssp!, {r4, r5, r7, pc}
L2:
\s\smov\s\sr0, #0
\s\sldmfd\s\ssp!, {r4, r5, r7, pc}


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
\s\sGCC_ASM_EXPORT(__muldi3)

ASM_PFX(__muldi3):
\s\sstmfd\s\ssp!, {r4, r5, r6, r7, lr}
\s\sadd\s\sr7, sp, #12
\s\sstmfd\s\ssp!, {r8, r10, r11}
\s\sldr\s\sr11, L4
\s\smov\s\sr4, r0, lsr #16
\s\sand\s\sr8, r0, r11
\s\sand\s\sip, r2, r11
\s\smul\s\slr, ip, r8
\s\smul\s\sip, r4, ip
\s\ssub\s\ssp, sp, #8
\s\sadd\s\sr10, ip, lr, lsr #16
\s\sand\s\sip, r10, r11
\s\sand\s\slr, lr, r11
\s\smov\s\sr6, r2, lsr #16
\s\sstr\s\sr4, [sp, #4]
\s\sadd\s\sr4, lr, ip, asl #16
\s\smul\s\sip, r8, r6
\s\smov\s\sr5, r10, lsr #16
\s\sadd\s\sr10, ip, r4, lsr #16
\s\sand\s\sip, r10, r11
\s\sand\s\slr, r4, r11
\s\sadd\s\sr4, lr, ip, asl #16
\s\smul\s\sr0, r3, r0
\s\sadd\s\sip, r5, r10, lsr #16
\s\sldr\s\sr5, [sp, #4]
\s\smla\s\sr0, r2, r1, r0
\s\smla\s\sr5, r6, r5, ip
\s\smov\s\sr10, r4
\s\sadd\s\sr11, r0, r5
\s\smov\s\sr1, r11
\s\smov\s\sr0, r4
\s\ssub\s\ssp, r7, #24
\s\sldmfd\s\ssp!, {r8, r10, r11}
\s\sldmfd\s\ssp!, {r4, r5, r6, r7, pc}
\s\s.p2align 2
L5:
\s\s.align 2
L4:
\s\s.long\s\s65535

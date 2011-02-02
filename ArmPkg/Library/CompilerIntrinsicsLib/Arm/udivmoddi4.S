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
\s\sGCC_ASM_EXPORT(__udivmoddi4)
\s\s
ASM_PFX(__udivmoddi4):
\s\sstmfd\s\ssp!, {r4, r5, r6, r7, lr}
\s\sadd\s\sr7, sp, #12
\s\sstmfd\s\ssp!, {r10, r11}
\s\ssub\s\ssp, sp, #20
\s\sstmia\s\ssp, {r2-r3}
\s\sldr\s\sr6, [sp, #48]
\s\sorrs\s\sr2, r2, r3
\s\smov\s\sr10, r0
\s\smov\s\sr11, r1
\s\sbeq\s\sL2
\s\ssubs\s\sip, r1, #0
\s\sbne\s\sL4
\s\scmp\s\sr3, #0
\s\sbne\s\sL6
\s\scmp\s\sr6, #0
\s\sbeq\s\sL8
\s\smov\s\sr1, r2
\s\sbl\s\sASM_PFX(__umodsi3)
\s\smov\s\sr1, #0
\s\sstmia\s\sr6, {r0-r1}
L8:
\s\sldr\s\sr1, [sp, #0]
\s\smov\s\sr0, r10
\s\sb\s\sL45
L6:
\s\scmp\s\sr6, #0
\s\smovne\s\sr1, #0
\s\sstmneia\s\sr6, {r0-r1}
\s\sb\s\sL2
L4:
\s\sldr\s\sr1, [sp, #0]
\s\scmp\s\sr1, #0
\s\sbne\s\sL12
\s\sldr\s\sr2, [sp, #4]
\s\scmp\s\sr2, #0
\s\sbne\s\sL14
\s\scmp\s\sr6, #0
\s\sbeq\s\sL16
\s\smov\s\sr1, r2
\s\smov\s\sr0, r11
\s\sbl\s\sASM_PFX(__umodsi3)
\s\smov\s\sr1, #0
\s\sstmia\s\sr6, {r0-r1}
L16:
\s\sldr\s\sr1, [sp, #4]
\s\smov\s\sr0, r11
L45:
\s\sbl\s\sASM_PFX(__udivsi3)
L46:
\s\smov\s\sr10, r0
\s\smov\s\sr11, #0
\s\sb\s\sL10
L14:
\s\ssubs\s\sr1, r0, #0
\s\sbne\s\sL18
\s\scmp\s\sr6, #0
\s\sbeq\s\sL16
\s\sldr\s\sr1, [sp, #4]
\s\smov\s\sr0, r11
\s\sbl\s\sASM_PFX(__umodsi3)
\s\smov\s\sr4, r10
\s\smov\s\sr5, r0
\s\sstmia\s\sr6, {r4-r5}
\s\sb\s\sL16
L18:
\s\ssub\s\sr3, r2, #1
\s\stst\s\sr2, r3
\s\sbne\s\sL22
\s\scmp\s\sr6, #0
\s\smovne\s\sr4, r0
\s\sandne\s\sr5, ip, r3
\s\sstmneia\s\sr6, {r4-r5}
L24:
\s\srsb\s\sr3, r2, #0
\s\sand\s\sr3, r2, r3
\s\sclz\s\sr3, r3
\s\srsb\s\sr3, r3, #31
\s\smov\s\sr0, ip, lsr r3
\s\sb\s\sL46
L22:
\s\sclz\s\sr2, r2
\s\sclz\s\sr3, ip
\s\srsb\s\sr3, r3, r2
\s\scmp\s\sr3, #30
\s\sbhi\s\sL48
\s\srsb\s\sr2, r3, #31
\s\sadd\s\slr, r3, #1
\s\smov\s\sr3, r1, asl r2
\s\sstr\s\sr3, [sp, #12]
\s\smov\s\sr3, r1, lsr lr
\s\sldr\s\sr0, [sp, #0]
\s\smov\s\sr5, ip, lsr lr
\s\sorr\s\sr4, r3, ip, asl r2
\s\sstr\s\sr0, [sp, #8]
\s\sb\s\sL29
L12:
\s\sldr\s\sr3, [sp, #4]
\s\scmp\s\sr3, #0
\s\sbne\s\sL30
\s\ssub\s\sr3, r1, #1
\s\stst\s\sr1, r3
\s\sbne\s\sL32
\s\scmp\s\sr6, #0
\s\sandne\s\sr3, r3, r0
\s\smovne\s\sr2, r3
\s\smovne\s\sr3, #0
\s\sstmneia\s\sr6, {r2-r3}
L34:
\s\scmp\s\sr1, #1
\s\sbeq\s\sL10
\s\srsb\s\sr3, r1, #0
\s\sand\s\sr3, r1, r3
\s\sclz\s\sr3, r3
\s\srsb\s\sr0, r3, #31
\s\smov\s\sr1, ip, lsr r0
\s\srsb\s\sr3, r0, #32
\s\smov\s\sr0, r10, lsr r0
\s\sorr\s\sip, r0, ip, asl r3
\s\sstr\s\sr1, [sp, #12]
\s\sstr\s\sip, [sp, #8]
\s\sldrd\s\sr10, [sp, #8]
\s\sb\s\sL10
L32:
\s\sclz\s\sr2, r1
\s\sclz\s\sr3, ip
\s\srsb\s\sr3, r3, r2
\s\srsb\s\sr4, r3, #31
\s\smov\s\sr2, r0, asl r4
\s\smvn\s\sr1, r3
\s\sand\s\sr2, r2, r1, asr #31
\s\sadd\s\slr, r3, #33
\s\sstr\s\sr2, [sp, #8]
\s\sadd\s\sr2, r3, #1
\s\smov\s\sr3, r3, asr #31
\s\sand\s\sr0, r3, r0, asl r1
\s\smov\s\sr3, r10, lsr r2
\s\sorr\s\sr3, r3, ip, asl r4
\s\sand\s\sr3, r3, r1, asr #31
\s\sorr\s\sr0, r0, r3
\s\smov\s\sr3, ip, lsr lr
\s\sstr\s\sr0, [sp, #12]
\s\smov\s\sr0, r10, lsr lr
\s\sand\s\sr5, r3, r2, asr #31
\s\srsb\s\sr3, lr, #31
\s\smov\s\sr3, r3, asr #31
\s\sorr\s\sr0, r0, ip, asl r1
\s\sand\s\sr3, r3, ip, lsr r2
\s\sand\s\sr0, r0, r2, asr #31
\s\sorr\s\sr4, r3, r0
\s\sb\s\sL29
L30:
\s\sclz\s\sr2, r3
\s\sclz\s\sr3, ip
\s\srsb\s\sr3, r3, r2
\s\scmp\s\sr3, #31
\s\sbls\s\sL37
L48:
\s\scmp\s\sr6, #0
\s\sstmneia\s\sr6, {r10-r11}
\s\sb\s\sL2
L37:
\s\srsb\s\sr1, r3, #31
\s\smov\s\sr0, r0, asl r1
\s\sadd\s\slr, r3, #1
\s\smov\s\sr2, #0
\s\sstr\s\sr0, [sp, #12]
\s\smov\s\sr0, r10, lsr lr
\s\sstr\s\sr2, [sp, #8]
\s\ssub\s\sr2, r3, #31
\s\sand\s\sr0, r0, r2, asr #31
\s\smov\s\sr3, ip, lsr lr
\s\sorr\s\sr4, r0, ip, asl r1
\s\sand\s\sr5, r3, r2, asr #31
L29:
\s\smov\s\sip, #0
\s\smov\s\sr10, ip
\s\sb\s\sL40
L41:
\s\sldr\s\sr1, [sp, #12]
\s\sldr\s\sr2, [sp, #8]
\s\smov\s\sr3, r4, lsr #31
\s\sorr\s\sr5, r3, r5, asl #1
\s\smov\s\sr3, r1, lsr #31
\s\sorr\s\sr4, r3, r4, asl #1
\s\smov\s\sr3, r2, lsr #31
\s\sorr\s\sr0, r3, r1, asl #1
\s\sorr\s\sr1, ip, r2, asl #1
\s\sldmia\s\ssp, {r2-r3}
\s\sstr\s\sr0, [sp, #12]
\s\ssubs\s\sr2, r2, r4
\s\ssbc\s\sr3, r3, r5
\s\sstr\s\sr1, [sp, #8]
\s\ssubs\s\sr0, r2, #1
\s\ssbc\s\sr1, r3, #0
\s\smov\s\sr2, r1, asr #31
\s\sldmia\s\ssp, {r0-r1}
\s\smov\s\sr3, r2
\s\sand\s\sip, r2, #1
\s\sand\s\sr3, r3, r1
\s\sand\s\sr2, r2, r0
\s\ssubs\s\sr4, r4, r2
\s\ssbc\s\sr5, r5, r3
\s\sadd\s\sr10, r10, #1
L40:
\s\scmp\s\sr10, lr
\s\sbne\s\sL41
\s\sldrd\s\sr0, [sp, #8]
\s\sadds\s\sr0, r0, r0
\s\sadc\s\sr1, r1, r1
\s\scmp\s\sr6, #0
\s\sorr\s\sr10, r0, ip
\s\smov\s\sr11, r1
\s\sstmneia\s\sr6, {r4-r5}
\s\sb\s\sL10
L2:
\s\smov\s\sr10, #0
\s\smov\s\sr11, #0
L10:
\s\smov\s\sr0, r10
\s\smov\s\sr1, r11
\s\ssub\s\ssp, r7, #20
\s\sldmfd\s\ssp!, {r10, r11}
\s\sldmfd\s\ssp!, {r4, r5, r6, r7, pc}

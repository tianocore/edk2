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
\s\sGCC_ASM_EXPORT(memcpy)

ASM_PFX(memcpy):
\s\sstmfd\s\ssp!, {r7, lr}
\s\smov\s\sip, #0
\s\sadd\s\sr7, sp, #0
\s\smov\s\slr, r0
\s\sb\s\sL4
L5:
\s\sldrb\s\sr3, [r1], #1\s\s@ zero_extendqisi2
\s\sadd\s\sip, ip, #1
\s\sand\s\sr3, r3, #255
\s\sstrb\s\sr3, [lr], #1
L4:
\s\scmp\s\sip, r2
\s\sbne\s\sL5
\s\sldmfd\s\ssp!, {r7, pc}


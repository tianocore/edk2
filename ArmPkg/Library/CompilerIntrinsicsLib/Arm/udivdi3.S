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
\s\sGCC_ASM_EXPORT(__udivdi3)

ASM_PFX(__udivdi3):
\s\sstmfd\s\ssp!, {r7, lr}
\s\sadd\s\sr7, sp, #0
\s\ssub\s\ssp, sp, #8
\s\smov\s\sip, #0
\s\sstr\s\sip, [sp, #0]
\s\sbl\s\sASM_PFX(__udivmoddi4)
\s\ssub\s\ssp, r7, #0
\s\sldmfd\s\ssp!, {r7, pc}

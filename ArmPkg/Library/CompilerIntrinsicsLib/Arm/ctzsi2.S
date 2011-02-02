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
\s\sGCC_ASM_EXPORT(__ctzsi2)
\s\s
ASM_PFX(__ctzsi2):
\s\suxth\s\sr3, r0
\s\scmp\s\sr3, #0
\s\smoveq\s\sip, #16
\s\smovne\s\sip, #0
\s\s@ lr needed for prologue
\s\smov\s\sr0, r0, lsr ip
\s\stst\s\sr0, #255
\s\smovne\s\sr3, #0
\s\smoveq\s\sr3, #8
\s\smov\s\sr0, r0, lsr r3
\s\stst\s\sr0, #15
\s\smovne\s\sr1, #0
\s\smoveq\s\sr1, #4
\s\sadd\s\sr3, r3, ip
\s\smov\s\sr0, r0, lsr r1
\s\stst\s\sr0, #3
\s\smovne\s\sr2, #0
\s\smoveq\s\sr2, #2
\s\sadd\s\sr3, r3, r1
\s\smov\s\sr0, r0, lsr r2
\s\sand\s\sr0, r0, #3
\s\sadd\s\sr2, r3, r2
\s\seor\s\sr3, r0, #1
\s\smov\s\sr0, r0, lsr #1
\s\sands\s\sr3, r3, #1
\s\smvnne\s\sr3, #0
\s\srsb\s\sr0, r0, #2
\s\sand\s\sr0, r3, r0
\s\sadd\s\sr0, r2, r0
\s\sbx\s\slr

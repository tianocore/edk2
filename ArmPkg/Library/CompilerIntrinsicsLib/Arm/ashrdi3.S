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
\s\sGCC_ASM_EXPORT(__ashrdi3)

ASM_PFX(__ashrdi3):
\s\scmp\s\sr2, #31
\s\sbls\s\sL2
\s\scmp\s\sr2, #63
\s\ssubls\s\sr2, r2, #32
\s\smov\s\sip, r1, asr #31
\s\smovls\s\sr2, r1, asr r2
\s\smovhi\s\sr2, ip
\s\smov\s\sr0, r2
\s\smov\s\sr1, ip
\s\sbx\s\slr
L2:
\s\scmp\s\sr2, #0
\s\srsbne\s\sr3, r2, #32
\s\smovne\s\sr3, r1, asl r3
\s\smovne\s\sr1, r1, asr r2
\s\sorrne\s\sr0, r3, r0, lsr r2
\s\sbx\s\slr

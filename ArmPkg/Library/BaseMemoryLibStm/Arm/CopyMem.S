#------------------------------------------------------------------------------ 
#
# CopyMem() worker for ARM
#
# This file started out as C code that did 64 bit moves if the buffer was
# 32-bit aligned, else it does a byte copy. It also does a byte copy for
# any trailing bytes. It was updated to do 32-byte copies using stm/ldm. 
#
# Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

/**
  Copy Length bytes from Source to Destination. Overlap is OK.

  This implementation 

  @param  Destination Target of copy
  @param  Source      Place to copy from
  @param  Length      Number of bytes to copy

  @return Destination


VOID *
EFIAPI
InternalMemCopyMem (
  OUT     VOID                      *DestinationBuffer,
  IN      CONST VOID                *SourceBuffer,
  IN      UINTN                     Length
  )
**/
.text
.align 2
GCC_ASM_EXPORT(InternalMemCopyMem)

ASM_PFX(InternalMemCopyMem):
\s\sstmfd\s\ssp!, {r4-r11, lr}
\s\stst\s\sr0, #3
\s\smov\s\sr11, r0
\s\smov\s\sr10, r0
\s\smov\s\sip, r2
\s\smov\s\slr, r1
\s\smovne\s\sr0, #0
\s\sbne\s\sL4
\s\stst\s\sr1, #3
\s\smovne\s\sr3, #0
\s\smoveq\s\sr3, #1
\s\scmp\s\sr2, #31
\s\smovls\s\sr0, #0
\s\sandhi\s\sr0, r3, #1
L4:
\s\scmp\s\sr11, r1
\s\sbcc\s\sL26
\s\sbls\s\sL7
\s\srsb\s\sr3, r1, r11
\s\scmp\s\sip, r3
\s\sbcc\s\sL26
\s\scmp\s\sip, #0
\s\sbeq\s\sL7
\s\sadd\s\sr10, r11, ip
\s\sadd\s\slr, ip, r1
\s\sb\s\sL16
L29:
\s\ssub\s\sip, ip, #8
\s\scmp\s\sip, #7
\s\sldrd\s\sr2, [lr, #-8]!
\s\smovls\s\sr0, #0
\s\scmp\s\sip, #0
\s\sstrd\s\sr2, [r10, #-8]!
\s\sbeq\s\sL7
L16:
\s\scmp\s\sr0, #0
\s\sbne\s\sL29
\s\ssub\s\sr3, lr, #1
\s\ssub\s\sip, ip, #1
\s\sldrb\s\sr3, [r3, #0]\s\s
\s\ssub\s\sr2, r10, #1
\s\scmp\s\sip, #0
\s\ssub\s\sr10, r10, #1
\s\ssub\s\slr, lr, #1
\s\sstrb\s\sr3, [r2, #0]
\s\sbne\s\sL16
\s\sb   L7
L11:
\s\sldrb\s\sr3, [lr], #1\s\s
\s\ssub\s\sip, ip, #1
\s\sstrb\s\sr3, [r10], #1
L26:
\s\scmp\s\sip, #0
\s\sbeq\s\sL7
L30:
\s\scmp\s\sr0, #0
\s\sbeq\s\sL11
\s\ssub\s\sip, ip, #32
\s\scmp\s\sip, #31
\s\sldmia\s\slr!, {r2-r9}
\s\smovls\s\sr0, #0
\s\scmp\s\sip, #0
\s\sstmia\s\sr10!, {r2-r9}
\s\sbne\s\sL30
L7:
  mov\s\sr0, r11
\s\sldmfd\s\ssp!, {r4-r11, pc}
  

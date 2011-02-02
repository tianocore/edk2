;------------------------------------------------------------------------------ 
;
; CopyMem() worker for ARM
;
; This file started out as C code that did 64 bit moves if the buffer was
; 32-bit aligned, else it does a byte copy. It also does a byte copy for
; any trailing bytes. Update using VSTM/SLDM to do 128 byte copies.
;
; Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

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
\s\sEXPORT InternalMemCopyMem

\s\sAREA AsmMemStuff, CODE, READONLY

InternalMemCopyMem
\s\sstmfd\s\ssp!, {r4, r9, lr}
\s\stst\s\sr0, #3
\s\smov\s\sr4, r0
\s\smov\s\sr9, r0
\s\smov\s\sip, r2
\s\smov\s\slr, r1
\s\smovne\s\sr0, #0
\s\sbne\s\sL4
\s\stst\s\sr1, #3
\s\smovne\s\sr3, #0
\s\smoveq\s\sr3, #1
\s\scmp\s\sr2, #127
\s\smovls\s\sr0, #0
\s\sandhi\s\sr0, r3, #1
L4
\s\scmp\s\sr4, r1
\s\sbcc\s\sL26
\s\sbls\s\sL7
\s\srsb\s\sr3, r1, r4
\s\scmp\s\sip, r3
\s\sbcc\s\sL26
\s\scmp\s\sip, #0
\s\sbeq\s\sL7
\s\sadd\s\sr9, r4, ip
\s\sadd\s\slr, ip, r1
\s\sb\s\sL16
L29
\s\ssub\s\sip, ip, #8
\s\scmp\s\sip, #7
\s\sldrd\s\sr2, [lr, #-8]!
\s\smovls\s\sr0, #0
\s\scmp\s\sip, #0
\s\sstrd\s\sr2, [r9, #-8]!
\s\sbeq\s\sL7
L16
\s\scmp\s\sr0, #0
\s\sbne\s\sL29
\s\ssub\s\sr3, lr, #1
\s\ssub\s\sip, ip, #1
\s\sldrb\s\sr3, [r3, #0]\s\s
\s\ssub\s\sr2, r9, #1
\s\scmp\s\sip, #0
\s\ssub\s\sr9, r9, #1
\s\ssub\s\slr, lr, #1
\s\sstrb\s\sr3, [r2, #0]
\s\sbne\s\sL16
\s\sb   L7
L11
\s\sldrb\s\sr3, [lr], #1\s\s
\s\ssub\s\sip, ip, #1
\s\sstrb\s\sr3, [r9], #1
L26
\s\scmp\s\sip, #0
\s\sbeq\s\sL7
L30
\s\scmp\s\sr0, #0
\s\sbeq\s\sL11
\s\ssub\s\sip, ip, #128          // 32
\s\scmp\s\sip, #127              // 31
\s\svldm     lr!, {d0-d15}
\s\smovls\s\sr0, #0
\s\scmp\s\sip, #0
\s\svstm  r9!, {d0-d15}
\s\sbne\s\sL30
L7
  dsb
  mov\s\sr0, r4
\s\sldmfd\s\ssp!, {r4, r9, pc}

  END
  

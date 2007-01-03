#******************************************************************************
#*
#* Copyright (c) 2007, Intel Corporation                                                         
#* All rights reserved. This program and the accompanying materials                          
#* are licensed and made available under the terms and conditions of the BSD License         
#* which accompanies this distribution.  The full text of the license may be found at        
#* http://opensource.org/licenses/bsd-license.php                                            
#*                                                                                           
#* THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#* WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#*
#******************************************************************************

.globl _OrigVector
.globl _InterruptEntryStub
.globl _StubSize
.globl _CommonIdtEntry
.globl _FxStorSupport

_AppEsp:         .long   0x11111111 # ?
_DebugEsp:       .long   0x22222222 # ?
_ExtraPush:      .long   0x33333333 # ?
_ExceptData:     .long   0x44444444 # ?
_Eflags:         .long   0x55555555 # ?
_OrigVector:     .long   0x66666666 # ?
_StubSize:       .long   _InterruptEntryStubEnd - _InterruptEntryStub

.globl _FxStorSupport
_FxStorSupport:
        ret    

.globl _GetIdtr
_GetIdtr:
        ret    

.globl _WriteInterruptFlag
_WriteInterruptFlag:
        ret    

.globl _Vect2Desc
_Vect2Desc:
        ret    

.globl _InterruptEntryStub
_InterruptEntryStub:
        ret    

.globl _InterruptEntryStubEnd
_InterruptEntryStubEnd:
        ret    

.globl _CommonIdtEntry
_CommonIdtEntry:
        ret    

PhonyIretd:
        iret   

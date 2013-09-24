;------------------------------------------------------------------------------
; @file
; This file includes all other code files to assemble the reset vector code
;
; Copyright (c) 2008 - 2013, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

%ifdef ARCH_IA32
  %ifdef ARCH_X64
    %error "Only one of ARCH_IA32 or ARCH_X64 can be defined."
  %endif
%elifdef ARCH_X64
%else
  %error "Either ARCH_IA32 or ARCH_X64 must be defined."
%endif

%include "CommonMacros.inc"

%include "PostCodes.inc"

%ifdef DEBUG_NONE
  %include "DebugDisabled.asm"
%elifdef DEBUG_PORT80
  %include "Port80Debug.asm"
%elifdef DEBUG_SERIAL
  %include "SerialDebug.asm"
%else
  %error "No debug type was specified."
%endif

%include "Ia32/SearchForBfvBase.asm"
%include "Ia32/SearchForSecEntry.asm"

%ifdef ARCH_X64
%include "Ia32/Flat32ToFlat64.asm"
%include "Ia32/PageTables64.asm"
%endif

%include "Ia16/Real16ToFlat32.asm"
%include "Ia16/Init16.asm"

%include "Main.asm"

%include "Ia16/ResetVectorVtf0.asm"


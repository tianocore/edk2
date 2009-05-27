;------------------------------------------------------------------------------
;
; Copyright (c) 2008, Intel Corporation
; All rights reserved. This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   ResetVectorCode.asm
;
; Abstract:
;
;   Create code for VTF raw section.
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
%include "SerialDebug.asm"
%include "Ia32/SearchForBfvBase.asm"
%include "Ia32/SearchForSecAndPeiEntries.asm"
%include "JumpToSec.asm"
%include "Ia16/16RealTo32Flat.asm"

%ifdef ARCH_X64
%include "Ia32/32FlatTo64Flat.asm"
%endif

%include "Ia16/ResetVectorVtf0.asm"


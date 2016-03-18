;------------------------------------------------------------------------------
;
; Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Abstract:
;
;------------------------------------------------------------------------------

MSR_IA32_PLATFORM_ID        EQU     000000017h
MSR_IA32_BIOS_UPDT_TRIG     EQU     000000079h
MSR_IA32_BIOS_SIGN_ID       EQU     00000008bh


MicrocodeHdr                    STRUCT 1t
    MicrocodeHdrVersion     DWORD     ?
    MicrocodeHdrRevision    DWORD     ?
    MicrocodeHdrDate        DWORD     ?
    MicrocodeHdrProcessor   DWORD     ?
    MicrocodeHdrChecksum    DWORD     ?
    MicrocodeHdrLoader      DWORD     ?
    MicrocodeHdrFlags       DWORD     ?
    MicrocodeHdrDataSize    DWORD     ?
    MicrocodeHdrTotalSize   DWORD     ?
    MicrocodeHdrRsvd        DWORD     3t DUP (?)
MicrocodeHdr                   ENDS

ExtSigHdr                       STRUCT 1t
    ExtSigHdrCount          DWORD     ?
    ExtSigHdrChecksum       DWORD     ?
    ExtSigHdrRsvd           DWORD     3t DUP (?)
ExtSigHdr                       ENDS

ExtSig                          STRUCT 1t
    ExtSigProcessor         DWORD     ?
    ExtSigFlags             DWORD     ?
    ExtSigChecksum          DWORD     ?
ExtSig                          ENDS

LoadMicrocodeParams             STRUCT 1t
    MicrocodeCodeAddr       DWORD          ?
    MicrocodeCodeSize       DWORD          ?
LoadMicrocodeParams             ENDS

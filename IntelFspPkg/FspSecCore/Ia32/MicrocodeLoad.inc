;------------------------------------------------------------------------------
;
; Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
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

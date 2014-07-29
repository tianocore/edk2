;------------------------------------------------------------------------------
;
; Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
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

ucode                       STRUCT 1t
    version                 DWORD     ?
    revision                DWORD     ?
    date                    DWORD     ?
    processor               DWORD     ?
    checksum                DWORD     ?
    loader                  DWORD     ?
    rsvd                    DWORD     6t DUP (?)
    data                    DWORD     500t DUP (?)
ucode                       ENDS
ucode_t                     TYPEDEF     ucode

ucode_hdr                   STRUCT 1t
    version                 DWORD     ?
    revision                DWORD     ?
    date                    DWORD     ?
    processor               DWORD     ?
    checksum                DWORD     ?
    loader                  DWORD     ?
    flags                   DWORD     ?
    data_size               DWORD     ?
    total_size              DWORD     ?
    rsvd                    DWORD     3t DUP (?)
ucode_hdr                   ENDS
ucode_hdr_t                 TYPEDEF     ucode_hdr

ext_sig_hdr                 STRUCT 1t
    count                   DWORD     ?
    checksum                DWORD     ?
    rsvd                    DWORD     3t DUP (?)
ext_sig_hdr                 ENDS
ext_sig_hdr_t               TYPEDEF     ext_sig_hdr

ext_sig                     STRUCT 1t
    processor               DWORD     ?
    flags                   DWORD     ?
    checksum                DWORD     ?
ext_sig                     ENDS
ext_sig_t                   TYPEDEF     ext_sig

LOAD_UCODE_PARAMS           STRUCT 1t
    ucode_code_addr         DWORD          ?
    ucode_code_size         DWORD          ?
LOAD_UCODE_PARAMS           ENDS

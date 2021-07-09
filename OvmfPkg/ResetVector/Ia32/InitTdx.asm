;------------------------------------------------------------------------------
; @file
;   Initialize TDX_WORK_AREA to record the Tdx flag ('TDXG') and other Tdx info
;   so that the following codes can use these information.
;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS 32

;
; Modified:  EBP
;
InitTdx:
    ;
    ; In Td guest, BSP/AP shares the same entry point
    ; BSP builds up the page table, while APs shouldn't do the same task.
    ; Instead, APs just leverage the page table which is built by BSP.
    ; APs will wait until the page table is ready.
    ; In Td guest, vCPU 0 is treated as the BSP, the others are APs.
    ; ESI indicates the vCPU ID.
    ;
    cmp     esi, 0
    je      tdBspEntry

apWait:
    cmp     byte[TDX_WORK_AREA_PGTBL_READY], 0
    je      apWait
    jmp     doneTdxInit

tdBspEntry:
    ;
    ; It is of Tdx Guest
    ; Save the Tdx info in TDX_WORK_AREA so that the following code can use
    ; these information.
    ;
    mov     dword [TDX_WORK_AREA], 0x47584454 ; 'TDXG'

    ;
    ; EBP[6:0] CPU supported GPA width
    ;
    and     ebp, 0x3f
    cmp     ebp, 52
    jl      NotPageLevel5
    mov     byte[TDX_WORK_AREA_PAGELEVEL5], 1

NotPageLevel5:
    ;
    ; ECX[31:0] TDINITVP - Untrusted Configuration
    ;
    mov     DWORD[TDX_WORK_AREA_INITVP], ecx
    mov     DWORD[TDX_WORK_AREA_INFO], ebp

doneTdxInit:
    OneTimeCallRet InitTdx

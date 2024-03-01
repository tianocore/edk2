;------------------------------------------------------------------------------
; @file
;   Intel TDX routines
;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

%define VM_GUEST_TDX     2

BITS 32

;
; Check if it is Intel Tdx
;
; Modified: EAX, EBX, ECX, EDX
;
; If it is Intel Tdx, EAX is 1
; If it is not Intel Tdx, EAX is 0
;
IsTdx:
    ;
    ; CPUID (0)
    ;
    mov     eax, 0
    cpuid
    cmp     ebx, 0x756e6547  ; "Genu"
    jne     IsNotTdx
    cmp     edx, 0x49656e69  ; "ineI"
    jne     IsNotTdx
    cmp     ecx, 0x6c65746e  ; "ntel"
    jne     IsNotTdx

    ;
    ; CPUID (1)
    ;
    mov     eax, 1
    cpuid
    test    ecx, 0x80000000
    jz      IsNotTdx

    ;
    ; CPUID[0].EAX >= 0x21?
    ;
    mov     eax, 0
    cpuid
    cmp     eax, 0x21
    jl      IsNotTdx

    ;
    ; CPUID (0x21,0)
    ;
    mov     eax, 0x21
    mov     ecx, 0
    cpuid

    cmp     ebx, 0x65746E49   ; "Inte"
    jne     IsNotTdx
    cmp     edx, 0x5844546C   ; "lTDX"
    jne     IsNotTdx
    cmp     ecx, 0x20202020   ; "    "
    jne     IsNotTdx

    mov     eax, 1
    jmp     ExitIsTdx

IsNotTdx:
    xor     eax, eax

ExitIsTdx:

  OneTimeCallRet IsTdx

;
; Initialize work area if it is Tdx guest. Detailed definition is in
; OvmfPkg/Include/WorkArea.h.
; BSP and APs all go here. Only BSP initialize this work area.
;
; Param[in] EBX[5:0]    CPU Supported GPAW (48 or 52)
; Param[in] ESI[31:0]   vCPU ID (BSP is 0, others are AP)
;
; Modified:  EBX
;
InitTdxWorkarea:

    ;
    ; First check if it is Tdx
    ;
    OneTimeCall IsTdx

    test    eax, eax
    jz      ExitInitTdxWorkarea

    cmp     esi, 0
    je      TdxBspEntry

    ;
    ; In Td guest, BSP/AP shares the same entry point
    ; BSP builds up the page table, while APs shouldn't do the same task.
    ; Instead, APs just leverage the page table which is built by BSP.
    ; APs will wait until the page table is ready.
    ;
TdxApWait:
    cmp     byte[TDX_WORK_AREA_PGTBL_READY], 0
    je      TdxApWait
    jmp     ExitInitTdxWorkarea

TdxBspEntry:
    ;
    ; Set Type of WORK_AREA_GUEST_TYPE so that the following code can use
    ; these information.
    ;
    mov     byte[WORK_AREA_GUEST_TYPE], VM_GUEST_TDX

    ;
    ; EBX[5:0] CPU supported GPA width
    ;
    and     ebx, 0x3f
    mov     DWORD[TDX_WORK_AREA_GPAW], ebx

ExitInitTdxWorkarea:
    OneTimeCallRet InitTdxWorkarea

;
; Load the GDT and set the CS/DS/ES/FS/GS/SS.
;
; Modified:  EAX, DS, ES, FS, GS, SS, CS
;
ReloadFlat32:

    cli
    mov     eax, ADDR_OF(gdtr)
    lgdt    [eax]

    jmp     LINEAR_CODE_SEL:dword ADDR_OF(jumpToFlat32BitAndLandHere)

jumpToFlat32BitAndLandHere:

    debugShowPostCode POSTCODE_32BIT_MODE

    mov     ax, LINEAR_SEL
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    OneTimeCallRet ReloadFlat32

;
; Tdx initialization after entering into ResetVector
;
; Modified:  EAX, EBX, ECX, EDX, EBP, EDI, ESP
;
InitTdx:
    ;
    ; First load the GDT and jump to Flat32 mode
    ;
    OneTimeCall ReloadFlat32

    ;
    ; Initialization of Tdx work area
    ;
    OneTimeCall  InitTdxWorkarea

    OneTimeCallRet InitTdx

;
; Check TDX features, TDX or TDX-BSP or TDX-APs?
;
; By design TDX BSP is reponsible for initializing the PageTables.
; After PageTables are ready, byte[TDX_WORK_AREA_PGTBL_READY] is set to 1.
; APs will spin when byte[TDX_WORK_AREA_PGTBL_READY] is 0 until it is set to 1.
;
; When this routine is run on TDX BSP, byte[TDX_WORK_AREA_PGTBL_READY] should be 0.
; When this routine is run on TDX APs, byte[TDX_WORK_AREA_PGTBL_READY] should be 1.
;
;
; Modified:  EAX, EDX
;
; 0-NonTdx, 1-TdxBsp, 2-TdxAps, 3-TdxAps5Level
;
CheckTdxFeaturesBeforeBuildPagetables:
    xor     eax, eax
    cmp     byte[WORK_AREA_GUEST_TYPE], VM_GUEST_TDX
    jne     NotTdx

    xor     edx, edx
    mov     al, byte[TDX_WORK_AREA_PGTBL_READY]
    inc     eax

NotTdx:
    OneTimeCallRet CheckTdxFeaturesBeforeBuildPagetables

;
; Set byte[TDX_WORK_AREA_PGTBL_READY] to 1
;
TdxPostBuildPageTables:
    mov     byte[TDX_WORK_AREA_PGTBL_READY], 1
    OneTimeCallRet TdxPostBuildPageTables

%if PG_5_LEVEL

;
; Set byte[TDX_WORK_AREA_PGTBL_READY] to 2
;
TdxPostBuildPageTables5Level:
    mov     byte[TDX_WORK_AREA_PGTBL_READY], 2
    OneTimeCallRet TdxPostBuildPageTables5Level

%endif

;
; Check if TDX is enabled
;
; Modified:  EAX
;
; If TDX is enabled then EAX will be 1
; If TDX is disabled then EAX will be 0.
;
IsTdxEnabled:
    xor     eax, eax
    cmp     byte[WORK_AREA_GUEST_TYPE], VM_GUEST_TDX
    jne     TdxNotEnabled
    mov     eax, 1

TdxNotEnabled:
    OneTimeCallRet IsTdxEnabled

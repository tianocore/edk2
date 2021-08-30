;------------------------------------------------------------------------------
; @file
;   Intel TDX routines
;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

%define SEC_DEFAULT_CR0  0x00000023
%define SEC_DEFAULT_CR4  0x640
%define VM_GUEST_TDX     2

BITS 32

;
; Check if it is Intel Tdx
;
; Modified: EAX, EBX, ECX, EDX
;
; If it is Intel Tdx, EAX is zero
; If it is not Intel Tdx, EAX is non-zero
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

    mov     eax, 0
    jmp     ExitIsTdx

IsNotTdx:
    mov     eax, 1

ExitIsTdx:

  OneTimeCallRet IsTdx

;
; Initialize work area if it is Tdx guest. Detailed definition is in
; OvmfPkg/Include/WorkArea.h.
; BSP and APs all go here. Only BSP initialize this work area.
;
; Param[in] EBP[5:0]    CPU Supported GPAW (48 or 52)
; Param[in] ESI[31:0]   vCPU ID (BSP is 0, others are AP)
;
; Modified:  EBP
;
InitTdxWorkarea:

    ;
    ; First check if it is Tdx
    ;
    OneTimeCall IsTdx

    test    eax, eax
    jnz     ExitInitTdxWorkarea

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
    ; Set Type/Subtype of WORK_AREA_GUEST_TYPE so that the following code can use
    ; these information.
    ;
    mov     byte[WORK_AREA_GUEST_TYPE], VM_GUEST_TDX
    mov     byte[WORK_AREA_GUEST_SUBTYPE], 0

    ;
    ; EBP[5:0] CPU supported GPA width
    ;
    and     ebp, 0x3f
    cmp     ebp, 52
    jl      NotPageLevel5
    mov     byte[TDX_WORK_AREA_PAGELEVEL5], 1

NotPageLevel5:
    mov     DWORD[TDX_WORK_AREA_GPAW], ebp

ExitInitTdxWorkarea:
    OneTimeCallRet InitTdxWorkarea

;
; Load the GDT and set the CR0, then jump to Flat 32 protected mode.
;
; Modified:  EAX, EBX, CR0, CR4, DS, ES, FS, GS, SS
;
ReloadFlat32:

    cli
    mov     ebx, ADDR_OF(gdtr)
    lgdt    [ebx]

    mov     eax, SEC_DEFAULT_CR0
    mov     cr0, eax

    jmp     LINEAR_CODE_SEL:dword ADDR_OF(jumpToFlat32BitAndLandHere)

jumpToFlat32BitAndLandHere:

    mov     eax, SEC_DEFAULT_CR4
    mov     cr4, eax

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
    ; Save EBX in EBP because EBX will be changed in ReloadFlat32
    ;
    mov     ebp, ebx

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
; Called after SetCr3PageTables64 in Tdx guest to set CR0/CR4.
; If GPAW is 52, then CR3 is adjusted as well.
;
; Modified: EAX, EBX, CR0, CR3, CR4
;
PostSetCr3PageTables64Tdx:
    ;
    ; WORK_AREA_GUEST_TYPE was set in InitTdx if it is Tdx guest
    ;
    cmp     byte[WORK_AREA_GUEST_TYPE], VM_GUEST_TDX
    jne     ExitPostSetCr3PageTables64Tdx

    mov     eax, cr4
    bts     eax, 5                      ; enable PAE

    ;
    ; byte[TDX_WORK_AREA_PAGELEVEL5] holds the indicator whether 52bit is
    ; supported. if it is the case, need to set LA57 and use 5-level paging
    ;
    cmp     byte[TDX_WORK_AREA_PAGELEVEL5], 0
    jz      TdxSetCr4
    bts     eax, 12

TdxSetCr4:
    mov     cr4, eax
    mov     ebx, cr3

    ;
    ; if la57 is not set, we are ok
    ; if using 5-level paging, adjust top-level page directory
    ;
    bt      eax, 12
    jnc     TdxSetCr3
    mov     ebx, TDX_PT_ADDR (0)

TdxSetCr3:
    mov     cr3, ebx

    xor     ebx, ebx

ExitPostSetCr3PageTables64Tdx:
    OneTimeCallRet PostSetCr3PageTables64Tdx

;
; Build TDX Extra page table
;
; Modified: EAX, ECX
;
TdxBuildExtraPageTables:
    cmp     byte[WORK_AREA_GUEST_TYPE], VM_GUEST_TDX
    jne     ExitTdxBuildExtraPageTables

    xor     eax, eax
    mov     ecx, 0x400
tdClearTdxPageTablesMemoryLoop:
    mov     dword [ecx * 4 + TDX_PT_ADDR(0) - 4], eax
    loop    tdClearTdxPageTablesMemoryLoop

    ;
    ; Top level Page Directory Pointers (1 * 256TB entry)
    ;
    mov     dword[TDX_PT_ADDR (0)], PT_ADDR(0) + PAGE_PDP_ATTR

    ;
    ; Set TDX_WORK_AREA_PGTBL_READY to notify APs to go
    ;
    mov     byte[TDX_WORK_AREA_PGTBL_READY], 1

ExitTdxBuildExtraPageTables:
    OneTimeCallRet TdxBuildExtraPageTables

;
; Check TDX features, Non-TDX or TDX-BSP or TDX-APs?
;
; By design TDX BSP is reponsible for inintializing the PageTables.
; After PageTables are ready, byte[TDX_WORK_AREA_PGTBL_READY] is set to 1.
; APs will spin when byte[TDX_WORK_AREA_PGTBL_READY] is 0 until it is set to 1.
;
; When this routine is run on TDX BSP, byte[TDX_WORK_AREA_PGTBL_READY] should be 0.
; When this routine is run on TDX APs, byte[TDX_WORK_AREA_PGTBL_READY] should be 1.
;
;
; Modified:  EAX, EDX
;
; 0-NonTdx, 1-TdxBsp, 2-TdxAps
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

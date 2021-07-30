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

%define VM_GUEST_TDX          2

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
; Initialize Tdx work area (CC_WORK_AREA) if it is Tdx guest.
; BSP and APs all go here. Only BSP can initialize the WORK_AREA
;
; typedef struct {
;   UINT8   Type;     // 0 legacy, 1 SEV, 2 TDX
;   UINT8   SubType;  // Depends on Type
;   UINT8   Rsvd[2];  // Reserved
;   union VM_GUEST {
;     TDX_WORK_AREA Tdx;
;     SEV_WORK_AREA Sev;
;   } Guest;
; } CC_WORK_AREA_HEAD;
;
; typedef struct {
;   UINT8    IsPageLevel5;
;   UINT8    IsPageTableReady;
;   UINT8    Rsvd[2];
;   UINT32   Gpaw;
; } TDX_WORK_AREA
;
; Param[in] EBP[6:0]    CPU Supported GPAW (48 or 52)
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

    ;
    ; In Td guest, BSP/AP shares the same entry point
    ; BSP builds up the page table, while APs shouldn't do the same task.
    ; Instead, APs just leverage the page table which is built by BSP.
    ; APs will wait until the page table is ready.
    ;
    cmp     esi, 0
    je      TdxBspEntry

TdxApWait:
    cmp     byte[TDX_WORK_AREA_PGTBL_READY], 0
    je      TdxApWait
    jmp     ExitInitTdxWorkarea

TdxBspEntry:
    ;
    ; Set Type and SubType of CC_WORK_AREA so that the
    ; following code can use these information.
    ;
    mov     byte[CC_WORK_AREA], VM_GUEST_TDX
    mov     byte[CC_WORK_AREA_SUBTYPE], 0

    ;
    ; EBP[6:0] CPU supported GPA width
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
    ; TDX_WORK_AREA was set in InitTdx if it is Tdx guest
    ;
    cmp     byte[CC_WORK_AREA], VM_GUEST_TDX
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
; Check if TDX is enabled
;
; Modified:  EAX
;
; If TDX is enabled then EAX will be 1
; If TDX is disabled then EAX will be 0.
;
IsTdxEnabled:
    xor     eax, eax
    cmp     byte[CC_WORK_AREA], VM_GUEST_TDX
    jne     NotTdx
    mov     eax, 1

NotTdx:
    OneTimeCallRet IsTdxEnabled


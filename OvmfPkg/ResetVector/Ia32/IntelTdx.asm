;------------------------------------------------------------------------------
; @file
;   Intel TDX routines
;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

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
; Initialization code if it is Tdx guest.
; If it is Tdx guest, EBP[6:0] holds CPU supported GPAW, ESI[31:0] is the vCPU ID
;
; Modified:  EBP
;
InitTdx:

    ;
    ; First check if it is Tdx
    ;
    OneTimeCall IsTdx

    test    eax, eax
    jnz     ExitInitTdx

    ;
    ; In Td guest, BSP/AP shares the same entry point
    ; BSP builds up the page table, while APs shouldn't do the same task.
    ; Instead, APs just leverage the page table which is built by BSP.
    ; APs will wait until the page table is ready.
    ; In Td guest, vCPU 0 is treated as the BSP, the others are APs.
    ; ESI indicates the vCPU ID.
    ;
    cmp     esi, 0
    je      TdBspEntry

ApWait:
    cmp     byte[TDX_WORK_AREA_PGTBL_READY], 0
    je      ApWait
    jmp     ExitInitTdx

TdBspEntry:
    ;
    ; It is of Tdx Guest
    ; Save the Tdx info in TDX_WORK_AREA so that the following code can use
    ; these information.
    ;
    mov     dword[TDX_WORK_AREA], 0x47584454 ; 'TDXG'

    ;
    ; EBP[6:0] CPU supported GPA width
    ;
    and     ebp, 0x3f
    cmp     ebp, 52
    jl      NotPageLevel5
    mov     byte[TDX_WORK_AREA_PAGELEVEL5], 1

NotPageLevel5:
    mov     DWORD[TDX_WORK_AREA_INFO], ebp

ExitInitTdx:
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
    cmp     dword[TDX_WORK_AREA], 0x47584454 ; 'TDXG'
    jnz     ExitPostSetCr3PageTables64Tdx

    mov     eax, cr4
    bts     eax, 5                      ; enable PAE

    ;
    ; byte[TDX_WORK_AREA_PAGELEVEL5] holds the indicator whether 52bit is supported.
    ; if it is the case, need to set LA57 and use 5-level paging
    ;
    cmp     byte[TDX_WORK_AREA_PAGELEVEL5], 0
    jz      SetCr4
    bts     eax, 12

SetCr4:
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

    mov     eax, cr0
    bts     eax, 31                     ; set PG
    mov     cr0, eax                    ; enable paging

ExitPostSetCr3PageTables64Tdx:
    OneTimeCallRet PostSetCr3PageTables64Tdx


;------------------------------------------------------------------------------
;
; Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Abstract:
;
;   Switch the stack from temporary memory to permanent memory.
;
;------------------------------------------------------------------------------

    SECTION .text

extern ASM_PFX(SwapStack)
extern ASM_PFX(FeaturePcdGet (PcdFspSaveRestorePageTableEnable))

; Page table related bits in CR0/CR4/EFER
%define CR0_PG_MASK          0x80010000  ; CR0.PG and CR0.WP
%define CR4_PG_MASK          0x10B0      ; CR4.PSE, CR4.PAE, CR4.PGE and CR4.LA57
%define EFER_PG_MASK         0x800       ; EFER.NXE

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; Pei2LoaderSwitchStack (
;   VOID
;   )
;------------------------------------------------------------------------------
global ASM_PFX(Pei2LoaderSwitchStack)
ASM_PFX(Pei2LoaderSwitchStack):
    xor     eax, eax
    jmp     ASM_PFX(FspSwitchStack)

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; Loader2PeiSwitchStack (
;   VOID
;   )
;------------------------------------------------------------------------------
global ASM_PFX(Loader2PeiSwitchStack)
ASM_PFX(Loader2PeiSwitchStack):
    jmp     ASM_PFX(FspSwitchStack)

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; FspSwitchStack (
;   VOID
;   )
;------------------------------------------------------------------------------
global ASM_PFX(FspSwitchStack)
ASM_PFX(FspSwitchStack):
    ; Save current contexts
    push    eax
    pushfd
    cli
    pushad

    ;
    ; Allocate 4x4 bytes on the stack.
    ;
    sub     esp, 16
    cmp     byte [dword ASM_PFX(FeaturePcdGet (PcdFspSaveRestorePageTableEnable))], 0
    jz      SkipPagetableSave

    add     esp, 16
    ; Save EFER MSR lower 32 bits
    push   ecx
    push   eax
    mov    ecx, 0xC0000080
    rdmsr
    mov    edx, eax
    pop    eax
    pop    ecx
    push   edx

    ; Save CR registers
    mov    eax, cr4
    push   eax
    mov    eax, cr3
    push   eax
    mov    eax, cr0
    push   eax
SkipPagetableSave:

    sub     esp, 8
    sidt    [esp]

    ; Load new stack
    push    esp
    call    ASM_PFX(SwapStack)
    mov     esp, eax

    ; Restore previous contexts
    lidt    [esp]
    add     esp, 8

    cmp     byte [dword ASM_PFX(FeaturePcdGet (PcdFspSaveRestorePageTableEnable))], 0
    jz      SkipPagetableRestore
    ; [esp]    stores new cr0
    ; [esp+4]  stores new cr3
    ; [esp+8]  stores new cr4
    ; [esp+12] stores new Efer
    ;
    ; When new EFER.NXE == 1, the restore flow is: EFER --> CRx
    ; Otherwise: CRx --> EFER
    ; When new CR0.PG == 1, the restore flow for CRx is: CR3 --> CR4 --> CR0
    ; Otherwise, the restore flow is: CR0 --> CR3 --> CR4
    ;
    ; If NXE bit is changed to 1, change NXE before CR register
    ; This is because Nx bit in page table entry in new CR3 will be invalid
    ; if updating CR3 before EFER MSR.
    ;
    mov eax, [esp+12]
    bt  eax, 11
    jnc SkipEferLabel1

    ; Restore EFER MSR
    mov    ecx, 0xC0000080
    rdmsr
    and    eax, ~EFER_PG_MASK
    mov    ebx, [esp+12]
    and    ebx, EFER_PG_MASK
    or     eax, ebx
    wrmsr

SkipEferLabel1:

    ;
    ; if new cr0 is to disable page table, change CR0 before CR3/CR4
    ;
    mov     eax, [esp]
    bt      eax, 31
    jc      SkipCr0Label1

    ; Restore CR0
    mov     edx, cr0
    and     edx, ~CR0_PG_MASK
    mov     eax, [esp]
    and     eax, CR0_PG_MASK
    or      edx, eax
    mov     cr0, edx

SkipCr0Label1:

    ; Restore CR3/CR4
    mov     eax, [esp+4]
    mov     cr3, eax

    mov     edx, cr4
    and     edx, ~CR4_PG_MASK
    mov     eax, [esp+8]
    and     eax, CR4_PG_MASK
    or      edx, eax
    mov     cr4, edx

    ;
    ; if new cr0 is to enable page table, change CR0 after CR3/CR4
    ;
    mov     eax, [esp]
    bt      eax, 31
    jnc     SkipCr0Label2

    ; Restore CR0
    mov     edx, cr0
    and     edx, ~CR0_PG_MASK
    mov     eax, [esp]
    and     eax, CR0_PG_MASK
    or      edx, eax
    mov     cr0, edx

SkipCr0Label2:
    ;
    ; If NXE bit is changed to 0, change NXE after than CR regiser
    ;
    mov eax, [esp+12]
    bt  eax, 11
    jc SkipEferLabel2

    ; Restore EFER MSR
    mov    ecx, 0xC0000080
    rdmsr
    and    eax, ~EFER_PG_MASK
    mov    ebx, [esp+12]
    and    ebx, EFER_PG_MASK
    or     eax, ebx
    wrmsr

SkipEferLabel2:
SkipPagetableRestore:

    ; pop page table related registers.
    add     esp, 16

    popad
    popfd
    add     esp, 4
    ret


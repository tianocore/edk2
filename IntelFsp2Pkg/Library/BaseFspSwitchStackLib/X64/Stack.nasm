;------------------------------------------------------------------------------
;
; Copyright (c) 2022 - 2025, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Abstract:
;
;   Switch the stack from temporary memory to permanent memory.
;
;------------------------------------------------------------------------------
    DEFAULT REL
    SECTION .text

%include    "PushPopRegsNasm.inc"

; Page table related bits in CR0/CR4/EFER
%define CR0_PG_MASK          0x80010000  ; CR0.PG and CR0.WP
%define CR4_PG_MASK          0x10B0      ; CR4.PSE, CR4.PAE, CR4.PGE and CR4.LA57
%define EFER_PG_MASK         0x800       ; EFER.NXE

extern ASM_PFX(SwapStack)
extern ASM_PFX(FeaturePcdGet (PcdFspSaveRestorePageTableEnable))

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; Pei2LoaderSwitchStack (
;   VOID
;   )
;------------------------------------------------------------------------------
global ASM_PFX(Pei2LoaderSwitchStack)
ASM_PFX(Pei2LoaderSwitchStack):
    xor     rax, rax
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
    ; Save current contexts. The format must align with CONTEXT_STACK_64.
    push    rdx     ; Reserved QWORD for stack alignment
    push    rdx     ; ApiParam2
    push    rcx     ; ApiParam1
    push    rax     ; FspInfoHeader
    pushfq
    cli
    PUSHA_64

    ;
    ; Allocate 4x8 bytes on the stack.
    ;
    sub     rsp, 32
    lea     rdx, [ASM_PFX(FeaturePcdGet (PcdFspSaveRestorePageTableEnable))]
    mov     dl, byte [rdx]
    cmp     dl, 0
    jz      SkipPagetableSave

    add     rsp, 32
    ; Save EFER MSR
    push   rcx
    push   rax
    mov    rcx, 0xC0000080
    rdmsr
    shl    rdx, 0x20
    or     rdx, rax
    pop    rax
    pop    rcx
    push   rdx

    ; Save CR registers
    mov    rdx, cr4
    push   rdx
    mov    rdx, cr3
    push   rdx
    mov    rdx, cr0
    push   rdx
SkipPagetableSave:

    ; Save Segment registers
    mov     rdx, ss
    push    rdx
    mov     rdx, gs
    push    rdx
    mov     rdx, fs
    push    rdx
    mov     rdx, es
    push    rdx
    mov     rdx, ds
    push    rdx
    mov     rdx, cs
    push    rdx

    ; Reserve 16 bytes for GDT save/restore
    sub     rsp, 16
    sgdt    [rsp]

    ; Reserve 16 bytes for IDT save/restore
    sub     rsp, 16
    sidt    [rsp]

    ; Load new stack
    mov     rcx, rsp
    sub     rsp, 0x20
    call    ASM_PFX(SwapStack)
    add     rsp, 0x20
    mov     rsp, rax

    ; Restore previous contexts
    lidt    [rsp]
    add     rsp, 16

    ; Restore GDTR
    lgdt    [rsp]
    add     rsp, 16

    ; Restore Segment registers
    lea     rdx, [.0]
    push    rdx       ; Push return address
    retfq             ; Far return to restore CS (uses CS from stack + return address)
.0:
    pop     rdx
    mov     ds, dx
    pop     rdx
    mov     es, dx
    pop     rdx
    mov     fs, dx
    pop     rdx
    mov     gs, dx
    pop     rdx
    mov     ss, dx

    lea     rax, [ASM_PFX(FeaturePcdGet (PcdFspSaveRestorePageTableEnable))]
    mov     al, byte [rax]
    cmp     al, 0
    jz      SkipPagetableRestore
    ; [rsp]    stores new cr0
    ; [rsp+8]  stores new cr3
    ; [rsp+16] stores new cr4
    ; [rsp+24] stores new Efer
    ;
    ; When new EFER.NXE == 1, the restore flow is: EFER --> CRx
    ; Otherwise: CRx --> EFER
    ;
    ; If NXE bit is changed to 1, change NXE before CR register
    ; This is because Nx bit in page table entry in new CR3 will be invalid
    ; if updating CR3 before EFER MSR.
    ;
    mov rax, [rsp + 24]
    bt  rax, 11
    jnc SkipEferLabel1

    ; Restore EFER MSR
    mov    ecx, 0xC0000080
    rdmsr
    and    eax, ~EFER_PG_MASK
    mov    ebx, [rsp + 24]
    and    ebx, EFER_PG_MASK
    or     eax, ebx
    wrmsr

SkipEferLabel1:

    mov     rbx, [rsp]
    mov     rdx, cr0
    and     rdx, ~CR0_PG_MASK
    and     rbx, CR0_PG_MASK
    or      rdx, rbx
    mov     cr0, rdx

    mov     rbx, [rsp + 8]
    mov     cr3, rbx

    mov     rbx, [rsp + 16]
    mov     rdx, cr4
    and     rdx, ~CR4_PG_MASK
    and     rbx, CR4_PG_MASK
    or      rdx, rbx
    mov     cr4, rdx

    ;
    ; If NXE bit is changed to 0, change NXE after than CR regiser
    ;
    mov rax, [rsp + 24]
    bt  rax, 11
    jc  SkipEferLabel2

    ; Restore EFER MSR
    mov    ecx, 0xC0000080
    rdmsr
    and    eax, ~EFER_PG_MASK
    mov    ebx, [rsp + 24]
    and    ebx, EFER_PG_MASK
    or     eax, ebx
    wrmsr

SkipEferLabel2:
SkipPagetableRestore:
    ; pop page table related registers.
    add     rsp, 32

    POPA_64
    popfq
    add     rsp, 32 ; FspInfoHeader + ApiParam[2] + Reserved QWORD
    ret


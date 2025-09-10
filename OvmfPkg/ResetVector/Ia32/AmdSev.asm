;------------------------------------------------------------------------------
; @file
; Provide the functions to check whether SEV and SEV-ES is enabled.
;
; Copyright (c) 2017 - 2021, Advanced Micro Devices, Inc. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    32

;
; SEV-ES #VC exception handler support
;
; #VC handler local variable locations
;
%define VC_CPUID_RESULT_EAX         0
%define VC_CPUID_RESULT_EBX         4
%define VC_CPUID_RESULT_ECX         8
%define VC_CPUID_RESULT_EDX        12
%define VC_GHCB_MSR_EDX            16
%define VC_GHCB_MSR_EAX            20
%define VC_CPUID_REQUEST_REGISTER  24
%define VC_CPUID_FUNCTION          28

; #VC handler total local variable size
;
%define VC_VARIABLE_SIZE           32

; #VC handler GHCB CPUID request/response protocol values
;
%define GHCB_CPUID_REQUEST          4
%define GHCB_CPUID_RESPONSE         5
%define GHCB_CPUID_REGISTER_SHIFT  30
%define CPUID_INSN_LEN              2

; #VC handler offsets/sizes for accessing SNP CPUID page
;
%define SNP_CPUID_ENTRY_SZ         48
%define SNP_CPUID_COUNT             0
%define SNP_CPUID_ENTRY            16
%define SNP_CPUID_ENTRY_EAX_IN      0
%define SNP_CPUID_ENTRY_ECX_IN      4
%define SNP_CPUID_ENTRY_EAX        24
%define SNP_CPUID_ENTRY_EBX        28
%define SNP_CPUID_ENTRY_ECX        32
%define SNP_CPUID_ENTRY_EDX        36


%define SEV_GHCB_MSR                0xc0010130
%define SEV_STATUS_MSR              0xc0010131

; The #VC was not for CPUID
%define TERM_VC_NOT_CPUID           1

; The unexpected response code
%define TERM_UNEXPECTED_RESP_CODE   2

%define PAGE_PRESENT            0x01
%define PAGE_READ_WRITE         0x02
%define PAGE_USER_SUPERVISOR    0x04
%define PAGE_WRITE_THROUGH      0x08
%define PAGE_CACHE_DISABLE     0x010
%define PAGE_ACCESSED          0x020
%define PAGE_DIRTY             0x040
%define PAGE_PAT               0x080
%define PAGE_GLOBAL           0x0100
%define PAGE_2M_MBO            0x080
%define PAGE_2M_PAT          0x01000

%define PAGE_4K_PDE_ATTR (PAGE_ACCESSED + \
                          PAGE_DIRTY + \
                          PAGE_READ_WRITE + \
                          PAGE_PRESENT)

%define PAGE_PDP_ATTR (PAGE_ACCESSED + \
                       PAGE_READ_WRITE + \
                       PAGE_PRESENT)


; Macro is used to issue the MSR protocol based VMGEXIT. The caller is
; responsible to populate values in the EDX:EAX registers. After the vmmcall
; returns, it verifies that the response code matches with the expected
; code. If it does not match then terminate the guest. The result of request
; is returned in the EDX:EAX.
;
; args 1:Request code, 2: Response code
%macro VmgExit 2
    ;
    ; Add request code:
    ;   GHCB_MSR[11:0]  = Request code
    or      eax, %1

    mov     ecx, SEV_GHCB_MSR
    wrmsr

    ; Issue VMGEXIT - NASM doesn't support the vmmcall instruction in 32-bit
    ; mode, so work around this by temporarily switching to 64-bit mode.
    ;
BITS    64
    rep     vmmcall
BITS    32

    mov     ecx, SEV_GHCB_MSR
    rdmsr

    ;
    ; Verify the reponse code, if it does not match then request to terminate
    ;   GHCB_MSR[11:0]  = Response code
    mov     ecx, eax
    and     ecx, 0xfff
    cmp     ecx, %2
    jne     SevEsUnexpectedRespTerminate
%endmacro

; Macro to terminate the guest using the VMGEXIT.
; arg 1: reason code
%macro TerminateVmgExit 1
    mov     eax, %1
    ;
    ; Use VMGEXIT to request termination. At this point the reason code is
    ; located in EAX, so shift it left 16 bits to the proper location.
    ;
    ; EAX[11:0]  => 0x100 - request termination
    ; EAX[15:12] => 0x1   - OVMF
    ; EAX[23:16] => 0xXX  - REASON CODE
    ;
    shl     eax, 16
    or      eax, 0x1100
    xor     edx, edx
    mov     ecx, SEV_GHCB_MSR
    wrmsr
    ;
    ; Issue VMGEXIT - NASM doesn't support the vmmcall instruction in 32-bit
    ; mode, so work around this by temporarily switching to 64-bit mode.
    ;
BITS    64
    rep     vmmcall
BITS    32

    ;
    ; We shouldn't come back from the VMGEXIT, but if we do, just loop.
    ;
%%TerminateHlt:
    hlt
    jmp     %%TerminateHlt
%endmacro

; Terminate the guest due to unexpected response code.
SevEsUnexpectedRespTerminate:
    TerminateVmgExit    TERM_UNEXPECTED_RESP_CODE

%ifdef ARCH_X64

; If SEV-ES is enabled then initialize and make the GHCB page shared
SevClearPageEncMaskForGhcbPage:
    ; Check if SEV-ES is enabled
    mov       ecx, 1
    bt        [SEV_ES_WORK_AREA_STATUS_MSR], ecx
    jnc       SevClearPageEncMaskForGhcbPageExit

    ;
    ; The initial GHCB will live at GHCB_BASE and needs to be un-encrypted.
    ; This requires the 2MB page for this range be broken down into 512 4KB
    ; pages.  All will be marked encrypted, except for the GHCB. Since the
    ; original PMD entry is no longer a leaf entry, remove the encryption
    ; bit when pointing to the PTE page.
    ;
    mov     ecx, (GHCB_BASE >> 21)
    mov     eax, GHCB_PT_ADDR + PAGE_PDP_ATTR
    mov     [ecx * 8 + PT_ADDR (0x2000)], eax
    mov     [ecx * 8 + PT_ADDR (0x2000) + 4], strict dword 0

    ;
    ; Page Table Entries (512 * 4KB entries => 2MB)
    ;
    mov     ecx, 512
pageTableEntries4kLoop:
    mov     eax, ecx
    dec     eax
    shl     eax, 12
    add     eax, GHCB_BASE & 0xFFE0_0000
    add     eax, PAGE_4K_PDE_ATTR
    mov     [ecx * 8 + GHCB_PT_ADDR - 8], eax
    mov     [(ecx * 8 + GHCB_PT_ADDR - 8) + 4], edx
    loop    pageTableEntries4kLoop

    ;
    ; Clear the encryption bit from the GHCB entry
    ;
    mov     ecx, (GHCB_BASE & 0x1F_FFFF) >> 12
    mov     [ecx * 8 + GHCB_PT_ADDR + 4], strict dword 0

SevClearPageEncMaskForGhcbPageExit:
    OneTimeCallRet SevClearPageEncMaskForGhcbPage

; Get the C-bit mask above 31.
; Modified: EDX
;
; The value is returned in the EDX
GetSevCBitMaskAbove31:
    mov       edx, dword[SEV_ES_WORK_AREA_ENC_MASK + 4]
    OneTimeCallRet GetSevCBitMaskAbove31

%endif

; Check if Secure Encrypted Virtualization (SEV) features are enabled.
;
; Register usage is tight in this routine, so multiple calls for the
; same CPUID and MSR data are performed to keep things simple.
;
; Modified:  EAX, EBX, ECX, EDX, ESP
;
; If SEV is enabled then EAX will be at least 32.
; If SEV is disabled then EAX will be zero.
;
CheckSevFeatures:
    ;
    ; Clear the workarea, if SEV is enabled then later part of routine
    ; will populate the workarea fields.
    ;
    mov    ecx, SEV_ES_WORK_AREA_SIZE
    mov    eax, SEV_ES_WORK_AREA
ClearSevEsWorkArea:
    mov    byte [eax], 0
    inc    eax
    loop   ClearSevEsWorkArea

    ; Check if we have a valid (0x8000_001F) CPUID leaf
    ;   CPUID raises a #VC exception if running as an SEV-ES guest
    mov       eax, 0x80000000
    cpuid

    ; This check should fail on Intel or Non SEV AMD CPUs. In future if
    ; Intel CPUs supports this CPUID leaf then we are guranteed to have exact
    ; same bit definition.
    cmp       eax, 0x8000001f
    jl        NoSev

    ; Check for SEV memory encryption feature:
    ; CPUID  Fn8000_001F[EAX] - Bit 1
    ; Check for the COHERENCY_SFW_NO feature:
    ; CPUID  Fn8000_001F[EBX] - Bit 31
    ;   CPUID raises a #VC exception if running as an SEV-ES guest
    mov       eax, 0x8000001f
    cpuid

    ; If COHERENCY_SFW_NO is set, set the CSFW_NO bit in the FLAGS field
    ; of the workarea (this can be set regardless of whether SEV is enabled).
    bt        ebx, 31
    jnc       CheckSev
    or        byte[SEV_ES_WORK_AREA_FLAGS], SEV_ES_WORK_AREA_FLAG_CSFW_NO

CheckSev:
    bt        eax, 1
    jnc       NoSev

    ; Check if SEV memory encryption is enabled
    ;  MSR_0xC0010131 - Bit 0 (SEV enabled)
    mov       ecx, SEV_STATUS_MSR
    rdmsr
    bt        eax, 0
    jnc       NoSev

    ; Set the work area header to indicate that the SEV is enabled
    mov     byte[WORK_AREA_GUEST_TYPE], 1

    ; Save the SevStatus MSR value in the workarea
    mov     [SEV_ES_WORK_AREA_STATUS_MSR], eax
    mov     [SEV_ES_WORK_AREA_STATUS_MSR + 4], edx

    ; Check if SEV-ES is enabled
    ;  MSR_0xC0010131 - Bit 1 (SEV-ES enabled)
    mov       ecx, SEV_STATUS_MSR
    rdmsr
    bt        eax, 1
    jnc       GetSevEncBit

GetSevEncBit:
    ; Get pte bit position to enable memory encryption
    ; CPUID Fn8000_001F[EBX] - Bits 5:0
    ;
    and       ebx, 0x3f
    mov       eax, ebx

    ; The encryption bit position is always above 31
    sub       ebx, 32
    jns       SevSaveMask

    ; Encryption bit was reported as 31 or below, enter a HLT loop
SevEncBitLowHlt:
    cli
    hlt
    jmp       SevEncBitLowHlt

SevSaveMask:
    xor       edx, edx
    bts       edx, ebx

    mov       dword[SEV_ES_WORK_AREA_ENC_MASK], 0
    mov       dword[SEV_ES_WORK_AREA_ENC_MASK + 4], edx
    jmp       SevExit

NoSev:
    ;
    ; Perform an SEV-ES sanity check by seeing if a #VC exception occurred.
    ;
    ; If SEV-ES is enabled, the CPUID instruction will trigger a #VC exception
    ; where the VC bit in the FLAGS field in the workarea will be set to one.
    ;
    test      byte[SEV_ES_WORK_AREA_FLAGS], SEV_ES_WORK_AREA_FLAG_VC
    jz        NoSevPass

    ;
    ; A #VC was received, yet CPUID indicates no SEV-ES support, something
    ; isn't right.
    ;
NoSevEsVcHlt:
    cli
    hlt
    jmp       NoSevEsVcHlt

NoSevPass:
    xor       eax, eax

SevExit:
    OneTimeCallRet CheckSevFeatures

; Start of #VC exception handling routines
;

SevEsIdtNotCpuid:
    TerminateVmgExit TERM_VC_NOT_CPUID
    iret

; Use the SNP CPUID page to handle the cpuid lookup
;
;  Modified: EAX, EBX, ECX, EDX
;
;  Relies on the stack setup/usage in #VC handler:
;
;    On entry,
;      [esp + VC_CPUID_FUNCTION] contains EAX input to cpuid instruction
;
;    On return, stores corresponding results of CPUID lookup in:
;      [esp + VC_CPUID_RESULT_EAX]
;      [esp + VC_CPUID_RESULT_EBX]
;      [esp + VC_CPUID_RESULT_ECX]
;      [esp + VC_CPUID_RESULT_EDX]
;
SnpCpuidLookup:
    mov     eax, [esp + VC_CPUID_FUNCTION]
    mov     ebx, [CPUID_BASE + SNP_CPUID_COUNT]
    mov     ecx, CPUID_BASE + SNP_CPUID_ENTRY
    ; Zero these out now so we can simply return if lookup fails
    mov     dword[esp + VC_CPUID_RESULT_EAX], 0
    mov     dword[esp + VC_CPUID_RESULT_EBX], 0
    mov     dword[esp + VC_CPUID_RESULT_ECX], 0
    mov     dword[esp + VC_CPUID_RESULT_EDX], 0

SnpCpuidCheckEntry:
    cmp     ebx, 0
    je      VmmDoneSnpCpuid
    cmp     dword[ecx + SNP_CPUID_ENTRY_EAX_IN], eax
    jne     SnpCpuidCheckEntryNext
    ; As with SEV-ES handler we assume requested CPUID sub-leaf/index is 0
    cmp     dword[ecx + SNP_CPUID_ENTRY_ECX_IN], 0
    je      SnpCpuidEntryFound

SnpCpuidCheckEntryNext:
    dec     ebx
    add     ecx, SNP_CPUID_ENTRY_SZ
    jmp     SnpCpuidCheckEntry

SnpCpuidEntryFound:
    mov     eax, [ecx + SNP_CPUID_ENTRY_EAX]
    mov     [esp + VC_CPUID_RESULT_EAX], eax
    mov     eax, [ecx + SNP_CPUID_ENTRY_EBX]
    mov     [esp + VC_CPUID_RESULT_EBX], eax
    mov     eax, [ecx + SNP_CPUID_ENTRY_ECX]
    mov     [esp + VC_CPUID_RESULT_ECX], eax
    mov     eax, [ecx + SNP_CPUID_ENTRY_EDX]
    mov     [esp + VC_CPUID_RESULT_EDX], eax
    jmp     VmmDoneSnpCpuid

;
; Total stack usage for the #VC handler is 44 bytes:
;   - 12 bytes for the exception IRET (after popping error code)
;   - 32 bytes for the local variables.
;
SevEsIdtVmmComm:
    ;
    ; If we're here, then we are an SEV-ES guest and this
    ; was triggered by a CPUID instruction
    ;
    ; Set the VC bit in the FLAGS field in the workarea to communicate
    ; that a #VC was taken.
    or      byte[SEV_ES_WORK_AREA_FLAGS], SEV_ES_WORK_AREA_FLAG_VC

    pop     ecx                     ; Error code
    cmp     ecx, 0x72               ; Be sure it was CPUID
    jne     SevEsIdtNotCpuid

    ; Set up local variable room on the stack
    ;   CPUID function         : + 28
    ;   CPUID request register : + 24
    ;   GHCB MSR (EAX)         : + 20
    ;   GHCB MSR (EDX)         : + 16
    ;   CPUID result (EDX)     : + 12
    ;   CPUID result (ECX)     : + 8
    ;   CPUID result (EBX)     : + 4
    ;   CPUID result (EAX)     : + 0
    sub     esp, VC_VARIABLE_SIZE

    ; Save the CPUID function being requested
    mov     [esp + VC_CPUID_FUNCTION], eax

    ; If SEV-SNP is enabled, use the CPUID page to handle the CPUID
    ; instruction.
    mov     ecx, SEV_STATUS_MSR
    rdmsr
    bt      eax, 2
    jc      SnpCpuidLookup

    ; The GHCB CPUID protocol uses the following mapping to request
    ; a specific register:
    ;   0 => EAX, 1 => EBX, 2 => ECX, 3 => EDX
    ;
    ; Set EAX as the first register to request. This will also be used as a
    ; loop variable to request all register values (EAX to EDX).
    xor     eax, eax
    mov     [esp + VC_CPUID_REQUEST_REGISTER], eax

    ; Save current GHCB MSR value
    mov     ecx, SEV_GHCB_MSR
    rdmsr
    mov     [esp + VC_GHCB_MSR_EAX], eax
    mov     [esp + VC_GHCB_MSR_EDX], edx

NextReg:
    ;
    ; Setup GHCB MSR
    ;   GHCB_MSR[63:32] = CPUID function
    ;   GHCB_MSR[31:30] = CPUID register
    ;   GHCB_MSR[11:0]  = CPUID request protocol
    ;
    mov     eax, [esp + VC_CPUID_REQUEST_REGISTER]
    cmp     eax, 4
    jge     VmmDone

    shl     eax, GHCB_CPUID_REGISTER_SHIFT
    mov     edx, [esp + VC_CPUID_FUNCTION]

    VmgExit GHCB_CPUID_REQUEST, GHCB_CPUID_RESPONSE

    ;
    ; Response GHCB MSR
    ;   GHCB_MSR[63:32] = CPUID register value
    ;   GHCB_MSR[31:30] = CPUID register
    ;   GHCB_MSR[11:0]  = CPUID response protocol
    ;

    ; Save returned value
    shr     eax, GHCB_CPUID_REGISTER_SHIFT
    mov     [esp + eax * 4], edx

    ; Next register
    inc     word [esp + VC_CPUID_REQUEST_REGISTER]

    jmp     NextReg

VmmDone:
    ;
    ; At this point we have all CPUID register values. Restore the GHCB MSR,
    ; set the return register values and return.
    ;
    mov     eax, [esp + VC_GHCB_MSR_EAX]
    mov     edx, [esp + VC_GHCB_MSR_EDX]
    mov     ecx, SEV_GHCB_MSR
    wrmsr

VmmDoneSnpCpuid:
    mov     eax, [esp + VC_CPUID_RESULT_EAX]
    mov     ebx, [esp + VC_CPUID_RESULT_EBX]
    mov     ecx, [esp + VC_CPUID_RESULT_ECX]
    mov     edx, [esp + VC_CPUID_RESULT_EDX]

    add     esp, VC_VARIABLE_SIZE

    ; Update the EIP value to skip over the now handled CPUID instruction
    ; (the CPUID instruction has a length of 2)
    add     word [esp], CPUID_INSN_LEN
    iret

%ifdef ARCH_X64

SevCpuidInit:
    ;
    ; Set up exception handlers to check for SEV-ES
    ;   Load temporary RAM stack based on PCDs (see SevEsIdtVmmComm for
    ;   stack usage)
    ;   Establish exception handlers
    ;
    mov       esp, SEV_ES_VC_TOP_OF_STACK
    mov       eax, ADDR_OF(Idtr)
    lidt      [cs:eax]

    OneTimeCallRet SevCpuidInit

SevCpuidExit:
    ;
    ; Clear exception handlers and stack
    ;
    mov       eax, ADDR_OF(IdtrClear)
    lidt      [cs:eax]
    xor       eax, eax
    mov       esp, eax

    OneTimeCallRet SevCpuidExit

%endif

ALIGN   2

Idtr:
    dw      IDT_END - IDT_BASE - 1  ; Limit
    dd      ADDR_OF(IDT_BASE)       ; Base

IdtrClear:
    dw      0                       ; Limit
    dd      0                       ; Base

ALIGN   16

;
; The Interrupt Descriptor Table (IDT)
;   This will be used to determine if SEV-ES is enabled.  Upon execution
;   of the CPUID instruction, a VMM Communication Exception will occur.
;   This will tell us if SEV-ES is enabled.  We can use the current value
;   of the GHCB MSR to determine the SEV attributes.
;
IDT_BASE:
;
; Vectors 0 - 28 (No handlers)
;
%rep 29
    dw      0                                    ; Offset low bits 15..0
    dw      0x10                                 ; Selector
    db      0                                    ; Reserved
    db      0x8E                                 ; Gate Type (IA32_IDT_GATE_TYPE_INTERRUPT_32)
    dw      0                                    ; Offset high bits 31..16
%endrep
;
; Vector 29 (VMM Communication Exception)
;
    dw      (ADDR_OF(SevEsIdtVmmComm) & 0xffff)  ; Offset low bits 15..0
    dw      0x10                                 ; Selector
    db      0                                    ; Reserved
    db      0x8E                                 ; Gate Type (IA32_IDT_GATE_TYPE_INTERRUPT_32)
    dw      (ADDR_OF(SevEsIdtVmmComm) >> 16)     ; Offset high bits 31..16
;
; Vectors 30 - 31 (No handlers)
;
%rep 2
    dw      0                                    ; Offset low bits 15..0
    dw      0x10                                 ; Selector
    db      0                                    ; Reserved
    db      0x8E                                 ; Gate Type (IA32_IDT_GATE_TYPE_INTERRUPT_32)
    dw      0                                    ; Offset high bits 31..16
%endrep
IDT_END:

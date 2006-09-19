      TITLE   LongMode.asm: Assembly code for the entering long mode

;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2006, Intel Corporation                                                         
;*   All rights reserved. This program and the accompanying materials                          
;*   are licensed and made available under the terms and conditions of the BSD License         
;*   which accompanies this distribution.  The full text of the license may be found at        
;*   http://opensource.org/licenses/bsd-license.php                                            
;*                                                                                             
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;*   
;*    LongMode.asm
;*  
;*   Abstract:
;*
;*    Transition from 32-bit protected mode EFI environment into x64 
;*    64-bit bit long mode.
;*  
;------------------------------------------------------------------------------

.686p
.model  flat        

;
; Create the exception handler code in IA32 C code
;

.code
.stack
.MMX
.XMM

_LoadGo64Gdt	PROC Near Public
    push    ebp               ; C prolog
    push    edi
    mov     ebp, esp
    ;
    ; Disable interrupts
    ;
    cli
    ;
    ; Reload the selectors
    ; Note:
    ;      Make the Selectors 64-bit ready
    ;
    mov     edi, OFFSET gdtr    ; Load GDT register
    mov     ax,cs               ; Get the selector data from our code image          
    mov     es,ax
    lgdt    FWORD PTR es:[edi]  ; and update the GDTR   

    db      067h
    db      0eah              ; Far Jump Offset:Selector to reload CS
    dd      OFFSET DataSelectorRld;   Offset is ensuing instruction boundary
    dw      LINEAR_CODE_SEL   ;   Selector is our code selector, 10h
DataSelectorRld::
    mov     ax, SYS_DATA_SEL ; Update the Base for the new selectors, too
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax  
    
    pop     edi
    pop     ebp
    ret
_LoadGo64Gdt endp   
    

; VOID
; ActivateLongMode (
;   IN  EFI_PHYSICAL_ADDRESS  PageTables,  
;   IN  EFI_PHYSICAL_ADDRESS  HobStart,
;   IN  EFI_PHYSICAL_ADDRESS  Stack,
;   IN  EFI_PHYSICAL_ADDRESS  PpisNeededByDxeIplEntryPoint,
;   IN  EFI_PHYSICAL_ADDRESS  DxeCoreEntryPoint
;   )
;
; Input:  [ebp][0h]  = Original ebp
;         [ebp][4h]  = Return address
;         [ebp][8h]  = PageTables
;         [ebp][10h] = HobStart
;         [ebp][18h] = Stack
;         [ebp][20h] = CodeEntryPoint1 <--- Call this first (for each call, pass HOB pointer)
;         [ebp][28h] = CodeEntryPoint2 <--- Call this second
;
;
_ActivateLongMode  PROC Near Public
    push    ebp               ; C prolog
    mov     ebp, esp

    ;
    ; Use CPUID to determine if the processor supports long mode.
    ;
    mov     eax, 80000000h  ; Extended-function code 8000000h.
    cpuid                   ; Is largest extended function
    cmp     eax, 80000000h  ; any function > 80000000h?
    jbe     no_long_mode    ; If not, no long mode.
    mov     eax, 80000001h  ; Extended-function code 8000001h.
    cpuid                   ; Now EDX = extended-features flags.
    bt      edx, 29         ; Test if long mode is supported.
    jnc     no_long_mode    ; Exit if not supported.

    ;
    ; Enable the 64-bit page-translation-table entries by
    ; setting CR4.PAE=1 (this is _required_ before activating
    ; long mode). Paging is not enabled until after long mode
    ; is enabled.
    ;
    mov eax, cr4
    bts eax, 5
    mov cr4, eax

    ;
    ; Get the long-mode page tables, and initialize the
    ; 64-bit CR3 (page-table base address) to point to the base
    ; of the PML4 page table. The PML4 page table must be located
    ; below 4 Gbytes because only 32 bits of CR3 are loaded when
    ; the processor is not in 64-bit mode.
    ;
    mov eax, [ebp+8h]       ; Get Page Tables
    mov cr3, eax            ; Initialize CR3 with PML4 base.

    ;
    ; Enable long mode (set EFER.LME=1).
    ;
    mov ecx, 0c0000080h ; EFER MSR number.
    rdmsr               ; Read EFER.
    bts eax, 8          ; Set LME=1.
    wrmsr               ; Write EFER.

    ;
    ; Enable paging to activate long mode (set CR0.PG=1)
    ;
   
   
    mov eax, cr0 ; Read CR0.
    bts eax, 31  ; Set PG=1.
    mov cr0, eax ; Write CR0.
    jmp   go_to_long_mode
go_to_long_mode:

    ;
    ; This is the next instruction after enabling paging.  Jump to long mode
    ;
    db      067h
    db      0eah              ; Far Jump Offset:Selector to reload CS
    dd      OFFSET in_long_mode;   Offset is ensuing instruction boundary
    dw      SYS_CODE64_SEL    ;   Selector is our code selector, 10h
in_long_mode::
    mov     ax, SYS_DATA64_SEL
    mov     es, ax
    mov     ss, ax
    mov     ds, ax
;;    jmp     $
    
           
    ;
    ; We're in long mode, so marshall the arguments to call the
    ; passed in function pointers
    ; Recall
    ;         [ebp][10h] = HobStart
    ;         [ebp][18h] = Stack
    ;         [ebp][20h] = PpisNeededByDxeIplEntryPoint <--- Call this first (for each call, pass HOB pointer)
    ;         [ebp][28h] = DxeCoreEntryPoint            <--- Call this second
    ;
    db  48h
    mov ebx, [ebp+18h]        ; Setup the stack
    db  48h
    mov esp, ebx              ; On a new stack now


;; 00000905  FF D0		    call rax

    db  48h
    mov ecx, [ebp+10h]        ; Pass Hob Start in RCX
    db  48h
    mov eax, [ebp+28h]        ; Get the function pointer for 
                              ; DxeCoreEntryPoint into EAX

;; 00000905  FF D0		    call rax
    db 0ffh
    db 0d0h

    ;
    ; WE SHOULD NEVER GET HERE!!!!!!!!!!!!!
    ;
no_long_mode:
    jmp   no_long_mode
_ActivateLongMode endp

        align 16

gdtr    dw GDT_END - GDT_BASE - 1   ; GDT limit
        dd OFFSET GDT_BASE          ; (GDT base gets set above)

;-----------------------------------------------------------------------------;
;   global descriptor table (GDT)
;-----------------------------------------------------------------------------;

        align 16

public GDT_BASE
GDT_BASE:
; null descriptor
NULL_SEL            equ $-GDT_BASE    ; Selector [0]
        dw 0            ; limit 15:0
        dw 0            ; base 15:0
        db 0            ; base 23:16
        db 0            ; type
        db 0            ; limit 19:16, flags
        db 0            ; base 31:24

; linear data segment descriptor
LINEAR_SEL      equ $-GDT_BASE        ; Selector [0x8]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 092h         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; linear code segment descriptor
LINEAR_CODE_SEL equ $-GDT_BASE        ; Selector [0x10]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 09Fh         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; system data segment descriptor
SYS_DATA_SEL    equ $-GDT_BASE        ; Selector [0x18]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 093h         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; system code segment descriptor
SYS_CODE_SEL    equ $-GDT_BASE        ; Selector [0x20]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 09Ah         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; spare segment descriptor
SPARE3_SEL  equ $-GDT_BASE            ; Selector [0x28]
        dw 0            ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 0            ; present, ring 0, data, expand-up, writable
        db 0            ; page-granular, 32-bit
        db 0

;
; system data segment descriptor
;
SYS_DATA64_SEL    equ $-GDT_BASE          ; Selector [0x30]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 092h         ; P | DPL [1..2] | 1   | 1   | C | R | A
        db 0CFh         ; G | D   | L    | AVL | Segment [19..16]
        db 0

;
; system code segment descriptor
;
SYS_CODE64_SEL    equ $-GDT_BASE          ; Selector [0x38]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 09Ah         ; P | DPL [1..2] | 1   | 1   | C | R | A
        db 0AFh         ; G | D   | L    | AVL | Segment [19..16]
        db 0

; spare segment descriptor
SPARE4_SEL  equ $-GDT_BASE            ; Selector [0x40]
        dw 0            ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 0            ; present, ring 0, data, expand-up, writable
        db 0            ; page-granular, 32-bit
        db 0

GDT_END:

END


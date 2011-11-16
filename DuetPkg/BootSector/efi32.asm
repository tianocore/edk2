;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
;*   This program and the accompanying materials                          
;*   are licensed and made available under the terms and conditions of the BSD License         
;*   which accompanies this distribution.  The full text of the license may be found at        
;*   http://opensource.org/licenses/bsd-license.php                                            
;*                                                                                             
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;*   
;*    efi32.asm
;*  
;*   Abstract:
;*
;------------------------------------------------------------------------------

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Now in 32-bit protected mode.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        .486
        .model  flat        
        .stack
        .code
        org 21000h
        
DEFAULT_HANDLER_SIZE EQU INT1 - INT0

JmpCommonIdtEntry  macro
    ; jmp     commonIdtEntry - this must be hand coded to keep the assembler from
    ;                          using a 8 bit reletive jump when the entries are
    ;                          within 255 bytes of the common entry.  This must
    ;                          be done to maintain the consistency of the size
    ;                          of entry points...
    db      0e9h                        ; jmp 16 bit relative 
    dd      commonIdtEntry - $ - 4      ;  offset to jump to
endm    

        
Start:  
    mov     ax,bx                      ; flat data descriptor in BX
    mov     ds,ax
    mov     es,ax
    mov     fs,ax
    mov     gs,ax
    mov     ss,ax
    mov     esp,0001ffff0h

    call    ClearScreen

    ; Populate IDT with meaningful offsets for exception handlers...
    sidt    fword ptr [Idtr]             ; get fword address of IDT

    mov     eax, offset Halt
    mov     ebx, eax                    ; use bx to copy 15..0 to descriptors
    shr     eax, 16                     ; use ax to copy 31..16 to descriptors 
    mov     ecx, 78h                    ; 78h IDT entries to initialize with unique entry points (exceptions)
    mov     esi, [offset Idtr + 2]
    mov     edi, [esi]
    
@@:                                             ; loop through all IDT entries exception handlers and initialize to default handler
    mov     word ptr [edi], bx                  ; write bits 15..0 of offset
    mov     word ptr [edi+2], 20h               ; SYS_CODE_SEL from GDT
    mov     word ptr [edi+4], 0e00h OR 8000h    ; type = 386 interrupt gate, present
    mov     word ptr [edi+6], ax                ; write bits 31..16 of offset
    add     edi, 8                              ; move up to next descriptor
    add     bx, DEFAULT_HANDLER_SIZE            ; move to next entry point
    loop    @b                                  ; loop back through again until all descriptors are initialized
    
    ;; at this point edi contains the offset of the descriptor for INT 20
    ;; and bx contains the low 16 bits of the offset of the default handler
    ;; so initialize all the rest of the descriptors with these two values...
;    mov     ecx, 101                            ; there are 100 descriptors left (INT 20 (14h) - INT 119 (77h)
;@@:                                             ; loop through all IDT entries exception handlers and initialize to default handler
;    mov     word ptr [edi], bx                  ; write bits 15..0 of offset
;    mov     word ptr [edi+2], 20h               ; SYS_CODE_SEL from GDT
;    mov     word ptr [edi+4], 0e00h OR 8000h    ; type = 386 interrupt gate, present
;    mov     word ptr [edi+6], ax                ; write bits 31..16 of offset
;    add     edi, 8                              ; move up to next descriptor
;    loop    @b                                  ; loop back through again until all descriptors are initialized
    
    
;;  DUMP    location of IDT and several of the descriptors
;    mov     ecx, 8
;    mov     eax, [offset Idtr + 2]
;    mov     eax, [eax]
;    mov     edi, 0b8000h
;    call    PrintDword
;    mov     esi, eax
;    mov     edi, 0b80a0h
;    jmp     OuterLoop
    
;;    
;; just for fun, let's do a software interrupt to see if we correctly land in the exception handler...
;    mov     eax, 011111111h
;    mov     ebx, 022222222h
;    mov     ecx, 033333333h
;    mov     edx, 044444444h
;    mov     ebp, 055555555h
;    mov     esi, 066666666h
;    mov     edi, 077777777h
;    push    011111111h
;    push    022222222h
;    push    033333333h
;    int     119

    
    mov     esi,022000h                 ; esi = 22000
    mov     eax,[esi+014h]              ; eax = [22014]
    add     esi,eax                     ; esi = 22000 + [22014] = Base of EFILDR.C
    mov     ebp,[esi+03ch]              ; ebp = [22000 + [22014] + 3c] = NT Image Header for EFILDR.C
    add     ebp,esi
    mov     edi,[ebp+034h]              ; edi = [[22000 + [22014] + 3c] + 30] = ImageBase
    mov     eax,[ebp+028h]              ; eax = [[22000 + [22014] + 3c] + 24] = EntryPoint
    add     eax,edi                     ; eax = ImageBase + EntryPoint
    mov     dword ptr [EfiLdrOffset],eax   ; Modify far jump instruction for correct entry point

    mov     bx,word ptr[ebp+6]          ; bx = Number of sections
    xor     eax,eax
    mov     ax,word ptr[ebp+014h]       ; ax = Optional Header Size
    add     ebp,eax
    add     ebp,018h                    ; ebp = Start of 1st Section

SectionLoop:
    push    esi                         ; Save Base of EFILDR.C
    push    edi                         ; Save ImageBase
    add     esi,[ebp+014h]              ; esi = Base of EFILDR.C + PointerToRawData
    add     edi,[ebp+00ch]              ; edi = ImageBase + VirtualAddress
    mov     ecx,[ebp+010h]              ; ecs = SizeOfRawData

    cld
    shr     ecx,2
    rep     movsd

    pop     edi                         ; Restore ImageBase
    pop     esi                         ; Restore Base of EFILDR.C

    add     bp,028h                     ; ebp = ebp + 028h = Pointer to next section record
    dec     bx
    cmp     bx,0
    jne     SectionLoop

    movzx   eax, word ptr [Idtr]         ; get size of IDT
    inc     eax
    add     eax, dword ptr [Idtr + 2]    ; add to base of IDT to get location of memory map...
    push    eax                         ; push memory map location on stack for call to EFILDR...

    push    eax                         ; push return address (useless, just for stack balance)
    db      0b8h
EfiLdrOffset:
    dd      000401000h                  ; Offset of EFILDR
; mov eax, 401000h
    push    eax
    ret

;    db      "**** DEFAULT IDT ENTRY ***",0
    align 02h
Halt:
INT0:
    push    0h      ; push error code place holder on the stack
    push    0h
    JmpCommonIdtEntry
;    db      0e9h                        ; jmp 16 bit reletive 
;    dd      commonIdtEntry - $ - 4      ;  offset to jump to
    
INT1:
    push    0h      ; push error code place holder on the stack
    push    1h
    JmpCommonIdtEntry
    
INT2:
    push    0h      ; push error code place holder on the stack
    push    2h
    JmpCommonIdtEntry
    
INT3:
    push    0h      ; push error code place holder on the stack
    push    3h
    JmpCommonIdtEntry
    
INT4:
    push    0h      ; push error code place holder on the stack
    push    4h
    JmpCommonIdtEntry
    
INT5:
    push    0h      ; push error code place holder on the stack
    push    5h
    JmpCommonIdtEntry
    
INT6:
    push    0h      ; push error code place holder on the stack
    push    6h
    JmpCommonIdtEntry
    
INT7:
    push    0h      ; push error code place holder on the stack
    push    7h
    JmpCommonIdtEntry
    
INT8:
;   Double fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    8h
    JmpCommonIdtEntry
    
INT9:
    push    0h      ; push error code place holder on the stack
    push    9h
    JmpCommonIdtEntry
    
INT10:
;   Invalid TSS causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    10
    JmpCommonIdtEntry
    
INT11:
;   Segment Not Present causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    11
    JmpCommonIdtEntry
    
INT12:
;   Stack fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    12
    JmpCommonIdtEntry
    
INT13:
;   GP fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    13
    JmpCommonIdtEntry
    
INT14:
;   Page fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    14
    JmpCommonIdtEntry
    
INT15:
    push    0h      ; push error code place holder on the stack
    push    15
    JmpCommonIdtEntry
    
INT16:
    push    0h      ; push error code place holder on the stack
    push    16
    JmpCommonIdtEntry
    
INT17:
;   Alignment check causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    17
    JmpCommonIdtEntry
    
INT18:
    push    0h      ; push error code place holder on the stack
    push    18
    JmpCommonIdtEntry
    
INT19:
    push    0h      ; push error code place holder on the stack
    push    19
    JmpCommonIdtEntry

INTUnknown:
REPEAT  (78h - 20)
    push    0h      ; push error code place holder on the stack
;    push    xxh     ; push vector number
    db      06ah
    db      ( $ - INTUnknown - 3 ) / 9 + 20 ; vector number
    JmpCommonIdtEntry
ENDM

commonIdtEntry:
    pushad
    mov     ebp, esp
;;
;;  At this point the stack looks like this:
;;
;;      eflags
;;      Calling CS
;;      Calling EIP
;;      Error code or 0
;;      Int num or 0ffh for unknown int num
;;      eax
;;      ecx
;;      edx
;;      ebx
;;      esp
;;      ebp
;;      esi
;;      edi <------- ESP, EBP
;;      

    call    ClearScreen
    mov     esi, offset String1
    call    PrintString
    mov     eax, [ebp + 32]     ;; move Int number into EAX 
    cmp     eax, 19
    ja      PrintDefaultString
PrintExceptionString:
    shl     eax, 2              ;; multiply by 4 to get offset from StringTable to actual string address
    add     eax, offset StringTable
    mov     esi, [eax]
    jmp     PrintTheString
PrintDefaultString:
    mov     esi, offset IntUnknownString
    ; patch Int number
    mov     edx, eax
    call    A2C
    mov     [esi + 1], al
    mov     eax, edx
    shr     eax, 4
    call    A2C
    mov     [esi], al
PrintTheString:        
    call    PrintString
    mov     esi, offset String2
    call    PrintString
    mov     eax, [ebp+44]          ; CS
    call    PrintDword
    mov     al, ':'
    mov     byte ptr [edi], al
    add     edi, 2
    mov     eax, [ebp+40]          ; EIP
    call    PrintDword
    mov     esi, offset String3
    call    PrintString
    
    mov     edi, 0b8140h
    
    mov     esi, offset StringEax     ; eax
    call    PrintString
    mov     eax, [ebp+28]
    call    PrintDword
    
    mov     esi, offset StringEbx     ; ebx
    call    PrintString
    mov     eax, [ebp+16]
    call    PrintDword
    
    mov     esi, offset StringEcx     ; ecx
    call    PrintString
    mov     eax, [ebp+24]
    call    PrintDword
    
    mov     esi, offset StringEdx     ; edx
    call    PrintString
    mov     eax, [ebp+20]
    call    PrintDword
    
    mov     esi, offset StringEcode   ; error code
    call    PrintString
    mov     eax, [ebp+36]
    call    PrintDword
    
    mov     edi, 0b81e0h
    
    mov     esi, offset StringEsp     ; esp
    call    PrintString
    mov     eax, [ebp+12]
    call    PrintDword
    
    mov     esi, offset StringEbp     ; ebp
    call    PrintString
    mov     eax, [ebp+8]
    call    PrintDword
    
    mov     esi, offset StringEsi     ; esi
    call    PrintString
    mov     eax, [ebp+4]
    call    PrintDword
    
    mov     esi, offset StringEdi    ; edi
    call    PrintString
    mov     eax, [ebp]
    call    PrintDword
    
    mov     esi, offset StringEflags ; eflags
    call    PrintString
    mov     eax, [ebp+48]
    call    PrintDword
    
    mov     edi, 0b8320h

    mov     esi, ebp
    add     esi, 52
    mov     ecx, 8

    
OuterLoop:
    push    ecx
    mov     ecx, 8
    mov     edx, edi

InnerLoop:
    mov     eax, [esi]
    call    PrintDword
    add     esi, 4
    mov     al, ' '
    mov     [edi], al
    add     edi, 2
    loop    InnerLoop

    pop     ecx
    add     edx, 0a0h
    mov     edi, edx
    loop    OuterLoop


    mov     edi, 0b8960h

    mov     eax, [ebp+40]  ; EIP
    sub     eax, 32 * 4
    mov     esi, eax        ; esi = eip - 32 DWORD linear (total 64 DWORD)

    mov     ecx, 8
    
OuterLoop1:
    push    ecx
    mov     ecx, 8
    mov     edx, edi

InnerLoop1:
    mov     eax, [esi]
    call    PrintDword
    add     esi, 4
    mov     al, ' '
    mov     [edi], al
    add     edi, 2
    loop    InnerLoop1

    pop     ecx
    add     edx, 0a0h
    mov     edi, edx
    loop    OuterLoop1



;    wbinvd ; Ken: this intruction does not support in early than 486 arch
@@:    
    jmp     @b
;
; return
;
    mov     esp, ebp
    popad
    add     esp, 8 ; error code and INT number
    
    iretd


PrintString:
    push    eax
@@:
    mov     al, byte ptr [esi]
    cmp     al, 0
    je      @f
    mov     byte ptr [edi], al
    inc     esi
    add     edi, 2
    jmp     @b
@@:
    pop     eax
    ret
        
;; EAX contains dword to print
;; EDI contains memory location (screen location) to print it to
PrintDword:
    push    ecx
    push    ebx
    push    eax
    
    mov     ecx, 8
looptop:
    rol     eax, 4
    mov     bl, al
    and     bl, 0fh
    add     bl, '0'
    cmp     bl, '9'
    jle     @f
    add     bl, 7
@@:
    mov     byte ptr [edi], bl
    add     edi, 2
    loop    looptop
    ;wbinvd
    
    pop     eax
    pop     ebx
    pop     ecx
    ret

ClearScreen:
    push    eax
    push    ecx
    
    mov     al, ' '
    mov     ah, 0ch
    mov     edi, 0b8000h
    mov     ecx, 80 * 24
@@:
    mov     word ptr [edi], ax
    add     edi, 2
    loop    @b
    mov     edi, 0b8000h
    
    pop     ecx
    pop     eax

    ret                
        
A2C:
    and     al, 0fh
    add     al, '0'
    cmp     al, '9'
    jle     @f
    add     al, 7
@@:
    ret
        
String1           db  "*** INT ",0

Int0String        db  "00h Divide by 0 -",0
Int1String        db  "01h Debug exception -",0
Int2String        db  "02h NMI -",0
Int3String        db  "03h Breakpoint -",0
Int4String        db  "04h Overflow -",0
Int5String        db  "05h Bound -",0
Int6String        db  "06h Invalid opcode -",0
Int7String        db  "07h Device not available -",0
Int8String        db  "08h Double fault -",0
Int9String        db  "09h Coprocessor seg overrun (reserved) -",0
Int10String       db  "0Ah Invalid TSS -",0
Int11String       db  "0Bh Segment not present -",0
Int12String       db  "0Ch Stack fault -",0
Int13String       db  "0Dh General protection fault -",0
Int14String       db  "0Eh Page fault -",0
Int15String       db  "0Fh (Intel reserved) -",0
Int16String       db  "10h Floating point error -",0
Int17String       db  "11h Alignment check -",0
Int18String       db  "12h Machine check -",0
Int19String       db  "13h SIMD Floating-Point Exception -",0
IntUnknownString  db  "??h Unknown interrupt -",0

StringTable       dd  offset Int0String, offset Int1String, offset Int2String, offset Int3String, 
                      offset Int4String, offset Int5String, offset Int6String, offset Int7String,
                      offset Int8String, offset Int9String, offset Int10String, offset Int11String,
                      offset Int12String, offset Int13String, offset Int14String, offset Int15String,
                      offset Int16String, offset Int17String, offset Int18String, offset Int19String

String2           db  " HALT!! *** (",0
String3           db  ")",0
StringEax         db  "EAX=",0
StringEbx         db  " EBX=",0
StringEcx         db  " ECX=",0
StringEdx         db  " EDX=",0
StringEcode       db  " ECODE=",0
StringEsp         db  "ESP=",0
StringEbp         db  " EBP=",0
StringEsi         db  " ESI=",0
StringEdi         db  " EDI=",0
StringEflags      db  " EFLAGS=",0

Idtr        df  0

    org 21ffeh
BlockSignature:
    dw      0aa55h
    
    end

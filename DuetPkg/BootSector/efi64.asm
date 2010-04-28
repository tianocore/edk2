;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
;*   This program and the accompanying materials                          
;*   are licensed and made available under the terms and conditions of the BSD License         
;*   which accompanies this distribution.  The full text of the license may be found at        
;*   http://opensource.org/licenses/bsd-license.php                                            
;*                                                                                             
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;*   
;*    efi64.asm
;*  
;*   Abstract:
;*
;------------------------------------------------------------------------------

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Now in 64-bit long mode.
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
    db      0e9h                        ; jmp 16 bit reletive 
    dd      commonIdtEntry - $ - 4      ;  offset to jump to
endm    

        
Start:  

    mov     esp,0001fffe8h ; make final stack aligned

    ; set OSFXSR and OSXMMEXCPT because some code will use XMM register
    db 0fh
    db 20h
    db 0e0h
;    mov rax, cr4
    bts eax, 9
    bts eax, 0ah
    db 0fh
    db 22h
    db 0e0h
;    mov cr4, rax

    call    ClearScreen

    ; Populate IDT with meaningful offsets for exception handlers...
    mov     eax, offset Idtr
    sidt    fword ptr [eax]             ; get fword address of IDT

    mov     eax, offset Halt
    mov     ebx, eax                    ; use bx to copy 15..0 to descriptors
    shr     eax, 16                     ; use ax to copy 31..16 to descriptors 
                                        ; 63..32 of descriptors is 0
    mov     ecx, 78h                    ; 78h IDT entries to initialize with unique entry points (exceptions)
    mov     esi, [offset Idtr + 2]
    mov     edi, [esi]

@@:                                             ; loop through all IDT entries exception handlers and initialize to default handler
    mov     word ptr [edi], bx                  ; write bits 15..0 of offset
    mov     word ptr [edi+2], 38h               ; SYS_CODE64_SEL from GDT
    mov     word ptr [edi+4], 0e00h OR 8000h    ; type = 386 interrupt gate, present
    mov     word ptr [edi+6], ax                ; write bits 31..16 of offset
    mov     dword ptr [edi+8], 0                ; write bits 63..32 of offset
    add     edi, 16                             ; move up to next descriptor
    add     bx, DEFAULT_HANDLER_SIZE            ; move to next entry point
    loop    @b                                  ; loop back through again until all descriptors are initialized
    
    ;; at this point edi contains the offset of the descriptor for INT 20
    ;; and bx contains the low 16 bits of the offset of the default handler
    ;; so initialize all the rest of the descriptors with these two values...
;    mov     ecx, 101                            ; there are 100 descriptors left (INT 20 (14h) - INT 119 (77h)
;@@:                                             ; loop through all IDT entries exception handlers and initialize to default handler
;    mov     word ptr [edi], bx                  ; write bits 15..0 of offset
;    mov     word ptr [edi+2], 38h               ; SYS_CODE64_SEL from GDT
;    mov     word ptr [edi+4], 0e00h OR 8000h    ; type = 386 interrupt gate, present
;    mov     word ptr [edi+6], ax                ; write bits 31..16 of offset
;    mov     dword ptr [edi+8], 0                ; write bits 63..32 of offset
;    add     edi, 16                             ; move up to next descriptor
;    loop    @b                                  ; loop back through again until all descriptors are initialized
    
    
;;  DUMP    location of IDT and several of the descriptors
;    mov     ecx, 8
;    mov     eax, [offset Idtr + 2]
;    mov     eax, [eax]
;    mov     edi, 0b8000h
;    call    PrintQword
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
    mov     edi,[ebp+030h]              ; edi = [[22000 + [22014] + 3c] + 2c] = ImageBase (63..32 is zero, ignore)
    mov     eax,[ebp+028h]              ; eax = [[22000 + [22014] + 3c] + 24] = EntryPoint
    add     eax,edi                     ; eax = ImageBase + EntryPoint
    mov     ebx, offset EfiLdrOffset
    mov     dword ptr [ebx],eax         ; Modify far jump instruction for correct entry point

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
    db 66h
    db 0ffh
    db 0cbh
;    dec     bx
    cmp     bx,0
    jne     SectionLoop

    mov     edx, offset Idtr
    movzx   eax, word ptr [edx]          ; get size of IDT
    db 0ffh
    db 0c0h
;    inc     eax
    add     eax, dword ptr [edx + 2]     ; add to base of IDT to get location of memory map...
    xor     ecx, ecx
    mov     ecx, eax                     ; put argument to RCX

    db 48h
    db 0c7h
    db 0c0h
EfiLdrOffset:
    dd      000401000h                  ; Offset of EFILDR
;   mov rax, 401000h
    db 50h
;   push rax

; ret
    db 0c3h

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
    push    eax
    push    ecx
    push    edx
    push    ebx
    push    esp
    push    ebp
    push    esi
    push    edi
    db 41h
    db 50h
;    push    r8
    db 41h
    db 51h
;    push    r9
    db 41h
    db 52h
;    push    r10
    db 41h
    db 53h
;    push    r11
    db 41h
    db 54h
;    push    r12
    db 41h
    db 55h
;    push    r13
    db 41h
    db 56h
;    push    r14
    db 41h
    db 57h
;    push    r15
    db 48h
    mov     ebp, esp
;    mov     rbp, rsp

;;
;;  At this point the stack looks like this:
;;
;;      Calling SS
;;      Calling RSP
;;      rflags
;;      Calling CS
;;      Calling RIP
;;      Error code or 0
;;      Int num or 0ffh for unknown int num
;;      rax
;;      rcx
;;      rdx
;;      rbx
;;      rsp
;;      rbp
;;      rsi
;;      rdi
;;      r8
;;      r9
;;      r10
;;      r11
;;      r12
;;      r13
;;      r14
;;      r15 <------- RSP, RBP
;;      

    call    ClearScreen
    mov     esi, offset String1
    call    PrintString
    db 48h
    mov     eax, [ebp + 16*8]     ;; move Int number into RAX 
    db 48h
    cmp     eax, 18
    ja      PrintDefaultString
PrintExceptionString:
    shl     eax, 3              ;; multiply by 8 to get offset from StringTable to actual string address
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
    db 48h
    mov     eax, [ebp+19*8]    ; CS
    call    PrintQword
    mov     al, ':'
    mov     byte ptr [edi], al
    add     edi, 2
    db 48h
    mov     eax, [ebp+18*8]    ; RIP
    call    PrintQword
    mov     esi, offset String3
    call    PrintString
    
    mov     edi, 0b8140h
    
    mov     esi, offset StringRax     ; rax
    call    PrintString
    db 48h
    mov     eax, [ebp+15*8]
    call    PrintQword
   
    mov     esi, offset StringRcx     ; rcx
    call    PrintString
    db 48h
    mov     eax, [ebp+14*8]
    call    PrintQword
    
    mov     esi, offset StringRdx     ; rdx
    call    PrintString
    db 48h
    mov     eax, [ebp+13*8]
    call    PrintQword
    
    mov     edi, 0b81e0h
    
    mov     esi, offset StringRbx     ; rbx
    call    PrintString
    db 48h
    mov     eax, [ebp+12*8]
    call    PrintQword
     
    mov     esi, offset StringRsp     ; rsp
    call    PrintString
    db 48h
    mov     eax, [ebp+21*8]
    call    PrintQword
    
    mov     esi, offset StringRbp     ; rbp
    call    PrintString
    db 48h
    mov     eax, [ebp+10*8]
    call    PrintQword
    
    mov     edi, 0b8280h
     
    mov     esi, offset StringRsi     ; rsi
    call    PrintString
    db 48h
    mov     eax, [ebp+9*8]
    call    PrintQword
    
    mov     esi, offset StringRdi     ; rdi
    call    PrintString
    db 48h
    mov     eax, [ebp+8*8]
    call    PrintQword
    
    mov     esi, offset StringEcode   ; error code
    call    PrintString
    db 48h
    mov     eax, [ebp+17*8]
    call    PrintQword
    
    mov     edi, 0b8320h
 
    mov     esi, offset StringR8      ; r8
    call    PrintString
    db 48h
    mov     eax, [ebp+7*8]
    call    PrintQword

    mov     esi, offset StringR9      ; r9
    call    PrintString
    db 48h
    mov     eax, [ebp+6*8]
    call    PrintQword

    mov     esi, offset StringR10     ; r10
    call    PrintString
    db 48h
    mov     eax, [ebp+5*8]
    call    PrintQword

    mov     edi, 0b83c0h

    mov     esi, offset StringR11     ; r11
    call    PrintString
    db 48h
    mov     eax, [ebp+4*8]
    call    PrintQword

    mov     esi, offset StringR12     ; r12
    call    PrintString
    db 48h
    mov     eax, [ebp+3*8]
    call    PrintQword

    mov     esi, offset StringR13     ; r13
    call    PrintString
    db 48h
    mov     eax, [ebp+2*8]
    call    PrintQword
 
    mov     edi, 0b8460h

    mov     esi, offset StringR14     ; r14
    call    PrintString
    db 48h
    mov     eax, [ebp+1*8]
    call    PrintQword
 
    mov     esi, offset StringR15     ; r15
    call    PrintString
    db 48h
    mov     eax, [ebp+0*8]
    call    PrintQword

    mov     esi, offset StringSs      ; ss
    call    PrintString
    db 48h
    mov     eax, [ebp+22*8]
    call    PrintQword
  
    mov     edi, 0b8500h

    mov     esi, offset StringRflags  ; rflags
    call    PrintString
    db 48h
    mov     eax, [ebp+20*8]
    call    PrintQword
    
    mov     edi, 0b8640h

    mov     esi, ebp
    add     esi, 23*8
    mov     ecx, 4

    
OuterLoop:
    push    ecx
    mov     ecx, 4
    db 48h
    mov     edx, edi

InnerLoop:
    db 48h
    mov     eax, [esi]
    call    PrintQword
    add     esi, 8
    mov     al, ' '
    mov     [edi], al
    add     edi, 2
    loop    InnerLoop

    pop     ecx
    add     edx, 0a0h
    mov     edi, edx
    loop    OuterLoop


    mov     edi, 0b8960h

    db 48h
    mov     eax, [ebp+18*8]  ; RIP
    sub     eax, 8 * 8
    db 48h
    mov     esi, eax        ; esi = rip - 8 QWORD linear (total 16 QWORD)

    mov     ecx, 4
    
OuterLoop1:
    push    ecx
    mov     ecx, 4
    mov     edx, edi

InnerLoop1:
    db 48h
    mov     eax, [esi]
    call    PrintQword
    add     esi, 8
    mov     al, ' '
    mov     [edi], al
    add     edi, 2
    loop    InnerLoop1

    pop     ecx
    add     edx, 0a0h
    mov     edi, edx
    loop    OuterLoop1



    ;wbinvd
@@:    
    jmp     @b

;
; return
;
    mov     esp, ebp
;    mov     rsp, rbp
    db 41h
    db 5fh
;    pop    r15
    db 41h
    db 5eh
;    pop    r14
    db 41h
    db 5dh
;    pop    r13
    db 41h
    db 5ch
;    pop    r12
    db 41h
    db 5bh
;    pop    r11
    db 41h
    db 5ah
;    pop    r10
    db 41h
    db 59h
;    pop    r9
    db 41h
    db 58h
;    pop    r8
    pop    edi
    pop    esi
    pop    ebp
    pop    eax ; esp
    pop    ebx
    pop    edx
    pop    ecx
    pop    eax
 
    db 48h
    db 83h
    db 0c4h
    db 10h   
;    add    esp, 16 ; error code and INT number

    db 48h
    db 0cfh
;    iretq

PrintString:
    push    eax
@@:
    mov     al, byte ptr [esi]
    cmp     al, 0
    je      @f
    mov     byte ptr [edi], al
    db 0ffh
    db 0c6h
;    inc     esi
    add     edi, 2
    jmp     @b
@@:
    pop     eax
    ret
        
;; RAX contains qword to print
;; RDI contains memory location (screen location) to print it to
PrintQword:
    push    ecx
    push    ebx
    push    eax
    
    db 48h
    db 0c7h
    db 0c1h
    dd 16
;    mov     rcx, 16
looptop:
    db 48h
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

StringTable       dq  offset Int0String, offset Int1String, offset Int2String, offset Int3String, 
                      offset Int4String, offset Int5String, offset Int6String, offset Int7String,
                      offset Int8String, offset Int9String, offset Int10String, offset Int11String,
                      offset Int12String, offset Int13String, offset Int14String, offset Int15String,
                      offset Int16String, offset Int17String, offset Int18String, offset Int19String

String2           db  " HALT!! *** (",0
String3           db  ")",0
StringRax         db  "RAX=",0
StringRcx         db  " RCX=",0
StringRdx         db  " RDX=",0
StringRbx         db  "RBX=",0
StringRsp         db  " RSP=",0
StringRbp         db  " RBP=",0
StringRsi         db  "RSI=",0
StringRdi         db  " RDI=",0
StringEcode       db  " ECODE=",0
StringR8          db  "R8 =",0
StringR9          db  " R9 =",0
StringR10         db  " R10=",0
StringR11         db  "R11=",0
StringR12         db  " R12=",0
StringR13         db  " R13=",0
StringR14         db  "R14=",0
StringR15         db  " R15=",0
StringSs          db  " SS =",0
StringRflags      db  "RFLAGS=",0

Idtr        df  0
            df  0

    org 21ffeh
BlockSignature:
    dw      0aa55h
    
    end

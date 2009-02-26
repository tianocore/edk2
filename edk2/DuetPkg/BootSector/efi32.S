#------------------------------------------------------------------------------
#*
#*   Copyright 2006, Intel Corporation                                                         
#*   All rights reserved. This program and the accompanying materials                          
#*   are licensed and made available under the terms and conditions of the BSD License         
#*   which accompanies this distribution.  The full text of the license may be found at        
#*   http://opensource.org/licenses/bsd-license.php                                            
#*                                                                                             
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#*   
#*    efi32.asm
#*  
#*   Abstract:
#*
#------------------------------------------------------------------------------

##############################################################################
# Now in 32-bit protected mode.
##############################################################################

        .486: 
        #.MODEL flat
        .stack: 
        .code16:  
        .org 0x21000

.equ                 DEFAULT_HANDLER_SIZE, INT1 - INT0

.macro jmpCommonIdtEntry  
    # jmp     commonIdtEntry - this must be hand coded to keep the assembler from
    #                          using a 8 bit reletive jump when the entries are
    #                          within 255 bytes of the common entry.  This must
    #                          be done to maintain the consistency of the size
    #                          of entry points...
    .byte   0xe9                        # jmp 16 bit relative
    .long      commonIdtEntry - . - 4  # A problem     
.endm

Start:  
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs
    movw    %ax, %ss
    movl    $0x001ffff0, %esp

    call    ClearScreen

    # Populate IDT with meaningful offsets for exception handlers...
    sidt    Idtr            

    movl    Halt, %eax
    movl    %eax, %ebx                  # use bx to copy 15..0 to descriptors
    shrl    $16, %eax                   # use ax to copy 31..16 to descriptors 
    movl    $0x78, %ecx                 # 78h IDT entries to initialize with unique entry points (exceptions)
    movl    (Idtr + 2), %esi
    movl    (%esi), %edi

LOOP_1:                                             # loop through all IDT entries exception handlers and initialize to default handler
    movw    %bx, (%edi)                         # write bits 15..0 of offset
    movw    $0x20, 2(%edi)                      # SYS_CODE_SEL from GDT
    movw    $(0x0e00 | 0x8000), 4(%edi)     		# type = 386 interrupt gate, present
    movw    %ax, 6(%edi)                        # write bits 31..16 of offset
    addl    $8, %edi                            # move up to next descriptor
    addw    DEFAULT_HANDLER_SIZE, %bx           # move to next entry point
    loopl   LOOP_1                                 # loop back through again until all descriptors are initialized

    ## at this point edi contains the offset of the descriptor for INT 20
    ## and bx contains the low 16 bits of the offset of the default handler
    ## so initialize all the rest of the descriptors with these two values...
#    mov     ecx, 101                            ; there are 100 descriptors left (INT 20 (14h) - INT 119 (77h)
#@@:                                             ; loop through all IDT entries exception handlers and initialize to default handler
#    mov     word ptr [edi], bx                  ; write bits 15..0 of offset
#    mov     word ptr [edi+2], 20h               ; SYS_CODE_SEL from GDT
#    mov     word ptr [edi+4], 0e00h OR 8000h    ; type = 386 interrupt gate, present
#    mov     word ptr [edi+6], ax                ; write bits 31..16 of offset
#    add     edi, 8                              ; move up to next descriptor
#    loop    @b                                  ; loop back through again until all descriptors are initialized


##  DUMP    location of IDT and several of the descriptors
#    mov     ecx, 8
#    mov     eax, [offset Idtr + 2]
#    mov     eax, [eax]
#    mov     edi, 0b8000h
#    call    PrintDword
#    mov     esi, eax
#    mov     edi, 0b80a0h
#    jmp     OuterLoop

##    
## just for fun, let's do a software interrupt to see if we correctly land in the exception handler...
#    mov     eax, 011111111h
#    mov     ebx, 022222222h
#    mov     ecx, 033333333h
#    mov     edx, 044444444h
#    mov     ebp, 055555555h
#    mov     esi, 066666666h
#    mov     edi, 077777777h
#    push    011111111h
#    push    022222222h
#    push    033333333h
#    int     119


    movl    $0x22000, %esi              # esi = 22000
    movl    0x14(%esi), %eax            # eax = [22014]
    addl    %eax, %esi                  # esi = 22000 + [22014] = Base of EFILDR.C
    movl    0x3c(%esi), %ebp            # ebp = [22000 + [22014] + 3c] = NT Image Header for EFILDR.C
    addl    %esi, %ebp
    movl    0x34(%ebp), %edi            # edi = [[22000 + [22014] + 3c] + 30] = ImageBase
    movl    0x28(%ebp), %eax            # eax = [[22000 + [22014] + 3c] + 24] = EntryPoint
    addl    %edi, %eax                  # eax = ImageBase + EntryPoint
    movl    %eax, EfiLdrOffset             # Modify far jump instruction for correct entry point

    movw    6(%ebp), %bx                # bx = Number of sections
    xorl    %eax, %eax
    movw    0x14(%ebp), %ax             # ax = Optional Header Size
    addl    %eax, %ebp
    addl    $0x18, %ebp                 # ebp = Start of 1st Section

SectionLoop: 
    pushl   %esi                        # Save Base of EFILDR.C
    pushl   %edi                        # Save ImageBase
    addl    0x14(%ebp), %esi            # esi = Base of EFILDR.C + PointerToRawData
    addl    0x0c(%ebp), %edi            # edi = ImageBase + VirtualAddress
    movl    0x10(%ebp), %ecx            # ecs = SizeOfRawData

    cld
    shrl    $2, %ecx
    rep
    movsl

    popl    %edi                        # Restore ImageBase
    popl    %esi                        # Restore Base of EFILDR.C

    addw    $0x28, %bp                  # ebp = ebp + 028h = Pointer to next section record
    decw    %bx
    cmpw    $0, %bx
    jne     SectionLoop

    movzwl  Idtr, %eax                  # get size of IDT
    incl    %eax
    addl    Idtr + 2, %eax             # add to base of IDT to get location of memory map... 
    pushl   %eax                        # push memory map location on stack for call to EFILDR...

    pushl   %eax                        # push return address (useless, just for stack balance)
    .byte   0xb8
EfiLdrOffset: 
    .long   0x00401000                  # Offset of EFILDR
# mov eax, 401000h
    pushl   %eax
    ret

#    db      "**** DEFAULT IDT ENTRY ***",0
    .align 0x2
Halt: 
INT0: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x0
    jmpCommonIdtEntry 
#    db      0e9h                        ; jmp 16 bit reletive 
#    dd      commonIdtEntry - $ - 4      ;  offset to jump to

INT1: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x1
    jmpCommonIdtEntry 

INT2: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x2
    jmpCommonIdtEntry 

INT3: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x3
    jmpCommonIdtEntry 

INT4: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x4
    jmpCommonIdtEntry 

INT5: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x5
    jmpCommonIdtEntry 

INT6: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x6
    jmpCommonIdtEntry 

INT7: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x7
    jmpCommonIdtEntry 

INT8: 
#   Double fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $0x8
    jmpCommonIdtEntry 

INT9: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x9
    jmpCommonIdtEntry 

INT10: 
#   Invalid TSS causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $10
    jmpCommonIdtEntry 

INT11: 
#   Segment Not Present causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $11
    jmpCommonIdtEntry 

INT12: 
#   Stack fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $12
    jmpCommonIdtEntry 

INT13: 
#   GP fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $13
    jmpCommonIdtEntry 

INT14: 
#   Page fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $14
    jmpCommonIdtEntry 

INT15: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $15
    jmpCommonIdtEntry 

INT16: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $16
    jmpCommonIdtEntry 

INT17: 
#   Alignment check causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $17
    jmpCommonIdtEntry 

INT18: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $18
    jmpCommonIdtEntry 

INT19: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $19
    jmpCommonIdtEntry 

INTUnknown: 
.rept  (0x78 - 20)
    pushl   $0x0    # push error code place holder on the stack
#   push    $0xxx   # push vector number
    .byte   0x6a
    .long   ( . - INTUnknown - 3 ) / 9 + 20  # vector number
    jmpCommonIdtEntry 
.endr

commonIdtEntry: 
    pushal
    movl    %esp, %ebp
##
##  At this point the stack looks like this:
##
##      eflags
##      Calling CS
##      Calling EIP
##      Error code or 0
##      Int num or 0ffh for unknown int num
##      eax
##      ecx
##      edx
##      ebx
##      esp
##      ebp
##      esi
##      edi <------- ESP, EBP
##      

    call    ClearScreen
    movl    String1, %esi
    call    PrintString
    movl    32(%ebp), %eax      ## move Int number into EAX 
    cmpl    $19, %eax
    ja      PrintDefaultString
PrintExceptionString: 
    shll    $2, %eax            ## multiply by 4 to get offset from StringTable to actual string address
    addl    StringTable, %eax
    movl    (%eax), %esi
    jmp     PrintTheString
PrintDefaultString: 
    movl    IntUnknownString, %esi
    # patch Int number
    movl    %eax, %edx
    call    A2C
    movb    %al, 1(%esi)
    movl    %edx, %eax
    shrl    $4, %eax
    call    A2C
    movb    %al, (%esi)
PrintTheString:        
    call    PrintString
    movl    String2, %esi
    call    PrintString
    movl    44(%ebp), %eax         # CS
    call    PrintDword
    movb    ':', %al
    movb    %al, (%edi)
    addl    $2, %edi
    movl    40(%ebp), %eax         # EIP
    call    PrintDword
    movl    String3, %esi
    call    PrintString

    movl    $0xb8140, %edi

    movl    StringEax, %esi           # eax
    call    PrintString
    movl    28(%ebp), %eax
    call    PrintDword

    movl    StringEbx, %esi           # ebx
    call    PrintString
    movl    16(%ebp), %eax
    call    PrintDword

    movl    StringEcx, %esi           # ecx
    call    PrintString
    movl    24(%ebp), %eax
    call    PrintDword

    movl    StringEdx, %esi           # edx
    call    PrintString
    movl    20(%ebp), %eax
    call    PrintDword

    movl    StringEcode, %esi         # error code
    call    PrintString
    movl    36(%ebp), %eax
    call    PrintDword

    movl    $0xb81e0, %edi

    movl    StringEsp, %esi           # esp
    call    PrintString
    movl    12(%ebp), %eax
    call    PrintDword

    movl    StringEbp, %esi           # ebp
    call    PrintString
    movl    8(%ebp), %eax
    call    PrintDword

    movl    StringEsi, %esi           # esi
    call    PrintString
    movl    4(%ebp), %eax
    call    PrintDword

    movl    StringEdi, %esi          # edi
    call    PrintString
    movl    (%ebp), %eax
    call    PrintDword

    movl    StringEflags, %esi       # eflags
    call    PrintString
    movl    48(%ebp), %eax
    call    PrintDword

    movl    $0xb8320, %edi

    movl    %ebp, %esi
    addl    $52, %esi
    movl    $8, %ecx


OuterLoop: 
    pushl   %ecx
    movl    $8, %ecx
    movl    %edi, %edx

InnerLoop: 
    movl    (%esi), %eax
    call    PrintDword
    addl    $4, %esi
    movb    ' ', %al 
    movb    %al, (%edi)
    addl    $2, %edi
    loop    InnerLoop

    popl    %ecx
    addl    $0xa0, %edx
    movl    %edx, %edi
    loop    OuterLoop


    movl    $0xb8960, %edi

    movl    40(%ebp), %eax # EIP
    subl    $32*4, %eax
    movl    %eax, %esi      # esi = eip - 32 DWORD linear (total 64 DWORD)

    movl    $8, %ecx

OuterLoop1: 
    pushl   %ecx
    movl    $8, %ecx
    movl    %edi, %edx

InnerLoop1: 
    movl    (%esi), %eax
    call    PrintDword
    addl    $4, %esi
    movb    ' ', %al 
    movb    %al, (%edi)
    addl    $2, %edi
    loop    InnerLoop1

    popl    %ecx
    addl    $0xa0, %edx
    movl    %edx, %edi
    loop    OuterLoop1



#    wbinvd ; this intruction does not support in early than 486 arch
LN_C1:    
    jmp     LN_C1
#
# return
#
    movl    %ebp, %esp
    popal
    addl    $8, %esp # error code and INT number

    iretl


PrintString: 
    pushl   %eax
LN_C2: 
    movb    (%esi), %al
    cmpb    $0, %al
    je      LN_C3
    movb    %al, (%edi)
    incl    %esi
    addl    $2, %edi
    jmp     LN_C2
LN_C3: 
    popl    %eax
    ret

## EAX contains dword to print
## EDI contains memory location (screen location) to print it to
PrintDword: 
    pushl   %ecx
    pushl   %ebx
    pushl   %eax

    movl    $8, %ecx
looptop: 
    roll    $4, %eax
    movb    %al, %bl
    andb    $0xf, %bl
    addb    '0', %bl
    cmpb    '9', %bl
    jle     LN_C4
    addb    $7, %bl
LN_C4: 
    movb    %bl, (%edi)
    addl    $2, %edi
    loop    looptop
    #wbinvd

    popl    %eax
    popl    %ebx
    popl    %ecx
    ret

ClearScreen: 
    pushl   %eax
    pushl   %ecx

    movb    ' ', %al
    movb    $0xc, %ah
    movl    $0xb8000, %edi
    movl    $80*24, %ecx
LN_C5: 
    movw    %ax, (%edi)
    addl    $2, %edi
    loop    LN_C5
    movl    $0xb8000, %edi

    popl    %ecx
    popl    %eax

    ret

A2C: 
    andb    $0xf, %al
    addb    '0', %al
    cmpb    '9', %al
    jle     LN_C6
    addb    $7, %al
LN_C6: 
    ret

String1:            .asciz      "*** INT "

Int0String:         .asciz      "00h Divide by 0 -"
Int1String:         .asciz      "01h Debug exception -"
Int2String:         .asciz      "02h NMI -"
Int3String:         .asciz      "03h Breakpoint -"
Int4String:         .asciz      "04h Overflow -"
Int5String:         .asciz      "05h Bound -"
Int6String:         .asciz      "06h Invalid opcode -"
Int7String:         .asciz      "07h Device not available -"
Int8String:         .asciz      "08h Double fault -"
Int9String:         .asciz      "09h Coprocessor seg overrun (reserved) -"
Int10String:        .asciz      "0Ah Invalid TSS -"
Int11String:        .asciz      "0Bh Segment not present -"
Int12String:        .asciz      "0Ch Stack fault -"
Int13String:        .asciz      "0Dh General protection fault -"
Int14String:        .asciz      "0Eh Page fault -"
Int15String:        .asciz      "0Fh (Intel reserved) -"
Int16String:        .asciz      "10h Floating point error -"
Int17String:        .asciz      "11h Alignment check -"
Int18String:        .asciz      "12h Machine check -"
Int19String:        .asciz      "13h SIMD Floating-Point Exception -"
IntUnknownString:   .asciz      "??h Unknown interrupt -"

StringTable:   .long  Int0String, Int1String, Int2String, Int3String,    \
                      Int4String, Int5String, Int6String, Int7String,    \
                      Int8String, Int9String, Int10String, Int11String,  \
                      Int12String, Int13String, Int14String, Int15String,\
                      Int16String, Int17String, Int18String, Int19String

String2:         .asciz  " HALT!! *** ("
String3:         .asciz  ")"
StringEax:       .asciz  "EAX="
StringEbx:       .asciz  "EBX="
StringEcx:       .asciz  "ECX="
StringEdx:       .asciz  "EDX="
StringEcode:     .asciz  "ECODE="
StringEsp:       .asciz  "ESP="
StringEbp:       .asciz  "EBP="
StringEsi:       .asciz  "ESI="
StringEdi:       .asciz  "EDI="
StringEflags:    .asciz  "EFLAGS="

Idtr:        .float  0

    .org 0x21ffe
BlockSignature: 
    .word   0xaa55



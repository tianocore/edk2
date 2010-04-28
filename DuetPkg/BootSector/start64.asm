;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
;*   This program and the accompanying materials                          
;*   are licensed and made available under the terms and conditions of the BSD License         
;*   which accompanies this distribution.  The full text of the license may be found at        
;*   http://opensource.org/licenses/bsd-license.php                                            
;*                                                                                             
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;*   
;*    start64.asm
;*  
;*   Abstract:
;*
;------------------------------------------------------------------------------

        .model  small
        .stack
        .486p
        .code

FAT_DIRECTORY_ENTRY_SIZE    EQU     020h
FAT_DIRECTORY_ENTRY_SHIFT   EQU     5
BLOCK_SIZE                  EQU     0200h
BLOCK_MASK                  EQU     01ffh
BLOCK_SHIFT                 EQU     9

        org 0h
Ia32Jump:
  jmp   BootSectorEntryPoint  ; JMP inst    - 3 bytes
  nop

OemId             db  "INTEL   "    ; OemId               - 8 bytes

SectorSize        dw  0             ; Sector Size         - 16 bits
SectorsPerCluster db  0             ; Sector Per Cluster  - 8 bits
ReservedSectors   dw  0             ; Reserved Sectors    - 16 bits
NoFats            db  0             ; Number of FATs      - 8 bits
RootEntries       dw  0             ; Root Entries        - 16 bits
Sectors           dw  0             ; Number of Sectors   - 16 bits
Media             db  0             ; Media               - 8 bits  - ignored
SectorsPerFat     dw  0             ; Sectors Per FAT     - 16 bits
SectorsPerTrack   dw  0             ; Sectors Per Track   - 16 bits - ignored
Heads             dw  0             ; Heads               - 16 bits - ignored
HiddenSectors     dd  0             ; Hidden Sectors      - 32 bits - ignored
LargeSectors      dd  0             ; Large Sectors       - 32 bits 
PhysicalDrive     db  0             ; PhysicalDriveNumber - 8 bits  - ignored
CurrentHead       db  0             ; Current Head        - 8 bits
Signature         db  0             ; Signature           - 8 bits  - ignored
VolId             db  "    "        ; Volume Serial Number- 4 bytes
FatLabel          db  "           " ; Label               - 11 bytes
SystemId          db  "FAT12   "    ; SystemId            - 8 bytes

BootSectorEntryPoint:
        ASSUME  ds:@code
        ASSUME  ss:@code
      ; ds = 1000, es = 2000 + x (size of first cluster >> 4)
      ; cx = Start Cluster of EfiLdr
      ; dx = Start Cluster of Efivar.bin

; Re use the BPB data stored in Boot Sector
        mov     bp,07c00h

        push    cx
; Read Efivar.bin
;       1000:dx    = DirectoryEntry of Efivar.bin -> BS.com has filled already
        mov     ax,01900h
        mov     es,ax
        test    dx,dx
        jnz     CheckVarStoreSize

        mov     al,1
NoVarStore:
        push    es
; Set the 5th byte start @ 0:19000 to non-zero indicating we should init var store header in DxeIpl
        mov     byte ptr es:[4],al
        jmp     SaveVolumeId

CheckVarStoreSize:
        mov     di,dx
        cmp     dword ptr ds:[di+2], 04000h
        mov     al,2
        jne     NoVarStore

LoadVarStore:
        mov     al,0
        mov     byte ptr es:[4],al
        mov     cx,word ptr[di]
;       ES:DI = 1500:0
        xor     di,di
        push    es
        mov     ax,01500h
        mov     es,ax
        call    ReadFile
SaveVolumeId:
        pop     es
        mov     ax,word ptr [bp+VolId]
        mov     word ptr es:[0],ax                  ; Save Volume Id to 0:19000. we will find the correct volume according to this VolumeId
        mov     ax,word ptr [bp+VolId+2]
        mov     word ptr es:[2],ax

; Read Efildr
        pop     cx
;       cx    = Start Cluster of Efildr -> BS.com has filled already
;       ES:DI = 2000:0, first cluster will be read again
        xor     di,di                               ; di = 0
        mov     ax,02000h
        mov     es,ax
        call    ReadFile
        mov     ax,cs
        mov     word ptr cs:[JumpSegment],ax

CheckEm64T:
        mov  eax, 080000001h
;        cpuid
        dw   0A20Fh
        bt   edx, 29
        jc   CheckEm64TPass
        push cs
        pop  ds
        lea  si, [Em64String]
        mov  cx, 18
        jmp  PrintStringAndHalt
CheckEm64TPass:
JumpFarInstruction:
        db      0eah
JumpOffset:
        dw      0200h
JumpSegment:
        dw      2000h



; ****************************************************************************
; ReadFile
;
; Arguments:
;   CX    = Start Cluster of File
;   ES:DI = Buffer to store file content read from disk
;
; Return:
;   (ES << 4 + DI) = end of file content Buffer
;
; ****************************************************************************
ReadFile:
; si      = NumberOfClusters
; cx      = ClusterNumber
; dx      = CachedFatSectorNumber
; ds:0000 = CacheFatSectorBuffer
; es:di   = Buffer to load file
; bx      = NextClusterNumber
        pusha
        mov     si,1                                ; NumberOfClusters = 1
        push    cx                                  ; Push Start Cluster onto stack
        mov     dx,0fffh                            ; CachedFatSectorNumber = 0xfff
FatChainLoop:
        mov     ax,cx                               ; ax = ClusterNumber    
        and     ax,0ff8h                            ; ax = ax & 0xff8
        cmp     ax,0ff8h                            ; See if this is the last cluster
        je      FoundLastCluster                    ; Jump if last cluster found
        mov     ax,cx                               ; ax = ClusterNumber
        shl     ax,1                                ; ax = ClusterNumber * 2
        add     ax,cx                               ; ax = ClusterNumber * 2 + ClusterNumber = ClusterNumber * 3
        shr     ax,1                                ; FatOffset = ClusterNumber*3 / 2
        push    si                                  ; Save si
        mov     si,ax                               ; si = FatOffset
        shr     ax,BLOCK_SHIFT                      ; ax = FatOffset >> BLOCK_SHIFT
        add     ax,word ptr [bp+ReservedSectors]    ; ax = FatSectorNumber = ReservedSectors + (FatOffset >> BLOCK_OFFSET)
        and     si,BLOCK_MASK                       ; si = FatOffset & BLOCK_MASK
        cmp     ax,dx                               ; Compare FatSectorNumber to CachedFatSectorNumber
        je      SkipFatRead
        mov     bx,2                                
        push    es
        push    ds
        pop     es
        call    ReadBlocks                          ; Read 2 blocks starting at AX storing at ES:DI
        pop     es
        mov     dx,ax                               ; CachedFatSectorNumber = FatSectorNumber
SkipFatRead:
        mov     bx,word ptr [si]                    ; bx = NextClusterNumber
        mov     ax,cx                               ; ax = ClusterNumber
        and     ax,1                                ; See if this is an odd cluster number
        je      EvenFatEntry
        shr     bx,4                                ; NextClusterNumber = NextClusterNumber >> 4
EvenFatEntry:
        and     bx,0fffh                            ; Strip upper 4 bits of NextClusterNumber
        pop     si                                  ; Restore si
        dec     bx                                  ; bx = NextClusterNumber - 1
        cmp     bx,cx                               ; See if (NextClusterNumber-1)==ClusterNumber
        jne     ReadClusters
        inc     bx                                  ; bx = NextClusterNumber
        inc     si                                  ; NumberOfClusters++
        mov     cx,bx                               ; ClusterNumber = NextClusterNumber
        jmp     FatChainLoop
ReadClusters:
        inc     bx
        pop     ax                                  ; ax = StartCluster
        push    bx                                  ; StartCluster = NextClusterNumber
        mov     cx,bx                               ; ClusterNumber = NextClusterNumber
        sub     ax,2                                ; ax = StartCluster - 2
        xor     bh,bh                               
        mov     bl,byte ptr [bp+SectorsPerCluster]  ; bx = SectorsPerCluster
        mul     bx                                  ; ax = (StartCluster - 2) * SectorsPerCluster
        add     ax, word ptr [bp]                   ; ax = FirstClusterLBA + (StartCluster-2)*SectorsPerCluster
        push    ax                                  ; save start sector
        mov     ax,si                               ; ax = NumberOfClusters
        mul     bx                                  ; ax = NumberOfClusters * SectorsPerCluster
        mov     bx,ax                               ; bx = Number of Sectors
        pop     ax                                  ; ax = Start Sector
        call    ReadBlocks
        mov     si,1                                ; NumberOfClusters = 1
        jmp     FatChainLoop
FoundLastCluster:
        pop     cx
        popa
        ret


; ****************************************************************************
; ReadBlocks - Reads a set of blocks from a block device
;
; AX    = Start LBA
; BX    = Number of Blocks to Read
; ES:DI = Buffer to store sectors read from disk
; ****************************************************************************

; cx = Blocks
; bx = NumberOfBlocks
; si = StartLBA

ReadBlocks:
        pusha
        add     eax,dword ptr [bp+LBAOffsetForBootSector]    ; Add LBAOffsetForBootSector to Start LBA
        add     eax,dword ptr [bp+HiddenSectors]    ; Add HiddenSectors to Start LBA
        mov     esi,eax                             ; esi = Start LBA
        mov     cx,bx                               ; cx = Number of blocks to read
ReadCylinderLoop:
        mov     bp,07bfch                           ; bp = 0x7bfc
        mov     eax,esi                             ; eax = Start LBA
        xor     edx,edx                             ; edx = 0
        movzx   ebx,word ptr [bp]                   ; bx = MaxSector
        div     ebx                                 ; ax = StartLBA / MaxSector
        inc     dx                                  ; dx = (StartLBA % MaxSector) + 1

        mov     bx,word ptr [bp]                    ; bx = MaxSector
        sub     bx,dx                               ; bx = MaxSector - Sector
        inc     bx                                  ; bx = MaxSector - Sector + 1
        cmp     cx,bx                               ; Compare (Blocks) to (MaxSector - Sector + 1)
        jg      LimitTransfer
        mov     bx,cx                               ; bx = Blocks
LimitTransfer:
        push    ax                                  ; save ax
        mov     ax,es                               ; ax = es
        shr     ax,(BLOCK_SHIFT-4)                  ; ax = Number of blocks into mem system
        and     ax,07fh                             ; ax = Number of blocks into current seg
        add     ax,bx                               ; ax = End Block number of transfer
        cmp     ax,080h                             ; See if it crosses a 64K boundry
        jle     NotCrossing64KBoundry               ; Branch if not crossing 64K boundry
        sub     ax,080h                             ; ax = Number of blocks past 64K boundry
        sub     bx,ax                               ; Decrease transfer size by block overage
NotCrossing64KBoundry:
        pop     ax                                  ; restore ax

        push    cx
        mov     cl,dl                               ; cl = (StartLBA % MaxSector) + 1 = Sector
        xor     dx,dx                               ; dx = 0
        div     word ptr [bp+2]                     ; ax = ax / (MaxHead + 1) = Cylinder  
                                                    ; dx = ax % (MaxHead + 1) = Head

        push    bx                                  ; Save number of blocks to transfer
        mov     dh,dl                               ; dh = Head
        mov     bp,07c00h                           ; bp = 0x7c00
        mov     dl,byte ptr [bp+PhysicalDrive]      ; dl = Drive Number
        mov     ch,al                               ; ch = Cylinder
        mov     al,bl                               ; al = Blocks
        mov     ah,2                                ; ah = Function 2
        mov     bx,di                               ; es:bx = Buffer address
        int     013h
        jc      DiskError
        pop     bx
        pop     cx
        movzx   ebx,bx
        add     esi,ebx                             ; StartLBA = StartLBA + NumberOfBlocks
        sub     cx,bx                               ; Blocks = Blocks - NumberOfBlocks
        mov     ax,es
        shl     bx,(BLOCK_SHIFT-4)
        add     ax,bx
        mov     es,ax                               ; es:di = es:di + NumberOfBlocks*BLOCK_SIZE
        cmp     cx,0
        jne     ReadCylinderLoop
        popa
        ret

DiskError:
        push cs
        pop  ds
        lea  si, [ErrorString]
        mov  cx, 7
        jmp  PrintStringAndHalt

PrintStringAndHalt:
        mov  ax,0b800h
        mov  es,ax
        mov  di,160
        rep  movsw
Halt:
        jmp   Halt

ErrorString:
        db 'S', 0ch, 'E', 0ch, 'r', 0ch, 'r', 0ch, 'o', 0ch, 'r', 0ch, '!', 0ch

        org     01fah
LBAOffsetForBootSector:
        dd      0h

        org     01feh
        dw      0aa55h

;******************************************************************************
;******************************************************************************
;******************************************************************************

DELAY_PORT           equ     0edh    ; Port to use for 1uS delay
KBD_CONTROL_PORT     equ     060h    ; 8042 control port     
KBD_STATUS_PORT      equ     064h    ; 8042 status port      
WRITE_DATA_PORT_CMD  equ     0d1h    ; 8042 command to write the data port
ENABLE_A20_CMD       equ     0dfh    ; 8042 command to enable A20

        org     200h
        jmp start
Em64String:
        db 'E', 0ch, 'm', 0ch, '6', 0ch, '4', 0ch, 'T', 0ch, ' ', 0ch, 'U', 0ch, 'n', 0ch, 's', 0ch, 'u', 0ch, 'p', 0ch, 'p', 0ch, 'o', 0ch, 'r', 0ch, 't', 0ch, 'e', 0ch, 'd', 0ch, '!', 0ch

start:  
        mov ax,cs
        mov ds,ax
        mov es,ax
        mov ss,ax
        mov sp,MyStack

;        mov ax,0b800h
;        mov es,ax
;        mov byte ptr es:[160],'a'
;        mov ax,cs
;        mov es,ax

        mov ebx,0
        lea edi,MemoryMap
MemMapLoop:
        mov eax,0e820h
        mov ecx,20
        mov edx,'SMAP'
        int 15h
        jc  MemMapDone
        add edi,20
        cmp ebx,0
        je  MemMapDone
        jmp MemMapLoop
MemMapDone:
        lea eax,MemoryMap
        sub edi,eax                         ; Get the address of the memory map
        mov dword ptr [MemoryMapSize],edi   ; Save the size of the memory map

        xor     ebx,ebx
        mov     bx,cs                       ; BX=segment
        shl     ebx,4                       ; BX="linear" address of segment base
        lea     eax,[GDT_BASE + ebx]        ; EAX=PHYSICAL address of gdt
        mov     dword ptr [gdtr + 2],eax    ; Put address of gdt into the gdtr
        lea     eax,[IDT_BASE + ebx]        ; EAX=PHYSICAL address of idt
        mov     dword ptr [idtr + 2],eax    ; Put address of idt into the idtr
        lea     edx,[MemoryMapSize + ebx]   ; Physical base address of the memory map

;        mov ax,0b800h
;        mov es,ax
;        mov byte ptr es:[162],'b'
;        mov ax,cs
;        mov es,ax

;
; Enable A20 Gate 
;

        mov ax,2401h                        ; Enable A20 Gate
        int 15h
        jnc A20GateEnabled                  ; Jump if it suceeded

;
; If INT 15 Function 2401 is not supported, then attempt to Enable A20 manually.
;

        call    Empty8042InputBuffer        ; Empty the Input Buffer on the 8042 controller
        jnz     Timeout8042                 ; Jump if the 8042 timed out
        out     DELAY_PORT,ax               ; Delay 1 uS
        mov     al,WRITE_DATA_PORT_CMD      ; 8042 cmd to write output port
        out     KBD_STATUS_PORT,al          ; Send command to the 8042
        call    Empty8042InputBuffer        ; Empty the Input Buffer on the 8042 controller
        jnz     Timeout8042                 ; Jump if the 8042 timed out
        mov     al,ENABLE_A20_CMD           ; gate address bit 20 on
        out     KBD_CONTROL_PORT,al         ; Send command to thre 8042
        call    Empty8042InputBuffer        ; Empty the Input Buffer on the 8042 controller
        mov     cx,25                       ; Delay 25 uS for the command to complete on the 8042
Delay25uS:
        out     DELAY_PORT,ax               ; Delay 1 uS
        loop    Delay25uS                       
Timeout8042:


A20GateEnabled:

;
; DISABLE INTERRUPTS - Entering Protected Mode
;

        cli                             

;        mov ax,0b800h
;        mov es,ax
;        mov byte ptr es:[164],'c'
;        mov ax,cs
;        mov es,ax

    lea eax, OffsetIn32BitProtectedMode
    add eax, 20000h + 6h
    mov dword ptr[OffsetIn32BitProtectedMode], eax

    lea eax, OffsetInLongMode
    add eax, 20000h + 6h
    mov dword ptr[OffsetInLongMode], eax

    ;
    ; load GDT
    ;
    db      66h     
    lgdt    fword ptr [gdtr]

    ;
    ; Enable Protect Mode (set CR0.PE=1)
    ;
    mov   eax, cr0        ; Read CR0.
    or    eax, 1h         ; Set PE=1
    mov   cr0, eax        ; Write CR0.
    db    066h
    db    0eah                        ; jmp far 16:32
OffsetIn32BitProtectedMode:
    dd    00000000h                   ; offset $+8   (In32BitProtectedMode)
    dw    10h                         ; selector  (flat CS)
In32BitProtectedMode:

;
; Entering Long Mode
;
    db   66h
    mov  ax, 8
    mov  ds, ax
    mov  es, ax
    mov  ss, ax

    ;
    ; Enable the 64-bit page-translation-table entries by
    ; setting CR4.PAE=1 (this is _required_ before activating
    ; long mode). Paging is not enabled until after long mode
    ; is enabled.
    ;
    db 0fh
    db 20h
    db 0e0h
;    mov eax, cr4
    bts eax, 5
    db 0fh
    db 22h
    db 0e0h
;    mov cr4, eax

    ;
    ; This is the Trapolean Page Tables that are guarenteed
    ;  under 4GB.
    ;
    ; Address Map:
    ;    10000 ~    12000 - efildr (loaded)
    ;    20000 ~    21000 - start64.com
    ;    21000 ~    22000 - efi64.com
    ;    22000 ~    90000 - efildr
    ;    90000 ~    96000 - 4G pagetable (will be reload later)
    ;
    db  0b8h
    dd  90000h
;    mov eax, 90000h
    mov cr3, eax

    ;
    ; Enable long mode (set EFER.LME=1).
    ;
    db  0b9h
    dd  0c0000080h
;    mov   ecx, 0c0000080h ; EFER MSR number.
    db 0fh
    db 32h
;    rdmsr                 ; Read EFER.
    db    0fh
    db    0bah
    db    0e8h
    db    08h
;    bts   eax, 8          ; Set LME=1.
    db 0fh
    db 30h
;    wrmsr                 ; Write EFER.

    ;
    ; Enable paging to activate long mode (set CR0.PG=1)
    ;
    mov   eax, cr0        ; Read CR0.
    db    0fh
    db    0bah
    db    0e8h
    db    01fh
;    bts   eax, 31         ; Set PG=1.
    mov   cr0, eax        ; Write CR0.
    jmp   GoToLongMode
GoToLongMode:

    db      067h
    db      0eah                ; Far Jump $+9:Selector to reload CS
OffsetInLongMode:
    dd      00000000            ;   $+9 Offset is ensuing instruction boundary
    dw      038h                ;   Selector is our code selector, 38h

InLongMode:
    db   66h
    mov     ax, 30h
    mov     ds, ax

    db   66h
    mov     ax, 18h
    mov     es, ax
    mov     ss, ax
    mov     ds, ax

    db 0bdh
    dd 400000h
;    mov ebp,000400000h                  ; Destination of EFILDR32
    db 0bbh
    dd 70000h
;    mov ebx,000070000h                  ; Length of copy

    ;
    ; load idt later
    ;
    db 48h
    db 33h
    db 0c0h
;    xor rax, rax
    db 66h
    mov ax, offset idtr
    db 48h
    db 05h
    dd 20000h
;    add rax, 20000h

    db 0fh
    db 01h
    db 18h
;    lidt    fword ptr [rax]

    db 48h
    db 0c7h
    db 0c0h
    dd 21000h
;   mov rax, 21000h
    db 50h
;   push rax

; ret
    db 0c3h

Empty8042InputBuffer:
        mov cx,0
Empty8042Loop:
        out     DELAY_PORT,ax               ; Delay 1us
        in      al,KBD_STATUS_PORT          ; Read the 8042 Status Port
        and     al,02h                      ; Check the Input Buffer Full Flag
        loopnz  Empty8042Loop               ; Loop until the input buffer is empty or a timout of 65536 uS
        ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        align 02h

gdtr    dw GDT_END - GDT_BASE - 1   ; GDT limit
        dd 0                        ; (GDT base gets set above)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   global descriptor table (GDT)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        align 02h

public GDT_BASE
GDT_BASE:
; null descriptor
NULL_SEL            equ $-GDT_BASE          ; Selector [0x0]
        dw 0            ; limit 15:0
        dw 0            ; base 15:0
        db 0            ; base 23:16
        db 0            ; type
        db 0            ; limit 19:16, flags
        db 0            ; base 31:24

; linear data segment descriptor
LINEAR_SEL      equ $-GDT_BASE          ; Selector [0x8]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 092h         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; linear code segment descriptor
LINEAR_CODE_SEL equ $-GDT_BASE          ; Selector [0x10]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 09Ah         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; system data segment descriptor
SYS_DATA_SEL    equ $-GDT_BASE          ; Selector [0x18]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 092h         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; system code segment descriptor
SYS_CODE_SEL    equ $-GDT_BASE          ; Selector [0x20]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 09Ah         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; spare segment descriptor
SPARE3_SEL  equ $-GDT_BASE          ; Selector [0x28]
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

        align 02h



idtr            dw IDT_END - IDT_BASE - 1   ; IDT limit
        dq 0                        ; (IDT base gets set above)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   interrupt descriptor table (IDT)
;
;   Note: The hardware IRQ's specified in this table are the normal PC/AT IRQ
;       mappings.  This implementation only uses the system timer and all other
;       IRQs will remain masked.  The descriptors for vectors 33+ are provided
;       for convenience.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;idt_tag db "IDT",0     
        align 02h

public IDT_BASE
IDT_BASE:
; divide by zero (INT 0)
DIV_ZERO_SEL        equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; debug exception (INT 1)
DEBUG_EXCEPT_SEL    equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; NMI (INT 2)
NMI_SEL             equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; soft breakpoint (INT 3)
BREAKPOINT_SEL      equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; overflow (INT 4)
OVERFLOW_SEL        equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; bounds check (INT 5)
BOUNDS_CHECK_SEL    equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; invalid opcode (INT 6)
INVALID_OPCODE_SEL  equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; device not available (INT 7)
DEV_NOT_AVAIL_SEL   equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; double fault (INT 8)
DOUBLE_FAULT_SEL    equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; Coprocessor segment overrun - reserved (INT 9)
RSVD_INTR_SEL1      equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; invalid TSS (INT 0ah)
INVALID_TSS_SEL     equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; segment not present (INT 0bh)
SEG_NOT_PRESENT_SEL equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; stack fault (INT 0ch)
STACK_FAULT_SEL     equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; general protection (INT 0dh)
GP_FAULT_SEL        equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; page fault (INT 0eh)
PAGE_FAULT_SEL      equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; Intel reserved - do not use (INT 0fh)
RSVD_INTR_SEL2      equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; floating point error (INT 10h)
FLT_POINT_ERR_SEL   equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; alignment check (INT 11h)
ALIGNMENT_CHECK_SEL equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; machine check (INT 12h)
MACHINE_CHECK_SEL   equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; SIMD floating-point exception (INT 13h)
SIMD_EXCEPTION_SEL  equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; 85 unspecified descriptors, First 12 of them are reserved, the rest are avail
        db (85 * 16) dup(0)
        
; IRQ 0 (System timer) - (INT 68h)
IRQ0_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 1 (8042 Keyboard controller) - (INT 69h)
IRQ1_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; Reserved - IRQ 2 redirect (IRQ 2) - DO NOT USE!!! - (INT 6ah)
IRQ2_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 3 (COM 2) - (INT 6bh)
IRQ3_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 4 (COM 1) - (INT 6ch)
IRQ4_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 5 (LPT 2) - (INT 6dh)
IRQ5_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 6 (Floppy controller) - (INT 6eh)
IRQ6_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 7 (LPT 1) - (INT 6fh)
IRQ7_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 8 (RTC Alarm) - (INT 70h)
IRQ8_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 9 - (INT 71h)
IRQ9_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 10 - (INT 72h)
IRQ10_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 11 - (INT 73h)
IRQ11_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 12 (PS/2 mouse) - (INT 74h)
IRQ12_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 13 (Floating point error) - (INT 75h)
IRQ13_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 14 (Secondary IDE) - (INT 76h)
IRQ14_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

; IRQ 15 (Primary IDE) - (INT 77h)
IRQ15_SEL            equ $-IDT_BASE
        dw 0            ; offset 15:0
        dw SYS_CODE64_SEL ; selector 15:0
        db 0            ; 0 for interrupt gate
        db 0eh OR 80h   ; (10001110)type = 386 interrupt gate, present
        dw 0            ; offset 31:16
        dd 0            ; offset 63:32
        dd 0            ; 0 for reserved

IDT_END:

        align 02h

MemoryMapSize   dd  0
MemoryMap   dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0

        dd  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        dd  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

        org 0fe0h
MyStack:    
        ; below is the pieces of the IVT that is used to redirect INT 68h - 6fh
        ;    back to INT 08h - 0fh  when in real mode...  It is 'org'ed to a
        ;    known low address (20f00) so it can be set up by PlMapIrqToVect in
        ;    8259.c
                
        int 8
        iret
        
        int 9
        iret
        
        int 10
        iret
        
        int 11
        iret
        
        int 12
        iret
        
        int 13
        iret
        
        int 14
        iret
        
        int 15
        iret
        
        
        org 0ffeh
BlockSignature:
        dw  0aa55h

        end 

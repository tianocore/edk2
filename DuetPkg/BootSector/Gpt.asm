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
;*    gpt.asm
;*  
;*   Abstract:
;*
;------------------------------------------------------------------------------

    .model  small
;   .dosseg
    .stack
    .486p
    .code

BLOCK_SIZE                EQU     0200h
BLOCK_MASK                EQU     01ffh
BLOCK_SHIFT               EQU     9

; ****************************************************************************
; Code loaded by BIOS at 0x0000:0x7C00
; ****************************************************************************

        org 0h
Start:

; ****************************************************************************
; Start Print
; ****************************************************************************

    mov  ax,0b800h
    mov  es,ax
    mov  ax, 07c0h
    mov  ds, ax
    lea  si, cs:[StartString]
    mov  cx, 10
    mov  di, 160
    rep  movsw 

; ****************************************************************************
; Print over
; ****************************************************************************

; ****************************************************************************
; Initialize segment registers and copy code at 0x0000:0x7c00 to 0x0000:0x0600
; ****************************************************************************
        xor   ax, ax                    ; AX = 0x0000  
        mov   bx, 07c00h                ; BX = 0x7C00
        mov   bp, 0600h                 ; BP = 0x0600
        mov   si, OFFSET RelocatedStart ; SI = Offset(RelocatedStart)
        mov   cx, 0200h                 ; CX = 0x0200
        sub   cx, si                    ; CS = 0x0200 - Offset(RelocatedStart)
        lea   di, [bp+si]               ; DI = 0x0600 + Offset(RelocatedStart)
        lea   si, [bx+si]               ; BX = 0x7C00 + Offset(RelocatedStart)
        mov   ss, ax                    ; SS = 0x0000
        mov   sp, bx                    ; SP = 0x7C00
        mov   es,ax                     ; ES = 0x0000
        mov   ds,ax                     ; DS = 0x0000
        push  ax                        ; PUSH 0x0000
        push  di                        ; PUSH 0x0600 + Offset(RelocatedStart)
        cld                             ; Clear the direction flag
        rep   movsb                     ; Copy 0x0200 bytes from 0x7C00 to 0x0600
        retf                            ; JMP 0x0000:0x0600 + Offset(RelocatedStart)

; ****************************************************************************
; Code relocated to 0x0000:0x0600
; ****************************************************************************

RelocatedStart:
; ****************************************************************************
; Get Driver Parameters to 0x0000:0x7BFC
; ****************************************************************************
        xor   ax,ax         ; ax = 0
        mov   ss,ax         ; ss = 0
        add   ax,1000h
        mov   ds,ax

        mov   sp,07c00h     ; sp = 0x7c00
        mov   bp,sp         ; bp = 0x7c00

        mov   ah,8                                ; ah = 8 - Get Drive Parameters Function
        mov   byte ptr [bp+PhysicalDrive],dl      ; BBS defines that BIOS would pass the booting driver number to the loader through DL
        int   13h                                 ; Get Drive Parameters
        xor   ax,ax                   ; ax = 0
        mov   al,dh                   ; al = dh
        inc   al                      ; MaxHead = al + 1
        push  ax                      ; 0000:7bfe = MaxHead
        mov   al,cl                   ; al = cl
        and   al,03fh                 ; MaxSector = al & 0x3f
        push  ax                      ; 0000:7bfc = MaxSector

; ****************************************************************************
; Read GPT Header from hard disk to 0x0000:0x0800
; ****************************************************************************
        xor     ax, ax
        mov     es, ax                            ; Read to 0x0000:0x0800
        mov     di, 0800h                         ; Read to 0x0000:0x0800
        mov     eax, 1                            ; Read LBA #1
        mov     edx, 0                            ; Read LBA #1
        mov     bx, 1                             ; Read 1 Block
        push    es
        call    ReadBlocks
        pop     es

; ****************************************************************************
; Read Target GPT Entry from hard disk to 0x0000:0x0A00
; ****************************************************************************
        cmp   dword ptr es:[di], 020494645h       ; Check for "EFI "
        jne   BadGpt
        cmp   dword ptr es:[di + 4], 054524150h   ; Check for "PART"
        jne   BadGpt
        cmp   dword ptr es:[di + 8], 000010000h   ; Check Revision - 0x10000
        jne   BadGpt

        mov   eax, dword ptr es:[di + 84]         ; EAX = SizeOfPartitionEntry
        mul   byte ptr [bp+GptPartitionIndicator] ; EAX = SizeOfPartitionEntry * GptPartitionIndicator
        mov   edx, eax                            ; EDX = SizeOfPartitionEntry * GptPartitionIndicator
        shr   eax, BLOCK_SHIFT                    ; EAX = (SizeOfPartitionEntry * GptPartitionIndicator) / BLOCK_SIZE
        and   edx, BLOCK_MASK                     ; EDX = Targer PartitionEntryLBA Offset
                                                  ;     = (SizeOfPartitionEntry * GptPartitionIndicator) % BLOCK_SIZE
        push  edx
        mov   ecx, dword ptr es:[di + 72]         ; ECX = PartitionEntryLBA (Low)
        mov   ebx, dword ptr es:[di + 76]         ; EBX = PartitionEntryLBA (High)
        add   eax, ecx                            ; EAX = Target PartitionEntryLBA (Low)
                                                  ;     = (PartitionEntryLBA + 
                                                  ;        (SizeOfPartitionEntry * GptPartitionIndicator) / BLOCK_SIZE)
        adc   edx, ebx                            ; EDX = Target PartitionEntryLBA (High)

        mov   di, 0A00h                           ; Read to 0x0000:0x0A00
        mov   bx, 1                               ; Read 1 Block
        push  es
        call  ReadBlocks
        pop   es

; ****************************************************************************
; Read Target DBR from hard disk to 0x0000:0x7C00
; ****************************************************************************
        pop   edx                                 ; EDX = (SizeOfPartitionEntry * GptPartitionIndicator) % BLOCK_SIZE
        add   di, dx                              ; DI = Targer PartitionEntryLBA Offset
        cmp   dword ptr es:[di], 0C12A7328h       ; Check for EFI System Partition "C12A7328-F81F-11d2-BA4B-00A0C93EC93B"
        jne   BadGpt
        cmp   dword ptr es:[di + 4], 011d2F81Fh   ; 
        jne   BadGpt
        cmp   dword ptr es:[di + 8], 0A0004BBAh   ; 
        jne   BadGpt
        cmp   dword ptr es:[di + 0ch], 03BC93EC9h ; 
        jne   BadGpt

        mov   eax, dword ptr es:[di + 32]         ; EAX = StartingLBA (Low)
        mov   edx, dword ptr es:[di + 36]         ; EDX = StartingLBA (High)
        mov   di, 07C00h                          ; Read to 0x0000:0x7C00
        mov   bx, 1                               ; Read 1 Block
        call  ReadBlocks

; ****************************************************************************
; Transfer control to BootSector - Jump to 0x0000:0x7C00
; ****************************************************************************
        xor   ax, ax
        push  ax                        ; PUSH 0x0000
        mov   di, 07c00h
        push  di                        ; PUSH 0x7C00
        retf                            ; JMP 0x0000:0x7C00

; ****************************************************************************
; ReadBlocks - Reads a set of blocks from a block device
;
; EDX:EAX = Start LBA
; BX      = Number of Blocks to Read (must < 127)
; ES:DI   = Buffer to store sectors read from disk
; ****************************************************************************

; si = DiskAddressPacket

ReadBlocks:
        pushad
        push  ds
        xor   cx, cx
        mov   ds, cx
        mov   bp, 0600h                         ; bp = 0x600
        lea   si, [bp + OFFSET AddressPacket]   ; DS:SI = Disk Address Packet
        mov   BYTE PTR ds:[si+2],bl             ;    02 = Number Of Block transfered
        mov   WORD PTR ds:[si+4],di             ;    04 = Transfer Buffer Offset
        mov   WORD PTR ds:[si+6],es             ;    06 = Transfer Buffer Segment
        mov   DWORD PTR ds:[si+8],eax           ;    08 = Starting LBA (Low)
        mov   DWORD PTR ds:[si+0ch],edx         ;    0C = Starting LBA (High)
        mov   ah, 42h                           ; ah = Function 42
        mov   dl,byte ptr [bp+PhysicalDrive]    ; dl = Drive Number
        int   13h
        jc    BadGpt
        pop   ds
        popad
        ret

; ****************************************************************************
; Address Packet used by ReadBlocks
; ****************************************************************************
AddressPacket:
        db    10h                       ; Size of address packet
        db    00h                       ; Reserved.  Must be 0
        db    01h                       ; Read blocks at a time (To be fixed each times)
        db    00h                       ; Reserved.  Must be 0
        dw    0000h                     ; Destination Address offset (To be fixed each times)
        dw    0000h                     ; Destination Address segment (To be fixed each times)
AddressPacketLba:
        dd    0h, 0h                    ; Start LBA (To be fixed each times)
AddressPacketEnd:

; ****************************************************************************
; ERROR Condition:
; ****************************************************************************

BadGpt:
    mov  ax,0b800h
    mov  es,ax
    mov  ax, 060h
    mov  ds, ax
    lea  si, cs:[ErrorString]
    mov  cx, 10
    mov  di, 320
    rep  movsw 
Halt:
    jmp   Halt

StartString:
    db 'G', 0ch, 'P', 0ch, 'T', 0ch, ' ', 0ch, 'S', 0ch, 't', 0ch, 'a', 0ch, 'r', 0ch, 't', 0ch, '!', 0ch
ErrorString:
    db 'G', 0ch, 'P', 0ch, 'T', 0ch, ' ', 0ch, 'E', 0ch, 'r', 0ch, 'r', 0ch, 'o', 0ch, 'r', 0ch, '!', 0ch

; ****************************************************************************
; PhysicalDrive - Used to indicate which disk to be boot
;                 Can be patched by tool
; ****************************************************************************
    org   01B6h
PhysicalDrive         db  80h

; ****************************************************************************
; GptPartitionIndicator - Used to indicate which GPT partition to be boot
;                         Can be patched by tool
; ****************************************************************************
    org   01B7h
GptPartitionIndicator db 0

; ****************************************************************************
; Unique MBR signature
; ****************************************************************************
    org   01B8h
    db 'DUET'

; ****************************************************************************
; Unknown
; ****************************************************************************
    org   01BCh
    dw 0

; ****************************************************************************
; PMBR Entry - Can be patched by tool
; ****************************************************************************
    org   01BEh
    db 0          ; Boot Indicator
    db 0ffh       ; Start Header
    db 0ffh       ; Start Sector
    db 0ffh       ; Start Track
    db 0eeh       ; OS Type
    db 0ffh       ; End Header
    db 0ffh       ; End Sector
    db 0ffh       ; End Track
    dd 1          ; Starting LBA
    dd 0FFFFFFFFh ; End LBA

    org   01CEh
    dd 0, 0, 0, 0
    org   01DEh
    dd 0, 0, 0, 0
    org   01EEh
    dd 0, 0, 0, 0

; ****************************************************************************
; Sector Signature
; ****************************************************************************

  org 01FEh
SectorSignature:
  dw        0aa55h      ; Boot Sector Signature

  end 
  

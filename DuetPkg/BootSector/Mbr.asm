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
;*    Mbr.asm
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
        xor   ax, ax                              ; AX = 0x0000
        mov   bx, 07c00h                          ; BX = 0x7C00
        mov   bp, 0600h                           ; BP = 0x0600
        mov   si, OFFSET RelocatedStart           ; SI = Offset(RelocatedStart)
        mov   cx, 0200h                           ; CX = 0x0200
        sub   cx, si                              ; CS = 0x0200 - Offset(RelocatedStart)
        lea   di, [bp+si]                         ; DI = 0x0600 + Offset(RelocatedStart)
        lea   si, [bx+si]                         ; BX = 0x7C00 + Offset(RelocatedStart)
        mov   ss, ax                              ; SS = 0x0000
        mov   sp, bx                              ; SP = 0x7C00
        mov   es,ax                               ; ES = 0x0000
        mov   ds,ax                               ; DS = 0x0000
        push  ax                                  ; PUSH 0x0000
        push  di                                  ; PUSH 0x0600 + Offset(RelocatedStart)
        cld                                       ; Clear the direction flag
        rep   movsb                               ; Copy 0x0200 bytes from 0x7C00 to 0x0600
        retf                                      ; JMP 0x0000:0x0600 + Offset(RelocatedStart)

; ****************************************************************************
; Code relocated to 0x0000:0x0600
; ****************************************************************************

RelocatedStart:
; ****************************************************************************
; Get Driver Parameters to 0x0000:0x7BFC
; ****************************************************************************

        xor   ax,ax                               ; AX = 0
        mov   ss,ax                               ; SS = 0
        add   ax,1000h
        mov   ds,ax

        mov   sp,07c00h                           ; SP = 0x7c00
        mov   bp,sp                               ; BP = 0x7c00

        mov   ah,8                                ; AH = 8 - Get Drive Parameters Function
        mov   byte ptr [bp+PhysicalDrive],dl      ; BBS defines that BIOS would pass the booting driver number to the loader through DL
        int   13h                                 ; Get Drive Parameters
        xor   ax,ax                               ; AX = 0
        mov   al,dh                               ; AL = DH
        inc   al                                  ; MaxHead = AL + 1
        push  ax                                  ; 0000:7bfe = MaxHead
        mov   al,cl                               ; AL = CL
        and   al,03fh                             ; MaxSector = AL & 0x3f
        push  ax                                  ; 0000:7bfc = MaxSector

; ****************************************************************************
; Read Target DBR from hard disk to 0x0000:0x7C00
; ****************************************************************************

        xor   ax, ax
        mov   al, byte ptr [bp+MbrPartitionIndicator]  ; AX = MbrPartitionIndex
        cmp   al, 0ffh                                 ; 0xFF means do legacy MBR boot
        jnz   EfiDbr
LegacyMbr:
        mov   eax, 00000600h                      ; Assume LegacyMBR is backuped in Sector 6
        jmp   StartReadTo7C00                     ; EAX = Header/Sector/Tracker/Zero

EfiDbr:
        cmp   al, 4                               ; MbrPartitionIndex should < 4
        jae   BadDbr
        shl   ax, 4                               ; AX  = MBREntrySize * Index
        add   ax, 1beh                            ; AX  = MBREntryOffset
        mov   di, ax                              ; DI  = MBREntryOffset

        ; Here we don't use the C/H/S information provided by Partition table
        ;  but calculate C/H/S from LBA ourselves
        ;       Ci: Cylinder number
        ;       Hi: Header number
        ;       Si: Sector number
        mov   eax, dword ptr es:[bp + di + 8]     ; Start LBA
        mov   edx, eax
        shr   edx, 16                             ; DX:AX = Start LBA
                                                  ;       = Ci * (H * S) + Hi * S + (Si - 1)

        ; Calculate C/H/S according to LBA
        mov   bp, 7bfah
        div   word ptr [bp+2]                     ; AX = Hi + H*Ci
                                                  ; DX = Si - 1
        inc   dx                                  ; DX = Si
        push  dx                                  ; 0000:7bfa = Si  <----
        xor   dx, dx                              ; DX:AX = Hi + H*Ci
        div   word ptr [bp+4]                     ; AX = Ci         <----
                                                  ; DX = Hi         <----

StartReadTo7C00:

        mov   cl, byte ptr [bp]                   ; Si
        mov   ch, al                              ; Ci[0-7]
        or    cl, ah                              ; Ci[8,9]
        mov   bx, 7c00h                           ; ES:BX = 0000:7C00h
        mov   ah, 2h                              ; Function 02h
        mov   al, 1                               ; 1 Sector
        mov   dh, dl                              ; Hi
        mov   bp, 0600h
        mov   dl, byte ptr [bp + PhysicalDrive]   ; Drive number
        int   13h
        jc    BadDbr



; ****************************************************************************
; Transfer control to BootSector - Jump to 0x0000:0x7C00
; ****************************************************************************
        xor   ax, ax
        push  ax                                  ; PUSH 0x0000 - Segment
        mov   di, 07c00h
        push  di                                  ; PUSH 0x7C00 - Offset
        retf                                      ; JMP 0x0000:0x7C00

; ****************************************************************************
; ERROR Condition:
; ****************************************************************************

BadDbr:
    push ax
    mov  ax, 0b800h
    mov  es, ax
    mov  ax, 060h
    mov  ds, ax
    lea  si, cs:[ErrorString]
    mov  di, 320
    pop  ax
    call A2C
    mov  [si+16], ah
    mov  [si+18], al
    mov  cx, 10
    rep  movsw
Halt:
    jmp   Halt

StartString:
    db 'M', 0ch, 'B', 0ch, 'R', 0ch, ' ', 0ch, 'S', 0ch, 't', 0ch, 'a', 0ch, 'r', 0ch, 't', 0ch, '!', 0ch
ErrorString:
    db 'M', 0ch, 'B', 0ch, 'R', 0ch, ' ', 0ch, 'E', 0ch, 'r', 0ch, 'r', 0ch, ':', 0ch, '?', 0ch, '?', 0ch

; ****************************************************************************
; A2C - convert Ascii code stored in AH to character stored in AX
; ****************************************************************************
A2C:
    mov  al, ah
    shr  ah, 4
    and  al, 0Fh
    add  ah, '0'
    add  al, '0'

    cmp  ah, '9'
    jle  @f
    add  ah, 7
@@:

    cmp al, '9'
    jle @f
    add al, 7
@@:
    ret


; ****************************************************************************
; PhysicalDrive - Used to indicate which disk to be boot
;                 Can be patched by tool
; ****************************************************************************
    org   01B6h
PhysicalDrive         db  80h

; ****************************************************************************
; MbrPartitionIndicator - Used to indicate which MBR partition to be boot
;                         Can be patched by tool
;                         OxFF means boot to legacy MBR. (LBA OFFSET 6)
; ****************************************************************************
    org   01B7h
MbrPartitionIndicator db 0

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
; MBR Entry - To be patched
; ****************************************************************************
    org   01BEh
    dd 0, 0, 0, 0
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


#------------------------------------------------------------------------------
#*
#*   Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
#*   This program and the accompanying materials
#*   are licensed and made available under the terms and conditions of the BSD License
#*   which accompanies this distribution.  The full text of the license may be found at
#*   http://opensource.org/licenses/bsd-license.php
#*
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#*
#*    Mbr.asm
#*
#*   Abstract:
#*
#------------------------------------------------------------------------------

    .code16 

.equ                      BLOCK_SIZE, 0x0200
.equ                      BLOCK_MASK, 0x01ff
.equ                      BLOCK_SHIFT, 9

# ****************************************************************************
# Code loaded by BIOS at 0x0000:0x7C00
# ****************************************************************************

.org 0x0

.global _start
_start:

# ****************************************************************************
# Start Print
# ****************************************************************************

        movw $0xb800, %ax
        movw %ax, %es
        movw $0x7c0, %ax
        movw %ax, %ds
        leaw %cs:StartString, %si
        movw $10, %cx
        movw $160, %di
        rep 
        movsw

# ****************************************************************************
# Print over
# ****************************************************************************

# ****************************************************************************
# Initialize segment registers and copy code at 0x0000:0x7c00 to 0x0000:0x0600
# ****************************************************************************
        xorw  %ax, %ax                            # AX = 0x0000
        movw  $0x7c00, %bx                        # BX = 0x7C00
        movw  $0x600, %bp                         # BP = 0x0600
        movw  $RelocatedStart, %si                # SI = Offset(RelocatedStart)
        movw  $0x200, %cx                         # CX = 0x0200
        subw  %si, %cx                            # CS = 0x0200 - Offset(RelocatedStart)
        leaw  (%bp,%si,), %di                     # DI = 0x0600 + Offset(RelocatedStart)
        leaw  (%bx,%si,), %si                     # BX = 0x7C00 + Offset(RelocatedStart)
        movw  %ax, %ss                            # SS = 0x0000
        movw  %bx, %sp                            # SP = 0x7C00
        movw  %ax, %es                            # ES = 0x0000
        movw  %ax, %ds                            # DS = 0x0000
        pushw %ax                                 # PUSH 0x0000
        pushw %di                                 # PUSH 0x0600 + Offset(RelocatedStart)
        cld                                       # Clear the direction flag
        rep
        movsb                                     # Copy 0x0200 bytes from 0x7C00 to 0x0600
        retl                                      # JMP 0x0000:0x0600 + Offset(RelocatedStart)

# ****************************************************************************
# Code relocated to 0x0000:0x0600
# ****************************************************************************

RelocatedStart: 
# ****************************************************************************
# Get Driver Parameters to 0x0000:0x7BFC
# ****************************************************************************

        xorw  %ax, %ax                            # AX = 0
        movw  %ax, %ss                            # SS = 0
        addw  $0x1000, %ax
        movw  %ax, %ds

        movw  $0x7c00, %sp                        # SP = 0x7c00
        movw  %sp, %bp                            # BP = 0x7c00

        movb  $8, %ah                             # AH = 8 - Get Drive Parameters Function
        movb  %dl, PhysicalDrive(%bp)             # BBS defines that BIOS would pass the booting driver number to the loader through DL
        int   $0x13                               # Get Drive Parameters
        xorw  %ax, %ax                            # AX = 0
        movb  %dh, %al                            # AL = DH
        incb  %al                                 # MaxHead = AL + 1
        pushw %ax                                 # 0000:7bfe = MaxHead
        movb  %cl, %al                            # AL = CL
        andb  $0x3f, %al                          # MaxSector = AL & 0x3f
        pushw %ax                                 # 0000:7bfc = MaxSector

# ****************************************************************************
# Read Target DBR from hard disk to 0x0000:0x7C00
# ****************************************************************************

        xorw  %ax, %ax
        movb  MbrPartitionIndicator(%bp), %al          # AX = MbrPartitionIndex
        cmpb  $0xff, %al                               # 0xFF means do legacy MBR boot
        jnz   EfiDbr
LegacyMbr: 
        movl  $0x0000600, %eax                    # Assume LegacyMBR is backuped in Sector 6
        jmp   StartReadTo7C00                     # EAX = Header/Sector/Tracker/Zero

EfiDbr: 
        cmpb  $4, %al                             # MbrPartitionIndex should < 4
        jae   BadDbr
        shlw  $4, %ax                             # AX  = MBREntrySize * Index
        addw  $0x1be, %ax                         # AX  = MBREntryOffset
        movw  %ax, %di                            # DI  = MBREntryOffset

        # Here we don't use the C/H/S information provided by Partition table
        #  but calculate C/H/S from LBA ourselves
        #       Ci: Cylinder number
        #       Hi: Header number
        #       Si: Sector number
        movl  %es:8(%bp,%di,), %eax               # Start LBA
        movl  %eax, %edx
        shrl  $16, %edx                           # DX:AX = Start LBA
                                                  #       = Ci * (H * S) + Hi * S + (Si - 1)

        # Calculate C/H/S according to LBA
        movw  $0x7bfa, %bp
        divw  2(%bp)                              # AX = Hi + H*Ci
                                                  # DX = Si - 1
        incw  %dx                                 # DX = Si
        pushw %dx                                 # 0000:7bfa = Si  <----
        xorw  %dx, %dx                            # DX:AX = Hi + H*Ci
        divw  4(%bp)                              # AX = Ci         <----
                                                  # DX = Hi         <----

StartReadTo7C00: 

        movb  (%bp), %cl                          # Si
        movb  %al, %ch                            # Ci[0-7]
        orb   %ah, %cl                            # Ci[8,9]
        movw  $0x7c00, %bx                        # ES:BX = 0000:7C00h
        movb  $0x2, %ah                           # Function 02h
        movb  $1, %al                             # 1 Sector
        movb  %dl, %dh                            # Hi
        movw  $0x600, %bp
        movb  PhysicalDrive(%bp), %dl             # Drive number
        int   $0x13
        jc    BadDbr



# ****************************************************************************
# Transfer control to BootSector - Jump to 0x0000:0x7C00
# ****************************************************************************
        xorw  %ax, %ax
        pushw %ax                                 # PUSH 0x0000 - Segment
        movw  $0x7c00, %di
        pushw %di                                 # PUSH 0x7C00 - Offset
        retl                                      # JMP 0x0000:0x7C00

# ****************************************************************************
# ERROR Condition:
# ****************************************************************************

BadDbr: 
    pushw %ax
    movw $0xb800, %ax
    movw %ax, %es
    movw $0x60, %ax
    movw %ax, %ds
    leaw %cs:ErrorString, %si
    movw $320, %di
    popw %ax
    call A2C
    movb %ah, 16(%si)
    movb %al, 18(%si)
    movw $10, %cx
    rep
    movsw
Halt: 
    jmp   Halt

StartString: 
.byte 'M', 0x0c, 'B', 0x0c, 'R', 0x0c, ' ', 0x0c, 'S', 0x0c, 't', 0x0c, 'a', 0x0c, 'r', 0x0c, 't', 0x0c, '!', 0x0c
ErrorString: 
.byte 'M', 0x0c, 'B', 0x0c, 'R', 0x0c, ' ', 0x0c, 'E', 0x0c, 'r', 0x0c, 'r', 0x0c, ':', 0x0c, '?', 0x0c, '?', 0x0c

# ****************************************************************************
# A2C - convert Ascii code stored in AH to character stored in AX
# ****************************************************************************
A2C: 
    movb %ah, %al
    shrb $4, %ah
    andb $0xF, %al
    addb '0', %ah
    addb '0', %al

    cmpb '9', %ah
    jle  A2C_L1
    addb $7, %ah
A2C_L1: 

    cmpb '9', %al
    jle A2C_L2
    addb $7, %al
A2C_L2: 
    ret


# ****************************************************************************
# PhysicalDrive - Used to indicate which disk to be boot
#                 Can be patched by tool
# ****************************************************************************
.org   0x01B6
PhysicalDrive:        .byte 0x80

# ****************************************************************************
# MbrPartitionIndicator - Used to indicate which MBR partition to be boot
#                         Can be patched by tool
#                         OxFF means boot to legacy MBR. (LBA OFFSET 6)
# ****************************************************************************
.org   0x01B7
MbrPartitionIndicator: .byte 0

# ****************************************************************************
# Unique MBR signature
# ****************************************************************************
.org   0x01B8
    .ascii "DUET"

# ****************************************************************************
# Unknown
# ****************************************************************************
.org   0x01BC
    .word 0

# ****************************************************************************
# MBR Entry - To be patched
# ****************************************************************************
.org   0x01BE
    .long 0,0,0,0
.org   0x01CE
    .long 0,0,0,0
.org   0x01DE
    .long 0,0,0,0
.org   0x01EE
    .long 0,0,0,0

# ****************************************************************************
# Sector Signature
# ****************************************************************************

.org 0x01FE
SectorSignature: 
  .word     0xaa55      # Boot Sector Signature




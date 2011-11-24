#------------------------------------------------------------------------------
#*
#*   Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
#*   This program and the accompanying materials                          
#*   are licensed and made available under the terms and conditions of the BSD License         
#*   which accompanies this distribution.  The full text of the license may be found at        
#*   http://opensource.org/licenses/bsd-license.php                                            
#*                                                                                             
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#*   
#*    gpt.asm
#*  
#*   Abstract:
#*
#------------------------------------------------------------------------------

    #.MODEL small
#   .dosseg
    .stack: 
    .486p: 
    .code16 

.equ                      BLOCK_SIZE,  0x0200
.equ                      BLOCK_MASK,  0x01ff
.equ                      BLOCK_SHIFT, 9

# ****************************************************************************
# Code loaded by BIOS at 0x0000:0x7C00
# ****************************************************************************

        .org 0x00
Start: 

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
        xorw  %ax, %ax                  # AX = 0x0000  
        movw  $0x7c00, %bx              # BX = 0x7C00
        movw  $0x600, %bp               # BP = 0x0600
        movw  RelocatedStart, %si       # SI = Offset(RelocatedStart)
        movw  $0x200, %cx               # CX = 0x0200
        subw  %si, %cx                  # CS = 0x0200 - Offset(RelocatedStart)
        leaw  (%bp,%si,), %di           # DI = 0x0600 + Offset(RelocatedStart)
        leaw  (%bx,%si,), %si           # BX = 0x7C00 + Offset(RelocatedStart)
        movw  %ax, %ss                  # SS = 0x0000
        movw  %bx, %sp                  # SP = 0x7C00
        movw  %ax, %es                  # ES = 0x0000
        movw  %ax, %ds                  # DS = 0x0000
        pushw %ax                       # PUSH 0x0000
        pushw %di                       # PUSH 0x0600 + Offset(RelocatedStart)
        cld                             # Clear the direction flag
        rep
        movsb                           # Copy 0x0200 bytes from 0x7C00 to 0x0600
        retl                            # JMP 0x0000:0x0600 + Offset(RelocatedStart)

# ****************************************************************************
# Code relocated to 0x0000:0x0600
# ****************************************************************************

RelocatedStart: 
# ****************************************************************************
# Get Driver Parameters to 0x0000:0x7BFC
# ****************************************************************************
        xorw  %ax, %ax      # ax = 0
        movw  %ax, %ss      # ss = 0
        addw  $0x1000, %ax
        movw  %ax, %ds

        movw  $0x7c00, %sp  # sp = 0x7c00
        movw  %sp, %bp      # bp = 0x7c00

        movb  $8, %ah                             # ah = 8 - Get Drive Parameters Function
        movb  %dl, PhysicalDrive(%bp)             # BBS defines that BIOS would pass the booting driver number to the loader through DL
        int   $0x13                               # Get Drive Parameters
        xorw  %ax, %ax                # ax = 0
        movb  %dh, %al                # al = dh
        incb  %al                     # MaxHead = al + 1
        pushw %ax                     # 0000:7bfe = MaxHead
        movb  %cl, %al                # al = cl
        andb  $0x3f, %al              # MaxSector = al & 0x3f
        pushw %ax                     # 0000:7bfc = MaxSector

# ****************************************************************************
# Read GPT Header from hard disk to 0x0000:0x0800
# ****************************************************************************
        xorw    %ax, %ax
        movw    %ax, %es                          # Read to 0x0000:0x0800
        movw    $0x800, %di                       # Read to 0x0000:0x0800
        movl    $1, %eax                          # Read LBA #1
        movl    $0, %edx                          # Read LBA #1
        movw    $1, %bx                           # Read 1 Block
        pushw   %es
        call    ReadBlocks
        popw    %es

# ****************************************************************************
# Read Target GPT Entry from hard disk to 0x0000:0x0A00
# ****************************************************************************
        cmpl  $0x20494645, %es:(%di)              # Check for "EFI "
        jne   BadGpt
        cmpl  $0x54524150, %es:4(%di)             # Check for "PART"
        jne   BadGpt
        cmpl  $0x00010000, %es:8(%di)             # Check Revision - 0x10000
        jne   BadGpt

        movl  %es:84(%di), %eax                   # EAX = SizeOfPartitionEntry
        mulb  GptPartitionIndicator(%bp)          # EAX = SizeOfPartitionEntry * GptPartitionIndicator
        movl  %eax, %edx                          # EDX = SizeOfPartitionEntry * GptPartitionIndicator
        shrl  $BLOCK_SHIFT, %eax                  # EAX = (SizeOfPartitionEntry * GptPartitionIndicator) / BLOCK_SIZE
        andl  $BLOCK_MASK, %edx                   # EDX = Targer PartitionEntryLBA Offset
                                                  #     = (SizeOfPartitionEntry * GptPartitionIndicator) % BLOCK_SIZE
        pushl %edx
        movl  %es:72(%di), %ecx                   # ECX = PartitionEntryLBA (Low)
        movl  %es:76(%di), %ebx                   # EBX = PartitionEntryLBA (High)
        addl  %ecx, %eax                          # EAX = Target PartitionEntryLBA (Low)
                                                  #     = (PartitionEntryLBA + 
                                                  #        (SizeOfPartitionEntry * GptPartitionIndicator) / BLOCK_SIZE)
        adcl  %ebx, %edx                          # EDX = Target PartitionEntryLBA (High)

        movw  $0xA00, %di                         # Read to 0x0000:0x0A00
        movw  $1, %bx                             # Read 1 Block
        pushw %es
        call  ReadBlocks
        popw  %es

# ****************************************************************************
# Read Target DBR from hard disk to 0x0000:0x7C00
# ****************************************************************************
        popl  %edx                                # EDX = (SizeOfPartitionEntry * GptPartitionIndicator) % BLOCK_SIZE
        addw  %dx, %di                            # DI = Targer PartitionEntryLBA Offset
        cmpl  $0xC12A7328, %es:(%di)              # Check for EFI System Partition "C12A7328-F81F-11d2-BA4B-00A0C93EC93B"
        jne   BadGpt
        cmpl  $0x11d2F81F, %es:4(%di)             # 
        jne   BadGpt
        cmpl  $0xA0004BBA, %es:8(%di)             # 
        jne   BadGpt
        cmpl  $0x3BC93EC9, %es:0xc(%di)           # 
        jne   BadGpt

        movl  %es:32(%di), %eax                   # EAX = StartingLBA (Low)
        movl  %es:36(%di), %edx                   # EDX = StartingLBA (High)
        movw  $0x7C00, %di                        # Read to 0x0000:0x7C00
        movw  $1, %bx                             # Read 1 Block
        call  ReadBlocks

# ****************************************************************************
# Transfer control to BootSector - Jump to 0x0000:0x7C00
# ****************************************************************************
        xorw  %ax, %ax
        pushw %ax                       # PUSH 0x0000
        movw  $0x7c00, %di
        pushw %di                       # PUSH 0x7C00
        retl                            # JMP 0x0000:0x7C00

# ****************************************************************************
# ReadBlocks - Reads a set of blocks from a block device
#
# EDX:EAX = Start LBA
# BX      = Number of Blocks to Read (must < 127)
# ES:DI   = Buffer to store sectors read from disk
# ****************************************************************************

# si = DiskAddressPacket

ReadBlocks: 
        pushal
        pushw %ds
        xorw  %cx, %cx
        movw  %cx, %ds
        movw  $0x600, %bp                       # bp = 0x600
        leaw  AddressPacket(%bp), %si
        movb  %bl, %ds:2(%si)                   #    02 = Number Of Block transfered
        movw  %di, %ds:4(%si)                   #    04 = Transfer Buffer Offset
        movw  %es, %ds:6(%si)                   #    06 = Transfer Buffer Segment
        movl  %eax, %ds:8(%si)                  #    08 = Starting LBA (Low)
        movl  %edx, %ds:0xc(%si)                #    0C = Starting LBA (High)
        movb  $0x42, %ah                        # ah = Function 42
        movb  PhysicalDrive(%bp), %dl           # dl = Drive Number
        int   $0x13
        jc    BadGpt
        popw  %ds
        popal
        ret

# ****************************************************************************
# Address Packet used by ReadBlocks
# ****************************************************************************
AddressPacket: 
        .byte 0x10                      # Size of address packet
        .byte 0x0                       # Reserved.  Must be 0
        .byte 0x1                       # Read blocks at a time (To be fixed each times)
        .byte 0x0                       # Reserved.  Must be 0
        .word 0x000                     # Destination Address offset (To be fixed each times)
        .word 0x000                     # Destination Address segment (To be fixed each times)
AddressPacketLba: 
        .long 0x0,0x0                   # Start LBA (To be fixed each times)
AddressPacketEnd: 

# ****************************************************************************
# ERROR Condition:
# ****************************************************************************

BadGpt: 
    movw $0xb800, %ax
    movw %ax, %es
    movw $0x60, %ax
    movw %ax, %ds
    leaw %cs:ErrorString, %si
    movw $10, %cx
    movw $320, %di
    rep
    movsw
Halt: 
    jmp   Halt

StartString: 
    .byte 'G', 0x0c, 'P', 0x0c, 'T', 0x0c, ' ', 0x0c, 'S', 0x0c, 't', 0x0c, 'a', 0x0c, 'r', 0x0c, 't', 0x0c, '!', 0x0c
ErrorString: 
    .byte 'G', 0x0c, 'P', 0x0c, 'T', 0x0c, ' ', 0x0c, 'E', 0x0c, 'r', 0x0c, 'r', 0x0c, 'o', 0x0c, 'r', 0x0c, '!', 0x0c

# ****************************************************************************
# PhysicalDrive - Used to indicate which disk to be boot
#                 Can be patched by tool
# ****************************************************************************
    # .org   0x01B6 # Just for passing build.
PhysicalDrive:        .byte 0x80

# ****************************************************************************
# GptPartitionIndicator - Used to indicate which GPT partition to be boot
#                         Can be patched by tool
# ****************************************************************************
   # .org   0x01B7  # Just for passing build.
GptPartitionIndicator: .byte 0

# ****************************************************************************
# Unique MBR signature
# ****************************************************************************
   # .org   0x01B8  # Just for passing build.
    .ascii "DUET"

# ****************************************************************************
# Unknown
# ****************************************************************************
    # .org   0x01BC  # Just for passing build.
    .word 0

# ****************************************************************************
# PMBR Entry - Can be patched by tool
# ****************************************************************************
    # .org   0x01BE  # Just for passing build.
    .byte 0       # Boot Indicator
    .byte 0xff    # Start Header
    .byte 0xff    # Start Sector
    .byte 0xff    # Start Track
    .byte 0xee    # OS Type
    .byte 0xff    # End Header
    .byte 0xff    # End Sector
    .byte 0xff    # End Track
    .long 1       # Starting LBA
    .long 0xFFFFFFFF # End LBA

    # .org   0x01CE  # Just for passing build.
    .long  0,0,0,0
    # .org   0x01DE  # Just for passing build.
    .long  0,0,0,0
    # .org   0x01EE  # Just for passing build.
    .long 0,0,0,0

# ****************************************************************************
# Sector Signature
# ****************************************************************************

  # .org      0x01FE  # Just for passing build.
SectorSignature: 
  .word     0xaa55      # Boot Sector Signature




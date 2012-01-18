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
#*    st32_64.asm
#*  
#*   Abstract:
#*
#------------------------------------------------------------------------------


        .stack: 
        .486p: 
        .code16

.equ                        FAT_DIRECTORY_ENTRY_SIZE, 0x020
.equ                        FAT_DIRECTORY_ENTRY_SHIFT, 5
.equ                        BLOCK_SIZE, 0x0200
.equ                        BLOCK_MASK, 0x01ff
.equ                        BLOCK_SHIFT, 9

       .org 0x0

.global _start
_start:

Ia32Jump: 
  jmp   BootSectorEntryPoint  # JMP inst    - 3 bytes
  nop

OemId:               .ascii  "INTEL   "    # OemId                           - 8 bytes
SectorSize:          .word  0              # Sector Size                     - 2 bytes
SectorsPerCluster:   .byte  0              # Sector Per Cluster              - 1 byte
ReservedSectors:     .word  0              # Reserved Sectors                - 2 bytes
NoFats:              .byte  0              # Number of FATs                  - 1 byte
RootEntries:         .word  0              # Root Entries                    - 2 bytes
Sectors:             .word  0              # Number of Sectors               - 2 bytes
Media:               .byte  0              # Media                           - 1 byte
SectorsPerFat16:     .word  0              # Sectors Per FAT for FAT12/FAT16 - 2 byte
SectorsPerTrack:     .word  0              # Sectors Per Track               - 2 bytes
Heads:               .word  0              # Heads                           - 2 bytes
HiddenSectors:       .long  0              # Hidden Sectors                  - 4 bytes
LargeSectors:        .long  0              # Large Sectors                   - 4 bytes

#******************************************************************************
#
#The structure for FAT32 starting at offset 36 of the boot sector. (At this point, 
#the BPB/boot sector for FAT12 and FAT16 differs from the BPB/boot sector for FAT32.)
#
#******************************************************************************

SectorsPerFat32:     .long  0              # Sectors Per FAT for FAT32       - 4 bytes
ExtFlags:            .word  0              # Mirror Flag                     - 2 bytes
FSVersion:           .word  0              # File System Version             - 2 bytes
RootCluster:         .long  0              # 1st Cluster Number of Root Dir  - 4 bytes
FSInfo:              .word  0              # Sector Number of FSINFO         - 2 bytes
BkBootSector:        .word  0              # Sector Number of Bk BootSector  - 2 bytes
Reserved:            .fill 12,1,0          # Reserved Field                  - 12 bytes
PhysicalDrive:       .byte  0              # Physical Drive Number           - 1 byte
Reserved1:           .byte  0              # Reserved Field                  - 1 byte
Signature:           .byte  0              # Extended Boot Signature         - 1 byte
VolId:               .ascii  "    "        # Volume Serial Number            - 4 bytes
FatLabel:            .ascii  "           " # Volume Label                    - 11 bytes
FileSystemType:      .ascii  "FAT32   "    # File System Type                - 8 bytes


BootSectorEntryPoint: 
      # ASSUME  ds:@code
      # ASSUME  ss:@code
      # ds = 1000, es = 2000 + x (size of first cluster >> 4)
      # cx = Start Cluster of EfiLdr
      # dx = Start Cluster of Efivar.bin

# Re use the BPB data stored in Boot Sector
        movw    $0x7c00,%bp

        pushw   %cx
# Read Efivar.bin
#       1000:dx    = DirectoryEntry of Efivar.bin -> BS.com has filled already
        movw    $0x1900,%ax
        movw    %ax,%es
        testw   %dx,%dx
        jnz     CheckVarStoreSize

        movb    $1,%al
NoVarStore: 
        pushw   %es
# Set the 5th byte start @ 0:19000 to non-zero indicating we should init var store header in DxeIpl
        movb    %al, %es:(4)
        jmp     SaveVolumeId

CheckVarStoreSize: 
        movw    %dx,%di
        cmpl    $0x4000, %ds:2(%di)
        movb    $2,%al
        jne     NoVarStore

LoadVarStore: 
        movb    $0,%al
        movb    %al, %es:(4)
        movw    (%di), %cx
#       ES:DI = 1500:0
        xorw    %di,%di
        pushw   %es
        movw    $0x1500,%ax
        movw    %ax,%es
        call    ReadFile
SaveVolumeId: 
        popw    %es
        movw    VolId(%bp), %ax
        movw    %ax, %es:(0)                    # Save Volume Id to 0:19000. we will find the correct volume according to this VolumeId
        movw    VolId+2(%bp), %ax
        movw    %ax, %es:(2)

# Read Efildr
        popw    %cx
#       cx    = Start Cluster of Efildr -> BS.com has filled already
#       ES:DI = 2000:0, first cluster will be read again
        xorw    %di,%di                             # di = 0
        movw    $0x2000,%ax
        movw    %ax,%es
        call    ReadFile
        movw    %cs,%ax
        movw    %ax, %cs:JumpSegment

CheckEm64T: 
        movl $0x80000001,%eax
#        cpuid
        .word 0xA20F
        btl  $29,%edx
        jc   CheckEm64TPass
        pushw %cs
        popw %ds
        leaw Em64String,%si
        movw $18,%cx
        jmp  PrintStringAndHalt
CheckEm64TPass: 
JumpFarInstruction: 
        .byte 0xea
JumpOffset: 
        .word 0x200
JumpSegment: 
        .word 0x2000



# ****************************************************************************
# ReadFile
#
# Arguments:
#   CX    = Start Cluster of File
#   ES:DI = Buffer to store file content read from disk
#
# Return:
#   (ES << 4 + DI) = end of file content Buffer
#
# ****************************************************************************
ReadFile: 
# si      = NumberOfClusters
# cx      = ClusterNumber
# dx      = CachedFatSectorNumber
# ds:0000 = CacheFatSectorBuffer
# es:di   = Buffer to load file
# bx      = NextClusterNumber
        pusha
        movw    $1,%si                              # NumberOfClusters = 1
        pushw   %cx                                 # Push Start Cluster onto stack
        movw    $0xfff,%dx                          # CachedFatSectorNumber = 0xfff
FatChainLoop: 
        movw    %cx,%ax                             # ax = ClusterNumber    
        andw    $0xfff8,%ax                         # ax = ax & 0xfff8
        cmpw    $0xfff8,%ax                         # See if this is the last cluster
        je      FoundLastCluster                    # Jump if last cluster found
        movw    %cx,%ax                             # ax = ClusterNumber
        shlw    $2, %ax                             # FatOffset = ClusterNumber * 2
        pushw   %si                                 # Save si
        movw    %ax,%si                             # si = FatOffset
        shrw    $BLOCK_SHIFT, %ax                   # ax = FatOffset >> BLOCK_SHIFT
        addw    ReservedSectors(%bp), %ax           # ax = FatSectorNumber = ReservedSectors + (FatOffset >> BLOCK_OFFSET)
        andw    $BLOCK_MASK, %si                    # si = FatOffset & BLOCK_MASK
        cmpw    %dx,%ax                             # Compare FatSectorNumber to CachedFatSectorNumber
        je      SkipFatRead
        movw    $2,%bx
        pushw   %es
        pushw   %ds
        popw    %es
        call    ReadBlocks                          # Read 2 blocks starting at AX storing at ES:DI
        popw    %es
        movw    %ax,%dx                             # CachedFatSectorNumber = FatSectorNumber
SkipFatRead: 
        movw    (%si), %bx                          # bx = NextClusterNumber
        movw    %cx,%ax                             # ax = ClusterNumber
        popw    %si                                 # Restore si
        decw    %bx                                 # bx = NextClusterNumber - 1
        cmpw    %cx,%bx                             # See if (NextClusterNumber-1)==ClusterNumber
        jne     ReadClusters
        incw    %bx                                 # bx = NextClusterNumber
        incw    %si                                 # NumberOfClusters++
        movw    %bx,%cx                             # ClusterNumber = NextClusterNumber
        jmp     FatChainLoop
ReadClusters: 
        incw    %bx
        popw    %ax                                 # ax = StartCluster
        pushw   %bx                                 # StartCluster = NextClusterNumber
        movw    %bx,%cx                             # ClusterNumber = NextClusterNumber
        subw    $2,%ax                              # ax = StartCluster - 2
        xorb    %bh,%bh
        movb    SectorsPerCluster(%bp), %bl         # bx = SectorsPerCluster
        mulw    %bx                                 # ax = (StartCluster - 2) * SectorsPerCluster
        addw    (%bp), %ax                          # ax = FirstClusterLBA + (StartCluster-2)*SectorsPerCluster
        pushw   %ax                                 # save start sector
        movw    %si,%ax                             # ax = NumberOfClusters
        mulw    %bx                                 # ax = NumberOfClusters * SectorsPerCluster
        movw    %ax,%bx                             # bx = Number of Sectors
        popw    %ax                                 # ax = Start Sector
        call    ReadBlocks
        movw    $1,%si                              # NumberOfClusters = 1
        jmp     FatChainLoop
FoundLastCluster: 
        popw    %cx
        popa
        ret


# ****************************************************************************
# ReadBlocks - Reads a set of blocks from a block device
#
# AX    = Start LBA
# BX    = Number of Blocks to Read
# ES:DI = Buffer to store sectors read from disk
# ****************************************************************************

# cx = Blocks
# bx = NumberOfBlocks
# si = StartLBA

ReadBlocks: 
        pusha
        addl    LBAOffsetForBootSector(%bp), %eax   # Add LBAOffsetForBootSector to Start LBA
        addl    HiddenSectors(%bp), %eax            # Add HiddenSectors to Start LBA
        movl    %eax,%esi                           # esi = Start LBA
        movw    %bx,%cx                             # cx = Number of blocks to read
ReadCylinderLoop: 
        movw    $0x7bfc,%bp                         # bp = 0x7bfc
        movl    %esi,%eax                           # eax = Start LBA
        xorl    %edx,%edx                           # edx = 0
        movzwl  (%bp), %ebx                         # bx = MaxSector
        divl    %ebx                                # ax = StartLBA / MaxSector
        incw    %dx                                 # dx = (StartLBA % MaxSector) + 1

        movw    (%bp), %bx                          # bx = MaxSector
        subw    %dx,%bx                             # bx = MaxSector - Sector
        incw    %bx                                 # bx = MaxSector - Sector + 1
        cmpw    %bx,%cx                             # Compare (Blocks) to (MaxSector - Sector + 1)
        jg      LimitTransfer
        movw    %cx,%bx                             # bx = Blocks
LimitTransfer: 
        pushw   %ax                                 # save ax
        movw    %es,%ax                             # ax = es
        shrw    $(BLOCK_SHIFT-4), %ax               # ax = Number of blocks into mem system
        andw    $0x7f,%ax                           # ax = Number of blocks into current seg
        addw    %bx,%ax                             # ax = End Block number of transfer
        cmpw    $0x80,%ax                           # See if it crosses a 64K boundry
        jle     NotCrossing64KBoundry               # Branch if not crossing 64K boundry
        subw    $0x80,%ax                           # ax = Number of blocks past 64K boundry
        subw    %ax,%bx                             # Decrease transfer size by block overage
NotCrossing64KBoundry: 
        popw    %ax                                 # restore ax

        pushw   %cx
        movb    %dl,%cl                             # cl = (StartLBA % MaxSector) + 1 = Sector
        xorw    %dx,%dx                             # dx = 0
        divw    2(%bp)                              # ax = ax / (MaxHead + 1) = Cylinder  
                                                    # dx = ax % (MaxHead + 1) = Head

        pushw   %bx                                 # Save number of blocks to transfer
        movb    %dl,%dh                             # dh = Head
        movw    $0x7c00,%bp                         # bp = 0x7c00
        movb    PhysicalDrive(%bp), %dl             # dl = Drive Number
        movb    %al,%ch                             # ch = Cylinder
        movb    %bl,%al                             # al = Blocks
        movb    $2,%ah                              # ah = Function 2
        movw    %di,%bx                             # es:bx = Buffer address
        int     $0x13
        jc      DiskError
        popw    %bx
        popw    %cx
        movzwl  %bx,%ebx
        addl    %ebx,%esi                           # StartLBA = StartLBA + NumberOfBlocks
        subw    %bx,%cx                             # Blocks = Blocks - NumberOfBlocks
        movw    %es,%ax
        shlw    $(BLOCK_SHIFT-4), %bx
        addw    %bx,%ax
        movw    %ax,%es                             # es:di = es:di + NumberOfBlocks*BLOCK_SIZE
        cmpw    $0,%cx
        jne     ReadCylinderLoop
        popa
        ret

DiskError: 
        pushw %cs
        popw %ds
        leaw ErrorString,%si
        movw $7,%cx
        jmp  PrintStringAndHalt

PrintStringAndHalt: 
        movw $0xb800,%ax
        movw %ax,%es
        movw $160,%di
        rep
        movsw
Halt: 
        jmp   Halt

ErrorString: 
        .byte 'S', 0x0c, 'E', 0x0c, 'r', 0x0c, 'r', 0x0c, 'o', 0x0c, 'r', 0x0c, '!',0x0c

        .org     0x01fa
LBAOffsetForBootSector: 
        .long 0x0

        .org    0x01fe
        .word 0xaa55

#******************************************************************************
#******************************************************************************
#******************************************************************************

.equ                 DELAY_PORT, 0x0ed # Port to use for 1uS delay
.equ                 KBD_CONTROL_PORT, 0x060 # 8042 control port     
.equ                 KBD_STATUS_PORT, 0x064 # 8042 status port      
.equ                 WRITE_DATA_PORT_CMD, 0x0d1 # 8042 command to write the data port
.equ                 ENABLE_A20_CMD, 0x0df # 8042 command to enable A20

        .org     0x200
        jmp start
Em64String: 
.byte 'E', 0x0c, 'm', 0x0c, '6', 0x0c, '4', 0x0c, 'T', 0x0c, ' ', 0x0c, 'U', 0x0c, 'n', 0x0c, 's', 0x0c, 'u', 0x0c, 'p', 0x0c, 'p', 0x0c, 'o', 0x0c, 'r', 0x0c, 't', 0x0c, 'e', 0x0c, 'd', 0x0c, '!', 0x0c

start:  
        movw %cs,%ax
        movw %ax,%ds
        movw %ax,%es
        movw %ax,%ss
        movw $MyStack, %sp

#        mov ax,0b800h
#        mov es,ax
#        mov byte ptr es:[160],'a'
#        mov ax,cs
#        mov es,ax

        movl $0,%ebx
        leal MemoryMap, %edi
MemMapLoop: 
        movl $0xe820,%eax
        movl $20,%ecx
        movl $0x534d4150, %edx  # SMAP
        int $0x15
        jc  MemMapDone
        addl $20,%edi
        cmpl $0,%ebx
        je  MemMapDone
        jmp MemMapLoop
MemMapDone: 
        leal MemoryMap, %eax
        subl %eax,%edi                      # Get the address of the memory map
        movl %edi, MemoryMapSize            # Save the size of the memory map

        xorl    %ebx,%ebx
        movw    %cs,%bx                     # BX=segment
        shll    $4,%ebx                     # BX="linear" address of segment base
        leal    GDT_BASE(%ebx), %eax        # EAX=PHYSICAL address of gdt
        movl    %eax, (gdtr + 2)            # Put address of gdt into the gdtr
        leal    IDT_BASE(%ebx), %eax        # EAX=PHYSICAL address of idt
        movl    %eax, (idtr + 2)            # Put address of idt into the idtr
        leal    MemoryMapSize(%ebx), %edx   # Physical base address of the memory map

#        mov ax,0b800h
#        mov es,ax
#        mov byte ptr es:[162],'b'
#        mov ax,cs
#        mov es,ax

#
# Enable A20 Gate 
#

        movw $0x2401,%ax                    # Enable A20 Gate
        int $0x15
        jnc A20GateEnabled                  # Jump if it suceeded

#
# If INT 15 Function 2401 is not supported, then attempt to Enable A20 manually.
#

        call    Empty8042InputBuffer        # Empty the Input Buffer on the 8042 controller
        jnz     Timeout8042                 # Jump if the 8042 timed out
        outw    %ax, $DELAY_PORT            # Delay 1 uS
        movb    $WRITE_DATA_PORT_CMD, %al   # 8042 cmd to write output port
        outb    %al, $KBD_STATUS_PORT       # Send command to the 8042
        call    Empty8042InputBuffer        # Empty the Input Buffer on the 8042 controller
        jnz     Timeout8042                 # Jump if the 8042 timed out
        movb    $ENABLE_A20_CMD, %al        # gate address bit 20 on
        outb    %al, $KBD_CONTROL_PORT      # Send command to thre 8042
        call    Empty8042InputBuffer        # Empty the Input Buffer on the 8042 controller
        movw    $25,%cx                     # Delay 25 uS for the command to complete on the 8042
Delay25uS: 
        outw    %ax, $DELAY_PORT            # Delay 1 uS
        loop    Delay25uS
Timeout8042: 


A20GateEnabled: 

#
# DISABLE INTERRUPTS - Entering Protected Mode
#

        cli

#        mov ax,0b800h
#        mov es,ax
#        mov byte ptr es:[164],'c'
#        mov ax,cs
#        mov es,ax

    leal OffsetIn32BitProtectedMode, %eax
    addl $0x20000+0x6,%eax
    movl %eax, OffsetIn32BitProtectedMode 

    leal OffsetInLongMode, %eax
    addl $0x20000+0x6,%eax
    movl %eax, OffsetInLongMode 

    #
    # load GDT
    #
    .byte 0x66
    lgdt    gdtr

    #
    # Enable Protect Mode (set CR0.PE=1)
    #
    movl  %cr0, %eax      # Read CR0.
    orl   $0x1,%eax       # Set PE=1
    movl  %eax, %cr0      # Write CR0.
    .byte 0x66
    .byte 0xea                        # jmp far 16:32
OffsetIn32BitProtectedMode: 
    .long 0x0000000                   # offset $+8   (In32BitProtectedMode)
    .word 0x10                        # selector  (flat CS)
In32BitProtectedMode: 

#
# Entering Long Mode
#
    .byte 0x66
    movw $8,%ax
    movw %ax,%ds
    movw %ax,%es
    movw %ax,%ss

    #
    # Enable the 64-bit page-translation-table entries by
    # setting CR4.PAE=1 (this is _required_ before activating
    # long mode). Paging is not enabled until after long mode
    # is enabled.
    #
    .byte 0xf
    .byte 0x20
    .byte 0xe0
#    mov eax, cr4
    btsl $5,%eax
    .byte 0xf
    .byte 0x22
    .byte 0xe0
#    mov cr4, eax

    #
    # This is the Trapolean Page Tables that are guarenteed
    #  under 4GB.
    #
    # Address Map:
    #    10000 ~    12000 - efildr (loaded)
    #    20000 ~    21000 - start64.com
    #    21000 ~    22000 - efi64.com
    #    22000 ~    90000 - efildr
    #    90000 ~    96000 - 4G pagetable (will be reload later)
    #
    .byte 0xb8
    .long 0x90000
#    mov eax, 90000h
    movl %eax, %cr3

    #
    # Enable long mode (set EFER.LME=1).
    #
    .byte 0xb9
    .long 0xc0000080
#    mov   ecx, 0c0000080h ; EFER MSR number.
    .byte 0xf
    .byte 0x32
#    rdmsr                 ; Read EFER.
    .byte 0xf
    .byte 0xba
    .byte 0xe8
    .byte 0x8
#    bts   eax, 8          ; Set LME=1.
    .byte 0xf
    .byte 0x30
#    wrmsr                 ; Write EFER.

    #
    # Enable paging to activate long mode (set CR0.PG=1)
    #
    movl  %cr0, %eax      # Read CR0.
    .byte 0xf
    .byte 0xba
    .byte 0xe8
    .byte 0x1f
#    bts   eax, 31         ; Set PG=1.
    movl  %eax, %cr0      # Write CR0.
    jmp   GoToLongMode
GoToLongMode: 

    .byte 0x67
    .byte 0xea                  # Far Jump $+9:Selector to reload CS
OffsetInLongMode: 
    .long 00000000              #   $+9 Offset is ensuing instruction boundary
    .word 0x38                  #   Selector is our code selector, 38h

InLongMode: 
    .byte 0x66
    movw    $0x30,%ax
    movw    %ax,%ds

    .byte 0x66
    movw    $0x18,%ax
    movw    %ax,%es
    movw    %ax,%ss
    movw    %ax,%ds

    .byte 0xbd
    .long 0x400000
#    mov ebp,000400000h                  ; Destination of EFILDR32
    .byte 0xbb
    .long 0x70000
#    mov ebx,000070000h                  ; Length of copy

    #
    # load idt later
    #
    .byte 0x48
    .byte 0x33
    .byte 0xc0
#    xor rax, rax
    .byte 0x66
     movw $idtr, %ax
    .byte 0x48
    .byte 0x5
    .long 0x20000
#    add rax, 20000h

    .byte 0xf
    .byte 0x1
    .byte 0x18
#    lidt    fword ptr [rax]

    .byte 0x48
    .byte 0xc7
    .byte 0xc0
    .long 0x21000
#   mov rax, 21000h
    .byte 0x50
#   push rax

# ret
    .byte 0xc3

Empty8042InputBuffer: 
        movw $0,%cx
Empty8042Loop: 
        outw    %ax, $DELAY_PORT            # Delay 1us
        inb     $KBD_STATUS_PORT, %al       # Read the 8042 Status Port
        andb    $0x2,%al                    # Check the Input Buffer Full Flag
        loopnz  Empty8042Loop               # Loop until the input buffer is empty or a timout of 65536 uS
        ret

##############################################################################
# data
##############################################################################

        .p2align 1

        gdtr:    .word  GDT_END - GDT_BASE - 1  # GDT limit 
        .long 0                     # (GDT base gets set above)
##############################################################################
#   global descriptor table (GDT)
##############################################################################

        .p2align 1

GDT_BASE: 
# null descriptor
.equ                NULL_SEL, .-GDT_BASE    # Selector [0x0]
        .word 0         # limit 15:0
        .word 0         # base 15:0
        .byte 0         # base 23:16
        .byte 0         # type
        .byte 0         # limit 19:16, flags
        .byte 0         # base 31:24

# linear data segment descriptor
.equ            LINEAR_SEL, .-GDT_BASE  # Selector [0x8]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x92      # present, ring 0, data, expand-up, writable
        .byte 0xCF              # page-granular, 32-bit
        .byte 0

# linear code segment descriptor
.equ            LINEAR_CODE_SEL, .-GDT_BASE # Selector [0x10]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x9A      # present, ring 0, data, expand-up, writable
        .byte 0xCF              # page-granular, 32-bit
        .byte 0

# system data segment descriptor
.equ            SYS_DATA_SEL, .-GDT_BASE # Selector [0x18]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x92      # present, ring 0, data, expand-up, writable
        .byte 0xCF              # page-granular, 32-bit
        .byte 0

# system code segment descriptor
.equ            SYS_CODE_SEL, .-GDT_BASE # Selector [0x20]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x9A      # present, ring 0, data, expand-up, writable
        .byte 0xCF              # page-granular, 32-bit
        .byte 0

# spare segment descriptor
.equ        SPARE3_SEL, .-GDT_BASE  # Selector [0x28]
        .word 0         # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0         # present, ring 0, data, expand-up, writable
        .byte 0         # page-granular, 32-bit
        .byte 0

#
# system data segment descriptor
#
.equ              SYS_DATA64_SEL, .-GDT_BASE # Selector [0x30]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x92      # P | DPL [1..2] | 1   | 1   | C | R | A
        .byte 0xCF      # G | D   | L    | AVL | Segment [19..16]
        .byte 0

#
# system code segment descriptor
#
.equ              SYS_CODE64_SEL, .-GDT_BASE # Selector [0x38]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x9A      # P | DPL [1..2] | 1   | 1   | C | R | A
        .byte 0xAF      # G | D   | L    | AVL | Segment [19..16]
        .byte 0

# spare segment descriptor
.equ        SPARE4_SEL, .-GDT_BASE    # Selector [0x40]
        .word 0         # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0         # present, ring 0, data, expand-up, writable
        .byte 0         # page-granular, 32-bit
        .byte 0

GDT_END: 

        .p2align 1



idtr:   .long  IDT_END - IDT_BASE - 1 # IDT limit
        .quad 0                     # (IDT base gets set above)

##############################################################################
#   interrupt descriptor table (IDT)
#
#   Note: The hardware IRQ's specified in this table are the normal PC/AT IRQ
#       mappings.  This implementation only uses the system timer and all other
#       IRQs will remain masked.  The descriptors for vectors 33+ are provided
#       for convenience.
##############################################################################

#idt_tag db "IDT",0     
        .p2align 1


IDT_BASE: 
# divide by zero (INT 0)
.equ                DIV_ZERO_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# debug exception (INT 1)
.equ                DEBUG_EXCEPT_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# NMI (INT 2)
.equ                NMI_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# soft breakpoint (INT 3)
.equ                BREAKPOINT_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# overflow (INT 4)
.equ                OVERFLOW_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# bounds check (INT 5)
.equ                BOUNDS_CHECK_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# invalid opcode (INT 6)
.equ                INVALID_OPCODE_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# device not available (INT 7)
.equ                DEV_NOT_AVAIL_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# double fault (INT 8)
.equ                DOUBLE_FAULT_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# Coprocessor segment overrun - reserved (INT 9)
.equ                RSVD_INTR_SEL1, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# invalid TSS (INT 0ah)
.equ                INVALID_TSS_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# segment not present (INT 0bh)
.equ                SEG_NOT_PRESENT_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# stack fault (INT 0ch)
.equ                STACK_FAULT_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# general protection (INT 0dh)
.equ                GP_FAULT_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# page fault (INT 0eh)
.equ                PAGE_FAULT_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# Intel reserved - do not use (INT 0fh)
.equ                RSVD_INTR_SEL2, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# floating point error (INT 10h)
.equ                FLT_POINT_ERR_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# alignment check (INT 11h)
.equ                ALIGNMENT_CHECK_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# machine check (INT 12h)
.equ                MACHINE_CHECK_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# SIMD floating-point exception (INT 13h)
.equ                SIMD_EXCEPTION_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# 85 unspecified descriptors, First 12 of them are reserved, the rest are avail
        .fill 85 * 16, 1, 0   # db (85 * 16) dup(0)

# IRQ 0 (System timer) - (INT 68h)
.equ                IRQ0_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 1 (8042 Keyboard controller) - (INT 69h)
.equ                IRQ1_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# Reserved - IRQ 2 redirect (IRQ 2) - DO NOT USE!!! - (INT 6ah)
.equ                IRQ2_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 3 (COM 2) - (INT 6bh)
.equ                IRQ3_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 4 (COM 1) - (INT 6ch)
.equ                IRQ4_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 5 (LPT 2) - (INT 6dh)
.equ                IRQ5_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 6 (Floppy controller) - (INT 6eh)
.equ                IRQ6_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 7 (LPT 1) - (INT 6fh)
.equ                IRQ7_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 8 (RTC Alarm) - (INT 70h)
.equ                IRQ8_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 9 - (INT 71h)
.equ                IRQ9_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 10 - (INT 72h)
.equ                 IRQ10_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 11 - (INT 73h)
.equ                 IRQ11_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 12 (PS/2 mouse) - (INT 74h)
.equ                 IRQ12_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 13 (Floating point error) - (INT 75h)
.equ                 IRQ13_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 14 (Secondary IDE) - (INT 76h)
.equ                 IRQ14_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

# IRQ 15 (Primary IDE) - (INT 77h)
.equ                 IRQ15_SEL, .-IDT_BASE
        .word 0               # offset 15:0
        .long SYS_CODE64_SEL  # selector 15:0
        .byte 0               # 0 for interrupt gate
        .byte 0x0e | 0x80     # type = 386 interrupt gate, present
        .word 0               # offset 31:16
        .long 0               # offset 63:32
        .long 0               # 0 for reserved

IDT_END: 

        .p2align 1

MemoryMapSize:  .long 0
MemoryMap:  .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0
        .long 0,0,0,0,0,0,0,0

        .long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

        .org 0x0fe0
MyStack:    
        # below is the pieces of the IVT that is used to redirect INT 68h - 6fh
        #    back to INT 08h - 0fh  when in real mode...  It is 'org'ed to a
        #    known low address (20f00) so it can be set up by PlMapIrqToVect in
        #    8259.c

        int $8
        iret

        int $9
        iret

        int $10
        iret

        int $11
        iret

        int $12
        iret

        int $13
        iret

        int $14
        iret

        int $15
        iret


        .org 0x0ffe
BlockSignature: 
        .word 0xaa55


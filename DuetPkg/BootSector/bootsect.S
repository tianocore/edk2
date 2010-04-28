#------------------------------------------------------------------------------
#*
#*   Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
#*   This program and the accompanying materials                          
#*   are licensed and made available under the terms and conditions of the BSD License         
#*   which accompanies this distribution.  The full text of the license may be found at        
#*   http://opensource.org/licenses/bsd-license.php                                            
#*                                                                                             
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#*   
#*   bootsect.S
#*  
#*   bootsect.S is built as 16-bit binary file in 512 bytes and patched to disk/partition's 
#*   first section - boot sector. 
#*
#*   The startup sequence for DUET disk boot sector is:
#*
#*   1, LegacyBios check 0xAA55 signature at boot sectore offset 0x1FE to judget 
#*      whether disk/partition is bootable.
#*   2, LegacyBios will load boot sector to 0x7c00 in real mode, pass BPB data and
#*      hand off control to 0x7c00 code.
#*   3, boot sector code simply parse FAT format in boot disk and find EfiLdr binary file 
#*      and EfiVar.bin if exists. For first boot, EfiVar.bin does not exist.
#*   4, boot sector load the first sector of EfiLdr binary which is start.com to
#*      0x2000:0x0000 address.
#*   5, boot sector handoff control to 0x2000:0x0000 for start.com binary.
#*
#------------------------------------------------------------------------------

        .stack: 
        .486p: 
        .code16

.equ                      FAT_DIRECTORY_ENTRY_SIZE, 0x020
.equ                      FAT_DIRECTORY_ENTRY_SHIFT, 5
.equ                      BLOCK_SIZE, 0x0200
.equ                      BLOCK_MASK, 0x01ff
.equ                      BLOCK_SHIFT, 9
                                               # "EFILDR_____"
.equ                      LOADER_FILENAME_PART1, 0x04c494645    # "EFIL"
.equ                      LOADER_FILENAME_PART2, 0x020205244    # "DR__"
.equ                      LOADER_FILENAME_PART3, 0x020202020    # "____"

        .org 0x0
.global _start
_start:
Ia32Jump: 
  jmp   BootSectorEntryPoint  # JMP inst                  - 3 bytes
  nop

OemId:              .ascii   "INTEL   "       # OemId               - 8 bytes
# BPB data below will be fixed by tool
SectorSize:         .word  0                  # Sector Size         - 16 bits
SectorsPerCluster:  .byte  0                  # Sector Per Cluster  - 8 bits
ReservedSectors:    .word  0                  # Reserved Sectors    - 16 bits
NoFats:             .byte  0                  # Number of FATs      - 8 bits
RootEntries:        .word  0                  # Root Entries        - 16 bits
Sectors:            .word  0                  # Number of Sectors   - 16 bits
Media:              .byte  0                  # Media               - 8 bits  - ignored
SectorsPerFat:      .word  0                  # Sectors Per FAT     - 16 bits
SectorsPerTrack:    .word  0                  # Sectors Per Track   - 16 bits - ignored
Heads:              .word  0                  # Heads               - 16 bits - ignored
HiddenSectors:      .long  0                  # Hidden Sectors      - 32 bits - ignored
LargeSectors:       .long  0                  # Large Sectors       - 32 bits 
PhysicalDrive:      .byte  0                  # PhysicalDriveNumber - 8 bits  - ignored
CurrentHead:        .byte  0                  # Current Head        - 8 bits
Signature:          .byte  0                  # Signature           - 8 bits  - ignored
VolId:              .ascii "    "             # Volume Serial Number- 4 bytes
FatLabel:           .ascii "           "      # Label               - 11 bytes
SystemId:           .ascii "FAT12   "         # SystemId            - 8 bytes

BootSectorEntryPoint: 
        #ASSUME ds:@code
        #ASSUME ss:@code

# ****************************************************************************
# Start Print
# ****************************************************************************
  movw $StartString, %si
  call PrintString

# ****************************************************************************
# Print over
# ****************************************************************************

  movw  %cs, %ax      # ax = 0
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

  cmpw  $0xaa55, SectorSignature(%bp)         # Verify Boot Sector Signature
  jne   BadBootSector
  movw  RootEntries(%bp), %cx             # cx = RootEntries
  shlw  $FAT_DIRECTORY_ENTRY_SHIFT, %cx   # cx = cx * 32 = cx * sizeof(FAT_DIRECTORY_ENTRY) = Size of Root Directory in bytes
  movw  %cx, %bx                          # bx = size of the Root Directory in bytes
  andw  $BLOCK_MASK, %bx                  # See if it is an even number of sectors long
  jne   BadBootSector                     # If is isn't, then the boot sector is bad.
  movw  %cx, %bx                          # bx = size of the Root Directory in bytes
  shrw  $BLOCK_SHIFT, %bx                 # bx = size of Root Directory in sectors
  movb  NoFats(%bp), %al                  # al = NoFats
  xorb  %ah, %ah                          # ah = 0  ==> ax = NoFats
  mulw  SectorsPerFat(%bp)                # ax = NoFats * SectorsPerFat
  addw  ReservedSectors(%bp), %ax         # ax = NoFats * SectorsPerFat + ReservedSectors = RootLBA
  pushw %ds
  popw  %es
  xorw  %di, %di                          # Store directory in es:di = 1000:0000
  call  ReadBlocks                        # Read entire Root Directory
  addw  %bx, %ax                          # ax = NoFats * SectorsPerFat + ReservedSectors + RootDirSectors = FirstClusterLBA (FirstDataSector)
  movw  %ax, (%bp)                        # Save FirstClusterLBA (FirstDataSector) for later use

  # dx - variable storage (initial value is 0)
  # bx - loader (initial value is 0)
  xorw  %dx, %dx
  xorw  %bx, %bx

FindEFILDR: 
  cmpl   $LOADER_FILENAME_PART1, (%di)        # Compare to "EFIL"
  jne   FindVARSTORE
  cmpl   $LOADER_FILENAME_PART2, 4(%di) 
  jne   FindVARSTORE
  cmpl   $LOADER_FILENAME_PART3, 7(%di) 
  jne   FindVARSTORE
  movw  26(%di), %bx                      # bx = Start Cluster for EFILDR  <----------------------------------
  testw %dx, %dx
  je    FindNext                          # Efivar.bin is not loaded
  jmp   FoundAll

FindVARSTORE: 
  ## if the file is not loader file, see if it's "EFIVAR  BIN"
  cmpl  $0x56494645, (%di)                # Compare to "EFIV"
  jne   FindNext
  cmpl  $0x20205241, 4(%di)               # Compare to "AR  "
  jne   FindNext
  cmpl  $0x4e494220, 7(%di)               # Compare to " BIN"
  jne   FindNext
  movw  %di, %dx                          # dx = Offset of Start Cluster for Efivar.bin <---------------------
  addw  $26, %dx
  testw %bx, %bx
  je    FindNext                          # Efildr is not loaded
  jmp   FoundAll

FindNext: 
  # go to next find
  addw  $FAT_DIRECTORY_ENTRY_SIZE, %di    # Increment di
  subw  $FAT_DIRECTORY_ENTRY_SIZE, %cx    # Decrement cx
  # TODO: jump to FindVarStore if ...
  jne   FindEFILDR
  jmp   NotFoundAll

FoundAll: 
FoundEFILDR: 
  movw    %bx, %cx                            # cx = Start Cluster for EFILDR  <----------------------------------
  movw    %cs, %ax                            # Destination = 2000:0000
  addw    $0x2000, %ax
  movw    %ax, %es
  xorw    %di, %di
ReadFirstClusterOfEFILDR: 
  movw    %cx, %ax                            # ax = StartCluster
  subw    $2, %ax                             # ax = StartCluster - 2
  xorb    %bh, %bh
  movb    SectorsPerCluster(%bp), %bl         # bx = SectorsPerCluster
  pushw   %dx
  mulw    %bx
  popw    %dx                                 # ax = (StartCluster - 2) * SectorsPerCluster
  addw    (%bp), %ax                          # ax = FirstClusterLBA + (StartCluster-2)*SectorsPerCluster
  xorb    %bh, %bh
  movb    SectorsPerCluster(%bp), %bl         # bx = Number of Sectors in a cluster
  pushw   %es
  call    ReadBlocks
  popw    %ax
JumpIntoFirstSectorOfEFILDR: 
  movw    %ax, JumpSegment(%bp)
JumpFarInstruction: 
  .byte   0xea
JumpOffset: 
  .word   0x000
JumpSegment: 
  .word   0x2000


PrintString: 
  movw $0xb800, %ax
  movw %ax, %es
  movw $0x7c0, %ax
  movw %ax, %ds
  movw $7, %cx
  movw $160, %di
  rep
  movsw
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
  addl    LBAOffsetForBootSector(%bp), %eax            # Add LBAOffsetForBootSector to Start LBA
  addl    HiddenSectors(%bp), %eax            # Add HiddenSectors to Start LBA
  movl    %eax, %esi                          # esi = Start LBA
  movw    %bx, %cx                            # cx = Number of blocks to read
ReadCylinderLoop: 
  movw    $0x7bfc, %bp                        # bp = 0x7bfc
  movl    %esi, %eax                          # eax = Start LBA
  xorl    %edx, %edx                          # edx = 0
  movzwl  (%bp), %ebx                         # bx = MaxSector
  divl    %ebx                                # ax = StartLBA / MaxSector
  incw    %dx                                 # dx = (StartLBA % MaxSector) + 1
  subw    %dx, %bx                            # bx = MaxSector - Sector
  incw    %bx                                 # bx = MaxSector - Sector + 1
  cmpw    %bx, %cx                            # Compare (Blocks) to (MaxSector - Sector + 1)
  jg      LimitTransfer
  movw    %cx, %bx                            # bx = Blocks
LimitTransfer: 
  pushw   %cx
  movb    %dl, %cl                            # cl = (StartLBA % MaxSector) + 1 = Sector
  xorw    %dx, %dx                            # dx = 0
  divw    2(%bp)                              # ax = ax / (MaxHead + 1) = Cylinder  
                                              # dx = ax % (MaxHead + 1) = Head

  pushw   %bx                                 # Save number of blocks to transfer
  movb    %dl, %dh                            # dh = Head
  movw    $0x7c00, %bp                        # bp = 0x7c00
  movb    PhysicalDrive(%bp), %dl             # dl = Drive Number
  movb    %al, %ch                            # ch = Cylinder
  movb    %bl, %al                            # al = Blocks
  movb    $2, %ah                             # ah = Function 2
  movw    %di, %bx                            # es:bx = Buffer address
  int     $0x13
  jc      DiskError
  popw    %bx
  popw    %cx
  movzwl  %bx, %ebx
  addl    %ebx, %esi                          # StartLBA = StartLBA + NumberOfBlocks
  subw    %bx, %cx                            # Blocks = Blocks - NumberOfBlocks
  movw    %es, %ax
  shlw    $(BLOCK_SHIFT-4), %bx
  addw    %bx, %ax
  movw    %ax, %es                            # es:di = es:di + NumberOfBlocks*BLOCK_SIZE
  cmpw    $0, %cx
  jne     ReadCylinderLoop
  popa
  ret

# ****************************************************************************
# ERROR Condition:
# ****************************************************************************
NotFoundAll: 
  ## if we found EFILDR, continue
  testw %bx, %bx
  jne  FoundEFILDR
BadBootSector: 
DiskError: 
  movw $ErrorString, %si
  call PrintString
Halt: 
  jmp   Halt

StartString: 
  .byte 'B', 0x0c, 'S', 0x0c, 't', 0x0c, 'a', 0x0c, 'r', 0x0c, 't', 0x0c, '!', 0x0c
ErrorString: 
  .byte 'B', 0x0c, 'E', 0x0c, 'r', 0x0c, 'r', 0x0c, 'o', 0x0c, 'r', 0x0c, '!', 0x0c

# ****************************************************************************
# LBA Offset for BootSector, need patched by tool for HD boot.
# ****************************************************************************

  .org 0x01fa   # Comment it for pass build. Should optimise code size. 
LBAOffsetForBootSector: 
  .long     0x0

# ****************************************************************************
# Sector Signature
# ****************************************************************************

  .org 0x01fe    # Comment it for pass build.
SectorSignature: 
  .word     0xaa55      # Boot Sector Signature




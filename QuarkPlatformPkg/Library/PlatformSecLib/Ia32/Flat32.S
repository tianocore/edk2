#------------------------------------------------------------------------------
#
# Copyright (c) 2013 - 2016 Intel Corporation.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Module Name:
#
#  Flat32.S
#
# Abstract:
#
#  This is the code that goes from real-mode to protected mode.
#  It consumes the reset vector, configures the stack.
#
#
#------------------------------------------------------------------------------

.macro RET32
    jmp    *%esp
.endm

#
# ROM/SPI/MEMORY Definitions
#
.equ  QUARK_DDR3_MEM_BASE_ADDRESS, (0x000000000)    # Memory Base Address = 0
.equ  QUARK_MAX_DDR3_MEM_SIZE_BYTES, (0x80000000)    # DDR3 Memory Size = 2GB
.equ  QUARK_ESRAM_MEM_SIZE_BYTES, (0x00080000)    # eSRAM Memory Size = 512K
.equ  QUARK_STACK_SIZE_BYTES, (0x008000)      # Quark stack size = 32K

#
# RTC/CMOS definitions
#
.equ  RTC_INDEX, (0x70)
.equ    NMI_DISABLE, (0x80)  # Bit7=1 disables NMI
.equ    NMI_ENABLE, (0x00)  # Bit7=0 disables NMI
.equ  RTC_DATA, (0x71)

#
# PCI Configuration definitions
#
.equ  PCI_CFG, (0x80000000) # PCI configuration access mechanism
.equ  PCI_ADDRESS_PORT, (0xCF8)
.equ  PCI_DATA_PORT, (0xCFC)

#
# Quark PCI devices
#
.equ  HOST_BRIDGE_PFA, (0x0000)   # B0:D0:F0 (Host Bridge)
.equ  ILB_PFA, (0x00F8)      # B0:D31:F0 (Legacy Block)

#
# ILB PCI Config Registers
#
.equ  BDE, (0x0D4)                                # BIOS Decode Enable register
.equ    DECODE_ALL_REGIONS_ENABLE, (0xFF000000)    # Decode all BIOS decode ranges

#
# iLB Reset Register
#
.equ  ILB_RESET_REG, (0x0CF9)
.equ    CF9_WARM_RESET, (0x02)
.equ    CF9_COLD_RESET, (0x08)

#
# Host Bridge PCI Config Registers
#
.equ  MESSAGE_BUS_CONTROL_REG, (0xD0)       # Message Bus Control Register
.equ    SB_OPCODE_FIELD, (0x18)              # Bit location of Opcode field
.equ      OPCODE_SIDEBAND_REG_READ, (0x10)  # Read opcode
.equ      OPCODE_SIDEBAND_REG_WRITE, (0x11) # Write opcode
.equ      OPCODE_SIDEBAND_ALT_REG_READ, (0x06)  # Alternate Read opcode
.equ      OPCODE_SIDEBAND_ALT_REG_WRITE, (0x07) # Alternate Write opcode
.equ      OPCODE_WARM_RESET_REQUEST, (0xF4)  # Reset Warm
.equ      OPCODE_COLD_RESET_REQUEST, (0xF5) # Reset Cold
.equ    SB_PORT_FIELD, (0x10)               # Bit location of Port ID field
.equ      MEMORY_ARBITER_PORT_ID, (0x00)
.equ      HOST_BRIDGE_PORT_ID, (0x03)
.equ      RMU_PORT_ID, (0x04)
.equ      MEMORY_MANAGER_PORT_ID, (0x05)
.equ      SOC_UNIT_PORT_ID, (0x31)
.equ    SB_ADDR_FIELD, (0x08)               # Bit location of Register field
.equ    SB_BE_FIELD, (0x04)                  # Bit location of Byte Enables field
.equ      ALL_BYTE_EN, (0x0F)                # All Byte Enables
.equ  MESSAGE_DATA_REG, (0xD4)              # Message Data Register

#
# Memory Arbiter Config Registers
#
.equ  AEC_CTRL_OFFSET, (0x00)

#
# Host Bridge Config Registers
#
.equ  HMISC2_OFFSET, (0x03) # PCI configuration access mechanism
.equ    OR_PM_FIELD, (0x10)
.equ      SMI_EN, (0x00080000)

.equ  HMBOUND_OFFSET, (0x08)
.equ    HMBOUND_ADDRESS, (QUARK_DDR3_MEM_BASE_ADDRESS + QUARK_MAX_DDR3_MEM_SIZE_BYTES + QUARK_ESRAM_MEM_SIZE_BYTES)
.equ    HMBOUND_LOCK, (0x01)
.equ  HECREG_OFFSET, (0x09)
.equ    EC_BASE, (0xE0000000)
.equ    EC_ENABLE, (0x01)
.equ  HLEGACY_OFFSET, (0x0A)
.equ    NMI, (0x00004000)
.equ    SMI, (0x00001000)
.equ    INTR, (0x00000400)

#
# Memory Manager Config Registers
#
.equ  ESRAMPGCTRL_BLOCK_OFFSET, (0x82)
.equ    BLOCK_ENABLE_PG, (0x10000000)
.equ  BIMRVCTL_OFFSET, (0x19)
.equ    ENABLE_IMR_INTERRUPT, (0x80000000)

#
# SOC UNIT Debug Registers
#
.equ  CFGSTICKY_W1_OFFSET, (0x50)
.equ    FORCE_COLD_RESET, (0x00000001)
.equ  CFGSTICKY_RW_OFFSET, (0x51)
.equ    RESET_FOR_ESRAM_LOCK, (0x00000020)
.equ    RESET_FOR_HMBOUND_LOCK, (0x00000040)
.equ  CFGNONSTICKY_W1_OFFSET, (0x52)
.equ    FORCE_WARM_RESET, (0x00000001)

#
# CR0 cache control bit definition
#
.equ                    CR0_CACHE_DISABLE, 0x040000000
.equ                    CR0_NO_WRITE,      0x020000000

ASM_GLOBAL  ASM_PFX(PcdGet32(PcdEsramStage1Base))


#
# Contrary to the name, this file contains 16 bit code as well.
#
.text
#----------------------------------------------------------------------------
#
# Procedure:    _ModuleEntryPoint
#
# Input:        None
#
# Output:       None
#
# Destroys:     Assume all registers
#
# Description:
#
#   Transition to non-paged flat-model protected mode from a
#   hard-coded GDT that provides exactly two descriptors.
#   This is a bare bones transition to protected mode only
#   used for a while in PEI and possibly DXE.
#
#   After enabling protected mode, a far jump is executed to
#   transfer to PEI using the newly loaded GDT.
#
# Return:       None
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):

  #
  # Warm Reset (INIT#) check.
  #
  .byte   0xbe,0x00,0xf0   #movw    $0xF000, %si
  .byte   0x8e,0xde        #movw    %si, %ds
  .byte   0xbe,0xf0,0xff   #movw    $0xFFF0, %si
  .byte   0x80,0x3c,0xea   #cmpb    $0xEA, (%si)          # Is it warm reset ?
  jne     NotWarmReset     # Jump if not.
  .byte   0xb0,0x08        #movb    $0x08, %al
  .byte   0xba,0xf9,0x0c   #movw    $0xcf9, %dx
  .byte   0xee             #outb    %al, %dx
  .byte   0xb0,0x55        #movb    $0x55, %al
  .byte   0xe6,0x80        #outb    %al, $0x80
  jmp     .
NotWarmReset:
  .byte   0x66,0x8b,0xe8   #movl    %eax, %ebp

  #
  # Load the GDT table in GdtDesc
  #
  .byte   0x66,0xbe        #movl    $GdtDesc, %esi
  .long   GdtDesc

  .byte   0x66,0x2e,0x0f,0x01,0x14   #lgdt    %cs:(%si)

  #
  # Transition to 16 bit protected mode
  #
  .byte   0x0f,0x20,0xc0       #movl    %cr0, %eax                  # Get control register 0
  .byte   0x66,0x83,0xc8,0x03  #orl     $0x0000003, %eax           # Set PE bit (bit #0) & MP bit (bit #1)
  .byte   0x0f,0x22,0xc0       #movl    %eax, %cr0                  # Activate protected mode

  #
  # Now we're in 16 bit protected mode
  # Set up the selectors for 32 bit protected mode entry
  #
  .byte   0xb8                 #movw    SYS_DATA_SEL, %ax
  .word   SYS_DATA_SEL

  .byte   0x8e,0xd8            #movw    %ax, %ds
  .byte   0x8e,0xc0            #movw    %ax, %es
  .byte   0x8e,0xe0            #movw    %ax, %fs
  .byte   0x8e,0xe8            #movw    %ax, %gs
  .byte   0x8e,0xd0            #movw    %ax, %ss

  #
  # Transition to Flat 32 bit protected mode
  # The jump to a far pointer causes the transition to 32 bit mode
  #
  .byte   0x66,0xbe            #movl   ProtectedModeEntryLinearAddress, %esi
  .long   ProtectedModeEntryLinearAddress
  .byte   0x66,0x2e,0xff,0x2c  #jmp    %cs:(%esi)

#
# Protected mode portion initializes stack, configures cache, and calls C entry point
#

#----------------------------------------------------------------------------
#
# Procedure:    ProtectedModeEntryPoint
#
# Input:        Executing in 32 Bit Protected (flat) mode
#                cs: 0-4GB
#                ds: 0-4GB
#                es: 0-4GB
#                fs: 0-4GB
#                gs: 0-4GB
#                ss: 0-4GB
#
# Output:       This function never returns
#
# Destroys:
#               ecx
#               edi
#                esi
#                esp
#
# Description:
#                Perform any essential early platform initilaisation
#               Setup a stack
#               Transfer control to EDKII code in eSRAM
#
#----------------------------------------------------------------------------
ProtectedModeEntryPoint:
  leal  L0, %esp
  jmp  stackless_EarlyPlatformInit
L0:

  #
  # Set up stack pointer
  #
  movl    ASM_PFX(PcdGet32(PcdEsramStage1Base)), %esp
  movl    $QUARK_ESRAM_MEM_SIZE_BYTES, %esi
  addl    %esi, %esp                          # ESP = top of stack (stack grows downwards).

  #
  # Store the the BIST value in EBP
  #
  movl    $0, %ebp    # No processor BIST on Quark

  #
  # Push processor count to stack first, then BIST status (AP then BSP)
  #
  movl    $1, %eax
  cpuid
  shrl    $16, %ebx
  andl    $0x000000FF, %ebx
  cmpb    $1, %bl
  jae     PushProcessorCount

  #
  # Some processors report 0 logical processors.  Effectively 0 = 1.
  # So we fix up the processor count
  #
  incl    %ebx

PushProcessorCount:
  pushl   %ebx

  #
  # We need to implement a long-term solution for BIST capture.  For now, we just copy BSP BIST
  # for all processor threads
  #
  xorl    %ecx, %ecx
  movb    %bl, %cl

PushBist:
  pushl   %ebp
  loop    PushBist

  #
  # Pass Control into the PEI Core
  #
  call PlatformSecLibStartup

  #
  # PEI Core should never return to here, this is just to capture an invalid return.
  #
  jmp     .

#----------------------------------------------------------------------------
#
# Procedure:    stackless_EarlyPlatformInit
#
# Input:        esp - Return address
#
# Output:       None
#
# Destroys:     Assume all registers
#
# Description:
#        Any early platform initialisation required
#
# Return:
#      None
#
#----------------------------------------------------------------------------
stackless_EarlyPlatformInit:

  #
  # Save return address
  #
  movl  %esp, %ebp

  #
  # Ensure cache is disabled.
  #
  movl %cr0, %eax
  orl $(CR0_CACHE_DISABLE + CR0_NO_WRITE), %eax
  invd
  movl    %eax, %cr0

  #
  # Disable NMI operation
  # Good convention suggests you should read back RTC data port after
  # accessing the RTC index port.
  #
  movb $(NMI_DISABLE), %al
  movw $(RTC_INDEX), %dx
  outb %al, %dx
  movw $(RTC_DATA), %dx
  inb  %dx, %al

  #
  # Disable SMI (Disables SMI wire, not SMI messages)
  #
  movl  $((OPCODE_SIDEBAND_REG_READ << SB_OPCODE_FIELD) | (HOST_BRIDGE_PORT_ID << SB_PORT_FIELD) | (HMISC2_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L1, %esp
  jmp  stackless_SideBand_Read
L1:
  andl $(~SMI_EN), %eax
  movl  $((OPCODE_SIDEBAND_REG_WRITE << SB_OPCODE_FIELD) | (HOST_BRIDGE_PORT_ID << SB_PORT_FIELD) | (HMISC2_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L2, %esp
  jmp  stackless_SideBand_Write
L2:

  #
  # Before we get going, check SOC Unit Registers to see if we are required to issue a warm/cold reset
  #
  movl  $((OPCODE_SIDEBAND_ALT_REG_READ << SB_OPCODE_FIELD) | (SOC_UNIT_PORT_ID << SB_PORT_FIELD) | (CFGNONSTICKY_W1_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L3, %esp
  jmp  stackless_SideBand_Read
L3:
  andl $(FORCE_WARM_RESET), %eax
  jz TestForceColdReset    # Zero means bit clear, we're not requested to warm reset so continue as normal
  jmp IssueWarmReset

TestForceColdReset:
  movl  $((OPCODE_SIDEBAND_ALT_REG_READ << SB_OPCODE_FIELD) | (SOC_UNIT_PORT_ID << SB_PORT_FIELD) | (CFGNONSTICKY_W1_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L4, %esp
  jmp  stackless_SideBand_Read
L4:
  andl $(FORCE_COLD_RESET), %eax
  jz TestHmboundLock    # Zero means bit clear, we're not requested to cold reset so continue as normal
  jmp IssueColdReset

  #
  # Before setting HMBOUND, check it's not locked
  #
TestHmboundLock:
  movl  $((OPCODE_SIDEBAND_REG_READ << SB_OPCODE_FIELD) | (HOST_BRIDGE_PORT_ID << SB_PORT_FIELD) | (HMBOUND_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L5, %esp
  jmp  stackless_SideBand_Read
L5:
  andl $(HMBOUND_LOCK), %eax
  jz ConfigHmbound  # Zero means bit clear, we have the config we want so continue as normal
  #
  # Failed to config - store sticky bit debug
  #
  movl  $((OPCODE_SIDEBAND_ALT_REG_READ << SB_OPCODE_FIELD) | (SOC_UNIT_PORT_ID << SB_PORT_FIELD) | (CFGSTICKY_RW_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L6, %esp
  jmp  stackless_SideBand_Read
L6:
  orl $(RESET_FOR_HMBOUND_LOCK), %eax
  movl  $((OPCODE_SIDEBAND_ALT_REG_WRITE << SB_OPCODE_FIELD) | (SOC_UNIT_PORT_ID << SB_PORT_FIELD) | (CFGSTICKY_RW_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L7, %esp
  jmp  stackless_SideBand_Write
L7:
  jmp IssueWarmReset

  #
  # Set up the HMBOUND register
  #
ConfigHmbound:
  movl $(HMBOUND_ADDRESS), %eax      # Data (Set HMBOUND location)
  movl  $((OPCODE_SIDEBAND_REG_WRITE << SB_OPCODE_FIELD) | (HOST_BRIDGE_PORT_ID << SB_PORT_FIELD) | (HMBOUND_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L8, %esp
  jmp  stackless_SideBand_Write
L8:

  #
  # Enable interrupts to Remote Management Unit when a IMR/SMM/HMBOUND violation occurs.
  #
  movl $(ENABLE_IMR_INTERRUPT), %eax      # Data (Set interrupt enable mask)
  movl  $((OPCODE_SIDEBAND_REG_WRITE << SB_OPCODE_FIELD) | (MEMORY_MANAGER_PORT_ID << SB_PORT_FIELD) | (BIMRVCTL_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L9, %esp
  jmp  stackless_SideBand_Write
L9:

  #
  # Set eSRAM address
  #
  movl    ASM_PFX(PcdGet32(PcdEsramStage1Base)), %eax   # Data (Set eSRAM location)
  shr   $(0x18), %eax
  addl  $(BLOCK_ENABLE_PG), %eax
  movl  $((OPCODE_SIDEBAND_REG_WRITE << SB_OPCODE_FIELD) | (MEMORY_MANAGER_PORT_ID << SB_PORT_FIELD) | (ESRAMPGCTRL_BLOCK_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L10, %esp
  jmp  stackless_SideBand_Write
L10:

  #
  # Check that we're not blocked from setting the config that we want.
  #
  movl  $((OPCODE_SIDEBAND_REG_READ << SB_OPCODE_FIELD) | (MEMORY_MANAGER_PORT_ID << SB_PORT_FIELD) | (ESRAMPGCTRL_BLOCK_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L11, %esp
  jmp  stackless_SideBand_Read
L11:
  andl $(BLOCK_ENABLE_PG), %eax
  jnz ConfigPci  # Non-zero means bit set, we have the config we want so continue as normal
  #
  # Failed to config - store sticky bit debug
  #
  movl  $((OPCODE_SIDEBAND_ALT_REG_READ << SB_OPCODE_FIELD) | (SOC_UNIT_PORT_ID << SB_PORT_FIELD) | (CFGSTICKY_RW_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L12, %esp
  jmp  stackless_SideBand_Read
L12:
  orl $(RESET_FOR_ESRAM_LOCK), %eax     # Set the bit we're interested in
  movl  $((OPCODE_SIDEBAND_ALT_REG_WRITE << SB_OPCODE_FIELD) | (SOC_UNIT_PORT_ID << SB_PORT_FIELD) | (CFGSTICKY_RW_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L13, %esp
  jmp  stackless_SideBand_Write
L13:
  jmp IssueWarmReset

  #
  # Enable PCIEXBAR
  #
ConfigPci:
  movl $(EC_BASE + EC_ENABLE), %eax      # Data
  movl  $((OPCODE_SIDEBAND_REG_WRITE << SB_OPCODE_FIELD) | (MEMORY_ARBITER_PORT_ID << SB_PORT_FIELD) | (AEC_CTRL_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L14, %esp
  jmp  stackless_SideBand_Write
L14:

  movl $(EC_BASE + EC_ENABLE), %eax      # Data
  movl  $((OPCODE_SIDEBAND_REG_WRITE << SB_OPCODE_FIELD) | (HOST_BRIDGE_PORT_ID << SB_PORT_FIELD) | (HECREG_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L15, %esp
  jmp  stackless_SideBand_Write
L15:

  #
  #  Open up full 8MB SPI decode
  #
  movl  $(PCI_CFG | (ILB_PFA << 8) | BDE), %ebx  # PCI Configuration address
  movl $(DECODE_ALL_REGIONS_ENABLE), %eax
  leal  L16, %esp
  jmp  stackless_PCIConfig_Write
L16:

  #
  # Enable NMI operation
  # Good convention suggests you should read back RTC data port after
  # accessing the RTC index port.
  #
  movb $(NMI_ENABLE), %al
  movw $(RTC_INDEX), %dx
  outb %al, %dx
  movw $(RTC_DATA), %dx
  inb  %dx, %al

  #
  # Clear Host Bridge SMI, NMI, INTR fields
  #
  movl  $((OPCODE_SIDEBAND_REG_READ << SB_OPCODE_FIELD) | (HOST_BRIDGE_PORT_ID << SB_PORT_FIELD) | (HLEGACY_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L21, %esp
  jmp  stackless_SideBand_Read
L21:
  andl $~(NMI + SMI + INTR), %eax      # Clear NMI, SMI, INTR fields
  movl  $((OPCODE_SIDEBAND_REG_WRITE << SB_OPCODE_FIELD) | (HOST_BRIDGE_PORT_ID << SB_PORT_FIELD) | (HLEGACY_OFFSET << SB_ADDR_FIELD)), %ecx
  leal  L22, %esp
  jmp  stackless_SideBand_Write
L22:

  #
  # Restore return address
  #
  movl  %ebp, %esp
  RET32

IssueWarmReset:
  #
  # Issue Warm Reset request to Remote Management Unit via iLB
  #
  movw  $(CF9_WARM_RESET), %ax
  movw  $(ILB_RESET_REG), %dx
  outw  %ax, %dx
  jmp  .  # Stay here until we are reset.

IssueColdReset:
  #
  # Issue Cold Reset request to Remote Management Unit via iLB
  #
  movw  $(CF9_COLD_RESET), %ax
  movw  $(ILB_RESET_REG), %dx
  outw  %ax, %dx
  jmp  .  # Stay here until we are reset.

#----------------------------------------------------------------------------
#
# Procedure:    stackless_SideBand_Read
#
# Input:        esp - return address
#                ecx[15:8] - Register offset
#                ecx[23:16] - Port ID
#                ecx[31:24] - Opcode
#
# Output:       eax - Data read
#
# Destroys:
#                eax
#                ebx
#                cl
#                esi
#
# Description:
#        Perform requested sideband read
#
#----------------------------------------------------------------------------
stackless_SideBand_Read:

  movl  %esp, %esi      # Save the return address

  #
  # Load the SideBand Packet Register to generate the transaction
  #
  movl  $((PCI_CFG) | (HOST_BRIDGE_PFA << 8) | (MESSAGE_BUS_CONTROL_REG)), %ebx   # PCI Configuration address
  movb  $(ALL_BYTE_EN << SB_BE_FIELD), %cl      # Set all Byte Enable bits
  xchgl  %ecx, %eax
  leal  L17, %esp
  jmp  stackless_PCIConfig_Write
L17:
  xchgl  %ecx, %eax

  #
  # Read the SideBand Data Register
  #
  movl  $((PCI_CFG) | (HOST_BRIDGE_PFA << 8) | (MESSAGE_DATA_REG)), %ebx   # PCI Configuration address
  leal  L18, %esp
  jmp  stackless_PCIConfig_Read
L18:

  movl  %esi, %esp      # Restore the return address
  RET32


#----------------------------------------------------------------------------
#
# Procedure:    stackless_SideBand_Write
#
# Input:        esp - return address
#                eax - Data
#                ecx[15:8] - Register offset
#                ecx[23:16] - Port ID
#                ecx[31:24] - Opcode
#
# Output:       None
#
# Destroys:
#                ebx
#                cl
#                esi
#
# Description:
#        Perform requested sideband write
#
#
#----------------------------------------------------------------------------
stackless_SideBand_Write:

  movl  %esp, %esi      # Save the return address

  #
  # Load the SideBand Data Register with the data
  #
  movl  $((PCI_CFG) | (HOST_BRIDGE_PFA << 8) | (MESSAGE_DATA_REG)), %ebx   # PCI Configuration address
  leal  L19, %esp
  jmp  stackless_PCIConfig_Write
L19:

  #
  # Load the SideBand Packet Register to generate the transaction
  #
  movl  $((PCI_CFG) | (HOST_BRIDGE_PFA << 8) | (MESSAGE_BUS_CONTROL_REG)), %ebx   # PCI Configuration address
  movb  $(ALL_BYTE_EN << SB_BE_FIELD), %cl      # Set all Byte Enable bits
  xchgl  %ecx, %eax
  leal  L20, %esp
  jmp  stackless_PCIConfig_Write
L20:
  xchgl  %ecx, %eax

  movl  %esi, %esp      # Restore the return address
  RET32


#----------------------------------------------------------------------------
#
# Procedure:    stackless_PCIConfig_Write
#
# Input:        esp - return address
#                eax - Data to write
#                ebx - PCI Config Address
#
# Output:       None
#
# Destroys:
#                dx
#
# Description:
#        Perform a DWORD PCI Configuration write
#
#----------------------------------------------------------------------------
stackless_PCIConfig_Write:

  #
  # Write the PCI Config Address to the address port
  #
  xchgl  %ebx, %eax
  movw  $(PCI_ADDRESS_PORT), %dx
  outl  %eax, %dx
  xchgl  %ebx, %eax

  #
  # Write the PCI DWORD Data to the data port
  #
  movw  $(PCI_DATA_PORT), %dx
  outl  %eax, %dx

  RET32


#----------------------------------------------------------------------------
#
# Procedure:    stackless_PCIConfig_Read
#
# Input:        esp - return address
#                ebx - PCI Config Address
#
# Output:       eax - Data read
#
# Destroys:
#                eax
#                dx
#
# Description:
#        Perform a DWORD PCI Configuration read
#
#----------------------------------------------------------------------------
stackless_PCIConfig_Read:

  #
  # Write the PCI Config Address to the address port
  #
  xchgl  %ebx, %eax
  movw  $(PCI_ADDRESS_PORT), %dx
  outl  %eax, %dx
  xchgl  %ebx, %eax

  #
  # Read the PCI DWORD Data from the data port
  #
  movw  $(PCI_DATA_PORT), %dx
  inl  %dx, %eax

  RET32


#
# ROM-based Global-Descriptor Table for the Tiano PEI Phase
#
.align 16
#
# GDT[0]: 000h: Null entry, never used.
#

GDT_BASE:
BootGdtTable:
# null descriptor
.equ                NULL_SEL, . - GDT_BASE # Selector [0]
        .word 0         # limit 15:0
        .word 0         # base 15:0
        .byte 0         # base 23:16
        .byte 0         # type
        .byte 0         # limit 19:16, flags
        .byte 0         # base 31:24

# linear data segment descriptor
.equ            LINEAR_SEL, . - GDT_BASE # Selector [0x8]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x92      # present, ring 0, data, expand-up, writable
        .byte 0xCF              # page-granular, 32-bit
        .byte 0

# linear code segment descriptor
.equ            LINEAR_CODE_SEL, . - GDT_BASE # Selector [0x10]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x9A      # present, ring 0, data, expand-up, writable
        .byte 0xCF              # page-granular, 32-bit
        .byte 0

# system data segment descriptor
.equ            SYS_DATA_SEL, . - GDT_BASE # Selector [0x18]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x92      # present, ring 0, data, expand-up, writable
        .byte 0xCF              # page-granular, 32-bit
        .byte 0

# system code segment descriptor
.equ            SYS_CODE_SEL, . - GDT_BASE
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x9A      # present, ring 0, data, expand-up, writable
        .byte 0xCF              # page-granular, 32-bit
        .byte 0

# spare segment descriptor
.equ        SYS16_CODE_SEL, . - GDT_BASE
        .word 0xffff    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0x0f
        .byte 0x9b      # present, ring 0, data, expand-up, writable
        .byte 0         # page-granular, 32-bit
        .byte 0

# spare segment descriptor
.equ        SYS16_DATA_SEL, . - GDT_BASE
        .word 0xffff    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x93      # present, ring 0, data, expand-up, not-writable
        .byte 0         # page-granular, 32-bit
        .byte 0

# spare segment descriptor
.equ        SPARE5_SEL, . - GDT_BASE
        .word 0         # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0         # present, ring 0, data, expand-up, writable
        .byte 0         # page-granular, 32-bit
        .byte 0
.equ        GDT_SIZE, . - GDT_BASE

#
# GDT Descriptor
#
GdtDesc:                                     # GDT descriptor
       .word    GDT_SIZE - 1
       .long    BootGdtTable

ProtectedModeEntryLinearAddress:
ProtectedModeEntryLinearOffset:
       .long    ProtectedModeEntryPoint
       .word    LINEAR_CODE_SEL

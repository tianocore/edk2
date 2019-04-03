;------------------------------------------------------------------------------
;
; Copyright (c) 2013-2015 Intel Corporation.
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;  Flat32.asm
;
; Abstract:
;
;  This is the code that goes from real-mode to protected mode.
;  It consumes the reset vector, configures the stack.
;
;
;------------------------------------------------------------------------------


;
; Define assembler characteristics
;
.586p
.model flat, c

;
; Include processor definitions
;

INCLUDE Platform.inc


;
; CR0 cache control bit definition
;
CR0_CACHE_DISABLE       EQU 040000000h
CR0_NO_WRITE            EQU 020000000h

;
; External and public declarations
;  TopOfStack is used by C code
;  SecStartup is the entry point to the C code
; Neither of these names can be modified without
; updating the C code.
;
EXTRN   PlatformSecLibStartup: NEAR
EXTERNDEF   C   PcdGet32 (PcdEsramStage1Base):DWORD

;
; Contrary to the name, this file contains 16 bit code as well.
;
_TEXT_REALMODE      SEGMENT PARA PUBLIC USE16 'CODE'
                    ASSUME  CS:_TEXT_REALMODE, DS:_TEXT_REALMODE

;----------------------------------------------------------------------------
;
; Procedure:    _ModuleEntryPoint
;
; Input:        None
;
; Output:       None
;
; Destroys:     Assume all registers
;
; Description:
;
;   Transition to non-paged flat-model protected mode from a
;   hard-coded GDT that provides exactly two descriptors.
;   This is a bare bones transition to protected mode only
;   used for a while in PEI and possibly DXE.
;
;   After enabling protected mode, a far jump is executed to
;   transfer to PEI using the newly loaded GDT.
;
; Return:       None
;
;----------------------------------------------------------------------------
align 16
_ModuleEntryPoint      PROC C PUBLIC

  ;
  ; Warm Reset (INIT#) check.
  ;
  mov     si, 0F000h
  mov     ds, si
  mov     si, 0FFF0h
  cmp     BYTE PTR [si], 0EAh   ; Is it warm reset ?
  jne     NotWarmReset          ; JIf not.

  mov     al, 08
  mov     dx, 0cf9h
  out     dx, al
  mov     al, 055h
  out     080h, al;
  jmp $
NotWarmReset:

  ;
  ; Load the GDT table in GdtDesc
  ;
  mov     esi, OFFSET GdtDesc
  db      66h
  lgdt    fword ptr cs:[si]

  ;
  ; Transition to 16 bit protected mode
  ;
  mov     eax, cr0                   ; Get control register 0
  or      eax, 00000003h             ; Set PE bit (bit #0) & MP bit (bit #1)
  mov     cr0, eax                   ; Activate protected mode

  ;
  ; Now we're in 16 bit protected mode
  ; Set up the selectors for 32 bit protected mode entry
  ;
  mov     ax, SYS_DATA_SEL
  mov     ds, ax
  mov     es, ax
  mov     fs, ax
  mov     gs, ax
  mov     ss, ax

  ;
  ; Transition to Flat 32 bit protected mode
  ; The jump to a far pointer causes the transition to 32 bit mode
  ;
  mov esi, offset ProtectedModeEntryLinearAddress
  jmp     fword ptr cs:[si]

_ModuleEntryPoint   ENDP

_TEXT_REALMODE      ENDS

.code
;
; Protected mode portion initializes stack, configures cache, and calls C entry point
;

;----------------------------------------------------------------------------
;
; Procedure:    ProtectedModeEntryPoint
;
; Input:        Executing in 32 Bit Protected (flat) mode
;                cs: 0-4GB
;                ds: 0-4GB
;                es: 0-4GB
;                fs: 0-4GB
;                gs: 0-4GB
;                ss: 0-4GB
;
; Output:       This function never returns
;
; Destroys:
;               ecx
;               edi
;                esi
;                esp
;
; Description:
;                Perform any essential early platform initilaisation
;               Setup a stack
;               Call the main EDKII Sec C code
;
;----------------------------------------------------------------------------

ProtectedModeEntryPoint PROC NEAR C PUBLIC

  JMP32  stackless_EarlyPlatformInit

  ;
  ; Set up stack pointer
  ;
  mov     esp, PcdGet32(PcdEsramStage1Base)
  mov     esi, QUARK_ESRAM_MEM_SIZE_BYTES
  add     esp, esi                         ; ESP = top of stack (stack grows downwards).

  ;
  ; Store the the BIST value in EBP
  ;
  mov     ebp, 00h        ; No processor BIST on Quark

  ;
  ; Push processor count to stack first, then BIST status (AP then BSP)
  ;
  mov     eax, 1
  cpuid
  shr     ebx, 16
  and     ebx, 0000000FFh
  cmp     bl, 1
  jae     PushProcessorCount

  ;
  ; Some processors report 0 logical processors.  Effectively 0 = 1.
  ; So we fix up the processor count
  ;
  inc     ebx

PushProcessorCount:
  push    ebx

  ;
  ; We need to implement a long-term solution for BIST capture.  For now, we just copy BSP BIST
  ; for all processor threads
  ;
  xor     ecx, ecx
  mov     cl, bl
PushBist:
  push    ebp
  loop    PushBist

  ;
  ; Pass Control into the PEI Core
  ;
  call PlatformSecLibStartup

  ;
  ; PEI Core should never return to here, this is just to capture an invalid return.
  ;
  jmp     $

ProtectedModeEntryPoint ENDP

;----------------------------------------------------------------------------
;
; Procedure:    stackless_EarlyPlatformInit
;
; Input:        esp - Return address
;
; Output:       None
;
; Destroys:
;                eax
;                ecx
;                dx
;                ebp
;
; Description:
;        Any essential early platform initialisation required:
;        (1) Disable Cache
;        (2) Disable NMI's/SMI's
;        (3) Setup HMBOUND (defines what memory accesses go to MMIO/RAM)
;        (4) Setup eSRAM (provide early memory to the system)
;        (5) Setup PCIEXBAR access mechanism
;        (6) Open up full SPI flash decode
;
;----------------------------------------------------------------------------
stackless_EarlyPlatformInit  PROC NEAR C PUBLIC

  ;
  ; Save return address
  ;
  mov  ebp, esp

  ;
  ; Ensure cache is disabled.
  ;
  mov     eax, cr0
  or      eax, CR0_CACHE_DISABLE + CR0_NO_WRITE
  invd
  mov     cr0, eax

  ;
  ; Disable NMI
  ; Good convention suggests you should read back RTC data port after
  ; accessing the RTC index port.
  ;
  mov  al, NMI_DISABLE
  mov  dx, RTC_INDEX
  out  dx, al
  mov  dx, RTC_DATA
  in  al, dx

  ;
  ; Disable SMI (Disables SMI wire, not SMI messages)
  ;
  mov  ecx, (OPCODE_SIDEBAND_REG_READ SHL SB_OPCODE_FIELD) OR (HOST_BRIDGE_PORT_ID SHL SB_PORT_FIELD) OR (HMISC2_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Read
  and  eax, NOT (SMI_EN)
  mov  ecx, (OPCODE_SIDEBAND_REG_WRITE SHL SB_OPCODE_FIELD) OR (HOST_BRIDGE_PORT_ID SHL SB_PORT_FIELD) OR (HMISC2_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Write

  ;
  ; Before we get going, check SOC Unit Registers to see if we are required to issue a warm/cold reset
  ;
  mov  ecx, (OPCODE_SIDEBAND_ALT_REG_READ SHL SB_OPCODE_FIELD) OR (SOC_UNIT_PORT_ID SHL SB_PORT_FIELD) OR (CFGNONSTICKY_W1_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Read
  and  eax, FORCE_WARM_RESET
  jz TestForceColdReset    ; Zero means bit clear, we're not requested to warm reset so continue as normal
  jmp IssueWarmReset

TestForceColdReset:
  mov  ecx, (OPCODE_SIDEBAND_ALT_REG_READ SHL SB_OPCODE_FIELD) OR (SOC_UNIT_PORT_ID SHL SB_PORT_FIELD) OR (CFGSTICKY_W1_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Read
  and  eax, FORCE_COLD_RESET
  jz TestHmboundLock    ; Zero means bit clear, we're not requested to cold reset so continue as normal
  jmp IssueColdReset

  ;
  ; Before setting HMBOUND, check it's not locked
  ;
TestHmboundLock:
  mov  ecx, (OPCODE_SIDEBAND_REG_READ SHL SB_OPCODE_FIELD) OR (HOST_BRIDGE_PORT_ID SHL SB_PORT_FIELD) OR (HMBOUND_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Read
  and  eax, HMBOUND_LOCK
  jz ConfigHmbound  ; Zero means bit clear, we have the config we want so continue as normal
  ;
  ; Failed to config - store sticky bit debug
  ;
  mov  ecx, (OPCODE_SIDEBAND_ALT_REG_READ SHL SB_OPCODE_FIELD) OR (SOC_UNIT_PORT_ID SHL SB_PORT_FIELD) OR (CFGSTICKY_RW_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Read
  or  eax, RESET_FOR_HMBOUND_LOCK  ; Set the bit we're interested in
  mov  ecx, (OPCODE_SIDEBAND_ALT_REG_WRITE SHL SB_OPCODE_FIELD) OR (SOC_UNIT_PORT_ID SHL SB_PORT_FIELD) OR (CFGSTICKY_RW_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Write
  jmp IssueWarmReset

  ;
  ; Set up the HMBOUND register
  ;
ConfigHmbound:
  mov  eax, HMBOUND_ADDRESS    ; Data (Set HMBOUND location)
  mov  ecx, (OPCODE_SIDEBAND_REG_WRITE SHL SB_OPCODE_FIELD) OR (HOST_BRIDGE_PORT_ID SHL SB_PORT_FIELD) OR (HMBOUND_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Write

  ;
  ; Enable interrupts to Remote Management Unit when a IMR/SMM/HMBOUND violation occurs.
  ;
  mov  eax, ENABLE_IMR_INTERRUPT        ; Data (Set interrupt enable mask)
  mov  ecx, (OPCODE_SIDEBAND_REG_WRITE SHL SB_OPCODE_FIELD) OR (MEMORY_MANAGER_PORT_ID SHL SB_PORT_FIELD) OR (BIMRVCTL_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Write

  ;
  ; Set eSRAM address
  ;
  mov  eax, PcdGet32 (PcdEsramStage1Base)        ; Data (Set eSRAM location)
  shr  eax, 18h        ; Data (Set eSRAM location)
  add eax, BLOCK_ENABLE_PG
  mov  ecx, (OPCODE_SIDEBAND_REG_WRITE SHL SB_OPCODE_FIELD) OR (MEMORY_MANAGER_PORT_ID SHL SB_PORT_FIELD) OR (ESRAMPGCTRL_BLOCK_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Write
  ;
  ; Check that we're not blocked from setting the config that we want.
  ;
  mov  ecx, (OPCODE_SIDEBAND_REG_READ SHL SB_OPCODE_FIELD) OR (MEMORY_MANAGER_PORT_ID SHL SB_PORT_FIELD) OR (ESRAMPGCTRL_BLOCK_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Read
  and  eax, BLOCK_ENABLE_PG
  jnz ConfigPci  ; Non-zero means bit set, we have the config we want so continue as normal
  ;
  ; Failed to config - store sticky bit debug
  ;
  mov  ecx, (OPCODE_SIDEBAND_ALT_REG_READ SHL SB_OPCODE_FIELD) OR (SOC_UNIT_PORT_ID SHL SB_PORT_FIELD) OR (CFGSTICKY_RW_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Read
  or  eax, RESET_FOR_ESRAM_LOCK  ; Set the bit we're interested in
  mov  ecx, (OPCODE_SIDEBAND_ALT_REG_WRITE SHL SB_OPCODE_FIELD) OR (SOC_UNIT_PORT_ID SHL SB_PORT_FIELD) OR (CFGSTICKY_RW_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Write
  jmp IssueWarmReset

  ;
  ; Enable PCIEXBAR
  ;
ConfigPci:
  mov  eax, (EC_BASE + EC_ENABLE)      ; Data
  mov  ecx, (OPCODE_SIDEBAND_REG_WRITE SHL SB_OPCODE_FIELD) OR (MEMORY_ARBITER_PORT_ID SHL SB_PORT_FIELD) OR (AEC_CTRL_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Write

  mov  eax, (EC_BASE + EC_ENABLE)      ; Data
  mov  ecx, (OPCODE_SIDEBAND_REG_WRITE SHL SB_OPCODE_FIELD) OR (HOST_BRIDGE_PORT_ID SHL SB_PORT_FIELD) OR (HECREG_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Write

  ;
  ;  Open up full 8MB SPI decode
  ;
  mov  ebx, PCI_CFG OR (ILB_PFA SHL 8) OR BDE  ; PCI Configuration address
  mov eax, DECODE_ALL_REGIONS_ENABLE
  JMP32  stackless_PCIConfig_Write

  ;
  ; Enable NMI operation
  ; Good convention suggests you should read back RTC data port after
  ; accessing the RTC index port.
  ;
  mov al, NMI_ENABLE
  mov dx, RTC_INDEX
  out dx, al
  mov dx, RTC_DATA
  in  al, dx

  ;
  ; Clear Host Bridge SMI, NMI, INTR fields
  ;
  mov  ecx, (OPCODE_SIDEBAND_REG_READ SHL SB_OPCODE_FIELD) OR (HOST_BRIDGE_PORT_ID SHL SB_PORT_FIELD) OR (HLEGACY_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Read
  and  eax, NOT(NMI + SMI + INTR)  ; Clear NMI, SMI, INTR fields
  mov  ecx, (OPCODE_SIDEBAND_REG_WRITE SHL SB_OPCODE_FIELD) OR (HOST_BRIDGE_PORT_ID SHL SB_PORT_FIELD) OR (HLEGACY_OFFSET SHL SB_ADDR_FIELD)
  JMP32  stackless_SideBand_Write

  ;
  ; Restore return address
  ;
  mov  esp, ebp
  RET32

IssueWarmReset:
  ;
  ; Issue Warm Reset request to Remote Management Unit via iLB
  ;
  mov ax, CF9_WARM_RESET
  mov  dx, ILB_RESET_REG
  out  dx, ax
  jmp  $  ; Stay here until we are reset.

IssueColdReset:
  ;
  ; Issue Cold Reset request to Remote Management Unit via iLB
  ;
  mov ax, CF9_COLD_RESET
  mov  dx, ILB_RESET_REG
  out  dx, ax
  jmp  $  ; Stay here until we are reset.

stackless_EarlyPlatformInit ENDP

;----------------------------------------------------------------------------
;
; Procedure:    stackless_SideBand_Read
;
; Input:        esp - return address
;                ecx[15:8] - Register offset
;                ecx[23:16] - Port ID
;                ecx[31:24] - Opcode
;
; Output:       eax - Data read
;
; Destroys:
;                eax
;                ebx
;                cl
;                esi
;
; Description:
;        Perform requested sideband read
;
;----------------------------------------------------------------------------
stackless_SideBand_Read  PROC NEAR C PUBLIC

  mov  esi, esp        ; Save the return address

  ;
  ; Load the SideBand Packet Register to generate the transaction
  ;
  mov  ebx, PCI_CFG OR (HOST_BRIDGE_PFA SHL 8) OR MESSAGE_BUS_CONTROL_REG  ; PCI Configuration address
  mov  cl, (ALL_BYTE_EN SHL SB_BE_FIELD)    ; Set all Byte Enable bits
  xchg  eax, ecx
  JMP32  stackless_PCIConfig_Write
  xchg  eax, ecx

  ;
  ; Read the SideBand Data Register
  ;
  mov  ebx, PCI_CFG OR (HOST_BRIDGE_PFA SHL 8) OR MESSAGE_DATA_REG    ; PCI Configuration address
  JMP32  stackless_PCIConfig_Read

  mov  esp, esi        ; Restore the return address
  RET32

stackless_SideBand_Read ENDP

;----------------------------------------------------------------------------
;
; Procedure:    stackless_SideBand_Write
;
; Input:        esp - return address
;                eax - Data
;                ecx[15:8] - Register offset
;                ecx[23:16] - Port ID
;                ecx[31:24] - Opcode
;
; Output:       None
;
; Destroys:
;                ebx
;                cl
;                esi
;
; Description:
;        Perform requested sideband write
;
;
;----------------------------------------------------------------------------
stackless_SideBand_Write  PROC NEAR C PUBLIC

  mov  esi, esp        ; Save the return address

  ;
  ; Load the SideBand Data Register with the data
  ;
  mov  ebx, PCI_CFG OR (HOST_BRIDGE_PFA SHL 8) OR MESSAGE_DATA_REG  ; PCI Configuration address
  JMP32  stackless_PCIConfig_Write

  ;
  ; Load the SideBand Packet Register to generate the transaction
  ;
  mov  ebx, PCI_CFG OR (HOST_BRIDGE_PFA SHL 8) OR MESSAGE_BUS_CONTROL_REG  ; PCI Configuration address
  mov  cl, (ALL_BYTE_EN SHL SB_BE_FIELD)    ; Set all Byte Enable bits
  xchg  eax, ecx
  JMP32  stackless_PCIConfig_Write
  xchg  eax, ecx

  mov  esp, esi        ; Restore the return address
  RET32

stackless_SideBand_Write ENDP

;----------------------------------------------------------------------------
;
; Procedure:    stackless_PCIConfig_Write
;
; Input:        esp - return address
;                eax - Data to write
;                ebx - PCI Config Address
;
; Output:       None
;
; Destroys:
;                dx
;
; Description:
;        Perform a DWORD PCI Configuration write
;
;----------------------------------------------------------------------------
stackless_PCIConfig_Write  PROC NEAR C PUBLIC

  ;
  ; Write the PCI Config Address to the address port
  ;
  xchg  eax, ebx
  mov  dx, PCI_ADDRESS_PORT
  out  dx, eax
  xchg  eax, ebx

  ;
  ; Write the PCI DWORD Data to the data port
  ;
  mov  dx, PCI_DATA_PORT
  out  dx, eax

  RET32

stackless_PCIConfig_Write ENDP

;----------------------------------------------------------------------------
;
; Procedure:    stackless_PCIConfig_Read
;
; Input:        esp - return address
;                ebx - PCI Config Address
;
; Output:       eax - Data read
;
; Destroys:
;                eax
;                dx
;
; Description:
;        Perform a DWORD PCI Configuration read
;
;----------------------------------------------------------------------------
stackless_PCIConfig_Read  PROC NEAR C PUBLIC

  ;
  ; Write the PCI Config Address to the address port
  ;
  xchg  eax, ebx
  mov  dx, PCI_ADDRESS_PORT
  out  dx, eax
  xchg  eax, ebx

  ;
  ; Read the PCI DWORD Data from the data port
  ;
  mov  dx, PCI_DATA_PORT
  in  eax, dx

  RET32

stackless_PCIConfig_Read ENDP

;
; ROM-based Global-Descriptor Table for the Tiano PEI Phase
;
align 16
PUBLIC  BootGdtTable

;
; GDT[0]: 0x00: Null entry, never used.
;
NULL_SEL        equ     $ - GDT_BASE        ; Selector [0]
GDT_BASE:
BootGdtTable    DD      0
                DD      0
;
; Linear data segment descriptor
;
LINEAR_SEL      equ     $ - GDT_BASE        ; Selector [0x8]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      092h                        ; present, ring 0, data, expand-up, writable
        DB      0CFh                        ; page-granular, 32-bit
        DB      0
;
; Linear code segment descriptor
;
LINEAR_CODE_SEL equ     $ - GDT_BASE        ; Selector [0x10]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      09Bh                        ; present, ring 0, data, expand-up, not-writable
        DB      0CFh                        ; page-granular, 32-bit
        DB      0
;
; System data segment descriptor
;
SYS_DATA_SEL    equ     $ - GDT_BASE        ; Selector [0x18]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      093h                        ; present, ring 0, data, expand-up, not-writable
        DB      0CFh                        ; page-granular, 32-bit
        DB      0

;
; System code segment descriptor
;
SYS_CODE_SEL    equ     $ - GDT_BASE        ; Selector [0x20]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      09Ah                        ; present, ring 0, data, expand-up, writable
        DB      0CFh                        ; page-granular, 32-bit
        DB      0
;
; Spare segment descriptor
;
SYS16_CODE_SEL  equ     $ - GDT_BASE        ; Selector [0x28]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0Fh
        DB      09Bh                        ; present, ring 0, code, expand-up, writable
        DB      00h                         ; byte-granular, 16-bit
        DB      0
;
; Spare segment descriptor
;
SYS16_DATA_SEL  equ     $ - GDT_BASE        ; Selector [0x30]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      093h                        ; present, ring 0, data, expand-up, not-writable
        DB      00h                         ; byte-granular, 16-bit
        DB      0

;
; Spare segment descriptor
;
SPARE5_SEL      equ     $ - GDT_BASE        ; Selector [0x38]
        DW      0                           ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      0                           ; present, ring 0, data, expand-up, writable
        DB      0                           ; page-granular, 32-bit
        DB      0
GDT_SIZE        EQU     $ - BootGDTtable    ; Size, in bytes

;
; GDT Descriptor
;
GdtDesc:                                    ; GDT descriptor
        DW      GDT_SIZE - 1                ; GDT limit
        DD      OFFSET BootGdtTable         ; GDT base address

ProtectedModeEntryLinearAddress   LABEL   FWORD
ProtectedModeEntryLinearOffset    LABEL   DWORD
  DD      OFFSET ProtectedModeEntryPoint  ; Offset of our 32 bit code
  DW      LINEAR_CODE_SEL

END

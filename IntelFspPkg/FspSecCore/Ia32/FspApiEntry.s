#------------------------------------------------------------------------------
#
# Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Abstract:
#
#   Provide FSP API entry points.
#
#------------------------------------------------------------------------------

#.INCLUDE "UcodeLoad.inc"

#
# Following are fixed PCDs
#

.equ MSR_IA32_PLATFORM_ID,	0x000000017
.equ MSR_IA32_BIOS_UPDT_TRIG,    0x000000079
.equ MSR_IA32_BIOS_SIGN_ID,     0x00000008b

ASM_GLOBAL    ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamBase)
ASM_GLOBAL    ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamSize)
ASM_GLOBAL    ASM_PFX(_gPcd_FixedAtBuild_PcdFspTemporaryRamSize)
ASM_GLOBAL    ASM_PFX(_gPcd_FixedAtBuild_PcdFspAreaSize)


#
# Following functions will be provided in C
#
#EXTERNDEF   SecStartup:PROC
#EXTERNDEF   FspApiCallingCheck:PROC

#
# Following functions will be provided in PlatformSecLib
#
#EXTERNDEF   GetFspBaseAddress:PROC
#EXTERNDEF   GetBootFirmwareVolumeOffset:PROC
#EXTERNDEF   PlatformTempRamInit:PROC
#EXTERNDEF   Pei2LoaderSwitchStack:PROC
#EXTERN      FspSelfCheck(FspSelfCheckDflt):PROC
#EXTERN      PlatformBasicInit(PlatformBasicInitDflt):PROC

#
# Define the data length that we saved on the stack top
#
.equ                     DATA_LEN_OF_PER0, 0x018
.equ                     DATA_LEN_OF_MCUD, 0x018
.equ                     DATA_LEN_AT_STACK_TOP, (DATA_LEN_OF_PER0 + DATA_LEN_OF_MCUD + 4)

#
# Define SSE macros
#
.macro ENABLE_SSE
  movl        %cr4, %eax
  orl         $0x00000600,%eax      # Set OSFXSR bit (bit #9) & OSXMMEXCPT bit (bit #10)
  movl        %eax,%cr4
.endm

.macro SAVE_REGS
  movd       %ebp, %xmm7
  pshufd     $0x93, %xmm7, %xmm7
  movd       %ebx, %xmm6
  por        %xmm6, %xmm7
  pshufd     $0x93, %xmm7, %xmm7
  movd       %esi,%xmm6
  por        %xmm6, %xmm7
  pshufd     $0x93, %xmm7, %xmm7
  movd       %edi, %xmm6
  por        %xmm6, %xmm7
  movd       %esp, %xmm6
.endm

.macro LOAD_REGS
  movd       %xmm6, %esp
  movd       %xmm7, %edi
  pshufd     $0x39,%xmm7, %xmm7
  movd       %xmm7, %esi
  pshufd     $0x39,%xmm7, %xmm7
  movd       %xmm7, %ebx
  pshufd     $0x39, %xmm7, %xmm7
  movd       %xmm7, %ebp
.endm

.macro LOAD_ESP
  movd       %xmm6, %esp
.endm

#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(FspSelfCheckDflt)
ASM_PFX(FspSelfCheckDflt):
   # Inputs:
   #   eax -> Return address
   # Outputs:
   #   eax -> 0 - Successful, Non-zero - Failed.
   # Register Usage:
   #   eax is cleared and ebp is used for return address.
   #   All others reserved.

   # Save return address to EBP
   movl  %eax, %ebp
   xorl  %eax, %eax
exit:
   jmp   *%ebp
#FspSelfCheckDflt   ENDP

#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(PlatformBasicInitDflt)
ASM_PFX(PlatformBasicInitDflt):
   # Inputs:
   #   eax -> Return address
   # Outputs:
   #   eax -> 0 - Successful, Non-zero - Failed.
   # Register Usage:
   #   eax is cleared and ebp is used for return address.
   #   All others reserved.

   # Save return address to EBP
   movl   %eax, %ebp
   xorl   %eax, %eax
exit2:
   jmp   *%ebp
#PlatformBasicInitDflt   ENDP

#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(LoadUcode)
ASM_PFX(LoadUcode):
   # Inputs:
   #   esp -> LOAD_UCODE_PARAMS pointer
   # Register Usage:
   #   esp  Preserved
   #   All others destroyed
   # Assumptions:
   #   No memory available, stack is hard-coded and used for return address
   #   Executed by SBSP and NBSP
   #   Beginning of microcode update region starts on paragraph boundary

   #
   #
   # Save return address to EBP
   movl   %eax, %ebp
   cmpl   $0, %esp
   jz     paramerror
   movl   (%esp), %eax        #dword ptr []     Parameter pointer
   cmpl   $0, %eax
   jz     paramerror
   movl   %eax, %esp
   movl   (%esp), %esi        #LOAD_UCODE_PARAMS.ucode_code_addr
   cmpl   $0, %esi
   jnz    L0

paramerror:
   movl    $0x080000002, %eax
   jmp     exit4

   movl   (%esp), %esi     #.LOAD_UCODE_PARAMS.ucode_code_addr

check_main_header:
   # Get processor signature and platform ID from the installed processor
   # and save into registers for later use
   # ebx = processor signature
   # edx = platform ID
   movl  $1, %eax
   cpuid
   movl   %eax, %ebx
   movl   MSR_IA32_PLATFORM_ID, %ecx
   rdmsr
   movl  %edx, %ecx
   #--------------------------------------------------------------------------------------------------------------------
   shrl  $18, %ecx      #($50-$32)
   andl  $0x7, %ecx
   movl  $1, %edx
   shll  %cl,%edx

   # Current register usage
   # esp -> stack with paramters
   # esi -> microcode update to check
   # ebx = processor signature
   # edx = platform ID

   # Check for valid microcode header
   # Minimal test checking for header version and loader version as 1
   movl  $1, %eax
   cmpl  %eax, (%esi)           #.ucode_hdr.version
   jne   advance_fixed_size
   cmpl  %eax, 0x18(%esi)       #.ucode_hdr.loader
   jne   advance_fixed_size

   # Check if signature and plaform ID match
   #--------------------------------------------------------------------------------------------------------------------------
   cmpl  0x10(%esi), %ebx      #(%esi).ucode_hdr.processor
   jne   L0
   testl 0x1c(%esi) , %edx     #(%esi).ucode_hdr.flags
   jnz   load_check  # Jif signature and platform ID match

L0:
   # Check if extended header exists
   # First check if total_size and data_size are valid
   xorl    %eax, %eax
   cmpl    %eax,0x24(%esi)      #(%esi).ucode_hdr.total_size
   je      next_microcode
   cmpl    %eax,0x20(%esi)      #(%esi) .ucode_hdr.data_size
   je      next_microcode

   # Then verify total size - sizeof header > data size
   movl  0x24(%esi), %ecx        #(%esi).ucode_hdr.total_size
   subl  $0x30, %ecx             #sizeof ucode_hdr = 48
   cmpl  0x20(%esi), %ecx        #(%esi).ucode_hdr.data_size
   jz    load_check
   jb    next_microcode    # Jif extended header does not exist

   # Check if total size fits in microcode region
   movl    %esi , %edi
   addl    0x24(%esi), %edi       # (%esi).ucode_hdr.total_size
   movl    (%esp), %ecx           # (%esp).LOAD_UCODE_PARAMS.ucode_code_addr
   addl    4(%esp), %ecx          #.LOAD_UCODE_PARAMS.ucode_code_size
   cmpl    %ecx , %edi
   xorl    %eax,  %eax
   ja      exit4              # Jif address is outside of ucode region

   # Set edi -> extended header
   movl   %esi , %edi
   addl   $0x30 , %edi               #sizeof ucode_hdr = 48
   addl   0x20(%esi), %edi           #%esi.ucode_hdr.data_size

   # Get count of extended structures
   movl   (%edi), %ecx               #(%edi).ext_sig_hdr.count

   # Move pointer to first signature structure
   addl  $0x20, %edi                      # sizeof ext_sig_hdr = 20

check_ext_sig:
   # Check if extended signature and platform ID match
   cmpl   %ebx, (%edi)                #[edi].ext_sig.processor
   jne   L1
   test  %edx, 4(%edi)                #[edi].ext_sig.flags
   jnz   load_check     # Jif signature and platform ID match
L9:
   # Check if any more extended signatures exist
   addl   $0xc, %edi                  #sizeof ext_sig = 12
   loop   check_ext_sig

next_microcode:
   # Advance just after end of this microcode
   xorl   %eax, %eax
   cmpl   %eax, 0x24(%esi)            #(%esi).ucode_hdr.total_size
   je     L2
   add    0x24(%esi) ,  %esi          #(%esi).ucode_hdr.total_size
   jmp    check_address
L10:
   addl  $0x800, %esi
   jmp   check_address

advance_fixed_size:
   # Advance by 4X dwords
   addl   $0x400, %esi

check_address:
   # Is valid Microcode start point ?
   cmp   $0x0ffffffff , %esi
   jz    done

   # Address >= microcode region address + microcode region size?
   movl   (%esp), %eax                  #(%esp).LOAD_UCODE_PARAMS.ucode_code_addr
   addl   4(%esp), %eax                   #(%esp).LOAD_UCODE_PARAMS.ucode_code_size
   cmpl   %eax, %esi
   jae    done        #Jif address is outside of ucode region
   jmp    check_main_header

load_check:
   # Get the revision of the current microcode update loaded
   movl   MSR_IA32_BIOS_SIGN_ID, %ecx
   xorl   %eax, %eax               # Clear EAX
   xorl   %edx, %edx               # Clear EDX
   wrmsr                           # Load 0 to MSR at 8Bh

   movl   $1, %eax
   cpuid
   movl   MSR_IA32_BIOS_SIGN_ID, %ecx
   rdmsr                         # Get current microcode signature

   # Verify this microcode update is not already loaded
   cmpl   %edx,  4(%esi)         #(%esi).ucode_hdr.revision
   je    continue

load_microcode:
   # EAX contains the linear address of the start of the Update Data
   # EDX contains zero
   # ECX contains 79h (IA32_BIOS_UPDT_TRIG)
   # Start microcode load with wrmsr
   mov   %esi, %eax
   add   $0x30, %eax                    #sizeof ucode_hdr = 48
   xorl  %edx, %edx
   mov   MSR_IA32_BIOS_UPDT_TRIG,%ecx
   wrmsr
   mov  $1, %eax
   cpuid

continue:
   jmp   next_microcode

done:
   mov   $1, %eax
   cpuid
   mov   MSR_IA32_BIOS_SIGN_ID, %ecx
   rdmsr                         # Get current microcode signature
   xorl   %eax, %eax
   cmp    $0 , %edx
   jnz   exit4
   mov   $0x08000000E, %eax

exit4:
   jmp   *%ebp

#LoadUcode   ENDP

#----------------------------------------------------------------------------
# TempRamInit API
#
# This FSP API will load the microcode update, enable code caching for the
# region specified by the boot loader and also setup a temporary stack to be
# used till main memory is initialized.
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(TempRamInitApi)
ASM_PFX(TempRamInitApi):
  #
  # Ensure SSE is enabled
  #
  ENABLE_SSE

  #
  # Save EBP, EBX, ESI, EDI & ESP in XMM7 & XMM6
  #
  SAVE_REGS

  #
  # Save timestamp into XMM4 & XMM5
  #
  rdtsc
  movd      %edx, %xmm4
  movd      %eax, %xmm5

  #
  # CPUID/DeviceID check
  #
  movl       L11, %eax
  jmp       ASM_PFX(FspSelfCheck)  # Note: ESP can not be changed.
L11:
  cmpl       $0, %eax
  jnz       NemInitExit

  #
  # Platform Basic Init.
  #
  movl       L1, %eax
  jmp       ASM_PFX(PlatformBasicInitDflt)
L1:
  cmp       $0, %eax
  jnz       NemInitExit

  #
  # Load microcode
  #
  movl       L2, %eax
  addl       $4, %esp
  jmp       LoadUcode
L2:
  LOAD_ESP
  cmpl       $0, %eax
  jnz       NemInitExit

  #
  # Call platform NEM init
  #-------------------------------------------------------------------------------------------------------------------------
  movl       L3, %eax
  addl       $4, %esp
  jmp       ASM_PFX(PlatformTempRamInit)
L3:
  subl       $4, %esp
  cmpl       $0, %eax
  jnz       NemInitExit

  #
  # Save parameter pointer in edx
  #
  movl       4(%esp), %edx

  #
  # Enable FSP STACK
  #
  movl       ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamBase), %esp
  addl       ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamSize), %esp

  pushl      $DATA_LEN_OF_MCUD     # Size of the data region
  pushl      0x4455434D            # Signature of the  data region 'MCUD'
  pushl      12(%edx)             # Code size
  pushl      8(%edx)               # Code base
  cmpl       $0, %edx              # Is parameter pointer valid ?
  jz         InvalidMicrocodeRegion
  pushl      4(%edx)               # Microcode size
  pushl      (%edx)                # Microcode base
  jmp        L4

InvalidMicrocodeRegion:
  pushl      $0                    # Microcode size
  pushl      $0                    # Microcode base

L4:
  #
  # Save API entry/exit timestamp into stack
  #
  pushl      DATA_LEN_OF_PER0      # Size of the data region
  pushl      0x30524550            # Signature of the  data region 'PER0'
  movd       %xmm4, %eax
  pushl      %eax
  movd       %xmm5, %eax
  pushl      %eax
  rdtsc
  pushl      %edx
  pushl      %eax

  #
  # Terminator for the data on stack
  #
  pushl      $0

  #
  # Set ECX/EDX to the bootloader temporary memory range
  #
  movl       ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamBase), %ecx
  movl       %ecx, %edx
  addl       ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamSize), %edx
  subl       ASM_PFX(_gPcd_FixedAtBuild_PcdFspTemporaryRamSize), %edx

  xorl       %eax, %eax

NemInitExit:
  #
  # Load EBP, EBX, ESI, EDI & ESP from XMM7 & XMM6
  #
  LOAD_REGS
  ret
#TempRamInitApi   ENDP

#----------------------------------------------------------------------------
# FspInit API
#
# This FSP API will perform the processor and chipset initialization.
# This API will not return.  Instead, it transfers the control to the
# ContinuationFunc provided in the parameter.
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(FspInitApi)
ASM_PFX(FspInitApi):
  #
  # Stack must be ready
  #
  pushl   $0x087654321
  pop     %eax
  cmpl    $0x087654321, %eax
  jz     L5
  movl    $0x080000003, %eax
  jmp    exit3

L5:
  #
  # Additional check
  #
  pusha
  pushl   $1
  call    ASM_PFX(FspApiCallingCheck)
  addl    $4,    %esp
  movl    %eax,  28(%esp)
  popa
  cmpl    $0 , %eax
  jz      L6
  jmp     exit3

L6:
  #
  # Save the Platform Data Pointer in EDI
  #
  movl    4(%esp), %edi

  #
  # Store the address in FSP which will return control to the BL
  #
  pushl   $exit3

  #
  # Create a Task Frame in the stack for the Boot Loader
  #
  pushfl
  pushfl     # 2 pushf for 4 byte alignment
  cli
  pushal

  # Reserve 8 bytes for IDT save/restore
  pushl    $0
  pushl    $0
  sidt     (%esp)

  #
  # Setup new FSP stack
  #
  movl     %esp, %eax
  movl     ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamBase), %esp
  addl     ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamSize)  , %esp
  subl     DATA_LEN_AT_STACK_TOP, %esp
  addl     $0x0FFFFFFC0, %esp

  #
  # Save the bootloader's stack pointer
  #
  pushl    %eax

  #
  # Pass entry point of the PEI core
  #
  call     ASM_PFX(GetFspBaseAddress)
  movl     %eax, %edi
  addl     ASM_PFX(_gPcd_FixedAtBuild_PcdFspAreaSize), %edi
  subl     $0x20, %edi
  addl     %ds:(%edi), %eax
  pushl    %eax

  #
  # Pass BFV into the PEI Core
  # It uses relative address to calucate the actual boot FV base
  # For FSP impleantion with single FV, PcdFlashFvRecoveryBase and
  # PcdFspAreaBaseAddress are the same. For FSP with mulitple FVs,
  # they are different. The code below can handle both cases.
  #
  call     ASM_PFX(GetFspBaseAddress)
  movl     %eax , %edi
  call     ASM_PFX(GetBootFirmwareVolumeOffset)
  addl     %edi ,%eax
  pushl    %eax

  #
  # Pass stack base and size into the PEI Core
  #
  movl     ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamBase), %eax
  addl     ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamSize), %eax
  subl     ASM_PFX(_gPcd_FixedAtBuild_PcdFspTemporaryRamSize), %eax
  pushl    %eax
  pushl    ASM_PFX(_gPcd_FixedAtBuild_PcdFspTemporaryRamSize)

  #
  # Pass Control into the PEI Core
  #
  call    ASM_PFX(SecStartup)

exit3:
  ret

# FspInitApi   ENDP

#----------------------------------------------------------------------------
# NotifyPhase API
#
# This FSP API will notify the FSP about the different phases in the boot
# process
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(NotifyPhaseApi)
ASM_PFX(NotifyPhaseApi):
  #
  # Stack must be ready
  #
  pushl  $0x0087654321
  pop    %eax
  cmpl   $0x087654321, %eax
  jz     L7
  movl   $0x080000003, %eax
  jmp    err_exit

L7:
  #
  # Verify the calling condition
  #
  pusha
  pushl   $2
  call    ASM_PFX(FspApiCallingCheck)
  add     $4, %esp
  mov     %eax, 28(%esp)
  popa

  cmpl   $0, %eax
  jz     L8

  #
  # Error return
  #
err_exit:
  ret

L8:
  jmp    ASM_PFX(Pei2LoaderSwitchStack)

#NotifyPhaseApi   ENDP


#END

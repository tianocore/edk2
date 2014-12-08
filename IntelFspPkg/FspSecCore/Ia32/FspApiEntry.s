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

#.INCLUDE   "UcodeLoadGcc.inc" - begin

.equ MSR_IA32_PLATFORM_ID,                   0x00000017
.equ MSR_IA32_BIOS_UPDT_TRIG,                0x00000079
.equ MSR_IA32_BIOS_SIGN_ID,                  0x0000008b

Ucode:
.equ        UcodeVersion,                    0x0000
.equ        UcodeRevision,                   0x0004
.equ        UcodeDate,                       0x0008
.equ        UcodeProcessor,                  0x000C
.equ        UcodeChecksum,                   0x0010
.equ        UcodeLoader,                     0x0014
.equ        UcodeRsvd,                       0x0018
UcodeEnd:

UcodeHdr:
.equ        UcodeHdrVersion,                 0x0000
.equ        UcodeHdrRevision,                0x0004
.equ        UcodeHdrDate,                    0x0008
.equ        UcodeHdrProcessor,               0x000c
.equ        UcodeHdrChecksum,                0x0010
.equ        UcodeHdrLoader,                  0x0014
.equ        UcodeHdrFlags,                   0x0018
.equ        UcodeHdrDataSize,                0x001C
.equ        UcodeHdrTotalSize,               0x0020
.equ        UcodeHdrRsvd,                    0x0024
UcodeHdrEnd:
.equ        UcodeHdrLength,                  0x0030  # UcodeHdrLength = UcodeHdrEnd - UcodeHdr


ExtSigHdr:
.equ        ExtSigHdrCount,                  0x0000
.equ        ExtSigHdrChecksum,               0x0004
.equ        rsvd,                            0x0008
ExtSigHdrEnd:
.equ        ExtSigHdrLength,                 0x0014  #ExtSigHdrLength = ExtSigHdrEnd - ExtSigHdr

ExtSig:
.equ        ExtSigProcessor,                 0x0000
.equ        ExtSigFlags,                     0x0004
.equ        ExtSigChecksum,                  0x0008
ExtSigEnd:
.equ        ExtSigLength,                    0x000C  #ExtSigLength = ExtSigEnd - ExtSig

LoadUcodeParams:
.equ        LoadUcodeParamsUcodeCodeAddr,    0x0000
.equ        LoadUcodeParamsUcodeCodeSize,    0x0004
LoadUcodeParamsEnd:

#.INCLUDE   "UcodeLoadGcc.inc" - end

#.INCLUDE   "SaveRestoreSseGcc.inc" - begin

.macro SAVE_REGS
  pinsrw     $0x00, %ebp, %xmm7
  ror        $0x10, %ebp
  pinsrw     $0x01, %ebp, %xmm7
  ror        $0x10, %ebp
#
  pinsrw     $0x02, %ebx, %xmm7
  ror        $0x10, %ebx
  pinsrw     $0x03, %ebx, %xmm7
  ror        $0x10, %ebx
#
  pinsrw     $0x04, %esi, %xmm7
  ror        $0x10, %esi
  pinsrw     $0x05, %esi, %xmm7
  ror        $0x10, %esi
#
  pinsrw     $0x06, %edi, %xmm7
  ror        $0x10, %edi
  pinsrw     $0x07, %edi, %xmm7
  ror        $0x10, %edi
#
  pinsrw     $0x00, %esp, %xmm6
  ror        $0x10, %esp
  pinsrw     $0x01, %esp, %xmm6
  ror        $0x10, %esp
.endm

.macro LOAD_REGS
  pshufd     $0xe4, %xmm7, %xmm7
  movd       %xmm7, %ebp 
  pshufd     $0xe4, %xmm7, %xmm7
#
  pshufd     $0x39, %xmm7, %xmm7
  movd       %xmm7, %ebx
  pshufd     $0x93, %xmm7, %xmm7
#
  pshufd     $0x4e, %xmm7, %xmm7
  movd       %xmm7, %esi
  pshufd     $0x4e, %xmm7, %xmm7
#
  pshufd     $0x93, %xmm7, %xmm7
  movd       %xmm7, %edi
  pshufd     $0x39, %xmm7, %xmm7
#
  movd       %xmm6, %esp
.endm

.macro LOAD_EAX
  pshufd     $0x39, %xmm6, %xmm6
  movd       %xmm6, %eax
  pshufd     $0x93, %xmm6, %xmm6
.endm

.macro LOAD_EDX
  pshufd     $0xe4, %xmm6, %xmm6
  movd       %xmm6, %edx
  pshufd     $0xe4, %xmm6, %xmm6
.endm

.macro SAVE_EAX
  pinsrw     $0x02, %eax, %xmm6
  ror        $0x10, %eax
  pinsrw     $0x03, %eax, %xmm6
  ror        $0x10, %eax
.endm

.macro SAVE_EDX
  pinsrw     $0x04, %edx, %xmm6
  ror        $0x10, %edx
  pinsrw     $0x05, %edx, %xmm6
  ror        $0x10, %edx
.endm

.macro LOAD_ESP
  movd       %xmm6, %esp
.endm

.macro ENABLE_SSE
  movl       %cr4, %eax
  orl        $0x00000600, %eax               # Set OSFXSR bit (bit #9) & OSXMMEXCPT bit (bit #10)
  movl       %eax,%cr4
.endm

#.INCLUDE   "SaveRestoreSseGcc.inc" - end


#
# Following are fixed PCDs
#
ASM_GLOBAL    ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamBase)
ASM_GLOBAL    ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamSize)
ASM_GLOBAL    ASM_PFX(_gPcd_FixedAtBuild_PcdFspTemporaryRamSize)

#
# Following functions will be provided in C
#
ASM_GLOBAL    ASM_PFX(FspImageSizeOffset)
ASM_GLOBAL    ASM_PFX(SecStartup)
ASM_GLOBAL    ASM_PFX(FspApiCallingCheck)

#
# Following functions will be provided in PlatformSecLib
#
ASM_GLOBAL    ASM_PFX(GetBootFirmwareVolumeOffset)
ASM_GLOBAL    ASM_PFX(Pei2LoaderSwitchStack)


#
# Define the data length that we saved on the stack top
#
.equ          DATA_LEN_OF_PER0, 0x018
.equ          DATA_LEN_OF_MCUD, 0x018
.equ          DATA_LEN_AT_STACK_TOP, (DATA_LEN_OF_PER0 + DATA_LEN_OF_MCUD + 4)

#------------------------------------------------------------------------------
# FspSelfCheckDflt
# Inputs:
#   eax -> Return address
# Outputs:
#   eax -> 0 - Successful, Non-zero - Failed.
# Register Usage:
#   eax is cleared and ebp is used for return address.
#   All others reserved.
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(FspSelfCheckDflt)
ASM_PFX(FspSelfCheckDflt):
   #
   # Save return address to EBP
   #
   movl  %eax, %ebp
   xorl  %eax, %eax

FspSelfCheckDfltExit:
   jmp   *%ebp


#------------------------------------------------------------------------------
# PlatformBasicInitDflt
# Inputs:
#   eax -> Return address
# Outputs:
#   eax -> 0 - Successful, Non-zero - Failed.
# Register Usage:
#   eax is cleared and ebp is used for return address.
#   All others reserved.
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(PlatformBasicInitDflt)
ASM_PFX(PlatformBasicInitDflt):
   #
   # Save return address to EBP
   #
   movl   %eax, %ebp
   xorl   %eax, %eax

PlatformBasicInitDfltExit:
   jmp   *%ebp


#------------------------------------------------------------------------------
# LoadUcode
#
# Inputs:
#   esp -> LOAD_UCODE_PARAMS pointer
# Register Usage:
#   esp  Preserved
#   All others destroyed
# Assumptions:
#   No memory available, stack is hard-coded and used for return address
#   Executed by SBSP and NBSP
#   Beginning of microcode update region starts on paragraph boundary
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(LoadUcode)
ASM_PFX(LoadUcode):   
   #
   # Save return address to EBP
   #
   movl   %eax, %ebp
   cmpl   $0x00, %esp
   jz     ParamError
   movl   (%esp), %eax                       #dword ptr []     Parameter pointer
   cmpl   $0x00, %eax
   jz     ParamError
   movl   %eax, %esp
   movl   LoadUcodeParamsUcodeCodeAddr(%esp), %esi          #mov    esi, [esp].LOAD_UCODE_PARAMS.ucode_code_addr
   cmpl   $0x00, %esi
   jnz    CheckMainHeader

ParamError:
   movl   $0x080000002, %eax
   jmp    LoadUcodeExit

CheckMainHeader:
   #
   # Get processor signature and platform ID from the installed processor
   # and save into registers for later use
   # ebx = processor signature
   # edx = platform ID
   #
   movl   $0x01, %eax
   cpuid
   movl   %eax, %ebx
   movl   $MSR_IA32_PLATFORM_ID, %ecx
   rdmsr
   movl   %edx, %ecx
   shrl   $0x12, %ecx                        #($50-$32)
   andl   $0x07, %ecx
   movl   $0x01, %edx
   shll   %cl,%edx

   #
   # Current register usage
   # esp -> stack with paramters
   # esi -> microcode update to check
   # ebx = processor signature
   # edx = platform ID
   #

   #
   # Check for valid microcode header
   # Minimal test checking for header version and loader version as 1
   #
   movl   $0x01, %eax
   cmpl   %eax, UcodeHdrVersion(%esi)        #cmp   [esi].ucode_hdr.version, eax
   jne    AdvanceFixedSize
   cmpl   %eax, UcodeHdrLoader(%esi)         #cmp   [esi].ucode_hdr.loader, eax
   jne    AdvanceFixedSize

   #
   # Check if signature and plaform ID match
   #
   cmpl   UcodeHdrProcessor(%esi), %ebx      #cmp   ebx, [esi].ucode_hdr.processor 
   jne    LoadUcodeL0
   testl  UcodeHdrFlags(%esi), %edx          #test  edx, [esi].ucode_hdr.flags
   jnz    LoadCheck                          #Jif signature and platform ID match

LoadUcodeL0:
   #
   # Check if extended header exists
   # First check if total_size and data_size are valid
   #
   xorl   %eax, %eax
   cmpl   %eax, UcodeHdrTotalSize(%esi)      #cmp   [esi].ucode_hdr.total_size, eax
   je     NextMicrocode
   cmpl   %eax, UcodeHdrDataSize(%esi)       #cmp   [esi].ucode_hdr.data_size, eax
   je     NextMicrocode

   #
   # Then verify total size - sizeof header > data size
   #
   movl   UcodeHdrTotalSize(%esi), %ecx      #mov   ecx, [esi].ucode_hdr.total_size
   subl   $UcodeHdrLength, %ecx              #sub   ecx, sizeof ucode_hdr
   cmpl   UcodeHdrDataSize(%esi), %ecx       #cmp   ecx, [esi].ucode_hdr.data_size
   jle NextMicrocode                         

   #
   # Set edi -> extended header
   #
   movl   %esi, %edi
   addl   $UcodeHdrLength, %edi              #add   edi, sizeof ucode_hdr
   addl   UcodeHdrDataSize(%esi), %edi       #add   edi, [esi].ucode_hdr.data_size

   #
   # Get count of extended structures
   #
   movl   ExtSigHdrCount(%edi), %ecx         #mov   ecx, [edi].ext_sig_hdr.count

   #
   # Move pointer to first signature structure
   #
   addl   ExtSigHdrLength, %edi              #add   edi, sizeof ext_sig_hdr

CheckExtSig:
   #
   # Check if extended signature and platform ID match
   #
   cmpl   %ebx, ExtSigProcessor(%edi)        #cmp   [edi].ext_sig.processor, ebx
   jne    LoadUcodeL1
   test   %edx, ExtSigFlags(%edi)            #test  [edi].ext_sig.flags, edx
   jnz    LoadCheck                          # Jif signature and platform ID match
LoadUcodeL1:
   #
   # Check if any more extended signatures exist
   #
   addl   $ExtSigLength, %edi                #add   edi, sizeof ext_sig
   loop   CheckExtSig

NextMicrocode:
   #
   # Advance just after end of this microcode
   #
   xorl   %eax, %eax
   cmpl   %eax, UcodeHdrTotalSize(%esi)      #cmp   [esi].ucode_hdr.total_size, eax
   je     LoadUcodeL2
   addl   UcodeHdrTotalSize(%esi), %esi      #add   esi, [esi].ucode_hdr.total_size
   jmp    CheckAddress
LoadUcodeL2:
   addl   $0x800, %esi                       #add   esi, dword ptr 2048
   jmp    CheckAddress

AdvanceFixedSize:
   #
   # Advance by 4X dwords
   #
   addl   $0x400, %esi                       #add   esi, dword ptr 1024

CheckAddress:
   #
   # Is valid Microcode start point ?
   #
   cmpl   $0x0ffffffff, UcodeHdrVersion(%esi)

   #
   # Is automatic size detection ?
   #
   movl   LoadUcodeParamsUcodeCodeSize(%esp), %eax
   cmpl   $0x0ffffffff, %eax
   jz     LoadUcodeL3
   #
   # Address >= microcode region address + microcode region size?
   #
   addl   LoadUcodeParamsUcodeCodeAddr(%esp), %eax                    #mov   eax, [esp].LOAD_UCODE_PARAMS.ucode_code_addr

   cmpl   %eax, %esi
   jae    Done                               #Jif address is outside of ucode region
   jmp    CheckMainHeader

LoadUcodeL3:
LoadCheck:
   #
   # Get the revision of the current microcode update loaded
   #
   movl   $MSR_IA32_BIOS_SIGN_ID, %ecx
   xorl   %eax, %eax                         # Clear EAX
   xorl   %edx, %edx                         # Clear EDX
   wrmsr                                     # Load 0 to MSR at 8Bh

   movl   $0x01, %eax
   cpuid
   movl   $MSR_IA32_BIOS_SIGN_ID, %ecx
   rdmsr                                     # Get current microcode signature

   #
   # Verify this microcode update is not already loaded
   #
   cmpl   %edx, UcodeHdrRevision(%esi)       #cmp   [esi].ucode_hdr.revision, edx
   je     Continue

LoadMicrocode:
   #
   # EAX contains the linear address of the start of the Update Data
   # EDX contains zero
   # ECX contains 79h (IA32_BIOS_UPDT_TRIG)
   # Start microcode load with wrmsr
   #
   movl   %esi, %eax
   addl   $UcodeHdrLength, %eax              #add   eax, sizeof ucode_hdr
   xorl   %edx, %edx
   movl   $MSR_IA32_BIOS_UPDT_TRIG, %ecx
   wrmsr
   movl   $0x01, %eax
   cpuid

Continue:
   jmp    NextMicrocode

Done:
   movl   $0x01, %eax
   cpuid
   movl   $MSR_IA32_BIOS_SIGN_ID, %ecx
   rdmsr                                     # Get current microcode signature
   xorl   %eax, %eax
   cmpl   $0x00, %edx
   jnz    LoadUcodeExit
   movl   $0x08000000E, %eax

LoadUcodeExit:
   jmp   *%ebp


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
  SAVE_EAX
  SAVE_EDX

  #
  # Check Parameter
  #
  movl    4(%esp), %eax
  cmpl    $0x00, %eax
  movl    $0x80000002, %eax
  jz      NemInitExit

  #
  # CPUID/DeviceID check
  #
  movl    $TempRamInitApiL0, %eax
  jmp     ASM_PFX(FspSelfCheckDflt)  # Note: ESP can not be changed.
TempRamInitApiL0:
  cmpl    $0x00, %eax
  jnz     NemInitExit

  #
  # Platform Basic Init.
  #
  movl    $TempRamInitApiL1, %eax
  jmp     ASM_PFX(PlatformBasicInitDflt)
TempRamInitApiL1:
  cmpl    $0x00, %eax
  jnz     NemInitExit

  #
  # Load microcode
  #
  movl    $TempRamInitApiL2, %eax
  addl    $0x04, %esp
  jmp     LoadUcode

TempRamInitApiL2:
  LOAD_ESP
  cmpl    $0x00, %eax
  jnz     NemInitExit

  #
  # Call platform NEM init
  #
  movl    $TempRamInitApiL3, %eax
  addl    $0x04, %esp
  jmp     ASM_PFX(PlatformTempRamInit)
TempRamInitApiL3:
  subl    $0x04, %esp
  cmpl    $0x00, %eax
  jnz     NemInitExit

  #
  # Save parameter pointer in edx
  #
  movl    4(%esp), %edx

  #
  # Enable FSP STACK
  #
  movl    ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamBase), %esp
  addl    ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamSize), %esp

  pushl   $DATA_LEN_OF_MCUD     # Size of the data region
  pushl   $0x4455434D           # Signature of the  data region 'MCUD'
  pushl   4(%edx)               # Microcode size
  pushl   (%edx)                # Microcode base
  pushl   12(%edx)              # Code size
  pushl   8(%edx)               # Code base

  #
  # Save API entry/exit timestamp into stack
  #
  pushl   $DATA_LEN_OF_PER0      # Size of the data region
  pushl   $0x30524550            # Signature of the  data region 'PER0'
  rdtsc
  pushl   %edx
  pushl   %eax
  LOAD_EAX
  LOAD_EDX
  pushl   %edx
  pushl   %eax

  #
  # Terminator for the data on stack
  #
  pushl   $0x00

  #
  # Set ECX/EDX to the bootloader temporary memory range
  #
  movl    ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamBase), %ecx
  movl    %ecx, %edx
  addl    ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamSize), %edx
  subl    ASM_PFX(_gPcd_FixedAtBuild_PcdFspTemporaryRamSize), %edx

  xorl    %eax, %eax

NemInitExit:
  #
  # Load EBP, EBX, ESI, EDI & ESP from XMM7 & XMM6
  #
  LOAD_REGS
  ret


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
  popl    %eax
  cmpl    $0x087654321, %eax
  jz      FspInitApiL0
  movl    $0x080000003, %eax
  jmp     FspInitApiexit

FspInitApiL0:
  #
  # Additional check
  #
  pusha
  pushl   $0x01
  call    ASM_PFX(FspApiCallingCheck)
  addl    $0x04, %esp
  movl    %eax, 28(%esp)
  popa
  cmpl    $0x00, %eax
  jz      FspInitApiL1
  jmp     FspInitApiexit

FspInitApiL1:
  #
  # Store the address in FSP which will return control to the BL
  #
  pushl   $FspInitApiexit

  #
  # Create a Task Frame in the stack for the Boot Loader
  #
  pushfl     # 2 pushf for 4 byte alignment
  cli
  pushal

  #
  # Reserve 8 bytes for IDT save/restore
  #
  subl    $0x08, %esp
  sidt    (%esp)

  #
  # Setup new FSP stack
  #
  movl    %esp, %eax
  movl    ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamBase), %esp
  addl    ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamSize), %esp
  subl    $(DATA_LEN_AT_STACK_TOP + 0x40), %esp

  # Save the bootloader's stack pointer
  #
  pushl    %eax

  #
  # Pass entry point of the PEI core
  #
  call     ASM_PFX(GetFspBaseAddress)
  movl     ASM_PFX(FspImageSizeOffset), %edi
  movl     (%eax, %edi), %edi
  addl     %eax, %edi
  subl     $0x20, %edi
  addl     (%edi), %eax
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

FspInitApiexit:
  ret


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
  popl   %eax
  cmpl   $0x087654321, %eax
  jz     NotifyPhaseApiL0
  movl   $0x080000003, %eax
  jmp    NotifyPhaseApiErrExit

NotifyPhaseApiL0:
  #
  # Verify the calling condition
  #
  pusha
  pushl  $0x02
  call   ASM_PFX(FspApiCallingCheck)
  addl   $0x04, %esp
  movl   %eax, 28(%esp)
  popa

  cmpl   $0x00, %eax
  jz     NotifyPhaseApiL1

  #
  # Error return
  #
NotifyPhaseApiErrExit:
  ret

NotifyPhaseApiL1:
  jmp    ASM_PFX(Pei2LoaderSwitchStack)



#END

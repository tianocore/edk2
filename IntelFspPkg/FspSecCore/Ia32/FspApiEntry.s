#------------------------------------------------------------------------------
#
# Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
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


.equ MSR_IA32_PLATFORM_ID,                   0x00000017
.equ MSR_IA32_BIOS_UPDT_TRIG,                0x00000079
.equ MSR_IA32_BIOS_SIGN_ID,                  0x0000008b


MicrocodeHdr:
.equ        MicrocodeHdrVersion,                 0x0000
.equ        MicrocodeHdrRevision,                0x0004
.equ        MicrocodeHdrDate,                    0x0008
.equ        MicrocodeHdrProcessor,               0x000c
.equ        MicrocodeHdrChecksum,                0x0010
.equ        MicrocodeHdrLoader,                  0x0014
.equ        MicrocodeHdrFlags,                   0x0018
.equ        MicrocodeHdrDataSize,                0x001C
.equ        MicrocodeHdrTotalSize,               0x0020
.equ        MicrocodeHdrRsvd,                    0x0024
MicrocodeHdrEnd:
.equ        MicrocodeHdrLength,                  0x0030  # MicrocodeHdrLength = MicrocodeHdrEnd - MicrocodeHdr


ExtSigHdr:
.equ        ExtSigHdrCount,                  0x0000
.equ        ExtSigHdrChecksum,               0x0004
.equ        ExtSigHdrRsvd,                   0x0008
ExtSigHdrEnd:
.equ        ExtSigHdrLength,                 0x0014  #ExtSigHdrLength = ExtSigHdrEnd - ExtSigHdr

ExtSig:
.equ        ExtSigProcessor,                 0x0000
.equ        ExtSigFlags,                     0x0004
.equ        ExtSigChecksum,                  0x0008
ExtSigEnd:
.equ        ExtSigLength,                    0x000C  #ExtSigLength = ExtSigEnd - ExtSig

LoadMicrocodeParams:
.equ        MicrocodeCodeAddr,               0x0000
.equ        MicrocodeCodeSize,               0x0004
LoadMicrocodeParamsEnd:



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
    jmp     NextAddress
.align 4
    #
    # Float control word initial value:
    # all exceptions masked, double-precision, round-to-nearest
    #
ASM_PFX(mFpuControlWord): .word     0x027F
    #
    # Multimedia-extensions control word:
    # all exceptions masked, round-to-nearest, flush to zero for masked underflow
    #
ASM_PFX(mMmxControlWord): .long     0x01F80
SseError:      
    #
    # Processor has to support SSE
    #
    jmp     SseError      
NextAddress:            
    #
    # Initialize floating point units
    #
    finit
    fldcw   ASM_PFX(mFpuControlWord)

    #
    # Use CpuId instructuion (CPUID.01H:EDX.SSE[bit 25] = 1) to test
    # whether the processor supports SSE instruction.
    #
    movl    $1,  %eax
    cpuid
    btl     $25, %edx
    jnc     SseError

    #
    # Set OSFXSR bit (bit #9) & OSXMMEXCPT bit (bit #10)
    #
    movl    %cr4, %eax
    orl     $BIT9, %eax
    movl    %eax, %cr4

    #
    # The processor should support SSE instruction and we can use
    # ldmxcsr instruction
    #
    ldmxcsr ASM_PFX(mMmxControlWord)
.endm

#Save in ECX-SLOT 3 in xmm6.
.macro SAVE_EAX_MICROCODE_RET_STATUS
  pinsrw     $0x6, %eax, %xmm6
  ror        $0x10, %eax
  pinsrw     $0x7, %eax, %xmm6
  rol        $0x10, %eax
.endm

#Restore from ECX-SLOT 3 in xmm6.
.macro LOAD_EAX_MICROCODE_RET_STATUS
  pshufd     $0x93, %xmm6, %xmm6
  movd       %xmm6, %eax
  pshufd     $0x39, %xmm6, %xmm6
.endm



#
# Following are fixed PCDs
#
ASM_GLOBAL    ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamBase)
ASM_GLOBAL    ASM_PFX(_gPcd_FixedAtBuild_PcdTemporaryRamSize)
ASM_GLOBAL    ASM_PFX(_gPcd_FixedAtBuild_PcdFspTemporaryRamSize)

#
# Following functions will be provided in C
#
ASM_GLOBAL    ASM_PFX(SecStartup)
ASM_GLOBAL    ASM_PFX(FspApiCallingCheck)

#
# Following functions will be provided in PlatformSecLib
#
ASM_GLOBAL    ASM_PFX(AsmGetFspBaseAddress)
ASM_GLOBAL    ASM_PFX(AsmGetFspInfoHeader)
ASM_GLOBAL    ASM_PFX(GetBootFirmwareVolumeOffset)
ASM_GLOBAL    ASM_PFX(Loader2PeiSwitchStack)


#
# Define the data length that we saved on the stack top
#
.equ          DATA_LEN_OF_PER0, 0x018
.equ          DATA_LEN_OF_MCUD, 0x018
.equ          DATA_LEN_AT_STACK_TOP, (DATA_LEN_OF_PER0 + DATA_LEN_OF_MCUD + 4)

#------------------------------------------------------------------------------
# SecPlatformInitDefault
# Inputs:
#   mm7 -> Return address
# Outputs:
#   eax -> 0 - Successful, Non-zero - Failed.
# Register Usage:
#   eax is cleared and ebp is used for return address.
#   All others reserved.
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(SecPlatformInitDefault)
ASM_PFX(SecPlatformInitDefault):
   #
   # Save return address to EBP
   #
   movd   %mm7, %ebp
   xorl   %eax, %eax

SecPlatformInitDefaultExit:
   jmp   *%ebp


#------------------------------------------------------------------------------
# LoadMicrocodeDefault
#
# Inputs:
#   esp -> LoadMicrocodeParams pointer
# Register Usage:
#   esp  Preserved
#   All others destroyed
# Assumptions:
#   No memory available, stack is hard-coded and used for return address
#   Executed by SBSP and NBSP
#   Beginning of microcode update region starts on paragraph boundary
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(LoadMicrocodeDefault)
ASM_PFX(LoadMicrocodeDefault):
   #
   # Save return address to EBP
   #
   movd   %mm7, %ebp

   cmpl   $0x00, %esp
   jz     ParamError
   movl   4(%esp), %eax                       #dword ptr []     Parameter pointer
   cmpl   $0x00, %eax
   jz     ParamError
   movl   %eax, %esp
   movl   MicrocodeCodeAddr(%esp), %esi
   cmpl   $0x00, %esi
   jnz    CheckMainHeader

ParamError:
   movl   $0x080000002, %eax
   jmp    LoadMicrocodeExit

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
   shrl   $0x12, %ecx                        # shift (50d-32d=18d=0x12) bits
   andl   $0x07, %ecx                        # platform id at bit[52..50]
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
   cmpl   %eax, MicrocodeHdrVersion(%esi)
   jne    AdvanceFixedSize
   cmpl   %eax, MicrocodeHdrLoader(%esi)
   jne    AdvanceFixedSize

   #
   # Check if signature and plaform ID match
   #
   cmpl   MicrocodeHdrProcessor(%esi), %ebx
   jne    LoadMicrocodeL0
   testl  MicrocodeHdrFlags(%esi), %edx
   jnz    LoadCheck                          #Jif signature and platform ID match

LoadMicrocodeL0:
   #
   # Check if extended header exists
   # First check if MicrocodeHdrTotalSize and MicrocodeHdrDataSize are valid
   #
   xorl   %eax, %eax
   cmpl   %eax, MicrocodeHdrTotalSize(%esi)
   je     NextMicrocode
   cmpl   %eax, MicrocodeHdrDataSize(%esi)
   je     NextMicrocode

   #
   # Then verify total size - sizeof header > data size
   #
   movl   MicrocodeHdrTotalSize(%esi), %ecx
   subl   $MicrocodeHdrLength, %ecx
   cmpl   MicrocodeHdrDataSize(%esi), %ecx
   jle NextMicrocode

   #
   # Set edi -> extended header
   #
   movl   %esi, %edi
   addl   $MicrocodeHdrLength, %edi
   addl   MicrocodeHdrDataSize(%esi), %edi

   #
   # Get count of extended structures
   #
   movl   ExtSigHdrCount(%edi), %ecx

   #
   # Move pointer to first signature structure
   #
   addl   ExtSigHdrLength, %edi

CheckExtSig:
   #
   # Check if extended signature and platform ID match
   #
   cmpl   %ebx, ExtSigProcessor(%edi)
   jne    LoadMicrocodeL1
   test   %edx, ExtSigFlags(%edi)
   jnz    LoadCheck                          # Jif signature and platform ID match
LoadMicrocodeL1:
   #
   # Check if any more extended signatures exist
   #
   addl   $ExtSigLength, %edi
   loop   CheckExtSig

NextMicrocode:
   #
   # Advance just after end of this microcode
   #
   xorl   %eax, %eax
   cmpl   %eax, MicrocodeHdrTotalSize(%esi)
   je     LoadMicrocodeL2
   addl   MicrocodeHdrTotalSize(%esi), %esi
   jmp    CheckAddress
LoadMicrocodeL2:
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
   cmpl   $0x0ffffffff, MicrocodeHdrVersion(%esi)

   #
   # Is automatic size detection ?
   #
   movl   MicrocodeCodeSize(%esp), %eax
   cmpl   $0x0ffffffff, %eax
   jz     LoadMicrocodeL3
   #
   # Address >= microcode region address + microcode region size?
   #
   addl   MicrocodeCodeAddr(%esp), %eax

   cmpl   %eax, %esi
   jae    Done                               #Jif address is outside of microcode region
   jmp    CheckMainHeader

LoadMicrocodeL3:
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
   cmpl   %edx, MicrocodeHdrRevision(%esi)
   je     Continue

LoadMicrocode0:
   #
   # EAX contains the linear address of the start of the Update Data
   # EDX contains zero
   # ECX contains 79h (IA32_BIOS_UPDT_TRIG)
   # Start microcode load with wrmsr
   #
   movl   %esi, %eax
   addl   $MicrocodeHdrLength, %eax
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
   jnz    LoadMicrocodeExit
   movl   $0x08000000E, %eax

LoadMicrocodeExit:
   jmp   *%ebp


#----------------------------------------------------------------------------
# EstablishStackFsp
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(EstablishStackFsp)
ASM_PFX(EstablishStackFsp):
  #
  # Save parameter pointer in edx
  #
  movl    4(%esp), %edx

  #
  # Enable FSP STACK
  #
  movl    PcdGet32(PcdTemporaryRamBase), %esp
  addl    PcdGet32(PcdTemporaryRamSize), %esp

  pushl   $DATA_LEN_OF_MCUD                  # Size of the data region
  pushl   $0x4455434D                        # Signature of the  data region 'MCUD'
  pushl   12(%edx)                           # Code size
  pushl   8(%edx)                            # Code base
  pushl   4(%edx)                            # Microcode size
  pushl   (%edx)                             # Microcode base

  #
  # Save API entry/exit timestamp into stack
  #
  pushl   $DATA_LEN_OF_PER0                  # Size of the data region
  pushl   $0x30524550                        # Signature of the  data region 'PER0'
  LOAD_EDX
  pushl   %edx
  LOAD_EAX
  pushl   %eax
  rdtsc
  pushl   %edx
  pushl   %eax

  #
  # Terminator for the data on stack
  #
  push    $0x00

  #
  # Set ECX/EDX to the BootLoader temporary memory range
  #
  movl       PcdGet32 (PcdTemporaryRamBase), %ecx
  movl       %ecx, %edx
  addl       PcdGet32 (PcdTemporaryRamSize), %edx
  subl       PcdGet32 (PcdFspTemporaryRamSize), %edx

  xorl       %eax, %eax

  movd       %mm7, %esi                      #RET_ESI
  jmp        *%esi

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
  # Save timestamp into XMM6
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
  # Sec Platform Init
  #
  movl    $TempRamInitApiL1, %esi            #CALL_MMX  SecPlatformInit
  movd    %esi, %mm7
  .weak   ASM_PFX(SecPlatformInit)
  .set    ASM_PFX(SecPlatformInit), ASM_PFX(SecPlatformInitDefault)
  jmp     ASM_PFX(SecPlatformInit)
TempRamInitApiL1:
  cmpl    $0x00, %eax
  jnz     NemInitExit

  #
  # Load microcode
  #
  LOAD_ESP
  movl    $TempRamInitApiL2, %esi            #CALL_MMX  LoadMicrocode
  movd    %esi, %mm7
  .weak   ASM_PFX(LoadMicrocode)
  .set    ASM_PFX(LoadMicrocode), ASM_PFX(LoadMicrocodeDefault)
  jmp     ASM_PFX(LoadMicrocode)
TempRamInitApiL2:
  SAVE_EAX_MICROCODE_RET_STATUS              #Save microcode return status in ECX-SLOT 3 in xmm6.
  #@note If return value eax is not 0, microcode did not load, but continue and attempt to boot from ECX-SLOT 3 in xmm6.

  #
  # Call Sec CAR Init
  #
  LOAD_ESP
  movl    $TempRamInitApiL3, %esi            #CALL_MMX  SecCarInit
  movd    %esi, %mm7
  jmp     ASM_PFX(SecCarInit)
TempRamInitApiL3:
  cmpl    $0x00, %eax
  jnz     NemInitExit

  #
  # EstablishStackFsp
  #
  LOAD_ESP
  movl    $TempRamInitApiL4, %esi            #CALL_MMX  EstablishStackFsp
  movd    %esi, %mm7
  jmp     ASM_PFX(EstablishStackFsp)
TempRamInitApiL4:

  LOAD_EAX_MICROCODE_RET_STATUS              #Restore microcode status if no CAR init error.

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
  movl   $0x01, %eax
  jmp    FspApiCommon

#----------------------------------------------------------------------------
# NotifyPhase API
#
# This FSP API will notify the FSP about the different phases in the boot
# process
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(NotifyPhaseApi)
ASM_PFX(NotifyPhaseApi):
  movl   $0x02, %eax
  jmp    FspApiCommon

#----------------------------------------------------------------------------
# FspMemoryInit API
#
# This FSP API is called after TempRamInit and initializes the memory.
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(FspMemoryInitApi)
ASM_PFX(FspMemoryInitApi):
  movl   $0x03, %eax
  jmp    FspApiCommon

#----------------------------------------------------------------------------
# TempRamExitApi API
#
# This API tears down temporary RAM
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(TempRamExitApi)
ASM_PFX(TempRamExitApi):
  movl   $0x04, %eax
  jmp    FspApiCommon

#----------------------------------------------------------------------------
# FspSiliconInit API
#
# This FSP API initializes the CPU and the chipset including the IO
# controllers in the chipset to enable normal operation of these devices.
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(FspSiliconInitApi)
ASM_PFX(FspSiliconInitApi):
  movl   $0x05, %eax
  jmp    FspApiCommon

#----------------------------------------------------------------------------
# FspApiCommon API
#
# This is the FSP API common entry point to resume the FSP execution
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(FspApiCommon)
ASM_PFX(FspApiCommon):
  #
  # EAX holds the API index
  #

  #
  # Stack must be ready
  #  
  pushl   %eax
  addl    $0x04, %esp
  cmpl    -4(%esp), %eax
  jz      FspApiCommonL0
  movl    $0x080000003, %eax
  jmp     FspApiCommonExit

FspApiCommonL0:
  #
  # Verify the calling condition
  #
  pushal
  pushl   36(%esp)  #push ApiParam  [esp + 4 * 8 + 4]
  pushl   %eax      #push ApiIdx
  call    ASM_PFX(FspApiCallingCheck)
  addl    $0x08, %esp
  cmpl    $0x00, %eax
  jz      FspApiCommonL1
  movl    %eax, 0x1C(%esp)                   # mov    dword ptr [esp + 4 * 7], eax
  popal
  ret

FspApiCommonL1:
  popal
  cmpl    $0x01, %eax                        # FspInit API
  jz      FspApiCommonL2
  cmpl    $0x03, %eax                        # FspMemoryInit API
  jz      FspApiCommonL2
  call    ASM_PFX(AsmGetFspInfoHeader)
  jmp     Loader2PeiSwitchStack

FspApiCommonL2:
  #
  # FspInit and FspMemoryInit APIs, setup the initial stack frame
  #  
  
  #
  # Place holder to store the FspInfoHeader pointer
  #
  pushl  %eax

  #
  # Update the FspInfoHeader pointer
  #
  pushl  %eax
  call   ASM_PFX(AsmGetFspInfoHeader)
  movl   %eax, 4(%esp)
  popl   %eax

  #
  # Create a Task Frame in the stack for the Boot Loader
  #
  pushfl                                     # 2 pushf for 4 byte alignment
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
  movl    %esp, %edi
  movl    PcdGet32(PcdTemporaryRamBase), %esp
  addl    PcdGet32(PcdTemporaryRamSize), %esp
  subl    $(DATA_LEN_AT_STACK_TOP + 0x40), %esp

  #
  # Pass the API Idx to SecStartup
  #
  pushl   %eax
  
  #
  # Pass the BootLoader stack to SecStartup
  #
  pushl   %edi

  #
  # Pass entry point of the PEI core
  #
  call    ASM_PFX(AsmGetFspBaseAddress)
  movl    %eax, %edi
  addl    PcdGet32(PcdFspAreaSize), %edi
  subl    $0x20, %edi
  addl    %ds:(%edi), %eax
  pushl   %eax

  #
  # Pass BFV into the PEI Core
  # It uses relative address to calucate the actual boot FV base
  # For FSP implementation with single FV, PcdFspBootFirmwareVolumeBase and
  # PcdFspAreaBaseAddress are the same. For FSP with mulitple FVs,
  # they are different. The code below can handle both cases.
  #
  call    ASM_PFX(AsmGetFspBaseAddress)
  movl    %eax, %edi
  call    ASM_PFX(GetBootFirmwareVolumeOffset)
  addl    %edi, %eax
  pushl   %eax

  #
  # Pass stack base and size into the PEI Core
  #
  movl    PcdGet32(PcdTemporaryRamBase), %eax
  addl    PcdGet32(PcdTemporaryRamSize), %eax
  subl    PcdGet32(PcdFspTemporaryRamSize), %eax
  pushl   %eax
  pushl   PcdGet32(PcdFspTemporaryRamSize)

  #
  # Pass Control into the PEI Core
  #
  call    ASM_PFX(SecStartup)
  addl    $4, %esp
FspApiCommonExit:
  ret


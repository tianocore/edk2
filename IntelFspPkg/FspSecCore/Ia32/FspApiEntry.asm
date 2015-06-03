;; @file
;  Provide FSP API entry points.
;
; Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;;

    .586p
    .model  flat,C
    .code
    .xmm

INCLUDE    SaveRestoreSse.inc
INCLUDE    MicrocodeLoad.inc

;
; Following are fixed PCDs
;
EXTERN   PcdGet32(PcdTemporaryRamBase):DWORD
EXTERN   PcdGet32(PcdTemporaryRamSize):DWORD
EXTERN   PcdGet32(PcdFspTemporaryRamSize):DWORD
EXTERN   PcdGet32(PcdFspAreaSize):DWORD

;
; Following functions will be provided in C
;

EXTERN   SecStartup:PROC
EXTERN   FspApiCallingCheck:PROC

;
; Following functions will be provided in PlatformSecLib
;
EXTERN   AsmGetFspBaseAddress:PROC
EXTERN   AsmGetFspInfoHeader:PROC
EXTERN   GetBootFirmwareVolumeOffset:PROC
EXTERN   Loader2PeiSwitchStack:PROC
EXTERN   LoadMicrocode(LoadMicrocodeDefault):PROC
EXTERN   SecPlatformInit(SecPlatformInitDefault):PROC
EXTERN   SecCarInit:PROC

;
; Define the data length that we saved on the stack top
;
DATA_LEN_OF_PER0         EQU   18h
DATA_LEN_OF_MCUD         EQU   18h
DATA_LEN_AT_STACK_TOP    EQU   (DATA_LEN_OF_PER0 + DATA_LEN_OF_MCUD + 4)

;
; Define SSE macros
;
LOAD_MMX_EXT MACRO   ReturnAddress, MmxRegister
  mov     esi, ReturnAddress
  movd    MmxRegister, esi              ; save ReturnAddress into MMX
ENDM

CALL_MMX_EXT MACRO   RoutineLabel, MmxRegister
  local   ReturnAddress
  mov     esi, offset ReturnAddress
  movd    MmxRegister, esi              ; save ReturnAddress into MMX
  jmp     RoutineLabel
ReturnAddress:
ENDM

RET_ESI_EXT  MACRO   MmxRegister
  movd    esi, MmxRegister              ; move ReturnAddress from MMX to ESI
  jmp     esi
ENDM

CALL_MMX MACRO   RoutineLabel
         CALL_MMX_EXT  RoutineLabel, mm7
ENDM

RET_ESI  MACRO
         RET_ESI_EXT   mm7
ENDM

;------------------------------------------------------------------------------
SecPlatformInitDefault PROC NEAR PUBLIC
   ; Inputs:
   ;   mm7 -> Return address
   ; Outputs:
   ;   eax -> 0 - Successful, Non-zero - Failed.
   ; Register Usage:
   ;   eax is cleared and ebp is used for return address.
   ;   All others reserved.
   
   ; Save return address to EBP
   movd  ebp, mm7

   xor   eax, eax
exit:
   jmp   ebp
SecPlatformInitDefault   ENDP

;------------------------------------------------------------------------------
LoadMicrocodeDefault   PROC  NEAR PUBLIC
   ; Inputs:
   ;   esp -> LoadMicrocodeParams pointer
   ; Register Usage:
   ;   esp  Preserved
   ;   All others destroyed
   ; Assumptions:
   ;   No memory available, stack is hard-coded and used for return address
   ;   Executed by SBSP and NBSP
   ;   Beginning of microcode update region starts on paragraph boundary

   ;
   ;
   ; Save return address to EBP
   movd   ebp, mm7

   cmp    esp, 0
   jz     paramerror
   mov    eax, dword ptr [esp + 4]    ; Parameter pointer
   cmp    eax, 0
   jz     paramerror
   mov    esp, eax
   mov    esi, [esp].LoadMicrocodeParams.MicrocodeCodeAddr
   cmp    esi, 0
   jnz    check_main_header

paramerror:
   mov    eax, 080000002h
   jmp    exit

   mov    esi, [esp].LoadMicrocodeParams.MicrocodeCodeAddr

check_main_header:
   ; Get processor signature and platform ID from the installed processor
   ; and save into registers for later use
   ; ebx = processor signature
   ; edx = platform ID
   mov   eax, 1
   cpuid
   mov   ebx, eax
   mov   ecx, MSR_IA32_PLATFORM_ID
   rdmsr
   mov   ecx, edx
   shr   ecx, 50-32                          ; shift (50d-32d=18d=0x12) bits
   and   ecx, 7h                             ; platform id at bit[52..50]
   mov   edx, 1
   shl   edx, cl

   ; Current register usage
   ; esp -> stack with paramters
   ; esi -> microcode update to check
   ; ebx = processor signature
   ; edx = platform ID

   ; Check for valid microcode header
   ; Minimal test checking for header version and loader version as 1
   mov   eax, dword ptr 1
   cmp   [esi].MicrocodeHdr.MicrocodeHdrVersion, eax
   jne   advance_fixed_size
   cmp   [esi].MicrocodeHdr.MicrocodeHdrLoader, eax
   jne   advance_fixed_size

   ; Check if signature and plaform ID match
   cmp   ebx, [esi].MicrocodeHdr.MicrocodeHdrProcessor
   jne   @f
   test  edx, [esi].MicrocodeHdr.MicrocodeHdrFlags
   jnz   load_check  ; Jif signature and platform ID match

@@:
   ; Check if extended header exists
   ; First check if MicrocodeHdrTotalSize and MicrocodeHdrDataSize are valid
   xor   eax, eax
   cmp   [esi].MicrocodeHdr.MicrocodeHdrTotalSize, eax
   je    next_microcode
   cmp   [esi].MicrocodeHdr.MicrocodeHdrDataSize, eax
   je    next_microcode

   ; Then verify total size - sizeof header > data size
   mov   ecx, [esi].MicrocodeHdr.MicrocodeHdrTotalSize
   sub   ecx, sizeof MicrocodeHdr
   cmp   ecx, [esi].MicrocodeHdr.MicrocodeHdrDataSize
   jng   next_microcode    ; Jif extended header does not exist

   ; Set edi -> extended header
   mov   edi, esi
   add   edi, sizeof MicrocodeHdr
   add   edi, [esi].MicrocodeHdr.MicrocodeHdrDataSize

   ; Get count of extended structures
   mov   ecx, [edi].ExtSigHdr.ExtSigHdrCount

   ; Move pointer to first signature structure
   add   edi, sizeof ExtSigHdr

check_ext_sig:
   ; Check if extended signature and platform ID match
   cmp   [edi].ExtSig.ExtSigProcessor, ebx
   jne   @f
   test  [edi].ExtSig.ExtSigFlags, edx
   jnz   load_check     ; Jif signature and platform ID match
@@:
   ; Check if any more extended signatures exist
   add   edi, sizeof ExtSig
   loop  check_ext_sig

next_microcode:
   ; Advance just after end of this microcode
   xor   eax, eax
   cmp   [esi].MicrocodeHdr.MicrocodeHdrTotalSize, eax
   je    @f
   add   esi, [esi].MicrocodeHdr.MicrocodeHdrTotalSize
   jmp   check_address
@@:
   add   esi, dword ptr 2048
   jmp   check_address

advance_fixed_size:
   ; Advance by 4X dwords
   add   esi, dword ptr 1024

check_address:
   ; Is valid Microcode start point ?
   cmp   dword ptr [esi].MicrocodeHdr.MicrocodeHdrVersion, 0ffffffffh
   jz    done

   ; Is automatic size detection ?
   mov   eax, [esp].LoadMicrocodeParams.MicrocodeCodeSize
   cmp   eax, 0ffffffffh
   jz    @f

   ; Address >= microcode region address + microcode region size?
   add   eax, [esp].LoadMicrocodeParams.MicrocodeCodeAddr
   cmp   esi, eax
   jae   done        ;Jif address is outside of microcode region
   jmp   check_main_header

@@:
load_check:
   ; Get the revision of the current microcode update loaded
   mov   ecx, MSR_IA32_BIOS_SIGN_ID
   xor   eax, eax               ; Clear EAX
   xor   edx, edx               ; Clear EDX
   wrmsr                        ; Load 0 to MSR at 8Bh

   mov   eax, 1
   cpuid
   mov   ecx, MSR_IA32_BIOS_SIGN_ID
   rdmsr                         ; Get current microcode signature

   ; Verify this microcode update is not already loaded
   cmp   [esi].MicrocodeHdr.MicrocodeHdrRevision, edx
   je    continue

load_microcode:
   ; EAX contains the linear address of the start of the Update Data
   ; EDX contains zero
   ; ECX contains 79h (IA32_BIOS_UPDT_TRIG)
   ; Start microcode load with wrmsr
   mov   eax, esi
   add   eax, sizeof MicrocodeHdr
   xor   edx, edx
   mov   ecx, MSR_IA32_BIOS_UPDT_TRIG
   wrmsr
   mov   eax, 1
   cpuid

continue:
   jmp   next_microcode

done:
   mov   eax, 1
   cpuid
   mov   ecx, MSR_IA32_BIOS_SIGN_ID
   rdmsr                         ; Get current microcode signature
   xor   eax, eax
   cmp   edx, 0
   jnz   exit
   mov   eax, 08000000Eh

exit:
   jmp   ebp

LoadMicrocodeDefault   ENDP

EstablishStackFsp    PROC    NEAR    PRIVATE
  ;
  ; Save parameter pointer in edx
  ;
  mov       edx, dword ptr [esp + 4]

  ;
  ; Enable FSP STACK
  ;
  mov       esp, PcdGet32 (PcdTemporaryRamBase)
  add       esp, PcdGet32 (PcdTemporaryRamSize)

  push      DATA_LEN_OF_MCUD     ; Size of the data region
  push      4455434Dh            ; Signature of the  data region 'MCUD'
  push      dword ptr [edx + 12] ; Code size
  push      dword ptr [edx + 8]  ; Code base
  push      dword ptr [edx + 4]  ; Microcode size
  push      dword ptr [edx]      ; Microcode base

  ;
  ; Save API entry/exit timestamp into stack
  ;
  push      DATA_LEN_OF_PER0     ; Size of the data region 
  push      30524550h            ; Signature of the  data region 'PER0'
  LOAD_EDX
  push      edx
  LOAD_EAX
  push      eax
  rdtsc
  push      edx
  push      eax

  ;
  ; Terminator for the data on stack
  ; 
  push      0

  ;
  ; Set ECX/EDX to the BootLoader temporary memory range
  ;
  mov       ecx, PcdGet32 (PcdTemporaryRamBase)
  mov       edx, ecx
  add       edx, PcdGet32 (PcdTemporaryRamSize)
  sub       edx, PcdGet32 (PcdFspTemporaryRamSize)

  xor       eax, eax

  RET_ESI

EstablishStackFsp    ENDP


;----------------------------------------------------------------------------
; TempRamInit API
;
; This FSP API will load the microcode update, enable code caching for the
; region specified by the boot loader and also setup a temporary stack to be
; used till main memory is initialized.
;
;----------------------------------------------------------------------------
TempRamInitApi   PROC    NEAR    PUBLIC
  ;
  ; Ensure SSE is enabled
  ;
  ENABLE_SSE

  ;
  ; Save EBP, EBX, ESI, EDI & ESP in XMM7 & XMM6
  ;
  SAVE_REGS

  ;
  ; Save timestamp into XMM6
  ;
  rdtsc
  SAVE_EAX
  SAVE_EDX

  ;
  ; Check Parameter
  ;
  mov       eax, dword ptr [esp + 4]
  cmp       eax, 0
  mov       eax, 80000002h
  jz        TempRamInitExit

  ;
  ; Sec Platform Init
  ;
  CALL_MMX  SecPlatformInit
  cmp       eax, 0
  jnz       TempRamInitExit

  ; Load microcode
  LOAD_ESP
  CALL_MMX  LoadMicrocode
  SXMMN     xmm6, 3, eax            ;Save microcode return status in ECX-SLOT 3 in xmm6.
  ;@note If return value eax is not 0, microcode did not load, but continue and attempt to boot.

  ; Call Sec CAR Init
  LOAD_ESP
  CALL_MMX  SecCarInit
  cmp       eax, 0
  jnz       TempRamInitExit

  LOAD_ESP
  CALL_MMX  EstablishStackFsp

  LXMMN      xmm6, eax, 3  ;Restore microcode status if no CAR init error from ECX-SLOT 3 in xmm6.

TempRamInitExit:
  ;
  ; Load EBP, EBX, ESI, EDI & ESP from XMM7 & XMM6
  ;
  LOAD_REGS
  ret
TempRamInitApi   ENDP

;----------------------------------------------------------------------------
; FspInit API
;
; This FSP API will perform the processor and chipset initialization.
; This API will not return.  Instead, it transfers the control to the
; ContinuationFunc provided in the parameter.
;
;----------------------------------------------------------------------------
FspInitApi   PROC    NEAR    PUBLIC
  mov    eax,  1
  jmp    FspApiCommon
  FspInitApi   ENDP

;----------------------------------------------------------------------------
; NotifyPhase API
;
; This FSP API will notify the FSP about the different phases in the boot
; process
;
;----------------------------------------------------------------------------
NotifyPhaseApi   PROC C PUBLIC
  mov    eax,  2
  jmp    FspApiCommon
NotifyPhaseApi   ENDP

;----------------------------------------------------------------------------
; FspMemoryInit API
;
; This FSP API is called after TempRamInit and initializes the memory.
;
;----------------------------------------------------------------------------
FspMemoryInitApi   PROC    NEAR    PUBLIC
  mov    eax,  3
  jmp    FspApiCommon
FspMemoryInitApi   ENDP


;----------------------------------------------------------------------------
; TempRamExitApi API
;
; This API tears down temporary RAM
;
;----------------------------------------------------------------------------
TempRamExitApi   PROC C PUBLIC
  mov    eax,  4
  jmp    FspApiCommon
TempRamExitApi   ENDP


;----------------------------------------------------------------------------
; FspSiliconInit API
;
; This FSP API initializes the CPU and the chipset including the IO
; controllers in the chipset to enable normal operation of these devices.
;
;----------------------------------------------------------------------------
FspSiliconInitApi   PROC C PUBLIC
  mov    eax,  5
  jmp    FspApiCommon
FspSiliconInitApi   ENDP

;----------------------------------------------------------------------------
; FspApiCommon API
;
; This is the FSP API common entry point to resume the FSP execution
;
;----------------------------------------------------------------------------
FspApiCommon   PROC C PUBLIC
  ;
  ; EAX holds the API index
  ;

  ;
  ; Stack must be ready
  ;  
  push   eax
  add    esp, 4
  cmp    eax, dword ptr [esp - 4]
  jz     @F
  mov    eax, 080000003h
  jmp    exit

@@:
  ;
  ; Verify the calling condition
  ;
  pushad
  push   [esp + 4 * 8 + 4]  ; push ApiParam
  push   eax                ; push ApiIdx
  call   FspApiCallingCheck
  add    esp, 8
  cmp    eax, 0
  jz     @F
  mov    dword ptr [esp + 4 * 7], eax
  popad
  ret

@@:
  popad
  cmp    eax, 1   ; FspInit API
  jz     @F
  cmp    eax, 3   ; FspMemoryInit API
  jz     @F

  call   AsmGetFspInfoHeader
  jmp    Loader2PeiSwitchStack

@@:
  ;
  ; FspInit and FspMemoryInit APIs, setup the initial stack frame
  ;
  
  ;
  ; Place holder to store the FspInfoHeader pointer
  ;
  push   eax

  ;
  ; Update the FspInfoHeader pointer
  ;
  push   eax
  call   AsmGetFspInfoHeader
  mov    [esp + 4], eax
  pop    eax

  ;
  ; Create a Task Frame in the stack for the Boot Loader
  ;
  pushfd     ; 2 pushf for 4 byte alignment
  cli
  pushad

  ; Reserve 8 bytes for IDT save/restore
  sub     esp, 8
  sidt    fword ptr [esp]

  ;
  ; Setup new FSP stack
  ;
  mov     edi, esp
  mov     esp, PcdGet32(PcdTemporaryRamBase)
  add     esp, PcdGet32(PcdTemporaryRamSize)
  sub     esp, (DATA_LEN_AT_STACK_TOP + 40h)

  ;
  ; Pass the API Idx to SecStartup
  ;
  push    eax
  
  ;
  ; Pass the BootLoader stack to SecStartup
  ;
  push    edi

  ;
  ; Pass entry point of the PEI core
  ;
  call    AsmGetFspBaseAddress
  mov     edi, eax
  add     edi, PcdGet32 (PcdFspAreaSize) 
  sub     edi, 20h
  add     eax, DWORD PTR ds:[edi]
  push    eax

  ;
  ; Pass BFV into the PEI Core
  ; It uses relative address to calucate the actual boot FV base
  ; For FSP implementation with single FV, PcdFspBootFirmwareVolumeBase and
  ; PcdFspAreaBaseAddress are the same. For FSP with mulitple FVs,
  ; they are different. The code below can handle both cases.
  ;
  call    AsmGetFspBaseAddress
  mov     edi, eax
  call    GetBootFirmwareVolumeOffset
  add     eax, edi
  push    eax

  ;
  ; Pass stack base and size into the PEI Core
  ;
  mov     eax,  PcdGet32(PcdTemporaryRamBase)
  add     eax,  PcdGet32(PcdTemporaryRamSize)
  sub     eax,  PcdGet32(PcdFspTemporaryRamSize)
  push    eax
  push    PcdGet32(PcdFspTemporaryRamSize)

  ;
  ; Pass Control into the PEI Core
  ;
  call    SecStartup
  add     esp, 4
exit:
  ret

FspApiCommon   ENDP

END

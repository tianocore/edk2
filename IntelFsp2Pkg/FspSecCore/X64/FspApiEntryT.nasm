;; @file
;  Provide FSP API entry points.
;
; Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;

    SECTION .text

%include    "SaveRestoreSseAvxNasm.inc"
%include    "MicrocodeLoadNasm.inc"

;
; Following are fixed PCDs
;
extern   ASM_PFX(PcdGet32 (PcdTemporaryRamBase))
extern   ASM_PFX(PcdGet32 (PcdTemporaryRamSize))
extern   ASM_PFX(PcdGet32 (PcdFspReservedBufferSize))

;
; Following functions will be provided in PlatformSecLib
;
extern ASM_PFX(AsmGetFspBaseAddress)
extern ASM_PFX(AsmGetFspInfoHeaderNoStack)
;extern ASM_PFX(LoadMicrocode)    ; @todo: needs a weak implementation
extern ASM_PFX(SecPlatformInit)   ; @todo: needs a weak implementation
extern ASM_PFX(SecCarInit)

;
; Define the data length that we saved on the stack top
;
DATA_LEN_OF_PER0         EQU   18h
DATA_LEN_OF_MCUD         EQU   18h
DATA_LEN_AT_STACK_TOP    EQU   (DATA_LEN_OF_PER0 + DATA_LEN_OF_MCUD + 4)

;
; @todo: These structures are moved from MicrocodeLoadNasm.inc to avoid
;        build error. This needs to be fixed later on.
;
struc MicrocodeHdr
    .MicrocodeHdrVersion:      resd    1
    .MicrocodeHdrRevision:     resd    1
    .MicrocodeHdrDate:         resd    1
    .MicrocodeHdrProcessor:    resd    1
    .MicrocodeHdrChecksum:     resd    1
    .MicrocodeHdrLoader:       resd    1
    .MicrocodeHdrFlags:        resd    1
    .MicrocodeHdrDataSize:     resd    1
    .MicrocodeHdrTotalSize:    resd    1
    .MicrocodeHdrRsvd:         resd    3
    .size:
endstruc

struc ExtSigHdr
    .ExtSigHdrCount:          resd    1
    .ExtSigHdrChecksum:       resd    1
    .ExtSigHdrRsvd:           resd    3
    .size:
endstruc

struc ExtSig
    .ExtSigProcessor:         resd    1
    .ExtSigFlags:             resd    1
    .ExtSigChecksum:          resd    1
    .size:
endstruc

struc LoadMicrocodeParamsFsp24
    ; FSP_UPD_HEADER {
    .FspUpdHeaderSignature:   resd    2
    .FspUpdHeaderRevision:    resb    1
    .FspUpdHeaderReserved:    resb   23
    ; }
    ; FSPT_ARCH2_UPD {
    .FsptArchRevision:        resb    1
    .FsptArchReserved:        resb    3
    .FsptArchLength:          resd    1
    .FspDebugHandler          resq    1
    .FsptArchUpd:             resd    4
    ; }
    ; FSPT_CORE_UPD {
    .MicrocodeCodeAddr:       resq    1
    .MicrocodeCodeSize:       resq    1
    .CodeRegionBase:          resq    1
    .CodeRegionSize:          resq    1
    ; }
    .size:
endstruc

%macro CALL_RDI  1

  mov     rdi,  %%ReturnAddress
  jmp     %1
%%ReturnAddress:

%endmacro

;
; @todo: The strong/weak implementation does not work.
;        This needs to be reviewed later.
;
;------------------------------------------------------------------------------
;
;;global ASM_PFX(SecPlatformInitDefault)
;ASM_PFX(SecPlatformInitDefault):
;   ; Inputs:
;   ;   ymm7 -> Return address
;   ; Outputs:
;   ;   rax -> 0 - Successful, Non-zero - Failed.
;   ; Register Usage:
;   ;   rax is cleared and rbp is used for return address.
;   ;   All others reserved.
;
;   ; Save return address to RBP
;   LOAD_RBP
;
;   xor   rax, rax
;Exit1:
;   jmp   rbp

;------------------------------------------------------------------------------
global ASM_PFX(LoadMicrocodeDefault)
ASM_PFX(LoadMicrocodeDefault):
   ; Inputs:
   ;   rcx -> LoadMicrocodeParams pointer
   ; Register Usage:
   ;   All are destroyed
   ; Assumptions:
   ;   No memory available, stack is hard-coded and used for return address
   ;   Executed by SBSP and NBSP
   ;   Beginning of microcode update region starts on paragraph boundary

   ;
   ; Save return address to RBP
   ;
   LOAD_RBP

   cmp    rsp, 0
   jz     ParamError
   cmp    rcx, 0
   jz     ParamError
   mov    rsp, rcx

   ; skip loading Microcode if the MicrocodeCodeSize is zero
   ; and report error if size is less than 2k
   ; first check UPD header revision
   cmp    byte [rsp + LoadMicrocodeParamsFsp24.FspUpdHeaderRevision], 2
   jb     ParamError
   cmp    byte [rsp + LoadMicrocodeParamsFsp24.FsptArchRevision], 2
   jne    ParamError

   ; UPD structure is compliant with FSP spec 2.4
   mov    rax, qword [rsp + LoadMicrocodeParamsFsp24.MicrocodeCodeSize]
   cmp    rax, 0
   jz     Exit2
   cmp    rax, 0800h
   jl     ParamError

   mov    rsi, qword [rsp + LoadMicrocodeParamsFsp24.MicrocodeCodeAddr]
   cmp    rsi, 0
   jnz    CheckMainHeader

ParamError:
   mov    rax, 08000000000000002h
   jmp    Exit2

CheckMainHeader:
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
   ; esp -> stack with parameters
   ; esi -> microcode update to check
   ; ebx = processor signature
   ; edx = platform ID

   ; Check for valid microcode header
   ; Minimal test checking for header version and loader version as 1
   mov   eax, dword 1
   cmp   dword [esi + MicrocodeHdr.MicrocodeHdrVersion], eax
   jne   AdvanceFixedSize
   cmp   dword [esi + MicrocodeHdr.MicrocodeHdrLoader], eax
   jne   AdvanceFixedSize

   ; Check if signature and plaform ID match
   cmp   ebx, dword [esi + MicrocodeHdr.MicrocodeHdrProcessor]
   jne   LoadMicrocodeDefault1
   test  edx, dword [esi + MicrocodeHdr.MicrocodeHdrFlags ]
   jnz   LoadCheck  ; Jif signature and platform ID match

LoadMicrocodeDefault1:
   ; Check if extended header exists
   ; First check if MicrocodeHdrTotalSize and MicrocodeHdrDataSize are valid
   xor   rax, rax
   cmp   dword [esi + MicrocodeHdr.MicrocodeHdrTotalSize], eax
   je    NextMicrocode
   cmp   dword [esi + MicrocodeHdr.MicrocodeHdrDataSize], eax
   je    NextMicrocode

   ; Then verify total size - sizeof header > data size
   mov   ecx, dword [esi + MicrocodeHdr.MicrocodeHdrTotalSize]
   sub   ecx, MicrocodeHdr.size
   cmp   ecx, dword [esi + MicrocodeHdr.MicrocodeHdrDataSize]
   jng   NextMicrocode    ; Jif extended header does not exist

   ; Set edi -> extended header
   mov   edi, esi
   add   edi, MicrocodeHdr.size
   add   edi, dword [esi + MicrocodeHdr.MicrocodeHdrDataSize]

   ; Get count of extended structures
   mov   ecx, dword [edi + ExtSigHdr.ExtSigHdrCount]

   ; Move pointer to first signature structure
   add   edi, ExtSigHdr.size

CheckExtSig:
   ; Check if extended signature and platform ID match
   cmp   dword [edi + ExtSig.ExtSigProcessor], ebx
   jne   LoadMicrocodeDefault2
   test  dword [edi + ExtSig.ExtSigFlags], edx
   jnz   LoadCheck      ; Jif signature and platform ID match
LoadMicrocodeDefault2:
   ; Check if any more extended signatures exist
   add   edi, ExtSig.size
   loop  CheckExtSig

NextMicrocode:
   ; Advance just after end of this microcode
   xor   rax, rax
   cmp   dword [esi + MicrocodeHdr.MicrocodeHdrTotalSize], eax
   je    LoadMicrocodeDefault3
   add   esi, dword [esi + MicrocodeHdr.MicrocodeHdrTotalSize]
   jmp   CheckAddress
LoadMicrocodeDefault3:
   add   esi, dword  2048
   jmp   CheckAddress

AdvanceFixedSize:
   ; Advance by 4X dwords
   add   esi, dword  1024

CheckAddress:
   ; Check UPD header revision
   cmp   byte [rsp + LoadMicrocodeParamsFsp24.FspUpdHeaderRevision], 2
   jb    ParamError
   cmp   byte [rsp + LoadMicrocodeParamsFsp24.FsptArchRevision], 2
   jne   ParamError

   ; UPD structure is compliant with FSP spec 2.4
   ; Is automatic size detection ?
   mov   rax, qword [rsp + LoadMicrocodeParamsFsp24.MicrocodeCodeSize]
   mov   rcx, 0ffffffffffffffffh
   cmp   rax, rcx
   jz    LoadMicrocodeDefault4

   ; Address >= microcode region address + microcode region size?
   add   rax, qword [rsp + LoadMicrocodeParamsFsp24.MicrocodeCodeAddr]
   cmp   rsi, rax
   jae   Done        ;Jif address is outside of microcode region
   jmp   CheckMainHeader

LoadMicrocodeDefault4:
   ; Is valid Microcode start point ?
   cmp   dword [esi + MicrocodeHdr.MicrocodeHdrVersion], 0ffffffffh
   jz    Done

LoadCheck:
   ; Get the revision of the current microcode update loaded
   mov   ecx, MSR_IA32_BIOS_SIGN_ID
   xor   eax, eax               ; Clear EAX
   xor   edx, edx               ; Clear EDX
   wrmsr                        ; Load 0 to MSR at 8Bh

   mov   eax, 1
   cpuid
   mov   ecx, MSR_IA32_BIOS_SIGN_ID
   rdmsr                        ; Get current microcode signature

   ; Verify this microcode update is not already loaded
   cmp   dword [esi + MicrocodeHdr.MicrocodeHdrRevision], edx
   je    Continue

LoadMicrocode:
   ; EAX contains the linear address of the start of the Update Data
   ; EDX contains zero
   ; ECX contains 79h (IA32_BIOS_UPDT_TRIG)
   ; Start microcode load with wrmsr
   mov   eax, esi
   add   eax, MicrocodeHdr.size
   xor   edx, edx
   mov   ecx, MSR_IA32_BIOS_UPDT_TRIG
   wrmsr
   mov   eax, 1
   cpuid

Continue:
   jmp   NextMicrocode

Done:
   mov   eax, 1
   cpuid
   mov   ecx, MSR_IA32_BIOS_SIGN_ID
   rdmsr                         ; Get current microcode signature
   xor   eax, eax
   cmp   edx, 0
   jnz   Exit2
   mov   eax, 0800000000000000Eh

Exit2:
   jmp   rbp


global ASM_PFX(EstablishStackFsp)
ASM_PFX(EstablishStackFsp):
  ;
  ; Save parameter pointer in rdx
  ;
  mov       rdx, rcx
  ;
  ; Enable FSP STACK
  ;
  mov       rax, ASM_PFX(PcdGet32 (PcdTemporaryRamBase))
  mov       esp, DWORD[rax]
  mov       rax, ASM_PFX(PcdGet32 (PcdTemporaryRamSize))
  add       esp, DWORD[rax]

  sub       esp, 4
  mov       dword[esp], DATA_LEN_OF_MCUD ; Size of the data region
  sub       esp, 4
  mov       dword[esp], 4455434Dh        ; Signature of the  data region 'MCUD'

  ; check UPD structure revision (rdx + 8)
  cmp       byte [rdx + LoadMicrocodeParamsFsp24.FspUpdHeaderRevision], 2
  jb        ParamError1
  cmp       byte [rdx + LoadMicrocodeParamsFsp24.FsptArchRevision], 2
  je        Fsp24UpdHeader

ParamError1:
  mov       rax, 08000000000000002h
  jmp       EstablishStackFspExit

Fsp24UpdHeader:
  ; UPD structure is compliant with FSP spec 2.4
  xor       rax, rax
  mov       rax, qword [rdx + LoadMicrocodeParamsFsp24.CodeRegionSize]     ; Code size       sizeof(FSPT_UPD_COMMON) + 18h
  sub       rsp, 8
  mov       qword[rsp], rax
  mov       rax, qword [rdx + LoadMicrocodeParamsFsp24.CodeRegionBase]     ; Code base       sizeof(FSPT_UPD_COMMON) + 10h
  sub       rsp, 8
  mov       qword[rsp], rax
  mov       rax, qword [rdx + LoadMicrocodeParamsFsp24.MicrocodeCodeSize]  ; Microcode size  sizeof(FSPT_UPD_COMMON) + 8h
  sub       rsp, 8
  mov       qword[rsp], rax
  mov       rax, qword [rdx + LoadMicrocodeParamsFsp24.MicrocodeCodeAddr]  ; Microcode base  sizeof(FSPT_UPD_COMMON) + 0h
  sub       rsp, 8
  mov       qword[rsp], rax

ContinueAfterUpdPush:
  ;
  ; Save API entry/exit timestamp into stack
  ;
  sub       esp, 4
  mov       dword[esp], DATA_LEN_OF_PER0 ; Size of the data region
  sub       esp, 4
  mov       dword[esp], 30524550h        ; Signature of the  data region 'PER0'
  rdtsc
  sub       esp, 4
  mov       dword[esp], edx
  sub       esp, 4
  mov       dword[esp], eax
  LOAD_TS   rax
  push      rax

  ;
  ; Terminator for the data on stack
  ;
  push      0

  ;
  ; Set ECX/EDX to the BootLoader temporary memory range
  ;
  mov       rcx, ASM_PFX(PcdGet32 (PcdTemporaryRamBase))
  mov       edx, [ecx]
  mov       rcx, ASM_PFX(PcdGet32 (PcdTemporaryRamSize))
  add       edx, [ecx]
  mov       rcx, ASM_PFX(PcdGet32 (PcdFspReservedBufferSize))
  sub       edx, [ecx]
  mov       rcx, ASM_PFX(PcdGet32 (PcdTemporaryRamBase))
  mov       ecx, [ecx]

  cmp       ecx, edx                 ; If PcdFspReservedBufferSize >= PcdTemporaryRamSize, then error.
  jb        EstablishStackFspSuccess
  mov       rax, 08000000000000003h  ; EFI_UNSUPPORTED
  jmp       EstablishStackFspExit
EstablishStackFspSuccess:
  xor       rax, rax

EstablishStackFspExit:
  RET_YMM

;----------------------------------------------------------------------------
; TempRamInit API
;
; This FSP API will load the microcode update, enable code caching for the
; region specified by the boot loader and also setup a temporary stack to be
; used till main memory is initialized.
;
;----------------------------------------------------------------------------
global ASM_PFX(TempRamInitApi)
ASM_PFX(TempRamInitApi):
  ;
  ; Ensure both SSE and AVX are enabled
  ;
  ENABLE_SSE
  ENABLE_AVX
  ;
  ; Save RBP, RBX, RSI, RDI and RSP in YMM7, YMM8 and YMM6
  ;
  SAVE_REGS

  ;
  ; Save BFV address in YMM9
  ;
  SAVE_BFV  rbp

  ;
  ; Save Input Parameter in YMM10
  ;
  cmp       rcx, 0
  jnz       ParamValid

  ;
  ; Fall back to default UPD
  ;
  CALL_RDI  ASM_PFX(AsmGetFspInfoHeaderNoStack)
  xor       rcx, rcx
  mov       ecx,  DWORD [rax + 01Ch]      ; Read FsptImageBaseAddress
  add       ecx,  DWORD [rax + 024h]      ; Get Cfg Region base address = FsptImageBaseAddress + CfgRegionOffset
ParamValid:
  SAVE_RCX

  ;
  ; Save timestamp into YMM6
  ;
  rdtsc
  shl       rdx, 32
  or        rax, rdx
  SAVE_TS   rax

  ;
  ; Sec Platform Init
  ;
  CALL_YMM  ASM_PFX(SecPlatformInit)
  cmp       eax, 0
  jnz       TempRamInitExit

  ; Load microcode
  LOAD_RCX
  CALL_YMM  ASM_PFX(LoadMicrocodeDefault)
  SAVE_UCODE_STATUS rax             ; Save microcode return status in SLOT 0 in YMM9 (upper 128bits).
  ; @note If return value rax is not 0, microcode did not load, but continue and attempt to boot.

  ; Call Sec CAR Init
  LOAD_RCX
  CALL_YMM  ASM_PFX(SecCarInit)
  cmp       rax, 0
  jnz       TempRamInitExit

  LOAD_RCX
  CALL_YMM  ASM_PFX(EstablishStackFsp)
  cmp       rax, 0
  jnz       TempRamInitExit

  LOAD_UCODE_STATUS rax             ; Restore microcode status if no CAR init error from SLOT 0 in YMM9 (upper 128bits).

TempRamInitExit:
  mov       bl, al                  ; save al data in bl
  mov       al, 07Fh                ; API exit postcode 7f
  out       080h, al
  mov       al, bl                  ; restore al data from bl

  ;
  ; Load RBP, RBX, RSI, RDI and RSP from YMM7, YMM8 and YMM6
  ;
  LOAD_REGS
  LOAD_BFV  rbp
  ret

;----------------------------------------------------------------------------
; Module Entrypoint API
;----------------------------------------------------------------------------
global ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):
  jmp $


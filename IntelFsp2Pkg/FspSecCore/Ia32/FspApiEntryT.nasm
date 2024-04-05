;; @file
;  Provide FSP API entry points.
;
; Copyright (c) 2016 - 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;

    SECTION .text

%include    "SaveRestoreSseNasm.inc"
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

struc LoadMicrocodeParams
    ; FSP_UPD_HEADER {
    .FspUpdHeader:            resd    8
    ; }
    ; FSPT_CORE_UPD {
    .MicrocodeCodeAddr:       resd    1
    .MicrocodeCodeSize:       resd    1
    .CodeRegionBase:          resd    1
    .CodeRegionSize:          resd    1
    ; }
    .size:
endstruc

struc LoadMicrocodeParamsFsp22
    ; FSP_UPD_HEADER {
    .FspUpdHeaderSignature:   resd    2
    .FspUpdHeaderRevision:    resb    1
    .FspUpdHeaderReserved:    resb   23
    ; }
    ; FSPT_ARCH_UPD {
    .FsptArchRevision:        resb    1
    .FsptArchReserved:        resb    3
    .FsptArchUpd:             resd    7
    ; }
    ; FSPT_CORE_UPD {
    .MicrocodeCodeAddr:       resd    1
    .MicrocodeCodeSize:       resd    1
    .CodeRegionBase:          resd    1
    .CodeRegionSize:          resd    1
    ; }
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
    .FspTemporaryRamSize:     resd    1  ; Supported only if ArchRevison is >= 3
    .FsptArchUpd:             resd    3
    ; }
    ; FSPT_CORE_UPD {
    .MicrocodeCodeAddr:       resq    1
    .MicrocodeCodeSize:       resq    1
    .CodeRegionBase:          resq    1
    .CodeRegionSize:          resq    1
    ; }
    .size:
endstruc

;
; Define SSE macros
;
;
;args 1: ReturnAddress  2:MmxRegister
;
%macro LOAD_MMX_EXT 2
  mov     esi, %1
  movd    %2, esi              ; save ReturnAddress into MMX
%endmacro

;
;args 1: RoutineLabel  2:MmxRegister
;
%macro CALL_MMX_EXT  2
  mov     esi, %%ReturnAddress
  movd    %2, esi              ; save ReturnAddress into MMX
  jmp     %1
%%ReturnAddress:
%endmacro

;
;arg 1:MmxRegister
;
%macro RET_ESI_EXT   1
  movd    esi, %1              ; move ReturnAddress from MMX to ESI
  jmp     esi
%endmacro

;
;arg 1:RoutineLabel
;
%macro CALL_MMX   1
         CALL_MMX_EXT  %1, mm7
%endmacro

%macro RET_ESI 0
         RET_ESI_EXT   mm7
%endmacro

%macro CALL_EDI  1

  mov     edi,  %%ReturnAddress
  jmp     %1
%%ReturnAddress:

%endmacro

%macro CALL_EBP 1
  mov     ebp, %%ReturnAddress
  jmp     %1
%%ReturnAddress:
%endmacro

%macro RET_EBP 0
  jmp     ebp                           ; restore EIP from EBP
%endmacro

;
; Load UPD region pointer in ECX
;
global ASM_PFX(LoadUpdPointerToECX)
ASM_PFX(LoadUpdPointerToECX):
  ;
  ; esp + 4 is input UPD parameter
  ; If esp + 4 is NULL the default UPD should be used
  ; ecx will be the UPD region that should be used
  ;
  mov       ecx, dword [esp + 4]
  cmp       ecx, 0
  jnz       ParamValid

  ;
  ; Fall back to default UPD region
  ;
  CALL_EDI  ASM_PFX(AsmGetFspInfoHeaderNoStack)
  mov       ecx, DWORD [eax + 01Ch]      ; Read FsptImageBaseAddress
  add       ecx, DWORD [eax + 024h]      ; Get Cfg Region base address = FsptImageBaseAddress + CfgRegionOffset
ParamValid:
  RET_EBP

;
; @todo: The strong/weak implementation does not work.
;        This needs to be reviewed later.
;
;------------------------------------------------------------------------------
;
;;global ASM_PFX(SecPlatformInitDefault)
;ASM_PFX(SecPlatformInitDefault):
;   ; Inputs:
;   ;   mm7 -> Return address
;   ; Outputs:
;   ;   eax -> 0 - Successful, Non-zero - Failed.
;   ; Register Usage:
;   ;   eax is cleared and ebp is used for return address.
;   ;   All others reserved.
;
;   ; Save return address to EBP
;   movd  ebp, mm7
;
;   xor   eax, eax
;Exit1:
;   jmp   ebp

;------------------------------------------------------------------------------
global ASM_PFX(LoadMicrocodeDefault)
ASM_PFX(LoadMicrocodeDefault):
   ; Inputs:
   ;   ecx -> UPD region contains LoadMicrocodeParams pointer
   ; Register Usage:
   ;   All are destroyed
   ; Assumptions:
   ;   No memory available, stack is hard-coded and used for return address
   ;   Executed by SBSP and NBSP
   ;   Beginning of microcode update region starts on paragraph boundary

   ;
   ; Save return address to EBP
   ;
   movd   ebp, mm7

   mov    esp, ecx  ; ECX has been assigned to UPD region
   cmp    esp, 0
   jz     ParamError

   ;
   ; If microcode already loaded before this function, exit this function with SUCCESS.
   ;
   mov   ecx, MSR_IA32_BIOS_SIGN_ID
   xor   eax, eax               ; Clear EAX
   xor   edx, edx               ; Clear EDX
   wrmsr                        ; Load 0 to MSR at 8Bh

   mov   eax, 1
   cpuid
   mov   ecx, MSR_IA32_BIOS_SIGN_ID
   rdmsr                         ; Get current microcode signature
   xor   eax, eax
   test  edx, edx
   jnz   Exit2

   ; skip loading Microcode if the MicrocodeCodeSize is zero
   ; and report error if size is less than 2k
   ; first check UPD header revision
   cmp    byte [esp + LoadMicrocodeParamsFsp22.FspUpdHeaderRevision], 2
   jb     Fsp20UpdHeader
   cmp    byte [esp + LoadMicrocodeParamsFsp22.FsptArchRevision], 2
   jae    Fsp24UpdHeader
   jmp    Fsp22UpdHeader

Fsp20UpdHeader:
   ; UPD structure is compliant with FSP spec 2.0/2.1
   mov    eax, dword [esp + LoadMicrocodeParams.MicrocodeCodeSize]
   cmp    eax, 0
   jz     Exit2
   cmp    eax, 0800h
   jl     ParamError

   mov    esi, dword [esp + LoadMicrocodeParams.MicrocodeCodeAddr]
   cmp    esi, 0
   jnz    CheckMainHeader
   jmp    ParamError

Fsp22UpdHeader:
   ; UPD structure is compliant with FSP spec 2.2
   mov    eax, dword [esp + LoadMicrocodeParamsFsp22.MicrocodeCodeSize]
   cmp    eax, 0
   jz     Exit2
   cmp    eax, 0800h
   jl     ParamError

   mov    esi, dword [esp + LoadMicrocodeParamsFsp22.MicrocodeCodeAddr]
   cmp    esi, 0
   jnz    CheckMainHeader
   jmp    ParamError

Fsp24UpdHeader:
   ; UPD structure is compliant with FSP spec 2.4
   mov    eax, dword [esp + LoadMicrocodeParamsFsp24.MicrocodeCodeSize]
   cmp    eax, 0
   jz     Exit2
   cmp    eax, 0800h
   jl     ParamError

   mov    esi, dword [esp + LoadMicrocodeParamsFsp24.MicrocodeCodeAddr]
   cmp    esi, 0
   jnz    CheckMainHeader

ParamError:
   mov    eax, 080000002h
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
   jnz   LoadMicrocode  ; Jif signature and platform ID match

LoadMicrocodeDefault1:
   ; Check if extended header exists
   ; First check if MicrocodeHdrTotalSize and MicrocodeHdrDataSize are valid
   xor   eax, eax
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
   jnz   LoadMicrocode      ; Jif signature and platform ID match
LoadMicrocodeDefault2:
   ; Check if any more extended signatures exist
   add   edi, ExtSig.size
   loop  CheckExtSig

NextMicrocode:
   ; Advance just after end of this microcode
   xor   eax, eax
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
   cmp   byte [esp + LoadMicrocodeParamsFsp22.FspUpdHeaderRevision], 2
   jb     Fsp20UpdHeader1
   cmp    byte [esp + LoadMicrocodeParamsFsp22.FsptArchRevision], 2
   jae    Fsp24UpdHeader1;
   jmp    Fsp22UpdHeader1

Fsp20UpdHeader1:
   ; UPD structure is compliant with FSP spec 2.0/2.1
   ; Is automatic size detection ?
   mov   eax, dword [esp + LoadMicrocodeParams.MicrocodeCodeSize]
   cmp   eax, 0ffffffffh
   jz    LoadMicrocodeDefault4

   ; Address >= microcode region address + microcode region size?
   add   eax, dword [esp + LoadMicrocodeParams.MicrocodeCodeAddr]
   cmp   esi, eax
   jae   Done        ;Jif address is outside of microcode region
   jmp   CheckMainHeader

Fsp22UpdHeader1:
   ; UPD structure is compliant with FSP spec 2.2
   ; Is automatic size detection ?
   mov   eax, dword [esp + LoadMicrocodeParamsFsp22.MicrocodeCodeSize]
   cmp   eax, 0ffffffffh
   jz    LoadMicrocodeDefault4

   ; Address >= microcode region address + microcode region size?
   add   eax, dword [esp + LoadMicrocodeParamsFsp22.MicrocodeCodeAddr]
   cmp   esi, eax
   jae   Done        ;Jif address is outside of microcode region
   jmp   CheckMainHeader

Fsp24UpdHeader1:
   ; UPD structure is compliant with FSP spec 2.4
   ; Is automatic size detection ?
   mov   eax, dword [esp + LoadMicrocodeParamsFsp24.MicrocodeCodeSize]
   cmp   eax, 0ffffffffh
   jz    LoadMicrocodeDefault4

   ; Address >= microcode region address + microcode region size?
   add   eax, dword [esp + LoadMicrocodeParamsFsp24.MicrocodeCodeAddr]
   cmp   esi, eax
   jae   Done        ;Jif address is outside of microcode region
   jmp   CheckMainHeader

LoadMicrocodeDefault4:
   ; Is valid Microcode start point ?
   cmp   dword [esi + MicrocodeHdr.MicrocodeHdrVersion], 0ffffffffh
   jz    Done
   jmp   CheckMainHeader
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

Done:
   mov   ecx, MSR_IA32_BIOS_SIGN_ID
   xor   eax, eax               ; Clear EAX
   xor   edx, edx               ; Clear EDX
   wrmsr                        ; Load 0 to MSR at 8Bh

   mov   eax, 1
   cpuid
   mov   ecx, MSR_IA32_BIOS_SIGN_ID
   rdmsr                         ; Get current microcode signature
   xor   eax, eax
   cmp   edx, 0
   jnz   Exit2
   mov   eax, 08000000Eh

Exit2:
   jmp   ebp

;
; EstablishStackFsp: EDI should be preserved cross this function
;
global ASM_PFX(EstablishStackFsp)
ASM_PFX(EstablishStackFsp):
  ;
  ; Save parameter pointer in edx
  ;
  mov       edx, ecx   ; ECX has been assigned to UPD region

  ;
  ; Enable FSP STACK
  ;
  mov       esp, DWORD [ASM_PFX(PcdGet32 (PcdTemporaryRamBase))]
  LOAD_TEMPORARY_RAM_SIZE ecx
  add       esp, ecx

  push      DATA_LEN_OF_MCUD     ; Size of the data region
  push      4455434Dh            ; Signature of the  data region 'MCUD'

  ; check UPD structure revision (edx + 8)
  cmp       byte [edx + LoadMicrocodeParamsFsp22.FspUpdHeaderRevision], 2
  jb        Fsp20UpdHeader2
  cmp       byte [esp + LoadMicrocodeParamsFsp22.FsptArchRevision], 2
  jae       Fsp24UpdHeader2
  jmp       Fsp22UpdHeader2

Fsp20UpdHeader2:
  ; UPD structure is compliant with FSP spec 2.0/2.1
  push      dword [edx + LoadMicrocodeParams.CodeRegionSize]     ; Code size       sizeof(FSPT_UPD_COMMON) + 12
  push      dword [edx + LoadMicrocodeParams.CodeRegionBase]     ; Code base       sizeof(FSPT_UPD_COMMON) + 8
  push      dword [edx + LoadMicrocodeParams.MicrocodeCodeSize]  ; Microcode size  sizeof(FSPT_UPD_COMMON) + 4
  push      dword [edx + LoadMicrocodeParams.MicrocodeCodeAddr]  ; Microcode base  sizeof(FSPT_UPD_COMMON) + 0
  jmp       ContinueAfterUpdPush

Fsp22UpdHeader2:
  ; UPD structure is compliant with FSP spec 2.2
  push      dword [edx + LoadMicrocodeParamsFsp22.CodeRegionSize]     ; Code size       sizeof(FSPT_UPD_COMMON) + 12
  push      dword [edx + LoadMicrocodeParamsFsp22.CodeRegionBase]     ; Code base       sizeof(FSPT_UPD_COMMON) + 8
  push      dword [edx + LoadMicrocodeParamsFsp22.MicrocodeCodeSize]  ; Microcode size  sizeof(FSPT_UPD_COMMON) + 4
  push      dword [edx + LoadMicrocodeParamsFsp22.MicrocodeCodeAddr]  ; Microcode base  sizeof(FSPT_UPD_COMMON) + 0
  jmp       ContinueAfterUpdPush

Fsp24UpdHeader2:
  ; UPD structure is compliant with FSP spec 2.4
  push      dword [edx + LoadMicrocodeParamsFsp24.CodeRegionSize]     ; Code size       sizeof(FSPT_UPD_COMMON) + 24
  push      dword [edx + LoadMicrocodeParamsFsp24.CodeRegionBase]     ; Code base       sizeof(FSPT_UPD_COMMON) + 16
  push      dword [edx + LoadMicrocodeParamsFsp24.MicrocodeCodeSize]  ; Microcode size  sizeof(FSPT_UPD_COMMON) + 8
  push      dword [edx + LoadMicrocodeParamsFsp24.MicrocodeCodeAddr]  ; Microcode base  sizeof(FSPT_UPD_COMMON) + 0

ContinueAfterUpdPush:
  ;
  ; Save API entry/exit timestamp into stack
  ;
  push      DATA_LEN_OF_PER0     ; Size of the data region
  push      30524550h            ; Signature of the  data region 'PER0'
  rdtsc
  push      edx
  push      eax
  LOAD_EDX
  push      edx
  LOAD_EAX
  push      eax

  ;
  ; Terminator for the data on stack
  ;
  push      0

  ;
  ; Set ECX/EDX to the BootLoader temporary memory range
  ;
  mov       edx,  [ASM_PFX(PcdGet32 (PcdTemporaryRamBase))]
  LOAD_TEMPORARY_RAM_SIZE ecx
  add       edx, ecx
  sub       edx,  [ASM_PFX(PcdGet32 (PcdFspReservedBufferSize))]
  mov       ecx,  [ASM_PFX(PcdGet32 (PcdTemporaryRamBase))]

  cmp       ecx, edx        ;If PcdFspReservedBufferSize >= TemporaryRamSize, then error.
  jb        EstablishStackFspSuccess
  mov       eax, 80000003h  ;EFI_UNSUPPORTED
  jmp       EstablishStackFspExit
EstablishStackFspSuccess:
  xor       eax, eax

EstablishStackFspExit:
  RET_ESI

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

  CALL_EBP  ASM_PFX(LoadUpdPointerToECX) ; ECX for UPD param
  SAVE_ECX                               ; save UPD param to slot 3 in xmm6

  mov       edx, ASM_PFX(PcdGet32 (PcdTemporaryRamSize))
  mov       edx, DWORD [edx]
  ;
  ; Read Fsp Arch2 revision
  ;
  cmp       byte [ecx + LoadMicrocodeParamsFsp24.FsptArchRevision], 3
  jb        UseTemporaryRamSizePcd
  ;
  ; Read ARCH2 UPD input value.
  ;
  mov       ebx, DWORD [ecx + LoadMicrocodeParamsFsp24.FspTemporaryRamSize]
  ;
  ; As per spec, if Bootloader pass zero, use Fsp defined Size
  ;
  cmp       ebx, 0
  jz        UseTemporaryRamSizePcd

  xor       eax, eax
  mov       ax,  WORD [esi + 020h]      ; Read ImageAttribute
  test      ax,  16                     ; check if Bit4 is set
  jnz       ConsumeInputConfiguration
  ;
  ; Sometimes user may change input value even if it is not supported
  ; return error if input is Non-Zero and not same as PcdTemporaryRamSize.
  ;
  cmp       ebx, edx
  je        UseTemporaryRamSizePcd
  mov       eax, 080000002h      ; RETURN_INVALID_PARAMETER
  jmp       TempRamInitExit
ConsumeInputConfiguration:
  ;
  ; Read ARCH2 UPD value and Save.
  ;
  SAVE_TEMPORARY_RAM_SIZE ebx
  jmp       GotTemporaryRamSize
UseTemporaryRamSizePcd:
  SAVE_TEMPORARY_RAM_SIZE edx
GotTemporaryRamSize:
  LOAD_ECX
  ;
  ; Sec Platform Init
  ;
  CALL_MMX  ASM_PFX(SecPlatformInit)
  cmp       eax, 0
  jnz       TempRamInitExit

  ; Load microcode
  LOAD_ESP
  LOAD_ECX
  CALL_MMX  ASM_PFX(LoadMicrocodeDefault)
  SAVE_UCODE_STATUS                 ; Save microcode return status in slot 1 in xmm5.
  ;@note If return value eax is not 0, microcode did not load, but continue and attempt to boot.

  ; Call Sec CAR Init
  LOAD_ESP
  LOAD_ECX
  CALL_MMX  ASM_PFX(SecCarInit)
  cmp       eax, 0
  jnz       TempRamInitExit

  LOAD_ESP
  LOAD_ECX
  mov       edi, ecx                ; Save UPD param to EDI for later code use
  CALL_MMX  ASM_PFX(EstablishStackFsp)
  cmp       eax, 0
  jnz       TempRamInitExit

  LOAD_UCODE_STATUS                 ; Restore microcode status if no CAR init error from slot 1 in xmm5.

TempRamInitExit:
  mov       bl, al                  ; save al data in bl
  mov       al, 07Fh                ; API exit postcode 7f
  out       080h, al
  mov       al, bl                  ; restore al data from bl

  ;
  ; Load EBP, EBX, ESI, EDI & ESP from XMM7 & XMM6
  ;
  LOAD_REGS
  ret

;----------------------------------------------------------------------------
; Module Entrypoint API
;----------------------------------------------------------------------------
global ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):
  jmp $

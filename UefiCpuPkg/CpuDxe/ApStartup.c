/** @file
  CPU DXE AP Startup

  Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuDxe.h"
#include "CpuGdt.h"
#include "CpuMp.h"

#pragma pack(1)

typedef struct {
  UINT8  MoveIa32EferMsrToEcx[5];
  UINT8  ReadIa32EferMsr[2];
  UINT8  SetExecuteDisableBitEnableBit[4];
  UINT8  WriteIa32EferMsr[2];

#if defined (MDE_CPU_IA32)
  UINT8  MovEaxCr3;
  UINT32 Cr3Value;
  UINT8  MovCr3Eax[3];

  UINT8  MoveCr4ToEax[3];
  UINT8  SetCr4Bit5[4];
  UINT8  MoveEaxToCr4[3];

  UINT8  MoveCr0ToEax[3];
  UINT8  SetCr0PagingBit[4];
  UINT8  MoveEaxToCr0[3];
#endif
} ENABLE_EXECUTE_DISABLE_CODE;

ENABLE_EXECUTE_DISABLE_CODE mEnableExecuteDisableCodeTemplate = {
  { 0xB9, 0x80, 0x00, 0x00, 0xC0 },   // mov ecx, 0xc0000080
  { 0x0F, 0x32 },                     // rdmsr
  { 0x0F, 0xBA, 0xE8, 0x0B },         // bts eax, 11
  { 0x0F, 0x30 },                     // wrmsr

#if defined (MDE_CPU_IA32)
  0xB8, 0x00000000,                   // mov eax, cr3 value
  { 0x0F, 0x22, 0xd8 },               // mov cr3, eax

  { 0x0F, 0x20, 0xE0 },               // mov eax, cr4
  { 0x0F, 0xBA, 0xE8, 0x05 },         // bts eax, 5
  { 0x0F, 0x22, 0xE0 },               // mov cr4, eax

  { 0x0F, 0x20, 0xC0 },               // mov eax, cr0
  { 0x0F, 0xBA, 0xE8, 0x1F },         // bts eax, 31
  { 0x0F, 0x22, 0xC0 },               // mov cr0, eax
#endif
};

typedef struct {
  UINT8  JmpToCli[2];

  UINT16 GdtLimit;
  UINT32 GdtBase;

  UINT8  Cli;

  UINT8  MovAxRealSegment; UINT16 RealSegment;
  UINT8  MovDsAx[2];

  UINT8  MovBxGdtr[3];
  UINT8  LoadGdt[5];

  UINT8  MovEaxCr0[2];
  UINT32 MovEaxCr0Value;
  UINT8  MovCr0Eax[3];

  UINT8  FarJmp32Flat[2]; UINT32 FlatJmpOffset; UINT16 FlatJmpSelector;

  //
  // Now in IA32
  //
  UINT8  MovEaxCr4;
  UINT32 MovEaxCr4Value;
  UINT8  MovCr4Eax[3];

  UINT8  MoveDataSelectorIntoAx[2]; UINT16 FlatDataSelector;
  UINT8  MoveFlatDataSelectorFromAxToDs[2];
  UINT8  MoveFlatDataSelectorFromAxToEs[2];
  UINT8  MoveFlatDataSelectorFromAxToFs[2];
  UINT8  MoveFlatDataSelectorFromAxToGs[2];
  UINT8  MoveFlatDataSelectorFromAxToSs[2];

  //
  // Code placeholder to enable PAE Execute Disable for IA32
  // and enable Execute Disable Bit for X64
  //
  ENABLE_EXECUTE_DISABLE_CODE EnableExecuteDisable;

#if defined (MDE_CPU_X64)
  //
  // Transition to X64
  //
  UINT8  MovEaxCr3;
  UINT32 Cr3Value;
  UINT8  MovCr3Eax[3];

  UINT8  MoveCr4ToEax[3];
  UINT8  SetCr4Bit5[4];
  UINT8  MoveEaxToCr4[3];

  UINT8  MoveLongModeEnableMsrToEcx[5];
  UINT8  ReadLmeMsr[2];
  UINT8  SetLongModeEnableBit[4];
  UINT8  WriteLmeMsr[2];

  UINT8  MoveCr0ToEax[3];
  UINT8  SetCr0PagingBit[4];
  UINT8  MoveEaxToCr0[3];
  //UINT8  DeadLoop[2];

  UINT8  FarJmp32LongMode; UINT32 LongJmpOffset; UINT16 LongJmpSelector;
#endif // defined (MDE_CPU_X64)

#if defined (MDE_CPU_X64)
  UINT8  MovEaxOrRaxCpuDxeEntry[2]; UINTN CpuDxeEntryValue;
#else
  UINT8  MovEaxOrRaxCpuDxeEntry; UINTN CpuDxeEntryValue;
#endif
  UINT8  JmpToCpuDxeEntry[2];

} STARTUP_CODE;

#pragma pack()

/**
  This .asm code used for translating processor from 16 bit real mode into
  64 bit long mode. which help to create the mStartupCodeTemplate value.

  To assemble:
    * nasm -o ApStartup ApStartup.asm
    Then disassemble:
    * ndisasm -b 16 ApStartup
    * ndisasm -b 16 -e 6 ApStartup
    * ndisasm -b 32 -e 32 ApStartup (This -e offset may need adjustment)
    * ndisasm -b 64 -e 0x83 ApStartup (This -e offset may need adjustment)

  %define DEFAULT_CR0  0x00000023
  %define DEFAULT_CR4  0x640

  BITS    16

      jmp     short TransitionFromReal16To32BitFlat

  ALIGN   2

  Gdtr:
      dw      0x5a5a
      dd      0x5a5a5a5a

  ;
  ; Modified:  EAX, EBX
  ;
  TransitionFromReal16To32BitFlat:

      cli
      mov     ax, 0x5a5a
      mov     ds, ax

      mov     bx, Gdtr
  o32 lgdt    [ds:bx]

      mov     eax, cr4
      btc     eax, 5
      mov     cr4, eax

      mov     eax, DEFAULT_CR0
      mov     cr0, eax

      jmp     0x5a5a:dword jumpTo32BitAndLandHere
  BITS    32
  jumpTo32BitAndLandHere:

      mov     eax, DEFAULT_CR4
      mov     cr4, eax

      mov     ax, 0x5a5a
      mov     ds, ax
      mov     es, ax
      mov     fs, ax
      mov     gs, ax
      mov     ss, ax

  ;
  ; Jump to CpuDxe for IA32
  ;
      mov     eax, 0x5a5a5a5a
      or      eax, eax
      jz      Transition32FlatTo64Flat
      jmp     eax

  ;
  ; Transition to X64
  ;
  Transition32FlatTo64Flat:
      mov     eax, 0x5a5a5a5a
      mov     cr3, eax

      mov     eax, cr4
      bts     eax, 5                      ; enable PAE
      mov     cr4, eax

      mov     ecx, 0xc0000080
      rdmsr
      bts     eax, 8                      ; set LME
      wrmsr

      mov     eax, cr0
      bts     eax, 31                     ; set PG
      mov     cr0, eax                    ; enable paging

  ;
  ; Jump to CpuDxe for X64
  ;
      jmp     0x5a5a:jumpTo64BitAndLandHere
  BITS    64
  jumpTo64BitAndLandHere:
      mov     rax, 0xcdcdcdcdcdcdcdcd
      jmp     rax
**/
STARTUP_CODE mStartupCodeTemplate = {
  { 0xeb, 0x06 },                     // Jump to cli
  0,                                  // GDT Limit
  0,                                  // GDT Base
  0xfa,                               // cli (Clear Interrupts)
  0xb8, 0x0000,                       // mov ax, RealSegment
  { 0x8e, 0xd8 },                     // mov ds, ax
  { 0xBB, 0x02, 0x00 },               // mov bx, Gdtr
  { 0x3e, 0x66, 0x0f, 0x01, 0x17 },   // lgdt [ds:bx]
  { 0x66, 0xB8 }, 0x00000023,         // mov eax, cr0 value
  { 0x0F, 0x22, 0xC0 },               // mov cr0, eax
  { 0x66, 0xEA },                     // far jmp to 32-bit flat
        OFFSET_OF(STARTUP_CODE, MovEaxCr4),
        LINEAR_CODE_SEL,
  0xB8, 0x00000640,                   // mov eax, cr4 value
  { 0x0F, 0x22, 0xe0 },               // mov cr4, eax
  { 0x66, 0xb8 }, CPU_DATA_SEL,       // mov ax, FlatDataSelector
  { 0x8e, 0xd8 },                     // mov ds, ax
  { 0x8e, 0xc0 },                     // mov es, ax
  { 0x8e, 0xe0 },                     // mov fs, ax
  { 0x8e, 0xe8 },                     // mov gs, ax
  { 0x8e, 0xd0 },                     // mov ss, ax

#if defined (MDE_CPU_X64)
  //
  // Code placeholder to enable Execute Disable Bit for X64
  // Default is all NOP - No Operation
  //
  {
    { 0x90, 0x90, 0x90, 0x90, 0x90 },
    { 0x90, 0x90 },
    { 0x90, 0x90, 0x90, 0x90 },
    { 0x90, 0x90 },
  },

  0xB8, 0x00000000,                   // mov eax, cr3 value
  { 0x0F, 0x22, 0xd8 },               // mov cr3, eax

  { 0x0F, 0x20, 0xE0 },               // mov eax, cr4
  { 0x0F, 0xBA, 0xE8, 0x05 },         // bts eax, 5
  { 0x0F, 0x22, 0xE0 },               // mov cr4, eax

  { 0xB9, 0x80, 0x00, 0x00, 0xC0 },   // mov ecx, 0xc0000080
  { 0x0F, 0x32 },                     // rdmsr
  { 0x0F, 0xBA, 0xE8, 0x08 },         // bts eax, 8
  { 0x0F, 0x30 },                     // wrmsr

  { 0x0F, 0x20, 0xC0 },               // mov eax, cr0
  { 0x0F, 0xBA, 0xE8, 0x1F },         // bts eax, 31
  { 0x0F, 0x22, 0xC0 },               // mov cr0, eax

  0xEA,                               // FarJmp32LongMode
        OFFSET_OF(STARTUP_CODE, MovEaxOrRaxCpuDxeEntry),
        LINEAR_CODE64_SEL,
#else
  //
  // Code placeholder to enable PAE Execute Disable for IA32
  // Default is all NOP - No Operation
  //
  {
    { 0x90, 0x90, 0x90, 0x90, 0x90 },
    { 0x90, 0x90 },
    { 0x90, 0x90, 0x90, 0x90 },
    { 0x90, 0x90 },

    0x90, 0x90909090,
    { 0x90, 0x90, 0x90 },

    { 0x90, 0x90, 0x90 },
    { 0x90, 0x90, 0x90, 0x90 },
    { 0x90, 0x90, 0x90 },

    { 0x90, 0x90, 0x90 },
    { 0x90, 0x90, 0x90, 0x90 },
    { 0x90, 0x90, 0x90 },
  },
#endif

  //0xeb, 0xfe,       // jmp $
#if defined (MDE_CPU_X64)
  { 0x48, 0xb8 }, 0x0,                // mov rax, X64 CpuDxe MP Entry Point
#else
  0xB8, 0x0,                          // mov eax, IA32 CpuDxe MP Entry Point
#endif
  { 0xff, 0xe0 },                     // jmp to eax/rax (CpuDxe MP Entry Point)

};

volatile STARTUP_CODE *StartupCode = NULL;

/**
  The function will check if BSP Execute Disable is enabled.
  DxeIpl may have enabled Execute Disable for BSP,
  APs need to get the status and sync up the settings.

  @retval TRUE      BSP Execute Disable is enabled.
  @retval FALSE     BSP Execute Disable is not enabled.

**/
BOOLEAN
IsBspExecuteDisableEnabled (
  VOID
  )
{
  UINT32            RegEax;
  UINT32            RegEdx;
  UINT64            MsrRegisters;
  BOOLEAN           Enabled;

  Enabled = FALSE;
  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000001) {
    AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
    //
    // Cpuid 0x80000001
    // Bit 20: Execute Disable Bit available.
    //
    if ((RegEdx & BIT20) != 0) {
      MsrRegisters = AsmReadMsr64 (0xC0000080);
      //
      // Msr 0xC0000080
      // Bit 11: Execute Disable Bit enable.
      //
      if ((MsrRegisters & BIT11) != 0) {
        Enabled = TRUE;
      }
    }
  }

  return Enabled;
}

/**
  Prepares Startup Code for APs.
  This function prepares Startup Code for APs.

  @retval EFI_SUCCESS           The APs were started
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate memory to start APs

**/
EFI_STATUS
PrepareAPStartupCode (
  VOID
  )
{
  EFI_STATUS            Status;
  IA32_DESCRIPTOR       Gdtr;
  EFI_PHYSICAL_ADDRESS  StartAddress;

  StartAddress = BASE_1MB;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES (sizeof (*StartupCode)),
                  &StartAddress
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  StartupCode = (STARTUP_CODE*)(VOID*)(UINTN) StartAddress;
  CopyMem ((VOID*) StartupCode, &mStartupCodeTemplate, sizeof (*StartupCode));
  StartupCode->RealSegment = (UINT16) (((UINTN) StartAddress) >> 4);

  AsmReadGdtr (&Gdtr);
  StartupCode->GdtLimit = Gdtr.Limit;
  StartupCode->GdtBase = (UINT32) Gdtr.Base;

  StartupCode->CpuDxeEntryValue = (UINTN) AsmApEntryPoint;

  StartupCode->FlatJmpOffset += (UINT32) StartAddress;

  if (IsBspExecuteDisableEnabled ()) {
    CopyMem (
      (VOID*) &StartupCode->EnableExecuteDisable,
      &mEnableExecuteDisableCodeTemplate,
      sizeof (ENABLE_EXECUTE_DISABLE_CODE)
      );
  }
#if defined (MDE_CPU_X64)
  StartupCode->Cr3Value = (UINT32) AsmReadCr3 ();
  StartupCode->LongJmpOffset += (UINT32) StartAddress;
#else
  StartupCode->EnableExecuteDisable.Cr3Value = (UINT32) AsmReadCr3 ();
#endif

  return EFI_SUCCESS;
}

/**
  Free the code buffer of startup AP.

**/
VOID
FreeApStartupCode (
  VOID
  )
{
  if (StartupCode != NULL) {
    gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)(VOID*) StartupCode,
                    EFI_SIZE_TO_PAGES (sizeof (*StartupCode)));
  }
}


/**
  Starts the Application Processors and directs them to jump to the
  specified routine.

  The processor jumps to this code in flat mode, but the processor's
  stack is not initialized.

  @retval EFI_SUCCESS           The APs were started

**/
EFI_STATUS
StartApsStackless (
  VOID
  )
{
  SendInitSipiSipiAllExcludingSelf ((UINT32)(UINTN)(VOID*) StartupCode);
  //
  // Wait 100 milliseconds for APs to arrive at the ApEntryPoint routine
  //
  MicroSecondDelay (100 * 1000);

  return EFI_SUCCESS;
}

/**
  Resets the Application Processor and directs it to jump to the
  specified routine.

  The processor jumps to this code in flat mode, but the processor's
  stack is not initialized.

  @param ProcessorId           the AP of ProcessorId was reset
**/
VOID
ResetApStackless (
  IN UINT32 ProcessorId
  )
{
  SendInitSipiSipi (ProcessorId,
                    (UINT32)(UINTN)(VOID*) StartupCode);
}

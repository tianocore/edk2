/** @file
  Default exception handler

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2012 - 2021, Arm Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PrintLib.h>
#include <Library/ArmDisassemblerLib.h>
#include <Library/SerialPortLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Guid/DebugImageInfoTable.h>

#include <Protocol/DebugSupport.h>
#include <Library/DefaultExceptionHandlerLib.h>

//
// The number of elements in a CHAR8 array, including the terminating NUL, that
// is meant to hold the string rendering of the CPSR.
//
#define CPSR_STRING_SIZE 32

typedef struct {
  UINT32  BIT;
  CHAR8   Char;
} CPSR_CHAR;

STATIC CONST CPSR_CHAR mCpsrChar[] = {
  { 31, 'n' },
  { 30, 'z' },
  { 29, 'c' },
  { 28, 'v' },

  { 9,  'e' },
  { 8,  'a' },
  { 7,  'i' },
  { 6,  'f' },
  { 5,  't' },
  { 0,  '?' }
};

CHAR8 *
GetImageName (
  IN  UINTN  FaultAddress,
  OUT UINTN  *ImageBase,
  OUT UINTN  *PeCoffSizeOfHeaders
  );

/**
  Convert the Current Program Status Register (CPSR) to a string. The string is
  a defacto standard in the ARM world.

  It is possible to add extra bits by adding them to mCpsrChar array.

  @param  Cpsr         ARM CPSR register value
  @param  ReturnStr    CPSR_STRING_SIZE byte string that contains string
                       version of CPSR

**/
VOID
CpsrString (
  IN  UINT32  Cpsr,
  OUT CHAR8   *ReturnStr
  )
{
  UINTN     Index;
  CHAR8*    Str;
  CHAR8*    ModeStr;

  Str = ReturnStr;

  for (Index = 0; mCpsrChar[Index].BIT != 0; Index++, Str++) {
    *Str = mCpsrChar[Index].Char;
    if ((Cpsr & (1 << mCpsrChar[Index].BIT)) != 0) {
      // Concert to upper case if bit is set
      *Str &= ~0x20;
    }
  }

  *Str++ = '_';
  *Str = '\0';

  switch (Cpsr & 0x1f) {
  case 0x10:
    ModeStr = "usr";
    break;
  case 0x011:
    ModeStr = "fiq";
    break;
  case 0x12:
    ModeStr = "irq";
    break;
  case 0x13:
    ModeStr = "svc";
    break;
  case 0x16:
    ModeStr = "mon";
    break;
  case 0x17:
    ModeStr = "abt";
    break;
  case 0x1b:
    ModeStr = "und";
    break;
  case 0x1f:
    ModeStr = "sys";
    break;

  default:
    ModeStr = "???";
    break;
  }

  //
  // See the interface contract in the leading comment block.
  //
  AsciiStrCatS (Str, CPSR_STRING_SIZE - (Str - ReturnStr), ModeStr);
}

CHAR8 *
FaultStatusToString (
  IN  UINT32  Status
  )
{
  CHAR8 *FaultSource;

  switch (Status) {
    case 0x01: FaultSource = "Alignment fault"; break;
    case 0x02: FaultSource = "Debug event fault"; break;
    case 0x03: FaultSource = "Access Flag fault on Section"; break;
    case 0x04: FaultSource = "Cache maintenance operation fault[2]"; break;
    case 0x05: FaultSource = "Translation fault on Section"; break;
    case 0x06: FaultSource = "Access Flag fault on Page"; break;
    case 0x07: FaultSource = "Translation fault on Page"; break;
    case 0x08: FaultSource = "Precise External Abort"; break;
    case 0x09: FaultSource = "Domain fault on Section"; break;
    case 0x0b: FaultSource = "Domain fault on Page"; break;
    case 0x0c: FaultSource = "External abort on translation, first level"; break;
    case 0x0d: FaultSource = "Permission fault on Section"; break;
    case 0x0e: FaultSource = "External abort on translation, second level"; break;
    case 0x0f: FaultSource = "Permission fault on Page"; break;
    case 0x16: FaultSource = "Imprecise External Abort"; break;
    default:   FaultSource = "No function"; break;
    }

  return FaultSource;
}

STATIC CHAR8 *gExceptionTypeString[] = {
  "Reset",
  "Undefined OpCode",
  "SVC",
  "Prefetch Abort",
  "Data Abort",
  "Undefined",
  "IRQ",
  "FIQ"
};

/**
  This is the default action to take on an unexpected exception

  Since this is exception context don't do anything crazy like try to allocate memory.

  @param  ExceptionType    Type of the exception
  @param  SystemContext    Register state at the time of the Exception


**/
VOID
DefaultExceptionHandler (
  IN     EFI_EXCEPTION_TYPE           ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT           SystemContext
  )
{
  CHAR8     Buffer[100];
  UINTN     CharCount;
  UINT32    DfsrStatus;
  UINT32    IfsrStatus;
  BOOLEAN   DfsrWrite;
  UINT32    PcAdjust;

  PcAdjust = 0;

  CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"\n%a Exception PC at 0x%08x  CPSR 0x%08x ",
         gExceptionTypeString[ExceptionType], SystemContext.SystemContextArm->PC, SystemContext.SystemContextArm->CPSR);
  SerialPortWrite ((UINT8 *)Buffer, CharCount);
  if (gST->ConOut != NULL) {
    AsciiPrint (Buffer);
  }

  DEBUG_CODE_BEGIN ();
    CHAR8   *Pdb;
    UINT32  ImageBase;
    UINT32  PeCoffSizeOfHeader;
    UINT32  Offset;
    CHAR8   CpsrStr[CPSR_STRING_SIZE];  // char per bit. Lower 5-bits are mode
                                        // that is a 3 char string
    CHAR8   Buffer[80];
    UINT8   *DisAsm;
    UINT32  ItBlock;

    CpsrString (SystemContext.SystemContextArm->CPSR, CpsrStr);
    DEBUG ((EFI_D_ERROR, "%a\n", CpsrStr));

    Pdb = GetImageName (SystemContext.SystemContextArm->PC, &ImageBase, &PeCoffSizeOfHeader);
    Offset = SystemContext.SystemContextArm->PC - ImageBase;
    if (Pdb != NULL) {
      DEBUG ((EFI_D_ERROR, "%a\n", Pdb));

      //
      // A PE/COFF image loads its headers into memory so the headers are
      // included in the linked addresses. ELF and Mach-O images do not
      // include the headers so the first byte of the image is usually
      // text (code). If you look at link maps from ELF or Mach-O images
      // you need to subtract out the size of the PE/COFF header to get
      // get the offset that matches the link map.
      //
      DEBUG ((EFI_D_ERROR, "loaded at 0x%08x (PE/COFF offset) 0x%x (ELF or Mach-O offset) 0x%x", ImageBase, Offset, Offset - PeCoffSizeOfHeader));

      // If we come from an image it is safe to show the instruction. We know it should not fault
      DisAsm = (UINT8 *)(UINTN)SystemContext.SystemContextArm->PC;
      ItBlock = 0;
      DisassembleInstruction (&DisAsm, (SystemContext.SystemContextArm->CPSR & BIT5) == BIT5, TRUE, &ItBlock, Buffer, sizeof (Buffer));
      DEBUG ((EFI_D_ERROR, "\n%a", Buffer));

      switch (ExceptionType) {
      case EXCEPT_ARM_UNDEFINED_INSTRUCTION:
      case EXCEPT_ARM_SOFTWARE_INTERRUPT:
      case EXCEPT_ARM_PREFETCH_ABORT:
      case EXCEPT_ARM_DATA_ABORT:
        // advance PC past the faulting instruction
        PcAdjust = (UINTN)DisAsm - SystemContext.SystemContextArm->PC;
        break;

      default:
        break;
      }

    }
  DEBUG_CODE_END ();
  DEBUG ((EFI_D_ERROR, "\n  R0 0x%08x   R1 0x%08x   R2 0x%08x   R3 0x%08x\n", SystemContext.SystemContextArm->R0, SystemContext.SystemContextArm->R1, SystemContext.SystemContextArm->R2, SystemContext.SystemContextArm->R3));
  DEBUG ((EFI_D_ERROR, "  R4 0x%08x   R5 0x%08x   R6 0x%08x   R7 0x%08x\n", SystemContext.SystemContextArm->R4, SystemContext.SystemContextArm->R5, SystemContext.SystemContextArm->R6, SystemContext.SystemContextArm->R7));
  DEBUG ((EFI_D_ERROR, "  R8 0x%08x   R9 0x%08x  R10 0x%08x  R11 0x%08x\n", SystemContext.SystemContextArm->R8, SystemContext.SystemContextArm->R9, SystemContext.SystemContextArm->R10, SystemContext.SystemContextArm->R11));
  DEBUG ((EFI_D_ERROR, " R12 0x%08x   SP 0x%08x   LR 0x%08x   PC 0x%08x\n", SystemContext.SystemContextArm->R12, SystemContext.SystemContextArm->SP, SystemContext.SystemContextArm->LR, SystemContext.SystemContextArm->PC));
  DEBUG ((EFI_D_ERROR, "DFSR 0x%08x DFAR 0x%08x IFSR 0x%08x IFAR 0x%08x\n", SystemContext.SystemContextArm->DFSR, SystemContext.SystemContextArm->DFAR, SystemContext.SystemContextArm->IFSR, SystemContext.SystemContextArm->IFAR));

  // Bit10 is Status[4] Bit3:0 is Status[3:0]
  DfsrStatus = (SystemContext.SystemContextArm->DFSR & 0xf) | ((SystemContext.SystemContextArm->DFSR >> 6) & 0x10);
  DfsrWrite = (SystemContext.SystemContextArm->DFSR & BIT11) != 0;
  if (DfsrStatus != 0x00) {
    DEBUG ((EFI_D_ERROR, " %a: %a 0x%08x\n", FaultStatusToString (DfsrStatus), DfsrWrite ? "write to" : "read from", SystemContext.SystemContextArm->DFAR));
  }

  IfsrStatus = (SystemContext.SystemContextArm->IFSR & 0xf) | ((SystemContext.SystemContextArm->IFSR >> 6) & 0x10);
  if (IfsrStatus != 0) {
    DEBUG ((EFI_D_ERROR, " Instruction %a at 0x%08x\n", FaultStatusToString (SystemContext.SystemContextArm->IFSR & 0xf), SystemContext.SystemContextArm->IFAR));
  }

  DEBUG ((EFI_D_ERROR, "\n"));
  ASSERT (FALSE);

  CpuDeadLoop ();   // may return if executing under a debugger

  // Clear the error registers that we have already displayed incase some one wants to keep going
  SystemContext.SystemContextArm->DFSR = 0;
  SystemContext.SystemContextArm->IFSR = 0;

  // If some one is stepping past the exception handler adjust the PC to point to the next instruction
  SystemContext.SystemContextArm->PC += PcAdjust;
}

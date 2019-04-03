/** @file
  Default exception handler

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PrintLib.h>
#include <Library/ArmDisassemblerLib.h>
#include <Library/SerialPortLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Guid/DebugImageInfoTable.h>
#include <Protocol/DebugSupport.h>
#include <Protocol/LoadedImage.h>

STATIC CHAR8 *gExceptionTypeString[] = {
  "Synchronous",
  "IRQ",
  "FIQ",
  "SError"
};

STATIC BOOLEAN mRecursiveException;

CHAR8 *
GetImageName (
  IN  UINTN  FaultAddress,
  OUT UINTN  *ImageBase,
  OUT UINTN  *PeCoffSizeOfHeaders
  );

STATIC
VOID
DescribeInstructionOrDataAbort (
  IN CHAR8 *AbortType,
  IN UINTN Iss
  )
{
  CHAR8 *AbortCause;

  switch (Iss & 0x3f) {
    case 0x0: AbortCause = "Address size fault, zeroth level of translation or translation table base register";  break;
    case 0x1: AbortCause = "Address size fault, first level";  break;
    case 0x2: AbortCause = "Address size fault, second level";  break;
    case 0x3: AbortCause = "Address size fault, third level";  break;
    case 0x4: AbortCause = "Translation fault, zeroth level";  break;
    case 0x5: AbortCause = "Translation fault, first level";  break;
    case 0x6: AbortCause = "Translation fault, second level";  break;
    case 0x7: AbortCause = "Translation fault, third level";  break;
    case 0x9: AbortCause = "Access flag fault, first level";  break;
    case 0xa: AbortCause = "Access flag fault, second level";  break;
    case 0xb: AbortCause = "Access flag fault, third level";  break;
    case 0xd: AbortCause = "Permission fault, first level";  break;
    case 0xe: AbortCause = "Permission fault, second level";  break;
    case 0xf: AbortCause = "Permission fault, third level";  break;
    case 0x10: AbortCause = "Synchronous external abort";  break;
    case 0x18: AbortCause = "Synchronous parity error on memory access";  break;
    case 0x11: AbortCause = "Asynchronous external abort";  break;
    case 0x19: AbortCause = "Asynchronous parity error on memory access";  break;
    case 0x14: AbortCause = "Synchronous external abort on translation table walk, zeroth level";  break;
    case 0x15: AbortCause = "Synchronous external abort on translation table walk, first level";  break;
    case 0x16: AbortCause = "Synchronous external abort on translation table walk, second level";  break;
    case 0x17: AbortCause = "Synchronous external abort on translation table walk, third level";  break;
    case 0x1c: AbortCause = "Synchronous parity error on memory access on translation table walk, zeroth level";  break;
    case 0x1d: AbortCause = "Synchronous parity error on memory access on translation table walk, first level";  break;
    case 0x1e: AbortCause = "Synchronous parity error on memory access on translation table walk, second level";  break;
    case 0x1f: AbortCause = "Synchronous parity error on memory access on translation table walk, third level";  break;
    case 0x21: AbortCause = "Alignment fault";  break;
    case 0x22: AbortCause = "Debug event";  break;
    case 0x30: AbortCause = "TLB conflict abort";  break;
    case 0x33:
    case 0x34: AbortCause = "IMPLEMENTATION DEFINED";  break;
    case 0x35:
    case 0x36: AbortCause = "Domain fault"; break;
    default: AbortCause = ""; break;
  }

  DEBUG ((EFI_D_ERROR, "\n%a: %a\n", AbortType, AbortCause));
}

STATIC
VOID
DescribeExceptionSyndrome (
  IN UINT32 Esr
  )
{
  CHAR8 *Message;
  UINTN Ec;
  UINTN Iss;

  Ec = Esr >> 26;
  Iss = Esr & 0x00ffffff;

  switch (Ec) {
    case 0x15: Message = "SVC executed in AArch64"; break;
    case 0x20:
    case 0x21: DescribeInstructionOrDataAbort ("Instruction abort", Iss); return;
    case 0x22: Message = "PC alignment fault"; break;
    case 0x23: Message = "SP alignment fault"; break;
    case 0x24:
    case 0x25: DescribeInstructionOrDataAbort ("Data abort", Iss); return;
    default: return;
  }

  DEBUG ((EFI_D_ERROR, "\n %a \n", Message));
}

#ifndef MDEPKG_NDEBUG
STATIC
CONST CHAR8 *
BaseName (
  IN  CONST CHAR8 *FullName
  )
{
  CONST CHAR8 *Str;

  Str = FullName + AsciiStrLen (FullName);

  while (--Str > FullName) {
    if (*Str == '/' || *Str == '\\') {
      return Str + 1;
    }
  }
  return Str;
}
#endif

/**
  This is the default action to take on an unexpected exception

  Since this is exception context don't do anything crazy like try to allcoate memory.

  @param  ExceptionType    Type of the exception
  @param  SystemContext    Register state at the time of the Exception

**/
VOID
DefaultExceptionHandler (
  IN     EFI_EXCEPTION_TYPE           ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT           SystemContext
  )
{
  CHAR8  Buffer[100];
  UINTN  CharCount;
  INT32  Offset;

  if (mRecursiveException) {
    STATIC CHAR8 CONST Message[] = "\nRecursive exception occurred while dumping the CPU state\n";

    SerialPortWrite ((UINT8 *)Message, sizeof Message - 1);
    if (gST->ConOut != NULL) {
      AsciiPrint (Message);
    }
    CpuDeadLoop ();
  }
  mRecursiveException = TRUE;

  CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"\n\n%a Exception at 0x%016lx\n", gExceptionTypeString[ExceptionType], SystemContext.SystemContextAArch64->ELR);
  SerialPortWrite ((UINT8 *) Buffer, CharCount);
  if (gST->ConOut != NULL) {
    AsciiPrint (Buffer);
  }

  DEBUG_CODE_BEGIN ();
    CHAR8  *Pdb, *PrevPdb;
    UINTN  ImageBase;
    UINTN  PeCoffSizeOfHeader;
    UINT64 *Fp;
    UINT64 RootFp[2];
    UINTN  Idx;

    PrevPdb = Pdb = GetImageName (SystemContext.SystemContextAArch64->ELR, &ImageBase, &PeCoffSizeOfHeader);
    if (Pdb != NULL) {
      DEBUG ((EFI_D_ERROR, "PC 0x%012lx (0x%012lx+0x%08x) [ 0] %a\n",
        SystemContext.SystemContextAArch64->ELR, ImageBase,
        SystemContext.SystemContextAArch64->ELR - ImageBase, BaseName (Pdb)));
    } else {
      DEBUG ((EFI_D_ERROR, "PC 0x%012lx\n", SystemContext.SystemContextAArch64->ELR));
    }

    if ((UINT64 *)SystemContext.SystemContextAArch64->FP != 0) {
      Idx = 0;

      RootFp[0] = ((UINT64 *)SystemContext.SystemContextAArch64->FP)[0];
      RootFp[1] = ((UINT64 *)SystemContext.SystemContextAArch64->FP)[1];
      if (RootFp[1] != SystemContext.SystemContextAArch64->LR) {
        RootFp[0] = SystemContext.SystemContextAArch64->FP;
        RootFp[1] = SystemContext.SystemContextAArch64->LR;
      }
      for (Fp = RootFp; Fp[0] != 0; Fp = (UINT64 *)Fp[0]) {
        Pdb = GetImageName (Fp[1], &ImageBase, &PeCoffSizeOfHeader);
        if (Pdb != NULL) {
          if (Pdb != PrevPdb) {
            Idx++;
            PrevPdb = Pdb;
          }
          DEBUG ((EFI_D_ERROR, "PC 0x%012lx (0x%012lx+0x%08x) [% 2d] %a\n",
            Fp[1], ImageBase, Fp[1] - ImageBase, Idx, BaseName (Pdb)));
        } else {
          DEBUG ((EFI_D_ERROR, "PC 0x%012lx\n", Fp[1]));
        }
      }
      PrevPdb = Pdb = GetImageName (SystemContext.SystemContextAArch64->ELR, &ImageBase, &PeCoffSizeOfHeader);
      if (Pdb != NULL) {
        DEBUG ((EFI_D_ERROR, "\n[ 0] %a\n", Pdb));
      }

      Idx = 0;
      for (Fp = RootFp; Fp[0] != 0; Fp = (UINT64 *)Fp[0]) {
        Pdb = GetImageName (Fp[1], &ImageBase, &PeCoffSizeOfHeader);
        if (Pdb != NULL && Pdb != PrevPdb) {
          DEBUG ((EFI_D_ERROR, "[% 2d] %a\n", ++Idx, Pdb));
          PrevPdb = Pdb;
        }
      }
    }
  DEBUG_CODE_END ();

  DEBUG ((EFI_D_ERROR, "\n  X0 0x%016lx   X1 0x%016lx   X2 0x%016lx   X3 0x%016lx\n", SystemContext.SystemContextAArch64->X0, SystemContext.SystemContextAArch64->X1, SystemContext.SystemContextAArch64->X2, SystemContext.SystemContextAArch64->X3));
  DEBUG ((EFI_D_ERROR, "  X4 0x%016lx   X5 0x%016lx   X6 0x%016lx   X7 0x%016lx\n", SystemContext.SystemContextAArch64->X4, SystemContext.SystemContextAArch64->X5, SystemContext.SystemContextAArch64->X6, SystemContext.SystemContextAArch64->X7));
  DEBUG ((EFI_D_ERROR, "  X8 0x%016lx   X9 0x%016lx  X10 0x%016lx  X11 0x%016lx\n", SystemContext.SystemContextAArch64->X8, SystemContext.SystemContextAArch64->X9, SystemContext.SystemContextAArch64->X10, SystemContext.SystemContextAArch64->X11));
  DEBUG ((EFI_D_ERROR, " X12 0x%016lx  X13 0x%016lx  X14 0x%016lx  X15 0x%016lx\n", SystemContext.SystemContextAArch64->X12, SystemContext.SystemContextAArch64->X13, SystemContext.SystemContextAArch64->X14, SystemContext.SystemContextAArch64->X15));
  DEBUG ((EFI_D_ERROR, " X16 0x%016lx  X17 0x%016lx  X18 0x%016lx  X19 0x%016lx\n", SystemContext.SystemContextAArch64->X16, SystemContext.SystemContextAArch64->X17, SystemContext.SystemContextAArch64->X18, SystemContext.SystemContextAArch64->X19));
  DEBUG ((EFI_D_ERROR, " X20 0x%016lx  X21 0x%016lx  X22 0x%016lx  X23 0x%016lx\n", SystemContext.SystemContextAArch64->X20, SystemContext.SystemContextAArch64->X21, SystemContext.SystemContextAArch64->X22, SystemContext.SystemContextAArch64->X23));
  DEBUG ((EFI_D_ERROR, " X24 0x%016lx  X25 0x%016lx  X26 0x%016lx  X27 0x%016lx\n", SystemContext.SystemContextAArch64->X24, SystemContext.SystemContextAArch64->X25, SystemContext.SystemContextAArch64->X26, SystemContext.SystemContextAArch64->X27));
  DEBUG ((EFI_D_ERROR, " X28 0x%016lx   FP 0x%016lx   LR 0x%016lx  \n", SystemContext.SystemContextAArch64->X28, SystemContext.SystemContextAArch64->FP, SystemContext.SystemContextAArch64->LR));

  /* We save these as 128bit numbers, but have to print them as two 64bit numbers,
     so swap the 64bit words to correctly represent a 128bit number.  */
  DEBUG ((EFI_D_ERROR, "\n  V0 0x%016lx %016lx   V1 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V0[1], SystemContext.SystemContextAArch64->V0[0], SystemContext.SystemContextAArch64->V1[1], SystemContext.SystemContextAArch64->V1[0]));
  DEBUG ((EFI_D_ERROR, "  V2 0x%016lx %016lx   V3 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V2[1], SystemContext.SystemContextAArch64->V2[0], SystemContext.SystemContextAArch64->V3[1], SystemContext.SystemContextAArch64->V3[0]));
  DEBUG ((EFI_D_ERROR, "  V4 0x%016lx %016lx   V5 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V4[1], SystemContext.SystemContextAArch64->V4[0], SystemContext.SystemContextAArch64->V5[1], SystemContext.SystemContextAArch64->V5[0]));
  DEBUG ((EFI_D_ERROR, "  V6 0x%016lx %016lx   V7 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V6[1], SystemContext.SystemContextAArch64->V6[0], SystemContext.SystemContextAArch64->V7[1], SystemContext.SystemContextAArch64->V7[0]));
  DEBUG ((EFI_D_ERROR, "  V8 0x%016lx %016lx   V9 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V8[1], SystemContext.SystemContextAArch64->V8[0], SystemContext.SystemContextAArch64->V9[1], SystemContext.SystemContextAArch64->V9[0]));
  DEBUG ((EFI_D_ERROR, " V10 0x%016lx %016lx  V11 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V10[1], SystemContext.SystemContextAArch64->V10[0], SystemContext.SystemContextAArch64->V11[1], SystemContext.SystemContextAArch64->V11[0]));
  DEBUG ((EFI_D_ERROR, " V12 0x%016lx %016lx  V13 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V12[1], SystemContext.SystemContextAArch64->V12[0], SystemContext.SystemContextAArch64->V13[1], SystemContext.SystemContextAArch64->V13[0]));
  DEBUG ((EFI_D_ERROR, " V14 0x%016lx %016lx  V15 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V14[1], SystemContext.SystemContextAArch64->V14[0], SystemContext.SystemContextAArch64->V15[1], SystemContext.SystemContextAArch64->V15[0]));
  DEBUG ((EFI_D_ERROR, " V16 0x%016lx %016lx  V17 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V16[1], SystemContext.SystemContextAArch64->V16[0], SystemContext.SystemContextAArch64->V17[1], SystemContext.SystemContextAArch64->V17[0]));
  DEBUG ((EFI_D_ERROR, " V18 0x%016lx %016lx  V19 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V18[1], SystemContext.SystemContextAArch64->V18[0], SystemContext.SystemContextAArch64->V19[1], SystemContext.SystemContextAArch64->V19[0]));
  DEBUG ((EFI_D_ERROR, " V20 0x%016lx %016lx  V21 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V20[1], SystemContext.SystemContextAArch64->V20[0], SystemContext.SystemContextAArch64->V21[1], SystemContext.SystemContextAArch64->V21[0]));
  DEBUG ((EFI_D_ERROR, " V22 0x%016lx %016lx  V23 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V22[1], SystemContext.SystemContextAArch64->V22[0], SystemContext.SystemContextAArch64->V23[1], SystemContext.SystemContextAArch64->V23[0]));
  DEBUG ((EFI_D_ERROR, " V24 0x%016lx %016lx  V25 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V24[1], SystemContext.SystemContextAArch64->V24[0], SystemContext.SystemContextAArch64->V25[1], SystemContext.SystemContextAArch64->V25[0]));
  DEBUG ((EFI_D_ERROR, " V26 0x%016lx %016lx  V27 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V26[1], SystemContext.SystemContextAArch64->V26[0], SystemContext.SystemContextAArch64->V27[1], SystemContext.SystemContextAArch64->V27[0]));
  DEBUG ((EFI_D_ERROR, " V28 0x%016lx %016lx  V29 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V28[1], SystemContext.SystemContextAArch64->V28[0], SystemContext.SystemContextAArch64->V29[1], SystemContext.SystemContextAArch64->V29[0]));
  DEBUG ((EFI_D_ERROR, " V30 0x%016lx %016lx  V31 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V30[1], SystemContext.SystemContextAArch64->V30[0], SystemContext.SystemContextAArch64->V31[1], SystemContext.SystemContextAArch64->V31[0]));

  DEBUG ((EFI_D_ERROR, "\n  SP 0x%016lx  ELR 0x%016lx  SPSR 0x%08lx  FPSR 0x%08lx\n ESR 0x%08lx          FAR 0x%016lx\n", SystemContext.SystemContextAArch64->SP, SystemContext.SystemContextAArch64->ELR, SystemContext.SystemContextAArch64->SPSR, SystemContext.SystemContextAArch64->FPSR, SystemContext.SystemContextAArch64->ESR, SystemContext.SystemContextAArch64->FAR));

  DEBUG ((EFI_D_ERROR, "\n ESR : EC 0x%02x  IL 0x%x  ISS 0x%08x\n", (SystemContext.SystemContextAArch64->ESR & 0xFC000000) >> 26, (SystemContext.SystemContextAArch64->ESR >> 25) & 0x1, SystemContext.SystemContextAArch64->ESR & 0x1FFFFFF ));

  DescribeExceptionSyndrome (SystemContext.SystemContextAArch64->ESR);

  DEBUG ((EFI_D_ERROR, "\nStack dump:\n"));
  for (Offset = -256; Offset < 256; Offset += 32) {
    DEBUG  ((EFI_D_ERROR, "%c %013lx: %016lx %016lx %016lx %016lx\n",
      Offset == 0 ? '>' : ' ',
      SystemContext.SystemContextAArch64->SP + Offset,
      *(UINT64 *)(SystemContext.SystemContextAArch64->SP + Offset),
      *(UINT64 *)(SystemContext.SystemContextAArch64->SP + Offset + 8),
      *(UINT64 *)(SystemContext.SystemContextAArch64->SP + Offset + 16),
      *(UINT64 *)(SystemContext.SystemContextAArch64->SP + Offset + 24)));
  }

  ASSERT (FALSE);
  CpuDeadLoop ();
}

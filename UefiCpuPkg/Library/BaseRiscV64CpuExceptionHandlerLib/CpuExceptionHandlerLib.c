/** @file
  RISC-V Exception Handler library implementation.

  Copyright (c) 2016 - 2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PrintLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/UefiLib.h>
#include <Register/RiscV64/RiscVEncoding.h>
#include <Guid/DebugImageInfoTable.h>
#include "CpuExceptionHandlerLib.h"

//
// Define the maximum message length
//
#define MAX_DEBUG_MESSAGE_LENGTH  0x100

STATIC EFI_CPU_INTERRUPT_HANDLER  mExceptionHandlers[EXCEPT_RISCV_MAX_EXCEPTIONS + 1];
STATIC EFI_CPU_INTERRUPT_HANDLER  mIrqHandlers[EXCEPT_RISCV_MAX_IRQS + 1];

STATIC CONST CHAR8  mExceptionOrIrqUnknown[]                            = "Unknown";
STATIC CONST CHAR8  *mExceptionNameStr[EXCEPT_RISCV_MAX_EXCEPTIONS + 1] = {
  "EXCEPT_RISCV_INST_MISALIGNED",
  "EXCEPT_RISCV_INST_ACCESS_FAULT",
  "EXCEPT_RISCV_ILLEGAL_INST",
  "EXCEPT_RISCV_BREAKPOINT",
  "EXCEPT_RISCV_LOAD_ADDRESS_MISALIGNED",
  "EXCEPT_RISCV_LOAD_ACCESS_FAULT",
  "EXCEPT_RISCV_STORE_AMO_ADDRESS_MISALIGNED",
  "EXCEPT_RISCV_STORE_AMO_ACCESS_FAULT",
  "EXCEPT_RISCV_ENV_CALL_FROM_UMODE",
  "EXCEPT_RISCV_ENV_CALL_FROM_SMODE",
  "EXCEPT_RISCV_ENV_CALL_FROM_VS_MODE",
  "EXCEPT_RISCV_ENV_CALL_FROM_MMODE",
  "EXCEPT_RISCV_INST_ACCESS_PAGE_FAULT",
  "EXCEPT_RISCV_LOAD_ACCESS_PAGE_FAULT",
  "EXCEPT_RISCV_14",
  "EXCEPT_RISCV_STORE_ACCESS_PAGE_FAULT",
  "EXCEPT_RISCV_16",
  "EXCEPT_RISCV_17",
  "EXCEPT_RISCV_18",
  "EXCEPT_RISCV_19",
  "EXCEPT_RISCV_INST_GUEST_PAGE_FAULT",
  "EXCEPT_RISCV_LOAD_GUEST_PAGE_FAULT",
  "EXCEPT_RISCV_VIRTUAL_INSTRUCTION",
  "EXCEPT_RISCV_STORE_GUEST_PAGE_FAULT"
};

STATIC CONST CHAR8  *mIrqNameStr[EXCEPT_RISCV_MAX_IRQS + 1] = {
  "EXCEPT_RISCV_IRQ_0",
  "EXCEPT_RISCV_IRQ_SOFT_FROM_SMODE",
  "EXCEPT_RISCV_IRQ_SOFT_FROM_VSMODE",
  "EXCEPT_RISCV_IRQ_SOFT_FROM_MMODE",
  "EXCEPT_RISCV_IRQ_4",
  "EXCEPT_RISCV_IRQ_TIMER_FROM_SMODE",
};

STATIC BOOLEAN  mRecursiveException;

/**
  Use the EFI Debug Image Table to lookup the FaultAddress and find which PE/COFF image
  it came from. As long as the PE/COFF image contains a debug directory entry a
  string can be returned. For ELF and Mach-O images the string points to the Mach-O or ELF
  image. Microsoft tools contain a pointer to the PDB file that contains the debug information.

  @param  FaultAddress         Address to find PE/COFF image for.
  @param  ImageBase            Return load address of found image
  @param  PeCoffSizeOfHeaders  Return the size of the PE/COFF header for the image that was found

  @retval NULL                 FaultAddress not in a loaded PE/COFF image.
  @retval                      Path and file name of PE/COFF image.

**/
STATIC
CHAR8 *
GetImageName (
  IN  UINTN  FaultAddress,
  OUT UINTN  *ImageBase,
  OUT UINTN  *PeCoffSizeOfHeaders
  )
{
  EFI_STATUS                         Status;
  EFI_DEBUG_IMAGE_INFO_TABLE_HEADER  *DebugTableHeader;
  EFI_DEBUG_IMAGE_INFO               *DebugTable;
  UINTN                              Entry;
  CHAR8                              *Address;

  Status = EfiGetSystemConfigurationTable (&gEfiDebugImageInfoTableGuid, (VOID **)&DebugTableHeader);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  DebugTable = DebugTableHeader->EfiDebugImageInfoTable;
  if (DebugTable == NULL) {
    return NULL;
  }

  Address = (CHAR8 *)(UINTN)FaultAddress;
  for (Entry = 0; Entry < DebugTableHeader->TableSize; Entry++, DebugTable++) {
    if (DebugTable->NormalImage != NULL) {
      if ((DebugTable->NormalImage->ImageInfoType == EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL) &&
          (DebugTable->NormalImage->LoadedImageProtocolInstance != NULL))
      {
        if ((Address >= (CHAR8 *)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase) &&
            (Address <= ((CHAR8 *)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase + DebugTable->NormalImage->LoadedImageProtocolInstance->ImageSize)))
        {
          *ImageBase           = (UINTN)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase;
          *PeCoffSizeOfHeaders = PeCoffGetSizeOfHeaders ((VOID *)(UINTN)*ImageBase);
          return PeCoffLoaderGetPdbPointer (DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase);
        }
      }
    }
  }

  return NULL;
}

STATIC
CONST CHAR8 *
BaseName (
  IN  CONST CHAR8  *FullName
  )
{
  CONST CHAR8  *Str;

  Str = FullName + AsciiStrLen (FullName);

  while (--Str > FullName) {
    if ((*Str == '/') || (*Str == '\\')) {
      return Str + 1;
    }
  }

  return Str;
}

/**
  Prints a message to the serial port.

  @param  Format      Format string for the message to print.
  @param  ...         Variable argument list whose contents are accessed
                      based on the format string specified by Format.

**/
STATIC
VOID
EFIAPI
InternalPrintMessage (
  IN  CONST CHAR8  *Format,
  ...
  )
{
  CHAR8    Buffer[MAX_DEBUG_MESSAGE_LENGTH];
  VA_LIST  Marker;

  //
  // Convert the message to an ASCII String
  //
  VA_START (Marker, Format);
  AsciiVSPrint (Buffer, sizeof (Buffer), Format, Marker);
  VA_END (Marker);

  //
  // Send the print string to a Serial Port
  //
  SerialPortWrite ((UINT8 *)Buffer, AsciiStrLen (Buffer));
}

/**
  Get ASCII format string exception name by exception type.

  @param ExceptionType  Exception type.

  @return  ASCII format string exception name.
**/
STATIC
CONST CHAR8 *
GetExceptionNameStr (
  IN EFI_EXCEPTION_TYPE  ExceptionType
  )
{
  if (EXCEPT_RISCV_IS_IRQ (ExceptionType)) {
    if (EXCEPT_RISCV_IRQ_INDEX (ExceptionType) > EXCEPT_RISCV_MAX_IRQS) {
      return mExceptionOrIrqUnknown;
    }

    return mIrqNameStr[EXCEPT_RISCV_IRQ_INDEX (ExceptionType)];
  }

  if (ExceptionType > EXCEPT_RISCV_MAX_EXCEPTIONS) {
    return mExceptionOrIrqUnknown;
  }

  return mExceptionNameStr[ExceptionType];
}

/**
  Helper for displaying a backtrace.

  @param Regs       Pointer to SMODE_TRAP_REGISTERS.
  @param FirstPdb   Pointer to the first symbol file used.
  @param ListImage  If true, only show the full path to symbol file, else
                    show the PC value and its decoded components.
**/
STATIC
VOID
DumpCpuBacktraceHelper (
  IN  SMODE_TRAP_REGISTERS  *Regs,
  IN  CHAR8                 *FirstPdb,
  IN  BOOLEAN               ListImage
  )
{
  UINTN    ImageBase;
  UINTN    PeCoffSizeOfHeader;
  BOOLEAN  IsLeaf;
  UINTN    RootFp;
  UINTN    RootRa;
  UINTN    Fp;
  UINTN    Ra;
  UINTN    Idx;
  CHAR8    *Pdb;
  CHAR8    *PrevPdb;

  RootRa = Regs->ra;
  RootFp = Regs->s0;

  Idx     = 0;
  IsLeaf  = TRUE;
  Fp      = RootFp;
  Ra      = RootRa;
  PrevPdb = FirstPdb;
  while (Fp != 0) {
    Pdb = GetImageName (Ra, &ImageBase, &PeCoffSizeOfHeader);
    if (Pdb != NULL) {
      if (Pdb != PrevPdb) {
        Idx++;
        if (ListImage) {
          DEBUG ((DEBUG_ERROR, "[% 2d] %a\n", Idx, Pdb));
        }

        PrevPdb = Pdb;
      }

      if (!ListImage) {
        DEBUG ((
          DEBUG_ERROR,
          "PC 0x%012lx (0x%012lx+0x%08x) [% 2d] %a\n",
          Ra,
          ImageBase,
          Ra - ImageBase,
          Idx,
          BaseName (Pdb)
          ));
      }
    } else if (!ListImage) {
      DEBUG ((DEBUG_ERROR, "PC 0x%012lx\n", Ra));
    }

    /*
     * After the prologue, the frame pointer register s0 will point
     * to the Canonical Frame Address or CFA, which is the stack
     * pointer value on entry to the current procedure. The previous
     * frame pointer and return address pair will reside just prior
     * to the current stack address held in s0. This puts the return
     * address at s0 - XLEN/8, and the previous frame pointer at
     * s0 - 2 * XLEN/8.
     */
    Fp -= sizeof (UINTN) * 2;
    Ra  = *(UINTN *)(Fp + sizeof (UINTN));
    Fp  = *(UINTN *)(Fp);
    if (IsLeaf) {
      if (Ra != RootRa) {
        /* We hit function where ra is not saved on the stack */
        Fp = Ra;
        Ra = RootRa;
      }

      IsLeaf = FALSE;
    }
  }
}

/**
  Display a backtrace.

  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
STATIC
VOID
EFIAPI
DumpCpuBacktrace (
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  SMODE_TRAP_REGISTERS  *Regs;
  CHAR8                 *Pdb;
  UINTN                 ImageBase;
  UINTN                 PeCoffSizeOfHeader;

  Regs = (SMODE_TRAP_REGISTERS *)SystemContext.SystemContextRiscV64;
  Pdb  = GetImageName (Regs->sepc, &ImageBase, &PeCoffSizeOfHeader);
  if (Pdb != NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "PC 0x%012lx (0x%012lx+0x%08x) [ 0] %a\n",
      Regs->sepc,
      ImageBase,
      Regs->sepc - ImageBase,
      BaseName (Pdb)
      ));
  } else {
    DEBUG ((DEBUG_ERROR, "PC 0x%012lx\n", Regs->sepc));
  }

  DumpCpuBacktraceHelper (Regs, Pdb, FALSE);

  if (Pdb != NULL) {
    DEBUG ((DEBUG_ERROR, "\n[ 0] %a\n", Pdb));
  }

  DumpCpuBacktraceHelper (Regs, Pdb, TRUE);
}

/**
  Display CPU information. This can be called by 3rd-party handlers
  set by RegisterCpuInterruptHandler.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
DumpCpuContext (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINTN                 Printed;
  SMODE_TRAP_REGISTERS  *Regs;

  Printed = 0;
  Regs    = (SMODE_TRAP_REGISTERS *)SystemContext.SystemContextRiscV64;

  if (mRecursiveException) {
    InternalPrintMessage ("\nRecursive exception occurred while dumping the CPU state\n");

    CpuDeadLoop ();
  }

  mRecursiveException = TRUE;

  InternalPrintMessage (
    "!!!! RISCV64 Exception Type - %016x(%a) !!!!\n",
    ExceptionType,
    GetExceptionNameStr (ExceptionType)
    );

  DEBUG_CODE_BEGIN ();

  #define REGS()                                                          \
  REG (t0); REG (t1); REG (t2); REG (t3); REG (t4); REG (t5); REG (t6); \
  REG (s0); REG (s1); REG (s2); REG (s3); REG (s4); REG (s5); REG (s6); \
  REG (s7); REG (s8); REG (s9); REG (s10); REG (s11);                   \
  REG (a0); REG (a1); REG (a2); REG (a3); REG (a4); REG (a5); REG (a6); \
  REG (a7);                                                             \
  REG (zero); REG (ra); REG (sp); REG (gp); REG (tp);                   \
  REG (sepc); REG (sstatus); REG (stval);

  #define REG(x)  do {                                      \
    InternalPrintMessage ("%7a = 0x%017lx%c", #x, Regs->x,  \
                          (++Printed % 2) ? L'\t' : L'\n'); \
  } while (0);

  REGS ();
  if (Printed % 2 != 0) {
    InternalPrintMessage ("\n");
  }

  #undef REG
  #undef REGS

  DumpCpuBacktrace (SystemContext);

  DEBUG_CODE_END ();

  mRecursiveException = FALSE;
}

/**
  Initializes all CPU exceptions entries and provides the default exception handlers.

  Caller should try to get an array of interrupt and/or exception vectors that are in use and need to
  persist by EFI_VECTOR_HANDOFF_INFO defined in PI 1.3 specification.
  If caller cannot get reserved vector list or it does not exists, set VectorInfo to NULL.
  If VectorInfo is not NULL, the exception vectors will be initialized per vector attribute accordingly.

  @param[in]  VectorInfo    Pointer to reserved vector list.

  @retval EFI_SUCCESS           CPU Exception Entries have been successfully initialized
                                with default exception handlers.
  @retval EFI_INVALID_PARAMETER VectorInfo includes the invalid content if VectorInfo is not NULL.
  @retval EFI_UNSUPPORTED       This function is not supported.

**/
EFI_STATUS
EFIAPI
InitializeCpuExceptionHandlers (
  IN EFI_VECTOR_HANDOFF_INFO  *VectorInfo OPTIONAL
  )
{
  RiscVSetSupervisorStvec ((UINT64)SupervisorModeTrap);
  return EFI_SUCCESS;
}

/**
  Registers a function to be called from the processor interrupt handler.

  This function registers and enables the handler specified by InterruptHandler for a processor
  interrupt or exception type specified by ExceptionType. If InterruptHandler is NULL, then the
  handler for the processor interrupt or exception type specified by ExceptionType is uninstalled.
  The installed handler is called once for each processor interrupt or exception.
  NOTE: This function should be invoked after InitializeCpuExceptionHandlers() or
  InitializeCpuInterruptHandlers() invoked, otherwise EFI_UNSUPPORTED returned.

  @param[in]  ExceptionType     Defines which interrupt or exception to hook.
  @param[in]  InterruptHandler  A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
                                when a processor interrupt occurs. If this parameter is NULL, then the handler
                                will be uninstalled.

  @retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for ExceptionType was
                                previously installed.
  @retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for ExceptionType was not
                                previously installed.
  @retval EFI_UNSUPPORTED       The interrupt specified by ExceptionType is not supported,
                                or this function is not supported.
**/
EFI_STATUS
EFIAPI
RegisterCpuInterruptHandler (
  IN EFI_EXCEPTION_TYPE         ExceptionType,
  IN EFI_CPU_INTERRUPT_HANDLER  InterruptHandler
  )
{
  DEBUG ((DEBUG_INFO, "%a: Type:%x Handler: %x\n", __func__, ExceptionType, InterruptHandler));
  if (EXCEPT_RISCV_IS_IRQ (ExceptionType)) {
    if (EXCEPT_RISCV_IRQ_INDEX (ExceptionType) > EXCEPT_RISCV_MAX_IRQS) {
      return EFI_UNSUPPORTED;
    }

    if (mIrqHandlers[EXCEPT_RISCV_IRQ_INDEX (ExceptionType)] != NULL) {
      return EFI_ALREADY_STARTED;
    } else if (InterruptHandler == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    mIrqHandlers[EXCEPT_RISCV_IRQ_INDEX (ExceptionType)] = InterruptHandler;
  } else {
    if (ExceptionType > EXCEPT_RISCV_MAX_EXCEPTIONS) {
      return EFI_UNSUPPORTED;
    }

    if (mExceptionHandlers[ExceptionType] != NULL) {
      return EFI_ALREADY_STARTED;
    } else if (InterruptHandler == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    mExceptionHandlers[ExceptionType] = InterruptHandler;
  }

  return EFI_SUCCESS;
}

/**
  Setup separate stacks for certain exception handlers.
  If the input Buffer and BufferSize are both NULL, use global variable if possible.

  @param[in]       Buffer        Point to buffer used to separate exception stack.
  @param[in, out]  BufferSize    On input, it indicates the byte size of Buffer.
                                 If the size is not enough, the return status will
                                 be EFI_BUFFER_TOO_SMALL, and output BufferSize
                                 will be the size it needs.

  @retval EFI_SUCCESS             The stacks are assigned successfully.
  @retval EFI_UNSUPPORTED         This function is not supported.
  @retval EFI_BUFFER_TOO_SMALL    This BufferSize is too small.
**/
EFI_STATUS
EFIAPI
InitializeSeparateExceptionStacks (
  IN     VOID   *Buffer,
  IN OUT UINTN  *BufferSize
  )
{
  return EFI_SUCCESS;
}

/**
  Supervisor mode trap handler.

  @param[in]  SmodeTrapReg     Registers before trap occurred.

**/
VOID
RiscVSupervisorModeTrapHandler (
  SMODE_TRAP_REGISTERS  *SmodeTrapReg
  )
{
  EFI_EXCEPTION_TYPE  ExceptionType;
  EFI_SYSTEM_CONTEXT  RiscVSystemContext;
  UINTN               IrqIndex;

  RiscVSystemContext.SystemContextRiscV64 = (EFI_SYSTEM_CONTEXT_RISCV64 *)SmodeTrapReg;
  ExceptionType                           = (UINTN)RiscVGetSupervisorTrapCause ();

  if (EXCEPT_RISCV_IS_IRQ (ExceptionType)) {
    IrqIndex = EXCEPT_RISCV_IRQ_INDEX (ExceptionType);

    if ((IrqIndex <= EXCEPT_RISCV_MAX_IRQS) &&
        (mIrqHandlers[IrqIndex] != NULL))
    {
      mIrqHandlers[IrqIndex](ExceptionType, RiscVSystemContext);
      return;
    }
  } else {
    if ((ExceptionType <= EXCEPT_RISCV_MAX_EXCEPTIONS) &&
        (mExceptionHandlers[ExceptionType] != 0))
    {
      mExceptionHandlers[ExceptionType](ExceptionType, RiscVSystemContext);
      return;
    }
  }

  DumpCpuContext (ExceptionType, RiscVSystemContext);
  CpuDeadLoop ();
}

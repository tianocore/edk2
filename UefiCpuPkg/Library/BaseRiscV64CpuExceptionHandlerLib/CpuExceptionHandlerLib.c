/** @file
  RISC-V Exception Handler library implementation.

  Copyright (c) 2016 - 2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PrintLib.h>
#include <Register/RiscV64/RiscVEncoding.h>
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

  DEBUG_CODE_END ();
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

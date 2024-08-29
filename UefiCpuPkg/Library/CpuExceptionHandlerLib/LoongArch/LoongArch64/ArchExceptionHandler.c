/** @file ArchExceptionHandler.c

  LoongArch64 CPU Exception Handler.

  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Register/LoongArch64/Csr.h>
#include "ExceptionCommon.h"

/**
  Get Exception Type

  @param[in] SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

  @return    LoongArch64 exception type.

**/
EFI_EXCEPTION_TYPE
EFIAPI
GetExceptionType (
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  EFI_EXCEPTION_TYPE  ExceptionType;

  ExceptionType = (SystemContext.SystemContextLoongArch64->ESTAT & CSR_ESTAT_EXC);
  return ExceptionType;
}

/**
  Get Interrupt Type

  @param[in] SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

  @return    LoongArch64 intrrupt type.

**/
EFI_EXCEPTION_TYPE
EFIAPI
GetInterruptType (
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  EFI_EXCEPTION_TYPE  InterruptType;

  for (InterruptType = 0; InterruptType <= EXCEPT_LOONGARCH_INT_IPI; InterruptType++) {
    if (SystemContext.SystemContextLoongArch64->ESTAT & (1 << InterruptType)) {
      //
      // 0  - EXCEPT_LOONGARCH_INT_SIP0
      // 1  - EXCEPT_LOONGARCH_INT_SIP1
      // 2  - EXCEPT_LOONGARCH_INT_IP0
      // 3  - EXCEPT_LOONGARCH_INT_IP1
      // 4  - EXCEPT_LOONGARCH_INT_IP2
      // 5  - EXCEPT_LOONGARCH_INT_IP3
      // 6  - EXCEPT_LOONGARCH_INT_IP4
      // 7  - EXCEPT_LOONGARCH_INT_IP5
      // 8  - EXCEPT_LOONGARCH_INT_IP6
      // 9  - EXCEPT_LOONGARCH_INT_IP7
      // 10 - EXCEPT_LOONGARCH_INT_PMC
      // 11 - EXCEPT_LOONGARCH_INT_TIMER
      // 12 - EXCEPT_LOONGARCH_INT_IPI
      // Greater than EXCEPT_LOONGARCH_INI_IPI is currently invalid.
      //
      return InterruptType;
    }
  }

  //
  // Invalid IRQ
  //
  return 0xFF;
}

/**
  Display CPU information.

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
  InternalPrintMessage (
    "\n!!!! LoongArch64 Exception Type - %02x(%a) !!!!\n",
    ExceptionType,
    GetExceptionNameStr (ExceptionType)
    );

  //
  // Dump TLB refill ERA and BADV
  //
  if (ExceptionType == (mExceptionKnownNameNum - 1)) {
    InternalPrintMessage ("TLB refill ERA  0x%llx\n", (CsrRead (LOONGARCH_CSR_TLBRERA) & (~0x3ULL)));
    InternalPrintMessage ("TLB refill BADV  0x%llx\n", CsrRead (LOONGARCH_CSR_TLBRBADV));
  }

  //
  // Dump the general registers
  //
  InternalPrintMessage (
    "Zero  - 0x%016lx, RA  - 0x%016lx, TP - 0x%016lx, SP - 0x%016lx\n",
    SystemContext.SystemContextLoongArch64->R0,
    SystemContext.SystemContextLoongArch64->R1,
    SystemContext.SystemContextLoongArch64->R2,
    SystemContext.SystemContextLoongArch64->R3
    );
  InternalPrintMessage (
    "  A0  - 0x%016lx, A1  - 0x%016lx, A2 - 0x%016lx, A3 - 0x%016lx\n",
    SystemContext.SystemContextLoongArch64->R4,
    SystemContext.SystemContextLoongArch64->R5,
    SystemContext.SystemContextLoongArch64->R6,
    SystemContext.SystemContextLoongArch64->R7
    );
  InternalPrintMessage (
    "  A4  - 0x%016lx, A5  - 0x%016lx, A6 - 0x%016lx, A7 - 0x%016lx\n",
    SystemContext.SystemContextLoongArch64->R8,
    SystemContext.SystemContextLoongArch64->R9,
    SystemContext.SystemContextLoongArch64->R10,
    SystemContext.SystemContextLoongArch64->R11
    );
  InternalPrintMessage (
    "  T0  - 0x%016lx, T1  - 0x%016lx, T2 - 0x%016lx, T3 - 0x%016lx\n",
    SystemContext.SystemContextLoongArch64->R12,
    SystemContext.SystemContextLoongArch64->R13,
    SystemContext.SystemContextLoongArch64->R14,
    SystemContext.SystemContextLoongArch64->R15
    );
  InternalPrintMessage (
    "  T4  - 0x%016lx, T5  - 0x%016lx, T6 - 0x%016lx, T7 - 0x%016lx\n",
    SystemContext.SystemContextLoongArch64->R16,
    SystemContext.SystemContextLoongArch64->R17,
    SystemContext.SystemContextLoongArch64->R18,
    SystemContext.SystemContextLoongArch64->R19
    );
  InternalPrintMessage (
    "  T8  - 0x%016lx, R21 - 0x%016lx, FP - 0x%016lx, S0 - 0x%016lx\n",
    SystemContext.SystemContextLoongArch64->R20,
    SystemContext.SystemContextLoongArch64->R21,
    SystemContext.SystemContextLoongArch64->R22,
    SystemContext.SystemContextLoongArch64->R23
    );
  InternalPrintMessage (
    "  S1  - 0x%016lx, S2  - 0x%016lx, S3 - 0x%016lx, S4 - 0x%016lx\n",
    SystemContext.SystemContextLoongArch64->R24,
    SystemContext.SystemContextLoongArch64->R25,
    SystemContext.SystemContextLoongArch64->R26,
    SystemContext.SystemContextLoongArch64->R27
    );
  InternalPrintMessage (
    "  S5  - 0x%016lx, S6  - 0x%016lx, S7 - 0x%016lx, S8 - 0x%016lx\n",
    SystemContext.SystemContextLoongArch64->R28,
    SystemContext.SystemContextLoongArch64->R29,
    SystemContext.SystemContextLoongArch64->R30,
    SystemContext.SystemContextLoongArch64->R31
    );
  InternalPrintMessage ("\n");

  //
  // Dump the CSR registers
  //
  InternalPrintMessage (
    "CRMD  - 0x%016lx, PRMD  - 0x%016lx, EUEN - 0x%016lx, MISC - 0x%016lx\n",
    SystemContext.SystemContextLoongArch64->CRMD,
    SystemContext.SystemContextLoongArch64->PRMD,
    SystemContext.SystemContextLoongArch64->EUEN,
    SystemContext.SystemContextLoongArch64->MISC
    );
  InternalPrintMessage (
    "ECFG  - 0x%016lx, ESTAT - 0x%016lx, ERA  - 0x%016lx, BADV - 0x%016lx\n",
    SystemContext.SystemContextLoongArch64->ECFG,
    SystemContext.SystemContextLoongArch64->ESTAT,
    SystemContext.SystemContextLoongArch64->ERA,
    SystemContext.SystemContextLoongArch64->BADV
    );
  InternalPrintMessage (
    "BADI  - 0x%016lx\n",
    SystemContext.SystemContextLoongArch64->BADI
    );
}

/**
  Display CPU information.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
DumpImageAndCpuContent (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  DumpCpuContext (ExceptionType, SystemContext);

  if (ExceptionType == (mExceptionKnownNameNum - 1)) {
    //
    // Dump TLB refill image info
    //
    DumpModuleImageInfo ((CsrRead (LOONGARCH_CSR_TLBRERA) & (~0x3ULL)));
  } else {
    DumpModuleImageInfo (SystemContext.SystemContextLoongArch64->ERA);
  }
}

/**
  IPI Interrupt Handler.

  @param InterruptType    The type of interrupt that occurred
  @param SystemContext    A pointer to the system context when the interrupt occurred
**/
VOID
EFIAPI
IpiInterruptHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINTN  ResumeVector;
  UINTN  Parameter;

  //
  // Clear interrupt.
  //
  IoCsrWrite32 (LOONGARCH_IOCSR_IPI_CLEAR, IoCsrRead32 (LOONGARCH_IOCSR_IPI_STATUS));

  //
  // Get the resume vector and parameter if populated.
  //
  ResumeVector = IoCsrRead64 (LOONGARCH_IOCSR_MBUF0);
  Parameter    = IoCsrRead64 (LOONGARCH_IOCSR_MBUF3);

  //
  // Clean up current processor mailbox 0 and mailbox 3.
  //
  IoCsrWrite64 (LOONGARCH_IOCSR_MBUF0, 0x0);
  IoCsrWrite64 (LOONGARCH_IOCSR_MBUF3, 0x0);

  //
  // If mailbox 0 is non-NULL, it means that the BSP or other cores called the IPI to wake
  // up the current core and let it use the resume vector stored in mailbox 0.
  //
  // If both the resume vector and parameter are non-NULL, it means that the IPI was
  // called in the BIOS.
  //
  // The situation where the resume vector is non-NULL and the parameter is NULL has been
  // processed after the exception entry is pushed onto the stack.
  //
  if ((ResumeVector != 0) && (Parameter != 0)) {
    SystemContext.SystemContextLoongArch64->ERA = ResumeVector;
    //
    // Set $a0 as APIC ID and $a1 as parameter value.
    //
    SystemContext.SystemContextLoongArch64->R4 = CsrRead (LOONGARCH_CSR_CPUID);
    SystemContext.SystemContextLoongArch64->R5 = Parameter;
  }

  MemoryFence ();
}

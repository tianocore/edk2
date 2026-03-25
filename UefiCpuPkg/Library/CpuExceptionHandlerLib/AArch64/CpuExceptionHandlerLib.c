/** @file
  AARCH64 CpuExceptionHandlerLib Implementation

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011-2021, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2016 HP Development Company, L.P.
  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/ArmLib.h>
#include <Library/PcdLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Protocol/DebugSupport.h> // for MAX_AARCH64_EXCEPTION
#include <Library/SerialPortLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <ExceptionSupport.h>
#include <CpuExceptionSupport.h>

UINTN                   gMaxExceptionNumber                           = MAX_AARCH64_EXCEPTION;
EFI_EXCEPTION_CALLBACK  gExceptionHandlers[MAX_AARCH64_EXCEPTION + 1] = { 0 };
PHYSICAL_ADDRESS        gExceptionVectorAlignmentMask                 = ARM_VECTOR_TABLE_ALIGNMENT;

//
// Maximum number of characters to print to serial (UINT8s) and to console if
// available (as UINT16s)
//
#define MAX_PRINT_CHARS  100

STATIC CHAR8  *gExceptionTypeString[] = {
  "Synchronous",
  "IRQ",
  "FIQ",
  "SError"
};

#define EL0_STACK_SIZE  EFI_PAGES_TO_SIZE(2)
STATIC UINTN    mNewStackBase[EL0_STACK_SIZE / sizeof (UINTN)];
STATIC BOOLEAN  mRecursiveException;

//
// Static Functions
//

/**
  Configures the architecture-specific exception vector and stack settings.

  Aligns the EL0 stack pointer to a 16-byte boundary and registers it via
  RegisterEl0Stack(). If the processor is executing at EL2, the Hypervisor
  Configuration Register (HCR) is updated to set the Trap General Exceptions
  (TGE) bit, routing all EL1-destined exceptions to EL2.

  @param[in]  VectorBaseAddress   Base address of the exception vector table.

  @retval RETURN_SUCCESS          Architecture vector configuration completed successfully.

**/
STATIC RETURN_STATUS
ArchVectorConfig (
  IN  UINTN  VectorBaseAddress
  )
{
  UINTN  HcrReg;

  // Round down sp by 16 bytes alignment
  RegisterEl0Stack (
    (VOID *)(((UINTN)mNewStackBase + EL0_STACK_SIZE) & ~0xFULL)
    );

  if (ArmReadCurrentEL () == AARCH64_EL2) {
    HcrReg = ArmReadHcr ();

    // Trap General Exceptions. All exceptions that would be routed to EL1 are routed to EL2
    HcrReg |= ARM_HCR_TGE;

    ArmWriteHcr (HcrReg);
  }

  return RETURN_SUCCESS;
}

/**
  Decodes and logs the cause of an instruction or data abort exception.

  Extracts the DFSC/IFSC (Data/Instruction Fault Status Code) from the lower
  6 bits of the ISS field and maps it to a human-readable description. The
  decoded abort type and cause are emitted via DEBUG().

  @param[in]  AbortType   Pointer to a null-terminated ASCII string identifying
                          the abort type (e.g., "Instruction abort" or
                          "Data abort").
  @param[in]  Iss         The Instruction Specific Syndrome (ISS) field extracted
                          from the Exception Syndrome Register (ESR_ELx).

**/
STATIC
VOID
DescribeInstructionOrDataAbort (
  IN CHAR8  *AbortType,
  IN UINTN  Iss
  )
{
  CHAR8  *AbortCause;

  switch (Iss & 0x3f) {
    case 0x0: AbortCause = "Address size fault, zeroth level of translation or translation table base register";
      break;
    case 0x1: AbortCause = "Address size fault, first level";
      break;
    case 0x2: AbortCause = "Address size fault, second level";
      break;
    case 0x3: AbortCause = "Address size fault, third level";
      break;
    case 0x4: AbortCause = "Translation fault, zeroth level";
      break;
    case 0x5: AbortCause = "Translation fault, first level";
      break;
    case 0x6: AbortCause = "Translation fault, second level";
      break;
    case 0x7: AbortCause = "Translation fault, third level";
      break;
    case 0x9: AbortCause = "Access flag fault, first level";
      break;
    case 0xa: AbortCause = "Access flag fault, second level";
      break;
    case 0xb: AbortCause = "Access flag fault, third level";
      break;
    case 0xd: AbortCause = "Permission fault, first level";
      break;
    case 0xe: AbortCause = "Permission fault, second level";
      break;
    case 0xf: AbortCause = "Permission fault, third level";
      break;
    case 0x10: AbortCause = "Synchronous external abort";
      break;
    case 0x18: AbortCause = "Synchronous parity error on memory access";
      break;
    case 0x11: AbortCause = "Asynchronous external abort";
      break;
    case 0x19: AbortCause = "Asynchronous parity error on memory access";
      break;
    case 0x14: AbortCause = "Synchronous external abort on translation table walk, zeroth level";
      break;
    case 0x15: AbortCause = "Synchronous external abort on translation table walk, first level";
      break;
    case 0x16: AbortCause = "Synchronous external abort on translation table walk, second level";
      break;
    case 0x17: AbortCause = "Synchronous external abort on translation table walk, third level";
      break;
    case 0x1c: AbortCause = "Synchronous parity error on memory access on translation table walk, zeroth level";
      break;
    case 0x1d: AbortCause = "Synchronous parity error on memory access on translation table walk, first level";
      break;
    case 0x1e: AbortCause = "Synchronous parity error on memory access on translation table walk, second level";
      break;
    case 0x1f: AbortCause = "Synchronous parity error on memory access on translation table walk, third level";
      break;
    case 0x21: AbortCause = "Alignment fault";
      break;
    case 0x22: AbortCause = "Debug event";
      break;
    case 0x30: AbortCause = "TLB conflict abort";
      break;
    case 0x33:
    case 0x34: AbortCause = "IMPLEMENTATION DEFINED";
      break;
    case 0x35:
    case 0x36: AbortCause = "Domain fault";
      break;
    default: AbortCause = "";
      break;
  }

  DEBUG ((DEBUG_ERROR, "\n%a: %a\n", AbortType, AbortCause));
}

/**
  Decodes and logs a description of an AArch64 exception syndrome.

  Extracts the Exception Class (EC) and Instruction Specific Syndrome (ISS)
  fields from the Exception Syndrome Register (ESR) value. Based on the EC,
  a human-readable description of the exception is emitted via DEBUG(). For
  instruction and data aborts, further decoding is delegated to
  DescribeInstructionOrDataAbort().

  Unrecognized EC values are silently ignored.

  @param[in]  Esr   The 64-bit value of the Exception Syndrome Register (ESR_ELx).

**/
STATIC
VOID
DescribeExceptionSyndrome (
  IN UINT64  Esr
  )
{
  CHAR8  *Message;
  UINTN  Ec;
  UINTN  Iss;

  Ec  = Esr >> 26;
  Iss = Esr & 0x00ffffff;

  switch (Ec) {
    case 0x15: Message = "SVC executed in AArch64";
      break;
    case 0x20:
    case 0x21: DescribeInstructionOrDataAbort ("Instruction abort", Iss);
      return;
    case 0x22: Message = "PC alignment fault";
      break;
    case 0x23: Message = "SP alignment fault";
      break;
    case 0x24:
    case 0x25: DescribeInstructionOrDataAbort ("Data abort", Iss);
      return;
    default: return;
  }

  DEBUG ((DEBUG_ERROR, "\n %a \n", Message));
}

/**
  Returns a pointer to the base filename component of a full file path.

  Scans the provided path string in reverse to locate the last occurrence of
  a path separator ('/' or '\\'). If found, returns a pointer to the character
  immediately following the separator. If no separator is found, returns a
  pointer to the beginning of the string.

  @param[in]  FullName    Pointer to a null-terminated ASCII string containing
                          the full file path.

  @retval     Pointer to the null-terminated ASCII string of the base filename.

**/
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
  Common C handler for CPU exceptions.

  Dispatches the exception to a registered handler in gExceptionHandlers[]
  if one exists for the given ExceptionType. If no registered handler is
  found, the CPU context is dumped via DumpCpuContext(). If the ExceptionType
  exceeds gMaxExceptionNumber, an error is logged and an assertion is raised.

  @param[in]      ExceptionType   The exception type as defined by
                                  EFI_EXCEPTION_TYPE.
  @param[in,out]  SystemContext   Pointer to the system context at the time
                                  of the exception, which may be modified by
                                  the registered handler.

**/
VOID
EFIAPI
CommonCExceptionHandler (
  IN     EFI_EXCEPTION_TYPE  ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  if ((UINTN)ExceptionType <= gMaxExceptionNumber) {
    if (gExceptionHandlers[ExceptionType] != NULL) {
      gExceptionHandlers[ExceptionType](ExceptionType, SystemContext);
      return;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Unknown exception type %d\n", ExceptionType));
    ASSERT (FALSE);
  }

  DumpCpuContext (ExceptionType, SystemContext);
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
  UINT64  VectorBase;

  // use VBAR to point to where our exception handlers are

  // The vector table must be aligned for the architecture.  If this
  // assertion fails ensure the appropriate FFS alignment is in effect,
  // which can be accomplished by ensuring the proper Align=X statement
  // in the platform packaging rules.  For AArch64 Align=4K is required.
  // Align=Auto can be used but this is known to cause an issue with
  // populating the reset vector area for encapsulated FVs.
  ASSERT (((UINTN)ExceptionHandlersStart & gExceptionVectorAlignmentMask) == 0);

  VectorBase = (UINT64)(UINTN)ExceptionHandlersStart;

  // call the architecture-specific routine to prepare for the new vector
  // configuration to take effect
  ArchVectorConfig ((UINTN)VectorBase);

  ArmWriteVBar ((UINTN)VectorBase);

  return RETURN_SUCCESS;
}

/**
Registers a function to be called from the processor exception handler. (On AArch64 this only
provides exception handlers, not interrupt handling which is provided through the Hardware Interrupt
Protocol.)

This function registers and enables the handler specified by ExceptionHandler for a processor
interrupt or exception type specified by ExceptionType. If ExceptionHandler is NULL, then the
handler for the processor interrupt or exception type specified by ExceptionType is uninstalled.
The installed handler is called once for each processor interrupt or exception.
NOTE: This function should be invoked after InitializeCpuExceptionHandlers() is invoked,
otherwise EFI_UNSUPPORTED returned.

@param[in]  ExceptionType     Defines which interrupt or exception to hook.
@param[in]  ExceptionHandler  A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
when a processor interrupt occurs. If this parameter is NULL, then the handler
will be uninstalled.

@retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
@retval EFI_ALREADY_STARTED   ExceptionHandler is not NULL, and a handler for ExceptionType was
previously installed.
@retval EFI_INVALID_PARAMETER ExceptionHandler is NULL, and a handler for ExceptionType was not
previously installed.
@retval EFI_UNSUPPORTED       The interrupt specified by ExceptionType is not supported,
or this function is not supported.
**/
RETURN_STATUS
RegisterCpuInterruptHandler (
  IN EFI_EXCEPTION_TYPE         ExceptionType,
  IN EFI_CPU_INTERRUPT_HANDLER  ExceptionHandler
  )
{
  if ((UINTN)ExceptionType > gMaxExceptionNumber) {
    return RETURN_UNSUPPORTED;
  }

  if ((ExceptionHandler != NULL) && (gExceptionHandlers[ExceptionType] != NULL)) {
    return RETURN_ALREADY_STARTED;
  }

  gExceptionHandlers[ExceptionType] = ExceptionHandler;

  return RETURN_SUCCESS;
}

/**
  This is the default action to take on an unexpected exception

  Since this is exception context don't do anything crazy like try to allocate memory.

  @param  ExceptionType    Type of the exception
  @param  SystemContext    Register state at the time of the Exception

**/
VOID
DumpCpuContext (
  IN     EFI_EXCEPTION_TYPE  ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  CHAR8   Buffer[MAX_PRINT_CHARS];
  CHAR16  UnicodeBuffer[MAX_PRINT_CHARS];
  UINTN   CharCount;
  INT32   Offset;

  if (mRecursiveException) {
    STATIC CHAR8 CONST  Message[] = "\nRecursive exception occurred while dumping the CPU state\n";
    SerialPortWrite ((UINT8 *)Message, sizeof Message - 1);
    CpuDeadLoop ();
  }

  mRecursiveException = TRUE;

  CharCount = AsciiSPrint (Buffer, sizeof (Buffer), "\n\n%a Exception at 0x%016lx\n", gExceptionTypeString[ExceptionType], SystemContext.SystemContextAArch64->ELR);
  SerialPortWrite ((UINT8 *)Buffer, CharCount);

  // Prepare a unicode buffer for ConOut, if applicable, in case the buffer
  // gets reused.
  UnicodeSPrintAsciiFormat (UnicodeBuffer, MAX_PRINT_CHARS, Buffer);

  DEBUG_CODE_BEGIN ();
  CHAR8   *Pdb, *PrevPdb;
  UINTN   ImageBase;
  UINTN   PeCoffSizeOfHeader;
  UINT64  *Fp;
  UINT64  RootFp[2];
  UINTN   Idx;

  PrevPdb = Pdb = GetImageName (SystemContext.SystemContextAArch64->ELR, &ImageBase, &PeCoffSizeOfHeader);
  if (Pdb != NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "PC 0x%012lx (0x%012lx+0x%08x) [ 0] %a\n",
      SystemContext.SystemContextAArch64->ELR,
      ImageBase,
      SystemContext.SystemContextAArch64->ELR - ImageBase,
      BaseName (Pdb)
      ));
  } else {
    DEBUG ((DEBUG_ERROR, "PC 0x%012lx\n", SystemContext.SystemContextAArch64->ELR));
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

        DEBUG ((
          DEBUG_ERROR,
          "PC 0x%012lx (0x%012lx+0x%08x) [% 2d] %a\n",
          Fp[1],
          ImageBase,
          Fp[1] - ImageBase,
          Idx,
          BaseName (Pdb)
          ));
      } else {
        DEBUG ((DEBUG_ERROR, "PC 0x%012lx\n", Fp[1]));
      }
    }

    PrevPdb = Pdb = GetImageName (SystemContext.SystemContextAArch64->ELR, &ImageBase, &PeCoffSizeOfHeader);
    if (Pdb != NULL) {
      DEBUG ((DEBUG_ERROR, "\n[ 0] %a\n", Pdb));
    }

    Idx = 0;
    for (Fp = RootFp; Fp[0] != 0; Fp = (UINT64 *)Fp[0]) {
      Pdb = GetImageName (Fp[1], &ImageBase, &PeCoffSizeOfHeader);
      if ((Pdb != NULL) && (Pdb != PrevPdb)) {
        DEBUG ((DEBUG_ERROR, "[% 2d] %a\n", ++Idx, Pdb));
        PrevPdb = Pdb;
      }
    }
  }

  DEBUG_CODE_END ();

  DEBUG ((DEBUG_ERROR, "\n  X0 0x%016lx   X1 0x%016lx   X2 0x%016lx   X3 0x%016lx\n", SystemContext.SystemContextAArch64->X0, SystemContext.SystemContextAArch64->X1, SystemContext.SystemContextAArch64->X2, SystemContext.SystemContextAArch64->X3));
  DEBUG ((DEBUG_ERROR, "  X4 0x%016lx   X5 0x%016lx   X6 0x%016lx   X7 0x%016lx\n", SystemContext.SystemContextAArch64->X4, SystemContext.SystemContextAArch64->X5, SystemContext.SystemContextAArch64->X6, SystemContext.SystemContextAArch64->X7));
  DEBUG ((DEBUG_ERROR, "  X8 0x%016lx   X9 0x%016lx  X10 0x%016lx  X11 0x%016lx\n", SystemContext.SystemContextAArch64->X8, SystemContext.SystemContextAArch64->X9, SystemContext.SystemContextAArch64->X10, SystemContext.SystemContextAArch64->X11));
  DEBUG ((DEBUG_ERROR, " X12 0x%016lx  X13 0x%016lx  X14 0x%016lx  X15 0x%016lx\n", SystemContext.SystemContextAArch64->X12, SystemContext.SystemContextAArch64->X13, SystemContext.SystemContextAArch64->X14, SystemContext.SystemContextAArch64->X15));
  DEBUG ((DEBUG_ERROR, " X16 0x%016lx  X17 0x%016lx  X18 0x%016lx  X19 0x%016lx\n", SystemContext.SystemContextAArch64->X16, SystemContext.SystemContextAArch64->X17, SystemContext.SystemContextAArch64->X18, SystemContext.SystemContextAArch64->X19));
  DEBUG ((DEBUG_ERROR, " X20 0x%016lx  X21 0x%016lx  X22 0x%016lx  X23 0x%016lx\n", SystemContext.SystemContextAArch64->X20, SystemContext.SystemContextAArch64->X21, SystemContext.SystemContextAArch64->X22, SystemContext.SystemContextAArch64->X23));
  DEBUG ((DEBUG_ERROR, " X24 0x%016lx  X25 0x%016lx  X26 0x%016lx  X27 0x%016lx\n", SystemContext.SystemContextAArch64->X24, SystemContext.SystemContextAArch64->X25, SystemContext.SystemContextAArch64->X26, SystemContext.SystemContextAArch64->X27));
  DEBUG ((DEBUG_ERROR, " X28 0x%016lx   FP 0x%016lx   LR 0x%016lx  \n", SystemContext.SystemContextAArch64->X28, SystemContext.SystemContextAArch64->FP, SystemContext.SystemContextAArch64->LR));

  /* We save these as 128bit numbers, but have to print them as two 64bit numbers,
     so swap the 64bit words to correctly represent a 128bit number.  */
  DEBUG ((DEBUG_ERROR, "\n  V0 0x%016lx %016lx   V1 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V0[1], SystemContext.SystemContextAArch64->V0[0], SystemContext.SystemContextAArch64->V1[1], SystemContext.SystemContextAArch64->V1[0]));
  DEBUG ((DEBUG_ERROR, "  V2 0x%016lx %016lx   V3 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V2[1], SystemContext.SystemContextAArch64->V2[0], SystemContext.SystemContextAArch64->V3[1], SystemContext.SystemContextAArch64->V3[0]));
  DEBUG ((DEBUG_ERROR, "  V4 0x%016lx %016lx   V5 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V4[1], SystemContext.SystemContextAArch64->V4[0], SystemContext.SystemContextAArch64->V5[1], SystemContext.SystemContextAArch64->V5[0]));
  DEBUG ((DEBUG_ERROR, "  V6 0x%016lx %016lx   V7 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V6[1], SystemContext.SystemContextAArch64->V6[0], SystemContext.SystemContextAArch64->V7[1], SystemContext.SystemContextAArch64->V7[0]));
  DEBUG ((DEBUG_ERROR, "  V8 0x%016lx %016lx   V9 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V8[1], SystemContext.SystemContextAArch64->V8[0], SystemContext.SystemContextAArch64->V9[1], SystemContext.SystemContextAArch64->V9[0]));
  DEBUG ((DEBUG_ERROR, " V10 0x%016lx %016lx  V11 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V10[1], SystemContext.SystemContextAArch64->V10[0], SystemContext.SystemContextAArch64->V11[1], SystemContext.SystemContextAArch64->V11[0]));
  DEBUG ((DEBUG_ERROR, " V12 0x%016lx %016lx  V13 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V12[1], SystemContext.SystemContextAArch64->V12[0], SystemContext.SystemContextAArch64->V13[1], SystemContext.SystemContextAArch64->V13[0]));
  DEBUG ((DEBUG_ERROR, " V14 0x%016lx %016lx  V15 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V14[1], SystemContext.SystemContextAArch64->V14[0], SystemContext.SystemContextAArch64->V15[1], SystemContext.SystemContextAArch64->V15[0]));
  DEBUG ((DEBUG_ERROR, " V16 0x%016lx %016lx  V17 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V16[1], SystemContext.SystemContextAArch64->V16[0], SystemContext.SystemContextAArch64->V17[1], SystemContext.SystemContextAArch64->V17[0]));
  DEBUG ((DEBUG_ERROR, " V18 0x%016lx %016lx  V19 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V18[1], SystemContext.SystemContextAArch64->V18[0], SystemContext.SystemContextAArch64->V19[1], SystemContext.SystemContextAArch64->V19[0]));
  DEBUG ((DEBUG_ERROR, " V20 0x%016lx %016lx  V21 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V20[1], SystemContext.SystemContextAArch64->V20[0], SystemContext.SystemContextAArch64->V21[1], SystemContext.SystemContextAArch64->V21[0]));
  DEBUG ((DEBUG_ERROR, " V22 0x%016lx %016lx  V23 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V22[1], SystemContext.SystemContextAArch64->V22[0], SystemContext.SystemContextAArch64->V23[1], SystemContext.SystemContextAArch64->V23[0]));
  DEBUG ((DEBUG_ERROR, " V24 0x%016lx %016lx  V25 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V24[1], SystemContext.SystemContextAArch64->V24[0], SystemContext.SystemContextAArch64->V25[1], SystemContext.SystemContextAArch64->V25[0]));
  DEBUG ((DEBUG_ERROR, " V26 0x%016lx %016lx  V27 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V26[1], SystemContext.SystemContextAArch64->V26[0], SystemContext.SystemContextAArch64->V27[1], SystemContext.SystemContextAArch64->V27[0]));
  DEBUG ((DEBUG_ERROR, " V28 0x%016lx %016lx  V29 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V28[1], SystemContext.SystemContextAArch64->V28[0], SystemContext.SystemContextAArch64->V29[1], SystemContext.SystemContextAArch64->V29[0]));
  DEBUG ((DEBUG_ERROR, " V30 0x%016lx %016lx  V31 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V30[1], SystemContext.SystemContextAArch64->V30[0], SystemContext.SystemContextAArch64->V31[1], SystemContext.SystemContextAArch64->V31[0]));

  DEBUG ((DEBUG_ERROR, "\n  SP 0x%016lx  ELR 0x%016lx  SPSR 0x%08lx  FPSR 0x%08lx\n ESR 0x%08lx          FAR 0x%016lx\n", SystemContext.SystemContextAArch64->SP, SystemContext.SystemContextAArch64->ELR, SystemContext.SystemContextAArch64->SPSR, SystemContext.SystemContextAArch64->FPSR, SystemContext.SystemContextAArch64->ESR, SystemContext.SystemContextAArch64->FAR));

  DEBUG ((DEBUG_ERROR, "\n ESR : EC 0x%02x  IL 0x%x  ISS 0x%08x\n", (SystemContext.SystemContextAArch64->ESR & 0xFC000000) >> 26, (SystemContext.SystemContextAArch64->ESR >> 25) & 0x1, SystemContext.SystemContextAArch64->ESR & 0x1FFFFFF));

  DescribeExceptionSyndrome (SystemContext.SystemContextAArch64->ESR);

  DEBUG ((DEBUG_ERROR, "\nStack dump:\n"));
  for (Offset = -256; Offset < 256; Offset += 32) {
    DEBUG ((
      DEBUG_ERROR,
      "%c %013lx: %016lx %016lx %016lx %016lx\n",
      Offset == 0 ? '>' : ' ',
      SystemContext.SystemContextAArch64->SP + Offset,
      *(UINT64 *)(SystemContext.SystemContextAArch64->SP + Offset),
      *(UINT64 *)(SystemContext.SystemContextAArch64->SP + Offset + 8),
      *(UINT64 *)(SystemContext.SystemContextAArch64->SP + Offset + 16),
      *(UINT64 *)(SystemContext.SystemContextAArch64->SP + Offset + 24)
      ));
  }

  // Attempt to print that we had a synchronous exception to ConOut.  We do
  // this after the serial logging as ConOut's logging is more complex and we
  // aren't guaranteed to succeed.
  LogToConsole (UnicodeBuffer);

  ASSERT (FALSE);
  CpuDeadLoop ();
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

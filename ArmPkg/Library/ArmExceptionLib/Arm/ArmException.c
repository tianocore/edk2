/** @file
*  Exception handling support specific for ARM
*
* Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
* Copyright (c) 2014, ARM Limited. All rights reserved.<BR>
*  Copyright (c) 2015 Hewlett-Packard Company. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/ArmExceptionLib.h>

//#include <Uefi.h>
//#include <Protocol/DebugSupport.h> // for EFI_EXCEPTION_CALLBACK and MAX_AARCH64_EXCEPTION

//FIXME: Will not compile on non-ARMv7 builds
#include <Chipset/ArmV7.h>

#include <Library/PcdLib.h>
#include <Library/ArmLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

VOID
ExceptionHandlersStart(
VOID
);

VOID
ExceptionHandlersEnd(
VOID
);

VOID
CommonExceptionEntry(
VOID
);

VOID
AsmCommonExceptionEntry(
VOID
);

UINTN gMaxExceptionNumber = MAX_ARM_EXCEPTION;

EFI_EXCEPTION_CALLBACK  gExceptionHandlers[MAX_ARM_EXCEPTION + 1];
EFI_EXCEPTION_CALLBACK  gDebuggerExceptionHandlers[MAX_ARM_EXCEPTION + 1];


RETURN_STATUS InstallExceptionHandlers(VOID)
{
  RETURN_STATUS        Status;
  UINTN                Offset;
  UINTN                Length;
  UINTN                Index;
  EFI_PHYSICAL_ADDRESS Base;
  UINT32               *VectorBase;

  //
  // Copy an implementation of the ARM exception vectors to PcdCpuVectorBaseAddress.
  //
  Length = (UINTN)ExceptionHandlersEnd - (UINTN)ExceptionHandlersStart;

  // Check if the exception vector is in the low address
  if (PcdGet32(PcdCpuVectorBaseAddress) == 0x0) {
    // Set SCTLR.V to 0 to enable VBAR to be used
    ArmSetLowVectors();
  }
  else {
    ArmSetHighVectors();
  }

  Base = (EFI_PHYSICAL_ADDRESS)PcdGet32(PcdCpuVectorBaseAddress);
  VectorBase = (UINT32 *)(UINTN)Base;

  if (FeaturePcdGet(PcdDebuggerExceptionSupport) == TRUE) {
    // Save existing vector table, in case debugger is already hooked in
    CopyMem((VOID *)gDebuggerExceptionHandlers, (VOID *)VectorBase, sizeof (gDebuggerExceptionHandlers));
  }

  // Copy our assembly code into the page that contains the exception vectors.
  CopyMem((VOID *)VectorBase, (VOID *)ExceptionHandlersStart, Length);

  //
  // Patch in the common Assembly exception handler
  //
  Offset = (UINTN)CommonExceptionEntry - (UINTN)ExceptionHandlersStart;
  *(UINTN *)((UINT8 *)(UINTN)PcdGet32(PcdCpuVectorBaseAddress) + Offset) = (UINTN)AsmCommonExceptionEntry;

  //
  // Initialize the C entry points for interrupts
  //
  for (Index = 0; Index <= MAX_ARM_EXCEPTION; Index++) {
    if (!FeaturePcdGet(PcdDebuggerExceptionSupport) ||
      (gDebuggerExceptionHandlers[Index] == 0) || (gDebuggerExceptionHandlers[Index] == (VOID *)(UINTN)0xEAFFFFFE)) {
      // Exception handler contains branch to vector location (jmp $) so no handler
      // NOTE: This code assumes vectors are ARM and not Thumb code
      Status = RegisterExceptionHandler(Index, NULL);
      ASSERT_EFI_ERROR(Status);
    }
    else {
      // If the debugger has already hooked put its vector back
      VectorBase[Index] = (UINT32)(UINTN)gDebuggerExceptionHandlers[Index];
    }
  }

  // Flush Caches since we updated executable stuff
  InvalidateInstructionCacheRange((VOID *)PcdGet32(PcdCpuVectorBaseAddress), Length);

  //Note: On ARM processor with the Security Extension, the Vector Table can be located anywhere in the memory.
  //      The Vector Base Address Register defines the location
  ArmWriteVBar(PcdGet32(PcdCpuVectorBaseAddress));

  return RETURN_SUCCESS;
}

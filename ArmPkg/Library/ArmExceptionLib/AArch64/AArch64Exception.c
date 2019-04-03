/** @file
*  Exception Handling support specific for AArch64
*
*  Copyright (c) 2016 HP Development Company, L.P.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>

#include <Chipset/AArch64.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/DebugSupport.h> // for MAX_AARCH64_EXCEPTION

UINTN                   gMaxExceptionNumber = MAX_AARCH64_EXCEPTION;
EFI_EXCEPTION_CALLBACK  gExceptionHandlers[MAX_AARCH64_EXCEPTION + 1] = { 0 };
EFI_EXCEPTION_CALLBACK  gDebuggerExceptionHandlers[MAX_AARCH64_EXCEPTION + 1] = { 0 };
PHYSICAL_ADDRESS        gExceptionVectorAlignmentMask = ARM_VECTOR_TABLE_ALIGNMENT;
UINTN                   gDebuggerNoHandlerValue = 0; // todo: define for AArch64

#define EL0_STACK_PAGES   2

VOID
RegisterEl0Stack (
  IN  VOID    *Stack
  );

RETURN_STATUS ArchVectorConfig(
  IN  UINTN       VectorBaseAddress
  )
{
  UINTN             HcrReg;
  UINT8             *Stack;

  Stack = AllocatePages (EL0_STACK_PAGES);
  if (Stack == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  RegisterEl0Stack ((UINT8 *)Stack + EFI_PAGES_TO_SIZE (EL0_STACK_PAGES));

  if (ArmReadCurrentEL() == AARCH64_EL2) {
    HcrReg = ArmReadHcr();

    // Trap General Exceptions. All exceptions that would be routed to EL1 are routed to EL2
    HcrReg |= ARM_HCR_TGE;

    ArmWriteHcr(HcrReg);
  }

  return RETURN_SUCCESS;
}

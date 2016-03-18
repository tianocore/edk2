/** @file
*  Exception Handling support specific for AArch64
*
*  Copyright (c) 2016 HP Development Company, L.P.
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

#include <Uefi.h>

#include <Chipset/AArch64.h>

#include <Protocol/DebugSupport.h> // for MAX_AARCH64_EXCEPTION

UINTN                   gMaxExceptionNumber = MAX_AARCH64_EXCEPTION;
EFI_EXCEPTION_CALLBACK  gExceptionHandlers[MAX_AARCH64_EXCEPTION + 1] = { 0 };
EFI_EXCEPTION_CALLBACK  gDebuggerExceptionHandlers[MAX_AARCH64_EXCEPTION + 1] = { 0 };
PHYSICAL_ADDRESS        gExceptionVectorAlignmentMask = ARM_VECTOR_TABLE_ALIGNMENT;
UINTN                   gDebuggerNoHandlerValue = 0; // todo: define for AArch64

RETURN_STATUS ArchVectorConfig(
  IN  UINTN       VectorBaseAddress
  )
{
  UINTN             HcrReg;

  if (ArmReadCurrentEL() == AARCH64_EL2) {
    HcrReg = ArmReadHcr();

    // Trap General Exceptions. All exceptions that would be routed to EL1 are routed to EL2
    HcrReg |= ARM_HCR_TGE;

    ArmWriteHcr(HcrReg);
  }

  return RETURN_SUCCESS;
}

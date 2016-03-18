/** @file
*  Exception handling support specific for ARM
*
* Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
* Copyright (c) 2014, ARM Limited. All rights reserved.<BR>
* Copyright (c) 2016 HP Development Company, L.P.<BR>
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

#include <Chipset/ArmV7.h>

#include <Library/ArmLib.h>

#include <Protocol/DebugSupport.h> // for MAX_ARM_EXCEPTION

UINTN                   gMaxExceptionNumber = MAX_ARM_EXCEPTION;
EFI_EXCEPTION_CALLBACK  gExceptionHandlers[MAX_ARM_EXCEPTION + 1] = { 0 };
EFI_EXCEPTION_CALLBACK  gDebuggerExceptionHandlers[MAX_ARM_EXCEPTION + 1] = { 0 };
PHYSICAL_ADDRESS        gExceptionVectorAlignmentMask = ARM_VECTOR_TABLE_ALIGNMENT;

// Exception handler contains branch to vector location (jmp $) so no handler
// NOTE: This code assumes vectors are ARM and not Thumb code
UINTN                   gDebuggerNoHandlerValue = 0xEAFFFFFE;

RETURN_STATUS ArchVectorConfig(
  IN  UINTN       VectorBaseAddress
  )
{
  // if the vector address corresponds to high vectors
  if (VectorBaseAddress == 0xFFFF0000) {
    // set SCTLR.V to enable high vectors
    ArmSetHighVectors();
  }
  else {
    // Set SCTLR.V to 0 to enable VBAR to be used
    ArmSetLowVectors();
  }

  return RETURN_SUCCESS;
}

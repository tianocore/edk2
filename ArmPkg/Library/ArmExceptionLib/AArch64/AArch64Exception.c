/** @file
*  Exception Handling support specific for AArch64
*
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

#include <Chipset/AArch64.h>
#include <Library/DebugLib.h>

UINTN gMaxExceptionNumber = MAX_AARCH64_EXCEPTION;

EFI_EXCEPTION_CALLBACK  gExceptionHandlers[MAX_AARCH64_EXCEPTION + 1];
EFI_EXCEPTION_CALLBACK  gDebuggerExceptionHandlers[MAX_AARCH64_EXCEPTION + 1];

RETURN_STATUS InstallExceptionHandlers(VOID) {
  // all AArch64 impelmentations have VBAR so this should never get called
  ASSERT(FALSE);
  return RETURN_UNSUPPORTED;
}

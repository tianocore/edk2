/** @file
*  Main file supporting the SEC Phase on ARM PLatforms
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
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

#ifndef __SEC_H__
#define __SEC_H__

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/ArmCpuLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/ArmPlatformSecLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#define IS_ALIGNED(Address, Align) (((UINTN)Address & (Align-1)) == 0)

VOID
TrustedWorldInitialization (
  IN  UINTN                     MpId,
  IN  UINTN                     SecBootMode
  );

VOID
NonTrustedWorldTransition (
  IN  UINTN                     MpId,
  IN  UINTN                     JumpAddress
  );

VOID
ArmSetupGicNonSecure (
  IN  INTN                  GicDistributorBase,
  IN  INTN                  GicInterruptInterfaceBase
);

VOID
enter_monitor_mode (
  IN UINTN                  MonitorEntryPoint,
  IN UINTN                  MpId,
  IN UINTN                  SecBootMode,
  IN VOID*                  MonitorStackBase
  );

VOID
return_from_exception (
  IN UINTN                  NonSecureBase
  );

VOID
copy_cpsr_into_spsr (
  VOID
  );

VOID
set_non_secure_mode (
  IN ARM_PROCESSOR_MODE     Mode
  );

VOID
SecCommonExceptionEntry (
  IN UINT32 Entry,
  IN UINTN  LR
  );

VOID
EFIAPI
ArmSecArchTrustzoneInit (
  VOID
  );

#endif

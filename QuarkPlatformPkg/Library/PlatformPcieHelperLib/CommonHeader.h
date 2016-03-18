/** @file
Common header file shared by all source files in this component.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __COMMON_HEADER_H_
#define __COMMON_HEADER_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/QNCAccessLib.h>
#include <Library/IntelQNCLib.h>
#include <IntelQNCRegs.h>
#include <IntelQNCConfig.h>
#include <Pcal9555.h>
#include <Platform.h>
#include <PlatformBoards.h>

#include <Library/PlatformPcieHelperLib.h>

//
// Routines shared between souce modules in this component.
//

VOID
EFIAPI
PlatformPcieErratas (
  VOID
  );

EFI_STATUS
EFIAPI
SocUnitEarlyInitialisation (
  VOID
  );

EFI_STATUS
EFIAPI
SocUnitReleasePcieControllerPreWaitPllLock (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  );

EFI_STATUS
EFIAPI
SocUnitReleasePcieControllerPostPllLock (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  );

#endif

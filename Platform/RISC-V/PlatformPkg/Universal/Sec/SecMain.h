/** @file
  RISC-V SEC phase module definitions..

  Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SECMAIN_H_
#define SECMAIN_H_

#include <PiPei.h>
#include <Library/PeimEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/DebugLib.h>
#include <Library/Edk2OpensbiPlatformWrapperLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/RiscVCpuLib.h>

int
SecPostOpenSbiPlatformEarlylInit(
  IN BOOLEAN ColdBoot
  );

int
SecPostOpenSbiPlatformFinalInit (
  IN BOOLEAN ColdBoot
  );

VOID
SecMachineModeTrapHandler (
  IN VOID
  );

VOID
EFIAPI
SecStartupPhase2 (
  IN VOID                     *Context
  );

#endif // _SECMAIN_H_

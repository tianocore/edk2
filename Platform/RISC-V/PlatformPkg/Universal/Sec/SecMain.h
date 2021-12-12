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
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/RiscVCpuLib.h>
#include <Ppi/TemporaryRamDone.h>
#include <Ppi/TemporaryRamSupport.h>

VOID
SecMachineModeTrapHandler (
  IN VOID
  );

VOID
EFIAPI
SecStartupPhase2 (
  IN VOID                     *Context
  );

EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  );

EFI_STATUS
EFIAPI
TemporaryRamDone (
  VOID
  );

#endif // _SECMAIN_H_

/** @file
  RISC-V SEC phase module definitions..

  Copyright (c) 2019-2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

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

/**
  OpenSBI platform early init hook.

  @param[in]   ColdBoot  Is cold boot path or warm boot path.
  @retval      OpenSBI error code.

**/
INT32
SecPostOpenSbiPlatformEarlyInit (
  IN BOOLEAN  ColdBoot
  );

/**
  OpenSBI platform final init hook.
  We restore the next_arg1 to the pointer of EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT.

  @param[in]   ColdBoot  Is cold boot path or warm boot path.
  @retval      OpenSBI error code.

**/
INT32
SecPostOpenSbiPlatformFinalInit (
  IN BOOLEAN  ColdBoot
  );

/**
  SEC machine mode trap handler.

**/
VOID
SecMachineModeTrapHandler (
  IN VOID
  );

#endif // _SECMAIN_H_

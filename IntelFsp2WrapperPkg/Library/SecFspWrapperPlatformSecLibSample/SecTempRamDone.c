/** @file
  Sample to provide SecTemporaryRamDone function.

  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Ppi/TemporaryRamDone.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/FspPlatformInfoLib.h>
#include <Library/FspApiLib.h>

/**
This interface disables temporary memory in SEC Phase.
**/
VOID
EFIAPI
SecPlatformDisableTemporaryMemory (
  VOID
  )
{
  EFI_STATUS                Status;
  VOID                      *TempRamExitParam;
  FSP_INFO_HEADER           *FspHeader;

  FspHeader = FspFindFspHeader (PcdGet32(PcdFspmBaseAddress));
  if (FspHeader == NULL) {
    return ;
  }

  DEBUG((DEBUG_INFO, "SecPlatformDisableTemporaryMemory enter\n"));

  TempRamExitParam = GetTempRamExitParam ();
  Status = CallTempRamExit (FspHeader, TempRamExitParam);
  DEBUG((DEBUG_INFO, "TempRamExit status: 0x%x\n", Status));
  ASSERT_EFI_ERROR(Status);

  return ;
}

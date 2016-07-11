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

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/FspWrapperApiLib.h>
#include <Library/FspWrapperPlatformLib.h>
#include <Guid/FspHeaderFile.h>

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

  TempRamExitParam = UpdateTempRamExitParam ();
  Status = CallTempRamExit (TempRamExitParam);
  DEBUG((DEBUG_INFO, "TempRamExit status: 0x%x\n", Status));
  ASSERT_EFI_ERROR(Status);

  return ;
}

/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PEILESS_STARTUP_INTERNAL_LIB_H_
#define PEILESS_STARTUP_INTERNAL_LIB_H_

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Uefi/UefiSpec.h>
#include <Uefi/UefiBaseType.h>
#include <IndustryStandard/IntelTdx.h>

EFI_STATUS
EFIAPI
DxeLoadCore (
  IN INTN  FvInstance
  );

VOID
EFIAPI
TransferHobList (
  IN CONST VOID  *HobStart
  );

/**
 * This function is to find a memory region which is the largest one below 4GB.
 * It will be used as the firmware hoblist.
 *
 * @param VmmHobList        Vmm passed hoblist which constains the memory information.
 * @return EFI_SUCCESS      Successfully construct the firmware hoblist.
 * @return EFI_NOT_FOUND    Cannot find a memory region to be the fw hoblist.
 */
EFI_STATUS
EFIAPI
ConstructFwHobList (
  IN CONST VOID  *VmmHobList
  );

/**
 *  Construct the HobList in SEC phase.
 *
 * @return EFI_SUCCESS      Successfully construct the firmware hoblist.
 * @return EFI_NOT_FOUND    Cannot find a memory region to be the fw hoblist.
 */
EFI_STATUS
EFIAPI
ConstructSecHobList (
  );

#endif

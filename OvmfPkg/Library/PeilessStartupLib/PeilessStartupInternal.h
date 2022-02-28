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

/**
  Measure the Hoblist passed from the VMM.

  This function will create a unique GUID hob entry will be
  found from the TCG driver building the event log.
  This module will generate the measurement with the data in
  this hob, and log the event.

  @param[in] VmmHobList    The Hoblist pass the firmware

  @retval EFI_SUCCESS           Fv image is measured successfully
                                or it has been already measured.
  @retval Others                Other errors as indicated
**/
EFI_STATUS
EFIAPI
MeasureHobList (
  IN CONST VOID  *VmmHobList
  );

/**
  Check the integrity of CFV data.

  @param[in] TdxCfvBase - A pointer to CFV header
  @param[in] TdxCfvSize - CFV data size

  @retval  TRUE   - The CFV data is valid.
  @retval  FALSE  - The CFV data is invalid.

**/
BOOLEAN
EFIAPI
TdxValidateCfv (
  IN UINT8   *TdxCfvBase,
  IN UINT32  TdxCfvSize
  );

/**
  Measure FV image.
  Add it into the measured FV list after the FV is measured successfully.

  @param[in]  FvBase            Base address of FV image.
  @param[in]  FvLength          Length of FV image.
  @param[in]  PcrIndex          Index of PCR

  @retval EFI_SUCCESS           Fv image is measured successfully
                                or it has been already measured.
  @retval Others                Other errors as indicated
**/
EFI_STATUS
EFIAPI
TdxMeasureCfvImage (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength,
  IN UINT8                 PcrIndex
  );

#endif

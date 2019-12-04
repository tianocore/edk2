/** @file
  Provide FSP wrapper API related function.

  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FSP_WRAPPER_API_LIB_H__
#define __FSP_WRAPPER_API_LIB_H__

#include <FspEas.h>

/**
  Find FSP header pointer.

  @param[in] FlashFvFspBase Flash address of FSP FV.

  @return FSP header pointer.
**/
FSP_INFO_HEADER *
EFIAPI
FspFindFspHeader (
  IN EFI_PHYSICAL_ADDRESS  FlashFvFspBase
  );

/**
  Call FSP API - FspNotifyPhase.

  @param[in] NotifyPhaseParams Address pointer to the NOTIFY_PHASE_PARAMS structure.

  @return EFI status returned by FspNotifyPhase API.
**/
EFI_STATUS
EFIAPI
CallFspNotifyPhase (
  IN NOTIFY_PHASE_PARAMS *NotifyPhaseParams
  );

/**
  Call FSP API - FspMemoryInit.

  @param[in]  FspmUpdDataPtr          Pointer to the FSPM_UPD data structure.
  @param[out] HobListPtr              Pointer to receive the address of the HOB list.

  @return EFI status returned by FspMemoryInit API.
**/
EFI_STATUS
EFIAPI
CallFspMemoryInit (
  IN VOID                       *FspmUpdDataPtr,
  OUT VOID                      **HobListPtr
  );

/**
  Call FSP API - TempRamExit.

  @param[in] TempRamExitParam    Address pointer to the TempRamExit parameters structure.

  @return EFI status returned by TempRamExit API.
**/
EFI_STATUS
EFIAPI
CallTempRamExit (
  IN VOID                       *TempRamExitParam
  );

/**
  Call FSP API - FspSiliconInit.

  @param[in] FspsUpdDataPtr     Pointer to the FSPS_UPD data structure.

  @return EFI status returned by FspSiliconInit API.
**/
EFI_STATUS
EFIAPI
CallFspSiliconInit (
  IN VOID                       *FspsUpdDataPtr
  );

#endif

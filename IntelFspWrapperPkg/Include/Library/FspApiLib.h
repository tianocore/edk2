/** @file
  Provide FSP API related function.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FSP_API_LIB_H__
#define __FSP_API_LIB_H__

#include <FspApi.h>
#include <FspInfoHeader.h>

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
  Call FSP API - FspInit.

  @param[in] FspHeader     FSP header pointer.
  @param[in] FspInitParams Address pointer to the FSP_INIT_PARAMS structure.

  @return EFI status returned by FspInit API.
**/
EFI_STATUS
EFIAPI
CallFspInit (
  IN FSP_INFO_HEADER     *FspHeader,
  IN FSP_INIT_PARAMS     *FspInitParams
  );

/**
  Call FSP API - FspNotifyPhase.

  @param[in] FspHeader         FSP header pointer.
  @param[in] NotifyPhaseParams Address pointer to the NOTIFY_PHASE_PARAMS structure.

  @return EFI status returned by FspNotifyPhase API.
**/
EFI_STATUS
EFIAPI
CallFspNotifyPhase (
  IN FSP_INFO_HEADER     *FspHeader,
  IN NOTIFY_PHASE_PARAMS *NotifyPhaseParams
  );

/**
  Call FSP API - FspMemoryInit.

  @param[in]     FspHeader           FSP header pointer.
  @param[in,out] FspMemoryInitParams Address pointer to the FSP_MEMORY_INIT_PARAMS structure.

  @return EFI status returned by FspMemoryInit API.
**/
EFI_STATUS
EFIAPI
CallFspMemoryInit (
  IN FSP_INFO_HEADER            *FspHeader,
  IN OUT FSP_MEMORY_INIT_PARAMS *FspMemoryInitParams
  );

/**
  Call FSP API - TempRamExit.

  @param[in]     FspHeader           FSP header pointer.
  @param[in,out] TempRamExitParam    Address pointer to the TempRamExit parameters structure.

  @return EFI status returned by TempRamExit API.
**/
EFI_STATUS
EFIAPI
CallTempRamExit (
  IN FSP_INFO_HEADER            *FspHeader,
  IN OUT VOID                   *TempRamExitParam
  );

/**
  Call FSP API - FspSiliconInit.

  @param[in]     FspHeader           FSP header pointer.
  @param[in,out] FspSiliconInitParam Address pointer to the Silicon Init parameters structure.

  @return EFI status returned by FspSiliconInit API.
**/
EFI_STATUS
EFIAPI
CallFspSiliconInit (
  IN FSP_INFO_HEADER            *FspHeader,
  IN OUT VOID                   *FspSiliconInitParam
  );

#endif

/** @file
  Provide FSP API related function.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

  @return FSP status returned by FspInit API.
**/
FSP_STATUS
EFIAPI
CallFspInit (
  IN FSP_INFO_HEADER     *FspHeader,
  IN FSP_INIT_PARAMS     *FspInitParams
  );

/**
  Call FSP API - FspNotifyPhase.

  @param[in] FspHeader         FSP header pointer.
  @param[in] NotifyPhaseParams Address pointer to the NOTIFY_PHASE_PARAMS structure.

  @return FSP status returned by FspNotifyPhase API.
**/
FSP_STATUS
EFIAPI
CallFspNotifyPhase (
  IN FSP_INFO_HEADER     *FspHeader,
  IN NOTIFY_PHASE_PARAMS *NotifyPhaseParams
  );

#endif

/** @file
  FSP MultiPhase Library.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_MULTIPHASE_LIB_H_
#define _FSP_MULTIPHASE_LIB_H_

EFI_STATUS
EFIAPI
FspMultiPhaseSwitchStack (
  );

EFI_STATUS
EFIAPI
FspVariableRequestSwitchStack (
  IN FSP_MULTI_PHASE_VARIABLE_REQUEST_INFO_PARAMS  *FspVariableRequestParams
  );

/**
  This function handles FspMultiPhaseMemInitApi.

  @param[in]  ApiIdx           Internal index of the FSP API.
  @param[in]  ApiParam         Parameter of the FSP API.

  @retval EFI_SUCCESS                 FSP execution was successful.
  @retval EFI_INVALID_PARAMETER       Input parameters are invalid.
  @retval EFI_UNSUPPORTED             The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR            FSP initialization failed.
**/
EFI_STATUS
EFIAPI
FspMultiPhaseMemInitApiHandler (
  IN UINT32  ApiIdx,
  IN VOID    *ApiParam
  );

/**
  This function handles FspMultiPhaseSiInitApi.

  @param[in]  ApiIdx           Internal index of the FSP API.
  @param[in]  ApiParam         Parameter of the FSP API.

**/
EFI_STATUS
EFIAPI
FspMultiPhaseSiInitApiHandlerV2 (
  IN UINT32  ApiIdx,
  IN VOID    *ApiParam
  );

#endif

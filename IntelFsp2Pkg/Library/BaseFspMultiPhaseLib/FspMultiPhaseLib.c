/** @file
  FSP MultiPhase library.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/FspCommonLib.h>
#include <Library/FspSwitchStackLib.h>
#include <Library/FspSecPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <FspEas/FspApi.h>
#include <FspGlobalData.h>

EFI_STATUS
EFIAPI
FspMultiPhaseSwitchStack (
  )
{
  SetFspApiReturnStatus (EFI_SUCCESS);
  Pei2LoaderSwitchStack ();

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FspVariableRequestSwitchStack (
  IN FSP_MULTI_PHASE_VARIABLE_REQUEST_INFO_PARAMS  *FspVariableRequestParams
  )
{
  FSP_GLOBAL_DATA  *FspData;

  FspData = GetFspGlobalDataPointer ();
  if (((UINTN)FspData == 0) || ((UINTN)FspData == 0xFFFFFFFF)) {
    return EFI_UNSUPPORTED;
  }

  FspData->VariableRequestParameterPtr = (VOID *)FspVariableRequestParams;
  SetFspApiReturnStatus (FSP_STATUS_VARIABLE_REQUEST);
  Pei2LoaderSwitchStack ();

  return EFI_SUCCESS;
}

/**
  This function supports FspMultiPhase implementation.

  @param[in]  ApiIdx           Internal index of the FSP API.
  @param[in]  ApiParam         Parameter of the FSP API.

  @retval EFI_SUCCESS                 FSP execution was successful.
  @retval EFI_INVALID_PARAMETER       Input parameters are invalid.
  @retval EFI_UNSUPPORTED             The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR            FSP initialization failed.
**/
EFI_STATUS
EFIAPI
FspMultiPhaseWorker (
  IN UINT32  ApiIdx,
  IN VOID    *ApiParam
  )
{
  FSP_MULTI_PHASE_PARAMS                       *FspMultiPhaseParams;
  FSP_GLOBAL_DATA                              *FspData;
  FSP_MULTI_PHASE_GET_NUMBER_OF_PHASES_PARAMS  *FspMultiPhaseGetNumber;
  BOOLEAN                                      FspDataValid;
  UINT32                                       NumberOfPhasesSupported;

  FspDataValid = TRUE;
  FspData      = GetFspGlobalDataPointer ();
  if (((UINTN)FspData == 0) || ((UINTN)FspData == 0xFFFFFFFF)) {
    FspDataValid = FALSE;
  }

  //
  // It is required that FspData->NumberOfPhases to be reset to 0 after
  // current FSP component finished.
  // The next component FspData->NumberOfPhases will only be re-initialized when FspData->NumberOfPhases = 0
  //
  if ((FspDataValid == TRUE) && (FspData->NumberOfPhases == 0)) {
    FspData->NumberOfPhases = PcdGet32 (PcdMultiPhaseNumberOfPhases);
    FspData->PhasesExecuted = 0;
    if (FspMultiPhasePlatformGetNumberOfPhases (ApiIdx, &NumberOfPhasesSupported) == TRUE) {
      //
      // Platform has implemented runtime controlling for NumberOfPhasesSupported
      //
      FspData->NumberOfPhases = NumberOfPhasesSupported;
    }
  }

  FspMultiPhaseParams = (FSP_MULTI_PHASE_PARAMS *)ApiParam;

  if (FspDataValid == FALSE) {
    return EFI_DEVICE_ERROR;
  } else {
    switch (FspMultiPhaseParams->MultiPhaseAction) {
      case EnumMultiPhaseGetNumberOfPhases:
        if ((FspMultiPhaseParams->MultiPhaseParamPtr == NULL) || (FspMultiPhaseParams->PhaseIndex != 0)) {
          return EFI_INVALID_PARAMETER;
        }

        FspMultiPhaseGetNumber                 = (FSP_MULTI_PHASE_GET_NUMBER_OF_PHASES_PARAMS *)FspMultiPhaseParams->MultiPhaseParamPtr;
        FspMultiPhaseGetNumber->NumberOfPhases = FspData->NumberOfPhases;
        FspMultiPhaseGetNumber->PhasesExecuted = FspData->PhasesExecuted;
        break;

      case EnumMultiPhaseExecutePhase:
        if ((FspMultiPhaseParams->PhaseIndex > FspData->PhasesExecuted) && (FspMultiPhaseParams->PhaseIndex <= FspData->NumberOfPhases)) {
          FspData->PhasesExecuted = FspMultiPhaseParams->PhaseIndex;
          return Loader2PeiSwitchStack ();
        } else {
          return EFI_INVALID_PARAMETER;
        }

        break;

      case EnumMultiPhaseGetVariableRequestInfo:
        //
        // return variable request info
        //
        FspMultiPhaseParams->MultiPhaseParamPtr = FspData->VariableRequestParameterPtr;
        break;

      case EnumMultiPhaseCompleteVariableRequest:
        //
        // retrieve complete variable request params
        //
        FspData->VariableRequestParameterPtr = FspMultiPhaseParams->MultiPhaseParamPtr;
        return Loader2PeiSwitchStack ();
        break;

      default:
        return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}

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
  )
{
  return FspMultiPhaseWorker (ApiIdx, ApiParam);
}

/**
  This function handles FspMultiPhaseSiInitApi.

  @param[in]  ApiIdx           Internal index of the FSP API.
  @param[in]  ApiParam         Parameter of the FSP API.

  @retval EFI_SUCCESS                 FSP execution was successful.
  @retval EFI_INVALID_PARAMETER       Input parameters are invalid.
  @retval EFI_UNSUPPORTED             The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR            FSP initialization failed.
**/
EFI_STATUS
EFIAPI
FspMultiPhaseSiInitApiHandlerV2 (
  IN UINT32  ApiIdx,
  IN VOID    *ApiParam
  )
{
  return FspMultiPhaseWorker (ApiIdx, ApiParam);
}

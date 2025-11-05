/** @file
  Null instance of Platform Sec Lib.

  Copyright (c) 2014 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/FspCommonLib.h>

/**
  This function check the signature of UPD.

  @param[in]  ApiIdx           Internal index of the FSP API.
  @param[in]  ApiParam         Parameter of the FSP API.

**/
EFI_STATUS
EFIAPI
FspUpdSignatureCheck (
  IN UINT32  ApiIdx,
  IN VOID    *ApiParam
  )
{
  return EFI_SUCCESS;
}

/**
  This function handles FspMultiPhaseSiInitApi.
  Starting from FSP 2.4 this function is obsolete and FspMultiPhaseSiInitApiHandlerV2 is the replacement.

  @param[in]  ApiIdx           Internal index of the FSP API.
  @param[in]  ApiParam         Parameter of the FSP API.

**/
EFI_STATUS
EFIAPI
FspMultiPhaseSiInitApiHandler (
  IN UINT32  ApiIdx,
  IN VOID    *ApiParam
  )
{
  return EFI_SUCCESS;
}

/**
  FSP MultiPhase Platform Get Number Of Phases Function.

  Allows an FSP binary to dynamically update the number of phases at runtime.
  For example, UPD settings could negate the need to enter the multi-phase flow
  in certain scenarios. If this function returns FALSE, the default number of phases
  provided by PcdMultiPhaseNumberOfPhases will be returned to the bootloader instead.

  @param[in] ApiIdx                  - Internal index of the FSP API.
  @param[in] NumberOfPhasesSupported - How many phases are supported by current FSP Component.

  @retval  TRUE  - NumberOfPhases are modified by Platform during runtime.
  @retval  FALSE - The Default build time NumberOfPhases should be used.

**/
BOOLEAN
EFIAPI
FspMultiPhasePlatformGetNumberOfPhases (
  IN     UINT8   ApiIdx,
  IN OUT UINT32  *NumberOfPhasesSupported
  )
{
  /* Example for platform runtime controlling
  if ((ApiIdx == FspMultiPhaseSiInitApiIndex) && (Feature1Enable == FALSE)) {
    *NumberOfPhasesSupported = 0;
    return TRUE;
  }
  return FALSE
  */

  return FALSE;
}

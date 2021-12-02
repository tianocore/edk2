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

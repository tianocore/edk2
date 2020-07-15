/** @file
  Provide FSP wrapper API test related function.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

/**
  Test the output of FSP API - FspMemoryInit.

  @param[in]  FspmUpdDataPtr Address pointer to the FSP_MEMORY_INIT_PARAMS structure.
  @param[in]  HobListPtr     Address of the HobList pointer.

  @return test result on output of FspMemoryInit API.
**/
EFI_STATUS
EFIAPI
TestFspMemoryInitApiOutput (
  IN  VOID        *FspmUpdDataPtr,
  IN  VOID        **HobListPtr
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  Test the output of FSP API - TempRamExit.

  @param[in] TempRamExitParam    Address pointer to the TempRamExit parameters structure.

  @return test result on output of TempRamExit API.
**/
EFI_STATUS
EFIAPI
TestFspTempRamExitApiOutput (
  IN VOID         *TempRamExitParam
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  Test the output of FSP API - FspSiliconInit.

  @param[in] FspsUpdDataPtr Address pointer to the Silicon Init parameters structure.

  @return test result on output of FspSiliconInit API.
**/
EFI_STATUS
EFIAPI
TestFspSiliconInitApiOutput (
  IN  VOID        *FspsUpdDataPtr
  )
{
  return RETURN_UNSUPPORTED;
}

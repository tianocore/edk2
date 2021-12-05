/** @file
  Provide FSP wrapper platform related function.

  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FSP_WRAPPER_PLATFORM_LIB_H__
#define __FSP_WRAPPER_PLATFORM_LIB_H__

/**
  This function overrides the default configurations in the FSP-M UPD data region.

  @param[in,out] FspUpdRgnPtr   A pointer to the UPD data region data structure.

**/
VOID
EFIAPI
UpdateFspmUpdData (
  IN OUT VOID  *FspUpdRgnPtr
  );

/**
  This function overrides the default configurations in the FSP-S UPD data region.

  @param[in,out] FspUpdRgnPtr   A pointer to the UPD data region data structure.

**/
VOID
EFIAPI
UpdateFspsUpdData (
  IN OUT VOID  *FspUpdRgnPtr
  );

/**
  Update TempRamExit parameter.

  @note At this point, memory is ready, PeiServices are available to use.

  @return TempRamExit parameter.
**/
VOID *
EFIAPI
UpdateTempRamExitParam (
  VOID
  );

/**
  Get S3 PEI memory information.

  @note At this point, memory is ready, and PeiServices are available to use.
  Platform can get some data from SMRAM directly.

  @param[out] S3PeiMemSize  PEI memory size to be installed in S3 phase.
  @param[out] S3PeiMemBase  PEI memory base to be installed in S3 phase.

  @return If S3 PEI memory information is got successfully.
**/
EFI_STATUS
EFIAPI
GetS3MemoryInfo (
  OUT UINT64                *S3PeiMemSize,
  OUT EFI_PHYSICAL_ADDRESS  *S3PeiMemBase
  );

/**
  Perform platform related reset in FSP wrapper.

  This function will reset the system with requested ResetType.

  @param[in] FspStatusResetType  The type of reset the platform has to perform.
**/
VOID
EFIAPI
CallFspWrapperResetSystem (
  IN UINT32  FspStatusResetType
  );

#endif

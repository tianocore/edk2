/** @file
  Provide FSP platform information related function.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FSP_PLATFORM_INFO_LIB_H__
#define __FSP_PLATFORM_INFO_LIB_H__

/**
  Get current boot mode.

  @note At this point, memory is ready, PeiServices are NOT available to use.
  Platform can get some data from chipset register.

  @return BootMode current boot mode.
**/
UINT32
EFIAPI
GetBootMode (
  VOID
  );

/**
  Get NVS buffer parameter.

  @note At this point, memory is NOT ready, PeiServices are available to use.

  @return NvsBuffer NVS buffer parameter.
**/
VOID *
EFIAPI
GetNvsBuffer (
  VOID
  );

/**
  Get UPD region size.

  @note At this point, memory is NOT ready, PeiServices are available to use.

  @return UPD region size.
**/
UINT32
EFIAPI
GetUpdRegionSize (
  VOID
  );

/**
  This function overrides the default configurations in the UPD data region.

  @param[in,out] FspUpdRgnPtr   A pointer to the UPD data region data strcture.

  @return  FspUpdRgnPtr   A pointer to the UPD data region data strcture.
**/
VOID *
EFIAPI
UpdateFspUpdConfigs (
  IN OUT VOID        *FspUpdRgnPtr
  );

/**
  Get BootLoader Tolum size.

  @note At this point, memory is NOT ready, PeiServices are available to use.

  @return BootLoader Tolum size.
**/
UINT32
EFIAPI
GetBootLoaderTolumSize (
  VOID
  );

/**
  Get TempRamExit parameter.

  @note At this point, memory is ready, PeiServices are available to use.

  @return TempRamExit parameter.
**/
VOID *
EFIAPI
GetTempRamExitParam (
  VOID
  );

/**
  Get FspSiliconInit parameter.

  @note At this point, memory is ready, PeiServices are available to use.

  @return FspSiliconInit parameter.
**/
VOID *
EFIAPI
GetFspSiliconInitParam (
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
  OUT UINT64               *S3PeiMemSize,
  OUT EFI_PHYSICAL_ADDRESS *S3PeiMemBase
  );

/**
  Get stack information according to boot mode.

  @note If BootMode is BOOT_ON_S3_RESUME or BOOT_ON_FLASH_UPDATE,
  this stack should be in some reserved memory space.

  @note If FspInitDone is TRUE, memory is ready, but no PeiServices there.
  Platform can get some data from SMRAM directly.
  @note If FspInitDone is FALSE, memory is NOT ready, but PeiServices are available to use.
  Platform can get some data from variable via VariablePpi.

  @param[in]  BootMode     Current boot mode.
  @param[in]  FspInitDone  If FspInit is called.
  @param[out] StackSize    Stack size to be used in PEI phase.
  @param[out] StackBase    Stack base to be used in PEI phase.

  @return If Stack information is got successfully.
**/
EFI_STATUS
EFIAPI
GetStackInfo (
  IN  UINT32               BootMode,
  IN  BOOLEAN              FspInitDone,
  OUT UINT64               *StackSize,
  OUT EFI_PHYSICAL_ADDRESS *StackBase
  );

#endif

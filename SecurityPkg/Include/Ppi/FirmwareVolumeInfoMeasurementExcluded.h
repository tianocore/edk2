/** @file
  This PPI means a FV does not need to be extended to PCR by TCG modules.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_H__
#define __EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_H__

#define EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_PPI_GUID \
 { 0x6e056ff9, 0xc695, 0x4364, { 0x9e, 0x2c, 0x61, 0x26, 0xf5, 0xce, 0xea, 0xae } }

typedef struct {
  EFI_PHYSICAL_ADDRESS    FvBase;
  UINT64                  FvLength;
} EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_FV;

//
// This PPI means a FV does not need to be extended to PCR by TCG modules.
//
typedef struct {
  UINT32                                                  Count;
  EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_FV    Fv[1];
} EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_PPI;

extern EFI_GUID  gEfiPeiFirmwareVolumeInfoMeasurementExcludedPpiGuid;

#endif

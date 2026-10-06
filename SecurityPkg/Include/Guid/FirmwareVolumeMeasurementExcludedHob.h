/** @file
  Defines the HOB GUID used to pass all excluded FVs to the DXE Driver.
  This allows the DXE phase measurement code to have access to the same information
  provided in PEI using EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_PPI.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

extern EFI_GUID  gEdkiiFirmwareVolumeMeasurementExcludedHobGuid;

typedef struct {
  UINT32                                                  NumberOfEntries;
  EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_FV    ExcludedFvs[];
} EDKII_FIRMWARE_VOLUME_MEASUREMENT_EXCLUDED_HOB_DATA;

/** @file
  This library is used by FSP modules to measure data to TPM.

Copyright (c) 2020, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_MEASUREMENT_LIB_H_
#define _FSP_MEASUREMENT_LIB_H_

#define FSP_MEASURE_FSP     BIT0
#define FSP_MEASURE_FSPT    BIT1
#define FSP_MEASURE_FSPM    BIT2
#define FSP_MEASURE_FSPS    BIT3
#define FSP_MEASURE_FSPUPD  BIT31

/**
  Measure a FSP FirmwareBlob.

  @param[in]  PcrIndex                PCR Index.
  @param[in]  Description             Description for this FirmwareBlob.
  @param[in]  FirmwareBlobBase        Base address of this FirmwareBlob.
  @param[in]  FirmwareBlobLength      Size in bytes of this FirmwareBlob.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_UNSUPPORTED       TPM device not available.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.
*/
EFI_STATUS
EFIAPI
MeasureFspFirmwareBlob (
  IN UINT32                PcrIndex,
  IN CHAR8                 *Description OPTIONAL,
  IN EFI_PHYSICAL_ADDRESS  FirmwareBlobBase,
  IN UINT64                FirmwareBlobLength
  );

#endif

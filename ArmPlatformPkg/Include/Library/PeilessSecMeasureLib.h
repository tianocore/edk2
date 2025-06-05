/** @file

  Copyright (c) 2025, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PEILESS_SEC_MEASURE_LIB_H_
#define PEILESS_SEC_MEASURE_LIB_H_

/**
  Measurement for PeilessSec.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid firmware volume information.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
MeasurePeilessSec (
  VOID
  );

#endif /* PEILESS_SEC_MEASURE_LIB_H_ */

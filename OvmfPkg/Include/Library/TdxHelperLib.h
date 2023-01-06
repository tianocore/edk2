/** @file
  TdxHelperLib header file

  Copyright (c) 2021 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TDX_HELPER_LIB_H
#define TDX_HELPER_LIB_H

#include <PiPei.h>

/**
  In Tdx guest, some information need to be passed from host VMM to guest
  firmware. For example, the memory resource, etc. These information are
  prepared by host VMM and put in HobList which is described in TdxMetadata.

  Information in HobList is treated as external input. From the security
  perspective before it is consumed, it should be validated and measured.

  @retval   EFI_SUCCESS   Successfully process the hoblist
  @retval   Others        Other error as indicated
**/
EFI_STATUS
EFIAPI
TdxHelperProcessHobList (
  VOID
  );

/**
 * In Tdx guest, Configuration FV (CFV) is treated as external input because it
 * may contain the data provided by VMM. From the sucurity perspective Cfv image
 * should be measured before it is consumed.
 *
 * @retval EFI_SUCCESS Successfully measure the CFV image
 * @retval Others      Other error as indicated
 */
EFI_STATUS
EFIAPI
TdxHelperMeasureCfvImage (
  VOID
  );

#endif

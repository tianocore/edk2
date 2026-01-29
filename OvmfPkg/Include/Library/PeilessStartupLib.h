/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PEILESS_STARTUP_LIB_H_
#define PEILESS_STARTUP_LIB_H_

#include <Library/BaseLib.h>
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <Pi/PiPeiCis.h>
#include <Library/DebugLib.h>
#include <Protocol/DebugSupport.h>

/**
 * This function brings up the Tdx guest from SEC phase to DXE phase.
 * PEI phase is skipped because most of the components in PEI phase
 * is not needed for Tdx guest, for example, MP Services, TPM etc.
 * In this way, the attack surfaces are reduced as much as possible.
 *
 * @param Context   The pointer to the SecCoreData
 * @return VOID     This function never returns
 */
VOID
EFIAPI
PeilessStartup (
  IN VOID  *Context
  );

#endif

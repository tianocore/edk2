/** @file
  Recovery library class defines a set of methods related recovery boot mode.
  This library class is no longer used and modules using this library should
  directly locate EFI_PEI_RECOVERY_MODULE_PPI, defined in the PI 1.2 specification.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __RECOVERY_LIB_H__
#define __RECOVERY_LIB_H__

/**
  Calling this function causes the system to carry out a recovery boot path.

  @retval EFI_SUCCESS   Recovery boot path succeeded.
  @retval Others        Recovery boot path failure.

**/
EFI_STATUS
EFIAPI
PeiRecoverFirmware (
  VOID
  );

#endif



/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_BMC_READY_LIB_H_
#define PLATFORM_BMC_READY_LIB_H_

/**
  This function checks whether BMC is ready for transaction or not.

  @return TRUE  The BMC is ready.
  @return FALSE  The BMC is not ready.

**/
BOOLEAN
EFIAPI
PlatformBmcReady (
  VOID
  );

#endif /* PLATFORM_BMC_READY_LIB_H_ */

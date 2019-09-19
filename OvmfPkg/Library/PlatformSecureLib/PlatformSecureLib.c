/** @file
  Provides a platform-specific method to enable Secure Boot Custom Mode setup.

  Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/PcdLib.h>

/**

  This function provides a platform-specific method to detect whether the platform
  is operating by a physically present user.

  Programmatic changing of platform security policy (such as disable Secure Boot,
  or switch between Standard/Custom Secure Boot mode) MUST NOT be possible during
  Boot Services or after exiting EFI Boot Services. Only a physically present user
  is allowed to perform these operations.

  NOTE THAT: This function cannot depend on any EFI Variable Service since they are
  not available when this function is called in AuthenticateVariable driver.

  @retval  TRUE       The platform is operated by a physically present user.
  @retval  FALSE      The platform is NOT operated by a physically present user.

**/
BOOLEAN
EFIAPI
UserPhysicalPresent (
  VOID
  )
{
  return TRUE;
}

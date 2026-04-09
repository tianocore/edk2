/** @file
  NULL PlatformSecureLib instance does NOT really detect whether a physical present
  user exists but return TRUE directly. This instance can be used to verify security
  related features during platform enabling and development. It should be replaced
  by a platform-specific method(e.g. Button pressed) in a real platform for product.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>

BOOLEAN  mUserPhysicalPresence = FALSE;

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
  return mUserPhysicalPresence;
}

/**
  Save user physical presence state from a PCD to mUserPhysicalPresence.

  @retval  EFI_SUCCESS          PcdUserPhysicalPresence is got successfully.

**/
RETURN_STATUS
EFIAPI
PlatformSecureLibNullConstructor (
  VOID
  )
{
  mUserPhysicalPresence = PcdGetBool (PcdUserPhysicalPresence);

  return RETURN_SUCCESS;
}

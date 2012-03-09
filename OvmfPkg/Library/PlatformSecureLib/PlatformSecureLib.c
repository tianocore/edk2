/** @file
  Provides a platform-specific method to enable Secure Boot Custom Mode setup.

  Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Library/PcdLib.h>

/**

  This function detects whether a secure platform-specific method to clear PK(Platform Key)
  is configured by platform owner. This method is provided for users force to clear PK
  in case incorrect enrollment mis-haps.

  UEFI231 spec chapter 27.5.2 stipulates: The platform key may also be cleared using
  a secure platform-specific method. In  this case, the global variable SetupMode
  must also be updated to 1.

  NOTE THAT: This function cannot depend on any EFI Variable Service since they are
  not available when this function is called in AuthenticateVariable driver.

  @retval  TRUE       The Platform owner wants to force clear PK.
  @retval  FALSE      The Platform owner doesn't want to force clear PK.

**/
BOOLEAN
EFIAPI
ForceClearPK (
  VOID
  )
{
    return TRUE;
}

/**

  This function detects whether current platform is operated by a physical present user.

  @retval  TRUE       The Platform is operated by a physical present user.
  @retval  FALSE      The Platform is NOT operated by a physical persent user.

**/
BOOLEAN
EFIAPI
UserPhysicalPresent (
  VOID
  )
{
  return TRUE;
}

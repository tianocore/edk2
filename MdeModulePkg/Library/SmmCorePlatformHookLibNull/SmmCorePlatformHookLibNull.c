/** @file
  Null instance of SmmCorePlatformHookLibNull.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/SmmCorePlatformHookLib.h>

/**
  Performs platform specific tasks before invoking registered SMI handlers.
  
  This function performs platform specific tasks before invoking registered SMI handlers.
  
  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The paltform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
PlatformHookBeforeSmmDispatch (
  VOID
  )
{
  return EFI_SUCCESS;
}


/**
  Performs platform specific tasks after invoking registered SMI handlers.
  
  This function performs platform specific tasks after invoking registered SMI handlers.
  
  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The paltform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
PlatformHookAfterSmmDispatch (
  VOID
  )
{
  return EFI_SUCCESS;
}

/** @file
  The library class provides platform variable cleanup services.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PLATFORM_VARIABLE_CLEANUP_LIB_
#define _PLATFORM_VARIABLE_CLEANUP_LIB_

#include <Guid/VarErrorFlag.h>

typedef enum {
  VarCleanupAll,
  VarCleanupManually,
  VarCleanupMax,
} VAR_CLEANUP_TYPE;

/**
  Get last boot variable error flag.

  @return   Last boot variable error flag.

**/
VAR_ERROR_FLAG
EFIAPI
GetLastBootVarErrorFlag (
  );

/**
  Platform variable cleanup.

  @param[in] Flag                   Variable error flag.
  @param[in] Type                   Variable cleanup type.
                                    If it is VarCleanupManually, the interface must be called after console connected.

  @retval EFI_SUCCESS               No error or error processed.
  @retval EFI_UNSUPPORTED           The specified Flag or Type is not supported.
                                    For example, system error may be not supported to process and Platform should have mechanism to reset system to manufacture mode.
                                    Another, if system and user variables are wanted to be distinguished to process, the interface must be called after EndOfDxe.
  @retval EFI_OUT_OF_RESOURCES      Not enough resource to process the error.
  @retval EFI_INVALID_PARAMETER     The specified Flag or Type is an invalid value.
  @retval Others                    Other failure occurs.

**/
EFI_STATUS
EFIAPI
PlatformVarCleanup (
  IN VAR_ERROR_FLAG     Flag,
  IN VAR_CLEANUP_TYPE   Type
  );

#endif


/** @file
  Null Recovery Library instance does nothing and returns unsupported status.

  This library instance is no longer used and module using this library
  class should update to directly locate EFI_PEI_RECOVERY_MODULE_PPI defined
  in PI 1.2 specification. 

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <PiPei.h>
#include <Library/RecoveryLib.h>

/**
  Calling this function causes the system do recovery boot path.
  
  @retval EFI_UNSUPPORTED       Recovery is not supported.
**/
EFI_STATUS
EFIAPI
PeiRecoverFirmware (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}


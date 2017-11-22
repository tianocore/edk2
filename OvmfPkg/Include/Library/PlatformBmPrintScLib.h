/** @file
  Register a status code handler for printing the Boot Manager's LoadImage()
  and StartImage() preparations, and return codes, to the UEFI console.

  This feature enables users that are not accustomed to analyzing the firmware
  log to glean some information about UEFI boot option processing (loading and
  starting).

  Copyright (C) 2019, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __PLATFORM_BM_PRINT_SC_LIB__
#define __PLATFORM_BM_PRINT_SC_LIB__

#include <Uefi/UefiBaseType.h>

/**
  Register a status code handler for printing the Boot Manager's LoadImage()
  and StartImage() preparations, and return codes, to the UEFI console.

  @retval EFI_SUCCESS  The status code handler has been successfully
                       registered.

  @return              Error codes propagated from boot services and from
                       EFI_RSC_HANDLER_PROTOCOL.
**/
EFI_STATUS
EFIAPI
PlatformBmPrintScRegisterHandler (
  VOID
  );

#endif // __PLATFORM_BM_PRINT_SC_LIB__

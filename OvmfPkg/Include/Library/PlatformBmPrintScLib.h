/** @file
  Register a status code handler for printing the Boot Manager's LoadImage()
  and StartImage() preparations, and return codes, to the UEFI console.

  This feature enables users that are not accustomed to analyzing the firmware
  log to glean some information about UEFI boot option processing (loading and
  starting).

  Copyright (C) 2019, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
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

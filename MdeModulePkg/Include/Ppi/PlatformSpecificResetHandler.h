/** @file
  This PPI provides services to register a platform specific handler for
  ResetSystem().  The registered handlers are processed after
  EDKII_PLATFORM_SPECIFIC_RESET_NOTIFICATION_PPI notifications.

  Copyright (c) 2017 - 2018 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PLATFORM_SPECIFIC_RESET_HANDLER_PPI_H_
#define _PLATFORM_SPECIFIC_RESET_HANDLER_PPI_H_

#include <Protocol/ResetNotification.h>

#define EDKII_PLATFORM_SPECIFIC_RESET_HANDLER_PPI_GUID \
  { 0x75cf14ae, 0x3441, 0x49dc, { 0xaa, 0x10, 0xbb, 0x35, 0xa7, 0xba, 0x8b, 0xab } }

typedef EFI_RESET_NOTIFICATION_PROTOCOL EDKII_PLATFORM_SPECIFIC_RESET_HANDLER_PPI;

extern EFI_GUID  gEdkiiPlatformSpecificResetHandlerPpiGuid;

#endif

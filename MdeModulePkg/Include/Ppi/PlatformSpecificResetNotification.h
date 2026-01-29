/** @file
  This PPI provides services to register a platform specific notification callback for
  ResetSystem().  The registered handlers are processed after
  EDKII_PLATFORM_SPECIFIC_RESET_FILTER_PPI notifications and before
  EDKII_PLATFORM_SPECIFIC_RESET_HANDLER_PPI notifications.

  Copyright (c) 2017 - 2018 Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017 Microsoft Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PLATFORM_SPECIFIC_RESET_NOTIFICATION_PPI_H_
#define _PLATFORM_SPECIFIC_RESET_NOTIFICATION_PPI_H_

#include <Protocol/ResetNotification.h>

#define EDKII_PLATFORM_SPECIFIC_RESET_NOTIFICATION_PPI_GUID \
  { 0xe09f355d, 0xdae8, 0x4910, { 0xb1, 0x4a, 0x92, 0x78, 0x0f, 0xdc, 0xf7, 0xcb } }

typedef EFI_RESET_NOTIFICATION_PROTOCOL EDKII_PLATFORM_SPECIFIC_RESET_NOTIFICATION_PPI;

extern EFI_GUID  gEdkiiPlatformSpecificResetNotificationPpiGuid;

#endif

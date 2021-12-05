/** @file
  This protocol provides services to register a platform specific handler for
  ResetSystem().  The registered handlers are called after the UEFI 2.7 Reset
  Notifications are processed

  Copyright (c) 2017 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PLATFORM_SPECIFIC_RESET_HANDLER_PROTOCOL_H_
#define _PLATFORM_SPECIFIC_RESET_HANDLER_PROTOCOL_H_

#include <Protocol/ResetNotification.h>

#define EDKII_PLATFORM_SPECIFIC_RESET_HANDLER_PROTOCOL_GUID \
  { 0x2df6ba0b, 0x7092, 0x440d, { 0xbd, 0x4, 0xfb, 0x9, 0x1e, 0xc3, 0xf3, 0xc1 } }

typedef EFI_RESET_NOTIFICATION_PROTOCOL EDKII_PLATFORM_SPECIFIC_RESET_HANDLER_PROTOCOL;

extern EFI_GUID  gEdkiiPlatformSpecificResetHandlerProtocolGuid;

#endif

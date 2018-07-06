/** @file
  This PPI provides services to register a platform specific handler for
  ResetSystem().  The registered handlers are processed after
  EDKII_PLATFORM_SPECIFIC_RESET_NOTIFICATION_PPI notifications.

  Copyright (c) 2017 - 2018 Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PLATFORM_SPECIFIC_RESET_HANDLER_PPI_H_
#define _PLATFORM_SPECIFIC_RESET_HANDLER_PPI_H_

#include <Protocol/ResetNotification.h>

#define EDKII_PLATFORM_SPECIFIC_RESET_HANDLER_PPI_GUID \
  { 0x75cf14ae, 0x3441, 0x49dc, { 0xaa, 0x10, 0xbb, 0x35, 0xa7, 0xba, 0x8b, 0xab } }

typedef EFI_RESET_NOTIFICATION_PROTOCOL  EDKII_PLATFORM_SPECIFIC_RESET_HANDLER_PPI;

extern EFI_GUID gEdkiiPlatformSpecificResetHandlerPpiGuid;

#endif

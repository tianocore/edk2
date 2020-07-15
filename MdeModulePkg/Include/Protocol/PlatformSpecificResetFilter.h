/** @file
  This Protocol provides services to register a platform specific reset filter
  for ResetSystem().  A reset filter evaluates the parameters passed to
  ResetSystem() and converts a ResetType of EfiResetPlatformSpecific to a
  non-platform specific reset type.  The registered filters are processed before
  the UEFI 2.7 Reset Notifications.

  Copyright (c) 2017 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PLATFORM_SPECIFIC_RESET_FILTER_PROTOCOL_H_
#define _PLATFORM_SPECIFIC_RESET_FILTER_PROTOCOL_H_

#include <Protocol/ResetNotification.h>

#define EDKII_PLATFORM_SPECIFIC_RESET_FILTER_PROTOCOL_GUID \
  { 0x695d7835, 0x8d47, 0x4c11, { 0xab, 0x22, 0xfa, 0x8a, 0xcc, 0xe7, 0xae, 0x7a } }

typedef EFI_RESET_NOTIFICATION_PROTOCOL  EDKII_PLATFORM_SPECIFIC_RESET_FILTER_PROTOCOL;

extern EFI_GUID gEdkiiPlatformSpecificResetFilterProtocolGuid;

#endif

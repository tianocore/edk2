/** @file
  This PPI provides services to register a platform specific reset filter
  for ResetSystem().  A reset filter evaluates the parameters passed to
  ResetSystem() and converts a ResetType of EfiResetPlatformSpecific to a
  non-platform specific reset type.  The registered filters are processed before
  EDKII_PLATFORM_SPECIFIC_RESET_NOTIFICATION_PPI handlers.

  Copyright (c) 2017 - 2018 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PLATFORM_SPECIFIC_RESET_FILTER_PPI_H_
#define _PLATFORM_SPECIFIC_RESET_FILTER_PPI_H_

#include <Protocol/ResetNotification.h>

#define EDKII_PLATFORM_SPECIFIC_RESET_FILTER_PPI_GUID \
  { 0x8c9f4de3, 0x7b90, 0x47ef, { 0x93, 0x8, 0x28, 0x7c, 0xec, 0xd6, 0x6d, 0xe8 } }

typedef EFI_RESET_NOTIFICATION_PROTOCOL  EDKII_PLATFORM_SPECIFIC_RESET_FILTER_PPI;

extern EFI_GUID gEdkiiPlatformSpecificResetFilterPpiGuid;

#endif

/** @file
  GUID is the name of events used with ExitBootServices in order to be notified
  when this ExitBootServices Call is failed.

  Copyright (c) 2012 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EVENT_EXIT_BOOT_FAILED_GUID_H__
#define __EVENT_EXIT_BOOT_FAILED_GUID_H__

#define EVENT_GROUP_EXIT_BOOT_SERVICES_FAILED \
  { 0x4f6c5507, 0x232f, 0x4787, { 0xb9, 0x5e, 0x72, 0xf8, 0x62, 0x49, 0xc, 0xb1 } }

extern EFI_GUID  gEventExitBootServicesFailedGuid;

#endif

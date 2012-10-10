/** @file
  GUID is the name of events used with ExitBootServices in order to be notified
  when this ExitBootServices Call is failed.

  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EVENT_EXIT_BOOT_FAILED_GUID_H__
#define __EVENT_EXIT_BOOT_FAILED_GUID_H__
                                             
#define EVENT_GROUP_EXIT_BOOT_SERVICES_FAILED \
  { 0x4f6c5507, 0x232f, 0x4787, { 0xb9, 0x5e, 0x72, 0xf8, 0x62, 0x49, 0xc, 0xb1 } }

extern EFI_GUID gEventExitBootServicesFailedGuid;

#endif

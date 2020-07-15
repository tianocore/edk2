/** @file
  GUID is the name of events used with CreateEventEx in order to be notified
  when the DXE Core is idle.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __IDLE_LOOP_EVENT_GUID_H__
#define __IDLE_LOOP_EVENT_GUID_H__

#define IDLE_LOOP_EVENT_GUID \
   { 0x3c8d294c, 0x5fc3, 0x4451, { 0xbb, 0x31, 0xc4, 0xc0, 0x32, 0x29, 0x5e, 0x6c } }

extern EFI_GUID gIdleLoopEventGuid;

#endif

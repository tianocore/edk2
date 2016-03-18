/** @file
  GUID is the name of events used with CreateEventEx in order to be notified
  when the DXE Core is idle.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __IDLE_LOOP_EVENT_GUID_H__
#define __IDLE_LOOP_EVENT_GUID_H__

#define IDLE_LOOP_EVENT_GUID \
   { 0x3c8d294c, 0x5fc3, 0x4451, { 0xbb, 0x31, 0xc4, 0xc0, 0x32, 0x29, 0x5e, 0x6c } }

extern EFI_GUID gIdleLoopEventGuid;

#endif

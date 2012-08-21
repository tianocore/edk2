/** @file
  GUID for an event that is signaled on the first attempt to check for a keystroke 
  from the ConIn device.

  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __CONNECT_CONIN_EVENT_GUID_H__
#define __CONNECT_CONIN_EVENT_GUID_H__

#define CONNECT_CONIN_EVENT_GUID \
    { 0xdb4e8151, 0x57ed, 0x4bed, { 0x88, 0x33, 0x67, 0x51, 0xb5, 0xd1, 0xa8, 0xd7 }}

extern EFI_GUID gConnectConInEventGuid;

#endif

/** @file
  This file declares Firmware Volume Dispatch protocol.

  This protocol will be installed into FV Handle if the drivers in this Firmware Volume 
  are required to be dispatched by the core dispatcher.

  Copyright (c) 2006 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FIRMWARE_VOLUME_DISPATCH_H__
#define __FIRMWARE_VOLUME_DISPATCH_H__

#define EFI_FIRMWARE_VOLUME_DISPATCH_PROTOCOL_GUID \
  { 0x7aa35a69, 0x506c, 0x444f, {0xa7, 0xaf, 0x69, 0x4b, 0xf5, 0x6f, 0x71, 0xc8 } }


extern EFI_GUID gEfiFirmwareVolumeDispatchProtocolGuid;

#endif

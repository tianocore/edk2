/** @file
  Protocol is used to help implement DebugSupport.RegisterPeriodicCallback() functionality.
  This enables the DXE timer driver to support the periodic callback function so the
  DebugSupport driver does not need to contain platform specific information about how a timer
  works.

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef __DEBUG_SUPPORT_PERIODIC_CALLBACK_H__
#define __DEBUG_SUPPORT_PERIODIC_CALLBACK_H__

#include <Protocol/DebugSupport.h>

typedef struct _EFI_DEBUG_SUPPORT_PERIODIC_CALLBACK_PROTOCOL EFI_DEBUG_SUPPORT_PERIODIC_CALLBACK_PROTOCOL;


// {9546E07C-2CBB-4c88-986C-CD341086F044}
#define EFI_DEBUG_SUPPORT_PERIODIC_CALLBACK_PROTOCOL_GUID \
  { 0x9546e07c, 0x2cbb, 0x4c88, { 0x98, 0x6c, 0xcd, 0x34, 0x10, 0x86, 0xf0, 0x44 } }


//
// DebugSupport protocol definition
//
struct _EFI_DEBUG_SUPPORT_PERIODIC_CALLBACK_PROTOCOL {
  EFI_PERIODIC_CALLBACK  PeriodicCallback;
};

extern EFI_GUID gEfiDebugSupportPeriodicCallbackProtocolGuid;

#endif

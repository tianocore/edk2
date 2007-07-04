/** @file
  This file defines the data structures per HOB specification v0.9.

  Copyright (c) 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  FrameworkFirmwareFileSystem.h

  @par Revision Reference:
  These definitions are from HOB Spec 0.9 but not adopted by PI specs.

**/

#ifndef _FRAMEWORK_HOB_H_
#define _FRAMEWORK_HOB_H_

#include <PiPei.h>

//
// Capsule volume HOB -- identical to a firmware volume
//
#define EFI_HOB_TYPE_CV 0x0008

typedef struct {
  EFI_HOB_GENERIC_HEADER  Header;
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  UINT64                  Length;
} EFI_HOB_CAPSULE_VOLUME;

#endif

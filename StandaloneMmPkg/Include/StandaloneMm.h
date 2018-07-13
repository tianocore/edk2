/** @file
  Standalone MM.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _STANDALONE_MM_H_
#define _STANDALONE_MM_H_

#include <PiMm.h>

typedef
EFI_STATUS
(EFIAPI *MM_IMAGE_ENTRY_POINT) (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_MM_SYSTEM_TABLE   *MmSystemTable
  );

typedef
EFI_STATUS
(EFIAPI *STANDALONE_MM_FOUNDATION_ENTRY_POINT) (
  IN VOID  *HobStart
  );

#endif

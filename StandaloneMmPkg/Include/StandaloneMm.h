/** @file
  Standalone MM.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

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

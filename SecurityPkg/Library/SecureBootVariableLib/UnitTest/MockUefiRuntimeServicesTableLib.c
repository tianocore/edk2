/** @file
  Mock implementation of the UEFI Runtime Services Table Library.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

extern EFI_RUNTIME_SERVICES  gMockRuntime;

EFI_RUNTIME_SERVICES  *gRT = &gMockRuntime;

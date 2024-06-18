/** @file
  MM Services Table Library.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include <Library/MmServicesTableLib.h>
#include <Library/DebugLib.h>

EFI_MM_SYSTEM_TABLE  *gMmst = NULL;

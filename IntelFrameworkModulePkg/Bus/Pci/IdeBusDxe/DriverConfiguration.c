/** @file
  Implementation of UEFI Driver Configuration Protocol for IDE bus driver which
  provides ability to set IDE bus controller specific options.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "IdeBus.h"

CHAR16 *OptionString[4] = {
  L"Enable Primary Master    (Y/N)? -->",
  L"Enable Primary Slave     (Y/N)? -->",
  L"Enable Secondary Master  (Y/N)? -->",
  L"Enable Secondary Slave   (Y/N)? -->"
};

//
// EFI Driver Configuration Protocol
//




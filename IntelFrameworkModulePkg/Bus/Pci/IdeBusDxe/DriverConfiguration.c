/** @file
  Implementation of UEFI Driver Configuration Protocol for IDE bus driver which
  provides ability to set IDE bus controller specific options.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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




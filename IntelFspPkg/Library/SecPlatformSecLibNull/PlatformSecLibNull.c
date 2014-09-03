/** @file
  Null instance of Platform Sec Lib.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

/**
  This function provides dummy function so that SecCore can pass build
  validation in IntelFspPkg. All real platform library instances needs
  to implement the real entry point in assembly.
**/
VOID
EFIAPI
_ModuleEntryPoint (
  VOID
  )
{
  return;
}

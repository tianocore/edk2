/** @file
  Include file for internal functions of PEI Services table pointer libary.

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  PeiServicesTablePointerLibInternals.h

**/

#ifndef __PEI_SERVICES_TABLE_POINTER_LIB_INTERTALS_H__
#define __PEI_SERVICES_TABLE_POINTER_LIB_INTERTALS_H__

/**
  Reads the current value of Kr1.

  @return The current value of Kr1.

**/
UINT64
EFIAPI
AsmReadKr1 (
  VOID
  );

/**
  Writes the current value of Kr1.

  @param  Value The 64-bit value to write to Kr1.

**/
VOID
EFIAPI
AsmWriteKr1 (
  IN      UINT64                    Value
  );

#endif

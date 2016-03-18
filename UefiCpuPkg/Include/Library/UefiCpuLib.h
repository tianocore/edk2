/** @file
  Public header file for UEFI CPU library class.

  This library class defines some routines that are generic for IA32 family CPU
  to be UEFI specification compliant.

  Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __UEFI_CPU_LIB_H__
#define __UEFI_CPU_LIB_H__



/**
  Initializes floating point units for requirement of UEFI specification.

  This function initializes floating-point control word to 0x027F (all exceptions
  masked,double-precision, round-to-nearest) and multimedia-extensions control word
  (if supported) to 0x1F80 (all exceptions masked, round-to-nearest, flush to zero
  for masked underflow).

**/
VOID
EFIAPI
InitializeFloatingPointUnits (
  VOID
  );

#endif

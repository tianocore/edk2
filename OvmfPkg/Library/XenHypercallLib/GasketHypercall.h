/** @file

  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _GASKET_HYPERCALL_H_
#define _GASKET_HYPERCALL_H_

UINTN
EFIAPI
Hypercall (
  IN  VOID   *HypercallPageAddress,  // Address of hypercall page
  IN  UINTN  HypercallOffset,        // Hypercall Offset ( = hypercall num * 32)
  IN  UINTN  Arg1,                   // parameters
  IN  UINTN  Arg2,                   //
  IN  UINTN  Arg3,                   //
  IN  UINTN  Arg4,                   //
  IN  UINTN  Arg5                    //
  );


//
// Hypercall2 is not used!
//
UINTN
EFIAPI
Hypercall2 (
  IN  VOID   *HypercallPageAddress,  // Address of hypercall page
  IN  UINTN  HypercallOffset,        // Hypercall Offset ( = hypercall num * 32)
  IN  UINTN  Arg1,                   // parameters
  IN  UINTN  Arg2                    //
  );

#endif

